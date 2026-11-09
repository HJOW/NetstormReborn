// 현재 실제 플레어 생성·예약·주기·소멸은 Session을 소유하지 않고 같은 물리를 사용한다.
#include "client/RawSurfaceWorld.h"
#include "o/RawFlare.h"
#include <bit>
#include <new>
#include <stdexcept>
#include <utility>

namespace netstorm::client { // 장기 저장소의 현재 TYPE/전역/Host만 빌린다.
namespace { // raw 프레임 DWORD를 정렬되지 않은 원문 폭으로 읽는다.
std::uint32_t FlareDword(std::span<const std::uint8_t> raw,std::size_t offset) { // 같은 현재 슬롯의 네 바이트만 조합한다.
    std::uint32_t value{};for (std::size_t i=0;i<4;++i) value|=std::uint32_t(raw[offset+i])<<(8*i);return value; // 이웃 BYTE를 읽거나 별도 프레임 사본을 만들지 않는다.
}
}
void RawSurfaceStorage::BindFlareDevices(RawSurfaceFlareDevices devices) { // 같은 억제 시각과 실제 음향만 최초 연결하며 값을 초기화하지 않는다.
    if (!UsesNativeTransmissionDevices() || flareDevices_.suppression || !devices.suppression || !devices.sound) throw std::invalid_argument("플레어의 실제 최초 Client 장치 계약");flareDevices_=std::move(devices);
}
o::RawFlare& RawSurfaceStorage::Flare() { // 구성 시 장치/난수를 소비하지 않으며 실제 선택한 효과만 실행한다.
    if (!UsesNativeTransmissionDevices()) throw std::logic_error("플레어의 실제 Native 장치 미연결");
    if (!flare_) {
        o::FlareHooks hooks;hooks.processCount=[this] { return static_cast<std::int32_t>(Processes().Kernel().Size()); };hooks.create=[this](auto type,auto flags) { return CreateAsset(type,flags); };
        hooks.sourceHotspot=[this](auto parent) { const auto raw=pool.Slot(parent);const auto frame=std::bit_cast<std::int32_t>(FlareDword(raw,36));if (raw[10]>=shapes.size() || frame<0 || static_cast<std::size_t>(frame)>=shapes[raw[10]].frames.size()) throw std::out_of_range("플레어의 현재 SHP 기준점 물리 범위");const auto& header=shapes[raw[10]].frames[static_cast<std::size_t>(frame)];return std::array<std::int16_t,2>{header.hotspotX,header.hotspotY}; };
        hooks.owner=[this](auto parent,auto owner) { Owner().Set(parent,owner); };hooks.pop=[this](auto parent,auto x,auto y,auto flags) { PopAsset(parent,x,y,flags); };
        hooks.reserveRegular=[this] { if (flareReservation_) throw std::logic_error("플레어 Regular 실제 확보 중첩");flareReservation_.reset(::operator new(sizeof(o::RegularProcess),std::nothrow));return static_cast<bool>(flareReservation_); };
        hooks.addRegular=[this](auto parent,auto event,auto payload) { if (!flareReservation_) throw std::logic_error("플레어 Regular 실제 확보 메모리 누락");auto* memory=flareReservation_.release();o::RegularProcess* process{};try { process=new(memory) o::RegularProcess(event,payload); }catch (...) { ::operator delete(memory);throw; }auto* attached=Processes().Host().AttachRegular(std::unique_ptr<o::RegularProcess>(process),parent);if (!attached) throw std::logic_error("플레어 Regular 실제 부착 실패");attached->SetNextTime(Processes().State().Now()); };
        hooks.frame=[this](auto parent,auto frame,auto flags) { Frame().Set(parent,frame,flags); };hooks.update=[this](auto parent) { Display().Update(pool.Slot(parent)); };hooks.destroy=[this](auto parent,auto flags) { DestroyAsset(parent,flags); };
        hooks.sound=[this](auto x,auto y,auto name,auto flags) { const auto play=flareDevices_.sound;if (!play) throw std::logic_error("플레어의 현재 위치 음향 미연결");play(x,y,name,flags); };
        o::FlareState state{[this] { const auto current=flareDevices_.suppression;if (!current) throw std::logic_error("플레어의 같은 현재 억제 시각 미연결");return *current; },LoadingDepth(),gameAux_->BombFlareType()};
        flare_=std::make_unique<o::RawFlare>(pool,types,frames,[this]()->o::GameRandom& { const auto get=resourceDevices_.random;if (!get) throw std::logic_error("플레어의 같은 현재 게임 난수 미연결");return get(); },std::move(state),std::move(hooks));
    }return *flare_;
}
}
