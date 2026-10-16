// 실제 두 PE의 세 메뉴 몸체와 같은 독립 입력/콜백 변경을 비교한다.
#include "RawSceneSupport.h"
#include "client/RawObjectMenu.h"
#include <bit>
#include <sstream>
using namespace netstorm::client;
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// fixture14개 입력 열의 원본 의미다. 부호 있는 환불도 저장은DWORD비트다.
enum Input { Owner,Local,RawType,PassedType,Modes,Deny,CurrentFlags,PassedFlags,Cost,Refund,Count,Items,Hp,Mutation };
// 스타일 문자열을 구분자와 충돌하지 않는 원문 바이트로 기록한다.
std::string TextHex(std::string_view text) { return Hex({reinterpret_cast<const std::uint8_t*>(text.data()),text.size()}); }
// 실제 원문 fixture의 효과만 수집하며 예상 메뉴 분기는 만들지 않는다.
struct ObjectScene {
    SidPool pool; // 같은 원본 판본의 실제 슬롯 저장소다.
    std::vector<RiftTypeRecord> types; // 실제 판본의 현재 TYPE 개수다.
    ObjectMenuState state; // 현재 원본 모드/명령 전역이다.
    ObjectMenuHooks hooks; // 원문과 같은 외부 조회/효과를 공급한다.
    std::vector<std::uint32_t> input; // fixture의 독립14DWORD 입력이다.
    std::string trace; // 실제로 호출된 전체 효과 순서다.
    std::uint32_t resolvedOwner{}; // 이전 oracle의 명시 opaque 반환값에 붙인 관찰용 label의 입력이다.
    // 원문 입력50번 슬롯만 실제 클라이언트 영역에서 확보한다.
    ObjectScene(OriginalEdition edition,const std::vector<std::string>& row):pool(edition,24000,true),types(edition==OriginalEdition::Patch1078 ? 188 : 171) {
        // 입력 정수의 부호와DWORD비트를 보존한다.
        for (std::size_t index=2;index<16;++index) input.push_back(static_cast<std::uint32_t>(std::stoll(row[index])));
        // 5..50번은 두 판본의 실제 클라이언트 슬롯이다.
        for (std::uint32_t number=5;number<=50;++number) CHECK(pool.Allocate(2).value==number);
        auto raw=pool.AllocatedBytes(Sid{50});raw[10]=static_cast<std::uint8_t>(input[RawType]);raw[edition==OriginalEdition::Patch1078 ? 34 : 32]=static_cast<std::uint8_t>(input[Owner]);
        // 변경 콜백이 가리킬 TYPE도 실제 표 안에 준비한다.
        for (auto& type:types) type.cost=1.0F;
        types[input[RawType]].flags2=input[CurrentFlags];types[input[RawType]].cost=std::bit_cast<float>(input[Cost]);types[input[PassedType]].flags2=input[PassedFlags];
        state.localOwner=input[Local];state.editor=(input[Modes]&1)!=0;state.special=(input[Modes]&2)!=0;state.chargeBlocked=(input[Modes]&4)!=0;state.denySalvage=input[Deny];state.helpWord="AboutType";
        hooks.hasHitPoints=[this](Sid sid) { Add("H,"+std::to_string(sid.value));return input[Hp]!=0; };
        hooks.count=[this](Sid sid) { Add("L,"+std::to_string(sid.value));if (input[Mutation]&1) pool.AllocatedBytes(sid)[10]=122;return input[Count]; };
        hooks.salvageRefund=[this](Sid sid,std::uint32_t type,std::uint32_t count) {
            Add("R,"+std::to_string(sid.value)+','+std::to_string(type)+','+std::to_string(count));if (input[Mutation]&2) state.denySalvage=1;return std::bit_cast<std::int32_t>(input[Refund]);
        };
        hooks.format=[this](std::string_view key,std::span<const PriestMenuArgument> arguments) {
            Add("T,"+TextHex(key));if (input[Mutation]&16) types[input[PassedType]].flags2=0x200;
            std::string text(key);if (!arguments.empty()) { CHECK(arguments.size()==1);const auto at=text.find("%d");CHECK(at!=std::string::npos);text.replace(at,2,std::to_string(std::get<std::int32_t>(arguments[0]))); }return text;
        };
        hooks.itemCount=[this] { Add("Q");if (input[Mutation]&32) types[input[PassedType]].flags2=0x4000;return input[Items]; };
        hooks.separator=[this] { Add("X");if (input[Mutation]&4) state.denySalvage=0; };
        hooks.setConfig=[this](std::string_view key,std::string_view value) { Add("C,"+TextHex(key)+','+TextHex(value));if (input[Mutation]&8) pool.AllocatedBytes(Sid{50})[10]=122; };
        hooks.item=[this](std::string_view label,std::uint32_t command,const ObjectMenuParameter& parameter) {
            const auto value=std::holds_alternative<std::uint32_t>(parameter) ? "N:"+std::to_string(std::get<std::uint32_t>(parameter)) : "S:"+TextHex(std::get<std::string>(parameter));
            Add("I,"+TextHex(label)+','+std::to_string(command)+','+value);
        };
        hooks.ownerSubscriber=[this](std::uint32_t owner) { Add("N,"+std::to_string(owner));resolvedOwner=owner;return 0x12200100U; };
        // 이전 fixture의 하위 경계는 고정 opaque값을 PlayerN label로 관찰했다. 문자열 API의 근거로 사용하지 않는다.
        hooks.playerMenu=[this](std::uint32_t subscriber,bool first,bool second) { CHECK(subscriber==0x12200100U);Add("P,"+TextHex("Player"+std::to_string(resolvedOwner))+','+std::to_string(first)+','+std::to_string(second));return 123U; };
        hooks.submenu=[this](std::string_view label,std::uint32_t menu) { Add("S,"+TextHex(label)+','+std::to_string(menu)); };
    }
    // 외부 효과의 순서와 빈 결과를 원문 형식에 맞춘다.
    void Add(std::string text) { if (!trace.empty()) trace+=';';trace+=text; }
    // 생성 중 실제 메뉴 효과를 실행하지 않는다.
    RawObjectMenu Menu() { return RawObjectMenu(pool,types,state,hooks); }
};
// 같은 몸체의 두 판본 고유478입력을 대조한다.
void Compare(std::string_view kind) {
    std::size_t count=0;
    // 기대값은 실제 PE에서 수집한 전체 효과 문자열이다.
    for (const auto& row:LoadFixture(NETSTORM_OBJECT_MENU_FIXTURE).rows) {
        if (row[1]!=kind) continue;CHECK(row.size()==17);ObjectScene scene(row[0]=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072,row);auto menu=scene.Menu();
        if (kind=="base") menu.Open(Sid{50});else if (kind=="damage") menu.OpenDamageable(Sid{50});else menu.AddSalvage(Sid{50},scene.input[PassedType],scene.input[Count]);
        const auto actual=scene.trace.empty() ? "-" : scene.trace;
        if (actual!=row.back()) std::fprintf(stderr,"객체 메뉴 원문 차이: %s %s owner%u raw%u type%u modes%u mut%u\n실제 %s\n원문 %s\n",row[0].c_str(),row[1].c_str(),scene.input[Owner],scene.input[RawType],scene.input[PassedType],scene.input[Modes],scene.input[Mutation],actual.c_str(),row.back().c_str());
        CHECK(actual==row.back());++count;
    }
    CHECK(count==478);
}
}
// 치트·도움말·SetOwner와 판본별 Player 번역/생성 순서를 실제 원문과 비교한다.
TEST_CASE(ObjectMenu_NativeBase478) { Compare("base"); }
// 가상+80에서 타입이 바뀌는 경우의 판본별 snapshot과 기본 메뉴 연결을 비교한다.
TEST_CASE(ObjectMenu_NativeDamageable478) { Compare("damage"); }
// 원문 회수/금지·음수환불·현재 설정·콜백 재진입·10.78의 특수 생략을 비교한다.
TEST_CASE(ObjectMenu_NativeSalvage478) { Compare("salvage"); }
// 미연결 Player 메뉴와 NULL 도움말을 빈 성공으로 처리하지 않아야 한다.
TEST_CASE(ObjectMenu_RequiresReachedDependencies) {
    const auto rows=LoadFixture(NETSTORM_OBJECT_MENU_FIXTURE).rows;ObjectScene scene(OriginalEdition::Patch1078,rows.at(1));scene.state.helpWord.reset();bool failed=false;
    try { scene.Menu().Open(Sid{50}); } catch (const std::logic_error&) { failed=true; }CHECK(failed);CHECK(scene.trace.empty());
    scene.state.helpWord="AboutType";scene.hooks.playerMenu={};failed=false;
    try { scene.Menu().Open(Sid{50}); } catch (const std::logic_error&) { failed=true; }CHECK(failed);CHECK(scene.trace.find("P,")==std::string::npos);
}
