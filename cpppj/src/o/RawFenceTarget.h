// PATCH10.78의 실제 fencemark 교차 좌표와 가장 가까운 표적 코너를 복원한다.
#pragma once
#include "o/RawProjectileGeometry.h"

namespace netstorm::o {
// 같은 raw 풀·해시·TYPE·현재 동맹으로 배리어 뒤의 투사체 도착 좌표를 계산한다.
class RawFenceTarget {
public:
    // 입력 저장소와 현재 상태는 이 객체보다 오래 살아야 한다. 현재 구현 판본은 PATCH10.78이다.
    RawFenceTarget(const SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,
        std::span<const ProjectileGeometryType> geometry,const RawGunTarget& target,
        const GunTargetState& targeting,const GunProcessState& process,std::uint32_t fenceType=90);
    // 004571b0: 실제 범위의 finder1을 순회하여 포구에 엄격히 가까운 유효 코너를 고른다.
    std::array<float,2> Point(Sid firing,Sid target) const;
    // 004568b0: 한 fencemark의 압축 끝점·동맹·발자국·선분 교차·픽셀 보정이다.
    std::array<float,2> Across(Sid mark,Sid firing,Sid target) const;
    // 004819b0/004818d0의 현재 SHP 포구/TYPE 핫스폿과 기본 발자국 조회다.
    std::array<float,2> Muzzle(Sid firing) const;
    GunTargetRect SightBounds(Sid firing) const; // 외부 관찰도 같은 실제 발사 범위 계산을 사용한다.
    // 00462800/00462710/00462650: 양 선분 안의 교차점이며 각 사각형의 양 대각 끝점은 제외한다.
    static std::array<float,2> Intersection(std::array<float,2> a,std::array<float,2> b,
        std::array<float,2> c,std::array<float,2> d);
private:
    std::span<const std::uint8_t> Slot(Sid sid) const; // 현재 유효 자산의 같은 raw 슬롯을 읽는다.
    std::array<float,2> Position(Sid sid) const; // 저장 float 좌표를 그대로 반환한다.
    std::array<float,2> Add(std::array<float,2> point,int x,int y) const; // 현재 BoardCoord 배율의 픽셀 덧셈이다.
    GunTargetRect Bounds(Sid sid) const; // 확장 높이 flags0의 실제 발자국 사각형이다.
    const SidPool& pool_; // 압축 배리어 끝점과 현재 발사자/표적의 실제 SID 저장소다.
    const SquidHash& hash_; // 실제 후보 순서와 현재 선택 해시 단계의 경계 도장에 사용한다.
    std::span<const RiftTypeRecord> types_; // 현재 발자국/TYPE 핫스폿/group 자료다.
    std::span<const ProjectileGeometryType> geometry_; // 실제 물리 SHP 포구 WORD 자료다.
    const RawGunTarget& target_; // 같은 월드의 실제 foot/2 중심 조회다.
    const GunTargetState& targeting_; // 현재 BoardCoord 픽셀 배율을 각 호출에서 다시 읽는다.
    const GunProcessState& process_; // 원본9×9 동맹 표와 같은 현재 저장소다.
    std::uint32_t fenceType_; // 실제 전역 fencemark 타입 번호다.
};
}
