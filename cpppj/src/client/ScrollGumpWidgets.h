// Slider/돌 화살표/배경 손잡이의 전체 정상 생성과 실제 가상 Draw/scalar 삭제를 기존 계층에 결합한다.
#pragma once
#include "client/ScrollGump.h"
#include "client/StoneButtonGump.h"
#include "client/GumpBackgroundDraw.h"

namespace netstorm::client {
// 원본 StoneButton 파생 +170 방향이다. 버튼/배경/글자 문맥과 동일 객체를 공유한다.
struct ScrollGumpArrowNode: StoneButtonGumpNode { /* direction: 방향이나 회전을 선택하는 값. */ std::uint32_t direction{}; };
// 현재 +20 종류 BYTE와 손잡이 기본 배경 전역이다. 초기 종류는 두 원본에서39/51/1이다. 위치 정렬 +28과 구별한다.
// defaultType은 원본 자산 로딩 후의 타입 토큰을 공급해야 한다. 원본 배포 PE 초기값은 0, frame은 1이다.
struct ScrollGumpWidgetState {
    // sliderAlignment: 스크롤바 종류 BYTE(기존 이름).
    // thumbAlignment: 손잡이 종류 BYTE(기존 이름).
    // arrowAlignment: 화살표 종류 BYTE(기존 이름).
    std::int8_t sliderAlignment{39},thumbAlignment{51},arrowAlignment{1};
    // defaultType: 기본 타입.
    // defaultFrame: 기본 프레임.
    std::uint32_t defaultType{},defaultFrame{1};
};
// allocate는 원본 new 바이트 수에 맞는 안정된 파생 저장소를 만든다. 반환 0은 원본 할당 실패다.
// node/arrow/thumb/base는 기존 생성·트리·버튼·배경·Slider와 같은 객체를 반환해야 한다.
struct ScrollGumpWidgetHooks {
    // node: 토큰에 대응하는 저장 노드를 조회하는 콜백.
    std::function<ScrollGumpNode*(std::uint32_t)> node;
    // arrow: 화살표를 조회하는 콜백.
    std::function<ScrollGumpArrowNode*(std::uint32_t)> arrow;
    // thumb: 스크롤 손잡이의 Gump 토큰를 조회하는 콜백.
    std::function<ScrollGumpThumbNode*(std::uint32_t)> thumb;
    // base: 파생 처리에서 공통 동작을 위임하는 기본 객체를 조회하는 콜백.
    std::function<GumpCreateNode*(std::uint32_t)> base;
    // allocate: 새 대상의 저장소를 확보하는 콜백.
    std::function<std::uint32_t(std::size_t)> allocate;
    // attach: 대상을 부모 또는 처리 목록에 부착하는 콜백.
    std::function<void(std::uint32_t,float,float,std::uint32_t,std::uint32_t)> attach;
};
// 원본 생성·출력을 기존 기본 Gump/돌 버튼/배경/Slider에 위임한다. 자동 Client 조립은 상위 계층 책임이다.
class ScrollGumpWidgets {
public:
    // 같은 처리기/상태/노드 저장소는 생성 및 가상 분배 동안 살아 있어야 한다.
    ScrollGumpWidgets(GumpCreateState& create,ScrollGumpWidgetState& state,const GumpCreate& factory,
        const StoneButtonGump& stones,const GumpBackgroundCreate& backgrounds,const ScrollGump& sliders,
        const GumpLifetime& lifetime,ScrollGumpWidgetHooks hooks);
    // 004a64b0/CD 00417bd0: 0 크기→40·방향/step·세 자식 생성/부착·range 0..100/page 10/position 50 순서다.
    std::uint32_t Construct(std::uint32_t token,std::int32_t width,std::int32_t height,
        std::uint32_t type,std::uint32_t frame) const;
    // 004a63b0/CD Slider 생성 내부: 빈 돌 버튼/명시 문맥→파생 가상 표→팝업 유지 bit 0x20/정렬→방향이다.
    std::uint32_t ConstructArrow(std::uint32_t token,std::int32_t width,std::int32_t height,
        std::uint32_t direction,std::uint32_t unused,std::uint32_t color) const;
    // 004a60e0/CD 004178f0: 기본 배경 (0,0,0x3ec0000,0x201)→파생 표/rect 0→정렬/크기/인자 사본이다.
    // 이전 y/x와 type 0일 때 기본 shape/타일 필드는 원본처럼 미작성 상태를 보존한다.
    std::uint32_t ConstructThumb(std::uint32_t token,std::int32_t width,std::int32_t height,
        std::uint32_t type,std::uint32_t frame) const;
    // 004a5920/CD 004176f0: 실제 돌 버튼 Draw 뒤 현재 Bounds/기본 글자 색/방향으로 삼각형 세 선을 그린다.
    void DrawArrow(std::uint32_t token,const GumpCollection& collection,const GumpTextDraw& text,const GumpPixelTarget& target) const;
    // 004a5c40/CD 00417b50: 실제 배경 Draw 뒤 현재 Bounds+handle을 기본 배경 type/frame으로 다시 그린다.
    // 원본은 포함 선택 rect의 right/bottom을 +1하지 않고 half-open 배경 함수에 전달한다.
    void DrawThumb(std::uint32_t token,const GumpCollection& collection,const GumpBackgroundDraw& backgrounds) const;
    // 원본 세 가상 scalar 삭제는 기본 Gump 소멸/flags bit 0 반납에 위임한다. 자식은 기존 수명 처리기가 먼저 지운다.
    std::uint32_t DeleteScalar(std::uint32_t token,std::uint32_t flags) const;
private:
    // null/미등록/다른 기본 저장소를 실제 도달 시 거부한다. 전체 파생 필드를 임의로 초기화하지 않는다.
    ScrollGumpNode& Node(std::uint32_t token) const;
    ScrollGumpArrowNode& Arrow(std::uint32_t token) const;
    ScrollGumpThumbNode& Thumb(std::uint32_t token) const;
    GumpCreateNode& Base(std::uint32_t token) const;
    // 원본 할당 실패는 생성자를 생략해 0을 반환한다. 성공 시 같은 공통 표에 등록한다.
    std::uint32_t NewArrow(std::int32_t width,std::int32_t height,std::uint32_t direction,std::uint32_t type,std::uint32_t frame) const;
    std::uint32_t NewThumb(std::int32_t width,std::int32_t height,std::uint32_t type,std::uint32_t frame) const;
    // 처음 세 자식 부착은 부모 token/mode 0이다. 호출 직전의 현재 자식 토큰은 Construct가 공급한다.
    void Attach(std::uint32_t token,float x,float y,std::uint32_t parent) const;
    // create_: 등록 표 +0이 실제 루트다. 외부에서 표를 바꿀 때 collection.root도 같은 토큰으로 맞춰야 한다 (GumpCreateState).
    GumpCreateState& create_;
    // state_: 현재 signed 정렬 BYTE와 손잡이 기본 배경 전역이다. 초기 정렬은 두 원본에서 39/51/1이다 (ScrollGumpWidgetState).
    ScrollGumpWidgetState& state_;
    // factory_: 타입에 맞는 객체를 생성하는 처리기.
    const GumpCreate& factory_;
    // stones_: 같은 ButtonGump/GumpCreate/GumpTree/GumpLifetime을 사용한다. 자동 Client 연결은 별도 계층이 맡는다 (StoneButtonGump).
    const StoneButtonGump& stones_;
    // backgrounds_: 기본 생성/등록은 GumpCreate, 부착/파괴는 같은 저장소의 GumpTree/GumpLifetime이 맡는다 (GumpBackgroundCreate).
    const GumpBackgroundCreate& backgrounds_;
    // sliders_: ScrollGump.cpp의 Slider 값/부착/입력 처리기다. 생성/파생 Draw는 ScrollGumpWidgets에 분리한다 (ScrollGump).
    const ScrollGump& sliders_;
    // lifetime_: 등록된 객체의 삭제·해제를 처리하는 수명 관리자.
    const GumpLifetime& lifetime_;
    // hooks_: 외부 저장소 조회와 가상 동작을 연결하는 콜백 묶음 (ScrollGumpWidgetHooks).
    ScrollGumpWidgetHooks hooks_;
};
// Slider의 빈 Draw와 화살표/손잡이 Draw를 같은 kind 2 클립/픽셀 처리기로 분배하고 기존 처리를 보존한다.
void BindScrollGumpWidgetDraw(GumpDrawHooks& hooks,const GumpCollection& collection,const GumpTextDraw& text,
    const GumpPixelTarget& target,const GumpBackgroundDraw& backgrounds,
    std::function<const ScrollGumpWidgets*(std::uint32_t)> slider,std::function<const ScrollGumpWidgets*(std::uint32_t)> arrow,
    std::function<const ScrollGumpWidgets*(std::uint32_t)> thumb);
// 세 파생 토큰의 기본 scalar 소멸자 위임을 같은 자식 우선 수명 처리기에 연결한다.
void BindScrollGumpWidgetLifetime(GumpLifetimeHooks& hooks,std::function<const ScrollGumpWidgets*(std::uint32_t)> resolve);
}
