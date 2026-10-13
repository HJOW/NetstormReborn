// 실제 xor eax,eax; ret4 기본 방어의 무조건false 계약만 복원한다.
#include "o/RawSquidDefense.h"
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 실제 두 판본의 기본 방어 진입 주소다. PE에서5바이트 몸체를 독립 실행해 대조한다.
constexpr std::uint32_t kPatchDefense=0x43d9b0,kCdDefense=0x4aed90;
// 비정렬 raw 가상 표의 현재 little endian DWORD를 읽는다.
std::uint32_t DefenseTable(std::span<const std::uint8_t> raw) {
    std::uint32_t value=0;
    // 현재 네 바이트만 낮은 자리부터 조합한다.
    for (std::size_t i=0;i<4;++i) value|=static_cast<std::uint32_t>(raw[i])<<(8*i);
    return value;
}
}
// 실제 기본 몸체의 인자 무관 반환이며 다른 파생 가상 호출의 기본값으로 사용하지 않는다.
bool RawSquidDefaultDefense(Sid,Sid) { return false; }
// 현재 vtable 대응표에 없는 파생 메서드를 묵시적으로false로 바꾸지 않는다.
DamageableHitHooks MakeRawSquidDefenseHooks(const SidPool& pool,std::span<const SquidDefenseEntry> entries,DamageableHitHooks hooks) {
    if (entries.empty()) throw std::invalid_argument("실제 기본 방어 가상 진입 표 누락");const auto base=pool.Edition()==OriginalEdition::Patch1078 ? kPatchDefense : kCdDefense;
    hooks.rejectDamage=[&pool,entries,base,fallback=std::move(hooks.rejectDamage)](Sid victim,Sid attacker) {
        if (!victim.value || victim.value>=pool.Capacity()) throw std::out_of_range("실제 방어 현재 SID 범위 오류");const auto table=DefenseTable(pool.Slot(victim));std::uint32_t address=0;
        // 현재 raw vtable와 일치하는 실제 진입 하나만 선택한다.
        for (const auto& entry:entries) if (entry.vtable==table) { address=entry.address;break; }
        if (address==base) return RawSquidDefaultDefense(victim,attacker);if (!fallback) throw std::logic_error("다른 파생 실제 방어 몸체 미복원");return fallback(victim,attacker);
    };return hooks;
}
}
