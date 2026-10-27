// 원본 Screen.cpp. 원본처럼 Win32 GDI를 직접 부른다(cpppj는 Windows 전용이다).
// 원본: 004a05a0(표면 생성) ↔ CD 00420f10, 004a0850(표면 해제), 004a08b0/004a0950(잠금), 004a0a60(복사) ↔ CD 004213b0,
//       004a1270(화면 초기화), 004a13d0(모드 검사), 004a14e0/004a1550(화면 잠금), 004a1580(자르기), 004a1800(창으로 복사),
//       004a2640(팔레트) ↔ CD 00424500, 004a40d0(커서 표시), 004a4570(DIB 섹션), 004a4fe0(화면 모드) ↔ CD 004220b0,
//       00445290(사각형 자르기).
// 범위: 창 모드(DIB 섹션) 경로와 가장 가까운 색 검색·파일 팔레트의 전체 색 표다. 글꼴·커서는 별도 실제 모듈에 연결했다.
// Windows10/11 호환성: 원본 DirectDraw 전체화면/flip 대신 같은 DIB의 비율 유지 borderless 출력을 사용한다.
#include "client/Screen.h"
#include "client/ScreenPresentation.h"
#include "client/ScreenPaletteApply.h"
#include <algorithm>
#include <bit>
#include <cstring>
#include <stdexcept>
#include <utility>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <dwmapi.h>

namespace netstorm::client {
namespace {
// 원본 표면의 복사 방식 기본값(+0x7c = 0xcc0020, SRCCOPY).
constexpr DWORD kCopyRasterOperation = SRCCOPY;
// 원본이 만드는 DIB 머리 정보: BITMAPINFOHEADER + 16비트 팔레트 번호 256개(DIB_PAL_COLORS).
struct DibHeader {
    // header: Win32 비트맵의 크기·형식을 지정하는 헤더.
    BITMAPINFOHEADER header;
    // paletteIndex: 팔레트 인덱스.
    WORD paletteIndex[256];
};
// 논리 팔레트 항목의 플래그(원본이 쓰는 4 = PC_NOCOLLAPSE).
constexpr BYTE kPaletteEntryFlags = PC_NOCOLLAPSE;
// 원본 LOGPALETTE의 버전(0x300)과 항목 수.
constexpr WORD kLogicalPaletteVersion = 0x300;
constexpr WORD kPaletteSize = 0x100;
// 색 검색의 초기 거리 상한이다(00509ce0 / CD 004246e3의 float32 0x4b189680). 모두 이 이상이면 0번을 반환한다.
constexpr float kInitialColorDistance=10000000.0f;
// 파일 팔레트 적용 뒤 채우는 원소별 RGB 색이다. 바람=노랑, 비=파랑, 천둥=빨강, 해=갈색 순서다.
// 원본: 004a4850의 DAT_005b5da8..005b5db4 / CD DAT_00549b40..00549b4c 대입.
constexpr std::array<std::array<std::int32_t,3>,4> kWeatherRgb{{{255,255,22},{22,22,255},{255,22,22},{181,140,111}}};

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
    bgrx_=bytes.size()==0x400;
    // 256개의 원본 색 번호를 변경하지 않고 변환한다.
    for (std::size_t i = 0; i < colors_.size(); ++i) {
        if (bytes.size() == 0x308) colors_[i] = {bytes[8+i*3], bytes[9+i*3], bytes[10+i*3]};
        else { colors_[i] = {bytes[i*4+2], bytes[i*4+1], bytes[i*4]};reserved_[i]=bytes[i*4+3]; }
    }
}
// 팔레트 내부에는 투명 색을 강제로 지정하지 않는다.
PaletteColor GamePalette::Color(std::uint8_t index) const { return colors_[index]; }
// RGB COL은 원본이 세 채널만 읽으므로 기존 저장 팔레트의 예약 바이트를 유지한다.
std::optional<std::uint8_t> GamePalette::Reserved(std::uint8_t index) const { return bgrx_ ? std::optional{reserved_[index]} : std::nullopt; }

// 실제 x87는 새 정수 거리를 반올림하기 전에 이전 float32 최솟값과 비교한다. 먼저 float로 바꾸면 큰 거리의 동률 처리에서 달라진다.
std::uint32_t FindPaletteColor(std::span<const std::uint32_t,256> logical,std::int32_t red,std::int32_t green,std::int32_t blue) {
    float best=kInitialColorDistance;
    std::uint32_t chosen=0;
    // 256색을 번호 순으로 검색한다. 플래그 바이트는 색 거리에 쓰지 않는다.
    for (std::size_t i=0;i<logical.size();++i) {
        const auto color=logical[i];
        const std::uint32_t dr=(color&255U)-static_cast<std::uint32_t>(red);
        const std::uint32_t dg=((color>>8)&255U)-static_cast<std::uint32_t>(green);
        const std::uint32_t db=((color>>16)&255U)-static_cast<std::uint32_t>(blue);
        const auto distance=std::bit_cast<std::int32_t>(dr*dr+dg*dg+db*db);
        if (static_cast<double>(distance)<static_cast<double>(best)) {
            best=static_cast<float>(distance);chosen=static_cast<std::uint32_t>(i);
        }
    }
    return chosen;
}

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
    // 원본 폭=간격 가정과 달리 Windows10/11의1366폭 등도 실제4BYTE DIB 행 정렬을 사용한다.
    std::fill_n(bits_, static_cast<std::size_t>(Pitch()) * static_cast<std::size_t>(height), std::uint8_t{0});
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
// 원본4:3 폭에서는 같지만 Windows10/11 호환 와이드 DIB의 실제 줄 간격은 DWORD 정렬이다.
int ScreenSurface::Pitch() const { return kind_==Kind::Dib ? (width_+3)&~3 : width_; }
// 호환 확대 출력은 같은8비트 DIB를 읽고 중첩 잠금/표면 수명을 바꾸지 않는다.
const std::uint8_t* ScreenSurface::Pixels() const { return bits_; }

// 창과 크기만 받아 둔다. 표면은 Init·InitDibSection에서 만든다.
Screen::Screen(NativeHandle window, NativeHandle windowDc, int width, int height,std::uint32_t* paletteLoaded)
    : paletteLoaded_(paletteLoaded?*paletteLoaded:localPaletteLoaded_),window_(window), windowDc_(windowDc), width_(width), height_(height) {}
// 표면을 먼저 없애고 GDI 객체를 푼다.
Screen::~Screen() {
    flags_&=~ScreenMode::kFullScreen;RestoreWindowed(); // 원본과 다른 borderless 자원/커서 제한을 장치 반납 전에 복원한다.
    flags_&=~ScreenMode::kSoftwareMouse;cursorHidden_=1;ShowCursor(true); // 호환 전체화면 종료 때 하드웨어 커서 표시도 반납한다.
    dibSurface_.reset();
    windowSurface_.reset();
    if (palette_) {
        if (previousPalette_) SelectPalette(static_cast<HDC>(windowDc_), static_cast<HPALETTE>(previousPalette_), FALSE);
        // GDI는 다른 DC에 선택된 팔레트도 삭제하지 못한다. 메모리 DC의 선택도 먼저 되돌린다.
        if (previousMemoryPalette_) SelectPalette(static_cast<HDC>(memoryDc_),static_cast<HPALETTE>(previousMemoryPalette_),FALSE);
        DeleteObject(static_cast<HPALETTE>(palette_));
    }
    if (memoryDc_) DeleteDC(static_cast<HDC>(memoryDc_));
}
// 원본004a1270의 DIB 장치다. Windows10/11 호환 전체화면은 ddraw.dll/독점 디스플레이 모드를 요구하지 않는다.
void Screen::Init() {
    windowSurface_ = std::make_unique<ScreenSurface>(ScreenSurface::Kind::DeviceContext, width_, height_, windowDc_, nullptr);
    memoryDc_ = CreateCompatibleDC(static_cast<HDC>(windowDc_));
    logical_.fill(0);
    // Win32 팔레트를 만들 때 전달하는 버전·색 개수·색 항목의 가변 길이 자료다.
    struct { /* version: 버전. */ WORD version; /* count: 현재 항목 또는 실행 횟수. */ WORD count; /* entries: 현재 등록하거나 읽은 항목 목록. */ PALETTEENTRY entries[256]; } logical{};
    logical.version = kLogicalPaletteVersion;
    logical.count = kPaletteSize;
    palette_ = CreatePalette(reinterpret_cast<LOGPALETTE*>(&logical));
    previousPalette_ = SelectPalette(static_cast<HDC>(windowDc_), static_cast<HPALETTE>(palette_), FALSE);
    previousMemoryPalette_=SelectPalette(static_cast<HDC>(memoryDc_),static_cast<HPALETTE>(palette_),FALSE);
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
    flags=ModernScreenMode(flags); // 원본과 다른 Windows10/11의 실제 DIB/비flip 모드로 정규화한다.
    lockDepth_ = 0;
    if (flags == flags_) return true;
    if (!ScreenModeIsLegal(flags,true) || !dibSurface_) return false;
    const bool wasFull=(flags_&ScreenMode::kFullScreen)!=0,wasSet=(flags_&ScreenMode::kModeSet)!=0;
    const HWND window=static_cast<HWND>(window_);if (!IsWindow(window)) return false;
    ShowCursor(false);
    saved_ = current_;
    drawSurface_=dibSurface_.get();primarySurface_=windowSurface_.get();flags_=flags;
    if (flags&ScreenMode::kFullScreen) {
        if (!wasFull) {
            RECT bounds{};GetWindowRect(window,&bounds);windowedStyle_=GetWindowLongPtrA(window,GWL_STYLE)&~(WS_MINIMIZE|WS_MAXIMIZE);
            if (wasSet) windowedBounds_={bounds.left,bounds.top,bounds.right,bounds.bottom};
            else { const auto initial=InitialWindowPosition(initWindowPos,GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN),windowWidth,windowHeight,position_);windowedBounds_={initial.x,initial.y,initial.x+windowWidth,initial.y+windowHeight}; }
            windowedSaved_=true;
        }
        // 원본 독점8비트 DirectDraw와 달리 현재 모니터의 데스크톱 모드/색 깊이를 그대로 유지한다.
        SetWindowLongPtrA(window,GWL_STYLE,WS_POPUP);SetMenu(window,nullptr);border_={};ShowWindow(window,SW_SHOWNORMAL);RelayoutFullscreen();
    } else if (wasFull && windowedSaved_) RestoreWindowed();
    else {
    LONG style = WS_OVERLAPPEDWINDOW;
    if (GetSystemMetrics(SM_CXSCREEN) == width_ && GetSystemMetrics(SM_CYSCREEN) == height_) {
        // 바탕 화면이 게임 화면과 같은 크기면 테두리 없는 창으로 화면을 덮는다.
        style = static_cast<LONG>(WS_POPUP);
        border_ = {0, 0};
    } else {
        border_.x = GetSystemMetrics(SM_CXFRAME);
        border_.y = GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYFRAME) + (menu_ ? GetSystemMetrics(SM_CYMENU) : 0);
    }
    SetWindowLongPtrA(window, GWL_STYLE, style);
    SetMenu(window, static_cast<HMENU>(menu_));
    position_ = InitialWindowPosition(initWindowPos, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
        windowWidth, windowHeight, position_);
    ShowWindow(window,SW_SHOWNORMAL);
    SetWindowPos(window,HWND_NOTOPMOST,position_.x,position_.y,windowWidth,windowHeight,SWP_FRAMECHANGED);
    }
    UpdateWindow(window);
    SetPalette(0, kPaletteSize, nullptr, true);
    cursorHidden_ = 1;
    ShowCursor(true);
    if (!drawSurface_) throw std::runtime_error("drawSurf"); // 원본 assert.
    ShowCursor(true);
    SetActive(GetForegroundWindow()==window);
    return true;
}
// 원본과 다른 호환 전체화면이 소유한 제한/창 스타일만 반납한다.
void Screen::RestoreWindowed() {
    if (cursorClipped_) { ClipCursor(nullptr);cursorClipped_=false; }
    const HWND window=static_cast<HWND>(window_);if (!windowedSaved_ || !IsWindow(window)) return;
    SetWindowLongPtrA(window,GWL_STYLE,windowedStyle_);SetMenu(window,static_cast<HMENU>(menu_));
    SetWindowPos(window,HWND_NOTOPMOST,windowedBounds_.left,windowedBounds_.top,windowedBounds_.right-windowedBounds_.left,windowedBounds_.bottom-windowedBounds_.top,SWP_FRAMECHANGED|SWP_NOACTIVATE);
    windowedSaved_=false;RECT bounds{};POINT origin{};GetWindowRect(window,&bounds);ClientToScreen(window,&origin);border_={origin.x-bounds.left,origin.y-bounds.top};
}
// 크기는 디스플레이/DPI/활성 변경 뒤에도 현재 Win32 클라이언트 영역에서 읽는다.
ScreenRect Screen::PresentationRect() const {
    if (!(flags_&ScreenMode::kFullScreen)) return {0,0,width_,height_};RECT client{};GetClientRect(static_cast<HWND>(window_),&client);
    if (client.right<=0 || client.bottom<=0) return {0,0,width_,height_};return FitScreenPresentation(width_,height_,client.right,client.bottom);
}
// 창 모드의 기존1:1 입력을 유지하며 호환 전체화면만 배율을 되돌린다.
ScreenPoint Screen::ToLogicalPoint(ScreenPoint point) const { return flags_&ScreenMode::kFullScreen ? MapScreenPresentationPoint(point,PresentationRect(),width_,height_) : point; }
// Win32 커서는 공유 자원이므로 다른 앱으로 전환하기 전에 반드시 제한을 해제한다.
void Screen::SetActive(bool active) {
    active_=active;const HWND window=static_cast<HWND>(window_);
    if (active_ && (flags_&ScreenMode::kFullScreen) && IsWindowVisible(window) && !IsIconic(window)) {
        const auto output=PresentationRect();POINT first{output.left,output.top},last{output.right,output.bottom};ClientToScreen(window,&first);ClientToScreen(window,&last);const RECT clip{first.x,first.y,last.x,last.y};cursorClipped_=ClipCursor(&clip)!=FALSE;
    } else if (cursorClipped_) { ClipCursor(nullptr);cursorClipped_=false; }
}
// 원본 디스플레이 모드 변경과 달리 현재 모니터 전체를 덮고 논리 화면은 다시 만들지 않는다.
void Screen::RelayoutFullscreen() {
    if (!(flags_&ScreenMode::kFullScreen)) return;const HWND window=static_cast<HWND>(window_);MONITORINFO monitor{sizeof(MONITORINFO)};
    if (!GetMonitorInfoA(MonitorFromWindow(window,MONITOR_DEFAULTTONEAREST),&monitor)) throw std::runtime_error("전체화면 모니터 조회 실패");
    SetWindowPos(window,HWND_TOP,monitor.rcMonitor.left,monitor.rcMonitor.top,monitor.rcMonitor.right-monitor.rcMonitor.left,monitor.rcMonitor.bottom-monitor.rcMonitor.top,SWP_FRAMECHANGED|SWP_NOACTIVATE);SetActive(active_);
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
        // 반복 날씨/모드 전환마다 이전 팔레트가 남지 않도록 두 DC 모두에서 선택을 해제한다.
        if (previousMemoryPalette_) SelectPalette(static_cast<HDC>(memoryDc_),static_cast<HPALETTE>(previousMemoryPalette_),FALSE);
        DeleteObject(static_cast<HPALETTE>(palette_));
    }
    // Win32 팔레트를 만들 때 전달하는 버전·색 개수·색 항목의 가변 길이 자료다.
    struct { /* version: 버전. */ WORD version; /* count: 현재 항목 또는 실행 횟수. */ WORD count; /* entries: 현재 등록하거나 읽은 항목 목록. */ PALETTEENTRY entries[256]; } logical{};
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
    // 원본8비트 독점 팔레트와 달리 현대 데스크톱은 이미 출력한 RGB 픽셀도 다시 칠해야 번개/날씨가 전체에 적용된다.
    if ((flags_&ScreenMode::kFullScreen) && drawSurface_) Update({0,0,width_,height_});
}
// 원본 004a4850: 파일의 256색을 저장 팔레트(DAT_005acd28)에 RGBQUAD 순서로 읽고 FUN_004a2640(0, 256, 0, 1)로 적용한다.
// clear비영은현재GDI의실제지우기/viewport복사를팔레트적용전에실행한다.기존호출의default0은그대로다.
void Screen::LoadPalette(const GamePalette& palette,o::OriginalEdition edition,std::uint32_t clear) {
    // 색 번호를 그대로 유지한다.
    for (std::size_t i = 0; i < saved_.size(); ++i) {
        const auto color = palette.Color(static_cast<std::uint8_t>(i));
        const auto reserved=palette.Reserved(static_cast<std::uint8_t>(i)).value_or(saved_[i].reserved);
        saved_[i] = {color.blue, color.green, color.red, reserved};
    }
    const ScreenPaletteApplyHooks devices{
        [this] { Lock(); }, //같은현재깊이/표면을사용한다.
        [this] { if ((flags_&ScreenMode::kModeSet)==0) throw std::logic_error("팔레트전환Surface.fillBlt미복원");FillRect({0,0,width_,height_},0); }, //bit80없으면원문별도fillBlt경계를빈성공으로가장하지않는다.
        [this] { Unlock(); }, //중단시이전부분효과를되돌리지않는다.
        [this] { WaitPaletteVerticalBlank(); }, //GDI자연관문뒤에만표시로간다.
        [this] { PresentPaletteClear(); }, //이전clip을viewport복사뒤복구한다.
        [this] { FlipPaletteClear(); } //bit4가없을때만자연생략한다.
    }; //원문loaded1과clear관문뒤의실제필수장치묶음이다.
    ApplyScreenPaletteClear(clear,flags_,paletteLoaded_,devices);
    SetPalette(0, kPaletteSize, nullptr, true);
    colors_=BuildPaletteColorTable(logical_,edition);
    shadeMaps_=BuildGumpShadeMaps(colorSortInput_ ? colorSortInput_() : std::span<const std::uint32_t,256>(logical_));
}
// 원본과 다른 Windows10/11 DIB 전체화면은 독점 DirectDraw vblank 대신 현재 GDI/데스크톱 합성을 동기화한다.
void Screen::WaitPaletteVerticalBlank() {
    if ((flags_&ScreenMode::kFullScreen)!=0) { GdiFlush();DwmFlush(); }
}
// 현재구현의viewport는SetMode에서정한0,0,width,height이다.004a46d0은Update가바꾼clip을복구한다.
void Screen::PresentPaletteClear() {
    const auto previous=clip_; //실제현재clip을Update앞에캡처한다.
    Update({0,0,width_,height_});
    clip_=previous;
}
// 원문004a16d0은bit4가0이면자연반환한다.현재GDI의합법모드는이관문을그대로지나간다.
void Screen::FlipPaletteClear() {
    if ((flags_&ScreenMode::kFlipping)!=0) throw std::logic_error("팔레트전환DirectDraw flip장치미복원");
}
// Windows10/11 호환 모드는 원본 독점 DirectDraw flip 대신 DIB를 사용하므로 bit4의 자연 반환을 공유한다.
void Screen::Flip() { FlipPaletteClear(); }
// 원본 DIB 복사를 그대로 사용한다. 호환 전체화면의 실제 창 확대 출력은 다음 Present에서 같은 완성 프레임을 내보낸다.
void Screen::Scroll(int x,int y,int width,int height,int dx,int dy) {
    if (!(flags_&ScreenMode::kBackBufferDib)) throw std::logic_error("화면 이동의 현재 DIB 장치 필요");
    if (!dx && !dy) return;auto* pixels=Lock();
    try { ScrollScreenPixels(std::span<std::uint8_t>(pixels,static_cast<std::size_t>(Pitch())*height_),Pitch(),width_,height_,x,y,width,height,dx,dy); }catch (...) { Unlock();throw; }
    Unlock();
}
// 실제paletteLoaded의참조를돌려주며별도bool사본을만들지않는다.
const std::uint32_t& Screen::PaletteLoaded() const { return paletteLoaded_; }
// 원본처럼 파일 읽기가 끝난 뒤 계산된 표를 보존한다. 번개의 부분 적용/복구는 표를 다시 만들지 않는다.
const PaletteColorTable& Screen::Colors() const { return colors_; }
// GUI 어댑터는 마지막 파일에서 만든 표를 참조한다. 원본 paletteDirty 소비 수명은 후속 복원 대상이다.
const GumpShadeMaps& Screen::ShadeMaps() const { return shadeMaps_; }
std::span<const std::uint32_t,256> Screen::LogicalPalette() const { return logical_; } //논리팔레트를현재화면RGB사본으로바꾸지않는다.
void Screen::BindColorSortInput(std::function<std::span<const std::uint32_t,256>()> current) { if (!current) throw std::invalid_argument("Screen 고정ColorSort 입력조회기누락");colorSortInput_=std::move(current); }
// apply와 무관하게 논리 팔레트가 색 검색의 원본이다.
std::uint32_t Screen::FindColor(std::int32_t red,std::int32_t green,std::int32_t blue) const {
    return FindPaletteColor(logical_,red,green,blue);
}
// 팔레트 로더의 노랑·파랑·빨강·갈색 대입 순서다. tint 자체는 장면 음악 감독이 원본 순서로 변경한다.
std::array<std::uint32_t,4> Screen::WeatherTints() const {
    std::array<std::uint32_t,4> result{};
    // 각 원소의 RGB에 가장 가까운 현재 팔레트 번호를 기록한다.
    for (std::size_t i=0;i<result.size();++i) result[i]=FindColor(kWeatherRgb[i][0],kWeatherRgb[i][1],kWeatherRgb[i][2]);
    return result;
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
// 구름 표시 부모가 실제 네 clip 값을 캡처한 뒤 같은 장치에 복구한다.
ScreenRect Screen::Clip() const { return clip_; }
// 004a0c10의 DIB 경로는 sourceLTRB를 대상 크기로 바꾼 뒤 화면/현재 clip으로 자르고4BYTE/나머지를 앞으로 복사한다.
void Screen::BlitBuffer(int x,int y,std::span<const std::uint8_t> source,int sourcePitch,ScreenRect sourceRect) {
    const auto add=[](int first,int second) { return std::bit_cast<std::int32_t>(std::uint32_t(first)+std::uint32_t(second)); }; // 같은 DWORD 감김 좌표다.
    const auto subtract=[](int first,int second) { return std::bit_cast<std::int32_t>(std::uint32_t(first)-std::uint32_t(second)); }; // signed overflow를 C++ 미정의 동작으로 만들지 않는다.
    const auto clip=ClipScreenRect(clip_,{0,0,width_,height_});const auto target=ClipScreenRect({x,y,add(x,subtract(sourceRect.right,sourceRect.left)),add(y,subtract(sourceRect.bottom,sourceRect.top))},clip);
    if (!buffer_) throw std::logic_error("구름 buffer 복사의 실제 잠금 누락");const auto width=subtract(target.right,target.left),height=subtract(target.bottom,target.top);const auto pitch=Pitch();
    if (width<0 || height<0 || sourcePitch<0) throw std::out_of_range("구름 buffer 복사의 현재 물리 폭");
    const auto first=std::int64_t(add(sourceRect.top,subtract(target.top,y)))*sourcePitch+add(sourceRect.left,subtract(target.left,x)); // 자르기 뒤 원문 source 시작 위치다.
    for (int row=0;row<height;++row) { // 행마다 현재 원시 source와 같은 실제 잠긴 DIB를 사용한다.
        const auto sourceRow=first+std::int64_t(row)*sourcePitch,destinationRow=std::int64_t(target.top+row)*pitch+target.left;
        for (int column=0;column<width;) { // Native의 DWORD 앞으로 복사와 마지막0~3BYTE를 같은 순서로 실행한다.
            const auto bytes=width-column>=4 ? 4 : 1;const auto from=sourceRow+column,to=destinationRow+column;
            if (from<0 || std::uint64_t(from)+bytes>source.size() || to<0 || std::uint64_t(to)+bytes>std::uint64_t(pitch)*height_) throw std::out_of_range("구름 buffer 복사의 실제 물리 범위");
            std::array<std::uint8_t,4> word{};std::copy_n(source.data()+from,bytes,word.data());std::copy_n(word.data(),bytes,buffer_+to);column+=bytes; // 겹친 입력도 먼저 해당 DWORD를 읽은 뒤 쓴다.
        }
    }
}
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
    if (rect.right<=rect.left || rect.bottom<=rect.top) return;
    if (flags_&ScreenMode::kFullScreen) {
        const HWND window=static_cast<HWND>(window_);
        if (!active_ || IsIconic(window) || !IsWindowVisible(window) || !dibSurface_) return;
        const auto output=PresentationRect();const HDC dc=static_cast<HDC>(windowDc_);
        RECT client{};GetClientRect(window,&client);
        // 원본과 다른 호환 확대의 레터박스만 검정으로 칠한다. 실제 비정수 GDI clip의 표본 위상 차이 때문에 전체 출력만 사용한다.
        const RECT bars[]={{0,0,client.right,output.top},{0,output.bottom,client.right,client.bottom},{0,output.top,output.left,output.bottom},{output.right,output.top,client.right,output.bottom}};
        for (const auto& bar:bars) if (bar.right>bar.left && bar.bottom>bar.top) ::FillRect(dc,&bar,static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH))); // 네 바깥 띠만 갱신한다.
        // 원본과 다른 호환 확대는 실제 표시DC와 메모리DC의 비정수 GDI 표본 차이를 피하도록 CPU 정수 최근접 복사 뒤1:1로 출력한다.
        struct OutputBitmap { BITMAPINFOHEADER header{}; /* header: 실제 DIB 크기/형식이다. */ RGBQUAD colors[256]{}; /* colors: 현재 출력256색이다. */ }; // 호환 출력 머리와 색 표다.
        const int outputWidth=output.right-output.left,outputHeight=output.bottom-output.top,outputPitch=(outputWidth+3)&~3;
        OutputBitmap bitmap;bitmap.header.biSize=sizeof(BITMAPINFOHEADER);bitmap.header.biWidth=outputWidth;bitmap.header.biHeight=-outputHeight;bitmap.header.biPlanes=1;bitmap.header.biBitCount=8;bitmap.header.biCompression=BI_RGB;
        std::memcpy(bitmap.colors,current_.data(),sizeof bitmap.colors);
        std::vector<std::uint8_t> pixels(static_cast<std::size_t>(outputPitch)*outputHeight);
        ScaleScreenPresentation({dibSurface_->Pixels(),static_cast<std::size_t>(dibSurface_->Pitch())*height_},dibSurface_->Pitch(),width_,height_,pixels,outputPitch,outputWidth,outputHeight);
        SetDIBitsToDevice(dc,output.left,output.top,outputWidth,outputHeight,0,0,0,outputHeight,pixels.data(),reinterpret_cast<const BITMAPINFO*>(&bitmap),DIB_RGB_COLORS);return;
    }
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
