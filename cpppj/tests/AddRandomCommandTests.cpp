// 대본 일반 입력과 고정 범위 부모를 독립 원문 관측에 대조한다.
#include "TestSupport.h"
#include "client/AddRandomInput.h"
#include "client/GameMenuCommands.h"
#include "client/ScriptCommand.h"
#include "o/RawAddRandom.h"
#include "o/RawFortTerritorySave.h"
#include <array>
#include <bit>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace { // 원문 필드와 호출 기록만 복원하는 검사 도구다.
// TSV의 입력/관측 순서를 보존한다.
std::vector<std::string> AddFields(std::string_view line) {
    std::vector<std::string> result;std::size_t begin{};
    for (;;) { const auto end=line.find('\t',begin);result.emplace_back(line.substr(begin,end==line.npos ? line.npos : end-begin));if (end==line.npos) return result;begin=end+1; } // 마지막 빈 필드도 보존한다.
}
// 같은 원시 DWORD를 넘침 없이 복원한다.
std::uint32_t AddNumber(std::string_view value) { return static_cast<std::uint32_t>(std::stoul(std::string(value))); }
// 실제 경계 호출을 원문과 같은 구분 순서로 기록한다.
void AddTrace(std::string& trace,std::string entry) { if (!trace.empty()) trace+=';';trace+=entry; }
// 원문 입력의 세 섬 기록을 정확한 BYTE/DWORD 위치에 설치한다.
std::array<std::uint8_t,3204> AddIslands() {
    std::array<std::uint8_t,3204> bytes{};
    const auto put=[&](std::size_t at,std::uint32_t value) { for (std::size_t i=0;i<4;++i) bytes[at+i]=static_cast<std::uint8_t>(value>>(i*8)); }; // 원문 little endian의 네BYTE만 쓴다.
    put(0,3);const std::array<std::uint32_t,3> owners{2,1,0},originals{1,1,9};
    for (std::size_t index=0;index<3;++index) { const auto at=4+index*32;put(at+4,1);put(at+8,owners[index]);bytes[at+13]=static_cast<std::uint8_t>(originals[index]); } // 지역BYTE는0이며 현재/원래owner는 다르다.
    return bytes;
}
}

// 보호 원문의 signed255 비교·현재모드·뗌·조회 실패 결과 전달을 대조한다.
TEST_CASE(AddRandom_Native288InputAndActualQueue) {
    using namespace netstorm::client;
    std::ifstream file(NETSTORM_ADD_RANDOM_INPUT_FIXTURE);if (!file) throw std::runtime_error("AddRandom 입력 fixture 열기 실패");
    std::string line;std::getline(file,line);std::size_t count{};
    while (std::getline(file,line)) { // 앞4열만 입력으로 사용하고 뒤 관측은 기대값으로 읽는다.
        const auto row=AddFields(line);CHECK(row.size()==7);if (row.size()!=7) continue;
        const auto code=AddNumber(row[0]),parameter=AddNumber(row[1]),mode=AddNumber(row[2]),lookup=AddNumber(row[3]);std::string trace;
        CHECK(DispatchAddRandomInput({code,305419896,std::bit_cast<std::int32_t>(parameter)},(mode&1)!=0,(mode&2)!=0,
            [&](auto token) { AddTrace(trace,"lookup:"+std::to_string(token));return std::bit_cast<std::int32_t>(lookup); },
            [&](auto type,auto quantity) { AddTrace(trace,"add:"+std::to_string(type)+':'+std::to_string(quantity)); }));
        CHECK((trace.empty() ? "-" : trace)==row[4] && row[5]=="1" && row[6]=="0");++count;
    }
    CHECK(count==288);InputQueue queue;ScriptStringRing strings;
    QueueScriptCommand("aDdRaNdOm,100",queue,strings,{GameMenuScriptCode,{}});auto event=queue.Pop(false);CHECK(event.code==kAddRandomCommand && event.x==100 && event.y==100 && IsGameMenuCommand(event.code));
    QueueScriptCommand("AddRandom,sunCannon",queue,strings,{GameMenuScriptCode,{}});event=queue.Pop(false);CHECK(event.code==kAddRandomCommand && event.x==event.y && strings.Resolve(std::bit_cast<std::uint32_t>(event.y))=="sunCannon");CHECK(GameMenuScriptCode("AddRandoms")==0xffffffffU);
}

// 실제 seed/기본 범위/섬 검색을 실행한720개 원문 부모의 전체 인자를 대조한다.
TEST_CASE(AddRandom_Native720ParentUsesCurrentOwnerAfterClock) {
    using namespace netstorm::o;
    std::ifstream file(NETSTORM_ADD_RANDOM_PARENT_FIXTURE);if (!file) throw std::runtime_error("AddRandom 부모 fixture 열기 실패");
    std::string line;std::getline(file,line);std::size_t count{};
    while (std::getline(file,line)) { // 기대seed/범위/지역을 다시 계산하지 않고 호출 결과와 비교한다.
        const auto row=AddFields(line);CHECK(row.size()==9);if (row.size()!=9) continue;
        const auto type=AddNumber(row[0]),quantity=AddNumber(row[1]),low=AddNumber(row[2]),high=AddNumber(row[3]);auto owner=AddNumber(row[4]);bool fort=AddNumber(row[5])!=0;
        const auto time=std::bit_cast<double>((std::uint64_t(high)<<32)|low);const auto islands=AddIslands();std::string trace;AddRandomHooks hooks;
        hooks.wallSeconds=[&] { AddTrace(trace,"T:"+std::to_string(low)+':'+std::to_string(high));if (AddNumber(row[6])) { owner=2;fort=true; }return time; };
        hooks.local=[&] { return owner; };hooks.island=[&](auto current,auto territory) { AddTrace(trace,"R:"+std::to_string(current)+':'+std::to_string(territory));return FindFortTerritoryIsland(islands,fort,std::bit_cast<std::int32_t>(current),territory); };
        hooks.place=[&](auto& random,const auto& area,auto region,auto amount,auto selected,auto force) {
            AddTrace(trace,"B:"+std::to_string(random.State())+':'+std::to_string(std::bit_cast<std::uint32_t>(area.left))+':'+std::to_string(std::bit_cast<std::uint32_t>(area.top))+':'+std::to_string(std::bit_cast<std::uint32_t>(area.right))+':'+std::to_string(std::bit_cast<std::uint32_t>(area.bottom))+':'+std::to_string(std::bit_cast<std::uint32_t>(region))+':'+std::to_string(amount)+':'+std::to_string(selected)+':'+std::to_string(force));return 7U;
        };
        AddRandomObjects(type,quantity,hooks);CHECK(trace==row[7] && row[8]=="4");++count;
    }
    CHECK(count==720);
}

// 선택되지 않은 하위와 이미 실행된 벽시각 효과를 잘못 처리하지 않는다.
TEST_CASE(AddRandom_OnlyReachedDevicesAreRequired) {
    using namespace netstorm::client;
    CHECK(!DispatchAddRandomInput({0x02000017U,0,100},false,false,{},{}));CHECK(DispatchAddRandomInput({kAddRandomCommand|InputCode::kRelease,0,100},true,true,{},{}));CHECK(DispatchAddRandomInput({kAddRandomCommand,0,100},false,false,{},{}));
    bool missing{};try { static_cast<void>(DispatchAddRandomInput({kAddRandomCommand,0,100},true,false,{},{})); }catch (const std::logic_error&) { missing=true; }CHECK(missing);
    std::size_t clocks{};netstorm::o::AddRandomHooks hooks;hooks.wallSeconds=[&] { ++clocks;return 0.0; };
    missing=false;try { netstorm::o::AddRandomObjects(89,1,hooks); }catch (const std::logic_error&) { missing=true; }CHECK(missing && clocks==1);
}

// 반환값은 원문 0040dfd0이 eax로 넘기는 마지막 배치 결과이며 Fort 섬 채우기(0046e630)가 임시 객체 SID로 읽어 삭제한다(M6-n). 실패는 0이다.
TEST_CASE(AddRandom_ReturnsLastPlacementResult) {
    using namespace netstorm::o;
    AddRandomHooks hooks;hooks.wallSeconds=[] { return 0.0; };hooks.local=[] { return 1U; };hooks.island=[](auto,auto) { return 0; };
    std::uint32_t seenType{},seenQuantity{};
    hooks.place=[&](auto&,const auto&,auto,auto amount,auto selected,auto) { seenType=selected;seenQuantity=amount;return 0x1234U; };
    CHECK(AddRandomObjects(87,1,hooks)==0x1234U && seenType==87 && seenQuantity==1);
    hooks.place=[](auto&,const auto&,auto,auto,auto,auto) { return 0U; };
    CHECK(AddRandomObjects(87,1,hooks)==0U);
}
