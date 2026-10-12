// 동일 노드의 콜백 후 현재 필드와 원본의 DWORD 산술·float 저장 전 배율·가로/세로 입력 차이를 유지한다.
#include "client/ScrollGump.h"
#include "client/TextGump.h"
#include <bit>
#include <cmath>
#include <stdexcept>

namespace netstorm::client {
namespace {
// 원본 flags의 void/프레임 통지와 scrollFlags의 방향, 사건 코드의 하위 분류 마스크다.
constexpr std::uint32_t kVoid=8,kBroadcast=0x100,kHorizontal=1,kEventMask=0x03ffffff;
// 원본 정수 ADD/SUB/IMUL의 하위 DWORD를 signed로 해석한다. C++ signed overflow를 사용하지 않는다.
std::int32_t Add(std::int32_t a,std::int32_t b) { return std::bit_cast<std::int32_t>(std::uint32_t(a)+std::uint32_t(b)); }
std::int32_t Subtract(std::int32_t a,std::int32_t b) { return std::bit_cast<std::int32_t>(std::uint32_t(a)-std::uint32_t(b)); }
std::int32_t Multiply(std::int32_t a,std::int32_t b) { return std::bit_cast<std::int32_t>(std::uint32_t(a)*std::uint32_t(b)); }
// 원본 _ftol은 signed 64비트 절삭 뒤 하위 DWORD를 반환한다. 무효 변환 결과의 하위 DWORD는 0이다.
std::int32_t Ftol(double value) {
    if (!std::isfinite(value) || value<-9223372036854775808.0 || value>=9223372036854775808.0) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::uint64_t>(static_cast<std::int64_t>(value))));
}
}
// 안정된 기본/두 파생 조회만 생성 시 요구한다. 가상 호출과 폴링은 실제 도달 시 검사한다.
ScrollGump::ScrollGump(GumpCreateState& create,ScrollGumpState& state,const GumpTree& tree,
    const GumpCollection& collection,const GumpEvents& events,const GumpInput& input,ScrollGumpHooks hooks)
    :create_(create),state_(state),tree_(tree),collection_(collection),events_(events),input_(input),hooks_(std::move(hooks)) {
    if (!hooks_.node || !hooks_.thumbNode || !hooks_.baseNode) throw std::invalid_argument("ScrollGump 공유 저장소 필요");
}
// Slider의 값/방향/링크와 기본 트리가 같은 노드를 사용하도록 확인한다.
ScrollGumpNode& ScrollGump::Node(std::uint32_t token) const {
    auto* node=token ? hooks_.node(token) : nullptr;
    if (!node || static_cast<GumpCreateNode*>(node)!=hooks_.baseNode(token)) throw std::runtime_error("ScrollGump 공유 노드 없음");
    return *node;
}
// 손잡이 rect/이전 좌표/부모 링크를 복사본이 아닌 같은 객체에서 읽는다.
ScrollGumpThumbNode& ScrollGump::Thumb(std::uint32_t token) const {
    auto* node=token ? hooks_.thumbNode(token) : nullptr;
    if (!node || static_cast<GumpCreateNode*>(node)!=hooks_.baseNode(token)) throw std::runtime_error("ScrollGump 손잡이 공유 노드 없음");
    return *node;
}
// 화살표 버튼은 StoneButtonGump 등 다른 파생 노드도 같은 기본 저장소로 허용한다.
GumpCreateNode& ScrollGump::Base(std::uint32_t token) const {
    auto* node=token ? hooks_.baseNode(token) : nullptr;
    if (!node) throw std::runtime_error("ScrollGump 기본 노드 없음");return *node;
}
// position 저장 뒤 현재 void를 읽는다. Push 콜백 이후의 현재 x/y를 가상 Attach에 전달한다.
void ScrollGump::SetPosition(std::uint32_t token,std::int32_t position) const {
    auto& node=Node(token);
    if (Subtract(node.maximum,node.minimum)<node.page) throw std::invalid_argument("ScrollGump page가 범위보다 큼");
    if (position<node.minimum) position=node.minimum;
    if (node.maximum<Add(node.page,position)) position=Subtract(node.maximum,node.page);
    node.position=position;
    if (!(node.flags&kVoid)) {
        Push(token);
        const float x=Node(token).x,y=Node(token).y;
        if (!hooks_.attach) throw std::invalid_argument("ScrollGump 가상 Attach 경계 필요");
        hooks_.attach(token,x,y,0,0);
    } else {
        if (!hooks_.redraw) throw std::invalid_argument("ScrollGump Redraw 경계 필요");hooks_.redraw(token,false);
    }
}
// signed 범위 길이보다 큰 page만 줄인다. 현재 position 재적용에서 부착/재그리기를 수행한다.
void ScrollGump::SetPage(std::uint32_t token,std::int32_t page) const {
    auto& node=Node(token);const auto length=Subtract(node.maximum,node.minimum);
    if (length<page) page=length;node.page=page;SetPosition(token,Node(token).position);
}
// 원본 패치는 page 보정을 인라인하며 CD는 SetPage를 호출한다. 두 판본 모두 position은 두 번 적용한다.
void ScrollGump::SetRange(std::uint32_t token,std::int32_t first,std::int32_t last) const {
    const auto minimum=first<last ? first : last;auto maximum=first<last ? last : first;
    if (first==last) maximum=Add(minimum,1);
    auto& node=Node(token);
    if (create_.collection.edition==o::OriginalEdition::Patch1078) {
        node.maximum=maximum;node.minimum=minimum;
        const auto length=Subtract(maximum,minimum);if (length<node.page) node.page=length;
        SetPosition(token,Node(token).position);
    } else { node.minimum=minimum;node.maximum=maximum;SetPage(token,Node(token).page); }
    SetPosition(token,Node(token).position);
}
// 조회에는 bool 변환/범위 보정을 추가하지 않는다. Slider 콜백이 바꾼 현재 값도 그대로 반환한다.
std::int32_t ScrollGump::Position(std::uint32_t token) const { return Node(token).position; }
// 실제 가상 Push는 호출자가 같은 GumpTree와 파생 계층에 연결한다.
void ScrollGump::Push(std::uint32_t token) const {
    if (!hooks_.push) throw std::invalid_argument("ScrollGump 가상 Push 경계 필요");hooks_.push(token);
}
// 자식은 mode 0으로 부착한다. 부모 serial/token이나 위치 캐시를 임의 생성하지 않는다.
void ScrollGump::AttachChild(std::uint32_t token,float x,float y,std::uint32_t parent) const {
    if (!hooks_.attach) throw std::invalid_argument("ScrollGump 자식 Attach 경계 필요");hooks_.attach(token,x,y,parent,0);
}
// 기본 크기를 float32에 쓰지만 배율 결과는 저장 전 size로 계산한다.
void ScrollGump::SetSize(std::uint32_t token,double size,bool width) const {
    auto& child=Base(token);
    if (width) { child.baseWidth=static_cast<float>(size);child.width=static_cast<float>(size*child.scale); }
    else { child.baseHeight=static_cast<float>(size);child.height=static_cast<float>(size*child.scale); }
}
// 기본 부모 Attach 이후의 방향/크기를 읽고 세 자식을 현재 토큰 순서로 Push한다.
void ScrollGump::Attach(std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode) const {
    Node(token);tree_.Attach(token,x,y,parent,mode);
    const bool horizontal=(Node(token).scrollFlags&kHorizontal)!=0;
    const auto thickness=Ftol(horizontal ? Node(token).baseHeight : Node(token).baseWidth);
    const float roundedThickness=static_cast<float>(thickness);
    const double secondThickness=create_.collection.edition==o::OriginalEdition::Cd1072 ? roundedThickness : static_cast<double>(thickness);
    const float doubleThickness=static_cast<float>(Add(thickness,thickness));
    Push(Node(token).decrement);Push(Node(token).increment);Push(Node(token).thumb);
    if (horizontal) {
        SetSize(Node(token).decrement,Node(token).baseHeight,false);SetSize(Node(token).decrement,thickness,true);
        SetSize(Node(token).increment,Node(token).baseHeight,false);SetSize(Node(token).increment,secondThickness,true);
        SetSize(Node(token).thumb,Node(token).baseHeight,false);
        SetSize(Node(token).thumb,static_cast<double>(Node(token).baseWidth)-doubleThickness,true);
        AttachChild(Node(token).decrement,0,0,token);
        AttachChild(Node(token).increment,static_cast<float>(static_cast<double>(Node(token).baseWidth)-roundedThickness),0,token);
        AttachChild(Node(token).thumb,roundedThickness,0,token);
    } else {
        SetSize(Node(token).decrement,thickness,false);SetSize(Node(token).decrement,Node(token).baseWidth,true);
        SetSize(Node(token).increment,secondThickness,false);SetSize(Node(token).increment,Node(token).baseWidth,true);
        SetSize(Node(token).thumb,static_cast<double>(Node(token).baseHeight)-doubleThickness,false);
        SetSize(Node(token).thumb,Node(token).baseWidth,true);
        AttachChild(Node(token).decrement,0,0,token);
        AttachChild(Node(token).increment,0,static_cast<float>(static_cast<double>(Node(token).baseHeight)-roundedThickness),token);
        AttachChild(Node(token).thumb,0,roundedThickness,token);
    }
    auto& node=Node(token);auto& thumb=Thumb(node.thumb);
    const double available=static_cast<double>(horizontal ? node.baseWidth : node.baseHeight)-doubleThickness;
    const auto range=Subtract(node.maximum,node.minimum);
    const auto page=Ftol(static_cast<double>(node.page)*available/range);
    const auto position=Ftol(static_cast<double>(node.position)*available/range);
    const auto cross=Subtract(Ftol(horizontal ? node.baseHeight : node.baseWidth),1);
    const auto first=Add(position,1),last=Subtract(Add(position,page),1);
    if (horizontal) {
        thumb.handle.bottom=cross;thumb.handle.left=first;thumb.handle.top=1;thumb.handle.right=last;
    } else { thumb.handle.right=cross;thumb.handle.left=1;thumb.handle.top=first;thumb.handle.bottom=last; }
}
// 원본 세로 몸체는 range*(delta+현재 손잡이 top)의 DWORD 곱을 height-2*width로 나눈다.
void ScrollGump::DragVertical(std::uint32_t token,std::int32_t delta) const {
    const auto& node=Node(token);if (node.scrollFlags&kHorizontal) return;
    const auto numerator=Multiply(Subtract(node.maximum,node.minimum),Add(delta,Thumb(node.thumb).handle.top));
    const double denominator=static_cast<double>(node.baseHeight)-static_cast<double>(node.baseWidth)*2.0;
    SetPosition(token,Ftol(static_cast<double>(numerator)/denominator));
}
// 실제 부모 통지의 발신 serial과 사건 ID는 호출 직전의 현재 값이다.
void ScrollGump::NotifyChange(std::uint32_t token) const { events_.Bubble(token,state_.changeEvent,Base(token).serial,0); }
// 원본은 명령을 소비해도 0을 반환하므로 Bubble의 더 위 부모 전달을 계속 허용한다.
std::uint32_t ScrollGump::Handle(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) const {
    event&=kEventMask;
    if ((event==state_.clickEvent || event==state_.repeatEvent) && first!=Base(Node(token).thumb).serial) {
        const auto previous=Node(token).position;bool arrow=false;
        if (first==Base(Node(token).decrement).serial) {
            arrow=true;
            if (!events_.Bubble(token,state_.stepEvent,Node(token).serial,2))
                SetPosition(token,Subtract(Node(token).position,Node(token).step));
        } else if (first==Base(Node(token).increment).serial) {
            arrow=true;
            if (!events_.Bubble(token,state_.stepEvent,Node(token).serial,1))
                SetPosition(token,Add(Node(token).step,Node(token).position));
        }
        if (arrow && previous!=Node(token).position) NotifyChange(token);
    }
    if (event==state_.stepEvent && first==Base(Node(token).thumb).serial) {
        const auto previous=Node(token).position;
        events_.Bubble(token,state_.stepEvent,Node(token).serial,second);
        if (previous!=Node(token).position) NotifyChange(token);
    }
    return 0;
}
// 범위 비교는 실제 양 끝 포함 손잡이 rect다. 화면 Bounds 조회는 현재 기준점을 계산할 때만 수행한다.
std::uint32_t ScrollGump::ThumbHandle(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) const {
    if ((event&kEventMask)!=InputCode::kLeftButton) return 0;
    auto& thumb=Thumb(token);
    const bool vertical=!(thumb.baseWidth>thumb.baseHeight);
    if (event&InputCode::kRelease) { if (vertical) thumb.flags&=~kBroadcast;return 1; }
    if (!vertical && create_.collection.edition==o::OriginalEdition::Cd1072) return 1;
    const auto bounds=collection_.Bounds(token);
    const auto point=std::bit_cast<std::int32_t>(vertical ? second : first);
    const auto local=Subtract(point,vertical ? bounds.top : bounds.left);
    const auto low=vertical ? Thumb(token).handle.top : Thumb(token).handle.left;
    const auto high=vertical ? Thumb(token).handle.bottom : Thumb(token).handle.right;
    if (local<low) events_.Bubble(token,state_.stepEvent,Thumb(token).serial,4);
    else if (high<local) events_.Bubble(token,state_.stepEvent,Thumb(token).serial,3);
    else {
        if (vertical) Thumb(token).previousY=point;else Thumb(token).previousX=point;
        Thumb(token).flags|=kBroadcast;input_.Capture(token);
    }
    return 1;
}
// 원본은 event/x를 무시하고 현재 왼쪽 버튼 폴링 결과와 전달 y만 사용한다. poll의 반환 좌표는 읽지 않는다.
void ScrollGump::ThumbBroadcast(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) const {
    (void)event;(void)first;
    if (!hooks_.poll) throw std::invalid_argument("ScrollGump 손잡이 폴링 경계 필요");
    if (hooks_.poll(InputCode::kLeftButton).code&InputCode::kRelease) { Thumb(token).flags&=~kBroadcast;return; }
    const auto y=std::bit_cast<std::int32_t>(second),delta=Subtract(y,Thumb(token).previousY);
    if (delta) {
        Thumb(token).previousY=y;DragVertical(Thumb(token).parent,delta);
        events_.Bubble(token,state_.changeEvent,Base(Thumb(token).parent).serial,0);
    }
}
// 순환 의존 처리기 생성 뒤 resolve 포인터를 연결할 수 있도록 각 호출 시 분류를 평가한다.
void BindScrollGump(GumpTreeHooks& tree,GumpEventHooks& events,GumpInputHooks& input,GumpFrameHooks& frame,
    std::function<const ScrollGump*(std::uint32_t)> slider,std::function<const ScrollGump*(std::uint32_t)> thumb) {
    if (!slider || !thumb) throw std::invalid_argument("ScrollGump 분류 경계 필요");
    const auto attach=tree.attach;
    // Slider 자체만 파생 Attach를 호출하고 나머지 돌 버튼/기본 부착 분배를 보존한다.
    tree.attach=[slider,attach](auto token,float x,float y,auto parent,auto mode) {
        if (const auto* handler=slider(token)) handler->Attach(token,x,y,parent,mode);
        else { if (!attach) throw std::invalid_argument("ScrollGump 외 Attach 처리기 필요");attach(token,x,y,parent,mode); }
    };
    const auto event=events.handle,inputEvent=input.handle;
    // 재귀/부모 Bubble과 직접 캡처 입력은 같은 실제 Slider/손잡이 Handle로 분배한다.
    const auto route=[slider,thumb](auto previous,auto token,auto code,auto first,auto second)->std::uint32_t {
        if (const auto* handler=slider(token)) return handler->Handle(token,code,first,second);
        if (const auto* handler=thumb(token)) return handler->ThumbHandle(token,code,first,second);
        if (!previous) throw std::invalid_argument("ScrollGump 외 입력 처리기 필요");return previous(token,code,first,second);
    };
    // 사건 전달은 기존 이벤트 가상 분배를 실패 시 이어서 사용한다.
    events.handle=[route,event](auto token,auto code,auto first,auto second) { return route(event,token,code,first,second); };
    // 직접 캡처/overlay 경로도 같은 클래스 몸체를 사용한다.
    input.handle=[route,inputEvent](auto token,auto code,auto first,auto second) { return route(inputEvent,token,code,first,second); };
    const auto broadcast=frame.broadcast;
    // 원본 손잡이의 가상 +2c만 복원한 폴링/드래그 몸체로 연결한다.
    frame.broadcast=[thumb,broadcast](auto token,auto code,auto first,auto second) {
        if (const auto* handler=thumb(token)) handler->ThumbBroadcast(token,code,first,second);
        else { if (!broadcast) throw std::invalid_argument("ScrollGump 외 Broadcast 처리기 필요");broadcast(token,code,first,second); }
    };
}
// position 조회까지 실제 Slider와 같은 저장소를 사용한다. 기존 TextGump의 현재 토큰 재조회 순서를 유지한다.
void BindScrollingTextSlider(ScrollingTextGumpHooks& hooks,const ScrollGump& slider) {
    // 원본 range(최소/최대) 호출을 새 Slider 범위 몸체로 연결한다.
    hooks.sliderRange=[&slider](auto token,auto first,auto last) { slider.SetRange(token,first,last); };
    // 원본 viewport page 보정/현재 값 재적용을 연결한다.
    hooks.sliderPage=[&slider](auto token,auto page) { slider.SetPage(token,page); };
    // 현재 요청 값을 실제 clamp/Push/Attach/Redraw 몸체로 연결한다.
    hooks.sliderSetPosition=[&slider](auto token,auto position) { slider.SetPosition(token,position); };
    // 세 호출 이후 원본 +a8의 현재 값을 반환한다.
    hooks.sliderPosition=[&slider](auto token) { return slider.Position(token); };
}
}
