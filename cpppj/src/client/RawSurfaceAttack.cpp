// 아크/애시드의 전체 정기·검색은 장기 물리에서 실행하고 큰 빔/피해/분해만 실제 Runtime을 빌린다.
#include "client/RawSurfaceWorld.h"
#include "client/ClientPlayerRecords.h"
#include "client/GameAssets.h" // 실제 자산의 activeSound 이름은 같은 Client SoundList에서 한 번 적재한다.
#include "o/RawDaisDeletion.h"
#include <bit> // 현재 프레임 DWORD와 좌표 비트를 계산 없이 읽는다.
#include <new>
#include <stdexcept>
#include <utility>

namespace netstorm::client { // 메뉴·NoWorld·Session이 같은 현재 TYPE/역할/동맹/Host를 사용한다.
namespace { // 선택된 효과는 실제 전체 owner와 함수 사본을 복귀까지 유지한다.
std::uint32_t AttackRead(std::span<const std::uint8_t> raw,std::size_t offset) { // 비정렬 원문 DWORD의 네 바이트만 읽으며 이웃 필드를 섞지 않는다.
    std::uint32_t result{};for (std::size_t byte=0;byte<4;++byte) result|=std::uint32_t(raw[offset+byte])<<(byte*8);return result; // 원문 little endian 폭을 보존한다.
}
template<class Function,class... Args>
decltype(auto) AttackCall(const std::weak_ptr<const RawSurfaceAttackBindings>& weak,Function RawSurfaceAttackBindings::* member,Args&&... args) { // 큰 효과가 없는 가지는 Runtime을 요구하지 않는다.
    const auto current=weak.lock();if (!current) throw std::logic_error("아크/애시드의 실제 현재 Runtime 미연결");const auto pin=current->pin;if (!pin) throw std::logic_error("아크/애시드의 실제 전체 pin 미연결");const auto hold=pin();if (!hold) throw std::logic_error("아크/애시드의 실제 전체 owner 반납");const auto call=current.get()->*member;if (!call) throw std::logic_error("아크/애시드의 선택된 큰 본문 미연결");return call(std::forward<Args>(args)...);
}
}
void RawSurfaceStorage::BindAttackRuntime(const std::shared_ptr<const RawSurfaceAttackBindings>& runtime) { // 현재 큰 효과만 대여하며 순수 장기 상태/본문을 초기화하지 않는다.
    if (!UsesNativeTransmissionDevices() || !runtime || !attackRuntime_.expired()) throw std::invalid_argument("아크/애시드 실제 후단 대여 중복/누락");attackRuntime_=runtime;
}
SoundProcessState& RawSurfaceStorage::SoundState() { // 프레임/TYPE별 마지막 재생 장부는 World 대여와 관계없이 같은 주소를 유지한다.
    if (!UsesNativeTransmissionDevices()) throw std::logic_error("장기 소리 장부의 Native 장치 미연결");soundState_.sharedProcessType=&gameAux_->SoundProcessType();soundState_.debug=Checking();return soundState_;
}
void RawSurfaceStorage::BindAttackSound(SoundList& names,std::function<void(float,float,std::uint32_t)> sound) { // 실제 이름표 정체성만 허용하며 재결합으로 현재 TYPE 토큰을 초기화하지 않는다.
    if (!UsesNativeTransmissionDevices() || !Source() || !sound || (attackSoundNames_ && attackSoundNames_!=&names)) throw std::invalid_argument("장기 공격 소리의 실제 이름표/장치 불일치");
    if (!attackSoundNames_) {
        std::vector<std::uint32_t> tokens(types.size()); // 원문의 미지정 activeSound 초기0을 보존한다.
        for (const auto& asset:Source()->Types()) { const auto name=asset.definition.String("activeSound");tokens.at(70+asset.block)=name && !name->empty() ? names.Lookup(*name):0U; } // 실제 이름표만 조회하며 재생/게임 난수를 소비하지 않는다.
        activeSounds_=std::move(tokens);attackSoundNames_=&names;
    }attackSound_=std::move(sound);
}
std::span<std::uint32_t> RawSurfaceStorage::ActiveSounds() { // 현재 토큰 변경도 같은 장기 Wind 본문에 즉시 반영한다.
    if (!attackSoundNames_ || activeSounds_.size()!=types.size()) throw std::logic_error("장기 공격 소리의 실제 TYPE 토큰 미연결");return activeSounds_;
}
float RawSurfaceStorage::AttackDamageRate(o::Sid source,o::Sid target) const { // 실제4d4d40은 현재 source TYPE를 선택된 반환 지점에서 다시 읽는다.
    if (types.at(pool.Slot(source)[10]).useAirDamage && (types.at(pool.Slot(target)[10]).flags2&0x120000U)) return types.at(pool.Slot(source)[10]).airDamagePerSecond;return types.at(pool.Slot(source)[10]).damagePerSecond;
}
o::RawWindFlyer& RawSurfaceStorage::WindFlyer() { // 정기/초기화/애니메이션은 같은 풀·시각·권한·음향 장부를 사용한다.
    if (!UsesNativeTransmissionDevices()) throw std::logic_error("더스트 데빌의 장기 Native 장치 미연결");
    if (!windFlyer_) {
        auto& sound=SoundState();o::WindFlyerHooks hooks; // 실제 큰 표적/피해 본문만 선택 시 전체 Runtime을 pin한다.
        hooks.runCount=[this](auto sid) { const auto raw=pool.Slot(sid);const auto current=std::bit_cast<std::int32_t>(AttackRead(raw,36));const auto& table=frames.at(raw[10]);if (current<0 || static_cast<std::size_t>(current)>=table.Codes().size()) throw std::out_of_range("더스트 데빌 현재 글자 프레임 범위");return table.Run(table.Codes()[static_cast<std::size_t>(current)].side).count; };
        hooks.assertion=[this](auto line) { if (line!=0xa4 && line!=0x19e) throw std::logic_error("더스트 데빌 다른 원문 진단");AssertDisplay(line==0xa4 ? "parent->getHead() != 0":"period == WIND_FLYER_ATTACK_TIME",line==0xa4 ? "\\Ns\\O\\Baseprocess.cpp":"\\Ns\\O\\Flyer.cpp",line); };
        hooks.damageable=[this](auto sid,auto event,auto count,auto payload) { return ResourceDamageableRegular(sid,event,count,payload); };hooks.destroy=[this](auto sid,auto flags) { DestroyAsset(sid,flags); };
        hooks.sound=[this](auto x,auto y,auto token) { const auto play=attackSound_;if (!play) throw std::logic_error("더스트 데빌 실제 위치 소리 미연결");play(x,y,token); };
        hooks.mayTarget=[this](auto source,auto target) { return AttackCall(attackRuntime_,&RawSurfaceAttackBindings::mayTarget,source,target); };hooks.damageRate=[this](auto source,auto target) { return AttackDamageRate(source,target); };
        hooks.damage=[this](auto target,auto amount,auto owner,auto source) { AttackCall(attackRuntime_,&RawSurfaceAttackBindings::damage,target,amount,owner,source); };hooks.weaken=[this](auto sid) { DamageablePost().Weaken(sid); };
        windFlyer_=std::make_unique<o::RawWindFlyer>(pool,hash,types,std::span(sound.typeFrames).first(types.size()),ActiveSounds(),Processes().State().Now(),authority_->boss,sound.frame,std::move(hooks));
    }return *windFlyer_;
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
