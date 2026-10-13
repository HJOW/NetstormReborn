// 복원된 섬·다리·noIsland 삭제 훅을 공통 삭제와 연결한다. 원본에 새 게임 규칙을 더하지 않는 호스트 분배기다.
#pragma once
#include "o/RawBridgeLifecycle.h"
#include "o/RawIslandLifecycle.h"
#include "o/RawSquidDestroy.h"

namespace netstorm::o {
// 프로세스·재귀 삭제·아직 복원하지 않은 표시/소리 경계는 호출자가 공급한다.
struct SurfaceLifecycleHooks {
    std::function<void(Sid,float,float)> scheduleFall; // 실제 ScheduleBridgeFall로 연결한다.
    std::function<void(Sid,std::uint32_t)> destroy; // 가상 +0x10이다. DestroyDispatch와 ProcessHost::Hooks로 재진입한다.
    std::function<void(Sid)> fallWalker; // 섬 표면과 다리의 가상 walker 낙하를 공유한다.
    std::function<void(const BridgeLifecycleEvent&)> bridgeEffect; // 파편 생성 요청과 낙하 소리만 전달한다.
};
// 복원된 섬·다리·noIsland 삭제 훅을 공통 삭제와 연결한다. 원본에 새 게임 규칙을 더하지 않는 호스트 분배기다.
class RawSurfaceLifecycle {
public:
    // 풀·이웃 탐색기·공통 훅의 대상은 더 오래 살아야 한다. 같은 풀/타입으로 내부 삭제 탐색기를 만든다.
    RawSurfaceLifecycle(const SidPool& pool,const RawSquidNeighbors& neighbors,std::uint32_t connectorType,
        SquidDestroyHooks common,SurfaceLifecycleHooks hooks);
    // 내부 콜백이 this를 참조하므로 복사/이동 뒤 다른 인스턴스를 가리키는 연결을 금지한다.
    RawSurfaceLifecycle(const RawSurfaceLifecycle&)=delete;
    RawSurfaceLifecycle& operator=(const RawSurfaceLifecycle&)=delete;
    RawSurfaceLifecycle(RawSurfaceLifecycle&&)=delete;
    RawSurfaceLifecycle& operator=(RawSurfaceLifecycle&&)=delete;
    // ProcessHost의 asset 훅으로 공급한다. form·종속 처리는 ProcessHost가 먼저 받고 자산만 여기로 넘긴다.
    // 같은 인스턴스의 다리 탐색은 중첩 사용하지 않는다. 연결 객체 삭제는 공통 훅만 사용하므로 탐색을 덮지 않는다.
    SquidDestroyHooks Hooks();
private:
    // 실제 가상 표 기록값으로 bridge pre/post, island pre, noIsland post를 분배한다. 나머지는 공통 훅이다.
    void Emit(const SquidDestroyEvent& event);
    // 연결 객체는 실제 삭제, Base는 공통 훅, walker는 낙하 훅, 파편/소리는 외부 효과로 연결한다.
    void EmitBridge(const BridgeLifecycleEvent& event);
    // pool_: 객체 슬롯의 생성·조회·반납에 사용하는 SID 풀.
    const SidPool& pool_;
    // connectorType_: 연결부 타입.
    std::uint32_t connectorType_;
    // common_: 파생 타입 사이에 공유하는 기본 처리기 또는 상태.
    SquidDestroyHooks common_;
    // hooks_: 외부 저장소 조회와 가상 동작을 연결하는 콜백 묶음 (SurfaceLifecycleHooks).
    SurfaceLifecycleHooks hooks_;
    // finder_: 검색기.
    RawSquidFinder finder_;
    // bridge_: 다리.
    RawBridgeLifecycle bridge_;
    // island_: 섬.
    RawIslandLifecycle island_;
    // bridgeHooks_: 탐색기는 begin의 첫 번호와 next의 후속 번호를 내준다. 0은 끝이며 순서/동적 필터는 호출자 계약이다 (BridgeLifecycleHooks).
    BridgeLifecycleHooks bridgeHooks_;
};
}
