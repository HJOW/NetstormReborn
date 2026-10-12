// 현재 필드 재조회·정수 절삭·모서리 호출 순서와 원본 변경 영역 여백을 유지한다.
#include "client/GumpDecoration.h"
#include <bit>
#include <cmath>
#include <stdexcept>
#include <unordered_set>

namespace netstorm::client {
namespace {
// 두 원본 가상 표는 식별값이며 실행 포인터로 해석하지 않는다. 배경 인자는 밝지도 어둡지도 않은 질감이다.
constexpr std::uint32_t kVtable=0x0050ae7c,kCdVtable=0x00504928,kBackground=0x03e00000;
// x87/CRT의 64비트 절삭 결과에서 하위 signed DWORD를 사용한다. 무효 정수 결과 하위값은 0이다.
std::int32_t Ftol(double value) {
    if (!std::isfinite(value) || value<-9223372036854775808.0 || value>=9223372036854775808.0) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::uint64_t>(static_cast<std::int64_t>(value))));
}
// 원본 좌표/영역 합의 하위 DWORD 감김이다.
std::int32_t Add(std::int32_t first,std::int32_t second) { return std::bit_cast<std::int32_t>(std::uint32_t(first)+std::uint32_t(second)); }
}
// 안정된 노드 조회만 필수이며 자산/영역/픽셀 경계는 원본 분기에 도달할 때 요구한다.
GumpDecoration::GumpDecoration(GumpCreateState& create,GumpDecorationState& state,const GumpBackgroundCreate& factory,
    const GumpBackgroundDraw& background,const GumpCollection& collection,const GumpLifetime& lifetime,GumpDecorationHooks hooks)
    :create_(create),state_(state),factory_(factory),background_(background),collection_(collection),lifetime_(lifetime),hooks_(std::move(hooks)) {
    if (!hooks_.node || !hooks_.base) throw std::invalid_argument("장식 배경 공유 노드 필요");
}
// 장식 캐시 필드는 실제 배경/기본 객체와 같은 저장소에 있어야 한다.
GumpDecorationNode& GumpDecoration::Node(std::uint32_t token) const {
    auto* node=token ? hooks_.node(token) : nullptr;
    if (!node || static_cast<GumpCreateNode*>(node)!=hooks_.base(token)) throw std::runtime_error("장식 배경 공유 노드 없음");return *node;
}
// 부모 visible 검사는 같은 트리 저장소의 현재 값으로 수행한다.
GumpCreateNode& GumpDecoration::Base(std::uint32_t token) const {
    auto* node=token ? hooks_.base(token) : nullptr;if (!node) throw std::runtime_error("장식 배경 부모 노드 없음");return *node;
}
// 배경 생성 뒤 원본 signed 정렬과 별도 모서리 타입/자산 캐시를 저장한다.
std::uint32_t GumpDecoration::Construct(std::uint32_t token,float width,float height) const {
    Node(token);factory_.Construct(token,width,height,kBackground,1,state_.backgroundType,state_.backgroundFrame);
    Node(token).vtable=create_.collection.edition==o::OriginalEdition::Patch1078 ? kVtable : kCdVtable;
    Node(token).alignFlags=static_cast<std::uint32_t>(static_cast<std::int32_t>(state_.alignment));Node(token).cornerType=state_.cornerType;
    if (!hooks_.type) throw std::invalid_argument("장식 배경 모서리 타입 경계 필요");
    const auto type=hooks_.type(state_.cornerType);Node(token).cornerShape=type.shape;Node(token).cornerFrame=type.firstFrame;return token;
}
// 앞 모서리 콜백이 바꾼 SHP/첫 프레임을 다음 모서리 출력에 반영한다.
void GumpDecoration::Corner(std::uint32_t token,std::uint32_t offset,ScreenPoint position) const {
    if (!hooks_.shape) throw std::invalid_argument("장식 배경 실제 모서리 출력 경계 필요");
    const auto& node=Node(token);hooks_.shape(node.cornerShape,node.cornerFrame+offset,position);
}
// 위치는 배경 출력 이후 한 번 저장하며 오른쪽/아래 위치에 현재 배율 적용 크기를 각각 절삭해 더한다.
void GumpDecoration::Draw(std::uint32_t token) const {
    background_.Draw(token);const auto position=collection_.Position(token);
    Corner(token,0,position);
    Corner(token,1,{Add(position.x,Ftol(Node(token).width)),position.y});
    const auto bottom=Add(position.y,Ftol(Node(token).height));
    Corner(token,2,{Add(position.x,Ftol(Node(token).width)),bottom});
    Corner(token,3,{position.x,Add(position.y,Ftol(Node(token).height))});
}
// 요청 원점은 1픽셀 왼쪽/위이며 크기는 width/height+4다. 부모/자신의 dead/void는 여기서 검사하지 않는다.
void GumpDecoration::Redraw(std::uint32_t token,bool erase) const {
    (void)erase;if (!(Node(token).flags&1U)) return;
    std::unordered_set<std::uint32_t> seen;
    // 모든 부모 visible만 확인하며 정상 링크에 영향 없는 호스트 순환 검사를 수행한다.
    for (auto parent=Node(token).parent;parent;parent=Base(parent).parent) {
        if (!seen.insert(parent).second || seen.size()>kGumpRegistryCapacity) throw std::runtime_error("장식 배경 부모 순환");
        if (!(Base(parent).flags&1U)) return;
    }
    const auto position=collection_.Position(token);
    const auto height=Ftol(static_cast<double>(Node(token).height)+4.0),width=Ftol(static_cast<double>(Node(token).width)+4.0);
    if (width<=0 || height<=0) return;
    const auto left=Add(position.x,-1),top=Add(position.y,-1);
    if (!hooks_.invalidate) throw std::invalid_argument("장식 배경 변경 영역 요청 경계 필요");
    hooks_.invalidate({left,top,Add(left,width),Add(top,height)});
}
// 장식은 SHP를 소유하지 않는다. 실제 기본 소멸자가 표/텍스트를 정리하고 flags&1일 때 호스트 토큰을 반납한다.
std::uint32_t GumpDecoration::DeleteScalar(std::uint32_t token,std::uint32_t flags) const { Node(token);return lifetime_.DeleteScalar(token,flags); }
// 현재 타입 번호와 같은 SHP 식별자 토큰을 공급한다. +150은 I 글자 구간의 첫 논리 프레임이다.
void BindGumpDecorationAssets(GumpDecorationHooks& hooks,const GameAssets& assets) {
    hooks.type=[&assets](std::uint32_t type) {
        if (type<o::kFirstAssetTypeNumber || type-o::kFirstAssetTypeNumber>=assets.Types().size())
            throw std::out_of_range("장식 배경 실제 타입 범위 초과");
        const auto& definition=assets.Types()[type-o::kFirstAssetTypeNumber].definition;
        return GumpDecorationType{type,static_cast<std::uint32_t>(definition.FrameTable().Run('I').first)};
    };
}
// 캐시는 실제 자산에만 대응한다. 배경/글자와 같은 현재 클립을 GumpPixelTarget에서 읽는다.
GumpDecorationPixelTarget::GumpDecorationPixelTarget(const GumpPixelTarget& target,const GameAssets& assets):target_(target),assets_(assets) {}
// 기존 생성/변경 영역 경계를 보존하고 현재 캐시 SHP 원색 출력만 이 픽셀 대상에 연결한다.
GumpDecorationHooks GumpDecorationPixelTarget::Hooks(GumpDecorationHooks hooks) {
    hooks.shape=[this](std::uint32_t shape,std::uint32_t frame,ScreenPoint position) { Shape(shape,frame,position); };return hooks;
}
// 다른 타입 전역을 재조회하지 않고 생성 때 캐시한 SHP 토큰의 실제 물리 프레임을 출력한다.
void GumpDecorationPixelTarget::Shape(std::uint32_t shape,std::uint32_t frame,ScreenPoint position) {
    if (shape<o::kFirstAssetTypeNumber || shape-o::kFirstAssetTypeNumber>=assets_.Types().size())
        throw std::out_of_range("장식 배경 캐시 SHP 범위 초과");
    const auto block=assets_.Types()[shape-o::kFirstAssetTypeNumber].block;
    const auto& frames=assets_.Shapes().Blocks()[block].frames;
    if (frame>=frames.size()) throw std::out_of_range("장식 배경 모서리 물리 프레임 범위 초과");
    const auto& source=frames[frame];if (source.IsSpecial()) return;
    const auto key=std::make_pair(shape,frame);auto image=images_.find(key);
    if (image==images_.end()) image=images_.emplace(key,assets_.Shapes().Decode(block,frame)).first;
    target_.Image(image->second,{Add(position.x,source.rect.left),Add(position.y,source.rect.top)});
}
// 호출 때 실제 장식 처리기를 조회해 다른 생성/배경/텍스트 분배와 순환 생성 의존성을 피한다.
void BindGumpDecoration(GumpDrawHooks& draw,GumpFrameHooks& frame,GumpLifetimeHooks& lifetime,
    std::function<const GumpDecoration*(std::uint32_t)> resolve) {
    if (!resolve) throw std::invalid_argument("장식 배경 분배 조회 필요");
    const auto previousDraw=draw.draw;const auto previousRedraw=frame.redraw;const auto previousDelete=lifetime.deletingDestructor;
    draw.draw=[resolve,previousDraw](std::uint32_t token) {
        if (const auto* decoration=resolve(token)) decoration->Draw(token);
        else { if (!previousDraw) throw std::invalid_argument("장식 외 Draw 경계 필요");previousDraw(token); }
    };
    frame.redraw=[resolve,previousRedraw](std::uint32_t token,bool erase) {
        if (const auto* decoration=resolve(token)) decoration->Redraw(token,erase);
        else { if (!previousRedraw) throw std::invalid_argument("장식 외 Redraw 경계 필요");previousRedraw(token,erase); }
    };
    lifetime.deletingDestructor=[resolve,previousDelete](std::uint32_t token,std::uint32_t flags) {
        if (const auto* decoration=resolve(token)) decoration->DeleteScalar(token,flags);
        else { if (!previousDelete) throw std::invalid_argument("장식 외 소멸 경계 필요");previousDelete(token,flags); }
    };
}
}
