// 공통 postPop의 비용 집계·생산 목록·타입 통계를 raw SID에 연결한다.
#pragma once
#include "o/SidPool.h"
#include "o/Player.h"
#include <array>

namespace netstorm::o {
struct SquidPostPopList {
    std::vector<std::uint32_t> entries; // 원본 미리 확보한 DWORD 배열. entries.size()가 capacity다.
    std::uint32_t count{};
    // 활성 항목만 읽으며 손상된 count는 거부한다.
    std::span<const std::uint32_t> Items() const;
};
struct SquidPostPopState {
    bool suppressed{},graphsEnabled{},pendingPlacement{};
    std::uint8_t localOwner{},invalidGraph{254}; // 원본 invalidGraphNum의 두 판본 초기값.
    std::array<bool,kPlayerCount+1> aiAttached{}; // AI 부착 경로는 아직 복원하지 않았다.
    std::int32_t totalCost{}; // 전역 비용 집계. Player의 현재 SP 차감과 구별한다.
    std::uint32_t productionDirty{},depth{}; // 공급/작업장 재계산 알림과 Activate/postPop 깊이.
    std::array<std::uint32_t,256> localCounts{},localSecondaryCounts{},globalCounts{}; // Totalmade의 로컬 두 표·전역 한 표. 모두 타입 번호로 접근한다.
    SquidPostPopList providers,factories;
    std::array<SquidPostPopList,kPlayerCount+1> ownerFactories;
};
class SquidPostPop {
public:
    // 타입/SHP와 같은 판본의 공유 풀·후처리 상태를 받는다. 타입은 호출자 수명과 분리해 복사한다.
    SquidPostPop(SidPool& pool,std::span<const RiftTypeRecord> types,SquidPostPopState& state);
    // 공간 변경 전 미복원 그래프/AI/배치 효과·목록 손상·비용 범위를 확인한다.
    void Validate(Sid sid,std::uint32_t flags) const;
    // 비전투 Activate에서 postPop을 호출하는 깊이 증가/감소를 보존한다. flags는 Pop이 정규화한다.
    void Activate(Sid sid,std::uint32_t flags);
    // 004b0d30 ↔ CD 004ae180의 공통 몸체. 명시적인 직접 호출은 depth를 한 단계 감소시킨다.
    void PostPop(Sid sid,std::uint32_t flags);
    // Pop과 다른 풀을 연결하지 않도록 소유자를 제공한다.
    const SidPool& Pool() const;
private:
    // 패치 encoded cost와 CD float cost의 누적 절삭·DWORD 감김을 계산한다.
    std::int32_t TotalCost(std::size_t type) const;
    SidPool& pool_;
    std::vector<RiftTypeRecord> types_;
    SquidPostPopState& state_;
};
}
