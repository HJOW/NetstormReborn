// Form없는선택Fort에서Builder/일반지형/저장후처리/Graph0를같은실제월드에조립한다.
#include "RawSceneSupport.h"
#include "client/RawFortWorldLoadBinding.h"
#include "o/RawFortFinishOwner.h"
#include <filesystem>

using namespace netstorm::o;
using namespace netstorm::client;
using namespace netstorm::test::rawscene;
namespace {
// 기존 실제 PlaceAll의 별도 골든에서 같은 판본의 전체 입력/출력 한 행을 읽는다.
std::vector<std::string> BuilderConsumerGolden(OriginalEdition edition) {
    const auto path=std::filesystem::path(NETSTORM_FORTWORLDLOAD_FIXTURE).parent_path()/"fortworldload-builder-consumer-x86.tsv";
    const auto fixture=LoadFixture(path.string().c_str());
    // 두 판본 중 현재 Pool과 같은 원본 관찰만 선택한다.
    for (const auto& row:fixture.rows) if (row[0]==(edition==OriginalEdition::Patch1078 ? "originals" : "originalCD")) {
        if (row.size()!=11) throw std::logic_error("Fort Builder 소비자 골든 열 오류");return row;
    }
    throw std::logic_error("Fort Builder 소비자 골든 판본 누락");
}
// 현재 청크의 전체3072BYTE를 원본 X열 우선 배치와 동일하게 직렬화한다.
std::string ChunkBytesHex(const ChunkMap& chunks) {
    std::vector<std::uint8_t> bytes(256*12);
    // 원본 저장소는 X열을 먼저 고정한다.
    for (std::int32_t x=0;x<16;++x)
        // 각 X열의 Y항목은 padding 없는 실제12BYTE 필드다.
        for (std::int32_t y=0;y<16;++y) {
            const auto& entry=chunks.At(x,y);const auto offset=static_cast<std::size_t>(x*16+y)*12;
            bytes[offset]=entry.island;bytes[offset+1]=static_cast<std::uint8_t>(entry.side);
            std::copy(entry.record.begin(),entry.record.end(),bytes.begin()+offset+2);Put(bytes,offset+8,std::bit_cast<std::uint32_t>(entry.ordinal));
        }
    return Hex(bytes);
}
// 동일성 입력 확인용 물리코드만 직렬화하며 검색 결과를 재계산하지 않는다.
std::string PuzzleCodeBytesHex(std::span<const FrameCode> codes) {
    std::vector<std::uint8_t> bytes;
    // 네BYTE 물리코드를 원본 순서대로 보존한다.
    for (const auto& code:codes) bytes.insert(bytes.end(),{code.side,code.variant,code.number,code.flags});
    return Hex(bytes);
}
// 선택빈객체/빈Technology범위밖의미복원하위효과를기본성공으로넘기지않는다.
[[noreturn]] void UnrestoredWorldEffect() { throw std::logic_error("선택FortWorld소비자의미복원하위효과요청"); }
// 빈저장목록에서도모든객체필수경계를공급하고선택되지않은가상요청은명시거부한다.
FortObjectLoadHooks EmptyFortObjects(RawSurfaceWorld& world,const SquidReward& hp) {
    FortObjectLoadHooks hooks;
    hooks.create=[&world](std::uint32_t type,std::uint32_t flags) { return world.Create(type,flags); };
    hooks.notifySurface=[](Sid) { UnrestoredWorldEffect(); };hooks.setState=hooks.setOwner=[](Sid,std::uint32_t) { UnrestoredWorldEffect(); };
    hooks.hasHitPoints=[&hp](Sid sid) { return hp.HasHitPoints(sid); };hooks.maxHitPoints=[&hp](Sid sid) { return hp.MaxHitPoints(sid); };
    hooks.virtualCount=[](Sid)->std::int32_t { UnrestoredWorldEffect(); };hooks.pop=[&world](Sid sid,float x,float y,std::uint32_t flags) { world.Pop(sid,x,y,flags); };
    hooks.destroy=[&world](Sid sid,std::uint32_t flags) { if (!world.Destroy(sid,flags)) UnrestoredWorldEffect(); };
    hooks.createContent=[](Sid,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)->Sid { UnrestoredWorldEffect(); };
    hooks.chunkRegion=[](std::int32_t,std::int32_t)->std::int8_t { UnrestoredWorldEffect(); };hooks.territoryAllowed=[](std::int8_t,std::uint32_t,std::int32_t)->bool { UnrestoredWorldEffect(); };
    hooks.loadingDepthChanged=hooks.finishOwner=[](std::uint32_t) { UnrestoredWorldEffect(); };return hooks;
}
}

// 실제단일지면/121생성·지역·청크·두난수·현재장부에후처리순서와Graph0를연결한다.
TEST_CASE(FortWorldLoadBinding_ActualSelectedWorldAndTwoRandomStores) {
    // 군도/PopulatedIsland0/실제Technology부모/변환/UI는완료로세지않는다.
    for (auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        const auto golden=BuilderConsumerGolden(edition); // 새 부모534와 분리한 기존 Builder의 정확 소비자 입력/출력이다.
        std::vector<RiftTypeRecord> types(edition==OriginalEdition::Patch1078 ? 188U : 171U);
        std::vector<RiftTypeFrames> frames(types.size(),RiftTypeFrames({}));std::vector<SquidDisplayShape> shapes(types.size());
        std::vector<FrameCode> terrainCodes,pieceCodes;
        // quarter에충분한16후보/방향과CD BYTE물리256프레임을같이공급한다.
        for (std::uint8_t side='A';side<='P';++side) {
            // 같은방향의A변형16개중선택은기존원본Choose몸체가결정한다.
            for (std::uint8_t number=0;number<16;++number) terrainCodes.push_back({side,'A',1,0});
            // puzzlePiece의실제P변형번호를모두공급한다.
            for (std::uint8_t number=0;number<11;++number) pieceCodes.push_back({side,'P',number,0});
        }
        frames[107]=RiftTypeFrames(std::move(pieceCodes));frames[97]=RiftTypeFrames(terrainCodes);frames[121]=RiftTypeFrames(std::move(terrainCodes));
        // 원본97의null/base계약과121의실제Damageable생성자를그대로사용한다.
        for (const auto type:{97U,121U}) {
            auto& record=types[type];record.constructorAddress=TypeConstructorAddress(edition,type);record.flags1=type==97 ? 0x28000803U : 0x00200012U;
            record.flags2=type==97 ? 2U : 0x80000U;record.footX=record.footY=1;record.maxHitPoints=73;
            shapes[type]={256,true,std::vector<SquidDisplayFrame>(256,{20,16,8,12,1,1})};
        }
        BridgeConnectState bridge;bridge.bridgeType=82;bridge.islandType=94;bridge.stalagType=95;bridge.connectorType=155;bridge.noIslandType=157;bridge.battle=true;
        RawSurfaceWorld world(edition,types,frames,shapes,bridge,97);auto core=world.Core();ChunkMap chunks;
        std::array<std::uint8_t,RawIslandRuntime::kBytes> islandBytes{};FortFinishOwnerState modes;SquidPostPopList residences;
        std::uint32_t revision=0,changed=0,piece=107,localOwner=1,depth=1,crt=7;std::int32_t noRegion=127;
        RawIslandRuntime islands(core.pool,core.hash,islandBytes,modes,residences,revision,changed,
            {[&owner=core.owner](Sid sid,std::uint32_t player) { owner.Set(sid,player); },[](IslandRuntimeAssertion) { UnrestoredWorldEffect(); }});islands.Reset();
        GameRandom random{0x12345678};ScrambledSpStore sp(random,[] { return 0x12345678U; });SquidRewardState hpState;
        SquidReward hp(core.pool,core.types,core.bookkeeping,hpState,edition==OriginalEdition::Patch1078 ? &sp : nullptr);
        std::vector<PriestPlainCanonType> plain(types.size());std::vector<PriestTypeHotspot> hotspots(types.size());
        // pipeline형프레임도같은원본입력이며실제Core SHP/hash/spots를빌린다.
        for (std::size_t type=0;type<plain.size();++type) plain[type].frames=core.frames[type];
        CanonPlacementPipelineState pipelineState;pipelineState.geometry.patternTypes={107,82,94,157,131,129,140,142};pipelineState.placement.localPlayer=1;
        pipelineState.permission.graphReady=0;pipelineState.permission.alliances.fill(1);SquidPostPopList additional;ContainedFinderState contained;
        RawCanonPlacementPipeline pipeline(core.pool,core.hash,{core.types,plain,core.shapes,hotspots,core.spots,core.hash.Entries(0),{}},core.bookkeeping,additional,contained,pipelineState);
        CarrierPostPopState carrierState;carrierState.loadingDepth=depth;
        RawCarrierPostPop carrier(core.pool,core.types,carrierState,MakeCarrierPostPopHooks(core.pool,core.postPop,
            [](Sid) { UnrestoredWorldEffect(); },[&world](float x,float y,std::uint8_t owner) { world.SetSupportOwner(x,y,owner); }));
        TerrainContextGlobals globals;TerrainStampState stamp;FortRandomPlacementState placement;FarmClusterState farm;
        NormalTerrainDiagnostics diagnostics{[](TerrainContextAssertion) { UnrestoredWorldEffect(); },[](TerrainStampAssertion) { UnrestoredWorldEffect(); },
            [](SquidRegionAssert) { UnrestoredWorldEffect(); },[](FarmClusterAssertion) { UnrestoredWorldEffect(); },[](TerrainFarmPostpassAssertion) { UnrestoredWorldEffect(); }};
        RawNormalTerrainWorldBinding normal(world,chunks,islands,random,crt,globals,stamp,placement,farm,piece,pipeline,hp,carrier,diagnostics);
        FortTemplate fort;fort.territory.resize(120);std::copy_n(std::array<std::uint8_t,6>{3,1,0x11,7,0xaa,0xbb}.begin(),6,fort.territory.begin());
        std::uint32_t serializationVersion{}; // 이독립일반월드장치의실제판본DWORD를상태에명시제공한다.
        std::vector<std::string> effects;FortObjectLoadState objectState{serializationVersion};objectState.loadingDepth=depth;
        // 현재깊이는한DWORD이고모든child/Carrier/표면에같은값을전달한다.
        const auto sync=[&](std::uint32_t value) { depth=value;objectState.loadingDepth=value;carrierState.loadingDepth=value;world.SetLoadingDepth(value); };
        FortFinishOwnerHooks finishHooks{[](std::int32_t)->FortRandomRectangle { UnrestoredWorldEffect(); },[]()->double { UnrestoredWorldEffect(); },
            [](GameRandom&,const FortRandomPlaceRequest&)->std::uint32_t { UnrestoredWorldEffect(); },[] { UnrestoredWorldEffect(); }};
        RawFortFinishOwner finish(core.pool,core.types,residences,modes,finishHooks);auto objectHooks=MakeFortFinishOwnerHooks(core.pool,finish,EmptyFortObjects(world,hp));objectHooks.loadingDepthChanged=sync;
        RawFortObjectLoad objects(core.pool,core.hash,core.types,core.frames,random,objectState,objectHooks);
        std::vector<std::uint8_t> playerRecords(9*(edition==OriginalEdition::Patch1078 ? 0xb4U : 0xacU));FortMoneyState moneyState;FortMoneyCursor moneyCursor{0,4};
        RawFortMoneyLoad money(edition,hpState,core.bookkeeping,playerRecords,depth,moneyState,edition==OriginalEdition::Patch1078 ? &sp : nullptr,
            {[](FortMoneyAssertion) { UnrestoredWorldEffect(); },sync,[] { UnrestoredWorldEffect(); }});
        std::vector<std::uint8_t> badges(6160,0xa5),emptyBadges{0};FortMetaStream badgeStream{emptyBadges,0,1,true},techStream{{},0,0,false};FortMetaLoadState metaState;
        FortMetaLoadHooks metaHooks;metaHooks.assertion=[](FortMetaAssertion) { UnrestoredWorldEffect(); };metaHooks.loadingDepthChanged=sync;
        metaHooks.technologyParent=[&] { effects.push_back("parent-boundary");return Sid{}; };metaHooks.prepareTypeNames=[] { UnrestoredWorldEffect(); };
        metaHooks.contents=[](Sid,FortMetaStream&,std::uint32_t,bool) { UnrestoredWorldEffect(); };metaHooks.releaseTypeConversion=[] { UnrestoredWorldEffect(); };
        metaHooks.refreshTechnologyUi=[&] { effects.push_back("ui-boundary"); };RawFortMetaLoad meta(core.pool,badges,depth,metaState,metaHooks);
        FortTerritoryStream territory{fort.territory,0,120};FortWorldLoadHooks hooks;
        hooks.templatePresent=[](std::uint32_t) { return true; };hooks.territory=[&](std::uint32_t owner)->FortTerritoryStream& { CHECK(owner==1);return territory; };
        hooks.currentTemplate=[&](std::uint32_t owner)->const FortTemplate& { CHECK(owner==1);return fort; };
        hooks.assertion=[](FortWorldLoadAssertion) { UnrestoredWorldEffect(); };hooks.loadingDepthChanged=sync;hooks.builderAssertion=[](RawIslandBuilderAssertion) { UnrestoredWorldEffect(); };
        hooks.populateIsland=[](const RawIslandBuilderState&,std::int32_t) { UnrestoredWorldEffect(); };
        hooks=MakeFortWorldLoadBinding(world,normal,objects,money,meta,[&](const FortTemplate& current,std::uint32_t owner)->FortMoneyCursor& {
            CHECK(&current==&fort && owner==1 && depth==2);effects.push_back("money");moneyState.templatePresent=true;moneyCursor.position=0;return moneyCursor;
        },[&](const FortTemplate& current,std::uint32_t owner,bool technology) {
            CHECK(&current==&fort);metaState.templateSlot=std::bit_cast<std::int32_t>(owner);metaState.localOwner=std::bit_cast<std::int32_t>(localOwner);metaState.templatePresent=true;
            effects.push_back(technology ? "technology" : "badges");
        },[&](const FortTemplate& current,bool technology)->FortMetaStream& {
            CHECK(&current==&fort && depth==(technology ? 2U : 1U));if (technology) CHECK(effects.back()=="parent-boundary");
            effects.push_back(technology ? "technology-select" : "badges-select");auto& stream=technology ? techStream : badgeStream;stream.position=0;return stream;
        },std::move(hooks));
        // 실제generate/objects/Graph만관찰로감싸고완료body호출은그대로유지한다.
        auto generate=hooks.generate;auto loadObjects=hooks.objects;auto rebuild=hooks.rebuildGraph;
        hooks.generate=[&,generate](const RawIslandBuilderState& state,std::int32_t region) {
            CHECK(ChunkBytesHex(chunks)==golden[5] && Hex(islandBytes)==golden[6]);
            CHECK(islands.Count()==Number(golden[8]) && golden[7]=="1" && golden[8]=="1" && golden[9]=="1" && golden[10]=="0");
            effects.push_back("generate");generate(state,region);
        };
        hooks.objects=[&,loadObjects](const FortTemplate& current,std::uint32_t owner) { effects.push_back("objects");loadObjects(current,owner); };
        hooks.rebuildGraph=[&,rebuild](bool reset) { effects.push_back("graph0");CHECK(!reset);rebuild(reset); };
        CHECK(Hex(fort.territory)==golden[1] && PuzzleCodeBytesHex(core.frames[107].Codes())==golden[2]);
        CHECK(ChunkBytesHex(chunks)==golden[3] && Hex(islandBytes)==golden[4]);
        world.BeginLoad();sync(1);RawFortWorldLoad parent(core.pool,chunks,islands,core.frames,piece,localOwner,noRegion,depth,hooks);parent.Load(0);
        CHECK(ChunkBytesHex(chunks)==golden[5] && Hex(islandBytes)==golden[6]);
        CHECK(islands.Count()==1 && islands.Exists(0)==1 && islands.Owner(0)==1 && chunks.At(2,2).island==0 && territory.position==120 && territory.end==120);
        CHECK(effects==std::vector<std::string>({"generate","objects","money","badges","badges-select","technology","parent-boundary","technology-select","ui-boundary","graph0"}));
        CHECK(depth==1 && objectState.loadingDepth==1 && carrierState.loadingDepth==1 && moneyCursor.position==4 && badgeStream.position==1 && techStream.position==0);
        CHECK(random.State()!=0x12345678 && crt!=7 && stamp.variants.has_value() && !sp.Initialized());
        const auto objectsNow=world.Objects();CHECK(!objectsNow.empty());std::size_t farms=0;
        // 모든생성SID는같은Factory/풀의현재TYPE/지역이며Farm도실제HP후처리를사용한다.
        for (const auto& object:objectsNow) { const auto raw=core.pool.Slot(object.sid);CHECK(Get(raw,8,2)==0);if (raw[10]==121) { ++farms;CHECK(Get(raw,26,edition==OriginalEdition::Patch1078 ? 4U : 2U)==73); } }
        CHECK(farms>0 && farms<=60 && core.kernel.Size()==0 && &normal.Pool()==&parent.Pool());
    }
}
