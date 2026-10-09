// 보호막의 최초 예약·프레임 진행·사제 존재 검사와 일반 Pop 분배를 연결한다.
#pragma once
#include "o/RawFrameAdvance.h"
#include "o/RawPriestPostPop.h"
#include "o/RawSquidFinder.h"
#include "o/SquidProcess.h"

namespace netstorm::o {
// 보호막이 사용하는 실제 타입/가상 표와 두 Regular 사건 번호다. 주소는 실행하지 않는다.
inline constexpr std::uint8_t kForcefieldType=167;
inline constexpr std::uint32_t kPatchForcefieldVtable=0x00507770,kCdForcefieldVtable=0x005042b8;
inline constexpr std::uint32_t kForcefieldFrameEvent=1,kForcefieldCheckEvent=2;
// 두 번의 원본 new(0x28)을 각각 관찰한다. 실패해도 다음 확보와 공통 후처리를 계속한다.
struct ForcefieldPostPopHooks {
    std::function<bool()> reserveRegular;
    std::function<void(Sid,std::uint32_t,float)> addRegular;
};
// 사용: 같은 풀의 실제 예약 훅을 구성한 뒤 SquidPostPop::SetForcefieldPostPop에 등록한다.
class RawForcefieldPostPop {
public:
    // 풀/효과 대상은 이 객체보다 오래 살아야 하며 두 필수 효과의 누락은 거부한다.
    RawForcefieldPostPop(const SidPool& pool,ForcefieldPostPopHooks hooks);
    // 공간 쓰기 전에 실제 타입/가상 표를 확인하며 예약/장부는 변경하지 않는다.
    void Validate(Sid sid) const;
    // flags 1일 때만 사건 1(0.08f), 사건 2(0.01f)를 순서대로 예약한다. 공통 base는 호출자가 이어 준다.
    // 원본: 00448d20 / CD 004847d0의 공통 후처리 직전 접두다.
    void Prefix(Sid sid,std::uint32_t flags) const;
    // 다른 월드의 일반 Pop에 등록되지 않도록 구성 단계에서 확인한다.
    const SidPool& Pool() const;
private:
    const SidPool& pool_;
    ForcefieldPostPopHooks hooks_;
};
// 사제 타입 DWORD는 매 검사 때 읽는다. 소유자는 존재 검사의 조건이 아니다.
struct ForcefieldRegularState { std::uint32_t priestType{kPriestType};bool debug{}; };
// 프레임 진행/가상 삭제만 외부 효과이며 위치의 일반 finder는 실제 모듈을 사용한다.
struct ForcefieldRegularHooks {
    std::function<void(Sid,std::int32_t,std::uint32_t)> advance;
    std::function<void(Sid,std::uint32_t)> destroy;
};
// 사용: Handle을 Regular 처리기에 연결하고 advance/destroy를 같은 월드의 실제 모듈로 공급한다.
class RawForcefieldRegular {
public:
    // 풀/해시/타입/상태와 효과 대상은 이 객체보다 오래 살아야 한다.
    RawForcefieldRegular(const SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,
        const ForcefieldRegularState& state,ForcefieldRegularHooks hooks);
    // 사건 1은 (1,0) 진행 뒤 0.08f, 사건 2는 사제가 있으면 0.01f/없으면 destroy(0) 뒤 -1이다.
    // 다른 사건은 0이며 패치 debug 모드에서는 원본 assert 대신 예외로 진단한다. count/payload는 읽지 않는다.
    float Handle(Sid sid,std::uint32_t event,std::uint32_t count,float payload) const;
    // 풀/프레임 진행 어댑터의 연결을 확인한다.
    const SidPool& Pool() const;
private:
    // 현재 좌표를 0방향 절삭해 원본 기본 finder 순서의 첫 타입 일치를 반환한다.
    Sid FindPriest(Sid sid) const;
    const SidPool& pool_;
    const SquidHash& hash_;
    std::span<const RiftTypeRecord> types_;
    const ForcefieldRegularState& state_;
    ForcefieldRegularHooks hooks_;
};
// 확보 경계는 보존하고 실제 Regular/form/Kernel 생성으로 예약을 연결한다.
ForcefieldPostPopHooks MakeForcefieldPostPopProcessHooks(const SidPool& pool,SquidProcessHost& host,std::function<bool()> reserve);
// 프레임 진행 효과만 실제 지정/공간 어댑터로 교체한다. 가상 삭제 효과는 호출자가 공급한다.
ForcefieldRegularHooks MakeForcefieldFrameHooks(const SidPool& pool,const RawFrameAdvance& advance,ForcefieldRegularHooks hooks);
}
