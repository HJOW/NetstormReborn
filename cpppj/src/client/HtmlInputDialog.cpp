// 공백 제거처럼 보이는 원문의 NUL 검사도 그대로 두고 숫자 입력을 재서식하지 않는다.
#include "client/HtmlInputDialog.h"
#include <algorithm>
#include <array>
#include <stdexcept>

namespace netstorm::client { //현재 모드와 고정80BYTE 후속 섹션을 실제 대화창에서 빌린다.
namespace { //도달한 실제 하위만 요구한다.
template<class F> const F& Required(const F& hook) { if (!hook) throw std::logic_error("HTML 입력 확인의 실제 장치 미연결");return hook; } //부분 효과를 성공으로 감추지 않는다.
}
std::uint32_t ConfirmHtmlInput(const std::uint32_t& mode,std::span<const char> continuation,const HtmlInputConfirmHooks& hooks) { //원문CopyText→강제NUL→첫LF→NUL부터공백검사→선택atol→서식/Tell순서다.
    std::array<char,256> text{};Required(hooks.copyText)(text);text.back()=0;auto end=std::find(text.begin(),text.end(),'\0');const auto newline=std::find(text.begin(),end,'\n');if (newline!=end) { *newline=0;end=newline; }
    while (end!=text.begin()) { //원문은마지막문자가아닌종료NUL에서시작한다.정상CRT에서는공백을지우지않는다.
        if (!Required(hooks.space)(static_cast<std::int8_t>(*end))) break;*end=0;--end;
    }
    const std::string_view input(text.data());if (mode!=1) static_cast<void>(Required(hooks.parseLong)(input));const auto last=std::find(continuation.begin(),continuation.end(),'\0');if (last==continuation.end()) throw std::length_error("HTML 입력 후속 섹션의80BYTE NUL 범위");
    const auto section=Required(hooks.format)(std::string_view(continuation.data(),static_cast<std::size_t>(last-continuation.begin())),input);Required(hooks.tell)(section);return 1;
}
}
