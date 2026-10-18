// 10.78 사제의604 의식 애니메이션을 실제 K..O 코드표와 현재 Host/프레임에 연결한다.
#pragma once
#include "o/RawPathAnimation.h"
#include <functional>
#include <string_view>
namespace netstorm::o {
// 원본 프레임 지정/605 조회/위치 음향의 실제 하위 효과다.
struct PriestRitualFrameHooks {
    std::function<void(Sid,std::int32_t,std::uint32_t)> setFrame; // 같은 SquidFrame::Set의 현재 부모/프레임/flags0이다.
    std::function<bool(Sid,std::uint32_t)> eventExists; // 현재605가 있으면 반복 의식 소리를 생략한다.
    std::function<void(float,float,std::string_view,std::uint32_t)> playAt; // 현재 부모 좌표의 beamOver03.wav/flags0이다.
};
// 00494160과 실제 도달하는 내부49417a의 전체604 프레임 몸체다. 다른 사제 사건은 담당하지 않는다.
class RawPriestRitualFrame {
public:
    // 참조 대상은 객체보다 오래 살아야 한다. 새 실제 관측이 없는 CD는 구성에서 거부한다.
    RawPriestRitualFrame(const SidPool& pool,std::span<const RiftTypeFrames> frames,PriestRitualFrameHooks hooks);
    float Tick(Sid priest) const; // K..O는+1/구간 끝에서첫번호+4, 첫번호+14의조건부소리와0.08초이며 나머지는0으로 종료한다.
private:
    const SidPool& pool_; // 효과 뒤 현재 타입/좌표를 다시 읽는 유일한 raw 저장소다.
    std::span<const RiftTypeFrames> frames_; // 원본 TYPE first/count/code의 같은 현재 표다.
    PriestRitualFrameHooks hooks_; // 세 필수 하위 효과의 실제 연결이다.
};
}
