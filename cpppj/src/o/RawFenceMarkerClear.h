//004566d0의 실제 fencemark 끝점 태그/현재 finder/가상 삭제를 같은 월드에서 실행한다.
#pragma once
#include "o/RawGunTarget.h"

namespace netstorm::o {
// 화면의 포스 필드(TYPE170)와 별도인 fencemark(TYPE90)의 기존 연결 표식을 지운다.
class RawFenceMarkerClear {
public:
    RawFenceMarkerClear(const SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,
        const RawGunTarget& target,const std::uint32_t& markerType,std::function<void(Sid,std::uint32_t)> destroy); //같은 현재 풀/해시/TYPE/중심/삭제 소비자를 빌린다.
    SquidSearchArea Bounds(Sid parent,Sid other) const; //실제 중심 두 점의 min/max·clamp·floor/ceil 검색 영역이다.
    void Clear(Sid parent,Sid other) const; //QA 또는 packedQB/graph가 현재 부모인 표식만 가상+10(flags40)으로 지운다.
private:
    const SidPool& pool_; //현재 두 끝점과 후보의 전체 원본 슬롯이다.
    const SquidHash& hash_; //삭제 도중 Next가 현재 체인을 읽는 실제 공간 해시다.
    std::span<const RiftTypeRecord> types_; //finder의 실제 발자국 표다.
    const RawGunTarget& target_; //같은 풀에서004adea0의 현재 중심을 계산한다.
    const std::uint32_t& markerType_; //DAT005411c0의 현재 fencemark 타입 번호다.
    std::function<void(Sid,std::uint32_t)> destroy_; //선택 위치의 실제 가상 삭제 소비자다.
};
}
