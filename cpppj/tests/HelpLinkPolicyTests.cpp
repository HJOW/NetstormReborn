// 원문91사례의 실제 토큰/분배/프로토콜 조회·명령/중단 순서를 비교한다.
#include "TestSupport.h"
#include "client/HelpLinkPolicy.h"
#include <algorithm>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <vector>
namespace { // 실제 외부 경계의 응답만 공급하고 제품 본문이 분기를 결정한다.
using namespace netstorm::client;using netstorm::o::OriginalEdition; // 같은 제품 판본과 정책이다.
struct Stop {}; // 지정한 외부 진입에서 원문 prefix를 보존한다.
// 마지막 빈 열도 보존하는 원시 TSV/응답 분리다.
std::vector<std::string> Split(std::string_view text,char separator) {
    std::vector<std::string> result;std::size_t begin{};
    for (;;) { const auto end=text.find(separator,begin);result.emplace_back(text.substr(begin,end==text.npos ? text.npos : end-begin));if (end==text.npos) return result;begin=end+1; } // 원래 구분자 사이의 바이트다.
}
// 입력의 '-'만 빈 문자열로 해석한다. 응답의 null 여부는 별도로 보관한다.
std::string Bytes(std::string_view value) { std::string result;if (value=="-") return result;for (std::size_t i=0;i<value.size();i+=2) result+=static_cast<char>(std::stoul(std::string(value.substr(i,2)),nullptr,16));return result; } // 원문 BYTE를 복원한다.
// 같은 원문 문자열의 첫NUL까지만 관측한다.
std::string Hex(std::string_view value) { value=value.substr(0,value.find('\0'));constexpr std::string_view digits="0123456789abcdef";std::string result;for (const unsigned char byte:value) { result+=digits[byte>>4];result+=digits[byte&15]; }return result; } // 실제 CString 경계다.
struct Probe { // Native의 응답과 현재 Config 출력 버퍼만 제공한다.
    std::string automatic,trace;bool shell{};std::uint32_t stop{},events{};std::size_t readIndex{}; // 독립 입력과 실제 호출 순서다.
    std::vector<std::pair<bool,std::optional<std::string>>> replies; // bool 반환과 출력 변경은 서로 독립인 명시 응답이다.
    void Event(std::string value) { if (!trace.empty()) trace+=';';trace+=value;if (++events==stop) throw Stop{}; } // 효과 전에 진입을 기록한다.
    ExternalHelpHooks External() { // 필요 없는 OS 장치를 성공 기본값으로 바꾸지 않는다.
        ExternalHelpHooks h;h.pathSpec=[this](auto key) { Event("P,"+Hex(key));return automatic; };
        h.shellOpen=[this](auto value) { Event("O,"+Hex(value));return shell; };
        h.readString=[this](auto key,auto& value) { Event("R,"+Hex(key)+','+Hex(value));if (replies.empty()) throw std::logic_error("명시 Config 응답 없음");const auto& reply=replies[std::min(readIndex++,replies.size()-1)];if (reply.second) value=*reply.second;return reply.first; };
        h.launch=[this](auto command) { Event("L,"+Hex(command)); };return h;
    }
    HelpLinkHooks Hooks() { // 실제 공통 분배와 외부 정책을 한 번의 호출로 조립한다.
        HelpLinkHooks h;h.script=[this](auto value) { Event("C,"+Hex(value)); };h.help=[this](auto value,auto mode,auto learn) { Event("H,"+Hex(value)+','+std::to_string(mode)+','+std::to_string(learn)); };
        h.external=[this](auto original,auto protocol,auto body) { OpenExternalHelpLink(OriginalEdition::Patch1078,original,protocol,body,External()); };return h;
    }
};
}
// 원래 token helper까지 실제 CPU로 실행한91개의 전체 trace/중단을 대조한다.
TEST_CASE(HelpLinkPolicy_Native91RoutesProtocolCommandsAndPartialEffects) {
    std::ifstream input(NETSTORM_HELP_LINKS_FIXTURE);CHECK(input.good());std::string line;std::size_t count{};
    while (std::getline(input,line)) { // 각 원문 행에는 새 현재 상태를 제공한다.
        if (line.empty() || line[0]=='#') continue;if (line.back()=='\r') line.pop_back();const auto f=Split(line,'\t');CHECK(f.size()==8);if (f.size()!=8) return;
        Probe p;p.automatic=Bytes(f[2]);p.shell=f[3]=="1";p.stop=static_cast<std::uint32_t>(std::stoul(f[5]));
        for (const auto& reply:Split(f[4],';')) { const auto separator=reply.find(',');const auto value=reply.substr(separator+1);p.replies.emplace_back(reply[0]=='1',value=="-" ? std::nullopt : std::make_optional(Bytes(value))); } // 반환/출력을 고정 입력에서 읽는다.
        bool completed=true;try { DispatchHelpLink(OriginalEdition::Patch1078,Bytes(f[1]),p.Hooks()); }catch (const Stop&) { completed=false; }
        CHECK(completed==(f[6]=="1"));CHECK(p.trace==f[7]);++count;
    }CHECK(count==91);
}
// 미선택 Config/실행은 요구하지 않고, 필요한 누락은 앞선 조회를 보존한 채 거부한다.
TEST_CASE(HelpLinkPolicy_SelectedMissingBoundariesAndCd) {
    Probe p;p.automatic="http:";p.shell=true;auto h=p.External();h.readString={};h.launch={};OpenExternalHelpLink(OriginalEdition::Patch1078,"http://x","http:","//x",h);CHECK(p.trace=="P,6175746f4f70656e;O,687474703a2f2f78");
    p.trace.clear();p.automatic.clear();p.replies={{false,std::nullopt}};h=p.External();h.launch={};bool missing{};try { OpenExternalHelpLink(OriginalEdition::Patch1078,"http://x","http:","//x",h); }catch (const std::logic_error&) { missing=true; }
    CHECK(missing && p.trace=="P,6175746f4f70656e;R,687474703a,;R,687474703a32,");
    p.trace.clear();bool cd{};try { DispatchHelpLink(OriginalEdition::Cd1072,"Topic",p.Hooks()); }catch (const std::invalid_argument&) { cd=true; }CHECK(cd && p.trace.empty());
    bool physical{};try { static_cast<void>(ParseHelpLinkAddress(std::string(256,'x'))); }catch (const std::length_error&) { physical=true; }CHECK(physical);
}
