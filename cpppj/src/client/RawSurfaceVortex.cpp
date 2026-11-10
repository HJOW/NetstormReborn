// 신전 네 VT의 정기/현재 모드는 장기 물리에서 실행하고 큰 이탈/Descend만 실제 Runtime을 빌린다.
#include "client/RawSurfaceWorld.h"
#include "o/RawAssetRegular.h"
#include "o/RawFrameAdvance.h"
#include <stdexcept>
#include <utility>

namespace netstorm::client { // Session이 반납돼도 같은 현재 신전 상태와 물리를 보존한다.
namespace { // 큰 효과는 선택 호출의 정상/예외 반환까지 실제 전체 owner와 함수 사본을 유지한다.
// 현재 큰 본문이 선택된 순간에만 대여를 조회하고 실제 전체 수명을 보장한다.
template<class Function,class... Args>
decltype(auto) VortexCall(const std::weak_ptr<const RawSurfaceVortexBindings>& weak,Function RawSurfaceVortexBindings::* member,Args&&... args) {
    const auto current=weak.lock();if (!current) throw std::logic_error("신전의 실제 현재 큰 Runtime 미연결");const auto pin=current->pin;if (!pin) throw std::logic_error("신전의 실제 전체 pin 미연결");const auto hold=pin();if (!hold) throw std::logic_error("신전의 실제 전체 owner 반납");const auto call=current.get()->*member;if (!call) throw std::logic_error("신전의 선택된 큰 후단 미연결");return call(std::forward<Args>(args)...);
}
}
void RawSurfaceStorage::BindVortexDevices(RawSurfaceVortexDevices devices) { // 실제 현재 Construction만 연결하고 게임 상태/난수를 초기화하지 않는다.
    if (!UsesNativeTransmissionDevices() || !devices.ascend || !devices.foreignIslands || !devices.islandNumber) throw std::invalid_argument("장기 신전의 실제 Construction 장치 미연결");vortexDevices_=std::move(devices);
}
void RawSurfaceStorage::BindVortexRuntime(const std::shared_ptr<const RawSurfaceVortexBindings>& runtime) { // 현재 World의 큰 후단만 약하게 빌리며 기존 상태는 덮어쓰지 않는다.
    if (!UsesNativeTransmissionDevices() || !runtime || !vortexRuntime_.expired()) throw std::invalid_argument("신전 실제 큰 Runtime 대여 중복/누락");vortexRuntime_=runtime;
}
o::AssetRegularState& RawSurfaceStorage::VortexState() { // 원문 현재 모드/권한/진단은 같은 장기 owner의 주소를 직접 읽는다.
    if (!UsesNativeTransmissionDevices()) throw std::logic_error("신전의 실제 Native 장치 미연결");if (!vortexState_) vortexState_=std::make_unique<o::AssetRegularState>();vortexState_->sharedFortMode=&ownerMode.fort;vortexState_->sharedAuthority=authority_.get();vortexState_->checkingForms=Checking();return *vortexState_;
}
float RawSurfaceStorage::VortexRegular(o::Sid sid,std::uint32_t event,std::uint32_t count,float payload) { // 공통4dc220과 실제 jmp thunk는 같은 한 본문/현재값을 사용한다.
    auto& state=VortexState();o::AssetRegularHooks hooks;o::VortexRegularDevices devices; // 실제 장치는 해당 분기에서만 조회한다.
    devices.frame=[this](auto parent,auto frame,auto flags) { Frame().Set(parent,frame,flags); };devices.advance=[this](auto parent,auto delta,auto flags) { static_cast<void>(o::RawFrameAdvance(pool,frames,Frame()).Advance(parent,delta,flags)); };
    devices.random=[this](auto limit) { const auto get=resourceDevices_.random;if (!get) throw std::logic_error("신전의 같은 실제 게임 난수 미연결");return get().Next(limit); };
    hooks.ascendTick=[this](auto parent,auto times) { const auto call=vortexDevices_.ascend;if (!call) throw std::logic_error("신전의 같은 장기 상승 본문 미연결");return call(parent,times); };
    hooks.countForeignOwnedIslands=[this](auto owner) { const auto call=vortexDevices_.foreignIslands;if (!call) throw std::logic_error("신전의 실제 현재 섬 순회 미연결");return call(owner); };hooks.islandNumber=[this](auto x,auto y) { const auto call=vortexDevices_.islandNumber;if (!call) throw std::logic_error("신전의 실제 현재 지역 getter 미연결");return call(x,y); };
    hooks.state=[this](auto parent,auto word) { auto raw=pool.AllocatedBytes(parent);raw[12]=static_cast<std::uint8_t>(word);raw[13]=static_cast<std::uint8_t>(word>>8);if (pool.Slot(parent)[10]==gameAux_->StateNotifyType()) Building().NotifySurface(parent); }; // 원문 WORD 저장 뒤 현재 지정 TYPE의 실제 표면 통지를 수행한다.
    hooks.suppressLeaveRequest=[this] { return links.mission; };hooks.playerLeavingNotice=[this](auto owner) { VortexCall(vortexRuntime_,&RawSurfaceVortexBindings::leavingNotice,owner); };hooks.requestLeave=[this](auto parent,auto island) { VortexCall(vortexRuntime_,&RawSurfaceVortexBindings::leave,parent,island); };hooks.descend=[this](auto x,auto y) { VortexCall(vortexRuntime_,&RawSurfaceVortexBindings::descend,x,y); };
    return o::RunVortexRegular(pool,frames,state,hooks,devices,[this](auto parent,auto code,auto times,auto value) { return ResourceDamageableRegular(parent,code,times,value); },sid,event,count,payload);
}
}
