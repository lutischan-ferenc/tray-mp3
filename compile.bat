..\mingw32\bin\windres TrayMp3.rc -o TrayMp3_res.o
..\mingw32\bin\gcc -Os -s -ffunction-sections -fdata-sections -fno-exceptions -Wl,--gc-sections -Wl,--strip-all -mwindows -I. -o TrayMp3 TrayMp3.c TrayMp3_res.o -lkernel32 -luser32 -lgdi32 -lwinmm -lcomctl32 -lshlwapi -lshell32 -lole32 -lpropsys -lmf -lmfplat -lmfreadwrite -static-libgcc
