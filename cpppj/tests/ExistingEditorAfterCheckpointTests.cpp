// PE 정적 순서에 근거한 적용후 제안 검사다. 전체 Editor 설치/GUI 성공을 대신하지 않는다.
#include "TestSupport.h"
#include "client/ClientMain.h"
#include "client/ClientLocalStateGlobals.h"
#include "client/ExistingEditorAfterCheckpoint.h"
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

using namespace netstorm::client;
namespace {
// 다섯 실제 블록의 stale140BYTE까지 구별하고 원문 State 몸체에 현재 외부 장치만 공급한다.
struct TailStateScene {
    ClientLocalStateGlobals owner;InputQueue input; //같은 블록/기한과 실제40칸 고리이며 Session 사본을 만들지 않는다.
    bool server{},boss{true},battle{},post{true},saved{},mission{},simul{},meta{}; //선택 Editor 부모의 실제 모드 입력이다.
    netstorm::o::PlayerWorldState player; //같은 현재Player DWORD를 State가 빌린다.
    std::uint32_t zone{},ticks{},source{},seed{},metaRefresh{},metaRebuild{}; //도달하지 않는 다른 상태 입력도 별개로 둔다.
    std::uint8_t title{},battleByte{};double connectEnd{},gameSeconds{},wallCached{100.5}; //기한과 독립인 현재 시계/인접 입력이다.
    std::vector<std::string> trace;std::vector<std::uint32_t> states; //원문 경계의 순서와 State 진입만 관측한다.
    ExistingEditorAfterCheckpointEffects effects;std::unique_ptr<netstorm::o::RawLocalMissionState> body; //위 실제 저장소보다 먼저 해제되는 처리기다.
    TailStateScene() { //모든 기대는 아래 명시 입력/PE 순서에서 만들며 제품 결과를 기대 생성기로 사용하지 않는다.
        auto& blocks=owner.Blocks(); //같은720BYTE의 서로 다른 블록이다.
        for (std::size_t block=0;block<blocks.size();++block) { //블록별 stale DWORD를 전부 구별한다.
            for (std::size_t word=0;word<blocks[block].size();++word) blocks[block][word]=static_cast<std::uint32_t>(block*1000+word); //상태 외140BYTE도 비교한다.
        }
        blocks[1][0]=99;blocks[2][0]=0;blocks[3][0]=1;blocks[4][0]=0;
        effects.hud=[this](auto command) { CHECK(command==0x0200003dU && owner.Blocks()[1][0]==99);trace.emplace_back("hud"); };
        effects.wallSeconds=[this] { trace.emplace_back("wall");return 100.5; };
        effects.requestMusic=[this](auto name) { CHECK(name=="ser22.mus");trace.emplace_back("music"); };
        effects.fortPalette=[this] { trace.emplace_back("palette"); };
        effects.islandCount=[this] { trace.emplace_back("islands");return 0U; };
        netstorm::o::LocalMissionStateHooks hooks; //State 블록 복사/즉시반복은 실제 원문 모듈을 사용한다.
        hooks.logState=[this](auto state) { states.push_back(state); };
        hooks.invalidState=[](auto) { throw std::logic_error("제안 State4가 invalid로 분배됨"); };
        hooks.assertion=[] { throw std::logic_error("제안의 예상밖 immediate 중복"); };
        hooks.parentState=[this](auto selected) { CHECK(selected==1 || selected==2);ApplyExistingEditorAfterCheckpoint(owner,input,effects);return netstorm::o::LocalMissionParentResult::Commit; };
        const auto storage=owner.State(); //menuWindow도 같은 immediate+84 DWORD를 전달한다.
        body=std::make_unique<netstorm::o::RawLocalMissionState>(netstorm::o::OriginalEdition::Patch1078,storage,
            netstorm::o::LocalMissionStateGlobals{server,boss,battle,post,saved,mission,simul,meta,player,zone,ticks,source,seed,blocks[2][33],metaRefresh,metaRebuild,title,battleByte,connectEnd,owner.ActivityEnd(),gameSeconds,wallCached},std::move(hooks));
    }
};
// 부분 효과 검사에서 예외 종류를 숨기지 않고 실패 존재만 확인하는 경계다.
template<class Function> bool TailRejects(Function&& function) { try { function();return false; } catch (const std::logic_error&) { return true; } }
}

// 현재1을 확정한 정상ret와 다음Poll의4 확정을 분리하고 다섯 블록 전체를 비교한다.
TEST_CASE(ExistingEditorAfterCheckpoint_FullBlocksCommitThenNextPollFour) {
    TailStateScene scene;const auto entered=scene.owner.Blocks()[3],previous=scene.owner.Blocks()[1]; //실행 전 독립 입력을 고정한다.
    scene.body->Run();auto expected=entered;expected[0]=0;
    CHECK(scene.owner.Blocks()[0]==previous && scene.owner.Blocks()[1]==entered && scene.owner.Blocks()[2]==expected && scene.owner.Blocks()[3]==expected);
    expected[0]=4;CHECK(scene.owner.Blocks()[4]==expected && scene.owner.ActivityEnd()==110.5);
    CHECK(scene.states==std::vector<std::uint32_t>{1} && scene.trace==std::vector<std::string>({"hud","wall","music","palette","islands"}));
    const auto reserved=scene.input.Pop(false);CHECK(reserved.code==0x02000022U && reserved.x==0 && reserved.y==0 && scene.input.Empty()); //창/섬 생성 대신 일반 입력 예약만 관측한다.
    scene.body->Poll();CHECK(scene.states==std::vector<std::uint32_t>({1,4})); //동기 next4 drain을 했다면 앞선 상태 검사부터 실패한다.
    CHECK(scene.owner.Blocks()[0]==entered && scene.owner.Blocks()[1]==expected);
    expected[0]=0;CHECK(scene.owner.Blocks()[2]==expected && scene.owner.Blocks()[3]==expected && scene.owner.Blocks()[4]==expected);
}

// 음악/palette/섬 getter가 바꾼 현재값을 각 원문 위치에서 다시 읽는다.
TEST_CASE(ExistingEditorAfterCheckpoint_LiveRequestAndCommandReads) {
    {
        TailStateScene scene;scene.effects.fortPalette=[&] { scene.owner.Blocks()[3][0]=3; };
        scene.effects.islandCount=[]()->std::uint32_t { throw std::logic_error("현재3에서 IslandList를 읽음"); };
        ApplyExistingEditorAfterCheckpoint(scene.owner,scene.input,scene.effects);CHECK(scene.owner.Blocks()[4][0]==4 && scene.input.Empty());
    }
    {
        TailStateScene scene;scene.effects.islandCount=[&] { scene.owner.AddIslandCommand()=0xa3010123U;return 0U; }; //count 복귀 뒤의 현재 명령 DWORD를 읽어야 한다.
        ApplyExistingEditorAfterCheckpoint(scene.owner,scene.input,scene.effects);const auto reserved=scene.input.Pop(false);CHECK(reserved.code==0x03000123U && reserved.x==0 && reserved.y==0);
    }
    {
        TailStateScene scene;scene.effects.requestMusic=[&](auto) { scene.owner.Blocks()[3][0]=6; };scene.effects.battleOver=[] {};
        CHECK(TailRejects([&] { ApplyExistingEditorAfterCheckpoint(scene.owner,scene.input,scene.effects); }));
        CHECK(scene.owner.ActivityEnd()==110.5 && scene.owner.Blocks()[4][0]==0 && scene.input.Empty()); //BattleOver를 정상 Editor 완료로 포장하지 않는다.
    }
}

// 실제 immediate14는 같은Run에서 처리하지만 next4는 별개이며 누락장치 예외는 앞선 효과/이전phase를 남긴다.
TEST_CASE(ExistingEditorAfterCheckpoint_ImmediateAndPartialFailureStayDistinct) {
    {
        TailStateScene scene;scene.effects.fortPalette=[&] { scene.body->Request(14); };
        scene.body->Run();CHECK(scene.states==std::vector<std::uint32_t>({1,14}) && scene.owner.Blocks()[1][0]==14 && scene.owner.Blocks()[4][0]==0);
        CHECK(!scene.input.Empty()); //실제 immediate가 다른 상태까지 실행한 경우 Editor loaded 확정 관문은 이결과를 별도 판단해야 한다.
    }
    {
        TailStateScene scene;const auto old=scene.owner.Blocks()[0],phase=scene.owner.Blocks()[1];scene.effects.fortPalette={};
        CHECK(TailRejects([&] { scene.body->Run(); }));
        CHECK(scene.trace==std::vector<std::string>({"hud","wall","music"}) && scene.owner.ActivityEnd()==110.5);
        CHECK(scene.owner.Blocks()[0]==old && scene.owner.Blocks()[1]==phase && scene.owner.Blocks()[3][0]==1 && scene.owner.Blocks()[4][0]==0 && scene.input.Empty());
    }
}

// 실제 Run 없는 Client getter/설정변경/반납과 다음 새Client의 BSS0을 구별한다.
TEST_CASE(ExistingEditorAfterCheckpoint_ActualClientOwnsBlocksAndDeadline) {
    static_assert(std::is_same_v<decltype(std::declval<const Client&>().LocalStateGlobals()),const std::shared_ptr<ClientLocalStateGlobals>&>); //실제소유슬롯 provider만 사용한다.
    ClientOptions options;options.gameDirectory=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"originals"; //보호 archive 읽기만 하고 Run/창을 실행하지 않는다.
    std::shared_ptr<ClientLocalStateGlobals> previous;std::weak_ptr<ClientLocalStateGlobals> life; //Client 소멸 뒤 빌린 소비자의 공유 수명을 검사한다.
    {
        Client client(options);previous=client.LocalStateGlobals();life=previous;auto state=previous->State(); //같은 Client 실제 소유자다.
        CHECK(&state.current==&previous->Blocks()[3] && &state.immediate[33]==&previous->Blocks()[2][33]);
        state.current[5]=0x98765432U;previous->ActivityEnd()=123.75;
        client.Configuration().Configuration().AppendSection("END");client.Configuration().SetInt("readOnlyForts",1);
        CHECK(client.LocalStateGlobals().get()==previous.get() && previous->Blocks()[3][5]==0x98765432U && previous->ActivityEnd()==123.75);
    }
    CHECK(!life.expired() && previous->ActivityEnd()==123.75);
    {
        Client fresh(options);CHECK(fresh.LocalStateGlobals().get()!=previous.get() && fresh.LocalStateGlobals()->ActivityEnd()==0.0);
        CHECK((fresh.LocalStateGlobals()->Blocks()==std::array<netstorm::o::LocalMissionStateBlock,5>{}));
    }
    previous.reset();CHECK(life.expired()); //최종 반납 뒤에는 탈출한 참조를 역참조하지 않는다.
}
