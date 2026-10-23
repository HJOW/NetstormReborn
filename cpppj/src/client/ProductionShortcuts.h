// 원본 Userinput의 생산 조각 문자 키를 실제 명령 큐에 예약한다.
#pragma once
#include "client/InputEvent.h"
#include <functional>

namespace netstorm::client {
inline constexpr std::uint32_t kProductionBridgeCommand=0x020000f4U; //00542ac4: 현재 기술의 slot BYTE를 취득하는 원본 명령이다.
// 명령 DWORD 조회와 원본452fe0 큐 예약만 외부에 연결한다.
struct ProductionShortcutHooks {
    std::function<std::uint32_t()> command; // E의 여섯 예약도 매번 같은 현재 명령 전역을 읽는다.
    std::function<void(std::uint32_t,std::int32_t,std::int32_t)> queue; // 명령,slot,0을 순서대로 받으며 큐의 포화 정책은 해당 큐가 수행한다.
};
// Guide/Gump 뒤의 일반 키 구간이다. release/정지에서는 예약하지 않으며 modifier와 활성 커서를 추가로 거부하지 않는다.
bool QueueProductionShortcut(InputEvent event,bool paused,const ProductionShortcutHooks& hooks);
}
