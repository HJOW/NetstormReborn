// 개별 함수의 원본 대조 근거는 bridgeeffects·islandlifecycle 문서, 연결 순서 검사는 SurfaceLifecycleTests다.
#include "o/RawSurfaceLifecycle.h"
#include "o/RawBridgeEvents.h"
#include "o/RawIslandPostPop.h"
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 슬롯 첫 DWORD의 원본 가상 표 기록값을 읽는다. 원본 주소를 호스트 포인터로 실행하지 않는다.
std::uint32_t Vtable(std::span<const std::uint8_t> raw) {
    std::uint32_t value=0;
    // little endian 순서로 네 바이트를 합친다.
    for (std::size_t i=0;i<4;++i) value|=static_cast<std::uint32_t>(raw[i])<<(8*i);
    return value;
}
}
// 공통 삭제 훅을 각 파생 Base 호출에 연결하고 내부 탐색기의 수명을 분배기와 맞춘다.
RawSurfaceLifecycle::RawSurfaceLifecycle(const SidPool& pool,const RawSquidNeighbors& neighbors,std::uint32_t connectorType,
    SquidDestroyHooks common,SurfaceLifecycleHooks hooks)
    :pool_(pool),connectorType_(connectorType),common_(std::move(common)),hooks_(std::move(hooks)),
     finder_(pool,neighbors.Hash(),neighbors.Types()),bridge_(pool,neighbors.Types()),
     island_(pool,neighbors,IslandLifecycleHooks{
        // 섬 삭제가 예약하는 실제 지연 낙하 경로다.
        [this](Sid sid,float x,float y) { hooks_.scheduleFall(sid,x,y); },
        // noIsland 칸 위 walker의 가상 낙하다.
        [this](Sid sid) { hooks_.fallWalker(sid); },
        // 파생 접두 뒤에도 공통 preDestroy를 정확히 한 번 수행한다.
        [this](Sid sid,std::uint32_t flags) { common_.emit({SquidDestroyEffect::PreDestroy,sid,{},flags}); },
        // 표면 낙하 뒤 공통 postDestroy로 삭제 깊이/장부를 마무리한다.
        [this](Sid sid,std::uint32_t flags) { common_.emit({SquidDestroyEffect::PostDestroy,sid,{},flags}); }}),
     bridgeHooks_(MakeBridgeLifecycleHooks(finder_,[this](const BridgeLifecycleEvent& event) { EmitBridge(event); })) {
    if (connectorType<kFirstAssetTypeNumber || connectorType>=neighbors.Types().size())
        throw std::out_of_range("표면 삭제 연결 객체 타입 범위 오류");
    if (!common_.emit || !common_.selected || !hooks_.scheduleFall || !hooks_.destroy || !hooks_.fallWalker || !hooks_.bridgeEffect)
        throw std::invalid_argument("표면 삭제 공통/외부 훅 누락");
}
// 선택 조회와 form 해제 계약은 공통 훅에서 유지한다. 파생 삭제 사건만 분배한다.
SquidDestroyHooks RawSurfaceLifecycle::Hooks() {
    auto result=common_;
    result.emit=[this](const SquidDestroyEvent& event) { Emit(event); };
    return result;
}
// 연결 객체 삭제는 부모 다리의 dead 비트가 켜진 preDestroy 안에서 재귀 호출된다.
void RawSurfaceLifecycle::EmitBridge(const BridgeLifecycleEvent& event) {
    switch (event.effect) {
    case BridgeLifecycleEffect::DestroyLink:hooks_.destroy(event.sid,event.flags);break;
    case BridgeLifecycleEffect::BasePreDestroy:common_.emit({SquidDestroyEffect::PreDestroy,event.sid,{},event.flags});break;
    case BridgeLifecycleEffect::BasePostDestroy:common_.emit({SquidDestroyEffect::PostDestroy,event.sid,{},event.flags});break;
    case BridgeLifecycleEffect::FallWalker:hooks_.fallWalker(event.sid);break;
    default:hooks_.bridgeEffect(event);break;
    }
}
// 타입 번호만 같아도 다른 가상 표이면 공통 훅으로 보낸다. 실제 Factory가 기록한 파생 표를 기준으로 한다.
void RawSurfaceLifecycle::Emit(const SquidDestroyEvent& event) {
    const bool pre=event.effect==SquidDestroyEffect::PreDestroy,post=event.effect==SquidDestroyEffect::PostDestroy;
    if (pre || post) {
        const auto vtable=Vtable(pool_.Slot(event.sid));
        const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
        if (vtable==(patch ? kPatchBridgeVtable : kCdBridgeVtable)) {
            if (pre) bridge_.PreDestroy(event.sid,event.flags,connectorType_,bridgeHooks_);
            else bridge_.PostDestroy(event.sid,event.flags,bridgeHooks_);
            return;
        }
        if (pre && vtable==(patch ? kPatchIslandVtable : kCdIslandVtable)) {
            island_.IslandPreDestroy(event.sid,event.flags);return;
        }
        if (post && vtable==(patch ? kPatchNoIslandVtable : kCdNoIslandVtable)) {
            island_.SurfacePostDestroy(event.sid,event.flags);return;
        }
    }
    common_.emit(event);
}
}
