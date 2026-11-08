// 같은 장기 Pool/Graph/공통 삭제로 표면 수명을 소유하고 큰 파생 효과만 현재 Runtime에서 빌린다.
#include "client/RawSurfaceWorld.h"
#include "o/RawBridgeEvents.h"
#include "o/RawBridgeFallParent.h" // 같은 Host의 현재 TYPE/checking 낙하 예약 진입을 선언한다.
#include <stdexcept>
#include <utility>

namespace netstorm::client { // World 생성/Acquire/값 초기화 없이 실제 현재 장치를 연결한다.
// 짧은 World의 실제 효과 소유권만 연결하며 다른 활성 대여가 있으면 거부한다.
void RawSurfaceStorage::BindSurfaceLifetimeRuntime(const std::shared_ptr<const RawSurfaceLifetimeBindings>& runtime) {
    if (!UsesNativeTransmissionDevices() || !runtime || !runtime->fallWalker || !runtime->bridgeEffect || !surfaceLifetimeRuntime_.expired()) throw std::invalid_argument("현재 표면 수명 실제 종단의 중복/누락");surfaceLifetimeRuntime_=runtime;
}
// Native의 실제 현재 editor DWORD/권한과 같은 물리/Graph를 빌리며 mode 사본을 갱신하지 않는다.
o::RawSquidDestroyDispatch& RawSurfaceStorage::VirtualDestroy() {
    if (!UsesNativeTransmissionDevices() || !rewardDevices_.editor) throw std::logic_error("장기 가상 삭제의 현재 Native editor owner 미연결");
    if (!virtualDestroy_) { auto body=std::make_unique<o::RawSquidDestroyDispatch>(BaseDestroy(),Neighbors(),nativeDestroyMode_,&Graph());body->BindCurrentModes(*authority_,rewardDevices_.editor.get());virtualDestroy_=std::move(body); }
    return *virtualDestroy_;
}
// 작은 부모는 실제 같은 장기 장치를 사용하고 큰 선택 효과는 실제 살아 있는 대여에서만 호출한다.
o::RawSurfaceLifecycle& RawSurfaceStorage::SurfaceLifetime() {
    if (!UsesNativeTransmissionDevices()) throw std::logic_error("장기 표면 수명의 실제 Native owner 미연결");
    if (!surfaceLifetime_) {
        o::SurfaceLifecycleHooks hooks;
        hooks.scheduleFall=[this](auto sid,auto x,auto y) { o::ScheduleBridgeFallChecked(Processes().Host(),pool,types,sid,x,y,Checking()); };
        hooks.destroy=[this](auto sid,auto flags) { const auto destroy=resourceDevices_.destroy;if (!destroy) throw std::logic_error("표면 연결 객체의 실제 현재 가상 삭제 미연결");destroy(sid,flags); };
        hooks.fallWalker=[this](auto sid) { const auto current=surfaceLifetimeRuntime_.lock();if (!current) throw std::logic_error("표면의 선택된 실제 이동 유닛 낙하 owner 미연결");const auto pin=current->pin ? current->pin():std::shared_ptr<void>{};const auto fall=current->fallWalker;fall(sid); };
        hooks.bridgeEffect=[this](const auto& event) { const auto current=surfaceLifetimeRuntime_.lock();if (!current) throw std::logic_error("표면의 선택된 실제 파편/소리 owner 미연결");const auto pin=current->pin ? current->pin():std::shared_ptr<void>{};const auto effect=current->bridgeEffect;effect(event); };
        auto body=std::make_unique<o::RawSurfaceLifecycle>(pool,Neighbors(),links.connectorType,CommonDeletion().Hooks(),std::move(hooks));body->BindCurrentConnectorType(links.connectorType);surfaceLifetime_=std::move(body);
    }
    return *surfaceLifetime_;
}
}
