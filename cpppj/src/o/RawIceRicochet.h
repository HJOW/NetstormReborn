// 10.78 Missile의00482080 반사점과00482190 후보 선택/후속 생성 요청을 복원한다.
#pragma once
#include "o/RawMissile.h"
#include "o/RawSquidFinder.h"
#include "o/SpStore.h"

namespace netstorm::o {
class SquidReward; // 같은 월드의 실제 HP 보유 판정을 빌리는 장기 처리기다.
// 원본 일반 Finder와 HP/새 Missile 수명을 같은 월드에 연결하는 경계다.
struct IceRicochetHooks {
    std::function<Sid(SquidSearchArea,std::uint32_t)> begin; // 실제00422230이 준비한 사각형/플래그의 일반 탐색 시작이다.
    std::function<Sid()> next; // 현재 Finder의 다음 후보를 반환한다. 후보 변경은 다음 조회에 반영된다.
    std::function<bool(Sid)> hasHitPoints; // 같은 SquidReward의004adc60 HP 보유 판정이다.
    std::function<void(const GunProjectileRequest&)> createProjectile; // 실제 반사 Missile의 생성·Pop·Host 부착을 수행해야 한다.
    std::function<void(std::span<std::uint8_t>)> initialPointBytes; // 변위0에서 Point가 쓰지 않은 호출자 지역점8BYTE를 공급한다. 임의 성공 좌표로 대체하지 않는다.
};
// 전체 반사탄 수명과 분리한 원본 반사점/확률/표적 선택 몸체다.
class RawIceRicochet {
public:
    // 패치판 같은 풀·현재 타입 범위·공유 GameRandom을 빌린다.
    RawIceRicochet(const SidPool& pool,std::span<const RiftTypeRecord> types,
        std::span<const MissileTypeData> missileTypes,GameRandom& random,IceRicochetHooks hooks);
    //00482080: 변위0이면 output과 난수를 보존하고 false, 나머지는 원본 점을 쓰고 true다.
    bool Point(const MissileFields& fields,std::array<float,2>& output) const;
    //00482190: [0,99) 확률·현재 후보 선택 후 새 Missile을 요청한다. 빈 경계를 성공 처리하지 않는다.
    void Generate(const MissileFields& fields) const;
private:
    // 물리 SID/TYPE 범위를 검사하며 원문에 없는 free/dead 제외는 추가하지 않는다.
    std::span<const std::uint8_t> Slot(Sid sid) const;
    const SidPool& pool_; // 표시와 후보가 사용하는 동일 raw 슬롯 저장소다.
    std::span<const RiftTypeRecord> types_; // 현재 후보 HP/비행 속성의 타입 표다.
    std::span<const MissileTypeData> missileTypes_; // 현재 표시 타입의 반사 사거리를 읽는다.
    GameRandom& random_; // SP/공격/소리도 소비하는 같은32비트 난수 상태다.
    IceRicochetHooks hooks_; // 같은 Finder/HP/후속 생성 경계다.
};
// 현재 일반 Finder와 HP를 실제 같은 풀에 묶고 후속 Missile 생성 경계는 그대로 보존한다.
// 반환한 콜백보다 pool/hash/types/reward가 오래 살아야 한다.
IceRicochetHooks MakeIceRicochetWorldHooks(const SidPool& pool,const SquidHash& hash,
    std::span<const RiftTypeRecord> types,const SquidReward& reward,IceRicochetHooks hooks);
}
