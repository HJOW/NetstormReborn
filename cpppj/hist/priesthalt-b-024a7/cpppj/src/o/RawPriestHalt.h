// 사제 실제 +38 Halt와 공통 Carrier Halt의 지면/검색/반환 순서를 복원한다.
#pragma once
#include "o/RawCarrierCheck.h"
#include "o/RawPriestState.h"
#include "o/RawSquidFinder.h"

namespace netstorm::o {
// 실제 전역은 각 하위 효과 뒤 다시 조회되며 mutable 타입 번호도 보존한다.
struct PriestHaltState {
    bool battle{},boss{true}; // 00594fbc/CD00540a20 및 실제 공간 권한00540bc4/CD00540a2c다.
    std::uint32_t carrierType{156},nuggetCarrierType{187}; // 패치의 두 특정 타입이며 CD는 carrierType만 조회한다.
};
// 큰 운반체 포획/너겟 전달 몸체는 빈 효과로 대체하지 않는 필수 하위 경계다.
struct PriestHaltHooks {
    std::function<void(Sid,std::uint32_t)> unpop; // 사제 가상+48을 flags0으로 실행한다.
    std::function<void(Sid,float,float,std::uint32_t)> pop; // 최초 보관 좌표의 거의올림 결과를 가상+90에 flags0으로 넘긴다.
    std::function<std::uint32_t(Sid)> carrierPickup; // 패치00429ed0은 반환12, CD004e7980은 0 아닌 반환을 검사한다.
    std::function<std::uint32_t(Sid)> nuggetTransfer; // 패치0042a0d0의 실제 반환12를 검사하며 CD에는 이 분기가 없다.
};
// 기존 CarrierCheck/현재HP/MaxHP/일반 finder 몸체를 같은 월드에서 재사용한다.
class RawPriestHalt {
public:
    // 참조된 실제 저장소/상태/하위 효과는 이 객체보다 오래 살아야 한다.
    RawPriestHalt(const SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,
        std::span<const std::uint8_t> spots,const RawCarrierCheck& check,const RawPriestState& priest,
        const SquidReward& hitPoints,PriestHaltState& state,PriestHaltHooks hooks);
    // 실제 사제 가상+38: 지면 모드/저HP 보정 다음 공통 Carrier Halt로 위임한다.
    std::uint32_t Halt(Sid sid) const;
    // 0042a5f0/CD004e45c0 전체 접두/검색이다. 큰 하위 포획/전달은 명시 hooks에서 실행한다.
    std::uint32_t CommonHalt(Sid sid) const;
    // 실제 Path 부모/공간 처리기와 다른 풀을 섞는 연결을 거부할 때 사용한다.
    const SidPool& Pool() const;
private:
    // 현재 raw 타입/좌표는 하위 효과가 끝난 뒤 다시 읽는 공통 검색 입력이다.
    std::array<float,2> Position(Sid sid) const;
    // 원본 __ftol의 정상 지도 좌표만 읽고 손상 범위는 조회 전에 거부한다.
    static std::int32_t Cell(float coordinate,float bias);
    const SidPool& pool_; // 모든 현재 raw 입력과 SID의 실제 풀이다.
    const SquidHash& hash_; // 현재 사제와 운반 후보가 등록된 같은 공간 해시다.
    std::span<const RiftTypeRecord> types_; // 현재 finder 발자국/genus의 원본 타입 목록이다.
    std::span<const std::uint8_t> spots_; // 지면 모드의 거의올림 칸 spot6을 읽는 지도다.
    const RawCarrierCheck& check_; // 가상+cc의 기존 실제 지면/낙하 조회다.
    const RawPriestState& priest_; // 현재HP의 판본별 signed 폭과 기존 상태 조회다.
    const SquidReward& hitPoints_; // 현재 HP 모드를 반영한 기존 MaxHitPoints 몸체다.
    PriestHaltState& state_; // callback 뒤에도 현재 권한/타입 전역을 읽는다.
    PriestHaltHooks hooks_; // 호출된 미복원 하위 효과는 소비자에서 명시 예외로 드러낸다.
};
}
