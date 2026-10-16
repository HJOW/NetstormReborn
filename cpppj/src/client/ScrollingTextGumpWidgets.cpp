// 원본의 소유 여부·현재 자식/요청/태그 재조회와 overlay·캡처·프레임 입력 순서를 유지한다.
#include "client/ScrollingTextGumpWidgets.h"
#include <bit>
#include <cmath>
#include <stdexcept>

namespace netstorm::client {
namespace {
// 두 PE 다중 줄 가상 표와 생성 요청 크기·기본 버퍼/Slider 크기다. 주소는 실행 포인터가 아닌 근거 식별자다.
constexpr std::uint32_t kVtable=0x51454c,kCdVtable=0x505d80;
constexpr std::size_t kBufferBytes=0x38,kSliderBytes=0xd8;
constexpr std::int32_t kLines=128,kSliderWidth=9,kSliderHeight=100,kStep=5;
// 원본 생성/입력 플래그와 세 가지 커서 번호다. 이벤트 분류는 modifier/release를 별도로 보관한다.
constexpr std::uint32_t kCreateFlags=0x400a00,kVisible=1,kNotify=0x100,kDirty=1,kUseSlider=2,kScroll=4,kLeftTag=8,kRightTag=16,kEventMask=0x03ffffff;
constexpr std::uint32_t kNormalCursor=1,kScrollCursor=15,kTagCursor=16;
// DWORD 덧셈/뺄셈을 signed overflow 없이 같은 비트로 해석한다.
std::int32_t Subtract(std::int32_t a,std::int32_t b) { return std::bit_cast<std::int32_t>(std::uint32_t(a)-std::uint32_t(b)); }
// 원본 CRT _ftol의 signed 64비트 절삭 결과에서 하위 DWORD를 읽는다. 무효 변환의 하위 DWORD는 0이다.
std::int32_t Ftol(double value) {
    if (!std::isfinite(value) || value<-9223372036854775808.0 || value>=9223372036854775808.0) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::uint64_t>(static_cast<std::int64_t>(value))));
}
}
// 실제 버퍼 생성은 Construct에서 수행한다. release는 원본 free의 필수 호스트 반납 경계다.
StyleTextBufferStore::StyleTextBufferStore(StyleTextState& fragments,const StyleText& style,StyleTextBufferHooks hooks,
    std::function<void(std::uint32_t)> release):fragments_(fragments),style_(style),hooks_(std::move(hooks)),release_(std::move(release)) {
    if (!release_) throw std::invalid_argument("StyleTextBuffer 토큰 반납 경계 필요");
}
// 버퍼 레코드는 별도 토큰 표에만 등록한다. Gump 등록 수/serial을 증가시키지 않는다.
std::uint32_t StyleTextBufferStore::Construct(std::uint32_t token,std::int32_t maxLines) {
    if (!token || records_.contains(token)) throw std::invalid_argument("StyleTextBuffer 새 토큰 필요");
    auto record=std::make_unique<Record>();
    record->handler=std::make_unique<StyleTextBuffer>(record->state,fragments_,style_,maxLines,hooks_);
    records_.emplace(token,std::move(record));return token;
}
// unique_ptr 레코드는 표 삽입으로 주소가 이동하지 않으며 소멸 전까지 같은 상태를 공급한다.
StyleTextBufferStore::Record& StyleTextBufferStore::Entry(std::uint32_t token) const {
    const auto found=records_.find(token);if (!token || found==records_.end()) throw std::runtime_error("StyleTextBuffer 등록 토큰 없음");
    return *found->second;
}
// 현재 버퍼 handler가 참조하는 state를 직접 반환한다. 표시 복사본을 만들지 않는다.
TextGumpBufferBinding StyleTextBufferStore::Binding(std::uint32_t token) const { auto& entry=Entry(token);return {&entry.state,entry.handler.get()}; }
// 원본 버퍼 소멸 순서를 수행한 뒤 실제 free 통지와 호스트 상태 기록 반납을 이어간다.
void StyleTextBufferStore::Destroy(std::uint32_t token) {
    Entry(token).handler->Destroy();release_(token);records_.erase(token);
}
// 공유 노드 조회만 필수다. 가상/커서/할당 경계는 실제 원본 분기에 도달할 때 검사한다.
ScrollingTextGumpWidgets::ScrollingTextGumpWidgets(GumpCreateState& create,ScrollingTextGumpWidgetState& state,const GumpCreate& factory,
    const GumpTree& tree,const GumpLifetime& lifetime,const GumpEvents& events,const GumpInput& input,
    StyleTextState& fragments,StyleTextBufferStore& buffers,const ScrollingTextGump& text,
    const ScrollGumpWidgets& sliderWidgets,ScrollingTextGumpWidgetHooks hooks)
    :create_(create),state_(state),factory_(factory),tree_(tree),lifetime_(lifetime),events_(events),input_(input),fragments_(fragments),
    buffers_(buffers),text_(text),sliderWidgets_(sliderWidgets),hooks_(std::move(hooks)) {
    if (!hooks_.node || !hooks_.slider || !hooks_.base) throw std::invalid_argument("다중 줄 Widget 공유 저장소 필요");
}
// 표시/입력/수명과 생성이 같은 기본·파생 객체를 사용해야 한다.
ScrollingTextGumpNode& ScrollingTextGumpWidgets::Node(std::uint32_t token) const {
    auto* node=token ? hooks_.node(token) : nullptr;
    if (!node || static_cast<GumpCreateNode*>(node)!=hooks_.base(token)) throw std::runtime_error("다중 줄 Widget 공유 노드 없음");return *node;
}
// Slider 값/serial/기본 크기는 실제 생성한 같은 Slider 저장소에서 읽는다.
ScrollGumpNode& ScrollingTextGumpWidgets::Slider(std::uint32_t token) const {
    auto* node=token ? hooks_.slider(token) : nullptr;
    if (!node || static_cast<GumpCreateNode*>(node)!=hooks_.base(token)) throw std::runtime_error("다중 줄 Widget Slider 노드 없음");return *node;
}
// 가상 Push/Attach의 토큰도 실제 기본 저장소가 존재해야 한다.
GumpCreateNode& ScrollingTextGumpWidgets::Base(std::uint32_t token) const {
    auto* node=token ? hooks_.base(token) : nullptr;if (!node) throw std::runtime_error("다중 줄 Widget 기본 노드 없음");return *node;
}
// 원본 가상 +8 인자는 erase=false다. 콜백이 변경한 현재 요청/플래그를 되돌리지 않는다.
void ScrollingTextGumpWidgets::Redraw(std::uint32_t token) const {
    if (!hooks_.redraw) throw std::invalid_argument("다중 줄 Widget Redraw 경계 필요");hooks_.redraw(token,false);
}
// Slider의 실제 가상 Push를 호출하고 이후 현재 Slider 필드를 다시 읽는다.
void ScrollingTextGumpWidgets::Push(std::uint32_t token) const {
    Base(token);if (!hooks_.push) throw std::invalid_argument("다중 줄 Widget Push 경계 필요");hooks_.push(token);
}
// 원본 우측 Slider는 mode 1로 화면 제한을 생략하며 y는 항상 0이다.
void ScrollingTextGumpWidgets::AttachSlider(std::uint32_t token,float x,std::uint32_t parent) const {
    Base(token);if (!hooks_.attach) throw std::invalid_argument("다중 줄 Widget Attach 경계 필요");hooks_.attach(token,x,0,parent,1);
}
// 비0은 원본 입력 제외 비트를 해제한다. 값의 최하위 비트가 아닌 전체 DWORD의 0/비0을 사용한다.
void ScrollingTextGumpWidgets::SetSelectable(std::uint32_t token,std::int32_t value) const {
    if (value) Node(token).flags&=~0x200U;else Node(token).flags|=0x200U;
}
// slider 설정 뒤 Push가 변경한 위치/필드를 현재 노드에서 읽어 같은 가상 Attach 경계에 전달한다.
void ScrollingTextGumpWidgets::SetSlider(std::uint32_t token,std::uint32_t value) const {
    auto& node=Node(token);node.textFlags=(node.textFlags&~2U)|((value&1U)<<1);
    if ((node.textFlags&6U)==2U) node.textFlags|=4U;
    if (!(Node(token).flags&8U)) {
        Push(token);const auto x=Node(token).x,y=Node(token).y;
        if (!hooks_.attach) throw std::invalid_argument("다중 줄 Widget 자기 Attach 경계 필요");
        hooks_.attach(token,x,y,0,0);
    }
}
// 줄바꿈 설정의 변경 플래그 누락도 유지한다. 현재 빌린 버퍼에도 같은 noWrap 필드를 쓴다.
void ScrollingTextGumpWidgets::SetWrap(std::uint32_t token,std::int32_t value) const {
    buffers_.Binding(Node(token).buffer).state->noWrap=value==0 ? 1U : 0U;
}
// scroll을 끄면 현재 slider도 원본 설정 함수로 꺼서 가상 재부착까지 이어간다.
void ScrollingTextGumpWidgets::SetScrollable(std::uint32_t token,std::uint32_t value) const {
    auto& node=Node(token);node.textFlags=(node.textFlags&~4U)|((value&1U)<<2);
    if ((node.textFlags&6U)==2U) SetSlider(token,0);
}
// 새 버퍼는 null만 거부한다. 원본의 같은 포인터 재설치와 소유 DWORD 그대로 쓰기를 유지한다.
void ScrollingTextGumpWidgets::ReplaceBuffer(std::uint32_t token,std::uint32_t buffer,std::uint32_t owns) const {
    if (!buffer) throw std::invalid_argument("다중 줄 Widget 교체 버퍼는 null일 수 없음");
    const auto old=Node(token).buffer;
    if (old && Node(token).ownsBuffer) { buffers_.Destroy(old);Node(token).buffer=0; }
    Node(token).buffer=buffer;Node(token).ownsBuffer=owns;
}
// 기본 등록 뒤 실제 128칸 버퍼와 Slider를 생성한다. +ac/dragY와 생성자 미작성 textFlags는 전체 초기화하지 않는다.
std::uint32_t ScrollingTextGumpWidgets::Construct(std::uint32_t token,std::int32_t width,std::int32_t height,std::uint32_t font,std::uint32_t color) const {
    Node(token);factory_.Construct(token,0,0,0,1);auto& node=Node(token);
    node.vtable=create_.collection.edition==o::OriginalEdition::Patch1078 ? kVtable : kCdVtable;
    node.reserved={0,0};node.command=static_cast<std::uint32_t>(static_cast<std::int32_t>(state_.alignment));
    if (!hooks_.allocate) throw std::invalid_argument("다중 줄 Widget 할당 경계 필요");
    const auto buffer=hooks_.allocate(kBufferBytes);
    Node(token).buffer=buffer ? buffers_.Construct(buffer,kLines) : 0;Node(token).ownsBuffer=1;
    if (!Node(token).buffer) throw std::runtime_error("다중 줄 Widget 버퍼 할당 실패");
    const float savedWidth=static_cast<float>(width);
    node.baseWidth=savedWidth;node.width=static_cast<float>(static_cast<double>(width)*node.scale);
    node.baseHeight=static_cast<float>(height);node.height=static_cast<float>(static_cast<double>(height)*node.scale);
    node.textContext={0,font,color,0,0,0,1,1};node.layoutWidth=0;node.scroll=0;node.callback=0;node.flags|=kCreateFlags;
    lifetime_.SetOverlay(token,true);node.scrollRequest=-1;node.bottomPadding=0;
    auto slider=hooks_.allocate(kSliderBytes);
    if (slider) slider=sliderWidgets_.Construct(slider,kSliderWidth,kSliderHeight,state_.sliderType,state_.sliderFrame);
    Node(token).slider=slider;Slider(slider).flags&=~kVisible;
    AttachSlider(Node(token).slider,static_cast<float>(static_cast<double>(savedWidth)-Slider(Node(token).slider).baseWidth),token);
    text_.RequestBottom(token);Node(token).textFlags=(Node(token).textFlags&~(kUseSlider|kScroll))|kLeftTag|kRightTag;
    Node(token).metadata=0;return token;
}
// 기본 부착 중에도 파생 자식 분배를 사용한다. 그 뒤 현재 Slider의 표시/높이와 부모 폭 변경을 처리한다.
void ScrollingTextGumpWidgets::Attach(std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode) const {
    Node(token);tree_.Attach(token,x,y,parent,mode);Push(Node(token).slider);
    auto& slider=Slider(Node(token).slider);const double height=Node(token).baseHeight;
    slider.baseHeight=static_cast<float>(height);slider.height=static_cast<float>(height*slider.scale);
    if ((Node(token).textFlags&(kUseSlider|kScroll))==(kUseSlider|kScroll)) slider.flags|=kVisible;else slider.flags&=~kVisible;
    AttachSlider(Node(token).slider,static_cast<float>(static_cast<double>(Node(token).baseWidth)-Slider(Node(token).slider).baseWidth),token);
    if (Node(token).scrollRequest==-1) text_.RequestScroll(token,Node(token).scroll);
    const auto previousWidth=Node(token).layoutWidth,currentWidth=Node(token).baseWidth;
    // 패치는 unordered도 폭 변경으로 처리하며 CD는 x87 C3만 검사해 NaN 비교에서는 저장을 생략한다.
    const bool changed=previousWidth!=currentWidth && (create_.collection.edition==o::OriginalEdition::Patch1078 || (!std::isnan(previousWidth) && !std::isnan(currentWidth)));
    if (changed) { Node(token).textFlags|=kDirty;Node(token).layoutWidth=Node(token).baseWidth; }
}
// 실제 조각의 현재 T 태그는 문자열/그림을 구분하지 않는다.
std::uint32_t ScrollingTextGumpWidgets::Tag(std::uint32_t fragment) const {
    const auto found=fragments_.fragments.find(fragment);if (!fragment || found==fragments_.fragments.end()) throw std::runtime_error("다중 줄 Widget 태그 조각 없음");
    return found->second.metadata;
}
// 화면 좌표를 원본 signed DWORD로 전달한다. HitTest 자체는 Layout/대기 스크롤을 적용하지 않는다.
std::uint32_t ScrollingTextGumpWidgets::Hit(std::uint32_t token,std::uint32_t x,std::uint32_t y) const {
    return text_.HitTest(token,{std::bit_cast<std::int32_t>(x),std::bit_cast<std::int32_t>(y)});
}
// Slider 사건을 소비한 뒤에도 원본의 후속 refresh/마우스/커서 분류를 계속 검사한다.
std::uint32_t ScrollingTextGumpWidgets::Handle(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) const {
    const auto code=event&kEventMask;std::uint32_t result=0;
    if (code==state_.changeEvent) {
        if (first!=Slider(Node(token).slider).serial) throw std::invalid_argument("다중 줄 Widget 변화 사건 발신 Slider 불일치");
        text_.RequestScroll(token,Slider(Node(token).slider).position);
    }
    if (code==state_.stepEvent && first==Slider(Node(token).slider).serial) {
        std::int32_t delta=0;bool step=true;
        switch (second) {
        case 1: delta=kStep;break;
        case 2: delta=-kStep;break;
        case 3: delta=Ftol(Node(token).baseHeight);break;
        case 4: delta=Ftol(-static_cast<double>(Node(token).baseHeight));break;
        default: step=false;break;
        }
        if (step) { text_.RequestRelativeScroll(token,delta);result=1; }
    }
    if (code==state_.refreshEvent) { Node(token).textFlags|=kDirty;Redraw(token); }
    if (code!=state_.cursorEvent) {
        if (code==InputCode::kLeftButton) {
            const auto fragment=Node(token).textFlags&kLeftTag ? Hit(token,first,second) : 0;
            if (!fragment || !Tag(fragment)) {
                if (event&InputCode::kRelease) Node(token).flags&=~kNotify;
                else { Node(token).dragY=std::bit_cast<std::int32_t>(second);Node(token).flags|=kNotify;input_.Capture(token); }
            } else if (!(event&InputCode::kRelease)) events_.Bubble(token,state_.leftTagEvent,Node(token).serial,Tag(fragment));
            result=1;
        }
        if (code==InputCode::kRightButton && (Node(token).textFlags&kRightTag)) {
            if (!(event&InputCode::kRelease)) {
                const auto fragment=Hit(token,first,second);
                if (fragment) { events_.Bubble(token,state_.rightTagEvent,Node(token).serial,Tag(fragment));return 1; }
            }
            result=0;
        }
        return result;
    }
    std::uint32_t cursor=kNormalCursor;
    const auto fragment=Node(token).textFlags&kLeftTag ? Hit(token,first,second) : 0;
    if (fragment && Tag(fragment)) cursor=kTagCursor;
    else if (Node(token).textFlags&kScroll) {
        const auto& slider=Slider(Node(token).slider);
        if (slider.page<Subtract(slider.maximum,slider.minimum)) cursor=kScrollCursor;
    }
    if (!hooks_.cursor) throw std::invalid_argument("다중 줄 Widget 커서 선택 경계 필요");hooks_.cursor(cursor);return 1;
}
// 폴링/사건 종류/x를 읽지 않는다. bit 4가 없으면 이전 y도 갱신하지 않는다.
void ScrollingTextGumpWidgets::Broadcast(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) const {
    (void)event;(void)first;if (!(Node(token).textFlags&kScroll)) return;
    const auto y=std::bit_cast<std::int32_t>(second),delta=Subtract(y,Node(token).dragY);
    if (delta) { Node(token).dragY=y;text_.RequestRelativeScroll(token,Subtract(0,delta)); }
}
// 버퍼 포인터는 소멸 전 저장하고 소유 여부는 현재 값을 검사한다. free 콜백 뒤 현재 필드를 0으로 덮는다.
std::uint32_t ScrollingTextGumpWidgets::DeleteScalar(std::uint32_t token,std::uint32_t flags) const {
    Node(token).vtable=create_.collection.edition==o::OriginalEdition::Patch1078 ? kVtable : kCdVtable;
    const auto buffer=Node(token).buffer;
    if (buffer && Node(token).ownsBuffer) { buffers_.Destroy(buffer);Node(token).buffer=0; }
    return lifetime_.DeleteScalar(token,flags);
}
// 원본 버퍼 생성/표시/소멸이 하나의 상태/handler 레코드를 공유하도록 연결한다.
void BindScrollingTextBuffers(ScrollingTextGumpHooks& hooks,const StyleTextBufferStore& buffers) {
    // 각 표시 호출에서 현재 token으로 같은 실제 버퍼를 지연 조회한다.
    hooks.buffer=[&buffers](auto token) { return buffers.Binding(token); };
}
// 처리기 생성 뒤에도 resolve 포인터를 연결할 수 있도록 매 가상 호출에서 현재 분류를 평가한다.
void BindScrollingTextGumpWidgets(GumpTreeHooks& tree,GumpEventHooks& events,GumpInputHooks& input,
    GumpFrameHooks& frame,GumpLifetimeHooks& lifetime,std::function<const ScrollingTextGumpWidgets*(std::uint32_t)> resolve) {
    if (!resolve) throw std::invalid_argument("다중 줄 Widget 분류 경계 필요");
    const auto attach=tree.attach;
    // 기본 부모 부착은 Widgets에서 실제 tree 몸체로 직접 위임해 가상 재귀를 피한다.
    tree.attach=[resolve,attach](auto token,float x,float y,auto parent,auto mode) {
        if (const auto* handler=resolve(token)) handler->Attach(token,x,y,parent,mode);
        else { if (!attach) throw std::invalid_argument("다중 줄 Widget 외 Attach 필요");attach(token,x,y,parent,mode); }
    };
    const auto event=events.handle,inputEvent=input.handle;
    // 부모 Bubble/재귀 선택과 직접 캡처 입력은 같은 실제 Widget Handle에 도달한다.
    const auto route=[resolve](auto previous,auto token,auto code,auto first,auto second)->std::uint32_t {
        if (const auto* handler=resolve(token)) return handler->Handle(token,code,first,second);
        if (!previous) throw std::invalid_argument("다중 줄 Widget 외 입력 필요");return previous(token,code,first,second);
    };
    // 기존 사건 분배를 보존하며 같은 다중 줄 토큰만 파생 처리기로 보낸다.
    events.handle=[route,event](auto token,auto code,auto first,auto second) { return route(event,token,code,first,second); };
    // 캡처/overlay의 직접 경로에도 같은 분배를 적용한다.
    input.handle=[route,inputEvent](auto token,auto code,auto first,auto second) { return route(inputEvent,token,code,first,second); };
    const auto broadcast=frame.broadcast;
    // 기존 프레임 통지 필터 이후 실제 원본 y 드래그 몸체를 실행한다.
    frame.broadcast=[resolve,broadcast](auto token,auto code,auto first,auto second) {
        if (const auto* handler=resolve(token)) handler->Broadcast(token,code,first,second);
        else { if (!broadcast) throw std::invalid_argument("다중 줄 Widget 외 Broadcast 필요");broadcast(token,code,first,second); }
    };
    const auto deleting=lifetime.deletingDestructor;
    // 자식 우선 파괴/등록 해제 뒤 버퍼 소유와 실제 기본 scalar 소멸을 수행한다.
    lifetime.deletingDestructor=[resolve,deleting](auto token,auto flags) {
        if (const auto* handler=resolve(token)) handler->DeleteScalar(token,flags);
        else { if (!deleting) throw std::invalid_argument("다중 줄 Widget 외 scalar 필요");deleting(token,flags); }
    };
}
}
