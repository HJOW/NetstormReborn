// 원문 일반 입력의 뗌/모드/전체DWORD 인자를 그대로 현재 MakeAi에 전달한다.
#include "client/MakeAiCommandInput.h"
#include "o/OriginalText.h"
#include <bit>
#include <stdexcept>

namespace netstorm::client { // 현재 모드만 빌리고 새 임의 기본 메뉴나 AI를 만들지 않는다.
namespace { // 원문이 도달한 경계만 지연 검증한다.
template<class F> const F& Need(const F& hook) { if (!hook) throw std::logic_error("MakeAi 입력의 실제 부모 미연결");return hook; } //무동작분기에 불필요한 장치를 요구하지 않는다.
}
std::uint32_t MakeAiCommandScriptCode(std::string_view name) { return o::AsciiLower(name.substr(0,name.find('\0')))=="makeai" ? kMakeAiCommand : 0xffffffffU; } //이름표 비교는 로케일/키하위BYTE 비교와 다르다.
bool IsMakeAiCommand(std::uint32_t code) { return code==kMakeAiCommand; } //정확한 일반 명령DWORD를 구별한다.
bool DispatchMakeAiCommandInput(InputEvent event,const MakeAiCommandHooks& h) { //명령 뗌 뒤 현재Editor/missionPost를 순서대로 읽는다.
    if (!IsMakeAiCommand(event.code&0x03ffffffU)) return false;if (event.code&InputCode::kRelease) return true;
    if (Need(h.editor)() || Need(h.missionPost)()) Need(h.makeAi)(std::bit_cast<std::uint32_t>(event.y));return true;
}
}
