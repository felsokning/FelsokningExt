// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include "helpers.h"

#define APPNAME _T("FelsokningExt")
#pragma warning(disable : 4996) // _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

//VCR003 Warnings - https://developercommunity.visualstudio.com/t/Information-message-VCR003-given-for-ext/10729403

using namespace std;

/// <summary>
///     The DebugExtensionUninitialize callback function is called by the engine to uninitialize the DbgEng extension DLL before it is unloaded.
/// </summary>
/// <param name=""></param>
/// <returns></returns>
VOID CALLBACK DebugExtensionUninitialize(VOID)
{
    return;
}

/// <summary>
///     The DebugExtensionInitialize callback function is called by the engine after loading a DbgEng extension DLL.
/// </summary>
/// <param name="Version">Receives the version of the extension. 
/// <para>The high 16 bits contain the major version number, and the low 16 bits contain the minor version number.</para></param>
/// <param name="Flags">Set this to zero. 
/// <para>(Reserved for future use.)</para></param>
/// <returns>Whether the extension was successfully initialized. </returns>
extern "C" HRESULT CALLBACK DebugExtensionInitialize(_Out_ PULONG Version, _Out_ PULONG Flags)
{
    *Version = DEBUG_EXTENSION_VERSION(0, 9);
    *Flags = 0;
    return S_OK;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
        // The DLL is being loaded into the virtual address space of the current process as a result of the process starting up or as a result of a call to LoadLibrary. 
        // DLLs can use this opportunity to initialize any instance data or to use the TlsAlloc function to allocate a thread local storage (TLS) index.
        // The lpvReserved parameter indicates whether the DLL is being loaded statically or dynamically.
        case DLL_PROCESS_ATTACH:
            break;
        // The DLL is being unloaded from the virtual address space of the calling process because it was loaded unsuccessfully or the reference count has reached zero (the processes has either terminated or called FreeLibrary one time for each time it called LoadLibrary).
        // The lpvReserved parameter indicates whether the DLL is being unloaded as a result of a FreeLibrary call, a failure to load, or process termination.
        // The DLL can use this opportunity to call the TlsFree function to free any TLS indices allocated by using TlsAlloc and to free any thread local data.
        // Note that the thread that receives the DLL_PROCESS_DETACH notification is not necessarily the same thread that received the DLL_PROCESS_ATTACH notification.
        case DLL_PROCESS_DETACH:
            break;
        // The current process is creating a new thread.When this occurs, the system calls the entry - point function of all DLLs currently attached to the process.
        // The call is made in the context of the new thread. DLLs can use this opportunity to initialize a TLS slot for the thread.
        // A thread calling the DLL entry - point function with DLL_PROCESS_ATTACH does not call the DLL entry - point function with DLL_THREAD_ATTACH.
        // Note that a DLL's entry-point function is called with this value only by threads created after the DLL is loaded by the process. 
        // When a DLL is loaded using LoadLibrary, existing threads do not call the entry-point function of the newly loaded DLL.
        case DLL_THREAD_ATTACH:
            break;
        // A thread is exiting cleanly.
        // If the DLL has stored a pointer to allocated memory in a TLS slot, it should use this opportunity to free the memory.
        // The system calls the entry-point function of all currently loaded DLLs with this value.
        // The call is made in the context of the exiting thread.
        case DLL_THREAD_DETACH:
            break;
        default:
            break;
    }

    return TRUE;
}