// 메타 객체의 원본+ac 입력을 현재 Client의 실제 장치에 연결한다.
#pragma once
#include "client/InputEvent.h"
#include <cstdint>
#include <functional>
#include <string_view>

namespace netstorm::client { //빌린 실제 소유자의 수명은 동기 입력 호출에 한정한다.
class Client;class RawMenuControls; //같은 Client와 controlled 메뉴 소유자이며 별도 인스턴스를 만들지 않는다.
bool InputClientMetaObject(Client& client,RawMenuControls& menus,std::uint32_t sid,InputEvent event,const std::function<void(std::string_view)>& query); //4789b0의 메뉴·CS·SHP·카메라·입력 큐를 현재 실제 상태에 연결한다.
}
