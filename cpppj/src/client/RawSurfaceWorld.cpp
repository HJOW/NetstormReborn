// 호스트 통합이다. 개별 몸체의 원본 근거는 surface-graph-integration 및 각 raw 모듈 문서를 따른다.
#include "client/RawSurfaceWorld.h"
#include "o/RawPeerWorldSync.h" // 실제 같은 Pool/송신 전역과 walker Unpop/재등록을 접합한다.
#include "client/GameAssets.h" // 최초 로딩 비용표는 수정 가능한 World TYPE와 별개인 불변 자산 표에서 읽는다(단위865).
#include "o/RawBridgeFallParent.h" // 실제낙하예약이원문현재checking/TYPE진단부모를거친다.
#include "o/RawBridgeEvents.h"
#include "o/RawIslandPostPop.h"
#include "o/RawSurfaceLifecycle.h"
#include "o/RawSquidDestroyDispatch.h"
#include "o/RawBridgeDecay.h"
#include "o/RawAuthorityState.h"
#include "o/SquidDestroyLifecycle.h"
#include "o/SquidFactory.h"
#include "o/SquidFrameBinding.h"
#include "o/SquidOwner.h"
#include "o/RawAssetOwner.h" // 생산/중립/표면 재정의를 현재 VT로 분배한다.
#include "o/SquidFormDispatch.h"
#include "o/RawObjectConversion.h" // 비석/주문의정확한가상표만읽기전용예약진단대상으로구별한다.
#include "o/RawBuildingSpace.h"
#include "o/RawContentParentNotice.h" // 기본 빈 통지와 미연결 파생 통지를 실제 VT로 구분한다.
#include "o/RawFormAttach.h" // 자산+50도 같은 현재 종속 링크/공통 Pop을 사용한다.
#include "o/RawFormPop.h" // 같은 실제 PostPop 깊이를 공유한다.
#include <algorithm>
#include <bit>
#include <cmath>
#include <sstream>
#include <optional>
#include <stdexcept>
#include <utility>

namespace netstorm::client {
namespace {
// free/dead/void/contained 상태를 동시에 거르는 기존 마스크다.
constexpr std::uint8_t kInactive=15;
// 정렬되지 않은 원본 필드를 little endian으로 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t offset,std::size_t width=4) {
    std::uint32_t value=0;
    // 지정 폭만 읽어 CD의 인접 extra 바이트를 섞지 않는다.
    for (std::size_t i=0;i<width;++i) value|=static_cast<std::uint32_t>(bytes[offset+i])<<(8*i);
    return value;
}
// 초기 저장 프레임을 일반 Pop 전에 쓰며 표시/공간은 Pop에 맡긴다.
void Write(std::span<std::uint8_t> bytes,std::size_t offset,std::uint32_t value,std::size_t width) {
    // 필드 밖의 바이트는 보존한다.
    for (std::size_t i=0;i<width;++i) bytes[offset+i]=static_cast<std::uint8_t>(value>>(8*i));
}
// 잘못된 판본/크기/타입 요청은 기존 실제 물리를 초기화하기 전에 거부한다.
std::shared_ptr<RawSurfaceStorage> SelectStorage(std::shared_ptr<RawSurfaceStorage> storage,o::OriginalEdition edition,std::span<const o::RiftTypeRecord> types,std::span<const o::RiftTypeFrames> frames,std::span<const o::SquidDisplayShape> shapes,const o::BridgeConnectState& links,std::uint32_t terrain,bool prepared,std::shared_ptr<o::RawAuthorityState> authority,std::shared_ptr<o::RawGameAuxState> gameAux) {
    if (prepared && (!storage || edition!=o::OriginalEdition::Patch1078)) throw std::invalid_argument("준비된 표면 대여의 실제 PATCH 저장소 누락");
    if (types.size()!=frames.size() || types.size()!=shapes.size()) throw std::invalid_argument("월드 대여 전 자산 배열 크기 불일치");
    if (terrain>=types.size() || links.bridgeType>=types.size() || links.noIslandType>=types.size()) throw std::out_of_range("월드 대여 전 타입 번호 오류");
    if (storage && (storage->pool.Edition()!=edition || storage->types.size()!=types.size() || storage->frames.size()!=frames.size() || storage->shapes.size()!=shapes.size())) throw std::invalid_argument("월드 대여 전 실제 물리/판본 불일치");
    if (storage && authority && storage->Authority()!=authority) throw std::invalid_argument("월드 대여 전 실제 권한 소유자 불일치"); // 불일치는 일반 fresh 쓰기나 대여보다 먼저 거부한다.
    if (storage && gameAux && storage->GameAux()!=gameAux) throw std::invalid_argument("월드 대여 전 실제 게임 보조 소유자 불일치"); // fresh/대여 전에 다른 owner를 거부한다.
    return storage ? std::move(storage) : std::make_shared<RawSurfaceStorage>(edition,types,frames,shapes,nullptr,std::move(authority),std::move(gameAux));
}
}
// RawSurfaceWorld의 공유 저장소와 처리기 연결을 소유하는 구현 객체다.
struct RawSurfaceWorld::Impl final:o::SquidDisplaySink {
    std::shared_ptr<RawSurfaceStorage> storage;std::unique_ptr<RawSurfaceStorage::Use> storageUse; // 모든 모듈/참조를 반납한 뒤 물리 대여를 풀어 준다.
    // edition: 판본별 필드 배치와 동작을 선택하는 원본 버전.
    o::OriginalEdition edition;/* types: 같은 실제 물리의 수정 가능한 TYPE다. */ std::vector<o::RiftTypeRecord>& types;/* frames: 같은 장기 프레임 표다. */ std::vector<o::RiftTypeFrames>& frames;
    // shapes: 프레임의 크기·그림 데이터를 조회하는 셰이프 자료.
    std::vector<o::SquidDisplayShape>& shapes;/* codes: 같은 실제 프레임 코드다. */ std::vector<std::vector<o::FrameCode>>& codes;
    // pool: 객체 슬롯의 생성·조회·반납에 사용하는 SID 풀.
    o::SidPool& pool;/* hash: 같은 실제 공간 해시다. */ o::SquidHash& hash;/* spots: 같은 실제 공간 배열이다. */ std::vector<std::uint8_t>& spots;
    // links: 연결 함수들이 읽는 원본 전역이다. 패치 주소 / CD 주소를 주석에 둔다 (o::BridgeConnectState).
    o::BridgeConnectState& links;/* terrainType: isle 지면 자산을 타입 표에서 찾는 번호. */ std::uint32_t terrainType;/* bookkeeping: 제어 상태는 새 World가 소유하고 목록2개/세 집계 표는 같은 장기 물리에서 빌린다. */ o::SquidPostPopState bookkeeping{.sharedTotalMade=&storage->totalMade,.sharedTotalCost=&storage->GameAux()->TotalCost(),.sharedProviders=&storage->sidLists.providers,.sharedFactories=&storage->sidLists.factories};
    // deletion: 삭제.
    o::SquidDeletionState deletion{.sharedLostX=&storage->GameAux()->LostX(),.sharedLostY=&storage->GameAux()->LostY()};/* mode: 원본 실행 상태 전역. 기본값은 싱글 플레이(로컬이 서버) 전투 화면이다 (o::BridgeDecayMode). */ o::BridgeDecayMode mode;/* eventState: 처리기가 읽는 원본 전역이다. 패치 주소 / CD 주소를 괄호에 둔다 (o::BridgeEventState). */ o::BridgeEventState eventState;
    // loadState: 로딩 상태.
    o::IslandPostPopState loadState{1,false};/* ownerMode: 미니맵과 같은 실제 모드 전역이다. */ o::SquidOwnerMode& ownerMode;
    // buildingState: 원본 타입 전역과 현재 debug 전역이다. 섬 확인은 영역 번호나 소유자를 조회하지 않는다 (o::BuildingSpaceState).
    o::BuildingSpaceState buildingState;
    // processState: 프로세스 실행·부착·이벤트 분배에 사용하는 상태.
    o::SquidProcessState& processState{storage->Processes().State()};/* time: 프레임의 게임 시각과 벽 시각 및 시간 차 (o::FrameTime). */ o::FrameTime time{};/* changed: 상태 변경으로 후속 갱신이 필요한지. */ /* loaded: 현재 자료의 로딩이 완료됐는지. */ /* loading: 현재 로딩 작업 중인지. */ /* runtimeBound: 실행 연결된 여부. */ bool changed{true},loaded{},loading{},runtimeBound{};
    RawWorldRuntimeHooks runtime; // 외부 미션 모듈의 참조는 첫 객체 생성 전에만 설치한다.
    std::function<o::Sid()> deletionSelected; // 실제 후설치 뒤 매 삭제/공통pre에서 현재 UI선택을 읽는다.
    bool deletionBound{}; // 같은공통state의실제보상/효과후설치는첫객체생성전한번만허용한다.
    // displayHooks: 해상도 변경으로 Renderer가 교체되어도 현재 장치에 전달할 수 있는 표시 콜백이다 (SurfaceDisplayHooks).
    SurfaceDisplayHooks displayHooks;/* effectRequests: 효과 요청. */ /* walkerFalls: 이동유닛 추락. */ /* deletionRequests: 삭제 요청. */ std::size_t effectRequests{},walkerFalls{},deletionRequests{};
    // graph: 서로 연결된 표면의 Graph 소속을 관리하는 처리기.
    std::unique_ptr<o::RawGraph> graph;/* display: 객체·프레임에서 실제 화면 출력 요청을 만드는 처리기. */ std::unique_ptr<o::SquidDisplay> display;
    // postPop: 객체 Pop 이후 목록·공간 후처리를 수행하는 처리기.
    std::uint32_t containedType{6},containedGumpType{3},containedUiEvent{0x02000039}; // 두 판본의 공통 최초 타입과 UpdateContents 명령이며 Player의31h 통지와 다르다.
    std::unique_ptr<o::RawSquidContainedUnpop> containedUnpop; // 월드 기본 종단은 같은 물리와 장기 진단 장치만 빌린다.
    std::weak_ptr<o::RawSquidContainedUnpop> sessionContainedUnpop; // Session 파생 부모/입력 큐가 해제된 뒤 callback을 보관하지 않는다.
    std::unique_ptr<o::SquidPostPop> postPop;/* unpop: 객체의 공간 등록을 해제하는 Unpop 처리기. */ std::unique_ptr<o::SquidUnpop> unpop;
    // pop: 객체를 공간 지도에 등록하는 Pop 처리기.
    std::unique_ptr<o::SquidPop> pop;/* factory: 타입에 맞는 객체를 생성하는 처리기. */ std::unique_ptr<o::SquidFactory> factory;
    // building: 일반 건물의 공간 등록·해제를 반영하는 처리기 또는 상태.
    std::unique_ptr<o::RawSquidTransmission> squidTransmission; //같은GameAux의공통버퍼/압축전역을쓰는장기writer다.
    std::unique_ptr<o::RawBuildingSpace> building;
    // frame: 사용할 그림 프레임 또는 프레임 처리 자료.
    std::unique_ptr<o::SquidFrame> frame;/* owner: 소유 플레이어 번호 또는 소유권 처리 자료. */ std::unique_ptr<o::SquidOwner> owner;
    std::function<void(o::Sid,std::uint32_t)> assetOwner; // 같은 Pool/생산 장부/표면 전역을 빌리는 현재 소유자 분배다.
    // neighbors: 발자국에 인접한 표면·객체를 조회하는 검색기.
    std::unique_ptr<o::RawSquidNeighbors> neighbors;/* connect: 연결. */ std::unique_ptr<o::RawBridgeConnect> connect;
    // surface: 표면.
    std::unique_ptr<o::RawIslandPostPop> surface;/* baseDestroy: 기본 삭제. */ std::unique_ptr<o::RawSquidDestroy> baseDestroy;
    // virtualDestroy: 타입별 가상 삭제 경로를 선택하는 분배기.
    std::unique_ptr<o::RawSquidDestroyDispatch> virtualDestroy;/* common: 파생 타입 사이에 공유하는 기본 처리기 또는 상태. */ std::unique_ptr<o::SquidDestroyLifecycle> common;
    // lifecycle: 수명.
    std::unique_ptr<o::RawSurfaceLifecycle> lifecycle;/* events: 발생한 이벤트 또는 이벤트 처리기. */ std::unique_ptr<o::RawBridgeEvents> events;
    // host: Form이나 프로세스의 등록·통지·수명을 연결하는 호스트.
    std::unique_ptr<o::SquidProcessHost> legacyHost; // raw Kernel*만 빌린 기존 독립 fixture의 좁은 수명이다.
    std::shared_ptr<const std::function<void(o::Sid)>> legacyAllocationReclaim; // 같은 현재 World의 회수 경계를 소유하며 Storage에는 weak 참조만 남긴다.
    o::SquidProcessHost* host{}; // 실제 경로는 Storage 장기 Host를 직접 가리킨다.
    std::unique_ptr<RawProcessHostStorage::Use> processUse; // 현재 World의 실제 가상 처리기 대여다.
    std::optional<RawProcessHostStorage::Returned> returnedProcesses; // 단일 Native Clear 뒤 실제 소비한 대여의 반납 증빙이다.
    // formDispatch: 타입별 서로 다른 fallback 사슬을 같은 분배기로 모아 혼합 종속 체인의 중첩 삭제를 지원한다 (std::unique_ptr<o::SquidFormDispatch>).
    std::unique_ptr<o::SquidFormDispatch> formDispatch;
    // dependencies: 같은 삭제 분배기에 영구 등록하는 실제 감시 Form 쌍 소유자다.
    std::unique_ptr<o::DependFormHost> dependencies;
    std::unique_ptr<o::RawBridgeDecay> bridgeDecay; //실제 Graph/풀을 읽고 같은 World의 가상 삭제/표시를 호출한다.
    const std::uint32_t* decayEditor{}; //실제 Client의 현재 Editor 주소이며 독립 World는 기존 mode를 사용한다.
    std::function<void(o::Sid)> decayCarriers,decaySound; //현재 Session의 대피/위치 소리 장치이며 World 반납 전에 같이 소멸한다.
    std::function<void(o::FrameTime)> decayTime; //Kernel 준비보다 먼저 현재 입자/소리의 순수 시각 필드만 공급한다.
    bool sharedProcessHost; // 명시 pin 장치 또는 Storage 독립 Kernel만 잔류 대여를 지원한다.
    o::Kernel& kernel; // 모든 Form/경로/Regular가 같은 실제 Client Kernel 또는 독립 소유자를 직접 빌린다.
    // 순환하는 콜백은 실제 호출 전에 모든 모듈을 구성한다. 생성 중에는 Pop하지 않는다.
    Impl(o::OriginalEdition selected,std::span<const o::RiftTypeRecord> inputTypes,std::span<const o::RiftTypeFrames> inputFrames,
        std::span<const o::SquidDisplayShape> inputShapes,o::BridgeConnectState state,std::uint32_t terrain,o::Kernel* processKernel,std::shared_ptr<RawSurfaceStorage> supplied,bool prepared,bool* sharedSavedGame,std::shared_ptr<o::RawAuthorityState> authority,std::shared_ptr<o::RawGameAuxState> gameAux)
        :storage(SelectStorage(std::move(supplied),selected,inputTypes,inputFrames,inputShapes,state,terrain,prepared,std::move(authority),std::move(gameAux))),storageUse(prepared ? storage->AcquirePrepared() : storage->Acquire()),
         edition(selected),types(storage->types),frames(storage->frames),shapes(storage->shapes),codes(storage->codes),pool(storage->pool),hash(storage->hash),spots(storage->spots),links(storage->links),terrainType(terrain),loadState{1,false,sharedSavedGame},ownerMode(storage->ownerMode),
         sharedProcessHost(!processKernel || storage->Processes().UsesKernel(processKernel)),kernel(sharedProcessHost ? storage->Processes().Kernel() : *processKernel) {
        if (!prepared) links=state; // 명시 Native prefix의 현재 종류/모드는 보존하고 기존 독립 설치만 새 상태를 쓴다.
        if (pool.Edition()!=selected || frames.size()!=inputFrames.size() || shapes.size()!=inputShapes.size() || types.size()!=inputTypes.size()) throw std::invalid_argument("raw 표면 월드 실제 물리/판본 불일치");
        if (terrainType>=types.size() || links.bridgeType>=types.size() || links.noIslandType>=types.size())
            throw std::out_of_range("raw 표면 월드 타입 번호 오류");
        bookkeeping.localOwner=1;if (!prepared) ownerMode={false,true,false};else mode.editor=true;eventState.bridgeType=links.bridgeType; // 준비 대여는 ExistingEditor 전용이며 새 지역 장부/붕괴 소비자만 구성한다.
        graph=std::make_unique<o::RawGraph>(pool,hash,spots,types,codes,storage->graph,o::GraphRecovery::FullPool); // 실제 전체 풀은 원문 소진 재구성을 허용한다. 생성 자체는 현재 물리를 쓰지 않는다.
        if (edition==o::OriginalEdition::Patch1078) graph->BindCurrentTypes(types); //현재World속성변경을같은실제Graph가즉시읽는다.
        graph->BindDiagnostics({storage->Checking(),[owner=storage.get()](auto expression,auto file,auto line) { owner->AssertDisplay(expression,file,line); }}); // 첫 Session 이전에도 같은 장기 Client 진단을 영역 참조 감소에 제공한다.
        graph->BindTrace([owner=storage.get()](auto level,auto text) { owner->TraceGraph(level,text); }); // 재구성/병합의 실제 level0x20도 같은 현재 Client writer를 소비한다.
        graph->BindDetachState(bookkeeping.graphsEnabled,deletion.rebuildGraph); // preDestroy와 내부 각 분할이 같은 현재 World 상태를 읽는다.
        graph->BindClock([owner=storage.get()] { return owner->ReadGraphClock(); }); // 두 지역 시각 읽기도 장기 Client 장치를 거친다.
        display=std::make_unique<o::SquidDisplay>(edition,types,shapes,*this,o::SquidDisplayView{0,0,65536,{0,0,640,480}});
        postPop=std::make_unique<o::SquidPostPop>(pool,types,bookkeeping,graph.get(),[this](o::Sid sid) { connect->Connect(sid); });
        if (edition==o::OriginalEdition::Patch1078) {
            postPop->BindCurrentTypes(types); // 후처리 플래그는 실제 현재 TYPE를 읽는다. 별도 비용 캐시와 혼합하지 않는다(단위865).
            if (const auto source=storage->Source()) postPop->BindLoadedCosts(source->TypeTable().Types()); // World 교체 뒤 변한float를 최초 로딩 캐시로 다시 해석하지 않는다.
        }
        postPop->BindManaDiagnostics({storage->Checking(),[owner=storage.get()](auto expression,auto file,auto line) { owner->AssertDisplay(expression,file,line); }}); // 첫 Session 이전에도 실제 Client의 현재 진단 장치로 복귀한다.
        postPop->SetIslandPrefix([this](o::Sid sid,std::uint32_t flags) { connect->IslandPostPopPrefix(sid,flags); });
        postPop->SetSurfacePrefix([this](o::Sid sid,std::uint32_t flags) { surface->Prefix(sid,flags); });
        unpop=std::make_unique<o::SquidUnpop>(pool,hash,spots,display.get());pop=std::make_unique<o::SquidPop>(pool,hash,spots,display.get(),postPop.get());
        unpop->BindDiagnostics({storage->Checking(),types,[owner=storage.get()](auto expression,auto file,auto line) { owner->AssertDisplay(expression,file,line); }}); // 첫 Session 전에도 같은 현재 Client 진단/TYPE를 쓴다.
        pop->BindDiagnostics({storage->Checking(),types,{},[owner=storage.get()] { return owner->ownerMode.challenge; },[owner=storage.get()](auto level,auto text) { owner->TraceGraph(level,text); },[owner=storage.get()](auto expression,auto file,auto line) { owner->AssertDisplay(expression,file,line); }}); // 좌표/spot 뒤 현재 SHP·같은 Client 진단/로그를 사용한다.
        o::SquidContainedUnpopHooks containedHooks;
        containedHooks.parent=[this](o::Sid sid) { return o::CommonContainedParent(pool,sid); }; // 현재 실제 VT의 부모 getter를 사용한다.
        containedHooks.previous=[this](o::Sid sid) { return o::CommonContainedPrevious(pool,sid); }; // Form과 자산의 서로 다른 WORD를 읽는다.
        containedHooks.setPrevious=[this](o::Sid sid,o::Sid previous) { o::CommonContainedSetPrevious(pool,sid,previous); }; // 현재 이웃 VT의 setter만 실행한다.
        containedHooks.parentNotice=[this](o::Sid parent,o::Sid form,std::uint32_t reason) { o::RawContentParentNotice(pool).Notice(parent,form,reason); }; // 실제 파생 종단이 필요한 경우 선택 시 명시 실패한다.
        containedHooks.assertion=[owner=storage.get()](auto expression,auto file,auto line) { owner->AssertDisplay(expression,file,line); }; // 같은 Client 장기 진단을 호출한다.
        containedUnpop=std::make_unique<o::RawSquidContainedUnpop>(pool,o::SquidContainedUnpopState{containedType,containedGumpType,containedUiEvent,storage->Checking(),[this] { return ownerMode.challenge; },[this] { return storage->Authority()->boss; }},std::move(containedHooks));
        unpop->BindContainedUnpop([this](o::Sid sid) { if (const auto current=sessionContainedUnpop.lock()) current->Unpop(sid);else containedUnpop->Unpop(sid); }); // 일반 Unpop의 void 선행 뒤 실제 종단만 이어 실행한다.
        buildingState={terrainType,links.bridgeType,false};
        building=std::make_unique<o::RawBuildingSpace>(pool,hash,types,buildingState);
        building->BindCurrentBridgeType(links.bridgeType); // WORD setter/연결과 같은 현재 TYPE를 읽고 초기 사본을 고정하지 않는다.
        building->BindNotifyDiagnostics(storage->Checking(),[owner=storage.get()](auto expression,auto file,auto line) { owner->AssertDisplay(expression,file,line); }); // 첫 Session 전에도 같은 장기 Client 장치를 매 진단에서 사용한다.
        pop->SetBuildingSpace(building.get());unpop->SetBuildingSpace(building.get());
        factory=std::make_unique<o::SquidFactory>(pool,types,false,unpop.get());
        factory->BindDiagnostics({types,[owner=storage.get()](auto expression,auto file,auto line) { owner->AssertDisplay(expression,file,line); },[owner=storage.get()](auto text) { owner->RecordAssertionDetail(text); },storage->Checking()}); // Take의 상세/정상 복귀와 공통 postTake가 같은 현재 TYPE·Client owner를 빌린다.
        factory->BindFormDetach([this](o::Sid sid) { if (const auto current=sessionContainedUnpop.lock()) current->Detach(sid);else containedUnpop->Detach(sid); }); // Form은 자신의 UI 검색을 추가하지 않고 같은 현재 종속 분리만 사용한다.
        factory->BindFormPopDepth(bookkeeping.depth); // 자산 Activate/후처리와 같은 장기 basePostPop DWORD를 Form도 공유한다.
        factory->BindFormAttach([this] { if (const auto current=sessionContainedUnpop.lock()) return current->ContentFormType();return containedType; },[this](o::Sid sid,std::uint32_t flags) { if (!runtime.formTransmit) throw std::logic_error("현재 공통 Form Pop 전송 미연결");runtime.formTransmit(sid,flags); }); // 같은 현재 Session의 번호와 World 전송만 짧게 빌린다.
        if (edition==o::OriginalEdition::Patch1078) {
            o::RawSquidTransmissionHooks transmission;
            transmission.type=[owner=storage.get()](std::uint8_t number)->const o::RiftTypeRecord& { return owner->types.at(number); };
            transmission.defaultType=[this] { return links.bridgeType; };
            transmission.maxHp=[this](std::uint8_t number) { return factory->ReadSelectedHitPoints(types.at(number),types); }; // 같은 World 수명의 전송기가 현재 TYPE/사제번호/약화/진단 getter를 사용한다. 신규 MP 기능 검사는 하지 않는다.
            transmission.broadcast=[this](auto packet) { if (!runtimeBound) return;if (!runtime.squidPacket) throw std::logic_error("현재월드의Squid방송장치미연결");const auto send=runtime.squidPacket;send(packet); }; //독립World는연결표가없고실제Session은동일broadcast를설치한다.
            transmission.trace=[owner=storage.get()](auto level,auto text) { owner->TraceGraph(level,text); };
            transmission.assertion=[owner=storage.get()](auto expression,auto file,auto line) { owner->AssertDisplay(expression,file,line); };
            transmission.connected=[this](auto target) { if (!runtimeBound || !runtime.squidConnected) throw std::logic_error("현재 대상별 Player 연결 관문 미연결");return runtime.squidConnected(target); };
            transmission.send=[this](auto target,auto packet) { if (!runtimeBound || !runtime.squidSend) throw std::logic_error("현재 대상별 Squid 송신 장치 미연결");runtime.squidSend(target,packet); };
            transmission.processPacket=[this](auto form,auto packet,auto argument) { if (!host) throw std::logic_error("ProcessForm의 실제 Host 미설치");return host->WriteFormPacket(form,argument,packet,storage->GameAux()->SquidTransmission()); };
            squidTransmission=std::make_unique<o::RawSquidTransmission>(pool,storage->GameAux()->SquidTransmission(),std::move(transmission));
            pop->BindTransmission([this](o::Sid sid,std::uint32_t flags) { squidTransmission->Pop(sid,flags); });
        }
        factory->BindWeakenedMana(storage->GameAux()->PriestQuarterHp()); // State 직접 쓰기와 첫 객체 생성이 같은 현재005ca8f0을 읽는다.
        factory->BindHitPointTypes(&storage->GameAux()->PriestType(),&storage->GameAux()->DaisType()); // HP 예외/약화 비교가 같은 장기 TYPE 주소를 읽는다.
        factory->BindFakeSurfaceType(storage->GameAux()->FakeSurfaceType()); // 생성 입구 진단도 같은Client 수명의 현재TYPE를 읽는다.
        frame=std::make_unique<o::SquidFrame>(pool,types,o::MakeSquidFrameHooks(pool,types,*display,*unpop,*pop));
        owner=std::make_unique<o::SquidOwner>(pool,types,bookkeeping,ownerMode);
        owner->BindDiagnostics({types,[owner=storage.get()](auto expression,auto file,auto line) { owner->AssertDisplay(expression,file,line); }}); // 실제 같은 World TYPE와 장기 Client 진단을 대여한다.
        neighbors=std::make_unique<o::RawSquidNeighbors>(pool,hash,spots,types,frames);
        if (edition==o::OriginalEdition::Patch1078) neighbors->BindCurrentTypes(types); //실제이웃과Graph가같은현재발자국/플래그를쓴다.
        o::BridgeConnectHooks objects;
        // 생성/소유자/프레임 변경은 표시 억제 상태와 무관하게 논리 변경을 알린다.
        objects.create=[this](std::uint32_t type,std::uint32_t flags) { changed=true;return factory->Create(type,flags); };
        assetOwner=o::MakeAssetOwnerDispatch(pool,links,bookkeeping.productionDirty,[this](o::Sid sid,std::uint32_t player) { owner->Set(sid,player); });
        objects.setOwner=[this](o::Sid sid,std::uint32_t player) { assetOwner(sid,player);changed=true; }; // 고정 중립 경로도 실제 변경으로 알린다.
        objects.pop=[this](o::Sid sid,float x,float y,std::uint32_t flags) { Pop(sid,x,y,flags); };
        objects.setFrame=[this](o::Sid sid,std::int32_t number,std::uint32_t flags) { frame->Set(sid,number,flags);changed=true; };
        objects.notifySurface=[this](o::Sid sid) { building->NotifySurface(sid); }; // 실제 판본별 무상태 진단 몸체다.
        connect=std::make_unique<o::RawBridgeConnect>(pool,*neighbors,links,objects);
        surface=std::make_unique<o::RawIslandPostPop>(pool,hash.Entries(0),types,frames,links,loadState,o::IslandPostPopHooks{
            objects,[this](o::Sid sid) { connect->Connect(sid); },
            [this](float x,float y,std::uint32_t type) { return connect->FindTypeAt(x,y,type); },
            [this](o::Sid sid) { changed=true;display->Update(pool.Slot(sid)); },
            [](bool,bool,bool) { throw std::logic_error("raw 표면 월드의 불완전한 받침 묶음"); },
            [owner=storage.get()](auto expression,auto file,auto line) { owner->AssertDisplay(expression,file,line); },
            [owner=storage.get()](auto text) { owner->RecordAssertionDetail(text); },
            [owner=storage.get()] { return owner->GameAux()->SupportTypeWas(); }}); // 같은 Client의 현재 상세/진단/TYPE 문맥이며 정상 복귀 뒤 연결을 계속한다.
        baseDestroy=std::make_unique<o::RawSquidDestroy>(pool,*unpop,types);
        virtualDestroy=std::make_unique<o::RawSquidDestroyDispatch>(*baseDestroy,*neighbors,mode,graph.get());
        virtualDestroy->BindCurrentModes(*storage->Authority()); //다른 효과에서 boss가 바뀌어도 다음 가상 삭제가 같은 현재 주소를 읽는다.
        // 이 기본값은 단독 표면 기하 검사용 관찰이다. 실제 세션은 BindDeletion으로 보상/실제 UI를 공급한다.
        common=std::make_unique<o::SquidDestroyLifecycle>(pool,types,bookkeeping,deletion,*baseDestroy,o::SquidDeletionHooks{
            [this](const o::SquidDeletionEvent&) { ++deletionRequests; },
            [](const o::SquidDestroyEvent& event) {
                if (event.effect!=o::SquidDestroyEffect::Transmit) throw std::logic_error("raw 표면 월드의 미연결 삭제 효과");
            }},graph.get());
        lifecycle=std::make_unique<o::RawSurfaceLifecycle>(pool,*neighbors,links.connectorType,CommonDeletionHooks(),o::SurfaceLifecycleHooks{
            [this](o::Sid sid,float x,float y) { o::ScheduleBridgeFallChecked(*host,pool,types,sid,x,y,storage->Checking()); },
            [this](o::Sid sid,std::uint32_t flags) { Destroy(sid,flags); },
            [this](o::Sid sid) { if (runtimeBound) { if (!runtime.walkerFall(sid)) throw std::logic_error("월드 낙하 처리 실패"); }else ++walkerFalls; },
            [this](const o::BridgeLifecycleEvent& event) { if (runtimeBound) runtime.bridgeEffect(event);else ++effectRequests; }}); // 단독 기하 검사의 관찰과 실제 Session의 파편/소리를 구별한다.
        o::BridgeEventHooks eventHooks;
        // 실제 사건의 삭제/재등록도 같은 가상 경로와 SHP 공급을 사용한다.
        eventHooks.notifySurface=objects.notifySurface;eventHooks.create=[this](std::uint32_t type) { changed=true;return factory->Create(type); };
        eventHooks.destroy=[this](o::Sid sid,std::uint32_t flags) { Destroy(sid,flags); };
        eventHooks.setOwner=[setOwner=objects.setOwner](o::Sid sid,std::uint8_t player) { setOwner(sid,player); };eventHooks.pop=objects.pop;
        events=std::make_unique<o::RawBridgeEvents>(pool,eventState,o::MakeBridgeNeighborHooks(*neighbors,std::move(eventHooks)));
        // 표면 월드의 실제 자산 재정의/프로세스 삭제도 공유 Form 분배기를 사용한다.
        formDispatch=std::make_unique<o::SquidFormDispatch>(pool,*baseDestroy,lifecycle->Hooks(),
            [this](o::Sid sid,std::uint32_t flags,const o::SquidDestroyHooks& hooks) { return virtualDestroy->Destroy(sid,flags,hooks); });
        if (sharedProcessHost) {
            auto binding=std::make_shared<o::ProcessHostRuntimeBindings>();binding->destroy=baseDestroy.get();binding->regular=o::MakeBridgeRegularHandler(pool,*events);binding->asset=formDispatch->Hooks(); // 실제 현재 World의 가상 부모만 짧게 빌린다.
            binding->factory=factory.get(); // 같은 World의 공장만 현재 대여에 보관하며 장기 Storage Host에 직접 남기지 않는다.
            binding->postPopDepth=[this] { return bookkeeping.depth; };binding->assertion=[this](auto expression,auto file,auto line) { storage->AssertDisplay(expression,file,line); }; // 공통 깊이는 짧은 World 대여, 진단은 같은 장기 Client 장치를 사용한다.
            processUse=storage->Processes().Bind(std::move(binding));host=&storage->Processes().Host();
        } else {
            legacyHost=std::make_unique<o::SquidProcessHost>(pool,types,kernel,*baseDestroy,processState,o::MakeBridgeRegularHandler(pool,*events),formDispatch->Hooks());legacyHost->BindFormFactory(*factory);host=legacyHost.get();
            legacyAllocationReclaim=storage->Processes().BindLegacyAllocationReclaim([this](o::Sid sid) { static_cast<void>(Destroy(sid,0)); }); // 실제 현재 Form/자산 분배로만 회수하며 raw Kernel을 Storage의 다른 Kernel로 대체하지 않는다.
        } // 직접 Host와 공장은 같은 World 수명이며 대여 밖으로 참조를 남기지 않는다.
        formDispatch->Bind(*host);
        dependencies=std::make_unique<o::DependFormHost>(pool,*baseDestroy,
            [this](o::Sid watcher,o::Sid watched,o::Sid form) {
                if (runtime.dependencyNotice) { runtime.dependencyNotice(watcher,watched,form);return; }
                // 실제 ProcessForm34는 server일 때 PID의 notify(2,form,watched)를 호출한다.
                host->NotifyDependency(watcher,watched,form);
            },formDispatch->Hooks(),[this](o::Sid form,std::uint32_t flags) {
                if (!runtime.formTransmit) throw std::logic_error("현재 DependForm 서버 전송 소비자 미연결");
                runtime.formTransmit(form,flags);
            });
        formDispatch->Bind(*dependencies);
        dependencies->BindCreation(*factory,storage->GameAux()->DependFormType(),[owner=storage.get()](auto expression,auto file,auto line) { owner->AssertDisplay(expression,file,line); }); // 장기 현재 번호를 두 번 읽고 같은 World의 공통 생성자/진단을 사용한다.
        dependencies->BindUnpop([this](o::Sid sid) { factory->UnpopForm(sid); }); // 감시 Form 삭제도 같은 현재 World의 진단과 부모 getter를 사용한다.
        if (edition==o::OriginalEdition::Patch1078) {
            o::RawBridgeDecayHooks decay;
            decay.destroy=[this](o::Sid sid,std::uint32_t flags) { static_cast<void>(Destroy(sid,flags)); };
            decay.notifySurface=[this](o::Sid sid) { building->NotifySurface(sid); };
            decay.refresh=[this](o::Sid sid) { changed=true;display->Update(pool.Slot(sid)); };
            decay.carriers=[this](o::Sid sid) { if (decayCarriers) decayCarriers(sid);else if (runtimeBound) throw std::logic_error("실제 다리 붕괴 대피 장치 미연결");else ++effectRequests; }; //독립 기하 관찰과 실제 Session의 필수 효과를 구별한다.
            decay.crackSound=[this](o::Sid sid) { if (decaySound) decaySound(sid);else if (runtimeBound) throw std::logic_error("실제 다리 균열 소리 장치 미연결");else ++effectRequests; };
            decay.assertion=[](auto condition,auto source,auto line) { throw std::logic_error("원문 다리 붕괴 진단 "+std::string(source)+":"+std::to_string(line)+": "+std::string(condition)); }; //진단 UI가 없는 호스트는 선택한 조건만 명시 실패하며 스캔의 기존 칸별 경계로 올린다.
            bridgeDecay=std::make_unique<o::RawBridgeDecay>(pool,types,frames,hash.Entries(0),spots,graph->Records(),mode,std::move(decay),storage->Authority().get(),storage->Checking());
        }
    }
    // proxy는후설치이전공통객체의주소를보존하지않아같은state/reward로교체한뒤에도현재몸체를호출한다.
    o::SquidDestroyHooks CommonDeletionHooks() {
        o::SquidDestroyHooks hooks;
        hooks.selected=[this] { return deletionSelected ? deletionSelected() : deletion.selected; };
        hooks.emit=[this](const o::SquidDestroyEvent& event) {
            if (event.effect==o::SquidDestroyEffect::PreDestroy && deletionSelected) deletion.selected=deletionSelected();
            common->Hooks().emit(event);
        };return hooks;
    }
    // 표시 억제는 현재 GUI 장치에서 조회한다. 콘솔에서는 변경 요청을 보존한다.
    bool Suppressed() const override { return displayHooks.suppressed && displayHooks.suppressed(); }
    // 장치가 교체되어도 현재 콜백으로만 전달한다.
    void Invalidate(o::SquidDisplayRect rect,std::uint32_t flags) override { changed=true;if (displayHooks.invalidate) displayHooks.invalidate(rect,flags); }
    o::RawCheckingFlag Checking() const override { return storage->Checking(); } // 표시 뷰 교체와 무관하게 같은 Client DWORD를 다시 빌린다.
    void FrameError(std::uint32_t type,std::int32_t frameNumber,std::uint32_t abstract) override { storage->RecordDisplayFrameError(type,frameNumber,abstract); } // 장치와 별도로 장기 상세 owner에 저장한다.
    void Assertion(std::string_view expression,std::string_view file,std::uint32_t line) override { storage->AssertDisplay(expression,file,line); } // 현재 진단 복귀를 실제 표시 부모에 돌려준다.
    // 현재 raw 프레임의 SHP 크기로 실제 공간과 파생 postPop을 등록한다.
    void Pop(o::Sid sid,float x,float y,std::uint32_t flags) {
        if (edition==o::OriginalEdition::Patch1078) {
            const auto result=pop->PopCurrent(sid,x,y,flags);if (result!=o::RawPopResult::Unchanged) changed=true;return; // 원문 앞단/좌표/spot 뒤에 현재 TYPE/SHP를 조회한다.
        }
        const auto raw=pool.Slot(sid);
        const auto number=static_cast<std::int32_t>(Read(raw,edition==o::OriginalEdition::Patch1078 ? 36 : 34,edition==o::OriginalEdition::Patch1078 ? 4 : 1));
        const auto size=display->FrameSize(edition,raw,number);
        const auto result=pop->Pop(sid,types[raw[10]],size[0],size[1],x,y,flags);
        if (result!=o::RawPopResult::Unchanged) changed=true; // 원문void Pop의겹침은부분좌표/spot을남긴정상반환이다. 호스트예외/성공등록/롤백으로변환하지않는다.
    }
    // 가상 삭제가 거부되면 changed를 새로 켜지 않는다.
    bool Destroy(o::Sid sid,std::uint32_t flags) { const bool result=formDispatch->Destroy(sid,flags);if (result) changed=true;return result; }
    // Native Clear와 구별되는 최종 소유권 반납이며 외부 Kernel의 다른 Host/Client 객체는 보존한다.
    void DrainCurrentProcessesForShutdown() {
        if (processUse) processUse->DrainOwnedForShutdown();
        else if (legacyHost) kernel.DrainOwnedForShutdown([this](const o::BaseProcess& process) { return legacyHost->Owns(process); }); // 슬롯0/retired와 소멸 중 재등록도 같은 실제 Host의 소유권만 반납한다.
    }
};
// 공개 객체는 내부 참조/콜백이 이동하지 않게 단일 구현 객체를 소유한다.
RawSurfaceWorld::RawSurfaceWorld(o::OriginalEdition edition,std::span<const o::RiftTypeRecord> types,
    std::span<const o::RiftTypeFrames> frames,std::span<const o::SquidDisplayShape> shapes,o::BridgeConnectState links,std::uint32_t terrainType,o::Kernel* processKernel,std::shared_ptr<RawSurfaceStorage> storage,bool adoptPrepared,bool* sharedSavedGame,std::shared_ptr<o::RawAuthorityState> authority,std::shared_ptr<o::RawGameAuxState> gameAux)
    :impl_(std::make_unique<Impl>(edition,types,frames,shapes,links,terrainType,processKernel,std::move(storage),adoptPrepared,sharedSavedGame,std::move(authority),std::move(gameAux))) {}
// 프로세스가 빌린 raw 데이터보다 먼저 반납되며 외부 Client Kernel 자체는 파괴하지 않는다.
RawSurfaceWorld::~RawSurfaceWorld() { impl_->DrainCurrentProcessesForShutdown(); } // 현재 대여의 생성 실패/일반 종료만 반납하며 이전 Native 잔류의 최종 소유자는 Storage다.
// 이 뷰는 내부 모듈의 현재 참조만 돌려주며 새 배열/풀을 만들지 않는다.
RawWorldCore RawSurfaceWorld::Core() {
    auto& s=*impl_;return {s.pool,s.hash,s.spots,s.types,s.frames,s.shapes,s.bookkeeping,s.ownerMode,
        *s.factory,*s.owner,*s.frame,*s.pop,*s.unpop,*s.postPop,*s.display,*s.building,*s.graph,*s.neighbors,
        *s.baseDestroy,*s.virtualDestroy,*s.lifecycle,*s.events,*s.formDispatch,*s.dependencies,*s.host,s.processState,s.kernel,s.links};
}
// 월드 모듈과 별개로 유지되는 실제 물리 소유자다.
const std::shared_ptr<RawSurfaceStorage>& RawSurfaceWorld::Storage() const { return impl_->storage; }
// 지역 부모는 소유 이미지를 만들지 않고 현재 장기 전역과 같은 공간 부모만 빌린다.
void RawSurfaceWorld::SynchronizePeer(std::uint32_t owner) {
    auto& s=*impl_;if (!s.squidTransmission) throw std::logic_error("현재 판본의 실제 Squid 전송기 미설치");o::PeerWorldSyncHooks hooks;
    hooks.sharedRegularType=[&s] { return s.storage->GameAux()->SharedRegularProcessType(); };hooks.unpop=[this](auto sid,auto flags) { Unpop(sid,flags); };hooks.repop=[this](auto sid,auto flags) { Repop(sid,flags); };
    o::RawPeerWorldSync(s.pool,s.types,s.storage->GameAux()->SquidTransmission(),*s.squidTransmission,std::move(hooks)).Synchronize(owner);
}
// 실제 삭제도 Activate/재동기화와 같은004ac260의 진단/양수 크기/현재 대상 송신을 사용한다.
void RawSurfaceWorld::TransmitDeletion(o::Sid sid,std::uint32_t flags) {
    auto& s=*impl_;if (!s.squidTransmission) throw std::logic_error("현재 판본의 실제 삭제 전송기 미설치");s.squidTransmission->Transmit(sid,-1,-2,0,flags);
}
// Session의globals만바꾸고private BridgeDecayMode.editor를남기지않는다.
void RawSurfaceWorld::ConfigureInstallModes(bool fort,bool battle,bool challenge,bool editor) {
    auto& s=*impl_;if (s.runtimeBound || s.loading || s.loaded || s.pool.FreeCount()!=s.pool.Capacity()-5) throw std::logic_error("실제설치모드는첫객체/Runtime등록전에필요");
    // 지정Take가freeCount를보존하는경우도이미만든실제슬롯을거부한다.
    for (std::uint32_t sid=5;sid<s.pool.Capacity();++sid) if (!(s.pool.Slot(o::Sid{static_cast<std::uint16_t>(sid)})[11]&1)) throw std::logic_error("실제설치모드설정전에객체존재");
    s.ownerMode={fort,battle,challenge};s.links.battle=battle;s.mode.editor=editor;
}
//현재같은전역TYPE의수정가능범위를원본옵션소비자에게만빌린다.
std::span<o::RiftTypeRecord> RawSurfaceWorld::MutableTypes() { return impl_->types; }
// 완성된 실제 파생 훅을 설치할 때 Form/Regular의 기존 callback도 현재 경계를 읽게 한다.
void RawSurfaceWorld::BindRuntime(RawWorldRuntimeHooks hooks) {
    auto& s=*impl_;
    if (s.runtimeBound || s.loading || s.loaded || s.pool.FreeCount()!=s.pool.Capacity()-5)
        throw std::logic_error("월드 runtime은 초기 객체 생성 전에 한 번 연결해야 합니다");
    // 지정 Take는 freeCount를 보존하므로 실제 슬롯도 확인하여 기존 객체가 있는 늦은 설치를 거부한다.
    for (std::uint32_t id=5;id<s.pool.Capacity();++id)
        if (!(s.pool.Slot(o::Sid{static_cast<std::uint16_t>(id)})[11]&1)) throw std::logic_error("월드 runtime 설치 전 객체가 존재합니다");
    if (!hooks.assets.emit || !hooks.assets.selected || !hooks.destroy || !hooks.regular || !hooks.transmit.writeData ||
        !hooks.transmit.transmit || !hooks.walkerFall || !hooks.loadingDepth || !hooks.bridgeEffect) throw std::invalid_argument("월드 runtime 필수 경계 누락");
    s.formDispatch->SetAssets(hooks.assets,hooks.destroy);s.host->SetRegularHandler(hooks.regular);s.host->SetFormTransmit(hooks.transmit);
    s.runtime=std::move(hooks);s.runtimeBound=true;s.runtime.loadingDepth(s.loadState.loadingDepth);
}
// Session은 엔진을 소유하고 월드는 현재 호출 동안만 pin하여 생성 실패/반납 뒤 수명 밖 접근을 막는다.
void RawSurfaceWorld::BindContainedUnpop(const std::shared_ptr<o::RawSquidContainedUnpop>& binding) {
    if (binding && &binding->Pool()!=&impl_->pool) throw std::invalid_argument("종속 Unpop의 실제 SID 풀 불일치");
    impl_->sessionContainedUnpop=binding;
}
// 기존표면분배기가보유한proxy를유지하면서단일공통삭제state의실제경제/UI소비자를교체한다.
void RawSurfaceWorld::BindDeletion(o::SquidReward& reward,std::function<o::Sid()> selected,o::SquidDeletionHooks hooks) {
    auto& state=*impl_;
    if (state.deletionBound || state.loading || state.loaded || state.pool.FreeCount()!=state.pool.Capacity()-5)
        throw std::logic_error("공통 삭제 실제 소비자는 첫 객체 생성 전에 한 번 연결해야 합니다");
    // Take는freeCount를보존할수있으므로동일물리슬롯도검사한다.
    for (std::uint32_t id=5;id<state.pool.Capacity();++id)
        if (!(state.pool.Slot(o::Sid{static_cast<std::uint16_t>(id)})[11]&1U)) throw std::logic_error("공통 삭제 실제 소비자 설치 전 객체가 존재합니다");
    if (!selected || !hooks.emit || !hooks.forward) throw std::invalid_argument("공통 삭제의 실제 선택/효과/파생 경계 누락");
    auto replacement=std::make_unique<o::SquidDestroyLifecycle>(state.pool,state.types,state.bookkeeping,state.deletion,*state.baseDestroy,
        std::move(hooks),state.graph.get(),&reward); // 같은Pool/장부가아닌reward는기존생성자가효과전에거부한다.
    state.common=std::move(replacement);state.deletionSelected=std::move(selected);state.deletionBound=true;
}
// 기존표면과후설치된공통body가읽는같은유일전역이다.
o::SquidDeletionState& RawSurfaceWorld::DeletionState() { return impl_->deletion; }
// BindDeletion이 교체한 현재 공통 몸체를 반환하고 별도 깊이/보상 사본을 만들지 않는다.
o::SquidDestroyLifecycle& RawSurfaceWorld::CommonDeletion() { return *impl_->common; }
// Fort의 원본 객체 순서를 보존하는 외부 로더는 이 경계 사이에서 Create/Pop을 직접 호출한다.
void RawSurfaceWorld::BeginLoad() {
    auto& s=*impl_;if (s.loaded || s.loading) throw std::logic_error("raw 월드 중복 로드 시작");
    s.bookkeeping.graphsEnabled=true; // 원본00540414는PE초기값1이다. 적재 중에도 noGraph 표면의254 지정과 건물 점유 영역 무효화가 필요하며, 완료 뒤에만 켜면 간헐천 호버/경로가 일반 지면으로 판정된다.
    s.loading=true;SetLoadingDepth(1);
}
// 모든 실제 객체를 등록한 뒤 원본 Graph/장부를 활성화한다.
void RawSurfaceWorld::FinishLoad() {
    auto& s=*impl_;if (!s.loading || s.loaded) throw std::logic_error("raw 월드 로드 종료 순서 오류");
    s.graph->Rebuild(true);FinishLoadAfterGraph();
}
// 실제 부모의 Graph(false)·mission 후처리·Camera 순서를 끝낸 뒤 로딩 활성화만 수행한다.
void RawSurfaceWorld::FinishLoadAfterGraph() {
    auto& s=*impl_;if (!s.loading || s.loaded) throw std::logic_error("raw 월드 로드 종료 순서 오류");
    s.bookkeeping.graphsEnabled=true;SetLoadingDepth(0);s.loading=false;s.loaded=true;s.changed=true;
}
// 복원된 타입 생성자를 같은 풀에서 실행한다.
o::Sid RawSurfaceWorld::Create(std::uint32_t type,std::uint32_t flags) { impl_->changed=true;return impl_->factory->Create(type,flags); }
// 현재 raw 프레임의 실제 SHP 크기가 공간 등록에 쓰인다.
void RawSurfaceWorld::Pop(o::Sid sid,float x,float y,std::uint32_t flags) { impl_->Pop(sid,x,y,flags); }
void RawSurfaceWorld::AttachBaseAsset(o::Sid sid,o::Sid parent,std::uint32_t flags) { // 원문83개 자산 VT의+50은 모두004ad330이다.
    auto& s=*impl_;if (s.edition!=o::OriginalEdition::Patch1078) throw std::logic_error("자산 종속 수신의 PATCH 가상 몸체 계약");
    s.storage->AssertDisplay("0","\\Ns\\O\\Squid.cpp",0x72a);
    auto slot=s.pool.Slot(sid);auto raw=std::span(const_cast<std::uint8_t*>(slot.data()),slot.size());
    Write(raw,18,0,4);Write(raw,14,std::bit_cast<std::uint32_t>(static_cast<float>(parent.value)),4);
    if (const auto current=s.sessionContainedUnpop.lock()) current->NotifyUi(sid);else s.containedUnpop->NotifyUi(sid);
    o::FormAttachHooks attach;attach.assertion=[&s](auto expression,auto file,auto line) { s.storage->AssertDisplay(expression,file,line); };
    attach.genus=[&s](auto type) { return s.types.at(type).flags2; };attach.contentType=[&s] { if (const auto current=s.sessionContainedUnpop.lock()) return current->ContentFormType();return s.containedType; };
    attach.setParent=[&s](auto form,auto,auto into) { o::CommonContainedSetParent(s.pool,form,into); };attach.setPrevious=[&s](auto form,auto table,auto previous) { o::CommonContainedSetPreviousAtTable(s.pool,form,table,previous); };
    attach.pop=[&s](auto form,auto popFlags) { // 공통 Pop의 실제 깊이와 현재 파생 PostPop을 재사용한다.
        o::FormPopHooks pop;pop.assertion=[&s](auto expression,auto file,auto line) { s.storage->AssertDisplay(expression,file,line); };pop.server=[&s] { return s.pool.IsServer(); };
        pop.depth=[&s] { return s.postPop->ActivationDepth(); };pop.setDepth=[&s](auto value) { s.postPop->ActivationDepth()=value; };pop.postPop=[&s](auto object,auto,auto value) { s.postPop->PostPopVirtual(object,value); };
        pop.transmit=[&s](auto object,auto,auto,auto,auto value) { if (!s.squidTransmission) throw std::logic_error("종속 자산의 실제 Squid 전송 미연결");s.squidTransmission->Pop(object,value); };
        o::PopCommonForm(s.pool,form,popFlags,pop);
    };
    o::AttachCommonForm(s.pool,sid,parent,flags,attach);s.changed=true;
}
// 현재 raw 타입의 실제 공간/표시 해제 몸체를 호출한다.
void RawSurfaceWorld::Unpop(o::Sid sid,std::uint32_t flags) {
    auto& s=*impl_;s.unpop->Unpop(sid,s.types.at(s.pool.Slot(sid)[10]),flags);s.changed=true;
}
// Unpop 뒤 현재 raw 좌표를 읽어 같은 공간에 다시 Pop한다. 원본 호출자가 해제 순서를 정한다.
void RawSurfaceWorld::Repop(o::Sid sid,std::uint32_t flags) {
    const auto raw=impl_->pool.Slot(sid);impl_->Pop(sid,std::bit_cast<float>(Read(raw,14)),std::bit_cast<float>(Read(raw,18)),flags);
}
// 예약 프로세스를 연결 모듈보다 먼저 해제해 가상 소멸자의 참조 수명을 보장한다.
void RawSurfaceWorld::ClearProcesses() { impl_->kernel.Clear(); }
// 최종 소멸자는 지난 슬롯의 후속 등록도 같은 모듈이 살아 있을 때 반납해야 한다.
void RawSurfaceWorld::DrainProcesses() { do { impl_->kernel.Clear(); } while (impl_->kernel.Size()); }
// 완성된 새 Session의 후반 실패도 이전 Native 잔류를 제거하지 않고 자기 현재 처리기만 반납한다.
void RawSurfaceWorld::DrainCurrentProcessBindingsForShutdown() {
    impl_->DrainCurrentProcessesForShutdown(); // 명시 종료와 직접 World 소멸이 같은 현재 소유권 필터를 사용한다.
}
// 명시 World 대여에서 실제 Native 단일 Clear가 끝난 경우만 증빙을 전달한다.
RawProcessHostStorage::ClearPass RawSurfaceWorld::ClearProcessesForState() {
    if (!impl_->processUse) throw std::logic_error("장기 ProcessHost 현재 대여 없는 State Clear");return impl_->processUse->ClearForState();
}
void RawSurfaceWorld::ReturnProcessBindingsForState(RawProcessHostStorage::ClearPass&& pass) {
    if (!impl_->processUse || impl_->returnedProcesses) throw std::logic_error("ProcessHost State 반납의 단일 실제 대여 필요");
    impl_->returnedProcesses.emplace(impl_->processUse->ReturnForState(std::move(pass)));impl_->processUse.reset();
} // 현재 대여를 소비한 뒤에만 증빙을 보관하며 Kernel/잔류를 강제 정리하지 않는다.
bool RawSurfaceWorld::ProcessBindingsReturned() const { return bool(impl_->returnedProcesses); } // 실제 반납 객체의 존재만 진단한다.
// 중첩 로딩의 표면/사제/자산 후처리가 같은 값을 읽게 한다.
void RawSurfaceWorld::SetLoadingDepth(std::uint32_t depth) {
    impl_->loadState.loadingDepth=depth;if (impl_->runtimeBound) impl_->runtime.loadingDepth(depth);
}
// 저장 객체의 순서와 부착 생성 순서는 구별한다. Graph는 전체 초기 등록을 마친 뒤 한 번 재구성한다.
void RawSurfaceWorld::Load(std::span<const SurfaceSeed> seeds) {
    auto& state=*impl_;if (state.loaded) throw std::logic_error("raw 표면 월드 중복 로드");
    std::vector<SurfaceSeed> sorted(seeds.begin(),seeds.end());
    // 쓰기 전에 전체 초기 입력의 지원 타입/정수 좌표/프레임/소유자를 검사한다.
    for (const auto& seed:sorted) {
        if (seed.type!=state.terrainType && seed.type!=state.links.noIslandType && seed.type!=state.links.bridgeType)
            throw std::invalid_argument("raw 표면 월드의 미지원 초기 타입");
        if (!std::isfinite(seed.x) || !std::isfinite(seed.y) || seed.x<1 || seed.y<1 || seed.x>=256 || seed.y>=256 ||
            std::trunc(seed.x)!=seed.x || std::trunc(seed.y)!=seed.y || seed.owner>o::kPlayerCount || seed.frame<0 ||
            static_cast<std::size_t>(seed.frame)>=state.frames[seed.type].Codes().size()) throw std::out_of_range("raw 표면 월드 초기 필드 오류");
    }
    // 비유한 좌표를 거부한 뒤 정렬한다. 받침 접두가 서/북 이웃을 읽으므로 noIsland는 행 우선이다.
    const auto rank=[&state](std::uint32_t type) { return type==state.terrainType ? 0 : type==state.links.noIslandType ? 1 : 2; };
    std::stable_sort(sorted.begin(),sorted.end(),[&](const SurfaceSeed& first,const SurfaceSeed& second) {
        if (rank(first.type)!=rank(second.type)) return rank(first.type)<rank(second.type);
        return first.y==second.y ? first.x<second.x : first.y<second.y;
    });
    BeginLoad();
    // 실제 생성자·가상 소유자·Pop을 사용하며 아직 Graph를 변경하지 않는다.
    for (const auto& seed:sorted) {
        const auto sid=state.factory->Create(seed.type);SetAssetOwner(sid,seed.owner);
        Write(state.pool.AllocatedBytes(sid),state.edition==o::OriginalEdition::Patch1078 ? 36 : 34,static_cast<std::uint32_t>(seed.frame),state.edition==o::OriginalEdition::Patch1078 ? 4 : 1);
        state.Pop(sid,seed.x,seed.y,0);
    }
    FinishLoad();
}
// GUI의 준비 단계는 시각만 쓰며 Kernel을 중첩 실행하지 않는다.
void RawSurfaceWorld::PrepareFrame(o::FrameTime time) {
    if (impl_->processState.sharedNow) time.game=impl_->processState.Now(); // 실제 현재 시각으로 독립 스냅샷의 게임 값만 교정한다.
    if (!std::isfinite(time.game) || time.game<impl_->time.game) throw std::invalid_argument("raw 월드의 역행/비유한 게임 시각");
    if (!impl_->processState.sharedNow) impl_->processState.now=time.game; // bound Client 시각은 스냅샷으로 역쓰기하지 않는다.
    impl_->time=time;
}
// 독립 호출자는 같은 준비 뒤 Kernel을 한 번 실행하며 기존 정지 계약을 유지한다.
void RawSurfaceWorld::RunFrame(o::FrameTime time,bool paused) { ScanBridgeDecay(time,paused);if (!paused) impl_->kernel.RunFrame(); }
//초기 상태를 재설정하지 않고 같은 현재 전역/장치만 설치한다.
void RawSurfaceWorld::BindBridgeDecay(const std::uint32_t& editor,std::function<void(o::Sid)> carriers,std::function<void(o::Sid)> sound,std::function<void(o::FrameTime)> time) {
    auto& s=*impl_;if (s.decayEditor || !carriers || !sound) throw std::logic_error("실제 다리 붕괴 장치 중복/미완성 연결");
    s.decayEditor=&editor;s.decayCarriers=std::move(carriers);s.decaySound=std::move(sound);s.decayTime=std::move(time);s.virtualDestroy->BindCurrentModes(*s.storage->Authority(),&editor);
}
//같은 시각을 먼저 공급한 호출자가 프레임의 원문 위치에서 한 칸을 요청한다.
void RawSurfaceWorld::DecayBridgeCell(o::FrameTime time,o::Sid sid) { PrepareFrame(time);if (!impl_->bridgeDecay) throw std::logic_error("다리 붕괴의 현재 PATCH 몸체 미연결");if (impl_->decayTime) impl_->decayTime(impl_->time);impl_->bridgeDecay->Cell(sid); }
//독립 World의 프레임은 Kernel 전에 같은 저장소 커서를 진행한다. Client는 전역 공통 입구에서 한 번 호출한다.
void RawSurfaceWorld::ScanBridgeDecay(o::FrameTime time,bool paused) {
    PrepareFrame(time);auto& s=*impl_;if (s.decayTime) s.decayTime(s.time);if (!s.bridgeDecay) return; //CD의 인라인 스캔은 후속 복원 대상이며 이 연결은10.78에 한정한다.
    auto current=s.mode;current.server=s.storage->Authority()->server;current.authority=s.storage->Authority()->boss;if (s.decayEditor) current.editor=*s.decayEditor!=0;
    o::RawBridgeDecayScan scan(s.pool,s.types,s.storage->bridgeDecayScan,s.storage->bridgeDecayPeriod,current,[&s](o::Sid sid) { s.bridgeDecay->Cell(sid); });scan.Frame(s.time,paused);
}
// 현재 카메라의 원본 픽셀 기준값과 GUI 변경 표를 연결한다.
void RawSurfaceWorld::SetDisplay(o::SquidDisplayView view,SurfaceDisplayHooks hooks) { impl_->display->SetView(view);impl_->displayHooks=std::move(hooks); }
// 일반 4단계 해시와 실제 타입 필터를 사용하는 조회다.
o::Sid RawSurfaceWorld::Find(float x,float y,std::uint32_t type) const { return impl_->connect->FindTypeAt(x,y,type); }
// 다리 거부 조건과 파생 pre/post를 동일한 호출로 실행한다.
bool RawSurfaceWorld::Destroy(o::Sid sid,std::uint32_t flags) { return impl_->Destroy(sid,flags); }
// 받침/종유석/9칸 소유자 갱신은 복원된 원본 몸체를 사용한다.
void RawSurfaceWorld::SetSupportOwner(float x,float y,std::uint32_t owner) { impl_->surface->SetSupportOwner(x,y,owner);impl_->changed=true; }
// 상위 사제/신전 분배 뒤의 나머지 자산은 같은 현재 VT와 생산 장부를 사용한다.
void RawSurfaceWorld::SetAssetOwner(o::Sid sid,std::uint32_t owner) { impl_->assetOwner(sid,owner);impl_->changed=true; }
// GUI 렌더링에 임시 객체의 옛 프레임/좌표를 복사하지 않는다.
std::vector<SurfaceSnapshot> RawSurfaceWorld::Objects() const {
    std::vector<SurfaceSnapshot> result;const bool patch=impl_->edition==o::OriginalEdition::Patch1078;
    // 현재 살아 있는 자산 번호만 읽는다. ProcessForm/반납 슬롯은 표시 대상이 아니다.
    for (std::uint32_t id=5;id<impl_->pool.Capacity();++id) {
        const o::Sid sid{static_cast<std::uint16_t>(id)};const auto raw=impl_->pool.Slot(sid);
        if ((raw[11]&kInactive) || raw[10]<o::kFirstAssetTypeNumber) continue;
        result.push_back({sid,{raw[10],raw[patch ? 34 : 32],static_cast<std::int32_t>(Read(raw,patch ? 36 : 34,patch ? 4 : 1)),
            std::bit_cast<float>(Read(raw,14)),std::bit_cast<float>(Read(raw,18))}});
    }
    return result;
}
// 변경은 Renderer 억제 여부와 무관하게 한 번만 소비한다.
bool RawSurfaceWorld::TakeChanged() { return std::exchange(impl_->changed,false); }
// 기존 저장 객체 보고와 구별되는 현재 raw 슬롯/Graph/프로세스 상태다.
// 실제 월드가 소유한 연결 생성기를 빌리며 별도 생성기를 만들지 않는다.
o::RawBridgeConnect& RawSurfaceWorld::BridgeConnections() { return *impl_->connect; }
std::string RawSurfaceWorld::Report() const {
    std::ostringstream out;out.precision(12);std::uint32_t surfaces=0,graphs=0;
    // 예약 0번 다음의 실제 연결 성분을 센다.
    for (std::size_t i=1;i<o::Graph::kCount;++i) if (impl_->graph->Records()[i].inUse) { ++graphs;surfaces+=static_cast<std::uint16_t>(impl_->graph->Records()[i].surfaces); }
    out<<"raw_world\t"<<impl_->time.game<<'\t'<<impl_->time.number<<'\t'<<impl_->kernel.Size()<<'\t'<<surfaces<<'\t'<<graphs<<'\t'<<impl_->bookkeeping.TotalCost()<<'\t'<<impl_->bookkeeping.depth<<'\t'<<impl_->baseDestroy->PreDepth()<<'\t'<<impl_->baseDestroy->PostDepth()<<'\t'<<impl_->effectRequests<<'\t'<<impl_->deletionRequests<<'\n';
    // 실제 Graph 표의 앞 두 signed WORD를 그대로 관측한다. 배치 직후 장부와 실제 소속 수의 차이를 보정하지 않는다.
    for (std::size_t index=0;index<o::Graph::kCount;++index) {
        const auto& record=impl_->graph->Records()[index]; // 예약0번도 독립 행으로 남기며 미정 reserved WORD는 비교하지 않는다.
        out<<"raw_graph_record\t"<<index<<'\t'<<record.surfaces<<'\t'<<record.inUse<<'\n';
    }
    const auto& scan=impl_->storage->bridgeDecayScan;out<<"bridge_decay\t"<<scan.first<<'\t'<<scan.last<<'\t'<<scan.cursor<<'\t'<<scan.next<<'\t'<<impl_->storage->bridgeDecayPeriod<<'\n'; //CLI 관측은 실제 현재 전역만 읽고 스캔/초기화를 실행하지 않는다.
    const auto terrainFrames=impl_->frames[impl_->terrainType].Run('J');out<<"raw_terrain_type\t"<<impl_->terrainType<<'\t'<<terrainFrames.first<<'\t'<<terrainFrames.count<<'\n'; //실제지면의기본/원소프레임범위를읽기만한다.
    const auto& islandRecords=impl_->storage->islands.records;out<<"raw_islands\t"<<Read(islandRecords,0,4)<<'\n';
    for (std::uint32_t index=0;index<std::min(Read(islandRecords,0,4),100U);++index) { const auto at=4+index*32;out<<"raw_island\t"<<index<<'\t'<<Read(islandRecords,at,4)<<'\t'<<Read(islandRecords,at+4,1)<<'\t'<<Read(islandRecords,at+8,4)<<'\n'; } //실제신전참조/존재/소유자를관측하며목록이나테마를다시계산하지않는다.
    // 렌더링과 같은 raw 조회 결과를 번호별로 제공한다.
    for (const auto& snapshot:Objects()) {
        const auto& seed=snapshot.object;out<<"raw_object\t"<<snapshot.sid.value<<'\t'<<seed.type<<'\t'<<seed.owner<<'\t'<<seed.frame<<'\t'<<seed.x<<'\t'<<seed.y<<'\n';
        const auto raw=impl_->pool.Slot(snapshot.sid);const bool patch=impl_->edition==o::OriginalEdition::Patch1078; // 같은 실제 슬롯의 상태/extra/HP 저장 비트다.
        out<<"raw_asset_fields\t"<<snapshot.sid.value<<'\t'<<static_cast<std::uint32_t>(raw[11])<<'\t'<<static_cast<std::uint32_t>(raw[patch ? 40 : 35])<<'\t'<<Read(raw,26,patch ? 4 : 2)<<'\n';
        if (impl_->types[raw[10]].flags1&o::TypeFlag1::kSurface) out<<"raw_surface_graph\t"<<snapshot.sid.value<<'\t'<<static_cast<std::uint32_t>(raw[patch ? 30 : 28])<<'\n'; // 렌더링에 쓰는 같은 표면의 실제 소속 BYTE이며 번호 재할당이나 flood를 실행하지 않는다.
        if (impl_->types[raw[10]].flags2&4U) { const auto codes=impl_->frames[raw[10]].Codes();if (seed.frame<0 || static_cast<std::size_t>(seed.frame)>=codes.size()) throw std::out_of_range("다리 현재 수명 보고의 프레임 범위");out<<"raw_bridge_fields\t"<<snapshot.sid.value<<'\t'<<Read(raw,12,2)<<'\t'<<static_cast<std::uint32_t>(codes[static_cast<std::size_t>(seed.frame)].flags)<<'\n'; } //CLI 읽기 전용 관측이며 실제 수명/Hard/Cracked 비트를 임의 주입하지 않는다.
        const auto table=Read(raw,0,4); // 원본 두FactoryVT의현재내용물을모든원소워크샾에서같은방식으로관측한다.
        if (patch && o::ObjectConversionBody(table)) { // 타입전환/저장재적재전후에같은실제Form과현재Regular예약을관측한다.
            std::size_t visited{},regulars{}; // 순환검사와실제등록된예약수이며상태/시각을변경하지않는다.
            for (o::Sid form{static_cast<std::uint16_t>(Read(raw,6,2))};form.value;) { // 현재부모의실제종속체인만따라가며임의프로세스를찾아붙이지않는다.
                if (++visited>impl_->pool.Capacity()) throw std::logic_error("비석/주문 진단의 Form 체인 순환");const auto content=impl_->pool.Slot(form); // 같은50BYTE물리의종류와다음링크다.
                if (content[10]==46 || content[10]==61) {
                    const auto* process=dynamic_cast<const o::RegularProcess*>(impl_->host->FromForm(form)); // 현재Kernel번호로기존Regular/SharedRegular객체만조회한다.
                    if (!process || process->Form()!=form || process->Parent()!=snapshot.sid) throw std::logic_error("비석/주문 진단의 실제 Regular 등록 불일치");
                    out<<"raw_object_regular\t"<<snapshot.sid.value<<'\t'<<form.value<<'\t'<<static_cast<std::uint32_t>(content[10])<<'\t'<<process->Event()<<'\t'<<process->Count()<<'\t'<<std::bit_cast<std::uint32_t>(process->Payload())<<'\t'<<process->Time()<<'\n';++regulars;
                }
                form={static_cast<std::uint16_t>(Read(content,4,2))};
            }
            out<<"raw_object_regular_count\t"<<snapshot.sid.value<<'\t'<<regulars<<'\n';
        }
        if (patch ? (table==0x509148U || table==0x509210U) : (raw[10]==85 || raw[10]==126)) { // 10.78의두생성자표를사용하며10.72의기존진단조건은보존한다.
            std::size_t visited{};
            // 같은 실제 Form 체인 전체를 읽고 저장 종류/flags와 client/server SID를 기록한다.
            for (o::Sid form{static_cast<std::uint16_t>(Read(raw,6,2))};form.value;) {
                if (++visited>impl_->pool.Capacity()) throw std::logic_error("Factory 진단의 Form 체인 순환");const auto content=impl_->pool.Slot(form);
                if (content[10]==6) out<<"raw_factory_content\t"<<snapshot.sid.value<<'\t'<<form.value<<'\t'<<Read(content,18,2)<<'\t'<<Read(content,20,2)<<'\n';
                form={static_cast<std::uint16_t>(Read(content,4,2))};
            }
        }
    }
    return out.str();
}
}
