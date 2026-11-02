// 원문 T/t 토글과 CRT 전투 시계의 현재 두 DWORD를 같은 소유자에 연결한다.
#pragma once
#include "client/InputEvent.h"
#include <functional>

namespace netstorm::client { //004d9862 문자 사건과004a68b0의 작은 계산을 제공한다.
bool DispatchGameTimeKey(InputEvent event,std::uint32_t& enabled,const std::function<bool()>& paused); //뗌/다른 키를 제외하며 현재 정지 뒤 비영 전체를0,0을1로 바꾼다.
std::int32_t ReadGameTimeSeconds(std::uint32_t& stamp,const std::uint32_t& origin,const std::function<std::uint32_t()>& clock); //현재CRT clock 호출 뒤 원점을 읽고stamp를 쓴 다음 감긴signed 차이를1000으로 절삭한다.
}
