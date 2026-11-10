// 신전·영역·섬의 실제 부모를 같은 Client 수명에 연결하고 Session 재생성을 요구하지 않는다.
#include "client/ClientConstructionGlobals.h"
#include "client/ClientGameplayGlobals.h"
#include "client/ClientKencloudGlobals.h"
#include "client/ClientPlayerRecords.h"
#include "client/ClientRandomGlobals.h"
#include "client/PriestPlacementAssets.h"
#include "o/RawVortexOwnerDiagnostic.h"
#include <bit>
#include <new> // 원문 new0x50의 확보 실패를 같은 선택 위치에서 보존한다.
#include <stdexcept>

namespace netstorm::client { //실제참조/함수수명만보유하고임시월드/Player를만들지않는다.
o::VortexAscendState& ClientConstructionGlobals::VortexAscendGlobals() { // 현재 noIsland 역할의 실제 주소만 연결하며 기존 상태를 초기화하지 않는다.
    if (!storage_) throw std::logic_error("장기 신전 상승의 실제 물리 미결합");vortexAscendState_.sharedNoIslandType=&storage_->links.noIslandType;return vortexAscendState_;
}
o::RawVortexAscend& ClientConstructionGlobals::VortexAscend() { // 같은 현재 섬/물리/난수와 실제 가상 삭제를 사용하는 한 본문이다.
    if (!storage_ || !random_) throw std::logic_error("장기 신전 상승의 실제 물리/난수 owner 미결합");
    if (!vortexAscend_) {
        o::VortexAscendHooks hooks;hooks.islandNumber=[this](auto x,auto y) { return IslandNumber(x,y); };
        hooks.reserveCandidateBuffer=[this] { auto memory=std::unique_ptr<std::uint8_t[]>(new(std::nothrow) std::uint8_t[0x50]);const bool allocated=memory!=nullptr;vortexCandidateBuffers_.push_back(std::move(memory));return allocated; }; // 중첩 호출별 실제80BYTE 확보 수명을 별도로 유지한다.
        hooks.releaseCandidateBuffer=[this] { if (vortexCandidateBuffers_.empty()) throw std::logic_error("장기 신전 상승 후보 버퍼의 반납 순서 불일치");vortexCandidateBuffers_.pop_back(); };
        hooks.destroy=[this](auto sid,auto flags) { storage_->DestroyAsset(sid,flags); }; // 실제 현재 가상 삭제를 선택하며 임시 World/성공값을 만들지 않는다.
        vortexAscend_=std::make_unique<o::RawVortexAscend>(storage_->pool,storage_->hash,storage_->spots,storage_->types,Islands(),random_->Game(),VortexAscendGlobals(),std::move(hooks));
    }return *vortexAscend_;
}
RawSurfaceVortexDevices MakeClientVortexDevices(const std::shared_ptr<ClientConstructionGlobals>& owner) { // weak 대여는 실제 선택 호출 끝까지 같은 전체 Construction을 보유한다.
    if (!owner) throw std::invalid_argument("장기 신전의 실제 Construction owner 누락");const auto acquire=[weak=std::weak_ptr<ClientConstructionGlobals>(owner)] { const auto current=weak.lock();if (!current) throw std::logic_error("장기 신전의 실제 Construction owner 반납");return current; }; // 같은 장기 owner만 조회하고 본체/상태를 초기화하지 않는다.
    RawSurfaceVortexDevices devices;devices.ascend=[acquire](auto sid,auto count) { const auto hold=acquire();return hold->VortexAscend().Tick(sid,count); };devices.foreignIslands=[acquire](auto ownerId) { const auto hold=acquire();return hold->VortexAscend().CountForeignOwnedIslands(ownerId); };devices.islandNumber=[acquire](auto x,auto y) { const auto hold=acquire();return std::bit_cast<std::uint32_t>(hold->IslandNumber(x,y)); };return devices;
}
// 최초 메뉴 전에 현재 실제 DWORD 주소만 연결하며 재결합은 같은 owner만 허용한다.
void ClientConstructionGlobals::BindRegionContext(std::shared_ptr<ClientKencloudGlobals> kencloud) {
    if (!storage_ || !gameplay_ || !kencloud || (kencloud_ && kencloud_!=kencloud)) throw std::invalid_argument("장기 영역의 같은 실제 redraw owner 필요");
    kencloud_=std::move(kencloud);regionPaint_.sharedWorldRedraw=&kencloud_->WorldRedraw();
    regionVotes_.sharedWorldRedraw=&kencloud_->WorldRedraw();regionVotes_.sharedMapRevision=&gameplay_->MapRevision();
}
// 장기 상태는 실제 backing 결합 후에만 빌리며 기존 캐시를 지우지 않는다.
o::RegionPaintState& ClientConstructionGlobals::RegionPaintGlobals() { if (!kencloud_) throw std::logic_error("장기 영역 redraw 미결합");return regionPaint_; }
// 같은 현재 영향을 받는 지역 목록을 실제 투표/도장이 공유한다.
o::RegionOwnershipState& ClientConstructionGlobals::RegionVoteGlobals() { static_cast<void>(RegionPaintGlobals());return regionVotes_; }
// 현재 장치를 선택한 호출은 재결합/해제 중에도 자기 함수 사본을 유지한다.
void ClientConstructionGlobals::RefreshRegionProduction() {
    const auto current=devices_;if (!current || !current->refreshProduction) throw std::logic_error("장기 영역의 실제 현재 생산 장치 미결합");
    const auto refresh=current->refreshProduction;refresh();
}
// 이 getter는 배치 후보/모양 순회를 실행하지 않고 현재 실제 RegionAt만 사용한다.
o::RawCanonPlacementTerrain& ClientConstructionGlobals::RegionTerrain() {
    if (!storage_) throw std::logic_error("장기 영역의 실제 물리 미결합");
    if (!regionTerrain_) {
        o::CanonPlacementTerrainHooks guards; //지역조회외경로를잘못선택하면기존큰배치본체를대체하지않고알린다.
        guards.candidatePermission=[](const auto&,auto)->bool { throw std::logic_error("장기 영역 getter의 배치 후보 경계"); };
        guards.shapePermission=[](const auto&,const auto&)->bool { throw std::logic_error("장기 영역 getter의 배치 모양 경계"); };
        guards.finishRelations=[](const auto&,auto&)->bool { throw std::logic_error("장기 영역 getter의 배치 완료 경계"); };
        regionTerrain_=std::make_unique<o::RawCanonPlacementTerrain>(storage_->pool,storage_->types,storage_->PlacementAssets().frames,storage_->hash.Entries(0),regionGetterState_,std::move(guards));
    }
    return *regionTerrain_;
}
// 0046f530의0041d7a0 편향/저장/절삭은 기존 실제 지역 어댑터와 동일하다.
std::int32_t ClientConstructionGlobals::IslandNumber(float x,float y) {
    if (!storage_) throw std::logic_error("장기 지역 getter의 실제 물리 미결합");
    const auto hooks=o::MakeTerrainRegionOwnershipHooks(storage_->pool,RegionTerrain(),{});return std::bit_cast<std::int32_t>(hooks.regionAt(x,y));
}
// 실제 현재 Player는 첫 지역 필드가 선택될 때만 읽고 결합 시 할당하지 않는다.
o::RawIslandRuntime& ClientConstructionGlobals::Islands() {
    if (!storage_ || !kencloud_ || !players_) throw std::logic_error("장기 섬의 실제 backing 미결합");
    if (!islands_) {
        o::IslandRuntimeHooks hooks; //같은거주구역가상owner와원문정확식/행진단을사용한다.
        hooks.setResidenceOwner=[this](auto sid,auto owner) { storage_->SetAssetOwner(sid,owner); };
        hooks.assertionAt=[this](auto expression,auto line) { storage_->AssertDisplay(expression,"\\Ns\\O\\Islandlist.cpp",line); };
        auto body=std::make_unique<o::RawIslandRuntime>(storage_->pool,storage_->hash,storage_->islands.records,FinishOwnerState(),storage_->sidLists.residences,kencloud_->WorldRedraw(),gameplay_->MapRevision(),std::move(hooks));
        body->BindCurrentPlayers([records=players_] { return records->Records(); });islands_=std::move(body);
    }
    return *islands_;
}
// 도장마다 현재 두 TYPE를 읽으며 원문 drawer의 같은 장기 캐시/난수만 빌린다.
void ClientConstructionGlobals::PaintRegion(const o::RegionOwnershipPaint& request) {
    if (!random_) throw std::logic_error("장기 영역 도장의 실제 난수 미결합");
    o::RegionPaintHooks hooks;hooks.setOwner=[this](auto sid,auto owner) { storage_->SetAssetOwner(sid,owner); };
    hooks.boundaryAt=[this](auto x,auto y,auto type) { return storage_->Connections().FindTypeAt(x,y,type); };
    hooks.regionAt=[this](auto x,auto y) { return std::bit_cast<std::uint32_t>(IslandNumber(x,y)); };
    const auto& auxiliary=*storage_->GameAux();o::RawRegionPaint body(storage_->pool,storage_->frames,storage_->hash.Entries(0),auxiliary.TerrainType(),auxiliary.EdgeFarmType(),random_->Game(),RegionPaintGlobals(),RegionVoteGlobals(),std::move(hooks));
    body.Paint(request);
}
// 일반 추가목록과 신전목록·현재 TYPE+98은 Session의 테마 사본과 구별한다.
o::RawRegionOwnership& ClientConstructionGlobals::Regions() {
    if (!storage_) throw std::logic_error("장기 영역 투표의 실제 물리 미결합");
    if (!regions_) {
        o::RegionOwnershipHooks hooks;hooks.paint=[this](const auto& request) { PaintRegion(request); };
        hooks.themeMode=[this] { const auto current=devices_;if (!current || !current->themeMode) throw std::logic_error("장기 영역의 실제 현재 theammode 장치 미결합");const auto read=current->themeMode;return read(); };
        hooks.typeTheme=[this](auto type) { return std::bit_cast<std::uint32_t>(storage_->types.at(type).theme); };
        hooks=o::MakeTerrainRegionOwnershipHooks(storage_->pool,RegionTerrain(),std::move(hooks));
        regions_=std::make_unique<o::RawRegionOwnership>(storage_->pool,storage_->sidLists.outposts,storage_->sidLists.temples,std::span<const std::uint32_t>{},RegionVoteGlobals(),std::move(hooks));
    }
    return *regions_;
}
// 기존 전체 신전 Owner를 같은 섬/투표/Player와 공통 Owner에 조립한다.
o::RawVortexOwner& ClientConstructionGlobals::VortexOwner() {
    if (!players_) throw std::logic_error("장기 신전의 실제 Player owner 미결합");
    if (!vortexOwner_) {
        o::VortexOwnerHooks hooks;hooks.islandNumber=[this](auto x,auto y) { return IslandNumber(x,y); };hooks.islandExists=[this](auto island) { return Islands().Exists(island)!=0; };
        hooks.setIslandOwner=[this](auto island,auto owner) { Islands().Owner(island,owner); };hooks.regionOwnership=[this](auto x,auto y) { Regions().UpdateNative(x,y); };
        hooks.refreshProduction=[this] { RefreshRegionProduction(); };
        hooks.missingIsland=[this](auto sid,auto island,auto owner) { o::ReportVortexOwnerMissingIsland(storage_->pool,sid,island,owner,Islands().Count(),{
            [this](auto text) { storage_->RecordAssertionDetail(text); },[this](auto expression,auto file,auto line) { storage_->AssertDisplay(expression,file,line); }}); };
        vortexOwner_=std::make_unique<o::RawVortexOwner>(storage_->pool,storage_->Owner(),[records=players_] { return records->Records(); },storage_->sidLists.outposts,gameplay_->MapRevision(),std::move(hooks));
    }
    return *vortexOwner_;
}
// 삭제의 실제 목록→섬→영역→Damageable→UI→현재Player 순서를 생략하지 않는다.
o::RawVortexPreDestroy& ClientConstructionGlobals::VortexPre() {
    if (!players_) throw std::logic_error("장기 신전 삭제의 실제 Player owner 미결합");
    if (!vortexPre_) {
        o::VortexPreDestroyHooks hooks;hooks.islandNumber=[this](auto x,auto y) { return IslandNumber(x,y); };hooks.islandExists=[this](auto island) { return Islands().Exists(island)!=0; };
        hooks.islandTemple=[this](auto island) { return Islands().Temple(island); };
        hooks.clearIslandTemple=[this](auto island) { if (!Islands().Exists(island)) storage_->AssertDisplay("islandList.exists( islandId )","\\Ns\\O\\Vortex.cpp",0x16f);Islands().Temple(island,0);RefreshRegionProduction(); };
        hooks.islandOwner=[this](auto island,auto owner) { Islands().Owner(island,owner); };hooks.regionOwnership=[this](auto x,auto y) { Regions().UpdateNative(x,y); };
        hooks.damageablePre=[this](auto sid,auto flags) { storage_->DamageablePre().PreDestroy(sid,flags); };hooks.refreshProduction=[this] { RefreshRegionProduction(); };
        hooks.assertion=[this](auto line) { storage_->AssertDisplay(line==0x2ab ? "islandList.exists( islandId )":"islandList.getVortexSid( islandId ) == getSid()","\\Ns\\O\\Vortex.cpp",line); };
        vortexPre_=std::make_unique<o::RawVortexPreDestroy>(storage_->pool,[records=players_] { return records->Records(); },storage_->sidLists.temples,storage_->sidLists.residences,storage_->sidLists.outposts,storage_->LoadingDepth(),std::move(hooks));
    }
    return *vortexPre_;
}
}
