// AI의 group3 배치가 사용하는 별도 지상 표적 검색을 같은 실제 Gun 월드에 연결한다.
#pragma once
#include "o/RawGunTarget.h"

namespace netstorm::o {
// 원본 각 조회 위치에서 다시 읽는 전역이며 별도 상태 사본을 만들지 않는다.
struct GunGroundTargetGlobals {
    const std::uint32_t& excludedType; //00541178의 현재 보행 운반체 검색 제외 TYPE다.초기값72다.
    const std::uint32_t& dependType; //00541074의 현재 Depend Form TYPE다.초기값1이다.
    const bool& checking; //005e4794의 현재 원본 진단 활성화다.
    const bool& metamap; //00594fc8의 현재 전역 모드다.
    const bool& boss; //00540bc4의 현재 권한이다.
    std::function<void(std::string_view,std::uint32_t)> assertion; //원본 진단 장치이며 반환하면 원래 순서로 계속한다.
};
//0046b540/0046b360과DependFinder를같은해시/타입/일반Gun필터에서수행한다.
class RawGunGroundTarget {
public:
    RawGunGroundTarget(const SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,std::span<const GunTargetType> targetTypes,const RawGunTarget& targeting,const GunTargetState& state,GunGroundTargetGlobals globals); //같은PATCH물리풀과현재전체TYPE를빌린다.
    Sid Find(float x,float y,std::uint32_t type,std::uint32_t owner) const; //0046b540:문맥방향/프레임0→중심보정사각finder5→지상조건→엄격한거리최소다.
    bool MayTarget(const GunTargetQuery& query,Sid target) const; //0046b360:비행/벌룬제외→보행운반체의현재TYPE/Depend→기존실제MayTarget이다.
    const SidPool& Pool() const; //같은실제월드연결확인용이다.
private:
    bool HasDependency(Sid parent) const; //004b22d0/004b1b80/004b1bd0의현재head/next·TYPE<70·현재dependTYPE첫일치다.
    double Distance(const GunTargetQuery& query,Sid target) const; //00469440의확장발자국/반대경계중점과제곱거리다.비교전에float로줄이지않는다.
    const SidPool& pool_; //현재상태/좌표/Depend종속체인의실제물리풀이다.
    const SquidHash& hash_; //실제일반finder의현재네단계버킷이다.
    std::span<const RiftTypeRecord> types_; //현재발자국/flags1/flags2다.
    std::span<const GunTargetType> targetTypes_; //현재사거리와높이확장이다.
    const RawGunTarget& targeting_; //같은실제HP/서버번호/동맹/사제수량·발자국필터다.
    const GunTargetState& state_; //같은현재friendlyFire와Player동맹조회다.
    GunGroundTargetGlobals globals_; //현재원본전역의참조와진단장치다.
};
}
