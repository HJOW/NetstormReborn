// Carrier42a750의 내용물 표시/시전 기한/회피/낙하 사건과 Damageable 위임이다.
#pragma once
#include "o/SquidProcess.h"
#include <array>

namespace netstorm::o {
// 기존 TYPE 표에서 아직 일반 레코드에 노출하지 않은 두 필드다.
struct CarrierRegularType {
    std::int32_t defaultFrame{}; // TYPE+118의 현재 기본 그림 프레임이다.
    std::int32_t castTime{}; // TYPE+1b8의 casttime 정수이며493에서 감긴10배를 비교한다.
};
// 큰 하위 몸체만 외부 경계로 둔다. 내용물 교대/프레임/좌표/조기 반환은 본문에서 수행한다.
struct CarrierRegularHooks {
    std::function<Sid(Sid,std::uint32_t,std::uint32_t,std::uint32_t)> find; // 4ac9f0(parent,mask,kind,index)의 실제 현재 내용물이다.
    std::function<Sid(std::uint32_t,std::uint32_t)> create; // 4af530(type,2)의 실제 새 미리보기 자산이다.
    std::function<void(Sid,std::uint32_t)> setWord,setOwner; // 실제4ac220과 현재가상74다.
    std::function<void(Sid,float,float,std::uint32_t)> pop; // 현재가상90의 공간 부착이다.
    std::function<bool()> allocateTimer; // new18의 실제 할당 성공 여부이며 실패하면 타이머 생성만 생략한다.
    std::function<void(Sid,float)> constructTimer; // 확보한 같은 메모리의428700 생성자다.
    std::function<void(Sid)> refresh; // 두 종류 중 하나라도 있으면 부모의 현재가상88을 호출한다.
    std::function<bool(float,float)> needsEscape; // 4225b0의 현재 다리 칸/코드 비트 판정이다.
    std::function<std::uint32_t(std::uint32_t)> movementType; // 488fe0(type,0,0)의 현재 프로세스 타입이다.
    std::function<bool(Sid,std::uint32_t)> hasPath; // 4b1a90의 현재 부착 프로세스 존재다.
    std::function<void(Sid,float,std::uint32_t)> escape; // 429010(parent,1.0,ffffffff)의 실제 회피다.
    RegularHandler damageable; // 44b900의 실제 기본 사건 처리기다.
    std::function<float(Sid,std::uint32_t)> falling; // 426ef0의 실제5105 처리와 반환이다.
};
// 10.78의 전체 Carrier 부모와41d0d0의 좌표 제한을 복원한다.
class RawCarrierRegular {
public:
    RawCarrierRegular(SidPool& pool,std::span<const RiftTypeRecord> types,std::span<const CarrierRegularType> details,const bool& boss,CarrierRegularHooks hooks); // 같은188TYPE/권한/풀의 장기 참조를 받는다.
    bool Handles(Sid parent) const; // 실제+5c가42a750인 여섯 VT만 직접 받는다.
    float Handle(Sid parent,std::uint32_t event,std::uint32_t count,float payload) const; // 사제 등 파생 부모가 실제 기본 호출로도 사용할 수 있다.
    RegularHandler Handler(RegularHandler fallback={}) const; // 여섯 직접 VT 이외의 파생은 caller 분배를 보존한다.
    const SidPool& Pool() const; // 같은 Session/Host 연결을 확인한다.
private:
    SidPool& pool_; // Carrier/내용물/새 미리보기의 동일 raw 풀이다.
    std::span<const RiftTypeRecord> types_; // 현재 내용물 genus를 읽는188TYPE다.
    std::span<const CarrierRegularType> details_; // 같은 TYPE 번호의 기본 프레임/casttime이다.
    const bool& boss_; // 매492 사건에서 읽는 현재 권한 전역이다.
    CarrierRegularHooks hooks_; // 실제 큰 생성/부착/가상 효과와 기본 Handle다.
};
}
