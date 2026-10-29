// 고정한 원본 CPU 관측과 실제 로컬 큐를 대조하며 기대값을 별도 모델로 재계산하지 않는다.
#include "RawSceneSupport.h"
#include "client/RawBattleLeave.h"
#include "client/ClientLocalStateGlobals.h"
#include "client/NativeMainMenuState.h"
#include "zacket/LocalMissionTransport.h"

namespace { // 원본 입력 배치와 결과 직렬화만 제공한다.
using namespace netstorm::client;
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
// Native 다섯144BYTE의 전체 내용과 순서를 보존하여 원문 관측에 맞춘다.
std::string StateBytes(ClientLocalStateGlobals& local) {
    std::string text;
    for (const auto& block:local.Blocks()) { if (!text.empty()) text+=':';text+=Hex({reinterpret_cast<const std::uint8_t*>(block.data()),144}); } // 첫DWORD 이외의 보존 물리도 검사한다.
    return text;
}
}

// 원문16행의 헤더/로컬 Player 전체/다섯 상태 전체/실제 State3 송신을 대조한다.
TEST_CASE(BattleLeave_IndependentNative16) {
    std::size_t rows{};
    for (const auto& row:LoadFixture(NETSTORM_BATTLE_LEAVE_FIXTURE).rows) { // 제품 작성 전에 고정한 원본 출력만 기대값으로 사용한다.
        CHECK(row.size()==4);if (row.size()!=4) continue;
        if (row[0]=="header") {
            std::array<std::uint8_t,16> packet;packet.fill(static_cast<std::uint8_t>(Number(row[1])));InitializeBattleLeavePacket(packet);CHECK(Hex(packet)==row[2]);
        } else if (row[0]=="receive") {
            std::array<std::uint8_t,1620> players;
            for (std::size_t index=0;index<players.size();++index) players[index]=static_cast<std::uint8_t>(index*19+7); // 원문과 같은 명시 입력만 채운다.
            Put(players,180+80,Number(row[1]));ReceiveBattleLeave(1,players);CHECK(Hex(players)==row[2]);
        } else if (row[0]=="next") {
            ClientLocalStateGlobals local;
            for (auto& block:local.Blocks()) std::fill_n(reinterpret_cast<std::uint8_t*>(block.data()),144,static_cast<std::uint8_t>(Number(row[1]))); // 다른 네 블록과 부가 필드에 입력 흔적을 남긴다.
            RequestMainMenuAfterLeave(local.State());CHECK(StateBytes(local)==row[2]);
        } else if (row[0]=="parent") {
            ClientLocalStateGlobals local;auto state=local.State();state.current[0]=3;state.phase[0]=row[1]=="battle-fourteen" ? 14 : row[1]=="battle-other-phase" ? 12 : 4;
            std::uint32_t source{},outcome{},first{},editor{};bool battle=row[1]!="default";std::array<char,80> background{};std::array<std::uint8_t,256> current{},pending{};std::string sent;bool prefixed{};
            NativeMainMenuStateHooks hooks;hooks.configInt=[](auto) { return 1; };hooks.leaveBattle=[&] { CHECK(!prefixed);sent="9:"+Hex(MakeBattleLeavePacket()); };
            hooks.prefix=[&] { prefixed=true;battle=false; };hooks.hudPrefix=[] {};hooks.title=[] {};hooks.closeGuide=[] {};hooks.tell=[](auto) {};hooks.startup=[] { return 0U; };hooks.tail=[] {};
            CHECK(InstallNativeMainMenuState({state,source,outcome,first,battle,editor,background,current,pending,[] { return 0U; },[] { return 0U; }},hooks)==LocalMissionParentResult::Commit);
            CHECK(prefixed && sent==row[2]);
        } else CHECK(false);
        ++rows;
    }
    CHECK(rows==16);
}

// 송신/Flush까지 Player를 직접 바꾸지 않고 실제 Poll에서만 원문 수신기를 실행한다.
TEST_CASE(BattleLeave_ActualLocalQueueAndRegistration) {
    std::array<std::uint8_t,1620> players{};bool server=true;std::uint32_t now{},records{};std::vector<std::uint32_t> senders;
    PacketDispatchHooks hooks;hooks.now=[&] { return ++now; };hooks.record=[&](auto& connection,auto bytes,auto length) { CHECK(bytes[1]==0x52 && length==5);++records;senders.push_back(connection.sender); };
    hooks.terminate=[](auto&) { CHECK(false); };hooks.close=[](auto&) {};hooks.release=[](auto&) {};
    netstorm::zacket::LocalMissionTransport transport(players,server,std::move(hooks));transport.RegisterLogin([](auto,auto&,auto) { CHECK(false); });RegisterBattleLeavePacket(transport.Dispatch(),players);
    const auto& registration=transport.Dispatch().Registration(0x52);CHECK(registration.version==1 && registration.mismatch==0 && registration.observer && !registration.packetObserver && registration.receive);
    transport.Open(6799);transport.AssignConnection(-1,1);const auto packet=MakeBattleLeavePacket();CHECK(transport.Send(9,packet));CHECK(Get(players,180+80)==0);
    transport.Flush();CHECK(Get(players,180+80)==0);CHECK(transport.Poll()>0);CHECK(Get(players,180+80)==4 && records==1 && senders==std::vector<std::uint32_t>{1});
}

// 추가 원문8행의 단독/편집 공통 직접 쓰기와 삭제 경계 중단 부분효과를 대조한다.
TEST_CASE(BattleLeave_IndependentAbortStateNative8) {
    std::size_t rows{};
    for (const auto& row:LoadFixture(NETSTORM_BATTLE_ABORT_STATE_FIXTURE).rows) { // 원문이 관측한 전체 상태와 두 DWORD를 함께 확인한다.
        CHECK(row.size()==9);if (row.size()!=9) continue;ClientLocalStateGlobals local;
        for (auto& block:local.Blocks()) std::fill_n(reinterpret_cast<std::uint8_t*>(block.data()),144,static_cast<std::uint8_t>(Number(row[1]))); // 원문과 동일한 입력 흔적만 설치한다.
        std::uint32_t pending=0xaabbccdd,outcome=Number(row[2]),editor=row[0]=="editor";bool post=true,meta=false,battle=!editor;std::string trace,cause="normal";
        const auto remove=[&] { trace="path";if (row[3]=="path") throw std::string("path");trace+=";delete";if (row[3]=="delete") throw std::string("delete"); }; // 실제 파일은 지우지 않고 선택된 경계 중단만 관측한다.
        try { PrepareBattleAbortState({local.State(),pending,outcome,post,editor,meta,battle},remove); }catch (const std::string& stopped) { cause=stopped; }
        CHECK(cause==row[4] && pending==Number(row[5]) && outcome==Number(row[6]) && StateBytes(local)==row[7] && trace==row[8]);++rows;
    }
    CHECK(rows==8);
}
