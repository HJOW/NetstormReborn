// 사제·보호막·Carrier의 복원된 몸체를 실제 월드의 풀/공간/프로세스에 연결한다.
#pragma once
#include "o/RawCarrierCheck.h"
#include "o/RawDamageablePreDestroy.h"
#include "o/RawForcefieldLifecycle.h"
#include "o/RawPriestFall.h"
#include "o/RawPriestPlacement.h"
#include "o/RawPriestPostPopTail.h"
#include "o/RawPriestRegen.h"
#include <memory>

namespace netstorm::client {
// 00488fe0/CD 0047d020의 현재 타입/프로세스 전역이다. 초기 번호는 두 보호 PE의 DWORD와 같다.
// gameplay에서 전역을 바꿔도 다음 조회가 이 상태를 다시 읽는다.
struct RawMovementProcessTypes {
    // 자산 타입 전역: sunWalker/빈 타입/windWalker/rainWalker/teleportEffect/bulf/dude/사제/manabolt 순서다.
    std::uint32_t sunWalker{133},empty{187},windWalker{137},rainWalker{156},teleport{134},bulf{135},dude{70},priest{o::kPriestType},manaBolt{105};
    // 내장 프로세스 타입 전역: worm/simpleWalker/balloon/teleport/bulf/dude/priest/simpleFly 순서다.
    std::uint32_t wormProcess{40},walkerProcess{38},balloonProcess{42},teleportProcess{23},bulfProcess{41},
        dudeProcess{24},priestProcess{54},flyProcess{39};
};
// 00488fe0/CD0047d020의 기존 몸체를 공유한다. PathStart 훅과 사제 월드 바인딩이 같은 mutable 선택 상태를 읽는다.
std::uint32_t SelectMovementProcessType(o::OriginalEdition edition,std::span<const o::RiftTypeRecord> types,
    const RawMovementProcessTypes& selection,std::uint32_t type);
// 다른 자산도 사용하는 단일 월드 자료다. 모든 참조는 연결 객체보다 오래 살아야 한다.
struct RawPriestWorldCore {
    // pool: 객체 슬롯의 생성·조회·반납에 사용하는 SID 풀.
    o::SidPool& pool;
    // hash: 좌표별 객체 후보를 찾는 공간 해시.
    const o::SquidHash& hash;
    // types: 타입 번호로 속성·플래그를 조회하는 타입 표.
    std::span<const o::RiftTypeRecord> types;
    // frames: 타입별 프레임 자료.
    std::span<const o::RiftTypeFrames> frames;
    // spots: 칸별 점유·통행 비트를 읽는 spot 지도.
    std::span<const std::uint8_t> spots;
    std::span<const std::uint16_t> surfaces; // 반드시 hash.Entries(0) 자체다. 지도 사본은 생성 시 거부한다.
    // reward: HP·SP·지갑 보상을 실제 장부에 적용하는 처리기.
    const o::SquidReward& reward;
    // random: 원본 FUN_004558c0: 상태 0이면 0x0bad0bad로 바꾼 뒤 상태*0x10003+3의 상위 16비트를 limit로 나눈 나머지를 쓴다 (o::GameRandom).
    o::GameRandom& random;
    // factory: 타입에 맞는 객체를 생성하는 처리기.
    o::SquidFactory& factory;
    // owner: 소유 플레이어 변경을 객체 필드와 장부에 반영하는 처리기.
    o::SquidOwner& owner;
    // ownerMode: 판본·서버·로딩 상태에 따른 소유권 처리 정책.
    const o::SquidOwnerMode& ownerMode;
    // postPop: 객체 Pop 이후 목록·공간 후처리를 수행하는 처리기.
    o::SquidPostPop& postPop;
    // frame: 프레임 변경을 물리 프레임·통지·갱신 요청에 반영하는 처리기.
    const o::SquidFrame& frame;
};
// 실제 전역을 보관하는 공유 상태다. 사제 목록 저장소 크기는 월드 초기화에서 명시 설정한다.
// MissionOutcome/조회도 priests를 직접 공유하며 별도 사제 복사 목록을 만들지 않는다.
struct RawPriestWorldState {
    // priests: 사제.
    o::PriestPostPopState priests;
    // owner: 소유 플레이어 번호 또는 소유권 처리 자료.
    o::PriestOwnerState owner;
    // hitPoints: 현재 HP 또는 HP 처리 자료.
    o::PriestHitPointMode hitPoints;
    // shield: 사제 보호막의 생성·통지·상태 처리 자료.
    o::PriestShieldState shield;
    // regen: 사제 HP 회복과 SP 갱신을 처리하는 자료.
    o::PriestRegenState regen;
    // fall: 객체 추락 상태와 프레임 진행 처리 자료.
    o::PriestFallState fall;
    // forcefield: 포스필드.
    o::PriestForcefieldState forcefield;
    // spawn: 사제의 생성·배치·등록 처리 자료.
    o::PriestSpawnState spawn;
    // carrier: 캐리어의 내용물·통행·수명 처리 자료.
    o::CarrierPostPopState carrier;
    // contained: 객체 내용물의 생성·검색·등록 처리 자료.
    o::CarrierPreDestroyState contained;
    // forcefieldRegular: 사제 타입 DWORD는 매 검사 때 읽는다. 소유자는 존재 검사의 조건이 아니다 (o::ForcefieldRegularState).
    o::ForcefieldRegularState forcefieldRegular;
    // movement: 00488fe0/CD 0047d020의 현재 타입/프로세스 전역이다. 초기 번호는 두 보호 PE의 DWORD와 같다 (RawMovementProcessTypes).
    RawMovementProcessTypes movement;
};
// 공간 효과는 월드의 실제 가상 Pop/Unpop/Destroy를 받는다. 계산을 임의 기본값으로 대체하지 않는다.
// 소리·안내 및 아직 별도 복원이 필요한 regen 하위 효과는 호출자가 명시적으로 공급한다.
struct RawPriestWorldEffects {
    // pop: 대상을 지정 위치의 공간에 등록하는 Pop 콜백.
    std::function<void(o::Sid,float,float,std::uint32_t)> pop;
    // unpop: 대상을 공간 등록에서 해제하는 Unpop 콜백.
    // repop: 변경한 대상을 공간에 다시 반영하는 Repop 콜백.
    // destroy: 대상의 실제 삭제 경로를 실행하는 콜백.
    std::function<void(o::Sid,std::uint32_t)> unpop,repop,destroy;
    // claimGround: 대상의 지면 소유권 변경을 실행하는 콜백.
    std::function<void(float,float,std::uint8_t)> claimGround;
    // ownerNotice: 소유자 변경을 실제 통지 경로에 전달하는 콜백.
    // notifySurface: 표면 객체에 변경을 통지하는 콜백.
    std::function<void(o::Sid)> ownerNotice,notifySurface;
    // dependentFound: 찾은 종속 객체를 실제 처리 경로에 전달하는 콜백.
    std::function<void()> dependentFound;
    // reserveRegular: 후속 Regular 프로세스 등록을 예약하는 콜백.
    std::function<bool()> reserveRegular;
    // shield: 사제 보호막의 생성·통지·상태 처리 자료.
    o::PriestShieldHooks shield;
    // fallSound: 추락 위치에서 해당 소리를 재생하는 콜백.
    std::function<void(o::Sid,float,float)> fallSound;
    // regen: 사제 HP 회복과 SP 갱신을 처리하는 자료.
    o::PriestRegenHooks regen;
    // otherWalkerFall: 사제 이외 이동 유닛의 실제 추락 경로를 호출하는 콜백.
    std::function<bool(o::Sid)> otherWalkerFall;
};
// 사용 순서: 생성→ProcessHost(RegularHandler/DestroyHooks) 생성→BindProcesses→후속 모듈 Bind다.
// 로딩 Pop 전에 BindProcesses가 필요하며, 삭제/해방 전에는 Damageable/Placement도 연결해야 한다.
class RawPriestWorldBindings {
public:
    // 같은 풀의 실제 생성/소유자/프레임/공통 postPop을 확인하고 사제·보호막 후처리를 등록한다.
    RawPriestWorldBindings(RawPriestWorldCore core,RawPriestWorldState& state,RawPriestWorldEffects effects);
    // postPop 등록을 해제한다. 외부 Kernel은 이 객체를 해제하기 전에 비워야 한다.
    ~RawPriestWorldBindings();
    RawPriestWorldBindings(const RawPriestWorldBindings&)=delete;
    RawPriestWorldBindings& operator=(const RawPriestWorldBindings&)=delete;
    // 생성의 순환 참조를 풀어 실제 Regular/Form/Kernel 예약 경로를 연결한다. 재연결은 거부한다.
    void BindProcesses(o::SquidProcessHost& host);
    // 사제 삭제의 Carrier→Damageable 실제 몸체를 연결한다. 목록/보호막 변경 전에 Validate를 실행한다.
    void BindDamageable(const o::RawDamageablePreDestroy& damageable);
    // 해방의 132개 나선 후보마다 실제 사제 배치 검사를 연결한다.
    void BindPlacement(const o::RawPriestPlacement& placement);
    // 처음 전투 틱/명령을 허용하기 전에 모든 실제 하위 연결을 확인한다.
    void ValidateReady() const;
    // 로딩 깊이를 읽는 세 몸체를 동일 값으로 갱신한다. ownerMode는 core의 공유 모드를 읽는다.
    void SetLoadingDepth(std::uint32_t depth);
    // 회복/낙하/Carrier/종속의 실제 권한 전역을 같은 값으로 갱신한다.
    void SetAuthority(bool authority);
    // 로컬 소유자에 대한 원본 owner 표식과 보호막 안내 조건을 함께 갱신한다.
    void SetLocalPlayer(std::uint32_t player);
    // 사제는 원래 소유자 규칙, 다른 자산은 공통 실제 소유자 지정으로 분배한다.
    void SetOwner(o::Sid sid,std::uint32_t player) const;
    // 실제 HP/spot/extra에 따른 경계 전환을 같은 월드의 Unpop/Repop에 연결한다.
    void SetHitPoints(o::Sid sid,std::int32_t value) const;
    // Carrier의 +0xcc 지면 검사와 사제 +0xc8 낙하 몸체를 사용한다.
    bool CheckCarrier(o::Sid sid) const;
    bool FallWalker(o::Sid sid) const;
    // 현재 자산 타입/balloon genus/판본 우선순위로 실제 이동 프로세스 번호를 선택한다.
    std::uint32_t MovementProcessType(std::uint32_t type) const;
    // 0044c060/CD 00462480의 free/damageEffect 0 반환을 실행하고 nonzero 전체 burning 경계를 요청한다.
    void DamageableRefresh(o::Sid sid) const;
    // 사제 0x25a/0x25b와 보호막 사건 1/2를 실제 타입으로 분배한다. 기타 사건만 fallback에 넘긴다.
    o::RegularHandler RegularHandler(o::RegularHandler fallback={}) const;
    // 사제 preDestroy만 실제 목록/보호막/Carrier로 분배하며 나머지 공통/폼 훅은 보존한다.
    o::SquidDestroyHooks DestroyHooks(o::SquidDestroyHooks fallback) const;
    // Damageable 해방 모듈이 같은 실제 나선 생성 몸체에 연결할 때 사용한다.
    const o::RawPriestSpawn& Spawn() const;
    // 사제 조회·선택·움직임 및 MissionOutcome이 읽는 단일 상태/목록을 제공한다.
    const o::RawPriestState& PriestState() const;
    const o::RawPriestForcefield& Forcefield() const;
    o::PriestPostPopState& Priests() const;
private:
    struct Impl;
    // impl_: 공개 인터페이스 뒤의 구현 상태와 연결 객체.
    std::unique_ptr<Impl> impl_;
};
}
