// 타입과 순번으로 종속 객체를 찾거나 그 객체의 가상 삭제를 요청한다.
#pragma once
#include "o/SidPool.h"
#include <functional>

namespace netstorm::o {
struct ConstructionProcessHooks;
// vtable +0x10의 삭제 경계다. flags는 항상 0이며 호출자는 실제 파생 삭제 분배기를 연결한다.
using ContainedDestroy=std::function<void(Sid,std::uint32_t)>;
// 원본 004b1a90 / CD 004eb310의 조회·삭제 몸체다. 범용 contained finder와 다음 링크를 읽는 시점이 다르다.
// 사용: 풀과 삭제 함수가 참조하는 모듈은 이 객체보다 오래 살아야 한다.
class RawContainedLookup {
public:
    // 삭제 함수가 비었으면 첫 조회/쓰기 전에 std::invalid_argument를 던진다.
    RawContainedLookup(SidPool& pool,ContainedDestroy destroy);
    // parent의 head WORD부터 타입이 같고 dead가 아닌 항목 중 ordinal번째(0부터)를 찾는다.
    // type은 DWORD 전체와 비교한다. free/void/contained 비트는 후보를 제외하지 않는다.
    // remove=false는 해당 SID(없으면 0)를 반환한다. remove=true는 서버 또는 client 번호(5~serverFirst-1)만
    // flags 0으로 삭제하고 계속 진행하여 0을 반환한다. 권한이 없어도 순번은 감소한다.
    // 다음 링크는 삭제 뒤 현재 슬롯에서 읽는다. 순번은 DWORD로 감소하며 0에서 감긴다.
    // 범위 밖/순환 체인은 방문 시 예외로 거부한다. 이미 실행한 삭제 효과를 되돌리지는 않는다.
    Sid Find(Sid parent,std::uint32_t type,std::uint32_t ordinal=0,bool remove=false) const;
    // 건설 어댑터가 같은 월드의 풀을 사용하는지 검사할 때 쓴다.
    const SidPool& Pool() const;
private:
    SidPool& pool_; // 현재 raw 종속 체인과 서버 권한을 공유한다.
    ContainedDestroy destroy_; // 선택한 객체의 가상 삭제다. 같은 풀의 raw를 바꿀 수 있다.
};
// 건설 완료의 removeContained 경계를 실제 조회/삭제(순번 0, 삭제 참)에 연결한다.
// 사용: 나머지 hooks는 유지한다. lookup은 반환한 hooks보다 오래 살아야 하며 다른 풀은 거부한다.
ConstructionProcessHooks MakeContainedConstructionHooks(const SidPool& pool,const RawContainedLookup& lookup,ConstructionProcessHooks hooks);
}
