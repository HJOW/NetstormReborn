// Factory/아웃포스트의 예약·상태 프레임·중복 삭제·목록 등록을 같은 raw 월드에서 실행한다.
#pragma once
#include "o/RawCarrierPostPop.h"
#include "o/RawAssetPostPop.h"
#include "o/RawSquidFinder.h"
#include "o/RawSquidLevel.h"

namespace netstorm::o {
class SquidFrame;
// 원본이 직접 읽는 타입 번호와 권한은 기존 자산 상태의 boss·목록과 함께 사용한다.
struct FactoryPostPopTypes {
    std::uint32_t firstTimerFactory{85}; // DAT005411ac/CD0051ca98: 썬 워크샾85의 최초 event2를 8초 뒤에 예약하는 타입이다.
    std::uint32_t secondTimer{87}; // DAT005411b4/CD0051caa0: 최초 event1을 0.5초 뒤에 예약하는 타입이다.
    std::uint32_t fixedFrame{126}; // DAT00541250/CD0051cb3c: 상태별 프레임 보정을 건너뛰는 타입이다.
};
// 몸체가 요구하는 실제 프로세스·프레임 가상 효과와 아웃포스트 소유권 재계산 경계다.
struct FactoryPostPopHooks {
    std::function<bool()> reserveRegular; // 원본 각 new(0x28)의 독립 성공 여부다.
    std::function<void(Sid,std::uint32_t,float)> addRegular; // 같은 SID의 Regular 생성·부착 전체를 실행한다.
    std::function<void(Sid,std::int32_t,std::uint32_t)> setFrame; // SquidFrame::Set의 dirty/Unpop/Repop 효과를 포함한다.
    std::function<void(float,float)> regionOwnership; // 00455380(x,y,0)/CD0047b3c0: 아웃포스트와 신전으로 섬 소유권을 다시 계산한다.
    std::function<void(Sid,std::int32_t,std::int32_t)> warnDuplicate; // 중복 워크샾 삭제 직전의 원본 경고 로그다.
    std::function<void(Sid,std::uint32_t)> destroy; // 실제 같은 풀의 가상 +0x10 삭제이며 flags0을 전달한다.
    std::function<void(Sid)> updateState; // 가상84의 Level 저장 후 실제 가상88 기본 표시 갱신이다. 상태 프레임 보정과 구별한다.
};
// 실제 vtable +0x20과 +0x80이 검증된 Factory 부류만 받아 공통 깊이를 한 번 줄인다.
class RawFactoryPostPop {
public:
    // 풀·해시·타입·프레임·공통 몸체·상태는 처리기보다 오래 살아야 한다. 표면/객체 사본은 만들지 않는다.
    RawFactoryPostPop(SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,
        std::span<const RiftTypeFrames> frames,SquidPostPop& base,const RawCarrierPostPop& damageable,
        AssetPostPopState& state,FactoryPostPopHooks hooks,FactoryPostPopTypes numbers={});
    // 타입 이름 대신 실제 가상 표가 Factory/아웃포스트 몸체를 가리키는지 판단한다.
    bool Handles(Sid sid) const;
    // 공간 쓰기 전에 현재 분기의 필수 외부 효과와 공통 장부를 확인한다.
    void Validate(Sid sid,std::uint32_t flags,const RawGraphPop* pop=nullptr) const;
    // wrapper 등록→최초 예약→Damageable→프레임→현재 일반 finder/중복 삭제 순서를 실행한다.
    void PostPop(Sid sid,std::uint32_t flags) const;
    // 실제 Factory 가상84는 공통Level 저장 뒤 같은 가상88 기본 표시 갱신을 호출한다.
    void SetState(const RawSquidLevel& level,Sid sid,std::uint32_t value) const;
    //00453780/CD0047a1f0:업그레이드 완료에서도 동일한 실제 상태 프레임 보정을 사용한다.
    void AdjustFrame(Sid sid) const;
    // 공통 분배기에 다른 풀의 처리기를 등록하지 않도록 소유자를 공개한다.
    const SidPool& Pool() const;
private:
    // 0은 미복원, 1은 Factory, 2는 아웃포스트 wrapper다.
    std::uint32_t Kind(Sid sid) const;
    // base 가상 +0x80은 raw 상태 상위 세 비트를 반환한다. 해당 네 가상 표에서만 사용한다.
    std::int32_t Count(Sid sid) const;
    // 독립 할당 실패를 보존하며 다음 예약이나 공통 몸체를 계속한다.
    void Regular(Sid sid,std::uint32_t event,float delay) const;
    SidPool& pool_; // 모든 객체를 조회·수정하는 실제 SID 풀이다.
    const SquidHash& hash_; // 중복 워크샾 검사에 사용하는 같은 월드의 공간 해시다.
    std::span<const RiftTypeRecord> types_; // 일반 finder의 현재 발자국·플래그 표다.
    std::span<const RiftTypeFrames> frames_; // 상태별 첫 프레임/개수 구간을 읽는 현재 프레임 표다.
    SquidPostPop& base_; // 공통 장부와 postPop 깊이를 공유한다.
    const RawCarrierPostPop& damageable_; // 직접 Damageable 후처리를 한 번 실행한다.
    AssetPostPopState& state_; // boss·아웃포스트/거주 구역 목록을 다른 후처리와 공유한다.
    FactoryPostPopHooks hooks_; // 실제 외부 효과의 수명은 이 처리기보다 길어야 한다.
    FactoryPostPopTypes numbers_; // 원본 타입 번호 전역의 현재 값을 보관한다.
};
// 동일 풀의 실제 프레임 지정/dirty·Unpop/Repop 몸체를 연결하고 나머지 효과는 호출자의 구현을 유지한다.
FactoryPostPopHooks MakeFactoryPostPopHooks(const SidPool& pool,SquidFrame& frame,FactoryPostPopHooks hooks);
}
