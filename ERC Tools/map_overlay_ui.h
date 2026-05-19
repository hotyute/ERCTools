// =================================================================================
// FILE: map_overlay_ui.h
// =================================================================================

#pragma once
#include "common.h"

struct OverlayButton
{
    int id = 0;
    std::wstring text;
    D2D1_RECT_F bounds{};
    bool enabled = true;
    bool hot = false;
    bool pressed = false;
};

struct OverlayToggle
{
    int id = 0;
    std::wstring text;
    D2D1_RECT_F bounds{};
    bool checked = false;
    bool enabled = true;
    bool hot = false;
};

struct OverlayTextBox
{
    int id = 0;
    std::wstring text;
    std::wstring placeholder;
    D2D1_RECT_F bounds{};
    bool enabled = true;
    bool focused = false;
};

struct OverlaySlider
{
    int id = 0;
    D2D1_RECT_F bounds{};
    float minimum = 0.0f;
    float maximum = 1.0f;
    float value = 0.0f;
    bool enabled = true;
    bool hot = false;
};

class MapOverlayUiRenderer
{
public:
    bool EnsureResources(ID2D1RenderTarget* renderTarget, IDWriteFactory* writeFactory);
    void DiscardDeviceResources();

    bool HitTest(const D2D1_RECT_F& rect, float x, float y) const;
    float MeasureTextHeight(const std::wstring& text, IDWriteTextFormat* format, float width) const;
    float MeasureTextWidth(const std::wstring& text, IDWriteTextFormat* format, float width) const;

    void DrawGlassPanel(const D2D1_RECT_F& rect, float radius = 12.0f);
    void DrawLabel(const std::wstring& text, IDWriteTextFormat* format, const D2D1_RECT_F& rect, ID2D1Brush* brush = nullptr);
    void DrawButton(const OverlayButton& button);
    void DrawToggle(const OverlayToggle& toggle);
    void DrawRadio(const OverlayToggle& radio);
    void DrawTextBox(const OverlayTextBox& textBox);
    void DrawSlider(const OverlaySlider& slider);
    void DrawProgressBar(const D2D1_RECT_F& rect, float fraction);
    void DrawSeparator(float left, float right, float y);
    void DrawScrollbar(const D2D1_RECT_F& track, float contentHeight, float viewportHeight, float scrollOffset);

    IDWriteTextFormat* TitleFormat() const { return m_titleFormat.Get(); }
    IDWriteTextFormat* BodyFormat() const { return m_bodyFormat.Get(); }
    IDWriteTextFormat* SmallFormat() const { return m_smallFormat.Get(); }
    IDWriteTextFormat* ControlFormat() const { return m_controlFormat.Get(); }

    ID2D1SolidColorBrush* TextBrush() const { return m_textBrush.Get(); }
    ID2D1SolidColorBrush* MutedTextBrush() const { return m_mutedTextBrush.Get(); }
    ID2D1SolidColorBrush* AccentBrush() const { return m_accentBrush.Get(); }

private:
    bool CreateTextFormat(IDWriteFactory* writeFactory, const wchar_t* family, DWRITE_FONT_WEIGHT weight, float size, IDWriteTextFormat** formatOut);

    ID2D1RenderTarget* m_boundRenderTarget = nullptr;
    IDWriteFactory* m_boundWriteFactory = nullptr;
    ComPtr<ID2D1SolidColorBrush> m_panelBrush;
    ComPtr<ID2D1SolidColorBrush> m_panelStrongBrush;
    ComPtr<ID2D1SolidColorBrush> m_borderBrush;
    ComPtr<ID2D1SolidColorBrush> m_textBrush;
    ComPtr<ID2D1SolidColorBrush> m_mutedTextBrush;
    ComPtr<ID2D1SolidColorBrush> m_accentBrush;
    ComPtr<ID2D1SolidColorBrush> m_buttonBrush;
    ComPtr<ID2D1SolidColorBrush> m_buttonHotBrush;
    ComPtr<ID2D1SolidColorBrush> m_scrollTrackBrush;
    ComPtr<ID2D1SolidColorBrush> m_scrollThumbBrush;
    ComPtr<IDWriteTextFormat> m_titleFormat;
    ComPtr<IDWriteTextFormat> m_bodyFormat;
    ComPtr<IDWriteTextFormat> m_smallFormat;
    ComPtr<IDWriteTextFormat> m_controlFormat;
};
