#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

#if !defined(__linux__)

#include <windows.h>

#define _DLLMAIN_C

extern "C" {
    BOOL APIENTRY DllMain( HMODULE hModule,
                           DWORD  ul_reason_for_call,
                           LPVOID lpReserved
                         ){
        switch (ul_reason_for_call){
            case DLL_PROCESS_ATTACH:
            case DLL_THREAD_ATTACH:
            case DLL_THREAD_DETACH:
            case DLL_PROCESS_DETACH:
                break;
        }
        return TRUE;
    }
} // extern "C"

#endif
