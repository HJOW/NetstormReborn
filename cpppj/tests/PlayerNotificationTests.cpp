// 원문 알림/실제SID getter와 별도 Core 합계의 최초 관측을 전체 기록/trace로 비교한다.
#include "TestSupport.h"
#include "o/RawPlayerNotification.h"
#include <array>
#include <fstream>
#include <stdexcept>
#include <vector>
namespace { //명시 하위 응답만 공급하고 실제 제품 부모가 쓰기/순서를 결정한다.
using namespace netstorm::o; //원문과 같은 판본/정책을 사용한다.
struct Stop {}; //효과 진입 전 부분 관측 중단 신호다.
std::vector<std::string> Split(std::string_view value,char separator='\t') { //마지막 빈 trace도 보존한다.
    std::vector<std::string> result;std::size_t begin{};for (;;) { const auto end=value.find(separator,begin);result.emplace_back(value.substr(begin,end==value.npos ? value.npos : end-begin));if (end==value.npos) return result;begin=end+1; } //원래 구분자 위치를 순회한다.
}
std::uint32_t Number(std::string_view value) { return static_cast<std::uint32_t>(std::stoull(std::string(value))); } //원문 DWORD/마스크/출처다.
std::string Hex(std::span<const std::uint8_t> bytes) { //NUL을 포함한 전체 물리 바이트를 비교한다.
    constexpr std::string_view digits="0123456789abcdef";std::string result;for (const auto byte:bytes) { result+=digits[byte>>4];result+=digits[byte&15]; }return result; //각 바이트를 두 자리로 기록한다.
}
void Store(std::span<std::uint8_t> bytes,std::size_t offset,std::uint32_t value) { for (std::size_t byte=0;byte<4;++byte) bytes[offset+byte]=static_cast<std::uint8_t>(value>>(byte*8)); } //명시 검사 입력/응답의 DWORD만 설치한다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t offset) { return bytes[offset]|(std::uint32_t(bytes[offset+1])<<8)|(std::uint32_t(bytes[offset+2])<<16)|(std::uint32_t(bytes[offset+3])<<24); } //원문 입력 기록에서 현재 값을 읽는다.
}
// 실제490da0/48fc60의118입력은 전체9기록/owner/부분효과까지 일치해야 한다.
TEST_CASE(PlayerNotification_Native118PendingResultsAndPartialEffects) {
    std::ifstream input(NETSTORM_PLAYER_NOTICES_FIXTURE);CHECK(input.good());std::string line;std::size_t cases{};
    while (std::getline(input,line)) { //각 행에 새 Player 물리를 만든다.
        if (!line.empty() && line.back()=='\r') line.pop_back();const auto f=Split(line);CHECK(f.size()==14);if (f.size()!=14) return;std::vector<std::uint8_t> players(9*180);
        for (std::size_t index=0;index<players.size();++index) players[index]=static_cast<std::uint8_t>(index*37+11); //원문의 이웃 기록/미사용 BYTE까지 고정한 입력이다.
        Store(players,180+0xa0,Number(f[3]));Store(players,180+0x9c,Number(f[4]));players[180+0x71]=static_cast<std::uint8_t>(Number(f[5]));std::uint32_t owner=1,events{};std::string trace;
        const auto event=[&](std::string value) { if (!trace.empty()) trace+=';';trace+=value;if (++events==Number(f[8])) throw Stop{}; }; //효과 적용 전 진입이다.
        const auto mutate=[&](std::uint32_t point) { if (Number(f[9])==point) { owner=2;Store(players,180+0xa0,125); } }; //명시 하위 응답의 현재 데이터 변경이다.
        PlayerNotificationHooks h;h.global=[&](std::uint32_t address)->std::uint32_t { switch (address) { case 0x595344:return Number(f[1]);case 0x594fbc:return Number(f[2]);case 0x540c70:return owner;case 0x5411cc:return 93;case 0x5412c0:return 154;default:throw std::logic_error("알림 검사 예상 밖 전역"); } };
        h.players=[&]()->std::span<std::uint8_t> { return players; };h.technology=[&] { event("T");return Number(f[1]) ? Read(players,owner*180+0x9c) : 0U; };
        h.weight=[&](auto core) { event("W,"+Hex(core));mutate(1);return static_cast<std::uint8_t>(Number(f[6])); };
        h.contained=[&](auto parent,auto mask,auto type,auto index) { event("F,"+std::to_string(parent)+','+std::to_string(mask)+','+std::to_string(type)+','+std::to_string(index));mutate(2);return Number(f[7]); };
        h.tell=[&](auto text) { event("N,"+std::string(text));mutate(3); };h.grant=[&](auto type,auto current) { event("G,"+std::to_string(type)+','+std::to_string(current)); };h.refresh=[&] { event("S"); };
        bool completed=true;try { NotifyPendingPlayerResult(OriginalEdition::Patch1078,h); }catch (const Stop&) { completed=false; }
        CHECK(completed==(f[10]=="1"));CHECK(owner==Number(f[11]));CHECK(Hex(players)==f[12]);CHECK(trace==f[13]);++cases;
    }CHECK(cases==118);
}
// 실제42b210의92입력은 결과BYTE/현재 Core 마스크·rank/타입수·getter순서를 비교한다.
TEST_CASE(PlayerNotification_Native92CoreTechnologyWeight) {
    std::ifstream input(NETSTORM_CORE_WEIGHT_FIXTURE);CHECK(input.good());std::string line;std::size_t cases{};
    while (std::getline(input,line)) { //타입 표/현재 개수와 실제10BYTE를 독립 입력으로 만든다.
        if (!line.empty() && line.back()=='\r') line.pop_back();const auto f=Split(line);CHECK(f.size()==9);if (f.size()!=9) return;std::array<std::uint8_t,10> core{};core[5]=static_cast<std::uint8_t>(Number(f[2]));Store(core,6,Number(f[1]));std::vector<CoreTechnologyWeightType> types;
        if (f[3]!="-") for (const auto& value:Split(f[3],';')) { const auto comma=value.find(',');types.push_back({std::stoi(value.substr(0,comma)),std::stoi(value.substr(comma+1))}); } //독립 원문 입력의 signed 두 TYPE필드다.
        auto count=static_cast<std::int32_t>(types.size());std::uint32_t calls{};std::string trace;CoreTechnologyWeightHooks h;h.count=[&] { return count; };
        h.type=[&](std::int32_t index) { if (!trace.empty()) trace+=';';trace+="Y,"+std::to_string(index);if (++calls==1) { if (f[4]=="1") { Store(core,6,1U<<31);core[5]=7; }if (f[4]=="2") count=1; }return types.at(static_cast<std::size_t>(index)); };
        CHECK(CoreTechnologyWeight(core,h)==Number(f[5]));CHECK(Hex(std::span<const std::uint8_t>(core).subspan(5))==f[6]);CHECK(count==static_cast<std::int32_t>(Number(f[7])));CHECK(trace==f[8]);++cases;
    }CHECK(cases==92);
}
// 원문의 선행 자연 반환은 선택되지 않은 큰 게임 장치를 요구하지 않는다.
TEST_CASE(PlayerNotification_SelectedDevicesAndEdition) {
    PlayerNotificationHooks h;std::uint32_t reads{};h.global=[&](auto) { ++reads;return 0U; };NotifyPendingPlayerResult(OriginalEdition::Patch1078,h);CHECK(reads==1);
    bool cd{};try { NotifyPendingPlayerResult(OriginalEdition::Cd1072,h); }catch (const std::invalid_argument&) { cd=true; }CHECK(cd && reads==1);
    std::array<std::uint8_t,10> core{};core[5]=43;CHECK(CoreTechnologyWeight(core,{})==43); //마스크0은 타입 개수/표를 읽지 않는다.
}
