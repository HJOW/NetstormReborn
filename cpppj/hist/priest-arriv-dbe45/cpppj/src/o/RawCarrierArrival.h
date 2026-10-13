// 실제 Carrier 도착의 현재 경로/내용물/다이스/운반 분기와 끝의 회피 검색 순서를 복원한다.
#pragma once
#include "o/RawContainedCreate.h"
#include "o/RawCarrierPreDestroy.h"
#include "o/RawPathCleanup.h"
#include "o/RawPlayerPlacementAnchor.h"
#include "o/RawPriestState.h"
#include "o/RawSquidNeighbors.h"
#include <string_view>

namespace netstorm::o {
// 실제 조회/효과 뒤 다시 읽는 도착 권한·동맹·타입·Player AI 존재 전역이다.
struct CarrierArrivalState {
    bool boss{true},debug{},allowAlliedPickup{}; // 현재 권한/패치Robust 검사/친구 사제 pickup 허용 전역이다.
    std::uint32_t priestType{158},nuggetType{154},daisType{165},geyserType{122},outpostType{126},isleStalagType{84}; // 원래 mutable 타입 getter들이다.
    std::array<std::uint32_t,81> alliances{}; // [carrierOwner*9+candidateOwner]와 반대 방향을 임의 통합하지 않는다.
    std::array<bool,9> controlledOwners{}; // 실제 Player+0xb0/CD+0xa8 AI 포인터의0 아님을 공급한다.
};
// 각 큰 운반/포획/경제/프로토콜/회피 몸체는 호출되면 필수 실제 효과를 공급해야 한다.
struct CarrierArrivalHooks {
    std::function<RawPathRuntime*(Sid)> path; // 기존 actual mapper/ContainedLookup/Host에서 현재 Path를 매 조회마다 읽는다.
    std::function<Sid(float,float,std::uint32_t)> genusAt; // 이미 복원한004b2070/CD004eb5c0의 공통 getter를 공급한다.
    std::function<bool()> cdPriestPickupPriest; // CD의 현재 이름있는설정 priestPickupPriest 조회이며 패치에는 이 조건이 없다.
    std::function<void(Sid)> pickup; // 원본00428e00/CD004e5800 전체 포획 요청 몸체다.
    std::function<std::uint32_t(Sid,float,std::uint32_t)> escape; //00429010/CD004e5c10의 현재 지연/사건 회피 반환이다.
    std::function<Sid(Sid)> chooseReturn,chooseGeyser; //00428ea0/CD004e5a90 및00429420/CD004e60d0의 실제 후보 선택이다.
    std::function<void(Sid,Sid)> goReturn,goGeyser; //004276a0/CD004e62d0 및00427600/CD004e6220의 실제 새 이동 요청이다.
    std::function<void(Sid)> minedPacket; // 현재 carrier/새 너겟 생성 뒤 지면 자산 SID의 실제 mined packet/RPC를 전송한다.
    std::function<void(Sid,std::uint32_t)> destroy; // 포획 Form/nugget의 실제 가상+10 삭제이며 빈 삭제를 공급하지 않는다.
    std::function<void(std::uint32_t,std::int32_t)> cdAddSp; // CD004d8cc0의 실제 현재owner SP 저장이다.
    std::function<void(std::uint32_t)> cdOwnerPacket; // CD0040abd0→004c0380의 실제 owner 갱신 요청/목록 흐름이다.
    std::function<void(float,float,std::string_view,std::uint32_t)> playAt; // CD 너겟 반환의 현재 returnPlace 중심 guy.wav/flags0이다.
    std::function<bool(Sid)> robust; // 패치 마지막004afd00의 현재 객체/내용물 전체 실제 검사를 공급한다.
};
// 큰 하위 효과 경계와 순수 조회를 분리하며 기존 완료 bodies를 다시 작성하지 않는다.
class RawCarrierArrival {
public:
    // 모든 풀/해시/타입/하위 처리기 참조와 mutable 상태는 이 객체보다 오래 살아야 한다.
    RawCarrierArrival(SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,
        const RawCarrierPreDestroy& carrier,const RawPathCleanup& cleanup,const RawPriestState& priest,
        const RawPlayerPlacementAnchor& anchor,const RawContainedCreate& create,const RawSquidNeighbors& footprint,
        CarrierArrivalState& state,CarrierArrivalHooks hooks);
    //00429800/CD004e66a0 전체 부모 몸체다. 선택된 큰 운반 하위 몸체는 명시 hooks로 연결한다.
    std::uint32_t Arrival(Sid sid,std::uint32_t targetA,std::uint32_t targetB) const;
    //00426900/CD004e4420: 현재 almostceil 지점의 첫 허용 pickup 후보와 genus2000 여부다.
    std::pair<Sid,bool> PickupCandidate(Sid sid,float x,float y) const;
    //00427580/CD004e5a10 및004274b0/CD004e5980의 현재 대상 검증 몸체다.
    bool ValidGeyser(Sid sid,Sid target) const;
    bool ValidReturn(Sid sid,Sid target) const;
    // 실제 사제+3c/걷기 어댑터가 다른 월드의 도착 처리기를 연결하는 일을 거부한다.
    const SidPool& Pool() const;
private:
    // 현재 raw/type/owner/extra는 하위 효과 뒤 같은 풀에서 다시 읽는다.
    std::array<float,2> Position(Sid sid) const;
    std::uint32_t Genus(Sid sid) const;
    std::uint32_t Owner(Sid sid) const;
    std::uint8_t Extra(Sid sid) const;
    bool Controlled(Sid sid) const; // 중립0은 AI 조회 없이 거짓이다.
    bool Allied(std::uint32_t first,std::uint32_t second) const; // 방향을 보존하는 원본9×9 현재표다.
    std::uint32_t CheckedDone(Sid sid) const; // 패치의 LAB00429b95 공통 Robust 접두 다음1000 반환이다.
    void SetTargets(Sid sid,std::uint32_t first,std::uint32_t second) const; //004261d0/CD004e3860의 기존Runtime.SetTargets 재사용이다.
    static std::int32_t Cell(float value,bool biased=false); // 지도 정상 signed절삭/좌표almostceil을 읽는다.
    SidPool& pool_; // 실제 부모/내용물/대상 raw를 읽는 현재 저장소다.
    const SquidHash& hash_; // 모든 현재 일반 finder의 네 단계 버킷이다.
    std::span<const RiftTypeRecord> types_; // 현재 genus/발자국/너겟 cost를 조회하는 원본 표다.
    const RawCarrierPreDestroy& carrier_; // 기존 실제 contained WORD/mask 조건 조회다.
    const RawPathCleanup& cleanup_; // 기존 실제 ReturnPlaceAt/NearbyType 몸체다.
    const RawPriestState& priest_; // 기존 실제 사제 지면/절반HP 불가 조회다.
    const RawPlayerPlacementAnchor& anchor_; // 기존 실제 GraphFor를 다시 구현하지 않는다.
    const RawContainedCreate& create_; // 기존 실제 Factory/ContentForm 생성·부착 몸체다.
    const RawSquidNeighbors& footprint_; // CD 위치 음향의 기존 실제 Center getter다.
    CarrierArrivalState& state_; // callback 뒤 현재 권한/동맹/Player 존재를 읽는다.
    CarrierArrivalHooks hooks_; // 현재 큰 하위 효과의 필수 실제 경계다.
};
}
