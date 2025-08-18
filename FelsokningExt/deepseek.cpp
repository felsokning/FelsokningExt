#include "pch.h"
#include "helpers.h"
#include <wrl/client.h> // For Microsoft::WRL::ComPtr

#pragma warning(disable : 4996) // _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

using namespace std;
using Microsoft::WRL::ComPtr;

const char deepSeekUsage[] = "FelsokningExt by John Bailey\n\tUsage: !deepseek <size> <symbol>\n\tExample:\n\t\t!deepseek 43 hostfxr!execute_app\n\n";

HRESULT CALLBACK deepseek(_In_ PDEBUG_CLIENT8 pDebugClient, _In_opt_ PCSTR args)
{
    auto status = S_OK;
    helpers h{};
    helpers* internalHelper = &h;

    if (SUCCEEDED(internalHelper->IsUserMode(pDebugClient)))
    {
        ComPtr<IDebugControl7> pDebugControl;
        if (!SUCCEEDED(pDebugClient->QueryInterface(__uuidof(IDebugControl7), &pDebugControl)))
        {
            return E_FAIL;
        }

        // Parse arguments
        string argsStr = args ? args : "";
        istringstream iss(argsStr);
        vector<string> arguments;
        string arg;
        while (iss >> arg) {
            arguments.push_back(arg);
        }

        if (arguments.size() != 2)
        {
            pDebugControl->Output(DEBUG_OUTPUT_NORMAL, deepSeekUsage);
            return S_OK;
        }

        ULONG targetSize = atoi(arguments[0].c_str());
        wstring searchTerm;
        wstring_convert<codecvt_utf8_utf16<wchar_t>> converter;
        searchTerm = converter.from_bytes(arguments[1]);

        ComPtr<IDebugSymbols5> pDebugSymbols;
        ComPtr<IDebugSystemObjects4> pDebugSystemObjects;

        if (!SUCCEEDED(pDebugClient->QueryInterface(__uuidof(IDebugSymbols5), &pDebugSymbols)) ||
            !SUCCEEDED(pDebugClient->QueryInterface(__uuidof(IDebugSystemObjects4), &pDebugSystemObjects)))
        {
            return E_FAIL;
        }

        ULONG numberOfThreads = 0;
        if (SUCCEEDED(pDebugSystemObjects->GetNumberThreads(&numberOfThreads)))
        {
            std::vector<ULONG> Ids(numberOfThreads);
            std::vector<ULONG> SysIds(numberOfThreads);
            
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
                                // Check if frame count exceeds target size
                                if (frames > targetSize)
                                {
                                    // Search for symbol in stack frames
                                    char symName[4096];
                                    ULONG symSize = 0;
                                    ULONG64 displacement = 0;
                                    bool foundSymbol = false;

                                    for (const DEBUG_STACK_FRAME& frame : stackFrames)
                                    {
                                        if (SUCCEEDED(pDebugSymbols->GetNameByOffset(frame.InstructionOffset, symName, 4096, &symSize, &displacement)))
                                        {
                                            wstring ws = converter.from_bytes(symName);
                                            if (ws.find(searchTerm) != std::string::npos)
                                            {
                                                foundSymbol = true;
                                                break;
                                            }
                                        }
                                    }

                                    if (foundSymbol)
                                    {
                                        pDebugControl->ControlledOutput(DEBUG_OUTCTL_DML, DEBUG_OUTPUT_NORMAL, 
                                            "Thread Id: <link cmd=\"~%llus\">%llu</link> (Stack depth: %lu)\n", 
                                            Ids[iterationThreads], Ids[iterationThreads], frames);
                                        pDebugControl->OutputStackTrace(DEBUG_OUTPUT_NORMAL, stackFrames.data(), frames, 
                                            DEBUG_STACK_ARGUMENTS | DEBUG_STACK_FRAME_ADDRESSES | DEBUG_STACK_SOURCE_LINE | DEBUG_STACK_FRAME_NUMBERS);
                                    }
                                }
                            }
                        }
                    }

                    // Reset thread context to original
                    if (!SUCCEEDED(pDebugSystemObjects->SetCurrentThreadId(currentThreadId)))
                    {
                        pDebugControl->Output(DEBUG_OUTPUT_WARNING, "Unable to return to original thread context.\n");
                    }
                }
            }
        }
    }

    return status;
}