// 원본 004a4410 ↔ CD 004266c0, 004a4260 ↔ CD 004264e0. 18개 이미지와 원본 핫스팟을 유지한다.
#include "client/Cursor.h"
#include <cstring>
#include <stdexcept>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

namespace netstorm::client {
namespace {
// 원본은 소프트웨어 커서를 32×32 DIB에서 배경 103 위에 그린다.
constexpr int kCursorSide = 32;
constexpr std::uint8_t kCursorBackground = 103;
// 실패 경로에서도 임시 DIB·DC·아이콘 정보를 정리한다.
struct CursorGdi {
    HDC dc{};
    HBITMAP bitmap{};
    HGDIOBJ previous{};
    ICONINFO icon{};
    // 임시로 소유한 GDI 자원만 해제한다.
    ~CursorGdi() {
        if (previous) SelectObject(dc, previous);
        if (bitmap) DeleteObject(bitmap);
        if (icon.hbmColor) DeleteObject(icon.hbmColor);
        if (icon.hbmMask) DeleteObject(icon.hbmMask);
        if (dc) DeleteDC(dc);
    }
};
}
// 기본 커서는 원본의 1번(리소스 113)이다.
Cursor::Cursor(NativeHandle resources, NativeHandle window) : resources_(resources), window_(window) { Set(1); }
// 리소스가 없는 자료 폴더에서는 Windows 기본 화살표를 사용한다.
void Cursor::Set(int index) {
    if (index < 1 || index >= static_cast<int>(kCursorResources.size())) throw std::out_of_range("Invalid cursor index");
    const HCURSOR cursor = resources_ ? LoadCursorA(static_cast<HINSTANCE>(resources_), MAKEINTRESOURCEA(kCursorResources[index])) : LoadCursorA(nullptr, IDC_ARROW);
    if (!cursor) throw std::runtime_error("Unable to load original cursor resource");
    index_ = index; handle_ = cursor;
    SetCursor(cursor);
    if (window_) SetClassLongPtrA(static_cast<HWND>(window_), GCLP_HCURSOR, reinterpret_cast<LONG_PTR>(cursor));
}
// 활성 클라이언트 영역에 현재 커서를 적용한다.
void Cursor::Apply() const { SetCursor(static_cast<HCURSOR>(handle_)); }
// 원본과 같이 팔레트에서 색을 선택하고 왼쪽 위 픽셀을 투명색으로 삼는다.
void Cursor::BuildSoftware(std::span<const ScreenColor> palette) {
    if (palette.size() != 256) throw std::invalid_argument("Software cursor requires 256 palette entries");
    // 각 모양을 원본 표의 순서대로 생성한다.
    for (std::size_t i = 1; i < kCursorResources.size(); ++i) {
        CursorGdi gdi;
        const HCURSOR cursor = resources_ ? LoadCursorA(static_cast<HINSTANCE>(resources_), MAKEINTRESOURCEA(kCursorResources[i])) : LoadCursorA(nullptr, IDC_ARROW);
        if (!cursor || !GetIconInfo(cursor, &gdi.icon)) throw std::runtime_error("Unable to read cursor hotspot");
        gdi.dc = CreateCompatibleDC(nullptr);
        struct CursorBitmapInfo { BITMAPINFOHEADER header; RGBQUAD colors[256]; } info{};
        info.header.biSize = sizeof(BITMAPINFOHEADER); info.header.biWidth = kCursorSide; info.header.biHeight = -kCursorSide;
        info.header.biPlanes = 1; info.header.biBitCount = 8; info.header.biCompression = BI_RGB;
        // 화면 팔레트의 BGRX 순서를 GDI 색 표에 그대로 옮긴다.
        for (std::size_t c = 0; c < palette.size(); ++c) info.colors[c] = {palette[c].blue, palette[c].green, palette[c].red, 0};
        void* buffer = nullptr;
        gdi.bitmap = CreateDIBSection(gdi.dc, reinterpret_cast<BITMAPINFO*>(&info), DIB_RGB_COLORS, &buffer, nullptr, 0);
        if (!gdi.dc || !gdi.bitmap || !buffer) throw std::runtime_error("Unable to create software cursor DIB");
        gdi.previous = SelectObject(gdi.dc, gdi.bitmap);
        std::memset(buffer, kCursorBackground, kCursorSide * kCursorSide);
        if (!DrawIconEx(gdi.dc, 0, 0, cursor, kCursorSide, kCursorSide, 0, nullptr, DI_NORMAL)) throw std::runtime_error("Unable to draw software cursor");
        GdiFlush();
        IndexedImage image{ kCursorSide, kCursorSide, {}, {} };
        const auto* pixels = static_cast<const std::uint8_t*>(buffer);
        image.indices.assign(pixels, pixels + kCursorSide * kCursorSide); image.opacity.resize(image.indices.size());
        // 원본 VFX 캡처가 배경과 같은 색만 투명하게 만든다.
        for (std::size_t p = 0; p < image.indices.size(); ++p) image.opacity[p] = image.indices[p] == pixels[0] ? 0 : 255;
        hotspots_[i] = {static_cast<int>(gdi.icon.xHotspot), static_cast<int>(gdi.icon.yHotspot)};
        images_[i] = std::move(image);
    }
}
// 현재 모양의 메모리 이미지.
const IndexedImage* Cursor::SoftwareImage() const { return images_[index_] ? &*images_[index_] : nullptr; }
// 현재 모양의 기준점.
ScreenPoint Cursor::Hotspot() const { return hotspots_[index_]; }
// 원본 커서 번호.
int Cursor::Index() const { return index_; }
}
