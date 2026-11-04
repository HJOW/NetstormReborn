// Carrier→Damageable→Squid의 비가상 postPop 호출 순서를 복원한다.
#pragma once
#include "o/SquidPostPop.h"

namespace netstorm::o {
// 캐리어 후처리 Pop 처리 중 조회·갱신하는 상태 묶음이다.
struct CarrierPostPopState {
    bool boss{true}; // 00540bc4 / CD 00540a2c: 권한이 없을 때만 비최초 carrier 확인을 부른다.
    std::uint32_t loadingDepth{}; // 005c89b8 / CD 005178d4: 요새 로딩의 중첩 횟수다.
    std::uint32_t geyserType{122}; // 00541240 / CD 0051cb2c: 로딩 중 지면 소유자 갱신에서 제외한다.
    const std::uint32_t* sharedGeyserType{}; // 실제 게임 TYPE를 빌린다.
    std::uint32_t GeyserType() const { return sharedGeyserType ? *sharedGeyserType : geyserType; } // 로딩 중 비교 위치에서 현재 TYPE를 읽는다.
    const bool* sharedBoss{}; // 실제 Client boss를 빌리며 독립 Native fixture는 기존 값형 boss를 유지한다.
    bool Boss() const { return sharedBoss ? *sharedBoss : boss; } // 최초 flag 관문 뒤 같은 현재 권한을 조회한다.
    const std::uint32_t* sharedLoadingDepth{}; //실제Session의현재로딩DWORD를빌리며독립입력은기존값을쓴다.
    std::uint32_t LoadingDepth() const { return sharedLoadingDepth ? *sharedLoadingDepth : loadingDepth; } //Carrier확인뒤현재로딩관문에서조회한다.
};
// 캐리어 후처리 Pop 외부 동작과 저장소 조회를 연결하는 콜백 묶음이다.
struct CarrierPostPopHooks {
    std::function<void(Sid)> carrierCheck; // vtable +0xcc. 원본은 반환값을 사용하지 않는다.
    std::function<void(float,float,std::uint8_t)> claimGround; // 0044bd20 / CD 00461f00: 지면/받침 소유자 갱신 경계다.
    std::function<void(Sid,std::uint32_t)> validateBase; //CD의기존사전검사다. 실제PATCH는이경계를호출하지않는다.
    std::function<void(Sid,std::uint32_t)> base; // 같은 flags로 SquidPostPop의 비가상 몸체를 부른다.
};
// Carrier 확인→Damageable 지면 처리→공통 장부를 직접 호출한다. 파생 가상 지원은 별도다.
class RawCarrierPostPop {
public:
    // 직접 호출용 몸체다. 전체 파생 흐름/가상 분배를 자동으로 허용하지 않는다.
    // 타입 표는 복사하고 풀·상태·훅 대상은 이 객체보다 오래 살아야 한다.
    RawCarrierPostPop(SidPool& pool,std::span<const RiftTypeRecord> types,const CarrierPostPopState& state,CarrierPostPopHooks hooks);
    // 00427720 / CD 004e6360: flags & 1이 없고 boss가 거짓이면 +0xcc, 이후 항상 Damageable을 부른다.
    void PostPop(Sid sid,std::uint32_t flags) const;
    // 0044bf80 / CD 004621c0: 로딩 중 조건이 맞으면 지면 소유자 갱신, 이후 항상 공통 postPop이다.
    void DamageablePostPop(Sid sid,std::uint32_t flags) const;
    void BindCurrentTypes(std::span<const RiftTypeRecord> types); //실제PATCH World의현재TYPE를빌리고빈span으로기존소유사본에복귀한다.
    // 사제 후처리 조합에서 다른 월드의 Carrier 연결을 거부한다.
    const SidPool& Pool() const;
private:
    // 효과 발생 전에 raw 자산/타입과 공통 경계를 검사한다.
    void Validate(Sid sid,std::uint32_t flags) const;
    // carrier 가상 확인 이후의 현재 raw 값으로 로딩 조건을 평가한다.
    void RunDamageable(Sid sid,std::uint32_t flags) const;
    // pool_: 객체 슬롯의 생성·조회·반납에 사용하는 SID 풀.
    SidPool& pool_;
    // types_: 타입 번호로 속성·플래그를 조회하는 타입 표.
    std::vector<RiftTypeRecord> types_;
    std::span<const RiftTypeRecord> currentTypes_; //실제World에서대여한주소고정배열이며재할당하지않는다.
    const RiftTypeRecord& CurrentType(std::uint32_t number) const; //최초genus/로딩중flags1을실제소비지점에서조회한다.
    // state_: 처리 중 조회·갱신하는 상태 (const CarrierPostPopState).
    const CarrierPostPopState& state_;
    // hooks_: 외부 저장소 조회와 가상 동작을 연결하는 콜백 묶음 (CarrierPostPopHooks).
    CarrierPostPopHooks hooks_;
};
// 같은 풀의 실제 공통 장부에 연결한다. +0xcc와 지면 효과는 해당 파생/월드가 공급한다.
CarrierPostPopHooks MakeCarrierPostPopHooks(SidPool& pool,SquidPostPop& base,
    std::function<void(Sid)> carrierCheck,std::function<void(float,float,std::uint8_t)> claimGround);
}
