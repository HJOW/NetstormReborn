// 실제 Client의 같은 CS/Pool·Image·TYPE·현재 메뉴 글꼴을 메타 섬 Draw에 연결한다.
#pragma once
#include "client/Screen.h"
#include "o/SidPool.h"

namespace netstorm::client { // 호출은 통합 메뉴 WithDrawing의 현재 픽셀 수명 안에서만 허용한다.
class Client;class RawMenuControls; // 실제 소유자를 빌리고 이 연결에서 생성하지 않는다.
void DrawClientMetaSprite(Client& client,RawMenuControls& menus,o::Sid object,ScreenPoint point,std::span<std::uint8_t> pixels,int pitch,int height,ScreenRect clip); //새 잠금/목록/프레임 없이 현재+a4를 같은 잠긴 화면에 출력한다.
}
