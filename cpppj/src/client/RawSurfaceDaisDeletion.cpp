// 다이스·제단 삭제를 같은 장기 Pool/해시/현재 역할과 실제 Client 선택 효과에 연결한다.
#include "client/RawSurfaceWorld.h"
#include "client/ClientPlayerRecords.h"
#include "client/RawProcessHostStorage.h"
#include "o/RawDaisRegular.h"
#include <stdexcept>
#include <utility>

namespace netstorm::client { // World를 생성하거나 현재 Player/SP/난수를 초기화하지 않는다.
// 같은 장기16칸 사제 목록과9DWORD 원소유자 상태를 일반 참여 조회에 직접 연결한다.
o::RawPriestParticipation& RawSurfaceStorage::PriestParticipation() {
    if (!UsesNativeTransmissionDevices()) throw std::logic_error("장기 사제 참여의 Native 판본 계약");
    if (!priestParticipation_) {
        priestDeletionState_.sharedPriests=&sidLists.priests;priestDeletionState_.sharedOwnerState=&priestOwnerState;
        priestParticipation_=std::make_unique<o::RawPriestParticipation>(pool,hash,types,priestDeletionState_,CarrierPre(),o::PriestParticipationState{gameAux_->PriestType(),gameAux_->AltarType(),gameAux_->DaisType()});
    }
    return *priestParticipation_;
}
// 아직 존재하지 않는 Runtime을 조기 조회하지 않고 장치를 최초 한 번만 결합한다.
void RawSurfaceStorage::BindDaisFinish(std::function<void(o::Sid)> finish) {
    if (!UsesNativeTransmissionDevices() || borrowed_ || daisFinish_ || !finish) throw std::invalid_argument("장기 다이스 희생 완료의 최초 Client 장치 계약");daisFinish_=std::move(finish);
}
// 고유 삭제는 실제 현재 자료를 사용하고 큰 희생 완료가 선택된 위치에서만 현재 Client 진입을 요구한다.
o::RawDaisDeletion& RawSurfaceStorage::DaisDeletion() {
    if (!UsesNativeTransmissionDevices()) throw std::logic_error("장기 다이스 삭제의 Native 판본 계약");
    if (!daisDeletion_) {
        o::DaisDeletionHooks hooks;
        hooks.findDais=[this](auto sid) { return o::RawDaisDeletion::FindTypeAt(pool,hash,types,sid,gameAux_->DaisType()); };
        hooks.findAltar=[this](auto sid) { return o::RawDaisDeletion::FindTypeAt(pool,hash,types,sid,gameAux_->AltarType()); };
        hooks.findPriest=[this](auto sid) { return o::RawTempleRitual::FindStrongOwnedPriestIn(pool,hash,types,Neighbors(),Reward(),gameAux_->PriestType(),sid); };
        hooks.stopPriest=[this](auto sid) { o::RawDaisRegular::StopPriestRitualIn(pool,frames,Processes().Host(),sid,[this](auto priest,auto frame,auto flags) { Frame().Set(priest,frame,flags); }); };
        hooks.finishForms=[this](auto sid,auto altar) {
            o::RawDaisDeletion::FinishForms(pool,Processes().Host(),sid,altar,gameAux_->RegularProcessType(),gameAux_->SharedRegularProcessType(),[this](auto dais) {
                const auto finish=daisFinish_;if (!finish) throw std::logic_error("장기 다이스 희생 완료의 실제 Client 장치 미연결");finish(dais);
            }); // 각24가 선택될 때마다 실제 함수를 사본으로 보유한다.
        };
        hooks.refreshOwner=[this](auto owner) { PriestParticipation().Refresh(owner); };hooks.boss=[this] { return authority_->boss; };
        hooks.destroy=[this](auto sid,auto flags) { DestroyAsset(sid,flags); };hooks.pre=[this](auto sid,auto flags) { DamageablePre().PreDestroy(sid,flags); };hooks.post=[this](auto sid,auto flags) { DamageablePost().PostDestroy(sid,flags); };
        hooks.players=[this] { const auto current=playerRecords_.lock();if (!current) throw std::logic_error("장기 다이스 삭제의 현재 Player owner 미연결");return o::PlayerRecordAccess{current,current->Records()}; };
        daisDeletion_=std::make_unique<o::RawDaisDeletion>(pool,hash,types,Neighbors(),gameAux_->AltarType(),gameAux_->RuneType(),std::move(hooks));
    }
    return *daisDeletion_;
}
}
