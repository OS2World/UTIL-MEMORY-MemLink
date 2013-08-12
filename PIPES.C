/*mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm
/* PIPES.C
/*
/* This module provides a mechanism to pass data between threads of the same
/* process or between processes.  It was written as a replacement for a 
/* a named pipe implementation of interprocess communications in an 
/* application with 4 communicating processes.  The named pipe implementation
/* was poorly written, caused lock-up problems, and proved to be more
/* complicated than needed.  This type of implementation seemed cleaner and
/* simpler to implement.
/* 
/* The following IPC mechanism is more efficient and does not involve OS/2 
/* named pipes.  It uses shared memory in a DLL, with memory access and flow 
/* control implemented with semaphores. 
/*
/* Although the name "pipe" is used in this file, this term is not referring 
/* to any OS/2 pipes.  This implementation can be thought of as a homemade 
/* piping mechanism using shared memory in a DLL and semaphores to 
/* synchronize access to the memory.  The module is self-initializing with
/* semaphores created and opened automatically with the first calls to
/* the following exported routines.  The following 3 routines are the only
/* routines that need to be called outside of this module. This file is the
/* only C file used to create MEMLINK.DLL.  
/*
/* To use these 3 routines you should include pipes.h in the files that call 
/* the routines.  The structure "message" must also be defined for this module. 
/* "message" is the data structure that is passed in the pipes between the
/* threads or processes using these pipes. 
/*
/* HOW TO USE BETWEEN PROCESSES (OR THREADS) "p1" AND "p2":
/*
/* 1) Pick a free pipe Id (i.e. PIPE_BOB, PIPE_MARY, ... defined in pipes.h).
/*    In our example we'll use the first one - PIPE_BOB.
/*
/* 2) Designate one of the processes as PIPE_END_SERVER and the other PIPE_END_CLIENT.
/*    It doesn't matter which one is which.  For this example we'll use 
/*                      p1 = PIPE_END_SERVER
/*                      p2 = PIPE_END_CLIENT
/*
/* 3) When p1 WRITES a "MyMessage" structure to p2 it makes a 
/*    call like the following:     PipeWrite(PIPE_BOB, PIPE_END_SERVER, &MyMessage); 
/*
/* 4) When p1 READS a "MyMessage" structure from p2 it makes a 
/*    call like the following:     PipeRead(PIPE_BOB, PIPE_END_SERVER, &MyMessage); 
/*
/* 5) When p2 WRITES a "MyMessage" structure to p1 it makes a 
/*    call like the following:     PipeWrite(PIPE_BOB, PIPE_END_CLIENT, &MyMessage); 
/*
/* 6) When p2 READS a "MyMessage" structure from p1 it makes a 
/*    call like the following:     PipeRead(PIPE_BOB, PIPE_END_CLIENT, &MyMessage); 
/*
/*  Note: Each write will block until the data is read from the last write. 
/*        Each read will block until data is available to be read ( unless you
/*        use PipeReadNoWait(), which returns without waiting for avail data).
/*        This blocking provides flow control of the messages between the p's.
/*
/* HOW TO INCORPORATE INTO YOUR APP(S): 
/*       1) Define your "struct message" data structure in pipes.h.
/*       2) Recompile to create MEMLINK.LIB & MEMLINK.DLL:  (i.e. nmake memlink.mak)
/*       3) Link MEMLINK.LIB into your executable(s) and make sure MEMLINK.DLL
/*           is available to them during run-time.
/*
/* EXPORTED ROUTINES:
/*	int EXPENTRY PipeRead(int PipeId, int PipeEnd, struct message *Msg)
/*	int EXPENTRY PipeReadNoWait(int PipeId, int PipeEnd, 
/*		struct message *Msg, int *DataReturnedFlag)
/*	int EXPENTRY PipeWrite(int PipeId, int PipeEnd, struct message *Msg)
/*
/* NOTE All functions return 0 if no error, else a nonzero value.
/*mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm*/

#define INCL_DOSSEMAPHORES

#include <os2.h>
#include <stdlib.h>
#include "pipes.h"

/* Semaphore API errors */
#ifndef ERROR_TIMEOUT
	#define NO_ERROR 0
	#define ERROR_INVALID_HANDLE 6
	#define ERROR_NOT_ENOUGH_MEMORY 8
	#define ERROR_INVALID_PARAMETER 87
	#define ERROR_INTERRUPT 95
	#define ERROR_TOO_MANY_SEM_REQUESTS 103
	#define ERROR_SEM_OWNER_DIED 105
	#define ERROR_INVALID_NAME 123
	#define ERROR_SEM_NOT_FOUND 187
	#define ERROR_NOT_OWNER 288
	#define ERROR_TOO_MANY_OPENS 291
	#define ERROR_TOO_MANY_POSTS 298
	#define ERROR_ALREADY_POSTED 299
	#define ERROR_ALREADY_RESET 300
	#define ERROR_TIMEOUT 640
#endif

// semaphore name used locally in this module
#define PINITMUTEXSEMNAME "\\SEM32\\PINITMTX.SEM"

// prototypes of locally called routines.
static int CreatePipeSemaphores(int PipeId);
static APIRET PipeQueryEventSem(HEV *SemHandle, ULONG *ulPostCt);
static APIRET PipeWaitAndResetEventSem(HEV *SemHandle);
static APIRET PipePostEventSem(HEV *SemHandle);
static APIRET RequestPipeInitMutexSem(void);
static APIRET ReleasePipeInitMutexSem(void);

// The following data is shared among all users of the DLL.
#pragma data_seg(SharedSeg1)	/* start of shared data segment */
  static HEV WriteEventSem[MAX_NUM_PIPES][2], ReadEventSem[MAX_NUM_PIPES][2];
  static struct message PipeData[MAX_NUM_PIPES][2];
  static short PipeInitFlag[MAX_NUM_PIPES];
  static HMTX hPInitMtxSem;
#pragma data_seg()	/* end of shared data segment */

/**************************************************************************
/* PipeRead	Performs a read from one of the "pipes".  This routine blocks 
/*		until a message is available to be read (when the other end
/*		of the pipe writes to the pipe).
/* arguments:
/*		PipeId - The id assigned to this pipe (1, 2, ...).
/*		PipeEnd - PIPE_END_SERVER or PIPE_END_CLIENT.
/*		Msg - Pointer to buffer for returned data.
/* returns:
/* 		0 if no error, else a nonzero value.
/**************************************************************************/
int EXPENTRY PipeRead(int PipeId, int PipeEnd, struct message *Msg)
{
	int ReturnVal=0, OtherEnd, PipeIndex;

	PipeIndex = PipeId - 1;	       /* Ids start with 1, indices with 0 */
	OtherEnd = (PipeEnd) ? 0 : 1;  /* Get Id of other side of pipe */

	/* Check for valid arguments */
	if ((PipeIndex < 0) || (PipeIndex >= MAX_NUM_PIPES) ||  
		(PipeEnd < 0) || (PipeEnd > 1)) return -1;

	/* Is this the first call?  If so then initialize the semaphores. */
	else if (!PipeInitFlag[PipeIndex]) 
	{
		ReturnVal = CreatePipeSemaphores(PipeId);
		if (ReturnVal) return ReturnVal;
	}

	/* wait till a message has been written by opposite side. */
	ReturnVal |= PipeWaitAndResetEventSem(&WriteEventSem[PipeIndex][OtherEnd]); 

	/* Get data from pipe. */
	*Msg = PipeData[PipeIndex][PipeEnd];
	
	/* Indicate data has been read and pipe can be written to again. */
	ReturnVal |= PipePostEventSem(&ReadEventSem[PipeIndex][PipeEnd]);

	return ReturnVal;
}

/**************************************************************************
/* PipeReadNoWait Performs a read from one of the "pipes".  This routine does
/*		not block if a message is not available to be read.  If data
/*		is available to be read the data is returned in "Msg" and
/*		"DataReturnedFlag" is set to 1, else "DataReturnedFlag" is
/*		set to 0 and there is no valid data returned in "Msg".
/*
/* arguments:
/*		PipeId - The id assigned to this pipe.(1, 2, ...).
/*		PipeEnd - PIPE_END_SERVER or PIPE_END_CLIENT.
/*		Msg - Pointer to buffer for returned data.
/*		DataReturnedFlag - Flag indicating if data is returned from
/*			the pipe read.
/*
/* returns:
/* 		0 if no error, else a nonzero value.
/**************************************************************************/
int EXPENTRY PipeReadNoWait(int PipeId, int PipeEnd, 
		struct message *Msg, int *DataReturnedFlag)
{
	ULONG SemPostCt;
	int ReturnVal=0, OtherEnd, PipeIndex;

	PipeIndex = PipeId - 1;	       /* Ids start with 1, indices with 0 */
	OtherEnd = (PipeEnd) ? 0 : 1;  /* Get Id of other side of pipe */

	/* Check for valid arguments */
	if ((PipeIndex < 0) || (PipeIndex >= MAX_NUM_PIPES) ||  
		(PipeEnd < 0) || (PipeEnd > 1)) return -1;

	/* Is this the first call?  If so then initialize the semaphores. */
	else if (!PipeInitFlag[PipeIndex]) 
	{
		ReturnVal = CreatePipeSemaphores(PipeId);
		if (ReturnVal) return ReturnVal;
	}

	/* check (without waiting) if Write Event is posted. */
	ReturnVal |= PipeQueryEventSem(&WriteEventSem[PipeIndex][OtherEnd], &SemPostCt);

	/* If error or no data in pipe then do quick exit. */
	if (ReturnVal || (SemPostCt == 0))
	{
		*DataReturnedFlag = 0;
		return 0;
	}

	/* check if Write Event is posted (data is available in pipe).  */
	ReturnVal |= PipeWaitAndResetEventSem(&WriteEventSem[PipeIndex][OtherEnd]); 

	/* Get data from pipe. */
	*Msg = PipeData[PipeIndex][PipeEnd];
	
	/* Indicate data has been read and pipe can be written to again. */
	ReturnVal |= PipePostEventSem(&ReadEventSem[PipeIndex][PipeEnd]);

	*DataReturnedFlag = 1;

	return ReturnVal;
}

/**************************************************************************
/* PipeWrite	Performs a write to one of the "pipes".  This routine blocks 
/*		until room is available in the pipe for the write. 
/*
/* arguments:
/*		PipeId - The id assigned to this pipe.(1, 2, ...).
/*		PipeEnd - PIPE_END_SERVER or PIPE_END_CLIENT.
/*		Msg - Pointer to data being written to the pipe.
/* returns:
/* 		0 if no error, else a nonzero value.
/**************************************************************************/
int EXPENTRY PipeWrite(int PipeId, int PipeEnd, struct message *Msg)
{
	ULONG SemPostCt;
	int ReturnVal=0, OtherEnd, PipeIndex;

	PipeIndex = PipeId - 1;		/* Ids start with 1, indices with 0 */
	OtherEnd = (PipeEnd) ? 0 : 1;   /* Get Id of other side of pipe */

	/* Check for valid arguments */
	if ((PipeIndex < 0) || (PipeIndex >= MAX_NUM_PIPES) ||
		(PipeEnd < 0) || (PipeEnd > 1)) return -1;

	/* Is this the first call?  If so then initialize the semaphores. */
	else if (!PipeInitFlag[PipeIndex]) 
	{
		ReturnVal = CreatePipeSemaphores(PipeId);
		if (ReturnVal) return ReturnVal;
	}

	/* wait till old message has been read by other side of pipe */
	ReturnVal |= PipeWaitAndResetEventSem(&ReadEventSem[PipeIndex][OtherEnd]); 

	/* Write data to pipe. */
	PipeData[PipeIndex][OtherEnd] = *Msg;
	
	/* Let other thread know data has been written and can be read.  */
	ReturnVal |= PipePostEventSem(&WriteEventSem[PipeIndex][PipeEnd]);

	return ReturnVal;
}


/*mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm*/
/*mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm*/
/* The rest of the routines in this module are used locally by routines in   */
/* this module and are not meant to be called from outside this module.      */
/*mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm*/
/*mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm*/

/**************************************************************************
/* CreatePipeSemaphores Performs creation of event semaphores used to 
/*			synchronize access to a pipe. 
/**************************************************************************/
static int CreatePipeSemaphores(int PipeId)
{
	int ReturnVal=0, PipeIndex;
	ULONG flattr=0;

	RequestPipeInitMutexSem();	/* block further access to routine */

	PipeIndex = PipeId - 1;		/* Ids start with 1, indices with 0 */

	if ((PipeIndex < 0) || (PipeIndex >= MAX_NUM_PIPES)) ReturnVal = -1;
	else if (PipeInitFlag[PipeIndex]) ReturnVal = 0;
	else
	{
	   PipeInitFlag[PipeIndex] = 1;

	   flattr |= DC_SEM_SHARED;

	   ReturnVal |= DosCreateEventSem(0, 
				&WriteEventSem[PipeIndex][0], flattr, FALSE);
	   ReturnVal |= DosCreateEventSem(0, 
				&ReadEventSem[PipeIndex][0], flattr, TRUE);
	   ReturnVal |= DosCreateEventSem(0, 
				&WriteEventSem[PipeIndex][1], flattr, FALSE);
	   ReturnVal |= DosCreateEventSem(0, 
				&ReadEventSem[PipeIndex][1], flattr, TRUE);
	}

	ReleasePipeInitMutexSem();	/* allow further access to routine */

	return ReturnVal;
}

/**************************************************************************
/* RequestPipeInitMutexSem  Performs Request for the mutex semaphore that
/*			protects access to the "init" function. 
/**************************************************************************/
static APIRET RequestPipeInitMutexSem(void)
{
#define ERRMTX_TIMEOUT 3000		/* milliseconds */

	APIRET rc;

	rc = DosRequestMutexSem(hPInitMtxSem, ERRMTX_TIMEOUT);

	if (rc == ERROR_INVALID_HANDLE)
	{
		hPInitMtxSem = 0;

		rc = DosOpenMutexSem(PINITMUTEXSEMNAME, &hPInitMtxSem);

		if ((rc == ERROR_SEM_NOT_FOUND) || (rc == ERROR_INVALID_HANDLE))
			rc = DosCreateMutexSem(PINITMUTEXSEMNAME, &hPInitMtxSem, 0, 0);

		if (!rc) 
			rc = DosRequestMutexSem(hPInitMtxSem, ERRMTX_TIMEOUT);
	}

	return rc;
}

/**************************************************************************
/* ReleasePipeInitMutexSem  Performs release of the mutex semaphore that
/*			protects access to the "init" function. 
/**************************************************************************/
static APIRET ReleasePipeInitMutexSem(void)
{
	APIRET rc;

	rc = DosReleaseMutexSem(hPInitMtxSem);

	if (rc == ERROR_INVALID_HANDLE)
	{
		hPInitMtxSem = 0;

		rc = DosOpenMutexSem(PINITMUTEXSEMNAME, &hPInitMtxSem);

		if ((rc == ERROR_SEM_NOT_FOUND) || (rc == ERROR_INVALID_HANDLE))
			rc = DosCreateMutexSem(PINITMUTEXSEMNAME, &hPInitMtxSem, 0, 0);

		if (!rc) 
			rc = DosReleaseMutexSem(hPInitMtxSem);
	}

	return rc;
}

/***************************************************************************
/* PipeQueryEventSem	Performs query of an event semaphore to determine
/*			how many times it has been posted since the last
/*			reset.  If it is 0 then a thread would block if 
/*			requesting this semaphore,until the sem is posted.
/*			This routine returns the post count in "ulPostCt".
/**************************************************************************/
static APIRET PipeQueryEventSem(HEV *SemHandle, ULONG *ulPostCt)
{
	APIRET rc;

	rc = DosQueryEventSem(*SemHandle, ulPostCt);

	if (rc == ERROR_INVALID_HANDLE)
	{
		rc = DosOpenEventSem(0, SemHandle);

		if (!rc) 
		    rc = DosQueryEventSem(*SemHandle, ulPostCt);
	}

	return rc;
}

/***************************************************************************
/* PipeWaitandResetEventSem  Performs wait for a PipeWrite() to happen before
/*			returning, to put a block on PipeRead() until a 
/*			message is in the queue.  After the wait finishes the
/*			semaphore is immediately reset again for next read. 
/***************************************************************************/
static APIRET PipeWaitAndResetEventSem(HEV *SemHandle)
{
#define EVENTSEM_TIMEOUT SEM_INDEFINITE_WAIT

	APIRET rc;
	ULONG PostCt;

	rc = DosWaitEventSem(*SemHandle, EVENTSEM_TIMEOUT);
	if (!rc) rc = DosResetEventSem(*SemHandle, &PostCt);

	if (rc == ERROR_INVALID_HANDLE)
	{
		rc = DosOpenEventSem(0, SemHandle);

		if (!rc) 
		{
		    rc = DosWaitEventSem(*SemHandle, EVENTSEM_TIMEOUT);
		    if (!rc) rc = DosResetEventSem(*SemHandle, &PostCt);
		}
	}

        if (rc == ERROR_ALREADY_RESET) rc = 0;

	return rc;
}

/***************************************************************************
/* PipePostEventSem 	Performs Post of the event semaphore that 
/*			allows PipeRead() to go ahead and read from the queue.
/***************************************************************************/
static APIRET PipePostEventSem(HEV *SemHandle)
{
	APIRET rc;

	rc = DosPostEventSem(*SemHandle);

	if (rc)
	{
	     if (rc == ERROR_INVALID_HANDLE)
	     {

		rc = DosOpenEventSem(0, SemHandle);

		if (!rc) 
			rc = DosPostEventSem(*SemHandle);
	     }
	     else if (rc == ERROR_ALREADY_POSTED) 
		rc = 0;
	}

	return rc;
}

