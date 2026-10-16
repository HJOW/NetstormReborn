// 실제 썬 디스크 발사기 포구와 Missile 표적 치수/사각형의 원본 조회를 복원한다.
#pragma once
#include "o/RawGunTarget.h"

namespace netstorm::o {
// TYPE 기본 프레임과 SHP 추가 헤더의 마지막 두 signed WORD다.
struct ProjectileGeometryType {
    std::int32_t defaultFrame{},frameCount{}; // 실제 가상78 기본프레임과 TYPE+114 클러스터 수다.
    std::int32_t directions{}; // 실제 TYPE+110의 균등 투사체 방향 수다.
    std::vector<std::array<std::int16_t,2>> shotOffsets; // 물리 SHP header+20/+22의 포구 픽셀 기준점이다.
    std::int32_t heightExtension{}; // Missile 충돌 상자의 실제 TYPE 확장 높이다.
};
// 표시 hotspot과 포구 픽셀을 구분하며 현재 raw 프레임을 매번 읽는다.
class RawProjectileGeometry {
public:
    // 모든 입력 표와 풀은 이 객체보다 오래 살아야 한다. scale은 현재 BoardCoord 픽셀 배율이다.
    RawProjectileGeometry(const SidPool& pool,std::span<const RiftTypeRecord> types,
        std::span<const ProjectileGeometryType> geometry,float scaleX=16,float scaleY=11);
    // PATCH0046a780/CD00437390: 기본+현재 물리 SHP 포구 픽셀을 더한 뒤 현재 좌표에 반영한다.
    std::array<float,2> ArcherMuzzle(Sid firing) const;
    // 실제 가상+b4를 분배한다.71은 두 SHP 포구의 합,10.78의140은 현재 자산 좌표다.
    std::array<float,2> Muzzle(Sid firing) const;
    // 현재 물리 SHP header+20/+22를 읽는다. 기본 프레임과 합산하지 않는다.
    std::array<std::int32_t,2> Hotspot(Sid firing) const;
    // PATCH0041d6e0: 실제 균등 방향 반올림 뒤 방향당 물리 프레임 수를 곱한다.
    std::int32_t DirectionFrame(std::array<float,2> from,std::array<float,2> to,std::uint32_t type) const;
    // TYPE 실제 footX/footY 조회다. 높이 확장은 ctor의 치수 요청에서 사용하지 않는다.
    std::array<std::int32_t,2> Dimensions(Sid target) const;
    // PATCH0049af10/CD00444b50: 기록된 표적 위치의 full foot/확장 높이 사각형이다.
    std::array<float,4> TargetBounds(Sid target,std::array<float,2> position) const;
    const SidPool& Pool() const; // 조립할 때 다른 월드의 자료 혼용을 거부한다.
private:
    // 현재 유효 SID/TYPE의 raw 슬롯을 조회한다.
    std::span<const std::uint8_t> Slot(Sid sid) const;
    // 실제 SHP header offset을 읽으며 abstract는 원본 진단 인자로만 유지한다.
    std::array<std::int16_t,2> Offset(std::uint32_t type,std::int32_t frame,bool abstract) const;
    const SidPool& pool_; // 실제 자산/표적을 공유하는 같은 풀이다.
    std::span<const RiftTypeRecord> types_; // 실제 발자국과 레이어 flags다.
    std::span<const ProjectileGeometryType> geometry_; // 같은 TYPE 순서의 SHP/기본 프레임 입력이다.
    float scaleX_,scaleY_; // 같은 원본 BoardCoord.add의 현재 픽셀 배율이다.
};
}
