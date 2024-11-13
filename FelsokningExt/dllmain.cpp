// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include "helpers.h"

#define APPNAME _T("FelsokningExt")
#pragma warning(disable : 4996) // _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

//VCR003 Warnings - https://developercommunity.visualstudio.com/t/Information-message-VCR003-given-for-ext/10729403

using namespace std;

const char seekUsage[] = "FelsokningExt by John Bailey\n\tUsage:\n\t\t* Specify '-q' (quiet) to omit the per-thread header\n\t\t* Specify '-s' to include stacks that contain 'symbol'\n\tExample:\n\t\t!seek -s hostfxr!execute_app\n\n\tTo file an issue or feature request: https://github.com/felsokning/FelsokningExt/issues/new/choose\n";

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

HRESULT CALLBACK seek(_In_ PDEBUG_CLIENT8 pDebugClient, _In_opt_ PCSTR args)
{
    auto status = S_OK;
    helpers h{};
    helpers* internalHelper = &h;
    wchar_t wideArgs[128] = { 0 };
    size_t convertedChars = 0;
    wstring target = L"";
    bool quietMode = false;

    if (SUCCEEDED(internalHelper->IsUserMode(pDebugClient)))
    {
        IDebugControl7* pDebugControl = nullptr;
        if (SUCCEEDED(pDebugClient->QueryInterface(__uuidof(IDebugControl7), (void**)&pDebugControl)))
        {
            if (SUCCEEDED(mbstowcs_s(&convertedChars, wideArgs, _countof(wideArgs), args, _TRUNCATE)))
            {
                if (convertedChars > 0)
                {
                    std::wistringstream wiss(wideArgs);
                    std::vector<std::wstring> wv(std::istream_iterator<std::wstring, wchar_t>(wiss), {});
                    size_t wvCapacity = wv.capacity();
                    if (wvCapacity == 3)
                    {
                        wstring quietCheck = wv[0];
                        if (quietCheck.find(L'q') != std::string::npos)
                        {
                            quietMode = true;
                        }
                        if (quietCheck.find(L's') == std::string::npos
                            && quietMode == false)
                        {
                            pDebugControl->Output(DEBUG_OUTPUT_NORMAL, seekUsage);
                            return S_OK;
                        }
                    }
                    else if (wvCapacity == 2)
                    {
                        wstring sCheck = wv[0];
                        if (sCheck.find(L's') == std::string::npos)
                        {
                            pDebugControl->Output(DEBUG_OUTPUT_NORMAL, seekUsage);
                            return S_OK;
                        }
                    }
                    else
                    {
                        pDebugControl->Output(DEBUG_OUTPUT_NORMAL, seekUsage);
                        return S_OK;
                    }


                    // Last should be our target to search for.
                    target = wv[wvCapacity - 1];
                }
                else
                {
                    pDebugControl->Output(DEBUG_OUTPUT_NORMAL, seekUsage);
                    return S_OK;
                }

                // Do le maths.
                IDebugSymbols5* pDebugSymbols = nullptr;
                if (SUCCEEDED(pDebugClient->QueryInterface(__uuidof(IDebugSymbols5), (void**)&pDebugSymbols)))
                {
                    IDebugSystemObjects4* pDebugSystemObjects = nullptr;
                    if (SUCCEEDED(pDebugClient->QueryInterface(__uuidof(IDebugSystemObjects4), (void**)&pDebugSystemObjects)))
                    {
                        ULONG numberOfThreads = 0;
                        if (SUCCEEDED(pDebugSystemObjects->GetNumberThreads(&numberOfThreads)))
                        {
                            std::vector<ULONG> Ids(numberOfThreads); // Debug Thread Ids
                            std::vector<ULONG> SysIds(numberOfThreads); // System Thread Ids (Not needed - leaving for future use)
                            numberOfThreads--;
                            if (SUCCEEDED(pDebugSystemObjects->GetThreadIdsByIndex(0, numberOfThreads, Ids.data(), SysIds.data())))
                            {
                                ULONG currentThreadId = 0;
                                if (SUCCEEDED(pDebugSystemObjects->GetCurrentThreadId(&currentThreadId)))
                                {
                                    for (ULONG iterationThreads = 0; iterationThreads < numberOfThreads; iterationThreads++)
                                    {
                                        if (SUCCEEDED(pDebugSystemObjects->SetCurrentThreadId(Ids[iterationThreads])))
                                        {
                                            ULONG maxStackDepth = 250;
                                            ULONG frames = 0;
                                            std::vector<DEBUG_STACK_FRAME> stackFrames(maxStackDepth);
                                            if (SUCCEEDED(pDebugControl->GetStackTrace(0, 0, 0, stackFrames.data(), maxStackDepth, &frames)))
                                            {
                                                char symName[4096];
                                                memset(symName, 0, 4096);
                                                ULONG symSize = 0;
                                                ULONG64 displacement = 0;
                                                bool shouldOutput = false;
                                                for (const DEBUG_STACK_FRAME& frame : stackFrames)
                                                {
                                                    if (SUCCEEDED(pDebugSymbols->GetNameByOffset(frame.InstructionOffset, symName, 4096, &symSize, &displacement)))
                                                    {
                                                        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
                                                        wstring ws = converter.from_bytes(symName);
                                                        if (ws.find(target) != std::string::npos)
                                                        {
                                                            shouldOutput = true;
                                                            break;
                                                        }
                                                    }
                                                }

                                                if (shouldOutput)
                                                {
                                                    if (!quietMode)
                                                    {
                                                        pDebugControl->ControlledOutput(DEBUG_OUTCTL_DML, DEBUG_OUTPUT_NORMAL, "Thread Id: <link cmd=\"~%llus\">%llu\n</link>", Ids[iterationThreads], Ids[iterationThreads]);
                                                    }


                                                    pDebugControl->OutputStackTrace(DEBUG_OUTPUT_NORMAL, stackFrames.data(), frames, DEBUG_STACK_ARGUMENTS | DEBUG_STACK_FRAME_ADDRESSES | DEBUG_STACK_SOURCE_LINE | DEBUG_STACK_FRAME_NUMBERS);
                                                }
                                            }
                                        }
                                    }

                                    // Reset thread context to original.
                                    if (!SUCCEEDED(pDebugSystemObjects->SetCurrentThreadId(currentThreadId)))
                                    {
                                        pDebugControl->Output(DEBUG_OUTPUT_WARNING, "Unable to return to original thread context.\n");
                                    }
                                }
                            }
                        }
                        else
                        {
                            printf("Unable to obtain the GetTotalNumberThreads.\n");
                        }

                        pDebugSystemObjects->Release();
                    }

                    pDebugSymbols->Release();
                }
            }

            pDebugControl->Release();
        }
    }

    return status;
}

HRESULT CALLBACK deep(_In_ PDEBUG_CLIENT8 pDebugClient, _In_ PCSTR args)
{
    helpers h{};
    helpers* internalHelper = &h;
    if (SUCCEEDED(internalHelper->IsUserMode(pDebugClient)))
    {
        auto Status = S_OK;
        IDebugControl7* pDebugControl = nullptr;
        if (SUCCEEDED(pDebugClient->QueryInterface(__uuidof(IDebugControl7), (void**)&pDebugControl)))
        {
            std::string test = args;
            if (auto size = test.size(); size == 0)
            {
                pDebugControl->Output(DEBUG_OUTPUT_EXTENSION_WARNING, "A target size MUST be supplied to target the frame size[s] against.\n  Example: !deep 43\n");
                return S_OK;
            }

            ULONG targetSize = atoi(test.c_str());
            IDebugSymbols5* pDebugSymbols = nullptr;
            if (SUCCEEDED(pDebugClient->QueryInterface(__uuidof(IDebugSymbols5), (void**)&pDebugSymbols)))
            {
                IDebugSystemObjects4* pDebugSystemObjects = nullptr;
                if (SUCCEEDED(pDebugClient->QueryInterface(__uuidof(IDebugSystemObjects4), (void**)&pDebugSystemObjects)))
                {
                    ULONG numberOfThreads = 0;
                    if (SUCCEEDED(pDebugSystemObjects->GetNumberThreads(&numberOfThreads)))
                    {
                        std::vector<ULONG> Ids(numberOfThreads); // Debug Thread Ids
                        std::vector<ULONG> SysIds(numberOfThreads); // System Thread Ids (Not needed - leaving for future use)
                        numberOfThreads--;
                        if (SUCCEEDED(pDebugSystemObjects->GetThreadIdsByIndex(0, numberOfThreads, Ids.data(), SysIds.data())))
                        {
                            ULONG currentThreadId = 0;
                            if (SUCCEEDED(pDebugSystemObjects->GetCurrentThreadId(&currentThreadId)))
                            {
                                for (ULONG iterationThreads = 0; iterationThreads < numberOfThreads; iterationThreads++)
                                {
                                    if (SUCCEEDED(pDebugSystemObjects->SetCurrentThreadId(Ids[iterationThreads])))
                                    {
                                        ULONG maxStackDepth = 100;
                                        ULONG frames = 0;
                                        std::vector<DEBUG_STACK_FRAME> stackFrames(maxStackDepth);
                                        if (SUCCEEDED(pDebugControl->GetStackTrace(0, 0, 0, stackFrames.data(), maxStackDepth, &frames)))
                                        {
                                            if (frames > targetSize) 
                                            {
                                                pDebugControl->ControlledOutput(DEBUG_OUTCTL_DML, DEBUG_OUTPUT_NORMAL, "Thread Id: <link cmd=\"~%llus\">%llu\n</link>", Ids[iterationThreads], Ids[iterationThreads]);
                                                pDebugControl->OutputStackTrace(DEBUG_OUTPUT_NORMAL, stackFrames.data(), frames, DEBUG_STACK_ARGUMENTS | DEBUG_STACK_FRAME_ADDRESSES | DEBUG_STACK_SOURCE_LINE | DEBUG_STACK_FRAME_NUMBERS);
                                            }
                                        }
                                    }
                                }

                                // Reset thread context to original.
                                if (!SUCCEEDED(pDebugSystemObjects->SetCurrentThreadId(currentThreadId)))
                                {
                                    pDebugControl->Output(DEBUG_OUTPUT_WARNING, "Unable to return to original thread context.\n");
                                }
                            }
                        }
                    }
                    else
                    {
                        printf("Unable to obtain the GetTotalNumberThreads.\n");
                    }

                    pDebugSystemObjects->Release();
                }
                else
                {
                    printf("Unable to obtain the IDebugSystemObjects.\n");
                }

                pDebugSymbols->Release();
            }
            else
            {
                printf("Unable to obtain the IDebugSymbols.\n");
            }

            pDebugControl->Release();
        }
        else
        {
            printf("Unable to obtain the IDebugControl.\n");
        }

        return Status;
    }

    return ERROR_BAD_COMMAND;
}