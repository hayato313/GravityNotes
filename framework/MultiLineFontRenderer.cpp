#include "MultiLineFontRenderer.h"

#include <algorithm>

namespace
{
constexpr float kFontHitWidthScale = 0.5f;
}

MultiLineFontRenderer::MultiLineFontRenderer(XMFLOAT2 pos, float fontSize, float rotation,
XMFLOAT4 color, const std::string& text, float lineSpacing)
: FontRenderer(pos, fontSize, rotation, color, text)
, m_SourceText(text)
, m_FontSize(fontSize)
, m_LineSpacing((std::max)(lineSpacing, 0.1f))
{
RebuildLayout();
}

void MultiLineFontRenderer::Draw()
{
	RebuildLayout();

const XMFLOAT2 originalPos = GetPos();

for (size_t i = 0; i < m_Lines.size(); ++i)
{
const std::string& line = m_Lines[i];
if (line.empty())
{
continue;
}

SetPos({ originalPos.x, originalPos.y + static_cast<float>(i) * m_FontSize * m_LineSpacing });
FontRenderer::SetText(line);
FontRenderer::Draw();
}

SetPos(originalPos);
FontRenderer::SetText(m_SourceText);
}

void MultiLineFontRenderer::SetText(const std::string& text)
{
m_SourceText = text;
FontRenderer::SetText(text);
RebuildLayout();
}

void MultiLineFontRenderer::SetLineSpacing(float lineSpacing)
{
m_LineSpacing = (std::max)(lineSpacing, 0.1f);
RebuildLayout();
}

void MultiLineFontRenderer::RebuildLayout()
{
m_Lines = SplitLines(m_SourceText);
m_LineRects.clear();
m_LineRects.reserve(m_Lines.size());

const XMFLOAT2 basePos = GetPos();
const float lineHeight = m_FontSize * m_LineSpacing;

for (size_t i = 0; i < m_Lines.size(); ++i)
{
const int codePointCount = CountUtf8CodePoints(m_Lines[i]);
const float width = static_cast<float>(codePointCount) * m_FontSize * kFontHitWidthScale;
const float centerY = basePos.y + static_cast<float>(i) * lineHeight;

FontLineRect rect{};
rect.left = basePos.x - width * 0.5f;
rect.right = basePos.x + width * 0.5f;
rect.top = centerY - lineHeight * 0.5f;
rect.bottom = centerY + lineHeight * 0.5f;
m_LineRects.push_back(rect);
}
}

std::vector<std::string> MultiLineFontRenderer::SplitLines(const std::string& text) const
{
std::vector<std::string> lines;
std::string current;

for (char c : text)
{
if (c == '\r')
{
continue;
}
if (c == '\n')
{
lines.push_back(current);
current.clear();
continue;
}
current.push_back(c);
}

lines.push_back(current);
if (lines.empty())
{
lines.push_back("");
}

return lines;
}

int MultiLineFontRenderer::CountUtf8CodePoints(const std::string& text) const
{
int count = 0;
for (size_t i = 0; i < text.size();)
{
const unsigned char c = static_cast<unsigned char>(text[i]);
if ((c & 0x80) == 0)
{
i += 1;
}
else if ((c & 0xE0) == 0xC0)
{
i += 2;
}
else if ((c & 0xF0) == 0xE0)
{
i += 3;
}
else if ((c & 0xF8) == 0xF0)
{
i += 4;
}
else
{
i += 1;
}
count++;
}
return count;
}
