// World 없는 메타 메뉴에 현재 Client의 식별자·주소·Chat·실제 Session 장치를 제공한다.
#pragma once
#include <cstdint>
#include <string_view>

namespace netstorm::client { //같은 NSInfo/Client 수명 동안만 실제 장치를 빌린다.
class Client;class RawMenuControls;struct RawNsInfoContext; //이미 존재하는 실제 소유자/공장이다.
void ParsePlayerMenuAddress(std::string_view text,std::uint32_t port); //004e0cd0과같은20BYTE문법을기존공통주소codec로해석한다.폐기외부DNS는복구하지않는다.
void BindClientMetaPlayerMenu(RawNsInfoContext& context,Client& client,RawMenuControls& menus); //같은현재CS/Player/Chat와원문이름/실제조건을기존전체Player메뉴공장에연결한다.
}
