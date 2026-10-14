// 캠페인의 실제 Gun 검색/공격 모드/피해 시각 getter를 동일 월드에 연결한다.
#pragma once
#include "client/RawSurfaceWorld.h"
#include "o/RawGunMode.h"
#include "o/RawGunTarget.h"
#include "o/RawGunHit.h"

namespace netstorm::client {
// 실제 모듈/시계/TYPE 간격의 수명은 반환한 callback과 GunProcess보다 길어야 한다.
// 감시/애니메이션/투사체/피해의 큰 효과는 기존 실제 소비자 callback을 보존한다.
o::GunProcessHooks MakeCampaignGunQueryHooks(RawWorldCore core,const o::RawGunMode& mode,
    const o::RawGunTarget& target,const o::RawGunHit& hit,const o::GunProcessState& state,
    std::span<const float> firingIntervals,o::GunProcessHooks hooks);
}
