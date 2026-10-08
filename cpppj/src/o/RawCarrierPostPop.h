// Carrier→Damageable→Squid의 비가상 postPop 호출 순서를 복원한다.
#pragma once
#include "o/SquidPostPop.h"

namespace netstorm::o {
struct CarrierPostPopState {
    bool boss{true}; // 00540bc4 / CD 00540a2c: 권한이 없을 때만 비최초 carrier 확인을 부른다.
    std::uint32_t loadingDepth{}; // 005c89b8 / CD 005178d4: 요새 로딩의 중첩 횟수다.
    std::uint32_t geyserType{122}; // 00541240 / CD 0051cb2c: 로딩 중 지면 소유자 갱신에서 제외한다.
};
struct CarrierPostPopHooks {
    std::function<void(Sid)> carrierCheck; // vtable +0xcc. 원본은 반환값을 사용하지 않는다.
    std::function<void(float,float,std::uint8_t)> claimGround; // 0044bd20 / CD 00461f00: 지면/받침 소유자 갱신 경계다.
    std::function<void(Sid,std::uint32_t)> validateBase; // 효과 전에 공통 후처리의 상태/Graph/장부를 검사한다.
    std::function<void(Sid,std::uint32_t)> base; // 같은 flags로 SquidPostPop의 비가상 몸체를 부른다.
};
class RawCarrierPostPop {
public:
    // 직접 호출용 몸체다. 전체 파생 흐름/가상 분배를 자동으로 허용하지 않는다.
    // 타입 표는 복사하고 풀·상태·훅 대상은 이 객체보다 오래 살아야 한다.
    RawCarrierPostPop(SidPool& pool,std::span<const RiftTypeRecord> types,const CarrierPostPopState& state,CarrierPostPopHooks hooks);
    // 00427720 / CD 004e6360: flags & 1이 없고 boss가 거짓이면 +0xcc, 이후 항상 Damageable을 부른다.
    void PostPop(Sid sid,std::uint32_t flags) const;
    // 0044bf80 / CD 004621c0: 로딩 중 조건이 맞으면 지면 소유자 갱신, 이후 항상 공통 postPop이다.
    void DamageablePostPop(Sid sid,std::uint32_t flags) const;
private:
    // 효과 발생 전에 raw 자산/타입과 공통 경계를 검사한다.
    void Validate(Sid sid,std::uint32_t flags) const;
    // carrier 가상 확인 이후의 현재 raw 값으로 로딩 조건을 평가한다.
    void RunDamageable(Sid sid,std::uint32_t flags) const;
    SidPool& pool_;
    std::vector<RiftTypeRecord> types_;
    const CarrierPostPopState& state_;
    CarrierPostPopHooks hooks_;
};
// 같은 풀의 실제 공통 장부에 연결한다. +0xcc와 지면 효과는 해당 파생/월드가 공급한다.
CarrierPostPopHooks MakeCarrierPostPopHooks(SidPool& pool,SquidPostPop& base,
    std::function<void(Sid)> carrierCheck,std::function<void(float,float,std::uint8_t)> claimGround);
}
