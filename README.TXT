				MEMLINK.DLL

MEMLINK.DLL is an OS/2 Dll that provides an alternative means for IPC 
(interprocess communication) between two concurrently running OS/2 threads. The
threads can be in the same process, or in two different processes.  It was 
written to replace the named pipe mechanism of passing user-defined data
structures between 4 concurrently running processes.  The application was
acquired from an outside source and the named pipe code was poorly written,
with initialization, termination, and the stuff in-between coded poorly.  This
caused the application to lock-up in various situations and the pipes to "break"
on occasion. 

A simpler, cleaner, method to pass this data was desired, hence
MEMLINK.  MEMLINK uses shared memory in a .DLL to pass the data between
the tasks.  The data is passed very much like in named pipes, where 
synchronization of the reading and writing of data is crucial.  When a thread 
attempts a write of data to another thread, the thread is blocked on the 
write until the other thread has read a previously written data item.  When a 
thread wants to read data from the other thread it is blocked until data is 
available to be read.  The synchronization is performed using semaphores.  
Initialization and use of the semphores is done automatically by the .DLL and 
the programmer does not have to deal with them.  There are only 3 simple 
routines in MEMLINK called from outside the .DLL.

The result of using MEMLINK was worth the time of writing it.  The mechanism
proved to be much simpler and cleaner.  After a month of heavy use the 
mechanism has yet to hickup.  It allowed us to get rid of the named pipe code 
from the application, making for a more reliable and easier to understand 
mechanism of IPC between the 4 processes.

The implementation of MEMLINK is pretty straight forward.  Look at the header
of the source file PIPES.C for more details and how to use it in your own
application.  The source code for the quick-and-dirty demo program may also be 
helpful.  The demo program MDEMO simply starts up 2 threads that pass data
between them using MEMLINK.  This is a very simple implementation with only a
single process and multiple threads.  MEMLINK can be used between threads in
the same process or threads in different processes.  

INCLUDED FILES:

README.TXT         This file.
PIPES.C            The only "C" file used to make MEMLINK.DLL
PIPES.H            The header file needed to make and use MEMLINK.DLL 
MEMLINK.MAK        The make file used to create MEMLINK.DLL
MEMLINK.DEP        The dependency file used to create MEMLINK.DLL
MEMLINK.DEF        The definition file used to export MEMLINK.DLL routines.
MEMLINK.DLL        A ready-made version of MEMLINK.DLL (made for MDEMO).
MEMLINK.LIB        .LIB of MEMLINK.DLL that gets linked into MDEMO. 
MDEMO.*            All the files used to make up the MDEMO demo program.

All files included here is freeware.  Do whatever you want with it.  I claim
no responsibilities or warranties with it's use.

If you have any questions or comments feel free to e-mail me at the following 
address.  If you find it useful I'd be interested to know how it was used.

Jeff Orth
74632.2324@COMPUSERVE.COM
