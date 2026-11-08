// 실제 피해가능 객체 전/후처리와 간헐천 고유 수명을 같은 장기 물리/전역에 연결한다.
#include "client/RawSurfaceWorld.h"
#include "o/SquidProcess.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::client { // 큰 선택 효과만 실제 현재 Runtime을 요구하며 새 World/상태 사본을 만들지 않는다.
// 첫 실제 모듈 전 actual finder/허용 genus owner와 현재 장치만 연결한다.
void RawSurfaceStorage::BindDamageableDevices(RawSurfaceDamageableDevices devices) {
    if (!UsesNativeTransmissionDevices() || !deletionDevices_.unitLost || borrowed_ || damageableDevices_.finder || !devices.finder || !devices.allowedGenus || !devices.collapse || !devices.explosion || !devices.sound || !devices.spawnPriest || !devices.dropDude || !devices.dropIsland) throw std::invalid_argument("장기 Damageable의 실제 Native 최초 장치 계약");
    damageableDevices_=std::move(devices);damageablePreState_.sharedPriestType=&gameAux_->PriestType();damageableReleaseState_.sharedAllowedGenus=damageableDevices_.allowedGenus.get();
    damageablePostState_.sharedBattle=&ownerMode.battle;damageablePostState_.sharedBoss=&authority_->boss;damageablePostState_.sharedSilentSurfaceType=&gameAux_->RainBlockerType();damageablePostState_.sharedDudePopulation=&sidLists.dudes;damageablePostState_.sharedDamageEvent=&gameAux_->DamageRegularEvent();
}
// 현재 Native owner를 빌리며 값을 복사하거나 기본값으로 되돌리지 않는다.
o::DamageablePreDestroyState& RawSurfaceStorage::DamageablePreGlobals() { if (!damageableDevices_.finder) throw std::logic_error("장기 Damageable 실제 finder 미연결");return damageablePreState_; }
o::DamageableReleaseState& RawSurfaceStorage::DamageableReleaseGlobals() { if (!damageableDevices_.finder) throw std::logic_error("장기 Damageable 실제 해방 owner 미연결");return damageableReleaseState_; } // 같은 허용 마스크 주소다.
o::DamageablePostDestroyState& RawSurfaceStorage::DamageablePostGlobals() { if (!damageableDevices_.finder) throw std::logic_error("장기 Damageable 실제 후처리 owner 미연결");return damageablePostState_; } // 같은 실제 population/mode/event다.
// 현재 공간/TYPE/내용물과 실제 Factory/Owner/Pop의 해방 몸체다.
o::RawDamageableRelease& RawSurfaceStorage::DamageableRelease() {
    static_cast<void>(DamageableReleaseGlobals()); // finder 역참조보다 먼저 실제 owner 결합을 확인한다.
    if (!damageableRelease_) {
        o::DamageableReleaseHooks hooks;hooks.spawnPriest=damageableDevices_.spawnPriest;hooks.create=[this](auto type,auto flags) { return CreateAsset(type,flags); };hooks.setOwner=[this](auto sid,auto owner) { SetAssetOwner(sid,owner); };hooks.pop=[this](auto sid,auto x,auto y,auto flags) { PopAsset(sid,x,y,flags); };hooks.notifySurface=[this](auto sid) { Building().NotifySurface(sid); };
        damageableRelease_=std::make_unique<o::RawDamageableRelease>(pool,types,spots,*damageableDevices_.finder,DamageableReleaseGlobals(),std::move(hooks));
    }
    return *damageableRelease_;
}
// 실제 Native 효과 순서와 각 소비 시점의 현재 권한/Bridge TYPE를 사용한다.
o::RawDamageablePreDestroy& RawSurfaceStorage::DamageablePre() {
    static_cast<void>(DamageablePreGlobals()); // 구성되지 않은 장치를 물리 포인터로 대신하지 않는다.
    if (!damageablePre_) {
        o::DamageablePreDestroyHooks hooks;hooks.collapse=damageableDevices_.collapse;hooks.explosion=damageableDevices_.explosion;hooks.sound=damageableDevices_.sound;
        hooks.releaseContained=[this](auto sid) { DamageableRelease().ReleaseNative(sid,ownerMode.battle,links.bridgeType); };hooks.validateBase=[this](auto sid,auto flags) { CommonDeletion().ValidatePre(sid,flags); };hooks.base=[this](auto sid,auto flags) { CommonDeletion().PreDestroy(sid,flags); };
        auto body=std::make_unique<o::RawDamageablePreDestroy>(pool,spots,*damageableDevices_.finder,DamageablePreGlobals(),std::move(hooks));body->EnableNativeBody();damageablePre_=std::move(body);
    }
    return *damageablePre_;
}
// 실제 같은 HP/목록/공간/Host로 드롭·피해 예약·다리 약화·공통 Post를 연결한다.
o::RawDamageablePostDestroy& RawSurfaceStorage::DamageablePost() {
    static_cast<void>(DamageablePostGlobals()); // 실제 같은 장기 전역이 있어야 한다.
    if (!damageablePost_) {
        o::DamageablePostDestroyHooks hooks;hooks.dropDude=damageableDevices_.dropDude;hooks.dropIsland=damageableDevices_.dropIsland;
        hooks.regular=[this](auto sid,auto event,auto payload,auto count) { auto* process=Processes().Host().AddRegular(sid,event,payload);if (!process) throw std::bad_alloc();process->SetInitialCount(count); };
        hooks.hasBridgeFall=[this](auto sid) { return o::HasScheduledBridgeFall(Processes().Host(),sid); };hooks.notifySurface=[this](auto sid) { Building().NotifySurface(sid); };hooks.displayUpdate=[this](auto sid) { Display().Update(pool.Slot(sid)); };
        hooks.destroy=[this](auto sid,auto flags) { const auto destroy=resourceDevices_.destroy;if (!destroy) throw std::logic_error("장기 Damageable의 선택된 실제 가상 삭제 미연결");destroy(sid,flags); };hooks.base=[this](auto sid,auto flags) { CommonDeletion().PostDestroy(sid,flags); };
        damageablePost_=std::make_unique<o::RawDamageablePostDestroy>(pool,hash,types,frames,Reward(),Neighbors(),DamageablePostGlobals(),std::move(hooks));
    }
    return *damageablePost_;
}
// Native 가상78의 같은 현재 TYPE+128과 실제 지면 목록/표시/AI 부모를 사용한다.
o::RawGeyserLifetime& RawSurfaceStorage::GeyserLifetime() {
    static_cast<void>(DamageablePreGlobals()); // 실제 editor/finder owner 결합 뒤에만 구성한다.
    if (!geyserLifetime_) {
        o::GeyserLifetimeHooks hooks;hooks.defaultFrame=[this](auto sid) { const auto number=pool.Slot(sid)[10];if (number>=types.size()) throw std::out_of_range("Native 간헐천 현재 기본 프레임 TYPE 범위");return std::bit_cast<std::uint32_t>(types[number].defaultFrame); };hooks.update=[this](auto sid) { Display().Update(pool.Slot(sid)); };
        hooks.registerAi=[this](auto sid,auto operation) { const auto registration=resourceDevices_.geyserRegistration;if (!registration) throw std::logic_error("장기 간헐천의 실제 AI 등록 부모 미연결");registration(sid,operation); };
        hooks.damageablePre=[this](auto sid,auto flags) { DamageablePre().PreDestroy(sid,flags); };hooks.damageablePost=[this](auto sid,auto flags) { DamageablePost().PostDestroy(sid,flags); };hooks.dropIsland=damageableDevices_.dropIsland;
        auto body=std::make_unique<o::RawGeyserLifetime>(pool,sidLists.geysers,*rewardDevices_.editor,std::move(hooks));body->EnableNativeBody();geyserLifetime_=std::move(body);
    }
    return *geyserLifetime_;
}
}
