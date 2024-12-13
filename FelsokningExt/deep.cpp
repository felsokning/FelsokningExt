#include "pch.h"
#include "helpers.h"

#pragma warning(disable : 4996) // _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

using namespace std;

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