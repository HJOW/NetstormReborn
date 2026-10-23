// 원본004c8ea0의Tell이름/인자토큰과현재Config치환→param.Append순서를보존한다.
#pragma once
#include <functional>
#include <string>
#include <string_view>

namespace netstorm::client {
// caller는먼저같은Registry에지역Config("param")를등록하고본문처리가끝날때까지유지한다.
struct TellParameterHooks {
    std::function<std::string(std::string_view)> substitute; //00441740의현재SubstituteLocal(text,1024,0,0,0)이다. quoteescape함수가아니다.
    std::function<void(std::string_view)> append; //004402f0의완성된index="value"를같은현재param Config에Append한다.
};
// 입력은원본NUL종료바이트다. 첫이름구분자는"(",인자구분자는",)"이며첫빈토큰에서종료한다.
// 1024BYTE지역배열을넘는원본의무효복사범위는쓰기전에명시거부한다. 문자열고리대신반환이름의소유바이트를준다.
std::string ParseTellParameters(std::string_view source,const TellParameterHooks& hooks);
}
