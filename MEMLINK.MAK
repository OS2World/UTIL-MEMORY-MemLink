# IBM Developer's Workframe/2 Make File Creation run at 11:23:48 on 11/17/94

# Make File Creation run in directory:
#   C:\MEMLINK;

! IF "$(DEBUG)" == "YES"
C_OPTION = /Tdc /Sp1 /Ss /Q /Ti /G4 /Ge- /C /Wpro   .\$*.c
L_OPTION = "/de /nologo /noe"
!ELSE
C_OPTION = /Tdc /Sp1 /Ss /Q /Ge- /C /Wpro   .\$*.c
L_OPTION = "/nologo /noe"
!ENDIF

.SUFFIXES:

.SUFFIXES: .c .cpp .cxx

ALL: MEMLINK.DLL \

MEMLINK.DLL:  \
  PIPES.OBJ \
  MEMLINK.MAK
   ICC.EXE @<<
 /B$(L_OPTION)
 /Fe"MEMLINK.DLL" STRINGS.LIB MEMLINK.DEF 
PIPES.OBJ 
<<
  IMPLIB MEMLINK.LIB MEMLINK.DLL

{.}.c.obj:
   ICC.EXE $(C_OPTION)

{.}.cpp.obj:
   ICC.EXE /Tdc /Sp1 /Ss /Q /Ge- /C /Wpro   .\$*.cpp

{.}.cxx.obj:
   ICC.EXE /Tdc /Sp1 /Ss /Q /Ge- /C /Wpro   .\$*.cxx

!include MEMLINK.DEP
