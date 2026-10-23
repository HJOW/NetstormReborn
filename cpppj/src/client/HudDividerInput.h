// 원본 divider의 입력과 flags100 기반 프레임 방송을 같은 공통 Gump에 연결한다.
#pragma once
#include "client/HudDock.h"
#include "client/GumpInput.h"
#include "client/GumpTree.h"
#include "client/InputEvent.h"

namespace netstorm::client {
// 입력 처리 중 현재 노드/메뉴 수와 외부 장치만 읽는다. 정지나 도움말 상태로 대체하지 않는다.
struct HudDividerInputHooks {
    std::function<HudDividerNode*(std::uint32_t)> divider; // 같은 파생 노드의 안정된 주소다.
    std::function<GumpCreateNode*(std::uint32_t)> base; // 공통 트리의 같은 기본 주소인지 검증한다.
    std::function<std::uint32_t(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> baseHandle; // 미처리 사건의 실제 기본 Help 처리다.
    std::function<void(std::uint32_t)> cursor; // 현재 커서4/5를 원본 장치 경계에 전달한다.
    std::function<std::uint32_t()> menuInstances; // 00593024의 실제 MenuGumpState.instances 현재값이다.
    std::function<InputEvent(std::uint32_t)> poll; // 원본00452e00 좌클릭 현재값 폴링이다.
    std::uint32_t cursorEvent{0x0200003b}; // 원본005427e4의 현재 hover 사건이다.
};
// 004c49f0 가상+28과 004c4b00 가상+2c만 담당한다. 별도 Idle 등록은 없다.
class HudDividerInput {
public:
    // 같은 노드/위치/입력/이동 처리기는 이 객체보다 오래 살아야 한다.
    HudDividerInput(const GumpCollection& collection,const GumpInput& input,const GumpTree& tree,HudDividerInputHooks hooks);
    // 먼저 로컬 변환한 뒤 좌클릭 기준점/캡처·release·hover·기본 Help 순서로 처리한다.
    std::uint32_t Handle(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) const;
    // 메뉴 인스턴스0일 때 폴링→release 비트 정리→축/제한 계산→공통 Move를 실행한다.
    void Broadcast(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) const;
private:
    HudDividerNode& Node(std::uint32_t token) const; // 공통 노드와 파생 노드가 같은 기본 주소인지 확인한다.
    const GumpCollection& collection_; // 입력마다 원본 부모 체인으로 현재 로컬 좌표를 계산한다.
    const GumpInput& input_; // 첫 캡처만 유지하는 실제 공유 focus를 사용한다.
    const GumpTree& tree_; // anchor 보정과 가상 Push/Attach 순서를 그대로 재사용한다.
    HudDividerInputHooks hooks_; // 실제 현재 메뉴 수와 외부 폴링/커서/Help 경계다.
};
}
