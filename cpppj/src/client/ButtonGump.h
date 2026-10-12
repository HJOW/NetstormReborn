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
    std::uint32_t value{},pressed{},blink{},buttonFlags{};
    double firstDeadline{},repeatDeadline{};
    std::uint32_t background{};
    ButtonGumpKind kind{ButtonGumpKind::Base};
};
// 실행 중 문자열 ID 전역 네 개와 기본 버튼 색 BYTE다. 사건 ID는 호출자가 실제 초기화 값을 공급한다.
// 값 사건 대상/부모 통지의 식별자는 serial(+2c)이다. command(+20)는 기본 버튼 색을 보관한다.
// 패치 전역: 005428b8/005427a8/00542740/005427b4/00540453. CD: 00512a40/00512930/005128c8/0051293c/00535f88.
struct ButtonGumpState {
    std::uint32_t valueEvent{},clickEvent{},rightEvent{},repeatEvent{};
    std::int8_t baseColor{};
};
// 노드는 호출 중 안정된 주소로 살아 있어야 한다. backgroundFlags는 실제 배경 +b4 저장소를 반환한다.
// now는 원본의 벽 시계 초, poll은 원본 왼쪽 버튼 폴링이다. redraw는 가상 +8(false), sound는 button.wav다.
// baseEvent는 GumpHelp로 연결할 수 있는 기본 도움말 처리기다. 도달한 경계가 없으면 예외로 거부한다.
// 네 선택적 가상 훅이 없으면 이 클래스의 실제 기본/돌 버튼 상태 함수를 사용한다.
struct ButtonGumpHooks {
    std::function<ButtonGumpNode*(std::uint32_t)> node;
    std::function<GumpCreateNode*(std::uint32_t)> baseNode;
    std::function<std::uint32_t*(std::uint32_t)> backgroundFlags;
    std::function<double()> now;
    std::function<InputEvent(std::uint32_t)> poll;
    std::function<void(std::uint32_t,bool)> redraw;
    std::function<void()> sound;
    std::function<std::uint32_t(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> baseEvent;
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
    GumpCreateState& create_;
    ButtonGumpState& state_;
    const GumpCreate& factory_;
    const GumpCollection& collection_;
    const GumpEvents& events_;
    const GumpInput& input_;
    const GumpLifetime& lifetime_;
    ButtonGumpHooks hooks_;
};
// 버튼 토큰이면 실제 Handle/Idle/Broadcast를 호출하고 나머지는 기존 가상 분배를 유지한다.
// resolve는 호출 때 현재 분배기를 반환한다. 순환 의존 객체를 만든 뒤 포인터를 연결할 때도 사용할 수 있다.
void BindButtonGump(GumpEventHooks& events,GumpInputHooks& input,GumpFrameHooks& frame,
    std::function<const ButtonGump*(std::uint32_t)> resolve);
}
