// Fountain의 대상 검색·도착·정기를 같은 현재 물리/카메라/Host에 연결한다.
#include "client/RawSurfaceWorld.h"
#include "client/RawCameraView.h"
#include "client/RawCursorPosition.h"
#include "o/RawRiftShapeAccess.h"
#include "o/RawProcessRemoval.h" // 같은 Host의 현재 Regular/SharedRegular 역할을 실제로 제거한다.
#include <new>
#include <stdexcept>
#include <utility>

namespace netstorm::client { // 장기 본문은 Session을 생성하거나 전역 상태를 초기화하지 않는다.
void RawSurfaceStorage::BindManaDevices(RawSurfaceManaDevices devices) { // 같은 카메라와 실제 음향을 첫 효과 전에 한 번 연결한다.
    if (!UsesNativeTransmissionDevices() || manaDevices_.camera || !devices.camera || !devices.sound) throw std::invalid_argument("Fountain의 실제 최초 카메라/음향 계약");
    manaDevices_=std::move(devices);
}
o::ManaBoltRegularState& RawSurfaceStorage::ManaState() { return manaState_; } // 같은 현재 간격/spacing의 주소를 빌린다.
o::RawManaBoltArrival& RawSurfaceStorage::ManaArrival() { // 현재 finder와 실제 Host 예약을 한 번 구성하며 구성 중 장치를 호출하지 않는다.
    if (!UsesNativeTransmissionDevices()) throw std::logic_error("Fountain의 실제 Native 도착 장치 미연결");
    if (!manaArrival_) {
        manaArrivalState_.currentRegularType=&gameAux_->RegularProcessType();manaArrivalState_.currentSharedRegularType=&gameAux_->SharedRegularProcessType();manaArrivalState_.currentDelay=&manaState_.interval;
        o::ManaBoltArrivalHooks hooks;
        hooks.sound=[this](auto x,auto y,auto name) { const auto play=manaDevices_.sound;if (!play) throw std::logic_error("Fountain의 실제 위치 음향 미연결");play(x,y,name); };
        hooks.removeProcesses=[this](auto parent,auto type) { o::RemoveAttachedProcesses(Processes().Host(),parent,type); };
        hooks.reserveRegular=[this] { if (manaReservation_) throw std::logic_error("Fountain의 실제 Regular 확보 중첩");manaReservation_.reset(::operator new(sizeof(o::RegularProcess),std::nothrow));return static_cast<bool>(manaReservation_); };
        hooks.addRegular=[this](auto parent,auto event,auto payload) {
            if (!manaReservation_) throw std::logic_error("Fountain의 실제 Regular 확보 누락");auto* memory=manaReservation_.release();o::RegularProcess* process{};
            try { process=new(memory) o::RegularProcess(event,payload); }catch (...) { ::operator delete(memory);throw; }
            auto* attached=Processes().Host().AttachRegular(std::unique_ptr<o::RegularProcess>(process),parent);if (!attached) throw std::logic_error("Fountain의 실제 Regular 부착 실패");attached->SetNextTime(Processes().State().Now());
        };
        hooks.destroy=[this](auto parent,auto flags) { DestroyAsset(parent,flags); };
        manaArrival_=std::make_unique<o::RawManaBoltArrival>(pool,hash,types,manaArrivalState_,std::move(hooks));
    }
    return *manaArrival_;
}
o::RawManaBoltRegular& RawSurfaceStorage::ManaRegular() { // 같은 장기 대상 검색·Frame·가상 공간과 현재 카메라를 선택 위치에서 사용한다.
    if (!UsesNativeTransmissionDevices()) throw std::logic_error("Fountain의 실제 Native 정기 장치 미연결");
    if (!manaRegular_) {
        o::ManaBoltRegularHooks hooks;
        hooks.setFrame=[this](auto parent,auto value,auto flags) { Frame().Set(parent,value,flags); };
        hooks.sound=[this](auto x,auto y,auto name) { const auto play=manaDevices_.sound;if (!play) throw std::logic_error("Fountain의 실제 위치 음향 미연결");play(x,y,name); };
        hooks.project=[this](auto x,auto y) { const auto camera=manaDevices_.camera;if (!camera) throw std::logic_error("Fountain의 현재 카메라 투영 미연결");return ProjectCameraView(*camera,x,y); };
        hooks.inverse=[this](auto x,auto y) { const auto camera=manaDevices_.camera;if (!camera) throw std::logic_error("Fountain의 현재 카메라 역투영 미연결");return RawCursorPosition::ScreenToWorld(x,y,camera->worldBits); };
        hooks.unpop=[this](auto parent,auto flags) { UnpopAsset(parent,flags); };hooks.pop=[this](auto parent,auto x,auto y,auto flags) { PopAsset(parent,x,y,flags); };hooks.destroy=[this](auto parent,auto flags) { DestroyAsset(parent,flags); };
        hooks.frameZero=[this](auto type) {
            o::RiftShapeAccessHooks access;access.typeNumber=[](auto selected) { return selected.token; };
            access.frameError=[this](auto number,auto frame,auto abstract) { RecordDisplayFrameError(number,frame,abstract); };access.assertion=[this](auto expression,auto file,auto line) { AssertDisplay(expression,file,line); };
            access.shape=[this](auto selected,auto frame) { const auto& shape=shapes.at(selected);if (!shape.loaded || frame<0 || static_cast<std::size_t>(frame)>=shape.frames.size()) throw std::out_of_range("Fountain 진단 복귀 뒤 실제 SHP 범위");const auto& header=shape.frames[static_cast<std::size_t>(frame)];return std::array<float,2>{header.cellWidth,header.cellHeight}; };
            const auto& record=types.at(type);const auto& shape=shapes.at(type);static_cast<void>(o::ReadRiftShapeFrame({type,record.flags1,shape.frameCount,shape.loaded,record.maxHitPoints},0,0,access));return shapes.at(type).frames.at(0);
        };
        manaRegular_=std::make_unique<o::RawManaBoltRegular>(pool,types,frames,shapes,ManaArrival(),manaState_,std::move(hooks));
    }
    return *manaRegular_;
}
}
