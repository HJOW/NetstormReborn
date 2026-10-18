// 이전1296행과 새 클래스2592행의 실제 비용/분기/호출 순서를 모두 대조한다.
#include "GumpRawSupport.h"
#include "RawSceneSupport.h"
#include "client/InventoryAboutMenu.h"
using namespace netstorm::client;
using namespace netstorm::test::gumpraw;
namespace {
constexpr std::uint32_t kMenu=0x12402000; // 독립 원본의 명시 메뉴/부모 경계 토큰이다.
// 비용/SP 입력과 기존4af010만 공급하며 새 About 계산의 기대값은 주입하지 않는다.
struct AboutScene {
    SidPool pool{OriginalEdition::Patch1078,24000,true};InventoryItemNode item;InventoryAboutState state; // 같은 현재 내용물/입력/전역이다.
    std::uint32_t owner{1},stock{},disabled{},last{},bridge{82},background{},backgroundFrame{},helpKind{};bool editor{}; // 기존 실제 Affordable 전역과 Help 입력이다.
    float x{16},y{11},money{};std::int32_t cost{};std::string trace; // 실제 비용/SP와 원문 효과 기록이다.
    std::unique_ptr<InventoryItemGump> inventory; // 기존132행에서 대조된 실제 비교 몸체다.
    // 원본과 같은 입력 슬롯과 정수 비용/SP를 설치한다.
    AboutScene(const std::vector<std::string>& input,bool withClass) {
        const std::size_t shift=withClass ? 1 : 0;Sid content{};
        // 원본의 고정 입력 SID50까지 실제 클라이언트 할당을 준비한다.
        do { content=pool.Allocate(2); } while (content.value<50);CHECK(content.value==50);
        auto raw=pool.AllocatedBytes(content);std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=withClass ? static_cast<std::uint8_t>(Number(input[0])) : 0;
        Put(raw,18,Number(input[shift]),2);Put(raw,20,Number(input[shift+1]),2);editor=Number(input[shift+2])!=0;cost=std::bit_cast<std::int32_t>(Number(input[shift+3]));money=std::bit_cast<float>(Number(input[shift+4]));helpKind=Number(input[shift+5]);item.content=50;item.object=314159;
        InventoryItemHooks h;h.costUnits=[this](auto) { return cost; };h.money=[this](auto who) { Event("Money,"+std::to_string(who));return money; };
        inventory=std::make_unique<InventoryItemGump>(pool,InventoryItemState{owner,editor,stock,disabled,last,bridge,x,y,background,backgroundFrame},std::move(h));
    }
    // 외부 효과를 현재 호출 순서대로 보존한다.
    void Event(std::string text) { if (!trace.empty()) trace+=';';trace+=text; }
    // 문자열 포인터 대신 원본 바이트16진수를 기록한다.
    static std::string TextHex(std::string_view text) { return Hex(std::span(reinterpret_cast<const std::uint8_t*>(text.data()),text.size())); }
    // 명시 메뉴/서식 경계만 기록하고 판단은 제품 코드에 맡긴다.
    InventoryAboutHooks Hooks() {
        InventoryAboutHooks h;h.begin=[this] { Event("Begin");return kMenu; };h.root=[] { return kMenu; };h.templateOwner=[] { return 2U; };h.editor=[this] { return editor; };
        h.cost=[this](auto) { return cost; };h.affordable=[this](auto sid) { return inventory->Affordable(sid)!=0; };
        h.helpWord=[this]()->std::optional<std::string> { if (!helpKind) return std::nullopt;return helpKind==2 ? "Help" : ""; };
        h.idle=[this](auto menu,bool on) { CHECK(menu==kMenu);Event("Idle,"+std::to_string(on)); };h.reference=[this](auto menu,auto object) { CHECK(menu==kMenu);Event("Object,"+std::to_string(object)); };
        h.describe=[this](auto kind,auto count,auto who,auto sid) { Event("Describe,"+std::to_string(kind)+','+std::to_string(count)+','+std::to_string(who)+','+std::to_string(sid)); };
        h.translate=[this](std::string_view text) { Event("Translate,"+TextHex(text));return std::string(text); };
        h.print=[](std::string_view pattern,std::int32_t value) { std::string result(pattern);const auto at=result.find("%d");CHECK(at!=std::string::npos && result.find("%d",at+2)==std::string::npos);return result.replace(at,2,std::to_string(value)); };
        h.header=[this](std::string_view text) { Event("Header,"+TextHex(text)); };
        h.entry=[this](std::optional<std::string_view> label,auto command,auto parameter,bool include,bool enabled,bool force) { Event("Entry,"+(label ? TextHex(*label) : "-")+','+std::to_string(command)+','+std::to_string(parameter)+','+std::to_string(include)+','+std::to_string(enabled)+','+std::to_string(force)); };
        h.attach=[this](auto p,auto parent) { Event("Attach,"+std::to_string(std::bit_cast<std::uint32_t>(p.x))+','+std::to_string(std::bit_cast<std::uint32_t>(p.y))+','+std::to_string(parent)); };
        h.sound=[this](std::string_view name) { Event("Sound,"+TextHex(name)+",0,0,0,0,0"); };return h;
    }
};
// 전체 입력을 재생하며 원문 결과와 풀/노드 불변을 확인한다.
void Native(const char* fixture,bool withClass,std::size_t expected) {
    std::size_t count{}; // 원문에서 실제로 비교한 입력 수다.
    // 기존 관측을 읽기만 하며 기대값이나 입력 행을 제거하지 않는다.
    for (const auto& row:netstorm::test::rawscene::LoadFixture(fixture).rows) {
        CHECK(row.size()==2);if (row.size()!=2) return;AboutScene s(Split(row[0],','),withClass);InventoryAboutMenu menu(s.pool,s.state,s.Hooks());const auto before=std::vector<std::uint8_t>(s.pool.Bytes().begin(),s.pool.Bytes().end());const auto node=Encode(s.item);
        CHECK(menu.Open(s.item,{17,-9})==1);if (s.trace!=row[1]) std::printf("생산 정보 원문 차이 %zu %s\n%s\n",count,row[0].c_str(),s.trace.c_str());CHECK(s.trace==row[1]);CHECK(std::equal(before.begin(),before.end(),s.pool.Bytes().begin()));CHECK(Encode(s.item)==node);++count;
    }
    CHECK(count==expected);
}
}
// 선행 단위에서 동결한 클래스0의 모든1296행이다.
TEST_CASE(InventoryAboutMenu_Native1296) { Native(NETSTORM_INVENTORY_ABOUT_FIXTURE,false,1296); }
// 실제 내용물 클래스6과 회수 제외122의2592행이다.
TEST_CASE(InventoryAboutMenu_NativeClasses2592) { Native(NETSTORM_INVENTORY_ABOUT_CLASS_FIXTURE,true,2592); }
