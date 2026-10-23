// 전체 Territory/Chaff 순회를 독립 원본 x86의 현재 크기·공유scope·하위 순서와 대조한다.
#include "TestSupport.h"
#include "o/RawFortRegionSave.h"
#include <bit>
#include <fstream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

using namespace netstorm::o;
namespace {
constexpr std::uint32_t kRegionStream=0x12345678U; // 독립 PE 입력과 같은 고정 stream 토큰이다.
// 마지막 빈 칸도 남겨 독립 TSV의 열과 명시 효과를 분리한다.
std::vector<std::string> RegionFields(std::string_view text,char separator) {
    std::vector<std::string> result;std::size_t begin{};
    for (;;) { // 각 구분자의 다음 위치에서 원문 그대로 복사한다.
        const auto end=text.find(separator,begin);result.emplace_back(text.substr(begin,end==text.npos ? text.npos : end-begin));if (end==text.npos) return result;begin=end+1;
    }
}
// 명시 중단 callback은 원본 부분 실행과 같은 현재 전역을 남기며 검사 자체의 실패와 구별한다.
struct RegionStop {};
// 독립 입력의 하위 장치만 제공하고 순회/판정/전역scope 쓰기는 제품 몸체에 맡긴다.
struct RegionDevices {
    std::int32_t width{},height{},region{}; // 현재 지도 크기와 호출자가 고정한 영역 인자다.
    std::uint32_t scope{},owner{}; // 실제 순회가 빌릴 같은 공유scope와 고정 raw owner다.
    std::vector<std::uint8_t> regions;std::vector<std::uint32_t> decisions; // 명시 하위 지도 BYTE와 판정 반환 순서다.
    std::map<std::string,std::size_t> calls;std::vector<std::vector<std::string>> effects;std::string trace; // 하위 경계 호출 수와 입력 효과·실제 순서다.
    // 실제 장치 반환을 캡처한 뒤 동일 경계의 명시 입력 효과를 적용한다.
    void Boundary(std::string event,std::string text) {
        if (!trace.empty()) trace+=';';trace+=text;event+=std::to_string(++calls[event]);
        for (const auto& effect:effects) { // 같은 외부 CALL의 여러 변경을 원문 입력 순서대로 적용한다.
            if (effect[0]!=event) continue;const auto value=std::stoll(effect[2]);
            if (effect[1]=="width") width=std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(value));
            else if (effect[1]=="height") height=std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(value));
            else if (effect[1]=="scope") scope=static_cast<std::uint32_t>(value);
            else { CHECK(effect[1]=="stop" && value==1);throw RegionStop{}; }
        }
    }
    // 앞10열만 입력으로 사용하며 뒤의 원문 결과로 장치 반환/분기를 만들지 않는다.
    void Initialize(const std::vector<std::string>& row) {
        width=std::stoi(row[2]);height=std::stoi(row[3]);scope=static_cast<std::uint32_t>(std::stoul(row[4]));owner=static_cast<std::uint32_t>(std::stoul(row[5]));region=std::stoi(row[6]);
        for (std::size_t index=0;index<row[7].size();index+=2) regions.push_back(static_cast<std::uint8_t>(std::stoul(row[7].substr(index,2),nullptr,16))); // 원본에 공급한 같은 지도 BYTE만 해석한다.
        for (const auto& value:RegionFields(row[8],',')) decisions.push_back(static_cast<std::uint32_t>(std::stoul(value))); // 반환값의0/양수/부호비트를 유지한다.
        if (row[9]!="-") for (const auto& effect:RegionFields(row[9],';')) effects.push_back(RegionFields(effect,'/')); // 명시 외부효과만 기록한다.
    }
    // getter가 원문 MOV 시점에 캡처한 값을 돌려주고 변경 후 전역은 다음 getter가 관측한다.
    FortRegionSaveHooks Hooks() {
        FortRegionSaveHooks hooks;
        hooks.chunksX=[this] { const auto value=width;Boundary("x","X:"+std::to_string(value)+':'+std::to_string(scope));return value; };
        hooks.chunksY=[this] { const auto value=height;Boundary("y","Y:"+std::to_string(value)+':'+std::to_string(scope));return value; };
        hooks.chunkRegion=[this](auto x,auto y) { CHECK(x>=0 && x<8 && y>=0 && y<8);const auto byte=regions.at(static_cast<std::size_t>(y*8+x)%regions.size());Boundary("cell","cell:"+std::to_string(x)+':'+std::to_string(y)+':'+std::to_string(byte)+':'+std::to_string(scope));return byte; };
        hooks.allowed=[this](auto byte,auto selectedOwner,auto selectedRegion) { const auto answer=decisions.at(calls["allow"]%decisions.size());Boundary("allow","allow:"+std::to_string(byte)+':'+std::to_string(selectedOwner)+':'+std::to_string(selectedRegion)+':'+std::to_string(answer)+':'+std::to_string(scope));return answer; };
        hooks.chunk=[this](auto stream,auto x,auto y) { Boundary("chunk","chunk:"+std::to_string(stream)+':'+std::to_string(x)+':'+std::to_string(y)+':'+std::to_string(scope)); };
        return hooks;
    }
};
// 누락된 실제 장치만 명시 실패하는지 확인하며 원문 부분 중단과 구별한다.
template<class Function> bool RegionRejects(Function function) { try { function(); } catch (const std::logic_error&) { return true; }return false; }
}

// 두 몸체의 signed BYTE/크기 읽기 위치·scope·선택/전체 저장·부분 실패를 원문930입력과 비교한다.
TEST_CASE(FortRegionSave_IndependentNativeTraversal) {
    std::ifstream input(NETSTORM_FORT_REGION_SAVE_FIXTURE);CHECK(input.good());std::string line;std::size_t compared{};
    while (std::getline(input,line)) { // 앞10열 입력과 뒤8열 실제 실행 관측을 분리한다.
        if (line.empty() || line.starts_with('#')) continue;const auto row=RegionFields(line,'\t');CHECK(row.size()==18);if (row.size()!=18) continue;
        RegionDevices devices;devices.Initialize(row);const auto failures=netstorm::test::FailureCount();bool completed=true;
        try {
            if (row[1]=="territory") SaveFortTerritoryChunks(kRegionStream,devices.owner,devices.region,devices.scope,devices.Hooks());
            else { CHECK(row[1]=="chaff");SaveFortChaffChunks(kRegionStream,devices.scope,devices.Hooks()); }
        } catch (const RegionStop&) { completed=false; }
        CHECK(completed==(row[10]=="1"));CHECK(devices.width==std::stoi(row[11]));CHECK(devices.height==std::stoi(row[12]));CHECK(devices.scope==std::stoul(row[13]));CHECK((devices.trace.empty() ? "-" : devices.trace)==row[14]);
        CHECK(completed ? std::stoi(row[15])==0 : std::stoi(row[15])<0);CHECK(std::stoul(row[16])>0);CHECK(std::stoul(row[17])<=1);
        if (row[1]=="chaff") CHECK(devices.calls["allow"]==0);++compared;
        if (netstorm::test::FailureCount()!=failures) std::fprintf(stderr,"전체 Fort 청크 입력: %s\n",row[0].c_str());
    }
    CHECK(compared==930);
}
// 빈 지도도 scope는 쓰지만 미도달 getter/지도/판정/저장 장치를 강제로 요구하지 않는다.
TEST_CASE(FortRegionSave_EmptyAndMissingEffects) {
    std::uint32_t scope=77;FortRegionSaveHooks hooks;hooks.chunksY=[&] { CHECK(scope==77);return 0; };
    SaveFortTerritoryChunks(kRegionStream,1,2,scope,hooks);CHECK(scope==1);
    scope=77;SaveFortChaffChunks(kRegionStream,scope,hooks);CHECK(scope==2);
    scope=77;hooks.chunksY={};CHECK(RegionRejects([&] { SaveFortChaffChunks(kRegionStream,scope,hooks); }));CHECK(scope==77);
    hooks.chunksY=[] { return 1; };CHECK(RegionRejects([&] { SaveFortTerritoryChunks(kRegionStream,1,2,scope,hooks); }));CHECK(scope==1);
    std::size_t rows{};hooks.chunksX=[] { return 0; };hooks.chunksY=[&] { ++rows;return 2; };scope=77;SaveFortChaffChunks(kRegionStream,scope,hooks);CHECK(scope==2);CHECK(rows==3);
}
// Chaff는 영역 반환/판정은 쓰지 않지만 실제 지도 조회가 쓰는 같은 scope와 크기 변경은 생략하지 않는다.
TEST_CASE(FortRegionSave_ChaffKeepsLookupEffects) {
    std::uint32_t scope=77;std::int32_t width=2;std::size_t queries{},writes{};FortRegionSaveHooks hooks;
    hooks.chunksY=[] { return 1; };hooks.chunksX=[&] { return width; };
    hooks.chunkRegion=[&](auto x,auto y) { CHECK(x==0 && y==0);CHECK(scope==2);scope=9;width=0;++queries;return std::uint8_t{255}; };
    hooks.allowed=[](auto,auto,auto)->std::uint32_t { throw std::logic_error("Chaff에서 영역 판정을 호출했습니다"); };
    hooks.chunk=[&](auto stream,auto x,auto y) { CHECK(stream==kRegionStream && x==0 && y==0);CHECK(scope==9);++writes; };
    SaveFortChaffChunks(kRegionStream,scope,hooks);CHECK(scope==9);CHECK(queries==1 && writes==1);
}
