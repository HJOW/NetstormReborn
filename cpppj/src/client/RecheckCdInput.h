// RecheckCD 일반 입력의 현재 serial 조회→공통 Kill→0쓰기 순서를 같은 Client에 전달한다.
#pragma once
#include "client/InputEvent.h"
#include <functional>
namespace netstorm::client { // 10.78의 RecheckCD는 CD를 다시 찾지 않고 현재 serial의 창만 정리한다(단위696).
// 00543370의 이름표→00542ae8 전역이 가진 초기 명령 DWORD다. 이름·HTML 메타·실제 입력 큐가 모두 이 값을 쓴다. 단위696에서 추가했다.
inline constexpr std::uint32_t kRecheckCdCommand=0x020000fdU;
// 실제로 선택한 현재 등록 표 조회와 공통 Kill 장치다. 두 장치 모두 같은 현재 Gump 등록표를 읽어야 한다. 단위696에서 추가했다.
struct RecheckCdHooks {
    std::function<std::uint32_t(std::uint32_t)> find; // 00464020: serial이 같고 dead가 아닌 첫 Gump 토큰을 돌려주며 없으면 0이다.
    std::function<void(std::uint32_t)> kill; // 00466480: 찾은 Gump를 Kill만 예약하며 실제 소멸은 이후 프레임 Cleanup이 맡는다.
};
// 004d8b8d..004d8b9e의 RecheckCD 분기다. 뗌은 효과 없이 소비하고, 그 밖에는 serial을 조회해 있으면 Kill한 뒤 항상 serial을 0으로 쓴다.
// serial은 같은 Client가 장기 보관하는 005c85ec의 참조이며 0이어도 조회한다(0은 살아 있는 serial 0 Gump와 일치할 수 있다). 단위696에서 추가했다.
bool DispatchRecheckCdInput(InputEvent event,std::uint32_t& serial,const RecheckCdHooks& hooks);
}
