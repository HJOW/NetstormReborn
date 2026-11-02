// 원본 World/선택메뉴50관측과 같은 공개 몸체/공유 CRT 정렬의 결과를 비교한다.
#include "DudeSpawnSupport.h"
#include "GumpRawSupport.h" //이름관측의원문문자열hex만해독한다.
#include "client/RawKnowledgeMenu.h"
#include "client/GameMenuCommands.h" //실제 HTML 이름 조회와 일반 명령 허용표까지 같은 경로로 검증한다.
#include "client/ScriptCommand.h"
#include <map>
using namespace netstorm::client;
namespace { //원문 입력과 외부 GUI 조회만 공급하는 독립 검사 장면이다.
// 실제 PE와 같은 명시 타입·내용물/제외·번역/GUI 입력이며 기대값은 fixture에서만 읽는다.
struct WorldKnowledgeScene {
    bool side; //실제원문4938c0선택메뉴인지493210World카드인지를명시한다.
    std::array<std::uint32_t,10> input{}; // 차단/타입수/보유/제외/언어/화면/아이콘/번역 profile이다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(12); // 원본 입력과 같은12개 물리 타입이다.
    std::vector<MenuGumpNode> nodes; // GUI 생성/가상 효과의 명시 입력 저장소다. 게임 몸체는 대체하지 않는다.
    KnowledgeMenuState state; // 같은 호출에서 사용하는 실제 화면/아이콘 치수다.
    std::uint32_t marker{0xa5a5a5a5U};std::array<std::uint8_t,80> argument{}; // 선택되지 않은 표식 쓰기를 구별하는 초기 입력이다.
    std::string trace; // 실제 생성/글자/부착/조회의 순서다.
    const std::array<std::string_view,12> names{"Unused","Wind Cannon","Long-name-here","Sun","Same","same","Wide Name Here","Priest","Temple","Zero Mana","Group10","BadTheme"}; // 네이티브 독립 번역 입력이다.
    const std::array<std::string_view,12> mana{"","","w","WW","w.r","r","ww","","","","",""}; // 원본 strlen이 읽는 실제 바이트 입력이다.
    // 원본 입력의 DWORD 비트와 필터/정렬 타입 필드만 준비한다.
    explicit WorldKnowledgeScene(std::string_view values,bool selector):side(selector) {
        const auto fields=Split(values,',');CHECK(fields.size()==input.size());argument.fill(0xa5);
        // 각 독립 입력을 읽으며 예상 메뉴 개수/순서는 계산하지 않는다.
        for (std::size_t index=0;index<input.size();++index) input[index]=Number(fields[index]);
        const std::array<std::uint32_t,12> flags{0,0,0x10000,0,0x800,0x800,0x40000,0x200000,0,0,0,0}; // 실제 정렬/사제 제외 입력이다.
        // 원본이 읽는 필드 폭과 동일한 타입 번호를 보존한다.
        for (std::size_t index=0;index<types.size();++index) { types[index].theme=index==11 ? 8U : 4U+static_cast<std::uint32_t>(index%4);types[index].group=index==0 || index==10 ? 10 : 0;types[index].flags2=flags[index]; }
        state.width=static_cast<std::int32_t>(input[5]);state.height=static_cast<std::int32_t>(input[6]);state.iconWidth=static_cast<std::int32_t>(input[7]);state.iconHeight=static_cast<std::int32_t>(input[8]);state.language=input[4];state.templeProduct=8;
    }
    // 원문 문자열의 바이트를 기대값과 같은 hex로 직렬화한다.
    static std::string Encoded(std::string_view text) { return Hex({reinterpret_cast<const std::uint8_t*>(text.data()),text.size()}); }
    // 외부 효과 하나의 실제 인자만 기록한다.
    void Add(std::string value) { if (!trace.empty()) trace+=';';trace+=value; }
    // 실제 지식 창 함수가 호출할 GUI/내용물/번역 경계다. 필터·정렬·배치 계산은 제품이 수행한다.
    KnowledgeMenuHooks Hooks() {
        KnowledgeMenuHooks hooks;hooks.blocked=[&] { return input[0]!=0; };hooks.cancel=[&] { Add("Cancel"); };hooks.currentTechnology=[] { return 9U; };
        hooks.owns=[&](std::uint32_t parent,std::uint32_t type) { CHECK(parent==9);return (input[2]&(1U<<type))!=0; };hooks.mana=[&](std::uint32_t type) { return std::string(mana.at(type)); };
        hooks.description=[&](std::uint32_t type) { return input[9] ? std::string(type%2 ? "Same" : "same") : std::string(names.at(type)); };
        hooks.inProduction=[](std::uint32_t) { return false; };hooks.excluded=[&](std::uint32_t type) { return (input[3]&(1U<<type))!=0; };hooks.translate=[](std::string_view text) { return std::string(text); };
        hooks.begin=[&] { const auto index=nodes.size();nodes.emplace_back();auto& node=nodes.back();node.flags=0x4001;node.scale=1;node.baseWidth=node.width=60;node.baseHeight=node.height=20;node.targetWidth=52.5F;Add("V,"+std::to_string(index));return static_cast<std::uint32_t>(index+1); };
        hooks.node=[&](std::uint32_t token)->MenuGumpNode& { return nodes.at(token-1); };hooks.title=[&](std::string_view text) { Add("T,"+Encoded(text)); };hooks.separator=[&] { Add("X"); };
        hooks.item=[&](std::string_view text,std::uint32_t code,KnowledgeParameter parameter,bool enabled) {
            const auto value=std::holds_alternative<std::uint32_t>(parameter) ? std::get<std::uint32_t>(parameter) : 5304400U; // 원본 정적 themeHelp 주소0050f050의 의미만 테스트에서 정규화한다. 제품은 별도 수명 토큰이다.
            if (std::holds_alternative<std::string>(parameter)) CHECK(std::get<std::string>(parameter)=="themeHelp");Add("I,"+Encoded(text)+','+std::to_string(code)+','+std::to_string(value)+",1,"+(enabled ? "1" : "0")+",0");
        };
        hooks.attach=[&](std::int32_t x,std::int32_t y) { auto& node=nodes.back();node.x=static_cast<float>(x);node.y=static_cast<float>(y);Add("A,"+std::to_string(nodes.size()-1)+','+std::to_string(std::bit_cast<std::uint32_t>(x))+','+std::to_string(std::bit_cast<std::uint32_t>(y))); };
        hooks.push=[&](std::uint32_t token) { Add("P,"+std::to_string(token-1)); };hooks.pop=[&](std::uint32_t token) { Add("Q,"+std::to_string(token-1)); };
        hooks.width=[&](std::uint32_t token,float value) { auto& node=nodes.at(token-1);node.baseWidth=node.width=value; };hooks.height=[&](std::uint32_t token,float value) { auto& node=nodes.at(token-1);node.baseHeight=node.height=value; };
        hooks.position=[&](std::uint32_t token) { Add("B,"+std::to_string(token-1));const auto& node=nodes.at(token-1);return ScreenPoint{static_cast<int>(node.x),static_cast<int>(node.y)}; };
        hooks.mark=[&](std::uint32_t value) { marker=value;argument.fill(0); };return hooks;
    }
    // 원본이 남긴 같은 필드/표식/호출 순서만 출력한다.
    std::string Run() {
        const auto tokens=side ? ShowKnowledgeSideMenu(state,Hooks()) : ShowWorldKnowledgeMenu(std::span(types).first(input[1]),state,Hooks());CHECK(tokens.size()==nodes.size());std::string fields;
        // 단정도 값은 소수 문자열로 바꾸지 않고 전체DWORD를 비교한다.
        for (const auto& node:nodes) {
            if (!fields.empty()) fields+='~';const std::array<std::uint32_t,9> words{node.flags,std::bit_cast<std::uint32_t>(node.baseWidth),std::bit_cast<std::uint32_t>(node.baseHeight),std::bit_cast<std::uint32_t>(node.width),std::bit_cast<std::uint32_t>(node.height),std::bit_cast<std::uint32_t>(node.x),std::bit_cast<std::uint32_t>(node.y),std::bit_cast<std::uint32_t>(node.targetWidth),node.menuFlags};
            // 각 노드의 원본 필드 순서를 유지한다.
            for (std::size_t index=0;index<words.size();++index) { if (index) fields+=',';fields+=std::to_string(words[index]); }
        }
        return (fields.empty() ? "-" : fields)+'|'+std::to_string(marker)+'|'+Hex(argument)+'|'+(trace.empty() ? "-" : trace);
    }
};
}

//실제World25장면/선택25장면의행·위치·단정도노드·표식과반환을같은제품몸체로대조한다.
TEST_CASE(KnowledgeWorldMenu_Native50) {
    const auto rows=LoadFixture(NETSTORM_KNOWLEDGE_WORLD_MENU_FIXTURE).rows;CHECK(rows.size()==50);
    for (const auto& row:rows) { CHECK(row.size()==3);if (row.size()!=3) continue;WorldKnowledgeScene scene(row[1],row[0]=="Side");CHECK(scene.Run()==row[2]); } //입력과외부조회만공급하며출력기대는fixture만읽는다.
}
//원문실제명령표/토큰/큐21관측을제품의같은이름조회/문자열고리/FIFO로확인한다.
TEST_CASE(KnowledgeWorldMenu_Native21NamesAndQueue) {
    CHECK(GameMenuCommandValue(0x00542b84U)==kShowTecSideMenuCommand && IsGameMenuCommand(kShowTecSideMenuCommand));
    CHECK(GameMenuCommandValue(0x00542b88U)==kShowTecMenu2Command && IsGameMenuCommand(kShowTecMenu2Command));
    const auto rows=LoadFixture(NETSTORM_KNOWLEDGE_WORLD_NAMES_FIXTURE).rows;CHECK(rows.size()==21);
    for (const auto& row:rows) { //원문코드와인자기대값은동결관측에서만읽는다.
        CHECK(row.size()==9);if (row.size()!=9) continue;const auto bytes=netstorm::test::gumpraw::Bytes(row[1]);const std::string source(bytes.begin(),bytes.end());InputQueue queue;ScriptStringRing strings;std::uint32_t unknown{};
        QueueScriptCommand(source,queue,strings,{HtmlScriptCommandValue,[&](auto) { ++unknown; }});CHECK(strings.Index()==Number(row[7]));
        if (row[6]=="-") { CHECK(queue.Empty() && unknown);continue; }
        const auto expected=Split(row[6],',');CHECK(expected.size()==2);if (expected.size()!=2) continue;const auto event=queue.Pop(false);CHECK(event.code==Number(expected[0]) && event.x==event.y && queue.Empty() && !unknown);
        const auto token=std::bit_cast<std::uint32_t>(event.y);if (expected[1].starts_with('n')) CHECK(token==Number(expected[1].substr(1)));
        else { CHECK(expected[1].starts_with('s'));const auto value=strings.Resolve(token);CHECK(Hex({reinterpret_cast<const std::uint8_t*>(value.data()),value.size()})==expected[1].substr(1)); }
    }
}
