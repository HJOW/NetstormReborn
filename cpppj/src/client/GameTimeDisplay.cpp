// 게임 정지 시계와 별개인 원문 CRT clock의 signed DWORD 계산을 보존한다.
#include "client/GameTimeDisplay.h"
#include <bit>
#include <stdexcept>

namespace netstorm::client { //실제 현재00594fc0/00594fb0/00594fb4를 호출자가 빌린다.
// 모달을 지난 T/t 누름만 현재 정지 뒤에 표시 DWORD로 반영한다.
bool DispatchGameTimeKey(InputEvent event,std::uint32_t& enabled,const std::function<bool()>& paused) {
    const auto key=event.code&0x03ffffffU; //상위입력플래그를제거하되가상키/명령을문자로축소하지않는다.
    if ((event.code&InputCode::kRelease) || (key!='T' && key!='t')) return false;
    if (!paused) throw std::logic_error("게임 시간 키의 현재 정지 조회 미연결");
    if (!paused()) enabled=enabled==0 ? 1U : 0U;return true;
}
// 실제 CRT clock 호출 뒤 현재 원점을 읽고 감긴 signed 밀리초를 정수 초로 바꾼다.
std::int32_t ReadGameTimeSeconds(std::uint32_t& stamp,const std::uint32_t& origin,const std::function<std::uint32_t()>& clock) {
    if (!clock) throw std::logic_error("게임 시간의 실제 CRT clock 미연결");
    const auto now=clock(),base=origin;stamp=now; //원문은clock복귀뒤원점을읽고현재stamp를쓴다.
    return std::bit_cast<std::int32_t>(now-base)/1000;
}
}
