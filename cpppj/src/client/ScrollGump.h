// 원본 Slider의 값/범위/페이지·자식 부착과 손잡이 선택/드래그/부모 통지를 같은 Gump 저장소에 연결한다.
#pragma once
#include "client/GumpTree.h"
#include "client/GumpInput.h"
#include "client/GumpFrame.h"
#include "client/InputEvent.h"
#include "client/GumpBackgroundCreate.h"

namespace netstorm::client {
struct ScrollingTextGumpHooks;
// 원본 +a0 min/+a4 max/+a8 position/+ac page/+b4 방향/+b8 step/+c8..d0 자식 토큰이다.
// 기본 생성과 화살표/손잡이 할당은 ScrollGumpWidgets가 수행한다. 별도 초기화 시에도 같은 저장소를 사용한다.
struct ScrollGumpNode: GumpCreateNode {
    // minimum: 스크롤 가능한 최소 위치.
    // maximum: 스크롤 가능한 최대 위치.
    // position: 현재 좌표 또는 저장소의 읽기 위치.
    // page: 표시할 페이지 또는 스크롤 페이지 크기.
    // step: 단계.
    std::int32_t minimum{},maximum{},position{},page{},step{};
    // scrollFlags: 스크롤바 방향·입력 동작을 선택하는 플래그.
    // decrement: 위치 감소 버튼의 Gump 토큰.
    // increment: 위치 증가 버튼의 Gump 토큰.
    // thumb: 스크롤 손잡이의 Gump 토큰.
    std::uint32_t scrollFlags{},decrement{},increment{},thumb{};
    // +b0은 생성 시 0, +bc는 미작성, +c0/+c4는 원본 생성자가 0으로 쓰는 보조 필드다.
    std::uint32_t reservedB0{},reservedBC{};
    // reserved: 예약영역.
    std::array<std::uint32_t,2> reserved{};
};
// 손잡이 +c0..cc의 양 끝 포함 사각형과 +d0 이전 y/+d4 이전 x다. +d4는 패치에만 존재한다.
// 기본 배경 필드/트리/캐시를 같은 객체에서 공유하며 생성자와 실제 출력은 ScrollGumpWidgets가 맡는다.
struct ScrollGumpThumbNode: GumpBackgroundNode {
    // +b8/+bc의 생성 인자 사본이다. 원본 손잡이 Draw는 이 값 대신 기본 배경의 type/frame을 사용한다.
    std::uint32_t detailType{},detailFrame{};
    // handle: 네이티브 자원이나 등록 대상을 찾는 핸들.
    ScreenRect handle{};
    // previousY: 이전 Y.
    // previousX: 이전 X.
    std::int32_t previousY{},previousX{};
};
// 초기 문자열 ID의 현재 전역이다. click/repeat는 ButtonGumpState와 같은 실제 ID를 공급한다.
// 패치 click/repeat/step/change: 005427a8/005427b4/005427b0/005427d0, CD: 00512930/3c/38/58.
struct ScrollGumpState { /* clickEvent: 왼쪽 클릭 동작의 이벤트 ID. */ /* repeatEvent: 누름 유지로 반복 발생하는 이벤트 ID. */ /* stepEvent: 스크롤 이동을 요청하는 이벤트 ID. */ /* changeEvent: 값이 변경됐음을 알리는 이벤트 ID. */ std::uint32_t clickEvent{},repeatEvent{},stepEvent{},changeEvent{}; };
// 동일 기본/파생 노드 조회와 자식 가상 Push/Attach·현재 가상 Redraw·왼쪽 버튼 폴링 경계다.
// node/baseNode/thumbNode는 안정된 같은 저장소를 공급하며 콜백 뒤에는 현재 토큰/필드를 다시 읽는다.
struct ScrollGumpHooks {
    // node: 토큰에 대응하는 저장 노드를 조회하는 콜백.
    std::function<ScrollGumpNode*(std::uint32_t)> node;
    // thumbNode: 스크롤손잡이 노드를 조회하는 콜백.
    std::function<ScrollGumpThumbNode*(std::uint32_t)> thumbNode;
    // baseNode: 파생 노드와 공유하는 기본 Gump 노드를 조회하는 콜백.
    std::function<GumpCreateNode*(std::uint32_t)> baseNode;
    // push: 현재 대상을 처리 목록에 넣는 콜백.
    std::function<void(std::uint32_t)> push;
    // attach: 대상을 부모 또는 처리 목록에 부착하는 콜백.
    std::function<void(std::uint32_t,float,float,std::uint32_t,std::uint32_t)> attach;
    // redraw: 대상의 화면 갱신을 요청하는 콜백.
    std::function<void(std::uint32_t,bool)> redraw;
    // poll: 요청한 키·마우스 버튼의 현재 입력을 폴링하는 콜백.
    std::function<InputEvent(std::uint32_t)> poll;
};
// ScrollGump.cpp의 Slider 값/부착/입력 처리기다. 생성/파생 Draw는 ScrollGumpWidgets에 분리한다.
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
    // create_: 등록 표 +0이 실제 루트다. 외부에서 표를 바꿀 때 collection.root도 같은 토큰으로 맞춰야 한다 (GumpCreateState).
    GumpCreateState& create_;
    // state_: 초기 문자열 ID의 현재 전역이다. click/repeat는 ButtonGumpState와 같은 실제 ID를 공급한다 (ScrollGumpState).
    ScrollGumpState& state_;
    // tree_: 00466750/CD 00493070의 머리 삽입과 원본의 후속 정렬/현재 자식 재배치를 담당한다 (GumpTree).
    const GumpTree& tree_;
    // collection_: 화면 밖 자식 통과, 최상위 삭제 필터, 원본의 루트별 제한 개수를 보존하는 UI 수집기다 (GumpCollection).
    const GumpCollection& collection_;
    // events_: 발생한 이벤트 또는 이벤트 처리기.
    const GumpEvents& events_;
    // input_: 처리할 입력 또는 입력 분배기.
    const GumpInput& input_;
    // hooks_: 외부 저장소 조회와 가상 동작을 연결하는 콜백 묶음 (ScrollGumpHooks).
    ScrollGumpHooks hooks_;
};
// Slider 가상 Attach/Handle과 손잡이 Handle/Broadcast를 같은 트리/입력/프레임에 연결한다.
// 각 resolve는 해당 파생 토큰일 때만 처리기를 반환하며 다른 기존 분배기는 보존한다.
void BindScrollGump(GumpTreeHooks& tree,GumpEventHooks& events,GumpInputHooks& input,GumpFrameHooks& frame,
    std::function<const ScrollGump*(std::uint32_t)> slider,std::function<const ScrollGump*(std::uint32_t)> thumb);
// 기존 다중 줄 TextGump의 네 Slider 경계를 실제 범위/페이지/위치/현재 값 구현에 연결한다.
void BindScrollingTextSlider(ScrollingTextGumpHooks& hooks,const ScrollGump& slider);
}
