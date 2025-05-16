REM this should be added to the standard makefile, and in fact the whole build system should be unified

@REM gcc -O2 gui.c -o gui.exe -luser32 -lgdi32 -Wl,-subsystem,console
tcc gui.c -Wl,-subsystem=console