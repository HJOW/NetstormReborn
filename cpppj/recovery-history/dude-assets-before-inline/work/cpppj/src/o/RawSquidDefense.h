// 실제 기본 가상+0x9c의 false 몸체를 현재 raw 가상 표와 검증된 진입 주소로 분배한다.
#pragma once
#include "o/RawDamageableHit.h"

namespace netstorm::o {
// 원본 PE에서 읽은 현재 가상 표와 해당+0x9c 진입 주소의 대응이다.
struct SquidDefenseEntry {
    std::uint32_t vtable{},address{}; // 가상 표를 호스트에서 역참조하지 않고 두32비트 값을 비교한다.
};
// 원본0043d9b0/CD004aed90의 두 명령은 this/공격SID를 읽지 않고 false를 반환한다.
bool RawSquidDefaultDefense(Sid victim,Sid attacker);
// 검증된0043d9b0/CD004aed90만 실제 false를 반환하고 다른 파생 방어는 기존 경계로 보낸다.
// 풀/대응표는 반환 callback보다 오래 살아야 하며 타입 번호로 현재 가상 표를 추측하지 않는다.
DamageableHitHooks MakeRawSquidDefenseHooks(const SidPool& pool,std::span<const SquidDefenseEntry> entries,
    DamageableHitHooks hooks);
}
