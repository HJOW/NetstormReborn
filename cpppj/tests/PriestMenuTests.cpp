// 실제 PATCH 기계어의 사제 메뉴329입력·선택64입력과 getter512입력을 현재 공유 raw 저장소로 재생한다.
#include "RawSceneSupport.h"
#include "client/RawPriestMenu.h"
#include <algorithm>
#include <bit>
#include <limits>
#include <sstream>
using namespace netstorm::client;
using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 실제PE의메뉴타입순서와fixture mask대응이다.
constexpr std::array<std::uint32_t,10> kOrder{123,124,125,85,86,87,88,126,165,164};
// 문자열을바이트단위hex로기록하여NUL/스타일표식/구분자충돌을피한다.
std::string Hex(std::string_view value) {
    constexpr char digits[]="0123456789abcdef"; // 상위/하위4비트의표시문자다.
    std::string result;
    // 원본문자열의바이트순서를변경하지않는다.
    for (const unsigned char byte:value) { result+=digits[byte>>4];result+=digits[byte&15]; }return result;
}
// 테스트입력의정수필드를little endian으로원본슬롯에쓴다.
void PutMenu(std::span<std::uint8_t> raw,std::size_t offset,std::uint32_t value,std::size_t width=4) {
    // 명시입력의낮은바이트부터정해진폭만쓴다.
    for (std::size_t index=0;index<width;++index) raw[offset+index]=static_cast<std::uint8_t>(value>>(index*8));
}
// 번역/printf는이검사의명시외부경계다. 원본고정서식의%s/%d만처리한다.
std::string MenuFormat(std::string_view text,std::span<const PriestMenuArgument> arguments) {
    std::string result;std::size_t argument=0;
    // 현재서식의변환인자를순서대로소비한다.
    for (std::size_t index=0;index<text.size();++index) {
        if (text[index]!='%') { result+=text[index];continue; }++index;CHECK(index<text.size());CHECK(argument<arguments.size());
        if (text[index]=='s') result+=std::get<std::string>(arguments[argument++]);else if (text[index]=='d') result+=std::to_string(std::get<std::int32_t>(arguments[argument++]));else CHECK(false);
    }
    CHECK(argument==arguments.size());return result;
}
// 기계어oracle과같은명시입력/관찰경계를구성하되메뉴조건의기대결과는계산하지않는다.
struct MenuScene {
    SidPool pool{OriginalEdition::Patch1078,24000,true}; // 같은사제/내용물의실제Pool이다.
    std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(188); // 실제메뉴가참조할현재타입속성이다.
    std::vector<RiftTypeFrames> frames=std::vector<RiftTypeFrames>(188,RiftTypeFrames({FrameCode{65,0,0,0}})); // 현재프레임코드의독립입력이다.
    std::array<std::uint8_t,1620> players{}; // 같은9×180BYTE Player기록이다.
    SquidPostPopList excluded; // 0052f7e0의명시건설제외타입입력이다.
    ContainedFinderState contained; // 기존실제finder의공유타입전역이다.
    PriestMenuState state; // 원본모드/타입/명령전역의현재입력이다.
    PriestMenuHooks hooks; // 실제외부효과관찰및독립자료조회다.
    std::uint32_t blocked{},building{},immobile{},allowed{1023},contents{1},taken{},moneyCalls{}; // 사건/조회/수명순서를비교할입력과카운터다.
    std::int32_t amount{400}; // 원본정수비용표의명시입력이다.
    std::vector<std::uint32_t> money{std::bit_cast<std::uint32_t>(3000.0F)}; // 두SP조회는각각다른float비트를읽을수있다.
    std::string trace; // 실제메뉴/커서효과를원본관찰과같은순서로기록한다.
    // 실사용Pool에입력슬롯을확보하고UI효과만기록한다. 새게임실행/메뉴픽셀검사가아니다.
    MenuScene() {
        // 원본같은client번호공간에서raw입력50/Technology20/내용물21..24를확보한다.
        for (std::uint32_t index=5;index<=70;++index) CHECK(pool.Allocate(2).value==index);
        // 타입별설명/테마를명시입력으로준비한다.
        for (std::size_t type=0;type<types.size();++type) types[type].theme=static_cast<std::int32_t>(type%4);
        auto raw=pool.AllocatedBytes(Sid{50});raw[10]=158;raw[34]=1;
        hooks.hasEvent=[this](Sid,std::uint32_t event) { return (event==0x493 ? blocked : event==0x25d ? building : 0U)!=0; };
        hooks.immobile=[this](Sid) { return immobile!=0; };hooks.currentTechnology=[] { return 20U; };
        hooks.containedType=[this](Sid parent,std::uint32_t,std::uint32_t type) { return (parent==Sid{20} && (contents&1)) || (parent==Sid{50} && (contents&2)) ? type : 0U; };
        hooks.allowed=[this](std::uint32_t type) { const auto found=std::find(kOrder.begin(),kOrder.end(),type);CHECK(found!=kOrder.end());return (allowed&(1U<<static_cast<unsigned>(found-kOrder.begin())))!=0; };
        hooks.cost=[this](std::uint32_t) { return amount; };hooks.money=[this](std::uint32_t) { return std::bit_cast<float>(money[std::min<std::size_t>(moneyCalls++,money.size()-1)]); };
        hooks.description=[](std::uint32_t type) { return "Type"+std::to_string(type); };hooks.format=MenuFormat;
        hooks.beginFancy=[this] { Add("F"); };hooks.beginVertical=[this] { Add("V"); };hooks.title=[this](std::string_view text) { Add("T,"+Hex(text)); };
        hooks.takeMenu=[this] { return 100U+ ++taken; };
        hooks.submenu=[this](std::string_view text,std::uint32_t child,bool enabled) { Add("S,"+Hex(text)+','+std::to_string(child)+','+std::to_string(enabled)+",0"); };
        hooks.item=[this](std::string_view text,std::uint32_t command,std::uint32_t parameter,bool enabled) { Add("I,"+Hex(text)+','+std::to_string(command)+','+std::to_string(parameter)+",1,"+std::to_string(enabled)+",0"); };
        hooks.separator=[this] { Add("X"); };hooks.baseMenu=[this](Sid sid) { Add("B,"+std::to_string(sid.value)); };hooks.tell=[this](std::string_view key) { Add("D,"+Hex(key)); };
        hooks.createPreview=[this](Sid parent,std::uint32_t type) { Add("C,"+std::to_string(parent.value)+','+std::to_string(type));return Sid{70}; };
        hooks.takePreview=[this](Sid sid,std::uint32_t flags) { Add("P,"+std::to_string(sid.value)+','+std::to_string(flags)); };
    }
    // 예상결과가아닌실제호출효과를세미콜론순서로연결한다.
    void Add(std::string value) { if (!trace.empty()) trace+=';';trace+=value; }
    // 같은실제Technology종속체인으로독립theme비트입력을표현한다.
    void Themes(std::uint32_t mask) {
        PutMenu(pool.AllocatedBytes(Sid{20}),6,21,2);
        // 각원소마다실제flags4의기술Form한개를체인에넣는다.
        for (std::uint16_t index=0;index<4;++index) {
            const Sid sid{static_cast<std::uint16_t>(21+index)};auto raw=pool.AllocatedBytes(sid);raw[10]=6;
            PutMenu(raw,4,index<3 ? sid.value+1 : 0,2);PutMenu(raw,18,170+index,2);PutMenu(raw,20,(mask&(1U<<index)) ? 4 : 0,2);types[170+index].theme=index;
        }
    }
    // 현재참조를빌린메뉴처리기를생성하며저장소를복사하지않는다.
    RawPriestMenu Menu() { return RawPriestMenu(pool,types,frames,players,excluded,contained,state,hooks); }
    // fixture의명시모드를원본각전역에적용한다.
    void Modes(std::uint32_t modes) { state.workshopMode=(modes&1)!=0;state.special=(modes&2)!=0;state.editor=(modes&4)!=0;state.chargeBlocked=(modes&8)!=0; }
};
}
// 사제의전체메뉴구성은실제PE329개고유입력의효과전체문자열과비교한다.
TEST_CASE(PriestMenu_NativeOpen329) {
    std::size_t count=0;
    // 기계어가관찰한독립입력을모두재생한다.
    for (const auto& row:LoadFixture(NETSTORM_PRIESTMENU_FIXTURE).rows) {
        if (row[0]!="Open") continue;CHECK(row.size()==16);MenuScene scene;auto raw=scene.pool.AllocatedBytes(Sid{50});raw[34]=static_cast<std::uint8_t>(Number(row[1]));scene.state.localOwner=Number(row[2]);scene.blocked=Number(row[3]);scene.building=Number(row[4]);scene.immobile=Number(row[5]);
        scene.frames[158]=RiftTypeFrames({FrameCode{static_cast<std::uint8_t>(std::stoi(row[6])+65),0,0,0}});scene.Modes(Number(row[7]));const auto flags=Number(row[8]);
        // 네실제Player필드의유무를독립입력에서쓴다.
        for (std::size_t index=0;index<4;++index) { constexpr std::array<std::size_t,4> offsets{0x7c,0x84,0x8c,0x94};PutMenu(scene.players,static_cast<std::size_t>(raw[34])*180+offsets[index],(flags&(1U<<index)) ? 1 : 0); }
        scene.players[scene.state.localOwner*180+0x71]=3;scene.allowed=Number(row[9]);scene.Themes(Number(row[10]));scene.contents=Number(row[11]);scene.amount=std::stoi(row[12]);scene.money={Number(row[13])};
        const auto excluded=Number(row[14]);scene.excluded.entries.assign(kOrder.begin(),kOrder.end());scene.excluded.count=0;
        // 실제제외목록은활성타입만같은순서로보관한다.
        for (std::size_t index=0;index<kOrder.size();++index) if (excluded&(1U<<index)) scene.excluded.entries[scene.excluded.count++]=kOrder[index];
        scene.Menu().Open(Sid{50});CHECK((scene.trace.empty() ? "-" : scene.trace)==row[15]);++count;
    }
    CHECK(count==329);
}
// 두금액조회/NaN/float축소경계·워크샾테마관문의native64개를전수비교한다.
TEST_CASE(PriestMenu_NativeSelect64) {
    std::size_t count=0;
    // 선택결과는생성/안내/커서효과및실제조회횟수로검증한다.
    for (const auto& row:LoadFixture(NETSTORM_PRIESTMENU_FIXTURE).rows) {
        if (row[0]!="Select") continue;CHECK(row.size()==9);MenuScene scene;scene.amount=std::stoi(row[1]);scene.money={Number(row[2]),Number(row[3])};scene.Modes(Number(row[4]));scene.types[123].flags2=Number(row[5]) ? 0x4000 : 0;scene.Themes(Number(row[6])<<3);
        const auto result=scene.Menu().SelectBuild(123,Sid{50});CHECK(result==(row[8].find("P,")!=std::string::npos));CHECK(scene.moneyCalls==Number(row[7]));CHECK((scene.trace.empty() ? "-" : scene.trace)==row[8]);++count;
    }
    CHECK(count==64);
}
// 현재Technology원소/자체제외속성256조합과원본signed-char256값을대조한다.
TEST_CASE(PriestMenu_NativeThemeAndFrame512) {
    std::size_t themes=0,frames=0;
    // 각기대값은원본getter몸체의반환값이며이검사에서계산하지않는다.
    for (const auto& row:LoadFixture(NETSTORM_PRIESTMENU_FIXTURE).rows) {
        if (row[0]=="Theme") {
            MenuScene scene;scene.Themes(Number(row[1]));const auto banned=Number(row[2]);
            // 원본타입의flags2800을기술네종에독립으로쓴다.
            for (std::size_t index=0;index<4;++index) scene.types[170+index].flags2=(banned&(1U<<index)) ? 0x800 : 0;
            CHECK(scene.Menu().HasTheme(static_cast<std::int32_t>(Number(row[3])))==(Number(row[4])!=0));++themes;
        } else if (row[0]=="Frame") {
            MenuScene scene;scene.frames[158]=RiftTypeFrames({FrameCode{static_cast<std::uint8_t>(Number(row[1])),0,0,0}});
            CHECK(scene.Menu().FrameState(Sid{50})==std::stoi(row[2]));++frames;
        }
    }
    CHECK(themes==256 && frames==256);
}
// 같은실제기술체인의theme/flags4/flags2800와현재signed프레임코드를확인한다.
TEST_CASE(PriestMenu_SharedTechnologyAndSignedFrame) {
    MenuScene scene;scene.Themes(15);auto menu=scene.Menu();CHECK(menu.HasTheme(0) && menu.HasTheme(3));scene.types[173].flags2=0x800;CHECK(!menu.HasTheme(3));scene.types[173].flags2=0;
    PutMenu(scene.pool.AllocatedBytes(Sid{24}),20,0,2);CHECK(!menu.HasTheme(3));PutMenu(scene.pool.AllocatedBytes(Sid{24}),20,4,2);CHECK(menu.HasTheme(3));
    scene.frames[158]=RiftTypeFrames({FrameCode{255,0,0,0},FrameCode{'K',0,0,0}});CHECK(menu.FrameState(Sid{50})==-66);PutMenu(scene.pool.AllocatedBytes(Sid{50}),36,1);CHECK(menu.FrameState(Sid{50})==10);
}
