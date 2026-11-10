// 아크/애시드의 전체 정기·검색은 장기 물리에서 실행하고 큰 빔/피해/분해만 실제 Runtime을 빌린다.
#include "client/RawSurfaceWorld.h"
#include "client/ClientPlayerRecords.h"
#include "o/RawDaisDeletion.h"
#include <new>
#include <stdexcept>
#include <utility>

namespace netstorm::client { // 메뉴·NoWorld·Session이 같은 현재 TYPE/역할/동맹/Host를 사용한다.
namespace { // 선택된 효과는 실제 전체 owner와 함수 사본을 복귀까지 유지한다.
template<class Function,class... Args>
decltype(auto) AttackCall(const std::weak_ptr<const RawSurfaceAttackBindings>& weak,Function RawSurfaceAttackBindings::* member,Args&&... args) { // 큰 효과가 없는 가지는 Runtime을 요구하지 않는다.
    const auto current=weak.lock();if (!current) throw std::logic_error("아크/애시드의 실제 현재 Runtime 미연결");const auto pin=current->pin;if (!pin) throw std::logic_error("아크/애시드의 실제 전체 pin 미연결");const auto hold=pin();if (!hold) throw std::logic_error("아크/애시드의 실제 전체 owner 반납");const auto call=current.get()->*member;if (!call) throw std::logic_error("아크/애시드의 선택된 큰 본문 미연결");return call(std::forward<Args>(args)...);
}
}
void RawSurfaceStorage::BindAttackRuntime(const std::shared_ptr<const RawSurfaceAttackBindings>& runtime) { // 현재 큰 효과만 대여하며 순수 장기 상태/본문을 초기화하지 않는다.
    if (!UsesNativeTransmissionDevices() || !runtime || !attackRuntime_.expired()) throw std::invalid_argument("아크/애시드 실제 후단 대여 중복/누락");attackRuntime_=runtime;
}
o::AcidFenceTypes& RawSurfaceStorage::AcidTypes() { // 여섯 실제 현재 역할 주소만 연결하고 기존 값을 쓰지 않는다.
    if (!UsesNativeTransmissionDevices()) throw std::logic_error("애시드 현재 역할의 Native 장치 미연결");acidTypes_.sharedRainFlyer=&gameAux_->RainFlyerType();acidTypes_.sharedSunFlyer=&gameAux_->SunFlyerType();acidTypes_.sharedWindFlyer=&gameAux_->WindFlyerType();acidTypes_.sharedRainBalloon=&gameAux_->RainBalloonType();acidTypes_.sharedWindBalloon=&gameAux_->WindBalloonType();acidTypes_.sharedSunBalloon=&gameAux_->SunBalloonType();return acidTypes_;
}
o::AcidZapState& RawSurfaceStorage::AcidState() { // 수신 제외도 같은 현재 제단/성장형 TYPE를 재조회한다.
    if (!UsesNativeTransmissionDevices()) throw std::logic_error("애시드 수신 역할의 Native 장치 미연결");acidState_.sharedAltar=&gameAux_->AltarType();acidState_.sharedGrowingIce=&gameAux_->GrowingRainBlockerType();return acidState_;
}
o::FormPacketHeader& RawSurfaceStorage::AcidHeader() { return acidHeader_; } // 현재61h 헤더는 Session 생성/반납으로 재초기화하지 않는다.
bool RawSurfaceStorage::ReserveAttackRegular() { // 원문 new28의 성공/실패를 실제 호스트 객체 크기의 확보로 보존한다.
    if (!UsesNativeTransmissionDevices()) throw std::logic_error("공격 정기 예약의 Native 장치 미연결");if (attackReservation_) throw std::logic_error("공격 정기 실제 확보 중첩");attackReservation_.reset(::operator new(sizeof(o::RegularProcess),std::nothrow));return static_cast<bool>(attackReservation_);
}
void RawSurfaceStorage::ConstructAttackRegular(o::Sid sid,std::uint32_t event,float payload) { // 확보했던 같은 주소의 생성/현재 역할/시각은 실제 Host에 한 번 맡긴다.
    if (!attackReservation_) throw std::logic_error("공격 정기 생성 전 실제 확보 누락");auto* memory=attackReservation_.release();o::RegularProcess* process{};try { process=new(memory) o::RegularProcess(event,payload); }catch (...) { ::operator delete(memory);throw; }Processes().Host().AttachRegular(std::unique_ptr<o::RegularProcess>(process),sid,false);
}
o::RawArcSpire& RawSurfaceStorage::ArcSpire() { // 현재 TYPE+44/+48과 같은 고정81DWORD 동맹/편집/지도·부모를 사용한다.
    if (!UsesNativeTransmissionDevices() || !rewardDevices_.editor) throw std::logic_error("아크의 실제 현재 편집 장치 미연결");
    if (!arcSpire_) {
        if (!attackAlliancesOwner_) attackAlliancesOwner_=playerRecords_.lock();if (!attackAlliancesOwner_) throw std::logic_error("아크의 실제 고정 동맹 owner 미연결");o::ArcSpireHooks hooks;
        hooks.range=[this](auto parent) { return types.at(pool.Slot(parent)[10]).range; };hooks.damageRate=[this](auto parent) { return types.at(pool.Slot(parent)[10]).damagePerSecond; };
        hooks.find=[this](auto x,auto y,auto type) { return o::RawDaisDeletion::FindTypeAt(pool,hash,types,x,y,type); };
        hooks.iterate=[this](auto area,const auto& consume) { o::RawSquidFinder finder(pool,hash,types);for (auto current=finder.Begin(area,1);current.value;current=finder.Next()) consume(current); }; // 현재 일반 Hash 체인을 원문 순서로 읽는다.
        hooks.beam=[this](auto source,auto target) { AttackCall(attackRuntime_,&RawSurfaceAttackBindings::beam,source,target); };hooks.damage=[this](auto target,auto amount,auto owner,auto source) { AttackCall(attackRuntime_,&RawSurfaceAttackBindings::damage,target,amount,owner,source); };
        hooks.setFrame=[this](auto parent,auto frame,auto flags) { Frame().Set(parent,frame,flags); };hooks.damageable=[this](auto parent,auto event,auto count,auto payload) { return ResourceDamageableRegular(parent,event,count,payload); };
        arcSpire_=std::make_unique<o::RawArcSpire>(pool,types,frames,spots,attackAlliancesOwner_->Alliances(),*rewardDevices_.editor,std::move(hooks));
    }return *arcSpire_;
}
o::RawAcidZap& RawSurfaceStorage::AcidZap() { // 현재 헤더/제외 역할/소리/가상삭제를 같은 수신 본문에 결합한다.
    if (!UsesNativeTransmissionDevices()) throw std::logic_error("애시드 수신의 Native 장치 미연결");
    if (!acidZap_) {
        o::AcidZapHooks hooks;hooks.sound=[this](auto x,auto y,auto name,auto loop) { if (loop) throw std::logic_error("애시드 실제 단발 위치 소리의 loop 불일치");const auto sound=flareDevices_.sound;if (!sound) throw std::logic_error("애시드 실제 위치 소리 미연결");sound(x,y,name,0); };
        hooks.dissolve=[this](auto target,auto first,auto second,auto orientation) { AttackCall(attackRuntime_,&RawSurfaceAttackBindings::dissolve,target,first,second,orientation); };hooks.destroy=[this](auto target,auto flags) { DestroyAsset(target,flags); };
        acidZap_=std::make_unique<o::RawAcidZap>(pool,types,AcidState(),acidHeader_,std::move(hooks));
    }return *acidZap_;
}
o::RawAcidFence& RawSurfaceStorage::AcidFence() { // 같은 현재 친군사격/권한/역할과 실제 검색·예약·송신·수신을 공유한다.
    if (!UsesNativeTransmissionDevices() || !rewardDevices_.editor || !playerRegularState_) throw std::logic_error("애시드 실제 현재 편집/로컬 owner 미연결");
    if (!acidFence_) {
        if (!attackAlliancesOwner_) attackAlliancesOwner_=playerRecords_.lock();if (!attackAlliancesOwner_) throw std::logic_error("애시드 실제 고정 동맹 owner 미연결");o::AcidFenceHooks hooks;
        hooks.range=[this](auto parent) { return types.at(pool.Slot(parent)[10]).range; };hooks.find=[this](auto x,auto y,auto type) { return o::RawDaisDeletion::FindTypeAt(pool,hash,types,x,y,type); };
        hooks.iterate=[this](auto area,const auto& consume) { o::RawSquidFinder finder(pool,hash,types);for (auto current=finder.Begin(area,1);current.value;current=finder.Next()) consume(current); }; // 현재 공간 체인의 중복/순서를 복사하지 않는다.
        hooks.reserveRegular=[this] { return ReserveAttackRegular(); };hooks.regular=[this](auto parent,auto event,auto payload) { ConstructAttackRegular(parent,event,payload); };
        hooks.damage=[this](auto target,auto amount,auto owner,auto source) { AttackCall(attackRuntime_,&RawSurfaceAttackBindings::damage,target,amount,owner,source); };hooks.broadcast=[this](auto packet) { const auto send=transmissionDevices_.broadcast;if (!send) throw std::logic_error("애시드 현재 실제 방송 장치 미연결");send(packet); };
        hooks.assertion=[this](auto line) { const char* expression{};switch (line) { case 0x2fa:expression="zd.victimSid == s.getSid()";break;case 0x2fb:expression="zd.aSid == getSid()";break;case 0x2fc:expression="zd.bSid == otherPole.getSid()";break;case 0x2fd:expression="zd.xOriented == xOriented";break;default:throw std::logic_error("애시드 다른 송신 후 진단"); }AssertDisplay(expression,"\\Ns\\O\\Periodicharmer.cpp",line); };
        hooks.setFrame=[this](auto parent,auto frame,auto flags) { Frame().Set(parent,frame,flags); };
        acidFence_=std::make_unique<o::RawAcidFence>(pool,types,frames,spots,attackAlliancesOwner_->Alliances(),authority_->boss,*rewardDevices_.editor,o::RawCheckingFlag{gameAux_->MissionSettings()[3]},playerRegularState_->battleOwner,AcidTypes(),AcidZap(),std::move(hooks));
    }return *acidFence_;
}
}
