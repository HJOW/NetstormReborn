// 사제 좌표 한 점의 보호막을 실제 일반 finder의 타입/소유자 순회로 조회한다.
#pragma once
#include "o/RawPriestPreDestroy.h"
#include "o/RawSquidFinder.h"

namespace netstorm::o {
struct PriestForcefieldState {
    std::uint32_t type{167}; // 005412f4 / CD 0051cbe0: BYTE로 줄이지 않는 실제 타입 번호 전역이다.
};
class RawPriestForcefield {
public:
    // 타입은 복사하고 풀/해시/전역은 참조한다. 참조 대상은 조회 객체보다 오래 살아야 한다.
    RawPriestForcefield(const SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,PriestForcefieldState& state);
    // 004918e0 / CD 0040bf00. 좌표를 0방향 절삭하고 같은 타입·owner BYTE의 첫 SID 또는 0을 반환한다.
    // dead/void/추상/매장은 사제 자체의 조회 생략 조건이 아니다. 후보의 매장은 finder가 제외한다.
    Sid Find(Sid priest) const;
    // 삭제 준비 연결 시 다른 월드의 풀 혼합을 거부한다.
    const SidPool& Pool() const;
    // 조회 뒤 생성에 사용할 현재 DWORD 타입 전역을 다시 읽는다.
    std::uint32_t Type() const;
private:
    const SidPool& pool_;
    const SquidHash& hash_;
    std::vector<RiftTypeRecord> types_;
    PriestForcefieldState& state_;
};
// 같은 풀의 findForcefield만 실제 조회로 바꾼다. 나머지 삭제/Carrier 경계는 호출자가 공급한다.
PriestPreDestroyHooks MakePriestForcefieldHooks(const SidPool& pool,const RawPriestForcefield& lookup,PriestPreDestroyHooks hooks);
}
