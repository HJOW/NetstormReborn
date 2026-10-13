// 원본 ButtonGump의 상태·입력·통지·idle를 같은 Gump 생성/좌표/입력/수명 저장소에 연결한다.
#pragma once
#include "client/GumpInput.h"
#include "client/GumpFrame.h"
#include "client/GumpVisual.h"
#include "client/InputEvent.h"

namespace netstorm::client {
// 기본 버튼과 돌 버튼의 값/누름 가상 함수 구현을 선택한다. 다른 파생 클래스는 훅으로 공급한다.
enum class ButtonGumpKind { Base,Stone };
// 기본 0xa0 필드는 기존 저장소와 공유한다. 아래 필드는 +a0/+a4/+a8/+ac/+b0/+b8/+100에 대응한다.
// kind는 호스트의 가상 분배 정보이며 원본 객체에 추가되는 필드가 아니다.
struct ButtonGumpNode : GumpCreateNode {
    // value: 토글·값 변경과 부모 통지에 사용하는 버튼의 DWORD 값.
    // pressed: 마우스 폴링과 가상 누름 함수가 갱신하는 DWORD 누름 값.
    // blink: Idle에서 버튼 점멸을 판정할 DWORD 상태.
    // buttonFlags: 버튼 입력·토글·반복 동작을 선택하는 비트.
    std::uint32_t value{},pressed{},blink{},buttonFlags{};
    // firstDeadline: 버튼을 누른 뒤 첫 반복 통지가 허용되는 벽 시계 시각(초).
    // repeatDeadline: 다음 반복 버튼 통지가 허용되는 벽 시계 시각(초).
    double firstDeadline{},repeatDeadline{};
    // background: 누름 표시를 갱신할 배경 Gump의 토큰.
    std::uint32_t background{};
    // kind: 기본 버튼과 돌 버튼의 가상 동작을 선택할 호스트 분류.
    ButtonGumpKind kind{ButtonGumpKind::Base};
};
// 실행 중 문자열 ID 전역 네 개와 기본 버튼 색 BYTE다. 사건 ID는 호출자가 실제 초기화 값을 공급한다.
// 값 사건 대상/부모 통지의 식별자는 serial(+2c)이다. command(+20)는 기본 버튼 색을 보관한다.
// 패치 전역: 005428b8/005427a8/00542740/005427b4/00540453. CD: 00512a40/00512930/005128c8/0051293c/00535f88.
struct ButtonGumpState {
    // valueEvent: 버튼 값 변경을 요청하는 이벤트 ID.
    // clickEvent: 왼쪽 클릭 동작의 이벤트 ID.
    // rightEvent: 우클릭 동작의 이벤트 ID.
    // repeatEvent: 누름 유지로 반복 발생하는 이벤트 ID.
    std::uint32_t valueEvent{},clickEvent{},rightEvent{},repeatEvent{};
    // baseColor: 기본 UI 출력의 팔레트 색 인덱스.
    std::int8_t baseColor{};
};
// 노드는 호출 중 안정된 주소로 살아 있어야 한다. backgroundFlags는 실제 배경 +b4 저장소를 반환한다.
// now는 원본의 벽 시계 초, poll은 원본 왼쪽 버튼 폴링이다. redraw는 가상 +8(false), sound는 button.wav다.
// baseEvent는 GumpHelp로 연결할 수 있는 기본 도움말 처리기다. 도달한 경계가 없으면 예외로 거부한다.
// 네 선택적 가상 훅이 없으면 이 클래스의 실제 기본/돌 버튼 상태 함수를 사용한다.
struct ButtonGumpHooks {
    // node: 토큰에 대응하는 저장 노드를 조회하는 콜백.
    std::function<ButtonGumpNode*(std::uint32_t)> node;
    // baseNode: 파생 노드와 공유하는 기본 Gump 노드를 조회하는 콜백.
    std::function<GumpCreateNode*(std::uint32_t)> baseNode;
    // backgroundFlags: 배경 Gump의 현재 플래그 저장 주소를 조회하는 콜백.
    std::function<std::uint32_t*(std::uint32_t)> backgroundFlags;
    // now: 현재 시각을 초 단위로 공급하는 콜백.
    std::function<double()> now;
    // poll: 요청한 키·마우스 버튼의 현재 입력을 폴링하는 콜백.
    std::function<InputEvent(std::uint32_t)> poll;
    // redraw: 대상의 화면 갱신을 요청하는 콜백.
    std::function<void(std::uint32_t,bool)> redraw;
    // sound: 해당 동작의 소리를 재생하는 콜백.
    std::function<void()> sound;
    // baseEvent: 기본 Gump에 이벤트를 전달하는 콜백.
    std::function<std::uint32_t(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> baseEvent;
    // value: 버튼 값의 가상 변경 함수를 호출하는 콜백.
    // pressed: 버튼 누름 상태의 가상 변경 함수를 호출하는 콜백.
    // pulse: 버튼 pulse 상태의 가상 변경 함수를 호출하는 콜백.
    // blink: 버튼 점멸 상태의 가상 변경 함수를 호출하는 콜백.
    std::function<void(std::uint32_t,std::uint32_t)> value,pressed,pulse,blink;
};
// 돌 버튼 생성/텍스트/부착은 StoneButtonGump에 맡기는 상태 처리기다. GumpEvents/입력/프레임 훅에 연결한다.
class ButtonGump {
public:
    // 같은 생성·수명·입력·좌표 상태가 이 처리기보다 오래 살아야 한다. 버튼/기본 노드 조회는 필수다.
    ButtonGump(GumpCreateState& create,ButtonGumpState& state,const GumpCreate& factory,
        const GumpCollection& collection,const GumpEvents& events,const GumpInput& input,
        const GumpLifetime& lifetime,ButtonGumpHooks hooks);
    // 004253e0/CD 00440bb0: 기본 생성/등록 후 버튼 크기·플래그·상태·두 예약 시각을 준비한다.
    // 돌 버튼 전체 생성은 StoneButtonGump::Construct가 이 함수에 위임한다. 이력: 2026-10-12 기본 파생 생성 복원.
    std::uint32_t ConstructBase(std::uint32_t token,std::int32_t width,std::int32_t height,std::uint32_t flags) const;
    // 00424960/00424980과 돌 버튼 00425150/00425200: DWORD 값을 보존하고 redraw(false)를 호출한다.
    // 돌 버튼의 두 상태 OR가 바뀌면 현재 배경 kPressed를 갱신하고 배경에도 redraw(false)를 요청한다.
    void SetValue(std::uint32_t token,std::uint32_t value) const;
    void SetPressed(std::uint32_t token,std::uint32_t value) const;
    // 00425470/004254a0: 비활성 비트(값 1)를 반전하거나 점멸 DWORD를 지정한다. 점멸 지정은 redraw하지 않는다.
    void SetEnabled(std::uint32_t token,bool enabled) const;
    void SetBlink(std::uint32_t token,std::uint32_t value) const;
    // 00424c90: buttonFlags bit 0x10과 실제 idle 등록을 같이 바꾼다. 누름 상태는 별도로 유지한다.
    void SetPulse(std::uint32_t token,bool enabled) const;
    // 004248e0/CD 인라인: 로컬 float 경계 [0,width)×[0,height). NaN/음수는 제외하며 -0은 허용한다.
    bool Contains(std::uint32_t token,std::array<float,2> local) const;
    // 004249a0/CD 00440ce0: 일반/토글/즉시·우클릭·값 명령을 처리하고 실제 부모 체인에 통지한다.
    std::uint32_t Handle(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) const;
    // 00424dd0/CD 00441170: 폴링 release로 통지 플래그를 지우고 전달 좌표로 눌림 상태를 갱신한다.
    void Broadcast(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) const;
    // 00424cd0/CD 00441050: 점멸/비누름 pulse 해제와 0.4초 이후 0.05초 반복 부모 통지를 처리한다.
    void Idle(std::uint32_t token) const;
    // 현재 상태와 실제 트리 경계로 기존 돌 버튼 출력 계획의 입력을 만든다. 값을 bool로 좁히지 않는다.
    ButtonVisualState VisualState(std::uint32_t token,std::int32_t textWidth,std::int32_t textHeight) const;
private:
    // 도달한 토큰/경계만 검사하며 콜백의 쓰기는 예외가 나도 되돌리지 않는다.
    ButtonGumpNode& Node(std::uint32_t token) const;
    GumpCreateNode& BaseNode(std::uint32_t token) const;
    double Now() const;
    void Redraw(std::uint32_t token) const;
    // 선택적 파생 가상 훅이 없으면 위 공개 상태 함수를 호출한다.
    void Value(std::uint32_t token,std::uint32_t value) const;
    void Pressed(std::uint32_t token,std::uint32_t value) const;
    void Pulse(std::uint32_t token,std::uint32_t value) const;
    void Blink(std::uint32_t token,std::uint32_t value) const;
    // 상태 쓰기/부모 통지/최상위 팝업 Kill의 원본 순서와 현재 필드 재조회를 유지한다.
    void WriteState(std::uint32_t token,std::uint32_t value,bool pressed) const;
    void Notify(std::uint32_t token,std::uint32_t event,std::uint32_t value) const;
    void ResetDeadlines(std::uint32_t token) const;
    void ClosePopups() const;
    // create_: 등록 표 +0이 실제 루트다. 외부에서 표를 바꿀 때 collection.root도 같은 토큰으로 맞춰야 한다 (GumpCreateState).
    GumpCreateState& create_;
    // state_: 실행 중 문자열 ID 전역 네 개와 기본 버튼 색 BYTE다. 사건 ID는 호출자가 실제 초기화 값을 공급한다 (ButtonGumpState).
    ButtonGumpState& state_;
    // factory_: 타입에 맞는 객체를 생성하는 처리기.
    const GumpCreate& factory_;
    // collection_: 화면 밖 자식 통과, 최상위 삭제 필터, 원본의 루트별 제한 개수를 보존하는 UI 수집기다 (GumpCollection).
    const GumpCollection& collection_;
    // events_: 발생한 이벤트 또는 이벤트 처리기.
    const GumpEvents& events_;
    // input_: 처리할 입력 또는 입력 분배기.
    const GumpInput& input_;
    // lifetime_: 등록된 객체의 삭제·해제를 처리하는 수명 관리자.
    const GumpLifetime& lifetime_;
    // hooks_: 외부 저장소 조회와 가상 동작을 연결하는 콜백 묶음 (ButtonGumpHooks).
    ButtonGumpHooks hooks_;
};
// 버튼 토큰이면 실제 Handle/Idle/Broadcast를 호출하고 나머지는 기존 가상 분배를 유지한다.
// resolve는 호출 때 현재 분배기를 반환한다. 순환 의존 객체를 만든 뒤 포인터를 연결할 때도 사용할 수 있다.
void BindButtonGump(GumpEventHooks& events,GumpInputHooks& input,GumpFrameHooks& frame,
    std::function<const ButtonGump*(std::uint32_t)> resolve);
}
