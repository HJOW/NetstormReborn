// 원본 공통 AI의 수집자 확보00415a10과 자기 영역 후보 배치00415920을 복원한다.
#pragma once
#include "o/RawAiBrain.h"
#include "o/RawCanonPlacement.h"
#include "o/SpStore.h"

namespace netstorm::o {
// 같은 원본 현재 시각/로컬 소유자/무효 그래프를 읽는다.
struct AiCollectorsGlobals {
    const double& now; //0055b4d0의 현재 QWORD 시각이다.
    const std::uint32_t& localOwner; //00540c70의 현재 로컬 소유자다.
    std::function<std::uint32_t()> invalidGraph; //00540410의 현재 무효 그래프다.
};
// 이미 복원된 실제 생산/비용/배치와 현재 사제 상태·로그/음향을 연결한다.
struct AiCollectorsHooks {
    std::function<Sid(std::uint32_t,std::uint32_t)> product; //00410360/004103c0의 같은 본체→현재 기술부모 mask8 조회다.
    std::function<bool(std::uint32_t,std::uint32_t)> canAfford; //00410280의 실제 signed 비용/지갑 비교다.
    std::function<std::int32_t(Sid)> maxHitPoints; //004adcb0의 현재 최대HP이며 실제HP는 호출 전에 저장한다.
    std::function<bool(std::uint32_t)> enemyAvailable; //00492280의 현재 Player flags22와 ownerState0이다.
    std::function<bool(const CanonPlacementQuery&)> mayPlace; //0049b510 전체이며 mode1/방향0/현재ownerBYTE를 전달한다.
    std::function<void(std::uint32_t,std::uint32_t,float,float,std::uint32_t,std::uint32_t)> deploy; //004106e0 실제 생산 소비/건설이다.
    std::function<void(std::uint32_t,std::uint32_t,std::int32_t,std::int32_t)> log; //004105b0의 현재 Lay 형식/로그 종류 장치다.
    std::function<void()> dropSound; //로컬 owner일 때만004a9cb0 droppiece.wav를 재생한다.
};
// 별도 공략/수집자 목록 없이 같은 본체/목록/TYPE/난수/원본 배치를 사용한다.
class RawAiCollectors {
public:
    RawAiCollectors(RawAiBrainMemory& memory,const SidPool& pool,std::span<const RiftTypeRecord> types,GameRandom& random,
        AiCollectorsGlobals globals,AiCollectorsHooks hooks); //같은 PATCH 풀/현재 TYPE만 먼저 확인한다.
    void Ensure(std::uint32_t token) const; //00415a10의 취약 사제용 비행 수집자 또는 일반 수집자 선택이다.
    void PlaceInHome(std::uint32_t token,std::uint32_t type) const; //00415920:자기 경계에서y→x 난수로 최대20회 후보를 찾는다.
    void DeployWithNotice(std::uint32_t token,std::uint32_t type,float x,float y,std::uint32_t argument,std::uint32_t direction) const; //004108f0의 실제 배치→생각 기한/로그→로컬 소리다.
    bool HasFlying(std::uint32_t token) const; //004115e0의 현재 수집자 목록 genus20000 검사다.
    bool Vulnerable(Sid priest) const; //00411ca0의 현재HP 선저장→최대HP/2 signed 비교다.
    const SidPool& Pool() const; //같은 실제 월드 연결을 확인한다.
private:
    std::span<std::uint8_t> Body(std::uint32_t token) const; //현재 실제0xb08 본체를 다시 조회한다.
    std::uint32_t Field(std::uint32_t token,std::size_t offset) const; //현재 DWORD를 읽는다.
    void Field(std::uint32_t token,std::size_t offset,std::uint32_t value) const; //지정 DWORD만 갱신한다.
    std::uint32_t Item(std::uint32_t token,std::size_t pointer,std::int32_t index) const; //현재 목록 포인터의 실제 DWORD를 읽는다.
    const RiftTypeRecord& Type(std::uint32_t type) const; //물리 범위 검사 뒤 현재 TYPE를 읽는다.
    Sid PhysicalSid(std::uint32_t value) const; //DWORD 번호의 물리 범위를 먼저 검사한다.
    bool Ready(std::uint32_t token,std::size_t offset) const; //원본 QWORD 기한≤현재 시각이며 unordered는 거짓이다.
    Sid AvailableProduct(std::uint32_t token,std::uint32_t type) const; //00410580:현재 mask8 조회 뒤 pending100을 거른다.
    void Notice(std::uint32_t token,std::uint32_t type,float x,float y) const; //00410620의 두 QWORD 생각 기한과 현재 로그를 기록한다.
    RawAiBrainMemory& memory_; //생성/자산 통지/전략이 같은 본체/목록을 공유한다.
    const SidPool& pool_; //사제와 현재 수집자의 실제 슬롯을 읽는다.
    std::span<const RiftTypeRecord> types_; //현재 생산/수집자의 flags2를 읽는다.
    GameRandom& random_; //기존 SP/생산/전투와 같은 원본 난수다.
    AiCollectorsGlobals globals_; //현재 전역의 참조/조회기다.
    AiCollectorsHooks hooks_; //선택된 실제 하위/장치 효과다.
};
}
