// 입력의 선행 로컬 변환과 release 프레임의 마지막 이동을 원문 순서로 유지한다.
#include "client/HudDividerInput.h"
#include <bit>
#include <stdexcept>

namespace netstorm::client {
namespace {
constexpr std::uint32_t kCodeMask=0x03ffffff,kLeft=0x10000,kRelease=0x80000000,kAnchor=0x10,kNotify=0x100; // 원문 사건/기준점/방송 비트다.
constexpr double kPointerOffset=2.0; // 00501444는 드래그 마우스 좌표에서 빼는 float2다.
// 원문은 int 제한과 비교한 뒤 제한이 선택될 때만 저장된 float 값을 사용한다.
double Limited(double value,std::int32_t minimum,std::int32_t maximum) {
    if (value<static_cast<double>(minimum)) value=static_cast<float>(minimum);
    if (static_cast<double>(maximum)<value) value=static_cast<float>(maximum);
    return value;
}
}
// 공유 조회만 필수다. 폴링/커서/Help는 실제 원문 분기에 도달했을 때 요구한다.
HudDividerInput::HudDividerInput(const GumpCollection& collection,const GumpInput& input,const GumpTree& tree,HudDividerInputHooks hooks)
    :collection_(collection),input_(input),tree_(tree),hooks_(std::move(hooks)) {
    if (!hooks_.divider || !hooks_.base) throw std::invalid_argument("Divider 입력 공유 저장소 필요");
}
// GumpTree/Collection이 읽는 주소와 파생 필드 주소가 동일해야 한다.
HudDividerNode& HudDividerInput::Node(std::uint32_t token) const {
    auto* node=token ? hooks_.divider(token) : nullptr;
    if (!node || static_cast<GumpCreateNode*>(node)!=hooks_.base(token)) throw std::runtime_error("Divider 입력 공유 노드 없음");
    return *node;
}
// 좌클릭 release는 축 비트와 무관하게 처리하며 focus 해제는 공통 Route가 담당한다.
std::uint32_t HudDividerInput::Handle(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) const {
    auto& node=Node(token);
    const auto local=collection_.ScreenToLocal(token,{std::bit_cast<std::int32_t>(first),std::bit_cast<std::int32_t>(second)});
    const auto code=event&kCodeMask;
    if (!(event&kRelease) && code==kLeft && (node.axis&3)) {
        node.flags|=kAnchor;node.positioning[0]=std::bit_cast<std::uint32_t>(local[0]);node.positioning[1]=std::bit_cast<std::uint32_t>(local[1]);
        node.flags|=kNotify;input_.Capture(token);return 1;
    }
    if ((event&kRelease) && code==kLeft) { node.flags&=~kNotify;return 1; }
    if (code==hooks_.cursorEvent) {
        if (node.axis&2) { if (!hooks_.cursor) throw std::invalid_argument("Divider 세로 커서 장치 필요");hooks_.cursor(5);return 1; }
        if (node.axis&1) { if (!hooks_.cursor) throw std::invalid_argument("Divider 가로 커서 장치 필요");hooks_.cursor(4);return 1; }
    }
    if (!hooks_.baseHandle) throw std::invalid_argument("Divider 기본 Help 처리 필요");
    return hooks_.baseHandle(token,event,first,second);
}
// 드래그는 visible/notify/dead 선택을 끝낸 공통 Broadcast가 호출하며 이 본체는 그 조건을 재검사하지 않는다.
void HudDividerInput::Broadcast(std::uint32_t token,std::uint32_t,std::uint32_t first,std::uint32_t second) const {
    auto& node=Node(token);
    if (!hooks_.menuInstances) throw std::invalid_argument("Divider 현재 메뉴 인스턴스 수 필요");
    if (hooks_.menuInstances()) return;
    if (!hooks_.poll) throw std::invalid_argument("Divider 좌클릭 폴링 필요");
    if (hooks_.poll(kLeft).code&kRelease) node.flags&=~kNotify;
    float x{},y{}; // 최종 Move 인자는 원본 지역 float 저장 경계에서 단정도로 변환한다.
    if (node.axis&2) {
        double position=static_cast<double>(std::bit_cast<std::int32_t>(second))-kPointerOffset;
        if (node.axis&8) position=Limited(position,node.minY,node.maxY);
        y=static_cast<float>(position);x=static_cast<float>(static_cast<double>(std::bit_cast<float>(node.positioning[0]))+node.x);
    } else {
        if (!(node.axis&1)) return;
        double position=static_cast<double>(std::bit_cast<std::int32_t>(first))-kPointerOffset;
        if (node.axis&4) position=Limited(position,node.minX,node.maxX);
        x=static_cast<float>(position);y=static_cast<float>(static_cast<double>(std::bit_cast<float>(node.positioning[1]))+node.y);
    }
    tree_.Move(token,x,y,0);
}
}
