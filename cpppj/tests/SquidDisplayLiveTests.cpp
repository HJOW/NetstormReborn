// 실제 SquidDisplay/Storage가 현재 Client 진단/타입/상세 owner를 사용하는지 창 없이 확인한다.
#include "RawSceneSupport.h"
#include "client/ClientGameplayGlobals.h"
#include "client/RawSurfaceWorld.h"
#include "o/SquidDisplay.h"
using namespace netstorm::test::rawscene;
using namespace netstorm::client;
namespace { // 실제 표시 계산과 진단 장치를 분리해 원문 순서를 기록한다.
struct DisplayLiveSink final:SquidDisplaySink {
    ClientAiDebugGlobals owner; // 현재 checking과 nullable 오류 상세의 장기 owner다.
    std::vector<RiftTypeRecord>& types;std::vector<SquidDisplayShape>& shapes; // 표시기와 같은 현재 배열을 직접 빌린다.
    bool suppressed{};mutable std::vector<std::string> events;std::vector<SquidDisplayRect> rectangles;std::vector<std::uint32_t> markers; // 실제 자식/진단 효과의 순서와 출력이다.
    std::function<void()> errorEffect,assertionEffect; // 선택된 명시 장치가 현재 raw/TYPE를 바꾸는 입력이다.
    DisplayLiveSink(std::vector<RiftTypeRecord>& inputTypes,std::vector<SquidDisplayShape>& inputShapes):types(inputTypes),shapes(inputShapes) {} // 배열 주소는 이 장치보다 오래 유지한다.
    RawCheckingFlag Checking() const override { return RawCheckingFlag{owner.Checking()}; } // 호출마다 같은 Client DWORD를 빌린다.
    bool Suppressed() const override { events.push_back("S");return suppressed; } // 억제 조회가 부모 진단 뒤인지 기록한다.
    void Invalidate(SquidDisplayRect rect,std::uint32_t flags) override { events.push_back("I");rectangles.push_back(rect);markers.push_back(flags); } // 기존 실제 계산 결과만 수집한다.
    void FrameError(std::uint32_t type,std::int32_t frame,std::uint32_t abstract) override { events.push_back("E");owner.AssertionDetail()=BuildSquidFrameAssertionDetail(types,shapes,type,frame,abstract);if (errorEffect) errorEffect(); } // 세 인자로 실제 상세를 저장한다.
    void Assertion(std::string_view expression,std::string_view file,std::uint32_t line) override { CHECK(expression=="getTypeStruct()->frameCheck( isAbstract(), theFrame, true )" && file=="\\Ns\\O\\Squid.cpp" && line==0x846);events.push_back("D");if (assertionEffect) assertionEffect(); } // 복귀 뒤 실제 부모가 타입을 재조회한다.
};
struct DisplayLiveScene { // 실제 공통88과 서로 다른 두 타입/프레임의 최소 물리 입력이다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188);std::vector<SquidDisplayShape> shapes=std::vector<SquidDisplayShape>(188);std::array<std::uint8_t,50> raw{}; // 같은 표시 부모가 직접 읽는다.
    DisplayLiveSink sink{types,shapes};SquidDisplay display{OriginalEdition::Patch1078,types,shapes,sink,{0,0,65536,{0,0,640,480}}}; // 모형 표시 계산을 만들지 않고 실제 표시기를 호출한다.
    DisplayLiveScene() {
        types[70].name="oldType";types[71].name="newType";shapes[70].loaded=shapes[71].loaded=true;shapes[70].frameCount=1;shapes[71].frameCount=3;shapes[70].frames.resize(1);shapes[71].frames.resize(3);shapes[71].frames[0].width=10;shapes[71].frames[0].height=12;shapes[71].frames[2].width=20;shapes[71].frames[2].height=6;
        Put(raw,0,0x501dd0);raw[10]=70;raw[40]=1;sink.owner.Checking()=1;
    } // 첫 프레임/위치는0이며 실제 타입 및 좌표 변경은 각 검사 장치가 수행한다.
    void ChangeCurrent(std::uint32_t extra) { raw[10]=71;Put(raw,36,9);Put(raw,14,std::bit_cast<std::uint32_t>(4.0f));Put(raw,18,std::bit_cast<std::uint32_t>(5.0f));raw[40]=static_cast<std::uint8_t>(extra);sink.owner.Checking()=0; } // raw frame을 바꿔도 부모의 지역값은 고정되어야 한다.
};
}

// SHP 해제 표식 진단이 복귀하면 현재 새 TYPE/좌표와 옛 지역 frame으로 기존 Mark를 실행한다.
TEST_CASE(SquidDisplayLive_DiagnosticReturnRequeriesCurrentTypeAndCoordinates) {
    DisplayLiveScene scene;scene.types[70].maxHitPoints=std::bit_cast<std::int32_t>(0xddddddddU);scene.sink.assertionEffect=[&] { scene.ChangeCurrent(4); };scene.display.Update(scene.raw);
    CHECK(scene.sink.events==std::vector<std::string>({"D","S","I"}));CHECK(!scene.sink.owner.AssertionDetail());CHECK(scene.sink.rectangles.size()==1 && scene.sink.markers==std::vector<std::uint32_t>{16});const auto rect=scene.sink.rectangles.at(0);CHECK(rect.left==52 && rect.top==40 && rect.right==87 && rect.bottom==68);CHECK(Get(scene.raw,36)==9);
}

// 오류 상세를 저장하는 효과가 checking을 꺼도 이미 선택된 진단을 실행하고 고정 frame을 유지한다.
TEST_CASE(SquidDisplayLive_ThreeArgumentDetailBeforeDiagnosticAndFixedFrame) {
    DisplayLiveScene scene;Put(scene.raw,36,2);scene.sink.errorEffect=[&] { scene.ChangeCurrent(0); };scene.display.Update(scene.raw);
    CHECK(scene.sink.events==std::vector<std::string>({"E","D","S","I"}));CHECK(scene.sink.owner.AssertionDetail()=="Frame assertion failed. Asking for type=70 (oldType) frame=2 abs=1\n\nType has 1 frames\n");const auto rect=scene.sink.rectangles.at(0);CHECK(rect.left==64 && rect.top==55 && rect.right==85 && rect.bottom==62 && scene.sink.markers.at(0)==0);
}

// 표시 억제는 기본88 부모의 진단을 건너뛰지 않고 실제 Mark 자식만 억제한다.
TEST_CASE(SquidDisplayLive_SuppressionFollowsParentAndInvalidTypeDetail) {
    DisplayLiveScene scene;scene.sink.suppressed=true;Put(scene.raw,36,2);scene.display.Update(scene.raw);CHECK(scene.sink.events==std::vector<std::string>({"E","D","E"}));scene.sink.events.clear();Put(scene.raw,36,0);scene.display.Update(scene.raw);CHECK(scene.sink.events==std::vector<std::string>{"S"} && scene.sink.rectangles.empty());
    CHECK(BuildSquidFrameAssertionDetail(scene.types,scene.shapes,0xffffffffU,-5,0)=="Frame assertion failed. Asking for type=-1 (invalidType) frame=-5 abs=0\n\nType has 0 frames\n"); // 원문 signed 타입 범위와 nullable 상세 저장을 구별한다.
}

// 실제 World sink/Storage가 Client의 동일한 진단/상세 주소를 쓰고 표시 뷰 교체로 끊기지 않는다.
TEST_CASE(SquidDisplayLive_ActualStorageAndClientDetailSurviveWorldView) {
    ClientGameplayGlobals globals;std::vector<RiftTypeRecord> types(188);std::vector<RiftTypeFrames> frames(188,RiftTypeFrames(std::vector<FrameCode>{}));std::vector<SquidDisplayShape> shapes(188);types[70].name="liveType";shapes[70].loaded=true;shapes[70].frameCount=1;shapes[70].frames.resize(1);shapes[70].frames[0].width=10;shapes[70].frames[0].height=12;std::uint32_t diagnostics{}; // 순수 창 없는 실제 World의 최소 배열이다.
    auto storage=std::make_shared<RawSurfaceStorage>(OriginalEdition::Patch1078,types,frames,shapes);storage->BindChecking(RawCheckingFlag{globals.AiDebug().Checking()});storage->BindDisplayDiagnostics(globals.AiDebug().AssertionDetail(),[&](auto,auto,auto line) { CHECK(line==0x846);++diagnostics;globals.AiDebug().Checking()=0; });globals.AiDebug().Checking()=1;
    BridgeConnectState links;links.bridgeType=82;links.islandType=94;links.stalagType=95;links.connectorType=155;links.noIslandType=157;RawSurfaceWorld world(OriginalEdition::Patch1078,types,frames,shapes,links,70,nullptr,storage);const auto sid=storage->pool.Allocate(2);auto raw=storage->pool.AllocatedBytes(sid);Put(raw,0,0x501dd0);raw[10]=70;raw[40]=1;Put(raw,36,0xffffffffU);std::uint32_t invalidates{};world.SetDisplay({0,0,65536,{0,0,640,480}},{[] { return false; },[&](auto,auto) { ++invalidates; }});world.Core().display.Update(raw);
    CHECK(diagnostics==1 && invalidates==0);CHECK(globals.AiDebug().AssertionDetail()=="Frame assertion failed. Asking for type=70 (liveType) frame=-1 abs=1\n\nType has 1 frames\n");Put(raw,36,0);world.SetDisplay({0,0,65536,{0,0,640,480}},{[] { return false; },[&](auto,auto) { ++invalidates; }});world.Core().display.Update(raw);CHECK(diagnostics==1 && invalidates==1 && globals.AiDebug().AssertionDetail().has_value());
}
