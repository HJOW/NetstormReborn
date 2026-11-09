// 의식계 정기는 장기 현재 물리에서 실행하고 선택된 큰 후단만 실제 Runtime을 빌린다.
#include "client/RawSurfaceWorld.h"
#include "client/ClientPlayerRecords.h"
#include "client/NativeFortSaveVirtual.h"
#include "o/RawRunePostPop.h"
#include "o/RawTempleRitual.h"
#include "o/RawFrameAdvance.h"
#include <bit>
#include <cmath>
#include <new>
#include <stdexcept>
#include <utility>

namespace netstorm::client { // Session 생성/reset 없이 현재 Pool/TYPE·권한·난수·Host를 사용한다.
namespace { // 큰 효과는 실제 owner pin과 함수 사본을 선택 호출 동안 유지한다.
template<class Function,class... Args>
decltype(auto) RitualCall(const std::weak_ptr<const RawSurfaceRitualBindings>& weak,Function RawSurfaceRitualBindings::* member,Args&&... args) {
    const auto current=weak.lock();if (!current) throw std::logic_error("의식의 실제 현재 큰 Runtime 미연결");const auto pin=current->pin;if (!pin) throw std::logic_error("의식의 실제 전체 Runtime pin 미연결");const auto hold=pin();if (!hold) throw std::logic_error("의식의 실제 전체 Runtime owner 반납");const auto call=current.get()->*member;if (!call) throw std::logic_error("의식의 선택된 실제 큰 본문 미연결");return call(std::forward<Args>(args)...);
}
std::uint32_t RitualDword(std::span<const std::uint8_t> raw,std::size_t at) { // 비정렬 현재 DWORD만 낮은 바이트부터 읽는다.
    std::uint32_t result{};for (std::size_t byte=0;byte<4;++byte) result|=std::uint32_t(raw[at+byte])<<(byte*8);return result; // 반복은 인접 필드를 읽지 않는다.
}
}
void RawSurfaceStorage::BindRitualRuntime(const std::shared_ptr<const RawSurfaceRitualBindings>& runtime) { // 실제 큰 후단 대여만 공개하고 현재 상태/역할은 쓰지 않는다.
    if (!UsesNativeTransmissionDevices() || !runtime || !ritualRuntime_.expired()) throw std::invalid_argument("의식 현재 큰 후단 대여 중복/누락");ritualRuntime_=runtime;
}
o::AltarSacrificeState& RawSurfaceStorage::AltarState() { return altarState_; } // 같은00532698/69c 주소를 Session과 후속 처리기에 빌린다.
o::RawForcefieldRegular& RawSurfaceStorage::ForcefieldRegular() { // 같은 현재 사제 역할과 진단을 실제 선택 지점에서 조회한다.
    if (!UsesNativeTransmissionDevices()) throw std::logic_error("보호막의 실제 Native 장치 미연결");
    if (!forcefieldRegular_) {
        forcefieldState_.sharedPriestType=&gameAux_->PriestType();forcefieldState_.debug=Checking();o::ForcefieldRegularHooks hooks;
        hooks.advance=[this](auto parent,auto delta,auto flags) { static_cast<void>(o::RawFrameAdvance(pool,frames,Frame()).Advance(parent,delta,flags)); };hooks.destroy=[this](auto parent,auto flags) { DestroyAsset(parent,flags); };
        hooks.assertion=[this](auto expression,auto file,auto line) { AssertDisplay(expression,file,line); };
        hooks.findTypeAt=[this](auto x,auto y,auto type) { return o::RawDaisDeletion::FindTypeAt(pool,hash,types,x,y,type); };
        forcefieldRegular_=std::make_unique<o::RawForcefieldRegular>(pool,hash,types,forcefieldState_,std::move(hooks));
    }
    forcefieldState_.debug=Checking();return *forcefieldRegular_;
}
float RawSurfaceStorage::RitualRegular(o::Sid sid,std::uint32_t event,std::uint32_t count,float payload) { // 실제 네 가상 표의 서로 다른 Regular를 선택한다.
    if (!UsesNativeTransmissionDevices()) throw std::logic_error("의식 정기의 실제 Native 장치 미연결");const auto table=RitualDword(pool.Slot(sid),0);
    if (table==0x507770U) return ForcefieldRegular().Handle(sid,event,count,payload);
    if (table==0x5075d8U) return o::RunRuneRegular(pool,sid,event,payload,{
        [this](auto parent) { return o::RawRunePostPop::FindOwnedDaisIn(pool,hash,types,gameAux_->DaisType(),parent); },
        [this](auto parent) { RitualCall(ritualRuntime_,&RawSurfaceRitualBindings::progress,parent); },[this](auto parent,auto flags) { DestroyAsset(parent,flags); }});
    if (table==0x507950U) {
        o::DaisHandleHooks hooks;hooks.stage=[this](auto parent,auto frame) { return o::RawTempleRitual::StageFromCount(pool.Edition(),frame,FortSaveVirtual().Level(parent)); };hooks.frame=[this](auto parent,auto value,auto flags) { Frame().Set(parent,value,flags); };
        hooks.sacrificeAdvance=[this](auto parent) { return RitualCall(ritualRuntime_,&RawSurfaceRitualBindings::sacrificeAdvance,parent); };hooks.ritual=[this](auto parent) { RitualCall(ritualRuntime_,&RawSurfaceRitualBindings::ritual,parent); };hooks.damageable=[this](auto parent,auto code,auto number,auto value) { return ResourceDamageableRegular(parent,code,number,value); };
        return o::RunDaisHandle(pool,sid,event,count,payload,hooks);
    }
    if (table!=0x5076b8U) throw std::logic_error("다른 의식 정기 가상 표");o::AltarRegularHooks hooks;
    hooks.dais=[this](auto parent) { return o::RawDaisDeletion::FindTypeAt(pool,hash,types,parent,gameAux_->DaisType()); };
    hooks.priest=[this](auto parent) { const auto type=gameAux_->PriestType();if (!type) throw std::logic_error("제단 specialized kind0은 일반 Carrier wildcard와 다르다");return CarrierPre().FindContained(parent,0,type,0,false); };hooks.boss=[this] { return authority_->boss; };
    hooks.alliance=[this](auto original,auto owner) { const auto records=playerRecords_.lock();if (!records) throw std::logic_error("제단 정기의 현재 Player owner 미연결");return std::bit_cast<std::int32_t>(records->Alliances().at(original+owner*9)); };
    hooks.release=[this](auto parent) { RitualCall(ritualRuntime_,&RawSurfaceRitualBindings::release,parent); };hooks.removeAnimation=[this](auto parent) { RitualCall(ritualRuntime_,&RawSurfaceRitualBindings::removeAnimation,parent); };hooks.escapeFrame=[this](auto parent,auto number) { RitualCall(ritualRuntime_,&RawSurfaceRitualBindings::escapeFrame,parent,number); };
    hooks.advance=[this](auto parent,auto delta,auto flags) { return o::RawFrameAdvance(pool,frames,Frame()).Advance(parent,delta,flags); };hooks.frame=[this](auto parent,auto value,auto flags) { Frame().Set(parent,value,flags); };hooks.destroy=[this](auto parent,auto flags) { DestroyAsset(parent,flags); };
    hooks.reserveShared=[this] { if (ritualReservation_) throw std::logic_error("제단 SharedRegular 실제 확보 중첩");ritualReservation_.reset(::operator new(sizeof(o::RegularProcess),std::nothrow));return static_cast<bool>(ritualReservation_); };
    hooks.addShared=[this](auto parent,auto code,auto value) { if (!ritualReservation_) throw std::logic_error("제단 SharedRegular 실제 확보 누락");auto* memory=ritualReservation_.release();o::RegularProcess* process{};try { process=new(memory) o::RegularProcess(code,value); }catch (...) { ::operator delete(memory);throw; }Processes().Host().AttachRegular(std::unique_ptr<o::RegularProcess>(process),parent,true); };
    hooks.randomInt=[this](auto low,auto high) { const auto get=resourceDevices_.random;if (!get) throw std::logic_error("제단의 같은 현재 게임 난수 미연결");if (high<=low) throw std::invalid_argument("제단 실제 정수 난수 범위");return static_cast<std::int32_t>(get().Next(static_cast<std::uint32_t>(high-low)))+low; };
    hooks.randomFloat=[this](auto low,auto high) { const auto get=resourceDevices_.random;if (!get) throw std::logic_error("제단의 같은 현재 게임 난수 미연결");return static_cast<float>(static_cast<double>(get().Next(65536))*(std::abs(static_cast<double>(high)-low)/65536.0)+low); };
    hooks.sound=[this](auto x,auto y,auto name,auto flags) { const auto play=flareDevices_.sound;if (!play) throw std::logic_error("제단의 현재 위치 음향 미연결");play(x,y,name,flags); };hooks.damageable=[this](auto parent,auto code,auto number,auto value) { return ResourceDamageableRegular(parent,code,number,value); };
    return o::RunAltarRegular(pool,frames,sid,event,count,payload,altarState_,hooks);
}
}
