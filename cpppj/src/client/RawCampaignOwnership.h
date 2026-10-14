// 실제 캠페인의 신전 Owner→IslandList→지역 투표/도장을 같은 Core에 조립한다.
#pragma once
#include "client/RawSurfaceWorld.h"
#include "o/RawAssetPostPop.h"
#include "o/RawCanonPlacementTerrain.h"
#include "o/RawIslandRuntime.h"
#include "o/RawRegionPaint.h"
#include "o/RawVortexOwner.h"
#include <memory>

namespace netstorm::client {
// 다른 파생 Owner와 생산/UI 진단의 실제 소비자는 상위 캠페인이 공급한다.
struct CampaignOwnershipEffects {
    std::function<void(o::Sid,std::uint32_t)> otherOwner; // 사제/일반/표면의 실제 가상 소유자 분배기다.
    std::function<void()> refreshProduction; // 신전 소유자 변경 뒤 실제 생산 목록을 갱신한다.
    std::function<std::uint32_t()> themeMode; // 패치 지역 투표가 매일치 읽는 실제 themeMode 전역이다.
    std::function<void(o::Sid,std::int32_t,std::uint32_t)> missingIsland; // 원본 신전의 현재 진단 효과다.
};
// Player/지역/도장 자료는 상위 월드의 같은 저장소를 빌린다. 별도 추정 섬/소유자 지도를 만들지 않는다.
class RawCampaignOwnership {
public:
    // 표면/프레임/난수/9 Player/세 목록과 원본 두 도장 상태를 동일 Core에 연결한다.
    RawCampaignOwnership(RawSurfaceWorld& world,o::RawIslandRuntime& islands,o::AssetPostPopState& assets,
        std::span<std::uint8_t> players,std::span<const o::PriestPlainCanonType> frames,
        std::span<const std::uint32_t> themes,o::GameRandom& random,o::RegionPaintState& paint,
        o::RegionOwnershipState& ownership,std::uint32_t& vortexRevision,std::uint32_t terrainType,
        std::uint32_t edgeFarmType,CampaignOwnershipEffects effects);
    // 이 모듈을 참조하는 AssetPostPop/Regular와 프로세스는 상위 월드가 먼저 해제한다.
    ~RawCampaignOwnership();
    RawCampaignOwnership(const RawCampaignOwnership&)=delete; // 훅이 빌린 내부 모듈 주소를 유지한다.
    RawCampaignOwnership& operator=(const RawCampaignOwnership&)=delete; // 다른 월드에 참조를 복사하지 않는다.
    // 신전의 실제 가상 표만 새 몸체로 보내고 다른 파생은 실제 상위 분배기에 전달한다.
    void SetOwner(o::Sid sid,std::uint32_t owner) const;
    // 현재 plain float 지면 getter와 실제 지역 투표를 자산/초기 로더에 공급한다.
    std::int32_t IslandNumber(float x,float y) const;
    void UpdateRegion(float x,float y) const;
    // 동일 풀/실제9기록을 상위 설치에서 확인한다.
    const o::RawVortexOwner& Vortex() const;
private:
    struct Impl; // 같은 Core와 실제 지면/신전 모듈을 고정 주소에 소유한다.
    std::unique_ptr<Impl> impl_; // 상위 월드의 저장소가 이 연결보다 오래 살아야 한다.
};
}
