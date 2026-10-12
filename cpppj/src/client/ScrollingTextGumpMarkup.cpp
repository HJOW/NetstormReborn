// 구간별 가상 표시 요청과 현재 문맥/태그/제한 필드 재조회를 유지하는 다중 줄 스타일 파서다.
#include "client/ScrollingTextGumpMarkup.h"
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <string>

namespace netstorm::client {
// 실제 버퍼/그림 처리기는 같은 상태를 참조한다. 여기서는 새 버퍼나 표시 조각을 소유하지 않는다.
ScrollingTextGumpMarkup::ScrollingTextGumpMarkup(const ScrollingTextGump& text,const StyleText& style,const Markup& pictures,
    const StyleTextMarkup& markup,ScrollingTextGumpMarkupHooks hooks)
    :text_(text),style_(style),pictures_(pictures),markup_(markup),hooks_(std::move(hooks)) {
    if (!hooks_.node || !hooks_.base) throw std::invalid_argument("다중 줄 스타일 공유 노드 경계 필요");
}
// 상위 처리기의 현재 기본 객체와 동일한 저장소를 사용해야 태그/문맥 변경이 입력/표시에 바로 반영된다.
ScrollingTextGumpNode& ScrollingTextGumpMarkup::Node(std::uint32_t token) const {
    auto* node=token ? hooks_.node(token) : nullptr;
    if (!node || static_cast<GumpCreateNode*>(node)!=hooks_.base(token)) throw std::runtime_error("다중 줄 스타일 공유 노드 없음");
    return *node;
}
// 문자열의 첫 NUL까지만 처리한다. 입력 바이트가 콜백에서 바뀌는 원본 포인터 alias는 이 호스트 API에서 지원하지 않는다.
void ScrollingTextGumpMarkup::Parse(std::uint32_t token,std::string_view text) const {
    if (text.size()>static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()))
        throw std::length_error("다중 줄 스타일 원본 signed 구간 길이 한도 초과");
    auto context=Node(token).textContext;
    const std::string source(text.substr(0,text.find('\0')));
    std::size_t position=0;
    // 명령 앞의 문자열을 먼저 더한다. 소리/통지가 바꾼 현재 태그/기본 문맥은 다음 구간에서 다시 사용한다.
    while (true) {
        const auto mark=source.find('~',position);
        const auto remaining=std::span<const char>(source.c_str()+position,source.size()-position+1);
        if (mark==std::string::npos) {
            text_.AppendText(token,remaining,context,-1,Node(token).metadata);return;
        }
        if (mark>position)
            text_.AppendText(token,remaining,context,static_cast<std::int32_t>(mark-position),Node(token).metadata);
        const auto command=source[mark+1];auto next=mark+2;
        if (command=='~') {
            // 리터럴도 길이 1의 버퍼 입력이므로 줄 추가 통지와 Redraw를 생략하지 않는다.
            constexpr std::array<char,2> literal{'~','\0'};
            text_.AppendText(token,literal,context,1,Node(token).metadata);
        } else if (command=='[') {
            const auto close=source.find(']',next);
            if (close!=std::string::npos) {
                // 현재 +d4는 실제 그림 검사 시 읽는다. 그림 태그도 추가 직전에 현재 +c8을 재조회한다.
                markup_.ApplyScrolling(std::string_view(source).substr(next,close-next),context,Node(token).metadata,
                    [this,token] { return Node(token).callback!=0; },
                    [this,token](std::uint32_t type,std::uint32_t frame,std::int32_t width,std::int32_t height) {
                        text_.AppendPicture(token,pictures_,type,frame,Node(token).metadata,width,height);
                    });
                next=close+1;
            }
        } else if (!style_.ApplyShortCommand(command,context,Node(token).textContext,Node(token).callback!=0)) next=mark+1;
        // 원본 끝 ~의 버퍼 밖 접근 대신 마지막 빈 입력을 수행한다. 미닫힌 대괄호는 ~[만 소비한다.
        position=std::min(next,source.size());
    }
}
}
