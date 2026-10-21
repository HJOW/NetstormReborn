// 원본 AI 전략 말미의 같은 지갑·SP 정리004138c0을 복원한다.
#pragma once
#include "o/RawAiBrain.h"
#include "o/SpStore.h"

namespace netstorm::o {
// 원본이 각 분기에서 현재 다시 조회하는 미션/사제 상태다.
struct AiEconomyHooks {
    std::function<bool()> demo; //00463b50의 현재 데모 여부다.
    std::function<bool(std::uint32_t)> captured; //00492340의 현재 참여 플래그와 사제 ownerState1이다.
};
// 별도 현금 사본이나 보상 가산 없이 같은 본체+2d8과 원본 분산 SP 저장소를 동기화한다.
class RawAiEconomy {
public:
    RawAiEconomy(RawAiBrainMemory& memory,ScrambledSpStore& sp,const double& now,AiEconomyHooks hooks); //같은 실제 저장소와 현재 시각을 빌린다.
    void Reconcile(std::uint32_t token) const; //004138c0:1초 지원/포획 예산·데모 하한 뒤 매번 실제 SP Set이다.
private:
    std::span<std::uint8_t> Body(std::uint32_t token) const; //현재 살아 있는0xb08 본체를 확인한다.
    std::uint32_t Field(std::uint32_t token,std::size_t offset) const; //하위 뒤 같은 본체의 현재 DWORD를 읽는다.
    void Field(std::uint32_t token,std::size_t offset,std::uint32_t value) const; //지정 DWORD만 쓰고 주변을 보존한다.
    void DemoFloor(std::uint32_t token) const; //데모 조회 뒤 현재 signed 지갑을 다시 읽고1190 이상으로 유지한다.
    void CaptureBudget(std::uint32_t token,std::uint32_t owner) const; //현재 포획 판정 뒤 최대비용+2200 미만이면 보정한다.
    RawAiBrainMemory& memory_; //생성/통지/보상과 같은 본체 저장소다.
    ScrambledSpStore& sp_; //같은 게임 난수/추적 상태를 사용하는 실제 SP 저장소다.
    const double& now_; //0055b4d0의 현재 QWORD 시각이다.
    AiEconomyHooks hooks_; //각 원문 위치에서 호출하는 현재 미션/사제 getter다.
};
}
