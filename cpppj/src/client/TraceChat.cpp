// 원문 설정을 추가하거나 강제로 켜지 않고 같은 Client의 현재 관문만 소비한다.
#include "client/TraceChat.h"
#include <stdexcept>
#include <string>

namespace netstorm::client { // Native 서식 의미와 호스트 UI 토큰을 구분한다.
namespace { // 도달한 장치만 요구하며 현재 포인터0을 가짜 UI로 대체하지 않는다.
// 앞선 서식/문맥 효과는 보존하고 실제 선택 장치의 부재에서 중단한다.
template<class F> const F& Need(const F& hook) { if (!hook) throw std::logic_error("trace Chat의 선택 장치 미연결");return hook; }
//434f40은42f190에 추가 가변인자를 주지 않으므로 정의된 일반 문자/%%만 처리한다.
std::string FormatTraceChat(std::string_view format) {
    format=format.substr(0,format.find('\0'));std::string text; // 원문 첫NUL까지의 입력과 실제1024BYTE 출력이다.
    for (std::size_t index=0;index<format.size();++index) { // 인자 없는 서식을 왼쪽부터 한 번 읽는다.
        auto character=format[index];if (character=='%') { if (++index>=format.size() || format[index]!='%') throw std::logic_error("trace Chat 원문 서식의 미제공 가변인자 읽기");character='%'; }
        if (text.size()==1023) throw std::length_error("trace Chat 원문1024BYTE 서식 범위");text+=character;
    }
    return text;
}
}
// 문맥은 색을 읽은 뒤 고정하며 formatter 뒤 바뀐 현재 Chat 포인터를 사용한다.
void ForwardTraceToChat(std::uint32_t enabled,std::string_view format,const TraceChatHooks& hooks) {
    if (!enabled) return;
    const std::array<std::uint32_t,8> context{0,1,Need(hooks.color)(),0,0,0,1,1}; // 원문434f40의32BYTE 지역 문맥이다.
    const auto text=FormatTraceChat(format);const auto chat=Need(hooks.currentChat)(); // root0에서도 원문은 먼저 서식을 처리한다.
    if (chat) Need(hooks.append)(chat,text,context);
}
}
