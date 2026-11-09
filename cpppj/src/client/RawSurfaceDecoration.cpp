// 장식/가장자리 팜의 현재HP·권한과 선택된 가상98를 같은 장기 물리에 연결한다.
#include "client/RawSurfaceWorld.h"
#include "o/RawDecorationRegular.h"
#include <bit>
#include <stdexcept>
namespace netstorm::client { // Session 없는 같은 실제 Host에서도 순수 분기와 최종 삭제를 사용한다.
void RawSurfaceStorage::BindDamageableRuntime(const std::shared_ptr<const RawSurfaceDamageableBindings>& runtime) { //현재 큰 가상98만 빌리며 전역/TYPE/목록을 초기화하지 않는다.
    if (!UsesNativeTransmissionDevices() || !runtime || !damageableRuntime_.expired()) throw std::invalid_argument("현재 가상98 후단 대여 중복/누락");damageableRuntime_=runtime;
}
float RawSurfaceStorage::DecorationRegular(o::Sid sid,std::uint32_t event,std::uint32_t count,float payload) { //현재 원문462420의 직접 호출/Host 호출을 같은 본문으로 보낸다.
    if (!UsesNativeTransmissionDevices()) throw std::logic_error("장식 Regular의 실제 Native 장치 미연결");
    o::DecorationRegularHooks hooks;
    hooks.maxHitPoints=[this](auto parent) { return Reward().MaxHitPoints(parent); };
    hooks.hitPoints=[this](auto parent,auto amount,auto owner,auto attacker) {
        const auto current=damageableRuntime_.lock();if (!current) throw std::logic_error("장식의 실제 현재 가상98 Runtime 미연결");
        const auto pin=current->pin;const auto hold=pin ? pin():std::shared_ptr<void>{};const auto call=current->hitPoints;
        if (!call) throw std::logic_error("장식의 선택된 실제 현재 가상98 본문 미연결");static_cast<void>(call(parent,std::bit_cast<std::uint32_t>(amount),std::bit_cast<std::int32_t>(owner),attacker));
    };
    hooks.destroy=[this](auto parent,auto flags) { DestroyAsset(parent,flags); };
    hooks.damageable=[this](auto parent,auto code,auto times,auto value) { return ResourceDamageableRegular(parent,code,times,value); };
    return o::RunDecorationRegular(pool,authority_->boss,sid,event,count,payload,hooks);
}
}
