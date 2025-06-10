..\mingw32\bin\windres TrayMp3.rc -o TrayMp3_res.o
..\mingw32\bin\gcc -ffunction-sections -fdata-sections -s -o TrayMp3 TrayMp3.c TrayMp3_res.o -mwindows -lwinmm -lcomctl32 -lshlwapi -lshell32 -lole32 -lpropsys -lmf -lmfplat -lmfreadwrite -Wl,--gc-sections -static-libgcc

