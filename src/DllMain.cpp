#include <Windows.h>
#include <ZenGin/zGothicAPI.h>

BOOL WINAPI DllMain(HMODULE hModule, DWORD fdwReason, LPVOID lpvReserved) {
    if (fdwReason == DLL_PROCESS_ATTACH) {
        switch (GetGameVersion()) {
#ifdef __G1
            case Engine_G1: break;
#endif
#ifdef __G1A
            case Engine_G1A: break;
#endif
#ifdef __G2
            case Engine_G2: break;
#endif
#ifdef __G2A
            case Engine_G2A: break;
#endif
            default: return FALSE;
        }
    }
    return TRUE;
}
