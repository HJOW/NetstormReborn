// 실제 키 분기와 E의 슬롯0~5 예약 순서를 보존한다.
#include "client/ProductionShortcuts.h"
#include <stdexcept>

namespace netstorm::client {
// 선택된 문자 키만 외부 명령 조회/예약을 요구한다. 커서 회전·Drop은 별도 원본 구간이다.
bool QueueProductionShortcut(InputEvent event,bool paused,const ProductionShortcutHooks& hooks) {
    if ((event.code&InputCode::kRelease) || paused) return false;
    const auto enqueue=[&](std::int32_t slot,bool repeat=false) { // getter 효과 뒤 해당 명령 인자값을 고정하고 같은 큐에 예약한다.
        const auto& lookup=repeat ? hooks.repeatCommand : hooks.command; //최근 항목과 여섯 다리 슬롯의 현재 명령을 구별한다.
        if (!lookup) throw std::logic_error("생산 단축키의 현재 명령 조회 누락");const auto command=lookup();
        if (!hooks.queue) throw std::logic_error("생산 단축키의 실제 큐 예약 누락");hooks.queue(command,slot,0);
    };
    switch (event.code&0x03ffffffU) {
    case 'd':case 'D':enqueue(0,true);break; //004d96fa의getlastitem은 별도 최근TYPE 소비자에0/0만 보낸다.
    case 'q':case 'Q':enqueue(0);break;
    case 'w':case 'W':enqueue(1);break;
    case 'a':case 'A':enqueue(2);break;
    case 's':case 'S':enqueue(3);break;
    case 'z':case 'Z':case 'y':case 'Y':enqueue(4);break;
    case 'x':case 'X':enqueue(5);break;
    case 'e':case 'E':
        for (std::int32_t slot=0;slot<6;++slot) enqueue(slot); // 원본은 현재 슬롯 수와 무관하게 여섯 명령을 FIFO로 예약한다.
        break;
    default:return false;
    }
    return true;
}
}
