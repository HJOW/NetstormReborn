// 본섬 표면·noIsland·다리와 파생 받침을 실제 raw 풀/Graph/Kernel로 소유하는 새 월드 연결부다.
#pragma once
#include "o/GameClock.h"
#include "o/RawAuthorityState.h"
#include "o/RawGraph.h"
#include "o/RawIslandRuntime.h"
#include "o/ChunkMap.h"
#include "o/RawPriestPostPop.h"
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
#include "o/SquidDestroyLifecycle.h"
#include "o/SquidOwner.h"
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <vector>

namespace netstorm::client {
class GameAssets; // 실제 Client가 먼저 읽은 자산의 정체성을 물리 저장소와 공유한다.
// Session보다 오래 사는 실제 Pool/공간/자산 배열이다. Runtime 모듈의 수명과 구별한다.
class RawSurfaceStorage {
private:
    std::shared_ptr<o::RawAuthorityState> authority_; // Pool의 현재 서버 포인터와 World 실행기보다 먼저 만들고 마지막에 반납한다.
public:
    // 대여 중에는 두 번째 월드가 같은 풀을 초기화하지 못하게 한다.
    class Use {
    public:
        ~Use(); // 월드의 모든 모듈을 반납한 뒤 대여만 해제한다. Kernel을 정리하지 않는다.
        Use(const Use&)=delete;Use& operator=(const Use&)=delete; // 동일 대여를 복사해 조기 해제하지 않는다.
    private:
        friend class RawSurfaceStorage;
        explicit Use(RawSurfaceStorage& storage); // 이미 검증한 단일 소유자의 반납 경계다.
        RawSurfaceStorage& storage_; // 실제 물리 소유자는 대여보다 오래 살아야 한다.
    };
    RawSurfaceStorage(o::OriginalEdition edition,std::span<const o::RiftTypeRecord> inputTypes,std::span<const o::RiftTypeFrames> inputFrames,std::span<const o::SquidDisplayShape> inputShapes,const GameAssets* source=nullptr,std::shared_ptr<o::RawAuthorityState> authority={}); // 배열은 최초 한 번만 확보한다.
    RawSurfaceStorage(const RawSurfaceStorage&)=delete;RawSurfaceStorage& operator=(const RawSurfaceStorage&)=delete; // 현재 모든 실행기가 빌리는 실제 물리를 복제하지 않는다.
    RawSurfaceStorage(RawSurfaceStorage&&)=delete;RawSurfaceStorage& operator=(RawSurfaceStorage&&)=delete; // Graph/Pool/공간 참조가 남은 객체의 주소를 이동하지 않는다.
    std::unique_ptr<Use> Acquire(); // 기존 Pool/해시/spot 및 fresh Graph/집계3표의 일반 설치 의미를 같은 물리에서 유지하며 TYPE 수정은 보존한다.
    std::unique_ptr<Use> AcquirePrepared(); // 명시 Native 정리/prefix 뒤 같은 Pool/Hash/spot/Graph/집계3표/모드/종류를 재초기화하지 않고 대여한다.
    bool Borrowed() const; // 현재 실제 월드가 물리를 빌렸는지 읽는다.
    std::uint64_t Generation() const; // 성공한 월드 대여 횟수이며 원본 게임 값으로 쓰지 않는다.
    const std::shared_ptr<o::RawAuthorityState>& Authority() const { return authority_; } // Client 전역/Session과 같은 실제 서버·권한 주소의 정체성이다.
    const GameAssets* Source() const; // Client 자산 정체성이다. 독립 raw 검사에서는 nullptr이다.
    std::vector<o::RiftTypeRecord> types; // 같은 수정 가능한 전역 TYPE이며 Session마다 원본 파일로 덮어쓰지 않는다.
    std::vector<o::RiftTypeFrames> frames;std::vector<o::SquidDisplayShape> shapes; // 자산보다 짧고 모든 Session보다 긴 프레임/셰이프 배열이다.
    std::vector<std::vector<o::FrameCode>> codes; // Graph의 동일 프레임 코드 배열이다.
    o::SidPool pool;o::SquidHash hash;std::vector<std::uint8_t> spots; // Reset(0)은 기존 SID 바이트/공간 배열의 주소를 유지한다.
    o::RawGraphStorage graph; // 기록255행과 flood4096DWORD만 장기 보유한다. 실행기/TYPE snapshot/복구 정책은 World 지역이다.
    o::RawTotalMadeStorage totalMade; // 현재 로컬/누적 로컬/전역256DWORD만 장기 보유하며 ownerFactories/로딩 정책은 World 지역이다.
    o::RawIslandStorage islands;o::ChunkMap chunks; // 같은3204BYTE 섬과 mode/크기/256×12BYTE 청크를 Session 전후에도 유지한다. 실행기/효과는 포함하지 않는다.
    std::array<std::uint32_t,o::kPlayerCount+1> priestOwnerState{}; //00595428..00595448의9DWORD 최초BSS0이다. 실제 사제 SID목록/count/지갑은 포함하지 않는다.
    o::SquidOwnerMode ownerMode; // 미니맵과 현재 월드가 같은 실제 세 모드 값을 읽는다.
    o::BridgeConnectState links; // 생산 UI와 현재 월드가 같은 실제 표면 종류 전역을 읽는다.
private:
    const GameAssets* source_;bool borrowed_{};std::uint64_t generation_{}; // 최초 자산 정체성·단일 대여·호스트 관측 번호다.
};
// 실제 보관 중인 자산에서 프레임/SHP를 한 번 구성한다. 게임이나 창은 실행하지 않는다.
std::shared_ptr<RawSurfaceStorage> MakeRawSurfaceStorage(const GameAssets& assets,std::shared_ptr<o::RawAuthorityState> authority={});
// 저장/지형 어댑터에서 넘기는 초기 위치다. 등록 뒤 현재 값은 raw 슬롯만 읽는다.
struct SurfaceSeed { /* type: 타입 표에서 객체 종류를 찾는 번호. */ /* owner: 객체나 지역을 소유한 플레이어 번호. */ std::uint32_t type{},owner{};/* frame: 타입에서 사용할 그림 프레임 번호. */ std::int32_t frame{};/* x: X 좌표. */ /* y: Y 좌표. */ float x{},y{}; };
// 표면 객체의 현재 SID와 직렬화할 배치 정보이다.
struct SurfaceSnapshot { /* sid: 원본 풀에서 객체를 찾는 SID 번호. */ o::Sid sid{};/* object: 해당 SID나 출력 항목에 대응하는 객체 자료. */ SurfaceSeed object; };
// 해상도 변경으로 Renderer가 교체되어도 현재 장치에 전달할 수 있는 표시 콜백이다.
struct SurfaceDisplayHooks {
    // suppressed: 현재 표시나 후처리를 억제하는 전역 상태를 조회하는 콜백.
    std::function<bool()> suppressed;
    // invalidate: 변경한 영역의 재그리기를 요청하는 콜백.
    std::function<void(o::SquidDisplayRect,std::uint32_t)> invalidate;
};
// 비표면 runtime이 참조하는 실제 월드 자료다. span은 원본 배열이며 풀/해시/장부의 사본을 만들지 않는다.
// 이 참조는 월드보다 오래 살 수 없고 연결 모듈 해제 전 ClearProcesses를 호출해야 한다.
struct RawWorldCore {
    // pool: 객체 슬롯의 생성·조회·반납에 사용하는 SID 풀.
    o::SidPool& pool;
    // hash: 좌표별 객체 후보를 찾는 공간 해시.
    o::SquidHash& hash;
    // spots: 칸별 점유·통행 비트를 읽는 spot 지도.
    std::span<std::uint8_t> spots;
    // types: 타입 번호로 속성·플래그를 조회하는 타입 표.
    std::span<const o::RiftTypeRecord> types;
    // frames: 타입별 프레임 자료.
    std::span<const o::RiftTypeFrames> frames;
    // shapes: 프레임의 크기·그림 데이터를 조회하는 셰이프 자료.
    std::span<const o::SquidDisplayShape> shapes;
    // bookkeeping: 객체 종류·소유자별 등록 목록과 개수를 관리하는 장부.
    o::SquidPostPopState& bookkeeping;
    // ownerMode: 판본·서버·로딩 상태에 따른 소유권 처리 정책.
    o::SquidOwnerMode& ownerMode;
    // factory: 타입에 맞는 객체를 생성하는 처리기.
    o::SquidFactory& factory;
    // owner: 소유 플레이어 변경을 객체 필드와 장부에 반영하는 처리기.
    o::SquidOwner& owner;
    // frame: 프레임 변경을 물리 프레임·통지·갱신 요청에 반영하는 처리기.
    o::SquidFrame& frame;
    // pop: 객체를 공간 지도에 등록하는 Pop 처리기.
    o::SquidPop& pop;
    // unpop: 객체의 공간 등록을 해제하는 Unpop 처리기.
    o::SquidUnpop& unpop;
    // postPop: 객체 Pop 이후 목록·공간 후처리를 수행하는 처리기.
    o::SquidPostPop& postPop;
    // display: 객체·프레임에서 실제 화면 출력 요청을 만드는 처리기.
    o::SquidDisplay& display;
    // building: 일반 건물의 공간 등록·해제를 반영하는 처리기 또는 상태.
    o::RawBuildingSpace& building;
    // graph: 서로 연결된 표면의 Graph 소속을 관리하는 처리기.
    o::RawGraph& graph;
    // neighbors: 발자국에 인접한 표면·객체를 조회하는 검색기.
    o::RawSquidNeighbors& neighbors;
    // destroy: 객체 삭제의 전처리·공간 해제·후처리를 실행하는 처리기.
    o::RawSquidDestroy& destroy;
    // virtualDestroy: 타입별 가상 삭제 경로를 선택하는 분배기.
    o::RawSquidDestroyDispatch& virtualDestroy;
    // surfaces: 표면 객체의 공간·연결·수명 처리 자료.
    o::RawSurfaceLifecycle& surfaces;
    // bridgeEvents: 다리의 연결·붕괴 변화에 따른 이벤트 처리기.
    o::RawBridgeEvents& bridgeEvents;
    // forms: 객체에 부착된 Form의 생성·통지·수명 처리기.
    o::SquidFormDispatch& forms;
    // dependencies: 같은 풀/삭제 깊이/ProcessForm 통지를 사용하는 실제 감시 Form 쌍이다.
    o::DependFormHost& dependencies;
    // processes: 공유 커널에 연결한 객체 프로세스 호스트.
    o::SquidProcessHost& processes;
    // processState: 프로세스 실행·부착·이벤트 분배에 사용하는 상태.
    o::SquidProcessState& processState;
    // kernel: 프로세스 등록과 프레임 실행을 담당하는 커널.
    o::Kernel& kernel;
    // bridgeState: 같은 월드의 실제 표면 소유자 분배·연결이 읽는 현재 mutable 전역이다.
    o::BridgeConnectState& bridgeState;
};
// 표면의 기존 훅을 합성한 미션 runtime 경계다. 새 월드 생성 후 첫 객체 생성/Load 전에 한 번 연결한다.
struct RawWorldRuntimeHooks {
    // assets: 타입·셰이프·팔레트 자산 또는 해당 자산의 처리 경계.
    o::SquidDestroyHooks assets;
    // destroy: 객체 삭제의 전처리·공간 해제·후처리를 실행하는 처리기.
    o::SquidAssetDestroy destroy;
    // regular: 주기 동작을 수행하는 Regular 프로세스 연결.
    o::RegularHandler regular;
    // transmit: Form 데이터 작성과 송신을 연결하는 전송 경계.
    o::ProcessFormTransmission transmit;
    // walkerFall: 지정 이동 유닛의 추락 처리를 실행하는 콜백.
    std::function<bool(o::Sid)> walkerFall;
    // loadingDepth: 현재 로딩 중첩 깊이를 외부 상태에 전달하는 콜백.
    std::function<void(std::uint32_t)> loadingDepth;
    // formTransmit: 실제 DependForm 서버 Pop의 현재 raw Form/flags 전송 요청 경계다.
    o::FormPopTransmit formTransmit;
    o::DependFormNotice dependencyNotice; // watcher의 실제 가상+34다. 누락 시 기존 ProcessForm 통지를 사용한다.
    std::function<void(const o::BridgeLifecycleEvent&)> bridgeEffect; // 실제 세션의 다리 파편/위치 소리 소비자이며 runtime 설치 전에 필수로 공급한다.
};
// 같은 SID 풀·공간 지도에서 표면 객체의 생성·등록·수명·표시를 관리하는 월드이다.
class RawSurfaceWorld {
public:
    // 프레임/타입/SHP를 소유하여 GUI 참조와 raw 조회가 같은 판본을 사용하게 한다.
    RawSurfaceWorld(o::OriginalEdition edition,std::span<const o::RiftTypeRecord> types,
        std::span<const o::RiftTypeFrames> frames,std::span<const o::SquidDisplayShape> shapes,
        o::BridgeConnectState links,std::uint32_t terrainType,o::Kernel* processKernel=nullptr,std::shared_ptr<RawSurfaceStorage> storage={},bool adoptPrepared=false,bool* sharedSavedGame=nullptr,std::shared_ptr<o::RawAuthorityState> authority={}); // 준비 대여는 실제 물리를 사용하며 저장 게임 관문은 첫 객체 전부터 Client의 같은 bool을 빌린다.
    // 실제 Client는 같은 전역 Kernel을 빌리고 독립 월드만 자체 Kernel을 소유한다. 빌린 Kernel은 월드보다 오래 살아야 한다.
    // 빌린 경우 호출자는 모듈 반납 전에 DrainProcesses를 마쳐야 한다. 자체 Kernel은 이 월드의 다른 멤버보다 먼저 반납한다.
    ~RawSurfaceWorld();
    RawSurfaceWorld(const RawSurfaceWorld&)=delete;
    RawSurfaceWorld& operator=(const RawSurfaceWorld&)=delete;
    // 본섬→행 우선 noIsland→다리 순으로 일반 Pop을 실행하고 마지막에 전체 Graph를 재구성한다.
    void Load(std::span<const SurfaceSeed> seeds);
    // 같은 풀에 실제 미션 모듈을 구성할 때 사용한다. 반환 참조의 수명은 이 월드와 같다.
    RawWorldCore Core();
    const std::shared_ptr<RawSurfaceStorage>& Storage() const; // 미니맵도 Session이 아닌 동일 물리 소유자를 직접 빌린다.
    //원본옵션의TYPE+c4 쓰기가같은월드의188TYPE를직접사용하게한다. 새사본을만들지않는다.
    std::span<o::RiftTypeRecord> MutableTypes();
    // 최초객체/Runtime등록전에원본모드의실제소유권·연결색·다리붕괴소비자를함께설정한다.
    void ConfigureInstallModes(bool fort,bool battle,bool challenge,bool editor);
    // 미리보기도 같은 실제 연결 생성기와 현재 표면 전역을 사용한다.
    o::RawBridgeConnect& BridgeConnections();
    // 같은 저장소를 사용하는 자산/Regular/Form 전송/낙하/로딩 경계를 설치한다. 미연결 콜백을 생략하지 않는다.
    void BindRuntime(RawWorldRuntimeHooks hooks);
    // 첫 객체 생성 전에 공통 삭제의 관찰 경계를 실제 보상/현재 선택/선택해제·소리 소비자로 한 번 교체한다.
    void BindDeletion(o::SquidReward& reward,std::function<o::Sid()> selected,o::SquidDeletionHooks hooks);
    // 후설치와 같은 현재 원본 삭제 전역이다. 별도 세션 사본을 만들지 않는다.
    o::SquidDeletionState& DeletionState();
    // 현재 후설치된 실제 공통 삭제의 검사/Pre/Post를 파생 자산과 공유한다.
    o::SquidDestroyLifecycle& CommonDeletion();
    // 원본 Fort 순서대로 Create/필드 저장/Pop하는 로더의 시작과 종료다. 종료 시 Graph를 재구성한다.
    void BeginLoad();
    void FinishLoad();
    // 실제 캠페인 부모가 Graph(false)를 이미 실행한 뒤 로딩 상태만 종료한다. Graph/풀/해시를 재구성하지 않는다.
    void FinishLoadAfterGraph();
    // 로더와 실제 명령이 사용하는 같은 가상 생성·SHP 공간 등록·해제·재등록이다.
    o::Sid Create(std::uint32_t type,std::uint32_t flags=0);
    void Pop(o::Sid sid,float x,float y,std::uint32_t flags);
    void Unpop(o::Sid sid,std::uint32_t flags);
    void Repop(o::Sid sid,std::uint32_t flags);
    // 외부 연결 모듈을 해제하기 전에 예약 객체를 먼저 정리한다. 월드 자료는 유지한다.
    void ClearProcesses();
    void DrainProcesses(); // C++ 최종 소멸 전용으로 소멸자가 지난 슬롯에 만든 프로세스까지 반납한다. 원문 Clear 한 번과 구별한다.
    // 파생 로더의 중첩 깊이를 표면과 비표면 몸체에 동일하게 전달한다.
    void SetLoadingDepth(std::uint32_t depth);
    // 원본 고정 게임 시각을 모든 Regular에 공급한다. 정지 중에는 커널을 실행하지 않는다.
    void RunFrame(o::FrameTime time,bool paused);
    void PrepareFrame(o::FrameTime time); // 같은 시각만 공급한다. 실제 Client는 이후 전역 Kernel을 정확히 한 번 실행한다.
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
    // impl_: 공개 인터페이스 뒤의 구현 상태와 연결 객체.
    std::unique_ptr<Impl> impl_;
};
}
