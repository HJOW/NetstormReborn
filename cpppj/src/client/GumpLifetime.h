// 생성/트리와 같은 노드 저장소에서 원본의 지연 삭제와 기본 소멸자 수명을 처리한다.
#pragma once
#include "client/GumpCreate.h"

namespace netstorm::client {
// 원본의 고정 크기 idle/overlay/삭제/dialog 표와 현재 삭제 전역이다.
// CD의 idle 표는 앞 80칸만 사용한다. override 포인터는 collection.overrideHead와 공유한다.
struct GumpLifetimeState {
    std::array<std::uint32_t,500> idle{};
    std::array<std::uint32_t,100> overlay{};
    std::array<std::uint32_t,1000> kill{};
    std::array<std::uint32_t,16> dialogs{};
    std::uint32_t killCount{},dialogCount{},focus{},currentDialog{},diagnostics{};
};
// 가상 Push/삭제 소멸자·Form 삭제·CRT free의 명시 경계다. 콜백은 현재 상태를 바꿀 수 있다.
// node는 실행 중 안정된 주소를 공급하며 가상 deletingDestructor/release만 실제 저장소를 해제한다.
struct GumpLifetimeHooks {
    std::function<GumpCreateNode*(std::uint32_t)> node;
    std::function<void(std::uint32_t)> push;
    std::function<void(std::uint32_t,std::uint32_t)> deleteForm;
    std::function<void(std::uint32_t,std::uint32_t)> deletingDestructor;
    std::function<void(std::uint32_t)> release;
};
// Kill은 자식 우선 요청만 쌓고 Cleanup이 등록 해제 및 가상 소멸자를 실행한다.
class GumpLifetime {
public:
    // 생성 상태/수명 전역/노드 저장소는 처리기보다 오래 살아야 한다. node는 필수다.
    GumpLifetime(GumpCreateState& state,GumpLifetimeState& lifetime,GumpLifetimeHooks hooks);
    // 첫 빈 idle 칸에 넣거나 첫 일치 칸만 지운다. 현재 플래그가 이미 같으면 아무것도 하지 않는다.
    void SetIdle(std::uint32_t token,bool enabled) const;
    // 마지막 빈 overlay 칸에 넣거나 마지막 일치 칸만 지운다. 중복 enable은 원본 assert 계약이다.
    void SetOverlay(std::uint32_t token,bool enabled) const;
    // 첫 dialog 일치 칸을 제거/압축하고 현재 dialog를 마지막 항목으로 바꾼다.
    bool RemoveDialog(std::uint32_t token) const;
    // 자식 우선, idle/overlay/Form/Push/큐/포인터/dialog/dead 순서다. 중복 요청을 제거하지 않는다.
    void Kill(std::uint32_t token) const;
    // 비어 있지 않은 현재 큐 항목을 Destroy하고 그 직후 현재 개수로 반복 범위를 갱신한다.
    void Cleanup() const;
    // 자식 우선 등록 해제 후 dead를 지우고 destroying을 세워 가상 소멸자(1)를 호출한다.
    void Destroy(std::uint32_t token) const;
    // 기본 가상 표 복원→현재 텍스트 두 개 해제→overlay/idle/큐의 모든 일치 칸 제거다.
    void DestructBase(std::uint32_t token) const;
    // 기본 소멸자 뒤 flags의 하위 바이트 bit 0(값 1)일 때 객체 자체를 해제하고 원래 토큰을 반환한다.
    std::uint32_t DeleteScalar(std::uint32_t token,std::uint32_t flags) const;
private:
    // 유효 노드와 판본별 idle 표 길이를 조회한다. 잘못된 토큰은 호스트 예외로 거부한다.
    GumpCreateNode& Node(std::uint32_t token) const;
    std::size_t IdleSize() const;
    // 링크 순환을 거부하면서 next를 호출 전에 보관하는 자식 우선 재귀를 유지한다.
    void Recurse(std::uint32_t token,bool destroy,std::vector<std::uint32_t>& active) const;
    GumpCreateState& state_;
    GumpLifetimeState& lifetime_;
    GumpLifetimeHooks hooks_;
};
}
