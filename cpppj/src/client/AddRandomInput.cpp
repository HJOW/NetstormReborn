//4d7c33..4d7c6c의 타입 숫자/문자열 분기를 현재 입력에 보존한다.
#include "client/AddRandomInput.h"
#include <bit>
#include <stdexcept>

namespace netstorm::client { // 별도 메뉴 수명/소유자 조건을 추가하지 않는다.
// 원문처럼 lookup의 -1/0도 배치 부모로 전달하고 잘못된 타입 진단은 그 하위에 맡긴다.
bool DispatchAddRandomInput(const InputEvent& event,bool editor,bool mission,
    const std::function<std::int32_t(std::uint32_t)>& lookup,
    const std::function<void(std::uint32_t,std::uint32_t)>& add) {
    if ((event.code&0x03ffffffU)!=kAddRandomCommand) return false;
    if ((event.code&InputCode::kRelease) || (!editor && !mission)) return true;
    auto type=std::bit_cast<std::uint32_t>(event.y);
    if (event.y>255) { if (!lookup) throw std::logic_error("AddRandom의 실제 타입 이름 조회 미연결");type=std::bit_cast<std::uint32_t>(lookup(type)); }
    if (!add) throw std::logic_error("AddRandom의 실제 현재 배치 부모 미연결");add(type,1);return true;
}
}
