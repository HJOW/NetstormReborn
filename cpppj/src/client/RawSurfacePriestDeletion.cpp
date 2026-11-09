// 실제 사제 목록과 보호막 조회·가상 삭제를 World/Session보다 긴 같은 저장소에 연결한다.
#include "client/RawSurfaceWorld.h"
#include <stdexcept>
#include <utility> // 구성한 실제 함수 묶음의 소유권을 장기 본체로 옮긴다.

namespace netstorm::client { // 실제 현재 물리/역할만 빌리며 게임 전역을 다시 초기화하지 않는다.
// 조회와 Shield 생성이 같은 현재005412f4 및 주소 고정 TYPE 표를 사용한다.
o::RawPriestForcefield& RawSurfaceStorage::PriestForcefield() {
    if (!UsesNativeTransmissionDevices()) throw std::logic_error("장기 사제 보호막의 Native 판본 계약");
    if (!priestForcefield_) priestForcefield_=std::make_unique<o::RawPriestForcefield>(pool,hash,types,gameAux_->PriestForcefieldType());
    return *priestForcefield_;
}
// 구성은 주소만 결합하며 실제 삭제에서만 목록 제거→조회→가상 삭제→Carrier 순서를 실행한다.
o::RawPriestPreDestroy& RawSurfaceStorage::PriestPre() {
    if (!UsesNativeTransmissionDevices()) throw std::logic_error("장기 사제 전처리의 Native 판본 계약");
    if (!priestPre_) {
        priestDeletionState_.sharedPriests=&sidLists.priests;priestDeletionState_.sharedOwnerState=&priestOwnerState;
        auto hooks=o::MakeCarrierPreDestroyHooks(pool,CarrierPre(),o::MakePriestForcefieldHooks(pool,PriestForcefield(),
            o::PriestPreDestroyHooks{{},[this](auto sid,auto flags) { DestroyAsset(sid,flags); },{},{}}));
        auto body=std::make_unique<o::RawPriestPreDestroy>(pool,priestDeletionState_,std::move(hooks));body->EnableNativeBody();priestPre_=std::move(body);
    }
    return *priestPre_;
}
}
