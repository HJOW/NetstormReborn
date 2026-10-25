#include "o/RawGameAuxState.h"
// 원본군집의모든배치entry/return과같은raw풀의실제단일Place/해시변경을전수대조한다.
#include "RawSceneSupport.h"
#include "o/RawFarmCluster.h"
#include <cstdio>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 독립native입력의지면범위/시작SID와생성SID·원본vtable입력이다.
constexpr int kFarmFirst=60,kFarmLast=132,kFarmSide=kFarmLast-kFarmFirst+1;
constexpr std::uint16_t kFarmGround=200,kFarmBorn=10000;
constexpr std::uint32_t kFarmVtable=0x13000000;
// 원본이관찰한fixture를읽으며기대군집/성공순회값을재계산하지않는다.
const std::vector<std::vector<std::string>>& FarmRows() { static const auto rows=LoadFixture(NETSTORM_FARMCLUSTER_FIXTURE).rows;return rows; }
// 실제단일풀/hash/spot/type/지역/두난수에서군집과단일배치를조립하는장면이다.
struct FarmScene {
    SidPool pool; // 판본별raw슬롯이며fixture입력용슬롯만직접초기화한다.
    SquidHash hash; // ground와현재edgeFarm을동시에등록하는단일해시다.
    std::array<std::uint8_t,65536> spots{}; // 실제단일Place가먼저읽는spotBYTE지도다.
    std::vector<RiftTypeRecord> types; // 원본fixture의현재타입플래그/foot/HP입력이다.
    std::vector<RiftTypeFrames> frames; // matchframe 및명시합성randframe프로필이다.
    GameRandom random; // 단일프레임선택이소비하는같은global난수다.
    FortRandomPlacementState placementState; // 실제edgeFarm121/isle97/owner전역이다.
    FarmClusterState state; // 군집진입의현재archipelago/span이다.
    ChunkMap chunks; // 지역record와단일region검사에같은지도를쓴다.
    std::array<std::uint8_t,RawIslandRuntime::kBytes> records{}; // 실제mutable섬100기록이다.
    FortFinishOwnerState modes; // 기존runtime생성에필요한같은월드모드다.
    SquidPostPopList residences; // 군집fixture에서는owner setter를호출하지않는다.
    std::uint32_t revision{},changed{}; // 기존runtime의독립전역이다.
    RawIslandRuntime islands; // 실제regionOwner/Exists getter다.
    std::unique_ptr<SquidFactory> factory; // 기존실제efarm121/basefallback 생성자몸체를같은pool에실행한다.
    std::unique_ptr<RawFortRandomPlacement> placement; // 완료된실제단일배치몸체다.
    std::unique_ptr<RawFarmCluster> cluster; // 위같은자료를소비하는신규군집이다.
    std::vector<std::string> events; // 원본primitive효과의순서/인자다. 단순get은별도native통계다.
    std::vector<std::uint8_t> trace; // 모든단일entry/return의DWORD비트다.
    std::vector<std::uint32_t> input; // 독립실행기의12개명시입력이다.
    std::uint32_t born{},attempts{},lastLocal{},assertions{}; // 생성수/배치시도/마지막local상태/원본진단이다.
    bool patch; // 현재vtable/owner/HP/frame 폭이다.
    // 원본영역보다큰pool과필수runtime경계를유지한다.
    explicit FarmScene(OriginalEdition edition):pool(edition,32768,false),types(edition==OriginalEdition::Patch1078 ? 188U : 171U),frames(types.size(),RiftTypeFrames({})),
        islands(pool,hash,records,modes,residences,revision,changed,{[](Sid,std::uint32_t) { throw std::runtime_error("군집예상밖거주 구역owner"); },
            [](IslandRuntimeAssertion) { throw std::runtime_error("군집예상밖IslandRuntime진단"); }}),patch(edition==OriginalEdition::Patch1078) {}
    // native합성입력슬롯을구성한다. 게임allocator행동의대조로계수하지않는다.
    std::span<std::uint8_t> Raw(Sid sid) { return {const_cast<std::uint8_t*>(pool.Slot(sid).data()),pool.Layout().stride}; }
    // 경계효과의원본순서/인자를같은콜론형식으로저장한다.
    template<class... T> void Event(const char* kind,T... values) { std::string text=kind;((text+=':'+std::to_string(values)),...);events.push_back(std::move(text)); }
    // 현재type가표면이면level0,일반1칸edgeFarm이면level1에등록하는명시공간경계다.
    int Level(Sid sid) const { return types[pool.Slot(sid)[10]].flags2&6U ? 0 : 1; }
    // 실제Unpop경계는현재WORD체인에서같은SID를제거한다.
    void Unhash(Sid sid) {
        const auto raw=pool.Slot(sid);const auto x=static_cast<int>(std::bit_cast<float>(Get(raw,14))),y=static_cast<int>(std::bit_cast<float>(Get(raw,18))),level=Level(sid),scale=SquidHash::kScales[static_cast<std::size_t>(level)];
        auto& cell=hash.Cell(level,x/scale,y/scale);Sid current{cell},previous{};
        // 실제next를따라가며해당SID이외의후행객체를유지한다.
        while (current.value) {
            const auto after=Get(pool.Slot(current),4,2);
            if (current==sid) { if (previous.value) Put(Raw(previous),4,after,2);else cell=static_cast<std::uint16_t>(after);Put(Raw(sid),4,0,2);return; }
            previous=current;current={static_cast<std::uint16_t>(after)};
        }
    }
    // 현재타입/원래좌표를다시읽어같은hash에등록한다.
    void Rehash(Sid sid) {
        const auto raw=pool.Slot(sid);const auto x=static_cast<int>(std::bit_cast<float>(Get(raw,14))),y=static_cast<int>(std::bit_cast<float>(Get(raw,18))),level=Level(sid),scale=SquidHash::kScales[static_cast<std::size_t>(level)];
        auto& cell=hash.Cell(level,x/scale,y/scale);Put(Raw(sid),4,cell,2);cell=sid.value;
    }
    // 실제Place의legal/factory/가상공간·owner 경계를기계어실행기와동일한명시입력으로연결한다.
    FortRandomPlacementHooks PlacementHooks() {
        FortRandomPlacementHooks hooks;
        hooks.mayPlace=[this](const CanonPlacementQuery& query) { Event("L",query.type,query.argument,std::bit_cast<std::uint32_t>(query.x),std::bit_cast<std::uint32_t>(query.y),query.flags,query.owner,query.mode);return input[6]!=0; };
        hooks.chunkRegion=[this](std::int32_t x,std::int32_t y) { return std::bit_cast<std::int8_t>(chunks.At(x,y).island); };
        hooks.regionOwner=[this](std::int32_t region) { Event("I",static_cast<std::uint32_t>(region));return islands.Owner(region); };
        hooks.create=[this](std::uint32_t type,std::uint32_t flags) { const Sid sid{static_cast<std::uint16_t>(kFarmBorn+born++)};auto raw=Raw(sid);
            // 명시factory입력은원본과같은잔여payload를제공한다.
            for (std::size_t index=0;index<raw.size();++index) raw[index]=static_cast<std::uint8_t>(index*37+11);
            Put(raw,0,kFarmVtable);raw[10]=static_cast<std::uint8_t>(type);raw[11]=4;Put(raw,6,0,2);Event("C",type,flags,sid.value);return sid; };
        hooks.construct=[this](Sid sid,std::uint32_t type) { Event("K",sid.value,type);factory->Construct(type,sid); };
        hooks.hasHitPoints=[this](Sid sid) { return (types[pool.Slot(sid)[10]].flags1&0x10U)!=0; };
        hooks.typeHitPoints=[](std::uint32_t) { return 803; };
        hooks.setOwner=[this](Sid sid,std::uint32_t owner) { Event("O",sid.value,owner);Put(Raw(sid),patch ? 34U : 32U,owner,1); };
        hooks.unpop=[this](Sid sid,std::uint32_t flags) { Event("U",sid.value,flags);Unhash(sid);Raw(sid)[11]|=4; };
        hooks.repop=[this](Sid sid,std::uint32_t flags) { Event("R",sid.value,flags);Raw(sid)[11]&=static_cast<std::uint8_t>(~4U);Rehash(sid); };
        hooks.pop=[this](Sid sid,float x,float y,std::uint32_t flags) { Event("P",sid.value,std::bit_cast<std::uint32_t>(x),std::bit_cast<std::uint32_t>(y),flags);auto raw=Raw(sid);Put(raw,14,std::bit_cast<std::uint32_t>(x));Put(raw,18,std::bit_cast<std::uint32_t>(y));raw[11]&=static_cast<std::uint8_t>(~4U);Rehash(sid); };
        return hooks;
    }
    // 원본배치entry/return을실제Place의앞뒤에서기록한다.
    FarmClusterHooks ClusterHooks() {
        FarmClusterHooks hooks;hooks.assertion=[this](FarmClusterAssertion assertion) { ++assertions;Event("A",assertion==FarmClusterAssertion::MissingIsland ? 294 : 300); };
        hooks=MakeFarmClusterPlacementHook(pool,*placement,std::move(hooks));const auto actual=hooks.place;
        hooks.place=[this,actual](GameRandom& local,const FortRandomPlaceRequest& request) {
            ++attempts;lastLocal=local.State();const std::array<std::uint32_t,7> values{lastLocal,request.type,std::bit_cast<std::uint32_t>(request.x),std::bit_cast<std::uint32_t>(request.y),std::bit_cast<std::uint32_t>(request.region),request.mode,request.force};
            // 단일진입의원본DWORD일곱개를순서대로기록한다.
            for (const auto value:values) { const auto offset=trace.size();trace.resize(offset+4);Put(trace,offset,value); }
            const auto result=actual(local,request);const auto offset=trace.size();trace.resize(offset+4);Put(trace,offset,result);return result;
        };return hooks;
    }
    // 원본입력만구성하며군집결과는실제두복원몸체가만든다.
    void Prepare(const std::vector<std::string>& row) {
        input.clear();events.clear();trace.clear();born=attempts=lastLocal=assertions=0;hash.Reset();records.fill(0);modes={};types.assign(types.size(),RiftTypeRecord{});frames.assign(frames.size(),RiftTypeFrames({}));
        // 원본DWORD비트를그대로읽는다.
        for (const auto& number:Split(row[1],',')) input.push_back(Number(number));CHECK(input.size()==12);random.SetState(input[8]);state={input[3]!=0,std::bit_cast<std::int32_t>(input[4])};spots.fill(static_cast<std::uint8_t>(input[6]!=0));
        types[97].flags1=0x10;types[97].flags2=2;types[97].footX=types[97].footY=1;types[121].flags1=input[5] ? 0x100010 : 0x200010;types[121].flags2=0x80000;types[121].footX=types[121].footY=1;
        types[121].constructorAddress=input[11] ? TypeConstructorAddress(pool.Edition(),121) : 0;
        frames[121]=RiftTypeFrames({{80,80,0,0},{80,80,1,255},{65,65,0,0}});
        // 같은정수좌표ground를실제raw/header/hash0에배치한다.
        for (int x=kFarmFirst;x<=kFarmLast;++x) {
            // x열우선SID를y순서대로등록한다.
            for (int y=kFarmFirst;y<=kFarmLast;++y) {
                const Sid sid{static_cast<std::uint16_t>(kFarmGround+(x-kFarmFirst)*kFarmSide+y-kFarmFirst)};auto raw=Raw(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{});Put(raw,0,kFarmVtable);raw[10]=97;Put(raw,14,std::bit_cast<std::uint32_t>(static_cast<float>(x)));Put(raw,18,std::bit_cast<std::uint32_t>(static_cast<float>(y)));Put(raw,patch ? 36U : 34U,7,patch ? 4U : 1U);Put(raw,patch ? 34U : 32U,input[7],1);hash.Cell(0,x,y)=sid.value;
            }
        }
        // 현재지역record의seed BYTE는별도입력이다.
        for (int x=0;x<16;++x) {
            // 모든청크가같은현재지역을조회한다.
            for (int y=0;y<16;++y) { auto& entry=chunks.At(x,y);entry.island=static_cast<std::uint8_t>(input[9] ? input[1] : 127);entry.record={0,0,0,static_cast<std::uint8_t>(input[2]),0,0}; }
        }
        Put(records,0,100);
        // 기존Owner/Exists가읽는실제100기록이다.
        for (std::size_t region=0;region<100;++region) { Put(records,4+region*32+4,input[10]);Put(records,4+region*32+8,input[7]); }
        factory=std::make_unique<SquidFactory>(pool,types);placement=std::make_unique<RawFortRandomPlacement>(pool,hash,spots,types,frames,random,placementState,PlacementHooks());cluster=std::make_unique<RawFarmCluster>(pool,chunks,islands,placementState,state,ClusterHooks());
    }
    // 실제변경된ground/생성객체의전체raw를원본과같은형식으로읽는다.
    std::string ChangedRaw() const {
        std::string result;
        // 군집후타입이달라진실제ground만읽는다.
        for (std::uint32_t number=kFarmGround;number<kFarmGround+kFarmSide*kFarmSide;++number) { const auto raw=pool.Slot(Sid{static_cast<std::uint16_t>(number)});if (raw[10]!=97) { if (!result.empty()) result+=';';result+=std::to_string(number)+':'+Hex(raw); } }
        // 실제factory경계가제공한새슬롯도포함한다.
        for (std::uint32_t number=kFarmBorn;number<kFarmBorn+born;++number) { if (!result.empty()) result+=';';result+=std::to_string(number)+':'+Hex(pool.Slot(Sid{static_cast<std::uint16_t>(number)})); }
        return result.empty() ? "-" : result;
    }
};
// 모든군집입력의배치시도/trace/primitive효과/raw/두난수/assert를비교한다.
void ReplayFarm(const char* edition,bool gameOwner=false) {
    RawGameAuxState owner;FarmScene scene(std::string_view(edition)=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);std::size_t count=0;
    // 실제PE에서관찰한모든고유입력을순회한다.
    for (const auto& row:FarmRows()) {
        if (row[0]!=edition) continue;CHECK(row.size()==9);scene.Prepare(row);if (gameOwner) { owner.Archipelago()=scene.input[3];scene.state.sharedArchipelago=&owner.Archipelago();scene.state.archipelago=scene.input[3]==0; }scene.cluster->Place({80.25f,80.75f,96.625f,96.125f},std::bit_cast<std::int32_t>(scene.input[1]),scene.input[0]);std::string events;
        // 기록한실제경계효과를원본순서로결합한다.
        for (const auto& event:scene.events) { if (!events.empty()) events+=';';events+=event; }
        const auto effectBytes=std::span(reinterpret_cast<const std::uint8_t*>(events.data()),events.size());
        const bool same=scene.attempts==Number(row[2]) && Adler(scene.trace)==Number(row[3]) && Adler(effectBytes)==Number(row[4]) && scene.ChangedRaw()==row[5] && scene.random.State()==Number(row[6]) && scene.lastLocal==Number(row[7]) && scene.assertions==Number(row[8]);
        if (!same) std::fprintf(stderr,"FarmCluster %s input=%s attempts=%u/%s trace=%u/%s effects=%u/%s global=%u/%s local=%u/%s assert=%u/%s\n",edition,row[1].c_str(),scene.attempts,row[2].c_str(),Adler(scene.trace),row[3].c_str(),Adler(effectBytes),row[4].c_str(),scene.random.State(),row[6].c_str(),scene.lastLocal,row[7].c_str(),scene.assertions,row[8].c_str());
        CHECK(same);++count;
    }
    CHECK(count==104);
}
}
// patch군집104개를기존실제단일Place/efarmCtor와같은풀에서대조한다.
TEST_CASE(FarmCluster_ReplaysPatch) { ReplayFarm("originals"); }
// CD군집104개를지역record원본진단/실제단일Place/efarmCtor와같은풀에서대조한다.
TEST_CASE(FarmCluster_ReplaysCd) { ReplayFarm("originalCD"); }
// 필수단일배치/진단과같은월드runtime 연결이빠지면거부한다.
TEST_CASE(FarmCluster_RejectsMissingEffectsAndOtherWorld) {
    FarmScene scene(OriginalEdition::Patch1078);CHECK(Throws([&] { RawFarmCluster invalid(scene.pool,scene.chunks,scene.islands,scene.placementState,scene.state,{}); }));
    SidPool other(OriginalEdition::Patch1078,32768,false);CHECK(Throws([&] { RawFarmCluster invalid(other,scene.chunks,scene.islands,scene.placementState,scene.state,{[](GameRandom&,const FortRandomPlaceRequest&) { return 0U; },[](FarmClusterAssertion) {}}); }));
}


// 실제FarmCluster는Login.bb4와별개인현재bb0DWORD를읽고반대bool사본을무시한다.
TEST_CASE(GameAux_FarmClusterUsesSharedArchipelagoNative104) { ReplayFarm("originals",true); }
