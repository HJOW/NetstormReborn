// 제품 이전의 실제 부모/serial 조회120관측으로 현재 필드와 중단 prefix를 검증한다.
#include "TestSupport.h"
#include "client/RawStormPowerRefresh.h"
#include <array>
#include <bit>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace { // 가상 장치의 관측 입력 이외에 제품 결과를 기대값으로 사용하지 않는다.
using namespace netstorm::client;
constexpr std::uint32_t kBudgetNodes=0x12000100; //최초 Native 격리 입력의 노드0 주소다.
// 실제 TSV의 모든 열을 같은 순서로 읽는다.
std::vector<std::string> BudgetFields(std::string_view line) { std::vector<std::string> fields;std::size_t at{};for (;;) { const auto end=line.find('\t',at);fields.emplace_back(line.substr(at,end==line.npos?line.npos:end-at));if (end==line.npos) return fields;at=end+1; } } //마지막 열까지 버리지 않는다.
// 최초 전체 노드/등록표를16진 BYTE로 복원한다.
std::vector<std::uint8_t> BudgetBytes(std::string_view hex) { std::vector<std::uint8_t> result;for (std::size_t at=0;at<hex.size();at+=2) result.push_back(static_cast<std::uint8_t>(std::stoul(std::string(hex.substr(at,2)),nullptr,16)));return result; } //기대 출력도 별도 최초 열에서 읽는다.
// 호스트 정렬/endian과 무관한 원문 DWORD 입력이다.
std::uint32_t BudgetRead(std::span<const std::uint8_t> bytes,std::size_t at) { std::uint32_t value{};for (std::size_t byte=0;byte<4;++byte) value|=std::uint32_t(bytes[at+byte])<<(byte*8);return value; } //인접 BYTE는 포함하지 않는다.
// 원문 store와 같은 네 BYTE만 바꾼다.
void BudgetPut(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value) { for (std::size_t byte=0;byte<4;++byte) bytes[at+byte]=static_cast<std::uint8_t>(value>>(byte*8)); } //입력의 미사용 모든 BYTE는 보존한다.
struct BudgetStop {}; //가상 Push/Attach 장치의 명시 중단이며 정상 복귀와 구별한다.
// 필요한 실제 장치가 선택된 위치에서 실패했는지 확인한다.
template<class Function> bool BudgetThrows(Function&& function) { try { function();return false; }catch (const std::exception&) { return true; } }
}

// 실제 두 몸체의120입력을 전체 노드/등록표/serial/가상호출순서로 대조한다.
TEST_CASE(StormPowerRefresh_IndependentNative120AndCurrentCoordinates) {
    std::ifstream file(std::filesystem::path(__FILE__).parent_path()/"fixtures/budget-refresh-x86.tsv");CHECK(bool(file));std::string line;std::size_t rows{},stopped{};
    while (std::getline(file,line)) { //없는 대상/루트/첫째·마지막 슬롯/죽음/중복/두 정밀도와 중단을 모두 재생한다.
        if (line.empty() || line.front()=='#') continue;if (line.back()=='\r') line.pop_back();const auto row=BudgetFields(line);CHECK(row.size()==12);if (row.size()!=12) continue;auto bytes=BudgetBytes(row[5]);const auto table=BudgetBytes(row[6]);std::array<std::uint32_t,1000> entries{};std::array<GumpCreateNode,3> nodes{};
        for (std::size_t index=0;index<entries.size();++index) entries[index]=BudgetRead(table,index*4); //최초 raw 조회표의 실제 포인터 토큰이다.
        for (std::size_t index=0;index<nodes.size();++index) { const auto at=index*192;auto& node=nodes[index];node.flags=BudgetRead(bytes,at+4);node.serial=BudgetRead(bytes,at+0x2c);node.depth=BudgetRead(bytes,at+0x30);node.x=std::bit_cast<float>(BudgetRead(bytes,at+0x4c));node.y=std::bit_cast<float>(BudgetRead(bytes,at+0x50)); } //Native에 해당하는 필드만 같은 host 노드에 입력한다.
        auto serial=static_cast<std::uint32_t>(std::stoul(row[4]));std::string calls;bool interrupted{};StormPowerRefreshHooks hooks;
        hooks.node=[&](auto token)->GumpCreateNode* { if (token<kBudgetNodes || (token-kBudgetNodes)%192 || (token-kBudgetNodes)/192>=nodes.size()) throw std::out_of_range("최초 예산 노드 토큰 범위");return &nodes[(token-kBudgetNodes)/192]; };
        const auto effect=[&](bool attach,auto token,std::uint32_t x=0,std::uint32_t y=0,std::uint32_t parent=0,std::uint32_t mode=0) {
            if (!calls.empty()) calls+=';';calls+=(attach?"A:":"P:")+std::to_string(token);if (attach) calls+=':'+std::to_string(x)+':'+std::to_string(y)+':'+std::to_string(parent)+':'+std::to_string(mode);auto& node=*hooks.node(token);
            if (std::stoul(row[1])) { node.depth=attach?0xfedcba98U:0x89abcdefU;if (!attach) { node.x=std::bit_cast<float>(0xc2300000U);node.y=std::bit_cast<float>(0x42b00000U);serial=0xfedcba98U; } }
            if (row[2]==(attach?"attach":"push")) throw BudgetStop{};
        };
        hooks.push=[&](auto token) { effect(false,token); };hooks.attach=[&](auto token,float x,float y,auto parent,auto mode) { effect(true,token,std::bit_cast<std::uint32_t>(x),std::bit_cast<std::uint32_t>(y),parent,mode); };
        try { RefreshStormPowerGump(entries,serial,hooks); }catch (const BudgetStop&) { interrupted=true;++stopped; }
        for (std::size_t index=0;index<nodes.size();++index) { const auto at=index*192;BudgetPut(bytes,at+0x30,nodes[index].depth);BudgetPut(bytes,at+0x4c,std::bit_cast<std::uint32_t>(nodes[index].x));BudgetPut(bytes,at+0x50,std::bit_cast<std::uint32_t>(nodes[index].y)); } //원문store가 접근한 실제 표현만 전체 입력 버퍼에 되돌린다.
        CHECK(bytes==BudgetBytes(row[7]) && serial==std::stoul(row[8]) && (calls.empty()?"-":calls)==row[9] && interrupted==(row[10]=="1"));for (std::size_t index=0;index<entries.size();++index) CHECK(entries[index]==BudgetRead(table,index*4));++rows; //등록표를 새로0으로 만든 결과와 혼동하지 않는다.
    }
    CHECK(rows==120 && stopped==48);
}

// 없는 대상은 가상 장치를 요구하지 않고 선택된 실패에서는 앞선 Push의 변화가 남는다.
TEST_CASE(StormPowerRefresh_DemandAndLateFailureKeepActualPrefix) {
    std::array<std::uint32_t,1000> entries{};RefreshStormPowerGump(entries,0,{});CHECK(BudgetThrows([&] { FindCurrentGumpSerial(std::span(entries).first(999),0,{}); }));GumpCreateNode node;node.serial=71;node.depth=19;node.x=3;node.y=-5;entries[1]=37;StormPowerRefreshHooks hooks;hooks.node=[&](auto token)->GumpCreateNode* { CHECK(token==37);return &node; };
    CHECK(BudgetThrows([&] { RefreshStormPowerGump(entries,71,hooks); }) && node.depth==19);hooks.push=[&](auto token) { CHECK(token==37);node.depth=29;node.x=7; };
    CHECK(BudgetThrows([&] { RefreshStormPowerGump(entries,71,hooks); }) && node.depth==29 && node.x==7);hooks.attach=[&](auto token,float x,float y,auto parent,auto mode) { CHECK(token==37 && x==7 && y==-5 && parent==0 && mode==0);node.depth=31; };node.depth=19;RefreshStormPowerGump(entries,71,hooks);CHECK(node.depth==19 && node.x==7);
}
