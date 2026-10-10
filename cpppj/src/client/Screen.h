// 원본 Screen.cpp: 팔레트 파일 읽기와 화면 장치(표면·화면 모드·팔레트·잠금·창으로 내보내기).
// 창 모드(8비트 DIB 섹션 → BitBlt)를 원본 방식 그대로 옮겼다. DirectDraw 표면(전체화면·플리핑)은 아직 옮기지 않았다.
// 근거·범위: docs/exe/cpp-screen-reconstruction.md
#pragma once
#include "client/PaletteColors.h"
#include "client/PaletteShade.h"
#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>

namespace netstorm::client {
struct PaletteColor { std::uint8_t red{}, green{}, blue{}; };
class GamePalette {
public:
    // 004a4850 ↔ CD 00424760: 0x308바이트 COL 또는 0x400바이트 BGRX 파일을 읽는다.
    explicit GamePalette(std::span<const std::uint8_t> bytes);
    // 원본 팔레트 번호에 대응하는 RGB 색을 반환한다.
    PaletteColor Color(std::uint8_t index) const;
    // BGRX 파일은 네 번째 바이트를 반환한다. RGB COL은 예약 바이트를 쓰지 않으므로 빈 값을 반환하여 이전 저장값을 보존하게 한다.
    std::optional<std::uint8_t> Reserved(std::uint8_t index) const;
private:
    std::array<PaletteColor, 256> colors_{};
    std::array<std::uint8_t,256> reserved_{}; // BGRX 파일의 원본 예약 바이트. 색 거리/표시 RGB에는 쓰지 않는다.
    bool bgrx_{};
};

// 화면 모드 플래그(원본 DAT_005c78d4). 설정 `windowScreenFlags`·`workingFullScreenFlags`의 값이며,
// 모드가 정해지면 `global.dd…` 설정으로도 적힌다.
namespace ScreenMode {
inline constexpr std::uint32_t kFullScreen = 0x01;     // DirectDraw 전체화면(global.ddFullScreen)
inline constexpr std::uint32_t kBackBufferDib = 0x02;  // 그리기 표면이 DIB 섹션(global.ddBackBufferDib)
inline constexpr std::uint32_t kFlipping = 0x04;       // 페이지 플리핑(global.ddFlipping)
inline constexpr std::uint32_t kClouds = 0x08;         // 구름("parallax without clouds" 검사의 짝)
inline constexpr std::uint32_t kParallax = 0x10;       // 구름 시차(global.ddParallax)
inline constexpr std::uint32_t kAnimating = 0x20;      // 구름 움직임
inline constexpr std::uint32_t kSoftwareMouse = 0x40;  // 커서를 직접 그림(global.ddSoftwareMouse)
inline constexpr std::uint32_t kModeSet = 0x80;        // 모드를 정할 때 항상 켜는 비트
// DirectDraw가 없는데 전체화면을 요구하면 원본이 대신 쓰는 값(004a4fe0의 `param_1 = 10`).
inline constexpr std::uint32_t kFallbackWindowed = kBackBufferDib | kClouds;
}
// 원본 FUN_004a13d0 ↔ CD: 화면 모드 조합이 허용되는지. 허용되지 않는 이유마다 원본은 로그를 남긴다.
bool ScreenModeIsLegal(std::uint32_t flags, bool directDrawInstalled);

// Windows RGBQUAD와 같은 순서의 색(파랑·초록·빨강·예약).
struct ScreenColor { std::uint8_t blue{}, green{}, red{}, reserved{}; };
// 원본 004a2820 / CD 004246e0: 논리 팔레트(R·G·B·플래그 DWORD 256개)에서 RGB 제곱 거리가 가장 작은 색 번호를 찾는다.
// 사용: byte 범위 RGB의 동률은 먼저 나온 번호를 유지한다. 좌표 차·곱·합은 32비트로 감고, 비교는 signed 거리와 float32 최솟값으로 한다.
// 범위 밖 RGB에서는 저장 반올림으로 정수 거리가 같아도 뒤 번호가 선택될 수 있다. 원본의 비교/저장 순서를 유지한다.
std::uint32_t FindPaletteColor(std::span<const std::uint32_t,256> logical,std::int32_t red,std::int32_t green,std::int32_t blue);
// 화면 좌표의 사각형. right·bottom은 포함하지 않는다(원본 004a1800의 인자).
struct ScreenRect { int left{}, top{}, right{}, bottom{}; };
// 원본 FUN_00445290: 사각형을 경계 안으로 자른다.
ScreenRect ClipScreenRect(ScreenRect rect, ScreenRect bounds);
// 원본 004a4fe0의 창 위치 계산(설정 `initWindowPos`): 1 왼쪽 위, 2 오른쪽 위, 3 오른쪽 아래, 4 왼쪽 아래, 5 가운데.
// 그 밖의 값이면 지금 위치를 그대로 둔다.
struct ScreenPoint { int x{}, y{}; };
ScreenPoint InitialWindowPosition(int initWindowPos, int desktopWidth, int desktopHeight,
    int windowWidth, int windowHeight, ScreenPoint current);

// Win32 핸들(HWND·HDC·HMENU 등)을 헤더에서 <windows.h> 없이 넘기기 위한 형식.
using NativeHandle = void*;

// 원본 표면 객체(0x8c바이트, 생성 004a05a0). 종류 1 = DIB 섹션, 2 = 이미 있는 장치 문맥(창).
// 종류 3·4·5(DirectDraw 주·보조·오프스크린 표면)는 옮기지 않았다.
class ScreenSurface {
public:
    enum class Kind { Dib = 1, DeviceContext = 2 };
    // 원본 FUN_004a05a0. DIB는 referenceDc의 팔레트를 가리키는 8비트 위→아래 DIB 섹션을 만들어 dc에 선택한다.
    ScreenSurface(Kind kind, int width, int height, NativeHandle dc, NativeHandle referenceDc);
    // 원본 FUN_004a0850: 비트맵과 머리 정보를 지운다.
    ~ScreenSurface();
    ScreenSurface(const ScreenSurface&) = delete;
    ScreenSurface& operator=(const ScreenSurface&) = delete;
    // 원본 FUN_004a0320: 장치 문맥.
    NativeHandle Dc() const;
    // 원본 FUN_004a08b0 / 004a0950: 픽셀 버퍼(DIB만). 창 표면은 널이다.
    std::uint8_t* Lock();
    void Unlock();
    // 원본 FUN_004a0a60의 GDI 경로: source의 [left,right)×[top,bottom)을 (x,y)에 BitBlt 한다.
    void BlitFrom(int x, int y, const ScreenSurface& source, int left, int top, int right, int bottom);
    // 원본 +0x14 폭, +0x10 높이, +0x18 줄 간격(원본은 폭과 같게 둔다).
    int Width() const;
    int Height() const;
    int Pitch() const;
private:
    Kind kind_;
    int width_{}, height_{};
    NativeHandle dc_{};
    NativeHandle bitmap_{};       // +0x84
    std::uint8_t* bits_{};        // +0x88
    std::uint8_t* locked_{};      // +0x74
};

// 원본 Screen.cpp의 전역 변수 묶음을 객체 하나로 둔다. 창과 창의 장치 문맥은 ClientMain이 만든다.
class Screen {
public:
    // 창(DAT_0054dbf4), 창의 장치 문맥(DAT_0054d960), 화면 크기(DAT_00531860·DAT_00531864).
    Screen(NativeHandle window, NativeHandle windowDc, int width, int height);
    ~Screen();
    Screen(const Screen&) = delete;
    Screen& operator=(const Screen&) = delete;

    // 원본 FUN_004a1270("init screen"): 창 표면, 메모리 장치 문맥, 빈 논리 팔레트를 만든다.
    void Init();
    // 원본 FUN_004a4570("init dib section"): 8비트 DIB 표면을 만들고 그리기 창(VFX window)을 그 버퍼로 맞춘다.
    void InitDibSection();
    // 원본 FUN_004a4fe0: 화면 모드를 정한다. 창 모드면 창 모양·위치를 맞추고 팔레트를 적용한다.
    // windowWidth·windowHeight는 클라이언트 영역이 화면 크기가 되는 창 크기(DAT_00542390·DAT_00542394)다.
    bool SetMode(std::uint32_t flags, int windowWidth, int windowHeight, int initWindowPos);
    // 원본 FUN_004a2640: 팔레트의 [start, start+count)를 colors로 바꾼다. colors가 널이면 저장해 둔 팔레트를 쓴다.
    // apply가 참이면 현재 팔레트에 복사하고 DIB 색 표와 논리 팔레트에 반영한다.
    void SetPalette(unsigned start, unsigned count, ScreenColor* colors, bool apply);
    // 원본 FUN_004a4850: 팔레트 파일의 색을 저장 팔레트에 넣고 적용한다.
    void LoadPalette(const GamePalette& palette,o::OriginalEdition edition=o::OriginalEdition::Patch1078);
    // 마지막 파일 팔레트로 계산한 원본 기본/표시/날씨 색 표다. SetPalette의 일시 번개/모드 적용으로 다시 계산하지 않는다.
    const PaletteColorTable& Colors() const;
    // 파일 팔레트의 GUI 밝음/어두움 변환표다. 번개의 일시 SetPalette와 화면 모드 적용으로 갱신하지 않는다.
    const GumpShadeMaps& ShadeMaps() const;
    // 현재 논리 팔레트에서 RGB 색을 찾는다. SetPalette(apply=false)로 준비한 논리 팔레트도 검색에 반영한다.
    std::uint32_t FindColor(std::int32_t red,std::int32_t green,std::int32_t blue) const;
    // 현재 논리 팔레트에서 날씨 RGB 네 색을 검색하는 계산 도우미다. 파일 로더의 저장 별칭은 Colors().weather를 쓴다.
    std::array<std::uint32_t,4> WeatherTints() const;
    // 원본 FUN_004a14e0 / 004a1550: 그리기 표면 잠금(중첩 횟수를 센다). 처음 잠글 때만 버퍼를 얻는다.
    std::uint8_t* Lock();
    void Unlock();
    // 원본 FUN_004a1580: 그리기 자르기 영역. right·bottom은 포함하지 않는다.
    void SetClip(int left, int top, int right, int bottom);
    // 원본 FUN_004a2060: 잠근 그리기 표면의 사각형을 한 색으로 채운다(현재 자르기 영역으로 자른다).
    void FillRect(ScreenRect rect, std::uint8_t color);
    // 원본 FUN_004a4790: 잠그고 채우고 푼다. 화면 모드가 정해지기 전에는 아무것도 하지 않는다.
    void Clear(ScreenRect rect, std::uint8_t color);
    // 원본 FUN_004a1800의 창 모드 경로: 그리기 표면의 사각형을 창으로 복사한다.
    void Update(ScreenRect rect);
    // 원본 FUN_004a40d0: 시스템 커서를 보이거나 숨긴다(숨김 횟수를 센다).
    void ShowCursor(bool show);

    // 현재 화면 모드 플래그.
    std::uint32_t Flags() const;
    int Width() const;
    int Height() const;
    // 그리기 표면의 줄 간격(DAT_005c78f0).
    int Pitch() const;
    // 현재 자르기 영역(양 끝 포함 — VFX pane 좌표, DAT_005b5de0~ec).
    ScreenRect Pane() const;
    // 현재 팔레트(DAT_0059afe0).
    std::span<const ScreenColor> Palette() const;
    // 창 테두리의 왼쪽·위쪽 두께(DAT_005c7904·DAT_005c7908). 입력 좌표를 화면 좌표로 바꿀 때 쓴다.
    ScreenPoint BorderOffset() const;
    // 창의 장치 문맥.
    NativeHandle WindowDc() const;
private:
    NativeHandle window_{};
    NativeHandle windowDc_{};
    int width_{}, height_{};
    NativeHandle memoryDc_{};                     // DAT_005c7928
    NativeHandle palette_{};                      // DAT_005c795c
    NativeHandle previousPalette_{};              // DAT_005c7960 / DAT_005c7964
    NativeHandle previousMemoryPalette_{};        // 메모리 DC의 이전 팔레트. 교체/종료 전에 복원하여 선택된 GDI 객체의 삭제 실패를 막는다.
    NativeHandle menu_{};                         // DAT_005c78e8
    std::unique_ptr<ScreenSurface> windowSurface_; // DAT_005c792c
    std::unique_ptr<ScreenSurface> dibSurface_;    // DAT_005c7930
    ScreenSurface* drawSurface_{};                 // DAT_005c7948
    ScreenSurface* primarySurface_{};              // DAT_005c794c
    std::array<ScreenColor, 256> current_{};       // DAT_0059afe0
    std::array<ScreenColor, 256> saved_{};         // DAT_005acd28
    std::array<std::uint32_t, 256> logical_{};     // DAT_005c7954: PALETTEENTRY(빨강·초록·파랑·플래그) 256개
    PaletteColorTable colors_;                   // 파일 팔레트 로더가 다시 계산하는 원본 기본/표시/날씨 색 표.
    GumpShadeMaps shadeMaps_;                    // 돌 배경 소스 번호를 변환하는 파일 팔레트 명암 표.
    std::uint32_t flags_{};                        // DAT_005c78d4
    int lockDepth_{};                              // DAT_005c7920
    std::uint8_t* buffer_{};                       // DAT_0059af88 / DAT_005c78f8
    ScreenRect clip_{};                            // DAT_005c7a44~50
    ScreenPoint border_{};                         // DAT_005c7904, DAT_005c7908
    ScreenPoint position_{};                       // DAT_005c78d8, DAT_005c78dc
    int cursorHidden_{};                           // DAT_005c7a3c
};
}
