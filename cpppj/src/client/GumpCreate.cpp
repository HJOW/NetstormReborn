// 기본 Gump 생성은 아직 트리에 부착하지 않는다. 등록 식별자와 로컬 표시 필드를 먼저 준비한다.
#include "client/GumpCreate.h"
#include <bit>
#include <stdexcept>

namespace netstorm::client {
namespace {
// 판본별 기본 Gump 가상 표 주소와 등록 생략/초기 void 플래그다. raw 근거 식별자로만 쓴다.
constexpr std::uint32_t kPatchVtable=0x50ac70,kCdVtable=0x5048a8,kUnregistered=0x100000,kVoid=8;
}
// 토큰 저장소는 필수이며 루트 생성에만 할당 경계를 요구한다.
GumpCreate::GumpCreate(GumpCreateState& state,GumpCreateHooks hooks):state_(state),hooks_(std::move(hooks)) {
    if (!hooks_.node) throw std::invalid_argument("Gump 생성 저장소 필요");
}
// 미작성 필드 +0x28/+0x74를 보존하면서 원본이 초기화하는 나머지 기본 필드를 준비한다.
std::uint32_t GumpCreate::Construct(std::uint32_t token,std::uint32_t command,float width,float height,std::uint32_t flags) const {
    auto* node=token ? hooks_.node(token) : nullptr;
    if (!node) throw std::runtime_error("Gump 생성 토큰/저장소 없음");
    // 원본 assert에 도달하는 개수는 호스트 안전 계약으로 변경 전에 거부한다. 표가 가득 찬 것과 구분한다.
    if (!(flags&kUnregistered) && std::bit_cast<std::int32_t>(state_.count+1U)>999) throw std::length_error("Gump 원본 개수 한도 초과");
    const auto form=node->form,untouched=node->untouched;*node=GumpCreateNode{};node->form=form;node->untouched=untouched;
    node->vtable=state_.collection.edition==o::OriginalEdition::Patch1078 ? kPatchVtable : kCdVtable;
    node->command=command;node->baseWidth=node->width=width;node->baseHeight=node->height=height;
    node->flags=flags|kVoid;node->scale=1.0F;node->depth=0xffffffd8U;
    node->textContext={0,0,0,0,0,0,1,1};node->serial=++state_.serial;
    if (!(node->flags&kUnregistered)) {
        // 첫 빈 칸 하나만 기록한다. 원본은 빈 칸이 없어도 id 0으로 정상 반환하며 개수를 늘린다.
        for (std::size_t index=0;index<state_.entries.size();++index) if (!state_.entries[index]) {
            state_.entries[index]=token;node->id=static_cast<std::uint32_t>(index);break;
        }
        ++state_.count;
    }
    state_.collection.root=state_.entries[0];return token;
}
// new 실패도 비운 등록 표를 유지한다. 할당 이후의 현재 화면 크기/카운터를 읽는다.
std::uint32_t GumpCreate::InitializeRoot() const {
    if (!hooks_.allocate) throw std::invalid_argument("Gump 루트 할당 경계 필요");
    state_.entries.fill(0);state_.collection.root=0;
    const auto token=hooks_.allocate(kGumpBaseSize);
    if (token) Construct(token,0,static_cast<float>(state_.screenWidth),static_cast<float>(state_.screenHeight),1);
    state_.entries[0]=token;state_.collection.root=token;return token;
}
// 기본 클래스 표시 필드의 동일 주소를 반환해 캐시/링크/플래그를 복제하지 않는다.
GumpCollectionHooks GumpCreate::CollectionHooks(std::function<ScreenPoint(std::uint32_t)> project) const {
    return {[this](auto token)->GumpCollectionNode* { return token ? hooks_.node(token) : nullptr; },std::move(project)};
}
}
