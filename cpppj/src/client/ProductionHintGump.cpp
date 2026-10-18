// x86의 부호 있는 DWORD 감김과 float 저장 경계를 보존한다.
#include "client/ProductionHintGump.h"
#include <bit>
#include <stdexcept>
namespace netstorm::client {
namespace {
constexpr std::uint32_t kVtable=0x50c2b4; // 원본10.78 안내 파생 가상 표다.
constexpr double kLifetime=4.; // PE506588의 벽시계 수명이다.
// signed 오버플로 없이 원본 DWORD 덧셈을 재현한다.
std::int32_t Add(std::int32_t value,std::uint32_t amount) { return std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(value)+amount); }
}
// 실제 공통 경계를 빠짐없이 요구하며 CD판 주소를 추정하지 않는다.
ProductionHintGump::ProductionHintGump(GumpCreateState& create,ProductionHintState& state,ProductionHintHooks hooks):create_(create),state_(state),hooks_(std::move(hooks)) {
    if (create.collection.edition!=o::OriginalEdition::Patch1078 || !hooks_.node || !hooks_.now || !hooks_.allocate || !hooks_.constructText || !hooks_.kill || !hooks_.duplicate || !hooks_.string || !hooks_.free || !hooks_.translate || !hooks_.style || !hooks_.fit || !hooks_.idle || !hooks_.bounds || !hooks_.position || !hooks_.draw || !hooks_.attach || !hooks_.deleteText) throw std::invalid_argument("생산 안내 실제 경계 누락");
}
// callback 뒤에도 현재 저장소를 재조회한다.
ProductionHintNode& ProductionHintGump::Node(std::uint32_t token) const { auto* n=hooks_.node(token);if (!n) throw std::logic_error("생산 안내 노드 없음");return *n; }
// 원본 기본 생성/소유권/파싱/측정/idle/시계 순서다.
std::uint32_t ProductionHintGump::Construct(std::uint32_t token,std::string_view text) const {
    hooks_.constructText(token);Node(token).vtable=kVtable;Node(token).until=0;Node(token).command=static_cast<std::int32_t>(state_.kind);
    Node(token).textContext={0,6,state_.white,0,0,state_.black,1,1};Node(token).reference=0;Node(token).owned=hooks_.duplicate(text);
    hooks_.style(token,hooks_.string(Node(token).owned));hooks_.fit(token,6,4);hooks_.idle(token,true);Node(token).until=hooks_.now()+kLifetime;return token;
}
// 기준 영역은 입력 직사각형으로 저장하며 실제 표시 위치는 입력 y+20이다.
std::uint32_t ProductionHintGump::Show(bool empty,std::int32_t x,std::int32_t y,std::uint32_t reference) const {
    const auto token=hooks_.allocate(0xb8);if (!token) throw std::bad_alloc();
    Construct(token,hooks_.translate(empty ? "~0~w~ENone in inventory" : "~0~w~ENot enough Storm Power to build this"));Node(token).flags|=0x4000;
    const auto r=hooks_.bounds(reference);auto& n=Node(token);n.positioning[2]=std::bit_cast<std::uint32_t>(static_cast<float>(r.left));n.positioning[3]=std::bit_cast<std::uint32_t>(static_cast<float>(r.top));n.positioning[4]=std::bit_cast<std::uint32_t>(static_cast<float>(r.right));n.positioning[5]=std::bit_cast<std::uint32_t>(static_cast<float>(r.bottom));n.flags|=0x8100;n.depth=0xffffff9cU;n.reference=reference;
    hooks_.attach(token,static_cast<float>(x),static_cast<float>(Add(y,20)),create_.collection.root,0);state_.lastSerial=Node(token).serial;return token;
}
// unordered(NaN) 비교는 만료시키지 않는다.
void ProductionHintGump::Idle(std::uint32_t token) const { if (Node(token).until<=hooks_.now()) hooks_.kill(token); }
// 빈 head에서도 StyleText 경계를 호출한다.
void ProductionHintGump::Draw(std::uint32_t token) const { hooks_.bounds(token);const auto p=hooks_.position(token);hooks_.draw(Node(token).head,{Add(p.x,3),Add(p.y,2)}); }
// free 뒤 포인터 필드를0으로 덮어쓰지 않는 원본 소멸 순서다.
std::uint32_t ProductionHintGump::DeleteScalar(std::uint32_t token,std::uint32_t flags) const { Node(token).vtable=kVtable;if (Node(token).owned) hooks_.free(Node(token).owned);return hooks_.deleteText(token,flags); }
}
