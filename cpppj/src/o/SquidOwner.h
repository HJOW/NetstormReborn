// Squid의 소유자 지정(vtable +0x74)을 raw SID 풀과 소유자별 작업장 목록에 연결한다.
#pragma once
#include "o/SidPool.h"
#include "o/SquidPostPop.h"

namespace netstorm::o {
// 소유자 지정이 읽는 원본 모드 전역이다. 패치 주소 / CD 주소를 괄호에 둔다.
struct SquidOwnerMode {
    bool fort{};      // 00594fb8 / CD 00540a1c: 요새(편집) 모드.
    bool battle{};    // 00594fbc / CD 00540a20: 전투 모드. 둘 중 하나가 켜져 있어야 작업장 목록을 고친다.
    bool challenge{}; // 00594fc8 / CD 00540a24: 켜져 있으면 유효한 소유자 번호가 1~39로 바뀐다(원본 CL_NUM_CHAL_PLAYERS).
};
class SquidOwner {
public:
    // 풀·장부·모드는 이 객체보다 오래 살아야 한다. 타입 표는 복사한다.
    SquidOwner(SidPool& pool, std::span<const RiftTypeRecord> types, SquidPostPopState& bookkeeping, const SquidOwnerMode& mode);
    // 004adf00 ↔ CD 004aefa0: 객체의 소유자 바이트를 바꾼다. 요새/전투 모드에서 genus가 vortex·factory(0x4200)이면
    // 이전 소유자의 작업장 목록에서 이 번호를 모두 빼고, void·buried가 아니면 새 소유자의 목록 끝에 넣는다(가득 차면 넣지 않는다).
    // 소유자 0은 목록이 없다. 범위 밖 번호는 원본이 assert를 보고한 뒤 하위 바이트를 쓰지만 여기서는 쓰기 전에 거부한다.
    void Set(Sid sid, std::uint32_t player);
    // 가상 효과 어댑터가 같은 실제 풀의 소유자를 변경하는지 확인한다.
    const SidPool& Pool() const;
private:
    SidPool& pool_;
    std::vector<RiftTypeRecord> types_;
    SquidPostPopState& bookkeeping_;
    const SquidOwnerMode& mode_;
};
}
