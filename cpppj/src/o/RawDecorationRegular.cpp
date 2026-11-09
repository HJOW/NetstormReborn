// 10.78의462420은 일반 간헐천이 아닌 장식/가장자리 팜의 수명 감소다.
#include "o/RawDecorationRegular.h"
#include <bit>
#include <stdexcept>
namespace netstorm::o { // 여러 실제 호출자가 같은 원문 본문을 사용한다.
namespace { // 현재 선택된 callback 사본과 비정렬 raw 읽기만 이 파일에 한정한다.
template<class Function> Function Need(const Function& function) { if (!function) throw std::logic_error("장식 Regular의 선택된 실제 하위 미연결");return function; } //콜백 중 교체해도 현재 함수 사본의 수명을 유지한다.
std::int32_t Health(const SidPool& pool,Sid sid) { const auto raw=pool.Slot(sid);std::uint32_t value{};for (std::size_t byte=0;byte<4;++byte) value|=std::uint32_t(raw[26+byte])<<(byte*8);return std::bit_cast<std::int32_t>(value); } //원문의 현재 signed DWORD26만 네 바이트로 읽는다.
}
float RunDecorationRegular(const SidPool& pool,const bool& boss,Sid parent,std::uint32_t event,std::uint32_t count,float payload,const DecorationRegularHooks& hooks) {
    if (pool.Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("장식462420은 PATCH10.78 본문입니다");
    if (!event) {
        if (std::bit_cast<std::int32_t>(count)>=3) { if (boss) Need(hooks.destroy)(parent,0);return -1.0F; }
        const auto maximum=Need(hooks.maxHitPoints)(parent); //최대HP의 실제 효과 뒤 현재 HP/VT를 읽는다.
        const auto product=std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(maximum)*(3U-count));
        const auto threshold=product/3;const auto current=Health(pool,parent); //원문 multiply-high/부호 보정은 signed /3의0방향 절삭이다.
        if (threshold<current) Need(hooks.hitPoints)(parent,std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(current)-std::bit_cast<std::uint32_t>(threshold)),0,0);
    }
    return Need(hooks.damageable)(parent,event,count,payload);
}
}
