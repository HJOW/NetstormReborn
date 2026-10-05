// 원본 Screen.cpp. 원본처럼 Win32 GDI를 직접 부른다(cpppj는 Windows 전용이다).
// 원본: 004a05a0(표면 생성) ↔ CD 00420f10, 004a0850(표면 해제), 004a08b0/004a0950(잠금), 004a0a60(복사) ↔ CD 004213b0,
//       004a1270(화면 초기화), 004a13d0(모드 검사), 004a14e0/004a1550(화면 잠금), 004a1580(자르기), 004a1800(창으로 복사),
//       004a2640(팔레트) ↔ CD 00424500, 004a40d0(커서 표시), 004a4570(DIB 섹션), 004a4fe0(화면 모드) ↔ CD 004220b0,
//       00445290(사각형 자르기).
// 범위: 창 모드(DIB 섹션) 경로. DirectDraw 표면(종류 3·4·5), 전체화면 전환, 플리핑, 구름 시차 표면,
//       색 찾기 표는 아직 옮기지 않았다. 글꼴·소프트웨어 커서는 BitmapFont·Cursor·Renderer에 연결했다.
#include "client/Screen.h"
#include <algorithm>
#include <stdexcept>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

namespace netstorm::client {
namespace {
// 원본 표면의 복사 방식 기본값(+0x7c = 0xcc0020, SRCCOPY).
constexpr DWORD kCopyRasterOperation = SRCCOPY;
// 원본이 만드는 DIB 머리 정보: BITMAPINFOHEADER + 16비트 팔레트 번호 256개(DIB_PAL_COLORS).
struct DibHeader {
    BITMAPINFOHEADER header;
    WORD paletteIndex[256];
};
// 논리 팔레트 항목의 플래그(원본이 쓰는 4 = PC_NOCOLLAPSE).
constexpr BYTE kPaletteEntryFlags = PC_NOCOLLAPSE;
// 원본 LOGPALETTE의 버전(0x300)과 항목 수.
constexpr WORD kLogicalPaletteVersion = 0x300;
constexpr WORD kPaletteSize = 0x100;

// 원본 FUN_00445290의 좌표 하나: 값을 [low, high] 안으로 넣는다.
int Clamp(int value, int low, int high) {
    const int limited = std::min(value, high);
    return low > limited ? low : limited;
}
}

// 원본 팔레트 로더는 COL 헤더 내용보다 파일 크기로 포맷을 결정한다.
// COL은 RGB 순서이고 1024바이트 파일은 Windows RGBQUAD의 BGRX 순서다.
GamePalette::GamePalette(std::span<const std::uint8_t> bytes) {
    if (bytes.size() != 0x308 && bytes.size() != 0x400) throw std::runtime_error("Unsupported game palette size");
    // 256개의 원본 색 번호를 변경하지 않고 변환한다.
    for (std::size_t i = 0; i < colors_.size(); ++i) {
        if (bytes.size() == 0x308) colors_[i] = {bytes[8+i*3], bytes[9+i*3], bytes[10+i*3]};
        else colors_[i] = {bytes[i*4+2], bytes[i*4+1], bytes[i*4]};
    }
}
// 팔레트 내부에는 투명 색을 강제로 지정하지 않는다.
PaletteColor GamePalette::Color(std::uint8_t index) const { return colors_[index]; }

// 원본 004a13d0. 원본은 맞지 않는 조건마다 "Illegal screen mode: …" 로그를 남기고 개수를 센다.
bool ScreenModeIsLegal(std::uint32_t flags, bool directDrawInstalled) {
    int problems = 0;
    if (!directDrawInstalled && (flags & ScreenMode::kFullScreen) != 0) ++problems;                        // DirectDraw without being installed
    if ((flags & ScreenMode::kFlipping) != 0 && (flags & ScreenMode::kFullScreen) == 0) ++problems;       // flipping without DirectDraw
    if ((flags & ScreenMode::kParallax) != 0 && (flags & ScreenMode::kClouds) == 0) ++problems;           // parallax without clouds
    if ((flags & ScreenMode::kAnimating) != 0 && (flags & ScreenMode::kClouds) == 0) ++problems;          // animating without clouds
    if ((flags & (ScreenMode::kFullScreen | ScreenMode::kBackBufferDib)) == 0) ++problems;                // no DirectDraw and no DIB
    return problems == 0;
}
// 네 좌표를 각각 경계 안으로 넣는다.
ScreenRect ClipScreenRect(ScreenRect rect, ScreenRect bounds) {
    return {Clamp(rect.left, bounds.left, bounds.right), Clamp(rect.top, bounds.top, bounds.bottom),
            Clamp(rect.right, bounds.left, bounds.right), Clamp(rect.bottom, bounds.top, bounds.bottom)};
}
// 원본의 switch(DAT_0054db40) 그대로다.
ScreenPoint InitialWindowPosition(int initWindowPos, int desktopWidth, int desktopHeight,
    int windowWidth, int windowHeight, ScreenPoint current) {
    switch (initWindowPos) {
    case 1: return {0, 0};
    case 2: return {desktopWidth - windowWidth, 0};
    case 3: return {desktopWidth - windowWidth, desktopHeight - windowHeight};
    case 4: return {0, desktopHeight - windowHeight};
    case 5: return {(desktopWidth - windowWidth) / 2, (desktopHeight - windowHeight) / 2};
    default: return current;
    }
}

// 원본 004a05a0의 종류 1·2.
ScreenSurface::ScreenSurface(Kind kind, int width, int height, NativeHandle dc, NativeHandle referenceDc)
    : kind_(kind), width_(width), height_(height), dc_(dc) {
    if (kind_ != Kind::Dib) return;
    DibHeader info{};
    info.header.biSize = sizeof(BITMAPINFOHEADER);
    info.header.biWidth = width;
    info.header.biHeight = -height; // 위에서 아래로.
    info.header.biPlanes = 1;
    info.header.biBitCount = 8;
    info.header.biClrUsed = kPaletteSize;
    info.header.biClrImportant = kPaletteSize;
    // 색 표는 장치 문맥의 논리 팔레트 번호 0~255다.
    for (WORD i = 0; i < kPaletteSize; ++i) info.paletteIndex[i] = i;
    void* bits = nullptr;
    bitmap_ = CreateDIBSection(static_cast<HDC>(referenceDc), reinterpret_cast<BITMAPINFO*>(&info), DIB_PAL_COLORS, &bits, nullptr, 0);
    bits_ = static_cast<std::uint8_t*>(bits);
    if (!bits_) throw std::runtime_error("dibBuffer"); // 원본 assert.
    std::fill_n(bits_, static_cast<std::size_t>(width) * static_cast<std::size_t>(height), std::uint8_t{0});
    if (dc_) SelectObject(static_cast<HDC>(dc_), static_cast<HBITMAP>(bitmap_));
}
// 원본은 비트맵을 지우고 머리 정보 메모리를 푼다(여기서는 머리 정보를 따로 잡아 두지 않는다).
ScreenSurface::~ScreenSurface() {
    if (bitmap_) DeleteObject(static_cast<HBITMAP>(bitmap_));
}
// GDI 표면은 가진 장치 문맥을 그대로 돌려준다.
NativeHandle ScreenSurface::Dc() const { return dc_; }
// DIB는 버퍼를, 창 표면은 널을 돌려준다.
std::uint8_t* ScreenSurface::Lock() {
    locked_ = kind_ == Kind::Dib ? bits_ : nullptr;
    return locked_;
}
// GDI 표면은 풀 것이 없다.
void ScreenSurface::Unlock() { locked_ = nullptr; }
// 폭 = right - left, 높이 = bottom - top.
void ScreenSurface::BlitFrom(int x, int y, const ScreenSurface& source, int left, int top, int right, int bottom) {
    BitBlt(static_cast<HDC>(Dc()), x, y, right - left, bottom - top, static_cast<HDC>(source.Dc()), left, top, kCopyRasterOperation);
}
// 표면의 폭.
int ScreenSurface::Width() const { return width_; }
// 표면의 높이.
int ScreenSurface::Height() const { return height_; }
// 원본은 줄 간격에 폭을 그대로 넣는다(화면 폭 640·800·1024는 4의 배수라 DIB의 실제 줄 간격과 같다).
int ScreenSurface::Pitch() const { return width_; }

// 창과 크기만 받아 둔다. 표면은 Init·InitDibSection에서 만든다.
Screen::Screen(NativeHandle window, NativeHandle windowDc, int width, int height)
    : window_(window), windowDc_(windowDc), width_(width), height_(height) {}
// 표면을 먼저 없애고 GDI 객체를 푼다.
Screen::~Screen() {
    dibSurface_.reset();
    windowSurface_.reset();
    if (palette_) {
        if (previousPalette_) SelectPalette(static_cast<HDC>(windowDc_), static_cast<HPALETTE>(previousPalette_), FALSE);
        DeleteObject(static_cast<HPALETTE>(palette_));
    }
    if (memoryDc_) DeleteDC(static_cast<HDC>(memoryDc_));
}
// 원본 004a1270. 원본은 여기서 ddraw.dll의 DirectDrawCreate를 찾아 "DirectDraw 있음"(DAT_005c7910)을 정한다.
// DirectDraw 경로를 아직 옮기지 않아 그 표시는 항상 꺼 둔다(SetMode가 원본 규칙대로 창 모드로 바꾼다).
void Screen::Init() {
    windowSurface_ = std::make_unique<ScreenSurface>(ScreenSurface::Kind::DeviceContext, width_, height_, windowDc_, nullptr);
    memoryDc_ = CreateCompatibleDC(static_cast<HDC>(windowDc_));
    logical_.fill(0);
    struct { WORD version; WORD count; PALETTEENTRY entries[256]; } logical{};
    logical.version = kLogicalPaletteVersion;
    logical.count = kPaletteSize;
    palette_ = CreatePalette(reinterpret_cast<LOGPALETTE*>(&logical));
    previousPalette_ = SelectPalette(static_cast<HDC>(windowDc_), static_cast<HPALETTE>(palette_), FALSE);
    SelectPalette(static_cast<HDC>(memoryDc_), static_cast<HPALETTE>(palette_), FALSE);
}
// 원본 004a4570.
void Screen::InitDibSection() {
    dibSurface_ = std::make_unique<ScreenSurface>(ScreenSurface::Kind::Dib, width_, height_, memoryDc_, windowDc_);
    flags_ = 0;
    buffer_ = dibSurface_->Lock();
    clip_ = {};
    menu_ = GetMenu(static_cast<HWND>(window_));
}
// 원본 004a4fe0의 창 모드 경로.
bool Screen::SetMode(std::uint32_t flags, int windowWidth, int windowHeight, int initWindowPos) {
    flags |= ScreenMode::kModeSet;
    lockDepth_ = 0;
    if (flags == flags_) return true;
    // DirectDraw가 없으면 전체화면 요구를 창 모드 값으로 바꾼다(원본 `param_1 = 10`).
    if ((flags & ScreenMode::kFullScreen) != 0) flags = ScreenMode::kFallbackWindowed;
    if (!ScreenModeIsLegal(flags, false)) return false;
    ShowCursor(false);
    if ((flags & ScreenMode::kFullScreen) == 0) flags &= ~ScreenMode::kSoftwareMouse;
    saved_ = current_;
    flags_ = 0;
    const HWND window = static_cast<HWND>(window_);
    LONG style = WS_OVERLAPPEDWINDOW;
    if (GetSystemMetrics(SM_CXSCREEN) == width_ && GetSystemMetrics(SM_CYSCREEN) == height_) {
        // 바탕 화면이 게임 화면과 같은 크기면 테두리 없는 창으로 화면을 덮는다.
        style = static_cast<LONG>(WS_POPUP);
        border_ = {0, 0};
    } else {
        border_.x = GetSystemMetrics(SM_CXFRAME);
        border_.y = GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYFRAME) + (menu_ ? GetSystemMetrics(SM_CYMENU) : 0);
    }
    SetWindowLongA(window, GWL_STYLE, style);
    SetMenu(window, static_cast<HMENU>(menu_));
    position_ = InitialWindowPosition(initWindowPos, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
        windowWidth, windowHeight, position_);
    SetWindowPos(window, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);
    ShowWindow(window, SW_SHOWMAXIMIZED);
    MoveWindow(window, position_.x, position_.y, windowWidth, windowHeight, TRUE);
    UpdateWindow(window);
    if (!dibSurface_) return false;
    drawSurface_ = dibSurface_.get();
    primarySurface_ = windowSurface_.get();
    flags_ = flags;
    SetPalette(0, kPaletteSize, nullptr, true);
    cursorHidden_ = 1;
    ShowCursor(true);
    if (!drawSurface_) throw std::runtime_error("drawSurf"); // 원본 assert.
    ShowCursor(true);
    return true;
}
// 원본 004a2640.
void Screen::SetPalette(unsigned start, unsigned count, ScreenColor* colors, bool apply) {
    if (!colors) colors = saved_.data();
    const unsigned end = start + count;
    // 논리 팔레트 항목: 빨강·초록·파랑·플래그 순서.
    for (unsigned i = start; i < end && i < kPaletteSize; ++i) {
        const auto& color = colors[i - start];
        logical_[i] = static_cast<std::uint32_t>(color.red) | (static_cast<std::uint32_t>(color.green) << 8)
            | (static_cast<std::uint32_t>(color.blue) << 16) | (static_cast<std::uint32_t>(kPaletteEntryFlags) << 24);
    }
    // 0번은 검정, 255번은 흰색으로 고정한다(플래그 없음).
    logical_[0] = 0;
    logical_[255] = 0x00ffffff;
    if (start == 0) colors[0] = {};
    // 원본은 colors[255]에 쓴다(start가 0이라고 가정한 코드). 부분 배열을 넘겨도 범위를 넘지 않게 끝 항목에 쓴다.
    if (end == kPaletteSize) colors[kPaletteSize - 1 - start] = {0xff, 0xff, 0xff, 0};
    if (!apply) return;
    if (count == 0) { start = 0; count = kPaletteSize; }
    // 현재 팔레트에 복사한다.
    for (unsigned i = 0; i < count && start + i < kPaletteSize; ++i) current_[start + i] = colors[i];
    // 창 모드: DIB 색 표를 바꾸고 논리 팔레트를 새로 만들어 두 장치 문맥에 고른다.
    SetDIBColorTable(static_cast<HDC>(memoryDc_), start, count, reinterpret_cast<const RGBQUAD*>(colors));
    const HDC windowDc = static_cast<HDC>(windowDc_);
    if (palette_) {
        SelectPalette(windowDc, static_cast<HPALETTE>(previousPalette_), FALSE);
        DeleteObject(static_cast<HPALETTE>(palette_));
    }
    struct { WORD version; WORD count; PALETTEENTRY entries[256]; } logical{};
    logical.version = kLogicalPaletteVersion;
    logical.count = kPaletteSize;
    // 항목은 메모리 배치가 PALETTEENTRY와 같다.
    for (unsigned i = 0; i < kPaletteSize; ++i) {
        logical.entries[i] = {static_cast<BYTE>(logical_[i]), static_cast<BYTE>(logical_[i] >> 8),
                              static_cast<BYTE>(logical_[i] >> 16), static_cast<BYTE>(logical_[i] >> 24)};
    }
    palette_ = CreatePalette(reinterpret_cast<LOGPALETTE*>(&logical));
    previousPalette_ = SelectPalette(windowDc, static_cast<HPALETTE>(palette_), FALSE);
    SelectPalette(static_cast<HDC>(memoryDc_), static_cast<HPALETTE>(palette_), FALSE);
    RealizePalette(windowDc);
}
// 원본 004a4850: 파일의 256색을 저장 팔레트(DAT_005acd28)에 RGBQUAD 순서로 읽고 FUN_004a2640(0, 256, 0, 1)로 적용한다.
// 화면 모드가 이미 정해진 뒤 부르면 원본은 먼저 화면을 지운다 — 그 부분과 이름 붙은 색 찾기(004a2820)는 아직 옮기지 않았다.
void Screen::LoadPalette(const GamePalette& palette) {
    // 색 번호를 그대로 유지한다.
    for (std::size_t i = 0; i < saved_.size(); ++i) {
        const auto color = palette.Color(static_cast<std::uint8_t>(i));
        saved_[i] = {color.blue, color.green, color.red, 0};
    }
    SetPalette(0, kPaletteSize, nullptr, true);
}
// 처음 잠글 때만 그리기 표면에서 버퍼를 얻는다.
std::uint8_t* Screen::Lock() {
    if (lockDepth_++ == 0) {
        if (!drawSurface_) throw std::runtime_error("drawSurf"); // 원본 assert.
        buffer_ = drawSurface_->Lock();
    }
    return buffer_;
}
// 마지막 잠금이 풀릴 때 표면을 푼다.
void Screen::Unlock() {
    if (--lockDepth_ == 0) {
        drawSurface_->Unlock();
        buffer_ = nullptr;
    }
}
// 원본은 right·bottom에서 1을 뺀 값을 VFX pane에 넣는다.
void Screen::SetClip(int left, int top, int right, int bottom) { clip_ = {left, top, right, bottom}; }
// 원본 004a2060의 "모드가 정해진 뒤" 경로: 줄마다 같은 색 바이트로 채운다.
void Screen::FillRect(ScreenRect rect, std::uint8_t color) {
    rect = ClipScreenRect(rect, clip_);
    if ((flags_ & ScreenMode::kModeSet) == 0 || !buffer_) return;
    const int pitch = Pitch();
    // 위에서 아래로 한 줄씩 채운다.
    for (int y = rect.top; y < rect.bottom; ++y)
        std::fill_n(buffer_ + static_cast<std::ptrdiff_t>(pitch) * y + rect.left, std::max(rect.right - rect.left, 0), color);
}
// 원본 004a4790. 뒤따르는 FUN_004a1fb0·FUN_004a46d0(내용 미확인)은 옮기지 않았다.
void Screen::Clear(ScreenRect rect, std::uint8_t color) {
    if (flags_ == 0) return;
    Lock();
    FillRect(rect, color);
    Unlock();
}
// 원본 004a1800: 화면 사각형으로 자른 뒤 자르기 영역을 화면 전체로 되돌리고 창으로 복사한다.
void Screen::Update(ScreenRect rect) {
    const ScreenRect screen{0, 0, width_, height_};
    rect = ClipScreenRect(rect, screen);
    clip_ = screen;
    if (primarySurface_ && drawSurface_) primarySurface_->BlitFrom(rect.left, rect.top, *drawSurface_, rect.left, rect.top, rect.right, rect.bottom);
}
// 원본 004a40d0: 숨김 횟수에 맞춰 Windows의 커서 표시 횟수를 0(보임) 또는 -1(숨김)로 맞춘다.
void Screen::ShowCursor(bool show) {
    cursorHidden_ += show ? -1 : 1;
    if ((flags_ & ScreenMode::kSoftwareMouse) == 0 && cursorHidden_ < 1) {
        // 보임: 표시 횟수를 1 이상으로 올렸다가 0까지 내린다.
        while (::ShowCursor(TRUE) < 1) {}
        // 정확히 0으로 맞춘다.
        while (::ShowCursor(FALSE) > 0) {}
        return;
    }
    // 숨김: -2 이하로 내렸다가 -1까지 올린다.
    while (::ShowCursor(FALSE) > -2) {}
    // 정확히 -1로 맞춘다.
    while (::ShowCursor(TRUE) < -1) {}
}
// 현재 화면 모드 플래그.
std::uint32_t Screen::Flags() const { return flags_; }
// 화면 폭.
int Screen::Width() const { return width_; }
// 화면 높이.
int Screen::Height() const { return height_; }
// 그리기 표면의 줄 간격.
int Screen::Pitch() const { return dibSurface_ ? dibSurface_->Pitch() : width_; }
// 자르기 영역을 양 끝 포함 좌표로 돌려준다.
ScreenRect Screen::Pane() const { return {clip_.left, clip_.top, clip_.right - 1, clip_.bottom - 1}; }
// 현재 팔레트.
std::span<const ScreenColor> Screen::Palette() const { return current_; }
// 창 테두리 두께.
ScreenPoint Screen::BorderOffset() const { return border_; }
// 창의 장치 문맥.
NativeHandle Screen::WindowDc() const { return windowDc_; }
}
