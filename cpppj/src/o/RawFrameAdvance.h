// 공통 프레임 진행의 한 번 감싸기와 실제 프레임 지정 호출을 복원한다.
#pragma once
#include "o/SquidFrame.h"

namespace netstorm::o {
// 현재 프레임의 글자별 첫 번호/개수로 진행한다. 프레임 표와 지정기는 이 객체보다 오래 살아야 한다.
// 원본: 004afc90 / CD 004acce0. 일반 사제 Pop의 지원 여부는 바꾸지 않는다.
class RawFrameAdvance {
public:
    // 실제 지정기와 같은 풀을 받고 현재 프레임 표를 참조한다. 자산 표 교체는 다음 호출에 반영된다.
    RawFrameAdvance(SidPool& pool,std::span<const RiftTypeFrames> frames,const SquidFrame& setter);
    // signed 증분을 DWORD 연산으로 더해 음수면 길이를 더하고 길이 이상이면 뺀다. 보정은 한 번뿐이다.
    // flags는 실제 SquidFrame::Set에 전달하고 보정 발생 여부를 반환한다. 원본: 004afc90 / CD 004acce0.
    bool Advance(Sid sid,std::int32_t delta,std::uint32_t flags=0) const;
    // 낙하 이벤트 어댑터가 같은 풀/지정기에 연결됐는지 확인한다.
    const SidPool& Pool() const;
    // 직접 프레임 지정과 진행이 같은 효과 경로를 사용하는지 확인한다.
    const SquidFrame& Setter() const;
private:
    SidPool& pool_;
    std::span<const RiftTypeFrames> frames_;
    const SquidFrame& setter_;
};
}
