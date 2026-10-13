// 실제 표면/다리의 섬 번호 지정 몸체를 같은 SID 풀과 현재 타입 표에 적용한다.
#pragma once
#include "o/SidPool.h"
#include <functional>

namespace netstorm::o {
// 패치판의 두 원본 assert 원인이다. 진단 뒤 원본처럼 region 쓰기를 생략한다.
enum class SquidRegionAssert {
    // UnsupportedGenus: island/bridge/island3x3 중 어느 genus도 없는 현재 타입이다.
    UnsupportedGenus,
    // BridgeRegion: 다리는 현재 noRegion DWORD만 받을 수 있으며 항상 기존 WORD를 보존한다.
    BridgeRegion
};
// 패치004acd20/CD004aca80: 허용 표면의raw+8 WORD에 region 하위16비트를 저장한다.
// 패치 진단 분기는 필수 callback을 호출하며 CD는 조용히 반환한다. callback이 없으면 해당 분기를 거부한다.
void SetSquidRegion(SidPool& pool,std::span<const RiftTypeRecord> types,Sid sid,std::uint32_t region,
    std::uint32_t noRegion,const std::function<void(SquidRegionAssert)>& diagnostic={});
}
