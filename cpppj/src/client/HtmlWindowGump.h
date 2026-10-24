// HTML 부모4cf120의 실제8192BYTE/20행과 원문 생성·본문·버튼·입력·소멸을 유지한다.
#pragma once
#include "client/GumpDecoration.h"
#include "client/ButtonGump.h"
#include <string_view>
namespace netstorm::client { // 원문 HTML 부모와 같은 기본/파생 저장소의 계약이다.
struct HtmlWindowControl { std::uint32_t child,command,parameter; }; //20c4부터20개 행의 실제 세DWORD다.
struct HtmlWindowGumpNode: GumpDecorationNode { // 원문0x21e8이며 호스트에서도 기본 주소를 공유한다.
    std::array<char,8192> html;std::array<HtmlWindowControl,20> controls; //c4의 원시 HTML와20c4의 실제 제어 행이다. 생성자는 html[0]만 쓴다.
    std::uint32_t controlCount,body,updateDepth,requestedWidth,requestedHeight,menuColumns,auxiliarySerial; //21b4..21cc의 현재 실제 필드다.
    double timeoutSeconds;std::uint32_t timeoutCommand,timeoutParameter,onExitCommand,onExitParameter; //21d0..21e4이며 생성자가 두 인자 DWORD를 덮지 않는다.
};
struct HtmlWindowGumpState { // Client 수명의 같은 원문 전역이다.
    std::uint32_t instances{},createdSerial{},transientSerial{};double lastCloseTime{}; //5cab2c/38/34와5cad30/34다.
    std::int8_t kind{27}; //540436의 실제 signed 종류BYTE다.
    std::uint32_t enterEvent{0x02000038},clickEvent{0x0200002c},tagEvent{0x02000025},closedEvent{0x020000be}; //5427d8/a8/8c와5429ec의 실제 초기값이다.
};
struct HtmlWindowGumpHooks { // 같은 공통 노드/생성/수명과 선택된 장치 경계만 공급한다.
    std::function<HtmlWindowGumpNode*(std::uint32_t)> node;std::function<GumpCreateNode*(std::uint32_t)> base;std::function<ButtonGumpNode*(std::uint32_t)> button; // 부모/자식의 현재 실제 주소다.
    std::function<void(std::uint32_t)> constructDecoration,destructBase,push,kill; //465d10(this,0,0)/464420/464560/466480의 실제 공통 몸체다.
    std::function<std::uint32_t(std::size_t)> allocate; // 원문170 돌 버튼 또는d8 스크롤 본문을 실제 할당한다.
    std::function<void(std::uint32_t,std::string_view)> constructButton,parseBody;std::function<void(std::uint32_t)> constructBody; // 원문 현재 색/문맥을 사용하는 실제 생성/Markup이다.
    std::function<void(std::uint32_t,std::int32_t)> scrollable; //4ba230의 현재 본문 scroll 허용이다.
    std::function<void(std::uint32_t,float,float,std::uint32_t,std::uint32_t)> attach; // 각 실제 가상 부착이며 HTML 전체 Fit은 후속이다.
    std::function<std::uint32_t(std::uint32_t)> reference;std::function<double()> wallSeconds; // 실제 현재 serial 조회와460d70의 벽 시각이다.
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> queue;std::function<void(std::uint32_t)> link; //452f90의 원시 큐와4a00c0의 현재 문자열 링크다.
    std::function<std::uint32_t(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> baseHandle; // 나머지 사건은464990의 전체DWORD를 보존한다.
};
class HtmlWindowGump { // 전체 부모의 이미 복원한 수명/내용/입력 본체를 같은 현재 저장소에서 수행한다.
public:
    HtmlWindowGump(HtmlWindowGumpState& state,HtmlWindowGumpHooks hooks); // 같은 기본/파생 주소만 미리 요구한다.
    std::uint32_t Construct(std::uint32_t token,std::uint32_t width,std::uint32_t height) const; //4cf120의20행/선택 필드 초기화와 인스턴스 증가다.
    void Destruct(std::uint32_t token) const; //4cf1c0의 count감소→OnExit큐→보조창Kill→시각→기본 소멸이다.
    void ClearButtons(std::uint32_t token) const; //4c7c10의 현재 count 재조회/각 child Kill·세필드0/마지막count0이다.
    void AddButton(std::uint32_t token,std::string_view label,std::uint32_t command,std::uint32_t parameter,std::uint32_t instant) const; //4c7c60의20행 한도/현재 행 저장/가상 부착/Instant/count 증가다.
    void SetBody(std::uint32_t token,std::string_view text) const; //4c7aa0의 LF보충/기존Kill/실제 새 본문·보조창/비void 재부착이다.
    std::uint32_t Handle(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) const; //4c79b0의20행 serial/첫 행/태그/메뉴 종료와 기본 위임이다.
    void Idle(std::uint32_t token) const; //4c8350의0기한 거부/벽 시각 뒤 현재 기한·명령 재조회/큐→Kill이다.
private:
    HtmlWindowGumpNode& Node(std::uint32_t token) const;GumpCreateNode& Base(std::uint32_t token) const; // 같은 현재 실제 주소를 검증한다.
    HtmlWindowGumpState& state_;HtmlWindowGumpHooks hooks_; // 호출보다 오래 유지되는 같은 상태/장치다.
};
}
