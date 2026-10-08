// 섬 생성부터 삭제·Regular 낙하·끝 칸 교체·연결 객체 정리까지 실제 raw 모듈로 이어 검사한다.
#include "RawSceneSupport.h"
#include "o/Bridge.h"
#include "o/RawBridgeEvents.h"
#include "o/RawIslandPostPop.h"
#include "o/RawSurfaceLifecycle.h"
#include "o/RawSquidDestroyDispatch.h"
#include "o/SquidDestroyLifecycle.h"
#include "o/SquidFactory.h"
#include "o/SquidFrame.h"
#include "o/SquidFrameBinding.h"
#include "o/SquidOwner.h"
#include "o/SquidPop.h"
#include "client/SquidRenderer.h"
#include "SurfaceLifecycleInspect.h"

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 실제 타입 번호와 공통 raw 필드의 위치다.
constexpr std::uint32_t kBridge=82,kIsland=94,kStalag=95,kConnector=155,kSurface=157;
constexpr std::size_t kType=10,kState=11,kWord=12,kX=14,kY=18;
// 원본 상태 비트 및 검사에 쓰는 3×3 표면 글자 순서다.
constexpr std::uint8_t kFree=1,kDead=2;
constexpr std::string_view kSurfaceLetters="FCGBADIEH";
// 원본 bridge.type에서 추출해 저장한 실제 프레임 코드 표를 읽는다.
RiftTypeFrames BridgeFrames() {
    std::ifstream input(NETSTORM_BRIDGEDECAY_FIXTURE);std::string line;
    // 첫 Frames 행만 읽어 큰 관찰 fixture 전체를 메모리에 올리지 않는다.
    while (std::getline(input,line)) {
        if (!line.starts_with("Frames\t0\t")) continue;
        const auto hex=Split(line,'\t').at(2);std::vector<FrameCode> codes;
        // 네 바이트가 하나의 실제 코드다.
        for (std::size_t i=0;i+7<hex.size();i+=8) {
            const auto byte=[&](std::size_t offset) { return static_cast<std::uint8_t>(std::stoul(hex.substr(i+offset,2),nullptr,16)); };
            codes.push_back({byte(0),byte(2),byte(4),byte(6)});
        }
        return RiftTypeFrames(std::move(codes));
    }
    throw std::runtime_error("실제 다리 프레임 fixture 없음");
}
// 표면은 첫 프레임만, 받침/종유석/연결 객체는 방향 A의 합성 프레임을 공급한다.
std::vector<RiftTypeFrames> FrameTable(std::size_t count) {
    std::vector<RiftTypeFrames> frames(count,RiftTypeFrames({}));frames[kBridge]=BridgeFrames();
    std::vector<FrameCode> surface;
    // 행 우선 배치에서 선택되는 원본 글자 순서다.
    for (const char side:kSurfaceLetters) surface.push_back({static_cast<std::uint8_t>(side),'P',1,0});
    frames[kSurface]=RiftTypeFrames(std::move(surface));
    // 방향은 이 세 비표면 타입의 연결 판단에 쓰이지 않는다.
    for (const auto type:{kIsland,kStalag,kConnector}) frames[type]=RiftTypeFrames(std::vector<FrameCode>(9,FrameCode{'A','P',1,0}));
    return frames;
}
// 정상 교체·고립 삭제·단단한 다리 유지·다리 보존 flags의 통합 시나리오다.
enum class Scenario { Replace,Isolated,Hard,Keep };
// 표시 변경 영역을 실제 Renderer에 전달한다. 원본 SHP 검사에서도 같은 경로를 쓴다.
struct DisplaySink final:SquidDisplaySink {
    netstorm::client::Renderer renderer{640,480};netstorm::client::SquidRenderer target{renderer};std::size_t notices{};
    // 실제 전체 갱신 상태를 따른다.
    bool Suppressed() const override { return target.Suppressed(); }
    // 픽셀 경계/flags를 바꾸지 않고 변경 표에 전달한다.
    void Invalidate(SquidDisplayRect rect,std::uint32_t flags) override { ++notices;target.Invalidate(rect,flags); }
};
// 한 판본에서 실제 모듈을 구성하고 섬 받침 삭제를 유일한 낙하 예약 트리거로 사용한다.
void Run(OriginalEdition edition,Scenario scenario,bool removeSurfaceBefore=false,bool graphActive=false,
    const netstorm::client::GameAssets* assets=nullptr) {
    // ProcessForm을 포함하는 실제 SID 전체 수다.
    constexpr std::uint32_t capacity=32768;
    const bool patch=edition==OriginalEdition::Patch1078;
    const std::size_t ownerOffset=patch ? 34 : 32,frameOffset=patch ? 36 : 34,frameWidth=patch ? 4 : 1,extraOffset=patch ? 40 : 35;
    SidPool pool(edition,capacity,true);SquidHash hash;std::vector<std::uint8_t> spots(65536);
    std::vector<RiftTypeRecord> types(patch ? 188 : 171);
    // 실제 생성자·플래그·발자국을 입력하고 다리만 검사 비용 5를 둔다.
    const auto define=[&](std::uint32_t number,std::uint32_t flags1,std::uint32_t flags2,int foot) {
        auto& type=types[number];type.constructorAddress=TypeConstructorAddress(edition,number);
        type.flags1=flags1;type.flags2=flags2;type.footX=type.footY=foot;
    };
    define(kBridge,0x802,4,1);types[kBridge].cost=5;
    define(kIsland,0x28000003,0x01000000,3);define(kStalag,0x28000002,0,3);
    define(kConnector,0x28000002,0x02000000,1);define(kSurface,0x08000803,0x01000002,1);
    // 표면 삭제의 분배만 확인하는 합성 walker 타입이다. 생성/표시/낙하 몸체는 이 검사 범위 밖이다.
    types[85].flags2=0x10000;types[85].footX=types[85].footY=1;
    auto frames=FrameTable(types.size());
    if (assets) {
        const auto loaded=assets->TypeTable().Types();types.assign(loaded.begin(),loaded.end());
        // 실제 .type 코드와 SHP 추가 헤더를 동일한 번호 체계로 연결한다.
        for (std::size_t i=0;i<assets->Types().size();++i) frames[kFirstAssetTypeNumber+i]=assets->Types()[i].definition.FrameTable();
    }
    std::vector<std::vector<FrameCode>> codes(types.size());
    // Graph는 실제 이웃 탐색기와 같은 프레임 코드를 사용한다.
    for (std::size_t i=0;i<frames.size();++i) codes[i].assign(frames[i].Codes().begin(),frames[i].Codes().end());
    RawGraph graph(pool,hash,spots,types,codes);auto* activeGraph=graphActive ? &graph : nullptr;
    // 빈 월드의 원본 전체 초기화는 0번을 예약한다. 생략하면 첫 Flood가 같은 번호로 끝난다.
    if (graphActive) graph.Rebuild(true);
    SquidPostPopState bookkeeping;bookkeeping.localOwner=3;bookkeeping.graphsEnabled=graphActive;
    RawBridgeConnect* linker=nullptr;RawIslandPostPop* surfaceLinker=nullptr;
    // 순환하는 생성/Pop 경로는 구성 완료 뒤 연결한다.
    SquidPostPop postPop(pool,types,bookkeeping,activeGraph,[&](Sid sid) { linker->Connect(sid); });
    postPop.SetIslandPrefix([&](Sid sid,std::uint32_t flags) { linker->IslandPostPopPrefix(sid,flags); });
    postPop.SetSurfacePrefix([&](Sid sid,std::uint32_t flags) { surfaceLinker->Prefix(sid,flags); });
    std::vector<SquidDisplayShape> shapes(types.size());
    // 단위 검사는 별도 픽셀/칸 크기를 공급한다. 선택 검사에서는 실제 두 판본 SHP로 대체한다.
    for (const auto type:{kBridge,kIsland,kStalag,kConnector,kSurface}) {
        auto& shape=shapes[type];shape.loaded=true;shape.frameCount=static_cast<int>(frames[type].Codes().size());
        shape.frames.assign(static_cast<std::size_t>(shape.frameCount),SquidDisplayFrame{20,16,8,12,
            static_cast<float>(types[type].footX),static_cast<float>(types[type].footY)});
    }
    if (assets) shapes=netstorm::client::SquidRenderer::Shapes(*assets,edition);
    DisplaySink sink;SquidDisplay display(edition,types,shapes,sink,{0,0,65536,{0,0,640,480}});
    std::vector<std::uint8_t> pixels(640*480);sink.renderer.Draw(pixels,640,7);sink.renderer.Present([](netstorm::client::ScreenRect){});
    SquidUnpop unpop(pool,hash,spots,&display);SquidPop pop(pool,hash,spots,&display,&postPop);SquidFactory factory(pool,types,false,&unpop);
    SquidOwnerMode ownerMode;ownerMode.battle=true;SquidOwner owner(pool,types,bookkeeping,ownerMode);
    RawSquidNeighbors neighbors(pool,hash,spots,types,frames);
    BridgeConnectState typeState;typeState.bridgeType=kBridge;typeState.islandType=kIsland;typeState.stalagType=kStalag;
    typeState.connectorType=kConnector;typeState.noIslandType=kSurface;typeState.battle=true;
    std::vector<Sid> links;Sid born{};int displays=0;
    BridgeConnectHooks objects;
    // 연결 생성도 같은 Factory와 raw 가상 표를 사용한다.
    objects.create=[&](std::uint32_t type,std::uint32_t flags) {
        const Sid sid=factory.Create(type,flags);if (type==kConnector) links.push_back(sid);return sid;
    };
    objects.setOwner=MakeOwnerDispatch(pool,typeState,[&](Sid sid,std::uint32_t player) { owner.Set(sid,player); });
    // 실제 Pop의 공간 쓰기와 파생 postPop을 모두 실행한다.
    objects.pop=[&](Sid sid,float x,float y,std::uint32_t flags) {
        const auto& type=types[pool.Slot(sid)[kType]];
        const auto size=display.FrameSize(edition,pool.Slot(sid),static_cast<int>(Get(pool.Slot(sid),frameOffset,frameWidth)));
        CHECK(pop.Pop(sid,type,size[0],size[1],x,y,flags)==RawPopResult::Registered);
    };
    SquidFrame frame(pool,types,MakeSquidFrameHooks(pool,types,display,unpop,pop));
    objects.setFrame=[&](Sid sid,std::int32_t value,std::uint32_t flags) { frame.Set(sid,value,flags); };
    objects.notifySurface=[](Sid) {}; // 상태를 바꾸지 않는 원본 검사 경계다.
    RawBridgeConnect connect(pool,neighbors,typeState,objects);linker=&connect;
    IslandPostPopState loadState{1,false};
    RawIslandPostPop surface(pool,hash.Entries(0),types,frames,typeState,loadState,IslandPostPopHooks{
        objects,[&](Sid sid) { connect.Connect(sid); },
        [&](float x,float y,std::uint32_t type) { return connect.FindTypeAt(x,y,type); },
        [&](Sid sid) { ++displays;display.Update(pool.Slot(sid)); },[](bool,bool,bool) { throw std::logic_error("받침 구성 누락"); }});surfaceLinker=&surface;
    // 요새 로드와 같은 9칸 등록이 받침과 종유석을 실제 생성한다.
    for (int y=28;y<=30;++y) {
        // 첫 칸의 소유자로 마지막 H에서 묶음 소유자가 전파된다.
        for (int x=28;x<=30;++x) {
            const Sid cell=factory.Create(kSurface);owner.Set(cell,x==28 && y==28 ? 3U : 0U);
            objects.pop(cell,static_cast<float>(x),static_cast<float>(y),0);
        }
    }
    const Sid island=connect.FindTypeAt(30,30,kIsland),stalag=connect.FindTypeAt(30,30,kStalag);
    CHECK(island.value && stalag.value && displays==11);
    const auto terrainCost=bookkeeping.totalCost;
    const Sid source=factory.Create(kBridge);owner.Set(source,3);
    // 실제 K 프레임 첫 번호 41, hard 번호 46이다. 소유자/수명/추가 비트 복사도 확인한다.
    Put(pool.AllocatedBytes(source),frameOffset,scenario==Scenario::Hard ? 46U : 41U,frameWidth);
    objects.pop(source,31,29,0);Put(pool.AllocatedBytes(source),8,23,2);pool.AllocatedBytes(source)[extraOffset]|=0x10;
    CHECK(links.size()==1 && Get(pool.Slot(links.at(0)),kWord,2)==source.value);
    const Sid link=links.at(0);const Sid second{static_cast<std::uint16_t>(Get(pool.Slot(link),8,2))};
    const auto linkLevel=static_cast<int>(pool.Slot(link)[patch ? 33 : 31]);
    CHECK(pool.Slot(second)[kType]==kSurface && hash.Bucket(linkLevel,30,29)==link.value);
    Sid survivor{};
    if (scenario!=Scenario::Isolated) {
        survivor=factory.Create(kBridge);owner.Set(survivor,3);Put(pool.AllocatedBytes(survivor),frameOffset,41,frameWidth);
        objects.pop(survivor,32,29,0);
    }
    RawSquidDestroy destroy(pool,unpop,types);BridgeDecayMode mode;RawSquidDestroyDispatch virtualDestroy(destroy,neighbors,mode,activeGraph);
    SquidDeletionState deletionState;
    SquidDestroyLifecycle common(pool,types,bookkeeping,deletionState,destroy,SquidDeletionHooks{
        [](const SquidDeletionEvent&) {},[](const SquidDestroyEvent& event) { CHECK(event.effect==SquidDestroyEffect::Transmit); }},activeGraph);
    Kernel kernel;SquidProcessState processState;processState.now=10;SquidProcessHost* hostPointer=nullptr;
    std::vector<Sid> removedLinks,fallen;std::vector<BridgeLifecycleEffect> effects;std::vector<std::string> order;
    RawSurfaceLifecycle lifecycle(pool,neighbors,kConnector,common.Hooks(),SurfaceLifecycleHooks{
        // 예약 직전에는 두 참조가 살아 있으므로 연결 객체를 지우지 않는다.
        [&](Sid sid,float x,float y) { CHECK(sid==source);order.push_back("schedule");CHECK(ScheduleBridgeFall(*hostPointer,sid,x,y)!=nullptr); },
        // 다리 preDestroy 안의 연결 삭제도 최종 ProcessHost 훅으로 재진입한다.
        [&](Sid sid,std::uint32_t flags) {
            CHECK(sid==link && (pool.Slot(source)[kState]&kDead)!=0);removedLinks.push_back(sid);order.push_back("link");
            CHECK(virtualDestroy.Destroy(sid,flags,hostPointer->Hooks()));
        },
        [&](Sid sid) { fallen.push_back(sid); },
        [&](const BridgeLifecycleEvent& event) { effects.push_back(event.effect);order.push_back("effect"); }});
    BridgeEventState eventState;eventState.bridgeType=kBridge;
    BridgeEventHooks eventsHooks;
    eventsHooks.notifySurface=[](Sid) {}; // 수명 단어 쓰기의 진단 경계만 대체한다.
    eventsHooks.create=[&](std::uint32_t type) { born=factory.Create(type);return born; };
    // 가상 삭제의 거부도 정상 결과다. 이벤트 처리기는 원본처럼 삭제 성공 여부를 읽지 않는다.
    eventsHooks.destroy=[&](Sid sid,std::uint32_t flags) { CHECK(sid==source);order.push_back("bridge");static_cast<void>(virtualDestroy.Destroy(sid,flags,hostPointer->Hooks())); };
    eventsHooks.setOwner=[&](Sid sid,std::uint8_t player) { objects.setOwner(sid,player); };eventsHooks.pop=objects.pop;
    RawBridgeEvents events(pool,eventState,MakeBridgeNeighborHooks(neighbors,std::move(eventsHooks)));
    SquidProcessHost host(pool,types,kernel,destroy,processState,MakeBridgeRegularHandler(pool,events),lifecycle.Hooks());hostPointer=&host;
    RawBridgeLifecycle linkCheck(pool,types);CHECK(!linkCheck.LinkNeedsDestroy(link));
    // 살아 있는 표면의 실제 raw 번호로 그래프 표의 연결 성분과 표면 수를 검사한다.
    const auto checkGraph=[&](std::uint32_t expected) {
        if (!graphActive) return;
        std::array<std::uint32_t,Graph::kTableSize> counts{};
        // 해시 0단계에 등록된 모든 표면이 유효한 활성 그래프에 속해야 한다.
        for (std::uint16_t value=5;value<pool.Capacity();++value) {
            const auto raw=pool.Slot(Sid{value});
            if ((raw[kState]&(1|2|4|8)) || raw[kType]<kFirstAssetTypeNumber || !(types[raw[kType]].flags1&TypeFlag1::kSurface)) continue;
            const auto number=raw[patch ? 30 : 28];CHECK(number!=Graph::kInvalid && graph.Records()[number].inUse!=0);++counts[number];
        }
        std::uint32_t total=0;
        // 첫 객체의 기본 번호 0은 Flood에서 감소한다. 예약 레코드의 -1 흔적을 표면 수로 세지 않는다.
        CHECK(counts[0]==0 && graph.Records()[0].surfaces==-1 && graph.Records()[0].inUse==1 && graph.Records()[0].reserved==1);
        // 예약 0번 다음 활성 레코드의 signed WORD 수와 실제 슬롯 수를 하나씩 대조한다.
        for (std::size_t i=1;i<Graph::kCount;++i) {
            CHECK(graph.Records()[i].surfaces==static_cast<std::int16_t>(counts[i]));
            CHECK((graph.Records()[i].inUse!=0)==(counts[i]!=0));total+=counts[i];
        }
        CHECK(total==expected && bookkeeping.depth==0 && destroy.PreDepth()==0 && destroy.PostDepth()==0);
    };
    checkGraph(scenario==Scenario::Isolated ? 10U : 11U);
    Sid walker{};
    if (removeSurfaceBefore) {
        // walker는 합성 raw 입력이며 표면 삭제/Unpop/탐색/통계 갱신은 실제 모듈이다.
        walker=pool.Allocate(2);auto raw=pool.AllocatedBytes(walker);Put(raw,0,kVtable);raw[kType]=85;raw[kState]=0;
        Put(raw,kX,std::bit_cast<std::uint32_t>(29.0f));Put(raw,kY,std::bit_cast<std::uint32_t>(29.0f));
        auto& head=hash.Bucket(1,29,29);Put(raw,4,head,2);head=walker.value;
        const Sid cell=connect.FindTypeAt(29,29,kSurface);CHECK(cell.value!=0);
        CHECK(virtualDestroy.Destroy(cell,0,host.Hooks()));
        CHECK(fallen==std::vector<Sid>({walker}) && hash.Bucket(0,29,29)==0 && (pool.Slot(cell)[kState]&kFree)!=0);
    }
    const auto freeBefore=pool.FreeCount();
    // 이 실제 삭제가 다리 예약을 만든다. 받침은 해시에서 빠지고 종유석/표면/연결은 각각 남는다.
    sink.renderer.Draw(pixels,640,7);sink.renderer.Present([](netstorm::client::ScreenRect){});sink.notices=0;
    CHECK(virtualDestroy.Destroy(island,scenario==Scenario::Keep ? kIslandDestroyKeepsBridges : 0,host.Hooks()));
    CHECK((pool.Slot(island)[kState]&kFree)!=0 && connect.FindTypeAt(30,30,kIsland).value==0);
    CHECK((pool.Slot(link)[kState]&kFree)==0 && !linkCheck.LinkNeedsDestroy(link));
    CHECK(bookkeeping.globalCounts[kIsland]==0 && bookkeeping.globalCounts[kSurface]==(removeSurfaceBefore ? 8U : 9U) && bookkeeping.globalCounts[kStalag]==1);
    if (scenario==Scenario::Keep) {
        CHECK(order.empty() && kernel.Size()==0 && !HasScheduledBridgeFall(host,source));
    } else {
        auto* scheduled=host.FindEvent(source,kBridgeFallEvent);
        CHECK(scheduled!=nullptr && kernel.Size()==1 && order==std::vector<std::string>({"schedule"}));
        if (scheduled) CHECK(scheduled->Payload()==7196 && scheduled->Count()==0);
        CHECK(Get(pool.Slot(source),frameOffset,frameWidth)==(scenario==Scenario::Hard ? 46U : 41U));
        kernel.RunFrame();
    }
    if (scenario==Scenario::Hard || scenario==Scenario::Keep) {
        CHECK(born.value==0 && removedLinks.empty() && effects.empty() && (pool.Slot(source)[kState]&kFree)==0);
        CHECK(Get(pool.Slot(source),frameOffset,frameWidth)==(scenario==Scenario::Hard ? 46U : 41U));
        if (scenario==Scenario::Hard) CHECK(kernel.Size()==1 && ((Get(pool.Slot(source),kWord,2)&kBridgeLifeMask)>>3)==4);
        if (scenario==Scenario::Hard && graphActive) {
            const auto before=Hex(pool.Bytes());const auto beforeCost=bookkeeping.totalCost;const auto beforeNotices=sink.notices;
            // 실제 자기 삭제 이벤트도 같은 가상 재정의를 거쳐 부모/예약/연결을 보존한다.
            CHECK(events.Handle(source,kBridgeDestroyEvent,0,0)==kBridgeEventKeep);
            CHECK(Hex(pool.Bytes())==before && bookkeeping.totalCost==beforeCost && sink.notices==beforeNotices && kernel.Size()==1);
            checkGraph(11);
            // 편집기에서는 재정의의 거부 조건이 풀린다. 실제 삭제로 Regular와 연결을 정리한다.
            mode.editor=true;
        }
        CHECK(virtualDestroy.Destroy(source,0,host.Hooks()));
    } else {
        CHECK(order==std::vector<std::string>({"schedule","bridge","link","effect","effect"}));
        if (scenario==Scenario::Replace) {
            CHECK(born.value!=0 && hash.Bucket(0,31,29)==born.value);
            CHECK(Get(pool.Slot(born),frameOffset,frameWidth)==56 && pool.Slot(born)[ownerOffset]==3);
            CHECK(((Get(pool.Slot(born),kWord,2)&kBridgeLifeMask)>>3)==4 && Get(pool.Slot(born),8,2)==23);
            CHECK((pool.Slot(born)[extraOffset]&0x10)!=0 && std::bit_cast<float>(Get(pool.Slot(born),kX))==31 && std::bit_cast<float>(Get(pool.Slot(born),kY))==29);
            CHECK(bookkeeping.totalCost==terrainCost+2*static_cast<std::int32_t>(types[kBridge].cost) && bookkeeping.globalCounts[kBridge]==2 && spots[29*256+31]==4);
        } else CHECK(born.value==0 && hash.Bucket(0,31,29)==0 && bookkeeping.totalCost==terrainCost && bookkeeping.globalCounts[kBridge]==0 && spots[29*256+31]==0);
    }
    CHECK(removedLinks==std::vector<Sid>({link}) && (pool.Slot(link)[kState]&kFree)!=0 && hash.Bucket(linkLevel,30,29)==0);
    CHECK((pool.Slot(source)[kState]&kFree)!=0 && !HasScheduledBridgeFall(host,source) && kernel.Size()==0);
    CHECK(effects==std::vector<BridgeLifecycleEffect>({BridgeLifecycleEffect::NotifyRemoval,BridgeLifecycleEffect::FallSound}));
    CHECK(bookkeeping.globalCounts[kConnector]==0 && bookkeeping.depth==0 && destroy.PreDepth()==0 && destroy.PostDepth()==0);
    CHECK(fallen==(removeSurfaceBefore ? std::vector<Sid>({walker}) : std::vector<Sid>{}));
    // 받침/옛 다리/연결은 반납되고 교체 시 새 다리 하나만 남는다. ProcessForm은 할당/반납이 상쇄된다.
    CHECK(pool.FreeCount()==freeBefore+(scenario==Scenario::Replace ? 2U : 3U));
    checkGraph((removeSurfaceBefore ? 8U : 9U)+(scenario==Scenario::Replace ? 2U : (scenario==Scenario::Isolated ? 0U : 1U)));
    CHECK(sink.notices>0 && !sink.renderer.Draw(pixels,640,9).empty());sink.renderer.Present([](netstorm::client::ScreenRect){});
    if (assets) std::printf("{\"edition\":\"%s\",\"scenario\":%d,\"invalidations\":%zu,\"surface_count\":%u,\"passed\":%s}\n",
        patch ? "10.78" : "CD",static_cast<int>(scenario),sink.notices,bookkeeping.globalCounts[kSurface],netstorm::test::FailureCount()==0 ? "true" : "false");
}
// 프레임 폭과 가상 표가 다른 두 판본에 같은 통합 시나리오를 적용한다.
void Both(Scenario scenario) {
    // CD와 패치의 실제 Factory 및 ProcessForm 경로를 각각 실행한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) Run(edition,scenario);
}
}
// 섬 삭제 후 O 끝 칸을 생성하고 old bridge/ProcessForm/연결을 정리한다.
TEST_CASE(surface_lifecycle_island_delete_replaces_bridge_and_cleans_link) { Both(Scenario::Replace); }
// 새 끝 방향에 살아 있는 이웃이 없으면 새 다리를 만들지 않는다.
TEST_CASE(surface_lifecycle_island_delete_removes_isolated_bridge) { Both(Scenario::Isolated); }
// hard 다리는 수명만 바꾸며 예약/연결을 유지한다. 후속 실제 삭제가 둘 다 정리한다.
TEST_CASE(surface_lifecycle_hard_bridge_keeps_link_until_real_delete) { Both(Scenario::Hard); }
// flags 0x1000은 받침만 삭제하고 예약을 막는다. 후속 실제 다리 삭제의 연결 정리는 유지된다.
TEST_CASE(surface_lifecycle_keep_bridges_skips_fall_schedule) { Both(Scenario::Keep); }
// noIsland postDestroy의 실제 분배가 같은 칸의 walker에 도달하고 뒤따르는 섬/다리 삭제도 정상 종료한다.
TEST_CASE(surface_lifecycle_surface_delete_dispatches_walker_before_island_fall) {
    // 두 판본의 raw 해시 순서와 postDestroy 재정의를 각각 확인한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) Run(edition,Scenario::Replace,true);
}
// Graph 활성 생성·분할·끝 칸 재등록을 표시와 공통 장부까지 함께 검사한다.
TEST_CASE(surface_lifecycle_graph_active_replacement_isolation_hard_and_keep) {
    // 두 판본에서 받침/종유석/연결 생성과 네 가지 삭제 흐름을 검사한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072})
        // hard는 큰 Graph에서 가상 삭제를 거부하고 editor 전환 후에만 정리한다.
        for (const auto scenario:{Scenario::Replace,Scenario::Isolated,Scenario::Hard,Scenario::Keep}) Run(edition,scenario,false,true);
}
// 미연결 효과를 구성 때 거부하고 공통 선택/form 계약과 실제 가상 표 분배를 유지한다.
TEST_CASE(surface_lifecycle_rejects_missing_hooks_and_preserves_common_dispatch) {
    SidPool pool(OriginalEdition::Patch1078,32768,true);SquidHash hash;std::vector<std::uint8_t> spots(65536);
    std::vector<RiftTypeRecord> types(188);auto frames=FrameTable(types.size());
    RawSquidNeighbors neighbors(pool,hash,spots,types,frames);int baseCalls=0,formCalls=0;
    SquidDestroyHooks common{[&](const SquidDestroyEvent&) { ++baseCalls; },[] { return Sid{9}; },[&](Sid) { ++formCalls; }};
    // 잘못된 분배가 곧바로 드러나는 외부 경계를 공급한다.
    SurfaceLifecycleHooks effects{
        [](Sid,float,float) { throw std::logic_error("예상 밖 예약"); },
        [](Sid,std::uint32_t) { throw std::logic_error("예상 밖 연결 삭제"); },
        [](Sid) { throw std::logic_error("예상 밖 walker 낙하"); },
        [](const BridgeLifecycleEvent&) { throw std::logic_error("예상 밖 다리 효과"); }};
    auto broken=effects;broken.destroy={};CHECK(Throws([&] { RawSurfaceLifecycle invalid(pool,neighbors,kConnector,common,broken); }));
    auto missing=common;missing.selected={};CHECK(Throws([&] { RawSurfaceLifecycle invalid(pool,neighbors,kConnector,missing,effects); }));
    CHECK(Throws([&] { RawSurfaceLifecycle invalid(pool,neighbors,188,common,effects); }));
    SidPool other(OriginalEdition::Patch1078,32768,true);
    CHECK(Throws([&] { RawSurfaceLifecycle invalid(other,neighbors,kConnector,common,effects); }));
    RawSurfaceLifecycle lifecycle(pool,neighbors,kConnector,common,effects);auto hooks=lifecycle.Hooks();
    const Sid sid=pool.Allocate(2);auto raw=pool.AllocatedBytes(sid);Put(raw,0,kVtable);raw[kType]=static_cast<std::uint8_t>(kBridge);
    // 타입 번호가 bridge여도 실제 가상 표가 다른 입력은 공통 훅만 사용한다.
    hooks.emit({SquidDestroyEffect::PreDestroy,sid,{},0});hooks.emit({SquidDestroyEffect::PostDestroy,sid,{},0});
    hooks.emit({SquidDestroyEffect::ReleaseDependent,sid,{},0});hooks.unpopForm(sid);
    CHECK(baseCalls==3 && hooks.selected()==Sid{9} && formCalls==1);
}

namespace netstorm::test {
// CTest의 합성 입력과 같은 통합 검사에 읽기 전용 원본 자산을 공급한다.
void InspectSurfaceLifecycle(const std::filesystem::path& root,o::OriginalEdition edition) {
    o::BaseFileSystem files(root);files.RegisterArchive(root/"netstorm.tarc");client::GameAssets assets(files,edition);
    // 끝 칸 생성·고립 삭제·hard 삭제 거부·flags 보존을 실제 SHP로 각각 검사한다.
    for (const auto scenario:{Scenario::Replace,Scenario::Isolated,Scenario::Hard,Scenario::Keep}) Run(edition,scenario,false,true,&assets);
}
}
