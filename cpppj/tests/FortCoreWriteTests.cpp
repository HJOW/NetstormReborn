// 제품전에동결한CoreWrite338+22와별도callbackPE관측의전체상태/하위순서/부분중단을대조한다.
#include "TestSupport.h"
#include "o/RawFortCoreWrite.h"
#include <algorithm>
#include <array>
#include <bit>
#include <fstream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
using namespace netstorm::o;
namespace {
// 원래빈칸과숫자열의순서를보존해fixture를분리한다.
std::vector<std::string> CoreWriteFields(std::string_view text,char separator) {
    std::vector<std::string> result;std::size_t first{};
    for (;;) { // 마지막빈칸도하나의열로남긴다.
        const auto next=text.find(separator,first);result.emplace_back(text.substr(first,next==text.npos ? text.npos : next-first));if (next==text.npos) return result;first=next+1;
    }
}
// 현재물리BYTE를원문과같은소문자hex로표현한다.
std::string CoreWriteHex(std::span<const std::uint8_t> bytes) {
    constexpr std::string_view digits="0123456789abcdef";std::string result; //BYTE당두글자의고정표다.
    for (const auto value:bytes) { result+=digits[value>>4];result+=digits[value&15U]; }return result; //사용하지않은Player기록도비교한다.
}
// rawseed의CPU읽기폭은원문fixture에보존하며C++직접ref읽기와는별도비교한다.
std::string CoreWriteWithoutSeed(std::string_view trace) {
    std::string result;
    for (const auto& event:CoreWriteFields(trace,';')) { //callback없는refload에인위장치를끼우지않는다.
        if (event.starts_with("seed:")) continue;if (!result.empty()) result+=';';result+=event;
    }return result;
}
// 원문emu_stop과같은명시외부중단이다.예상밖표준예외를삼키지않는다.
struct CoreWriteStop {};
// 최초관측의입력과명시callback효과만구성한다.기대열은실행뒤별도비교한다.
struct CoreWriteDevices {
    std::int32_t slot{};std::uint32_t manager{},metamap{},post{},editor{},owner{},seed{},sequence{},active{1}; //현재전역과명시난수입력이다.
    std::array<std::array<std::uint8_t,1620>,2> players{}; //Create뒤현재base가바뀌어도옛물리가살아있는두9×180BYTE다.
    std::map<std::string,std::uint32_t> calls; //경계별실제호출번호다.
    std::string mutation,trace;std::vector<std::uint8_t> output; //전체현재trace와일반write부분출력을보존한다.
    // fixture의입력열만읽고제품의분기나기대를계산하지않는다.
    explicit CoreWriteDevices(std::string_view input) {
        const auto fields=CoreWriteFields(input,',');if (fields.size()!=9) throw std::runtime_error("CoreWrite입력열수오류");slot=std::stoi(fields[0]);manager=std::stoul(fields[1]);metamap=std::stoul(fields[2]);post=std::stoul(fields[3]);editor=std::stoul(fields[4]);owner=std::stoul(fields[5]);seed=std::stoul(fields[6]);sequence=std::stoul(fields[7]);mutation=fields[8];
        for (std::size_t block=0;block<players.size();++block) { //최초PE에준비한동일물리입력만재구성한다.
            for (std::size_t index=0;index<9;++index) { //같은고정CoreData10BYTE만명시한다.
                for (std::size_t byte=0;byte<10;++byte) players[block][index*180+108+byte]=static_cast<std::uint8_t>(block*79+index*17+byte*13); //나머지PlayerBYTE는부모가접근하지않는다.
            }
        }
    }
    // callback이나직접현재getter의실제발생순서만기록한다.
    std::uint32_t Event(std::string_view kind,std::string text) { if (!trace.empty()) trace+=';';trace+=text;return ++calls[std::string(kind)]; }
    // 최초독립PE에준비한외부변경입력만동일하게공급한다.
    void Mutate(std::string_view boundary) {
        const auto split=mutation.find('-');if (split==mutation.npos || mutation.substr(0,split)!=boundary) return;const auto action=mutation.substr(split+1);
        if (action=="stop") throw CoreWriteStop{};
        if (action.starts_with("manager")) manager=std::stoul(action.substr(7));else if (action.starts_with("post")) post=std::stoul(action.substr(4));else if (action.starts_with("meta")) metamap=std::stoul(action.substr(4));else if (action.starts_with("editor")) editor=std::stoul(action.substr(6));else if (action.starts_with("owner")) owner=std::stoul(action.substr(5));else if (action=="playerB") active=2;
        else if (action=="core") {
            std::string kind(boundary);while (!kind.empty() && kind.back()>='0' && kind.back()<='9') kind.pop_back();
            for (auto& block:players) { //옛물리/현재물리를모두원문입력과같은값으로명시변경한다.
                for (std::size_t index=0;index<9;++index) { //각기록은서로다른10BYTE입력이다.
                    for (std::size_t byte=0;byte<10;++byte) block[index*180+108+byte]=static_cast<std::uint8_t>(calls[kind]*23+index*17+byte*11); //뒤필드는각child뒤실제로다시읽어야한다.
                }
            }
        }else if (action.starts_with("seed")) seed=std::stoul(action.substr(4),nullptr,action=="seed12340000" || action=="seed80000000" ? 16 : 10);else throw std::runtime_error("CoreWrite명시변경미해석");
    }
    // 같은현재Player전역/seedDWORD를빌리는실제getter다.
    FortCoreWriteGlobals Globals() {
        return {[this] { Event("metamap","metamap:4:"+std::to_string(metamap));return metamap; },[this] { Event("post","post:4:"+std::to_string(post));return post; },[this] { Event("editor","editor:4:"+std::to_string(editor));return editor; },[this] { Event("players","players:4:"+std::to_string(active));return std::span<std::uint8_t>(players.at(active-1)); },[this] { Event("owner","owner:4:"+std::to_string(owner));return owner; },[this]()->std::uint32_t& { return seed; }};
    }
    // 관측한원문외부하위의응답만제공한다.실제writer/SP복원을대신하는제품기본값은아니다.
    FortCoreWriteHooks Hooks() {
        FortCoreWriteHooks hooks;
        hooks.manager=[this](auto selected) { CHECK(selected==slot);Event("manager","manager:"+std::to_string(manager));return manager; };
        hooks.assertion=[this](auto reason) { const auto number=Event("assert","A:"+std::to_string(static_cast<std::uint32_t>(reason)));Mutate("assert"+std::to_string(number)); };
        hooks.create=[this](auto current,auto name) { const auto number=Event("create","C:"+std::to_string(current)+':'+std::string(name));Mutate("create"+std::to_string(number));return 101U; };
        hooks.refresh=[this] { const auto number=Event("refresh","F");Mutate("refresh"+std::to_string(number)); };
        hooks.write=[this](auto stream,auto bytes) { output.insert(output.end(),bytes.begin(),bytes.end());const auto number=Event("write","W:"+std::to_string(stream)+':'+std::to_string(bytes.size())+':'+CoreWriteHex(bytes));Mutate("write"+std::to_string(number)); };
        hooks.prepareRandom=[this] { const auto number=Event("prepare","P");Mutate("prepare"+std::to_string(number)); };
        hooks.nextRandom=[this](auto maximum) { constexpr std::array<std::uint32_t,3> zeros{0,65536,0x12348001U};const auto result=sequence==0 ? zeros.at(std::min<std::size_t>(calls["random"],2)) : sequence==1 ? 0xffffffffU : 1U;const auto number=Event("random","R:"+std::to_string(maximum)+':'+std::to_string(result));Mutate("random"+std::to_string(number));return result; }; //각원문반환의low16변환은제품이수행한다.
        hooks.sp=[this](auto stream,float value) { const auto number=Event("sp","S:"+std::to_string(stream)+':'+std::to_string(std::bit_cast<std::uint32_t>(value)));Mutate("sp"+std::to_string(number)); };return hooks;
    }
    // 전체현재global의원시DWORD와현재Playerbase번호를표현한다.
    std::string State() const { return std::to_string(manager)+','+std::to_string(metamap)+','+std::to_string(post)+','+std::to_string(editor)+','+std::to_string(owner)+','+std::to_string(seed)+','+std::to_string(active); }
    // 사용하지않은Player와이전할당의CoreData도모두대조한다.
    std::string Cores() const {
        std::string result;
        for (const auto& block:players) { //두살아있는할당의원시순서를보존한다.
            for (std::size_t index=0;index<9;++index) result+=CoreWriteHex(std::span<const std::uint8_t>(block).subspan(index*180+108,10)); //같은기록의10BYTE전체다.
        }return result;
    }
};
// 원문fixture한개를제품실행뒤에만기대열과비교한다.
void CompareCoreWriteFixture(const char* path,std::size_t expected,std::size_t expectedStops) {
    std::ifstream input(path);if (!input) throw std::runtime_error("CoreWrite최초PEfixture없음");std::string line;std::size_t compared{},stops{};std::set<std::pair<std::string,std::string>> seen;
    while (std::getline(input,line)) { //실제관측의두정밀도/모든부분중단을빠짐없이대조한다.
        if (!line.empty() && line.back()=='\r') line.pop_back();if (line.empty() || line.front()=='#') continue;const auto fields=CoreWriteFields(line,'\t');if (fields.size()!=7) throw std::runtime_error("CoreWritefixture열수오류");CHECK(seen.emplace(fields[0],fields[1]).second);CoreWriteDevices devices(fields[1]);bool completed=true;
        try { WriteFortCoreData(devices.slot,devices.Globals(),devices.Hooks()); }catch (const CoreWriteStop&) { completed=false;++stops; }
        CHECK(completed==(fields[2]=="normal"));CHECK(devices.State()==fields[3]);CHECK(devices.Cores()==fields[4]);CHECK((devices.output.empty() ? std::string("-") : CoreWriteHex(devices.output))==fields[5]);CHECK(devices.trace==CoreWriteWithoutSeed(fields[6]));++compared;
    }CHECK(compared==expected && stops==expectedStops);
}
}
// 최초169입력의두정밀도338관측을전체원시상태와대조한다.
TEST_CASE(FortCoreWrite_IndependentNative338) { CompareCoreWriteFixture(NETSTORM_FORT_CORE_WRITE_FIXTURE,338,16); }
// 별도고정한세번째진단/상위비영DWORD22관측을대조한다.
TEST_CASE(FortCoreWrite_IndependentNativeExtra22) { CompareCoreWriteFixture(NETSTORM_FORT_CORE_WRITE_EXTRA_FIXTURE,22,2); }
// 실제1기준으로선택한Create/refresh/preparecallback의별도36관측을대조한다.
TEST_CASE(FortCoreWrite_IndependentNativeCallbacks36) { CompareCoreWriteFixture(NETSTORM_FORT_CORE_WRITE_CALLBACKS_FIXTURE,36,6); }
// 자연관문은미선택장치를요구하지않고실제선택시에는그위치에서부분효과와함께거부한다.
TEST_CASE(FortCoreWrite_SelectedMissingDeviceBoundaries) {
    CoreWriteDevices natural("1,0,1,1,0,1,0,0,none");auto skip=natural.Hooks();skip.create={};skip.refresh={};skip.write={};skip.prepareRandom={};skip.nextRandom={};skip.sp={};auto globals=natural.Globals();globals.editor={};globals.players={};globals.playerOwner={};globals.seed={};WriteFortCoreData(1,globals,skip);CHECK(natural.calls["assert"]==2 && natural.calls["create"]==0 && natural.calls["editor"]==0 && !natural.seed);
    for (std::uint32_t stage=0;stage<7;++stage) { //선택하위만순서별제거하여앞선게임효과를확인한다.
        CoreWriteDevices selected("1,11,0,0,0,1,0,0,none");auto hooks=selected.Hooks();auto current=selected.Globals();if (stage==0) hooks.create={};else if (stage==1) hooks.refresh={};else if (stage==2) hooks.write={};else if (stage==3) current.seed={};else if (stage==4) hooks.prepareRandom={};else if (stage==5) hooks.nextRandom={};else hooks.sp={};bool rejected{};
        try { WriteFortCoreData(1,current,hooks); }catch (const std::logic_error&) { rejected=true; }CHECK(rejected);CHECK(selected.calls["create"]==(stage ? 1U : 0U));CHECK(selected.calls["refresh"]==(stage>=2 ? 1U : 0U));CHECK(selected.output.size()==(stage<=2 ? 0U : stage<6 ? 10U : 12U));CHECK(selected.seed==(stage==6 ? 32769U : 0U));
    }
}
// 고정Player주소의물리폭검사는자연ret보다앞당기지않고선행refresh부분효과도보존한다.
TEST_CASE(FortCoreWrite_PhysicalReadAfterSelectedRefresh) {
    CoreWriteDevices devices("1,11,0,0,1,1,0,0,none");auto globals=devices.Globals();globals.players=[] { return std::span<std::uint8_t>{}; };WriteFortCoreData(1,globals,devices.Hooks());CHECK(!devices.calls["create"]);
    devices.editor=0;bool rejected{};try { WriteFortCoreData(1,globals,devices.Hooks()); }catch (const std::out_of_range&) { rejected=true; }CHECK(rejected && devices.calls["create"]==1 && devices.calls["refresh"]==1 && devices.output.empty() && !devices.seed);
}
