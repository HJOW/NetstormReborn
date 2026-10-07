// 공통 pre/postDestroy의 비전투 선택·공급/작업장 목록·타입 통계·비용 장부를 복원한다.
#pragma once
#include "o/RawSquidDestroy.h"
#include "o/SquidPostPop.h"

namespace netstorm::o {
// UI 전체·삭제 보상/SP·실제 소리 출력은 명시적인 외부 효과로 연결한다.
enum class SquidDeletionEffect { ClearSelection,Refund,UnitLostSound };
struct SquidDeletionEvent {
    SquidDeletionEffect effect{};
    Sid sid{};
    std::uint32_t recipient{}; // Refund의 flags 16..19비트. 원본 보상 함수의 마지막 인자는 0이다.
};
struct SquidDeletionState {
    Sid selected{};
    std::uint32_t placementType{}; // abstract/buried 선택 객체가 남기는 배치 타입. 0이면 없음.
    float placementX{},placementY{},lostX{},lostY{}; // 원본 좌표 비트를 그대로 보존한다.
    bool unitLostSuppressed1{},unitLostSuppressed2{}; // 원본 두 전역의 소리/좌표 기록 억제 조건.
    std::uint32_t silentType{}; // 패치판의 unitLost 소리 제외 타입. CD에는 이 분기가 없다.
    std::uint32_t regionMask{0x50444200}; // 두 PE의 초기 postDestroy 대상 전역. 실제 분기는 이 값에 8을 OR한다.
    bool rebuildGraph{}; // 원본 삭제 분할 정책 전역. GraphRecovery의 소진 복구와 구별한다.
    std::uint8_t specialSurfaceType{157},specialSurfaceReplacementType{162},largeSurfaceType{162}; // 특수 H 치환·9 감소 타입 전역.
};
struct SquidDeletionHooks {
    std::function<void(const SquidDeletionEvent&)> emit;
    std::function<void(const SquidDestroyEvent&)> forward; // 종속 가상 메서드/전파를 별도로 연결한다.
};
class SquidDestroyLifecycle {
public:
    // Pop과 같은 장부/풀을 사용한다. 풀·원본 삭제 어댑터·상태의 수명은 이 객체보다 길어야 한다.
    SquidDestroyLifecycle(SidPool& pool,std::span<const RiftTypeRecord> types,SquidPostPopState& bookkeeping,
        SquidDeletionState& state,RawSquidDestroy& destroy,SquidDeletionHooks hooks,RawGraph* graph=nullptr);
    // 손상 목록·미연결 Graph/AI 효과·비유한 비용·누락 외부 효과를 장부 쓰기 전에 거부한다.
    void ValidatePre(Sid sid,std::uint32_t flags) const;
    void ValidatePost(Sid sid,std::uint32_t flags) const;
    // 004b0950 ↔ CD 004add20. 끝에서 기존 공통 삭제 어댑터의 pre 깊이를 감소시킨다.
    void PreDestroy(Sid sid,std::uint32_t flags);
    // 004b0840 ↔ CD 004adbe0. 비용은 Pre의 통계 억제와 무관하며 ordinary만 차감한다.
    void PostDestroy(Sid sid,std::uint32_t flags);
    // 공통 destroy의 실제 가상 훅으로 연결한다. 파생 다리 훅은 BasePre/BasePost에서 이 몸체를 호출한다.
    SquidDestroyHooks Hooks();
private:
    // 매 효과 사이에 현재 raw 타입/상태를 다시 읽는다. 호스트에서 원본 vtable을 호출하지 않는다.
    std::span<const std::uint8_t> Object(Sid sid) const;
    const RiftTypeRecord& Type(Sid sid) const;
    std::uint8_t Owner(Sid sid) const;
    bool Ordinary(Sid sid) const;
    bool LostNotice(Sid sid,std::uint32_t flags) const;
    // encoded 비용은 postPop과 같은 인코딩 접두 결과를 해독하고 DWORD 감김을 보존한다.
    std::int32_t RemainingCost(Sid sid) const;
    // 선택 해제의 외부 UI 경계와 논리 선택 번호를 함께 처리한다.
    void ClearSelection(Sid sid);
    // 선택 처리 뒤·장부 억제 앞의 실제 분할/Free를 처리한다. validate이면 변경하지 않는다.
    void PreGraph(Sid sid,std::uint32_t flags,bool validate) const;
    // post의 주변 표면 Add는 비용 차감보다 먼저 실행된다.
    bool NeedsPostGraph(Sid sid,std::uint32_t flags) const;
    SidPool& pool_;
    std::vector<RiftTypeRecord> types_;
    SquidPostPopState& bookkeeping_;
    SquidDeletionState& state_;
    RawSquidDestroy& destroy_;
    SquidDeletionHooks hooks_;
    RawGraph* graph_{}; // 풀/타입/공간을 생성 시 확인하며 연결 객체는 더 오래 살아야 한다.
};
}
