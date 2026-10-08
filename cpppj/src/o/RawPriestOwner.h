// Priest의 소유자 재정의(00491790 / CD 0040bda0)를 raw SID에 복원한다.
#pragma once
#include "o/SquidOwner.h"
#include <functional>

namespace netstorm::o {
// 실제 priest 타입과 가상 표 기록값이다. 주소를 호스트에서 역참조하지 않는다.
inline constexpr std::uint32_t kPriestType=158,kPatchPriestVtable=0x0050f210,kCdPriestVtable=0x005003e0;
struct PriestOwnerState {
    std::uint32_t loadingDepth{}; // 005c89b8 / CD 005178d4: 요새를 읽는 중첩 횟수다.
    std::uint32_t fortPlayer{}; // 00540cac / CD 0050f6d4: 요새 모드에서 비트 7을 켜는 플레이어다.
    std::uint32_t battlePlayer{}; // 00540c70 / CD 0050f6c8: 전투 모드에서 비트 7을 켜는 플레이어다.
};
struct PriestOwnerHooks {
    std::function<void(Sid,std::uint32_t)> base; // 공통 SquidOwner::Set에 연결한다.
    std::function<void(Sid)> notify; // 004214a0 / CD 00448c10: 원본은 사제 상태를 바꾸지 않는 호출이다.
};
class RawPriestOwner {
public:
    // 풀/공통 모드/상태는 이 객체보다 오래 살아야 한다. 가상 효과를 모두 연결한다.
    RawPriestOwner(SidPool& pool,const SquidOwnerMode& mode,const PriestOwnerState& state,PriestOwnerHooks hooks);
    // 실제 가상 표 기록값으로 사제 소유자 재정의 여부를 판정한다.
    bool Handles(Sid sid) const;
    // 로딩/비전투는 word의 low 7비트를 갱신하고, 전투 중에는 그 원래 소유자를 다시 사용한다.
    // 패치판의 범위 밖 인자는 공통 지정만 생략한다. CD의 공통 assert와 패치의 로딩 중 0은 변경 전 예외다.
    void Set(Sid sid,std::uint32_t player) const;
private:
    SidPool& pool_;
    const SquidOwnerMode& mode_;
    const PriestOwnerState& state_;
    PriestOwnerHooks hooks_;
};
// 다른 가상 소유자 분배기(MakeOwnerDispatch)의 base로 연결할 수 있다. 사제만 재정의하고 나머지는 base로 넘긴다.
// priest와 base가 참조하는 월드는 반환 함수보다 오래 살아야 한다.
std::function<void(Sid,std::uint32_t)> MakePriestOwnerDispatch(RawPriestOwner& priest,std::function<void(Sid,std::uint32_t)> base);
}
