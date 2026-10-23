// 제품전고정PE의Core/Pool전체와하위trace를대조하며선택장치를별도입력으로둔다.
#include "TestSupport.h"
#include "o/RawPlayerCoreRefresh.h"
#include "o/RawPlayerCoreData.h"
#include <algorithm>
#include <array>
#include <bit>
#include <filesystem>
#include <fstream>
#include <limits>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {
using namespace netstorm::o;
// 최초입력의순서와빈마지막trace를보존한다.
std::vector<std::string> RefreshSplit(std::string_view text,char delimiter) {
    std::vector<std::string> values;std::size_t at{};
    for (;;) { // 분리한내용으로원문기대나분기를계산하지않는다.
        const auto end=text.find(delimiter,at);values.emplace_back(text.substr(at,end==text.npos?text.size()-at:end-at));if (end==text.npos) return values;at=end+1;
    }
}
// 같은little endian입력을실제물리에직접쓴다.
void RefreshPut(std::span<std::uint8_t> bytes,std::size_t offset,std::uint32_t value,std::size_t width=4) {
    if (offset>bytes.size() || width>bytes.size()-offset) throw std::out_of_range("CoreRefresh검사입력쓰기범위");
    for (std::size_t byte=0;byte<width;++byte) bytes[offset+byte]=static_cast<std::uint8_t>(value>>(byte*8)); // 지정폭만쓰기한다.
}
// 장치가필요로하는원시WORD/BYTE만읽는다.
std::uint32_t RefreshRead(std::span<const std::uint8_t> bytes,std::size_t offset,std::size_t width) {
    if (offset>bytes.size() || width>bytes.size()-offset) throw std::out_of_range("CoreRefresh검사입력읽기범위");std::uint32_t value{};
    for (std::size_t byte=0;byte<width;++byte) value|=std::uint32_t(bytes[offset+byte])<<(byte*8); // 장치입력의실제바이트만조합한다.
    return value;
}
// 현재Core/Pool의미작성BYTE까지전부비교한다.
std::string RefreshHex(std::span<const std::uint8_t> bytes) {
    constexpr std::string_view digits="0123456789abcdef";std::string value;
    for (const auto byte:bytes) { value+=digits[byte>>4];value+=digits[byte&15]; } // NUL도버리지않는다.
    return value;
}
// 실제하위의명시중단을물리계약오류와구별한다.
struct RefreshStop {};
// 현재Native입력만소유하고부모의max/Core갱신기대를장치에서계산하지않는다.
struct RefreshDevice {
    std::array<std::array<std::uint8_t,16>,3> cores{}; // 현재A/B/lastKnown의같은전체물리다.
    std::array<std::array<std::uint8_t,400>,2> pools{}; // 현재Pool변경전후의두실제raw표다.
    std::array<std::uint8_t,65536> players{}; // Native명시Player물리64KiB다.앞1620BYTE는9기록이며범위외owner256도이관측의실제0물리를읽는다.
    std::span<std::uint8_t> current,playerView{players}; // Coregetter에공급하는같은현재슬롯/Player뷰다.
    std::uint32_t poolIndex{},capacity{},owner{},playerType{},containedType{6},known{1},metamap{},templateOwner{},active{}; // 원문각전역의실제독립저장소다.
    std::array<std::int32_t,3> bits{}; // TYPE1/2/ffff의명시+1ec입력이다.
    std::size_t coreLength{};int levelMode{};std::string mutation; // 물리계약/가상반환열/한명시변경이다.
    std::map<std::string,std::size_t> calls;std::map<std::pair<std::uint32_t,std::uint32_t>,std::size_t> levelCalls; // 중첩에서도같은장치호출번호다.
    std::vector<std::string> trace;std::uint32_t currentLevelIndex{}; // 하위호출순서와dead입력대상이다.

    // 최초정수/물리입력만구성하며예상출력은fixture에서따로읽는다.
    explicit RefreshDevice(const std::vector<std::string>& values) {
        if (values.size()!=14) throw std::runtime_error("CoreRefresh입력칸수");
        const auto floor=std::stoul(values[0]),mask=std::stoul(values[1]),free=std::stoul(values[5]),dead=std::stoul(values[6]);
        for (std::size_t index=0;index<3;++index) bits[index]=std::stoi(values[index+2]); // TYPE별signed기술bit를보존한다.
        owner=static_cast<std::uint32_t>(std::stoul(values[7]));playerType=static_cast<std::uint32_t>(std::stoul(values[8]));capacity=static_cast<std::uint32_t>(std::stoul(values[9]));levelMode=std::stoi(values[10]);const auto kind=std::stoul(values[11]);coreLength=std::stoul(values[12]);mutation=values[13];
        for (std::size_t block=0;block<3;++block) { // Native주소차이에서나온패턴도입력그대로재현한다.
            const auto bias=block==0?0LL:block==1?29LL:-33786073LL; // floor((005953f0-12204000)/256)*29인명시입력패턴이다.
            for (std::size_t byte=0;byte<16;++byte) cores[block][byte]=static_cast<std::uint8_t>(41+bias+static_cast<long long>(byte)*11);
            cores[block][5]=static_cast<std::uint8_t>(block==0?floor:floor+17);
        }
        for (std::size_t block=0;block<2;++block) { // 두Pool의같은필드를원문입력과같이쓴다.
            for (std::size_t index=0;index<8;++index) {
                const auto offset=index*50;RefreshPut(pools[block],offset,0x12080000);pools[block][offset+10]=static_cast<std::uint8_t>((index==2||index==6||index==7)?152:6);pools[block][offset+11]=static_cast<std::uint8_t>(((free>>index)&1)|(((dead>>index)&1)<<1));pools[block][offset+34]=1;
            }
            RefreshPut(pools[block],56,3,2);
            for (std::uint32_t index=3;index<=5;++index) { // 기술목록의실제next/kind/flags입력이다.
                RefreshPut(pools[block],index*50+4,index==5?0:index+1,2);RefreshPut(pools[block],index*50+18,index==5?65535:index-2,2);pools[block][index*50+20]=static_cast<std::uint8_t>(mask&(1U<<(index-3))?4:0);
            }
            if (block) pools[block][111]=2;
        }
        RefreshPut(players,180+156,1);RefreshPut(players,8*180+156,1);
        if (kind<3) current=std::span(cores[kind]).first(coreLength);
    }
    // 같은입력물리주소로만현재Core를정규화한다.
    std::string Pointer() const {
        if (!current.data()) return "0";
        for (std::size_t index=0;index<3;++index) if (current.data()==cores[index].data()) return std::string(1,"ABC"[index]); // byte내용으로주소를추정하지않는다.
        throw std::runtime_error("CoreRefresh미등록현재Core");
    }
    // 실제하위의같은종류호출번호를재진입에서도공유한다.
    std::string Event(const std::string& kind,const std::string& text) { trace.push_back(text);return kind+std::to_string(++calls[kind]); }
    // callback은현재입력만바꾸며부모의분기/최종값을대신계산하지않는다.
    void Mutate(const std::string& boundary) {
        const auto dash=mutation.find('-');if (dash==mutation.npos || mutation.substr(0,dash)!=boundary) return;const auto action=mutation.substr(dash+1);
        if (action=="stop") throw RefreshStop{};
        if (action=="currentB") current=std::span(cores[1]).first(coreLength);
        else if (action=="cache") { current=std::span(cores[2]).first(coreLength);metamap=1; }
        else if (action=="poolB") poolIndex=1;
        else if (action=="cap2") capacity=2;
        else if (action=="known0") known=0;
        else if (action=="owner8") owner=8;
        else if (action=="owner0") owner=0;
        else if (action=="contained152") containedType=152;
        else if (action=="playerType6") playerType=6;
        else if (action=="dead") pools[poolIndex][currentLevelIndex*50+11]=2;
        else if (action!="reenter") throw std::runtime_error("CoreRefresh미해석명시변경");
    }
    // 선택한진단은현재변경후복귀하거나그위치에서중단한다.
    void Assertion(std::uint32_t line) { const auto boundary=Event("a","A:"+std::to_string(line));Mutate(boundary); }
    // 기존실제0048f0d0몸체를사용하며현재span을사본버퍼로교체하지않는다.
    std::span<std::uint8_t> Core() {
        const auto boundary=Event("g","G:"+Pointer());Mutate(boundary);
        return ReadPlayerCoreData({templateOwner,owner,known,active,metamap,current,cores[2],playerView},{[this](auto line) { Assertion(line); }});
    }
    // Native와동일한원시Player+9c하위경계다.부모의기술순회결과를계산하지않는다.
    std::uint32_t TechnologyParent() {
        const auto boundary=Event("p","P");Mutate(boundary);if (!known) return 0;
        return RefreshRead(players,static_cast<std::size_t>(owner)*180+156,4);
    }
    // 독립원시finder장치의next/current는Begin별로따로소유한다.
    PlayerCoreRefreshCursor Begin(std::uint32_t parent) {
        const auto boundary=Event("b","B:"+std::to_string(parent));Mutate(boundary);
        struct Cursor { std::uint32_t current{},next{}; }; // 원문finder의현재/이미캡처한next두DWORD다.
        const auto cursor=std::make_shared<Cursor>();cursor->next=RefreshRead(pools[poolIndex],parent*50+6,2);
        auto advance=[this,cursor] { // 실제현재Pool과containedType을후보마다읽는하위fixture장치다.
            for (;;) {
                cursor->current=cursor->next;if (!cursor->current) { cursor->next=0;return; }
                const auto offset=cursor->current*50;cursor->next=RefreshRead(pools[poolIndex],offset+4,2);
                if (RefreshRead(pools[poolIndex],offset+10,1)==containedType) return;
            }
        };
        advance();return {[cursor] { return cursor->current; },[this,advance] { const auto next=Event("n","N");Mutate(next);advance(); }};
    }
    // 가상level의명시반환만공급하며max선택은제품본문이수행한다.
    std::int32_t Level(PlayerCoreRefreshForm form) {
        const std::uint32_t block=form.address>=0x12010000?1U:0U,index=(form.address-(block?0x12010000U:0x12000000U))/50;currentLevelIndex=index;
        const bool second=(++levelCalls[{block,index}]%2)==0;std::int32_t value=static_cast<std::int32_t>((4U-(index-2U)%3U)+block);
        if (levelMode==1 && second) value=-3;
        else if (levelMode==2 && second) value=std::numeric_limits<std::int32_t>::min();
        else if (levelMode==3) value=std::numeric_limits<std::int32_t>::max();
        else if (levelMode==4) value=-1;
        CHECK(form.bytes.data()==pools[block].data()+index*50);
        const auto boundary=Event("v","V:"+std::to_string(block)+":"+std::to_string(index)+":"+std::to_string(value));Mutate(boundary);
        if (mutation==boundary+"-reenter") { trace.push_back("R:begin");RefreshPlayerCore(Hooks());trace.push_back("R:end"); }
        return value;
    }
    // 기존실제Corehelper와부모가선택한하위장치만연결한다.
    PlayerCoreRefreshHooks Hooks() {
        PlayerCoreRefreshHooks hooks;hooks.core=[this] { return Core(); };hooks.technologyParent=[this] { return TechnologyParent(); };hooks.begin=[this](auto parent) { return Begin(parent); };
        hooks.pool=[this] { return PlayerCoreRefreshPool{pools[poolIndex],poolIndex?0x12010000U:0x12000000U,capacity}; };
        hooks.technologyBit=[this](auto kind) { const auto boundary=Event("t","T:"+std::to_string(kind));Mutate(boundary);return bits[kind==1?0:kind==2?1:2]; };
        hooks.playerType=[this] { return playerType; };hooks.owner=[this] { return owner; };hooks.level=[this](auto form) { return Level(form); };hooks.assertion=[this](auto line) { Assertion(line); };return hooks;
    }
    // 현재전역/모든Core와Pool/trace를원시기대칸과같은순서로출력한다.
    std::string State() const { return std::to_string(poolIndex)+","+std::to_string(capacity)+","+std::to_string(owner)+","+std::to_string(playerType)+","+std::to_string(containedType)+","+std::to_string(known)+","+std::to_string(metamap); }
    std::string Cores() const { std::string text;for (const auto& core:cores) text+=RefreshHex(core);return text; } // 물리세벌전체를비교한다.
    std::string Pools() const { std::string text;for (const auto& pool:pools) text+=RefreshHex(pool);return text; } // 두Pool전체를비교한다.
    std::string Trace() const { std::string text;for (const auto& event:trace) { if (!text.empty()) text+=';';text+=event; }return text.empty()?"-":text; } // 순서/중복호출을보존한다.
};
// 최초차이를원시행/칸이름과함께출력한다.
void CheckRefresh(std::size_t row,const char* field,const std::string& actual,const std::string& expected) {
    if (actual!=expected) std::fprintf(stderr,"CoreRefresh row%zu %s actual=%s expected=%s\n",row,field,actual.c_str(),expected.c_str());CHECK(actual==expected);
}
}

// 논리span쓰기정지24행은store커밋의별도관측이고실제제품과동일하다고세지않는다.
TEST_CASE(PlayerCoreRefresh_ReplaysIndependentPe618SupportedObservations) {
#ifdef NETSTORM_PLAYER_CORE_REFRESH_FIXTURE
    const std::filesystem::path path=NETSTORM_PLAYER_CORE_REFRESH_FIXTURE; // 공용등록의최초원문절대경로다.
#else
    const auto path=std::filesystem::path(__FILE__).parent_path()/"fixtures/player-core-refresh-x86.tsv"; // 격리빌드도같은자료만읽는다.
#endif
    std::ifstream input(path);if (!input) throw std::runtime_error("CoreRefresh원문fixture열기실패");std::string line;std::size_t row{},compared{},excluded{};
    while (std::getline(input,line)) { // 최초642행을빠짐없이분류한다.
        if (line.empty()||line[0]=='#') continue;if (line.back()=='\r') line.pop_back();++row;const auto values=RefreshSplit(line,'\t');if (values.size()!=9) throw std::runtime_error("CoreRefreshfixture칸수");
        if (values[2]=="fault") { ++excluded;continue; }
        RefreshDevice device(RefreshSplit(values[1],','));std::string status="normal";
        try { RefreshPlayerCore(device.Hooks()); }catch(const RefreshStop&) { status="stop"; }catch(const std::out_of_range&) { if (device.current.data()) throw;status="fault-null"; }
        ++compared;CheckRefresh(row,"status",status,values[2]);CheckRefresh(row,"current",device.Pointer(),values[3]);CheckRefresh(row,"state",device.State(),values[4]);CheckRefresh(row,"cores",device.Cores(),values[5]);CheckRefresh(row,"pools",device.Pools(),values[6]);CheckRefresh(row,"trace",device.Trace(),values[7]);
    }
    CHECK(row==642 && compared==618 && excluded==24);
}

// 짧은hostspan은단일DWORD store前에거부하는계약이며Unicorn부분store와같다고하지않는다.
TEST_CASE(PlayerCoreRefresh_ShortCoreRejectsWholeStoreAndMissingLaterDeviceKeepsZero) {
    for (std::size_t length=0;length<10;++length) { // 실제10BYTE미만의모든hostview를별도계약으로검증한다.
        std::array<std::uint8_t,16> bytes;bytes.fill(0x5a);PlayerCoreRefreshHooks hooks;hooks.core=[&] { return std::span(bytes).first(length); };bool rejected{};
        try { RefreshPlayerCore(hooks); }catch(const std::out_of_range&) { rejected=true; }CHECK(rejected && std::all_of(bytes.begin(),bytes.end(),[](auto byte) { return byte==0x5a; }));
    }
    std::array<std::uint8_t,10> bytes;bytes.fill(0x5a);PlayerCoreRefreshHooks hooks;hooks.core=[&] { return std::span(bytes); };bool missing{};
    try { RefreshPlayerCore(hooks); }catch(const std::logic_error&) { missing=true; }CHECK(missing && bytes[5]==0x5a && RefreshRead(bytes,6,4)==0);
}

// bit범위진단뒤current가바뀌어도Ch는이미얻은같은this에쓴다.
TEST_CASE(PlayerCoreRefresh_RequiredBitDiagnosticKeepsCapturedCoreAndAllowsReentry) {
    RefreshDevice device(RefreshSplit("0,7,32,31,-1,0,0,1,152,8,0,0,10,a1-currentB",','));RefreshPlayerCore(device.Hooks());CHECK(device.Pointer()=="B" && RefreshRead(device.cores[0],6,4)==1 && (RefreshRead(device.cores[1],6,4)&0x80000000U));
    RefreshDevice nested(RefreshSplit("0,7,0,31,-1,0,0,1,152,8,0,0,10,v1-reenter",','));RefreshPlayerCore(nested.Hooks());CHECK(std::find(nested.trace.begin(),nested.trace.end(),"R:begin")!=nested.trace.end() && nested.cores[0][5]==3);
}

// 실제guard20중Unicorn부분store4행을구별하고지원16행의물리/trace를대조한다.
TEST_CASE(PlayerCoreRefresh_ReplaysGuard16AndSeparatesPartialStore4) {
#ifdef NETSTORM_PLAYER_CORE_REFRESH_GUARD_FIXTURE
    const std::filesystem::path path=NETSTORM_PLAYER_CORE_REFRESH_GUARD_FIXTURE; // 별도제품전guard관측의절대경로다.
#else
    const auto path=std::filesystem::path(__FILE__).parent_path()/"fixtures/player-core-refresh-guard-x86.tsv"; // 최초자료를읽기만한다.
#endif
    std::ifstream input(path);if (!input) throw std::runtime_error("CoreRefreshguardfixture열기실패");std::string line;std::size_t rows{},compared{},excluded{};
    while (std::getline(input,line)) { // 양정밀도의최초20행을모두분류한다.
        if (line.empty()||line[0]=='#') continue;if (line.back()=='\r') line.pop_back();const auto values=RefreshSplit(line,'\t');const auto args=RefreshSplit(values[1],',');const auto length=std::stoul(args[0]),kind=std::stoul(args[1]);++rows;
        if (length==9) { ++excluded;continue; } // 실제guard에서Unicorn이앞3BYTE를쓴4행은host단일store거부와같다고하지않는다.
        std::array<std::array<std::uint8_t,16>,2> tails{};std::array<std::uint8_t,400> pool{};std::vector<std::string> trace;
        for (std::size_t block=0;block<2;++block) { // 원문page마지막16BYTE의동일입력이다.
            tails[block].fill(static_cast<std::uint8_t>(0x5a+block));for (std::size_t byte=0;byte<length;++byte) tails[block][16-length+byte]=static_cast<std::uint8_t>(0x31+block*29+byte*11);
        }
        PlayerCoreRefreshHooks hooks;hooks.core=[&] { trace.push_back(kind?"G:B":"G:A");return std::span(tails[kind]).subspan(16-length,length); };hooks.technologyParent=[&] { trace.push_back("P");return 0U; };hooks.begin=[&](auto parent) { trace.push_back("B:"+std::to_string(parent));return PlayerCoreRefreshCursor{[] { return 0U; },{}}; };hooks.pool=[&] { return PlayerCoreRefreshPool{pool,0x12000000,0}; };
        std::string status="normal";try { RefreshPlayerCore(hooks); }catch(const std::out_of_range&) { status="fault-guard"; }
        std::string actualTrace;for (const auto& event:trace) { if (!actualTrace.empty()) actualTrace+=';';actualTrace+=event; } // 실제중복getter도trace로확인한다.
        ++compared;CheckRefresh(rows,"guard-status",status,values[2]);CheckRefresh(rows,"guard-bytes",RefreshHex(tails[0])+RefreshHex(tails[1]),values[3]);CheckRefresh(rows,"guard-trace",actualTrace,values[4]);
    }
    CHECK(rows==20 && compared==16 && excluded==4);
}
