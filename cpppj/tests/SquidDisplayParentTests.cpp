// 전체 표시 부모의 두 TYPE 조회/현재 진단/지역 frame 고정을 저장 원문과 대조한다.
#include "RawSceneSupport.h"
#include "o/RawSquidDisplayParent.h"
using namespace netstorm::test::rawscene;
using namespace netstorm::o;
namespace { // 원문에 준 입력과 명시 callback 효과만 준비한다.
struct DisplayParentType { // 같은 TYPE 필드 주소를 유지하는 검사 전용 저장소다.
    std::uint32_t flags{};std::int32_t count{4};bool loaded{true};std::int32_t header{100}; // 부모가 실제 읽는 네 필드다.
};
std::vector<std::uint32_t> DisplayParentNumbers(std::string_view text) { // signed 입력의 전체 DWORD 비트를 보존한다.
    std::vector<std::uint32_t> values; // 열 순서의 원문 입력이다.
    for (const auto& word:Split(text,',')) values.push_back(Number(word));return values; // 각 입력을 하나씩 읽으며 기대값은 계산하지 않는다.
}
std::string DisplayParentHex(std::string_view text) { // 원문 식/파일의 ASCII 바이트를 동일한 hex 사건으로 기록한다.
    return Hex(std::span(reinterpret_cast<const std::uint8_t*>(text.data()),text.size()));
}
}

//696개 전체 부모 출력의 raw/타입필드/전역/모든 효과와 진단 중단을 비교한다.
TEST_CASE(SquidDisplayParent_NativeWholeParent696) {
    std::size_t count{}; // 실제 원문 고유 행 수다.
    for (const auto& row:LoadFixture(NETSTORM_SQUID_DISPLAY_PARENT_FIXTURE).rows) { // 보호 PE나 게임을 실행하지 않고 최초 관측만 읽는다.
        CHECK(row.size()==6);const auto input=DisplayParentNumbers(row[0]);CHECK(input.size()==7);std::array<std::uint8_t,50> raw{};std::array<DisplayParentType,2> types{};auto checking=input[0];std::uint32_t lookups{};bool mutated{};std::string trace,outcome="return";
        for (std::size_t index=0;index<raw.size();++index) raw[index]=static_cast<std::uint8_t>(index*17+3); // 무관한 바이트까지 같은 표식으로 대조한다.
        raw[10]=70;raw[40]=5;Put(raw,36,input[1]);Put(raw,14,0x40500000);Put(raw,18,0xc0200000);types[0]={input[2],4,input[3]!=0,std::bit_cast<std::int32_t>(input[4])};types[1]={0x440000U,2,true,200};
        const auto event=[&](std::string value) { if (!trace.empty()) trace+=';';trace+=value; }; // 효과 인수는 모두 원문 DWORD 십진수다.
        const auto mutate=[&](std::uint32_t kind) {
            if (!kind || mutated || input[5]!=kind) return;mutated=true;checking=0;raw[10]=71;Put(raw,36,2);Put(raw,14,0x40f00000);Put(raw,18,0x3f400000);raw[40]=0;
            if (kind==3) { types[0].count=1;types[0].header=std::bit_cast<std::int32_t>(0xddddddddU); }
        }; // 지역 frame을 재계산하지 않고 같은 현재 raw/전역만 바꾸는 명시 장치다.
        RawSquidDisplayParentHooks hooks; // 큰 자식 결과는 입력이며 부모의 조건은 제품 함수가 계산한다.
        hooks.type=[&](std::uint32_t selected) { CHECK(selected==70 || selected==71);event("T:"+std::to_string(selected));++lookups;mutate(lookups==1 ? 1U : 0U);if (lookups==(input[0] ? 2U : 1U)) mutate(5);const auto& type=types[selected-70];return RawSquidDisplayTypeView{selected,type.flags,type.count,type.loaded,type.header}; };
        hooks.describe=[&](RawSquidDisplayTypeView type,std::int32_t frame,std::uint32_t abstract) { const auto bits=std::bit_cast<std::uint32_t>(frame);event("F:"+std::to_string(type.token)+':'+std::to_string(bits)+':'+std::to_string(abstract));mutate(2);return 0x70ad0000U+(type.token-70)*0x100U+(bits&255U); };
        hooks.log=[&](std::uint32_t token) { event("L:"+std::to_string(token));mutate(3); };
        hooks.assertion=[&](std::string_view expression,std::string_view file,std::uint32_t line) { event("D:"+DisplayParentHex(expression)+':'+DisplayParentHex(file)+':'+std::to_string(line));if (!input[6]) throw std::runtime_error("명시 진단 중단");mutate(4); };
        hooks.mark=[&](std::uint32_t type,std::int32_t frame,std::uint32_t x,std::uint32_t y,std::uint32_t flags) { event("M:"+std::to_string(type)+':'+std::to_string(std::bit_cast<std::uint32_t>(frame))+':'+std::to_string(x)+':'+std::to_string(y)+':'+std::to_string(flags)); };
        try { RunRawSquidDisplayParent(raw,RawCheckingFlag{checking},hooks); } catch (const std::runtime_error& error) { if (std::string_view(error.what())!="명시 진단 중단") throw;outcome="assertion"; }
        std::string fields; // 두 현재 TYPE의 네 필드를 원문 순서로 직렬화한다.
        for (const auto& type:types) { // 타입 주소별 전체 비교 필드를 보존한다.
            for (const auto value:{std::bit_cast<std::uint32_t>(type.header),type.flags,std::bit_cast<std::uint32_t>(type.count),type.loaded ? 1U : 0U}) fields+=(fields.empty() ? "" : ",")+std::to_string(value);
        }
        CHECK(trace==row[1]);CHECK(Hex(raw)==row[2]);CHECK(fields==row[3]);CHECK(checking==Number(row[4]));CHECK(outcome==row[5]);++count;
    }
    CHECK(count==696);
}

// 현재 조건이 선택하지 않은 로그/진단/표시 장치는 요구하지 않고 실제 누락은 명시한다.
TEST_CASE(SquidDisplayParent_SelectedDevicesAndStableBorrowedFields) {
    std::array<std::uint8_t,50> raw{};raw[10]=70;Put(raw,36,0xffffffffU);DisplayParentType type;std::uint32_t checking{};RawSquidDisplayParentHooks hooks;std::uint32_t calls{}; // 단일 타입 주소를 직접 빌리는 독립 입력이다.
    hooks.type=[&](std::uint32_t number) { CHECK(number==70);++calls;return RawSquidDisplayTypeView{70,type.flags,type.count,type.loaded,type.header}; };RunRawSquidDisplayParent(raw,RawCheckingFlag{checking},hooks);CHECK(calls==1);
    checking=1;CHECK(Throws([&] { RunRawSquidDisplayParent(raw,RawCheckingFlag{checking},hooks); })); // 선택된 describe 누락에서 중단한다.
    Put(raw,36,0);type.loaded=false;checking=0;RunRawSquidDisplayParent(raw,RawCheckingFlag{checking},hooks);checking=1;CHECK(Throws([&] { RunRawSquidDisplayParent(raw,RawCheckingFlag{checking},hooks); })); // SHP만 없으면 describe 없이 assertion을 요구한다.
    type.loaded=true;checking=0;CHECK(Throws([&] { RunRawSquidDisplayParent(raw,RawCheckingFlag{checking},hooks); })); // 정상 프레임은 실제 mark를 요구한다.
    CHECK(Throws([&] { RunRawSquidDisplayParent(std::span(raw).first(49),RawCheckingFlag{checking},hooks); }));
}
