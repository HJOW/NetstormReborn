// 10.78 스톤 타워 배치의 기존 Gun 후보 수집·표적 교체 통지를 같은 실제 월드에 연결한다.
#pragma once
#include "o/RawFenceTarget.h"
#include <optional>

namespace netstorm::o {
// TYPE 초기화가 전체 번호를 순회해 만든 두 최대 탐색 거리와 현재 스톤 타워 번호다.
struct GunBlockerState {
    float cannonRange{},archerRange{}; // 0059ab18/0059ab1c이며 TYPE 사거리의 각각10/3 여유를 포함한다.
    std::uint32_t stoneTowerType{78}; // 00541190의 발자국을 새 장애물의 크기로 사용한다.
};
// 원본 Form/Kernel·현재 방향·포구와 기존 Gun의 표적 지정 경계다.
struct GunBlockerHooks {
    std::function<std::optional<Sid>(Sid)> currentTarget; // 004b1a90→004ae2d0: 총이 없으면nullopt, 표적0도 실제 값이다.
    std::function<std::uint32_t(Sid)> mode; // 004d4d00의 현재 공격 방향이다.
    std::function<std::array<float,2>(Sid)> muzzle; // 004819b0의 SHP 포구와 TYPE 핫스폿 보정 위치다.
    std::function<void(Sid,Sid)> setTarget; // 기존 GunProcess의0046a960을 호출한다. 새 Gun을 만들지 않는다.
};
// 일반 finder의 현재 해시/체인 순서와 발사기→네 캐논 방향 순서를 보존한다.
class RawGunBlocker {
public:
    // 같은 풀·해시·TYPE·기존 표적 getter와 방향별 동맹 상태를 참조로 빌린다.
    RawGunBlocker(const SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,
        std::span<const GunTargetType> targetTypes,const RawGunTarget& targeting,
        const GunProcessState& process,const GunBlockerState& state,GunBlockerHooks hooks);
    // 00469590: 현재 소유자/새 위치의 스톤 타워보다 먼 표적을 보는 기존 Gun SID를 모은다.
    // 통지는20DWORD, AI 평가는30DWORD 목록을 전달한다. capacity는 caller의 signed 용량이며 중복도 지우지 않는다.
    std::vector<std::uint32_t> Collect(std::uint32_t owner,std::array<float,2> point,std::size_t capacity=20) const;
    // 0046ae80: boss일 때 현재 타워의 소유자/원점으로 수집하고 아직 존재하는 각 Gun에 통지한다.
    void Notify(Sid blocker) const;
private:
    // 현재 raw 좌표와 타입 범위를 읽되 사망/추상 여부는 원본 수집 조건이 판단한다.
    std::span<const std::uint8_t> Slot(Sid sid) const;
    std::array<float,2> Position(Sid sid) const; // SID0도 원본 점수 계산에서 읽는 실제 raw 슬롯이다.
    GunTargetRect Bounds(Sid sid) const; // 확장 높이와 보드 clamp가 포함된 원본 표적 발자국이다.
    double Score(std::array<float,2> center,Sid target,bool archer) const; // 004693b0/00469440의 표적 경계점 제곱거리다.
    const SidPool& pool_; // 현재 Gun/표적/타워가 공유하는 raw 저장소다.
    const SquidHash& hash_; // 일반 finder의 네 단계 현재 버킷 머리다.
    std::span<const RiftTypeRecord> types_; // 실제 group/추상/발자국/공중 플래그다.
    std::span<const GunTargetType> targetTypes_; // 같은 번호의 현재 확장 높이다.
    const RawGunTarget& targeting_; // 실제 발자국 중심 getter를 같은 풀에서 재사용한다.
    const GunProcessState& process_; // 원본 boss/friendlyFire/방향별 동맹의 현재 값이다.
    const GunBlockerState& state_; // TYPE 초기화의 최대 사거리와 스톤 타워 번호다.
    GunBlockerHooks hooks_; // 실제 기존 Form/Kernel/방향/포구/표적 교체 경계다.
};
}
