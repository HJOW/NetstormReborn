// 벌룬의 그림자와 실제 Carrier 부모를 World·Session보다 오래 같은 저장소에 보유한다.
#include "client/RawSurfaceWorld.h"
#include "o/RawFrameAdvance.h" // Cloud Floater의 사건3도 실제 현재 프레임 변경을 사용한다.
#include <stdexcept>
#include <utility>

namespace netstorm::client { // 현재 장기 물리만 빌리고 새 월드나 상태 사본은 만들지 않는다.
// 두 실제 VT의 그림자 생성·표시·삭제와 공통 깊이를 같은 본체에 연결한다.
o::RawBalloonBody& RawSurfaceStorage::BalloonBody() {
    static_cast<void>(DamageablePreGlobals()); // 실제 Client 장치 결합 뒤에만 구성하며 자연 관문은 앞당기지 않는다.
    if (!balloonBody_) {
        o::BalloonBodyHooks hooks; // 선택 시점의 실제 TYPE/Frame/Factory/Host를 소비한다.
        hooks.frames=[this](auto type) { if (type>=frames.size()) throw std::out_of_range("장기 벌룬 현재 TYPE 프레임 범위");return static_cast<std::int32_t>(frames[type].Codes().size()); };
        hooks.create=[this](auto type,auto flags) { return CreateAsset(type,flags); };
        hooks.pop=[this](auto sid,auto x,auto y,auto flags) { PopAsset(sid,x,y,flags); };
        hooks.unpop=[this](auto sid,auto flags) { UnpopAsset(sid,flags); };
        hooks.destroy=[this](auto sid,auto flags) { DestroyAsset(sid,flags); };
        hooks.check=[this](auto sid) { const auto check=resourceDevices_.carrierCheck;if (!check) throw std::logic_error("장기 벌룬 현재 Carrier check 미연결");check(sid); };
        hooks.allocateRegular=[this] { return AllocateResourceRegular(); };
        hooks.constructRegular=[this](auto parent,auto event,auto payload) { ConstructResourceRegular(parent,event,payload); };
        balloonBody_=std::make_unique<o::RawBalloonBody>(pool,types,spots,authority_->boss,ResourceCarrierPostPop(),CarrierPre(),std::move(hooks));
    }
    return *balloonBody_;
}
// 원문42af60의 사건3은 현재 프레임만 진행하고 다른 사건은 기존 실제42a750에 그대로 전달한다.
float RawSurfaceStorage::BalloonRegular(o::Sid sid,std::uint32_t event,std::uint32_t count,float payload) {
    return BalloonBody().CloudRegular(sid,event,count,payload,
        [this](auto parent,auto delta,auto flags) { o::RawFrameAdvance advance(pool,frames,Frame());static_cast<void>(advance.Advance(parent,delta,flags)); },
        [this](auto parent,auto code,auto times,auto value) { return CarrierRegular().Handle(parent,code,times,value); });
}
}
