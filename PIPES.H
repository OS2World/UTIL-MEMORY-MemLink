#ifndef PIPES_H
#define PIPES_H
//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm
// PIPES.H	Header file for PIPES.c in MEMLINK.DLL
//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm

// Data structure passed between processes/threads.
// Define your own message struct here
struct message
{
   long count;
   char MsgBuffer[100];
};

// define max number of pipes, and an name for each one
#define MAX_NUM_PIPES 	5
#define PIPE_BOB 	1
#define PIPE_MARY	2
#define PIPE_TOM 	3
#define PIPE_DICK	4
#define PIPE_HARRY	5

// Pipe End definitions
#define PIPE_END_SERVER	0
#define PIPE_END_CLIENT	1

// Exported routines from PIPES.C  
int EXPENTRY PipeRead(int PipeId, int PipeEnd, struct message *Msg);
int EXPENTRY PipeWrite(int PipeId, int PipeEnd, struct message *Msg);
int EXPENTRY PipeReadNoWait(int PipeId, int PipeEnd, 
		struct message *Msg, int *DataReturnedFlag);

#endif
