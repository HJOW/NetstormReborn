// 다리 preDestroy/postDestroy의 raw SID 판단과 외부 효과 호출 순서를 복원한다.
// 일반 탐색은 RawSquidFinder로 연결한다. 기본 삭제·낙하·소리·통지는 호출자가 연결한다.
#pragma once
#include "o/SidPool.h"
#include <functional>

namespace netstorm::o {
class RawSquidFinder;
// 00421530 ↔ CD 004490b0가 등록하는 낙하 처리 이벤트 번호다.
inline constexpr std::uint32_t kBridgeFallEvent = 0x2692;
// 원본 preDestroy의 ±1 사각형과 postDestroy의 한 칸 탐색 범위다(양 끝 포함).
struct BridgeLifecycleSearch { int left{},top{},right{},bottom{}; };
// 원본에서 호출되는 외부 효과. Base는 공통 Squid 구현으로 이어지는 단계다.
// NotifyRemoval은 기존 API 이름이다. 실제 00460600의 의미는 Flyingshrapnel 파편 생성이다.
enum class BridgeLifecycleEffect { DestroyLink,NotifyRemoval,FallSound,FallWalker,BasePreDestroy,BasePostDestroy };
struct BridgeLifecycleEvent {
    BridgeLifecycleEffect effect{};
    Sid sid{};
    std::uint32_t flags{};
    float x{},y{}; // FallSound에서만 쓰는 원본 단정도 좌표다.
};
// 탐색기는 begin의 첫 번호와 next의 후속 번호를 내준다. 0은 끝이며 순서/동적 필터는 호출자 계약이다.
// emit은 같은 풀을 변경할 수 있다. 다음 객체의 참조/타입/상태는 그 변경 이후 다시 읽는다.
struct BridgeLifecycleHooks {
    std::function<Sid(BridgeLifecycleSearch)> begin;
    std::function<Sid()> next;
    std::function<void(const BridgeLifecycleEvent&)> emit;
};
// 실제 raw 일반 탐색기의 Begin/Next를 삭제 훅에 연결한다. emit의 삭제/소리/낙하 구현은 호출자 책임이다.
// 반환한 콜백은 finder를 참조하므로 finder가 더 오래 살아야 하며 동시에 다른 탐색에 재사용하면 안 된다.
BridgeLifecycleHooks MakeBridgeLifecycleHooks(RawSquidFinder& finder,
    std::function<void(const BridgeLifecycleEvent&)> emit);
class RawBridgeLifecycle {
public:
    // 타입 표와 풀은 이 어댑터보다 오래 살아야 하며 같은 판본의 실제 번호 배치를 사용한다.
    RawBridgeLifecycle(const SidPool& pool,std::span<const RiftTypeRecord> types);
    // 00422290(패치)/CD preDestroy 인라인: +12의 첫 참조만 extra를 검사하고 두 참조의 dead를 OR한다.
    bool LinkNeedsDestroy(Sid link) const;
    // 004221b0 ↔ CD 004498e0: 주변 특수 타입을 순서대로 처리한 뒤 공통 preDestroy로 이어진다.
    void PreDestroy(Sid bridge,std::uint32_t flags,std::uint32_t linkType,const BridgeLifecycleHooks& hooks) const;
    // 00422300 ↔ CD 00449c60: 파편 생성 요청→소리→칸 위 walker 낙하→공통 postDestroy 순서다.
    void PostDestroy(Sid bridge,std::uint32_t flags,const BridgeLifecycleHooks& hooks) const;
    // 00421530/00421f90 ↔ CD 004490b0: 두 _ftol의 하위 비트를 합친 signed DWORD를 float 수치로 저장한다.
    // 반환값은 이벤트 0x2692의 payload다. 등록/실행·끝 칸 변환(004215d0)은 별도 연결이 필요하다.
    static float DelayedFallPayload(float x,float y);
private:
    // 원본 extra(패치 +40/CD +35)의 abstract·buried만 확인한다.
    bool Ordinary(Sid sid) const;
    // 0/풀 밖 자기 번호는 호스트에서 역참조하기 전에 거부한다. 참조 대상은 원본처럼 조용히 무효로 판정한다.
    std::span<const std::uint8_t> Object(Sid sid) const;
    const SidPool& pool_;
    std::span<const RiftTypeRecord> types_;
};
}
