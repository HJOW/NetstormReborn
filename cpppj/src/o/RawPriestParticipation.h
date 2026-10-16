// 사제의 살아 있는 목록과 포획 원소유자를 같은 실제 월드에서 찾아 참여 상태를 갱신한다.
#pragma once
#include "o/RawPriestLookup.h"
#include "o/RawCarrierPreDestroy.h"
#include "o/RawSquidFinder.h"

namespace netstorm::o {
class RawAltarSacrifice; // 이미 설치된 제단이 있으면 동일한 실제 DaisAt 몸체를 재사용한다.
// 세 타입 전역은 매 조회 시 읽으며 호출자는 실제 세션의 장기 저장소를 공급한다.
struct PriestParticipationState {
    const std::uint32_t& priestType; // 005412d0 / CD0051cbbc의 현재 사제 타입 DWORD다.
    const std::uint32_t& altarType; // 005412e8 / CD0051cbd4의 현재 제단 타입 DWORD다.
    const std::uint32_t& daisType; // 005412ec / CD0051cbd8의 현재 다이스 타입 DWORD다.
};
// 00494f10/00495060의 전체 참여 조회와 기존 ownerState00595428 갱신이다.
class RawPriestParticipation {
public:
    // live 목록·Carrier·일반 typed Dais 조회가 모두 같은 풀을 사용한다. 참조 대상은 더 오래 살아야 한다.
    RawPriestParticipation(const SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,
        PriestPostPopState& priests,const RawCarrierPreDestroy& carrier,PriestParticipationState state);
    // 실제 제단 몸체가 설치되어 있으면 그 DaisAt을 사용하고 다른 풀은 첫 조회 전에 거부한다.
    RawPriestParticipation(const SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,
        PriestPostPopState& priests,const RawCarrierPreDestroy& carrier,PriestParticipationState state,const RawAltarSacrifice& altar);
    // 살아 있는 목록의 첫 SID, 첫 일치 Carrier, 마지막 일치 Altar 순서다. 없으면0이다.
    // 풀 순회는 SID2부터 free bit1만 건너뛰고 현재 상태/원소유자와 타입을 읽는다.
    std::uint32_t Find(std::uint32_t player) const;
    // 조회는 유효하지 않은 소유자에도 수행한다. 결과 이후1..8만 기존 ownerState에0/1/2를 쓴다.
    void Refresh(std::uint32_t player) const;
    // 로그인 종료 등 실제 소비자가 같은 월드/같은 ownerState인지 확인할 읽기 전용 접근이다.
    const SidPool& Pool() const;
    const PriestPostPopState& Priests() const;
private:
    // 00448bb0의 실제 좌표 절삭·현재 다이스 타입·일반4단계 finder를 재사용한다.
    Sid DaisAt(Sid altar) const;
    const SidPool& pool_; // 후보와 종속 및 Dais의 현재 raw 바이트를 공급하는 유일한 풀이다.
    const SquidHash& hash_; // 동일 월드의 현재4단계 버킷 머리다.
    std::span<const RiftTypeRecord> types_; // 각 후보의 현재 genus와 typed 조회의 flags2를 읽는다.
    PriestPostPopState& priests_; // 사제 등록/제거 목록과 참여 결과9DWORD를 그대로 공유한다.
    const RawCarrierPreDestroy& carrier_; // 실제004ac9f0의 종속 타입/kind/mask 조회 몸체다.
    PriestParticipationState state_; // 복사된 판정값이 아닌 실제 현재 타입 전역의 참조다.
    const RawAltarSacrifice* altar_{}; // 설치된 경우 기존 실제 제단 조회를 호출한다. 없으면 동일 typed finder를 직접 사용한다.
};
}
