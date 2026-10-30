// 원문현재사건610행의일반명령효과·같은현재자료·부분중단을대조한다.
#include "TestSupport.h"
#include "client/GameMenuActions.h"
#include <array>
#include <bit>
#include <fstream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

using namespace netstorm::client;
namespace {
constexpr std::size_t kMenuRows=610,kMenuColumns=21; // 두제어어에서독립관측한고정자료수다.
constexpr std::uint32_t kMenuArena=0x01010000; // 독립외부장치의고정주소다.
constexpr std::array<std::uint32_t,16> kMenuGlobals{0x5c85a4,0x594fb8,0x594fc8,0x594fa4,0x595344,0x540c70,0x59a8ac,0x59a8b0,
    0x55b4b0,0x565dd4,0x55a714,0x5e18a4,0x59534c,0x5c8464,0x55a70c,0x5e189c}; // fixture의현재전역순서다.
constexpr std::array<std::pair<GameMenuActionCommand,std::uint32_t>,11> kMenuCommands{{
    {GameMenuActionCommand::AddSp,0x02000061}, // 원문54287c의 추가 명령이며 기존610행의 기대값은 바꾸지 않는다.
    {GameMenuActionCommand::QuitApp,0x02000001},{GameMenuActionCommand::RequestQuitApp,0x02000094},
    {GameMenuActionCommand::SoundVolume,0x020000e6},{GameMenuActionCommand::MusicVolume,0x020000e7},
    {GameMenuActionCommand::Pause,0x020000cf},{GameMenuActionCommand::ViewHome,0x020000ae},
    {GameMenuActionCommand::ViewYourPriest,0x020000b8},{GameMenuActionCommand::HideBuildings,0x020000f1},
    {GameMenuActionCommand::ReloadTheams,0x02000123},{GameMenuActionCommand::GetFullDeck,0x020000cd}}}; // 보호PE에서읽은서로다른불변명령ID다.
struct MenuExplicitStop {}; // 명시효과중단을정상메뉴반환과구별한다.

// 마지막빈필드까지원문구분자사이의바이트를보존한다.
std::vector<std::string> MenuSplit(std::string_view text,char separator) {
    std::vector<std::string> output;std::size_t begin{};
    for (;;) { // 순서와빈문자열을바꾸지않는다.
        const auto end=text.find(separator,begin);output.emplace_back(text.substr(begin,end==text.npos ? text.npos : end-begin));if (end==text.npos) return output;begin=end+1;
    }
}
// 부호입력도원문하위DWORD를보존한다.
std::uint32_t MenuNumber(std::string_view text) { return static_cast<std::uint32_t>(std::stoll(std::string(text))); }
// 실제메모리바이트를기대값계산없이읽는다.
std::vector<std::uint8_t> MenuBytes(std::string_view text) {
    if (text.size()%2) throw std::runtime_error("메뉴장치16진폭");std::vector<std::uint8_t> output;
    for (std::size_t i=0;i<text.size();i+=2) output.push_back(static_cast<std::uint8_t>(std::stoul(std::string(text.substr(i,2)),nullptr,16))); // 두문자가한원시바이트다.
    return output;
}
// 현재실제바이트를fixture표기와같이관측한다.
std::string MenuHex(std::span<const std::uint8_t> bytes) {
    constexpr char digits[]="0123456789abcdef"; // 원문메모리표기다.
    std::string output;
    for (auto byte:bytes) { output+=digits[byte>>4];output+=digits[byte&15]; } // 메모리순서대로보존한다.
    return output;
}

// 앞10열의자료만소유하며하위장치효과를실제입력대로공급한다.
struct MenuScene {
    std::array<std::uint32_t,16> globals{}; // 처리기와장치가공유하는현재전역이다.
    std::vector<std::uint8_t> devices,queue; // 실제Player/Form/SID자료와명시큐장치다.
    std::uint32_t read{},write{}; // 같은고리장치의현재커서다.
    std::map<std::string,std::size_t> calls; // 명시경계별1기반호출번호다.
    std::vector<std::array<std::string,3>> effects; // 입력이명시한현재DWORD변경이다.
    std::set<std::string> stops; // 부분중단을요청한경계호출번호다.
    std::string trace; // 제품본체가요청한효과만순서대로관측한다.

    // 관측뒤열을입력으로사용하지않고독립현재상태를준비한다.
    explicit MenuScene(const std::vector<std::string>& row):devices(MenuBytes(row[5])),queue(480),read(MenuNumber(row[6])),write((read+MenuNumber(row[7]))%40) {
        const auto values=MenuSplit(row[4],':');if (values.size()!=globals.size()) throw std::runtime_error("메뉴전역열폭");
        for (std::size_t i=0;i<values.size();++i) globals[i]=MenuNumber(values[i]); // 원시순서대로설치한다.
        for (std::uint32_t i=0;i<40;++i) { // 비활성슬롯까지독립원문입력표식을설치한다.
            Store(queue,i*12,0x2000300+i);Store(queue,i*12+4,100+i);Store(queue,i*12+8,200+i);
        }
        if (row[8]!="-") for (const auto& item:MenuSplit(row[8],';')) { // 입력변경만명시형식으로읽는다.
            const auto values_=MenuSplit(item,'/');if (values_.size()!=3) throw std::runtime_error("메뉴효과열폭");effects.push_back({values_[0],values_[1],values_[2]});
        }
        if (row[9]!="-") for (const auto& item:MenuSplit(row[9],';')) stops.insert(item); // 명시부분중단이다.
    }
    // 실제같은바이트에비정렬DWORD를쓴다.
    static void Store(std::span<std::uint8_t> bytes,std::size_t offset,std::uint32_t value) {
        if (offset>bytes.size() || bytes.size()-offset<4) throw std::out_of_range("메뉴장치쓰기범위");
        for (unsigned i=0;i<4;++i) bytes[offset+i]=static_cast<std::uint8_t>(value>>(8*i)); // 원문littleendian이다.
    }
    // 비정렬실제DWORD를현재바이트에서읽는다.
    static std::uint32_t Load(std::span<const std::uint8_t> bytes,std::size_t offset) {
        if (offset>bytes.size() || bytes.size()-offset<4) throw std::out_of_range("메뉴장치읽기범위");
        return bytes[offset]|(std::uint32_t(bytes[offset+1])<<8)|(std::uint32_t(bytes[offset+2])<<16)|(std::uint32_t(bytes[offset+3])<<24);
    }
    // 실제주소에대응하는같은현재전역필드다.
    std::uint32_t& Global(std::uint32_t address) {
        for (std::size_t i=0;i<kMenuGlobals.size();++i) if (kMenuGlobals[i]==address) return globals[i]; // 현재필드의참조만반환한다.
        throw std::out_of_range("메뉴전역주소");
    }
    // 현재포인터가가리키는장치메모리의남은폭이다.
    std::span<const std::uint8_t> Memory(std::uint32_t address) const {
        if (address<kMenuArena || address-kMenuArena>=devices.size()) throw std::out_of_range("메뉴장치주소");return std::span<const std::uint8_t>(devices).subspan(address-kMenuArena);
    }
    // 같은경계에배정한변경과중단을호출순서대로공급한다.
    void Event(std::string name,std::vector<std::string> values={},bool body=true) {
        if (body) { if (!trace.empty()) trace+=';';trace+=name;for (const auto& value:values) trace+=':'+value; } // 실제효과인자만기록한다.
        const auto trigger=name+std::to_string(++calls[name]);
        for (const auto& effect:effects) { // 경계복귀전같은실제자료를변경한다.
            if (effect[0]!=trigger) continue;const auto address=MenuNumber(effect[1]),value=MenuNumber(effect[2]);
            if (address>=kMenuArena && address+4<=kMenuArena+devices.size()) Store(devices,address-kMenuArena,value);else Global(address)=value;
        }
        if (stops.contains(trigger)) throw MenuExplicitStop{};
    }
    // 실제현재SID/목록/자료를조회하는메뉴외부장치를제공한다.
    GameMenuActionHooks Hooks() {
        GameMenuActionHooks hooks;
        hooks.command=[](auto command) { for (const auto& [name,id]:kMenuCommands) if (name==command) return id;throw std::out_of_range("메뉴명령이름"); }; // 불변원문이름표다.
        hooks.queue=[this](auto command,auto first,auto second) {
            Event("queue",{std::to_string(command),std::to_string(first),std::to_string(second)});
            // 큐는본체밖명시장치다. 실제452fe0과같이포화시물리슬롯을쓰고write만복구한다.
            Store(queue,write*12,command);Store(queue,write*12+4,first);Store(queue,write*12+8,second);const auto next=(write+1)%40;if (next!=read) write=next;
        };
        hooks.tell=[this](auto text) { Event("tell",{std::string(text)}); };
        hooks.config=[this](auto text) { Event("config",{std::string(text)}); };
        hooks.sound=[this](auto name,auto a,auto b,auto c,auto d,auto e) { Event("sound",{std::string(name),std::to_string(a),std::to_string(b),std::to_string(c),std::to_string(d),std::to_string(e)}); };
        hooks.paused=[this] { Event("paused",{std::to_string(Global(0x55b4b0))});return std::bit_cast<std::int32_t>(Global(0x55b4b0))>0; };
        hooks.status=[this](auto a,auto b,auto c,auto d,auto text) { Event("status",{std::to_string(a),std::to_string(b),std::to_string(c),std::to_string(d),std::string(text)}); };
        hooks.cancelCursor=[this] { Event("cancel"); };
        hooks.pauseClock=[this] { ++Global(0x55b4b0);Event("pause"); };
        hooks.resumeClock=[this] { auto& depth=Global(0x55b4b0);if (depth) --depth;Event("resume"); };
        hooks.muteSound=[this] { Event("mute"); };
        hooks.resumeSound=[this] { Event("unmute"); };
        hooks.player=[this](auto owner) { const auto address=Global(0x59534c)+owner*0xb4U;return address ? Memory(address) : std::span<const std::uint8_t>{}; };
        hooks.centerObject=[this](auto sid) { Event("center",{std::to_string(sid)}); };
        hooks.viewPriest=[this](auto owner) { Event("priest",{std::to_string(owner)}); };
        hooks.reloadCount=[this](auto list) { return std::bit_cast<std::int32_t>(Global(list==GameMenuReloadList::First ? 0x55a714 : 0x5e18a4)); };
        hooks.reloadSid=[this](auto list,auto index) { return Load(Memory(Global(list==GameMenuReloadList::First ? 0x55a70c : 0x5e189c)),index*4); };
        hooks.form=[this](auto sid) { return Memory(Global(0x5c8464)+sid*50); };
        hooks.reloadAt=[this](auto x,auto y,auto zero) { Event("reload",{std::to_string(x),std::to_string(y),std::to_string(zero)}); };
        hooks.assertion=[this](auto line) { Event("assert",{std::to_string(line)}); };
        return hooks;
    }
    // 제품/장치가공유하는최종전역을원문과같은순서로관측한다.
    std::string Globals() const {
        std::string output;
        for (auto value:globals) { if (!output.empty()) output+=':';output+=std::to_string(value); } // 원시DWORD순서다.
        return output;
    }
};
}

// Guide/Gump는명시장치의선행효과만공급하며제품UserInput전체복원검사로확대하지않는다.
TEST_CASE(GameMenuActions_Native610CurrentBodyAndPartialEffects) {
    std::ifstream input(NETSTORM_GAME_MENU_ACTIONS_FIXTURE);CHECK(input.good());if (!input) return;
    std::string line;std::size_t rows{},partial{},quit{},gateSkipped{};
    while (std::getline(input,line)) { // 관측열을장치초기화/분기선택에사용하지않는다.
        if (!line.empty() && line.back()=='\r') line.pop_back();if (line.empty() || line.starts_with('#')) continue;
        const auto row=MenuSplit(line,'\t');CHECK(row.size()==kMenuColumns);if (row.size()!=kMenuColumns) continue;++rows;
        MenuScene scene(row);const auto eventFields=MenuSplit(row[1],':');if (eventFields.size()!=3) throw std::runtime_error("메뉴사건폭");
        const auto event=MenuNumber(eventFields[0]),first=MenuNumber(eventFields[1]),second=MenuNumber(eventFields[2]);
        GameMenuActions actions({[&](auto address) { return scene.Global(address); },scene.Global(0x59a8ac),scene.Global(0x59a8b0)},scene.Hooks());
        bool completed=true;std::optional<std::uint32_t> result;
        try {
            scene.Event("guide",{},false);scene.Event("mission",{},false);
            if (MenuNumber(row[2])==0) {
                scene.Event("gump",{},false);
                if (MenuNumber(row[3])==0) result=actions.Handle(event,first,second);else ++gateSkipped;
            } else ++gateSkipped;
        } catch (const MenuExplicitStop&) { completed=false; }
        if (!completed) ++partial;
        if (result && *result==1) ++quit;
        CHECK(completed==(row[10]=="1"));CHECK((result && *result==1)==(row[11]=="1"));
        CHECK(scene.Globals()==row[12]);CHECK(MenuHex(scene.devices)==row[13]);CHECK(scene.read==MenuNumber(row[14]));CHECK(scene.write==MenuNumber(row[15]));CHECK(MenuHex(scene.queue)==row[16]);
        CHECK((scene.trace.empty() ? "-" : scene.trace)==row[17]);CHECK(MenuNumber(row[20])>0);
        // 외부tail/기계스택은제품subset의반환값이아니다. 실제관측자료의프레임종료만검사한다.
        if (completed) CHECK(std::stoll(row[19])==(row[11]=="1" ? 12296 : 0));
        if ((event&0x03ffffffU)==0x020000beU || (event&0x03ffffffU)==0x020003feU) CHECK(!result); // 부모MenuJustClosed/비대상은미처리다.
    }
    CHECK(rows==kMenuRows);CHECK(partial==26);CHECK(quit==16);CHECK(gateSkipped==270);
}

// 범위밖명령/뗌은부작용장치를요구하지않으며선택한실제효과누락은성공으로대체하지않는다.
TEST_CASE(GameMenuActions_UnhandledReleaseAndRequiredEffects) {
    std::uint32_t hide{},redraw{};GameMenuActionHooks hooks;
    hooks.command=[](auto command) { for (const auto& [name,id]:kMenuCommands) if (name==command) return id;throw std::out_of_range("메뉴명령"); }; // 실제ID조회만설치한다.
    GameMenuActions actions({[](auto) { return 0U; },hide,redraw},hooks);
    CHECK(!actions.Handle(0x020000be,1,2));CHECK(!actions.Handle(0x020003fe,1,2));
    const auto released=actions.Handle(0x82000001,0,99);CHECK(released && *released==0);
    bool missing{};try { static_cast<void>(actions.Handle(0x02000001,0,99)); } catch (const std::runtime_error&) { missing=true; }CHECK(missing);
    CHECK(hide==0 && redraw==0);
}
