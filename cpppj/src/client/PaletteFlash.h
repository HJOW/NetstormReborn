// 원본 Kencloud.cpp의 번개 팔레트 프로세스(10.78 00470d40~00471820)를 복원한다.
#pragma once
#include "client/Screen.h"
#include <functional>

namespace netstorm::client {
// 원본 번개가 바꾸는 색 번호 범위다. 기본 팔레트의 구름 색 228~245번을 사용한다.
inline constexpr unsigned kThunderFirstColor=228;
inline constexpr unsigned kThunderColorCount=18;
// 원본은 RGB를 기본 64만큼 밝히고 매 단계 1씩 복구한다. 밝기 인자가 달라도 단계 수는 64다.
inline constexpr int kThunderSteps=64;
// 생성 요청의 밝기 0은 기본 밝기 64를 뜻한다(00470fa0 / CD 004b10d0).
inline constexpr int ThunderOffset(int requested) { return requested==0 ? 64 : requested; }
// 생성 당시 전체화면 여부로 수명을 정한다. 창 모드는 진행만 하고 팔레트를 표시하지 않는다.
inline constexpr double ThunderDuration(bool fullScreen) { return fullScreen ? 0.5 : 1.0/60.0; }

// 원본 +0xc/+0x40c의 원래 색/변경 색과 +0x810 이후 진행 상태다. 색 배열은 부분 팔레트를 앞에서부터 담는다.
struct PaletteFlashState {
    std::array<ScreenColor,256> original{},current{};
    double interval{},last{},duration{};
    unsigned start{kThunderFirstColor},count{kThunderColorCount};
    std::int32_t offset{64};
    ScreenColor solid{}; // offset 0인 일반 색 전환 경로의 목표 색(+0x82c).
    int steps{};
    bool initialized{};
};
// 시각은 게임 정지와 무관한 프레임 실시간이다. 화면 모드는 생성 시 수명 결정 뒤에도 매 프레임 다시 읽는다.
struct PaletteFlashHooks {
    std::function<double()> wallSeconds;
    std::function<bool()> fullScreen;
    std::function<void(unsigned start,unsigned count,ScreenColor* destination)> readPalette;
    std::function<void(unsigned start,unsigned count,ScreenColor* colors)> applyPalette;
    std::function<void()> restorePalette; // 당시의 저장 팔레트 전체를 복구한다. 생성 때 읽은 부분 배열을 복구하는 것이 아니다.
};
// 실제 팔레트 계산과 종료 판정을 맡는다. 소유자는 Init 뒤 Frame을 호출하고 중단/파괴 전에 Cancel을 호출한다.
// state/hooks 대상은 이 객체보다 오래 살아야 하며, 커널 등록·부모 월드 수명은 호출자가 관리한다.
class PaletteFlash {
public:
    // 필수 경계와 부분 색 범위를 검사한다. OS 자원을 만들지 않는다.
    PaletteFlash(PaletteFlashState& state,PaletteFlashHooks hooks);
    // 원본 00470e30 / CD 004b1310: 현재 색을 두 번 읽고 밝기 변경 또는 목표 색 채우기를 준비한다.
    void Init();
    // 원본 00471650 / CD 004b1450: interval보다 엄격히 큰 경과 시간마다 RGB를 1씩 복구한다. 계속 진행하면 참이다.
    // 정상 종료 때 전체화면이면 저장 팔레트를 복구하며, 창 모드에서는 화면 팔레트를 전혀 바꾸지 않는다.
    bool Frame();
    // 원본 00471820 / CD 004b15d0: 초기화된 전체화면 효과를 중단할 때 저장 팔레트를 복구한다. 중복 취소는 무시한다.
    void Cancel();
private:
    PaletteFlashState& state_;
    PaletteFlashHooks hooks_;
};
}
