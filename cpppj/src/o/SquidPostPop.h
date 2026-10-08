// 공통 postPop의 비용 집계·생산 목록·타입 통계를 raw SID에 연결한다.
#pragma once
#include "o/SidPool.h"
#include "o/Player.h"
#include "o/RawGraph.h"
#include <array>
#include <functional>

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
    // 같은 판본의 풀·상태·선택적인 raw Graph를 받는다. 타입을 복사하며 상태/Graph 수명은 더 길어야 한다.
    SquidPostPop(SidPool& pool,std::span<const RiftTypeRecord> types,SquidPostPopState& state,RawGraph* graph=nullptr,
        std::function<void(Sid)> bridgeConnector={});
    // 다리 postPop(00422150 / CD 00449890)의 접두 부분이다. flag 1은 extra 비트 2를 켜며,
    // flag 1 또는 (flag 4이고 abstract 아님)이면 004213b0/CD 00448b00의 연결 효과를 요청한다.
    // 연결 효과는 명시적 외부 경계이며 누락되면 쓰기 전에 거부한다.
    static void BridgePrefix(SidPool& pool,Sid sid,std::uint32_t flags,const std::function<void(Sid)>& connector);
    // 다리 전용 후처리는 connector가 연결된 인스턴스만 지원한다. 다른 파생 가상 함수를 허용하지 않는다.
    bool HandlesBridge(Sid sid) const;
    // 섬 받침 postPop(004421c0 / CD 004d01d0)의 접두(종유석 생성·소유자·Pop·연결 순회)를 연결한다.
    // 인자는 섬 받침 번호와 Activate가 넘긴 flags다. 연결한 인스턴스만 섬 받침의 가상 표를 받는다.
    void SetIslandPrefix(std::function<void(Sid,std::uint32_t)> prefix);
    // 섬 받침의 가상 표이고 접두 효과가 연결돼 있는가.
    bool HandlesIsland(Sid sid) const;
    // noIsland postPop(004423b0 / CD 004d2790)의 접두를 연결한다. RawIslandPostPop::Prefix를 공급한다.
    void SetSurfacePrefix(std::function<void(Sid,std::uint32_t)> prefix);
    // noIsland 가상 표이며 접두가 연결돼 있는가.
    bool HandlesSurface(Sid sid) const;
    // 이 인스턴스가 처리할 수 있는 파생 postPop(다리·섬 받침·noIsland)인가. Pop이 공간 변경 전에 묻는다.
    bool HandlesDerived(Sid sid) const;
    // 공간 변경 전 연결된 그래프의 최종 Pop 상태·미복원 영역/AI/배치 효과·목록·비용을 확인한다.
    void Validate(Sid sid,std::uint32_t flags,const RawGraphPop* pop=nullptr) const;
    // 선택한 raw Graph가 Pop과 같은 해시·spot을 읽는지 공간 변경 전에 확인한다.
    void ValidateSpace(const SquidHash& hash,std::span<const std::uint8_t> spots) const;
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
    RawGraph* graph_{};
    std::function<void(Sid)> bridgeConnector_; // 연결 객체 생성/소유자 전파(004213b0). RawBridgeConnect::Connect를 잇는다.
    std::function<void(Sid,std::uint32_t)> islandPrefix_; // 섬 받침 postPop의 접두. RawBridgeConnect::IslandPostPopPrefix를 잇는다.
    std::function<void(Sid,std::uint32_t)> surfacePrefix_; // noIsland 최초 등록의 프레임/받침/소유자 효과.
};
}
