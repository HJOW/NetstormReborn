// Player 정기는 같은 현재 장기 상태/Host를 쓰고 선택된 Template만 실제 Runtime을 빌린다.
#include "client/RawSurfaceWorld.h"
#include <new>
#include <stdexcept>
#include <utility>
namespace netstorm::client { // Player9/World를 새로 만들지 않는 현재 Client 정기 분배다.
namespace { // 선택된 큰 경계는 실제 owner 확보 뒤 함수 사본을 호출한다.
std::shared_ptr<const RawSurfacePlayerBindings> CurrentPlayerRuntime(const std::weak_ptr<const RawSurfacePlayerBindings>& weak) { // 현재 Template 대여의 실제 생존을 확인하며 빈 후단을 성공으로 바꾸지 않는다.
    const auto current=weak.lock();if (!current) throw std::logic_error("Player 정기의 실제 현재 Template Runtime 미연결");return current;
}
}
void RawSurfaceStorage::BindPlayerRegularState(std::shared_ptr<o::PlayerWorldState> state) { // 현재 주소만 최초 공개하고 owner/known/모드는 쓰지 않는다.
    if (!UsesNativeTransmissionDevices() || !state || borrowed_ || playerRegularState_) throw std::invalid_argument("Player 정기의 실제 최초 현재 전역 계약");playerRegularState_=std::move(state);
}
void RawSurfaceStorage::BindPlayerRegularRuntime(const std::shared_ptr<const RawSurfacePlayerBindings>& runtime) { // 같은 현재 큰 후단 대여만 갱신하고 장기 정기를 재생성하지 않는다.
    if (!UsesNativeTransmissionDevices() || !runtime || !playerRegularRuntime_.expired()) throw std::invalid_argument("Player 정기의 실제 Template 대여 중복/누락");playerRegularRuntime_=runtime;
}
bool RawSurfaceStorage::ReservePlayerRegular() { // 원문 new28 한 개와 호스트 Regular 객체의 실제 크기를 연결한다.
    if (!UsesNativeTransmissionDevices()) throw std::logic_error("Player 예약의 Native 장치 미연결");if (playerRegularReservation_) throw std::logic_error("Player 실제 Regular 확보 중첩");playerRegularReservation_.reset(::operator new(sizeof(o::RegularProcess),std::nothrow));return static_cast<bool>(playerRegularReservation_);
}
void RawSurfaceStorage::ConstructPlayerRegular(o::Sid sid,std::uint32_t event,float payload) { // 같은 확보 메모리의 생성/부착/현재시각을 실제 Host에 맡긴다.
    if (!playerRegularReservation_) throw std::logic_error("Player 실제 Regular 생성 전 확보 누락");auto* memory=playerRegularReservation_.release();o::RegularProcess* process{};try { process=new(memory) o::RegularProcess(event,payload); }catch (...) { ::operator delete(memory);throw; }Processes().Host().AttachRegular(std::unique_ptr<o::RegularProcess>(process),sid,false);
}
float RawSurfaceStorage::PlayerRegular(o::Sid sid,std::uint32_t event,std::uint32_t count,float payload) { // 실제 Pool/현재 두 owner/battle/로그/진단과 예약을 같은 원문 몸체로 연결한다.
    if (!UsesNativeTransmissionDevices() || !playerRegularState_) throw std::logic_error("Player 정기의 실제 현재 전역 미연결");
    const auto raw=pool.Slot(sid);std::uint32_t table{};for (std::size_t byte=0;byte<4;++byte) table|=std::uint32_t(raw[byte])<<(byte*8); // 실제 현재 VT 네 BYTE만 조합하며 TYPE/Player9는 읽지 않는다.
    if (table!=0x50ee08U) throw std::logic_error("다른 Player 정기 가상 표");
    {
        o::PlayerRegularHooks hooks;hooks.assertion=[this](auto) { AssertDisplay("0","\\Ns\\O\\Player.cpp",0x41d); };
        hooks.log=[this](auto group,auto text) { const auto log=graphTrace_;if (!log) throw std::logic_error("Player 정기의 현재 실제 로그 미연결");log(static_cast<std::int32_t>(group),text); };
        hooks.reserveRegular=[this] { return ReservePlayerRegular(); };hooks.addRegular=[this](auto parent,auto event,auto payload) { ConstructPlayerRegular(parent,event,payload); };
        hooks.templatePin=[this] { const auto current=CurrentPlayerRuntime(playerRegularRuntime_);const auto pin=current->pin;if (!pin) throw std::logic_error("Player Template의 실제 전체 Runtime pin 미연결");const auto hold=pin();if (!hold) throw std::logic_error("Player Template의 실제 전체 Runtime owner 반납");return hold; };
        hooks.templateState=[this](auto owner) { const auto current=CurrentPlayerRuntime(playerRegularRuntime_);const auto state=current->state;if (!state) throw std::logic_error("Player 정기의 실제 현재 State getter 미연결");return state(owner); };
        hooks.templateChanged=[this](auto owner) { const auto current=CurrentPlayerRuntime(playerRegularRuntime_);const auto changed=current->changed;if (!changed) throw std::logic_error("Player 정기의 실제 현재 Template Notify 미연결");changed(owner); };
        return o::RunPlayerRegular(pool,*playerRegularState_,ownerMode,sid,event,count,payload,hooks);
    }
}
}
