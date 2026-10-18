// 보호PE에서 생성한 정확한83개 메뉴 슬롯만 분배한다.
#include "client/RawObjectMenuSlots.h"
#include <array>
#include <stdexcept>
namespace netstorm::client {
namespace {
#include "client/ObjectMenuSlots.inc"
}
// 현재raw VT값을 대응표에서 직접 찾는다. 타입 이름으로 메뉴 종류를 추정하지 않는다.
ObjectMenuSlot ObjectMenuSlots(std::uint32_t vtable) {
    // 원본의 실제 생성자VT만 허용한다.
    for (const auto& entry:kObjectMenuSlots) if (entry.vtable==vtable) return entry;
    throw std::out_of_range("현재 객체메뉴 VT 미등록");
}
}
