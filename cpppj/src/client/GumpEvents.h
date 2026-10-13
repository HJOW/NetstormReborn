// 같은 Gump 생성/트리/수명 저장소에서 원본 커서 대상 선택과 가상 이벤트 전달을 복원한다.
#pragma once
#include "client/GumpLifetime.h"

namespace netstorm::client {
// 원본 전체 트리 커서 선택의 현재 좌표/필터/최소 signed 깊이/선택 토큰 전역이다.
struct GumpEventState {
    // point: 원본 004a4fe0의 창 위치 계산(설정 `initWindowPos`): 1 왼쪽 위, 2 오른쪽 위, 3 오른쪽 아래, 4 왼쪽 아래, 5 가운데 (ScreenPoint).
    ScreenPoint point{};
    // mask: 대상 비트 또는 투명 영역을 선택하는 마스크.
    // bestDepth: 이벤트 후보에서 지금까지 선택한 최대 깊이.
    // selected: 현재 선택한 객체·컨트롤의 번호 또는 토큰.
    std::uint32_t mask{},bestDepth{},selected{};
};
// 실제 가상 +0x34 이벤트 필터와 +0x28 처리기를 공급한다. 콜백은 현재 링크/전역을 바꿀 수 있다.
// filter가 없으면 복원한 기본 필터를 사용한다. 노드는 처리 중 안정된 주소로 살아 있어야 한다.
struct GumpEventHooks {
    // node: 토큰에 대응하는 저장 노드를 조회하는 콜백.
    std::function<GumpCreateNode*(std::uint32_t)> node;
    // filter: 현재 후보가 검사 조건을 만족하는지 판정하는 콜백.
    std::function<bool(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> filter;
    // handle: 대상의 이벤트 처리기를 호출하는 콜백.
    std::function<std::uint32_t(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> handle;
};
// 원본의 UI 트리 선택은 표시 목록 qsort 순서와 별개이며 동일 깊이는 먼저 방문한 노드가 우선한다.
class GumpEvents {
public:
    // 생성/수명/선택 전역과 같은 노드의 실제 위치 수집기는 이 처리기보다 오래 살아야 한다.
    GumpEvents(GumpCreateState& create,GumpLifetimeState& lifetime,GumpEventState& state,
        const GumpCollection& collection,GumpEventHooks hooks);
    // 현재 dialog는 모든 이벤트를 받는다. 그 외에는 도움말 마우스/키보드/마우스 플래그를 판정한다.
    bool BaseFilter(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) const;
    // 루트 직계 자식만 선택한다. visible/dead와 경계만 검사하고 void는 제외하지 않는다.
    std::uint32_t PickTopLevel(ScreenPoint point) const;
    // 선택 전역을 초기화하고 전체 자식을 탐색한다. 화면 밖 자식 통과/void/dead/플래그 마스크를 적용한다.
    std::uint32_t Pick(ScreenPoint point,std::uint32_t mask) const;
    // 직접 자식부터 재귀 전달하고 실패하면 자신의 가상 필터/처리기를 호출한다. 성공 토큰을 반환한다.
    std::uint32_t Dispatch(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second,bool includeHidden=false) const;
    // 자신을 제외한 현재 부모 체인으로 전달하고 첫 비영 처리기 반환값의 전체 DWORD를 보존한다.
    std::uint32_t Bubble(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) const;
private:
    // 잘못된 토큰은 원본 포인터 계약 밖의 호스트 입력으로 거부한다.
    GumpCreateNode& Node(std::uint32_t token) const;
    // 현재 자식과 형제 링크를 읽으며 순환 경로만 거부하는 선택 재귀다.
    void PickChildren(std::uint32_t token,std::vector<std::uint32_t>& active) const;
    // 자식 성공을 먼저 반환하고 실패하면 현재 노드의 필터와 처리기를 실행하는 전달 재귀다.
    std::uint32_t Deliver(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second,
        bool includeHidden,std::vector<std::uint32_t>& active) const;
    // create_: 등록 표 +0이 실제 루트다. 외부에서 표를 바꿀 때 collection.root도 같은 토큰으로 맞춰야 한다 (GumpCreateState).
    GumpCreateState& create_;
    // lifetime_: 등록된 객체의 삭제·해제를 처리하는 수명 관리자.
    GumpLifetimeState& lifetime_;
    // state_: 원본 전체 트리 커서 선택의 현재 좌표/필터/최소 signed 깊이/선택 토큰 전역이다 (GumpEventState).
    GumpEventState& state_;
    // collection_: 화면 밖 자식 통과, 최상위 삭제 필터, 원본의 루트별 제한 개수를 보존하는 UI 수집기다 (GumpCollection).
    const GumpCollection& collection_;
    // hooks_: 외부 저장소 조회와 가상 동작을 연결하는 콜백 묶음 (GumpEventHooks).
    GumpEventHooks hooks_;
};
}
