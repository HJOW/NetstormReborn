// 실제 저장 부모/getter의 최초1352관측과 전체 raw 장부/stream/출력/부분 중단을 비교한다.
#include "TestSupport.h"
#include "o/RawFortBadgesSave.h"
#include <array>
#include <fstream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
using namespace netstorm::o;
namespace {
constexpr std::uint32_t kBadgesBufferA=0x12008000U,kBadgesBufferB=0x12009000U; // 최초 PE 격리 입력의 두 살아 있는 할당 주소다.
constexpr std::uint32_t kBadgesManager11=0x12000100U,kBadgesManager22=0x12000200U; // raw Template에 저장하는 실제 가상 manager 포인터다.
// 마지막 빈 칸도 보존하는 최초 입력/관측 필드 분리다.
std::vector<std::string> BadgesFields(std::string_view text,char separator) {
    std::vector<std::string> result;std::size_t start{};
    for (;;) { // 각 원래 구분자와 빈 문자열의 순서를 유지한다.
        const auto end=text.find(separator,start);result.emplace_back(text.substr(start,end==text.npos ? text.npos : end-start));if (end==text.npos) return result;start=end+1;
    }
}
// 해당 네 BYTE를 little endian DWORD로 읽는다.
std::uint32_t BadgesGet(std::span<const std::uint8_t> bytes,std::size_t at) {
    std::uint32_t value{};
    for (std::size_t index=0;index<4;++index) value|=std::uint32_t(bytes[at+index])<<(index*8);return value; // 주변 padding은 읽지 않는다.
}
// 지정 네 BYTE만 바꾸고 주변 입력은 그대로 보존한다.
void BadgesPut(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value) {
    for (std::size_t index=0;index<4;++index) bytes[at+index]=static_cast<std::uint8_t>(value>>(index*8)); // 원본 DWORD 폭만 쓴다.
}
// 실제 물리 메모리의 모든 BYTE를 관측과 같은 소문자 hex로 표현한다.
std::string BadgesHex(std::span<const std::uint8_t> bytes) {
    constexpr std::string_view digits="0123456789abcdef";std::string result; // 한 BYTE의 두 글자 표현에 쓰는 고정 표다.
    for (const auto byte:bytes) { result+=digits[byte>>4];result+=digits[byte&15U]; }return result; // 사용하지 않은 꼬리도 함께 비교한다.
}
// 전체6160BYTE를 무손실 RLE로 표현해 type/count 사이212BYTE도 검증한다.
std::string BadgesRle(std::span<const std::uint8_t> bytes) {
    std::string result;std::size_t start{};
    while (start<bytes.size()) { // 연속한 같은 BYTE의 길이와 값만 출력한다.
        auto end=start+1;while (end<bytes.size() && bytes[end]==bytes[start]) ++end; // 다음 run의 경계만 찾는다.
        if (!result.empty()) result+=',';result+=std::to_string(end-start)+':'+BadgesHex(bytes.subspan(start,1));start=end;
    }
    return result;
}
// manager 장치 ID만 실제 raw 포인터로 표현한다.
std::uint32_t BadgesPointer(std::uint32_t manager) { if (!manager) return 0;if (manager==11) return kBadgesManager11;if (manager==22) return kBadgesManager22;throw std::out_of_range("Badges manager ID 범위"); }
// raw Template의 포인터를 외부 장치 ID로만 정규화한다.
std::uint32_t BadgesManager(std::uint32_t pointer) { if (!pointer) return 0;if (pointer==kBadgesManager11) return 11;if (pointer==kBadgesManager22) return 22;throw std::out_of_range("Badges manager 포인터 범위"); }
// 원본 emu_stop과 같은 지정 경계의 중단 표식이다.
struct BadgesStop {};
// 최초 입력과 외부 장치 응답만 제공한다. 출력/조회 순서는 제품이 결정한다.
struct BadgesDevices {
    std::uint32_t kind{},profile{},query{},count{},manager{},position{},end{},capacity{}; // 최초 입력의 각 실제 DWORD다.
    std::uint32_t created{},token{},result{},capacityCalls{}; // 명시 장치의 실제 응답/부분 상태다.
    std::uint32_t active{1},counterToken{},counterPosition{}; // 현재 반환 stream과 최초 count 할당/위치다.
    std::array<std::uint8_t,20> header{}; // 실제 Template1의 전체20BYTE다.
    std::array<std::array<std::uint8_t,16>,2> nodes{}; // 명시 Create가 반환할 두 실제 stream이다.
    std::array<std::array<std::uint8_t,256>,2> buffers{}; // data 변경 후에도 최초 할당이 살아 있는 두 실제 출력이다.
    std::array<std::uint8_t,6160> badges{}; // 의미없는 padding도 입력 그대로 가진 실제28×220BYTE 장부다.
    std::map<std::uint32_t,std::uint32_t> currents{{0,90},{11,91},{22,92}}; // 원문 외부 DataManager의 초기 current다.
    std::map<std::string,std::uint32_t> calls; // 실제 외부 경계별 호출 횟수다.
    std::string mutation,trace; // 최초 입력의 callback 변경과 실제 발생한 전체 경계 순서다.
    // 최초 fixture의 입력열만 파싱하며 결과열은 제품 실행 뒤에만 사용한다.
    explicit BadgesDevices(std::string_view input) {
        const auto fields=BadgesFields(input,',');if (fields.size()!=9) throw std::runtime_error("Badges 저장 입력 칸 수 오류");
        kind=std::stoul(fields[0]);profile=std::stoul(fields[1]);query=std::stoul(fields[2]);count=std::stoul(fields[3]);manager=std::stoul(fields[4]);position=std::stoul(fields[5]);end=std::stoul(fields[6]);capacity=std::stoul(fields[7]);mutation=fields[8];
        for (std::size_t byte=0;byte<header.size();++byte) header[byte]=static_cast<std::uint8_t>(byte); // 전체 Template 초기 입력이다.
        BadgesPut(header,0,BadgesPointer(manager));badges.fill(0xa5);buffers[0].fill(0xcc);buffers[1].fill(0xdd);
        for (std::size_t index=0;index<nodes.size();++index) { // 두 명시 stream은 같은 pos/end/capacity와 서로 다른 data를 갖는다.
            BadgesPut(nodes[index],0,index ? kBadgesBufferB : kBadgesBufferA);BadgesPut(nodes[index],4,position);BadgesPut(nodes[index],8,end);BadgesPut(nodes[index],12,capacity);
        }
        for (std::uint32_t index=0;index<28;++index) { // 최초 도구와 같은 장부 입력만 구성하며 저장 기대값을 만들지 않는다.
            std::uint32_t type=index,quantity{};
            if (profile==0) { type=index==0 ? query : 70+index;quantity=index==0 ? count : 0; }
            else if (profile==1) { const bool duplicate=index==0 || index==13 || index==27;type=duplicate ? query : 100+index;quantity=duplicate ? count+(index==0 ? 0U : index==13 ? 1U : 2U) : 0U; }
            else if (profile==2) { type=index<3 ? std::array<std::uint32_t,3>{0,254,255}[index] : index==27 ? 0U : 100+index;quantity=index<3 ? count : index==27 ? count+7U : 0U; }
            else if (profile==3) { type=index<27 ? index : 254U;quantity=count+index; }
            else if (profile==4) { type=1000+index;quantity=count; }
            BadgesPut(badges,index*220,type);BadgesPut(badges,index*220+216,quantity);
        }
    }
    // 실제 buffer 포인터를 같은 할당의 span으로만 변환한다.
    std::span<std::uint8_t> Buffer(std::uint32_t pointer) { if (pointer==kBadgesBufferA) return buffers[0];if (pointer==kBadgesBufferB) return buffers[1];throw std::out_of_range("Badges 명시 물리 buffer 범위"); }
    // 최초 원문 EDI 포인터가 가리킨 같은 BYTE만 읽는다.
    std::int32_t Counter() const { return counterToken ? buffers.at(counterToken-1).at(counterPosition) : -1; }
    // native 경계와 같은 한 시간순 trace에 기록한다.
    std::uint32_t Event(std::string_view name,std::string text) { if (!trace.empty()) trace+=';';trace+=text;return ++calls[std::string(name)]; }
    // 원문에 공급한 지정 외부효과만 반영한다. 이후 stream 선택/분기는 제품이 수행한다.
    void Mutate(std::string_view boundary) {
        const auto split=mutation.rfind('-');if (split==mutation.npos || mutation.substr(0,split)!=boundary) return;const auto action=mutation.substr(split+1);auto& node=nodes.at(active-1);
        if (action.starts_with("manager")) BadgesPut(header,0,BadgesPointer(std::stoul(action.substr(7))));
        else if (action=="dataA") BadgesPut(node,0,kBadgesBufferA);else if (action=="dataB") BadgesPut(node,0,kBadgesBufferB);
        else if (action.starts_with("pos")) BadgesPut(node,4,std::stoul(action.substr(3)));
        else if (action.starts_with("end")) BadgesPut(node,8,std::stoul(action.substr(3)));
        else if (action.starts_with("cap")) BadgesPut(node,12,std::stoul(action.substr(3)));
        else if (action.starts_with("counter")) buffers.at(counterToken-1).at(counterPosition)=static_cast<std::uint8_t>(std::stoul(action.substr(7)));
        else if (action=="table999") BadgesPut(badges,216,999);
        else if (action=="current2") for (auto& current:currents) current.second=2; // 다른 current를 골라도 반환 stream을 교체하지 않는다.
        else if (action=="stop") throw BadgesStop{};
    }
    // 원문 Create/진단/getter반환 callback과 같은 명시 응답만 구성한다.
    FortBadgesSaveHooks Hooks() {
        FortBadgesSaveHooks hooks;
        hooks.manager=[this](auto slot) { CHECK(slot==1);return BadgesManager(BadgesGet(header,0)); };
        hooks.create=[this](auto selected,std::string_view name) { CHECK(name=="Badges");++created;token=mutation=="create-return2" ? 2U : 1U;active=token;Event("create","create:"+std::to_string(selected)+":Badges");currents.at(selected)=token;Mutate("create");return token; };
        hooks.assertion=[this](auto reason) { const auto& node=nodes.at(active-1);const auto line=static_cast<std::uint32_t>(reason);const auto index=Event("assert","assert:"+std::to_string(line)+':'+std::to_string(BadgesGet(node,0))+':'+std::to_string(BadgesGet(node,4))+':'+std::to_string(BadgesGet(node,8))+':'+std::to_string(BadgesGet(node,12))+':'+std::to_string(Counter()));Mutate(line==36 ? "capacity"+std::to_string(++capacityCalls) : "assert"+std::to_string(index)); };
        hooks.count=[this](auto type) { const auto value=ReadFortBadgeCount(badges,type);Event("count","count:"+std::to_string(type)+':'+std::to_string(value)+':'+std::to_string(Counter()));Mutate("count"+std::to_string(type));return value; };
        hooks.access=[this](auto selected,const auto& operation) {
            CHECK(selected==token);auto& node=nodes.at(selected-1);if (!counterToken) { counterToken=BadgesGet(node,0)==kBadgesBufferA ? 1U : 2U;counterPosition=BadgesGet(node,4); }
            MemStreamHooks memory;memory.buffer=[this](auto pointer) { return Buffer(pointer); };operation(node,memory);
        };return hooks;
    }
    // 한 행의 제품 부모 또는 실제 장부 getter만 명시 실행한다.
    void Run(FortBadgesSaveHooks hooks) { if (!kind) SaveFortBadges(1,hooks);else result=ReadFortBadgeCount(badges,query); }
    // 전체 현재 외부 상태7DWORD를 최초 원문과 같은 순서로 표현한다.
    std::string State() const { return std::to_string(BadgesManager(BadgesGet(header,0)))+','+std::to_string(created)+','+std::to_string(token)+','+std::to_string(result)+','+std::to_string(currents.at(0))+','+std::to_string(currents.at(11))+','+std::to_string(currents.at(22)); }
    // 사용하지 않은 두번째stream/버퍼와 Template 부가 필드도 비교한다.
    std::string Raw() const { return BadgesHex(header)+BadgesHex(nodes[0])+BadgesHex(nodes[1])+BadgesHex(buffers[0])+BadgesHex(buffers[1]); }
};
}
// 최초676입력×두정밀도의 모든 관측과80회 부분 중단을 비교한다.
TEST_CASE(FortBadgesSave_IndependentNative1352) {
    std::ifstream input(NETSTORM_FORT_BADGES_SAVE_FIXTURE);if (!input) throw std::runtime_error("Badges 저장 최초 x86 fixture 없음");std::string line;std::size_t compared{},stopped{},counts{},assertions{},creates{};std::set<std::pair<std::string,std::string>> unique;
    while (std::getline(input,line)) { // 관측 기대열은 제품 실행이 끝난 뒤에만 사용한다.
        if (!line.empty() && line.back()=='\r') line.pop_back();if (line.empty() || line[0]=='#') continue;const auto row=BadgesFields(line,'\t');if (row.size()!=7) throw std::runtime_error("Badges 저장 fixture 칸 수 오류");CHECK(unique.emplace(row[0],row[1]).second);
        BadgesDevices devices(row[1]);bool completed=true;try { devices.Run(devices.Hooks()); }catch (const BadgesStop&) { completed=false;++stopped; }
        CHECK(completed==(row[2]=="1"));CHECK(devices.State()==row[3]);CHECK(devices.Raw()==row[4]);CHECK(BadgesRle(devices.badges)==row[5]);CHECK(devices.trace==row[6]);++compared;counts+=devices.calls["count"];assertions+=devices.calls["assert"];creates+=devices.calls["create"];
    }
    CHECK(compared==1352 && stopped==80 && counts==167834 && assertions==3788 && creates==710);
}
// raw getter의 장부 물리 계약과 선택된 누락 child의 선행 효과를 명시 검사한다.
TEST_CASE(FortBadgesSave_MissingDevicesAndPhysicalLedger) {
    BadgesDevices missing("0,0,0,1,11,0,0,128,none");auto hooks=missing.Hooks();hooks.count={};bool rejected{};
    try { missing.Run(hooks); }catch (const std::logic_error&) { rejected=true; }CHECK(rejected && missing.created==1 && BadgesGet(missing.nodes[0],4)==1 && missing.buffers[0][0]==0);
    BadgesDevices getter("1,1,255,0,11,0,0,128,none");CHECK(ReadFortBadgeCount(getter.badges,255)==0);CHECK(ReadFortBadgeCount(getter.badges,0xffffffffU)==0);
    rejected=false;try { ReadFortBadgeCount(std::span(getter.badges).first(6159),0); }catch (const std::invalid_argument&) { rejected=true; }CHECK(rejected);
}
