// 원본 x87의 double 시간 차이 저장과 float 위치/속도 저장 순서를 유지한다.
#include "client/FloatingSpGump.h"
#include <stdexcept>

namespace netstorm::client {
namespace {
constexpr std::uint32_t kVtable=0x0050688c; // 원본10.78의 비용 표시 가상 표 식별값이다.
}
// 이 복원은10.78 전용이며 CD판의 가상 표/몸체를 추정하지 않는다.
FloatingSpGump::FloatingSpGump(GumpCreateState& create,FloatingSpGumpHooks hooks):create_(create),hooks_(std::move(hooks)) {
    if (create.collection.edition!=o::OriginalEdition::Patch1078 || !hooks_.node || !hooks_.now || !hooks_.allocate ||
        !hooks_.constructText || !hooks_.clearText || !hooks_.fitText || !hooks_.styledText || !hooks_.idle ||
        !hooks_.redraw || !hooks_.kill || !hooks_.attach || !hooks_.deleteText) throw std::invalid_argument("비용 표시의 실제 Gump/시계 연결 필요");
}
// 가상 콜백이 필드를 바꿀 수 있으므로 이후 사용은 현재 저장소를 다시 조회한다.
FloatingSpGumpNode& FloatingSpGump::Node(std::uint32_t token) const {
    auto* node=token ? hooks_.node(token) : nullptr;if (!node) throw std::runtime_error("비용 표시 실제 노드 없음");return *node;
}
// 원본 서식은 글꼴3/색·그림 명령을 포함한다. 비용 차감은 이 표시기의 역할이 아니다.
std::uint32_t FloatingSpGump::Construct(std::uint32_t token,std::int32_t cost,float acceleration) const {
    Node(token);hooks_.constructText(token);Node(token).vtable=kVtable;Node(token).stamp=0;
    Node(token).acceleration=acceleration;Node(token).velocity=0;hooks_.clearText(token);
    hooks_.styledText(token,"~3~E~y"+std::to_string(cost)+"~[IfortGump.3]");hooks_.fitText(token);hooks_.idle(token,true);
    Node(token).stamp=hooks_.now();Node(token).depth=0xffffffd8U;return token;
}
// 객체 참조가 있을 때만 해당 실제 경계가 필요하다. 일반 커서 배치는 참조0이다.
std::uint32_t FloatingSpGump::Show(std::int32_t cost,std::int32_t x,std::int32_t y,std::uint32_t reference) const {
    const auto token=hooks_.allocate(0xb8);if (!token) throw std::bad_alloc();Construct(token,cost,-2.0F);
    float px=static_cast<float>(x),py=static_cast<float>(y);
    if (reference) { if (!hooks_.reference) throw std::logic_error("비용 표시 실제 객체 참조 미연결");hooks_.reference(token,reference);px=py=0; }
    hooks_.attach(token,px,py,create_.collection.root,0);return token;
}
// jnp(C0/C2 parity)는 unordered를 범위 이탈로 보지 않는다. >= 포함 조건으로 바꾸면 NaN 동작이 달라진다.
void FloatingSpGump::Idle(std::uint32_t token) const {
    const auto now=hooks_.now();const double elapsed=now-Node(token).stamp;Node(token).stamp=now;hooks_.redraw(token,false);
    const float y=static_cast<float>(double(Node(token).velocity)+Node(token).y);Node(token).y=y;
    if (y<0.0 || Node(token).x<0.0 || double(create_.screenWidth)<Node(token).x || double(create_.screenHeight)<y) {
        hooks_.idle(token,false);hooks_.kill(token);return;
    }
    hooks_.redraw(token,false);const double change=double(Node(token).acceleration)*elapsed;
    Node(token).velocity=static_cast<float>(change+Node(token).velocity);
}
// 기존 소멸자는 같은 head를 두 번 Clear한 뒤 기본 수명/반납을 수행한다.
std::uint32_t FloatingSpGump::DeleteScalar(std::uint32_t token,std::uint32_t flags) const { return hooks_.deleteText(token,flags); }
}
