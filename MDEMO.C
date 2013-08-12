/*************************************************************************
* 
*  Filename:             mdemo.c
*                  
*  Description:          
*			This module is meant to be a demo program to show
*                       how the MEMLINK.DLL library works for Interprocess
*                       Communication. 
*************************************************************************/

#define INCL_GPI
#define INCL_WIN
#define INCL_DOS
#include <os2.h>
#include <stdlib.h>
#include "mdemo.h"
#include "pipes.h"

#define THREAD_STACK_SIZE	8000

/* function prototypes */
static VOID APIENTRY thread1_routine(ULONG DummyValue);
static VOID APIENTRY thread2_routine(ULONG DummyValue);
static void HandleOutput(char *MsgString);
static MRESULT EXPENTRY ClientWindowProc(HWND window, ULONG message,
                                          MPARAM param1, MPARAM param2 );
/* global variables */
static HAB anchor_block;
static char *pszMainWindowClass = "MEMLINK DEMO PROGRAM";
static HWND  client_window;
static HWND  main_window;
static TID thread1_id, thread2_id;
static short slowflag;

/*mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm
*  Description: This function is the main routine for the application.
/*mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm*/
int main( void )
{

   HMQ   message_queue = NULLHANDLE;        
   ULONG frame_flags   = 0UL;          
   QMSG  message;                 
   int   return_value  = 1;

   do
     {
     if (( anchor_block = WinInitialize(0UL)) == NULLHANDLE )
       break;

     if (( message_queue = WinCreateMsgQueue( anchor_block, 0UL )) == 
          NULLHANDLE )
       break;

     if ( !WinRegisterClass( anchor_block, pszMainWindowClass, 
            (PFNWP) ClientWindowProc, CS_SIZEREDRAW, 0UL ))       
		       break;

      /* Create the main window */
      frame_flags = FCF_STANDARD & ~FCF_TASKLIST;
      main_window =  WinCreateStdWindow( HWND_DESKTOP,  
                                         WS_VISIBLE,    
                                         &frame_flags,     
                                         pszMainWindowClass,  
                                         "",            
                                         CS_SIZEREDRAW | WS_VISIBLE,
                                         0UL,           
                                         ID_WINDOW,     
                                         &client_window );
      if ( main_window == NULLHANDLE )
        break;

      WinSetWindowText(main_window, pszMainWindowClass);

      while( WinGetMsg( anchor_block, &message, 0UL, 0UL, 0UL ) )
        WinDispatchMsg( anchor_block, &message );  
      return_value = 0;

   }  while ( FALSE );

   if ( main_window != NULLHANDLE )
     WinDestroyWindow( main_window ); 

   if ( message_queue != NULLHANDLE )
      WinDestroyMsgQueue( message_queue );

   if ( anchor_block != NULLHANDLE )
     WinTerminate( anchor_block );

   return return_value;
}


/*mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm
*                                                                       
*  Description: This function is the window procedure for the client
*               window.
*                                                                       
/*mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm*/
MRESULT EXPENTRY ClientWindowProc( HWND window, ULONG message, 
                                   MPARAM param1, MPARAM param2 )
{
  APIRET	rc;
  CONVCONTEXT   context;
  RECTL         update_rect;
  HPS           present_space;
  HWND		ClientWnd, ServerWnd;
  int           i;
  static short  TimerFlag = 0;
  short		MenuPick;


  switch( message )
    {
    case WM_CLOSE:
      WinPostMsg( window, WM_QUIT, NULL, NULL );
      break;

    case WM_PAINT:
      present_space = WinBeginPaint(window, NULLHANDLE, (PRECTL)&update_rect);
      WinFillRect( present_space, (PRECTL)&update_rect, SYSCLR_WINDOW);
      WinEndPaint( present_space );
      break;

    case WM_COMMAND:
      MenuPick = SHORT1FROMMP(param1);
      switch (MenuPick)
         {
         case IDM_GOSLOW:
         case IDM_GOFAST:
              slowflag = (MenuPick == IDM_GOSLOW) ? 1 : 0;

	      if (thread1_id || thread2_id) break;

	      HandleOutput("Starting Thread1."); 
              rc = DosCreateThread(&thread1_id, thread1_routine, 0L, 0L, THREAD_STACK_SIZE);
              if (rc != 0) 
              { 
			DosBeep(4000,500); 
			HandleOutput("Error in DosCreateThread()!  Could'nt start thread1." );
	      }

              rc = DosCreateThread(&thread2_id, thread2_routine, 0L, 0L, THREAD_STACK_SIZE);
              if (rc != 0) 
              { 
			DosBeep(4000,500); 
			HandleOutput("Error in DosCreateThread()!  Could'nt start thread2.");
	      }

	      break;

         case IDM_STOP:
             if (thread1_id) DosKillThread(thread1_id);
             if (thread2_id) DosKillThread(thread2_id);
             thread1_id = 0;
             thread2_id = 0;
	     break;

         case IDM_EXITPROG:
             WinPostMsg( window, WM_CLOSE, NULL, NULL );
             break;
         }
      break;

    default:
      return ( WinDefWindowProc( window, message, param1, param2 ) );
    }

  return NULL;
}

/*mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm
// thread1_routine	Routine that writes to the MEMLINK.DLL pipe used
//			to communicate synchronously between thread1 (this 
//                      thread) and thread2.  Thread1 writes the data
//			to the MEMLINK pipe and thread2 reads it.
//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm*/
static VOID APIENTRY thread1_routine(ULONG DummyValue)
{
    struct message m;
    static long counter = 0;
    char TempBuff[100];

    while (1)
    {
        counter++;
        sprintf(m.Msg, "MESSAGE %d", counter);
        sprintf(TempBuff, "thread1: sent %s", m.Msg); 
	HandleOutput(TempBuff);
	PipeWrite(PIPE_BOB, PIPE_END_SERVER, &m);
        if (slowflag) DosSleep(1000);
    }
}

/*mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm
// thread2_routine	Routine that reads from the MEMLINK.DLL pipe used
//			to communicate synchronously between thread1 and 
//			thread2 (this thread).  Thread1 writes the data
//			to the MEMLINK pipe and thread2 reads it.
//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm*/
static VOID APIENTRY thread2_routine(ULONG DummyValue)
{
    struct message m;
    char TempBuff[100];

    while (1)
    {
	PipeRead(PIPE_BOB, PIPE_END_CLIENT, &m);
        sprintf(TempBuff, "thread2: received %s", m.Msg);
	HandleOutput(TempBuff);
    }
}

/*mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm
// HandleOutput		Displays debug messages in test application window.
//			Needs global variable "client_window" to be 
//			initialized - the handle to the applications
//			client window.
//mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm*/
static void HandleOutput(char *MsgString)
{
	RECTL Rectl;
	POINTL Pointl;
	FONTMETRICS FontMetrics;
	static hPS = NULLHANDLE;
	static int cyChar;
	static int cxChar;
	static int Top;
	SIZEL  sizel;
	LONG   i, Delay = 1;
	float  f;

	WinQueryWindowRect(client_window, (PRECTL) &Rectl);

	if (hPS == NULLHANDLE)
	{
		sizel.cx = sizel.cy = 0;
		hPS = GpiCreatePS(anchor_block, WinOpenWindowDC(client_window),
			(PSIZEL) &sizel, PU_PELS | GPIF_DEFAULT |
			GPIT_MICRO | GPIA_ASSOC);
		GpiSetCharMode(hPS, CM_DEFAULT);
		GpiSetCharSet(hPS, LCID_DEFAULT);
		GpiQueryFontMetrics(hPS, sizeof(FONTMETRICS),
			(PFONTMETRICS) &FontMetrics);
		cyChar = FontMetrics.lMaxBaselineExt;
		cxChar = FontMetrics.lAveCharWidth;
		Top = Rectl.yTop - cyChar;
	}

	/* set text position */
	if (Top < Rectl.yBottom) 
	{
		Top = Rectl.yTop - cyChar;
		WinInvalidateRect(client_window, 0, 0);
	}
		
	Pointl.x = 5; 
	Pointl.y = Top;	
	Top -= cyChar;
	
	GpiCharStringAt(hPS, &Pointl, strlen(MsgString), MsgString);
}

