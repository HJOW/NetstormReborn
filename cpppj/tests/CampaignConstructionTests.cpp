// 보호 TYPE188/프레임/SHP와 같은 실제 공간/삭제/간헐천 후처리에서 받침 확정 사슬을 검사한다.
#include "DudeSpawnSupport.h"
#include "client/PriestPlacementAssets.h"
#include "client/RawCampaignConstructionBindings.h"
#include "o/RawAssetPostPop.h"
#include "o/RawFormPacket.h"
#include "o/RawGeyserAiRegistration.h"
#include "o/RawSquidFinder.h"
#include <algorithm>
#include <memory>

using namespace netstorm::client;
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 검사 공용 수정 TYPE74 대신 보호 GameAssets의 실제188프레임을 그대로 읽는다.
std::vector<RiftTypeFrames> ProtectedConstructionFrames(const GameAssets& game) {
    std::vector<RiftTypeFrames> frames(game.TypeTable().Types().size(),RiftTypeFrames({}));
    // 원본 물리 블록과 TYPE 번호70의 대응만 적용한다.
    for (const auto& asset:game.Types()) frames[70+asset.block]=asset.definition.FrameTable();
    return frames;
}
// 실제 미복원 효과를 기본 관찰/성공으로 채우지 않는다.
template<class Result,class... Arguments> std::function<Result(Arguments...)> ConstructionForbidden() {
    return [](Arguments...)->Result { throw std::logic_error("보호 받침/간헐천 검사에서 선택되지 않아야 할 효과"); };
}
// 장치 없이 실제 보호 자산/공간/후처리/삭제/송신 분배를 구성한다. 부모 로그인/미션 진행 검사는 별도다.
struct ProtectedConstructionScene {
    const GameAssets& assets; // 공용 보호 자산의 실제 GameAssets이며 수정74 표를 사용하지 않는다.
    PriestPlacementAssets metadata; // 실제 Canon 프레임/핫스폿/SHP를 소유한다.
    std::vector<RiftTypeFrames> frames; // 보호 자산의 실제188개 프레임 표다.
    RawSurfaceWorld world; // 유일한 실제Pool/hash/spot/Graph/삭제/Kernel의 소유자다.
    RawWorldCore core; // 같은 월드의 원본 배열과 실제 모듈 참조다.
    GameRandom random{0x12345678}; // 실제 SP 순열 초기화의 명시 난수 입력이다.
    ScrambledSpStore sp; // 실제 패치 SP 저장소이며 건설 비용 경계와 삭제 보상이 공유한다.
    SquidRewardState reward; // 삭제 보상과 로컬 생성이 같은 geyserPool을 읽는다.
    SquidReward hp; // 같은 Pool/장부/SP의 실제 HP/보상 몸체다.
    std::array<std::uint8_t,9*0xb4> players{}; // 현재 flags/region/AI 포인터의 실제9기록이다.
    BattlePacketPeers peers; // 단독 캠페인의 실제 NULL 연결 표다.
    std::uint32_t localOwner{1}; // 현재 송신 분배의 로컬 Player DWORD다.
    bool server{true},mission{}; // 실제 단독 서버와 이 검사에서 선택한 비미션 전투 배치 입력이다.
    std::uint8_t mapSize{}; // 현재 옵션0의 Player 배치 간격 입력이다.
    std::uint32_t piece{107}; // 같은 실제 영역패턴 TYPE 전역이다.
    std::array<std::uint32_t,9> directions{16448,4,0,6,2,7,3,5,1}; // 보호 PE의 Player0값까지 보존한 현재 방향표 초기값이다.
    ChunkMap chunks; // PlayerBounds가 읽는 실제16×16청크 지도다.
    RawBattleBroadcast broadcast; // 실제 연결 표/Player 기록에 원본 송신 순회를 실행한다.
    FormPacketHeaders formHeaders; // 삭제 writer의 실제 초기 등록 헤더다.
    FormDeletionStamp stamp; // 같은 현재 삭제 표식의 저장소다.
    RawFormPacket formWriter; // common 삭제의 실제8바이트 패킷을 작성한다.
    std::array<std::uint8_t,2048> formBuffer{}; // 삭제 송신의 실제 충분한 공유 버퍼다.
    CarrierPostPopState carrierState; // 실제Damageable 후처리의 현재 권한/로딩 전역이다.
    std::unique_ptr<RawCarrierPostPop> carrier; // 실제공통깊이/지면처리몸체다.
    AssetPostPopState assetState; // 전체 간헐천 목록과 실제postPop전역이다.
    RawGeyserAiRegistration ai; // 현재Player의null AI를원본처럼자연반환한다.
    std::unique_ptr<RawAssetPostPop> assetPostPop; // 실제Geyser의수량/목록/Regular/받침후처리몸체다.
    RawCampaignConstructionState state; // 바인딩의장기실제Confirm/Clear/Place/Notice/Local전역이다.
    SquidPostPopList recent; // 전체목록과분리한실제최근5칸저장소다.
    DirtyRegions dirty{1024,768}; // 원본변경영역병합기를실제표시갱신소비자로사용한다.
    std::int32_t predictable{1}; // 설정장치에서공급하는명시현재옵션이다.
    std::unique_ptr<RawCampaignConstructionBindings> construction; // 위실제모듈/상태보다먼저해제한다.

    // 보호자료의실제TYPE188을그대로복사하며후처리를첫객체생성이전에설치한다.
    ProtectedConstructionScene():assets(netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078).game),metadata(assets),frames(ProtectedConstructionFrames(assets)),
        world(OriginalEdition::Patch1078,assets.TypeTable().Types(),frames,metadata.shapes,Links(),97),core(world.Core()),
        sp(random,[] { return 1U; }),hp(core.pool,core.types,core.bookkeeping,reward,&sp),
        broadcast(OriginalEdition::Patch1078,players,peers,localOwner,server),formWriter(core.pool,formHeaders,stamp),ai(OriginalEdition::Patch1078,players) {
        sp.Initialize();recent.entries.resize(5);assetState.geysers.entries.resize(100);assetState.inBattle=true;
        state.notice.localPlayer=state.place.localPlayer=1;
        SquidDeletionHooks deletion; // 공통삭제는기본관찰대신같은hp/SP/자원pool을직접갱신한다.
        deletion.emit=ConstructionForbidden<void,const SquidDeletionEvent&>();
        deletion.forward=[this](const SquidDestroyEvent& event) {
            if (event.effect!=SquidDestroyEffect::Transmit) throw std::logic_error("보호 건설 검사 다른 삭제 파생 효과");
            const auto size=formWriter.WriteDeletion(event.sid,event.flags,formBuffer); // 반납전실제SID의패킷을작성한다.
            broadcast.Send(std::span<const std::uint8_t>(formBuffer).first(size));
        };
        world.BindDeletion(hp,[] { return Sid{}; },std::move(deletion));
        world.SetDisplay({0,0,65536,{0,0,1024,768}},{[this] { return dirty.FullRedraw(); },
            [this](SquidDisplayRect rectangle,std::uint32_t flags) { dirty.Add({rectangle.left,rectangle.top,rectangle.right,rectangle.bottom},flags); }});
        carrier=std::make_unique<RawCarrierPostPop>(core.pool,core.types,carrierState,
            MakeCarrierPostPopHooks(core.pool,core.postPop,ConstructionForbidden<void,Sid>(),
                [this](float x,float y,std::uint8_t owner) { world.SetSupportOwner(x,y,owner); }));
        AssetPostPopHooks asset; // 이장면이선택하는Geyser하위를모두실제몸체에연결한다.
        asset.reserveRegular=[] { return true; };
        asset.addRegular=[this](Sid sid,std::uint32_t event,float payload) {
            if (!core.processes.AddRegular(sid,event,payload)) throw std::logic_error("실제 Geyser Regular 부착 실패");
        };
        asset.geyserRegistration=[this](Sid sid,std::uint32_t operation) { ai.Register(sid.value,operation); };
        asset.atPosition=[this](float x,float y) {
            RawSquidFinder finder(core.pool,core.hash,core.types);std::vector<Sid> result; // 원본onepoint현재공간순서다.
            // 현재해시후보를기존일반탐색기에서순서대로수집한다.
            for (auto sid=finder.Begin({static_cast<int>(x),static_cast<int>(y),static_cast<int>(x),static_cast<int>(y)});sid.value;sid=finder.Next()) result.push_back(sid);
            return result;
        };
        asset.unpop=[this](Sid sid,std::uint32_t flags) { world.Unpop(sid,flags); };
        asset.repop=[this](Sid sid,std::uint32_t flags) { world.Repop(sid,flags); };
        assetPostPop=std::make_unique<RawAssetPostPop>(core.pool,core.types,core.postPop,*carrier,assetState,std::move(asset));
        assetPostPop->BindPlayerRecords(players);core.postPop.SetAssetPostPop(assetPostPop.get());
        RawCampaignConstructionEffects effects; // 외부설정/표시는명시현재입력과실제DirtyRegions를사용한다.
        effects.configInt=[this](std::string_view key,std::int32_t fallback) {
            if (key!="predictableGeysers" || fallback!=1) throw std::logic_error("다른 설정 조회");return predictable;
        };
        effects.wallSeconds=[] { return 100.0; };effects.refresh=[this] { dirty.InvalidateAll(); };
        effects.negativeQuantity=ConstructionForbidden<void>();effects.boundsAssertion=ConstructionForbidden<void,PlayerGeyserBoundsAssertion>();
        construction=std::make_unique<RawCampaignConstructionBindings>(world,hp,sp,reward,metadata.frames,chunks,players,broadcast,state,
            PlayerGeyserBoundsState{core.ownerMode.battle,mission,core.ownerMode.challenge,mapSize,piece,directions},recent,assetState.geysers,std::move(effects));
        world.Load({});dirty.Clear();
    }
    // 원본다리/받침초기전역을현재실제월드에공급한다.
    static BridgeConnectState Links() {
        BridgeConnectState value;value.bridgeType=82;value.islandType=94;value.stalagType=95;value.connectorType=155;
        value.noIslandType=157;value.battle=true;return value;
    }
    // 예약Regular와파생callback의참조를소유모듈해제이전에정리한다.
    ~ProtectedConstructionScene() { world.ClearProcesses();core.postPop.SetAssetPostPop(nullptr); }
};
}
// 실제서버SID9개/0x3f가변패킷/같은공간Pop과예측조각삭제를종단대조한다.
TEST_CASE(CampaignConstruction_ProtectedNoIslandPacketAndSelfNotice) {
    ProtectedConstructionScene scene;CHECK(scene.core.types.size()==188);
    CHECK(scene.core.types[74].flags1==scene.assets.TypeTable().Types()[74].flags1);
    const auto preview=scene.world.Create(157,2); // 명시로컬예측입력이며서버확정에서번호를재사용하지않는다.
    scene.core.pool.AllocatedBytes(preview)[40]|=1;scene.world.Pop(preview,100,100,2);
    const auto before=scene.core.pool.FreeCount();CHECK(scene.construction->ConfirmSupport({157,100,100,0,0,0,0,0,0,0}));
    const auto bytes=scene.construction->LastPacket();CHECK(bytes.size()==45 && bytes[0]==1 && bytes[1]==0x3f && bytes[26]==9);
    CHECK((scene.core.pool.Slot(preview)[11]&1U)!=0 && scene.core.pool.FreeCount()<before);
    // 실제wire의각서버SID가같은살아있는raw표면/spot/공통장부에등록되어야한다.
    for (std::size_t index=0;index<bytes[26];++index) {
        const Sid sid{static_cast<std::uint16_t>(Get(bytes,27+index*2,2))};const auto raw=scene.core.pool.Slot(sid);
        CHECK(sid.value>=scene.core.pool.Layout().serverFirst && sid!=preview && raw[10]==157 && !(raw[11]&15U) && !(raw[40]&1U));
        const auto x=static_cast<int>(std::bit_cast<float>(Get(raw,14))),y=static_cast<int>(std::bit_cast<float>(Get(raw,18)));
        CHECK(scene.core.hash.Entries(0)[y*256+x]==sid.value && (scene.core.spots[y*256+x]&2U));
    }
    CHECK(scene.core.bookkeeping.depth==0 && scene.core.destroy.PreDepth()==0 && scene.core.destroy.PostDepth()==0 && scene.dirty.FullRedraw());
    const auto free=scene.core.pool.FreeCount();bool rejected=false; // 일반TYPE는실제factory첫쓰기전에거부한다.
    try { scene.construction->ConfirmSupport({108,140,140,1,0,0,0,0,0,0}); } catch (const std::logic_error&) { rejected=true; }
    CHECK(rejected && scene.core.pool.FreeCount()==free);
}
// 현재Playerflags/실제Bounds/전용최근5/동일보상pool/실제Geyser후처리를함께검사한다.
TEST_CASE(CampaignConstruction_ProtectedLocalGeysersUseCurrentWorld) {
    ProtectedConstructionScene scene;scene.reward.geyserPool=18000;
    CHECK(&scene.construction->LocalGeysers().ResourcePool()==&scene.reward.geyserPool);
    CHECK(&scene.construction->LocalGeysers().Recent()==&scene.recent && &scene.construction->PlayerBounds().Map()==&scene.chunks);
    scene.recent.count=1;scene.recent.entries[0]=123;scene.construction->GenerateLocalGeysers();
    CHECK(scene.recent.count==1 && scene.assetState.geysers.count==0 && scene.reward.geyserPool==18000); // 비활성Player는실제로생성하지않는다.
    Put(scene.players,0xb4+0x50,2,1);Put(scene.players,0xb4+0x78,1);scene.state.geyser.firstPlayerQuantity=2;
    scene.construction->GenerateLocalGeysers();CHECK(scene.recent.count==2 && scene.assetState.geysers.count==2);
    CHECK(scene.reward.geyserPool==14000 && scene.state.geyser.resourcePool==0 && scene.construction->LastPacket().size()==45);
    // 두목록은같은실제SID를가리키지만count/저장소는서로독립적이다.
    for (const auto value:scene.recent.Items()) {
        const Sid sid{static_cast<std::uint16_t>(value)};const auto raw=scene.core.pool.Slot(sid);
        CHECK(raw[10]==122 && Get(raw,12,2)==2000 && !(raw[11]&15U));
        CHECK(std::ranges::find(scene.assetState.geysers.Items(),value)!=scene.assetState.geysers.Items().end());
    }
    CHECK(scene.core.bookkeeping.depth==0 && scene.core.kernel.Size()!=0);
    const auto free=scene.core.pool.FreeCount();scene.state.place.authority=false;bool rejected=false; // 공유전역불일치는새생성전에드러낸다.
    try { scene.construction->GenerateLocalGeysers(); } catch (const std::logic_error&) { rejected=true; }
    CHECK(rejected && scene.core.pool.FreeCount()==free);
}
