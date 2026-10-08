// 섬 받침의 preDestroy 재정의(00442240)와 noIsland의 postDestroy 재정의(00442640)를 raw SID 풀 위에서 복원한다.
// 지연 낙하 예약·walker 낙하·공통 pre/postDestroy는 호출자가 연결하고 순서·인자만 원본대로 호출한다.
// 근거와 검증 범위: docs/exe/cpp-islandlifecycle-reconstruction.md, 세 실제 PE의 제한 x86 기대값 islandlifecycle-x86.tsv.
#pragma once
#include "o/RawSquidNeighbors.h"
#include <functional>

namespace netstorm::o {
// 섬 받침 preDestroy가 둘레 다리의 지연 낙하 예약을 건너뛰는 삭제 flags 비트다(원본 `flags & 0x1000`).
inline constexpr std::uint32_t kIslandDestroyKeepsBridges = 0x1000;
// 두 재정의가 부르는 외부 효과다. 호출 순서는 원본과 같다.
struct IslandLifecycleHooks {
    // 00421f90 → 00421530 / CD 004490b0: 다리 칸에 (x, y)를 가리키는 지연 낙하 이벤트(0x2692)를 예약한다.
    // 패치판 넘김 함수의 디버그 검사(대상이 다리가 아니면 assert)는 호출 조건이 이미 다리이므로 도달하지 않는다.
    std::function<void(Sid bridge, float x, float y)> scheduleFall;
    // 가상 표 +0xc8: 칸 위 walker의 낙하.
    std::function<void(Sid walker)> fallWalker;
    // 공통 preDestroy(004b0950 / CD 004add20).
    std::function<void(Sid sid, std::uint32_t flags)> basePreDestroy;
    // 공통 postDestroy(004b0840 / CD 004adbe0).
    std::function<void(Sid sid, std::uint32_t flags)> basePostDestroy;
};
class RawIslandLifecycle {
public:
    // 풀과 이웃 탐색기는 이 객체보다 오래 살아야 하고 같은 풀을 읽어야 한다. 훅은 모두 연결돼 있어야 한다.
    RawIslandLifecycle(const SidPool& pool, const RawSquidNeighbors& neighbors, IslandLifecycleHooks hooks);
    // 00442240 ↔ CD 004d0250(섬 받침 vtable +0x14): 받침이 abstract/buried가 아니면 중심을 구하고 flag 0 연결 이웃을 순회한다.
    // flags에 0x1000이 없을 때, 죽지 않은 다리 이웃마다 받침의 중심 좌표로 지연 낙하를 예약한다(섬이 사라지면 닿아 있던 다리가 끝 칸으로 바뀐다).
    // 그 뒤 항상 공통 preDestroy를 부른다.
    void IslandPreDestroy(Sid island, std::uint32_t flags) const;
    // 00442640 ↔ CD 004d2a40(noIsland vtable +0x18): 표면 칸이 abstract/buried가 아니면 그 칸에 걸린 객체를
    // 일반 탐색(해시 0단계 건너뜀)으로 훑어 walker genus(0x10000)마다 가상 낙하를 부른다. 그 뒤 항상 공통 postDestroy를 부른다.
    void SurfacePostDestroy(Sid surface, std::uint32_t flags) const;
private:
    // 004ac200 ↔ CD 004abae0: 객체 타입의 genus(flags2)다.
    std::uint32_t Genus(Sid sid) const;
    const SidPool& pool_;
    const RawSquidNeighbors& neighbors_;
    IslandLifecycleHooks hooks_;
};
}
