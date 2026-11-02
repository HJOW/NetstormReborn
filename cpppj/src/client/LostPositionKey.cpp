// 공통 삭제가 기록한 현재005c848c/90을 읽으며 별도 손실 이력이나 좌표 초기화를 만들지 않는다.
#include "client/LostPositionKey.h"
#include "client/ConstructionHistoryKey.h"
#include <stdexcept>

namespace netstorm::client { //원본10.78의004d98e2..004d990d와 같은 일반 입력 분기다.
bool DispatchLostPositionKey(InputEvent event,const float& lostX,const float& lostY,const std::int32_t& worldCells,const LostPositionKeyHooks& hooks) {
    const auto key=event.code&0x03ffffffU; //문자 사건과 일반 명령/가상 키를 구별하며 outside는 원문의 키 마스크처럼 제거한다.
    if ((event.code&InputCode::kRelease) || (key!='U' && key!='u')) return false;
    if (!hooks.paused) throw std::logic_error("최근 손실 위치 키의 실제 정지 판정 미연결");if (hooks.paused()) return true;
    if (IsConstructionHistoryPointValid({lostX,lostY},worldCells)) { //Tab도 사용하는 같은0040e800의 strict 양축 범위이며0/상한/NaN은 무효다.
        if (!hooks.center) throw std::logic_error("최근 손실 위치의 실제 즉시 카메라 미연결");hooks.center(lostX,lostY);
    }
    return true;
}
}
