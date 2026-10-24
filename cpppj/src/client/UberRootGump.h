// 원본 Uber의 전체 자식 생성 순서와 현재 루트 교체를 같은 공통 Gump 저장소에서 실행한다.
#pragma once
#include "client/HudDock.h"

namespace netstorm::client {
// 004c7160의0xc8BYTE 본체다. 같은 기본 주소에 실제 자식 포인터를 보관한다.
struct UberRootNode:GumpCreateNode {
    std::uint32_t vertical{},minimapDivider{},chatDivider{},minimap{},production{},reservedB4{},chat{},reservedBC{},knowledge{},message{}; // +a0..c4의 현재 원문 필드다.두 예약필드는 생성자가 보존한다.
};
// 현재 원본 전역과 자식 생성/가상 분배만 빌린다. 노드나 등록표 사본을 만들지 않는다.
struct UberRootHooks {
    std::function<UberRootNode*(std::uint32_t)> node; // 같은 고정this의 실제 파생 주소다.
    std::function<HudDividerNode*(std::uint32_t)> divider; // 세 구분선은 같은 실제0x11c 파생 저장소를 제공한다.
    std::function<GumpCreateNode*(std::uint32_t)> base; // 새/기존 루트와 모든 자식의 같은 현재 기본 주소다.
    std::function<std::uint32_t(std::uint32_t)> read; // 종류BYTE·화면·타입·frame·현재 메시지·명령을 실제 소비 위치에서 읽는다.
    std::function<void(std::uint32_t,std::uint32_t)> write; // 배경/프레임 전역 세 개를 실제 위치에서 쓴다.
    std::function<std::uint32_t(std::size_t)> allocate; // 실제0xc8 루트/0x11c 구분선/0xb0 메시지 저장소를 확보한다.
    std::function<void(std::uint32_t,std::uint32_t,float,float,std::uint32_t)> constructBase; // 같은 GumpCreate::Construct다.
    std::function<void(std::uint32_t,std::uint32_t,std::int32_t,std::int32_t,std::uint32_t,std::uint32_t,std::uint32_t)> constructDivider; // HudDock::Construct의 실제 기존 몸체다.
    std::function<std::uint32_t()> createMinimap,createProduction,createKnowledge,createChat; // 원문 네 공장의 전체 생성자다.빈 노드/이전 자식 재사용으로 대신하지 않는다.
    std::function<void()> emptyObserver; //4437f0은실제1BYTE RET다.선택적호출관측만허용하며게임하위몸체를요구하지않는다.
    std::function<void()> refreshTechnology,cleanup; //42f540/4662b0의 그 시점 현재 실제 몸체다.
    std::function<void(std::uint32_t,float,float,std::uint32_t,std::uint32_t)> attach; // 가상+20은 각 자식의 실제 파생 부착이다.
    std::function<void(std::uint32_t,float,float,std::uint32_t)> move; // 가상+10은 현재 좌표/기준점의 실제 Move다.
    std::function<void(std::uint32_t)> push; // 가상+18은 현재 노드의 실제 분리다.
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> registerSlave; //004c4790의 전체10회 원문순서다.
    std::function<std::uint32_t(std::uint32_t)> type; //0049a840의 실제 TYPE 조회다.반환은 생성자에서 읽지 않는다.
    std::function<std::uint32_t(std::string_view)> translate; // 원문 Chat Window 번역 포인터다.
    std::function<void(std::uint32_t,std::uint32_t)> help; //00464ed0의 같은 번역 포인터 보관이다.
    std::function<void(std::uint32_t,std::string_view)> helpWord; //00464f80의 chatView 포인터 보관이다.
    std::function<std::uint32_t()> savedMessage; //565e70 현재 C문자열의 원본 포인터 토큰이며 복사 시점과 구별한다.
    std::function<bool(std::uint32_t,std::string_view)> equalText; // 현재 원본 C문자열의 대소문자 무시 비교다.
    std::function<void(std::uint32_t,std::uint32_t)> constructMessage; //0041a030의 같은 메시지 이름/전체 HTML 부모다.
    std::function<std::uint32_t(const std::array<std::uint32_t,9>&)> formatMessage; //459c60의 실제9개원시인자로 조건표현식을 작성한다.
    std::function<std::uint32_t(std::uint32_t)> testMessage; //41ab10의 실제 완성 표현식 포인터를 검사한다.
    std::function<void(std::uint32_t)> visitMessage; //4b6100의 실제 메시지 방문 기록이다.
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> messageEvent; //419e10의 실제 물리 인자다.
    std::function<void(std::uint32_t,bool)> overlay; //466600의 공통 Overlay 등록이다.
    std::function<void(std::uint32_t,std::uint32_t)> setDepth; //4652f0의 현재 자식 우선 전체 깊이 쓰기다.
    std::function<void(std::uint32_t)> replaceRoot; //4666a0의 실제 이전루트Kill/Cleanup/등록0 교체다.
};
// 원본 전체 부모를 기존 자식 처리기에 조립한다. 미연결 자식은 선택된 위치에서 실패한다.
class UberRootGump {
public:
    UberRootGump(GumpCreateState& create,UberRootHooks hooks); // 실제 단일 등록표와 고정 노드 주소를 빌린다.
    std::uint32_t Construct(std::uint32_t token) const; //004c7160: 전체200BYTE Uber 생성자다.
    void Replace() const; //004c77c0: 새 전체 Uber→임시등록 반납→이전자식 이동→공통 루트 교체다.
    HudDockNodeView DockView(std::uint32_t token) const; // 이 처리기/해당 실제 노드보다 짧게 빌리는 현재필드 뷰다.전체자식사본을만들지않는다.
private:
    GumpCreateNode& Base(std::uint32_t token) const; // 실제 소비 시점의 고정 기본 주소를 요구한다.
    UberRootNode& Node(std::uint32_t token) const; // 같은 기본 주소인 실제 파생 노드를 요구한다.
    std::uint32_t Read(std::uint32_t address) const; // 현재 원본 DWORD/종류BYTE를 매 원문 위치에서 조회한다.
    void Empty() const; //4437f0의실제빈본문이며선택관측이없어도게임효과없이복귀한다.
    HudDividerNode& Divider(std::uint32_t token) const; // 같은 기본 주소인 실제 구분선 확장 필드를 요구한다.
    void Attach(std::uint32_t child,float x,float y,std::uint32_t parent) const; // 현재 자식의 가상Attach(…,0)를 실행한다.
    void Layout(std::uint32_t token) const; // 같은 세 구분선/실제 네 패널에 원본10등록과 제한을 쓴다.
    GumpCreateState& create_; // 실제1000칸/현재0루트/count/serial의 같은 저장소다.
    UberRootHooks hooks_; // 전체 하위 몸체와 현재 전역의 실제 연결이다.
};
}
