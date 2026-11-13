// 실제 CD 메뉴의 Player 진단 순서만 검사하며 전투·승패·패킷·통신은 실행하지 않는다.
#include "TestSupport.h"
#include "o/RawCdBattleCompletion.h"
#include <array>
#include <stdexcept>
#include <vector>

namespace { // 원문 물리와 현재 장치를 제공하는 비전투 입력이다.
struct CdMenuFrame {
    std::array<std::uint8_t,1548> players{}; // CD 원문의 실제9×172BYTE다.
    std::array<std::uint8_t,8> options{}; // 메뉴에서 읽지 않는 실제 CD 옵션8BYTE다.
    std::array<std::uint32_t,81> alliances{}; // 현재 방향별 동맹 장부다.
    bool battle{},mission{},boss{},server{}; // 이 검사는 실제 비전투 메뉴 상태만 사용한다.
    std::uint32_t scriptBreak{},phase{},owner{},result{}; // 현재 State/로컬/결과 필드다.
    double login{},capture{},draw{},seconds{}; // 현재 시계와 기한이며 메뉴에서 변경되면 안 된다.
    std::uint8_t settle{}; // 전투 종료 안정화 계수도 메뉴에서 바꾸면 안 된다.
    void Put(std::uint32_t player,std::uint32_t offset,std::uint32_t value) { // CD 실제 행 안의 DWORD만 검사 입력으로 쓴다.
        for (std::uint32_t byte=0;byte<4;++byte) players[player*172+offset+byte]=static_cast<std::uint8_t>(value>>(8*byte));
    }
    void Run(const netstorm::o::BattleCompletionHooks& playerHooks,const netstorm::o::BattleCompletionFrameHooks& frameHooks) { // 제품에 같은 두 현재 저장소 뷰를 전달한다.
        netstorm::o::BattleCompletionGlobals globals{players,options,alliances,battle,mission,boss,scriptBreak,login};
        netstorm::o::BattleCompletionFrameGlobals frame{players,server,phase,owner,capture,draw,settle,result,seconds};
        netstorm::o::RunCdBattleCompletionPrefix(globals,frame,playerHooks,frameHooks);
    }
};
}

// 원문의 메뉴 진단은64DWORD를 조회하지만 서버/승패/시계/결과 장치를 호출하지 않는다.
TEST_CASE(CdBattleCompletion_MenuReadsCdPairsAndKeepsInactiveState) {
    CdMenuFrame menu;menu.capture=12.5;menu.draw=8;menu.result=9;menu.settle=3;
    unsigned reads{}; // 각 실제 행에서 현재 Player base를 캡처한 횟수다.
    netstorm::o::BattleCompletionHooks players;
    players.currentPlayers=[&] { ++reads;return std::span<std::uint8_t>(menu.players); };
    netstorm::o::BattleCompletionFrameHooks frame; // 선택되지 않는 서버/시간/결과 장치는 공급하지 않는다.
    menu.Run(players,frame);
    CHECK(reads==8 && menu.capture==12.5 && menu.draw==8 && menu.result==9 && menu.settle==3);
}

// 진단이 돌아온 뒤 변경된 다음 필드를 재조회하고 CD 파일명·줄 번호를 사용해야 한다.
TEST_CASE(CdBattleCompletion_MenuDiagnosticsReadCurrentFieldsAfterCallback) {
    CdMenuFrame menu;menu.Put(1,0x74,1);menu.Put(1,0x78,2);menu.Put(1,0x80,3);menu.Put(2,0x80,1);
    std::vector<std::uint32_t> lines; // 선택된 실제 진단 순서다.
    netstorm::o::BattleCompletionFrameHooks frame;
    frame.assertion=[&](auto expression,auto file,auto line) {
        CHECK(file=="dais.cpp");lines.push_back(line);
        if (line==0x5ef) { CHECK(expression=="p->firstTempleSid == p->debugFirstTempleSid");menu.Put(1,0x80,0); }
        else CHECK(expression=="p->inProcessTempleSid == p->debugInProcessTempleSid");
    };
    menu.Run({},frame);
    CHECK(lines==std::vector<std::uint32_t>({0x5ef,0x5f0}));
}

// CD의 실제 물리가 한 BYTE만 짧아도 PATCH 폭이나 빈 기록을 대신 사용하지 않는다.
TEST_CASE(CdBattleCompletion_MenuRejectsShortCurrentPlayerStorage) {
    CdMenuFrame menu;netstorm::o::BattleCompletionHooks players;
    players.currentPlayers=[&] { return std::span<std::uint8_t>(menu.players).first(1547); };
    bool rejected{}; // 진단이나 전투 장치 호출 전에 실제 기록의 길이를 확인한다.
    try { menu.Run(players,{}); }catch (const std::out_of_range&) { rejected=true; }
    CHECK(rejected);
}

// CD00484157의 EDI는 진단이 base를 바꿔도 현재 행의 네 쌍이 끝날 때까지 유지된다.
TEST_CASE(CdBattleCompletion_MenuRetainsCapturedRowUntilNextOwner) {
    CdMenuFrame menu;menu.Put(1,0x74,1);
    std::array<std::uint8_t,1548> alternate{}; // 진단 복귀 뒤의 다른 실제 Player 저장소다.
    alternate[172+0x80]=1;alternate[2*172+0x74]=2; // 새1행은 두 번째 쌍만 다르고 새2행은 첫 쌍이 다르다.
    bool changed{}; // 첫 진단이 실제 현재 base만 바꾸며 이전 물리는 유효하게 유지한다.
    std::vector<std::uint32_t> lines;
    netstorm::o::BattleCompletionHooks players;
    players.currentPlayers=[&] { return changed ? std::span<std::uint8_t>(alternate):std::span<std::uint8_t>(menu.players); };
    netstorm::o::BattleCompletionFrameHooks frame;
    frame.assertion=[&](auto,auto,auto line) { lines.push_back(line);changed=true; };
    menu.Run(players,frame);
    CHECK(lines==std::vector<std::uint32_t>({0x5ef,0x5ef})); // 새1행의+80은 읽지 않고 다음 실제 행부터 새 base를 사용한다.
}
