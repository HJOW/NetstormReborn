// 원본 raw SID 풀의 non-void 공간 해제. 공통 표시와 건물 옆면을 같은 공간에 선택 연결한다.
#pragma once
#include "o/SidPool.h"
#include "o/SquidHash.h"

namespace netstorm::o {
class RawGraph;
class SquidDisplay;
class RawBuildingSpace;
// 객체를 공간 해시와 점유 지도에서 해제하는 Unpop 처리기이다.
class SquidUnpop {
public:
    // 기존 SID 풀·네 단계 해시·spot을 함께 갱신한다. nullptr 표시 대상은 기존 억제 경로다.
    SquidUnpop(SidPool& pool,SquidHash& hash,std::span<std::uint8_t> spots,SquidDisplay* display=nullptr);
    // 004afe50 ↔ CD 004ad0b0. 일반 자산/매몰 객체의 spot·머리/이전 next를 직접 해제한다.
    // 일반 섬/다리 통지 큐는 비전투 null 경로다. 미연결 건물 옆면/화면 override는 거부한다.
    void Unpop(Sid sid,const RiftTypeRecord& type,std::uint32_t flags=0);
    // 원본 vtable의 공통 update88/update8c 경로인지 확인한다. 호스트 포인터로 역참조하지 않는다.
    static bool SupportsDisplay(OriginalEdition edition,std::uint32_t vtable,std::uint32_t flags);
    // Factory와 같은 풀을 사용하는지 확인하여 서로 다른 수명 상태의 혼용을 막는다.
    const SidPool& Pool() const;
    // 프레임 변경의 옛 영역 제거가 같은 풀/표시 대상을 쓰는지 확인한다.
    void ValidateFrameBinding(const SidPool& pool,const SquidDisplay& display,const SquidHash& hash,
        std::span<const std::uint8_t> spots) const;
    // 삭제 Graph가 이 Unpop과 같은 해시/spot을 사용하는지 확인한다.
    void ValidateGraph(const RawGraph& graph) const;
    // Pop과 같은 건물 옆면 몸체를 등록하며 서로 다른 풀/해시는 즉시 거부한다.
    void SetBuildingSpace(const RawBuildingSpace* space);
private:
    // 원본 unaligned raw 필드를 호스트 포인터 산술로 해석하지 않는다.
    std::span<std::uint8_t> Bytes(Sid sid);
    // pool_: 객체 슬롯의 생성·조회·반납에 사용하는 SID 풀.
    SidPool& pool_;
    // hash_: 좌표별 객체 후보를 찾는 공간 해시.
    SquidHash& hash_;
    // spots_: 칸별 점유·통행 비트를 읽는 spot 지도.
    std::span<std::uint8_t> spots_;
    SquidDisplay* display_{}; // 제거 전 위치의 변경 표를 공통 표시 경로로 전달한다.
    const RawBuildingSpace* building_{}; // 건물 옆면의 실제 BYTE 해제/진단 몸체다.
};
}
