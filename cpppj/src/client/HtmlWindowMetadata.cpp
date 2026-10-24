// Trim 없이 실제 캡처/우선순위/효과 뒤 현재 버퍼/필드 재조회와 원문 부분 복사를 유지한다.
#include "client/HtmlWindowMetadata.h"
#include "client/DialogExitCommand.h"
#include "o/OriginalText.h"
#include <algorithm>
#include <bit>
#include <cstring>
#include <regex>
#include <stdexcept>
namespace netstorm::client { // 실제 HTML 메타 구현이다.
namespace { // 고정 문법/물리 입력/선택 장치 확인을 모은다.
template<class F> const F& Need(const F& hook) { if (!hook) throw std::logic_error("HTML 메타 실제 장치 미연결");return hook; } // 해당 원문 분기에서만 요구한다.
std::size_t Length(std::span<const char> buffer) { const auto end=std::find(buffer.begin(),buffer.end(),'\0');if (end==buffer.end()) throw std::length_error("HTML 메타 실제 버퍼 NUL 범위 초과");return static_cast<std::size_t>(end-buffer.begin()); } // 임의 바깥 바이트를 공급하지 않는다.
std::string Capture(const std::csub_match& group,std::size_t capacity) { const auto size=static_cast<std::size_t>(group.length());if (size>=capacity) throw std::length_error("HTML 메타 원문 지역 캡처 NUL 범위 초과");return std::string(group.first,group.second); } // Native는capacityBYTE 뒤 NUL을 쓰므로 한 칸 넘는 입력은 명시 거부한다.
}
HtmlWindowMetadata::HtmlWindowMetadata(HtmlWindowMetadataHooks hooks):hooks_(std::move(hooks)) { if (!hooks_.node) throw std::invalid_argument("HTML 메타 같은 실제 부모 필요"); } // 장치는 선택 시 확인한다.
HtmlWindowGumpNode& HtmlWindowMetadata::Node(std::uint32_t token) const { auto* node=token ? hooks_.node(token) : nullptr;if (!node) throw std::logic_error("HTML 메타 현재 부모 없음");return *node; } // 표시용 복사본을 만들지 않는다.
std::uint32_t HtmlWindowMetadata::Process(std::uint32_t token,std::span<char> buffer) const {
    Node(token);Need(hooks_.clearButtons)(token);
    static const std::array<std::regex,9> patterns{ // 원문9regex와 같은 대소문자/검색 우선순위다.
        std::regex(R"(\$Button[ \t]*=[ \t]*([^,]+),([^,]+),([^$ ]+))"),std::regex(R"(\$ButtonInstant[ \t]*=[ \t]*([^,]+),([^,]+),([^$ ]+))"),std::regex(R"(\$Menu[ \t]*=[ \t]*([^,]+),([^,]+),([^$ ]+))"),std::regex(R"(\$Checked[ \t]*=[ \t]*([^,]+),([^,]+),([^,]+),([^,]+),([^$ ]+))"),std::regex(R"(\$Timeout[ \t]*=[ \t]*([^,]+),([^,]+),([^$ ]+))"),std::regex(R"(\$OnExit[ \t]*=[ \t]*([^,]+),([^,]+),([^$ ]+))"),std::regex(R"(\$Key[ \t]*=[ \t]*([^,]+),([^,]+),([^$ ]+))"),std::regex(R"(\$Input[ \t]*=[ \t]*([^,]+),([^,]+),([^$ ]+))"),std::regex(R"(\$Zsend[ \t]*=[ \t]*([^,]+),([^,]+),([^$ ]+))")};
    std::uint32_t buttons{}; // 원문은 실제20행 한도와 별도로 처리한 Button 수를 센다.
    for (;;) { // 제거/치환 뒤 항상 Button부터 다시 검색한다.
        const auto length=Length(buffer);std::cmatch match;std::size_t kind{};const char* begin=buffer.data();
        for (;kind<patterns.size();++kind) if (std::regex_search(begin,begin+length,match,patterns[kind])) break; // 앞 종류의 첫 일치가 뒤 종류의 더 이른 위치보다 우선한다.
        if (kind==patterns.size()) break;
        const auto label=Capture(match[1],80),command=Capture(match[2],80),argument=Capture(match[3],256);DialogExitCommandState resolved;ResolveDialogExitCommand(resolved,command,argument,{hooks_.commands,hooks_.storeText});std::string replacement;bool insert{};
        if (kind==3) {
            const auto checked=Capture(match[4],256),enabled=Capture(match[5],256);if (!Node(token).menuColumns) { Need(hooks_.beginMenu)(0x40000);++Node(token).menuColumns; }
            const bool allow=o::ConfigParseLong(enabled)!=0 || Need(hooks_.read)(0x5cab30)!=0 || Need(hooks_.read)(0x55a480)!=0;Need(hooks_.checked)(o::ConfigParseLong(checked),label,resolved.command,resolved.parameter,allow);
        } else if (kind==2) {
            if (!Node(token).menuColumns) { Need(hooks_.beginMenu)(0x40000);++Node(token).menuColumns; }
            const auto lower=o::AsciiLower(label);
            if (lower=="*sololist*" || lower=="*fortlist*") Need(hooks_.maps)(label,Node(token).menuColumns);
            else if (lower=="*techlist*") Need(hooks_.tech)(resolved.parameter);
            else if (lower=="*themetechlist*") Need(hooks_.themeTech)(resolved.parameter&0xffffU,std::bit_cast<std::int32_t>(resolved.parameter)>>16);
            else if (!label.empty() && label.front()=='@') Need(hooks_.namedList)(std::string_view(label).substr(1),resolved.command,argument);
            else if (!label.empty() && label.front()=='#') Need(hooks_.fileList)(std::string_view(label).substr(1),resolved.command,argument);
            else if (!label.empty() && label.front()=='%') { replacement=Need(hooks_.inlineList)(std::string_view(label).substr(1),command,argument);replacement.resize(replacement.find('\0')==std::string::npos ? replacement.size() : replacement.find('\0'));insert=true; }
            else Need(hooks_.menuText)(label,resolved.command,resolved.parameter);
        } else if (kind==4) { const auto seconds=o::ConfigParseLong(label);const auto deadline=double(seconds)+Need(hooks_.wallSeconds)();Node(token).timeoutCommand=resolved.command;Node(token).timeoutParameter=resolved.parameter;Node(token).timeoutSeconds=deadline;Need(hooks_.idle)(token,true); }
        else if (kind==5) { Node(token).onExitCommand=resolved.command;Node(token).onExitParameter=resolved.parameter; }
        else if (kind==7) { Need(hooks_.input)(o::ConfigParseLong(command),label,argument);Need(hooks_.queue)(Need(hooks_.read)(0x5429ec),0,0); }
        else if (kind==8) Need(hooks_.send)(label,command,argument);
        else if (kind!=6) { Need(hooks_.addButton)(token,label,resolved.command,resolved.parameter,kind==1 ? 1U : 0U);++buttons; } // Key는 실제 원문처럼 해석/고리 효과 뒤 별도 컨트롤 없이 제거한다.
        const auto start=static_cast<std::size_t>(match.position());const auto currentLength=Length(buffer);if (start>currentLength) throw std::out_of_range("HTML 메타 현재 일치 시작 범위 초과");auto end=start+(buffer[start]=='$' ? 1U : 0U);
        while (end<currentLength && buffer[end]!='\n' && buffer[end]!='$') ++end; // 캡처 끝이 아니라 현재 다음 LF/$/NUL까지 지운다.
        if (end<currentLength && buffer[end]=='\n') ++end;std::memmove(buffer.data()+start,buffer.data()+end,currentLength-end+1);
        if (insert) { std::string merged(buffer.data(),Length(buffer));merged.insert(start,replacement);if (replacement.size()>buffer.size()) throw std::length_error("HTML 메타 원문 치환 복사 범위 초과");std::copy_n(merged.data(),replacement.size(),buffer.data()); } // PE4c8c12..36은 replacement 길이만 복사하며 NUL/전체 merged 길이를 추가하지 않는다.
    }
    return buttons || Node(token).menuColumns ? 1U : 0U;
}
}
