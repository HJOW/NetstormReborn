// 감시 쌍·현재 Form 공장/TYPE·가상 삭제를 같은 Client 물리에 장기 연결한다.
#include "client/RawSurfaceWorld.h"
#include <stdexcept>
#include <utility>

namespace netstorm::client { // World/Session을 생성하거나 현재 전역을 복사하지 않는다.
void RawSurfaceStorage::BindDependRuntime(const std::shared_ptr<const RawSurfaceDependBindings>& runtime) {
    if (!UsesNativeTransmissionDevices() || !runtime || !dependRuntime_.expired()) throw std::invalid_argument("현재 Depend 알림 대여의 중복/누락");dependRuntime_=runtime; // 빈 알림은 실제 장기 VT 본문으로 이어진다.
}
// Native Session과 NoWorld가 같은 주소의 실제 감시 본체를 사용한다.
o::DependFormHost& RawSurfaceStorage::DependForms() {
    if (!UsesNativeTransmissionDevices() || pool.Edition()!=o::OriginalEdition::Patch1078) throw std::logic_error("장기 Depend의 실제 PATCH Client 장치 미연결");
    if (!dependForms_) {
        auto body=std::make_unique<o::DependFormHost>(pool,BaseDestroy(),[this](auto watcher,auto watched,auto form) { DeliverDependNotice(watcher,watched,form); },DefaultProcessAssetHooks()); // Form 전송은 같은 공장의 현재 Runtime/장기 기본 경계를 그대로 선택한다.
        body->BindCreation(Factory(),gameAux_->DependFormType(),[this](auto expression,auto file,auto line) { AssertDisplay(expression,file,line); });
        body->BindUnpop([this](auto sid) { Factory().UnpopForm(sid); });
        body->BindVirtualDestroy([this](auto sid,auto flags) { static_cast<void>(DestroyAsset(sid,flags)); },[this](auto parent) {
            if (Checking() && !ownerMode.challenge && (pool.Slot(parent)[11]&2U) && !authority_->boss) AssertDisplay("inMetamapMode || !cont.isDead() || boss || yesIKnowThatIShouldntLookInsideMyselfDuringADestroy","\\Ns\\O\\Squidfinder.cpp",0x158); // 진단이 정상 복귀한 뒤 현재 head를 읽는다.
        });
        dependForms_=std::move(body);
    }
    return *dependForms_;
}
// 현재 실제 가상34는 알림 대여로 재전달하지 않아 Session의 직접 호출이 순환하지 않는다.
void RawSurfaceStorage::NotifyDependency(o::Sid watcher,o::Sid watched,o::Sid form) {
    if (NotifyAssetDependency(watcher,watched,form.value)) return;
    const auto raw=pool.Slot(watcher);const auto table=std::uint32_t(raw[0])|(std::uint32_t(raw[1])<<8)|(std::uint32_t(raw[2])<<16)|(std::uint32_t(raw[3])<<24);
    if (table==0x512110U || table==0x512188U || table==0x512208U) return; // 실제 세 표의+34는 모두0041c100이며 TYPE/owner/server/인수를 읽지 않는다.
    if (table==0x512098U) { Processes().Host().NotifyDependency(watcher,watched,form);return; } // 004ae280의 서버 관문·실제 현재 PID 통지를 유지한다.
    throw std::logic_error("현재 Depend 수신자의 실제 가상34 미연결");
}
// 약한 대여를 선택한 뒤 실제 외곽 pin과 함수 사본을 호출 종료까지 유지한다.
void RawSurfaceStorage::DeliverDependNotice(o::Sid watcher,o::Sid watched,o::Sid form) {
    const auto current=dependRuntime_.lock();
    if (current && current->notice) { const auto pin=current->pin;const auto runtimeOwner=pin ? pin():std::shared_ptr<void>{};const auto notice=current->notice;notice(watcher,watched,form);return; }
    NotifyDependency(watcher,watched,form);
}
}
