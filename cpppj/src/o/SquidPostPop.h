// 공통 postPop의 비용 집계·생산 목록·타입 통계를 raw SID에 연결한다.
#pragma once
#include "o/SidPool.h"
#include "o/Player.h"
#include "o/RawGraph.h"
#include <array>
#include <functional>

namespace netstorm::o {
class RawPriestPostPopTail;
class RawForcefieldPostPop;
class RawCarrierPostPop;
class RawAssetPostPop;
class RawFactoryPostPop;
class RawRunePostPop; // Rune166의 실제304/건설/공통후처리몸체를명시연결한다.
class RawDaisLifecycle; // 다이스165/제단164의 실제 최초예약·등록·공통후처리를 연결한다.
class RawDudePostPop; // Dude70의 실제 idle/population/I 프레임·공통후처리를 연결한다.
class RawPlayerWorld; // Player152의 실제 최초 기록·예약·공통후처리를 연결한다.
class RawBuriedPostPop; // buried112의 최초 예약/QA/공통후처리를 같은 풀에 연결한다.
class RawFlyerPostPop; // 맨 오 워의 실제 프로세스/그림자/공통 후처리를 연결한다.
// 객체 후처리 Pop 등록한 대상과 그 개수를 보관하는 목록이다.
struct SquidPostPopList {
    std::vector<std::uint32_t> entries; // 원본 미리 확보한 DWORD 배열. entries.size()가 capacity다.
    // count: 현재 항목 또는 실행 횟수.
    std::uint32_t count{};
    // 활성 항목만 읽으며 손상된 count는 거부한다.
    std::span<const std::uint32_t> Items() const;
};
// 객체 후처리 Pop 처리 중 조회·갱신하는 상태 묶음이다.
struct SquidPostPopState {
    // suppressed: 표시·등록 후처리를 억제할지.
    // graphsEnabled: Graph 활성 여부.
    // pendingPlacement: 보류 배치 여부.
    bool suppressed{},graphsEnabled{},pendingPlacement{};
    std::uint8_t localOwner{},invalidGraph{254}; // 원본 invalidGraphNum의 두 판본 초기값.
    std::array<bool,kPlayerCount+1> aiAttached{}; // AI 부착 경로는 아직 복원하지 않았다.
    std::int32_t totalCost{}; // 전역 비용 집계. Player의 현재 SP 차감과 구별한다.
    std::uint32_t productionDirty{},depth{}; // 공급/작업장 재계산 알림과 Activate/postPop 깊이.
    std::array<std::uint32_t,256> localCounts{},localSecondaryCounts{},globalCounts{}; // Totalmade의 로컬 두 표·전역 한 표. 모두 타입 번호로 접근한다.
    // providers: 공급원.
    // factories: 팩토리.
    SquidPostPopList providers,factories;
    // ownerFactories: 소유자 팩토리.
    std::array<SquidPostPopList,kPlayerCount+1> ownerFactories;
};
// Pop 이후 타입별 등록 목록과 표면 연결 상태를 갱신하는 처리기이다.
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
    // 완성한 사제 후처리를 명시 등록한다. 같은 풀이어야 하며 nullptr은 연결을 해제한다.
    // 후처리의 Carrier는 이 인스턴스의 PostPopBase에 연결해야 한다. 후처리가 공통 깊이를 한 번 감소시킨다.
    void SetPriestPostPop(const RawPriestPostPopTail* postPop);
    // 실제 사제 타입/가상 표이며 전체 후처리를 등록했는가.
    bool HandlesPriest(Sid sid) const;
    // 보호막의 최초 Regular 예약 접두를 같은 풀에 명시 등록한다. nullptr은 해제한다.
    void SetForcefieldPostPop(const RawForcefieldPostPop* postPop);
    // 실제 보호막 타입/가상 표이며 전용 접두가 등록돼 있는가.
    bool HandlesForcefield(Sid sid) const;
    // 실제 +0x20이 Damageable/Carrier 몸체인 가상 표를 같은 풀의 완성한 몸체에 연결한다.
    // 해당 몸체의 base 경계는 이 객체의 PostPopBase여야 하며 nullptr은 연결을 해제한다.
    void SetCarrierPostPop(const RawCarrierPostPop* postPop);
    // 가상 표가 직접 Carrier 후처리를 사용하며 그 전체 몸체가 연결됐는가.
    bool HandlesCarrier(Sid sid) const;
    // 가상 표가 직접 Damageable 후처리를 사용하며 그 전체 몸체가 연결됐는가.
    bool HandlesDamageable(Sid sid) const;
    // Gun/제네레이터/거주 구역/바리케이드/간헐천/신전의 실제 전용 몸체를 같은 풀에 연결한다.
    void SetAssetPostPop(const RawAssetPostPop* postPop);
    // 실제 가상 표의 전용 자산 후처리가 명시 등록돼 있는가.
    bool HandlesAsset(Sid sid) const;
    // 실제 Factory/훨리베이스 전체 후처리를 같은 풀에 등록한다. nullptr은 연결을 해제한다.
    void SetFactoryPostPop(const RawFactoryPostPop* postPop);
    // 해당 가상 표의 몸체와 같은 월드 연결이 모두 준비돼 있는지 확인한다.
    bool HandlesFactory(Sid sid) const;
    // Rune304/건설후처리를같은풀에등록한다. nullptr이면Rune의파생Pop를허용하지않는다.
    void SetRunePostPop(const RawRunePostPop* postPop);
    // 현재Rune타입/실제가상표이며전용전체몸체가등록돼있는가.
    bool HandlesRune(Sid sid) const;
    // 다이스/제단의 전체 최초 Pop 몸체를 같은 풀에 등록하며 nullptr은 등록을 해제한다.
    void SetDaisPostPop(const RawDaisLifecycle* postPop);
    // 실제 다이스/제단 타입과 생성자 가상표가 등록된 전체 몸체와 일치하는가.
    bool HandlesDais(Sid sid) const;
    // Dude의 실제 최초 등록 전체 몸체를 같은 풀에 등록하며 nullptr은 연결을 해제한다.
    void SetDudePostPop(const RawDudePostPop* postPop);
    // 현재 Dude 타입과 생성자 가상 표가 등록된 전체 몸체와 일치하는가.
    bool HandlesDude(Sid sid) const;
    // Player152의 실제 전체 후처리를 같은 풀에 등록하며 nullptr은 연결을 해제한다.
    void SetPlayerPostPop(const RawPlayerWorld* postPop);
    // 현재 Player 생성자 가상 표와 등록된 전체 몸체가 일치하는가.
    bool HandlesPlayer(Sid sid) const;
    void SetBuriedPostPop(const RawBuriedPostPop* postPop); // 실제 buried 몸체를 같은 풀에 등록하며 nullptr은 해제한다.
    bool HandlesBuried(Sid sid) const; // 원본 가상 표와 등록된 전체 몸체가 일치하는가.
    void SetFlyerPostPop(const RawFlyerPostPop* postPop); // 같은 풀의 맨 오 워 전체 몸체를 등록하며 nullptr은 해제한다.
    bool HandlesFlyer(Sid sid) const; // 실제0050a388만 전체 몸체로 분배한다.
    // 이 인스턴스가 처리할 수 있는 파생 postPop인가. Pop이 공간 변경 전에 묻는다.
    bool HandlesDerived(Sid sid) const;
    // 공간 변경 전 연결된 그래프의 최종 Pop 상태·미복원 영역/AI/배치 효과·목록·비용을 확인한다.
    void Validate(Sid sid,std::uint32_t flags,const RawGraphPop* pop=nullptr) const;
    // 복원한 파생 몸체가 공통 함수를 직접 부르기 전에 검사한다. 가상 표의 전체 postPop 지원 여부만 제외한다.
    // Pop/Activate의 가상 분배는 계속 Validate를 사용한다.
    void ValidateBase(Sid sid,std::uint32_t flags,const RawGraphPop* pop=nullptr) const;
    // 선택한 raw Graph가 Pop과 같은 해시·spot을 읽는지 공간 변경 전에 확인한다.
    void ValidateSpace(const SquidHash& hash,std::span<const std::uint8_t> spots) const;
    // 비전투 Activate에서 postPop을 호출하는 깊이 증가/감소를 보존한다. flags는 Pop이 정규화한다.
    void Activate(Sid sid,std::uint32_t flags);
    // 004b0d30 ↔ CD 004ae180의 공통 몸체. 명시적인 직접 호출은 depth를 한 단계 감소시킨다.
    void PostPop(Sid sid,std::uint32_t flags);
    // 004b0d30 / CD 004ae180의 비가상 직접 호출이다. 파생 흐름의 일부만 복원한 호스트에서도 호출할 수 있다.
    // raw/Graph/장부 보호 검사는 유지하며 Pop의 허용 가상 표를 늘리지 않는다.
    void PostPopBase(Sid sid,std::uint32_t flags);
    // Pop과 다른 풀을 연결하지 않도록 소유자를 제공한다.
    const SidPool& Pool() const;
private:
    const RawFlyerPostPop* flyerPostPop_{}; // 같은 현재 server/TYPE/공통 장부를 쓰는 장기 몸체다.
    const RawBuriedPostPop* buriedPostPop_{}; // 장기 현재 전투/로딩과 같은 공통 장부를 사용하는 전용 몸체다.
    // 패치 encoded cost와 CD float cost의 누적 절삭·DWORD 감김을 계산한다.
    std::int32_t TotalCost(std::size_t type) const;
    // pool_: 객체 슬롯의 생성·조회·반납에 사용하는 SID 풀.
    SidPool& pool_;
    // types_: 타입 번호로 속성·플래그를 조회하는 타입 표.
    std::vector<RiftTypeRecord> types_;
    // state_: 처리 중 조회·갱신하는 상태 (SquidPostPopState).
    SquidPostPopState& state_;
    // graph_: 서로 연결된 표면의 Graph 소속을 관리하는 처리기.
    RawGraph* graph_{};
    std::function<void(Sid)> bridgeConnector_; // 연결 객체 생성/소유자 전파(004213b0). RawBridgeConnect::Connect를 잇는다.
    std::function<void(Sid,std::uint32_t)> islandPrefix_; // 섬 받침 postPop의 접두. RawBridgeConnect::IslandPostPopPrefix를 잇는다.
    std::function<void(Sid,std::uint32_t)> surfacePrefix_; // noIsland 최초 등록의 프레임/받침/소유자 효과.
    const RawPriestPostPopTail* priestPostPop_{}; // 전체 wrapper다. 접두처럼 공통 장부를 추가 호출하지 않는다.
    const RawForcefieldPostPop* forcefieldPostPop_{}; // 예약 접두 뒤 공통 후처리는 이 인스턴스가 한 번 호출한다.
    const RawCarrierPostPop* carrierPostPop_{}; // 직접 Damageable/Carrier 가상 후처리다. 파생 재정의는 별도로 복원한다.
    const RawAssetPostPop* assetPostPop_{}; // 초기 요새 건물의 전체 후처리다. 내부에서 공통 깊이를 한 번 줄인다.
    const RawFactoryPostPop* factoryPostPop_{}; // 같은 해시/목록의 Factory wrapper를 실행하고 공통 깊이를 한 번 줄인다.
    const RawRunePostPop* runePostPop_{}; // 실제Rune예약/건설다음공통깊이를한번감소시키는전체몸체다.
    const RawDaisLifecycle* daisPostPop_{}; // 최초 Player 등록/제단 생성·21 예약 뒤 같은 공통 깊이를 한 번 줄인다.
    const RawDudePostPop* dudePostPop_{}; // idle/population/I 프레임 뒤 같은 공통 깊이를 한 번 줄이는 전체 몸체다.
    const RawPlayerWorld* playerPostPop_{}; // 같은9기록/SID·최초예약 뒤 공통 깊이를 한 번 줄이는 전체 몸체다.
};
}
