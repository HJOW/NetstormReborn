// 원본 미션의 플레이어/팀 승패 조건과 한 번만 발생하는 스크립트 사건을 복원한다.
#pragma once
#include "o/RawPriestLookup.h"
#include "o/SquidReward.h"
#include <array>
#include <functional>
#include <string_view>

namespace netstorm::o {
// 원본 Player의 활성 플래그와 현재 신전 SID다. SP/생산 장부와 같은 월드에서 현재 값을 공급한다.
struct MissionOutcomePlayer {
    std::uint8_t flags{}; // +0x50의 0x22 중 하나라도 켜져 있으면 검사 대상이다.
    std::uint32_t temple{}; // 패치 +0x7c/CD +0x74. 0은 신전 없음이며 WORD로 먼저 줄이지 않는다.
};
// 플레이어마다 유지하는 원본 DWORD 사건 잠금이다. 참이 된 조건을 처리하면 1을 쓰며 자동 초기화하지 않는다.
struct MissionPlayerOutcomeLatches {
    std::uint32_t templeDead{},templeHalfDead{},priestDead{},priestCaptured{},priestSaved{};
};
// 미션의 현재 입력과 사건 잠금이다. 콜백이 바꾼 값은 원본의 다음 조회 시점에 다시 읽는다.
struct MissionOutcomeState {
    std::array<MissionOutcomePlayer,kPlayerCount+1> players{};
    std::array<std::array<std::uint32_t,kPlayerCount+1>,kPlayerCount+1> allies{}; // 두 방향이 모두 비제로여야 같은 팀이다.
    std::array<MissionPlayerOutcomeLatches,kPlayerCount+1> latches{};
    std::uint32_t goodTeamDead{},badTeamDead{},failed{}; // 미션 +0x180/+0x184/+0x188.
    std::uint32_t localPlayer{1}; // 패치 00540c70/CD 0050f6c8. DWORD 입력이다.
    std::uint32_t demo{}; // 미션 +0x68. 비제로면 사건 이름 앞에 Demo를 붙인다.
    std::uint32_t tellBlocked{}; // 패치 00565dd4/CD 00535e8c. 0에서만 사건 처리/잠금을 확정한다.
};
// 이 모듈 밖의 실제 게임 효과다. foundSection만 false여도 gate가 열려 있으면 사건 잠금을 확정한다.
struct MissionOutcomeHooks {
    std::function<std::int32_t(Sid)> maxHitPoints; // 004adcb0/CD 004aed30. 같은 풀의 SquidReward 몸체를 연결한다.
    std::function<std::uint32_t(float,float)> regionOwner; // 0046f770/CD 004bde10. 사제의 현재 좌표가 속한 소유자다.
    std::function<bool(std::string_view)> section; // 00482de0/CD 00480de0. 사건 섹션 실행/발견 여부다.
    std::function<void()> closeTransientGumps; // 00466710/CD 00492c20. 해당 flags의 현재 임시 Gump를 정리한다.
    std::function<void()> onSection; // 미션 가상 +0x28. 사건 섹션이 존재한 뒤에 호출한다.
    std::function<void(std::uint32_t)> invalidPlayerAssert; // 패치의 범위 assert 기록만 공급한다. CD는 호출하지 않는다.
};
// 004c36c0/CD 00410380, 004c3940/CD 00410a90, 004922e0/CD 0040da20의 실행 계층이다.
class RawMissionOutcome {
public:
    // 참조 대상은 객체보다 오래 살아야 한다. 현재 사제 목록/ownerState를 그대로 공유하고 복사하지 않는다.
    RawMissionOutcome(const SidPool& pool,const PriestPostPopState& priests,MissionOutcomeState& state,MissionOutcomeHooks hooks);
    // inactive/범위 밖은 사망으로 본다. 활성 소유자의 상태가 2일 때만 true이며 raw dead/void 검사를 덧붙이지 않는다.
    bool PriestDead(std::uint32_t player) const;
    // 범위 밖은 true, inactive는 false, 활성 소유자는 상태가 1일 때만 true다.
    bool PriestCaptured(std::uint32_t player) const;
    // 조건 참/잠금0/gate열림일 때만 section→임시 UI 정리→가상 통지→잠금1 순서다. 성공 여부는 section 발견 여부와 별개다.
    // 콜백 안의 재진입을 막지 않는다. 잠금은 콜백이 끝난 뒤 원래 참조에 쓴다.
    bool Event(std::uint32_t& latch,bool condition,std::string_view name) const;
    // 플레이어 1~8의 halfTemple/deadTemple/deadPriest/captured/saved 순서 뒤 팀/로컬 실패를 검사한다.
    // 신전 파괴만으로 승리하지 않으며 사제 ownerState와 두 방향 동맹으로 적 팀의 생존을 집계한다.
    void Check() const;
    // 후속 연결에서 같은 풀의 객체/최대 HP 몸체를 사용하는지 검증한다.
    const SidPool& Pool() const;
private:
    // 패치만 범위 assert 효과를 요청한 뒤 양 판본의 정상 범위 여부를 돌려준다.
    bool ValidPlayer(std::uint32_t player) const;
    // DWORD SID를 범위 검사한 뒤 해당 raw 슬롯을 읽는다. 0번 슬롯은 원본처럼 유효하다.
    std::span<const std::uint8_t> Raw(std::uint32_t sid) const;
    // 현재 신전 존재/HP 조건이다. 패치는 HP를 max 조회 전, CD는 조회 후 읽는 순서를 보존한다.
    bool TempleHalfDead(std::uint32_t player) const;
    // 현재 신전 없음 또는 raw dead bit2만 읽는다. free/void를 추가 조건으로 사용하지 않는다.
    bool TempleDead(std::uint32_t player) const;
    // 현재 사제 목록에서 찾은 좌표의 영역 소유자와 callback 직전에 저장한 로컬 번호를 비교한다.
    bool PriestSaved(std::uint32_t player) const;
    const SidPool& pool_;
    const PriestPostPopState& priests_;
    MissionOutcomeState& state_;
    MissionOutcomeHooks hooks_;
    RawPriestLookup lookup_;
};
// 최대 HP만 같은 풀의 실제 모듈로 연결한다. UI/스크립트/영역 효과는 호출자가 채운 그대로 둔다.
MissionOutcomeHooks MakeMissionOutcomeHooks(const SidPool& pool,const SquidReward& reward,MissionOutcomeHooks hooks);
}
