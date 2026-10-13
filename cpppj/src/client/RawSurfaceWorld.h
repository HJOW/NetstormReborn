// 본섬 표면·noIsland·다리와 파생 받침을 실제 raw 풀/Graph/Kernel로 소유하는 새 월드 연결부다.
#pragma once
#include "o/GameClock.h"
#include "o/RawBridgeConnect.h"
#include "o/SquidDisplay.h"
#include "o/RawBridgeEvents.h"
#include "o/RawBuildingSpace.h"
#include "o/RawIslandPostPop.h"
#include "o/RawSurfaceLifecycle.h"
#include "o/RawSquidDestroyDispatch.h"
#include "o/SquidFactory.h"
#include "o/SquidFormDispatch.h"
#include "o/SquidFrameBinding.h"
#include "o/SquidOwner.h"
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <vector>

namespace netstorm::client {
// 저장/지형 어댑터에서 넘기는 초기 위치다. 등록 뒤 현재 값은 raw 슬롯만 읽는다.
struct SurfaceSeed { std::uint32_t type{},owner{};std::int32_t frame{};float x{},y{}; };
struct SurfaceSnapshot { o::Sid sid{};SurfaceSeed object; };
// 해상도 변경으로 Renderer가 교체되어도 현재 장치에 전달할 수 있는 표시 콜백이다.
struct SurfaceDisplayHooks {
    std::function<bool()> suppressed;
    std::function<void(o::SquidDisplayRect,std::uint32_t)> invalidate;
};
// 비표면 runtime이 참조하는 실제 월드 자료다. span은 원본 배열이며 풀/해시/장부의 사본을 만들지 않는다.
// 이 참조는 월드보다 오래 살 수 없고 연결 모듈 해제 전 ClearProcesses를 호출해야 한다.
struct RawWorldCore {
    o::SidPool& pool;
    o::SquidHash& hash;
    std::span<std::uint8_t> spots;
    std::span<const o::RiftTypeRecord> types;
    std::span<const o::RiftTypeFrames> frames;
    std::span<const o::SquidDisplayShape> shapes;
    o::SquidPostPopState& bookkeeping;
    o::SquidOwnerMode& ownerMode;
    o::SquidFactory& factory;
    o::SquidOwner& owner;
    o::SquidFrame& frame;
    o::SquidPop& pop;
    o::SquidUnpop& unpop;
    o::SquidPostPop& postPop;
    o::SquidDisplay& display;
    o::RawBuildingSpace& building;
    o::RawGraph& graph;
    o::RawSquidNeighbors& neighbors;
    o::RawSquidDestroy& destroy;
    o::RawSquidDestroyDispatch& virtualDestroy;
    o::RawSurfaceLifecycle& surfaces;
    o::RawBridgeEvents& bridgeEvents;
    o::SquidFormDispatch& forms;
    o::SquidProcessHost& processes;
    o::SquidProcessState& processState;
    o::Kernel& kernel;
};
// 표면의 기존 훅을 합성한 미션 runtime 경계다. 새 월드 생성 후 첫 객체 생성/Load 전에 한 번 연결한다.
struct RawWorldRuntimeHooks {
    o::SquidDestroyHooks assets;
    o::SquidAssetDestroy destroy;
    o::RegularHandler regular;
    o::ProcessFormTransmission transmit;
    std::function<bool(o::Sid)> walkerFall;
    std::function<void(std::uint32_t)> loadingDepth;
};
class RawSurfaceWorld {
public:
    // 프레임/타입/SHP를 소유하여 GUI 참조와 raw 조회가 같은 판본을 사용하게 한다.
    RawSurfaceWorld(o::OriginalEdition edition,std::span<const o::RiftTypeRecord> types,
        std::span<const o::RiftTypeFrames> frames,std::span<const o::SquidDisplayShape> shapes,
        o::BridgeConnectState links,std::uint32_t terrainType);
    // 내부 Kernel을 먼저 해제하여 ProcessForm이 pool보다 오래 살아남지 않게 한다.
    ~RawSurfaceWorld();
    RawSurfaceWorld(const RawSurfaceWorld&)=delete;
    RawSurfaceWorld& operator=(const RawSurfaceWorld&)=delete;
    // 본섬→행 우선 noIsland→다리 순으로 일반 Pop을 실행하고 마지막에 전체 Graph를 재구성한다.
    void Load(std::span<const SurfaceSeed> seeds);
    // 같은 풀에 실제 미션 모듈을 구성할 때 사용한다. 반환 참조의 수명은 이 월드와 같다.
    RawWorldCore Core();
    // 같은 저장소를 사용하는 자산/Regular/Form 전송/낙하/로딩 경계를 설치한다. 미연결 콜백을 생략하지 않는다.
    void BindRuntime(RawWorldRuntimeHooks hooks);
    // 원본 Fort 순서대로 Create/필드 저장/Pop하는 로더의 시작과 종료다. 종료 시 Graph를 재구성한다.
    void BeginLoad();
    void FinishLoad();
    // 로더와 실제 명령이 사용하는 같은 가상 생성·SHP 공간 등록·해제·재등록이다.
    o::Sid Create(std::uint32_t type,std::uint32_t flags=0);
    void Pop(o::Sid sid,float x,float y,std::uint32_t flags);
    void Unpop(o::Sid sid,std::uint32_t flags);
    void Repop(o::Sid sid,std::uint32_t flags);
    // 외부 연결 모듈을 해제하기 전에 예약 객체를 먼저 정리한다. 월드 자료는 유지한다.
    void ClearProcesses();
    // 파생 로더의 중첩 깊이를 표면과 비표면 몸체에 동일하게 전달한다.
    void SetLoadingDepth(std::uint32_t depth);
    // 원본 고정 게임 시각을 모든 Regular에 공급한다. 정지 중에는 커널을 실행하지 않는다.
    void RunFrame(o::FrameTime time,bool paused);
    // 카메라/viewport와 현재 Renderer의 표시 전달 경로를 갱신한다.
    void SetDisplay(o::SquidDisplayView view,SurfaceDisplayHooks hooks);
    // 지정 위치의 실제 객체/받침을 조회한다. 없으면 SID 0이다.
    o::Sid Find(float x,float y,std::uint32_t type) const;
    // vtable +0x10의 삭제 재정의를 거쳐 연결 객체·장부·프로세스를 함께 정리한다.
    bool Destroy(o::Sid sid,std::uint32_t flags=0);
    // 저장 건물의 소유자를 받침에 반영하는 기존 초기 월드 어댑터의 경계다.
    void SetSupportOwner(float x,float y,std::uint32_t owner);
    // 표시/검사에 현재 살아 있는 자산만 제공한다. ProcessForm은 제외한다.
    std::vector<SurfaceSnapshot> Objects() const;
    // 변경 요청은 한 번만 소비한다. 원본 표시가 전체 갱신으로 억제되어도 논리 변경은 남긴다.
    bool TakeChanged();
    // 깊이/Graph/프로세스/공간의 실제 상태를 읽기 전용으로 노출한다.
    std::string Report() const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
