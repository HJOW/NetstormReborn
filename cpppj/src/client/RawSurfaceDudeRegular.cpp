// Dude의 순수 정기/지면/task는 장기 저장소를 공유하고 큰 이동만 현재 Runtime을 빌린다.
#include "client/RawSurfaceWorld.h"
#include "client/RawProcessHostStorage.h"
#include "o/RawDudeRegular.h"
#include "o/RawDudeBehavior.h"
#include "o/RawDudeTask.h"
#include "o/RawDudeNotice.h"
#include "o/RawContainedLookup.h"
#include <stdexcept>
#include <utility>
namespace netstorm::client { // 같은 실제 Pool/지도/목록/Host를 재구성하거나 초기화하지 않는다.
namespace { // 실제 선택된 함수와 전체 Runtime의 호출 수명만 유지한다.
template<class Function,class... Args>
decltype(auto) DudeCall(const std::weak_ptr<const RawSurfaceDudeBindings>& weak,Function RawSurfaceDudeBindings::* member,Args&&... args) {
    const auto current=weak.lock();if (!current) throw std::logic_error("Dude의 실제 현재 이동 Runtime 미연결");const auto pin=current->pin;const auto hold=pin ? pin():std::shared_ptr<void>{};const auto call=current.get()->*member;if (!call) throw std::logic_error("Dude의 선택된 실제 이동 본문 미연결");return call(std::forward<Args>(args)...);
}
}
void RawSurfaceStorage::BindDudeRuntime(const std::shared_ptr<const RawSurfaceDudeBindings>& runtime) { // 현재 큰 효과만 약하게 공개한다.
    if (!UsesNativeTransmissionDevices() || !runtime || !dudeRuntime_.expired()) throw std::invalid_argument("Dude 실제 현재 Runtime 대여 계약");dudeRuntime_=runtime;
}
void RawSurfaceStorage::DudeAssert(std::uint32_t line) { // 원문의 같은 표현/파일/행을 실제 공통 장치로 전달한다.
    const char* expression=line==0xde ? "!s.isDead() && !s.isFree()":line==0x127 ? "walkSid":line==0x9c ? "task == taskFALL":"0";AssertDisplay(expression,".\\Dude.cpp",line);
}
void RawSurfaceStorage::SetDudeWord(o::Sid sid,std::uint32_t value) { // 실제4ac220의 저장 뒤 현재 TYPE를 다시 읽는다.
    const auto current=pool.Slot(sid);auto* raw=const_cast<std::uint8_t*>(current.data());raw[12]=static_cast<std::uint8_t>(value);raw[13]=static_cast<std::uint8_t>(value>>8);if (pool.Slot(sid)[10]==links.bridgeType) Building().NotifySurface(sid);
}
o::RawDudeTask& RawSurfaceStorage::DudeTask() { // 같은 Host의 masked Regular/현재 시각/WORD를 장기로 제공한다.
    if (!UsesNativeTransmissionDevices()) throw std::logic_error("Dude task의 실제 Native 장치 미연결");
    if (!dudeTask_) {
        o::DudeTaskHooks hooks;hooks.assertion=[this] { DudeAssert(0x9c); };hooks.setWord=[this](auto parent,auto word) { SetDudeWord(parent,word); };hooks.notifySurface=[this](auto parent) { Building().NotifySurface(parent); };
        auto& host=Processes().Host();o::DudeTaskState state{Processes().State()};state.native=true;dudeTask_=std::make_unique<o::RawDudeTask>(pool,state,o::MakeDudeTaskHostHooks(pool,host,std::move(hooks)));
    }return *dudeTask_;
}
o::RawDudeBehavior& RawSurfaceStorage::DudeBehavior() { // 현재 점유 지도/작업/종속을 기존 실제 부모에 연결한다.
    if (!UsesNativeTransmissionDevices()) throw std::logic_error("Dude 부모의 실제 Native 장치 미연결");
    if (!dudeBehavior_) {
        o::DudeBehaviorHooks hooks;hooks.task=[this](auto parent,auto event,auto delay) { DudeTask().Set(parent,event,delay); };hooks.movementType=[this](auto type) { return DudeCall(dudeRuntime_,&RawSurfaceDudeBindings::movementType,type); };
        hooks.find=[this](auto parent,auto type,auto index,auto remove) { o::RawContainedLookup lookup(pool,[this](auto sid,auto flags) { DestroyAsset(sid,flags); });return lookup.Find(parent,type,index,remove); };hooks.destroy=[this](auto parent,auto flags) { DestroyAsset(parent,flags); };hooks.basePostDestroy=[this](auto parent,auto flags) { CommonDeletion().PostDestroy(parent,flags); };hooks.assertion=[this](auto line) { DudeAssert(line); };
        o::DudeBehaviorState state{Checking(),sidLists.dudes};state.native=true;dudeBehavior_=std::make_unique<o::RawDudeBehavior>(pool,spots,state,std::move(hooks));
    }return *dudeBehavior_;
}
o::RawDudeRegular& RawSurfaceStorage::DudeRegular() { // 원문450fa0과 각 순수 하위가 같은 현재 TYPE/난수/공급원/순환 값을 사용한다.
    if (!UsesNativeTransmissionDevices()) throw std::logic_error("Dude 정기의 실제 Native 장치 미연결");
    if (!dudeRegular_) {
        o::DudeRegularHooks hooks;hooks.checkSpace=[this](auto parent) { return DudeBehavior().CheckSpace(parent); };hooks.findPath=[this](auto parent,auto type) { o::RawContainedLookup lookup(pool,[this](auto sid,auto flags) { DestroyAsset(sid,flags); });return lookup.Find(parent,type,0,false); };hooks.task=[this](auto parent,auto event,auto delay) { DudeTask().Set(parent,event,delay); };
        hooks.nearestResidence=[this](auto parent) { return o::RawDudeNotice::NearestResidenceCurrent(pool,sidLists.residences,parent); };hooks.entrance=[this](auto target) { return DudeCall(dudeRuntime_,&RawSurfaceDudeBindings::entrance,target); };hooks.move=[this](auto parent,auto point,auto quickly) { return DudeCall(dudeRuntime_,&RawSurfaceDudeBindings::move,parent,point,quickly); };
        hooks.setFrame=[this](auto parent,auto frame,auto flags) { Frame().Set(parent,frame,flags); };hooks.unpop=[this](auto parent,auto flags) { UnpopAsset(parent,flags); };hooks.pop=[this](auto parent,auto point,auto flags) { PopAsset(parent,point.x,point.y,flags); };hooks.destroy=[this](auto parent,auto flags) { DestroyAsset(parent,flags); };hooks.assertion=[this](auto line) { DudeAssert(line); };hooks.setWord=[this](auto parent,auto word) { SetDudeWord(parent,word); };
        o::DudeRegularState state{Checking(),sidLists.providers,gameAux_->DudeProviderIndex()};state.native=true;state.sharedPollEvent=&gameAux_->DamageRegularEvent();state.sharedPathType=&gameAux_->DudePathType();dudeRegular_=std::make_unique<o::RawDudeRegular>(pool,frames,[this]()->o::GameRandom& { const auto get=resourceDevices_.random;if (!get) throw std::logic_error("Dude의 같은 현재 게임 난수 미연결");return get(); },state,std::move(hooks));
    }return *dudeRegular_;
}
}
