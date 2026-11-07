// 일반/메타 수신이 같은71h observer·State CS 관문을 공유한다.
#pragma once
#include "o/RawPacketDispatch.h"
#include <string_view> //진단 API의 실제 문자열 뷰 형식을 직접 포함한다.

namespace netstorm::o { // 각 등록표는 실제 현재 CS 주소와 수신 장치를 해당 수명 동안 빌린다.
using StateMetaBattleDescriptionReceive=std::function<void(std::uint32_t,std::span<const std::uint8_t>)>; //0042cad0의 실제 Client CS 자식이다.
using StateMetaBattleDescriptionAssert=std::function<void(std::string_view)>; //원문 packet 파일ae3의 복귀형 진단이다.
void RegisterStateMetaBattleDescription(RawPacketDispatch& dispatch,const std::uint32_t& currentCs,StateMetaBattleDescriptionReceive receive,StateMetaBattleDescriptionAssert assertion); //0040b0d0/004b49c0 전체 observer·CS 관문·버전1/0/가변 길이를 등록한다.
}
