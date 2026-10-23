// 실제PE79BYTE의두Player전체/현재base재조회/free순서를새부모와대조한다.
#include "TestSupport.h"
#include "o/RawPlayerLinksCleanup.h"
#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
using namespace netstorm::o;
// 최초원시칸을마지막빈칸까지읽는다.
std::vector<std::string> LinksSplit(std::string_view text,char separator) {
    std::vector<std::string> parts;std::size_t at{};
    for (;;) { // 기대상태를계산하지않고입력문자열만분리한다.
        const auto end=text.find(separator,at);parts.emplace_back(text.substr(at,end==text.npos?text.size()-at:end-at));if (end==text.npos) return parts;at=end+1;
    }
}
// 지정한같은실제PlayerDWORD에입력만쓴다.
void LinksPut(std::span<std::uint8_t> bytes,std::size_t offset,std::uint32_t value) {
    for (std::size_t byte=0;byte<4;++byte) bytes[offset+byte]=static_cast<std::uint8_t>(value>>(byte*8)); // 비정렬호스트정수참조를만들지않는다.
}
// 필수경계/독립검사의원시PlayerDWORD를읽는다.
std::uint32_t LinksRead(std::span<const std::uint8_t> bytes,std::size_t offset) {
    std::uint32_t value{};for (std::size_t byte=0;byte<4;++byte) value|=std::uint32_t(bytes[offset+byte])<<(byte*8);return value; // 실제4BYTE만조합한다.
}
// 전체Player와미작성필드를NUL과무관하게비교한다.
std::string LinksHex(std::span<const std::uint8_t> bytes) {
    constexpr std::string_view digits="0123456789abcdef";std::string text;
    for (const auto byte:bytes) { text+=digits[byte>>4];text+=digits[byte&15]; }return text; // 숨은바이트차이를버리지않는다.
}
// 선택free의명시중단을실제물리계약오류와구별한다.
struct LinksStop {};
// 부모기대가아닌명시초기물리/현재base/free외부장치만제공한다.
struct LinksDevice {
    std::array<std::array<std::uint8_t,1620>,2> blocks{}; // callback뒤에도살아있는현재/교체Player두물리다.
    std::uint32_t current{},owner{7},stop{};std::size_t calls{};std::string mutation; // owner는free외부효과일뿐부모가조회하지않는다.
    std::vector<std::uint32_t> tokens;std::vector<std::string> trace; // 입력free호출/선택중단까지의순서다.

    // 최초Native입력과동일한두전체배열/opaque토큰만구성한다.
    explicit LinksDevice(const std::vector<std::string>& input) {
        if (input.size()!=6) throw std::runtime_error("Player링크Native입력칸수");const auto seed=std::stoul(input[0]);const std::array<std::uint32_t,2> masks{static_cast<std::uint32_t>(std::stoul(input[1])),static_cast<std::uint32_t>(std::stoul(input[2]))};current=static_cast<std::uint32_t>(std::stoul(input[3]));stop=static_cast<std::uint32_t>(std::stoul(input[4]));mutation=input[5];
        for (std::size_t block=0;block<2;++block) { // 다른미작성필드도모두원시입력과같이만든다.
            for (std::size_t byte=0;byte<1620;++byte) blocks[block][byte]=static_cast<std::uint8_t>(seed+block*53+byte*17);
            for (std::uint32_t player=0;player<9;++player) LinksPut(blocks[block],player*180+104,masks[block]&(1U<<player)?0x21000000U+static_cast<std::uint32_t>(block)*0x1000U+player*0x10U:0); // 토큰을host포인터로해석하지않는다.
        }
    }
    // 원문의매현재base읽기와동일하게조회하고별도사본으로동기화하지않는다.
    std::span<std::uint8_t> Players() { trace.push_back("R:"+std::to_string(current));return current==2?std::span<std::uint8_t>{}:std::span<std::uint8_t>(blocks[current]); }
    // Native의같은free인자/현재값변경/재진입만제공하며실제CRTfree완료를주장하지않는다.
    void Release(std::uint32_t token) {
        trace.push_back("F:"+std::to_string(token));tokens.push_back(token);++calls;if (calls==stop) throw LinksStop{};
        const auto separator=mutation.find('-');if (mutation.substr(0,separator)!="f"+std::to_string(calls)) return;const auto action=mutation.substr(separator+1);
        if (action=="baseB") current=1;
        else if (action=="baseA") current=0;
        else if (action=="null") current=2;
        else if (action=="owner8") owner=8;
        else if (action=="pointerA") LinksPut(blocks[0],104,0x21234567);
        else if (action=="nextA") LinksPut(blocks[0],180+104,0);
        else if (action=="reenter") { trace.push_back("RE:begin");CleanupPlayerLinks(Hooks());trace.push_back("RE:end"); }
        else throw std::runtime_error("Player링크미해석명시효과");
    }
    // 선택할실제부모에같은현재물리/자식을연결한다.
    PlayerLinksCleanupHooks Hooks() { return {[this] { return Players(); },[this](auto token) { Release(token); }}; }
    // 중단까지의순서와중복호출을원시문자열로표현한다.
    std::string Trace() const { std::string text;for (const auto& value:trace) { if (!text.empty()) text+=';';text+=value; }return text; } // 재진입구간도합치지않는다.
    std::string Tokens() const { std::string text;for (const auto value:tokens) { if (!text.empty()) text+=',';text+=std::to_string(value); }return text.empty()?"-":text; } // token0호출을추가하지않는다.
};
// 불일치의원문행/칼럼을검사로그에남긴다.
void CheckLinks(std::size_t row,const char* field,const std::string& actual,const std::string& expected) {
    if (actual!=expected) std::fprintf(stderr,"PlayerLinks row%zu %s actual=%s expected=%s\n",row,field,actual.c_str(),expected.c_str());CHECK(actual==expected);
}
}

// 실제Native정상138/중단18/NULL6의전체물리/현재전역/trace를모두대조한다.
TEST_CASE(PlayerLinksCleanup_ReplaysIndependentPe162) {
#ifdef NETSTORM_PLAYER_LINKS_CLEANUP_FIXTURE
    const std::filesystem::path path=NETSTORM_PLAYER_LINKS_CLEANUP_FIXTURE; // 공용등록은최초TSV절대경로다.
#else
    const auto path=std::filesystem::path(__FILE__).parent_path()/"fixtures/player-links-cleanup-x86.tsv"; // 격리빌드도같은자료를읽는다.
#endif
    std::ifstream input(path);if (!input) throw std::runtime_error("Player링크원문fixture열기실패");std::string line;std::size_t rows{};std::map<std::string,std::size_t> statuses;
    while (std::getline(input,line)) { // 두정밀도162행에서지원되지않는행을조용히빼지않는다.
        if (line.empty()||line[0]=='#') continue;if (line.back()=='\r') line.pop_back();const auto values=LinksSplit(line,'\t');if (values.size()!=8) throw std::runtime_error("Player링크fixture칼럼수");LinksDevice device(LinksSplit(values[1],','));std::string status="normal";
        try { CleanupPlayerLinks(device.Hooks()); }catch(const LinksStop&) { status="stop"; }catch(const std::out_of_range&) { if (device.current!=2) throw;status="fault-null"; }
        ++rows;++statuses[status];CheckLinks(rows,"status",status,values[2]);CheckLinks(rows,"globals",std::to_string(device.current)+","+std::to_string(device.owner),values[3]);CheckLinks(rows,"players",LinksHex(device.blocks[0])+LinksHex(device.blocks[1]),values[4]);CheckLinks(rows,"free-inputs",device.Tokens(),values[5]);CheckLinks(rows,"trace",device.Trace(),values[6]);
    }
    CHECK(rows==162 && statuses["normal"]==138 && statuses["stop"]==18 && statuses["fault-null"]==6);
}

// 실제clone할당반납은선택토큰의유일한owner가수행하고부모가Player슬롯만뒤에서0으로쓴다.
TEST_CASE(PlayerLinksCleanup_ReleasesRealOwnedAllocationsAndSkipsZero) {
    std::array<std::uint8_t,1620> players{};std::map<std::uint32_t,std::unique_ptr<std::uint8_t[]>> allocations;std::size_t releases{};
    for (std::uint32_t owner=0;owner<9;++owner) { // 실제352BYTE를각토큰에소유하며미초기화BYTE를읽지않는다.
        const auto token=owner+1;allocations.emplace(token,std::make_unique<std::uint8_t[]>(352));LinksPut(players,owner*180+104,token);LinksPut(players,owner*180+156,17);LinksPut(players,owner*180+8,21);LinksPut(players,owner*180,0x89abcdef);
    }
    PlayerLinksCleanupHooks hooks{[&] { return std::span(players); },[&](auto token) { CHECK(allocations.contains(token));CHECK(LinksRead(players,(token-1)*180+104)==token);allocations.erase(token);++releases; }};CleanupPlayerLinks(hooks);CHECK(allocations.empty() && releases==9);
    for (std::uint32_t owner=0;owner<9;++owner) CHECK(LinksRead(players,owner*180+104)==0 && LinksRead(players,owner*180+156)==0 && LinksRead(players,owner*180+8)==0 && LinksRead(players,owner*180)==0x89abcdef); // Player+0의별도할당/메인Player물리는반납하지않는다.
    hooks.release={};CleanupPlayerLinks(hooks);CHECK(releases==9); // 모든0동적필드에서미선택release를선행요구하지않는다.
}

// 필수free/물리오류는앞선선택필드0을보존하고뒤의pointer0/다음기록을앞당기지않는다.
TEST_CASE(PlayerLinksCleanup_MissingReleaseAndShortPhysicalKeepEarlierEffects) {
    std::array<std::uint8_t,1620> players;players.fill(0xa5);LinksPut(players,104,1);bool missing{};PlayerLinksCleanupHooks hooks{[&] { return std::span(players); },{}};
    try { CleanupPlayerLinks(hooks); }catch(const std::logic_error&) { missing=true; }CHECK(missing && LinksRead(players,156)==0 && LinksRead(players,8)==0 && LinksRead(players,104)==1 && players[180+156]==0xa5);
    std::size_t releases{};hooks.players=[&] { return std::span(players).first(160); };hooks.release=[&](auto token) { CHECK(token==1);++releases; };bool physical{};
    try { CleanupPlayerLinks(hooks); }catch(const std::out_of_range&) { physical=true; }CHECK(physical && releases==1 && LinksRead(players,104)==0 && players[180+156]==0xa5); // 두번째record의첫store는짧은host뷰에서실패한다.
}
