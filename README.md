# libvmm
portable memory manager (Virtual address space level)

This is proof of concept, maybe I'll develop it better if it works ....

libvmm was developed for patch current luajit-2.0.3 and its 1GB ram problem on 64bit.
My solution is portable (windows / linux) it support 2GB - 1 page of ram

I used this aproach:

- Reserve as much ram as possible VirtualAlloc/VirtualFree and company calls used in windows, and simulation of it in linux (http://blog.nervus.org/managing-virtual-address-spaces-with-mmap). 
- Commit / Decommit of ram pages when luajit ask for ram

Libvmm use page labeling in pages arena. For help to do it fast I use bit_array ported form code of Isaac Turner (url: https://github.com/noporpoise/BitArray/) I adapt it and port to windows just what I need. 
Using this method I have also full supporto for mmremap on windows.

Platforms: 

linux / windows

Build and Install:

in folder luajit-modify there are 2 files what you need copy to sources 
in order to patch and build it (I tested it a bit, and it function 
but im in early project stage, so dont blame me if somthing is not working ...)

I work with luajit managed by luadist great project.

Install:
 
1. Build libvmm using CMake, it depends on boost ( maby ill remove it with time, 
   im used to use boost for anything, well I'll see ....)
2. clone luajit from https://github.com/LuaDist/luajit.git
3. comy my files into sources of it
4. build it.
5. test it with test/L.lua (I took it from somwher ein the net, its mindless but it sucks allot of ram :)

If you need help contact me: ladislav.sopko@gmail.com


