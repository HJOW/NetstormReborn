// 건물 Pop/Unpop의 옆면 표면 단어와 원본 다리 진단을 같은 raw 공간에 적용한다.
#pragma once
#include "o/SidPool.h"
#include "o/SquidHash.h"

namespace netstorm::o {
// 원본 타입 전역과 현재 debug 전역이다. 섬 확인은 영역 번호나 소유자를 조회하지 않는다.
struct BuildingSpaceState {
    std::uint32_t islandType{},bridgeType{};
    bool debug{};
};
// 표면 WORD 지도는 실제 해시 0단계 배열이다. 복사 지도는 사용하지 않는다.
class RawBuildingSpace {
public:
    // 사용 순서: 실제 풀/해시/타입으로 생성하고 같은 SquidPop/Unpop에 SetBuildingSpace로 등록한다.
    RawBuildingSpace(SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,BuildingSpaceState& state);
    // 0040da00/CD 004abac0: 지도 밖이면 0, 그 외 해당 칸의 WORD SID를 반환한다.
    Sid Surface(int x,int y) const;
    // 0046f680/CD 004bdd20: 지면 타입이 현재 islandType인지만 확인한다. float 경계는 0 방향 절삭이다.
    bool HasIsland(float x,float y) const;
    // Pop의 옆면 한 칸이다. 표면 플래그와 건물 자체의 현재 좌표 지면을 확인해 BYTE/WORD를 쓴다.
    void PopSide(int x,int y,float buildingX,float buildingY) const;
    // Unpop의 옆면 한 칸이다. 표면 플래그 검사 없이 유효 SID의 낮은 BYTE 비트 1/2를 지운다.
    void UnpopSide(int x,int y) const;
    // 004214a0/CD 00448c10: 실제 무상태 다리 수명 진단이다. debug 오류는 호스트 예외로 보고한다.
    void NotifySurface(Sid sid) const;
    // 서로 다른 풀/해시의 혼용을 구성 단계에서 거부한다.
    void ValidateSpace(const SidPool& pool,const SquidHash& hash) const;
private:
    // 원본 유효 번호 후보는 free 상태라도 쓰므로 일반 AllocatedBytes 필터를 적용하지 않는다.
    std::span<std::uint8_t> Bytes(Sid sid) const;
    SidPool& pool_;
    const SquidHash& hash_;
    std::span<const RiftTypeRecord> types_;
    BuildingSpaceState& state_;
};
}
