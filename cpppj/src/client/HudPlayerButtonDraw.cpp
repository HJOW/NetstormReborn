// signed 좌표 연산의 DWORD 감김과 실제 원문의 0방향 중앙 나눗셈을 보존한다.
#include "client/HudPlayerButtonDraw.h"
#include <bit>
#include <stdexcept>

namespace netstorm::client { //가상 Draw의 실제 부작용은 예외 뒤에도 되돌리지 않는다.
namespace { //현재 장치/32비트 산술만 구별하며 독립 버튼 상태를 만들지 않는다.
template<class F> const F& Need(const F& hook) { if (!hook) throw std::logic_error("PlayerInfo 버튼의 실제 Draw 장치 미연결");return hook; } //선택된 장치만 원문 호출 위치에서 요구한다.
std::int32_t Add(std::int32_t value,std::uint32_t amount) { return std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(value)+amount); } //원문 add/sub의 하위 DWORD를 signed로 돌린다.
std::int32_t Center(std::int32_t lower,std::int32_t upper) { const auto difference=std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(upper)-std::bit_cast<std::uint32_t>(lower));return Add(lower,static_cast<std::uint32_t>(difference/2)); } //음수 폭도 floor가 아닌 0방향 절삭이다.
}
// 현재 pressed/value는 배경을 그리기 전에 한 번만 캡처한다.
void DrawPlayerInfoButton(std::uint32_t token,const HudPlayerButtonDrawHooks& h) {
    auto rectangle=Need(h.bounds)(token);const auto& node=Need(h.node)(token);const bool active=node.value!=0 || node.pressed!=0;const auto frame=Need(h.read)(0x5ca924),backgroundType=h.read(0x5ca920);Need(h.background)(rectangle,0x03e40000U,backgroundType,frame);
    const auto color=node.untouchedC0;rectangle.left=Add(rectangle.left,2);rectangle.top=Add(rectangle.top,2);rectangle.right=Add(rectangle.right,0xfffffffeU);rectangle.bottom=Add(rectangle.bottom,0xfffffffeU);Need(h.fill)(rectangle,color);
    const auto type=Need(h.type)(h.read(0x5411c8));const auto entry=node.entry;const auto first=Need(h.firstL)(type);if (!active) return;
    const auto selected=std::bit_cast<std::uint32_t>(first)+(entry ? 7U:6U);Need(h.shape)(type,selected,{Center(rectangle.left,rectangle.right),Center(rectangle.top,rectangle.bottom)});
}
}
