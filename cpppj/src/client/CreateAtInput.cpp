//004d84f8..851b의 현재 모드/명령 인자 계약을 유지한다.
#include "client/CreateAtInput.h"
#include "o/OriginalText.h"
#include <bit>
#include <stdexcept>

namespace netstorm::client { //Console/정지/로컬 소유자 조건을 새로 붙이지 않는다.
std::uint32_t CreateAtScriptCode(std::string_view name) { return o::AsciiLower(name.substr(0,name.find('\0'))) == "createat" ? kCreateAtCommand : 0xffffffffU; } //첫NUL 뒤를 원문 이름으로 해석하지 않는다.
bool DispatchCreateAtInput(InputEvent event,bool editor,bool mission,const std::function<void(std::uint32_t)>& create) {
    if ((event.code&0x03ffffffU)!=kCreateAtCommand) return false;if ((event.code&InputCode::kRelease) || (!editor && !mission)) return true;
    if (!create) throw std::logic_error("CreateAt의 실제 현재 Runtime 생성 부모 미연결");create(std::bit_cast<std::uint32_t>(event.y));return true;
}
}
