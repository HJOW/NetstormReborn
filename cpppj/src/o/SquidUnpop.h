// 원본 raw SID 풀의 non-void 공간 해제. 공통 표시 선택 연결이며 파생 영역 효과는 후속이다.
#pragma once
#include "o/SidPool.h"
#include "o/SquidHash.h"

namespace netstorm::o {
class SquidDisplay;
class SquidUnpop {
public:
    // 기존 SID 풀·네 단계 해시·spot을 함께 갱신한다. nullptr 표시 대상은 기존 억제 경로다.
    SquidUnpop(SidPool& pool,SquidHash& hash,std::span<std::uint8_t> spots,SquidDisplay* display=nullptr);
    // 004afe50 ↔ CD 004ad0b0. 일반 자산/매몰 객체의 spot·머리/이전 next를 직접 해제한다.
    // 일반 섬/다리의 표면 통지 큐는 비전투 null 경로다. 건물 부착/미복원 화면 override는 거부한다.
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
    SquidDisplay* display_{}; // 제거 전 위치의 변경 표를 공통 표시 경로로 전달한다.
};
}
