# IBM Developer's Workframe/2 Make File Creation run at 08:58:32 on 02/02/94

# Make File Creation run in directory:
#   C:\MEMLINK

.SUFFIXES:

.SUFFIXES: .c .cpp .cxx .rc

ALL: MDEMO.EXE \
     MDEMO.RES

MDEMO.EXE:  \
  MDEMO.OBJ \
  MDEMO.RES \
  MDEMO.MAK
   ICC.EXE @<<
 /B" /pmtype:pm /noe"
 /Fe"MDEMO.EXE" MEMLINK.LIB 
MDEMO.OBJ
<<
   RC MDEMO.RES MDEMO.EXE

{.}.rc.res:
   RC -r .\$*.RC

{.}.c.obj:
   ICC.EXE /Sp1 /Ss /C   .\$*.c

{.}.cpp.obj:
   ICC.EXE /Sp1 /Ss /C   .\$*.cpp

{.}.cxx.obj:
   ICC.EXE /Sp1 /Ss /C   .\$*.cxx

!include MDEMO.DEP
