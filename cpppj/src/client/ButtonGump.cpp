// 원본의 가상 호출 인자·콜백 뒤 필드 조회·벽 시계 조회 순서를 보존한다.
#include "client/ButtonGump.h"
#include "client/GumpBackground.h"
#include <bit>
#include <stdexcept>
#include <unordered_set>

namespace netstorm::client {
namespace {
// 버튼 플래그: 비활성/토글/즉시/점멸 pulse/팝업 유지다. Gump 통지와 팝업/삭제 비트는 별개다.
constexpr std::uint32_t kDisabled=1,kToggle=2,kInstant=4,kPulse=0x10,kKeepPopups=0x20;
constexpr std::uint32_t kNotify=0x100,kPopup=0x4000,kDead=0x10000,kEventMask=0x03ffffff;
// PE의 double 상수다. 각 예약마다 벽 시계를 따로 읽으며 프레임 간격/게임 시간으로 대체하지 않는다.
constexpr double kFirstDelay=0.4,kRepeatDelay=0.05,kBlinkDelay=1.0;
// 기본 ButtonGump 가상 표 주소는 raw 근거용 식별자다. C++에서 원본 주소로 분기하지 않는다.
constexpr std::uint32_t kPatchVtable=0x503848,kCdVtable=0x5017d8;
}
// 실제 생성/입력/수명 처리기를 참조하며 복제한 표나 별도 캡처 상태를 만들지 않는다.
ButtonGump::ButtonGump(GumpCreateState& create,ButtonGumpState& state,const GumpCreate& factory,
    const GumpCollection& collection,const GumpEvents& events,const GumpInput& input,
    const GumpLifetime& lifetime,ButtonGumpHooks hooks)
    :create_(create),state_(state),factory_(factory),collection_(collection),events_(events),input_(input),
    lifetime_(lifetime),hooks_(std::move(hooks)) {
    if (!hooks_.node || !hooks_.baseNode) throw std::invalid_argument("버튼/기본 Gump 저장소 필요");
}
// 기본 필드와 파생 필드는 같은 객체 주소여야 한다. 손상된 토큰은 호스트 계약으로 거부한다.
ButtonGumpNode& ButtonGump::Node(std::uint32_t token) const {
    auto* node=token ? hooks_.node(token) : nullptr;
    if (!node || static_cast<GumpCreateNode*>(node)!=hooks_.baseNode(token))
        throw std::runtime_error("버튼 Gump 토큰/공유 저장소 없음");
    return *node;
}
// 팝업 순회에는 버튼 이외의 파생 객체도 포함된다.
GumpCreateNode& ButtonGump::BaseNode(std::uint32_t token) const {
    auto* node=token ? hooks_.baseNode(token) : nullptr;
    if (!node) throw std::runtime_error("버튼 기본 Gump 토큰 없음");return *node;
}
// 콜백 호출 수를 줄이려고 같은 시각을 재사용하지 않는다.
double ButtonGump::Now() const {
    if (!hooks_.now) throw std::invalid_argument("버튼 벽 시계 필요");return hooks_.now();
}
// 실제 픽셀 Draw와 구분되는 가상 재그리기 요청이다.
void ButtonGump::Redraw(std::uint32_t token) const {
    if (!hooks_.redraw) throw std::invalid_argument("버튼 재그리기 경계 필요");hooks_.redraw(token,false);
}
// 기존 기본 생성자를 호출해 등록/serial/void 초기화를 공유하며 미작성 파생 필드는 보존한다.
std::uint32_t ButtonGump::ConstructBase(std::uint32_t token,std::int32_t width,std::int32_t height,std::uint32_t flags) const {
    auto& node=Node(token);factory_.Construct(token,0,0,0,1);
    node.vtable=create_.collection.edition==o::OriginalEdition::Patch1078 ? kPatchVtable : kCdVtable;
    node.firstDeadline=0;node.repeatDeadline=0;node.command=static_cast<std::uint32_t>(static_cast<std::int32_t>(state_.baseColor));
    node.flags|=0xa00;node.buttonFlags=flags;node.value=0;node.pressed=0;node.blink=0;node.kind=ButtonGumpKind::Base;
    node.baseWidth=static_cast<float>(width);node.width=static_cast<float>(static_cast<double>(width)*node.scale);
    node.baseHeight=static_cast<float>(height);node.height=static_cast<float>(static_cast<double>(height)*node.scale);
    return token;
}
// 돌 버튼은 쓰기 전의 두 상태 OR와 부모 redraw 뒤 현재 두 상태 OR를 비교한다.
void ButtonGump::WriteState(std::uint32_t token,std::uint32_t value,bool pressed) const {
    auto& node=Node(token);const bool old=bool(node.value || node.pressed);
    const auto kind=node.kind;if (pressed) node.pressed=value;else node.value=value;Redraw(token);
    const bool current=bool(node.value || node.pressed);
    if (kind==ButtonGumpKind::Stone && old!=current) {
        if (!hooks_.backgroundFlags) throw std::invalid_argument("돌 버튼 배경 저장소 필요");
        auto* flags=hooks_.backgroundFlags(node.background);
        if (!flags) throw std::runtime_error("돌 버튼 배경 플래그 없음");
        *flags=current ? *flags|GumpBackgroundFlags::kPressed : *flags&~GumpBackgroundFlags::kPressed;
        Redraw(node.background);
    }
}
// 동일 값이라도 원본처럼 부모 redraw를 호출한다.
void ButtonGump::SetValue(std::uint32_t token,std::uint32_t value) const { WriteState(token,value,false); }
// 마우스 누름도 전체 DWORD를 보존한다.
void ButtonGump::SetPressed(std::uint32_t token,std::uint32_t value) const { WriteState(token,value,true); }
// 비활성화를 캡처/idle 해제로 확대하지 않는다.
void ButtonGump::SetEnabled(std::uint32_t token,bool enabled) const {
    auto& node=Node(token);node.buttonFlags=enabled ? node.buttonFlags&~kDisabled : node.buttonFlags|kDisabled;Redraw(token);
}
// 점멸 setter는 상태 쓰기만 한다.
void ButtonGump::SetBlink(std::uint32_t token,std::uint32_t value) const { Node(token).blink=value; }
// 원본은 pulse bit를 먼저 바꾸고 일반 idle 등록/해제를 수행한다.
void ButtonGump::SetPulse(std::uint32_t token,bool enabled) const {
    auto& node=Node(token);node.buttonFlags=enabled ? node.buttonFlags|kPulse : node.buttonFlags&~kPulse;
    lifetime_.SetIdle(token,enabled);
}
// 각 ordered 비교는 NaN을 거부하고 음의 0을 원점으로 취급한다.
bool ButtonGump::Contains(std::uint32_t token,std::array<float,2> local) const {
    const auto& node=Node(token);return local[0]>=0 && local[0]<node.width && local[1]>=0 && local[1]<node.height;
}
// 파생 value 가상 함수는 선택적으로 재정의할 수 있다.
void ButtonGump::Value(std::uint32_t token,std::uint32_t value) const {
    if (hooks_.value) hooks_.value(token,value);else SetValue(token,value);
}
// 파생 pressed 가상 함수도 기본/돌 버튼 쓰기로 대체할 수 있다.
void ButtonGump::Pressed(std::uint32_t token,std::uint32_t value) const {
    if (hooks_.pressed) hooks_.pressed(token,value);else SetPressed(token,value);
}
// pulse의 가상 호출 인자는 전체 DWORD이며 기본 구현만 bool로 해석한다.
void ButtonGump::Pulse(std::uint32_t token,std::uint32_t value) const {
    if (hooks_.pulse) hooks_.pulse(token,value);else SetPulse(token,value!=0);
}
// 점멸 가상 setter도 별도 redraw를 삽입하지 않는다.
void ButtonGump::Blink(std::uint32_t token,std::uint32_t value) const {
    if (hooks_.blink) hooks_.blink(token,value);else SetBlink(token,value);
}
// 현재 serial(+2c)과 전달 값으로 실제 부모 전파를 호출한다. 부모의 반환값은 원본처럼 버린다.
void ButtonGump::Notify(std::uint32_t token,std::uint32_t event,std::uint32_t value) const {
    events_.Bubble(token,event,Node(token).serial,value);
}
// 두 예약은 각각 다른 벽 시계 조회를 사용한다.
void ButtonGump::ResetDeadlines(std::uint32_t token) const {
    Node(token).firstDeadline=Now()+kFirstDelay;Node(token).repeatDeadline=Now()+kRepeatDelay;
}
// 현재 루트 직계 팝업만 삭제 요청한다. next를 Kill 전에 저장하고 visible/void는 검사하지 않는다.
void ButtonGump::ClosePopups() const {
    auto token=BaseNode(create_.entries[0]).child;std::unordered_set<std::uint32_t> seen;
    // 삭제 훅이 링크를 바꿔도 호출 전의 next로 진행한다. 실제 해제는 GumpLifetime의 후속 Cleanup이다.
    while (token) {
        if (seen.size()>=kGumpRegistryCapacity || !seen.insert(token).second) throw std::runtime_error("버튼 팝업 형제 순환/한도 초과");
        const auto& node=BaseNode(token);const auto next=node.next;
        if ((node.flags&kPopup) && !(node.flags&kDead)) lifetime_.Kill(token);
        token=next;
    }
}
// 값 명령을 처리한 뒤에도 사건 분기를 계속한다. 원본은 그 명령만으로 성공 1을 반환하지 않는다.
std::uint32_t ButtonGump::Handle(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) const {
    auto& node=Node(token);if (node.buttonFlags&kDisabled) return 0;
    const auto code=event&kEventMask;const bool release=bool(event&InputCode::kRelease);
    if (code==state_.valueEvent && first==node.serial) {
        Value(token,second);Redraw(token);Notify(token,state_.clickEvent,node.value);
    }
    if (!node.alternateTextPointer && code==InputCode::kRightButton && !release) {
        if (!(node.buttonFlags&kKeepPopups)) ClosePopups();
        Notify(token,state_.rightEvent,node.value);return 1;
    }
    if (code==InputCode::kLeftButton && !release) {
        if (!(node.buttonFlags&kKeepPopups)) ClosePopups();
        if (!(node.buttonFlags&kInstant)) {
            ResetDeadlines(token);input_.Capture(token);node.flags|=kNotify;lifetime_.SetIdle(token,true);Pressed(token,1);
        } else if (node.buttonFlags&kToggle) {
            Value(token,node.value==0 ? 1U : 0U);Notify(token,state_.clickEvent,node.value);
        } else if (!node.value) {
            Value(token,1);Notify(token,state_.clickEvent,node.value);
        }
        if (!hooks_.sound) throw std::invalid_argument("button.wav 재생 경계 필요");
        hooks_.sound();Redraw(token);return 1;
    }
    if (code==InputCode::kLeftButton && release && !(node.buttonFlags&kInstant)) {
        const auto local=collection_.ScreenToLocal(token,{std::bit_cast<std::int32_t>(first),std::bit_cast<std::int32_t>(second)});
        if (!Contains(token,local)) {
            node.flags&=~kNotify;lifetime_.SetIdle(token,false);Pressed(token,0);return 1;
        }
        if (node.buttonFlags&kToggle) {
            Value(token,node.value==0 ? 1U : 0U);Notify(token,state_.clickEvent,node.value);
        } else if (node.pressed) Notify(token,state_.clickEvent,node.value);
        node.flags&=~kNotify;lifetime_.SetIdle(token,false);Pressed(token,0);Redraw(token);return 1;
    }
    if (!hooks_.baseEvent) throw std::invalid_argument("기본 Gump 도움말 처리 경계 필요");
    return hooks_.baseEvent(token,event,first,second);
}
// 왼쪽 버튼의 폴링은 전달된 사건/좌표와 별개다. 비활성 플래그도 이 함수의 필터가 아니다.
void ButtonGump::Broadcast(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) const {
    auto& node=Node(token);if (node.buttonFlags&kInstant) throw std::runtime_error("즉시 버튼은 원본 통지 대상이 아님");
    if (!hooks_.poll) throw std::invalid_argument("버튼 폴링 경계 필요");
    if (hooks_.poll(InputCode::kLeftButton).code&InputCode::kRelease) node.flags&=~kNotify;
    (void)event;const auto local=collection_.ScreenToLocal(token,{std::bit_cast<std::int32_t>(first),std::bit_cast<std::int32_t>(second)});
    const bool inside=Contains(token,local);
    const auto value=node.buttonFlags&kToggle ? (inside ? std::uint32_t(node.value==0) : node.value) : std::uint32_t(inside);
    if (value!=node.pressed) { Pressed(token,value);ResetDeadlines(token);Redraw(token); }
}
// 첫 시각 만료 이후 현재 두 번째 시각까지 만료한 때에만 한 번 통지하며 지연 프레임을 반복 보충하지 않는다.
void ButtonGump::Idle(std::uint32_t token) const {
    auto& node=Node(token);
    if (node.buttonFlags&kPulse) {
        if (node.value || node.pressed) Pulse(token,0);
        else {
            const auto now=Now();
            if (node.firstDeadline<=now) { Blink(token,node.blink==0 ? 1U : 0U);node.firstDeadline=Now()+kBlinkDelay; }
        }
    }
    if (!(node.buttonFlags&kInstant) && node.pressed) {
        const auto firstNow=Now();
        if (node.firstDeadline<=firstNow) {
            const auto repeatNow=Now();
            if (node.repeatDeadline<=repeatNow) { Notify(token,state_.repeatEvent,1);node.repeatDeadline=Now()+kRepeatDelay; }
        }
    }
}
// 기존 출력 계획은 현재 value/pressed를 signed DWORD로 받아 같은 눌림 OR를 계산한다.
ButtonVisualState ButtonGump::VisualState(std::uint32_t token,std::int32_t textWidth,std::int32_t textHeight) const {
    const auto rect=collection_.Bounds(token);const auto& node=Node(token);
    return {rect,textWidth,textHeight,std::bit_cast<std::int32_t>(node.value),std::bit_cast<std::int32_t>(node.pressed),node.buttonFlags};
}
// 현재 버튼 여부를 호출 때 결정한다. 버튼 외 대상의 기존 가상 처리기가 없으면 도달한 시점에 거부한다.
void BindButtonGump(GumpEventHooks& events,GumpInputHooks& input,GumpFrameHooks& frame,
    std::function<const ButtonGump*(std::uint32_t)> resolve) {
    if (!resolve) throw std::invalid_argument("버튼 가상 분배 조회 필요");
    // 선택·캡처 전달의 기존 처리기를 각각 보존한다. 두 계층이 서로 다른 fallback을 가질 수 있다.
    const auto bindHandle=[resolve](auto previous) {
        // 토큰을 복원 버튼으로 판정한 경우 실제 원본 이벤트 처리기를 사용한다.
        return [resolve,previous](std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second)->std::uint32_t {
            if (const auto* button=resolve(token)) return button->Handle(token,event,first,second);
            if (!previous) throw std::invalid_argument("버튼 외 Gump 입력 처리기 필요");return previous(token,event,first,second);
        };
    };
    events.handle=bindHandle(events.handle);input.handle=bindHandle(input.handle);
    const auto idle=frame.idle;const auto broadcast=frame.broadcast;
    // 프레임 idle 표의 중복/변경 처리 순서는 GumpFrame에 맡기고 개별 가상 몸체만 분배한다.
    frame.idle=[resolve,idle](std::uint32_t token) {
        if (const auto* button=resolve(token)) button->Idle(token);
        else { if (!idle) throw std::invalid_argument("버튼 외 Gump idle 처리기 필요");idle(token); }
    };
    // 등록표 통지의 필터·순서는 GumpFrame에 맡기며 현재 버튼 몸체에는 전체 DWORD를 전달한다.
    frame.broadcast=[resolve,broadcast](std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) {
        if (const auto* button=resolve(token)) button->Broadcast(token,event,first,second);
        else { if (!broadcast) throw std::invalid_argument("버튼 외 Gump 통지 처리기 필요");broadcast(token,event,first,second); }
    };
}
}
