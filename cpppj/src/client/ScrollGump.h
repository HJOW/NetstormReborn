// 원본 Slider의 값/범위/페이지·자식 부착과 손잡이 선택/드래그/부모 통지를 같은 Gump 저장소에 연결한다.
#pragma once
#include "client/GumpTree.h"
#include "client/GumpInput.h"
#include "client/GumpFrame.h"
#include "client/InputEvent.h"

namespace netstorm::client {
struct ScrollingTextGumpHooks;
// 원본 +a0 min/+a4 max/+a8 position/+ac page/+b4 방향/+b8 step/+c8..d0 자식 토큰이다.
// 기본 생성과 화살표/손잡이 할당은 원래 생성 계층이 먼저 수행한다. 이 저장소는 생성자를 대신하지 않는다.
struct ScrollGumpNode: GumpCreateNode {
    std::int32_t minimum{},maximum{},position{},page{},step{};
    std::uint32_t scrollFlags{},decrement{},increment{},thumb{};
};
// 손잡이 +c0..cc의 양 끝 포함 사각형과 +d0 이전 y/+d4 이전 x다. +d4는 패치에만 존재한다.
// 실제 FrameGump 기본 필드/배경·화살표 픽셀 출력은 별도 복원 대상이다.
struct ScrollGumpThumbNode: GumpCreateNode {
    ScreenRect handle{};
    std::int32_t previousY{},previousX{};
};
// 초기 문자열 ID의 현재 전역이다. click/repeat는 ButtonGumpState와 같은 실제 ID를 공급한다.
// 패치 click/repeat/step/change: 005427a8/005427b4/005427b0/005427d0, CD: 00512930/3c/38/58.
struct ScrollGumpState { std::uint32_t clickEvent{},repeatEvent{},stepEvent{},changeEvent{}; };
// 동일 기본/파생 노드 조회와 자식 가상 Push/Attach·현재 가상 Redraw·왼쪽 버튼 폴링 경계다.
// node/baseNode/thumbNode는 안정된 같은 저장소를 공급하며 콜백 뒤에는 현재 토큰/필드를 다시 읽는다.
struct ScrollGumpHooks {
    std::function<ScrollGumpNode*(std::uint32_t)> node;
    std::function<ScrollGumpThumbNode*(std::uint32_t)> thumbNode;
    std::function<GumpCreateNode*(std::uint32_t)> baseNode;
    std::function<void(std::uint32_t)> push;
    std::function<void(std::uint32_t,float,float,std::uint32_t,std::uint32_t)> attach;
    std::function<void(std::uint32_t,bool)> redraw;
    std::function<InputEvent(std::uint32_t)> poll;
};
// ScrollGump.cpp의 Slider 값/부착/입력 처리기다. 전체 생성·화살표 파생 Draw는 후속이다.
class ScrollGump {
public:
    // 생성/방향/사건 전역과 실제 트리/좌표/부모 통지/캡처가 이 처리기보다 오래 살아야 한다.
    ScrollGump(GumpCreateState& create,ScrollGumpState& state,const GumpTree& tree,
        const GumpCollection& collection,const GumpEvents& events,const GumpInput& input,ScrollGumpHooks hooks);
    // 004a6050/CD 00418700: page 계약→최솟값/최댓값-page 제한→position 저장→Push/Attach 또는 Redraw(false).
    void SetPosition(std::uint32_t token,std::int32_t position) const;
    // 004a6320/CD 004186d0: page를 signed 범위 길이 이하로 제한한 뒤 현재 position을 다시 적용한다.
    // 음수 page는 원본처럼 하한 제한하지 않는다.
    void SetPage(std::uint32_t token,std::int32_t page) const;
    // 004a6350/CD 004187a0: 두 끝점 정렬·동률 최대+1·page 제한 뒤 현재 position을 두 번 다시 적용한다.
    void SetRange(std::uint32_t token,std::int32_t first,std::int32_t last) const;
    // 현재 +a8을 그대로 반환한다. TextGump의 마지막 slider 재조회가 같은 저장소를 읽게 한다.
    std::int32_t Position(std::uint32_t token) const;
    // 004a5cd0/CD 00418170: 기본 부착→세 자식 Push→크기/위치→현재 값/페이지의 손잡이 사각형 계산이다.
    // 방향 bit 1은 가로다. 손잡이 위치 분자는 position-minimum이 아닌 원본의 position이다.
    void Attach(std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode) const;
    // 004a6170/CD 004184e0: 세로에서만 DWORD 곱/CRT 절삭으로 y delta를 position으로 바꾼다.
    void DragVertical(std::uint32_t token,std::int32_t delta) const;
    // 004a61c0/CD 00418550: 화살표 click/repeat·손잡이 step을 부모 체인에 통지하고 현재 값 변경을 전달한다.
    std::uint32_t Handle(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) const;
    // 004a5ae0/CD 004179c0: 왼쪽 누름에서 손잡이 포함 선택/캡처 또는 page 방향 3/4를 부모에 통지한다.
    // 패치의 가로 캡처와 CD의 가로 무동작, 두 판본의 세로 release만 플래그 해제를 유지한다.
    std::uint32_t ThumbHandle(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) const;
    // 004a6430/CD 00417ad0: 폴링 release는 플래그만 해제하며 그 밖에는 전달 y delta로 부모 Slider를 갱신한다.
    void ThumbBroadcast(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) const;
private:
    // 기본/파생 저장소가 같은 주소인지 도달 시 확인한다. 미등록/null 토큰은 호스트 예외다.
    ScrollGumpNode& Node(std::uint32_t token) const;
    ScrollGumpThumbNode& Thumb(std::uint32_t token) const;
    GumpCreateNode& Base(std::uint32_t token) const;
    // 현재 자식 토큰의 가상 +18/+20을 호출한다. 기본 부모 부착은 tree_.Attach로 직접 위임한다.
    void Push(std::uint32_t token) const;
    void AttachChild(std::uint32_t token,float x,float y,std::uint32_t parent) const;
    // 원본 float 저장 전 값을 현재 child scale과 곱한다. 폭/높이 중 지정 축만 변경한다.
    void SetSize(std::uint32_t token,double size,bool width) const;
    // 변화 통지는 현재 사건 전역/serial을 부모 체인에 보낸다. 값은 항상 0이다.
    void NotifyChange(std::uint32_t token) const;
    GumpCreateState& create_;
    ScrollGumpState& state_;
    const GumpTree& tree_;
    const GumpCollection& collection_;
    const GumpEvents& events_;
    const GumpInput& input_;
    ScrollGumpHooks hooks_;
};
// Slider 가상 Attach/Handle과 손잡이 Handle/Broadcast를 같은 트리/입력/프레임에 연결한다.
// 각 resolve는 해당 파생 토큰일 때만 처리기를 반환하며 다른 기존 분배기는 보존한다.
void BindScrollGump(GumpTreeHooks& tree,GumpEventHooks& events,GumpInputHooks& input,GumpFrameHooks& frame,
    std::function<const ScrollGump*(std::uint32_t)> slider,std::function<const ScrollGump*(std::uint32_t)> thumb);
// 기존 다중 줄 TextGump의 네 Slider 경계를 실제 범위/페이지/위치/현재 값 구현에 연결한다.
void BindScrollingTextSlider(ScrollingTextGumpHooks& hooks,const ScrollGump& slider);
}
