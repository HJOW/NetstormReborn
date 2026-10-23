// 원본Tell파라미터부모는이미복원한80BYTE토큰helper와현재Config치환을공유한다.
#include "client/TellParameters.h"
#include "o/RawTextToken.h"
#include <cstdint>
#include <stdexcept>

namespace netstorm::client {
namespace {
constexpr std::size_t kTellSourceBytes=1024; //004c8ea0의local_400은종료NUL을포함한1024BYTE다.
// printf의%s와원본토큰입력은첫NUL뒤바이트를사용하지않는다.
std::string_view CString(std::string_view text) { return text.substr(0,text.find('\0')); }
}
// 각Append뒤후속인자치환은앞서추가한같은param.N을즉시볼수있다.
std::string ParseTellParameters(std::string_view source,const TellParameterHooks& hooks) {
    source=CString(source);if (source.size()>=kTellSourceBytes) throw std::length_error("원본Tell1024BYTE지역입력범위");
    const std::string input(source);const auto name=o::ReadRawTextToken(input,0,"(");auto offset=name.next;std::uint32_t index=1; // 원본지역복사/첫80BYTE토큰/숫자키초기값이다.
    // 마지막빈토큰도실제helper로읽는다. 구분자뒤의다음비빈값을다시찾아계속하지않는다.
    while (true) {
        const auto token=o::ReadRawTextToken(input,offset,",)");offset=token.next;if (token.text.empty()) break; // quoted빈문자열도원본처럼종료한다.
        if (!hooks.substitute || !hooks.append) throw std::logic_error("Tell파라미터의현재Config치환/Append연결누락");
        const auto value=hooks.substitute(token.text);hooks.append(std::to_string(index)+"=\""+std::string(CString(value))+"\"");++index; // 별도escape/Set/값사본목록을사용하지않는다.
    }
    return name.text;
}
}
