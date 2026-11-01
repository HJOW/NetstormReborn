//004d8528..004d8551의 모드·키 뗌 관문을 보존한다.
#include "client/DestroyAtInput.h"
#include <bit>
#include <stdexcept>

namespace netstorm::client { // 현재 선택이나 메뉴 열기 조건을 실행에 추가하지 않는다.
// 문자열 해석/권한/대상 필터는 실제 하위 부모가 처리한다.
bool DispatchDestroyAtInput(const InputEvent& event,bool editor,bool mission,const std::function<void(std::uint32_t)>& destroyAt) {
    if ((event.code&0x03ffffffU)!=kDestroyAtCommand) return false;
    if ((event.code&InputCode::kRelease) || (!editor && !mission)) return true;
    if (!destroyAt) throw std::logic_error("DestroyAt의 실제 현재 부모 미연결");destroyAt(std::bit_cast<std::uint32_t>(event.y));return true;
}
}
