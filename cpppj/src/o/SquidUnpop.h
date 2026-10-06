// 원본 raw SID 풀의 non-void 공간 해제. 표시 비활성 경로이며 실제 월드/파생 영역 효과는 후속이다.
#pragma once
#include "o/SidPool.h"
#include "o/SquidHash.h"

namespace netstorm::o {
class SquidUnpop {
public:
    // 기존 SID 풀·네 단계 해시·spot을 함께 갱신한다. 표시 갱신이 비활성인 호출 단계에서 사용한다.
    SquidUnpop(SidPool& pool,SquidHash& hash,std::span<std::uint8_t> spots);
    // 004afe50 ↔ CD 004ad0b0. 일반 자산/매몰 객체의 spot·머리/이전 next를 직접 해제한다.
    // 섬/다리·건물 부착 표면 효과·미복원 화면 override는 활성 상태에서 변경 전에 거부한다.
    void Unpop(Sid sid,const RiftTypeRecord& type,std::uint32_t flags=0);
    // 원본 vtable의 공통 update88/update8c 경로인지 확인한다. 호스트 포인터로 역참조하지 않는다.
    static bool SupportsDisplay(OriginalEdition edition,std::uint32_t vtable,std::uint32_t flags);
    // Factory와 같은 풀을 사용하는지 확인하여 서로 다른 수명 상태의 혼용을 막는다.
    const SidPool& Pool() const;
private:
    // 원본 unaligned raw 필드를 호스트 포인터 산술로 해석하지 않는다.
    std::span<std::uint8_t> Bytes(Sid sid);
    SidPool& pool_;
    SquidHash& hash_;
    std::span<std::uint8_t> spots_;
};
}
