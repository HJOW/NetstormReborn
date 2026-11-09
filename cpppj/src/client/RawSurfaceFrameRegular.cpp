// 두 실제 프레임 Regular를 같은 장기 Pool/TYPE/Frame/현재 가상 삭제에 연결한다.
#include "client/RawSurfaceWorld.h"
#include "o/RawGunImpact.h"
#include "o/RawDiscRegular.h"
#include <stdexcept>

namespace netstorm::client { // 생성자/난수/카메라/Session을 임시로 구성하지 않고 실제 선택 본문만 호출한다.
float RawSurfaceStorage::ImpactRegular(o::Sid sid,std::uint32_t event,std::uint32_t count,float payload) {
    if (!UsesNativeTransmissionDevices()) throw std::logic_error("장기 충돌 Regular의 Native 판본 계약");
    return o::RawGunImpact::HandleIn(pool,frames,Frame(),[this](auto object,auto flags) { DestroyAsset(object,flags); },sid,event,count,payload);
}
// event/count는 원문도 읽지 않으며 별도 Session 방향 자료 사본을 만들지 않는다.
float RawSurfaceStorage::DiscRegular(o::Sid sid,std::uint32_t,std::uint32_t,float payload) {
    if (!UsesNativeTransmissionDevices()) throw std::logic_error("장기 Disc Regular의 Native 판본 계약");
    return o::RawDiscRegular::HandleIn(pool,Frame(),[this](auto type) {
        if (type>=types.size()) throw std::out_of_range("장기 Disc 현재 TYPE 범위");
        o::ArcherTypeData data;data.frameCount=types[type].frameCount;data.directions=types[type].directions;return data; // 원문+114/+110을 같은 현재 레코드에서 읽는다.
    },sid,payload,true);
}
}
