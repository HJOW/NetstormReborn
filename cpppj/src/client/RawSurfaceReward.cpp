// Client의 실제 HP·보상·분산 SP를 World/Session 수명과 독립된 같은 장기 물리에 연결한다.
#include "client/RawSurfaceWorld.h"
#include "client/NativeFortSaveVirtual.h"
#include "client/GameAssets.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::client { // 장기 물리와 실제 현재 장치만 공유하며 Session을 생성하지 않는다.
// 첫 Native 사용 전 동일 owner만 결합한다. 준비 입력을 실행하거나 현재 전역을 초기화하지 않는다.
void RawSurfaceStorage::BindRewardDevices(RawSurfaceRewardDevices devices) {
    if (pool.Edition()!=o::OriginalEdition::Patch1078 || !UsesNativeTransmissionDevices() || !devices.sp || !devices.editor || !devices.refreshSp || !devices.creditWallet || rewardDevices_.sp || reward_ || borrowed_) throw std::invalid_argument("장기 HP/보상의 실제 Native 최초 장치 계약");
    rewardDevices_=std::move(devices);
    rewardState_.sharedFixedSpA=&ownerMode.fort;rewardState_.sharedFixedSpB=rewardDevices_.editor.get();rewardState_.sharedHalfHealthMode=&ownerMode.battle;
    rewardState_.sharedPercent=&gameAux_->RefundPercent();rewardState_.sharedGeyserPool=&gameAux_->ResourcePool();rewardState_.sharedPriestQuarterHp=&gameAux_->PriestQuarterHp();
    rewardState_.sharedPriestType=&gameAux_->PriestType();rewardState_.sharedAltarType=&gameAux_->AltarType();rewardState_.sharedDaisType=&gameAux_->DaisType();
    rewardState_.sharedSilentType=&gameAux_->RainBlockerType();rewardState_.sharedAliasType=&gameAux_->GrowingRainBlockerType();rewardState_.sharedNuggetType=&gameAux_->NuggetType();rewardState_.debugAsserts=Checking();
}
// 실제 Native 장치 누락은 선택 시점 오류로 드러내고 임시 SP/전역을 만들지 않는다.
o::SquidRewardState& RawSurfaceStorage::RewardState() {
    if (!rewardDevices_.sp) throw std::logic_error("장기 HP/보상의 실제 SP owner 미연결");return rewardState_;
}
// 같은 현재 Pool/TYPE/장부와 최초 로딩 정수 비용을 기존 보상 본체에 한 번 연결한다.
o::SquidReward& RawSurfaceStorage::Reward() {
    if (!reward_) {
        if (!Source()) throw std::logic_error("장기 HP/보상의 실제 최초 로딩 자산 미연결");
        reward_=std::make_unique<o::SquidReward>(pool,types,postPopState,RewardState(),rewardDevices_.sp.get(),o::SquidRewardHooks{[this](o::Sid sid) { return std::bit_cast<std::int32_t>(FortSaveVirtual().Level(sid)); },rewardDevices_.refreshSp});
        reward_->BindCurrentTypes(types);reward_->BindLoadedCosts(Source()->TypeTable().Types());
        reward_->BindHitPointDiagnostics([this](auto expression,auto file,auto line) { AssertDisplay(expression,file,line); });reward_->BindPatchWallet(rewardDevices_.creditWallet);
    }
    return *reward_;
}
}
