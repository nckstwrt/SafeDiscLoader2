#include "version.h"

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        
        //DisableThreadLibraryCalls(hModule);
        //CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Load, hModule, NULL, NULL);
        
        Load(hModule);
        break;
    case DLL_PROCESS_DETACH:
        // FreeLibrary(version_dll);
        break;
    }
    return TRUE;
}
