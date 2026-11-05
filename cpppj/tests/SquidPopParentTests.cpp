// Pop의 부분 변경·현재 읽기·진단 정상 복귀와 실제 World 연결을 창/게임/통신 없이 검사한다.
#include "FormTypeInputs.h" // 공통 ProcessForm 생성이 읽는 원문 내장 생성자 주소를 입력에 명시한다.
#include "TestSupport.h"
#include "o/RawSquidPopParent.h"
#include "o/SquidPop.h"
#include "client/ClientMain.h"
#include "client/ClientGameplayGlobals.h"
#include "client/RawSurfaceWorld.h"
#include <algorithm>
#include <bit>
#include <filesystem>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

using namespace netstorm::o;
using namespace netstorm::client;
namespace { // 명시 입력과 장치 관측만 공급하며 기대 분기/반환을 제품 대신 계산하지 않는다.
// 선택된 미연결 장치/물리 범위가 명시적으로 실패하는지만 관찰한다.
template<class F> bool PopFailed(F&& action) { try { action(); } catch (const std::exception&) { return true; }return false; }
// 현재 슬롯과 진단이 바꿀 원문 저장소를 한 수명 안에 유지한다.
struct PopParentScene {
    SidPool pool{OriginalEdition::Patch1078,24000,false}; // 원본50BYTE 물리 슬롯이며 게임 유닛을 생성하지 않는다.
    SquidHash hash; // 원문과 같은 네 공간 해시다.
    std::vector<std::uint8_t> spots=std::vector<std::uint8_t>(65536); // 부분 OR를 관찰하는 전체 점유 지도다.
    std::vector<RiftTypeRecord> types=::netstorm::test::MakeFormTypeInputs(188); // 현재 TYPE와 처음 선택한 TYPE 참조를 함께 보관한다.
    Sid source{50}; // 명시 입력의 대상 슬롯이다.
    std::int32_t checking{1}; // 원문 관문이 다시 읽는 현재 DWORD다.
    bool metamap{true}; // 기존 원문 fixture처럼 좌표 오류 로그를 기본으로 억제한다.
    std::array<float,2> size{1,1}; // 실제 SHP 경계의 명시 반환 입력이다.
    std::uint32_t firstFlags{1},frameReads{},updates{},firsts{},activations{},activateFlags{},miniGenus{}; // 각 경계의 입력/호출 관측이다.
    std::vector<std::uint32_t> lines; // 정상 복귀한 원문 진단의 순서다.
    std::vector<std::string> files,logs,events; // 진단 파일/완성 로그와 하위 호출 순서를 보관한다.
    std::function<void(std::uint32_t)> onAssertion; // 진단 callback의 명시 현재 입력 변경이다.
    std::function<void()> onMiniMap,onFrame,onUpdate,onActivate,onLog; // 각 외부 경계에서 요청할 명시 변경/실패다.
    SquidPopParentHooks hooks; // 같은 실제 부모에 연결되는 선택 장치다.
    // 슬롯을 확보하고 호출 결과를 만들지 않는 현재 조회/기록 장치만 연결한다.
    PopParentScene() {
        for (std::uint16_t index=5;index<=60;++index) CHECK(pool.Allocate(2)==Sid{index}); // 필요한 합성 슬롯만 실제 풀에서 확보한다.
        types[74].name="base";types[75].name="NOW";types[76].name="old";types[77].name="THEN";
        for (auto& type:types) type.footX=type.footY=1; // 명시한 모든 종류의 기본 발자국 입력이다.
        types[74].flags2=1;auto raw=Raw(source);std::fill(raw.begin(),raw.end(),std::uint8_t{});Put(source,0,0x501dd0);raw[10]=74;raw[11]=4;
        hooks.type=[this](std::uint8_t number)->const RiftTypeRecord& { return types.at(number); };
        hooks.metamap=[this] { return metamap; };hooks.frameSize=[this](Sid sid) { CHECK(sid==source);++frameReads;events.push_back("frame");if (onFrame) onFrame();return size; };
        hooks.trace=[this](auto level,auto text) { CHECK(level==29);logs.emplace_back(text);events.push_back("log");if (onLog) onLog(); };
        hooks.miniMap=[this](Sid sid,auto genus) { CHECK(sid==source);miniGenus=genus;events.push_back("mini");if (onMiniMap) onMiniMap(); };
        hooks.update=[this](Sid sid,auto) { CHECK(sid==source);++updates;events.push_back("update");if (onUpdate) onUpdate(); };
        hooks.firstPop=[this](Sid sid) { CHECK(sid==source);++firsts;events.push_back("first");return firstFlags; }; // 가상 경계의 명시 반환이며 firstPop 계산 자체의 원문 관측은 아니다.
        hooks.activate=[this](Sid sid,auto flags) { CHECK(sid==source);++activations;activateFlags=flags;events.push_back("activate");Raw(source)[11]&=0xfb;if (onActivate) onActivate(); };
        hooks.assertion=[this](auto,auto file,auto line) { lines.push_back(line);files.emplace_back(file);events.push_back("assert");if (onAssertion) onAssertion(line); };
    }
    // 현재 free/dead도 물리 슬롯에만 기록하며 게임 규칙의 성공 결과를 대신 쓰지 않는다.
    std::span<std::uint8_t> Raw(Sid sid) { const auto raw=pool.Slot(sid);return {const_cast<std::uint8_t*>(raw.data()),raw.size()}; }
    // 이웃 필드를 보존하는 명시 WORD/DWORD 입력이다.
    void Put(Sid sid,std::size_t offset,std::uint32_t value,std::size_t width=4) { auto raw=Raw(sid);for (std::size_t index=0;index<width;++index) raw[offset+index]=static_cast<std::uint8_t>(value>>(index*8)); } // 원래 낮은 바이트부터 쓴다.
    // 원문 폭의 현재 정수 필드만 관측한다.
    std::uint32_t Read(Sid sid,std::size_t offset,std::size_t width=4) { const auto raw=Raw(sid);std::uint32_t value{};for (std::size_t index=0;index<width;++index) value|=std::uint32_t(raw[offset+index])<<(index*8);return value; } // 필요한 바이트만 조립한다.
    // 저장된 위치의 float 비트를 추가 반올림 없이 관측한다.
    float Coordinate(std::size_t offset) { return std::bit_cast<float>(Read(source,offset)); }
    // 실제 부모가 선택한 모든 공간 변경과 경계를 실행한다.
    RawPopResult Run(float x=30,float y=40,std::uint32_t flags=0) { return RunSquidPopParent(pool,hash,spots,source,x,y,flags,RawCheckingFlag{checking},hooks); }
};
}

TEST_CASE(SquidPopParent_EarlyReturnsDoNotReadTypeOrDevices) { // 범위/void/contained 앞단은 TYPE/SHP/장치가 없어도 반환한다.
    PopParentScene scene;scene.hooks={};scene.Raw(scene.source)[10]=255;const auto before=std::vector(scene.pool.Bytes().begin(),scene.pool.Bytes().end());
    CHECK(RunSquidPopParent(scene.pool,scene.hash,scene.spots,Sid{},30,40,0,{},scene.hooks)==RawPopResult::Unchanged);
    CHECK(RunSquidPopParent(scene.pool,scene.hash,scene.spots,Sid{24000},30,40,0,{},scene.hooks)==RawPopResult::Unchanged);
    CHECK(std::equal(before.begin(),before.end(),scene.pool.Bytes().begin()));
    for (const auto state:{0U,12U}) { scene.Raw(scene.source)[11]=static_cast<std::uint8_t>(state);const auto raw=std::vector(scene.Raw(scene.source).begin(),scene.Raw(scene.source).end());CHECK(scene.Run()==RawPopResult::Unchanged);CHECK(std::equal(raw.begin(),raw.end(),scene.Raw(scene.source).begin())); } // 두 앞단 상태를 독립 입력으로 확인한다.
}

TEST_CASE(SquidPopParent_InvalidCoordinateLogsBeforeStoreAndUsesCurrentType) { // 로그가 바꾼 현재 TYPE를 좌표 저장 뒤에 조회한다.
    PopParentScene scene;scene.metamap=false;scene.types[75].flags2=1;scene.size={3,1};
    scene.onLog=[&] { CHECK(scene.Coordinate(14)==0 && scene.Coordinate(18)==0);scene.Raw(scene.source)[10]=75;scene.Put(scene.source,14,std::bit_cast<std::uint32_t>(99.0F)); };
    CHECK(scene.Run(-1,7)==RawPopResult::Registered);
    CHECK(scene.logs==(std::vector<std::string>{"Error base popped to(  -1.0,  7.0). Set to( 10,10)\n"}));
    CHECK(scene.Coordinate(14)==10 && scene.Coordinate(18)==10 && scene.spots[10*256+10]==1 && scene.Raw(scene.source)[33]==2);
    CHECK(scene.Read(scene.source,22,2)==160 && scene.Read(scene.source,24,2)==110 && scene.hash.Bucket(2,10,10)==scene.source.value);
}

TEST_CASE(SquidPopParent_MiniMapPreservesInputBoundsAndReadsCurrentLoopType) { // 초기 TYPE의 현재 발자국과 저장 입력 좌표를 쓰며 각 칸의 TYPE/해시는 다시 조회한다.
    PopParentScene scene;scene.types[74].flags2=2;scene.types[75].flags2=1;scene.size={3,2};
    scene.onMiniMap=[&] { CHECK(scene.Coordinate(14)==30 && scene.Coordinate(18)==40);scene.types[74].footX=2;scene.Raw(scene.source)[10]=75;scene.Put(scene.source,14,std::bit_cast<std::uint32_t>(100.0F));scene.Put(scene.source,18,std::bit_cast<std::uint32_t>(110.0F)); };
    scene.onFrame=[&] { CHECK(scene.spots[40*256+29]==1 && scene.spots[40*256+30]==1);CHECK(scene.Raw(scene.source)[10]==75); };
    CHECK(scene.Run()==RawPopResult::Registered && scene.miniGenus==2);
    CHECK(scene.hash.Bucket(2,100,110)==scene.source.value && scene.hash.Bucket(2,30,40)==0 && scene.spots[110*256+100]==0);
    CHECK(scene.events==(std::vector<std::string>{"mini","frame","update","first","activate"}));
}

TEST_CASE(SquidPopParent_InvalidStartDiagnosesWithCheckingOffAndResumesCellZero) { // 원문6b7은 checking과 무관하며 물리적으로 유효한0번 칸은 계속 처리한다.
    PopParentScene scene;scene.checking=0;
    CHECK(scene.Run(0.25F,20.5F)==RawPopResult::Registered);
    CHECK(scene.lines==std::vector<std::uint32_t>{0x6b7} && scene.files.front()=="\\Ns\\O\\Squid.cpp" && scene.spots[20*256]==1);
    CHECK(scene.Coordinate(14)==0.25F && scene.Coordinate(18)==20.5F && scene.frameReads==1);
}

TEST_CASE(SquidPopParent_GridDiagnosticKeepsCandidateAndLogsCurrentNames) { // 진단 뒤 현재 TYPE/이름과 보관한 후보를 사용하고 다음 칸은 현재 genus를 다시 읽는다.
    PopParentScene scene;scene.types[74].flags2=0;scene.types[74].flags1=0x800;scene.types[74].footX=2;scene.types[75].flags2=1;
    scene.Raw(Sid{9})[10]=76;scene.hash.Cell(0,29,40)=9;
    scene.onAssertion=[&](auto line) { CHECK(line==0x6e8);scene.checking=0;scene.Raw(scene.source)[10]=75;scene.Raw(Sid{9})[10]=77;scene.hash.Cell(0,29,40)=0;scene.Put(scene.source,14,std::bit_cast<std::uint32_t>(60.0F));scene.Put(scene.source,18,std::bit_cast<std::uint32_t>(80.0F)); };
    CHECK(scene.Run()==RawPopResult::Registered);
    CHECK(scene.lines==std::vector<std::uint32_t>{0x6e8} && scene.logs==(std::vector<std::string>{"ERROR: gridSid 50( NOW) replacing 9( THEN)\n"}));
    CHECK(scene.spots[40*256+29]==0 && scene.spots[40*256+30]==1 && scene.hash.Bucket(1,60,80)==50);
}

TEST_CASE(SquidPopParent_MissingSideDeviceKeepsEarlierSpots) { // 도달한 옆면 장치 실패 앞의 좌표와 y/x 순서 OR는 남긴다.
    PopParentScene scene;scene.types[74].flags2=0x4001;scene.types[74].footX=scene.types[74].footY=3;scene.Raw(Sid{9})[10]=75;scene.types[75].flags1=0x800;scene.hash.Cell(0,28,39)=9;
    CHECK(PopFailed([&] { scene.Run(); }));
    CHECK(scene.Coordinate(14)==30 && scene.Coordinate(18)==40 && scene.Raw(scene.source)[11]==4 && scene.frameReads==0);
    CHECK(scene.spots[38*256+28]==1 && scene.spots[38*256+29]==1 && scene.spots[38*256+30]==1 && scene.spots[39*256+28]==1);
    CHECK(std::count_if(scene.spots.begin(),scene.spots.end(),[](auto value) { return value!=0; })==4 && scene.hash.Bucket(1,30,40)==0);
}

TEST_CASE(SquidPopParent_FrameFailureOccursAfterSpotsBeforeHash) { // SHP 조회가 실패해도 그 앞에서 실제로 실행한 부분 공간 변경은 보존한다.
    PopParentScene scene;scene.types[74].footX=2;scene.onFrame=[] { throw std::logic_error("명시 SHP 실패"); };
    CHECK(PopFailed([&] { scene.Run(); }));
    CHECK(scene.frameReads==1 && scene.spots[40*256+29]==1 && scene.spots[40*256+30]==1 && scene.Coordinate(14)==30);
    CHECK(scene.hash.Bucket(1,30,40)==0 && scene.Raw(scene.source)[11]==4 && scene.Raw(scene.source)[40]==0 && scene.updates==0);
}

TEST_CASE(SquidPopParent_OverlapReturnsBeforeUnknownVirtualAndFrame) { // 겹침은 미지원 가상 표/그림을 조회하기 전에 정상 반환한다.
    PopParentScene scene;scene.Put(scene.source,0,0x12345678);scene.spots[40*256+30]=1;SquidPop pop(scene.pool,scene.hash,scene.spots);
    SquidPopDiagnostics diagnostics;diagnostics.types=scene.types;diagnostics.frameSize=[](Sid)->std::array<float,2> { throw std::logic_error("겹침 뒤 SHP 조회 금지"); };pop.BindDiagnostics(std::move(diagnostics));
    CHECK(pop.PopCurrent(scene.source,30,40)==RawPopResult::Overlap);
    CHECK(scene.Coordinate(14)==30 && scene.Coordinate(18)==40 && scene.Raw(scene.source)[11]==4 && scene.hash.Bucket(1,30,40)==0);
}

TEST_CASE(SquidPopParent_NextDiagnosticKeepsSavedHeadAndCurrentIslandBranch) { // 진단이 head/좌표/단계를 바꿔도 선택 버킷과 next는 보관하며 island 분기는 현재 단계다.
    PopParentScene scene;scene.hash.Bucket(1,30,40)=50;scene.Put(scene.source,8,41,2);scene.types[75].flags2=2;
    scene.onAssertion=[&](auto line) { CHECK(line==0x123 && scene.files.back()==R"(c:\ns\o\squid.h)");scene.hash.Bucket(1,30,40)=9;scene.Raw(scene.source)[10]=75;scene.Raw(scene.source)[33]=0;scene.Put(scene.source,14,std::bit_cast<std::uint32_t>(90.0F));scene.Put(scene.source,18,std::bit_cast<std::uint32_t>(80.0F)); };
    CHECK(scene.Run()==RawPopResult::Registered);
    CHECK(scene.lines==std::vector<std::uint32_t>{0x123} && scene.Read(scene.source,4,2)==50 && scene.Read(scene.source,8,2)==127);
    CHECK(scene.hash.Bucket(1,30,40)==50 && scene.hash.Bucket(1,90,80)==0 && scene.Raw(scene.source)[33]==0);
}

TEST_CASE(SquidPopParent_UpdateControlsCurrentFirstPopAndFlags) { // 표시 뒤 현재 extra를 읽어 firstPop 생략/실행을 판단한다.
    for (const bool skip:{false,true}) { // 초기 extra와 표시 뒤 extra를 반대로 두어 처음 snapshot 사용을 검출한다.
        PopParentScene scene;scene.Raw(scene.source)[40]=skip ? 0 : 0x80;scene.firstFlags=2;
        scene.onUpdate=[&] { CHECK(scene.hash.Bucket(1,30,40)==50 && (scene.Raw(scene.source)[11]&4U));scene.Raw(scene.source)[40]=skip ? 0x80 : 1; };
        CHECK(scene.Run(30,40,0x2000)==RawPopResult::Registered);
        CHECK(scene.firsts==(skip ? 0U : 1U) && scene.activateFlags==(skip ? 0x2000U : 0x2002U) && scene.Raw(scene.source)[40]==(skip ? 0x80U : 0x81U));
    }
}

TEST_CASE(SquidPopParent_FinalContainedDiagnosticKeepsCompletedEffects) { // Activate 뒤71a는 checking0에서도 진단하며 정상 복귀/중단 모두 앞선 변경을 유지한다.
    for (const bool abort:{false,true}) { // 같은 원문 진단에서 외부 장치의 정상 반환과 예외를 구별한다.
        PopParentScene scene;scene.checking=0;scene.onActivate=[&] { scene.Raw(scene.source)[11]|=8;scene.Raw(scene.source)[10]=75; };
        scene.onAssertion=[&](auto line) { CHECK(line==0x71a && scene.activations==1 && scene.Raw(scene.source)[40]==0x80);if (abort) throw std::logic_error("명시 후단 진단 종료"); };
        if (abort) CHECK(PopFailed([&] { scene.Run(); }));else CHECK(scene.Run()==RawPopResult::Registered);
        CHECK(scene.lines==std::vector<std::uint32_t>{0x71a} && scene.hash.Bucket(1,30,40)==50 && scene.Raw(scene.source)[11]==8 && scene.Raw(scene.source)[10]==75);
    }
}

TEST_CASE(SquidPopParent_ActualAdapterTransmittingDiagnosticRunsAfterVoidClear) { // 기존 front 거부 대신 실제35f 진단이 부분 등록/void 해제 뒤에 실행된다.
    PopParentScene scene;scene.Raw(scene.source)[11]=0x14;SquidPop pop(scene.pool,scene.hash,scene.spots);SquidPopDiagnostics diagnostics;diagnostics.types=scene.types;
    diagnostics.frameSize=[](Sid) { return std::array<float,2>{1,1}; };diagnostics.assertion=[&](auto expression,auto file,auto line) { CHECK(expression=="!xmitJustCreatedFlag" && file=="\\Ns\\O\\Squid.cpp" && line==0x35f);CHECK(scene.Raw(scene.source)[11]==0x10 && scene.hash.Bucket(1,30,40)==50);scene.lines.push_back(line);scene.Raw(scene.source)[11]=0; };pop.BindDiagnostics(std::move(diagnostics));
    CHECK(pop.PopCurrent(scene.source,30,40)==RawPopResult::Registered);
    CHECK(scene.lines==std::vector<std::uint32_t>{0x35f} && scene.Raw(scene.source)[11]==0 && scene.Raw(scene.source)[40]==0x80);
}

TEST_CASE(SquidPopParent_ActualWorldFrameFailurePreservesCoordinatesAndSpots) { // 실제 World의 사전 SHP 조회를 제거해 같은 부모의 부분 변경을 확인한다.
    std::vector<RiftTypeRecord> types=::netstorm::test::MakeFormTypeInputs(188);types[74].flags2=1;types[74].footX=types[74].footY=1;
    std::vector<RiftTypeFrames> frames(188,RiftTypeFrames({}));std::vector<SquidDisplayShape> shapes(188);shapes[74].loaded=true;shapes[74].frameCount=1;shapes[74].frames.resize(1);
    BridgeConnectState links;links.bridgeType=82;links.islandType=94;links.stalagType=95;links.connectorType=155;links.noIslandType=157;
    RawSurfaceWorld world(OriginalEdition::Patch1078,types,frames,shapes,links,83);auto core=world.Core();const auto source=world.Create(74);auto raw=core.pool.AllocatedBytes(source);
    for (std::size_t index=0;index<4;++index) raw[36+index]=0xff; // 실제 SHP 범위 밖 현재 프레임을 명시 입력한다.
    CHECK(PopFailed([&] { world.Pop(source,30,40,0x40); }));
    CHECK(std::bit_cast<float>(std::uint32_t(raw[14])|(std::uint32_t(raw[15])<<8)|(std::uint32_t(raw[16])<<16)|(std::uint32_t(raw[17])<<24))==30);
    CHECK(core.spots[40*256+30]==1 && raw[11]==4 && !(raw[40]&0x80U) && core.hash.Bucket(1,30,40)==0);
}

TEST_CASE(SquidPopParent_ActualClientWorldDiagnosticAndTraceShareCurrentOwner) { // 실제 Client checking과 World 진단/수준29 로그가 같은 현재 상태를 사용한다.
    ClientOptions options;options.gameDirectory=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals";options.noAudio=true;Client client(std::move(options));
    std::vector<RiftTypeRecord> types=::netstorm::test::MakeFormTypeInputs(188);types[74].name="source";types[74].flags1=0x800;types[74].footX=types[74].footY=1;types[75].name="surface";
    std::vector<RiftTypeFrames> frames(188,RiftTypeFrames({}));std::vector<SquidDisplayShape> shapes(188);shapes[74].loaded=true;shapes[74].frameCount=1;shapes[74].frames.resize(1);shapes[74].frames[0].cellWidth=shapes[74].frames[0].cellHeight=1;
    auto storage=std::make_shared<RawSurfaceStorage>(OriginalEdition::Patch1078,types,frames,shapes);auto& checking=client.GameplayGlobals()->AiDebug().Checking();checking=1;storage->BindChecking(RawCheckingFlag{checking});
    std::uint32_t assertions{};std::vector<std::string> logs;
    storage->BindDisplayDiagnostics(client.GameplayGlobals()->AiDebug().AssertionDetail(),[&](auto expression,auto file,auto line) { CHECK(expression=="false" && file=="\\Ns\\O\\Squid.cpp" && line==0x6e8);++assertions;checking=0; });
    storage->BindGraphTrace([&](auto level,auto text) { CHECK(level==29);logs.emplace_back(text); });
    BridgeConnectState links;links.bridgeType=82;links.islandType=94;links.stalagType=95;links.connectorType=155;links.noIslandType=157;
    RawSurfaceWorld world(OriginalEdition::Patch1078,types,frames,shapes,links,83,nullptr,storage);auto core=world.Core();const auto source=world.Create(74);auto candidate=core.pool.Slot(Sid{9});const_cast<std::uint8_t*>(candidate.data())[10]=75;core.hash.Cell(0,30,40)=9;
    world.Pop(source,30,40,0x40);
    CHECK(assertions==1 && checking==0 && logs==(std::vector<std::string>{"ERROR: gridSid 15000( source) replacing 9( surface)\n"}));
    CHECK(core.hash.Bucket(1,30,40)==source.value && !(core.pool.Slot(source)[11]&4U));
}

TEST_CASE(PopWorldBounds_ActualParentClampsUnorderedAndRechecksLimitAfterMiniMap) { // 처음 비교의float저장과 미니맵 뒤 시작점의 현재 크기를 같은 부모에서 확인한다.
    PopParentScene clamped;std::int32_t limit=32;std::uint32_t reads=0;clamped.hooks.worldCells=[&] { ++reads;return limit; };CHECK(clamped.Run(40,20)==RawPopResult::Registered);CHECK(clamped.Coordinate(14)==10 && clamped.Coordinate(18)==10 && reads==2 && clamped.lines.empty()); // 입구/시작점은 각각 한 번 읽는다.
    PopParentScene unordered;std::uint32_t nanReads=0;unordered.hooks.worldCells=[&] { ++nanReads;return 32; };CHECK(unordered.Run(std::numeric_limits<float>::quiet_NaN(),20)==RawPopResult::Registered);CHECK(unordered.Coordinate(14)==10 && unordered.Coordinate(18)==10 && nanReads==1 && unordered.lines.empty()); // NaN x는 입구의 크기를 읽지 않고 시작점만 읽는다.
    PopParentScene current;current.types[74].flags2=2;limit=64;reads=0;current.hooks.worldCells=[&] { ++reads;return limit; };current.onMiniMap=[&] { limit=16; };CHECK(current.Run(40,20)==RawPopResult::Registered);CHECK(current.Coordinate(14)==40 && current.Coordinate(18)==20 && reads==2 && current.lines==std::vector<std::uint32_t>{0x6b7} && current.spots[20*256+40]==2 && current.activations==1); // 현재 시작점 진단 뒤에도 실제 물리 지도/원래 저장 범위로 계속한다.
}
