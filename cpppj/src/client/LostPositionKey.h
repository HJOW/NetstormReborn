// 최근 손실 좌표를 원문 U/u 키와 현재 즉시 카메라에 연결한다.
#pragma once
#include "client/InputEvent.h"
#include <functional>

namespace netstorm::client { // Guide/Console/모달 뒤의 일반 키 부모에서 사용한다.
struct LostPositionKeyHooks { // 선택한 키에서 실제로 도달한 장치만 요구한다.
    std::function<bool()> paused; //00460de0의 현재 정지 판정이며 좌표를 읽기 전에 호출한다.
    std::function<void(float,float)> center; //00433aa0의 현재 X/Y 두 단정도를 받는 즉시 중심 이동이다.
};
//004d98e2의 U/u는 수정키를 거부하지 않으며0040e800의 유효점만 이동한다. 좌표는 같은 장기 owner에서 빌린다.
bool DispatchLostPositionKey(InputEvent event,const float& lostX,const float& lostY,const std::int32_t& worldCells,const LostPositionKeyHooks& hooks);
}
