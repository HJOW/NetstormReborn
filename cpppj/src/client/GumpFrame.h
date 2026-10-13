// 원본 Gump 등록표의 통지·idle 실행과 자식 재그리기 요청을 같은 생성/수명 저장소에서 처리한다.
#pragma once
#include "client/GumpLifetime.h"

namespace netstorm::client {
// node는 처리 동안 안정된 노드를 제공한다. 세 가상 콜백은 같은 표/링크/플래그를 변경할 수 있다.
// redraw는 가상 +8의 재그리기 요청이며 실제 픽셀 Draw와 별개다. 객체 해제는 지연 삭제 단계에서 수행한다.
struct GumpFrameHooks {
    // node: 토큰에 대응하는 저장 노드를 조회하는 콜백.
    std::function<GumpCreateNode*(std::uint32_t)> node;
    std::function<void(std::uint32_t)> idle; // 가상 +0x30, 인자 없음.
    std::function<void(std::uint32_t,bool)> redraw; // 가상 +8, 원본 프레임 요청 인자는 항상 false다.
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> broadcast; // 가상 +0x2c.
};
// 10.78의 00464100/00464170/00464c20/004660f0와 CD의 대응 전체 정상 몸체를 복원한다.
// OS 입력 큐/실제 파생 가상 함수/Client 자동 프레임은 별도 연결 대상이다.
class GumpFrame {
public:
    // 같은 생성·수명 상태와 안정된 노드 저장소가 처리기보다 오래 살아야 한다. node 조회는 필수다.
    GumpFrame(GumpCreateState& create,GumpLifetimeState& lifetime,GumpFrameHooks hooks);
    // 등록표의 1~999칸을 방문한다. 비영 칸 수가 현재 signed count에 도달하면 바로 종료한다.
    // visible·0x100·비 dead인 객체만 가상 +0x2c에 통지한다. 필터에 실패한 비영 칸도 개수에 포함한다.
    // 원본: 00464100 ↔ CD 00492940. 이력: 2026-10-12 프레임 통지 전달 복원.
    void Broadcast(std::uint32_t event,std::uint32_t first,std::uint32_t second) const;
    // 패치판 500칸/CD 80칸의 현재 idle 토큰을 순서대로 호출한다. 노드 플래그/부모 visible은 검사하지 않는다.
    // 원본: 00464170 ↔ CD 00492990. 이력: 2026-10-12 등록표 프레임 처리 복원.
    void Idle() const;
    // 자신의 0x42000이 없으면 redraw(false), 현재 0x20이면 visible 자식을 재귀 처리한다.
    // 자신과 하위 자식의 dead/void는 제외 조건이 아니다. 자식 next는 재귀 호출 뒤 현재 값을 읽는다.
    // 원본: 00464c20 ↔ CD 004939e0. 이력: 2026-10-12 하위 재그리기 요청 복원.
    void RedrawNode(std::uint32_t token) const;
    // 실제 등록표 +0의 자식부터 visible·비 dead 루트만 처리한다. 다음 루트는 콜백 전에 저장한다.
    // 수집 override/루트 자신의 visible은 사용하지 않는다. 하위 요청은 RedrawNode와 같은 필터다.
    // 원본: 004660f0 ↔ CD 004927c0. 이력: 2026-10-12 최상위 재그리기 요청 복원.
    void RedrawRoots() const;
private:
    // 실제 가상 호출/필드 읽기에 도달한 토큰만 검사한다. 없는 저장소는 호스트 예외로 거부한다.
    GumpCreateNode& Node(std::uint32_t token) const;
    // 현재 경로의 재귀 순환과 형제 순환을 거부하며 원본 콜백 뒤 필드 재조회 순서를 유지한다.
    void Redraw(std::uint32_t token,std::vector<std::uint32_t>& active) const;
    // create_: 등록 표 +0이 실제 루트다. 외부에서 표를 바꿀 때 collection.root도 같은 토큰으로 맞춰야 한다 (GumpCreateState).
    GumpCreateState& create_;
    // lifetime_: 등록된 객체의 삭제·해제를 처리하는 수명 관리자.
    GumpLifetimeState& lifetime_;
    // hooks_: 외부 저장소 조회와 가상 동작을 연결하는 콜백 묶음 (GumpFrameHooks).
    GumpFrameHooks hooks_;
};
}
