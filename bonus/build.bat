@REM gcc -O2 gui.c -o gui.exe -luser32 -lgdi32 -Wl,-subsystem,console
tcc gui.c -Wl,-subsystem=console