// 공통 삭제의 현재 장부·Graph·보상을 Client 수명으로 공유하고 파생 종단은 짧게 빌린다.
#include "client/RawSurfaceWorld.h"
#include <stdexcept>
#include <utility>

namespace netstorm::client { // 같은 실제 원문 전역만 사용하며 World/AI/성공 값을 새로 만들지 않는다.
// 장치는 연결만 하며 아직 생성되지 않은 메뉴/원문 전역의 값을 읽지 않는다.
void RawSurfaceStorage::BindDeletionDevices(RawSurfaceDeletionDevices devices) {
    if (!UsesNativeTransmissionDevices() || !rewardDevices_.sp || borrowed_ || commonDeletion_ || deletionDevices_.unitLost || !devices.unitLost || !devices.rememberPlacement) throw std::invalid_argument("장기 공통 삭제의 실제 Native 최초 장치 계약");deletionDevices_=std::move(devices);
}
// 일반 fresh 대여가 상태를 교체해도 같은 장기 원문 주소만 다시 연결한다.
void RawSurfaceStorage::RestoreDeletionGlobals() {
    if (!serializationGlobals_) throw std::logic_error("공통 삭제의 실제 현재 로딩 깊이 owner 미연결");
    deletionState.sharedLoadingDepth=&serializationGlobals_->loadingDepth;deletionState.sharedFortMode=&ownerMode.fort;deletionState.sharedSilentType=&gameAux_->RainBlockerType();
    deletionState.sharedSpecialSurfaceType=&links.noIslandType;deletionState.sharedReplacementSurfaceType=&gameAux_->FakeSurfaceType();deletionState.sharedLargeSurfaceType=&gameAux_->FakeSurfaceType();
}
// 실제 파생 callback을 소유한 현재 World만 연결하고 반납된 Session을 보관하지 않는다.
void RawSurfaceStorage::BindDeletionRuntime(const std::shared_ptr<const RawSurfaceDeletionBindings>& runtime) {
    if (!runtime || !runtime->hooks.forward || !deletionRuntime_.expired()) throw std::invalid_argument("현재 공통 삭제 파생 대여의 중복/실제 종단 누락");deletionRuntime_=runtime;
}
// 장기 공통 부모는 같은 HP/보상/장부를 사용하고 실제 선택된 파생 효과만 현재 owner를 빌린다.
o::SquidDestroyLifecycle& RawSurfaceStorage::CommonDeletion() {
    if (!UsesNativeTransmissionDevices() || !deletionDevices_.unitLost || !postPopDevices_.aiNotice || !postPopDevices_.localOwner) throw std::logic_error("장기 공통 삭제의 실제 Client 장치 미연결");RestoreDeletionGlobals();
    if (!commonDeletion_) {
        o::SquidDeletionHooks hooks;
        hooks.selectedRaw=[this] { return CurrentProcessSelection(); };hooks.localOwner=[this] { const auto owner=postPopDevices_.localOwner;return owner(); };
        hooks.rememberPlacement=[this](auto x,auto y,auto type) { const auto remember=deletionDevices_.rememberPlacement;remember(x,y,type); };
        hooks.emit=[this](const o::SquidDeletionEvent& event) {
            if (event.effect==o::SquidDeletionEffect::ClearSelection) { DefaultProcessAssetHooks().emit({o::SquidDestroyEffect::ClearSelection,event.sid,{},0});return; }
            if (event.effect==o::SquidDeletionEffect::UnitLostSound) { const auto sound=deletionDevices_.unitLost;sound(event.sid);return; }
            throw std::logic_error("장기 공통 삭제 Refund는 같은 실제 HP/보상만 사용합니다");
        };
        hooks.forward=[this](const o::SquidDestroyEvent& event) {
            if (const auto current=deletionRuntime_.lock()) { const auto pin=current->pin ? current->pin():std::shared_ptr<void>{};const auto forward=current->hooks.forward;forward(event);return; }
            DefaultProcessAssetHooks().emit(event); // NoWorld의 실제 전송/선택은 실행하며 아직 없는 파생 종속 삭제는 해당 위치에서 명시 실패한다.
        };
        auto common=std::make_unique<o::SquidDestroyLifecycle>(pool,types,postPopState,deletionState,BaseDestroy(),std::move(hooks),&Graph(),&Reward());
        common->BindPatchAiNotice([this](auto owner,auto sid,auto added) { const auto notice=postPopDevices_.aiNotice;notice(owner,sid,added); });commonDeletion_=std::move(common);
    }
    return *commonDeletion_;
}
}
