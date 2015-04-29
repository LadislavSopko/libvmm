# libvmm
portable memory manager (Virtual address space level)

This is proof of concept, maby I'll develop it better if it function ....

libvmm was developed for patch luajit-2.0.3 for its 1GB ram problem on 64bit

linux / windows

in folder luajit-modify there are 2 files what you need copy to sources 
in order to patch and build it (I tested it a bit, and it function 
but im in early project stage, so dont blame me if somthing is not working ...)

I work with luajit managed by luadist great project.

Install:
 
1: Build libvmm using CMake, it depends on boost ( maby ill remove it with time, 
   im used to use boost for anything, well I'll see ....)
2: clone luajit from https://github.com/LuaDist/luajit.git
3: comy my files into sources of it
4: build it.
5: test it with test/L.lua (I took it from somwher ein the net, its mindless but it sucks allot of ram :)

If you need help contact me: ladislav.sopko@gmail.com


