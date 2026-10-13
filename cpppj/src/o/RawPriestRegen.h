// 사제 이벤트 0x25a의 실제 HP 회복과 중립 후처리/패치 추적 순서를 복원한다.
#pragma once
#include "o/RawPriestPostPop.h"
#include "o/RawPriestState.h"

namespace netstorm::o {
// 사제 회복 패치 처리 중 조회·갱신하는 상태 묶음이다.
struct PriestRegenPatchState {
    std::uint32_t sample{}; // 00540cb8: 원본 Next(9)의 결과다.
    std::int32_t sequence{}; // 0054d4e0: 3씩 증가하고 9600을 한 번 뺀다.
    std::int32_t observedSequence{}; // 00568af8: 외부 검증 함수 뒤 현재 값을 읽는다.
    std::uint32_t changedSample{}; // 005318ec: 추적 조건이 참이면 현재 sample을 쓴다.
    std::uint32_t remaining{}; // 005c89c4: 0이 아니면 측정 후 하나 줄인다.
    std::uint32_t gate{}; // 00594fd0: 0이면 changedSample을 쓴다.
    std::int16_t sentinel{}; // 00531890: DWORD가 아닌 signed WORD의 -1 조건이다.
};
// 사제 회복 처리 중 조회·갱신하는 상태 묶음이다.
struct PriestRegenState {
    bool neutralBlocked{}; // 005c89b8 / CD 005178d4: 중립 사제 후처리를 차단하는 0 아님 조건이다.
    bool modeBlocked{}; // 00594fa4 / CD 0052e170: 두 번째 차단 전역이다. 게임 내 의미는 미확정이다.
    // patch: 10.78 패치판의 상태 또는 패치판 검사용 자료.
    PriestRegenPatchState patch;
};
// 사제 회복 외부 동작과 저장소 조회를 연결하는 콜백 묶음이다.
struct PriestRegenHooks {
    std::function<void(Sid)> refresh; // 회복을 시도한 뒤 vtable +0x88을 부른다. HP가 거부되어도 호출한다.
    std::function<void(Sid)> damageable; // 0044c060 / CD 00462480 후처리의 전체 효과는 외부 경계다.
    std::function<std::uint32_t(std::uint32_t)> shape; // 00488fe0 / CD 0047d020(type)의 결과를 공급한다.
    std::function<bool(Sid,std::uint32_t)> occupied; // 004b1a90 / CD 004eb310(sid, shape, 0, 0)의 0 아님 결과다.
    std::function<void(Sid)> neutralAction; // 조건을 만족하고 점유 결과가 0일 때 00491db0 / CD 0040d030을 부른다.
    std::function<void()> patchAudit; // 패치 전용 00491680. 추적/SP 전역 효과는 호출자가 공급한다.
    std::function<double()> measurement; // 패치 전용 0040ee90의 x87 측정 결과다. remaining!=0일 때만 읽는다.
};
// 사제 이벤트 0x25a의 실제 HP 회복과 중립 후처리/패치 추적 순서를 복원한다.
class RawPriestRegen {
public:
    // 동일 월드의 HP/상태 모듈과 전역 난수를 받는다. 외부 공간/표시 경계는 객체보다 오래 살아야 한다.
    RawPriestRegen(SidPool& pool,const SquidReward& hp,const RawPriestState& priest,
        const PriestHitPointMode& mode,PriestHitPointHooks space,GameRandom& random,
        PriestRegenState& state,PriestRegenHooks hooks);
    // 회복 이벤트 분기만 실행한다. count는 원본도 읽지 않으며 반환 payload 비트를 유지한다.
    float Handle(Sid sid,std::uint32_t event,std::uint32_t count,float payload) const;
    // 분배기를 만들 때 풀 불일치를 막는다.
    const SidPool& Pool() const;
private:
    // pool_: 객체 슬롯의 생성·조회·반납에 사용하는 SID 풀.
    SidPool& pool_;
    // hp_: 객체의 HP를 조회·변경하는 처리기 또는 HP 값.
    const SquidReward& hp_;
    // priest_: 사제 상태를 조회·변경하는 처리기 또는 사제 경로 여부.
    const RawPriestState& priest_;
    // mode_: 동작 분기를 선택하는 모드.
    const PriestHitPointMode& mode_;
    // space_: 공간.
    PriestHitPointHooks space_;
    // random_: 원본 FUN_004558c0: 상태 0이면 0x0bad0bad로 바꾼 뒤 상태*0x10003+3의 상위 16비트를 limit로 나눈 나머지를 쓴다 (GameRandom).
    GameRandom& random_;
    // state_: 처리 중 조회·갱신하는 상태 (PriestRegenState).
    PriestRegenState& state_;
    // hooks_: 외부 저장소 조회와 가상 동작을 연결하는 콜백 묶음 (PriestRegenHooks).
    PriestRegenHooks hooks_;
};
// 실제 사제 vtable의 0x25a만 연결한다. 다른 사제 이벤트는 fallback 없으면 미복원 예외다.
// 비사제는 fallback 또는 기존 base의 payload 반환으로 넘긴다.
RegularHandler MakePriestRegenHandler(const SidPool& pool,const RawPriestRegen& regen,RegularHandler fallback={});
}
