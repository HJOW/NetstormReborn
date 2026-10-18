// 원본 Factory 메뉴 관측을 하나의 명시 입력 풀과 실제 새 몸체로 재생한다.
#include "ConstructionSupport.h"
#include "client/RawFactoryMenu.h"
#include "client/RawObjectMenuOpen.h"
#include <sstream>
using namespace netstorm::client;
using namespace netstorm::test::construction;
namespace {
constexpr std::array<std::uint32_t,8> kFlags{0x800,0x8000000,0x10000,0x4000000,0x20000000,0x40000,0x20000,0}; // 독립 원본 관측의 명시 TYPE 플래그 입력이다.
// UTF8/스타일 문자열의 바이트를 원본 관측 형식으로 보존한다.
std::string TextHex(std::string_view text) { return Hex({reinterpret_cast<const std::uint8_t*>(text.data()),text.size()}); }
// 빈 문자열 표식과 원본 hex 바이트를 읽는다.
std::string TextBytes(std::string_view hex) {
    if (hex=="-") return {};std::string text;
    // 각 두 자리의 원본 바이트만 읽는다.
    for (std::size_t i=0;i<hex.size();i+=2) text.push_back(static_cast<char>(std::stoul(std::string(hex.substr(i,2)),nullptr,16)));return text;
}
// 입력 목록을 해석하며 생산/지식의 기대 순서를 계산하지 않는다.
std::vector<std::uint32_t> List(const std::string& text) { return text=="-" ? std::vector<std::uint32_t>{} : Inputs(text); }
// 번역/printf는 원본 관측과 같은 명시 외부 경계다.
std::string Print(std::string_view text,std::span<const PriestMenuArgument> args) {
    std::string result;std::size_t arg{};
    // 고정 서식의 문자열/정수만 원래 순서로 소비한다.
    for (std::size_t i=0;i<text.size();++i) { if (text[i]!='%') { result+=text[i];continue; }++i;CHECK(arg<args.size());if (text[i]=='s') result+=std::get<std::string>(args[arg++]);else { CHECK(text[i]=='d');result+=std::to_string(std::get<std::int32_t>(args[arg++])); } }
    CHECK(arg==args.size());return result;
}
// 실제 풀/검색기와 현재 TYPE/지식 조회 입력만 준비한다.
struct FactoryMenuScene {
    SidPool pool{OriginalEdition::Patch1078,24000,true}; // 원본50번 부모와100번 이후 Form을 담는다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); // 명시 TYPE 필드다.
    ContainedFinderState contained;SquidPostPopList excluded;FactoryMenuState state; // 실제 검색 정책/제외 목록/원본 기본 전역이다.
    FactoryMenuHooks hooks;std::string trace;std::uint32_t taken{}; // 실제 UI 호출의 순서만 기록한다.
    std::vector<std::uint32_t> tech,blocked,production; // 원본 조회 경계에 공급한 독립 목록이다.
    std::int32_t cost{};float money{};std::uint32_t owner{},profile{};bool editor{}; // 현재 원문 비용/SP/소유/이름 입력이다.
    // 명시 raw 입력을 만들며 게임 진행이나 기대 출력은 만들지 않는다.
    explicit FactoryMenuScene(const std::vector<std::string>& row) {
        // 실제 원본과 같은 client SID를 순서대로 확보한다.
        for (std::uint16_t i=5;i<151;++i) CHECK(pool.Allocate(2).value==i);
        const auto type=Number(row[1]),level=Number(row[5]),theme=Number(row[6]);owner=Number(row[3]);editor=Number(row[7])!=0;cost=static_cast<std::int32_t>(Number(row[8]));money=std::bit_cast<float>(Number(row[9]));profile=Number(row[14]);tech=List(row[10]);blocked=List(row[12]);production=List(row[13]);
        auto raw=InputRaw(pool,Sid{50});std::fill(raw.begin(),raw.end(),std::uint8_t{});Put(raw,0,type==85 ? 0x509148U : 0x509210U);raw[10]=static_cast<std::uint8_t>(type);raw[11]=static_cast<std::uint8_t>(level<<5);raw[34]=static_cast<std::uint8_t>(Number(row[2]));raw[40]=static_cast<std::uint8_t>(Number(row[4]));
        const auto products=List(row[11]);Put(raw,6,products.empty() ? 0U : 100U,2);
        // 중복 생산도 각각의 실제 Form을 만들고 next 순서만 입력한다.
        for (std::size_t i=0;i<products.size();++i) { auto form=InputRaw(pool,Sid{static_cast<std::uint16_t>(100+i)});std::fill(form.begin(),form.end(),std::uint8_t{});form[10]=6;Put(form,4,i+1<products.size() ? static_cast<std::uint32_t>(101+i) : 0U,2);Put(form,18,products[i],2);Put(form,20,0x10,2); }
        // 현재 TYPE 입력은 원본 관측의 번호/테마/플래그/설명과 같다.
        for (std::size_t i=0;i<types.size();++i) { types[i].theme=static_cast<std::int32_t>(i==type ? theme : i%4);types[i].flags2=kFlags[i%kFlags.size()];types[i].description=profile ? (i%2 ? "Same" : "same") : "Type"+std::to_string(i); }
        excluded.entries=blocked;excluded.count=static_cast<std::uint32_t>(blocked.size());hooks.format=Print;
        hooks.localOwner=[this] { return owner; };hooks.editor=[this] { return editor; };hooks.currentTechnology=[] { return 9U; };hooks.technologyType=[this](Sid parent,std::uint32_t index) { CHECK(parent.value==9);return index<tech.size() ? tech[index] : 0U; };
        hooks.capacity=[this](Sid sid) { return int(pool.Slot(sid)[11]>>5)+2; };hooks.level=[this](Sid sid) { return int(pool.Slot(sid)[11]>>5); };
        hooks.allowed=[this](auto type) { return std::find(blocked.begin(),blocked.end(),type)==blocked.end(); };hooks.inProduction=[this](auto type) { return std::find(production.begin(),production.end(),type)!=production.end(); };
        hooks.cost=[this](auto) { return cost; };hooks.money=[this](auto) { return money; };hooks.description=[this](auto type) { return types.at(type).description; };hooks.mana=[](auto) { return std::string{}; };hooks.helpWord=[] { return std::string("Help"); };
        hooks.beginFancy=[this] { Add("F"); };hooks.title=[this](auto text) { Add("T,"+TextHex(text)); };hooks.header=[this](auto text) { Add("H,"+TextHex(text)); };
        hooks.takeMenu=[this] { return 100U+(++taken); };hooks.submenu=[this](auto text,auto child) { Add("S,"+TextHex(text)+","+std::to_string(child)); };
        hooks.item=[this](auto text,auto command,ObjectMenuParameter parameter,bool include,bool enabled) { const auto value=std::holds_alternative<std::string>(parameter) ? "str:"+TextHex(std::get<std::string>(parameter)) : std::to_string(std::get<std::uint32_t>(parameter));Add("I,"+TextHex(text)+","+std::to_string(command)+","+value+","+std::to_string(include)+","+std::to_string(enabled)+",0"); };
        hooks.baseMenu=[this](Sid sid) { Add("B,"+std::to_string(sid.value)); };
    }
    // 각 실제 호출 사이에만 구분자를 넣고 빈 호출 목록은 별도로 비교한다.
    void Add(std::string value) { if (!trace.empty()) trace+=';';trace+=value; }
};
}
// 두 원본 함수 체인의 전체 작성 순서/인자/동률 정렬을 독립 원문과 비교한다.
TEST_CASE(FactoryMenu_Native39FullOpen) {
    std::size_t count{};
    // 원본 출력은 계산하지 않고 같은 입력의 실제 새 몸체만 실행한다.
    for (const auto& row:LoadFixture(NETSTORM_FACTORY_MENU_FIXTURE).rows) if (row[0]=="Open") {
        CHECK(row.size()==17);FactoryMenuScene scene(row);RawFactoryMenu menu(scene.pool,scene.types,scene.contained,scene.excluded,scene.state,scene.hooks);ObjectMenuOpenHooks hooks;
        hooks.beginFancy=scene.hooks.beginFancy;hooks.summary=[&](Sid sid) { scene.Add("Q,"+std::to_string(sid.value)); };hooks.populate=[&](Sid sid) { menu.Open(sid); };hooks.attach=[&](Sid sid,int x,int y) { scene.Add("A,"+std::to_string(sid.value)+","+std::to_string(static_cast<std::uint32_t>(x))+","+std::to_string(static_cast<std::uint32_t>(y))); };
        RawObjectMenuOpen open(scene.pool,scene.types,std::move(hooks));const auto result=open.OpenFactory(Sid{50},InputEvent{0,37,-11});CHECK(result==Number(row[15]));const auto trace=scene.trace.empty() ? "-" : scene.trace;
        if (trace!=row[16]) std::printf("Factory 메뉴 원문%zu 불일치\n실제:%s\n원본:%s\n",count,trace.c_str(),row[16].c_str());CHECK(trace==row[16]);++count;
    }
    CHECK(count==39);
}
// 점/활성 개수/알 수 없는ASCII/0슬롯/음수 슬롯을 실제 원문과 대조한다.
TEST_CASE(FactoryMenu_Native32Mana5Slots) {
    std::size_t mana{},slots{};
    // 전체76행 중 문자열 함수의 독립 결과37개만 재생한다.
    for (const auto& row:LoadFixture(NETSTORM_FACTORY_MENU_FIXTURE).rows) {
        if (row[0]=="Mana") { const auto result=FormatFactoryMana(TextBytes(row[1]),std::stoi(row[2]));CHECK(result==TextBytes(row[3]));++mana; }
        if (row[0]=="Slots") {
            std::vector<std::string> input{"Open","85","1","1","2","0","3","0","800","0","-","-","-","-","0"};FactoryMenuScene scene(input);RawFactoryMenu menu(scene.pool,scene.types,scene.contained,scene.excluded,scene.state,scene.hooks);CHECK(menu.Slots(std::stoi(row[1]))==TextBytes(row[2]));++slots;
        }
    }
    CHECK(mana==32 && slots==5);
}
