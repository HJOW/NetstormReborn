// 오벨리스크의 실제 buried Handle004622f0을 같은 풀/프레임/공유 난수에 연결한다.
#pragma once
#include "o/SquidFrame.h"
#include "o/RiftType.h"
#include "o/SpStore.h"
#include "o/SquidProcess.h"

namespace netstorm::o {
// 별도 예약/표시 사본 없이 현재 오벨리스크의 Regular 사건을 실행한다.
class RawBuriedRegular {
public:
    RawBuriedRegular(SidPool& pool,std::span<const RiftTypeFrames> frames,const SquidFrame& setter,GameRandom& random); // 같은 풀/188개 현재TYPE 프레임만 허용한다.
    bool Handles(Sid sid) const; // 실제0050a748 가상 표를 확인하며 호스트 주소로 실행하지 않는다.
    float Handle(Sid sid,std::uint32_t event,std::uint32_t count,float payload) const; // signed 나머지와 원본 프레임/지연을 적용한다.
    RegularHandler Handler(RegularHandler fallback={}) const; // 다른 부모의 사건은 기존 실제 처리기로 전달한다.
private:
    SidPool& pool_; // 현재 QA/TYPE/프레임을 읽고 쓰는 실제 raw 풀이다.
    std::span<const RiftTypeFrames> frames_; // 현재 TYPE의 A/B 구간이며 수명은 상위 월드에 있다.
    const SquidFrame& setter_; // 같은 풀의 실제 표시 갱신/프레임 지정기다.
    GameRandom& random_; // 전체 월드가 공유하는00532710의 현재 난수 상태다.
};
}
