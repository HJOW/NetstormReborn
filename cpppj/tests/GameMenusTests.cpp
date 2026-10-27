// 원본네메뉴부모의918독립관측과실제공개C++몸체를비교한다.
#include "DudeSpawnSupport.h"
#include "client/RawGameMenus.h"
#include "client/GameMenuCommands.h"
#include <map>
using namespace netstorm::client;
namespace {
// 모드/설정/큰게임helper/GUI만명시입력으로제공한다. 메뉴분기는제품이실행한다.
struct GameMenusScene {
    std::array<std::uint32_t,20> input{}; // kind부터mutation까지원문20DWORD입력이다.
    std::map<std::uint32_t,std::uint32_t> globals; // 실제원문주소로서로다른현재전역을구별한다.
    std::array<std::array<std::uint8_t,180>,9> players{}; // 같은현재Player9의원시몸체다.
    std::map<std::string,std::uint32_t> reads; // 설정재조회순서의명시입력이다.
    std::vector<std::uint32_t> stack; // 명시controlled스택이며제품의분기결과만반영한다.
    std::uint32_t created{};std::string trace; // 실제GUI생성개수와전체경계순서다.
    // 독립원문자료의20입력과현재명령PE상수를제공한다.
    explicit GameMenusScene(std::string_view values) {
        const auto fields=Split(values,',');CHECK(fields.size()==input.size());
        // 입력은원문과같은DWORD로읽고기대목록은계산하지않는다.
        for (std::size_t index=0;index<input.size();++index) input[index]=Number(fields[index]);
        const std::array<std::uint32_t,16> addresses{0x5c85a4,0x594fbc,0x594fa4,0x594fb8,0x594fc8,0x594fa0,0x59507c,0x595344,0x531860,0x531864,0x5c78d4,0x5c7b14,0x531900,0x531904,0x55b4b0,0}; // 실제입력전역의주소다.
        const std::array<std::size_t,15> indices{1,2,3,4,5,6,9,10,11,12,13,14,15,16,17}; // 원문입력필드의현재위치다.
        // 각전역의입력필드를서로합치지않고초기화한다.
        for (std::size_t index=0;index<indices.size();++index) globals[addresses[index]]=input[indices[index]];
        globals[0x5c7910]=1;globals[0x59a8ac]=1;globals[0x540c70]=1;
        const std::array<std::array<std::uint32_t,2>,23> commands{{{0x5429fc,0x020000c2},{0x542948,0x02000095},{0x542938,0x02000091},{0x542928,0x0200008d},{0x542924,0x0200008c},{0x542a60,0x020000db},{0x542a08,0x020000c5},{0x542944,0x02000094},{0x5428dc,0x02000079},{0x542780,0x02000022},{0x542a4c,0x020000d6},{0x542914,0x02000088},{0x542a0c,0x020000c6},{0x542ab8,0x020000f1},{0x5429ac,0x020000ae},{0x5429d4,0x020000b8},{0x542974,0x020000a0},{0x5428ac,0x0200006d},{0x542ab4,0x020000f0},{0x5427b8,0x02000030},{0x542a8c,0x020000e6},{0x542a90,0x020000e7},{0x542a30,0x020000cf}}}; // 보호PE의현재명령전역이다.
        // 실제관측도원래명령전역을사용하므로이초기값은기대메뉴목록과구별한다.
        for (const auto& command:commands) globals[command[0]]=command[1];
    }
    // 현재문자열의원본바이트를hex로보존한다.
    static std::string Encoded(std::string_view text) { return Hex({reinterpret_cast<const std::uint8_t*>(text.data()),text.size()}); }
    // 숫자DWORD와문자열인자의원문의미를구별한다.
    static std::string Parameter(const GameMenuParameter& parameter) { return std::holds_alternative<std::uint32_t>(parameter) ? 'n'+std::to_string(std::get<std::uint32_t>(parameter)) : 's'+Encoded(std::get<std::string>(parameter)); }
    // 실제경계와전달값을현재순서로기록한다.
    void Add(std::string value) { if (!trace.empty()) trace+=';';trace+=value; }
    // 명시설정마스크를읽고선택한경우두번째조회결과만바꾼다.
    std::int32_t Config(std::string_view key) {
        const std::array<std::string_view,12> keys{"theammode","useBanners","sound","music","playWind","swapLeftRightSpeakers","edgeScroll","cursorSnapToGrid","autoDemo","tellTips","rotmode","passedDiags"}; // 원문설정목록이다.
        const auto found=std::find(keys.begin(),keys.end(),key);CHECK(found!=keys.end());const auto index=static_cast<std::size_t>(found-keys.begin()); // 실제설정입력위치다.
        const auto count=++reads[std::string(key)];return static_cast<std::int32_t>(((input[18]>>index)&1U)^((input[19]&4) ? ((count-1)&1U) : 0U));
    }
    // 같은controlled스택의명시생성효과다.
    void Begin(std::string_view kind) { Add(std::string(kind));stack.push_back(0x01001000U+(++created)*0x100U); }
    // 큰게임/GUI경계만원문과같은명시공급값으로연결한다.
    GameMenusHooks Hooks() {
        GameMenusHooks hooks;hooks.global=[&](std::uint32_t address) { const auto found=globals.find(address);if (found==globals.end()) throw std::logic_error("미명시메뉴전역"+std::to_string(address));if (address==0x55b4b0) Add("PA");return found->second; };
        hooks.player=[&](std::uint32_t owner)->std::span<const std::uint8_t> { return players.at(owner); };
        hooks.translate=[&](std::string_view text) {
            Add("T,"+Encoded(text));
            if ((input[19]&1) && text=="Hide/Show chat - ~iF3~.") globals[0x594fa4]^=1;
            if ((input[19]&2) && text=="Direct Draw / Full Screen") globals[0x5c7910]^=1;
            if ((input[19]&8) && text=="View Your Priest - ~iF5~.") globals[0x540c70]=2;
            return std::string(text);
        };
        hooks.config=[&](std::string_view key) { Add("G,"+std::string(key));return Config(key); };hooks.readInt=[&](std::string_view key,std::int32_t& value) { Add("R,"+std::string(key));value=Config(key); };
        hooks.vertical=[&] { Begin("V"); };hooks.fancy=[&] { Begin("F"); };hooks.separator=[&] { Add("X"); };
        hooks.take=[&] { Add("Q");CHECK(!stack.empty());const auto token=stack.back();stack.pop_back();return token; };
        hooks.item=[&](std::string_view text,std::uint32_t code,const GameMenuParameter& parameter,std::uint32_t include,std::uint32_t enabled) { Add("I,"+Encoded(text)+','+std::to_string(code)+','+Parameter(parameter)+','+std::to_string(include)+','+std::to_string(enabled)+",0"); };
        hooks.checked=[&](std::uint32_t check,std::string_view text,std::uint32_t code,const GameMenuParameter& parameter,std::uint32_t enabled) { Add("K,"+std::to_string(check)+','+Encoded(text)+','+std::to_string(code)+','+Parameter(parameter)+",1,"+std::to_string(enabled)); };
        hooks.submenu=[&](std::string_view text,std::uint32_t child,std::uint32_t enabled,std::uint32_t check) { Add("S,"+Encoded(text)+','+std::to_string(child)+','+std::to_string(enabled)+','+std::to_string(check)); };
        hooks.editingMission=[&] { Add("E");return input[7]; };hooks.demo=[&] { Add("D");return input[8]; };hooks.loggedIn=[&] { Add("L");return 2U; };
        hooks.priest=[&](std::uint32_t owner) { Add("P,"+std::to_string(owner));return 99U; };hooks.money=[&](std::uint32_t owner) { Add("M,"+std::to_string(owner));return 10000.0F; };hooks.templeCost=[&] { Add("COST");return 1500.0F; };
        hooks.priestBusy=[&](std::uint32_t sid) { Add("B,"+std::to_string(sid));return 0U; };hooks.ritual=[&] { Add("RI");return 0U; };hooks.otherRitual=[&] { Add("RO,0");return 0U; };hooks.captured=[&](std::uint32_t owner) { Add("CA,"+std::to_string(owner));return 0U; };return hooks;
    }
    // 실제공개제품의최종반환/전체효과를독립원문출력과같은형태로읽는다.
    std::string Run() {
        RawGameMenus menus(Hooks());std::uint32_t token{}; // 실제선택부모의반환값이다.
        switch (input[0]) { case 0:token=menus.Game();break;case 1:token=menus.Edit();break;case 2:token=menus.View();break;case 3:token=menus.Options();break;default:throw std::logic_error("미명시메뉴부모"); }
        CHECK(stack.empty());return std::to_string(token)+'|'+trace;
    }
};
// 같은현재Player원시몸체와명시사제/랭크/GUI를추가한목록검사장치다.
struct PlayerListMenuScene:GameMenusScene {
    GumpCreateNode menuNode; // 명시GUI생성이공급하는현재기본폭/높이다.
    // 각원문행의현재Player/관전BYTE를같은물리저장소에준비한다.
    explicit PlayerListMenuScene(std::string_view values):GameMenusScene(values) {
        globals[0x563e38]=0x01001234U;menuNode.baseWidth=130.5F;menuNode.baseHeight=201.25F;
        // Player0도물리적으로존재하되원문부모는1..8만방문한다.
        for (std::uint32_t owner=0;owner<9;++owner) {
            const auto active=owner && ((input[18]>>(owner-1U))&1U);auto& player=players[owner]; // 해당Player의명시입력이다.
            const auto subscriber=active ? owner*100U : 0U;player[0x2c]=static_cast<std::uint8_t>(subscriber);player[0x2d]=static_cast<std::uint8_t>(subscriber>>8);
            player[0x50]=static_cast<std::uint8_t>((owner%2 ? 1U : 0x20U) | (owner%3==0 ? 8U : 0U));const auto name="Player"+std::to_string(owner); // 원문32BYTE필드안에NUL을남긴다.
            std::copy(name.begin(),name.end(),player.begin()+0xc);globals[0x52f81e+owner]=(input[15]>>owner)&1U;
        }
    }
    // 실제게임helper는각원문fixture와같은명시반환/효과만공급한다.
    GameMenusHooks Hooks() {
        auto hooks=GameMenusScene::Hooks();hooks.title=[&](std::string_view text) { Add("TITLE,"+Encoded(text)); };
        hooks.refreshPriest=[&](std::uint32_t owner) { Add("RF,"+std::to_string(owner));if (input[19]&16) players.at(owner)[0x50]^=0x21U; };
        hooks.rank=[&](std::uint32_t index) { Add("RANK,"+std::to_string(index));return static_cast<std::int32_t>(index+3); };
        hooks.rankPicture=[&](std::int32_t rank) { Add("PIC,"+std::to_string(rank));return "~[Iicon.B"+std::to_string(rank)+']'; };
        hooks.level=[&](std::uint32_t owner) { Add("LEVEL,"+std::to_string(owner));if (input[19]&32) globals[0x594fa4]=1;return static_cast<std::int32_t>(0x100U+owner+4U); };
        hooks.format=[&](std::string_view pattern,std::int32_t value) {
            Add("FMT,"+Encoded(pattern)+','+std::to_string(value));std::string text(pattern);const auto found=text.find("%d");if (found==text.npos) throw std::logic_error("명시Players서식누락");text.replace(found,2,std::to_string(value));return text;
        };
        hooks.playerMenu=[&](std::uint32_t subscriber) { Add("PM,"+std::to_string(subscriber));return 200U+subscriber/100U; };
        hooks.current=[&] { Add("N");CHECK(!stack.empty());return stack.back(); };hooks.node=[&](std::uint32_t token) { CHECK(!stack.empty() && token==stack.back());return &menuNode; };
        hooks.attachCurrent=[&](std::int32_t x,std::int32_t y,std::uint32_t parent) { Add("A,"+std::to_string(std::bit_cast<std::uint32_t>(x))+','+std::to_string(std::bit_cast<std::uint32_t>(y))+','+std::to_string(parent));CHECK(!stack.empty());stack.pop_back(); };return hooks;
    }
    // 실제Players부모가반환한현재토큰과전체호출순서를관측한다.
    std::string Run() { RawGameMenus menus(Hooks());const auto token=menus.Players(input[7]!=0);CHECK(stack.empty());return std::to_string(token)+'|'+trace; }
};
}
// 네부모의모든모드와체크·하위메뉴·현재조회효과를918실제원문관측과비교한다.
TEST_CASE(GameMenus_Native918) {
    const auto rows=LoadFixture(NETSTORM_GAME_MENUS_FIXTURE).rows;CHECK(rows.size()==918);
    // 각원문행을새로운장치/같은공개C++몸체로독립실행한다.
    for (const auto& row:rows) { CHECK(row.size()==3);GameMenusScene scene(row[1]);CHECK(scene.Run()==row[2]); }
}
// 빈/교차/전체현재목록·관전·미션·사제갱신과fancy위치의530원문관측을대조한다.
TEST_CASE(PlayerListMenu_Native530) {
    const auto rows=LoadFixture(NETSTORM_PLAYER_LIST_MENU_FIXTURE).rows;CHECK(rows.size()==530);
    // 각행은같은현재Player9와실제공개부모를새로준비한다.
    for (const auto& row:rows) { CHECK(row.size()==3);PlayerListMenuScene scene(row[1]);CHECK(scene.Run()==row[2]); }
}
// F9의 기존 큐 순서와 Shift/Control/Alt/뗌 분기를 실제 명령으로 대조한다.
TEST_CASE(PlayerListMenu_QueuesF9AndShiftPause) {
    InputQueue queue;queue.Push(0x1234,5,6);
    CHECK(QueuePlayerListKey({0x780000U,99,88},queue));
    CHECK(QueuePlayerListKey({InputCode::kShift|0x780000U,0,0},queue));
    CHECK(QueuePlayerListKey({InputCode::kAlt|0x780000U,0,0},queue));
    CHECK(!QueuePlayerListKey({InputCode::kControl|0x780000U,0,0},queue));
    CHECK(!QueuePlayerListKey({InputCode::kControl|InputCode::kShift|0x780000U,0,0},queue));
    CHECK(!QueuePlayerListKey({InputCode::kRelease|0x780000U,0,0},queue));
    CHECK(!QueuePlayerListKey({0x770000U,0,0},queue));
    const auto old=queue.Pop(false);CHECK(old.code==0x1234 && old.x==5 && old.y==6);
    // 원본 예약 순서대로 기존 사건 다음에 목록/정지/목록이 오며 좌표를 인자로 재사용하지 않는다.
    for (const auto code:{0x020000f0U,0x020000cfU,0x020000f0U}) {
        const auto event=queue.Pop(false);CHECK(event.code==code && event.x==0 && event.y==0);
    }
    CHECK(queue.Empty());
}
