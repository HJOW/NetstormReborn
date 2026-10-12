// 이미 배율 적용된 크기/원본 여백을 사용하며 styled 글자의 색을 plain 문맥으로 덮어쓰지 않는다.
#include "client/MenuGumpDraw.h"
#include <bit>
#include <cmath>
#include <stdexcept>

namespace netstorm::client {
namespace {
// 항목의 체크/선택/구분선/자식/굵게/비활성/스타일 문자열/동적 자식 비트다.
constexpr std::uint32_t kChecked=4,kSelected=8,kSeparator=0x10,kChild=0x20,kBold=0x40,kDisabled=0x200,kStyled=0x400,kDynamic=0x800;
// 세로/compact 방향과 강조/구분선 배경 flags다. 그림자 명암 번호와 Gump 표시 flags를 혼동하지 않는다.
constexpr std::uint32_t kVertical=0x80000,kCompact=0x1000000,kHighlight=0x2100000,kSeparatorFirst=0x2100000,kSeparatorSecond=0x2080000;
// 원본 선택 안쪽 여백·구분선 길이/높이 보정·체크 그림 위치와 왼쪽 여백 비율이다.
constexpr double kInset=2.0,kLineTrim=4.0,kHalf=0.5,kLineRaise=1.0,kQuarter=0.25;
// 하위 메뉴 표시는 같은 원본 글꼴의 두 ASCII 바이트다. NUL은 길이 -1 출력에 필요하다.
constexpr std::array<char,3> kChildMarker{' ','>','\0'};
// CRT 64비트 절삭의 signed 하위 DWORD이며 무효 정수 결과는 하위 DWORD 0이다.
std::int32_t Ftol(double value) {
    if (!std::isfinite(value) || value<-9223372036854775808.0 || value>=9223372036854775808.0) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::uint64_t>(static_cast<std::int64_t>(value))));
}
// 원본 화면 위치/그림 위치 덧셈의 DWORD 감김을 보존한다.
std::int32_t Add(std::int32_t first,std::int32_t second) {
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(first)+static_cast<std::uint32_t>(second));
}
}
// 기존 실제 배경/글리프/조각/측정 처리기를 공유하고 메뉴 노드 조회만 필수로 확인한다.
MenuGumpDraw::MenuGumpDraw(GumpCreateState& create,GumpDrawState& clip,MenuGumpDrawState& state,
    const GumpCollection& collection,const GumpBackgroundDraw& background,const GumpTextDraw& text,
    const StyleText& style,const MenuGumpLayout& layout,MenuGumpDrawHooks hooks)
    :create_(create),clip_(clip),state_(state),collection_(collection),background_(background),text_(text),style_(style),layout_(layout),hooks_(std::move(hooks)) {
    if (!hooks_.node || !hooks_.base) throw std::invalid_argument("메뉴 출력 공유 저장소 필요");
}
// 기본 배경/입력/출력이 같은 객체를 가리켜야 한다.
MenuGumpNode& MenuGumpDraw::Node(std::uint32_t token) const {
    auto* node=token ? hooks_.node(token) : nullptr;
    if (!node || static_cast<GumpCreateNode*>(node)!=hooks_.base(token)) throw std::runtime_error("메뉴 출력 공유 노드 없음");return *node;
}
// 출력 콜백이 바꾼 항목 수도 이후 반복에 반영하되 배열 밖 접근은 거부한다.
MenuGumpItem& MenuGumpDraw::Item(std::uint32_t token,std::int32_t index) const {
    if (index<0 || static_cast<std::size_t>(index)>=kMenuItemCapacity) throw std::out_of_range("메뉴 출력 항목 범위 오류");
    return Node(token).items[static_cast<std::size_t>(index)];
}
// X와 top Y는 원래 정수의 확장값이다. 패치만 bottom 계산의 Y를 float32로 저장해 재사용한다.
ScreenRect MenuGumpDraw::Rect(std::uint32_t token,const std::array<float,4>& local) const {
    const auto point=collection_.Position(token);const double x=point.x;
    const double y=create_.collection.edition==o::OriginalEdition::Patch1078 ? static_cast<double>(static_cast<float>(point.y)) : static_cast<double>(point.y);
    return {Ftol(x+local[0]),Ftol(static_cast<double>(point.y)+local[1]),Ftol(x+local[2]),Ftol(y+local[3])};
}
// 원본이 넘기는 로컬 사각형 네 값은 최종 인자 쓰기 때 float32로 저장된다.
void MenuGumpDraw::Highlight(std::uint32_t token,std::int32_t index) const {
    auto& node=Node(token);std::array<float,4> local{};
    if (node.menuFlags&kCompact) local={2.0F,2.0F,static_cast<float>(static_cast<double>(node.width)-kInset),static_cast<float>(static_cast<double>(node.height)-kInset)};
    else if (node.menuFlags&kVertical) {
        const auto offset=static_cast<std::size_t>(index);
        local={2.0F,static_cast<float>(static_cast<double>(node.offsets[offset])+kInset),
            static_cast<float>(static_cast<double>(node.width)-kInset),static_cast<float>(static_cast<double>(node.heights[offset])+node.offsets[offset]-kInset)};
    } else local={static_cast<float>(static_cast<double>(index)*node.itemWidth+kInset),2.0F,
        static_cast<float>(static_cast<double>(Add(index,1))*node.itemWidth-kInset),static_cast<float>(static_cast<double>(node.height)-kInset)};
    const auto rect=Rect(token,local);GumpClipScope scope(clip_,rect);
    background_.Background(rect,kHighlight,Node(token).type,Node(token).frame);
}
// 두 번째 pass는 이전 화면 rect의 top/bottom을 1씩 증가시킨다. 매 pass 클립을 개별 복구한다.
void MenuGumpDraw::Separator(std::uint32_t token,std::int32_t index) const {
    auto& node=Node(token);const auto item=static_cast<std::size_t>(index);
    const auto y=Ftol((static_cast<double>(node.offsets[item])+node.margins[2])-(static_cast<double>(node.targetHeight)-node.height)+static_cast<double>(node.heights[item])*kHalf-kLineRaise);
    const auto right=Ftol(static_cast<double>(node.width)+kInset-kLineTrim);
    auto rect=Rect(token,{2.0F,static_cast<float>(y),static_cast<float>(right),static_cast<float>(Add(y,1))});
    // 명암 flags 순서는 두 PE에서 같으며 전역 타입/프레임은 호출 직전에 읽는다.
    for (std::int32_t pass=0;pass<2;++pass) {
        rect.top=Add(rect.top,pass);rect.bottom=Add(rect.bottom,pass);GumpClipScope scope(clip_,rect);
        background_.Background(rect,pass ? kSeparatorSecond : kSeparatorFirst,state_.backgroundType,state_.backgroundFrame);
    }
}
// styled 출력의 override는 null이다. 비활성 색 등은 문자열 생성 시 조각에 저장한 원본 문맥을 따른다.
void MenuGumpDraw::Text(std::uint32_t token,std::int32_t index,ScreenPoint point,const std::array<std::uint32_t,8>& context) const {
    const auto& item=Item(token,index);
    if (item.flags&kStyled) style_.Draw(item.styled,point,nullptr);
    else { const auto bytes=MenuGumpItemBytes(item);text_.Draw(bytes,point,DecodeGumpTextContext(context)); }
}
// 원본은 별도 부모 필터 없이 자신의 Draw 몸체를 실행한다. 표시/삭제 필터는 실제 수집기에 맡긴다.
void MenuGumpDraw::Draw(std::uint32_t token) const {
    const auto bounds=collection_.Bounds(token);GumpClipScope scope(clip_,bounds);
    background_.Draw(token);const auto point=collection_.Position(token);
    // 매 항목에서 현재 문맥/흰색/비활성 색을 복사하고 출력 콜백 뒤 현재 flags를 다시 읽는다.
    for (std::int32_t index=0;index<std::bit_cast<std::int32_t>(Node(token).itemCount);++index) {
        const auto flags=Item(token,index).flags;auto context=Node(token).textContext;context[1]=0;context[2]=state_.foreground;
        if (flags&kBold) context[3]=1;if (flags&kDisabled) { context[2]=state_.disabled;context[0]=0; }
        if ((flags&kSelected) && !(flags&kSeparator)) Highlight(token,index);
        if (!(Node(token).menuFlags&kVertical)) {
            const auto y=Ftol(static_cast<double>(point.y)+Node(token).margins[2]);
            const auto x=Ftol(static_cast<double>(index)*Node(token).itemWidth+Node(token).margins[0]+point.x);
            Text(token,index,{x,y},context);continue;
        }
        if (Item(token,index).flags&kSeparator) { Separator(token,index);continue; }
        const auto offset=static_cast<std::size_t>(index);
        if (Item(token,index).flags&kChecked) {
            const auto x=Ftol(static_cast<double>(Node(token).margins[0])*kQuarter-(static_cast<double>(Node(token).targetWidth)-Node(token).width));
            const auto y=Ftol(static_cast<double>(Node(token).offsets[offset])+Node(token).margins[2]-(static_cast<double>(Node(token).targetHeight)-Node(token).height));
            if (!hooks_.marker) throw std::invalid_argument("메뉴 체크 SHP 출력 경계 필요");
            hooks_.marker(state_.markerType,{Add(Add(point.x,x),5),Add(Add(point.y,y),7)});
        }
        const auto y=Ftol(static_cast<double>(point.y)+Node(token).offsets[offset]+Node(token).margins[2]-(static_cast<double>(Node(token).targetHeight)-Node(token).height));
        const auto x=Ftol(static_cast<double>(point.x)+Node(token).margins[0]-(static_cast<double>(Node(token).targetWidth)-Node(token).width));
        Text(token,index,{x,y},context);
        if (Item(token,index).flags&(kChild|kDynamic)) {
            const auto measured=layout_.MeasureItem(token,offset,context);
            const auto markerX=Ftol((static_cast<double>(Node(token).margins[0])-(static_cast<double>(Node(token).targetWidth)-Node(token).width))+measured[0]);
            const auto markerY=Ftol(static_cast<double>(Node(token).offsets[offset])+Node(token).margins[2]-(static_cast<double>(Node(token).targetHeight)-Node(token).height));
            const auto decoded=DecodeGumpTextContext(context);
            text_.DrawGump(collection_,token,kChildMarker,{markerX,markerY},Node(token).textContext,&decoded);
        }
    }
}
// 현재 분류 결과를 호출마다 조회한다. 다른 타입의 기존 실제 출력은 보존한다.
void BindMenuGumpDraw(GumpDrawHooks& hooks,std::function<const MenuGumpDraw*(std::uint32_t)> resolve) {
    if (!resolve) throw std::invalid_argument("메뉴 출력 분배 조회 필요");const auto previous=hooks.draw;
    // 원본의 가상 +24 분배에 해당하며 임시 클립은 실제 메뉴 Draw가 담당한다.
    hooks.draw=[resolve,previous](std::uint32_t token) {
        if (const auto* menu=resolve(token)) menu->Draw(token);
        else { if (!previous) throw std::invalid_argument("메뉴 외 Draw 경계 필요");previous(token); }
    };
}
// +154는 J 첫 인덱스가 아니라 I 구간 count다. 같은 장식 SHP의 count+2를 물리 프레임으로 출력한다.
void BindMenuGumpDrawAssets(MenuGumpDrawHooks& hooks,const GameAssets& assets,GumpDecorationPixelTarget& pixels) {
    const auto shape=pixels.Hooks({}).shape;
    // 매 표시 요청의 현재 타입 정의를 조회하고 기존 원색 해제 캐시/클립/투명 합성을 사용한다.
    hooks.marker=[&assets,shape](std::uint32_t type,ScreenPoint point) {
        if (type<o::kFirstAssetTypeNumber || type-o::kFirstAssetTypeNumber>=assets.Types().size()) throw std::out_of_range("메뉴 체크 실제 타입 범위 초과");
        const auto count=assets.Types()[type-o::kFirstAssetTypeNumber].definition.FrameTable().Run('I').count;
        shape(type,static_cast<std::uint32_t>(count)+2U,point);
    };
}
}
