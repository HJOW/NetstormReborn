// 장식·가장자리 팜의 단계별 HP 감소와 마지막 삭제를 원문462420대로 공유한다.
#pragma once
#include "o/SquidProcess.h"
namespace netstorm::o { // 실제 raw 필드와 선택된 가상 하위만 사용한다.
struct DecorationRegularHooks { // 큰 최대HP/현재가상98·삭제와 공통 부모의 실제 경계다.
    std::function<std::int32_t(Sid)> maxHitPoints; //4adcb0의 현재 TYPE/제단 역할 조회다.
    std::function<void(Sid,std::int32_t,std::uint32_t,std::uint32_t)> hitPoints; //현재가상98(amount,owner0,attacker0)의 실제 분배다.
    std::function<void(Sid,std::uint32_t)> destroy; //count>=3의 현재가상10(flags0)이며 서버만 호출한다.
    RegularHandler damageable; //나머지 사건의 실제44b900이며 반환 payload를 그대로 전달한다.
};
float RunDecorationRegular(const SidPool& pool,const bool& boss,Sid parent,std::uint32_t event,std::uint32_t count,float payload,const DecorationRegularHooks& hooks); // signed count·DWORD 곱/감김·0방향 나눗셈·현재HP 읽기 순서를 보존한다.
}
