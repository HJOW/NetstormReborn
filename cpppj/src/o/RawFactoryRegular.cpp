// Factory의 순수 사건 분배와 현재 조회를 두 실제 호출자가 같은 본문에서 수행한다.
#include "o/RawFactoryRegular.h"
#include <bit>
#include <stdexcept>
namespace netstorm::o { // 게임 전역/타입 표/장치 사본을 만들지 않는다.
namespace { // 선택된 callback 사본만 이 파일에서 고정한다.
template<class Function> Function Need(const Function& function) { if (!function) throw std::logic_error("Factory Regular의 선택된 실제 하위 미연결");return function; } //콜백 중 교체해도 현재 호출을 유지한다.
}
float RunFactoryRegular(const SidPool& pool,Sid parent,std::uint32_t event,std::uint32_t count,float payload,const FactoryRegularHooks& hooks) {
    if (pool.Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("Factory4549c0은 PATCH10.78 본문입니다");
    if (event==2) {
        const auto firstLevel=Need(hooks.level)(parent);const auto first=Need(hooks.firstFrame)(parent,firstLevel);
        const auto currentLevel=Need(hooks.level)(parent);const auto size=Need(hooks.frameCount)(parent,currentLevel);
        const auto offset=Need(hooks.random)(std::bit_cast<std::uint32_t>(size));
        Need(hooks.frame)(parent,std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(first)+offset),0);
    } else if (event==1) Need(hooks.advance)(parent,1,0);
    else if (event==3) {
        if (count) return Need(hooks.finish)(parent);
        Need(hooks.begin)(parent,payload);Need(hooks.advance)(parent,1,0);
    } else if (event==4 && count) return 0.0F;
    else if (event==5 && Need(hooks.level)(parent)>2) Need(hooks.flare)(parent,2,1);
    return Need(hooks.damageable)(parent,event,count,payload);
}
}
