// 같은 장기 Pool/Graph/공통 삭제로 표면 수명을 소유하고 큰 파생 효과만 현재 Runtime에서 빌린다.
#include "client/RawSurfaceWorld.h"
#include "o/RawBridgeEvents.h"
#include "o/RawBridgeFallParent.h" // 같은 Host의 현재 TYPE/checking 낙하 예약 진입을 선언한다.
#include <stdexcept>
#include <utility>

namespace netstorm::client { // World 생성/Acquire/값 초기화 없이 실제 현재 장치를 연결한다.
namespace { // 선택된 큰 표면 효과의 현재 외곽 수명만 보유한다.
// 실제 전체 owner를 보유한 뒤 현재 함수 사본을 호출하며 원문의 선행 효과/후속 분배는 변경하지 않는다.
template<class Function,class... Args>
decltype(auto) SurfaceLifetimeCall(const std::weak_ptr<const RawSurfaceLifetimeBindings>& weak,Function RawSurfaceLifetimeBindings::* member,Args&&... args) {
    const auto current=weak.lock();if (!current) throw std::logic_error("표면의 선택된 실제 큰 Runtime 미연결");
    const auto pin=current->pin;if (!pin) throw std::logic_error("표면의 선택된 실제 전체 Runtime pin 미연결");
    const auto owner=pin();if (!owner) throw std::logic_error("표면의 선택된 실제 전체 Runtime owner 반납");
    const auto call=current.get()->*member;if (!call) throw std::logic_error("표면의 선택된 실제 큰 효과 미연결");return call(std::forward<Args>(args)...);
}
}
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
        hooks.destroy=[this](auto sid,auto flags) { DestroyAsset(sid,flags); }; //중첩된실제연결객체도현재World또는같은장기VT분배를다시선택한다.
        hooks.fallWalker=[this](auto sid) { SurfaceLifetimeCall(surfaceLifetimeRuntime_,&RawSurfaceLifetimeBindings::fallWalker,sid); };
        hooks.bridgeEffect=[this](const auto& event) { SurfaceLifetimeCall(surfaceLifetimeRuntime_,&RawSurfaceLifetimeBindings::bridgeEffect,event); };
        auto body=std::make_unique<o::RawSurfaceLifecycle>(pool,Neighbors(),links.connectorType,CommonDeletion().Hooks(),std::move(hooks));body->BindCurrentConnectorType(links.connectorType);surfaceLifetime_=std::move(body);
    }
    return *surfaceLifetime_;
}
// 끝 칸 변환의 기존 본문과 현재 공통 장치를 사용하며 실제 World 대여를 만들지 않는다.
o::RawBridgeEvents& RawSurfaceStorage::BridgeRegular() {
    if (!UsesNativeTransmissionDevices()) throw std::logic_error("장기 다리 Regular의 Native 판본 계약");
    if (!nativeBridgeEvents_) {
        const auto objects=SurfaceObjects();o::BridgeEventHooks hooks;
        hooks.notifySurface=objects.notifySurface;hooks.create=[this](auto type) { return CreateAsset(type,0); };
        hooks.destroy=[this](auto sid,auto flags) { DestroyAsset(sid,flags); };
        hooks.setOwner=[this](auto sid,auto owner) { SetAssetOwner(sid,owner); };hooks.pop=[this](auto sid,auto x,auto y,auto flags) { static_cast<void>(PopAsset(sid,x,y,flags)); };
        auto body=std::make_unique<o::RawBridgeEvents>(pool,nativeBridgeEventState_,o::MakeBridgeNeighborHooks(Neighbors(),std::move(hooks)));
        body->BindCurrentState(authority_->boss,nativeDestroyMode_.debugKeep,links.bridgeType);nativeBridgeEvents_=std::move(body);
    }
    return *nativeBridgeEvents_;
}
}
