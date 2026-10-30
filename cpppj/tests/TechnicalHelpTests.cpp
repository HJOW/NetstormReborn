// 원문 부모/경로 선택/별도 조합 자식의 최초 trace와 출력을 대조한다.
#include "TestSupport.h"
#include "client/TechnicalHelp.h"
#include "client/GameMenuCommands.h"
#include <fstream>
#include <vector>
#include <stdexcept>
namespace { //각 행의 명시 응답만 공급하고 제품이 호출 순서를 결정한다.
using namespace netstorm::client;using netstorm::o::OriginalEdition; //같은 실제 정책과 판본이다.
struct Stop {}; //효과 진입에서 본문을 멈추는 명시 관측 신호다.
std::vector<std::string> Fields(std::string_view line) { //마지막 빈 trace 열도 보존한다.
    std::vector<std::string> result;std::size_t begin{};for (;;) { const auto end=line.find('\t',begin);result.emplace_back(line.substr(begin,end==line.npos ? line.npos : end-begin));if (end==line.npos) return result;begin=end+1; } //원래 구분자 위치를 순회한다.
}
std::string Bytes(std::string_view value) { //'-'는 빈 문자열이며 NULL의 null 여부는 별도로 처리한다.
    std::string result;if (value=="-") return result;for (std::size_t i=0;i<value.size();i+=2) result+=static_cast<char>(std::stoul(std::string(value.substr(i,2)),nullptr,16));return result; //실제 BYTE를 복원한다.
}
std::string Hex(std::string_view value) { //원래 CString의 첫NUL까지만 출력한다.
    constexpr std::string_view digits="0123456789abcdef";std::string result;value=value.substr(0,value.find('\0'));for (const unsigned char byte:value) { result+=digits[byte>>4];result+=digits[byte&15]; }return result; //같은 바이트를 소문자 두 자리로 기록한다.
}
std::uint32_t Number(std::string_view value) { return static_cast<std::uint32_t>(std::stoull(std::string(value))); } //원문 DWORD 반환/출처를 보존한다.
}
// 실제435e40와 가용성/실행 래퍼84개를 전체 trace/부분 출력으로 비교한다.
TEST_CASE(TechnicalHelp_Native84AvailabilityLaunchAndPartialOutputs) {
    std::ifstream input(NETSTORM_TECHNICAL_HELP_FIXTURE);CHECK(input.good());std::string line;std::size_t count{};
    while (std::getline(input,line)) { //각 독립 원문 행은 새 출력/명시 응답을 사용한다.
        if (!line.empty() && line.back()=='\r') line.pop_back();const auto f=Fields(line);CHECK(f.size()==14);if (f.size()!=14) return;
        std::string trace,exe="OLD-EXE",file="OLD-FILE";std::uint32_t events{},resolveIndex{};const auto stop=Number(f[8]);
        const auto event=[&](std::string value) { if (!trace.empty()) trace+=';';trace+=value;if (++events==stop) throw Stop{}; }; //효과 적용 전 진입이다.
        TechnicalHelpHooks h;h.readString=[&](auto key,auto& output) { event("R,"+Hex(key)+','+Hex(output));const auto index=key=="techHelpFile" ? 1U : 0U;output=Bytes(f[4+index]);return Number(f[2+index]); };
        h.resolve=[&](auto value)->std::optional<std::string> { event("Q,"+Hex(value));const auto& result=f[6+resolveIndex++];return result=="NULL" ? std::nullopt : std::make_optional(Bytes(result)); };
        h.launch=[&](auto program,auto argument) { event("L,"+Hex(program)+','+Hex(argument)); };
        bool completed=true,result{};try { result=f[1]=="read" ? ResolveTechnicalHelp(OriginalEdition::Patch1078,exe,file,h) : f[1]=="available" ? TechnicalHelpAvailable(OriginalEdition::Patch1078,h) : LaunchTechnicalHelp(OriginalEdition::Patch1078,h); }catch (const Stop&) { completed=false; }
        CHECK(completed==(f[10]=="1"));if (completed) CHECK(result==(Number(f[9])!=0));CHECK(trace==f[13]);if (f[1]=="read") { CHECK(Hex(exe)==Hex(Bytes(f[11])));CHECK(Hex(file)==Hex(Bytes(f[12]))); }++count;
    }CHECK(count==84);
}
// 실제41ad10의64개 선택/출처 출력과 실패 시77 보존을 비교한다.
TEST_CASE(TechnicalHelp_Native64ExternalFileSourcesAndFailurePreservation) {
    std::ifstream input(NETSTORM_EXTERNAL_FILE_FIXTURE);CHECK(input.good());std::string line;std::size_t count{};
    while (std::getline(input,line)) { //다섯 입력 bool과 출처 포인터 유무를 독립적으로 제공한다.
        if (!line.empty() && line.back()=='\r') line.pop_back();const auto f=Fields(line);CHECK(f.size()==11);if (f.size()!=11) return;std::string trace;const auto event=[&](std::string value) { if (!trace.empty()) trace+=';';trace+=value; }; //실제 선택된 경계만 기록한다.
        ExternalFileHooks h;h.device=[&](auto name) { event("D,"+Hex(name));return f[2]=="1"; };h.path=[&](bool secondary,auto name) { event("P,"+std::to_string(secondary)+','+Hex(name));return std::string(secondary ? "D:\\disc\\" : "C:\\base\\")+std::string(name); };
        h.readable=[&](auto path) { event("F,"+Hex(path));return f[path.starts_with("D:") ? 5 : 3]=="1"; };h.archive=[&](bool secondary,auto name) { event("A,"+std::to_string(secondary)+','+Hex(name));return f[secondary ? 6 : 4]=="1"; };
        std::uint32_t source=77;const auto result=ResolveExternalFilePath(Bytes(f[1]),f[7]=="1" ? &source : nullptr,h);CHECK(result.has_value()==(f[8]!="NULL"));if (result) CHECK(*result==Bytes(f[8]));CHECK(source==Number(f[9]));CHECK(trace==f[10]);++count;
    }CHECK(count==64);
}
// 실제459c60의 별도28개 결과로 특수 -2/-3 조합을 비교한다.
TEST_CASE(TechnicalHelp_Native28ExternalRootJoin) {
    std::ifstream input(NETSTORM_EXTERNAL_JOIN_FIXTURE);CHECK(input.good());std::string line;std::size_t count{};
    while (std::getline(input,line)) { //빈 루트/장치/상대/UNC와 끝 구분자 중복을 구별한다.
        if (!line.empty() && line.back()=='\r') line.pop_back();const auto f=Fields(line);CHECK(f.size()==3);if (f.size()!=3) return;CHECK(JoinExternalFileRoot(Bytes(f[0]),Bytes(f[1]))==Bytes(f[2]));++count;
    }CHECK(count==28);
}
// 미선택 실행/조회는 요구하지 않고 실제 명령의 누름·뗌과 판본을 구별한다.
TEST_CASE(TechnicalHelp_SelectedMissingDevicesReleaseAndCd) {
    std::uint32_t calls{};TechnicalHelpHooks h;h.readString=[&](auto,auto&)->std::uint32_t { ++calls;return 0; };CHECK(!TechnicalHelpAvailable(OriginalEdition::Patch1078,h) && calls==2);
    CHECK(DispatchTechnicalHelpInput(0x82000049U,{}) && !DispatchTechnicalHelpInput(123,{}));CHECK(DispatchTechnicalHelpInput(0x02000049U,[&] { ++calls; }) && calls==3);CHECK(GameMenuScriptCode("TechHelp")==0x02000049U);
    bool cd{};try { TechnicalHelpAvailable(OriginalEdition::Cd1072,h); }catch (const std::invalid_argument&) { cd=true; }CHECK(cd && calls==3);
    ExternalFileHooks f;f.device=[](auto) { return true; };std::uint32_t source{};CHECK(ResolveExternalFilePath("C:\\not-required",&source,f)==std::make_optional(std::string("C:\\not-required")) && source==5);
}
