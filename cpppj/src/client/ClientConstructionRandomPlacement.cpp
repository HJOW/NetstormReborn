// 실제난수배치부모와단일Factory/HP/가상Pop을같은장기Client물리에연결한다.
#include "client/ClientConstructionGlobals.h"
#include "client/ClientGameplayGlobals.h"
#include "client/ClientRandomGlobals.h"
#include "o/RawBattleNuggetPlacement.h"
#include <bit>
#include <iostream>
#include <stdexcept>

namespace netstorm::client { //새World/Player/전역seed없이기존장기몸체를조립한다.
// 같은원본난수owner만허용하며첫결합/재결합은GameRandom값을읽거나쓰지않는다.
void ClientConstructionGlobals::BindRandom(std::shared_ptr<ClientRandomGlobals> random) {
    if (!storage_ || !random || (random_ && random_!=random)) throw std::invalid_argument("난수 배치의 같은 실제 Client 난수 owner 필요");
    random_=std::move(random);
}
// 현재배치와기존지형/Farm이같은TYPE/ownerOverride를빌린다.
o::FortRandomPlacementState& ClientConstructionGlobals::RandomPlacementState() { static_cast<void>(State());return randomPlacementState_; }
// 최초지역/모드/거주구역TYPE를섬과기존후처리Caller가공유한다.
o::FortFinishOwnerState& ClientConstructionGlobals::FinishOwnerState() { static_cast<void>(State());return finishOwnerState_; }
// 현재오벨리스크TYPE는시계/진단효과뒤같은주소에서다시읽는다.
o::GemRewardState& ClientConstructionGlobals::GemState() { static_cast<void>(State());return gemState_; }
// spot/edgeFarm/MayPlace/region/frame/생성/HP/owner/Pop의기존본문순서를변경하지않는다.
std::uint32_t ClientConstructionGlobals::PlaceRandomSingle(o::GameRandom& random,const o::FortRandomPlaceRequest& request) {
    if (!storage_ || !random_) throw std::logic_error("단일 난수 배치의 실제 장기 물리/난수 미결합");
    o::FortRandomPlacementHooks hooks; //각효과는현재장기부품만선택하고Session주소를보관하지않는다.
    hooks.mayPlace=[this](const auto& query) { return MayPlace(query); };
    hooks.chunkRegion=[this](auto x,auto y) { return std::bit_cast<std::int8_t>(storage_->chunks.At(x,y).island); };
    hooks.regionOwner=[this](auto region) { return o::RawIslandOwner(storage_->islands.records,region,[this] { storage_->AssertDisplay("exists( islandId )","\\Ns\\O\\Islandlist.cpp",0x2fb); }); };
    hooks.create=[this](auto type,auto flags) { return storage_->Factory().Create(type,flags); }; //기존단일body와같은4af530이며World의보조changed접두를앞에더하지않는다.
    hooks.construct=[this](auto sid,auto type) { storage_->Factory().Construct(type,sid); };
    hooks.hasHitPoints=[this](auto sid) { return storage_->Reward().HasHitPoints(sid); };
    hooks.typeHitPoints=[this](auto type) { return storage_->Reward().TypeHitPoints(type); };
    hooks.setOwner=[this](auto sid,auto owner) { storage_->SetAssetOwner(sid,owner); };
    hooks.unpop=[this](auto sid,auto flags) { storage_->UnpopAsset(sid,flags); };
    hooks.repop=[this](auto sid,auto flags) { storage_->RepopAsset(sid,flags); };
    hooks.pop=[this](auto sid,auto x,auto y,auto flags) { static_cast<void>(storage_->PopAsset(sid,x,y,flags)); };
    o::RawFortRandomPlacement single(storage_->pool,storage_->hash,storage_->spots,storage_->types,storage_->frames,random_->Game(),randomPlacementState_,std::move(hooks));
    return single.Place(random,request);
}
// 원문0040e050의시계/지역seed/현재TYPE와0040ded0의실제반복배치를그대로연결한다.
void ClientConstructionGlobals::PlaceBattleNuggets(o::FortRandomRectangle rectangle,std::uint32_t quantity) {
    static_cast<void>(State());
    const auto wall=[this] { //선택한현재장치/함수를이번호출끝까지보유하고다음호출만새장치를읽는다.
        const auto current=devices_;if (!current || !current->wallSeconds) throw std::logic_error("너겟 배치의 실제 현재 벽시각 미결합");const auto call=current->wallSeconds;return call();
    };
    o::FortFinishOwnerHooks finish; //미선택Bounds/TYPE진단을앞당겨호출하지않는다.
    finish.regionBounds=[this](auto region) { return o::RawIslandBounds(storage_->islands.records,region,[this] { storage_->AssertDisplay("exists( islandId )","\\Ns\\O\\Islandlist.cpp",0x2ba); }); };
    finish.wallSeconds=wall;finish.invalidType=[] { std::clog<<"[29] AddRandom: Invalid Type\n"; }; //기존실제Player그룹1d의같은로그/복귀다.
    finish.place=[this](auto& random,const auto& request) { return PlaceRandomSingle(random,request); };
    o::RawFortFinishOwner placement(storage_->pool,storage_->types,storage_->sidLists.residences,finishOwnerState_,std::move(finish));
    o::BattleNuggetPlacementHooks hooks; //지역좌표난수와전역프레임난수를혼합하지않는다.
    hooks.singleAssertion=[this] { storage_->AssertDisplay("!inSingle","\\Ns\\O\\Addrandom.cpp",0xb4); };hooks.wallSeconds=wall;
    hooks.regionZero=[this] { return Body(sp_,players_,options_).PlayerBounds().RegionRecord(0); };
    hooks.place=[&placement](auto& random,auto bounds,auto region,auto count,auto type,auto force) { static_cast<void>(placement.PlaceRandom(random,bounds,region,count,type,force)); };
    o::PlaceBattleNuggets(rectangle,quantity,{gameplay_->world.missionPost,gameplay_->Boss(),gemState_.buriedType},hooks);
}
}
