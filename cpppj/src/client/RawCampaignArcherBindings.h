// 실제 캠페인의 썬 디스크 발사기·썬/썬더 캐논·반더 타워·크로스보우 애니메이션을 같은 Core/Host에 조립한다.
#pragma once
#include "client/RawSurfaceWorld.h"
#include "o/RawArcherAnimation.h"
#include "o/RawGunTarget.h"
#include "o/SpStore.h"
#include "o/RawPriestPlacementGeometry.h"
#include "o/RawCannonFire.h"
#include <memory>

namespace netstorm::client {
// 음향·배리어 코너와 아직 외부에 있는 파생 효과는 실제 소비자가 공급한다.
struct RawCampaignArcherEffects {
    o::ArcherAnimationHooks animation; // 현재 음향/방향/배리어 코너의 실제 경계다.
    o::MissileHooks missile; // 포구/SHP/피해/충돌의 실제 경계이며 기본 성공으로 대신하지 않는다.
    o::CannonBeamHooks beam; // 실제 반더 빔 생성·초기 source 시각/피해와 같은 게임 난수다.
    std::function<void(o::Sid)> iceCooling; //004a8770의 실제 아이스 냉각 표시이며 선택 시 누락을 거부한다.
};
// 같은 풀의 실제 Archer/Missile 몸체를 고정 주소에 소유한다.
class RawCampaignArcherBindings {
public:
    // Core/TYPE 보충 표/실제 프레임 코드/전역/난수는 이 연결 객체보다 오래 살아야 한다.
    RawCampaignArcherBindings(RawSurfaceWorld& world,const o::RawGunTarget& target,o::GameRandom& random,
        std::span<const o::ArcherTypeData> animationTypes,std::span<const o::PriestPlainCanonType> frameTypes,std::span<const o::MissileTypeData> missileTypes,
        o::ArcherAnimationState& animation,o::MissileState& missile,RawCampaignArcherEffects effects);
    // 해제 전에 상위가 같은 Kernel을 비우고 반환 callback 사용을 끝내야 한다.
    ~RawCampaignArcherBindings();
    RawCampaignArcherBindings(const RawCampaignArcherBindings&)=delete; // callback의 동일 객체 주소를 유지한다.
    RawCampaignArcherBindings& operator=(const RawCampaignArcherBindings&)=delete; // 다른 Core로 복사하지 않는다.
    // 같은 시계의 게임 시각/경과를 다음 전체 Kernel 실행 전에 공급한다.
    void PrepareFrame(o::FrameTime time);
    // 기존 다른 총 경계를 보존하며 실제 타입71/128/131/140/148의 원본 생성기에 연결한다.
    o::GunProcessHooks GunHooks(o::GunProcessHooks hooks) const;
    // 실제 썬 디스크의 예약 프레임 처리를 같은 지정기에 연결한다.
    o::RegularHandler Handler(o::RegularHandler fallback={}) const;
    // 후속 실제 전투/검사가 같은 생성/프레임 처리기를 재사용한다.
    const o::RawArcherAnimation& Animation() const;
    const o::RawMissile& Missile() const;
    const o::SidPool& Pool() const;
private:
    struct Impl; // 같은 Core와 실제 처리기를 고정 주소에 소유하는 내부 자료다.
    std::unique_ptr<Impl> impl_; // 외부 전역/World/Host 수명을 상위가 보장한다.
};
}
