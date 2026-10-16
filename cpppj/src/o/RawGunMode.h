// 원본 총 공격 방향 getter를 현재 raw graph/frame과 실제 프레임 문자에 연결한다.
#pragma once
#include "o/SidPool.h"
#include "o/RiftType.h"

namespace netstorm::o {
// PATCH004d4d00/CD00443110의 순수 현재 공격 모드 몸체다.
class RawGunMode {
public:
    // 실제 판본의 프레임 표와 현재 windArcher 타입은 객체보다 오래 살아야 한다.
    RawGunMode(const SidPool& pool,std::span<const RiftTypeFrames> frames,const std::uint32_t& windArcherType);
    // 비astral 크로스보우는 현재graphBYTE×2, 나머지는 signed 프레임문자×2-152의0..7만 반환한다.
    std::uint32_t Get(Sid sid) const;
    // 다른 월드의 Gun 검색에 연결하지 않도록 같은 Pool을 제공한다.
    const SidPool& Pool() const;
private:
    const SidPool& pool_; // 현재 자산의 TYPE/extra/graph/frame을 읽는 실제 저장소다.
    std::span<const RiftTypeFrames> frames_; // 같은 TYPE 번호의 실제4BYTE 프레임 코드다.
    const std::uint32_t& windArcherType_; // 원본 mutable 타입 DWORD를 매 조회한다.
};
}
