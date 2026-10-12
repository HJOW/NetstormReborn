// 원본의 부분 문맥 초기화·63바이트 복사·배경 부착과 DWORD 크기 감김을 유지한다.
#include "client/StoneButtonGump.h"
#include "client/GumpBackground.h"
#include <bit>
#include <stdexcept>

namespace netstorm::client {
namespace {
// 원본 돌 버튼 가상 표와 배경 요청 크기다. PE 주소는 근거용 식별자다.
constexpr std::uint32_t kPatchStoneVtable=0x503898,kCdStoneVtable=0x501828;
constexpr std::size_t kBackgroundBytes=0xb8,kLabelCopyBytes=63;
// signed DWORD 덧셈의 감김을 유지하고 이후 float/int 변환에 사용할 정수로 반환한다.
std::int32_t AddSize(std::int32_t value,std::uint32_t delta) {
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(value)+delta);
}
}
// 저장소 조회만 필수이며 할당/측정/가상 호출 경계는 실제 도달 시 확인한다.
StoneButtonGump::StoneButtonGump(GumpCreateState& create,StoneButtonGumpState& state,const GumpCreate& factory,
    const ButtonGump& buttons,const GumpBackgroundCreate& backgrounds,const GumpTree& tree,
    const GumpLifetime& lifetime,StoneButtonGumpHooks hooks)
    :create_(create),state_(state),factory_(factory),buttons_(buttons),backgrounds_(backgrounds),tree_(tree),
    lifetime_(lifetime),hooks_(std::move(hooks)) {
    if (!hooks_.node) throw std::invalid_argument("돌 버튼 저장소 필요");
}
// 같은 생성/수집 저장소에 실제 파생 필드가 붙어 있어야 한다.
StoneButtonGumpNode& StoneButtonGump::Node(std::uint32_t token) const {
    auto* node=token ? hooks_.node(token) : nullptr;
    if (!node || factory_.CollectionHooks().node(token)!=static_cast<GumpCollectionNode*>(node))
        throw std::runtime_error("돌 버튼 토큰/공유 저장소 없음");
    return *node;
}
// 배경은 버튼과 다른 파생 타입이므로 기본 생성 노드로 조회한다.
GumpCreateNode& StoneButtonGump::BaseNode(std::uint32_t token) const {
    const auto hooks=factory_.CollectionHooks();auto* node=token ? hooks.node(token) : nullptr;
    if (!node) throw std::runtime_error("돌 버튼 배경/기본 저장소 없음");
    return *static_cast<GumpCreateNode*>(node);
}
// label[63]을 강제로 종료하지 않는다. 원본의 미작성 마지막 바이트를 유지한다.
void StoneButtonGump::CopyLabel(StoneButtonGumpNode& node,const char* label) const {
    if (!label) { node.label[0]=0;return; }
    bool ended=false;
    // 첫 NUL 이후에는 소스를 더 읽지 않고 strncpy처럼 나머지를 0으로 채운다.
    for (std::size_t i=0;i<kLabelCopyBytes;++i) {
        const char value=ended ? 0 : label[i];node.label[i]=value;if (!value) ended=true;
    }
}
// 색/문맥의 미작성 DWORD, 측정값, 라벨 꼬리를 전체 0 초기화로 덮어쓰지 않는다.
std::uint32_t StoneButtonGump::Construct(std::uint32_t token,std::int32_t width,std::int32_t height,
    const char* label,std::uint32_t unused,const std::array<std::uint32_t,8>* context) const {
    auto& node=Node(token);buttons_.ConstructBase(token,width,height,0);(void)unused;
    node.vtable=create_.collection.edition==o::OriginalEdition::Patch1078 ? kPatchStoneVtable : kCdStoneVtable;
    node.kind=ButtonGumpKind::Stone;
    // 세 문맥의 앞 네 DWORD만 초기화한다. extra와 disabled shadowX/shadowY는 원본처럼 보존한다.
    for (auto& text:node.contexts) { text.flags=0;text.font=0;text.color=0;text.style=0; }
    node.command=static_cast<std::uint32_t>(static_cast<std::int32_t>(state_.color));
    node.contexts[2].color=state_.palette.white;
    node.contexts[1].color=state_.palette.black;node.contexts[1].shadowColor=state_.palette.white;
    node.contexts[1].flags|=2;node.contexts[1].shadowX=1;node.contexts[1].shadowY=1;
    node.contexts[0].color=state_.palette.black;node.contexts[0].flags|=2;
    node.contexts[0].shadowX=1;node.contexts[0].shadowY=1;node.contexts[0].shadowColor=state_.palette.white;
    node.textContext=context ? *context : state_.defaultContext;CopyLabel(node,label);
    node.baseWidth=static_cast<float>(width);node.width=static_cast<float>(double(width)*node.scale);
    node.baseHeight=static_cast<float>(height);node.height=static_cast<float>(double(height)*node.scale);
    if (!hooks_.allocate) throw std::invalid_argument("돌 버튼 배경 할당 경계 필요");
    auto background=hooks_.allocate(kBackgroundBytes);
    if (background) {
        const auto frame=state_.backgroundFrame;const auto type=state_.backgroundType;
        background=backgrounds_.Construct(background,static_cast<float>(AddSize(width,0xfffffffeU)),
            static_cast<float>(AddSize(height,0xfffffffeU)),GumpBackgroundFlags::kButton,1,type,frame);
    }
    node.background=background;BaseNode(background).secondaryDepth=1;
    if (!hooks_.attach) throw std::invalid_argument("돌 버튼 배경 가상 부착 경계 필요");
    hooks_.attach(node.background,1,1,token,0);return token;
}
// 원본처럼 현재 기본 문맥과 두 출력 필드를 직접 측정 경계에 넘긴다.
void StoneButtonGump::Measure(std::uint32_t token) const {
    auto& node=Node(token);
    if (!hooks_.measure) throw std::invalid_argument("돌 버튼 글자 측정 경계 필요");
    hooks_.measure(token,node.label,node.textContext,node.textWidth,node.textHeight,0xffffffffU);
}
// CD 라벨 변경 함수는 크기 +12/+6 처리를 하지 않는다.
void StoneButtonGump::SetLabel(std::uint32_t token,const char* label) const {
    if (!label) throw std::invalid_argument("돌 버튼 라벨 변경의 원본 non-null 계약 필요");
    CopyLabel(Node(token),label);Measure(token);
}
// 측정 콜백이 바꾼 현재 scale과 출력값을 읽고 정수 감김 뒤 원본 float 저장 지점을 유지한다.
void StoneButtonGump::FitToText(std::uint32_t token) const {
    Measure(token);auto& node=Node(token);
    const auto width=AddSize(node.textWidth,12);node.baseWidth=static_cast<float>(width);
    node.width=static_cast<float>(double(width)*node.scale);
    const auto height=AddSize(node.textHeight,6);node.baseHeight=static_cast<float>(height);
    node.height=static_cast<float>(double(height)*node.scale);
}
// 가상 Push 뒤 현재 background/부모 크기를 사용한다. 자식 배경을 다시 붙인 뒤 부모를 붙인다.
void StoneButtonGump::Attach(std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode) const {
    auto& node=Node(token);
    if (!hooks_.push) throw std::invalid_argument("돌 버튼 배경 가상 Push 경계 필요");
    hooks_.push(node.background);
    auto& background=BaseNode(node.background);
    const double width=double(node.baseWidth)-2; background.baseWidth=static_cast<float>(width);
    background.width=static_cast<float>(width*background.scale);
    // 패치판은 높이 저장 전에 배경 포인터를 다시 읽는다. CD판은 같은 배경 포인터를 유지한다.
    auto& heightBackground=create_.collection.edition==o::OriginalEdition::Patch1078 ? BaseNode(node.background) : background;
    const double height=double(node.baseHeight)-2; heightBackground.baseHeight=static_cast<float>(height);
    heightBackground.height=static_cast<float>(height*heightBackground.scale);
    if (!hooks_.attach) throw std::invalid_argument("돌 버튼 배경 가상 부착 경계 필요");
    hooks_.attach(node.background,1,1,token,0);tree_.Attach(token,x,y,parent,mode);
}
// 배경은 자식 우선 Destroy로 이미 파괴된다. 별도 배경 해제를 추가하지 않는다.
std::uint32_t StoneButtonGump::DeleteScalar(std::uint32_t token,std::uint32_t flags) const {
    Node(token);return lifetime_.DeleteScalar(token,flags);
}
// 기존 픽셀 계획은 원본 미작성 문맥 값을 안전 문맥으로 대체하지 않고 그대로 받는다.
ButtonDrawPlan StoneButtonGump::DrawPlan(std::uint32_t token) const {
    const auto& node=Node(token);
    return PlanButtonDraw(buttons_.VisualState(token,node.textWidth,node.textHeight),node.contexts,state_.palette.edge);
}
// 처리기를 만든 뒤 resolve 포인터를 연결할 수 있도록 훅만 감싼다.
void BindStoneButtonGump(GumpTreeHooks& tree,GumpLifetimeHooks& lifetime,
    std::function<const StoneButtonGump*(std::uint32_t)> resolve) {
    if (!resolve) throw std::invalid_argument("돌 버튼 수명/부착 분배 조회 필요");
    const auto attach=tree.attach;const auto destroy=lifetime.deletingDestructor;
    // 실제 돌 버튼 Attach가 마지막에 기본 GumpTree::Attach를 직접 호출해 자기 분배 재귀를 피한다.
    tree.attach=[resolve,attach](std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode) {
        if (const auto* stone=resolve(token)) stone->Attach(token,x,y,parent,mode);
        else { if (!attach) throw std::invalid_argument("돌 버튼 외 Gump 부착 분배기 필요");attach(token,x,y,parent,mode); }
    };
    // 자식 파괴/등록 해제는 GumpLifetime이 수행하며 scalar 몸체만 파생 처리기로 분배한다.
    lifetime.deletingDestructor=[resolve,destroy](std::uint32_t token,std::uint32_t flags) {
        if (const auto* stone=resolve(token)) stone->DeleteScalar(token,flags);
        else { if (!destroy) throw std::invalid_argument("돌 버튼 외 Gump 소멸자 분배기 필요");destroy(token,flags); }
    };
}
}
