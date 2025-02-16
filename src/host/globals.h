/*++
Copyright (c) Microsoft Corporation
Licensed under the MIT license.

Module Name:
- globals.h

Abstract:
- This module contains the global variables used by the console server DLL.

Author:
- Jerry Shea (jerrysh) 21-Sep-1993

Revision History:
- Modified to reduce globals over Console V2 project (MiNiksa/PaulCam, 2014)
--*/

#pragma once

#include "selection.hpp"
#include "server.h"
#include "ConsoleArguments.hpp"
#include "ApiRoutines.h"

#include "../renderer/base/Renderer.hpp"

#include "../server/DeviceComm.h"
#include "../server/ConDrvDeviceComm.h"

#include <TraceLoggingProvider.h>
#include <winmeta.h>
TRACELOGGING_DECLARE_PROVIDER(g_hConhostV2EventTraceProvider);

class Globals
{
public:
    Globals();

    UINT uiOEMCP = GetOEMCP();
    UINT uiWindowsCP = GetACP();
    HINSTANCE hInstance;
    UINT uiDialogBoxCount;

    ConsoleArguments launchArgs;

    CONSOLE_INFORMATION& getConsoleInformation();

    IDeviceComm* pDeviceComm{ nullptr };

    wil::unique_event_nothrow hInputEvent;

    SHORT sVerticalScrollSize;
    SHORT sHorizontalScrollSize;

    int dpi = USER_DEFAULT_SCREEN_DPI;
    ULONG cursorPixelWidth = 1;

    NTSTATUS ntstatusConsoleInputInitStatus;
    wil::unique_event_nothrow hConsoleInputInitEvent;
    DWORD dwInputThreadId;

    std::vector<wchar_t> WordDelimiters;

    Microsoft::Console::Render::Renderer* pRender;

    Microsoft::Console::Render::IFontDefaultList* pFontDefaultList;

    bool IsHeadless() const;

    IApiRoutines* api;

    bool handoffTarget = false;

    std::optional<CLSID> handoffConsoleClsid;
    std::optional<CLSID> handoffTerminalClsid;
    wil::unique_hfile handoffInboxConsoleHandle;
    wil::unique_threadpool_wait handoffInboxConsoleExitWait;

#ifdef UNIT_TESTING
    void EnableConptyModeForTests(std::unique_ptr<Microsoft::Console::Render::VtEngine> vtRenderEngine);
#endif

private:
    CONSOLE_INFORMATION ciConsoleInformation;
    ApiRoutines defaultApiRoutines;
};
