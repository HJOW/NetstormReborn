// ChatWindow의 실제 자식 기하와 표시 전환만 복원한다. Console/TextEntry 생성·입력은 별도 필수 경계다.
#pragma once
#include "client/GumpCreate.h"

namespace netstorm::client {
// 원본0x13cBYTE ChatWindow의 기본 Gump와 +a0..138 상태다.
struct HudChatNode : GumpCreateNode {
    std::uint32_t console{},text{},helper{},background{}; // +a0/+a4/+a8/+ac의 실제 네 자식이다.
    std::uint32_t capacity{}; // +b0: 생성자가 쓰는 입력 버퍼128 크기다.
    std::int32_t leftPadding{}; // +b4: 모든 재부착에서 다시 읽는 signed 정수 여백이다.
    std::array<char,128> inputPrefix{}; // +b8..137: 실제 Console 입력과 공유하는 접두 문자열이다.
    std::uint8_t helperVisible{}; // +138: helper 높이15를 허용하는 원시 BYTE다.
};
// 다음 단계 어댑터는 실제 공통 저장소와 ScrollingText/Console의 가상 경계를 공급해야 한다.
struct HudChatGeometryHooks {
    std::function<HudChatNode*(std::uint32_t)> chat; // 같은 root 파생 노드다.
    std::function<GumpCreateNode*(std::uint32_t)> node; // root와 모든 자식의 같은 기본 주소다.
    std::function<void(std::uint32_t,float,float,std::uint32_t,std::uint32_t)> baseAttach; //00466750 공통 트리 부착이다.
    std::function<void(std::uint32_t)> push; // 각 현재 자식의 실제 가상+18이다.
    std::function<void(std::uint32_t,float,float,std::uint32_t,std::uint32_t)> attach; // 각 현재 대상의 실제 가상+20이다.
    std::function<std::uint32_t(std::uint32_t)> bottomIndex; //004bc2c0의 실제 표시 줄/scroll 조회이며 아직 공개 제품 API는 없다.
    std::function<void(std::uint32_t,std::uint32_t)> requestBottom; //004ba3d0의 ScrollingTextGump::RequestBottom과 같은 요청이다.
    std::function<std::uint32_t()> challenge; //0042f270 분기 시점의 현재00594fc8이다.
};
// 생성/Console/네트워크/Chat Helper Handle을 완료로 계산하지 않는 독립 기하 모듈이다.
class HudChatGeometry {
public:
    HudChatGeometry(HudChatGeometryHooks hooks); // 현재 노드 조회만 생성 시 요구한다.
    void Attach(std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode) const; //0042e6e0 전체 자식 재부착 순서다.
    void SetHelperVisible(std::uint32_t token,std::uint32_t value) const; //0042f250: 비영 값을 BYTE1로 바꾸며 재부착은 하지 않는다.
    void SetInputVisible(std::uint32_t token,std::uint32_t value) const; //0042f270: 현재 Console visible 변경 뒤 Console/root를 재부착한다.
private:
    HudChatNode& Chat(std::uint32_t token) const; // root와 기본 저장소가 같은 주소인지 확인한다.
    GumpCreateNode& Node(std::uint32_t token) const; // 실제 도달한 자식만 조회한다.
    void Size(std::uint32_t token,double width,double height) const; // FST32 뒤에도 x87 중간값으로 배율을 곱하는 원문을 유지한다.
    void Push(std::uint32_t token) const; // 현재 가상 Push 경계만 실행한다.
    void AttachVirtual(std::uint32_t token,float x,float y,std::uint32_t parent) const; // 현재 가상 Attach의 mode0 호출이다.
    HudChatGeometryHooks hooks_; // 실제 공유 노드와 공통/텍스트 효과의 명시 경계다.
};
}
