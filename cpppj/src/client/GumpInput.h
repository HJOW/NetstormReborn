// 같은 Gump 저장소에서 캡처/dialog 우선 전달과 overlay의 실제 커서 선택을 결합한다.
#pragma once
#include "client/GumpEvents.h"

namespace netstorm::client {
// overlay 처리기는 호출자의 사건 대신 원본의 별도 사건 코드 전역을 사용한다.
struct GumpInputState { std::uint32_t overlayEvent{}; };
// 가상 +0x28과 dialog 한도 로그의 경계다. 콜백은 현재 캡처/표/노드를 바꿀 수 있다.
struct GumpInputHooks {
    std::function<GumpCreateNode*(std::uint32_t)> node;
    std::function<std::uint32_t(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> handle;
    std::function<void(std::uint32_t)> overflow;
};
// OS 입력 큐 앞의 원본 UI 전달 계층이다. 기본 선택/필터/재귀 전달은 실제 GumpEvents를 호출한다.
class GumpInput {
public:
    // 같은 생성/수명/위치/이벤트 저장소와 안정된 노드는 입력 처리기보다 오래 살아야 한다.
    GumpInput(GumpCreateState& create,GumpLifetimeState& lifetime,GumpInputState& state,
        const GumpCollection& collection,const GumpEvents& events,GumpInputHooks hooks);
    // 이미 캡처된 객체가 있으면 유지한다. 토큰 0도 원본처럼 그대로 대입할 수 있다.
    void Capture(std::uint32_t token) const;
    // enabled이고 현재 override가 비었을 때만 설정하며, 나머지는 기존 객체와 무관하게 지운다.
    void SetOverride(std::uint32_t token,bool enabled) const;
    // 중복이면 0, 새 등록이면 1이다. 가득 찼을 때는 중복 검색 전에 원본 로그를 호출한다.
    bool AddDialog(std::uint32_t token) const;
    // 캡처가 있으면 필터 없이 직접 전달한다. 없으면 현재 dialog 또는 루트의 실제 재귀 전달이다.
    std::uint32_t Route(std::uint32_t event,std::uint32_t first,std::uint32_t second,bool includeHidden=false) const;
    // 표 순서로 visible 부모/경계/실제 전체 선택을 검사하고 첫 대상의 raw 처리기 반환을 보존한다.
    // event 인자는 원본 몸체에서도 사용하지 않는다. overlayEvent 전역이 가상 처리기에 전달된다.
    std::uint32_t Overlay(std::uint32_t event,ScreenPoint point) const;
private:
    // 유효한 현재 노드만 조회하며 잘못된 토큰은 호스트 예외로 거부한다.
    GumpCreateNode& Node(std::uint32_t token) const;
    // 직접 가상 처리기에 도달한 때만 저장소와 처리기 경계의 존재를 확인한다.
    std::uint32_t Handle(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) const;
    // 자신부터 모든 부모의 visible만 검사한다. 부모의 dead/void는 여기서 제외하지 않는다.
    bool ParentsVisible(std::uint32_t token) const;
    GumpCreateState& create_;
    GumpLifetimeState& lifetime_;
    GumpInputState& state_;
    const GumpCollection& collection_;
    const GumpEvents& events_;
    GumpInputHooks hooks_;
};
}
