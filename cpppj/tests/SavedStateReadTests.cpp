// 제품전에동결한772BYTE/SEH542관측중지원물리538관측과실제파일읽기를대조한다.
#include "TestSupport.h"
#include "o/RawSavedStateRead.h"
#include "o/BaseFile.h"
#include <algorithm>
#include <bit>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>
using namespace netstorm::o;
namespace {
// 원시입력/관측열의빈문자열까지그대로유지한다.
std::vector<std::string> SavedFields(std::string_view text,char separator) {
    std::vector<std::string> output;std::size_t first{};
    for (;;) { // 마지막빈열도한번반환한다.
        const auto next=text.find(separator,first);output.emplace_back(text.substr(first,next==text.npos ? text.npos : next-first));if (next==text.npos) return output;first=next+1;
    }
}
// 원문관측과같은littleendianDWORD입력만쓴다.
void SavedPut(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value) { for (std::size_t index=0;index<4;++index) bytes[at+index]=static_cast<std::uint8_t>(value>>(index*8)); } //해당네BYTE밖을초기화하지않는다.
// 사용하지않은padding까지전체원시출력을비교한다.
std::string SavedHex(std::span<const std::uint8_t> bytes) {
    constexpr std::string_view digits="0123456789abcdef";std::string result; //한BYTE당두글자의고정표다.
    for (const auto byte:bytes) { result+=digits[byte>>4];result+=digits[byte&15U]; }return result; //실제할당의꼬리도생략하지않는다.
}
// 명시초기stack392BYTE입력이다.원문결과가아닌동일초기물리만만든다.
std::array<std::uint8_t,kSavedStateReadScratchBytes> SavedScratch(std::uint32_t profile) {
    std::array<std::uint8_t,kSavedStateReadScratchBytes> bytes{};bytes.fill(profile ? 0xa5 : 0);SavedPut(bytes,4,1);SavedPut(bytes,8,0x22334455);SavedPut(bytes,0,0x201);SavedPut(bytes,0x3c,0x80000000);SavedPut(bytes,0x40,profile ? 0xa004e : 0);
    for (const auto& [at,name,width]:std::array<std::tuple<std::size_t,std::string_view,std::size_t>,3>{{{0x48,"StackA",32},{0x68,"StackB",32},{0x88,"StackFort",256}}}) { //두NULpadding도원문과같은명시입력이다.
        std::fill_n(bytes.begin()+at,width,std::uint8_t{});std::copy(name.begin(),name.end(),bytes.begin()+at);
    }return bytes;
}
// 저장파일의고정레이아웃입력만작성한다.새parser나제품기대생성기가아니다.
std::vector<std::uint8_t> SavedImage(std::uint32_t profile,std::uint32_t participants,std::uint32_t tail,std::uint32_t time) {
    std::vector<std::uint8_t> bytes(976);SavedPut(bytes,0,profile==5 ? 0x12345678 : 0xa004e);const std::array<std::string,6> names{"SavedBattle",".Hidden","Has.Dot","",std::string(255,'A'),"Short"};std::copy(names.at(profile).begin(),names.at(profile).end(),bytes.begin()+4);SavedPut(bytes,260,1);SavedPut(bytes,264,time);SavedPut(bytes,268,0x8899aabb);
    for (std::uint32_t index=0;index<8;++index) { //8개86BYTE레코드를서로다른명시입력으로준비한다.
        const auto at=272+index*86;const auto first="New"+std::to_string(index+1),second="Address"+std::to_string(index+1);SavedPut(bytes,at,index+1);SavedPut(bytes,at+4,100+index);std::copy(first.begin(),first.end(),bytes.begin()+at+8);std::copy(second.begin(),second.end(),bytes.begin()+at+38);SavedPut(bytes,at+68,index<participants ? 0x41 : 0x201);SavedPut(bytes,at+72,200+index);
        for (std::uint32_t byte=0;byte<10;++byte) bytes[at+76+byte]=static_cast<std::uint8_t>(index*17+byte*13); //마지막Core10BYTE는별도값이다.
    }SavedPut(bytes,960,0x11223344);SavedPut(bytes,964,0x55667788);SavedPut(bytes,968,0x99aabbcc);SavedPut(bytes,972,tail);return bytes;
}
// 최초PE의명시하위중단과같은표식이다.
struct SavedStop {};
// 최초도구의입력/하위응답만제공한다.출력기대열은제품실행후에읽는다.
struct SavedDevices {
    std::uint32_t present{},profile{},participantInput{},tailInput{},timeInput{},length{},initial{},owner{7},identity{0x11223344},activeBase{1},cleanup{}; //파일입력과현재같은출력전역이다.
    std::array<std::uint32_t,3> tails{0xabababab,0xabababab,0xabababab}; //직접read대상의이전상위BYTE입력이다.
    std::array<std::array<std::uint8_t,1620>,2> players{};std::array<std::uint8_t,144> cache{};std::array<std::uint8_t,256> handle{}; //두Player물리와다른9×16장부/이전이름이다.
    std::array<std::uint8_t,kSavedStateReadScratchBytes> scratch{};std::array<std::uint8_t,256> path{}; //고정경로주소와명시초기stack가수명끝까지유지된다.
    std::vector<std::uint8_t> data;std::size_t position{};std::map<std::string,std::uint32_t> calls; //파일할당과선택된외부호출순번이다.
    std::string mutation,pending{"BeforeSaved"},trace;bool stopped{}; //입력외부변경과실제부분중단/trace다.
    // fixture입력열만해석해원시상태를준비한다.
    explicit SavedDevices(std::string_view input) {
        const auto fields=SavedFields(input,',');if (fields.size()!=8) throw std::runtime_error("SavedState입력열수오류");present=std::stoul(fields[0]);profile=std::stoul(fields[1]);participantInput=std::stoul(fields[2]);tailInput=std::stoul(fields[3]);timeInput=std::stoul(fields[4]);length=std::stoul(fields[5]);initial=std::stoul(fields[6]);mutation=fields[7];scratch=SavedScratch(initial);data=SavedImage(profile,participantInput,tailInput,timeInput);data.resize(std::min<std::size_t>(length,data.size()));cache.fill(0x91);const std::string before="BeforeHandle",file="!state.dat";std::copy(before.begin(),before.end(),handle.begin());std::copy(file.begin(),file.end(),path.begin());
        for (std::size_t block=0;block<players.size();++block) { //원문에서복사할이전Player이름을미리입력한다.
            players[block].fill(static_cast<std::uint8_t>(0x31+block));
            for (std::uint32_t player=0;player<9;++player) { const auto name="Old"+std::to_string(block)+std::to_string(player);std::copy(name.begin(),name.end(),players[block].begin()+player*180+12);players[block][player*180+12+name.size()]=0; } //나머지BYTE는초기값그대로다.
        }
    }
    // 실제발화한현재getter/하위순서만한trace에기록한다.
    std::uint32_t Event(std::string_view kind,std::string text) { if (!trace.empty()) trace+=';';trace+=text;return ++calls[std::string(kind)]; }
    // 원문도구의외부효과입력만동일하게공급한다.
    void Mutate(std::string_view boundary) {
        const auto split=mutation.find('-');if (split==mutation.npos || mutation.substr(0,split)!=boundary) return;const auto action=mutation.substr(split+1);
        if (action=="stop") { stopped=true;throw SavedStop{}; }if (action=="cursor976") position=976;else if (action=="owner8") owner=8;else if (action=="baseB") activeBase=2;else if (action=="baseA") activeBase=1;else if (action=="flags0") SavedPut(scratch,0,0);else if (action=="time0") SavedPut(scratch,0x3c,0);else if (action=="tail1") SavedPut(scratch,0x34,1);else if (action=="tail0") SavedPut(scratch,0x34,0);else if (!action.starts_with("short")) throw std::runtime_error("SavedState입력변경미해석");
    }
    // 같은전역DWORD와현재Player물리를빌린다.읽기용사본을만들지않는다.
    SavedStateReadGlobals Globals() {
        SavedStateReadGlobals result;result.owner=[this]()->std::uint32_t& { return owner; };result.subscriber=[this]()->std::uint32_t& { return identity; };result.previousHandle=[this] { return std::span<std::uint8_t>(handle); };result.players=[this] { Event("base","B:"+std::to_string(activeBase));return std::span<std::uint8_t>(players.at(activeBase-1)); };
        for (std::size_t index=0;index<tails.size();++index) result.tail[index]=[this,index]()->std::uint32_t& { return tails[index]; };return result; //각고정전역의직접대상이다.
    }
    // Native의요청대상주소를같은stackoffset/global번호로정규화한다.
    std::string Target(std::span<std::uint8_t> target) {
        if (target.data()==reinterpret_cast<std::uint8_t*>(&owner)) return "G0";
        for (std::size_t index=0;index<tails.size();++index) if (target.data()==reinterpret_cast<std::uint8_t*>(&tails[index])) return "G"+std::to_string(index+1); //다른전역의사본에read하면일치하지않는다.
        if (target.data()<scratch.data() || target.data()+target.size()>scratch.data()+scratch.size()) throw std::out_of_range("SavedState명시read대상주소");return "S"+std::to_string(target.data()-scratch.data());
    }
    // 실제일반파일읽기는부분폭만복사하고나머지를비워주지않는다.
    std::uint32_t Read(std::span<std::uint8_t> target,std::uint32_t limit) {
        CHECK(limit==0xffffffffU);const auto boundary="read"+std::to_string(calls["read"]+1);auto count=std::min(target.size(),position<data.size() ? data.size()-position : 0U);if (mutation==boundary+"-short0") count=0;else if (mutation==boundary+"-short2") count=std::min<std::size_t>(count,2);std::copy_n(data.begin()+std::min(position,data.size()),count,target.begin());position+=count;const auto number=Event("read","R:"+Target(target)+':'+std::to_string(target.size())+':'+std::to_string(count));Mutate("read"+std::to_string(number));return static_cast<std::uint32_t>(count);
    }
    // 최초PE의외부경계응답이다.Core초기화/9×16쓰기의제품완료를주장하지않는다.
    SavedStateReadHooks Hooks() {
        SavedStateReadHooks hooks;hooks.path=[this] { const auto number=Event("path","P");Mutate("path"+std::to_string(number));return std::span<const std::uint8_t>(path); };hooks.exists=[this](auto name) { CHECK(name=="!state.dat");const auto number=Event("exists","E:!state.dat");Mutate("exists"+std::to_string(number));return static_cast<std::int32_t>(present); };
        hooks.open=[this](auto name,auto object,auto first,auto second) { CHECK(name=="!state.dat" && object.size()==24 && !first && !second);const auto number=Event("open","O:!state.dat:0:0");Mutate("open"+std::to_string(number));return 101U; };
        hooks.read=[this](auto file,auto target,auto limit) { CHECK(file==101);return Read(target,limit); };hooks.close=[this](auto file,auto object) { CHECK(file==101 && object.size()==24);if (stopped) cleanup=1;const auto number=Event("close","D");Mutate("close"+std::to_string(number)); };
        hooks.pending=[this](auto name) { pending=name;const auto number=Event("pending","N:"+SavedHex({reinterpret_cast<const std::uint8_t*>(name.data()),name.size()}));Mutate("pending"+std::to_string(number)); };
        hooks.initializeCore=[this](auto core) { CHECK(core.size()==10);std::fill(core.begin(),core.end(),std::uint8_t{});const auto number=Event("init","I");Mutate("init"+std::to_string(number)); };
        hooks.metadata=[this](auto number,auto subscriber,auto core) { if (number>=9) throw std::out_of_range("SavedState명시9기록캐시");SavedPut(cache,number*16,subscriber);std::copy(core.begin(),core.end(),cache.begin()+number*16+4);const auto current=Event("metadata","M:"+std::to_string(number)+':'+std::to_string(subscriber)+':'+SavedHex(core));Mutate("metadata"+std::to_string(current)); };return hooks;
    }
    // 실제현재global/파일cursor/예외해제상태를그대로표현한다.
    std::string State() const { return std::to_string(owner)+','+std::to_string(identity)+','+std::to_string(tails[0])+','+std::to_string(tails[1])+','+std::to_string(tails[2])+','+std::to_string(activeBase)+','+std::to_string(position)+','+std::to_string(cleanup); }
    // 명시두Player/캐시padding/이름전체의모든BYTE를비교한다.
    std::string Raw() const { return SavedHex(handle)+SavedHex(cache)+SavedHex(players[0])+SavedHex(players[1]); }
};
}
// 지원물리의모든538관측은전역/Player/stack/파일해제와실제전체trace를대조한다.
TEST_CASE(SavedStateRead_IndependentNative538AndPhysical4) {
    std::ifstream input(NETSTORM_SAVED_STATE_READ_FIXTURE);if (!input) throw std::runtime_error("SavedState최초PEfixture없음");std::string line;std::size_t compared{},stops{},physical{},cleanups{};std::set<std::pair<std::string,std::string>> seen;
    while (std::getline(input,line)) { // 호스트물리밖4관측은원문분석자료로유지하고정상제품성공으로비교하지않는다.
        if (!line.empty() && line.back()=='\r') line.pop_back();if (line.empty() || line.front()=='#') continue;const auto row=SavedFields(line,'\t');if (row.size()!=9) throw std::runtime_error("SavedStatefixture열수오류");CHECK(seen.emplace(row[0],row[1]).second);if (row[2]=="fault") { ++physical;continue; }SavedDevices devices(row[1]);bool completed=true;std::uint32_t result{};
        try { result=std::bit_cast<std::uint32_t>(ReadSavedState(devices.scratch,devices.Globals(),devices.Hooks())); }catch (const SavedStop&) { completed=false;++stops; }
        CHECK(completed==(row[2]=="normal"));if (completed) CHECK(result==std::stoul(row[3]));CHECK(devices.State()==row[4]);CHECK((devices.pending.empty() ? std::string("-") : SavedHex({reinterpret_cast<const std::uint8_t*>(devices.pending.data()),devices.pending.size()}))==row[5]);CHECK(devices.Raw()==row[6]);CHECK(SavedHex(devices.scratch)==row[7]);CHECK(devices.trace==row[8]);++compared;cleanups+=devices.cleanup;
    }CHECK(compared==538 && stops==32 && physical==4 && cleanups==24);
}
// 없음은미선택장치를요구하지않고선택된누락하위는원문SEH위치의해제뒤거부된다.
TEST_CASE(SavedStateRead_SelectedMissingDevicesAndCleanup) {
    SavedDevices absent("0,0,2,0,1,976,0,none");auto none=absent.Hooks();none.open={};none.read={};none.close={};none.pending={};none.initializeCore={};none.metadata={};CHECK(!ReadSavedState({}, {},none) && !absent.calls["open"]);
    for (std::uint32_t stage=0;stage<4;++stage) { // 서로다른실제하위의부분prefix/파일해제를확인한다.
        SavedDevices devices("1,0,2,0,1,976,0,none");auto hooks=devices.Hooks();if (!stage) hooks.read={};else if (stage==1) hooks.pending={};else if (stage==2) hooks.initializeCore={};else hooks.metadata={};bool rejected{};try { ReadSavedState(devices.scratch,devices.Globals(),hooks); }catch (const std::logic_error&) { rejected=true; }CHECK(rejected && devices.calls["open"]==1 && devices.calls["close"]==1);CHECK(devices.calls["read"]==(stage==0 ? 0U : stage==1 ? 2U : stage==2 ? 5U : 12U));
    }
}
// 원문안전계약밖의Player번호와NUL없는물리끝은명시실패이고앞선pending/읽기와파일해제는남는다.
TEST_CASE(SavedStateRead_PhysicalContractsPreservePrefix) {
    SavedDevices devices("1,0,2,0,1,976,1,none");SavedPut(devices.data,272,9);bool rejected{};try { ReadSavedState(devices.scratch,devices.Globals(),devices.Hooks()); }catch (const std::out_of_range&) { rejected=true; }CHECK(rejected && devices.pending=="SavedBattle" && devices.calls["metadata"]==0 && devices.calls["close"]==1 && devices.position==358);
    SavedDevices missingNul("1,0,2,0,1,976,0,none");std::fill(missingNul.data.begin()+4,missingNul.data.begin()+260,std::uint8_t{'A'});rejected=false;try { ReadSavedState(missingNul.scratch,missingNul.Globals(),missingNul.Hooks()); }catch (const std::out_of_range&) { rejected=true; }CHECK(rejected && missingNul.pending=="BeforeSaved" && missingNul.calls["close"]==1 && missingNul.position==260);
}
// 기존BaseFileSystem이읽은실제작업파일할당을같은readcursor에빌려정상/짧은꼬리입력을검증한다.
TEST_CASE(SavedStateRead_ActualBaseFileBytesAndShortTail) {
    const auto root=std::filesystem::path(__FILE__).parent_path().parent_path().parent_path()/"extracted/cpp-saved-state-read-20261024-isolated/test-data"/std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());std::filesystem::create_directories(root);BaseFileSystem files(root);
    for (const auto length:{976U,972U}) { //끝검증DWORD가아예없어도원문은초기0을사용한다.
        auto bytes=SavedImage(0,2,0,0x80000000U);bytes.resize(length);{ std::ofstream output(root/"!state.dat",std::ios::binary);output.write(reinterpret_cast<const char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()));if (!output) throw std::runtime_error("소유SavedState입력파일쓰기실패"); }
        SavedDevices devices("1,0,2,0,2147483648,976,0,none");auto hooks=devices.Hooks();const auto opened=hooks.open;hooks.exists=[&](auto name) { return std::filesystem::is_regular_file(root/std::string(name)) ? 1 : 0; };hooks.open=[&](auto name,auto object,auto first,auto second) { devices.data=files.Read(name);return opened(name,object,first,second); };
        const auto value=ReadSavedState(devices.scratch,devices.Globals(),hooks);CHECK(std::bit_cast<std::uint32_t>(value)==0x80000000U && devices.pending=="SavedBattle" && devices.calls["metadata"]==8 && devices.calls["close"]==1 && devices.position==length && ReadFileBytes(root/"!state.dat")==bytes);
    }
}
