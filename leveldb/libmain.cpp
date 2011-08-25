

#include "stdafx.h"

#include "leveldb/db.h"
#include "port/port.h"
#include "util/env_win32.h"

#include <iostream>

#if defined DLL_BUILD

BOOL WINAPI DllMain(HINSTANCE hinstDLL,DWORD fdwReason,LPVOID lpvReserved)
{
    switch(fdwReason){
    case DLL_PROCESS_ATTACH :
        DisableThreadLibraryCalls(hinstDLL);
        break;
    case DLL_THREAD_ATTACH :
        break;
    case DLL_THREAD_DETACH :
        break;
    case DLL_PROCESS_DETACH :
        {
            //std::cout << "dll Detached ! " << std::endl;
        }
        break;
    default:
        break;
    }
    return TRUE;
}


#endif