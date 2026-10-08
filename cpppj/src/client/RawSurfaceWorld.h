// 본섬 표면·noIsland·다리와 파생 받침을 실제 raw 풀/Graph/Kernel로 소유하는 새 월드 연결부다.
#pragma once
#include "o/GameClock.h"
#include "o/RawBridgeConnect.h"
#include "o/SquidDisplay.h"
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
