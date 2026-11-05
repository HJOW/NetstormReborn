// 두 번의 현재 Create와 판본별 WORD/VT 읽기 순서를 원문대로 보존한다.
#include "o/RawDependCreate.h"
#include <stdexcept>
namespace netstorm::o { // 실제 부착 자식의 진단/전송을 이 부모에서 미리 실행하지 않는다.
namespace { // 선택한 장치만 해당 원문 위치에서 요구한다.
template<class Function> const Function& NeedDepend(const Function& function) { if (!function) throw std::logic_error("Depend 생성의 선택된 실제 장치 미연결");return function; } // 앞선 할당/링크 쓰기는 그대로 둔다.
}
DependCreateObjects CreateDependForms(OriginalEdition edition,std::uint32_t watched,std::uint32_t watcher,const DependCreateHooks& hooks) {
    const bool patch=edition==OriginalEdition::Patch1078;const std::uint32_t firstServer=patch ? 15000U : 6000U; // 원문 client 범위는 예약0~4를 제외한다.
    const auto client=[firstServer](std::uint32_t parent) { return parent>=5 && parent<firstServer; }; // signed 원문 비교의 유효 client 집합과 같다.
    const std::uint32_t flags=client(watched) || client(watcher) ? 2U : 0U;
    if (!flags && !NeedDepend(hooks.server)()) NeedDepend(hooks.assertion)("server",patch ? "\\Ns\\O\\Squid.cpp" : "\\ns\\o\\Squid.cpp",patch ? 0xb29U : 0xae0U);
    const auto firstType=NeedDepend(hooks.formType)();const auto first=NeedDepend(hooks.create)(firstType,flags);
    const auto secondType=NeedDepend(hooks.formType)();const auto second=NeedDepend(hooks.create)(secondType,flags);
    const auto secondSid=static_cast<std::uint16_t>(NeedDepend(hooks.sid)(second));std::uint32_t firstTable{}; // 두 번째 객체의 번호는 링크 쓰기보다 먼저 계산한다.
    if (patch) {
        NeedDepend(hooks.word)(first,20,secondSid);NeedDepend(hooks.word)(first,18,0);
        NeedDepend(hooks.word)(second,18,static_cast<std::uint16_t>(NeedDepend(hooks.sid)(first)));NeedDepend(hooks.word)(second,20,0);
        firstTable=NeedDepend(hooks.vtable)(first);
    } else {
        NeedDepend(hooks.word)(first,18,0);NeedDepend(hooks.word)(second,20,0);NeedDepend(hooks.word)(first,20,secondSid);
        firstTable=NeedDepend(hooks.vtable)(first); // CD는 두 번째 watcher WORD 저장보다 앞에서 첫 VT를 저장한다.
        NeedDepend(hooks.word)(second,18,static_cast<std::uint16_t>(NeedDepend(hooks.sid)(first)));
    }
    NeedDepend(hooks.attach)(first,firstTable,watcher,0); // 첫 부착이 두 번째 VT/필드를 바꿀 수 있다.
    const auto secondTable=NeedDepend(hooks.vtable)(second);NeedDepend(hooks.attach)(second,secondTable,watched,0);
    return {second,first};
}
}
