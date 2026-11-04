// 비멀티 공통 Activate의 진단 복귀/부분 효과/깊이와 실제 World의 현재 진단을 검사한다.
#include "TestSupport.h"
#include "o/RawSquidActivate.h"
#include "o/SquidPop.h"
#include "o/SquidPostPop.h"
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
namespace { // 원문 서버 송신/통신/패킷 전용 검사는 실행하지 않는다.
template<class F> bool ActivateFailed(F&& action) { try { action(); } catch (const std::exception&) { return true; }return false; } // 선택한 진단/하위/물리 범위의 명시 실패를 관찰한다.
struct ActivateScene { // 같은 실제 슬롯과 순수 경계 입력을 보관한다.
    SidPool pool{OriginalEdition::Patch1078,24000,false}; // 일반 서버 기능을 사용하지 않는 원문 물리 풀이다.
    Sid source{50}; // 합성 입력의 현재 SID다.
    std::uint32_t depth{2},posts{},flags{}; // 현재 깊이와 가상 후처리 입력 관측이다.
    std::vector<std::uint32_t> lines; // 원문 진단 순서를 보관한다.
    std::function<void(std::uint32_t)> onAssertion; // 외부 진단에서 요청할 현재 상태 변경이다.
    std::function<void()> onPost; // 후처리 경계에서 요청할 변경/명시 실패다.
    SquidActivateHooks hooks; // 현재 비멀티 권한과 관찰 장치다.
    ActivateScene() { // 실제풀에서client슬롯을확보하고정상void입력만준비한다.
        for (std::uint16_t number=5;number<=50;++number) CHECK(pool.Allocate(2)==Sid{number}); // source까지의명시슬롯만확보한다.
        hooks.server=[] { return false; };
        hooks.transmit=[](Sid,std::uint32_t) { throw std::logic_error("비멀티현재권한입력에서송신선택금지"); };
        hooks.postPop=[this](Sid sid,std::uint32_t input) { CHECK(sid==source);++posts;flags=input;const auto selected=onPost;if (selected) selected();--depth; };
        hooks.assertion=[this](auto,auto file,auto line) { CHECK(file=="\\Ns\\O\\Squid.cpp");lines.push_back(line);const auto selected=onAssertion;if (selected) selected(line); };
    }
    std::span<std::uint8_t> Raw() { const auto raw=pool.Slot(source);return {const_cast<std::uint8_t*>(raw.data()),raw.size()}; } // 현재free/예약슬롯도같은물리에서읽고쓴다.
    void Run(std::uint32_t input=0) { RunSquidActivate(pool,source,input,depth,hooks); } // 반환을대신만들지않고실제부모전체를호출한다.
};
}

TEST_CASE(SquidActivate_VoidDiagnosticRepairsCurrentStateBeforeTransmittingCheck) { //35c복귀뒤현재BYTE로void를끄고저장된transmitting을검사한다.
    ActivateScene scene;scene.Raw()[11]=0;
    scene.onAssertion=[&](auto line) { if (line==0x35c) scene.Raw()[11]=0x14;else { CHECK(line==0x35f && scene.Raw()[11]==0x10);scene.Raw()[11]=0; } };
    scene.Run();CHECK((scene.lines==std::vector<std::uint32_t>{0x35c,0x35f}));CHECK(scene.Raw()[11]==0 && scene.depth==2 && scene.posts==1 && scene.flags==0x10);
}

TEST_CASE(SquidActivate_ZeroSidDiagnosticResumesSamePhysicalSlot) { //35b진단복귀가물리0슬롯접근을임의로정상SID로바꾸지않는다.
    ActivateScene scene;scene.source=Sid{};scene.onAssertion=[&](auto line) { CHECK(line==0x35b);scene.Raw()[11]=4; };
    scene.Run();CHECK(scene.lines==std::vector<std::uint32_t>{0x35b} && scene.Raw()[11]==0 && scene.depth==2 && scene.posts==1);
}

TEST_CASE(SquidActivate_InvalidPhysicalSidFailsAfterOriginalDiagnostic) { //원문진단을먼저실행하고그뒤풀밖접근은호스트범위오류로남긴다.
    ActivateScene scene;scene.source=Sid{24000};CHECK(ActivateFailed([&] { scene.Run(); }));
    CHECK(scene.lines==std::vector<std::uint32_t>{0x35b} && scene.posts==0 && scene.depth==2);
}

TEST_CASE(SquidActivate_CurrentDepthDiagnosticReturnsWithoutAutomaticRepair) { //후처리뒤현재깊이를진단하고외부에서요청한값만남긴다.
    ActivateScene scene;scene.onPost=[&] { CHECK(scene.depth==3 && scene.Raw()[11]==0);scene.depth=9; };
    scene.onAssertion=[&](auto line) { CHECK(line==0x392 && scene.depth==8 && scene.posts==1);scene.depth=7; };
    scene.Run(8);CHECK(scene.lines==std::vector<std::uint32_t>{0x392} && scene.depth==7 && scene.flags==8 && scene.Raw()[11]==0);
}

TEST_CASE(SquidActivate_AbortingDepthDiagnosticKeepsCompletedEffects) { //진단예외도이미완료한void해제와후처리/현재깊이를되돌리지않는다.
    ActivateScene scene;scene.onPost=[&] { scene.depth=9;scene.Raw()[40]=0xa5; };
    scene.onAssertion=[&](auto line) { CHECK(line==0x392 && scene.depth==8);throw std::logic_error("명시후단진단중단"); };
    CHECK(ActivateFailed([&] { scene.Run(); }));CHECK(scene.depth==8 && scene.Raw()[11]==0 && scene.Raw()[40]==0xa5 && scene.posts==1);
}

TEST_CASE(SquidActivate_BalancedDepthWrapDoesNotReadDiagnostic) { //inc/공통dec의DWORD감김이균형을이루면진단장치를요구하지않는다.
    ActivateScene scene;scene.depth=std::numeric_limits<std::uint32_t>::max();scene.hooks.assertion={};scene.onPost=[&] { CHECK(scene.depth==0); };
    scene.Run();CHECK(scene.depth==std::numeric_limits<std::uint32_t>::max() && scene.posts==1);
}

TEST_CASE(SquidActivate_PostFailurePreservesIncrementWithoutFinalDiagnostic) { //가상몸체가반환하지않았으면392를미리진단하거나깊이를감소시키지않는다.
    ActivateScene scene;scene.onPost=[] { throw std::logic_error("명시가상몸체실패"); };
    CHECK(ActivateFailed([&] { scene.Run(); }));CHECK(scene.depth==3 && scene.posts==1 && scene.lines.empty() && scene.Raw()[11]==0);
}

TEST_CASE(SquidActivate_DiagnosticSelfRebindReadsNextCurrentCallback) { //첫진단의자기연결교체중수명을보존하고두번째진단은새장치를읽는다.
    ActivateScene scene;scene.Raw()[11]=0;std::vector<std::uint32_t> calls;
    scene.hooks.assertion=[&](auto expression,auto,auto line) { CHECK(expression=="isVoid()" && line==0x35c);calls.push_back(line);scene.Raw()[11]=0x14;scene.hooks.assertion=[&](auto expression,auto,auto line) { CHECK(expression=="!xmitJustCreatedFlag" && line==0x35f && scene.Raw()[11]==0x10);calls.push_back(line);scene.Raw()[11]=0; }; };
    scene.Run();CHECK((calls==std::vector<std::uint32_t>{0x35c,0x35f}) && scene.depth==2 && scene.posts==1);
}

TEST_CASE(SquidActivate_ActualCommonPostPopKeepsStatisticsBeforeCurrentDiagnostic) { //실제AI통지와공통통계/감소뒤의같은깊이를부모에서관찰한다.
    ActivateScene scene;std::vector<RiftTypeRecord> types(188);types[74].cost=3;types[74].footX=types[74].footY=1;auto raw=scene.Raw();raw[10]=74;raw[34]=1;
    for (std::size_t index=0;index<4;++index) raw[index]=static_cast<std::uint8_t>(0x501dd0U>>(index*8)); //실제공통가상표를명시입력한다.
    SquidPostPopState state;state.depth=2;state.localOwner=1;SquidPostPop post(scene.pool,types,state);post.BindPatchAiNotice([&](auto owner,Sid sid,bool made) { CHECK(owner==1 && sid==scene.source && made && state.depth==3);state.depth=9;raw[10]=75; });
    SquidHash hash;std::vector<std::uint8_t> spots(65536);SquidPop pop(scene.pool,hash,spots,nullptr,&post);std::uint32_t diagnoses{};SquidPopDiagnostics diagnostics;diagnostics.types=types;
    diagnostics.assertion=[&](auto expression,auto file,auto line) { CHECK(expression=="curPostPop == basePostPop" && file=="\\Ns\\O\\Squid.cpp" && line==0x392);CHECK(state.depth==8 && state.totalCost==3 && state.globalCounts[75]==1 && state.localCounts[75]==1);++diagnoses;state.depth=2; };pop.BindDiagnostics(std::move(diagnostics));
    CHECK(pop.Pop(scene.source,types[74],1,1,30,40,0x40)==RawPopResult::Registered);CHECK(diagnoses==1 && state.depth==2 && raw[10]==75 && raw[11]==0 && (raw[40]&0x80U));
}

TEST_CASE(SquidActivate_ActualWorldUsesCurrentClientDiagnosticWithCheckingOff) { //실제통지가교체한같은Client장치를후처리반환뒤읽으며checking0에서도진단한다.
    ClientOptions options;options.gameDirectory=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals";options.noAudio=true;Client client(std::move(options));
    std::vector<RiftTypeRecord> types(188);types[74].footX=types[74].footY=1;std::vector<RiftTypeFrames> frames(188,RiftTypeFrames({}));std::vector<SquidDisplayShape> shapes(188);shapes[74].loaded=true;shapes[74].frameCount=1;shapes[74].frames.resize(1);shapes[74].frames[0].cellWidth=shapes[74].frames[0].cellHeight=1;
    auto storage=std::make_shared<RawSurfaceStorage>(OriginalEdition::Patch1078,types,frames,shapes);auto& checking=client.GameplayGlobals()->AiDebug().Checking();checking=0;storage->BindChecking(RawCheckingFlag{checking});
    storage->BindDisplayDiagnostics(client.GameplayGlobals()->AiDebug().AssertionDetail(),[](auto,auto,auto) { throw std::logic_error("교체전진단장치호출금지"); });
    BridgeConnectState links;links.bridgeType=82;links.islandType=94;links.stalagType=95;links.connectorType=155;links.noIslandType=157;RawSurfaceWorld world(OriginalEdition::Patch1078,types,frames,shapes,links,83,nullptr,storage);auto core=world.Core();const auto source=world.Create(74);core.pool.AllocatedBytes(source)[34]=1;core.bookkeeping.depth=2;std::uint32_t diagnoses{};
    core.postPop.BindPatchAiNotice([&](auto,Sid sid,bool) { CHECK(sid==source && core.bookkeeping.depth==3);core.bookkeeping.depth=9;storage->BindDisplayDiagnostics(client.GameplayGlobals()->AiDebug().AssertionDetail(),[&](auto expression,auto file,auto line) { CHECK(expression=="curPostPop == basePostPop" && file=="\\Ns\\O\\Squid.cpp" && line==0x392 && core.bookkeeping.depth==8 && core.bookkeeping.GlobalCounts()[74]==1);++diagnoses;core.bookkeeping.depth=2; }); });
    world.Pop(source,30,40,0x40);CHECK(diagnoses==1 && checking==0 && core.bookkeeping.depth==2 && !(core.pool.Slot(source)[11]&4U));
}
