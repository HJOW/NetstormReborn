// 메뉴/대화창 장식 배경의 생성·모서리 출력·확장 redraw 영역을 기존 Gump 저장소와 픽셀 표면에 연결한다.
#pragma once
#include "client/GumpBackgroundDraw.h"
#include "client/GumpLifetime.h"
#include "client/GumpFrame.h"

namespace netstorm::client {
// 원본 +b8 캐시 SHP/+bc 장식 타입/+c0 첫 모서리 프레임이다. 배경 타입/프레임과 독립적으로 보존한다.
struct GumpDecorationNode: GumpBackgroundNode { std::uint32_t cornerShape{},cornerType{},cornerFrame{}; };
// +20 식별 BYTE는 두 PE 초기값 57이다. 타입/프레임은 실제 게임 초기화 결과를 공급해야 한다.
struct GumpDecorationState { std::int8_t alignment{57};std::uint32_t backgroundType{},backgroundFrame{},cornerType{}; };
// 원본 타입 +dc SHP/+150(I 글자 첫 프레임)을 현재 자산 조회에서 공급한다.
struct GumpDecorationType { std::uint32_t shape{},firstFrame{}; };
// 같은 기본/파생 객체·현재 타입·캐시 SHP 원색 출력·half-open 변경 영역 요청의 실제 경계다.
struct GumpDecorationHooks {
    std::function<GumpDecorationNode*(std::uint32_t)> node;
    std::function<GumpCreateNode*(std::uint32_t)> base;
    std::function<GumpDecorationType(std::uint32_t)> type;
    std::function<void(std::uint32_t,std::uint32_t,ScreenPoint)> shape;
    std::function<void(ScreenRect)> invalidate;
};
// 원본의 0xc4 장식 배경 파생 몸체다. Form 객체 전체와 이 장식 배경은 서로 다른 복원 대상이다.
class GumpDecoration {
public:
    // 생성/배경/수집/수명과 상태/저장소/출력 표면은 처리기 및 연결 분배보다 오래 살아야 한다.
    GumpDecoration(GumpCreateState& create,GumpDecorationState& state,const GumpBackgroundCreate& factory,
        const GumpBackgroundDraw& background,const GumpCollection& collection,const GumpLifetime& lifetime,GumpDecorationHooks hooks);
    // 00465d10/CD 00494ec0: 배경(0x03e00000,flags 1)→장식 표/정렬→현재 타입/SHP/첫 I 프레임 저장이다.
    std::uint32_t Construct(std::uint32_t token,float width,float height) const;
    // 00465db0/CD 00494f80: 실제 배경 Draw 뒤 현재 Position을 저장하고 TL/TR/BR/BL의 원색 모서리를 순서대로 그린다.
    void Draw(std::uint32_t token) const;
    // 00465e60/CD 00495050: 자신/모든 부모가 visible일 때 (x-1,y-1,절삭(width+4),절삭(height+4)) 영역을 요청한다.
    // erase 인자는 원본처럼 사용하지 않으며 크기 양수 조건은 실제 영역 요청 단계에서 적용한다.
    void Redraw(std::uint32_t token,bool erase) const;
    // 004d4c40/CD 00495200: 파생 자원 없이 기존 기본 scalar 소멸/토큰 반납만 수행한다.
    std::uint32_t DeleteScalar(std::uint32_t token,std::uint32_t flags) const;
private:
    // 같은 기본/파생 저장소인지 검사한다. 부모도 기본 Gump의 현재 visible/parent 필드를 직접 읽는다.
    GumpDecorationNode& Node(std::uint32_t token) const;
    GumpCreateNode& Base(std::uint32_t token) const;
    // 모서리 호출마다 현재 캐시 SHP/첫 프레임을 재조회한다. 위치는 Draw가 저장한 값이다.
    void Corner(std::uint32_t token,std::uint32_t offset,ScreenPoint position) const;
    GumpCreateState& create_;
    GumpDecorationState& state_;
    const GumpBackgroundCreate& factory_;
    const GumpBackgroundDraw& background_;
    const GumpCollection& collection_;
    const GumpLifetime& lifetime_;
    GumpDecorationHooks hooks_;
};
// 실제 GameAssets의 타입 번호/현재 SHP와 I 첫 인덱스를 생성 경계에 공급한다.
void BindGumpDecorationAssets(GumpDecorationHooks& hooks,const GameAssets& assets);
// 캐시 SHP를 타입 번호 토큰으로 공급한 같은 실제 자산을 픽셀 출력에 연결한다. 그림 치환/배율은 적용하지 않는다.
class GumpDecorationPixelTarget {
public:
    // GameAssets와 픽셀 표면은 반환한 훅보다 오래 살아야 한다. SHP의 실제 블록/물리 프레임을 사용한다.
    GumpDecorationPixelTarget(const GumpPixelTarget& target,const GameAssets& assets);
    // 기존 노드/타입/변경 영역 경계를 보존하며 원색 모서리 출력만 연결한다.
    GumpDecorationHooks Hooks(GumpDecorationHooks hooks);
private:
    // 캐시 SHP의 물리 프레임을 해제하고 상대 VFX 좌표를 더해 같은 현재 클립 안에 합성한다.
    void Shape(std::uint32_t shape,std::uint32_t frame,ScreenPoint position);
    const GumpPixelTarget& target_;
    const GameAssets& assets_;
    std::map<std::pair<std::uint32_t,std::uint32_t>,IndexedImage> images_;
};
// 장식의 Draw/Redraw/scalar만 실제 분배에 연결하고 다른 기존 파생 타입 처리를 보존한다.
void BindGumpDecoration(GumpDrawHooks& draw,GumpFrameHooks& frame,GumpLifetimeHooks& lifetime,
    std::function<const GumpDecoration*(std::uint32_t)> resolve);
}
