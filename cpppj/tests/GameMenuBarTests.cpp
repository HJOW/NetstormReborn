// 원본004cc210 전체 부모의 독립510행과 같은 공개 C++ 본체를 대조한다.
#include "DudeSpawnSupport.h"
#include "client/RawGameMenuBar.h"
using namespace netstorm::client;
namespace {
// 하위 부모/번역/GUI 효과는 독립 원문 관측과 같은 명시 장치이며 제품 계산을 대신하지 않는다.
struct GameMenuBarScene {
    std::array<std::uint32_t,9> input{}; // editor/battle/cheat/mission/checksum/technical/width/left/mutation 입력이다.
    std::int32_t tracking{}; // 독립원문장치의같은추적DWORD소유자다.
    GameMenuBarState state{tracking}; // 제품은실제기존소유주소를빌리며별도검산사본을갖지않는다.
    std::uint32_t takes{},serial{77}; // 명시controlled꺼내기순번과부착후실제생성순번입력이다.
    std::string trace; // 실제 경계 호출 순서다.
    // 각 원시DWORD를 변환 없이 읽고 출력 기대는 fixture에만 둔다.
    explicit GameMenuBarScene(std::string_view values) {
        const auto fields=Split(values,',');CHECK(fields.size()==input.size());
        // 원문 입력과 같은9개 물리DWORD를 준비한다.
        for (std::size_t index=0;index<input.size();++index) input[index]=Number(fields[index]);
        state.checksum=std::bit_cast<std::int32_t>(input[4]);state.menuSerial=0xa5a5a5a5U;
    }
    // 원본 문자열 바이트를 기대값과 같은 hex로 기록한다.
    static std::string Encoded(std::string_view text) { return Hex({reinterpret_cast<const std::uint8_t*>(text.data()),text.size()}); }
    // 실제 외부 효과를 한 번 기록한다.
    void Add(std::string event) { if (!trace.empty()) trace+=';';trace+=event; }
    // 각 실제 접근 시점의 같은 원문 전역을 읽는다.
    std::uint32_t Read(GameMenuBarValue value) const {
        switch (value) {
        case GameMenuBarValue::Editor:return input[0];case GameMenuBarValue::Battle:return input[1];
        case GameMenuBarValue::Cheat:return input[2];case GameMenuBarValue::Mission:return input[3];
        case GameMenuBarValue::Width:return input[6];case GameMenuBarValue::Left:return input[7];
        case GameMenuBarValue::Root:return 0x01001234U;
        }
        throw std::logic_error("관측 밖 메뉴 전역");
    }
    // 원문 실행의 명시 장치만 같은 제품 훅에 제공한다.
    GameMenuBarHooks Hooks() {
        GameMenuBarHooks hooks;hooks.read=[&](auto value) { return Read(value); };hooks.prepare=[&] { Add("P"); };
        hooks.translate=[&](std::string_view text) { Add("T,"+Encoded(text));return std::string(text); };hooks.about=[] { return "About NetStorm"; };
        hooks.horizontal=[&] { Add("H");return 0x01001000U; };hooks.vertical=[&] { Add("V"); };hooks.separator=[&] { Add("X"); };
        hooks.take=[&] { Add("Q");return ++takes==1 ? 7U : 0x01001000U; };
        hooks.child=[&](GameMenuBarChild kind) {
            const auto child=static_cast<std::uint32_t>(kind);Add("C,"+std::to_string(child)); // 실제 도달 하위만 관측한다.
            if ((input[8]&1) && child==0) input[0]^=1;
            if ((input[8]&2) && child==3) { input[1]^=1;state.checksum=9599; }
            return child==5 ? ((input[8]&32) ? 0U : 8U) : child+2U;
        };
        hooks.submenu=[&](std::string_view text,std::uint32_t child) { Add("S,"+Encoded(text)+','+std::to_string(child)); };
        hooks.item=[&](std::string_view text,std::uint32_t code,std::string_view parameter,std::uint32_t include) {
            Add("I,"+Encoded(text)+','+std::to_string(code)+','+Encoded(parameter)+','+std::to_string(include)+",1,0");
        };
        hooks.technicalHelp=[&] { Add("D");if (input[8]&4) { input[2]^=1;input[3]^=1; }return input[5]; };
        hooks.width=[&](std::uint32_t token,float width) { Add("W,"+std::to_string(token)+','+std::to_string(std::bit_cast<std::uint32_t>(width)));if (input[8]&8) input[7]=123; };
        hooks.attach=[&](std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode) {
            Add("A,"+std::to_string(token)+','+std::to_string(std::bit_cast<std::uint32_t>(x))+','+std::to_string(std::bit_cast<std::uint32_t>(y))+','+std::to_string(parent)+','+std::to_string(mode));if (input[8]&16) serial=991;
        };
        hooks.serial=[&](std::uint32_t token) { CHECK(token==0x01001000U);return serial; };return hooks;
    }
    // 실제 부모가 남긴 최종 두DWORD와 실제 순서만 직렬화한다.
    std::string Run() { RawGameMenuBar menu(state,Hooks());CHECK(menu.Open()==0x01001000U);CHECK(takes==2);CHECK(&state.checksum==&tracking);return std::to_string(std::bit_cast<std::uint32_t>(state.checksum))+'|'+std::to_string(state.menuSerial)+'|'+trace; }
};
}
// 모든 모드/검산 signed 감김/기술 도움말 include/하위 효과 뒤 재조회를 원문 전체와 비교한다.
TEST_CASE(GameMenuBar_Native510) {
    const auto rows=LoadFixture(NETSTORM_GAME_MENU_BAR_FIXTURE).rows;CHECK(rows.size()==510);
    // 각 독립 행은 새로운 장치와 실제 공개 제품 본체를 사용한다.
    for (const auto& row:rows) { CHECK(row.size()==3);GameMenuBarScene scene(row[1]);CHECK(scene.Run()==row[2]); }
}
// 없는 하위는 화면처럼 보이는 빈 성공으로 바꾸지 않는다.
TEST_CASE(GameMenuBar_MissingChildIsExplicit) {
    GameMenuBarScene scene("0,1,0,1,0,1,1024,85,0");auto hooks=scene.Hooks();hooks.child={};bool rejected=false; // 첫 Game 하위 경계다.
    try { RawGameMenuBar menu(scene.state,std::move(hooks));static_cast<void>(menu.Open()); } catch (const std::logic_error&) { rejected=true; }
    CHECK(rejected && scene.trace=="P;H;T,47616d65");
}
