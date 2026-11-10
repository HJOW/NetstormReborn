// 내용물의 정렬·Regular·삭제를 같은 Client 풀/공장/현재 전역과 실제 장치에 연결한다.
#include "client/RawSurfaceWorld.h"
#include "o/RawContentParentNotice.h" // 실제 빈 부모와 파생 가상24를 구분한다.
#include <stdexcept>
#include <utility>

namespace netstorm::client { // 장기 본문은 Session을 만들거나 현재 게임 상태를 초기화하지 않는다.
// 실제 전역의 aliasing owner만 보유하며 연결 중 커서/Player/SP를 조회하거나 쓰지 않는다.
void RawSurfaceStorage::BindContentDevices(RawSurfaceContentDevices devices) {
    if (!UsesNativeTransmissionDevices() || contentDevices_.cursor || containedUnpop_ || borrowed_ || !devices.cursor || !devices.localOwner || !devices.contentType ||
        !devices.effects.cancelCursor || !devices.effects.localNotice || !devices.effects.remoteNotice || !devices.effects.sound)
        throw std::invalid_argument("장기 Content의 실제 최초 현재 전역/장치 계약");
    contentDevices_=std::move(devices);contentLifetimeState_.sharedLocalOwner=contentDevices_.localOwner.get();
}
// World는 실제 큰 통지만 짧게 보관하고 Storage에는 약한 대여만 남긴다.
void RawSurfaceStorage::BindContentRuntime(const std::shared_ptr<const RawSurfaceContentBindings>& runtime) {
    if (!UsesNativeTransmissionDevices() || !runtime || !contentRuntime_.expired()) throw std::invalid_argument("현재 Content 파생 통지 대여 중복/누락");contentRuntime_=runtime;
}
// 실제 Client는 최초 연결한 finder DWORD를 읽고 독립 저장소의 기존 번호 입력도 유지한다.
const std::uint32_t& RawSurfaceStorage::ContentType() const { return contentDevices_.contentType ? *contentDevices_.contentType:containedType; }
// callback은 선택 위치마다 같은 현재 preview 전체 DWORD를 읽으며 미연결을0으로 대체하지 않는다.
o::ContentLifetimeState& RawSurfaceStorage::ContentState() {
    contentLifetimeState_.sharedBoss=&authority_->boss;contentLifetimeState_.sharedNuggetType=&gameAux_->NuggetType();
    if (!contentLifetimeState_.currentCursor) contentLifetimeState_.currentCursor=[this] {
        const auto owner=contentDevices_.cursor;if (!owner) throw std::logic_error("장기 Content의 실제 현재 cursor owner 미연결");return *owner;
    };
    return contentLifetimeState_;
}
// 같은 실제 공장/현재 TYPE/공통 깊이와 현재 가상50을 정렬 본체에 한 번 연결한다.
o::ContentFormHost& RawSurfaceStorage::ContentForms() {
    if (!UsesNativeTransmissionDevices() || pool.Edition()!=o::OriginalEdition::Patch1078) throw std::logic_error("장기 Content의 실제 PATCH Client 장치 미연결");
    if (!contentForms_) {
        contentFormState_.inBattle.Bind(ownerMode.battle);contentFormState_.checkingLinks=Checking();
        contentFormState_.contentType=[this] { return ContentType(); };
        contentFormState_.assertion=[this](auto expression,auto file,auto line) { AssertDisplay(expression,file,line); };
        auto body=std::make_unique<o::ContentFormHost>(pool,types,[this](auto parent,auto form,auto reason) { NoticeContent(parent,form,reason); },o::FormPopTransmit{},&contentFormState_,
            [this](auto parent,auto previous) {
                const auto current=contentRuntime_.lock();if (!current) throw std::logic_error("Content의 실제 현재 포획 안내 Runtime 미연결");
                const auto pin=current->pin;if (!pin) throw std::logic_error("선택된 큰 후단의 실제 전체 Runtime pin 미연결");
                const auto owner=pin();if (!owner) throw std::logic_error("선택된 큰 후단의 실제 전체 Runtime owner 반납"); // 선택된 함수의 정상/예외 복귀까지 실제 소유자를 보유한다.
                const auto captured=current->captured;
                if (!captured) throw std::logic_error("Content의 실제 현재 포획 안내 장치 미연결");captured(parent,previous);
            });
        body->BindCurrentTypes(types);body->BindPop([this](auto sid,auto flags) { Factory().PopForm(sid,flags); });body->BindUnpop([this](auto sid) { Factory().UnpopForm(sid); });
        body->BindReattach([this](auto sid,auto parent,auto flags) {
            const auto raw=pool.Slot(sid);const auto table=std::uint32_t(raw[0])|(std::uint32_t(raw[1])<<8)|(std::uint32_t(raw[2])<<16)|(std::uint32_t(raw[3])<<24);
            const o::SquidForms forms(pool);if (table==forms.Vtable(6)) { ContentForms().Attach(sid,parent,flags);return; }
            for (const auto type:{1U,2U,3U}) if (table==forms.Vtable(type)) { Factory().AttachForm(sid,parent,flags);return; } // 현재 네 내장 Form의 실제 가상50만 선택한다.
            throw std::logic_error("Content Regular의 현재 가상50 미연결");
        });
        contentForms_=std::move(body);
    }
    return *contentForms_;
}
// 같은 장기 Reward가 실제 지갑/SP를 처리하고 다른 효과는 현재 Client 장치를 선택한다.
o::ContentFormLifetime& RawSurfaceStorage::ContentLifetime() {
    if (!contentLifetime_) {
        o::ContentLifetimeHooks effects;
        effects.cancelCursor=[this] { const auto call=contentDevices_.effects.cancelCursor;if (!call) throw std::logic_error("Content의 실제 커서 취소 미연결");call(); };
        effects.localNotice=[this](auto code) { const auto call=contentDevices_.effects.localNotice;if (!call) throw std::logic_error("Content의 실제 로컬 수입 큐 미연결");call(code); };
        effects.remoteNotice=[this](auto player) { const auto call=contentDevices_.effects.remoteNotice;if (!call) throw std::logic_error("Content의 실제 원격 수입 송신 미연결");call(player); };
        effects.sound=[this](auto x,auto y) { const auto call=contentDevices_.effects.sound;if (!call) throw std::logic_error("Content의 실제 위치 소리 미연결");call(x,y); };
        contentLifetime_=std::make_unique<o::ContentFormLifetime>(pool,types,ContentForms(),BaseDestroy(),ContentState(),std::move(effects),DefaultProcessAssetHooks(),&Reward());
    }
    return *contentLifetime_;
}
// 원문의 빈 가상24는 인수를 더 읽지 않으며 파생 호출만 실제 전체 Runtime pin을 보유한다.
void RawSurfaceStorage::NoticeContent(o::Sid parent,o::Sid form,std::uint32_t reason) {
    o::RawContentParentNotice notice(pool,[this](auto selectedParent,auto selectedForm,auto selectedReason) {
        const auto current=contentRuntime_.lock();if (!current) throw std::logic_error("Content의 실제 현재 파생 부모 Runtime 미연결");
        const auto pin=current->pin;if (!pin) throw std::logic_error("선택된 큰 후단의 실제 전체 Runtime pin 미연결");
        const auto owner=pin();if (!owner) throw std::logic_error("선택된 큰 후단의 실제 전체 Runtime owner 반납"); // 선택된 함수의 정상/예외 복귀까지 실제 소유자를 보유한다.
        const auto call=current->notice;
        if (!call) throw std::logic_error("Content의 실제 현재 파생 부모 가상24 미연결");call(selectedParent,selectedForm,selectedReason);
    });notice.Notice(parent,form,reason);
}
}
