// 원본 상태 블록 전체와 현재 전역/Template/효과 순서를 독립 기계어 기록에 대조한다.
#include "RawSceneSupport.h"
#include "o/RawLocalMissionState.h"
#include <array>
#include <cstdio>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
constexpr std::uint32_t kZone=0x12500000; // 관찰기의 Zone opaque 주소이며 호스트 포인터로 사용하지 않는다.
// 원본 선택 입력만 준비하며 상태 전환/단계 완료를 기대값에서 역산하지 않는다.
struct StateScene {
    std::array<LocalMissionStateBlock,5> blocks{}; // old/phase/immediate/current/next의 서로 다른 실제 저장소다.
    bool server{},boss{true},battle{},post{true},saved{true},mission{true},simul{},meta{}; // 원본의 서로 다른 현재 모드다.
    PlayerWorldState player; // 현재 known과 서로 다른 두 owner를 공유한다.
    std::uint32_t zonePending{},ticks{42},source{17},seed{88},metaRefresh{},metaRebuild{}; // 원본 외부 전역의 명시 입력이다.
    std::uint8_t title{90},battleByte{7}; // 서로 다른 원본 BYTE 전역이다.
    double end{120.0},activity{77.0},seconds{45.75},wall{100.5}; // 연결/활동 예약과 게임/벽시각의 독립 입력이다.
    std::array<std::uint8_t,192> cores{}; // 현재 Template 세 개의 64BYTE 입력을 보존한다.
    std::array<std::uint8_t,20> address{}; // 실제 localAddress 외부 입력이다.
    std::uint32_t variant{},mutation{}; // native가 공급한 선택 경로/효과 변경 종류다.
    std::string trace; // 실제 경계 요청 순서만 직렬화한다.
    std::unique_ptr<RawLocalMissionState> body; // 위 저장소의 고정 주소를 빌린다.
    // 초기 블록의 모든 stale DWORD와 현재 외부 입력을 native와 동일하게 준비한다.
    StateScene(std::string_view kind,std::uint32_t selected,std::uint32_t flags,std::uint32_t change):variant(flags),mutation(change) {
        // 원문 다섯 블록의 인접 필드를 서로 구별한다.
        for (std::size_t block=0;block<blocks.size();++block) {
            // 상태 번호 이외의 stale DWORD도 출력 전체 비교에 포함한다.
            for (std::size_t index=0;index<blocks[block].size();++index) blocks[block][index]=static_cast<std::uint32_t>(block*1000+index);
        }
        auto& input=blocks[kind=="Poll" ? 4 : 3];input[0]=selected;input[32]=kZone;input[33]=(variant&2U)!=0;input[34]=1250;
        if (kind=="Poll") blocks[3][0]=0;
        if (kind=="Request") blocks[2][0]=(variant&1U) ? 7U : 0U;
        server=(variant&1U)!=0;zonePending=(variant&4U)!=0;meta=(variant&256U)!=0;metaRebuild=meta ? 1U : 0U;
        player.playerKnown=9;end=(variant&1024U) ? 0.0 : (variant&32U) ? 90.0 : 120.0;
        // 관찰기가 입력한 물리 Core 전체를 사용하며 수정 결과를 미리 공급하지 않는다.
        for (std::size_t index=0;index<cores.size();++index) cores[index]=static_cast<std::uint8_t>(index*3+5);
        // 실제 주소 기록의 사용하지 않는 인접 BYTE도 보존한다.
        for (std::size_t index=0;index<address.size();++index) address[index]=static_cast<std::uint8_t>(index);
        Put(address,0,(variant&128U) ? 6U : 2U,2);address[2]=0x12;address[3]=0x34;address[12]=0x56;address[13]=0x78;
        body=std::make_unique<RawLocalMissionState>(OriginalEdition::Patch1078,Storage(),Globals(),Hooks());
    }
    // 서로 다른 원본 다섯 영역을 같은 주소와 순서로 전달한다.
    LocalMissionStateStorage Storage() { return {blocks[0],blocks[1],blocks[2],blocks[3],blocks[4]}; }
    // 상태 배열+84와 메뉴 창 DWORD는 같은 실제 영역을 사용한다.
    LocalMissionStateGlobals Globals() { return {server,boss,battle,post,saved,mission,simul,meta,player,zonePending,ticks,source,seed,blocks[2][33],metaRefresh,metaRebuild,title,battleByte,end,activity,seconds,wall}; }
    // 출력 trace는 경계 진입 시 추가하며 상태 효과를 대신 계산하지 않는다.
    void Event(std::string value) { if (!trace.empty()) trace+=';';trace+=value; }
    // 큰 UI/파일/전송은 native와 같은 명시 외부 입력이고 작은 상태 계산은 실제 제품이 수행한다.
    LocalMissionStateHooks Hooks() {
        LocalMissionStateHooks h;
        h.logState=[this](std::uint32_t state) { Event("State,"+std::to_string(state));if (mutation==4) blocks[3][0]=8; };
        h.assertion=[this] { Event("Assert"); };
        h.invalidState=[](std::uint32_t) { throw std::logic_error("선택 native 입력 밖의 상태 진단"); };
        h.clearSelection=[this] { Event("Selection"); };
        h.cleanupBattle=[this](std::uint32_t reason) { Event("Cleanup,"+std::to_string(reason));server=false;boss=false;post=true;zonePending=0;saved=false;ticks=0;source=0;if (mutation==7) blocks[3][33]=0; };
        h.cdPresent=[this] { Event("Seed");return 123U; }; // 기존 동결 trace의 Seed는 CD 호출 경계의 옛 표식이다. 난수 몸체를 실행하지 않는다.
        h.refreshCommands=[this] { Event("Commands"); };h.stopMusic=[this] { Event("Music"); };h.refreshMission=[this] { Event("Mission"); };
        h.installZone=[this](std::uint32_t zone) { Event("Zone,"+std::to_string(zone));server=true;boss=true;if (mutation==2) blocks[3][32]=kZone+16; };
        h.freeZone=[this](std::uint32_t zone) { Event("Free,"+std::to_string(zone)); };h.installMapHandlers=[this] { Event("MapHandlers"); };
        h.initializeRenderer=[this] { Event("Cursor"); };h.uiMode=[this](std::uint32_t mode) { Event("UI,"+std::to_string(mode)); }; // 기존 Cursor 표식도 실제4993c0의 명시 경계이며 새 해석은 Renderer다.
        h.wallSeconds=[this] { Event("Wall");if (mutation==1) end=99.0;return wall; };
        h.loadReconnectFort=[this] { Event("ReconnectFort"); };
        h.progress=[this](LocalMissionProgress progress,std::int32_t remaining) {
            constexpr std::array<std::string_view,5> names{"MissionStarting","ReconnectStarting","ConnectingCountdown","ReconnectCountdown","ServerLost"}; // 실제 원문 문구 선택의 관찰 이름이다.
            Event("Progress,"+std::string(names.at(static_cast<std::size_t>(progress)))+','+std::to_string(remaining));
        };
        h.battlePalette=[this] { Event("Palette");return 77U; };h.applyPalette=[this](std::uint32_t value) { Event("ApplyPalette,"+std::to_string(value)); };
        h.connectServer=[this](std::span<const std::uint8_t> raw,std::uint32_t value) { Event("Connect,"+Hex(raw)+','+std::to_string(value)); };
        h.connectionStatus=[this](std::int32_t target) { Event("Status,"+std::to_string(target));if (mutation==5) server=true;return target==-1 ? ((variant&512U) ? 1 : 0) : (variant&2048U) ? -1 : (variant&8U) ? 0 : 1; };
        h.connectionOpen=[this](std::int32_t target) { Event("Open,"+std::to_string(target));return !(variant&16U); };
        h.closeConnection=[this](std::int32_t target) { Event("Close,"+std::to_string(target)); };h.renameConnection=[this](std::int32_t from,std::int32_t to) { Event("Rename,"+std::to_string(from)+','+std::to_string(to)); };
        h.logConnection=[this](std::int32_t line,std::int32_t value) { Event("ConnLog,"+std::to_string(line)+','+std::to_string(value)); };
        h.sendLogin=[this] { Event("SendLogin"); };h.connectedProgress=[this] { Event("ConnectedProgress"); };h.closeProgress=[this] { Event("CloseProgress"); };
        h.serverConnection=[this] { Event("ServerConnection");return 55U; };h.attachCommander=[this](std::uint32_t token) { Event("Commander,"+std::to_string(token)); };
        h.fortAvailable=[this](std::uint32_t) { return !(variant&64U); };
        h.loadCurrentFort=[this] { Event("FortName");Event("LoadFort,"+std::to_string(player.templateOwner)); };
        h.fortCore=[this](std::uint32_t owner) { Event("Core,"+std::to_string(owner));return std::span<std::uint8_t>(cores).subspan(owner*64,64); };
        h.localAddress=[this] { Event("Address");if (mutation==3) player.templateOwner=2;return address; };
        h.touchFort=[this](std::uint32_t owner) { Event("Touch,"+std::to_string(owner)); };h.refreshFort=[this] { Event("RefreshFort"); };
        h.setReconnectSp=[this](float money) { Event("Sp,"+std::to_string(std::bit_cast<std::uint32_t>(money))); };
        h.loginTimer=[this](bool start) { Event("Timer,"+std::to_string(start ? 1 : 0)); };h.inputMode=[this](bool value) { Event("Input,"+std::to_string(value ? 1 : 0)); };
        h.metaFirst=[this] { Event("MetaFirst"); };h.metaSecond=[this] { Event("MetaSecond"); };h.metaThird=[this] { Event("MetaThird"); };
        return h;
    }
    // 실제 다섯 블록의 모든720BYTE를 원문 순서로 직렬화한다.
    std::string BlockBytes() const { std::string result;for (const auto& block:blocks) result+=Hex(std::bit_cast<std::array<std::uint8_t,144>>(block));return result; }
    // 같은 현재 외부 전역을 native 기록 순서대로 읽는다.
    std::string GlobalBytes() const {
        const std::array<std::uint32_t,15> values{server ? 1U : 0U,boss ? 1U : 0U,battle ? 1U : 0U,post ? 1U : 0U,saved ? 1U : 0U,player.playerKnown,player.battleOwner,player.templateOwner,zonePending,ticks,source,seed,metaRefresh,metaRebuild,title}; // 원본14DWORD와 마지막 BYTE다.
        std::string result;for (const auto value:values) { if (!result.empty()) result+=',';result+=std::to_string(value); }return result;
    }
    // 독립 두 타이머의 double 비트 전체다.
    std::string TimerBytes() const { return Hex(std::bit_cast<std::array<std::uint8_t,8>>(end))+Hex(std::bit_cast<std::array<std::uint8_t,8>>(activity)); }
};
}
// 작은 상태 전환과 현재 효과 뒤 재조회 결과를 모든 바이트/순서로 대조한다.
TEST_CASE(LocalMissionState_Native33) {
    const auto fixture=LoadFixture(NETSTORM_LOCAL_MISSION_STATE_FIXTURE);CHECK(fixture.rows.size()==34 && fixture.rows.front()==std::vector<std::string>({"input","blocks","globals","timers","cores","trace"}));
    // 기대값은 원본 기록에서만 읽고 입력만 동일 장면으로 공급한다.
    for (std::size_t index=1;index<fixture.rows.size();++index) {
        const auto& row=fixture.rows[index]; // 정확한 헤더 한 행만 건너뛰고33개 원본 입력을 모두 비교한다.
        CHECK(row.size()==6);const auto input=Split(row[0],',');CHECK(input.size()==4);StateScene scene(input[0],Number(input[1]),Number(input[2]),Number(input[3]));
        if (input[0]=="Request") scene.body->Request(Number(input[1]));else if (input[0]=="Poll") scene.body->Poll();else scene.body->Run();
        if (scene.BlockBytes()!=row[1] || scene.GlobalBytes()!=row[2] || scene.TimerBytes()!=row[3] || Hex(scene.cores)!=row[4] || (scene.trace.empty() ? "-" : scene.trace)!=row[5])
            std::fprintf(stderr,"원본 상태 불일치: %s\n실제효과 %s\n기대효과 %s\n",row[0].c_str(),scene.trace.c_str(),row[5].c_str());
        CHECK(scene.BlockBytes()==row[1]);CHECK(scene.GlobalBytes()==row[2]);CHECK(scene.TimerBytes()==row[3]);CHECK(Hex(scene.cores)==row[4]);CHECK((scene.trace.empty() ? "-" : scene.trace)==row[5]);
    }
}
// 실제 송신 위치에서는 phase9가 아직 확정되지 않았으며 수신을 공통부 뒤까지 미뤄야 한다.
TEST_CASE(LocalMissionState_QueuedReplyRunsAfterPhaseTenCommit) {
    StateScene scene("Run",9,1,0);auto hooks=scene.Hooks();bool sent{},received{};
    hooks.sendLogin=[&] {
        CHECK(scene.blocks[1][0]==1000 && scene.blocks[3][0]==9);sent=true;
        scene.body->EnqueueReceive([&] { CHECK(scene.blocks[1][0]==10 && scene.blocks[3][0]==0);received=true;scene.blocks[3][0]=13;scene.body->Run(); });
        CHECK(Throws([&] { scene.body->DispatchQueued(); }));
    };
    scene.body=std::make_unique<RawLocalMissionState>(OriginalEdition::Patch1078,scene.Storage(),scene.Globals(),std::move(hooks));
    scene.body->Run();CHECK(sent && !received && scene.blocks[1][0]==10);scene.body->DispatchQueued();CHECK(received && scene.blocks[1][0]==14 && scene.blocks[3][0]==0);
    const auto phase=scene.blocks[1];scene.body->DispatchQueued();CHECK(scene.blocks[1]==phase);
}
// 실제 저장소 별칭과 필수 효과 누락을 상태 확정 성공으로 감추지 않는다.
TEST_CASE(LocalMissionState_AliasesAndRequiredConsumers) {
    StateScene scene("Run",7,4,0);
    const auto before=scene.blocks[1];auto hooks=scene.Hooks();hooks.cleanupBattle={};
    scene.body=std::make_unique<RawLocalMissionState>(OriginalEdition::Patch1078,scene.Storage(),scene.Globals(),std::move(hooks));
    CHECK(Throws([&] { scene.body->Run(); }));CHECK(scene.blocks[1]==before);
    CHECK(Throws([&] { RawLocalMissionState invalid(OriginalEdition::Cd1072,scene.Storage(),scene.Globals(),scene.Hooks()); }));
    LocalMissionStateStorage same{scene.blocks[0],scene.blocks[1],scene.blocks[2],scene.blocks[1],scene.blocks[4]};
    CHECK(Throws([&] { RawLocalMissionState invalid(OriginalEdition::Patch1078,same,scene.Globals(),scene.Hooks()); }));
}

#include "LocalMissionCdPresenceTests.inc" // 기존 State 입력 장치를 같은 실제 Client CD 소유자에 연결한다.
