libvmm
==========

portable memory manager (Virtual address space level)

This is proof of concept, maybe I'll develop it better if it works ....

libvmm was developed for to patch current luajit-2.0.3 and its 1GB ram problem on 64bit platforms.
My solution is portable (windows / linux / MacOSx) it support 2GB - 1 page of ram

Special thanks to Stefan Hett for its MacOSx port

I used this approach:

- Reserve as much ram as possible VirtualAlloc/VirtualFree and company calls used in windows, and corresponding approach in linux (http://blog.nervus.org/managing-virtual-address-spaces-with-mmap). 
- Commit / Decommit of ram pages when luajit ask for ram

Libvmm use page labeling in pages arena in order to be as fast as possible, I used bit_array ported form code of Isaac Turner (url: https://github.com/noporpoise/BitArray/) I adapted it and ported it to windows just what I need. 
Using this method I have also full support for mmremap on windows.

Platforms: 

linux / windows

Build and Install
=================

in folder luajit-modify there are 2 files what you need copy to sources 
in order to patch and build it (I tested it a bit, and it function 
but im in early project stage, so dont blame me if something is not working ...)

I work with luajit managed by luadist great project.

Install:
 
1. Build libvmm using CMake, it depends on boost ( maby ill remove it with time, 
   im used to use boost for anything, well I'll see ....)
2. clone luajit from https://github.com/LuaDist/luajit.git
3. comy my files into sources of it
4. build it.
5. test it with test/L.lua (I took it from somewhere in the net, its mindless but it sucks allot of ram :)

If you need help contact me: ladislav.sopko@gmail.com

Contributing
============

Please feel free to submit issues and pull requests. I appreciate bug reports.


License
=======

This software is in the *Public Domain*. That means you can do whatever you like
with it. That includes being used in proprietary products without attribution or
restrictions. There are no warranties and there may be bugs. 

Formally we are using CC0 - a Creative Commons license to place this work in the
public domain. A copy of CC0 is in the LICENSE file. 

    "CC0 is a public domain dedication from Creative Commons. A work released
    under CC0 is dedicated to the public domain to the fullest extent permitted
    by law. If that is not possible for any reason, CC0 also provides a lax,
    permissive license as a fallback. Both public domain works and the lax
    license provided by CC0 are compatible with the GNU GPL."
      - http://www.gnu.org/licenses/license-list.html#CC0
