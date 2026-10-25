// 실제edgeFarm 군집의 지역시드/archipelago/중심·주변순회와기존단일배치를같은월드에서실행한다.
#pragma once
#include "o/RawFortRandomPlacement.h"
#include "o/RawIslandRuntime.h"
#include "o/ChunkMap.h"

namespace netstorm::o {
// 군집의초기archipelago분기가읽는현재전역이다.
struct FarmClusterState {
    bool archipelago{}; // 원본군도모드이며지역0에서만군집범위/수량/반경을바꾼다.
    std::int32_t archipelagoSpan{1}; // 패치00542654/CD00537188의현재섬확장단위다.
    const std::uint32_t* sharedArchipelago{}; // Login.bb4와 다른 bb0을 직접 빌린다.
    bool Archipelago() const { return sharedArchipelago ? *sharedArchipelago!=0 : archipelago; } // bool의 물리 폭으로 대체하지 않는다.
};
// CD의지역record조회에서보고하고원본처럼계속진행하는두진단이다.
enum class FarmClusterAssertion {
    MissingIsland, // 현재IslandRuntime에요청지역의존재DWORD가없다.
    MissingRegionRecord // 현재ChunkMap에서요청지역의첫Territory record를찾지못했다.
};
// 실제단일배치와원본진단의필수효과이며군집의성공을기본값으로제공하지않는다.
struct FarmClusterHooks {
    std::function<std::uint32_t(GameRandom&,const FortRandomPlaceRequest&)> place; // 같은local난수와실제Place 전체를연결한다.
    std::function<void(FarmClusterAssertion)> assertion; // CD진단후record0/시드0으로계속되는흐름을보존한다.
};
// 패치0040e560/CD0042cd80와지역record시드helper를복원한다. 단일배치의완료몸체는재사용한다.
class RawFarmCluster {
public:
    // 동일pool/현재ChunkMap/실제3204BYTE 섬목록·edgeFarm타입과모드수명을호출월드가유지한다.
    RawFarmCluster(const SidPool& pool,const ChunkMap& chunks,const RawIslandRuntime& islands,
        const FortRandomPlacementState& placementState,const FarmClusterState& state,FarmClusterHooks hooks);
    // 수량×20 성공을목표로중심과주변을배치한다. 수량0도원본처럼첫중심을시도한다.
    void Place(FortRandomRectangle area,std::int32_t region,std::uint32_t quantity) const;
    // 동일월드의실제단일배치어댑터를확인한다.
    const SidPool& Pool() const;
private:
    // 현재청크를y바깥/x안쪽으로찾은첫지역record의BYTE+3을원본시드로읽는다.
    std::uint32_t RegionSeed(std::int32_t region) const;
    // pool_: 두판본의같은raw저장소와진단규칙을제공한다.
    const SidPool& pool_;
    // chunks_: 실제지역record와단일배치의지역조회가공유하는지도다.
    const ChunkMap& chunks_;
    // islands_: 실제CD Exists/validId가읽는같은3204BYTE mutable목록이다.
    const RawIslandRuntime& islands_;
    // placementState_: 매시도현재edgeFarm타입을읽는다. callback중바뀐전역도다음시도에반영한다.
    const FortRandomPlacementState& placementState_;
    // state_: 군집진입의archipelago/확장단위전역이다.
    const FarmClusterState& state_;
    // hooks_: 실제단일Place와원본지역진단이다.
    FarmClusterHooks hooks_;
};
// 같은pool의기존RawFortRandomPlacement::Place를필수군집효과에연결한다.
FarmClusterHooks MakeFarmClusterPlacementHook(const SidPool& pool,const RawFortRandomPlacement& placement,FarmClusterHooks hooks);
}
