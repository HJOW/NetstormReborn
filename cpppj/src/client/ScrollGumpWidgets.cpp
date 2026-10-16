// 부분 초기화·signed 정렬·원본 new 크기·현재 자식 재조회와 돌 버튼/배경의 실제 출력 순서를 유지한다.
#include "client/ScrollGumpWidgets.h"
#include <bit>
#include <cmath>
#include <stdexcept>

namespace netstorm::client {
namespace {
// 실제 파생 가상 표와 new 요청 크기다. 주소는 근거 식별자이며 호스트 함수 포인터로 사용하지 않는다.
constexpr std::uint32_t kSlider=0x5116f8,kCdSlider=0x500c78,kArrow=0x5116a8,kCdArrow=0x500c28,kThumb=0x511668,kCdThumb=0x500be8;
constexpr std::size_t kArrowBytes=0x178,kThumbBytes=0xd8,kCdThumbBytes=0xd4;
// 원본 손잡이 바탕은 눌림 질감/전체 테두리, 내부 사각형은 일반 질감/전체 테두리다.
constexpr std::uint32_t kTrackFlags=0x03ec0000,kHandleFlags=0x03e00000,kThumbFlags=0x201,kKeyboard=0x200,kKeepPopups=0x20;
// 원본 기본 크기/초기 스크롤 값이다. 두 축 중 값 0인 축만 40으로 치환한다.
constexpr std::int32_t kDefaultSize=40,kInitialMaximum=100,kInitialPage=10,kInitialPosition=50;
// 원본 정수 크기/좌표 산술은 하위 DWORD만 유지한다.
std::int32_t Add(std::int32_t a,std::int32_t b) { return std::bit_cast<std::int32_t>(std::uint32_t(a)+std::uint32_t(b)); }
std::int32_t Subtract(std::int32_t a,std::int32_t b) { return std::bit_cast<std::int32_t>(std::uint32_t(a)-std::uint32_t(b)); }
// signed BYTE 정렬의 확장 결과를 원본 DWORD 비트로 보관한다.
std::uint32_t Alignment(std::int8_t value) { return static_cast<std::uint32_t>(static_cast<std::int32_t>(value)); }
// 원본 _ftol의 signed 64비트 절삭 결과에서 하위 DWORD를 읽는다. 무효 변환 결과의 하위 DWORD는 0이다.
std::int32_t Ftol(double value) {
    if (!std::isfinite(value) || value<-9223372036854775808.0 || value>=9223372036854775808.0) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::uint64_t>(static_cast<std::int64_t>(value))));
}
}
// 조회 경계만 필수이며 new/가상 Attach/폰트/픽셀/자산은 실제 도달 시 기존 처리기가 검사한다.
ScrollGumpWidgets::ScrollGumpWidgets(GumpCreateState& create,ScrollGumpWidgetState& state,const GumpCreate& factory,
    const StoneButtonGump& stones,const GumpBackgroundCreate& backgrounds,const ScrollGump& sliders,
    const GumpLifetime& lifetime,ScrollGumpWidgetHooks hooks)
    :create_(create),state_(state),factory_(factory),stones_(stones),backgrounds_(backgrounds),sliders_(sliders),lifetime_(lifetime),hooks_(std::move(hooks)) {
    if (!hooks_.node || !hooks_.arrow || !hooks_.thumb || !hooks_.base) throw std::invalid_argument("Slider 생성/표시 공유 저장소 필요");
}
// 동일 Slider의 값/자식/기본 등록 필드를 읽는다.
ScrollGumpNode& ScrollGumpWidgets::Node(std::uint32_t token) const {
    auto* node=token ? hooks_.node(token) : nullptr;
    if (!node || static_cast<GumpCreateNode*>(node)!=hooks_.base(token)) throw std::runtime_error("Slider 생성 공유 노드 없음");return *node;
}
// 기존 돌 버튼 처리기가 같은 파생 주소의 글자 문맥/배경을 사용해야 한다.
ScrollGumpArrowNode& ScrollGumpWidgets::Arrow(std::uint32_t token) const {
    auto* node=token ? hooks_.arrow(token) : nullptr;
    if (!node || static_cast<GumpCreateNode*>(node)!=hooks_.base(token)) throw std::runtime_error("Slider 화살표 공유 노드 없음");return *node;
}
// 손잡이는 실제 기본 배경을 상속하므로 shape/타입/타일/클립을 복제하지 않는다.
ScrollGumpThumbNode& ScrollGumpWidgets::Thumb(std::uint32_t token) const {
    auto* node=token ? hooks_.thumb(token) : nullptr;
    if (!node || static_cast<GumpCreateNode*>(node)!=hooks_.base(token)) throw std::runtime_error("Slider 손잡이 공유 노드 없음");return *node;
}
// scalar 위임은 세 파생 객체 모두 같은 기본 저장소로 처리한다.
GumpCreateNode& ScrollGumpWidgets::Base(std::uint32_t token) const {
    auto* node=token ? hooks_.base(token) : nullptr;if (!node) throw std::runtime_error("Slider 기본 노드 없음");return *node;
}
// 빈 라벨/모든 8슬롯을 초기화한 명시 문맥으로 기존 돌 버튼 전체 생성자를 호출한다.
std::uint32_t ScrollGumpWidgets::ConstructArrow(std::uint32_t token,std::int32_t width,std::int32_t height,
    std::uint32_t direction,std::uint32_t unused,std::uint32_t color) const {
    Arrow(token);const std::array<std::uint32_t,8> context{0,0,color,0,0,0,1,1};
    stones_.Construct(token,width,height,nullptr,unused,&context);auto& node=Arrow(token);
    node.vtable=create_.collection.edition==o::OriginalEdition::Patch1078 ? kArrow : kCdArrow;
    node.buttonFlags|=kKeepPopups;node.command=Alignment(state_.arrowAlignment);node.direction=direction;return token;
}
// 기본 배경 생성에 넘기는 현재 type/frame은 손잡이의 추가 인자 사본과 별개다.
std::uint32_t ScrollGumpWidgets::ConstructThumb(std::uint32_t token,std::int32_t width,std::int32_t height,
    std::uint32_t type,std::uint32_t frame) const {
    Thumb(token);const auto defaultType=state_.defaultType,defaultFrame=state_.defaultFrame;
    backgrounds_.Construct(token,0,0,kTrackFlags,kThumbFlags,defaultType,defaultFrame);auto& node=Thumb(token);
    node.vtable=create_.collection.edition==o::OriginalEdition::Patch1078 ? kThumb : kCdThumb;
    node.handle={0,0,0,0};node.command=Alignment(state_.thumbAlignment);node.detailType=type;node.detailFrame=frame;
    node.baseWidth=static_cast<float>(width);node.width=static_cast<float>(static_cast<double>(width)*node.scale);
    node.baseHeight=static_cast<float>(height);node.height=static_cast<float>(static_cast<double>(height)*node.scale);
    node.flags|=kKeyboard;return token;
}
// 성공한 new만 파생 생성자로 진행한다. 실패는 원본 null 토큰을 반환한다.
std::uint32_t ScrollGumpWidgets::NewArrow(std::int32_t width,std::int32_t height,std::uint32_t direction,std::uint32_t type,std::uint32_t frame) const {
    if (!hooks_.allocate) throw std::invalid_argument("Slider 자식 할당 경계 필요");
    const auto token=hooks_.allocate(kArrowBytes);return token ? ConstructArrow(token,width,height,direction,type,frame) : 0;
}
// CD 손잡이는 이전 x 필드가 없으며 원본 할당 크기도 4바이트 작다.
std::uint32_t ScrollGumpWidgets::NewThumb(std::int32_t width,std::int32_t height,std::uint32_t type,std::uint32_t frame) const {
    if (!hooks_.allocate) throw std::invalid_argument("Slider 자식 할당 경계 필요");
    const auto token=hooks_.allocate(create_.collection.edition==o::OriginalEdition::Patch1078 ? kThumbBytes : kCdThumbBytes);
    return token ? ConstructThumb(token,width,height,type,frame) : 0;
}
// 처음 자식 부착에서도 실제 가상 함수를 사용하고 원본 null 접근은 호스트 예외로 처리한다.
void ScrollGumpWidgets::Attach(std::uint32_t token,float x,float y,std::uint32_t parent) const {
    Base(token);if (!hooks_.attach) throw std::invalid_argument("Slider 자식 가상 Attach 경계 필요");hooks_.attach(token,x,y,parent,0);
}
// Slider 기본 필드 초기화 이후 +bc와 미작성 자식 파생 필드는 보존한다. 세 자식 할당 뒤 부착/값 통지를 수행한다.
std::uint32_t ScrollGumpWidgets::Construct(std::uint32_t token,std::int32_t width,std::int32_t height,
    std::uint32_t type,std::uint32_t frame) const {
    Node(token);factory_.Construct(token,0,0,0,1);auto& node=Node(token);
    const bool patch=create_.collection.edition==o::OriginalEdition::Patch1078;
    node.vtable=patch ? kSlider : kCdSlider;node.minimum=node.maximum=node.position=node.page=node.step=0;
    node.reservedB0=0;node.scrollFlags=0;node.reserved={0,0};node.decrement=node.increment=node.thumb=0;
    node.command=Alignment(state_.sliderAlignment);
    if (!width) width=kDefaultSize;if (!height) height=kDefaultSize;
    const bool horizontal=height<width;node.scrollFlags=horizontal ? 1U : 0U;node.flags|=kKeyboard;node.step=1;
    node.baseWidth=static_cast<float>(width);node.width=static_cast<float>(static_cast<double>(width)*node.scale);
    // 패치는 높이만 지역 float32를 다시 읽으며 CD는 두 축 모두 FILD 중간값을 배율 곱까지 유지한다.
    node.baseHeight=static_cast<float>(height);node.height=static_cast<float>((patch ? static_cast<double>(node.baseHeight) : static_cast<double>(height))*node.scale);
    // 패치 세로 분기는 저장 전 폭의 x87 값을 절삭한다. 그 밖의 분기는 저장한 짧은 축을 다시 읽는다.
    const auto thickness=Ftol(horizontal ? node.baseHeight : patch ? static_cast<double>(width) : node.baseWidth),twice=Add(thickness,thickness);
    if (horizontal) {
        Node(token).decrement=NewArrow(thickness,height,3,type,frame);
        Node(token).increment=NewArrow(thickness,height,1,type,frame);
        Node(token).thumb=NewThumb(Subtract(width,twice),height,type,frame);
        Attach(Node(token).decrement,0,0,token);
        Attach(Node(token).increment,static_cast<float>(Subtract(width,thickness)),0,token);
        Attach(Node(token).thumb,static_cast<float>(thickness),0,token);
    } else {
        Node(token).decrement=NewArrow(width,thickness,0,type,frame);
        Node(token).increment=NewArrow(width,thickness,2,type,frame);
        Node(token).thumb=NewThumb(width,Subtract(height,twice),type,frame);
        Attach(Node(token).decrement,0,0,token);
        Attach(Node(token).increment,0,static_cast<float>(Subtract(height,thickness)),token);
        Attach(Node(token).thumb,0,static_cast<float>(thickness),token);
    }
    Node(token).position=0;sliders_.SetRange(token,0,kInitialMaximum);sliders_.SetPage(token,kInitialPage);sliders_.SetPosition(token,kInitialPosition);
    return token;
}
// 삼각형 색은 기본 32바이트 문맥에서 읽는다. 중점의 합은 DWORD로 감기고 나눗셈은 0 방향으로 절삭한다.
void ScrollGumpWidgets::DrawArrow(std::uint32_t token,const GumpCollection& collection,const GumpTextDraw& text,const GumpPixelTarget& target) const {
    Arrow(token);stones_.Draw(token,collection,text,target);const auto bounds=collection.Bounds(token);
    const auto color=Arrow(token).textContext[2],direction=Arrow(token).direction;
    const auto left=Add(bounds.left,2),top=Add(bounds.top,2),right=Subtract(bounds.right,2),bottom=Subtract(bounds.bottom,2);
    const auto middleX=Add(left,right)/2,middleY=Add(top,bottom)/2;
    std::array<GumpLine,3> lines;
    if (direction==1) lines={GumpLine{{left,top},{left,bottom},color},{{left,top},{right,middleY},color},{{left,bottom},{right,middleY},color}};
    else if (direction==2) lines={GumpLine{{left,top},{right,top},color},{{left,top},{middleX,bottom},color},{{right,top},{middleX,bottom},color}};
    else if (direction==3) lines={GumpLine{{right,top},{right,bottom},color},{{right,top},{left,middleY},color},{{right,bottom},{left,middleY},color}};
    else lines={GumpLine{{left,bottom},{right,bottom},color},{{left,bottom},{middleX,top},color},{{right,bottom},{middleX,top},color}};
    target.Lines(lines);
}
// 배경 Draw가 갱신한 현재 위치/타입/프레임을 다시 읽는다. 포함 손잡이 끝점을 그대로 배경 함수에 전달한다.
void ScrollGumpWidgets::DrawThumb(std::uint32_t token,const GumpCollection& collection,const GumpBackgroundDraw& backgrounds) const {
    Thumb(token);backgrounds.Draw(token);const auto bounds=collection.Bounds(token);const auto& node=Thumb(token);
    const ScreenRect rect{Add(bounds.left,node.handle.left),Add(bounds.top,node.handle.top),Add(bounds.left,node.handle.right),Add(bounds.top,node.handle.bottom)};
    backgrounds.Background(rect,kHandleFlags,node.type,node.frame);
}
// 원본 파생 scalar는 추가 소유 자원이 없어 공통 기본 소멸과 반납으로 진행한다.
std::uint32_t ScrollGumpWidgets::DeleteScalar(std::uint32_t token,std::uint32_t flags) const { Base(token);return lifetime_.DeleteScalar(token,flags); }
// 픽셀/글꼴/배경이 같은 화면의 현재 클립을 사용하도록 가상 Draw에 분배한다.
void BindScrollGumpWidgetDraw(GumpDrawHooks& hooks,const GumpCollection& collection,const GumpTextDraw& text,
    const GumpPixelTarget& target,const GumpBackgroundDraw& backgrounds,
    std::function<const ScrollGumpWidgets*(std::uint32_t)> slider,std::function<const ScrollGumpWidgets*(std::uint32_t)> arrow,
    std::function<const ScrollGumpWidgets*(std::uint32_t)> thumb) {
    if (!slider || !arrow || !thumb) throw std::invalid_argument("Slider Draw 분류 경계 필요");const auto previous=hooks.draw;
    // 원본 Slider 가상 Draw는 ret뿐인 빈 몸체다. 두 표시 자식과 그 밖의 기존 분배를 별도로 호출한다.
    hooks.draw=[&collection,&text,&target,&backgrounds,slider=std::move(slider),arrow=std::move(arrow),thumb=std::move(thumb),previous](auto token) {
        if (slider(token)) return;
        if (const auto* handler=arrow(token)) handler->DrawArrow(token,collection,text,target);
        else if (const auto* thumbHandler=thumb(token)) thumbHandler->DrawThumb(token,collection,backgrounds);
        else { if (!previous) throw std::invalid_argument("Slider 외 Draw 처리기 필요");previous(token); }
    };
}
// 현재 파생 토큰의 처리기를 지연 조회해 공통 자식 우선 Destroy/Cleanup과 결합한다.
void BindScrollGumpWidgetLifetime(GumpLifetimeHooks& hooks,std::function<const ScrollGumpWidgets*(std::uint32_t)> resolve) {
    if (!resolve) throw std::invalid_argument("Slider 수명 분류 경계 필요");const auto previous=hooks.deletingDestructor;
    // 파생 가상 분배를 재호출하지 않고 기본 scalar 소멸 몸체에 직접 위임한다.
    hooks.deletingDestructor=[resolve=std::move(resolve),previous](auto token,auto flags) {
        if (const auto* handler=resolve(token)) handler->DeleteScalar(token,flags);
        else { if (!previous) throw std::invalid_argument("Slider 외 scalar 소멸자 필요");previous(token,flags); }
    };
}
}
