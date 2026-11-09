// UI Form은 실제 Client의 공장/현재 TYPE/물리와 실제 UI 등록표를 장기 공유한다.
#include "client/RawSurfaceWorld.h"
#include <stdexcept>
#include <utility>

namespace netstorm::client { // 임시 World/Session/등록표나 성공 대체를 만들지 않는다.
void RawSurfaceStorage::BindGumpRuntime(const std::shared_ptr<const RawSurfaceGumpBindings>& runtime) {
    if (!UsesNativeTransmissionDevices() || !runtime || !gumpRuntime_.expired()) throw std::invalid_argument("현재 Gump World 통지 대여 중복/누락");gumpRuntime_=runtime;
}
// 실제 UI 저장소는 Session보다 오래 살고 화면 종료 때 명시적으로 반납한다.
void RawSurfaceStorage::BindGumpUiRuntime(const std::shared_ptr<const RawSurfaceGumpBindings>& runtime) {
    if (!UsesNativeTransmissionDevices() || !runtime || !runtime->notice || !runtime->pin || !gumpUiRuntime_.expired()) throw std::invalid_argument("현재 Gump UI 등록표/수명 대여 중복/누락");gumpUiRuntime_=runtime;
}
// getter는 객체/타입/장치를 초기화하지 않고 같은 공장과 현재 Gump DWORD를 빌린다.
o::GumpFormHost& RawSurfaceStorage::GumpForms() {
    if (!UsesNativeTransmissionDevices() || pool.Edition()!=o::OriginalEdition::Patch1078) throw std::logic_error("장기 Gump의 실제 PATCH Client 장치 미연결");
    if (!gumpForms_) {
        auto body=std::make_unique<o::GumpFormHost>(pool,Factory(),types,BaseDestroy(),[this](auto owner,auto parent) { DeliverGumpNotice(owner,parent); },DefaultProcessAssetHooks());
        body->BindCurrent(containedGumpType,[this](auto sid,auto flags) { static_cast<void>(DestroyAsset(sid,flags)); });gumpForms_=std::move(body);
    }
    return *gumpForms_;
}
// 각 통지에서 실제 현재 owner와 등록표를 선택하며 소유 ID0 생략은 원문 Gump 몸체가 먼저 처리한다.
void RawSurfaceStorage::DeliverGumpNotice(std::uint32_t owner,o::Sid parent) {
    const auto current=gumpRuntime_.lock();const auto ui=gumpUiRuntime_.lock();
    const auto runtimePin=current && current->notice ? current->pin:std::function<std::shared_ptr<void>()>{};const auto runtimeOwner=runtimePin ? runtimePin():std::shared_ptr<void>{};
    const auto uiPin=ui ? ui->pin:std::function<std::shared_ptr<void>()>{};const auto uiOwner=uiPin ? uiPin():std::shared_ptr<void>{};
    const auto notice=current && current->notice ? current->notice : ui ? ui->notice:o::GumpFormNotice{};
    if (!notice) throw std::logic_error("현재 Gump 실제 UI 등록표/가상 통지 미연결");notice(owner,parent);
}
}
