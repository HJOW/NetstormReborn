// 제품보다먼저고정한State부모294관측의전체직접전역/Player/TYPE/현재Core/호출순서를대조한다.
#include "TestSupport.h"
#include "o/RawStateWorldCleanup.h"
#include "o/RawPlayerCoreData.h"
#include <algorithm>
#include <array>
#include <bit>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <map>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {
using namespace netstorm::o;
constexpr std::array<std::uint32_t,3> kCleanupPlayers{0x12200000,0x12202000,0x12204000}; //격리PE의세실제Player물리식별자다.
// 원시입력/TSV를마지막빈칸까지보존한다.
std::vector<std::string> CleanupSplit(std::string_view text,char separator) {
    std::vector<std::string> values;std::size_t start=0; //아직소비하지않은위치와각칸이다.
    for (;;) { //원시표의마지막칸도버리지않는다.
        const auto end=text.find(separator,start);values.emplace_back(text.substr(start,end==text.npos?text.size()-start:end-start)); //현재원시한칸이다.
        if (end==text.npos) return values;
        start=end+1;
    }
}
// 실제전체BYTE를C문자열/NUL절단없이소문자hex로비교한다.
std::string CleanupHex(std::span<const std::uint8_t> bytes) {
    constexpr std::string_view digits="0123456789abcdef";std::string text; //고정원문자릿수와출력이다.
    for (const auto value:bytes) { text+=digits[value>>4];text+=digits[value&15]; } //미작성전역/PlayerBYTE도생략하지않는다.
    return text;
}
// 외부호출주소식별자만고정소문자hex로출력한다.
std::string CleanupAddress(std::uint32_t address) {
    std::array<char,16> text{};std::snprintf(text.data(),text.size(),"%x",address);return text.data(); //host포인터를만들지않는다.
}
// 원문하위에서명시중단한입력만표시한다.
struct CleanupStop {};
// 실제현재저장소와명시하위장치만구성하고부모분기/기대를계산하지않는다.
struct CleanupDevice {
    std::vector<std::pair<std::uint32_t,std::size_t>> fields; //TSV메타데이터의관측물리폭이며기대효과가아니다.
    std::map<std::uint32_t,std::uint32_t> words; //실제DWORD저장소이며Core자식도같은참조를직접쓴다.
    std::map<std::uint32_t,std::uint64_t> wides; //원문double필드의그대로인64비트입력이다.
    std::array<std::array<std::uint8_t,1620>,3> playerBytes{}; //바뀐base와예전current대상을구별하는세물리다.
    std::array<std::uint8_t,180> headers{}; //실제9×20BYTETemplate표다.
    std::array<std::uint8_t,10> cached{},external{}; //실제lastKnown/외부Core입력이며사본동기화하지않는다.
    std::span<std::uint8_t> players,current; //현재Playerbase와Core포인터의실제주소슬롯이다.
    float typeDefault{std::bit_cast<float>(0x3f123456U)}; //NativeTYPE+c4의실제단일float저장소다.
    std::uint32_t reason{},pause{}; //부모인자와실제정지깊이장치입력이다.
    std::string mutation; //특정하위의현재값교체/중단만기술한다.
    std::map<std::string,std::uint32_t> calls;std::vector<std::string> trace; //실제호출번호/순서다.

    // 실제Player뷰의주소를격리PE의opaque주소식별자로정규화한다.
    std::uint32_t PlayerToken() const {
        for (std::size_t index=0;index<playerBytes.size();++index) if (players.data()==playerBytes[index].data()) return kCleanupPlayers[index]; //동일물리주소만대응한다.
        throw std::runtime_error("State검사현재Player뷰미등록");
    }
    // 주소역할의current는별도DWORD사본없이같은span의주소로관측한다.
    std::string Pointer() const {
        if (current.data()==nullptr) return "0";
        if (current.data()==cached.data()) return "CACHED";
        if (current.data()==external.data()) return "EXTERNAL";
        for (std::size_t index=0;index<playerBytes.size();++index) for (std::size_t owner=0;owner<9;++owner) if (current.data()==playerBytes[index].data()+owner*180+108) return "P"+std::to_string(index)+":"+std::to_string(owner); //내용으로현재포인터를추정하지않는다.
        throw std::runtime_error("State검사현재Core뷰미등록");
    }
    // 같은현재DWORD를조회한다.Playerbase는실제span슬롯을opaque식별자로표현하는장치경계다.
    std::uint32_t Read(std::uint32_t address) const { return address==0x59534c?PlayerToken():words.at(address); }
    // 같은실제필드/현재Player뷰슬롯에만쓰며범위밖새전역을만들지않는다.
    void Write(std::uint32_t address,std::uint32_t value) {
        if (address==0x59534c) {
            for (std::size_t index=0;index<playerBytes.size();++index) if (value==kCleanupPlayers[index]) { players=playerBytes[index];return; } //실제저장소를다시선택한다.
            throw std::out_of_range("State검사Player토큰미등록");
        }
        words.at(address)=value;
    }
    // littleendian원시입력의DWORD/QWORD를구성한다.부모출력기대는계산하지않는다.
    static std::uint64_t InputBits(std::uint32_t address,std::size_t offset,std::size_t width,std::uint32_t seed) {
        std::uint64_t value=0; //최대8BYTE의명시초기입력이다.
        for (std::size_t byte=0;byte<width;++byte) value|=std::uint64_t(static_cast<std::uint8_t>(seed+address+(offset+byte)*13))<<(byte*8); //원시fixture의입력패턴과같다.
        return value;
    }
    // 전체물리를먼저명시초기화한뒤Native와같은입력전역만덮는다.
    CleanupDevice(const std::vector<std::string>& input,std::vector<std::pair<std::uint32_t,std::size_t>> ranges):fields(std::move(ranges)) {
        if (input.size()!=15) throw std::runtime_error("State원문입력칸수");
        const auto seed=static_cast<std::uint32_t>(std::stoul(input[0]));reason=static_cast<std::uint32_t>(std::stoul(input[1]));mutation=input[14];pause=static_cast<std::uint32_t>(std::stoul(input[11])); //원시입력이며출력을역산하지않는다.
        for (const auto& [address,width]:fields) { //기록의폭만참조해같은초기물리를구성한다.
            if (width==4) words.emplace(address,static_cast<std::uint32_t>(InputBits(address,0,4,seed)));
            else if (width==8) wides.emplace(address,InputBits(address,0,8,seed));
            else if (width==10) { CHECK(address==0x5953f0);for (std::size_t byte=0;byte<10;++byte) cached[byte]=static_cast<std::uint8_t>(seed+address+byte*13); } //현재캐시10BYTE다.
            else if (width==88) { for (std::size_t offset=0;offset<88;offset+=4) words.emplace(address+static_cast<std::uint32_t>(offset),static_cast<std::uint32_t>(InputBits(address,offset,4,seed))); } //원문22DWORD의연속입력이다.
            else throw std::runtime_error("State원문미지원관측폭");
        }
        for (std::size_t index=0;index<playerBytes.size();++index) for (std::size_t byte=0;byte<1620;++byte) playerBytes[index][byte]=static_cast<std::uint8_t>(seed+index*47+byte*17); //모든PlayerBYTE를정의한다.
        for (std::size_t byte=0;byte<10;++byte) external[byte]=static_cast<std::uint8_t>(seed+byte*7+31); //쓰기없는외부source다.
        players=playerBytes[0];current=players.subspan(180+108,10);
        const auto modes=static_cast<std::uint32_t>(std::stoul(input[3])); //각모드의독립입력비트다.
        Write(0x5c85c0,static_cast<std::uint32_t>(std::stoul(input[2])));Write(0x594fb8,modes&1U);Write(0x594fbc,(modes>>1)&1U);Write(0x594fc8,(modes>>2)&1U);
        const std::array<std::pair<std::uint32_t,std::size_t>,8> numbers{{{0x5c85a4,4},{0x594fa4,5},{0x5c8758,6},{0x595344,7},{0x540c70,8},{0x540cac,9},{0x594fa0,13},{0x540bdc,12}}}; //실제DWORD와원시입력칸의대응이다.
        for (const auto& [address,index]:numbers) Write(address,static_cast<std::uint32_t>(std::stoll(input[index]))); //음수defaultSp도원문DWORD비트그대로쓴다.
        Write(0x541240,152);Write(0x5949d4,seed?0xffffffffU:7U);
        for (std::size_t byte=0;byte<headers.size();++byte) headers[byte]=static_cast<std::uint8_t>(seed+byte*3); //전체header인접필드를보존한다.
        const auto mask=std::stoul(input[10]); //현재각header의존재입력이다.
        for (std::uint32_t owner=0;owner<9;++owner) { const auto value=(mask&(1U<<owner))?0x11110000U+owner:0U;for (std::size_t byte=0;byte<4;++byte) headers[owner*20+byte]=static_cast<std::uint8_t>(value>>(byte*8)); } //첫DWORD만실제manager로설정한다.
    }
    // 각하위의진입인자를기록하고같은주소별호출번호를반환한다.
    std::string Event(std::uint32_t address,std::initializer_list<std::uint32_t> arguments={}) {
        const auto key=CleanupAddress(address);std::string text="C:"+key+":"; //Native와동일한호출식별자다.
        bool first=true;for (const auto argument:arguments) { if (!first) text+=',';first=false;text+=std::to_string(argument); } //인자순서를보존한다.
        trace.push_back(std::move(text));return key+"."+std::to_string(++calls[key]);
    }
    // 명시하위에서만현재값을바꾸며부모전역접근에숨은callback을추가하지않는다.
    void Mutate(const std::string& boundary) {
        const auto separator=mutation.find('-');if (separator==mutation.npos || mutation.substr(0,separator)!=boundary) return; //선택한한경계만바꾼다.
        const auto action=mutation.substr(separator+1); //외부장치의실제효과다.
        if (action=="stop") throw CleanupStop{};
        if (action=="client1") Write(0x5c85c0,1);
        else if (action=="template2") Write(0x540cac,2);
        else if (action=="battle1") Write(0x594fbc,1);
        else if (action=="editor1") Write(0x5c85a4,1);
        else if (action=="owner8") Write(0x540c70,8);
        else if (action=="playerB") players=playerBytes[1];
        else if (action=="known0") Write(0x595344,0);
        else if (action=="known1") Write(0x595344,1);
        else if (action=="knownEditor") { Write(0x595344,1);Write(0x5c85a4,1); }
        else if (action=="defaultNegative") Write(0x540bdc,0x80000000U);
        else if (action=="state29") Write(0x5c8758,29);
        else if (action=="coreExternal") current=external;
        else if (action=="counterMax") Write(0x5949d4,0xffffffff);
        else throw std::runtime_error("State원문미해석변경");
    }
    // 실제저장소에접근하는순수provider와선택된현재TYPE참조를공급한다.
    StateWorldCleanupGlobals Globals() {
        StateWorldCleanupGlobals globals; //snapshot사본을실제전역이라고공급하지않는다.
        globals.read32=[this](auto address) { return Read(address); };globals.write32=[this](auto address,auto value) { Write(address,value); };
        globals.write16=[this](auto address,auto value) { Write(address,(Read(address)&0xffff0000U)|value); };globals.write64Bits=[this](auto address,auto bits) { wides.at(address)=bits; };
        globals.players=[this] { return players; };
        globals.templateManager=[this](std::uint32_t slot) { if (slot>=9) throw std::out_of_range("State검사header물리");std::uint32_t value=0;for (std::size_t byte=0;byte<4;++byte) value|=std::uint32_t(headers[slot*20+byte])<<(byte*8);return value; }; //순수현재header첫DWORD다.
        globals.typeDefault=[this](auto type)->float& { const auto boundary=Event(0x49a840,{type});Mutate(boundary);return typeDefault; };return globals;
    }
    // 모든미복원자식은명시장치다.Core선택만동결된실제C++자식으로합성한다.
    StateWorldCleanupHooks Hooks() {
        StateWorldCleanupHooks hooks; //빈생산성공callback을자동공급하는코드가아니다.
        hooks.effect=[this](auto effect) {
            const auto address=static_cast<std::uint32_t>(effect); //Native자식주소식별자다.
            if (effect==StateWorldCleanupEffect::ResumeGame) --pause;
            if (effect==StateWorldCleanupEffect::InitializePlayers) { trace.push_back("BEFOREPLAYER:"+Pointer());players=playerBytes[2]; } //명시하위장치의현재base교체이며실제할당기복원주장이아니다.
            const auto boundary=(effect==StateWorldCleanupEffect::Reset568b60)?Event(address,{0x568b60}):(effect==StateWorldCleanupEffect::Reset54c8a0)?Event(address,{0x54c8a0}):(effect==StateWorldCleanupEffect::ResetZone)?Event(address,{0x5c86c0}):Event(address);Mutate(boundary);
        };
        hooks.disconnect=[this](auto owner) { const auto boundary=Event(0x4e2940,{owner});Mutate(boundary); };
        hooks.assertion=[this](auto line) { const auto boundary=Event(0x4e0620,{line});Mutate(boundary); };
        hooks.pauseDepth=[this] { const auto value=pause;const auto boundary=Event(0x460de0,{value});Mutate(boundary);return value; };
        hooks.saveFort=[this](auto slot) { const auto boundary=Event(0x4bf9a0,{slot});Mutate(boundary); };hooks.saveBattle=[this](auto slot) { const auto boundary=Event(0x4bf880,{slot});Mutate(boundary); };
        hooks.resetAi=[this](auto owner) { const auto boundary=Event(0x413ae0,{owner});Mutate(boundary); };hooks.resetPriest=[this](auto owner) { const auto boundary=Event(0x492240,{owner});Mutate(boundary); };
        hooks.resetOtherPlayer=[this](auto owner,auto value) { const auto boundary=Event(0x447810,{owner,value});Mutate(boundary); };
        hooks.setSp=[this](auto owner,float value) { const auto boundary=Event(0x40edb0,{owner,std::bit_cast<std::uint32_t>(value)});Mutate(boundary); };
        hooks.selectCore=[this](auto owner,auto selected) {
            const auto boundary=Event(0x48f3b0,{owner,selected});Mutate(boundary);
            SelectPlayerCoreData({words.at(0x540cac),words.at(0x540c70),words.at(0x595344),words.at(0x595348),words.at(0x594fc8),current,cached,players},owner,selected); //같은실제전역/캐시/Player뷰를동결자식이직접쓴다.
        };
        hooks.resetPool=[this](auto size) { const auto boundary=Event(0x4af170,{size});Mutate(boundary); };
        hooks.wallSeconds=[this] { const auto boundary=Event(0x460d70);Mutate(boundary);return 123.5; };return hooks;
    }
    // 원문이기록한각범위의전체실제BYTE를littleendian으로출력한다.
    std::string RawGlobals() const {
        std::string text; //실제필드별폭을그대로연결한다.
        for (const auto& [address,width]:fields) {
            if (width==10) { text+=CleanupHex(cached);continue; }
            for (std::size_t offset=0;offset<width;) { //연속22DWORD도같은개별현재필드를읽는다.
                const auto count=width==8?8U:4U;const auto value=width==8?wides.at(address):std::uint64_t(Read(address+static_cast<std::uint32_t>(offset)));std::array<std::uint8_t,8> bytes{}; //현재필드의그대로인비트다.
                for (std::size_t byte=0;byte<count;++byte) bytes[byte]=static_cast<std::uint8_t>(value>>(byte*8)); //host구조체패딩을원문이라고비교하지않는다.
                text+=CleanupHex(std::span(bytes).first(count));offset+=count;
            }
        }
        return text;
    }
    // 실제하위진입순서를그대로원시표형식으로반환한다.
    std::string Trace() const { std::string text;for (const auto& event:trace) { if (!text.empty()) text+=';';text+=event; }return text; } //종류별정렬을하지않는다.
};
// 첫차이를행/칸/길이/앞부분으로기록하고전체원시는fixture와장치에그대로보존한다.
void CheckCleanup(std::size_t row,std::string_view column,const std::string& actual,const std::string& expected) {
    if (actual!=expected) std::fprintf(stderr,"StateCleanup row=%zu column=%.*s actual(%zu)=%.180s expected(%zu)=%.180s\n",row,static_cast<int>(column.size()),column.data(),actual.size(),actual.c_str(),expected.size(),expected.c_str());
    CHECK(actual==expected);
}
// 고정fixture의관측범위선언만해석한다.원문효과기대는계산하지않는다.
std::vector<std::pair<std::uint32_t,std::size_t>> CleanupFields(std::string_view line) {
    if (!line.starts_with("# fields=")) throw std::runtime_error("State원문관측범위없음");
    std::vector<std::pair<std::uint32_t,std::size_t>> result; //주소와실제BYTE폭이다.
    for (const auto& field:CleanupSplit(line.substr(9),',')) { const auto values=CleanupSplit(field,'.');if (values.size()!=2) throw std::runtime_error("State관측범위형식");result.emplace_back(static_cast<std::uint32_t>(std::stoul(values[0],nullptr,16)),std::stoul(values[1])); } //선언순서를그대로보존한다.
    return result;
}
}

// 정상194·명시중단100을포함한최초Native294행의모든직접효과와선택/정리자식을대조한다.
TEST_CASE(StateWorldCleanup_ReplaysIndependentPe294) {
#ifdef NETSTORM_STATE_WORLD_CLEANUP_FIXTURE
    const std::filesystem::path path=NETSTORM_STATE_WORLD_CLEANUP_FIXTURE; //공용등록의명시고정경로다.
#else
    const auto path=std::filesystem::path(__FILE__).parent_path()/"fixtures/state-world-cleanup-x86.tsv"; //격리검사도동일원문을읽는다.
#endif
    std::ifstream input(path);if (!input) throw std::runtime_error("State정리fixture열기실패");std::string line;std::getline(input,line);const auto fields=CleanupFields(line); //기대가아닌관측물리폭선언이다.
    std::size_t rows=0;std::map<std::string,std::size_t> statuses; //부분중단을정상복귀로합치지않는다.
    while (std::getline(input,line)) { //양모드원문전체행을대조한다.
        if (line.empty()||line[0]=='#') continue;
        if (line.back()=='\r') line.pop_back();
        const auto values=CleanupSplit(line,'\t');if (values.size()!=13) throw std::runtime_error("State정리fixture칸수");
        CHECK(values[0]=="639"||values[0]=="895");CHECK(std::stoul(values[12])>0);CleanupDevice device(CleanupSplit(values[1],','),fields);std::string status="normal"; //같은명시입력으로새장치를구성한다.
        try { CleanupStateWorld(device.reason,device.Globals(),device.Hooks()); } catch (const CleanupStop&) { status="stop"; }
        ++rows;++statuses[status];CheckCleanup(rows,"status",status,values[2]);CheckCleanup(rows,"pause",std::to_string(device.pause),values[3]);CheckCleanup(rows,"current",device.Pointer(),values[4]);CheckCleanup(rows,"globals",device.RawGlobals(),values[5]);
        CheckCleanup(rows,"playerA",CleanupHex(device.playerBytes[0]),values[6]);CheckCleanup(rows,"playerB",CleanupHex(device.playerBytes[1]),values[7]);CheckCleanup(rows,"playerC",CleanupHex(device.playerBytes[2]),values[8]);
        const auto type=std::bit_cast<std::array<std::uint8_t,4>>(device.typeDefault);CheckCleanup(rows,"type+c4",CleanupHex(type),values[9]);CheckCleanup(rows,"headers180",CleanupHex(device.headers),values[10]);CheckCleanup(rows,"trace",device.Trace(),values[11]); //실제현재field참조와호출순서를원시그대로비교한다.
    }
    CHECK(rows==294);CHECK(statuses["normal"]==194);CHECK(statuses["stop"]==100);
}

// 선택된Core정리/Player재생성경계가없을때앞선효과만남고그뒤의가짜정상진행을하지않는다.
TEST_CASE(StateWorldCleanup_MissingCorePreservesEarlierEffects) {
#ifdef NETSTORM_STATE_WORLD_CLEANUP_FIXTURE
    std::ifstream input(NETSTORM_STATE_WORLD_CLEANUP_FIXTURE); //동일관측폭입력만재사용한다.
#else
    std::ifstream input(std::filesystem::path(__FILE__).parent_path()/"fixtures/state-world-cleanup-x86.tsv"); //격리fallback고정경로다.
#endif
    std::string line;std::getline(input,line);CleanupDevice device(CleanupSplit("165,1,0,0,0,0,14,1,1,1,6,2,10000,1,none",','),CleanupFields(line));auto hooks=device.Hooks();hooks.selectCore={}; //실제도달경계만누락한다.
    bool rejected=false; //선택장치누락을0성공으로바꾸지않는다.
    try { CleanupStateWorld(device.reason,device.Globals(),hooks); } catch (const std::logic_error&) { rejected=true; }
    CHECK(rejected);CHECK(device.Read(0x595344)==0);CHECK((device.Read(0x5c89b4)&0xffffU)==0);CHECK(device.Pointer()=="P0:1");CHECK(device.calls["4910a0"]==0);CHECK(device.calls["4af170"]==0);CHECK(device.Trace().ends_with("C:43b540:"));
}
