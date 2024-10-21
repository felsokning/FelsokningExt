// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"

#define APPNAME _T("FelsokningExt")

using namespace std;

HRESULT IsKernelMode(_In_ PDEBUG_CLIENT4 DebugClient)
{
    auto Status = S_OK;
    ULONG Class;
    ULONG Qualifier;

    IDebugControl* DebugControl = nullptr;
    __try {
        if ((Status = DebugClient->QueryInterface(__uuidof(IDebugControl), (PVOID*)&DebugControl)) != S_OK) 
        {
            __leave;
        }

        if ((Status = DebugControl->GetDebuggeeType(&Class, &Qualifier)) != S_OK) 
        {
            DebugControl->Output(DEBUG_OUTPUT_NORMAL, "Could not get debuggee type.\n");
            __leave;
        }

        if (Class != DEBUG_CLASS_USER_WINDOWS) 
        {

            DebugControl->Output(DEBUG_OUTPUT_WARNING, "FelsokningExt: Only works in user-mode debugging mode.\n");
            Status = S_FALSE;

            __leave;
        }
    }
    __finally {

        if (DebugControl) {

            DebugControl->Release();
        }
    }

    return Status;
}

VOID CALLBACK DebugExtensionUninitialize(VOID)
{
    return;
}

HRESULT CALLBACK DebugExtensionInitialize(_Out_ PULONG Version, _Out_ PULONG Flags)
{
    *Version = DEBUG_EXTENSION_VERSION(1, 0);
    *Flags = 0;

    return S_OK;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH:
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
        default:
            break;
    }
    return TRUE;
}

HRESULT CALLBACK deep(_In_ PDEBUG_CLIENT4 Client, _In_opt_ PCSTR args)
{
    if (SUCCEEDED(IsKernelMode(Client)))
    {
        auto Status = S_OK;
        IDebugControl* pDebugControl;
        if (SUCCEEDED(Client->QueryInterface(__uuidof(IDebugControl), (void**)&pDebugControl)))
        {
            std::string test = args;
            if (auto size = test.size(); size == 0)
            {
                pDebugControl->Output(DEBUG_OUTPUT_EXTENSION_WARNING, "A target size MUST be supplied to target the frame size[s] against.\n  Example: !deep 43\n");
                return S_OK;
            }

            ULONG targetSize = atoi(test.c_str());
            IDebugSymbols* pDebugSymbols;
            if (SUCCEEDED(Client->QueryInterface(__uuidof(IDebugSymbols), (void**)&pDebugSymbols)))
            {
                IDebugSystemObjects4* pDebugSystemObjects;
                if (SUCCEEDED(Client->QueryInterface(__uuidof(IDebugSystemObjects4), (void**)&pDebugSystemObjects)))
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
                                    pDebugControl->Output(DEBUG_OUTPUT_WARNING, "Unable to return to original thread context.");
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