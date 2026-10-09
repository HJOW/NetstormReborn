// Player 배치 기준점의 현재 작업장/추가 목록·그래프·종속 조건과 거리 선택을 복원한다.
#pragma once
#include "o/RawCanonPlacementPermission.h"
#include "o/RawContainedFinder.h"
#include "o/SquidPostPop.h"

namespace netstorm::o {
struct PlayerPlacementAnchorState {
    std::uint32_t graphReady{},invalidGraph{254},ignoreRestrictions{}; // 실제 그래프 활성·무효 번호·종속 조건 우회 전역이다.
    std::uint32_t fenceType{},windArcherType{}; // 패치 raw graph getter가 surface 이외에도 허용하는 두 현재 타입 번호다.
};
struct PlayerAnchorSelection {
    std::uint32_t sid{}; // 후보가 하나면 그대로, 여러 개면 소유자/거리 선택의 원본 반환이다.
    std::vector<std::uint32_t> candidates; // 순서·중복·40개 상한을 유지한 함수 내부 임시 목록의 관찰이다.
};
class RawPlayerPlacementAnchor {
public:
    // 원본 별도 추가 목록을 공통 factories와 구분한다. 모든 자료는 이 객체보다 오래 살아야 한다.
    RawPlayerPlacementAnchor(const SidPool& pool,std::span<const RiftTypeRecord> types,std::span<const std::uint16_t> surfaces,
        const SquidPostPopState& bookkeeping,const SquidPostPopList& additional,const ContainedFinderState& contained,
        const PlayerPlacementAnchorState& state);
    // 0048fdb0 / CD 00406f80: 그래프→작업장→조건부 추가 목록→하나/거리 선택 순서의 DWORD 반환이다.
    std::uint32_t Locate(std::uint32_t owner,std::uint32_t type,float x,float y) const;
    // 같은 몸체의 임시 후보 목록을 함께 돌려주어 원본 순서/제한 관찰을 대조한다.
    PlayerAnchorSelection Inspect(std::uint32_t owner,std::uint32_t type,float x,float y) const;
    // 00462b80 / CD 0045b670: 0.9999를 더해 0쪽 절삭한 surface 지도에서 raw graph BYTE를 읽는다.
    std::uint32_t GraphAt(float x,float y) const;
    // 00462bc0 / CD 0045b6f0: SID 0→surface raw 번호 또는 비표면 좌표 조회다.
    std::uint32_t GraphFor(std::uint32_t sid) const;
    // 0048fd30 / CD 00406ef0: owner가 같은 후보의 거리/float 최소값(초기 9999)을 비교한다. CD는 NaN도 채택한다.
    std::uint32_t Nearest(std::span<const std::uint32_t> candidates,std::uint32_t owner,float x,float y) const;
    // 권한 함수가 같은 raw 풀만 연결하도록 참조를 제공한다.
    const SidPool& Pool() const;
private:
    // WORD SID로 좁히기 전에 실제 풀과 번호의 물리 범위를 검사한다.
    std::span<const std::uint8_t> Raw(std::uint32_t sid) const;
    // 패치의 surface/특수 타입 assert 계약을 보존하고 graph BYTE를 읽는다.
    std::uint32_t RawGraphByte(std::uint32_t sid) const;
    // 기존 실제 contained 커서의 타입·mask 0x10·요청 kind 조건을 사용한다.
    bool HasContained(std::uint32_t parent,std::uint32_t kind) const;
    const SidPool& pool_;
    std::span<const RiftTypeRecord> types_;
    std::span<const std::uint16_t> surfaces_;
    const SquidPostPopState& bookkeeping_;
    const SquidPostPopList& additional_;
    const ContainedFinderState& contained_;
    const PlayerPlacementAnchorState& state_;
};
// 같은 풀의 실제 Player 조회를 후보 권한의 필수 경계에 연결한다.
std::function<std::uint32_t(std::uint32_t,std::uint32_t,float,float)> MakePlayerAnchorQuery(
    const SidPool& pool,const RawPlayerPlacementAnchor& anchor);
}
