// 사제 494580의 사건 선택과 Carrier 부모 순서를 typed/Native에서 공유한다.
#pragma once
#include "o/SquidProcess.h"
#include <functional>
namespace netstorm::o { // 현재 SID/사건을 원문 순서로 전달하는 공용 정기 몸체다.
struct PriestRegularHooks { // 선택한 고유 후단만 요구하며 미선택 경계는 읽지 않는다.
    std::function<void(Sid,std::uint32_t)> destroy; //601 양수count의 실제 가상10 삭제다.
    RegularHandler unique; //602 회복/603 추락/604 의식 프레임/605 기도의 기존 실제 본문이다.
    RegularHandler carrier; // 나머지 사건의 직접42a750 부모다.
};
float RunPriestRegular(Sid sid,std::uint32_t event,std::uint32_t count,float payload,const PriestRegularHooks& hooks); // count는 부호 없는 비트로 전달하고601은0인지 여부만 읽는다.
}
