// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "precomp.h"

#include "BgfxEngine.hpp"

#include "ConIoSrvComm.hpp"
#include "../inc/ServiceLocator.hpp"

#pragma hdrstop

//
// Default non-bright white.
//

#define DEFAULT_COLOR_ATTRIBUTE (0xC)

using namespace Microsoft::Console::Render;
using namespace Microsoft::Console::Interactivity;
using namespace Microsoft::Console::Interactivity::OneCore;

BgfxEngine::BgfxEngine(PVOID SharedViewBase, LONG DisplayHeight, LONG DisplayWidth, LONG FontWidth, LONG FontHeight) noexcept :
    RenderEngineBase(),
    _sharedViewBase(static_cast<std::byte*>(SharedViewBase)),
    _displayHeight(gsl::narrow_cast<SIZE_T>(DisplayHeight)),
    _displayWidth(gsl::narrow_cast<SIZE_T>(DisplayWidth)),
    _currentLegacyColorAttribute(DEFAULT_COLOR_ATTRIBUTE)
{
    _runLength = sizeof(CD_IO_CHARACTER) * DisplayWidth;

    _fontSize.X = FontWidth > SHORT_MAX ? SHORT_MAX : gsl::narrow_cast<SHORT>(FontWidth);
    _fontSize.Y = FontHeight > SHORT_MAX ? SHORT_MAX : gsl::narrow_cast<SHORT>(FontHeight);
}

[[nodiscard]] HRESULT BgfxEngine::Invalidate(const SMALL_RECT* const /*psrRegion*/) noexcept
{
    return S_OK;
}

[[nodiscard]] HRESULT BgfxEngine::InvalidateCursor(const SMALL_RECT* const /*psrRegion*/) noexcept
{
    return S_OK;
}

[[nodiscard]] HRESULT BgfxEngine::InvalidateSystem(const RECT* const /*prcDirtyClient*/) noexcept
{
    return S_OK;
}

[[nodiscard]] HRESULT BgfxEngine::InvalidateSelection(const std::vector<SMALL_RECT>& /*rectangles*/) noexcept
{
    return S_OK;
}

[[nodiscard]] HRESULT BgfxEngine::InvalidateScroll(const COORD* const /*pcoordDelta*/) noexcept
{
    return S_OK;
}

[[nodiscard]] HRESULT BgfxEngine::InvalidateAll() noexcept
{
    return S_OK;
}

[[nodiscard]] HRESULT BgfxEngine::PrepareForTeardown(_Out_ bool* const pForcePaint) noexcept
{
    *pForcePaint = false;
    return S_FALSE;
}

[[nodiscard]] HRESULT BgfxEngine::StartPaint() noexcept
{
    return S_OK;
}

[[nodiscard]] HRESULT BgfxEngine::EndPaint() noexcept
try
{
    const auto Status = ConIoSrvComm::GetConIoSrvComm()->RequestUpdateDisplay(0);

    if (NT_SUCCESS(Status))
    {
        for (SIZE_T i = 0; i < _displayHeight; i++)
        {
            const auto OldRunBase = _sharedViewBase + (i * 2 * _runLength);
            const auto NewRunBase = OldRunBase + _runLength;
            memcpy_s(OldRunBase, _runLength, NewRunBase, _runLength);
        }
    }

    return HRESULT_FROM_NT(Status);
}
CATCH_RETURN()

// Routine Description:
// - Used to perform longer running presentation steps outside the lock so the other threads can continue.
// - Not currently used by BgfxEngine.
// Arguments:
// - <none>
// Return Value:
// - S_FALSE since we do nothing.
[[nodiscard]] HRESULT BgfxEngine::Present() noexcept
{
    return S_FALSE;
}

[[nodiscard]] HRESULT BgfxEngine::ScrollFrame() noexcept
{
    return S_OK;
}

[[nodiscard]] HRESULT BgfxEngine::PaintBackground() noexcept
{
    for (SIZE_T i = 0; i < _displayHeight; i++)
    {
        const auto NewRun = reinterpret_cast<PCD_IO_CHARACTER>(_sharedViewBase + (i * 2 * _runLength) + _runLength);

        for (SIZE_T j = 0; j < _displayWidth; j++)
        {
            NewRun[j].Character = L' ';
            NewRun[j].Attribute = 0;
        }
    }

    return S_OK;
}

[[nodiscard]] HRESULT BgfxEngine::PaintBufferLine(const gsl::span<const Cluster> clusters,
                                                  const COORD coord,
                                                  const bool /*trimLeft*/,
                                                  const bool /*lineWrapped*/) noexcept
{
    try
    {
        const auto y = gsl::narrow_cast<SIZE_T>(coord.Y);
        const auto NewRun = reinterpret_cast<PCD_IO_CHARACTER>(_sharedViewBase + (y * 2 * _runLength) + _runLength);

        for (SIZE_T i = 0; i < clusters.size() && i < _displayWidth; i++)
        {
            NewRun[coord.X + i].Character = til::at(clusters, i).GetTextAsSingle();
            NewRun[coord.X + i].Attribute = _currentLegacyColorAttribute;
        }

        return S_OK;
    }
    CATCH_RETURN()
}

[[nodiscard]] HRESULT BgfxEngine::PaintBufferGridLines(GridLineSet const /*lines*/,
                                                       COLORREF const /*color*/,
                                                       size_t const /*cchLine*/,
                                                       const COORD /*coordTarget*/) noexcept
{
    return S_OK;
}

[[nodiscard]] HRESULT BgfxEngine::PaintSelection(const SMALL_RECT /*rect*/) noexcept
{
    return S_OK;
}

[[nodiscard]] HRESULT BgfxEngine::PaintCursor(const CursorOptions& options) noexcept
try
{
    // TODO: MSFT: 11448021 - Modify BGFX to support rendering full-width
    // characters and a full-width cursor.
    CD_IO_CURSOR_INFORMATION CursorInfo;
    CursorInfo.Row = options.coordCursor.Y;
    CursorInfo.Column = options.coordCursor.X;
    CursorInfo.Height = options.ulCursorHeightPercent;
    CursorInfo.IsVisible = TRUE;

    const auto Status = ConIoSrvComm::GetConIoSrvComm()->RequestSetCursor(&CursorInfo);

    return HRESULT_FROM_NT(Status);
}
CATCH_RETURN()

[[nodiscard]] HRESULT BgfxEngine::UpdateDrawingBrushes(const TextAttribute& textAttributes,
                                                       const RenderSettings& /*renderSettings*/,
                                                       const gsl::not_null<IRenderData*> /*pData*/,
                                                       const bool /*usingSoftFont*/,
                                                       const bool /*isSettingDefaultBrushes*/) noexcept
{
    _currentLegacyColorAttribute = textAttributes.GetLegacyAttributes();

    return S_OK;
}

[[nodiscard]] HRESULT BgfxEngine::UpdateFont(const FontInfoDesired& /*pfiFontInfoDesired*/, FontInfo& /*pfiFontInfo*/) noexcept
{
    return S_OK;
}

[[nodiscard]] HRESULT BgfxEngine::UpdateDpi(const int /*iDpi*/) noexcept
{
    return S_OK;
}

// Method Description:
// - This method will update our internal reference for how big the viewport is.
//      Does nothing for BGFX.
// Arguments:
// - srNewViewport - The bounds of the new viewport.
// Return Value:
// - HRESULT S_OK
[[nodiscard]] HRESULT BgfxEngine::UpdateViewport(const SMALL_RECT /*srNewViewport*/) noexcept
{
    return S_OK;
}

[[nodiscard]] HRESULT BgfxEngine::GetProposedFont(const FontInfoDesired& /*pfiFontInfoDesired*/, FontInfo& /*pfiFontInfo*/, const int /*iDpi*/) noexcept
{
    return S_OK;
}

[[nodiscard]] HRESULT BgfxEngine::GetDirtyArea(gsl::span<const til::rect>& area) noexcept
{
    _dirtyArea.bottom = gsl::narrow_cast<til::CoordType>(std::max(static_cast<SIZE_T>(0), _displayHeight));
    _dirtyArea.right = gsl::narrow_cast<til::CoordType>(std::max(static_cast<SIZE_T>(0), _displayWidth));

    area = { &_dirtyArea,
             1 };

    return S_OK;
}

[[nodiscard]] HRESULT BgfxEngine::GetFontSize(_Out_ COORD* const pFontSize) noexcept
{
    *pFontSize = _fontSize;
    return S_OK;
}

[[nodiscard]] HRESULT BgfxEngine::IsGlyphWideByFont(const std::wstring_view /*glyph*/, _Out_ bool* const pResult) noexcept
{
    *pResult = false;
    return S_OK;
}

// Method Description:
// - Updates the window's title string.
//      Does nothing for BGFX.
// Arguments:
// - newTitle: the new string to use for the title of the window
// Return Value:
// - S_OK
[[nodiscard]] HRESULT BgfxEngine::_DoUpdateTitle(_In_ const std::wstring_view /*newTitle*/) noexcept
{
    return S_OK;
}
