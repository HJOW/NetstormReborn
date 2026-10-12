// 원본 0xb8바이트 배경 Gump의 생성 필드를 기존 등록/트리 저장소에 연결한다.
#pragma once
#include "client/GumpCreate.h"

namespace netstorm::client {
// +a0/+a4/+a8/+ac/+b0/+b4의 SHP·타일 크기·타입·프레임·배경 플래그다.
struct GumpBackgroundNode : GumpCreateNode {
    std::uint32_t shape{};
    std::int32_t tileWidth{},tileHeight{};
    std::uint32_t type{},frame{},backgroundFlags{};
};
// 실제 타입 저장소의 주소 토큰과 +dc SHP 포인터다. frame 조회 중에도 주소가 안정되어야 한다.
struct GumpBackgroundType { std::uint32_t token{},shape{}; };
// 실제 SHP 프레임 헤더 +18/+1a의 signed WORD를 부호 확장하기 전 값이다.
struct GumpBackgroundFrame { std::int16_t width{},height{}; };
// 타입/프레임 조회는 실제 자산 공급 경계다. type=0이면 두 경계를 호출하지 않는다.
struct GumpBackgroundCreateHooks {
    std::function<GumpBackgroundNode*(std::uint32_t)> node;
    std::function<const GumpBackgroundType*(std::uint32_t)> type;
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
    GumpCreateState& state_;
    const GumpCreate& factory_;
    const std::int8_t& color_;
    GumpBackgroundCreateHooks hooks_;
};
}
