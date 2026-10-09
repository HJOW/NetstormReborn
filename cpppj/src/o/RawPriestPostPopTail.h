// 사제 postPop 전체 순서를 조합한다. 일반 Pop과 낙하 애니메이션의 가상 지원은 별도다.
#pragma once
#include "o/RawPriestPostPop.h"
#include "o/RawCarrierPostPop.h"
#include "o/RawPriestState.h"
#include "o/RawPriestShield.h"
#include "o/RawPathAnimation.h"

namespace netstorm::o {
// postPop 후반의 외부 효과다. 낙하는 자체 보호막 요청/공간 재등록을 포함하는 별도 몸체다.
struct PriestPostPopTailHooks {
    std::function<void(Sid)> fall; // 004941f0 / CD 0040c880의 전체 낙하 요청이다.
    std::function<void(Sid)> ensureShield; // 현재 이동 불가 분기에서 보호막을 확보한다.
    std::function<void(Sid)> clearShield; // 이동 가능한 분기에서 현재 보호막을 조회/삭제한다.
};
// 목록/회복 접두→Carrier→현재 지면/프레임→낙하/보호막을 같은 풀에서 실행한다.
// 사용: 전용 postPop 직접 호출용이며 SquidPop의 지원 가상 표를 자동 확장하지 않는다.
class RawPriestPostPopTail {
public:
    // 같은 풀의 실제 모듈·프레임/spot을 연결한다. 프레임은 복사하고 나머지 대상은 더 오래 살아야 한다.
    RawPriestPostPopTail(SidPool& pool,const RawPriestPostPop& prefix,const RawCarrierPostPop& carrier,
        const RawPriestState& state,std::span<const RiftTypeFrames> frames,std::span<const std::uint8_t> spots,
        PriestPostPopTailHooks hooks);
    // 현재 extra를 Carrier 뒤 다시 읽고 낙하를 요청한 뒤에도 보호막 확보를 실행한다.
    // 원본: 004950f0 / CD 0040c110 전체 wrapper. 낙하/보호막 효과 내부는 연결 대상에 위임한다.
    void PostPop(Sid sid,std::uint32_t flags) const;
private:
    SidPool& pool_;
    const RawPriestPostPop& prefix_;
    const RawCarrierPostPop& carrier_;
    const RawPriestState& state_;
    std::vector<RiftTypeFrames> frames_;
    std::span<const std::uint8_t> spots_;
    PriestPostPopTailHooks hooks_;
};
// 같은 풀의 실제 보호막 생성/조회에 연결한다. destroy는 가상 +0x10(flags 0), fall은 전체 낙하 경계다.
PriestPostPopTailHooks MakePriestPostPopTailShieldHooks(const SidPool& pool,const RawPriestShield& shield,
    const RawPriestForcefield& lookup,std::function<void(Sid,std::uint32_t)> destroy,std::function<void(Sid)> fall);
}
