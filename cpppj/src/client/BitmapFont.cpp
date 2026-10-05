// 원본 Screen.cpp 004a3240 ↔ CD 00425980, 004a2e40 ↔ CD 00425570, 004a39f0 ↔ CD 00425d10.
#include "client/BitmapFont.h"
#include <algorithm>
#include <bit>
#include <cstring>
#include <limits>
#include <stdexcept>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

namespace netstorm::client {
namespace {
// 16바이트 서명, 머리의 다섯 정수, 네 개의 256항목 표.
constexpr std::string_view kSignature{"BitmapFontData\x1a\0", 16};
constexpr std::size_t kTableStart = 36, kTableBytes = 1024, kPayloadStart = 4132;
// 원본 VFX 캐시 생성의 바탕·글자 팔레트 번호.
constexpr std::uint8_t kFontBackground = 32, kFontInk = 100;
// 원본 슬롯의 CreateFontA 높이·굵기. 슬롯 2는 사용하지 않는다.
constexpr std::array<int, 7> kHeights{14, 13, 0, 20, 48, 14, 12};
constexpr std::array<int, 7> kWeights{700, 0, 0, 700, 0, 0, 0};
// 원본 파일 이름의 스타일 문자열. 문맥의 번호 순서와 일치한다.
constexpr std::array<const char*, 5> kStyles{"normal", "italic", "bold", "strikeout", "underline"};
// 정렬에 의존하지 않고 파일 경계를 검사하여 32비트 정수를 읽는다.
std::uint32_t Read32(std::span<const std::uint8_t> bytes, std::size_t offset) {
    if (offset > bytes.size() || bytes.size() - offset < 4) throw std::runtime_error("Truncated bitmap font");
    std::uint32_t value = 0;
    // 낮은 바이트부터 모은다.
    for (std::size_t i = 0; i < 4; ++i) value |= static_cast<std::uint32_t>(bytes[offset + i]) << (i * 8);
    return value;
}
// 캐시 및 VFX 머리의 정수를 리틀 엔디언으로 기록한다.
void Put(std::vector<std::uint8_t>& bytes, std::size_t offset, std::uint32_t value, std::size_t size = 4) {
    // 지정한 크기만큼 기록한다.
    for (std::size_t i = 0; i < size; ++i) bytes.at(offset + i) = static_cast<std::uint8_t>(value >> (i * 8));
}
// GDI 생성 도중 예외가 나도 선택 객체·DC·비트맵·글꼴을 정리한다.
struct FontGdi {
    HDC dc{};
    HFONT font{};
    HBITMAP bitmap{};
    HGDIOBJ oldFont{}, oldBitmap{};
    // 선택했던 객체를 복구한 뒤 이 생성 경로에서 소유한 객체만 지운다.
    ~FontGdi() {
        if (oldFont) SelectObject(dc, oldFont);
        if (oldBitmap) SelectObject(dc, oldBitmap);
        if (font) DeleteObject(font);
        if (bitmap) DeleteObject(bitmap);
        if (dc) DeleteDC(dc);
    }
};
// GDI의 단색 글리프를 VFX의 투명 건너뛰기·반복 run으로 저장한다.
std::vector<std::uint8_t> EncodeGlyph(const std::uint8_t* pixels, int pitch, int width, int height) {
    int left = width, top = height, right = -1, bottom = -1;
    // 실제 글자가 있는 최소 사각형을 찾는다.
    for (int y = 0; y < height; ++y) {
        // 이 행의 불투명 픽셀을 검사한다.
        for (int x = 0; x < width; ++x) if (pixels[y * pitch + x] != kFontBackground) {
            left = std::min(left, x); top = std::min(top, y); right = std::max(right, x); bottom = std::max(bottom, y);
        }
    }
    std::vector<std::uint8_t> shape(40);
    Put(shape, 0, 0x30312e31); Put(shape, 4, 1); Put(shape, 8, 16);
    Put(shape, 16, static_cast<std::uint32_t>(height - 1), 2); Put(shape, 18, static_cast<std::uint32_t>(width - 1), 2);
    if (right < 0) {
        // 공백도 원본처럼 VFX 특수 빈 프레임과 정상 전진 폭을 갖는다.
        Put(shape, 24, 0x7fffffff); Put(shape, 28, 0x7fffffff); Put(shape, 32, 0x80000001); Put(shape, 36, 0x80000001);
        return shape;
    }
    Put(shape, 24, static_cast<std::uint32_t>(left)); Put(shape, 28, static_cast<std::uint32_t>(top));
    Put(shape, 32, static_cast<std::uint32_t>(right)); Put(shape, 36, static_cast<std::uint32_t>(bottom));
    // 압축 사각형의 행을 차례로 쓴다.
    for (int y = top; y <= bottom; ++y) {
        int x = left;
        // 투명/불투명 구간을 127픽셀 이하 run으로 분할한다.
        while (x <= right) {
            const bool ink = pixels[y * pitch + x] != kFontBackground;
            int count = 1;
            // 같은 종류의 다음 픽셀을 묶는다.
            while (count < 127 && x + count <= right && (pixels[y * pitch + x + count] != kFontBackground) == ink) ++count;
            shape.push_back(static_cast<std::uint8_t>(ink ? count * 2 : 1));
            shape.push_back(static_cast<std::uint8_t>(ink ? kFontInk : count));
            x += count;
        }
        shape.push_back(0);
    }
    return shape;
}
}

// 네 표는 측정 폭, ABC A, ABC B+C, 글리프 블록 바이트 수다. 포인터 표는 파일에 저장되지 않는다.
BitmapFont::BitmapFont(std::span<const std::uint8_t> bytes) {
    if (bytes.size() < kPayloadStart || std::memcmp(bytes.data(), kSignature.data(), kSignature.size()) != 0 ||
        Read32(bytes, 16) != 1 || Read32(bytes, 32) != 256) throw std::runtime_error("Invalid bitmap font header");
    height_ = static_cast<int>(Read32(bytes, 20)); ascent_ = static_cast<int>(Read32(bytes, 24)); descent_ = static_cast<int>(Read32(bytes, 28));
    if (height_ <= 0 || height_ > 512 || ascent_ < 0 || descent_ < 0 || ascent_ > height_ || descent_ > height_)
        throw std::runtime_error("Invalid bitmap font metrics");
    std::size_t position = kPayloadStart;
    // 바이트 코드 0~255의 독립 VFX 블록을 읽는다.
    for (std::size_t c = 0; c < glyphs_.size(); ++c) {
        auto& glyph = glyphs_[c];
        glyph.advance = std::bit_cast<std::int32_t>(Read32(bytes, kTableStart + c * 4));
        glyph.bearing = std::bit_cast<std::int32_t>(Read32(bytes, kTableStart + kTableBytes + c * 4));
        glyph.drawAdvance = std::bit_cast<std::int32_t>(Read32(bytes, kTableStart + kTableBytes * 2 + c * 4));
        const auto size = Read32(bytes, kTableStart + kTableBytes * 3 + c * 4);
        if (size > bytes.size() - position) throw std::runtime_error("Truncated bitmap font glyph");
        if (size != 0) {
            glyph.shape = std::make_unique<ShapeDatabase>(std::vector<std::uint8_t>(bytes.begin() + position, bytes.begin() + position + size));
            if (glyph.shape->Blocks().size() != 1 || glyph.shape->Blocks()[0].frames.size() != 1)
                throw std::runtime_error("Invalid bitmap font glyph block");
            if (!glyph.shape->Blocks()[0].frames[0].IsSpecial()) glyph.shape->Decode(0, 0);
        }
        position += size;
    }
    if (position != bytes.size()) throw std::runtime_error("Trailing bitmap font bytes");
}

// 004a2e40의 CreateFontA·ABC·배경 32/글자 100 경로를 독립 메모리 DIB에서 수행한다.
std::unique_ptr<BitmapFont> BitmapFont::Generate(std::string_view face, int height, int weight, FontStyle style) {
    if (height <= 0 || height > 128) throw std::invalid_argument("Invalid generated font height");
    // 글리프 하나의 최대 캔버스. 손상된 GDI 폭에 의한 쓰기 범위 초과를 막는다.
    constexpr int canvasWidth = 512, canvasHeight = 256;
    FontGdi gdi;
    gdi.dc = CreateCompatibleDC(nullptr);
    gdi.font = CreateFontA(height, 0, 0, 0, weight, style == FontStyle::Italic, style == FontStyle::Underline,
        style == FontStyle::Strikeout, ANSI_CHARSET, OUT_TT_ONLY_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        FF_SWISS, std::string(face).c_str());
    if (!gdi.dc || !gdi.font) throw std::runtime_error("Unable to create bitmap font GDI objects");
    // 8비트 DIB의 색 표를 실제 팔레트 번호와 일대일로 만든다.
    struct FontBitmapInfo { BITMAPINFOHEADER header; RGBQUAD colors[256]; } info{};
    info.header.biSize = sizeof(BITMAPINFOHEADER); info.header.biWidth = canvasWidth; info.header.biHeight = -canvasHeight;
    info.header.biPlanes = 1; info.header.biBitCount = 8; info.header.biCompression = BI_RGB;
    // 색 번호가 GDI의 PALETTEINDEX 지정과 동일하게 되도록 회색조 표를 채운다.
    for (int i = 0; i < 256; ++i) info.colors[i] = {static_cast<BYTE>(i), static_cast<BYTE>(i), static_cast<BYTE>(i), 0};
    void* pixels = nullptr;
    gdi.bitmap = CreateDIBSection(gdi.dc, reinterpret_cast<BITMAPINFO*>(&info), DIB_RGB_COLORS, &pixels, nullptr, 0);
    if (!gdi.bitmap || !pixels) throw std::runtime_error("Unable to create bitmap font DIB");
    gdi.oldBitmap = SelectObject(gdi.dc, gdi.bitmap); gdi.oldFont = SelectObject(gdi.dc, gdi.font);
    TEXTMETRICA metrics{};
    std::array<int, 256> widths{}; std::array<ABC, 256> abc{};
    if (!GetTextMetricsA(gdi.dc, &metrics) || !GetCharWidthA(gdi.dc, 0, 255, widths.data()))
        throw std::runtime_error("Unable to read bitmap font metrics");
    if (!GetCharABCWidthsA(gdi.dc, 0, 255, abc.data())) {
        // 비트맵/비트루타입 글꼴에는 원본처럼 A=C=0을 사용한다.
        for (std::size_t c = 0; c < abc.size(); ++c) abc[c] = {0, static_cast<UINT>(widths[c]), 0};
    }
    SetTextAlign(gdi.dc, TA_LEFT | TA_TOP); SetBkMode(gdi.dc, OPAQUE);
    SetBkColor(gdi.dc, RGB(kFontBackground, kFontBackground, kFontBackground));
    SetTextColor(gdi.dc, RGB(kFontInk, kFontInk, kFontInk));
    std::vector<std::uint8_t> cache(kPayloadStart);
    std::copy(kSignature.begin(), kSignature.end(), cache.begin()); Put(cache, 16, 1); Put(cache, 32, 256);
    Put(cache, 20, static_cast<std::uint32_t>(metrics.tmHeight)); Put(cache, 24, static_cast<std::uint32_t>(metrics.tmAscent));
    Put(cache, 28, static_cast<std::uint32_t>(metrics.tmDescent));
    // 각 글자를 따로 그려 캐시의 네 표와 VFX 블록을 만든다.
    for (std::size_t c = 0; c < 256; ++c) {
        const int width = static_cast<int>(abc[c].abcB) + 2;
        if (width <= 0 || width > canvasWidth || metrics.tmHeight >= canvasHeight) throw std::runtime_error("Generated glyph exceeds DIB");
        std::memset(pixels, kFontBackground, canvasWidth * canvasHeight);
        RECT rect{0, 0, width - 1, metrics.tmHeight + 1}; const char character = static_cast<char>(c);
        if (!ExtTextOutA(gdi.dc, -abc[c].abcA, 0, ETO_CLIPPED | ETO_OPAQUE, &rect, &character, 1, nullptr))
            throw std::runtime_error("Unable to render generated glyph");
        GdiFlush();
        const auto shape = EncodeGlyph(static_cast<const std::uint8_t*>(pixels), canvasWidth, width, metrics.tmHeight + 1);
        const auto advance = static_cast<std::uint32_t>(static_cast<int>(abc[c].abcB) + abc[c].abcC);
        Put(cache, kTableStart + c * 4, advance); Put(cache, kTableStart + kTableBytes + c * 4, static_cast<std::uint32_t>(abc[c].abcA));
        Put(cache, kTableStart + kTableBytes * 2 + c * 4, advance); Put(cache, kTableStart + kTableBytes * 3 + c * 4, static_cast<std::uint32_t>(shape.size()));
        cache.insert(cache.end(), shape.begin(), shape.end());
    }
    return std::make_unique<BitmapFont>(cache);
}
// 측정 경로는 그리기 경로와 달리 첫 번째 폭 표를 사용한다.
int BitmapFont::Measure(std::string_view bytes) const {
    std::int64_t width = 0;
    // 코드 페이지 바이트를 부호 없이 색인한다.
    for (const unsigned char c : bytes) width += glyphs_[c].advance;
    if (width < std::numeric_limits<int>::min() || width > std::numeric_limits<int>::max()) throw std::overflow_error("Font width overflow");
    return static_cast<int>(width);
}
// 코드별 글리프.
const FontGlyph& BitmapFont::Glyph(std::uint8_t code) const { return glyphs_[code]; }
// 높이.
int BitmapFont::Height() const { return height_; }
// 기준선 위 높이.
int BitmapFont::Ascent() const { return ascent_; }
// 기준선 아래 높이.
int BitmapFont::Descent() const { return descent_; }
// 설정의 fontFaceName을 보존한다.
FontStore::FontStore(const o::BaseFileSystem& files, std::string face) : files_(files), face_(face.empty() ? "Arial" : std::move(face)) {}
// 원본 슬롯·스타일 제한을 유지하고 생성 시 파일을 덮어쓰지 않는다.
const BitmapFont& FontStore::Get(int slot, FontStyle style) {
    const auto index = static_cast<std::size_t>(style);
    if (slot < 0 || slot >= 7 || slot == 2 || index >= kStyles.size() ||
        ((slot == 1 || slot == 3 || slot == 4) && style != FontStyle::Normal)) throw std::invalid_argument("Unavailable font slot/style");
    auto& font = fonts_[static_cast<std::size_t>(slot)][index];
    if (!font) {
        const std::string face = slot == 1 ? "Courier New" : face_;
        const auto path = "d/!" + face + "." + kStyles[index] + "." + std::to_string(kHeights[slot]) + "." + std::to_string(kWeights[slot]) + ".chfnt";
        if (const auto bytes = files_.TryRead(path)) {
            try { font = std::make_unique<BitmapFont>(*bytes); }
            catch (const std::runtime_error&) { /* 원본도 읽기에 실패한 캐시는 GDI로 다시 만든다. */ }
        }
        if (!font) font = BitmapFont::Generate(face, kHeights[slot], style == FontStyle::Bold ? kWeights[slot] + (slot == 0 ? 100 : 700) : kWeights[slot], style);
    }
    return *font;
}
}
