// 두 Factory VT의 순수 프레임/난수는 장기 현재 상태를 쓰고 큰 효과만 현재 Runtime을 빌린다.
#include "client/RawSurfaceWorld.h"
#include "client/NativeFortSaveVirtual.h"
#include "o/RawFactoryRegular.h"
#include "o/RawFrameAdvance.h"
#include <bit>
#include <stdexcept>
#include <utility>
namespace netstorm::client { // World/Session 생성이나 상태 초기화 없이 같은 본문을 공유한다.
namespace { // 실제 선택된 큰 후단의 현재 수명만 이 호출 동안 보유한다.
template<class Function,class... Args>
decltype(auto) FactoryCall(const std::weak_ptr<const RawSurfaceFactoryBindings>& weak,Function RawSurfaceFactoryBindings::* member,Args&&... args) {
    const auto current=weak.lock();if (!current) throw std::logic_error("Factory의 실제 현재 큰 Runtime 미연결");
    const auto pin=current->pin;const auto hold=pin ? pin():std::shared_ptr<void>{};const auto call=current.get()->*member;
    if (!call) throw std::logic_error("Factory의 선택된 실제 큰 본문 미연결");return call(std::forward<Args>(args)...);
}
// 레벨 조회 뒤에만 현재 TYPE/글자 구간을 읽고 범위를 확인한다.
o::RiftTypeFrames::LetterRun CurrentFactoryRun(const RawSurfaceStorage& storage,o::Sid sid,std::int32_t level) {
    if (level<0 || level>=16) throw std::out_of_range("Factory의 현재 상태 프레임 물리 범위");return storage.frames.at(storage.pool.Slot(sid)[10]).Run(static_cast<std::uint8_t>('A'+level));
}
}
void RawSurfaceStorage::BindFactoryRegularRuntime(const std::shared_ptr<const RawSurfaceFactoryBindings>& runtime) { //현재 큰 후단 대여만 공개하며 난수/프레임/권한은 재설정하지 않는다.
    if (!UsesNativeTransmissionDevices() || !runtime || !factoryRegularRuntime_.expired()) throw std::invalid_argument("Factory 현재 큰 후단 대여 중복/누락");factoryRegularRuntime_=runtime;
}
float RawSurfaceStorage::FactoryRegular(o::Sid sid,std::uint32_t event,std::uint32_t count,float payload) { //원문4549c0의 현재 조회/분배를 동일한 장기 부모에 연결한다.
    if (!UsesNativeTransmissionDevices()) throw std::logic_error("Factory의 실제 Native 장치 미연결");o::FactoryRegularHooks hooks;
    hooks.level=[this](auto parent) { return std::bit_cast<std::int32_t>(FortSaveVirtual().Level(parent)); };
    hooks.firstFrame=[this](auto parent,auto level) { return CurrentFactoryRun(*this,parent,level).first; };hooks.frameCount=[this](auto parent,auto level) { return CurrentFactoryRun(*this,parent,level).count; };
    hooks.random=[this](auto limit) { const auto get=resourceDevices_.random;if (!get) throw std::logic_error("Factory의 실제 같은 게임 난수 미연결");auto& random=get();if (!limit) { static_cast<void>(random.Next(1));throw std::domain_error("Factory 원문 난수0범위 나눗셈"); }return random.Next(limit); };
    hooks.frame=[this](auto parent,auto value,auto flags) { Frame().Set(parent,value,flags); };hooks.advance=[this](auto parent,auto delta,auto flags) { static_cast<void>(o::RawFrameAdvance(pool,frames,Frame()).Advance(parent,delta,flags)); };
    hooks.begin=[this](auto parent,auto value) { FactoryCall(factoryRegularRuntime_,&RawSurfaceFactoryBindings::begin,parent,value); };hooks.finish=[this](auto parent) { return FactoryCall(factoryRegularRuntime_,&RawSurfaceFactoryBindings::finish,parent); };hooks.flare=[this](auto parent,auto number,auto code) { FactoryCall(factoryRegularRuntime_,&RawSurfaceFactoryBindings::flare,parent,number,code); };
    hooks.damageable=[this](auto parent,auto code,auto times,auto value) { return ResourceDamageableRegular(parent,code,times,value); };
    return o::RunFactoryRegular(pool,sid,event,count,payload,hooks);
}
}
// 실제 Factory 가상20도 같은 장기 목록/현재 역할/예약/프레임 보정을 사용한다.
namespace netstorm::client { // Session이 설치/반납돼도 본체와 현재 전역은 초기화하지 않는다.
o::RawFactoryPostPop& RawSurfaceStorage::FactoryPostPop() {
    if (!UsesNativeTransmissionDevices()) throw std::logic_error("Factory 후처리의 실제 Native 장치 미연결");
    if (!resourceFactoryPostPop_) {
        resourceAssetState_.sharedAuthority=authority_.get();resourceAssetState_.sharedResidences=&sidLists.residences;resourceAssetState_.sharedOutposts=&sidLists.outposts;
        o::FactoryPostPopHooks hooks;hooks.reserveRegular=[this] { return AllocateResourceRegular(); };hooks.addRegular=[this](auto parent,auto event,auto payload) { ConstructResourceRegular(parent,event,payload); };
        hooks.setFrame=[this](auto parent,auto frame,auto flags) { Frame().Set(parent,frame,flags); };hooks.virtualCount=[this](auto parent) { return FortSaveVirtual().Level(parent); };hooks.updateState=[this](auto parent) { Display().Update(pool.Slot(parent)); };
        hooks.regionOwnership=[this](auto x,auto y) { const auto current=regionChanged_;if (!current) throw std::logic_error("Factory 후처리의 같은 실제 영역 owner 미연결");current(x,y); };
        hooks.warnDuplicate=[this](auto,auto x,auto y) { TraceGraph(4,"WARNING!  Extra workshop destroyed at ("+std::to_string(x)+","+std::to_string(y)+")\n"); };hooks.destroy=[this](auto parent,auto flags) { DestroyAsset(parent,flags); };
        o::FactoryPostPopTypes roles;roles.currentFirstTimer=&gameAux_->FirstTimerFactoryType();roles.currentSecondTimer=&gameAux_->SecondTimerFactoryType();roles.currentFixedFrame=&gameAux_->OutpostType();
        resourceFactoryPostPop_=std::make_unique<o::RawFactoryPostPop>(pool,hash,types,frames,PostPop(),ResourceCarrierPostPop(),resourceAssetState_,std::move(hooks),roles);
    }
    return *resourceFactoryPostPop_;
}
}
