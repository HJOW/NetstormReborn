// Missile.cpp의 실제 썬 캐논·썬 디스크 발사기 투사체 생성/이동을같은raw 자산/Kernel에연결한다.
#pragma once
#include "o/RawGunAnimation.h"
#include "o/SquidFactory.h"

namespace netstorm::o {
// 패치 0x80/CD0x68 생성자 필드이며 사용하지 않는 padding도 보존한다.
struct MissileFields {
    // bytes: 판본별 raw 객체 필드와 padding 이미지다.
    std::array<std::uint8_t,0x80> bytes{};
    // 실제 Baseprocess의 표시 자산 부모와 발사/표적 SID를 읽는다.
    Sid Form() const;
    Sid Parent() const;
    Sid Firing() const;
    Sid Target() const;
};
// 타입 표에서 Missile 생성/이동이 직접 읽는 보충 속성이다.
struct MissileTypeData {
    // range: 현재 발사 타입 +0x44의 실제 사거리다.
    float range{};
    // frameCount: +0x110의 방향 수이며 0이면 표시 프레임을 선택하지 않는다.
    std::int32_t frameCount{};
    // firingSound/impactSound: 타입 +0x70/+0x74의 실제 사운드 토큰이다.
    std::uint32_t firingSound{},impactSound{};
};
// 같은 실제 게임 시각과 원본 전역 타입 번호다.
struct MissileState {
    // now: 정지 시 유지되는 현재 게임 시각이다.
    double now{};
    // sunCannonType/sunDiscType/sunArcherType: 캐논128·표시sunDisc83·발사기71의원본타입이다.
    std::uint32_t sunCannonType{128},sunDiscType{83},sunArcherType{71};
    // crossbowType: 10.78의 group0 화살 발사기이며 포구 보정은71과 구별한다.
    std::uint32_t crossbowType{140};
    // thunderCannonType: 같은 group1 Missile 경로를 사용하는10.78 썬더 캐논의 현재 타입이다.
    std::uint32_t thunderCannonType{131};
    // thunderMissileType: 사거리 종료 여유를 별도로 읽는10.78 썬더 표시 타입이다.
    std::uint32_t thunderMissileType{132};
    // stoneTowerType: 썬더 충돌 시 튜토리얼 알림이 필요한 원본00541190의 현재 타입이다.
    std::uint32_t stoneTowerType{78};
    // hitCommand: 원본005429c8의 충돌 명령 토큰이며 두 인자는 표시/표적 타입이다.
    std::uint32_t hitCommand{0x020000b5U};
    // rangeMargin/thunderRangeMargin: 원본00540bb4/00540bb8의 일반4·썬더10 정수 여유다.
    std::int32_t rangeMargin{4},thunderRangeMargin{10};
    // processType: 실제 missileProcessType이며 원본 두 판본 모두28이다.
    std::uint32_t processType{28};
    // cornerImpactType: incomingCorner 충돌에 쓰는 공통 타입 사운드 번호다.
    std::uint32_t cornerImpactType{136};
    // scaleX/scaleY: 원본 BoardCoord.add가 읽는 현재 타일별 픽셀 배율이다.
    float scaleX{16.0f},scaleY{11.0f};
    std::uint32_t rainCannonType{129},iceMissileType{130}; //10.78의 초기 아이스 발사자와 충돌/반사 표시 TYPE다. 기존 값형 초기화 순서는 유지한다.
};
// 실제 원본 월드 효과 경계다. 미복원 효과를 빈 성공으로 처리하지 않는다.
struct MissileHooks {
    // muzzle: 현재 발사 자산의 가상 +0xb4 포구 위치다.
    std::function<std::array<float,2>(Sid)> muzzle;
    // hotspot: 현재 SHP 프레임의 +0x20/+0x22 픽셀 좌표다.
    std::function<std::array<std::int32_t,2>(Sid)> hotspot;
    // rayEnd: 00481750/CD0044a960의 실제 사정선 끝점이다.
    std::function<std::array<float,2>(std::array<float,2>,std::array<float,2>,Sid)> rayEnd;
    // targetDimensions: 표적 SHP의 실제 타일 폭/높이이며 기본 자산 밖은3/3이다.
    std::function<std::array<std::int32_t,2>(Sid)> targetDimensions;
    // create: 실제 SquidFactory.Create(type,2) 자산 생성이다.
    std::function<Sid(std::uint32_t,std::uint32_t)> create;
    // directionFrame: 생성한 투사체 타입의 실제 방향 프레임을 선택한다.
    std::function<std::int32_t(std::array<float,2>,std::array<float,2>,std::uint32_t)> directionFrame;
    // owner: 실제 자산 가상 +0x74의 소유자 지정이다.
    std::function<void(Sid,std::uint8_t)> owner;
    // unpop/pop: 표시 자산의 실제 공간 이동이며 Pop에 좌표와 flags를 모두 전달한다.
    std::function<void(Sid,std::uint32_t)> unpop;
    std::function<void(Sid,float,float,std::uint32_t)> pop;
    // firingSound: 실제 타입 사운드 토큰을 지정 좌표에서 재생한다.
    std::function<void(float,float,std::uint32_t)> firingSound;
    // flightEffect: 타입11 등 실제 비행 효과 프로세스를 새 표시 자산에 붙인다.
    std::function<void(std::uint32_t,Sid)> flightEffect;
    // center: 현재 발사 건물의 실제 발자국 중심이다.
    std::function<std::array<float,2>(Sid)> center;
    // targetBounds: 기록한 표적 위치에서 실제 타입 발자국 사각형을 만든다.
    std::function<std::array<float,4>(Sid,std::array<float,2>)> targetBounds;
    // hit: 현재 총의004d4f00/CD004432f0 실제 직접 표적 피해를 처리한다.
    std::function<void(Sid,Sid)> hit;
    // hitEffect: 정상 표적 충돌의00470ac0/CD00455b40 실제 시각 효과다.
    std::function<void(float,float)> hitEffect;
    // queueCommand: 원본00452fe0에 충돌 토큰과 표시/표적 타입을 순서대로 넣는다.
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> queueCommand;
    // refresh: 현재 총의004d5000/CD004433e0 사거리 갱신이다.
    std::function<void(Sid)> refresh;
    // impact: incomingCorner 충돌 시 실제 효과0045a750/CD0047c8a0을 좌표에 생성한다.
    std::function<void(float,float,std::uint32_t,float)> impact;
    // impactSound: incomingCorner의 공통 타입 사운드가 존재할 때 실제 재생한다.
    std::function<void(float,float,std::uint32_t)> impactSound;
    // destroy: 표시 자산의 가상 Destroy(0)이며 부착한 Missile 프로세스도 같은 Host가 삭제한다.
    std::function<void(Sid,std::uint32_t)> destroy;
    std::function<std::int32_t(std::int32_t,std::int32_t)> randomInteger; //004558f0의 min포함/max제외 정수 난수이며 아이스 전용 음향이 소비한다.
    std::function<void(float,float,std::string_view)> impactName; //004a9d70의 실제 파일명 위치 음향이며 token 재생과 구별한다.
    std::function<void(const MissileFields&)> ricochet; //00482190의 현재 필드/표시를 사용하는 실제 반사 생성이다. 선택된 누락은 거부한다.
};
// 최초썬캐논및SunArcher 발사의실제숫자필드와표시자산수명/이동/충돌사슬이다.
class RawMissile {
public:
    // 전체 판본 타입 표와 보충 사운드/방향 데이터를 같은 저장소로 연결한다.
    RawMissile(SidPool& pool,std::span<const RiftTypeRecord> types,std::span<const MissileTypeData> missileTypes,MissileState& state,MissileHooks hooks);
    // 실제 ctor의 숫자·좌표·예약을 쓴 뒤 표시 자산 생성/소유자/Pop/비행 효과까지 수행한다.
    // returned SID는 실제 Missile 표시 자산이다. 이어지는 Host 부착은 factory가 완료한다.
    Sid Construct(MissileFields& fields,const GunProjectileRequest& request) const;
    // 썬캐논/SunArcher 최초incomingCorner의실제이동·swept충돌·범위종료를처리한다.
    // 반사/다른총/원작debug 좌표이동은아직명시미복원계약이다.
    void Frame(MissileFields& fields) const;
    // 동일한 월드인지 검증하는 adapter 자료다.
    const SidPool& Pool() const;
    std::uint32_t ProcessType() const;
private:
    // 지정한 SID와 raw 타입이 실제 살아 있는 입력 계약을 충족하는지 검사한다.
    std::span<const std::uint8_t> Slot(Sid sid) const;
    // 원본 frame의 패치 DWORD/CD BYTE 폭으로 표시 자산 프레임을 쓴다.
    void FrameNumber(Sid sid,std::uint32_t frame) const;
    // pool_: 실제 표시 자산/발사 건물/표적이 공유하는 저장소다.
    SidPool& pool_;
    // types_: 현재 원본 타입의 group/hotspot/발자국이다.
    std::span<const RiftTypeRecord> types_;
    // missileTypes_: 실제 타입 사운드 토큰/방향 수다.
    std::span<const MissileTypeData> missileTypes_;
    // state_: 현재 게임 시각/원본 타입 번호다.
    MissileState& state_;
    // hooks_: 실제 자산 생성/공간/비행 효과/피해/삭제 경계다.
    MissileHooks hooks_;
};
// 원본 실제 Missile process가 같은 Kernel과 ProcessForm 수명을 소유한다.
class Missile final : public SquidProcess {
public:
    // 생성된 raw 객체 필드와 frame 처리기의 수명을 연결한다.
    Missile(const RawMissile& logic,MissileFields fields);
    // Kernel 프레임과 부착 결과를 같은 raw 필드에 전달한다.
    void RunFrame() override;
    void OnAttached() override;
    // 독립 대조/수명 관찰을 위한 현재 실제 필드다.
    const MissileFields& Fields() const;
private:
    // logic_: 이 객체보다 오래 살아야 하는 실제 프레임 처리기다.
    const RawMissile& logic_;
    // fields_: 원본 판본별 raw 객체 이미지다.
    MissileFields fields_;
};
// 실제 SquidFactory 생성 몸체를 같은 풀의 Missile에 연결하고 나머지 효과 경계는 보존한다.
MissileHooks MakeMissileFactoryHooks(const SidPool& pool,SquidFactory& factory,MissileHooks hooks);
// 실제 자산 생성/Pop 뒤 원본 flags0x10으로 같은 Host/Kernel에 Missile을 붙인다.
Missile* CreateMissile(SquidProcessHost& host,const RawMissile& logic,const GunProjectileRequest& request);
// GunAnimation의 엄격한 createProjectile 경계를 실제 Missile factory로 연결한다.
GunAnimationHooks MakeGunMissileHooks(SquidProcessHost& host,const RawMissile& logic,GunAnimationHooks hooks);
}
