// 원본 Screen.cpp 004a4410·004a4260: 하드웨어 커서 리소스와 팔레트 소프트웨어 커서.
#pragma once
#include "client/Screen.h"
#include "client/VFXDraw.h"
#include <optional>

namespace netstorm::client {
// 원본 005423a8/CD 00516c60의 19항목 표. 0은 아직 모양을 정하지 않은 상태다.
inline constexpr std::array<std::uint16_t, 19> kCursorResources{0, 113, 110, 111, 108, 107, 109, 115, 116, 131, 117, 130, 129, 132, 133, 134, 136, 141, 148};
class Cursor {
public:
    // 원본 실행 파일을 자료로 연 모듈과 게임 창을 사용한다. 두 핸들을 소유하지 않는다.
    Cursor(NativeHandle resources, NativeHandle window);
    // 004a4410: 커서 번호와 창 클래스의 기본 커서를 함께 바꾼다.
    void Set(int index);
    // WM_SETCURSOR가 클라이언트 영역에서 원본 모양을 유지하도록 다시 적용한다.
    void Apply() const;
    // 004a4260: 현재 화면 팔레트의 8비트 DIB에 원본 커서를 그려 32×32 프레임을 만든다.
    void BuildSoftware(std::span<const ScreenColor> palette);
    // 현재 소프트웨어 프레임과 핫스팟. BuildSoftware 뒤에만 유효하다.
    const IndexedImage* SoftwareImage() const;
    // 현재 이미지의 커서 기준점.
    ScreenPoint Hotspot() const;
    // 현재 원본 커서 번호(1~18).
    int Index() const;
private:
    NativeHandle resources_, window_, handle_{};
    int index_{1};
    std::array<std::optional<IndexedImage>, 19> images_{};
    std::array<ScreenPoint, 19> hotspots_{};
};
}
