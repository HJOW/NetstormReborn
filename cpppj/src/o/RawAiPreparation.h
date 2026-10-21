// 원본00416eb0의 현재 목표·지역·지갑·초기 월드 등록 준비를 같은 AI 본체에 적용한다.
#pragma once
#include "o/RawAiBrain.h"
#include <array>

namespace netstorm::o {
// 원본이 각 위치에서 다시 읽는 현재 전역의 저장소를 빌린다.
struct AiPreparationGlobals {
    const std::uint32_t& localOwner; //00540c70의 현재 로컬 소유자다.
    const std::uint32_t& playerKnown; //00595344의 현재 로컬 소유자 확정 여부다.
    const std::int32_t& noRegion; //0050c038의 현재 지역 없음 번호다.
    std::function<std::uint32_t()> invalidGraph; //00540410의 현재 무효 그래프를 읽는다. BYTE 저장소의 임시 DWORD를 참조하지 않는다.
    const double& now; //0055b4d0의 현재 QWORD 게임 시각이다.
};
// 기존 실제 월드 모듈과 장치 경계만 공급한다. 도달하지 않은 경계는 요구하지 않는다.
struct AiPreparationHooks {
    std::function<void()> missingEnemyAssertion; //00416eff의 enemyPlayerId 진단이며 반환 뒤 원문처럼 계속한다.
    std::function<std::uint32_t(std::uint32_t)> priest; //004921f0의 현재 소유 사제 SID다.
    std::function<std::uint32_t(std::int32_t)> temple; //0046f260의 현재 지역 신전 SID다.
    std::function<std::int32_t(Sid)> maxHitPoints; //004adcb0의 현재 최대 HP이며 현재 HP는 호출 전에 raw+1a에서 읽는다.
    std::function<double(std::array<float,2>,std::array<float,2>)> pointDistance; //0041cfc0의 두 raw 좌표 사이 거리다. 발 중심 보정을 추가하지 않는다.
    std::function<std::uint32_t(std::uint32_t)> graph; //00462bc0의 현재 SID 그래프 번호다.
    std::function<std::array<float,4>(std::int32_t)> regionBounds; //00432210→0040d900의 청크 int<<4→float 사각형이다.
    std::function<float(std::uint32_t)> storedSp; //0040ec50의 현재 분산 SP 읽기다.
    std::function<void(float)> addLocalSp; //0040eec0의 현재 로컬 SP·같은 AI 지갑·표시 갱신이다.
    std::function<std::uint32_t()> demo; //00463b50의 현재 미션+68이다.
    std::function<bool()> knowledgeOpen; //00445330의 현재005325c8==0 판정이다. 창 객체 존재 여부로 대체하지 않는다.
    std::function<bool()> blockingGumpOpen; //004641d0의 현재 루트 직계 차단 창 판정이다.
    std::function<void(std::uint32_t)> initialScan; //00415720 전체를 원래+25c 조건 위치에서 실행한다.
};
// 별도 값형 AI 상태 없이 실제0xb08 본체와 Player9·방향별 동맹을 공유한다.
class RawAiPreparation {
public:
    RawAiPreparation(RawAiBrainMemory& memory,const SidPool& pool,std::span<const std::uint8_t> players,
        std::span<const std::uint32_t> allies,AiPreparationGlobals globals,AiPreparationHooks hooks); //PATCH/Player1620/동맹81의 물리 계약만 먼저 확인한다.
    void Update(std::uint32_t token) const; //00416eb0 전체이며 원래 초기 스캔 시점을 보존한다.
    void SelectEnemy(std::uint32_t token) const; //00410300의 강제 대상 또는 첫 비동맹 사제 보유자를 선택한다.
    std::int32_t CountAttachedGeysers(std::uint32_t token) const; //00411a80의+324 포인터/+338 개수 조합을 그대로 읽는다.
    const SidPool& Pool() const; // 조립 시 같은 실제 풀인지 확인한다.
private:
    std::span<std::uint8_t> Body(std::uint32_t token) const; // 살아 있는 실제 크기의 본체를 다시 빌린다.
    std::uint32_t Field(std::uint32_t token,std::size_t offset) const; // 현재 본체 DWORD를 다시 읽는다.
    void Field(std::uint32_t token,std::size_t offset,std::uint32_t value) const; // 해당 DWORD만 바꾸고 주변 원문을 보존한다.
    std::uint32_t Player(std::uint32_t owner,std::size_t offset) const; // 현재 Player의 실제 DWORD를 읽는다.
    bool Allied(std::uint32_t owner,std::uint32_t target) const; // 원본 한 방향의 현재 동맹 칸만 읽는다.
    Sid PhysicalSid(std::uint32_t value) const; // DWORD SID를 물리 풀 검사 뒤에만 WORD로 좁힌다.
    RawAiBrainMemory& memory_; // 생성자·사건 통지·다른 전략이 공유하는 본체/목록 소유자다.
    const SidPool& pool_; // 사제의 현재 HP·좌표를 공급하는 실제 풀이다.
    std::span<const std::uint8_t> players_; // 같은9×180BYTE의 현재 Player 기록이다.
    std::span<const std::uint32_t> allies_; // 같은 방향별9×9 DWORD 표다.
    AiPreparationGlobals globals_; // 복사본 대신 현재 전역 참조를 유지한다.
    AiPreparationHooks hooks_; // 선택된 기존 실제 하위의 수명을 빌린다.
};
}
