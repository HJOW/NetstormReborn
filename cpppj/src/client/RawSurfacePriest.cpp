// 사제 정기 분기와 현재 상태를 Client 장기로 보관하고 큰 후단만 실제 Runtime을 pin한다.
#include "client/RawSurfaceWorld.h"
#include "client/RawPriestWorldBindings.h"
#include "o/RawPriestRegular.h"
#include <stdexcept>
namespace netstorm::client { // 메뉴·NoWorld·Session이 같은 현재 사제 상태와 부모를 사용한다.
RawPriestWorldState& RawSurfaceStorage::PriestState() { // 실제 장기 TYPE 주소를 빌리고 재대여 시 현재 회복/목록/모드를 초기화하지 않는다.
    if (!UsesNativeTransmissionDevices()) throw std::logic_error("사제 현재 상태의 Native 장치 미연결");if (!priestState_) priestState_=std::make_shared<RawPriestWorldState>(&gameAux_->PriestType(),gameAux_.get());return *priestState_;
}
void RawSurfaceStorage::BindPriestRuntime(const std::shared_ptr<const RawSurfacePriestBindings>& runtime) { // 현재 큰 본문 대여만 공개하며 장기 상태는 쓰지 않는다.
    if (!UsesNativeTransmissionDevices() || !runtime || !priestRuntime_.expired()) throw std::invalid_argument("사제 실제 현재 후단 대여 중복/누락");priestRuntime_=runtime;
}
float RawSurfaceStorage::PriestRegular(o::Sid sid,std::uint32_t event,std::uint32_t count,float payload) { // VT로 선택된494580의 분기와 직접 Carrier 부모를 같은 장기 물리에서 실행한다.
    if (!UsesNativeTransmissionDevices()) throw std::logic_error("사제 정기의 Native 장치 미연결");o::PriestRegularHooks hooks;
    hooks.destroy=[this](auto parent,auto flags) { DestroyAsset(parent,flags); };
    hooks.carrier=[this](auto parent,auto code,auto number,auto value) { return CarrierRegular().Handle(parent,code,number,value); };
    hooks.unique=[this](auto parent,auto code,auto number,auto value) { // 선택 후 실제 전체 owner와 함수 사본을 정상/예외 복귀까지 유지한다.
        const auto current=priestRuntime_.lock();if (!current) throw std::logic_error("사제 고유 정기의 실제 Runtime 미연결");const auto pin=current->pin;if (!pin) throw std::logic_error("사제 고유 정기의 실제 pin 미연결");const auto hold=pin();if (!hold) throw std::logic_error("사제 고유 정기의 실제 owner 반납");const auto call=current->unique;if (!call) throw std::logic_error("사제 고유 정기의 실제 본문 미연결");return call(parent,code,number,value);
    };
    return o::RunPriestRegular(sid,event,count,payload,hooks);
}
}
