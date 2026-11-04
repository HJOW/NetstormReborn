// 원문 SHP 진단 복귀와 현재 TYPE 배열의 실제 후처리/그래프/이웃 연결을 창 없이 검사한다.
#include "TestSupport.h"
#include "o/RawRiftShapeAccess.h"
#include "o/RawGraph.h"
#include "o/RawSquidNeighbors.h"
#include "o/SquidFactory.h"
#include "o/SquidPostPop.h"
#include "client/ClientMain.h"
#include "client/ClientGameplayGlobals.h"
#include "client/RawSurfaceWorld.h"
#include <algorithm>
#include <bit>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

using namespace netstorm::o;
using namespace netstorm::client;
namespace { // 모든 callback은 명시 입력/관찰이며 부모 분기나 반환을 대신 계산하지 않는다.
template<class F> bool ShapeFailed(F&& action) { try { action(); } catch (const std::exception&) { return true; }return false; } // 선택된 장치/물리 범위의 명시 실패를 관찰한다.
struct ShapeAccessScene { // 저장된 TYPE/frame과 진단이 변경할 현재 필드를 보관한다.
    std::uint32_t token{74},flags{},errors{},assertions{},reads{},selected{},abstract{}; // 원문 번호/플래그와 실제 경계 관측이다.
    std::int32_t count{1},header{},savedFrame{}; // 현재 프레임 수/표식과 shape에서 받은 저장 frame이다.
    bool loaded{true}; // 현재 SHP null 여부다.
    std::array<float,2> size{2,3}; // 실제 SHP 경계의 명시 반환 입력이다.
    std::vector<std::string> events; // 상세→진단→물리 읽기 순서를 보관한다.
    std::function<void()> onError,onAssertion,onShape; // 각 원문 외부 경계의 명시 변경/실패다.
    RiftShapeAccessHooks hooks; // 같은 실제 부모에 공급하는 장치다.
    ShapeAccessScene() { // 요구된 경계에서만 현재 입력과 호출 순서를 기록한다.
        hooks.typeNumber=[this](RawSquidDisplayTypeView type) { events.push_back("number");return type.token; };
        hooks.frameError=[this](auto type,auto frame,auto abs) { ++errors;selected=type;savedFrame=frame;abstract=abs;events.push_back("error");const auto call=onError;if (call) call(); };
        hooks.assertion=[this](auto expression,auto file,auto line) { CHECK(expression=="frameCheck( abstract, frame, false )" && file==R"(..\o\rifttype.h)" && line==0xe1);++assertions;events.push_back("assert");const auto call=onAssertion;if (call) call(); };
        hooks.shape=[this](auto type,auto frame) { ++reads;selected=type;savedFrame=frame;events.push_back("shape");const auto call=onShape;if (call) call();return size; };
    }
    std::array<float,2> Run(std::int32_t frame=0,std::uint32_t abs=0) { return ReadRiftShapeFrame({token,flags,count,loaded,header},frame,abs,hooks); } // 같은 현재 필드 참조와 저장된 인자를 부모에 전달한다.
};
std::vector<RiftTypeRecord> CurrentTypeRows() { std::vector<RiftTypeRecord> rows(188);for (auto& row:rows) row.footX=row.footY=1;return rows; } // 명시 기본 발자국만 준비한다.
struct CurrentTypeScene { // 타입 복사와 현재 대여의 결과를 같은 실제 슬롯에서 비교한다.
    SidPool pool{OriginalEdition::Patch1078,24000,false}; // 서버/통신을 사용하지 않는 실제 raw 풀이다.
    Sid sid; // 한 번 확보한 client 슬롯이다.
    std::vector<RiftTypeRecord> types=CurrentTypeRows(); // 현재 필드는 재할당 없이 수정한다.
    SquidHash hash; // 같은 공간 해시다.
    std::vector<std::uint8_t> spots=std::vector<std::uint8_t>(65536); // 점유 지도 전체를 보관한다.
    CurrentTypeScene():sid(pool.Allocate(2)) { auto raw=pool.AllocatedBytes(sid);Put(0,0x501dd0);raw[10]=74;raw[11]=0;raw[30]=254;raw[34]=2;Put(14,std::bit_cast<std::uint32_t>(30.0F));Put(18,std::bit_cast<std::uint32_t>(40.0F)); } // 정상 base/current active의 명시 입력이다.
    void Put(std::size_t offset,std::uint32_t value) { auto raw=pool.AllocatedBytes(sid);for (std::size_t index=0;index<4;++index) raw[offset+index]=static_cast<std::uint8_t>(value>>(index*8)); } // 정확한 DWORD만 낮은 바이트부터 쓴다.
};
BridgeConnectState ShapeLinks() { BridgeConnectState links;links.bridgeType=82;links.islandType=94;links.stalagType=95;links.connectorType=155;links.noIslandType=157;return links; } // 실제 World 생성에 필요한 명시 종류 입력이다.
}

TEST_CASE(RiftShapeAccess_ValidFrameDoesNotReadDiagnosticDevices) { // 정상 범위는 번호/상세/진단 장치가 없어도 같은 물리를 읽는다.
    ShapeAccessScene scene;scene.hooks.typeNumber={};scene.hooks.frameError={};scene.hooks.assertion={};
    CHECK(scene.Run()==(std::array<float,2>{2,3}) && scene.reads==1 && scene.selected==74 && scene.savedFrame==0);
}
TEST_CASE(RiftShapeAccess_RangeErrorKeepsSelectedTypeAndSavedArguments) { // 상세가 범위를 고쳐도 e1은 실행하며 현재 raw TYPE 변경으로 선택 TYPE를 교체하지 않는다.
    ShapeAccessScene scene;scene.onError=[&] { CHECK(scene.selected==74 && scene.savedFrame==3 && scene.abstract==13);scene.count=8;scene.token=75; };
    scene.onAssertion=[&] { scene.size={5,6}; };CHECK(scene.Run(3,13)==(std::array<float,2>{5,6}));
    CHECK(scene.selected==74 && scene.savedFrame==3 && scene.abstract==13 && scene.assertions==1 && scene.events==(std::vector<std::string>{"number","error","assert","shape"}));
}
TEST_CASE(RiftShapeAccess_NullShapeRepairsWithoutFrameDetail) { // 범위는 정상이고 SHP만null이면 상세 없이 e1 뒤 현재 pointer 조건을 사용한다.
    ShapeAccessScene scene;scene.loaded=false;scene.hooks.typeNumber={};scene.hooks.frameError={};scene.onAssertion=[&] { scene.loaded=true;scene.size={7,8}; };
    CHECK(scene.Run()==(std::array<float,2>{7,8}) && scene.errors==0 && scene.assertions==1 && scene.reads==1);
}
TEST_CASE(RiftShapeAccess_FreedHeaderDiagnosticDoesNotRepeatPredicateAfterReturn) { // 해제 표식이 그대로여도 진단이 복귀하면 원문처럼 선택 SHP 읽기를 진행한다.
    for (const auto value:{-0x22222223,-0x32323233}) { ShapeAccessScene scene;scene.header=value;CHECK(scene.Run()==scene.size && scene.header==value && scene.errors==0 && scene.assertions==1 && scene.reads==1); } // 두 CRT 표식을 별도 입력으로 확인한다.
}
TEST_CASE(RiftShapeAccess_DoubleFrameCountUsesSignedDwordWrap) { // 물리 치수 입력은 같게 두고 원문 shl/cmp의 결과만 진단 횟수로 관찰한다.
    const std::array<std::array<std::int64_t,4>,5> samples{{{0,1,1,1},{0x40000,1,1,0},{0x400000,1,1,0},{0x40000,0x40000000,0,1},{0x40000,-2147483647,1,0}}}; // flags/count/frame/예상 범위 진단의 명시 표다.
    for (const auto& input:samples) { ShapeAccessScene scene;scene.flags=static_cast<std::uint32_t>(input[0]);scene.count=static_cast<std::int32_t>(input[1]);CHECK(scene.Run(static_cast<std::int32_t>(input[2]))==scene.size);CHECK(scene.errors==input[3] && scene.assertions==input[3] && scene.reads==1); } // 각 입력은 독립 현재 TYPE 필드를 사용한다.
}
TEST_CASE(RiftShapeAccess_FrameDetailFailureStopsBeforeAssertionAndPhysicalRead) { // 상세 writer가 반환하지 않으면 뒤 진단/읽기를 실행하지 않는다.
    ShapeAccessScene scene;scene.onError=[] { throw std::logic_error("명시상세실패"); };CHECK(ShapeFailed([&] { scene.Run(3); }));CHECK(scene.errors==1 && scene.assertions==0 && scene.reads==0);
}
TEST_CASE(RiftShapeAccess_AbortingAssertionKeepsEarlierDetail) { // e1 중단도 앞서 기록한 상세 인수와 순서를 지우지 않는다.
    ShapeAccessScene scene;scene.onAssertion=[] { throw std::logic_error("명시e1중단"); };CHECK(ShapeFailed([&] { scene.Run(3,1); }));CHECK(scene.errors==1 && scene.assertions==1 && scene.reads==0 && scene.savedFrame==3 && scene.abstract==1);
}
TEST_CASE(RiftShapeAccess_DiagnosticSelfRebindKeepsCurrentCallableAlive) { // 자기 연결을 비워도 실행 중인 진단은 유효하고 선택된 물리 읽기를 마친다.
    ShapeAccessScene scene;scene.hooks.assertion=[&](auto,auto,auto) { ++scene.assertions;scene.hooks.assertion={};scene.size={8,9}; };
    CHECK(scene.Run(3)==(std::array<float,2>{8,9}) && scene.assertions==1 && scene.reads==1);
}
TEST_CASE(CurrentTypeBindings_PostPopKeepsLoadedCostAndUsesCurrentRegistrationFlags) { // 최초 로딩 비용0을 유지하고 생성 뒤 변경한 같은 TYPE의 공급/공장군 플래그만 현재 표에서 읽는다(단위865).
    CurrentTypeScene scene;SquidPostPopState state;state.localOwner=2;state.providers.entries.resize(1);state.factories.entries.resize(1);state.ownerFactories[2].entries.resize(1);SquidPostPop post(scene.pool,scene.types,state);post.BindCurrentTypes(scene.types);
    scene.types[74].cost=7;scene.types[74].flags1=0x10000000;scene.types[74].flags2=0x4200;post.Activate(scene.sid,1);
    CHECK(state.totalCost==0 && state.providers.count==1 && state.factories.count==1 && state.ownerFactories[2].count==1 && state.productionDirty==2 && state.globalCounts[74]==1 && state.localCounts[74]==1 && state.depth==0);
}
TEST_CASE(CurrentTypeBindings_RejectsWrongLengthWithoutLosingLiveArray) { // 잘못된 새 대여를 거부해도 기존 실제 대여와 물리를 유지한다.
    CurrentTypeScene scene;std::vector<std::vector<FrameCode>> codes(188,std::vector<FrameCode>{{'A','0',0,0}});std::vector<RiftTypeFrames> frames(188,RiftTypeFrames({{'A','0',0,0}}));SquidPostPopState state;SquidPostPop post(scene.pool,scene.types,state);RawGraph graph(scene.pool,scene.hash,scene.spots,scene.types,codes);RawSquidNeighbors neighbors(scene.pool,scene.hash,scene.spots,scene.types,frames);
    post.BindCurrentTypes(scene.types);graph.BindCurrentTypes(scene.types);neighbors.BindCurrentTypes(scene.types);const auto wrong=std::span<const RiftTypeRecord>(scene.types).first(187);
    CHECK(ShapeFailed([&] { post.BindCurrentTypes(wrong); }) && ShapeFailed([&] { graph.BindCurrentTypes(wrong); }) && ShapeFailed([&] { neighbors.BindCurrentTypes(wrong); }));
    scene.types[74].cost=9;scene.types[74].footX=3;graph.ValidateTypes(scene.types);CHECK(neighbors.Footprint(scene.sid).left==28);post.Activate(scene.sid,1);CHECK(state.totalCost==0 && scene.pool.Slot(scene.sid)[10]==74); // 현재float9와 별도인 최초 비용 캐시0을 유지한다(단위865).
}
TEST_CASE(CurrentTypeBindings_NeighborFootprintDistinguishesOwnedAndBorrowedRows) { // 기존 독립 snapshot은 유지하고 명시 대여만 현재 발자국을 소비한다.
    CurrentTypeScene scene;std::vector<RiftTypeFrames> frames(188,RiftTypeFrames({}));RawSquidNeighbors neighbors(scene.pool,scene.hash,scene.spots,scene.types,frames);scene.types[74].footX=3;scene.types[74].footY=2;
    CHECK(neighbors.Footprint(scene.sid).left==30 && neighbors.Footprint(scene.sid).top==40);neighbors.BindCurrentTypes(scene.types);CHECK(neighbors.Footprint(scene.sid).left==28 && neighbors.Footprint(scene.sid).top==39);neighbors.BindCurrentTypes({});CHECK(neighbors.Footprint(scene.sid).left==30 && neighbors.Footprint(scene.sid).top==40);
}
TEST_CASE(CurrentTypeBindings_GraphAddReadsCurrentSurfaceAndFootprint) { // 생성 때 비표면이던 실제 대여 TYPE를 바꿔 같은 Graph의 검증/실행/이웃이 소비한다.
    CurrentTypeScene scene;std::vector<std::vector<FrameCode>> codes(188,std::vector<FrameCode>{{'A','0',0,0}});RawGraph graph(scene.pool,scene.hash,scene.spots,scene.types,codes);graph.BindCurrentTypes(scene.types);scene.types[74].flags1=0x800;scene.types[74].flags2=2;scene.hash.Cell(0,30,40)=scene.sid.value;
    graph.Add(scene.sid);const auto number=scene.pool.Slot(scene.sid)[30];CHECK(number==0 && graph.Records()[0].inUse==1 && graph.Records()[0].surfaces==1);
}
TEST_CASE(CurrentTypeBindings_ActualWorldPostPopKeepsLoadedCostBeforeCurrentAiNotice) { // 실제 World는 최초 비용 캐시0을 소비하며 현재float11/AI 통지 뒤99를 비용으로 재해석하지 않는다(단위865).
    auto types=CurrentTypeRows();std::vector<RiftTypeFrames> frames(188,RiftTypeFrames({}));std::vector<SquidDisplayShape> shapes(188);shapes[74].loaded=true;shapes[74].frameCount=1;shapes[74].frames.resize(1);shapes[74].frames[0].cellWidth=shapes[74].frames[0].cellHeight=1;
    RawSurfaceWorld world(OriginalEdition::Patch1078,types,frames,shapes,ShapeLinks(),83);auto core=world.Core();auto& current=world.Storage()->types;current[74].cost=11;const auto sid=world.Create(74);core.pool.AllocatedBytes(sid)[34]=1;
    core.postPop.BindPatchAiNotice([&](auto,Sid object,bool) { CHECK(object==sid && core.bookkeeping.TotalCost()==0);current[74].cost=99;core.pool.AllocatedBytes(sid)[10]=75; });world.Pop(sid,30,40,0x40);CHECK(core.bookkeeping.TotalCost()==0 && core.bookkeeping.GlobalCounts()[75]==1 && current[74].cost==99 && core.bookkeeping.depth==0);
}
TEST_CASE(RiftShapeAccess_ActualWorldUsesSelectedShapeAfterCurrentDiagnostic) { // checking0의 실제 Client 상세/e1 뒤 raw TYPE/frame 변경과 선택 SHP/frame을 구별한다.
    ClientOptions options;options.gameDirectory=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals";options.noAudio=true;Client client(std::move(options));auto types=CurrentTypeRows();types[74].name="source";types[74].flags2=1;std::vector<RiftTypeFrames> frames(188,RiftTypeFrames({}));std::vector<SquidDisplayShape> shapes(188);
    for (const auto number:{74U,75U}) { shapes[number].loaded=true;shapes[number].frameCount=1;shapes[number].frames.resize(2);shapes[number].frames[0].cellWidth=shapes[number].frames[0].cellHeight=1; } // 선택/변경 TYPE의 물리 입력을 독립적으로 둔다.
    auto storage=std::make_shared<RawSurfaceStorage>(OriginalEdition::Patch1078,types,frames,shapes);auto& checking=client.GameplayGlobals()->AiDebug().Checking();checking=0;storage->BindChecking(RawCheckingFlag{checking});RawSurfaceWorld world(OriginalEdition::Patch1078,types,frames,shapes,ShapeLinks(),83,nullptr,storage);auto core=world.Core();const auto sid=world.Create(74);auto raw=core.pool.AllocatedBytes(sid);raw[36]=1;raw[40]=1;std::uint32_t diagnoses{};
    storage->BindDisplayDiagnostics(client.GameplayGlobals()->AiDebug().AssertionDetail(),[&](auto expression,auto file,auto line) { CHECK(expression=="frameCheck( abstract, frame, false )" && file==R"(..\o\rifttype.h)" && line==0xe1);CHECK(client.GameplayGlobals()->AiDebug().AssertionDetail()=="Frame assertion failed. Asking for type=74 (source) frame=1 abs=0\n\nType has 1 frames\n" && core.spots[40*256+30]==1);++diagnoses;raw[10]=75;raw[36]=0;storage->shapes[74].frames[1].cellWidth=storage->shapes[74].frames[1].cellHeight=5; });
    world.Pop(sid,30,40,0x40);CHECK(diagnoses==1 && checking==0 && raw[10]==75 && raw[36]==0 && raw[33]==3 && core.hash.Bucket(3,30,40)==sid.value && !(raw[11]&4U));
}
