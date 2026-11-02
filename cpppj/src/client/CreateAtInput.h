// 일반 CreateAt 이름/전체DWORD 인자를 같은 현재 Runtime 생성 부모에 연결한다.
#pragma once
#include "client/InputEvent.h"
#include <functional>
#include <string_view>

namespace netstorm::client { //미노출 관리 명령의 기본 UI 노출은 추가하지 않는다.
inline constexpr std::uint32_t kCreateAtCommand=0x020000e0U; //00542a74의 실제 일반 명령이다.
std::uint32_t CreateAtScriptCode(std::string_view name); //원문 이름표의 ASCII 대소문자 비교다.
bool DispatchCreateAtInput(InputEvent event,bool editor,bool mission,const std::function<void(std::uint32_t)>& create); //Guide/Gump/release 뒤 Editor OR mission이며 param2의 문자열 토큰을 그대로 전달한다.
}
