// 원본 0xb8바이트 배경 Gump의 생성 필드를 기존 등록/트리 저장소에 연결한다.
#pragma once
#include "client/GumpCreate.h"

namespace netstorm::client {
// +a0/+a4/+a8/+ac/+b0/+b4의 SHP·타일 크기·타입·프레임·배경 플래그다.
struct GumpBackgroundNode : GumpCreateNode {
    // shape: 프레임 그림을 찾는 셰이프 식별자 또는 셰이프 자료.
    std::uint32_t shape{};
    // tileWidth: 배경 타일 하나의 픽셀 너비.
    // tileHeight: 배경 타일 하나의 픽셀 높이.
    std::int32_t tileWidth{},tileHeight{};
    // type: 타입 표에서 객체 종류를 찾는 번호.
    // frame: 타입에서 사용할 그림 프레임 번호.
    // backgroundFlags: 배경 타일·누름 표시의 출력 플래그.
    std::uint32_t type{},frame{},backgroundFlags{};
};
// 실제 타입 저장소의 주소 토큰과 +dc SHP 포인터다. frame 조회 중에도 주소가 안정되어야 한다.
struct GumpBackgroundType { /* token: 저장소나 분배기에서 대상을 구별하는 토큰. */ /* shape: 프레임 그림을 찾는 셰이프 식별자 또는 셰이프 자료. */ std::uint32_t token{},shape{}; };
// 실제 SHP 프레임 헤더 +18/+1a의 signed WORD를 부호 확장하기 전 값이다.
struct GumpBackgroundFrame { /* width: 가로 크기. */ /* height: 세로 크기. */ std::int16_t width{},height{}; };
// 타입/프레임 조회는 실제 자산 공급 경계다. type=0이면 두 경계를 호출하지 않는다.
struct GumpBackgroundCreateHooks {
    // node: 토큰에 대응하는 저장 노드를 조회하는 콜백.
    std::function<GumpBackgroundNode*(std::uint32_t)> node;
    // type: 타입 표에서 객체 종류를 찾는 번호를 조회하는 콜백.
    std::function<const GumpBackgroundType*(std::uint32_t)> type;
    // frame: 타입에서 사용할 그림 프레임 번호를 조회하는 콜백.
    std::function<const GumpBackgroundFrame*(std::uint32_t,std::uint32_t,std::uint32_t)> frame;
};
// 기본 생성/등록은 GumpCreate, 부착/파괴는 같은 저장소의 GumpTree/GumpLifetime이 맡는다.
class GumpBackgroundCreate {
public:
    // color는 패치 0054043e/CD 00535f34의 signed BYTE다. 상태/저장소는 생성기보다 오래 살아야 한다.
    GumpBackgroundCreate(GumpCreateState& state,const GumpCreate& factory,
        const std::int8_t& color,GumpBackgroundCreateHooks hooks);
    // 004655d0/CD 00494640: 기본 생성 뒤 배경 가상 표/색/플래그·타입·프레임/타일 정보를 쓴다.
    // 타입 0이면 미작성 shape/tileWidth/tileHeight를 보존한다. 정상 반환값은 입력 토큰이다.
    std::uint32_t Construct(std::uint32_t token,float width,float height,std::uint32_t backgroundFlags,
        std::uint32_t flags,std::uint32_t type,std::uint32_t frame) const;
private:
    // 기본 노드와 파생 필드가 실제로 같은 저장소인지 호출 시 검사한다.
    GumpBackgroundNode& Node(std::uint32_t token) const;
    // state_: 등록 표 +0이 실제 루트다. 외부에서 표를 바꿀 때 collection.root도 같은 토큰으로 맞춰야 한다 (GumpCreateState).
    GumpCreateState& state_;
    // factory_: 타입에 맞는 객체를 생성하는 처리기.
    const GumpCreate& factory_;
    // color_: 그림·글자 출력에 사용할 팔레트 색 인덱스 또는 색 상태.
    const std::int8_t& color_;
    // hooks_: 외부 저장소 조회와 가상 동작을 연결하는 콜백 묶음 (GumpBackgroundCreateHooks).
    GumpBackgroundCreateHooks hooks_;
};
}
