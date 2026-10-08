// GUI가 소유하는 raw 월드의 시각·정지·장치 교체·예약 수명을 원본 파일 없이 검사한다.
#include "RawSceneSupport.h"
#include "client/RawSurfaceWorld.h"
#include "o/SquidFactory.h"
#include <limits>

using namespace netstorm::o;
using namespace netstorm::client;
using namespace netstorm::test::rawscene;
namespace {
// 실제 표면 타입 번호 및 정상/단단한 K 다리의 원본 프레임 번호다.
constexpr std::uint32_t kBridge=82,kTerrain=83,kIsland=94,kStalag=95,kConnector=155,kSurface=157;
constexpr int kNormal=41,kHard=46,kEnd=56;
// 저장된 실제 bridge.type 코드 한 행을 읽는다. 연결 결과를 합성하지 않는다.
RiftTypeFrames ActualBridgeFrames() {
    std::ifstream input(NETSTORM_BRIDGEDECAY_FIXTURE);std::string line;
    // 첫 프레임 행 뒤의 큰 관찰 목록은 읽지 않는다.
    while (std::getline(input,line)) {
        if (!line.starts_with("Frames\t0\t")) continue;
        const auto hex=Split(line,'\t').at(2);std::vector<FrameCode> result;
        // 네 바이트가 하나의 프레임 코드다.
        for (std::size_t i=0;i+7<hex.size();i+=8) {
            const auto byte=[&](std::size_t offset) { return static_cast<std::uint8_t>(std::stoul(hex.substr(i+offset,2),nullptr,16)); };
            result.push_back({byte(0),byte(2),byte(4),byte(6)});
        }
        return RiftTypeFrames(std::move(result));
    }
    throw std::runtime_error("실제 다리 프레임 fixture 없음");
}
struct Inputs {
    std::vector<RiftTypeRecord> types;
    std::vector<RiftTypeFrames> frames;
    std::vector<SquidDisplayShape> shapes;
    BridgeConnectState links;
    // 실제 생성자/플래그와 작은 합성 SHP를 사용한다. 실제 SHP는 GUI 스모크에서 별도 확인한다.
    explicit Inputs(OriginalEdition edition):types(edition==OriginalEdition::Patch1078 ? 188 : 171),
        frames(types.size(),RiftTypeFrames({})),shapes(types.size()) {
        const auto define=[&](std::uint32_t type,std::uint32_t flags1,std::uint32_t flags2,int foot) {
            auto& value=types[type];value.constructorAddress=TypeConstructorAddress(edition,type);
            value.flags1=flags1;value.flags2=flags2;value.footX=value.footY=foot;
        };
        define(kBridge,0x802,4,1);types[kBridge].cost=5;define(kTerrain,0x28000803,2,1);
        define(kIsland,0x28000003,0x01000000,3);define(kStalag,0x28000002,0,3);
        define(kConnector,0x28000002,0x02000000,1);define(kSurface,0x08000803,0x01000002,1);
        frames[kBridge]=ActualBridgeFrames();frames[kTerrain]=RiftTypeFrames({FrameCode{'A','A',0,0}});
        std::vector<FrameCode> surface;
        // noIsland 적재에서 선택되는 실제 글자 순서다.
        for (const char side:std::string_view("FCGBADIEH")) surface.push_back({static_cast<std::uint8_t>(side),'P',1,0});
        frames[kSurface]=RiftTypeFrames(std::move(surface));
        // 소유자별 받침·종유석 프레임과 연결 조각에 실제 크기의 발자국을 공급한다.
        for (const auto type:{kIsland,kStalag,kConnector}) frames[type]=RiftTypeFrames(std::vector<FrameCode>(9,FrameCode{'A','P',1,0}));
        // 표시 픽셀과 공간 칸 크기를 각각 공급한다.
        for (const auto type:{kBridge,kTerrain,kIsland,kStalag,kConnector,kSurface}) {
            auto& shape=shapes[type];shape.loaded=true;shape.frameCount=static_cast<int>(frames[type].Codes().size());
            shape.frames.assign(static_cast<std::size_t>(shape.frameCount),SquidDisplayFrame{20,16,8,12,
                static_cast<float>(types[type].footX),static_cast<float>(types[type].footY)});
        }
        links.bridgeType=kBridge;links.islandType=kIsland;links.stalagType=kStalag;links.connectorType=kConnector;
        links.noIslandType=kSurface;links.battle=true;
    }
    // 월드 생성 뒤 입력 자료 수명이 끝나도 내부 표는 유효하다.
    std::unique_ptr<RawSurfaceWorld> Make(OriginalEdition edition) const {
        return std::make_unique<RawSurfaceWorld>(edition,types,frames,shapes,links,kTerrain);
    }
};
// 순서를 뒤섞은 3×3 받침과 동쪽 두 다리를 초기 입력으로 제공한다.
std::vector<SurfaceSeed> Seeds(bool hard=false) {
    std::vector<SurfaceSeed> result{{kBridge,3,kNormal,32,29},{kBridge,3,hard ? kHard : kNormal,31,29}};
    // 역순 저장이어도 월드는 행 우선으로 적재해야 한다.
    for (int y=30;y>=28;--y)
        // 같은 행도 뒤에서부터 전달한다.
        for (int x=30;x>=28;--x) result.push_back({kSurface,0,0,static_cast<float>(x),static_cast<float>(y)});
    return result;
}
// 요약의 필드만 읽고 raw SID 번호는 시나리오에서 직접 조회한다.
std::vector<std::string> Summary(const RawSurfaceWorld& world) { const auto report=world.Report();return Split(report.substr(0,report.find('\n')),'\t'); }
// 특정 SID의 현재 렌더링 입력을 찾는다.
SurfaceSeed Object(const RawSurfaceWorld& world,Sid sid) {
    // live snapshot은 현재 SID를 기준으로 찾는다.
    for (const auto& snapshot:world.Objects()) if (snapshot.sid==sid) return snapshot.object;
    throw std::runtime_error("검사할 raw 객체 없음");
}
}

TEST_CASE(RawSurfaceWorld_PausedRegularResumesWithClientGameClock) {
    // 두 판본 모두 같은 GUI 호스트 경로를 사용한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        auto world=Inputs(edition).Make(edition);world->Load(Seeds());
        CHECK(world->Objects().size()==14);CHECK(Summary(*world)[4]=="11" && Summary(*world)[5]=="1");
        const auto bridge=world->Find(31,29,kBridge),link=world->Find(30,29,kConnector),island=world->Find(30,30,kIsland);
        CHECK(bridge.value && link.value && island.value);CHECK(Object(*world,island).owner==3 && Object(*world,island).frame==2);
        GameClock clock;world->RunFrame(clock.Capture(1000),false);clock.Pause(1000);
        CHECK(world->Destroy(island));CHECK(Summary(*world)[3]=="1");world->TakeChanged();
        world->RunFrame(clock.Capture(8000),clock.IsPaused());
        CHECK(world->Find(31,29,kBridge)==bridge && world->Find(30,29,kConnector)==link);
        CHECK(Summary(*world)[1]=="1" && Summary(*world)[3]=="1" && !world->TakeChanged());
        clock.Resume(8000);world->RunFrame(clock.Capture(8000),clock.IsPaused());
        const auto end=world->Find(31,29,kBridge);CHECK(end.value && end!=bridge && Object(*world,end).frame==kEnd);
        CHECK(world->Find(30,29,kConnector).value==0 && world->Find(32,29,kBridge).value!=0);
        const auto summary=Summary(*world);CHECK(summary[3]=="0" && summary[4]=="11" && summary[6]=="10");
        CHECK(summary[7]=="0" && summary[8]=="0" && summary[9]=="0" && world->TakeChanged());
        CHECK(Throws([&] { world->RunFrame({0.5,0,0,4},false); }));
    }
}
TEST_CASE(RawSurfaceWorld_HardBridgeAndPendingMissionTeardown) {
    // 미션 종료 시 유지되는 Regular가 있어도 다음 월드와 SID/시각을 공유하지 않는다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        auto world=Inputs(edition).Make(edition);world->Load(Seeds(true));world->TakeChanged();
        const auto bridge=world->Find(31,29,kBridge);const auto before=world->Report();
        CHECK(!world->Destroy(bridge) && world->Report()==before && !world->TakeChanged());
        CHECK(world->Destroy(world->Find(30,30,kIsland)));world->RunFrame({12,12,12,1},false);
        CHECK(world->Find(31,29,kBridge)==bridge && Object(*world,bridge).frame==kHard && Summary(*world)[3]=="1");
        world.reset();world=Inputs(edition).Make(edition);world->Load(Seeds());world->RunFrame({0,0,0,1},false);
        CHECK(Summary(*world)[3]=="0" && world->Find(30,30,kIsland).value!=0 && Object(*world,world->Find(31,29,kBridge)).frame==kNormal);
    }
}
TEST_CASE(RawSurfaceWorld_DisplaySuppressionAndDeviceReplacement) {
    auto world=Inputs(OriginalEdition::Patch1078).Make(OriginalEdition::Patch1078);world->Load(Seeds());
    bool suppressed=true;int oldNotices=0,newNotices=0;
    world->SetDisplay({400,280,65536,{0,0,640,480}},{[&] { return suppressed; },[&](SquidDisplayRect,std::uint32_t) { ++oldNotices; }});
    world->TakeChanged();world->SetSupportOwner(30,30,4);CHECK(world->TakeChanged() && oldNotices==0);
    CHECK(Object(*world,world->Find(30,30,kIsland)).frame==3);
    suppressed=false;world->SetDisplay({400,280,65536,{0,0,640,480}},{[&] { return suppressed; },[&](SquidDisplayRect rect,std::uint32_t) {
        CHECK(rect.left<rect.right && rect.top<rect.bottom);++newNotices;
    }});
    world->SetSupportOwner(30,30,5);CHECK(newNotices>0 && oldNotices==0 && world->TakeChanged());
    CHECK(Object(*world,world->Find(30,30,kStalag)).frame==4);
}
TEST_CASE(RawSurfaceWorld_InvalidLoadRejectedBeforeAllocation) {
    auto world=Inputs(OriginalEdition::Patch1078).Make(OriginalEdition::Patch1078);const auto before=world->Report();
    auto seeds=Seeds();seeds.back().x=std::numeric_limits<float>::quiet_NaN();
    CHECK(Throws([&] { world->Load(seeds); }) && world->Report()==before);
    seeds=Seeds();seeds.back().owner=9;CHECK(Throws([&] { world->Load(seeds); }) && world->Report()==before);
    world->Load(Seeds());const auto loaded=world->Report();CHECK(Throws([&] { world->Load(Seeds()); }) && world->Report()==loaded);
}
