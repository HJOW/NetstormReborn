// 원본 Screen.cpp 004a3240 ↔ CD 00425980, 004a2e40 ↔ CD 00425570, 004a39f0 ↔ CD 00425d10.
#include "client/BitmapFont.h"
#include "client/NativeFontCacheFile.h" // 현재 FILE과 같은 생성 글리프의 순차 캐시 reader/writer다.
#include "o/RawFileName.h" // 원문(-2,d,-3,literal)의 같은 이름고리를 사용한다.
#include "o/OriginalText.h" // 6차: 확장 문자(한글) 시퀀스를 글자 폭 측정에서 한 글자로 읽는다.
#include "platform/UserDataRoot.h" // 5차: 사용자 글꼴 캐시 폴더를 첫 저장에서만 만든다.
#include "client/FontFaceEncoding.h" //raw ACP CHFNT 이름은 TAFF에, UTF-8 이름은 디스크에만 사용한다.
#include <algorithm>
#include <mutex>
#include <bit>
#include <cstring>
#include <exception> //Win32 Enum callback의 C++ 실패를 DC 반납 뒤 다시 던진다.
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
    // dc: GDI 출력에 사용하는 장치 컨텍스트.
    HDC dc{};
    // font: 문자 출력에 사용할 글꼴 또는 글꼴 핸들.
    HFONT font{};
    // bitmap: GDI 출력에 사용하는 비트맵 핸들.
    HBITMAP bitmap{};
    // oldFont: GDI 작업 뒤 장치 컨텍스트에 다시 선택할 이전 글꼴.
    // oldBitmap: GDI 작업 뒤 장치 컨텍스트에 다시 선택할 이전 비트맵.
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
    struct FontBitmapInfo { /* header: Win32 비트맵의 크기·형식을 지정하는 헤더. */ BITMAPINFOHEADER header; /* colors: 팔레트 색 또는 색 인덱스 변환 자료. */ RGBQUAD colors[256]; } info{};
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
namespace { // 6차 확장 글리프 보조다.
// 글꼴 파일을 프로세스 전용으로 한 번만 등록한다. 같은 경로를 다시 요청하면 아무것도 하지 않는다.
void EnsurePrivateFontFile(const std::filesystem::path& file) {
    static std::mutex mutex; static std::vector<std::filesystem::path> loaded;
    if (file.empty()) return;
    const std::lock_guard lock(mutex);
    if (std::find(loaded.begin(), loaded.end(), file) != loaded.end()) return;
    loaded.push_back(file);
    AddFontResourceExW(file.c_str(), FR_PRIVATE, nullptr); // 실패해도 시스템에 같은 이름의 글꼴이 있으면 그것을 쓴다.
}
}
void BitmapFont::ConfigureExtended(ExtendedFontSpec spec) { extended_ = std::move(spec); extendedGlyphs_.clear(); extendedAdvance_ = -1; }
const FontGlyph* BitmapFont::ExtendedGlyph(std::uint32_t codePoint) const {
    if (!HasExtended() || codePoint > 0xffff) return nullptr;
    if (const auto found = extendedGlyphs_.find(codePoint); found != extendedGlyphs_.end()) return &found->second;
    EnsurePrivateFontFile(extended_.fontFile);
    constexpr int canvasWidth = 512, canvasHeight = 256;
    FontGdi gdi;
    gdi.dc = CreateCompatibleDC(nullptr);
    gdi.font = CreateFontW(extended_.height, 0, 0, 0, extended_.weight, extended_.italic, extended_.underline, extended_.strikeout, DEFAULT_CHARSET,
        OUT_TT_ONLY_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, std::wstring(extended_.face.begin(), extended_.face.end()).c_str());
    if (!gdi.dc || !gdi.font) return nullptr;
    struct FontBitmapInfo { BITMAPINFOHEADER header; RGBQUAD colors[256]; } info{};
    info.header.biSize = sizeof(BITMAPINFOHEADER); info.header.biWidth = canvasWidth; info.header.biHeight = -canvasHeight;
    info.header.biPlanes = 1; info.header.biBitCount = 8; info.header.biCompression = BI_RGB;
    for (int i = 0; i < 256; ++i) info.colors[i] = {static_cast<BYTE>(i), static_cast<BYTE>(i), static_cast<BYTE>(i), 0}; // 라틴 글리프와 같은 회색조 색 번호 표다.
    void* pixels = nullptr;
    gdi.bitmap = CreateDIBSection(gdi.dc, reinterpret_cast<BITMAPINFO*>(&info), DIB_RGB_COLORS, &pixels, nullptr, 0);
    if (!gdi.bitmap || !pixels) return nullptr;
    gdi.oldBitmap = SelectObject(gdi.dc, gdi.bitmap); gdi.oldFont = SelectObject(gdi.dc, gdi.font);
    TEXTMETRICW metrics{};
    if (!GetTextMetricsW(gdi.dc, &metrics)) return nullptr;
    const wchar_t character = static_cast<wchar_t>(codePoint);
    ABC abc{};
    if (!GetCharABCWidthsW(gdi.dc, character, character, &abc)) {
        int width = 0;
        if (!GetCharWidth32W(gdi.dc, character, character, &width)) return nullptr;
        abc = {0, static_cast<UINT>(width), 0};
    }
    SetTextAlign(gdi.dc, TA_LEFT | TA_TOP); SetBkMode(gdi.dc, OPAQUE);
    SetBkColor(gdi.dc, RGB(kFontBackground, kFontBackground, kFontBackground)); SetTextColor(gdi.dc, RGB(kFontInk, kFontInk, kFontInk));
    // 라틴 글꼴과 같은 기준선에 맞추도록 글자를 (라틴 ascent - 한글 ascent)만큼 아래로 내린다. 음수면 위로 올리지 않고 0으로 둔다.
    const int shift = std::max(0, ascent_ - static_cast<int>(metrics.tmAscent));
    const int cell = abc.abcA + static_cast<int>(abc.abcB) + abc.abcC; // 모노스페이스 한 칸의 전체 폭(좌측 여백+잉크+우측 여백)이다.
    const int width = cell + 2, height = std::max(static_cast<int>(metrics.tmHeight), height_) + 1 + shift;
    if (cell <= 0 || width > canvasWidth || height >= canvasHeight) return nullptr;
    std::memset(pixels, kFontBackground, canvasWidth * canvasHeight);
    RECT rect{0, 0, width - 1, height};
    if (!ExtTextOutW(gdi.dc, 0, shift, ETO_CLIPPED | ETO_OPAQUE, &rect, &character, 1, nullptr)) return nullptr; // 라틴과 달리 -A 이동 없이 칸 안의 제 위치에 그린다.
    GdiFlush();
    FontGlyph glyph;
    glyph.shape = std::make_unique<ShapeDatabase>(EncodeGlyph(static_cast<const std::uint8_t*>(pixels), canvasWidth, width, height));
    glyph.advance = cell; glyph.bearing = 0; glyph.drawAdvance = cell; // 칸 전체가 전진 폭이라 모든 한글/자모/전각이 같은 폭이다.
    if (!glyph.shape->Blocks()[0].frames[0].IsSpecial()) glyph.shape->Decode(0, 0); // 라틴 글리프처럼 미리 해제해 둔다.
    return &extendedGlyphs_.emplace(codePoint, std::move(glyph)).first->second;
}
int BitmapFont::ExtendedAdvance() const {
    if (!HasExtended()) return 0;
    if (extendedAdvance_ < 0) { const auto* glyph = ExtendedGlyph(extended_.face.find("Noto Sans JP")!=std::string::npos ? 0x3042U : 0xac00U); extendedAdvance_ = glyph ? glyph->advance : 0; /* 11차 일본어: 일본어 글꼴은 히라가나 'あ'의 전각 폭을 대표로 쓴다 */ } // 모노스페이스라 대표 글자('가') 하나의 폭이 모든 확장 문자의 폭이다.
    return extendedAdvance_;
}
// 측정 경로는 그리기 경로와 달리 첫 번째 폭 표를 사용한다. 6차: 확장 설정이 있으면 4바이트 시퀀스를 한 글자 폭으로 센다.
int BitmapFont::Measure(std::string_view bytes) const {
    std::int64_t width = 0;
    const std::span<const std::uint8_t> raw(reinterpret_cast<const std::uint8_t*>(bytes.data()), bytes.size());
    // 코드 페이지 바이트를 부호 없이 색인한다.
    for (std::size_t position = 0; position < bytes.size(); ++position) {
        const unsigned char c = static_cast<unsigned char>(bytes[position]);
        if (c == o::kExtendedTextLead && HasExtended() && o::DecodeExtendedSequence(raw, position)) { width += ExtendedAdvance(); position += o::kExtendedTextLength - 1; continue; }
        width += glyphs_[c].advance;
    }
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
std::vector<std::uint8_t> BitmapFont::CacheBytes() const { // 같은 전진폭/기준점/그림과 원문 파일 레이아웃을 보존한다.
    std::vector<std::uint8_t> bytes(kPayloadStart);std::copy(kSignature.begin(),kSignature.end(),bytes.begin());Put(bytes,16,1);Put(bytes,20,std::uint32_t(height_));Put(bytes,24,std::uint32_t(ascent_));Put(bytes,28,std::uint32_t(descent_));Put(bytes,32,256);
    for (std::size_t code=0;code<glyphs_.size();++code) { // 같은 보관 VFX 블록을 읽기만 하며 그리기/재인코딩하지 않는다.
        const auto& glyph=glyphs_[code];const auto shape=glyph.shape ? glyph.shape->Bytes():std::span<const std::uint8_t>{};if (shape.size()>std::numeric_limits<std::uint32_t>::max()) throw std::overflow_error("글꼴 캐시 블록 길이 초과");
        Put(bytes,kTableStart+code*4,std::uint32_t(glyph.advance));Put(bytes,kTableStart+kTableBytes+code*4,std::uint32_t(glyph.bearing));Put(bytes,kTableStart+kTableBytes*2+code*4,std::uint32_t(glyph.drawAdvance));Put(bytes,kTableStart+kTableBytes*3+code*4,std::uint32_t(shape.size()));bytes.insert(bytes.end(),shape.begin(),shape.end());
    }
    return bytes;
}
namespace { // EnumFontFamiliesA의 두 번 열거와 원문32BYTE 행 계약을 Win32 경계에 한정한다.
// ANSI TrueType 후보를 두 번 읽어 최초 개수와 다음 기록 순서를 유지한다.
std::vector<std::array<char,32>> CollectFontFaces(const FontFaceEnumeration& enumerate) {
    std::size_t count{};enumerate([&](std::string_view,std::uint32_t type,std::uint8_t charset) { if (type==4 && !charset) ++count; });
    std::vector<std::array<char,32>> result(count);std::size_t written{};
    enumerate([&](std::string_view face,std::uint32_t type,std::uint8_t charset) {
        if (type!=4 || charset) return;if (written>=result.size()) throw std::runtime_error("원문 글꼴 두 번 열거 사이 후보 수 증가");
        face=face.substr(0,face.find('\0'));const auto size=std::min<std::size_t>(31,face.size());std::copy_n(face.begin(),size,result[written].begin());++written; //strncpy32 뒤 마지막BYTE0과 같은 유효 문자열이다.
    });
    if (written!=count) throw std::runtime_error("원문 글꼴 두 번 열거 사이 후보 수 감소");return result; //원문의 미작성/범위 밖 heap을 호스트에서 가짜 후보로 만들지 않는다.
}
// OS callback 안에서 예외를 넘기지 않고 Enum/ReleaseDC가 끝난 다음 같은 실패를 다시 던진다.
void EnumerateSystemFontFaces(const FontFaceVisitor& visitor) {
    struct Context { const FontFaceVisitor& visitor;std::exception_ptr error; }; //visitor는 이번 동기 Enum 동안 살아 있으며 최초 C++ 실패만 소유한다.
    Context context{visitor,{}};const auto dc=GetDC(nullptr);if (!dc) throw std::runtime_error("글꼴 목록의 화면 DC 열기 실패");
    const auto callback=+[](const LOGFONTA* font,const TEXTMETRICA*,DWORD type,LPARAM raw)->int {
        auto& current=*reinterpret_cast<Context*>(raw);
        try { current.visitor(font->lfFaceName,type,font->lfCharSet);return 1; }catch (...) { current.error=std::current_exception();return 0; }
    }; //원본 EnumFontFamiliesA가 제공한 순서를 정렬/중복 제거하지 않는다.
    EnumFontFamiliesA(dc,nullptr,callback,reinterpret_cast<LPARAM>(&context));ReleaseDC(nullptr,dc);if (context.error) std::rethrow_exception(context.error);
}
}
// 설정의 첫 NUL까지 보관하며 원문 고정 현재 이름은 별도31BYTE로 제한한다.
FontStore::FontStore(const o::BaseFileSystem& files,std::string face,FontFaceEnumeration enumerate)
    :files_(files),face_(face.empty() || face.front()=='\0' ? "Arial" : std::string(face.substr(0,face.find('\0')))),faces_(CollectFontFaces(enumerate ? enumerate : FontFaceEnumeration(EnumerateSystemFontFaces))) {
    std::copy_n(face_.begin(),std::min<std::size_t>(31,face_.size()),currentFace_.begin());
}
// 원문 음수는 assert 뒤 유효 영역 밖 접근 가능성이 있어 호스트는 진단 예외로 명시 거부한다.
std::optional<std::string_view> FontStore::FaceAt(std::int32_t index) const {
    if (index<0) throw std::out_of_range("원문 글꼴 which >= 0 관문");if (static_cast<std::size_t>(index)>=faces_.size()) return std::nullopt;
    const auto& face=faces_[static_cast<std::size_t>(index)];return std::string_view(face.data(),static_cast<std::size_t>(std::find(face.begin(),face.end(),'\0')-face.begin()));
}
// 모든 이전 글리프를 먼저 반납하고 같은 저장소 참조 아래18개 유효 캐시를 원문 순서로 다시 읽는다.
void FontStore::Rebuild(std::optional<std::string_view> requested) {
    for (auto& slot:fonts_) for (auto& font:slot) font.reset(); //원문4a39f0도7칸 전체 Clear를 새 글꼴 생성보다 먼저 수행한다.
    face_=requested ? std::string(requested->substr(0,requested->find('\0'))) : "Arial"; //유효 빈 이름은 NULL→Arial과 다르다.
    constexpr std::array<FontStyle,5> styles{FontStyle::Normal,FontStyle::Italic,FontStyle::Bold,FontStyle::Underline,FontStyle::Strikeout}; //4a34a0의 실제 캐시 조회 순서다.
    for (int slot=0;slot<7;++slot) for (const auto style:styles) if (Available(slot,style)) static_cast<void>(Get(slot,style)); //CHFNT가 없거나 손상된 조합만 기존 GDI 생성으로 내려간다.
    currentFace_.fill(0);std::copy_n(face_.begin(),std::min<std::size_t>(31,face_.size()),currentFace_.begin()); //생성/캐시가 모두 자연 반환한 뒤에만4a3cc4의 현재 이름을 쓴다.
}
// Config 기록은 실제 dirty 통지 뒤 이 현재값을 읽으며 GDI가 대체 선택한 물리 이름을 조회하지 않는다.
std::string_view FontStore::CurrentFace() const { return {currentFace_.data(),static_cast<std::size_t>(std::find(currentFace_.begin(),currentFace_.end(),'\0')-currentFace_.begin())}; }
// 캐시 내부의 tmHeight 대신 원본의 CreateFontA 요청 높이 전역을 읽는다.
int FontStore::LogicalHeight(int slot) {
    if (slot<0 || slot>=static_cast<int>(kHeights.size())) throw std::invalid_argument("Gump 글꼴 슬롯 범위 오류");
    return kHeights[static_cast<std::size_t>(slot)];
}
// 원본이 만들지 않는 조합을 GDI fallback으로 채우지 않는다.
bool FontStore::Available(int slot,FontStyle style) {
    return slot>=0 && slot<7 && slot!=2 && static_cast<std::size_t>(style)<kStyles.size() &&
        ((slot!=1 && slot!=3 && slot!=4) || style==FontStyle::Normal);
}
// 원본 슬롯·스타일 제한과 현재 캐시 읽기/생성 뒤 쓰기 순서를 유지한다. 보호 원본/쓰기불가 설치는 메모리 글꼴을 유지한다.
// 5차: 사용자 홈 정책의 글꼴 캐시 폴더를 연결한다. 폴더는 첫 저장에서만 만든다.
void FontStore::SetUserCacheDirectory(std::filesystem::path directory) {
    userCacheDirectory_=std::move(directory);userCache_=userCacheDirectory_.empty() ? nullptr:std::make_shared<o::BaseFileSystem>(userCacheDirectory_); //원문 경로 고리를 공유하지 않는 독립 장치다.
}
const BitmapFont& FontStore::Get(int slot, FontStyle style) {
    const auto index = static_cast<std::size_t>(style);
    if (!Available(slot,style)) throw std::invalid_argument("Unavailable font slot/style");
    auto& font = fonts_[static_cast<std::size_t>(slot)][index];
    if (!font) {
        const std::string face = slot == 1 ? "Courier New" : face_;
        const auto literal="!"+face+"."+kStyles[index]+"."+std::to_string(kHeights[slot])+"."+std::to_string(kWeights[slot])+".chfnt";const std::array<o::FileNamePart,2> parts{{{-2,"d"},{-3,literal}}};const std::string path(files_.FileNameOwner()->Format(parts));
        if (userCache_) { // 5차: 사용자 캐시를 먼저 읽는다(설치 폴더가 읽기 전용이어도 한 번 만든 글꼴 캐시를 재사용한다).
            try { if (const auto bytes=LoadNativeFontCache(*userCache_,literal)) font=std::make_unique<BitmapFont>(*bytes); }
            catch (const std::runtime_error&) { /* 손상된 사용자 캐시는 아래 설치 캐시/GDI 재생성으로 처리하고 지우지 않는다. */ }
        }
        try { if (!font) if (const auto bytes=LoadNativeFontCache(files_,path)) font=std::make_unique<BitmapFont>(*bytes); } // 설치에 함께 배포된 기존 d 캐시는 읽기 전용으로 계속 사용한다.
        catch (const std::runtime_error&) { /* 기존 호스트의 손상 캐시 물리/metrics 검증 실패는 같은 GDI 재생성으로 처리한다. */ }
        if (!font) {
            font=BitmapFont::Generate(face,kHeights[slot],style==FontStyle::Bold ? kWeights[slot]+(slot==0 ? 100:700):kWeights[slot],style);
            if (userCache_) { // 5차: 새 캐시는 사용자 폴더에만 쓴다. 쓰기 실패는 이미 만든 메모리 글꼴을 유지한다.
                try { platform::UserDataRoot::EnsureDirectory(userCacheDirectory_);static_cast<void>(SaveNativeFontCache(*userCache_,literal,font->CacheBytes())); }
                catch (const std::exception&) { /* 캐시는 성능용이므로 저장 실패가 게임 시작을 막지 않는다. */ }
            } else static_cast<void>(SaveNativeFontCache(files_,path,font->CacheBytes())); // 생성/설치가 완료된 뒤 같은 파일 순서로 저장한다.
        }
    }
    if (!extendedFace_.empty() && !font->HasExtended()) { // 6차: 한글 글꼴을 같은 높이/굵기/스타일로 맞춘다.
        const int weight = style == FontStyle::Bold ? kWeights[slot] + (slot == 0 ? 100 : 700) : kWeights[slot];
        font->ConfigureExtended({extendedFace_, extendedFontFile_, kHeights[slot], weight, style == FontStyle::Italic, style == FontStyle::Underline, style == FontStyle::Strikeout});
    }
    return *font;
}
// 6차: 한글 글꼴을 설정하고 이미 만든 글꼴에도 적용한다(다음 Get에서 명세를 다시 맞춘다).
void FontStore::SetExtendedFace(std::string face, std::filesystem::path fontFile) {
    extendedFace_ = std::move(face); extendedFontFile_ = std::move(fontFile);
    for (auto& row : fonts_) for (auto& font : row) if (font) font->ConfigureExtended({}); // 기존 명세를 비워 다음 Get이 새 글꼴로 다시 설정하게 한다.
}
}
