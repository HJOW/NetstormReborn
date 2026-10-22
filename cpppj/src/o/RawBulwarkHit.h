// 불워크의48ef20 피해 관문과 일반Damageable 호출을 원본 순서로 연결한다.
#pragma once
#include "o/SidPool.h"
#include "o/RiftType.h"
#include <functional>

namespace netstorm::o {
// 실제+98은공격자0이면곧바로일반피해로넘기며그외현재genus100000만거부한다.
// pool/types는같은월드이며general은같은일반44c8e0을호출해야한다. 반환DWORD를좁히지않는다.
std::uint32_t RawBulwarkHit(const SidPool& pool,std::span<const RiftTypeRecord> types,Sid victim,
    std::int32_t amount,std::uint32_t owner,Sid attacker,
    const std::function<std::uint32_t(Sid,std::int32_t,std::uint32_t,Sid)>& general);
}
