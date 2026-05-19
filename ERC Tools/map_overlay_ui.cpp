// =================================================================================
// FILE: map_overlay_ui.cpp
// =================================================================================

#include "map_overlay_ui.h"

namespace
{
    float ClampFloat(float value, float lo, float hi)
    {
        return value < lo ? lo : (value > hi ? hi : value);
    }

    float MaxFloat(float a, float b)
    {
        return a > b ? a : b;
    }

    bool ValidRect(const D2D1_RECT_F& rect)
    {
        return rect.right > rect.left && rect.bottom > rect.top;
    }
}

bool MapOverlayUiRenderer::EnsureResources(ID2D1RenderTarget* renderTarget, IDWriteFactory* writeFactory)
{
    if (!renderTarget || !writeFactory)
        return false;

    if (m_boundRenderTarget == renderTarget && m_panelBrush && m_titleFormat)
        return true;

    DiscardDeviceResources();
    m_boundRenderTarget = renderTarget;
    m_boundWriteFactory = writeFactory;

    HRESULT hr = S_OK;
    hr = renderTarget->CreateSolidColorBrush(D2D1::ColorF(0.025f, 0.040f, 0.055f, 0.78f), &m_panelBrush);
    if (FAILED(hr)) return false;
    hr = renderTarget->CreateSolidColorBrush(D2D1::ColorF(0.025f, 0.040f, 0.055f, 0.90f), &m_panelStrongBrush);
    if (FAILED(hr)) return false;
    hr = renderTarget->CreateSolidColorBrush(D2D1::ColorF(0.93f, 0.97f, 1.0f, 0.32f), &m_borderBrush);
    if (FAILED(hr)) return false;
    hr = renderTarget->CreateSolidColorBrush(D2D1::ColorF(0.98f, 0.99f, 1.0f, 0.96f), &m_textBrush);
    if (FAILED(hr)) return false;
    hr = renderTarget->CreateSolidColorBrush(D2D1::ColorF(0.76f, 0.84f, 0.90f, 0.82f), &m_mutedTextBrush);
    if (FAILED(hr)) return false;
    hr = renderTarget->CreateSolidColorBrush(D2D1::ColorF(0.24f, 0.70f, 0.95f, 0.94f), &m_accentBrush);
    if (FAILED(hr)) return false;
    hr = renderTarget->CreateSolidColorBrush(D2D1::ColorF(0.12f, 0.18f, 0.23f, 0.78f), &m_buttonBrush);
    if (FAILED(hr)) return false;
    hr = renderTarget->CreateSolidColorBrush(D2D1::ColorF(0.18f, 0.28f, 0.35f, 0.88f), &m_buttonHotBrush);
    if (FAILED(hr)) return false;
    hr = renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.12f), &m_scrollTrackBrush);
    if (FAILED(hr)) return false;
    hr = renderTarget->CreateSolidColorBrush(D2D1::ColorF(0.56f, 0.78f, 0.92f, 0.72f), &m_scrollThumbBrush);
    if (FAILED(hr)) return false;

    if (!CreateTextFormat(writeFactory, L"Segoe UI", DWRITE_FONT_WEIGHT_SEMI_BOLD, 15.0f, &m_titleFormat))
        return false;
    if (!CreateTextFormat(writeFactory, L"Segoe UI", DWRITE_FONT_WEIGHT_NORMAL, 13.0f, &m_bodyFormat))
        return false;
    if (!CreateTextFormat(writeFactory, L"Segoe UI", DWRITE_FONT_WEIGHT_NORMAL, 11.0f, &m_smallFormat))
        return false;
    if (!CreateTextFormat(writeFactory, L"Segoe UI", DWRITE_FONT_WEIGHT_SEMI_BOLD, 12.0f, &m_controlFormat))
        return false;

    return true;
}

void MapOverlayUiRenderer::DiscardDeviceResources()
{
    m_boundRenderTarget = nullptr;
    m_boundWriteFactory = nullptr;
    m_panelBrush.Reset();
    m_panelStrongBrush.Reset();
    m_borderBrush.Reset();
    m_textBrush.Reset();
    m_mutedTextBrush.Reset();
    m_accentBrush.Reset();
    m_buttonBrush.Reset();
    m_buttonHotBrush.Reset();
    m_scrollTrackBrush.Reset();
    m_scrollThumbBrush.Reset();
    m_titleFormat.Reset();
    m_bodyFormat.Reset();
    m_smallFormat.Reset();
    m_controlFormat.Reset();
}

bool MapOverlayUiRenderer::HitTest(const D2D1_RECT_F& rect, float x, float y) const
{
    return x >= rect.left && x <= rect.right && y >= rect.top && y <= rect.bottom;
}

float MapOverlayUiRenderer::MeasureTextHeight(const std::wstring& text, IDWriteTextFormat* format, float width) const
{
    if (!format || text.empty())
        return 0.0f;

    width = ClampFloat(width, 1.0f, 4000.0f);
    if (!m_boundWriteFactory)
        return 0.0f;

    ComPtr<IDWriteTextLayout> layout;
    if (FAILED(m_boundWriteFactory->CreateTextLayout(
        text.c_str(),
        static_cast<UINT32>(text.size()),
        format,
        width,
        4000.0f,
        &layout)))
    {
        return 0.0f;
    }

    DWRITE_TEXT_METRICS metrics{};
    if (FAILED(layout->GetMetrics(&metrics)))
        return 0.0f;

    return metrics.height;
}

float MapOverlayUiRenderer::MeasureTextWidth(const std::wstring& text, IDWriteTextFormat* format, float width) const
{
    if (!format || text.empty())
        return 0.0f;

    width = ClampFloat(width, 1.0f, 4000.0f);
    if (!m_boundWriteFactory)
        return 0.0f;

    ComPtr<IDWriteTextLayout> layout;
    if (FAILED(m_boundWriteFactory->CreateTextLayout(
        text.c_str(),
        static_cast<UINT32>(text.size()),
        format,
        width,
        4000.0f,
        &layout)))
    {
        return 0.0f;
    }

    DWRITE_TEXT_METRICS metrics{};
    if (FAILED(layout->GetMetrics(&metrics)))
        return 0.0f;

    return metrics.widthIncludingTrailingWhitespace;
}

void MapOverlayUiRenderer::DrawGlassPanel(const D2D1_RECT_F& rect, float radius)
{
    if (!m_boundRenderTarget || !m_panelBrush || !ValidRect(rect))
        return;

    const D2D1_ROUNDED_RECT rounded = D2D1::RoundedRect(rect, radius, radius);
    m_boundRenderTarget->FillRoundedRectangle(rounded, m_panelBrush.Get());
    m_boundRenderTarget->DrawRoundedRectangle(rounded, m_borderBrush.Get(), 1.0f);
}

void MapOverlayUiRenderer::DrawLabel(const std::wstring& text, IDWriteTextFormat* format, const D2D1_RECT_F& rect, ID2D1Brush* brush)
{
    if (!m_boundRenderTarget || !format || text.empty() || !ValidRect(rect))
        return;

    m_boundRenderTarget->DrawTextW(
        text.c_str(),
        static_cast<UINT32>(text.size()),
        format,
        rect,
        brush ? brush : m_textBrush.Get(),
        D2D1_DRAW_TEXT_OPTIONS_CLIP);
}

void MapOverlayUiRenderer::DrawButton(const OverlayButton& button)
{
    if (!m_boundRenderTarget || !ValidRect(button.bounds))
        return;

    ID2D1SolidColorBrush* fill = button.hot ? m_buttonHotBrush.Get() : m_buttonBrush.Get();
    ID2D1SolidColorBrush* text = button.enabled ? m_textBrush.Get() : m_mutedTextBrush.Get();
    if (button.pressed)
        fill = m_panelStrongBrush.Get();

    const D2D1_ROUNDED_RECT rounded = D2D1::RoundedRect(button.bounds, 7.0f, 7.0f);
    m_boundRenderTarget->FillRoundedRectangle(rounded, fill);
    m_boundRenderTarget->DrawRoundedRectangle(rounded, m_borderBrush.Get(), 1.0f);

    if (m_controlFormat) {
        m_controlFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        m_controlFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        DrawLabel(button.text, m_controlFormat.Get(), button.bounds, text);
        m_controlFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
        m_controlFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
    }
}

void MapOverlayUiRenderer::DrawToggle(const OverlayToggle& toggle)
{
    if (!m_boundRenderTarget || !ValidRect(toggle.bounds))
        return;

    const float boxSize = 18.0f;
    const float boxTop = toggle.bounds.top + MaxFloat(0.0f, (toggle.bounds.bottom - toggle.bounds.top - boxSize) * 0.5f);
    D2D1_RECT_F box = D2D1::RectF(toggle.bounds.left, boxTop, toggle.bounds.left + boxSize, boxTop + boxSize);
    const D2D1_ROUNDED_RECT boxRounded = D2D1::RoundedRect(box, 5.0f, 5.0f);
    m_boundRenderTarget->FillRoundedRectangle(boxRounded, toggle.hot ? m_buttonHotBrush.Get() : m_buttonBrush.Get());
    m_boundRenderTarget->DrawRoundedRectangle(boxRounded, m_borderBrush.Get(), 1.0f);

    if (toggle.checked) {
        m_boundRenderTarget->DrawLine(D2D1::Point2F(box.left + 4.0f, box.top + 9.0f), D2D1::Point2F(box.left + 8.0f, box.bottom - 5.0f), m_accentBrush.Get(), 2.0f);
        m_boundRenderTarget->DrawLine(D2D1::Point2F(box.left + 8.0f, box.bottom - 5.0f), D2D1::Point2F(box.right - 4.0f, box.top + 5.0f), m_accentBrush.Get(), 2.0f);
    }

    D2D1_RECT_F textRect = D2D1::RectF(box.right + 8.0f, toggle.bounds.top, toggle.bounds.right, toggle.bounds.bottom);
    DrawLabel(toggle.text, m_bodyFormat.Get(), textRect, toggle.enabled ? m_textBrush.Get() : m_mutedTextBrush.Get());
}

void MapOverlayUiRenderer::DrawRadio(const OverlayToggle& radio)
{
    if (!m_boundRenderTarget || !ValidRect(radio.bounds))
        return;

    const float size = 18.0f;
    const float cy = radio.bounds.top + (radio.bounds.bottom - radio.bounds.top) * 0.5f;
    const D2D1_POINT_2F center = D2D1::Point2F(radio.bounds.left + size * 0.5f, cy);
    const D2D1_ELLIPSE outer = D2D1::Ellipse(center, size * 0.5f, size * 0.5f);
    m_boundRenderTarget->FillEllipse(outer, radio.hot ? m_buttonHotBrush.Get() : m_buttonBrush.Get());
    m_boundRenderTarget->DrawEllipse(outer, m_borderBrush.Get(), 1.0f);
    if (radio.checked)
        m_boundRenderTarget->FillEllipse(D2D1::Ellipse(center, 5.5f, 5.5f), m_accentBrush.Get());

    D2D1_RECT_F textRect = D2D1::RectF(radio.bounds.left + size + 8.0f, radio.bounds.top, radio.bounds.right, radio.bounds.bottom);
    DrawLabel(radio.text, m_bodyFormat.Get(), textRect, radio.enabled ? m_textBrush.Get() : m_mutedTextBrush.Get());
}

void MapOverlayUiRenderer::DrawTextBox(const OverlayTextBox& textBox)
{
    if (!m_boundRenderTarget || !ValidRect(textBox.bounds))
        return;

    const D2D1_ROUNDED_RECT rounded = D2D1::RoundedRect(textBox.bounds, 7.0f, 7.0f);
    m_boundRenderTarget->FillRoundedRectangle(rounded, m_buttonBrush.Get());
    m_boundRenderTarget->DrawRoundedRectangle(rounded, textBox.focused ? m_accentBrush.Get() : m_borderBrush.Get(), textBox.focused ? 1.5f : 1.0f);

    const std::wstring& text = textBox.text.empty() ? textBox.placeholder : textBox.text;
    ID2D1Brush* brush = textBox.enabled && !textBox.text.empty() ? m_textBrush.Get() : m_mutedTextBrush.Get();
    D2D1_RECT_F textRect = D2D1::RectF(textBox.bounds.left + 10.0f, textBox.bounds.top + 6.0f, textBox.bounds.right - 10.0f, textBox.bounds.bottom - 4.0f);
    DrawLabel(text, m_bodyFormat.Get(), textRect, brush);
}

void MapOverlayUiRenderer::DrawSlider(const OverlaySlider& slider)
{
    if (!m_boundRenderTarget || !ValidRect(slider.bounds))
        return;

    const float cy = slider.bounds.top + (slider.bounds.bottom - slider.bounds.top) * 0.5f;
    const float left = slider.bounds.left + 8.0f;
    const float right = slider.bounds.right - 8.0f;
    const float span = MaxFloat(1.0f, right - left);
    const float denom = MaxFloat(0.0001f, slider.maximum - slider.minimum);
    const float t = ClampFloat((slider.value - slider.minimum) / denom, 0.0f, 1.0f);
    const float knobX = left + span * t;

    m_boundRenderTarget->DrawLine(D2D1::Point2F(left, cy), D2D1::Point2F(right, cy), m_scrollTrackBrush.Get(), 5.0f);
    m_boundRenderTarget->DrawLine(D2D1::Point2F(left, cy), D2D1::Point2F(knobX, cy), slider.enabled ? m_accentBrush.Get() : m_mutedTextBrush.Get(), 5.0f);
    const D2D1_ELLIPSE knob = D2D1::Ellipse(D2D1::Point2F(knobX, cy), slider.hot ? 8.0f : 7.0f, slider.hot ? 8.0f : 7.0f);
    m_boundRenderTarget->FillEllipse(knob, slider.enabled ? m_textBrush.Get() : m_mutedTextBrush.Get());
    m_boundRenderTarget->DrawEllipse(knob, m_borderBrush.Get(), 1.0f);
}

void MapOverlayUiRenderer::DrawProgressBar(const D2D1_RECT_F& rect, float fraction)
{
    if (!m_boundRenderTarget || !ValidRect(rect))
        return;

    fraction = ClampFloat(fraction, 0.0f, 1.0f);
    const D2D1_ROUNDED_RECT outer = D2D1::RoundedRect(rect, 6.0f, 6.0f);
    m_boundRenderTarget->FillRoundedRectangle(outer, m_scrollTrackBrush.Get());
    D2D1_RECT_F fill = rect;
    fill.right = fill.left + (rect.right - rect.left) * fraction;
    if (fill.right > fill.left)
        m_boundRenderTarget->FillRoundedRectangle(D2D1::RoundedRect(fill, 6.0f, 6.0f), m_accentBrush.Get());
    m_boundRenderTarget->DrawRoundedRectangle(outer, m_borderBrush.Get(), 1.0f);
}

void MapOverlayUiRenderer::DrawSeparator(float left, float right, float y)
{
    if (!m_boundRenderTarget || right <= left)
        return;

    m_boundRenderTarget->DrawLine(D2D1::Point2F(left, y), D2D1::Point2F(right, y), m_borderBrush.Get(), 1.0f);
}

void MapOverlayUiRenderer::DrawScrollbar(const D2D1_RECT_F& track, float contentHeight, float viewportHeight, float scrollOffset)
{
    if (!m_boundRenderTarget || !ValidRect(track) || contentHeight <= viewportHeight || viewportHeight <= 1.0f)
        return;

    const D2D1_ROUNDED_RECT trackRounded = D2D1::RoundedRect(track, 4.0f, 4.0f);
    m_boundRenderTarget->FillRoundedRectangle(trackRounded, m_scrollTrackBrush.Get());

    const float trackHeight = track.bottom - track.top;
    const float thumbHeight = ClampFloat(trackHeight * viewportHeight / contentHeight, 26.0f, trackHeight);
    const float maxScroll = MaxFloat(1.0f, contentHeight - viewportHeight);
    const float thumbTop = track.top + (trackHeight - thumbHeight) * ClampFloat(scrollOffset / maxScroll, 0.0f, 1.0f);
    const D2D1_ROUNDED_RECT thumb = D2D1::RoundedRect(
        D2D1::RectF(track.left, thumbTop, track.right, thumbTop + thumbHeight),
        4.0f,
        4.0f);
    m_boundRenderTarget->FillRoundedRectangle(thumb, m_scrollThumbBrush.Get());
}

bool MapOverlayUiRenderer::CreateTextFormat(IDWriteFactory* writeFactory, const wchar_t* family, DWRITE_FONT_WEIGHT weight, float size, IDWriteTextFormat** formatOut)
{
    if (!writeFactory || !formatOut)
        return false;

    ComPtr<IDWriteTextFormat> format;
    HRESULT hr = writeFactory->CreateTextFormat(
        family,
        nullptr,
        weight,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        size,
        L"en-gb",
        &format);
    if (FAILED(hr))
        return false;

    format->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);
    format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
    *formatOut = format.Detach();
    return true;
}
