// 공통 pre/postDestroy의 비전투 선택·공급/작업장 목록·타입 통계·비용 장부를 복원한다.
#pragma once
#include "o/RawSquidDestroy.h"
#include "o/SquidPostPop.h"
#include "o/SquidReward.h"

namespace netstorm::o {
// UI 전체·삭제 보상/SP·실제 소리 출력은 명시적인 외부 효과로 연결한다.
enum class SquidDeletionEffect { ClearSelection,Refund,UnitLostSound };
// 객체 삭제 발생한 동작의 대상과 인자를 기록하는 이벤트이다.
struct SquidDeletionEvent {
    // effect: 효과.
    SquidDeletionEffect effect{};
    // sid: 원본 풀에서 객체를 찾는 SID 번호.
    Sid sid{};
    std::uint32_t recipient{}; // Refund의 flags 16..19비트. 원본 보상 함수의 마지막 인자는 0이다.
};
// 객체 삭제 처리 중 조회·갱신하는 상태 묶음이다.
struct SquidDeletionState {
    // selected: 현재 선택한 객체·컨트롤의 번호 또는 토큰.
    Sid selected{};
    std::uint32_t placementType{}; // abstract/buried 선택 객체가 남기는 배치 타입. 0이면 없음.
    float placementX{},placementY{},lostX{},lostY{}; // 원본 좌표 비트를 그대로 보존한다.
    bool unitLostSuppressed1{},unitLostSuppressed2{}; // 원본 두 전역의 소리/좌표 기록 억제 조건.
    std::uint32_t silentType{}; // 패치판의 unitLost 소리 제외 타입. CD에는 이 분기가 없다.
    std::uint32_t regionMask{0x50444200}; // 두 PE의 초기 postDestroy 대상 전역. 실제 분기는 이 값에 8을 OR한다.
    bool rebuildGraph{}; // 원본 삭제 분할 정책 전역. GraphRecovery의 소진 복구와 구별한다.
    std::uint8_t specialSurfaceType{157},specialSurfaceReplacementType{162},largeSurfaceType{162}; // 특수 H 치환·9 감소 타입 전역.
    float* sharedLostX{};float* sharedLostY{}; // Native와 같은 float 폭의 손실 위치를 직접 빌린다.
    float& LostX() { return sharedLostX ? *sharedLostX : lostX; } // 음향 효과 뒤 현재 저장소를 읽고 쓴다.
    const float& LostX() const { return sharedLostX ? *sharedLostX : lostX; } // const 관측도 같은 실제 X를 읽는다.
    float& LostY() { return sharedLostY ? *sharedLostY : lostY; } // X와 Y를 별개의 실제 주소에 보존한다.
    const float& LostY() const { return sharedLostY ? *sharedLostY : lostY; } // const 관측도 같은 실제 Y를 읽는다.
    const std::uint32_t* sharedLoadingDepth{};const bool* sharedFortMode{}; // Native 소리/좌표 억제의 현재005c89b8 DWORD와00594fb8 bool이다.
    const std::uint32_t* sharedSilentType{}; // 현재005412a4의 레인 바리케이드 TYPE를 소리 관문에서만 읽는다.
    const std::uint32_t* sharedSpecialSurfaceType{};const std::uint32_t* sharedReplacementSurfaceType{};const std::uint32_t* sharedLargeSurfaceType{}; // 같은 noIsland/fakeSurface 전역을 Graph 분할이 현재 조회한다.
    bool LoadingSuppressed() const { return sharedLoadingDepth ? *sharedLoadingDepth!=0:unitLostSuppressed1; } // 원문 loadingDepth 비영을 bool 사본으로 바꾸지 않는다.
    bool FortSuppressed() const { return sharedFortMode ? *sharedFortMode:unitLostSuppressed2; } // 효과 뒤 현재 fort 모드를 다시 읽는다.
    std::uint32_t SilentType() const { return sharedSilentType ? *sharedSilentType:silentType; } // CD/독립 입력만 기존 타입을 사용한다.
    std::uint32_t SpecialSurfaceType() const { return sharedSpecialSurfaceType ? *sharedSpecialSurfaceType:specialSurfaceType; } // Graph H 치환 대상의 현재 전체 DWORD다.
    std::uint32_t ReplacementSurfaceType() const { return sharedReplacementSurfaceType ? *sharedReplacementSurfaceType:specialSurfaceReplacementType; } // 선택된 H 프레임의 실제 현재 대체 타입이다.
    std::uint32_t LargeSurfaceType() const { return sharedLargeSurfaceType ? *sharedLargeSurfaceType:largeSurfaceType; } //9칸 감소 대상을 같은 현재 fakeSurface로 판단한다.
};
// 객체 삭제 외부 동작과 저장소 조회를 연결하는 콜백 묶음이다.
struct SquidDeletionHooks {
    // emit: 관찰한 이벤트를 외부 처리기에 전달하는 콜백.
    std::function<void(const SquidDeletionEvent&)> emit;
    std::function<void(const SquidDestroyEvent&)> forward; // 종속 가상 메서드/전파를 별도로 연결한다.
    std::function<std::uint32_t()> selectedRaw; //Native Pre가실제비교위치에서읽는현재Client전체DWORD이며기존typed Sid state는보존한다.
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> rememberPlacement; // Native004d5440의 실제 UI 기억점/TYPE를 기록한다. 독립 입력은 기존 상태 기록을 유지한다.
    std::function<std::uint32_t()> localOwner; // Native의 같은 현재00540c70을 각 비교 위치에서 읽는다.
};
// 공통 pre/postDestroy의 비전투 선택·공급/작업장 목록·타입 통계·비용 장부를 복원한다.
class SquidDestroyLifecycle {
public:
    // Pop과 같은 장부/풀을 사용한다. 풀·원본 삭제 어댑터·상태의 수명은 이 객체보다 길어야 한다.
    SquidDestroyLifecycle(SidPool& pool,std::span<const RiftTypeRecord> types,SquidPostPopState& bookkeeping,
        SquidDeletionState& state,RawSquidDestroy& destroy,SquidDeletionHooks hooks,RawGraph* graph=nullptr,SquidReward* reward=nullptr);
    // 독립 typed/CD는 손상 목록·미연결 효과를 사전검사한다. 실제 PATCH는 원문에 없는 선행 효과 없이 반환한다.
    void ValidatePre(Sid sid,std::uint32_t flags) const;
    void ValidatePost(Sid sid,std::uint32_t flags) const;
    // 004b0950 ↔ CD 004add20. 끝에서 기존 공통 삭제 어댑터의 pre 깊이를 감소시킨다.
    void PreDestroy(Sid sid,std::uint32_t flags);
    // 004b0840 ↔ CD 004adbe0. 비용은 Pre의 통계 억제와 무관하며 ordinary만 차감한다.
    void PostDestroy(Sid sid,std::uint32_t flags);
    // 공통 destroy의 실제 가상 훅으로 연결한다. 파생 다리 훅은 BasePre/BasePost에서 이 몸체를 호출한다.
    SquidDestroyHooks Hooks();
    void BindPatchAiNotice(std::function<void(std::uint32_t,Sid,bool)> notice); //PATCH004166e0의 같은 현재 Player/brain 삭제 통지다. 빈 함수는 수명 종료 때 분리한다.
private:
    // 매 효과 사이에 현재 raw 타입/상태를 다시 읽는다. 호스트에서 원본 vtable을 호출하지 않는다.
    std::span<const std::uint8_t> Object(Sid sid) const;
    std::span<const RiftTypeRecord> CurrentTypes() const; // 실제 PATCH는 같은 World의 현재 TYPE를 빌리고 미연결/CD는 기존 생성 시 사본을 사용한다.
    const RiftTypeRecord& Type(Sid sid) const;
    std::uint8_t Owner(Sid sid) const;
    bool Ordinary(Sid sid) const;
    bool LostNotice(Sid sid,std::uint32_t flags) const;
    std::uint32_t LocalOwner() const; // 실제 Native는 장기 Client getter를 사용하고 독립 typed는 기존 장부 BYTE를 쓴다.
    // encoded 비용은 postPop과 같은 인코딩 접두 결과를 해독하고 DWORD 감김을 보존한다.
    std::int32_t RemainingCost(Sid sid) const;
    // 선택 해제의 외부 UI 경계와 논리 선택 번호를 함께 처리한다.
    void ClearSelection(Sid sid);
    bool IsSelected(Sid sid) const; //실제Native getter는진단뒤선택비교위치에서만조회하고직접typed/CD는기존Sid state를사용한다.
    // 선택 처리 뒤·장부 억제 앞의 실제 분할/Free를 처리한다. validate이면 변경하지 않는다.
    void PreGraph(Sid sid,std::uint32_t flags,bool validate) const;
    // post의 주변 표면 Add는 비용 차감보다 먼저 실행된다.
    bool NeedsPostGraph(Sid sid,std::uint32_t flags) const;
    // pool_: 객체 슬롯의 생성·조회·반납에 사용하는 SID 풀.
    SidPool& pool_;
    // types_: 타입 번호로 속성·플래그를 조회하는 타입 표.
    std::vector<RiftTypeRecord> types_;
    // bookkeeping_: 객체 종류·소유자별 등록 목록과 개수를 관리하는 장부.
    SquidPostPopState& bookkeeping_;
    // state_: 처리 중 조회·갱신하는 상태 (SquidDeletionState).
    SquidDeletionState& state_;
    // destroy_: 객체 삭제의 전처리·공간 해제·후처리를 실행하는 처리기.
    RawSquidDestroy& destroy_;
    // hooks_: 외부 저장소 조회와 가상 동작을 연결하는 콜백 묶음 (SquidDeletionHooks).
    SquidDeletionHooks hooks_;
    RawGraph* graph_{}; // 풀/타입/공간을 생성 시 확인하며 연결 객체는 더 오래 살아야 한다.
    SquidReward* reward_{}; // 있으면 보상 사건을 외부로 내보내지 않고 실제 SP/지갑/샘 풀 계산을 수행한다. 풀과 장부가 같아야 한다.
    std::function<void(std::uint32_t,Sid,bool)> aiNotice_; // 공급 목록 제거 뒤·전체 작업장 제거 앞의 실제 삭제 통지다.
};
}
