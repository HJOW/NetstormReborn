// 실제 원문이 계산한 재귀/서식/현재 전역 결과를 같은 독립 입력으로 재생한다.
#include "TestSupport.h"
#include "o/RawObjectTrace.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace { // 합성 슬롯 입력과 외부 출력 기록만 보유하며 기대 결과는 fixture에서 읽는다.
using namespace netstorm::o;
// 바이트를 문자 인코딩 변환 없이 TSV의 단일 열로 표현한다.
std::string Hex(std::string_view bytes) {
    constexpr char digits[]="0123456789abcdef";std::string result; // 각 낮은8비트 값을 두 자리로 보존하는 표다.
    for (const unsigned char value:bytes) { result.push_back(digits[value>>4]);result.push_back(digits[value&15]); } // 출력 바이트 전체를 순서대로 기록한다.
    return result;
}
// 고정 fixture의 탭/쉼표/노드 구분자를 분리하며 빈 문자열도 보존한다.
std::vector<std::string> Split(const std::string& value,char separator) {
    std::stringstream stream(value);std::vector<std::string> result;std::string field; // 현재 열과 원문 순서의 결과다.
    while (std::getline(stream,field,separator)) result.push_back(field); // 입력에 적힌 모든 열을 빠짐없이 읽는다.
    return result;
}
// 동일한50BYTE 물리 풀·현재 전역과 외부 변형 번호를 보유하는 독립 입력 장면이다.
struct TraceScene {
    SidPool pool{OriginalEdition::Patch1078,24000,true}; // free/dead/void도 원문이 읽는 물리 슬롯이다.
    std::uint32_t sid{},count{},depth{},indent{},checking{},metamap{},mutation{},calls{}; // 진입번호/현재 전역과 가상 호출 입력이다.
    bool boss{}; // 현재 boss DWORD의 참/거짓 값이다.
    std::string events; // 호출된 실제 장치의 순서/인자를 보존하는 기록이다.
    // 검사 입력을 설치할 물리 풀의 현재 슬롯을 빌린다.
    std::span<std::uint8_t> Raw(std::uint32_t number) { const auto bytes=pool.Bytes();return std::span(const_cast<std::uint8_t*>(bytes.data()),bytes.size()).subspan(number*50,50); }
    // 정렬되지 않은 원문 링크 WORD만 바꾸고 주변 바이트를 보존한다.
    void Word(std::uint32_t number,std::size_t offset,std::uint32_t value) { const auto raw=Raw(number);raw[offset]=static_cast<std::uint8_t>(value);raw[offset+1]=static_cast<std::uint8_t>(value>>8); }
    // 명시된 원문 외부 가상 호출 반환 시점에만 링크/전역을 바꾼다.
    void Mutate(std::uint32_t number) {
        if (calls==1) {
            switch (mutation) {
            case 1:Word(number,6,7);break;case 2:Word(3,4,7);break;case 3:Word(4,4,7);break;
            case 4:depth=41;break;case 5:Raw(3)[11]=1;break;case 6:Raw(4)[10]=70;break;case 7:count=4;break;default:break;
            }
        }
        if (calls==2 && mutation==8) Word(3,4,7);
        if (calls==2 && mutation==9) Word(4,4,7);
    }
    // 이벤트 구분자를 앞선 기록이 있을 때만 추가한다.
    void Event(const std::string& value) { if (!events.empty()) events+=';';events+=value; }
    // 원문 도구의 입력 열만 읽고 예상 출력은 보관하지 않는다.
    explicit TraceScene(const std::vector<std::string>& columns) {
        sid=static_cast<std::uint32_t>(std::stoul(columns.at(0)));count=static_cast<std::uint32_t>(std::stoul(columns.at(1)));
        depth=static_cast<std::uint32_t>(std::stoul(columns.at(2)));indent=static_cast<std::uint32_t>(std::stoul(columns.at(3)));
        checking=static_cast<std::uint32_t>(std::stoul(columns.at(4)));metamap=static_cast<std::uint32_t>(std::stoul(columns.at(5)));
        boss=std::stoul(columns.at(6))!=0;mutation=static_cast<std::uint32_t>(std::stoul(columns.at(7)));
        for (const auto& node:Split(columns.at(8),'/')) { // 현재 TYPE·상태와 head/next를 같은 원문 물리 폭으로 설치한다.
            const auto fields=Split(node,',');const auto number=static_cast<std::uint32_t>(std::stoul(fields.at(0)));
            Raw(number)[10]=static_cast<std::uint8_t>(std::stoul(fields.at(1)));Raw(number)[11]=static_cast<std::uint8_t>(std::stoul(fields.at(2)));
            Word(number,6,static_cast<std::uint32_t>(std::stoul(fields.at(3))));Word(number,4,static_cast<std::uint32_t>(std::stoul(fields.at(4))));
        }
    }
    // 현재 장면 전역의 주소를 빌리므로 외부 효과 이후의 읽기도 즉시 반영된다.
    ObjectTraceGlobals Globals() { return {depth,count,checking,metamap,boss}; }
    // 원문 함수 호출 위치에서 가상 출력/들여쓰기/로그/assert 인자를 기록한다.
    ObjectTraceHooks Hooks() {
        return {
            [this](Sid value) { Event("V:"+std::to_string(value.value)+":"+std::to_string(depth)+":"+std::to_string(indent));++calls;Mutate(value.value); },
            [this](std::uint32_t value) { Event("I:"+std::to_string(value));indent=value; },
            [this](std::int32_t level,std::string_view line) { Event("L:"+std::to_string(level)+":"+Hex(line)+":"+std::to_string(depth)+":"+std::to_string(indent));if (mutation==10) depth+=2; },
            [this] { Event("A:"+Hex("inMetamapMode || !cont.isDead() || boss || yesIKnowThatIShouldntLookInsideMyselfDuringADestroy")); }
        };
    }
};
}

// TYPE0..69·끝 번호·free/dead·다단 링크·현재 콜백 변경의 모든 원문 경계를 비교한다.
TEST_CASE(ObjectTrace_IndependentNative103RecursiveFormsAndCurrentGlobals) {
    std::ifstream input(NETSTORM_OBJECT_TRACE_FIXTURE);CHECK(bool(input));std::string line;std::size_t rows{}; // 동결 원문의 전체 행을 센다.
    while (std::getline(input,line)) { // 두 정밀도가 일치한 고유 입력을 모두 실제 C++ 함수로 실행한다.
        if (!line.empty() && line.back()=='\r') line.pop_back();if (line.empty() || line[0]=='#') continue;
        const auto columns=Split(line,'\t');TraceScene scene(columns);
        TraceRawObject(scene.pool,scene.Globals(),scene.Hooks(),scene.sid);
        CHECK(scene.events+"|"+std::to_string(scene.depth)+"|"+std::to_string(scene.indent)==columns.at(9));++rows;
    }
    CHECK(rows==103);
}

// 미연결 장치/손상 체인을 거부할 때 앞선 가상 출력과 깊이를 되돌리지 않는다.
TEST_CASE(ObjectTrace_HostErrorsPreservePriorEffectsAndZeroNeedsNoDevices) {
    const auto columns=Split("2\t20\t0\t17\t0\t0\t1\t0\t2,82,0,3,0/3,46,0,0,0",'\t');TraceScene scene(columns);
    auto hooks=scene.Hooks();hooks.indent={};bool threw=false; // 가상 trace 뒤의 선택된 setter만 누락시킨다.
    try { TraceRawObject(scene.pool,scene.Globals(),hooks,2); }catch (const std::logic_error&) { threw=true; }
    CHECK(threw && scene.depth==3 && scene.events=="V:2:0:17");
    TraceRawObject(scene.pool,scene.Globals(),{},0);CHECK(scene.depth==3);
    TraceScene cycle(columns);cycle.Raw(2)[10]=69;cycle.Word(3,6,2);threw=false;
    try { TraceRawObject(cycle.pool,cycle.Globals(),cycle.Hooks(),2); }catch (const std::logic_error&) { threw=true; }
    CHECK(threw && cycle.depth==6 && cycle.calls==2);
    TraceScene range(columns);range.Word(2,6,65535);threw=false;
    try { TraceRawObject(range.pool,range.Globals(),range.Hooks(),2); }catch (const std::out_of_range&) { threw=true; }
    CHECK(threw && range.depth==3 && range.calls==1);
}
