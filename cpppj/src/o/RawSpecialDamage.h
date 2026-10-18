// 실제 신전의 건설 중 피해 분배와 윈드 타워의 방향 방어를 복원한다.
#pragma once
#include "o/RawGunHit.h"
#include "o/RiftType.h"

namespace netstorm::o {
// 신전 가상 피해가 호출하는 기존 실제 몸체의 경계다.
struct VortexDamageHooks {
    std::function<std::uint32_t()> constructionType; // 현재 건설 프로세스30 전역을 호출 시 읽는다.
    std::function<Sid(Sid,std::uint32_t,std::uint32_t,bool)> find; // 실제4b1a90의 부모/타입/순번/삭제 인수다.
    std::function<void(Sid,std::uint32_t)> resetEnd; // 실제443dc0으로 같은 건설 Form의 끝을 변경한다.
    std::function<void(Sid)> refresh; // 같은 신전의 실제 가상88 표시 갱신이다.
    std::function<bool(Sid,std::int32_t,std::uint32_t,Sid)> general; // 완공 신전은 기존44c8e0 일반 피해를 사용한다.
};
// PATCH4dba40: abstract bit1은 현재 건설을 지연하고1반환, 나머지는 일반피해의 반환을 보존한다.
bool RawVortexDamage(const SidPool& pool,Sid victim,std::int32_t damage,std::uint32_t owner,Sid attacker,const VortexDamageHooks& hooks);
// PATCH48ef70: 현재 L/M/N/O와 공격자 중심의4방향을 비교하며 flyer/balloon/arcSpire는 방어하지 않는다.
bool RawWindTowerDefense(const SidPool& pool,std::span<const RiftTypeRecord> types,std::span<const RiftTypeFrames> frames,
    Sid victim,Sid attacker,std::uint32_t arcSpireType=149);
}
