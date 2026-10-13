// 일반 지형 성장과 초기 도장·섬별 edgeFarm을 같은 실제 표면 월드의 소비자로 조립한다.
#pragma once
#include "client/RawTerrainWorldBinding.h"
#include "o/RawTerrainGrowth.h"
#include "o/RawFarmCluster.h"
#include "o/RawCarrierPostPop.h"
#include <memory>

namespace netstorm::client {
// 호출자가 실제 상위 분기를 확인한 결과다. 군도와 미복원 부모는 정상 지형으로 대체하지 않는다.
enum class NormalTerrainBranch {
    Normal, // 004c1510/CD0049e620의 일반 성장 분기를 실행한다.
    Archipelago, // 섬 확장·플레이어 배치를 포함하므로 이 소비자는 거부한다.
    UnrestoredParent // 전체 Fort/IslandBuilder 로드 완료로 해석할 수 없는 요청을 거부한다.
};
// 원본 일반 wrapper에 들어가는 소유자·청크 범위·선택 지역과 확인한 분기다.
struct NormalTerrainRequest {
    std::uint32_t owner{}; // context가 저장하는 요청 소유자이며 초기 지면의 owner0과 구별한다.
    o::ChunkRect bounds{}; // right/bottom을 제외하는 현재 실제 청크 범위다.
    std::int32_t selectedRegion{127}; // 현재 noRegion이면 모든 섬의 후처리를 실행한다.
    NormalTerrainBranch branch{NormalTerrainBranch::UnrestoredParent}; // 호출자가 정상 분기임을 명시해야 한다.
};
// 기존 Bounds/첫 record helper가 보고하는 진단을 서로 구분한다.
enum class TerrainFarmPostpassAssertion {
    RecordMissingIsland, // CD 첫 record 조회의 현재 Exists 검사가 실패했다.
    RecordNotFound, // CD 첫 record 조회가 해당 지역 청크를 찾지 못했다.
    BoundsMissingIsland, // 두 판본의 Bounds 앞 현재 Exists 검사가 실패했다.
    BoundsNotFound // CD Bounds의 found assertion이 실패했다.
};
// 실제 진단 경계는 모두 필수이며 생성/HP/프레임/공간 효과를 대신 제공하지 않는다.
struct NormalTerrainDiagnostics {
    std::function<void(o::TerrainContextAssertion)> context; // 원본 context의 bounds 진단이다.
    std::function<void(o::TerrainStampAssertion)> stamp; // 실제 frame 검색과 stamp 타입 진단이다.
    std::function<void(o::SquidRegionAssert)> region; // 원본 지역 setter의 판본별 진단이다.
    std::function<void(o::FarmClusterAssertion)> farm; // 군집 내부 지역 seed 조회의 진단이다.
    std::function<void(TerrainFarmPostpassAssertion)> postpass; // 상위의 첫 record/Bounds 조회 진단이다.
};
// 기존 전체 배치·Factory·HP getter를 현재 ChunkMap/섬 owner·가상 공간/owner에 연결한다.
// placement는 이 world.Core()의 types/frames/shapes/spots/hash/장부로 구성해야 한다.
o::FortRandomPlacementHooks MakeNormalTerrainFarmWorldHooks(RawSurfaceWorld& world,const o::ChunkMap& chunks,
    const o::RawIslandRuntime& islands,o::RawCanonPlacementPipeline& placement,const o::SquidReward& hp);

// 기존 복원 몸체를 재작성하지 않고 같은 저장소/난수/가상 효과로 실행하는 client 소비자다.
class RawNormalTerrainWorldBinding {
public:
    // 모든 참조는 소비자보다 오래 살아야 한다. carrier는 실제 Damageable/base 후처리와 지면 owner 효과를 포함해야 한다.
    RawNormalTerrainWorldBinding(RawSurfaceWorld& world,const o::ChunkMap& chunks,const o::RawIslandRuntime& islands,
        o::GameRandom& random,std::uint32_t& crt,o::TerrainContextGlobals& contextGlobals,o::TerrainStampState& stampState,
        o::FortRandomPlacementState& placementState,o::FarmClusterState& farmState,std::uint32_t& puzzlePieceType,
        o::RawCanonPlacementPipeline& placement,const o::SquidReward& hp,const o::RawCarrierPostPop& carrier,
        NormalTerrainDiagnostics diagnostics);
    // 내부 군집 훅이 이 소비자가 소유한 단일 배치의 주소를 빌리므로 복사/이동을 금지한다.
    RawNormalTerrainWorldBinding(const RawNormalTerrainWorldBinding&)=delete;
    RawNormalTerrainWorldBinding& operator=(const RawNormalTerrainWorldBinding&)=delete;
    RawNormalTerrainWorldBinding(RawNormalTerrainWorldBinding&&)=delete;
    RawNormalTerrainWorldBinding& operator=(RawNormalTerrainWorldBinding&&)=delete;
    // Context→성장→MarkEdges→Stamp→Normalize→context 해제→현재 섬별 군집의 순서다.
    // 마지막 초기 지면 SID는 관찰 편의용이며 군집 반환값1을 새 SID로 해석하지 않는다.
    o::Sid Generate(const NormalTerrainRequest& request);
    // 이미 초기 지면을 등록한 실제 월드에서 상위의 섬별 후처리만 실행한다. 군도 상태는 거부한다.
    void PostprocessFarms(std::int32_t selectedRegion);
    // 004bfde0/CD0040b9d0: 현재 puzzlePiece 프레임과 기존 CanonDecoder가 통과한 패턴 칸만 센다.
    std::uint32_t PatternCount(std::span<const std::uint8_t,6> record) const;
    // 같은 월드에 연결했는지 검사할 실제 풀 참조다.
    const o::SidPool& Pool() const;
private:
    // 현재 normal 전역의 같은 타입/지역 값과 군도 거부 조건을 효과 전에 확인한다.
    void RequireNormal() const;
    // 첫 record는 Y행/X청크 순서로 읽고 CD 진단 뒤 원본의 실제 영 레코드 반환을 보존한다.
    std::array<std::uint8_t,6> FirstRecord(std::int32_t region) const;
    // world_: 현재 실제 Factory/가상 Pop/표시·장부의 소유자다.
    RawSurfaceWorld& world_;
    // core_: 같은 월드 모듈의 참조만 보관하며 배열이나 풀을 복사하지 않는다.
    RawWorldCore core_;
    // chunks_/islands_: 성장·도장 지역·단일 배치·군집이 공유하는 실제 지도와3204BYTE 목록이다.
    const o::ChunkMap& chunks_;
    const o::RawIslandRuntime& islands_;
    // random_/crt_: 게임 프레임 난수와 독립 CRT holdrand의 원본 저장소다.
    o::GameRandom& random_;
    std::uint32_t& crt_;
    // contextGlobals_/stampState_: 현재 문자 mapping 전역과 수명이 긴 초기99cache다.
    o::TerrainContextGlobals& contextGlobals_;
    o::TerrainStampState& stampState_;
    // placementState_/farmState_: 매 실제 배치가 읽는 타입·ownerOverride·군도 전역이다.
    o::FortRandomPlacementState& placementState_;
    o::FarmClusterState& farmState_;
    // puzzlePieceType_: 패턴Count가 매 요청 읽는 현재 타입 전역이다.
    std::uint32_t& puzzlePieceType_;
    // diagnostics_: 호출자가 원본 진단의 효과를 처리하며 빈 callback은 거부한다.
    NormalTerrainDiagnostics diagnostics_;
    // single_/farms_: 완료된 두 배치 몸체를 같은 풀과 실제 효과에 연결한 인스턴스다.
    std::unique_ptr<o::RawFortRandomPlacement> single_;
    std::unique_ptr<o::RawFarmCluster> farms_;
};
}
