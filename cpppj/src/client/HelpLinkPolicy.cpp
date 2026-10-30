// 설정이 선택한 기존 외부 명령을 해석하며 OS 호출은 별도 현재 장치에 맡긴다.
#include "client/HelpLinkPolicy.h"
#include "o/RawTextToken.h"
#include "o/OriginalText.h"
#include <algorithm>
#include <stdexcept>
namespace netstorm::client { // 실제 링크의 순서/부분효과를 원문대로 유지한다.
namespace { // 도달한 필수 경계와 원래 C문자열만 처리한다.
template<class Function> const Function& Need(const Function& function) { // 미선택 OS 장치를 미리 요구하지 않는다.
    if (!function) throw std::logic_error("Help 링크의 선택된 실제 장치 미연결");return function;
}
std::string_view Text(std::string_view value) { return value.substr(0,value.find('\0')); } // 첫NUL 뒤는 원문 CString처럼 소비하지 않는다.
void Patch(o::OriginalEdition edition) { if (edition!=o::OriginalEdition::Patch1078) throw std::invalid_argument("Help 링크 정책은 실제10.78 경계가 필요합니다"); } //CD 순서는 별도 근거가 필요하다.
}
// 원본 토큰을78개 입력 바이트에서 제한하므로 콜론의 단순 substr 분할로 대신하지 않는다.
HelpLinkAddress ParseHelpLinkAddress(std::string_view value) {
    value=Text(value);HelpLinkAddress result;
    if (value.find(':')==value.npos) result.body=value;
    else { const auto token=o::ReadRawTextToken(value,0,":");result.protocol=token.text+':';result.body=value.substr(token.next); }
    if (result.protocol.size()>=80 || result.body.size()>=256) throw std::length_error("Help 원본 링크의80/256BYTE 물리 범위 초과");return result;
}
// cmd 접두는 전체 일치가 아닌 첫 네 바이트의 ASCII 대소문자 무시 비교다.
void DispatchHelpLink(o::OriginalEdition edition,std::string_view value,const HelpLinkHooks& hooks) {
    Patch(edition);value=Text(value);const auto address=ParseHelpLinkAddress(value);
    if (o::AsciiLower(std::string_view(address.protocol).substr(0,4))=="cmd:") Need(hooks.script)(address.body);
    else if (address.protocol.empty() || (!address.body.empty() && address.body.front()=='#')) Need(hooks.help)(address.body,1,0);
    else Need(hooks.external)(value,address.protocol,address.body);
}
// 반환이true여도 빈 값은 정상 성공이며 첫 값이'{'이면 다음 키를 같은 출력에 다시 읽는다.
bool ReadHelpProtocolCommand(std::string_view protocol,std::string& output,const ExternalHelpHooks& hooks) {
    protocol=Text(protocol);const auto first=Need(hooks.readString)(protocol,output);output=Text(output);
    if (!first || (!output.empty() && output.front()=='{')) {
        const auto second=Need(hooks.readString)(std::string(protocol)+'2',output);output=Text(output);
        if (!second || (!output.empty() && output.front()=='{')) { output.clear();return false; }
    }
    return true;
}
// 원본512BYTE 명령 버퍼는 Win10/11 UTF-8 경로를 보존하는 동적 문자열로 확장한다. 정상 원문의 연결/대체 규칙은 동일하다.
void OpenExternalHelpLink(o::OriginalEdition edition,std::string_view original,std::string_view protocol,std::string_view body,const ExternalHelpHooks& hooks) {
    Patch(edition);original=Text(original);protocol=Text(protocol);body=Text(body);const auto automatic=Need(hooks.pathSpec)("autoOpen");
    if (Text(automatic).find(protocol)!=std::string_view::npos && Need(hooks.shellOpen)(original)) return;
    std::string command;if (!ReadHelpProtocolCommand(protocol,command,hooks)) { Need(hooks.launch)(body);return; }
    if (const auto marker=command.find('%');marker!=command.npos) command.replace(marker,std::min(std::size_t{2},command.size()-marker),"  "); // 첫'%'와 바로 뒤 한 바이트를 공백 둘로 바꾸며 나머지%는 보존한다.
    command.append(body);Need(hooks.launch)(command);
}
}
