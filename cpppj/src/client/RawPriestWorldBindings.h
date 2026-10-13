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
// 다른 자산도 사용하는 단일 월드 자료다. 모든 참조는 연결 객체보다 오래 살아야 한다.
struct RawPriestWorldCore {
    o::SidPool& pool;
    const o::SquidHash& hash;
    std::span<const o::RiftTypeRecord> types;
    std::span<const o::RiftTypeFrames> frames;
    std::span<const std::uint8_t> spots;
    std::span<const std::uint16_t> surfaces; // 반드시 hash.Entries(0) 자체다. 지도 사본은 생성 시 거부한다.
    const o::SquidReward& reward;
    o::GameRandom& random;
    o::SquidFactory& factory;
    o::SquidOwner& owner;
    const o::SquidOwnerMode& ownerMode;
    o::SquidPostPop& postPop;
    const o::SquidFrame& frame;
};
// 실제 전역을 보관하는 공유 상태다. 사제 목록 저장소 크기는 월드 초기화에서 명시 설정한다.
// MissionOutcome/조회도 priests를 직접 공유하며 별도 사제 복사 목록을 만들지 않는다.
struct RawPriestWorldState {
    o::PriestPostPopState priests;
    o::PriestOwnerState owner;
    o::PriestHitPointMode hitPoints;
    o::PriestShieldState shield;
    o::PriestRegenState regen;
    o::PriestFallState fall;
    o::PriestForcefieldState forcefield;
    o::PriestSpawnState spawn;
    o::CarrierPostPopState carrier;
    o::CarrierPreDestroyState contained;
    o::ForcefieldRegularState forcefieldRegular;
    RawMovementProcessTypes movement;
};
// 공간 효과는 월드의 실제 가상 Pop/Unpop/Destroy를 받는다. 계산을 임의 기본값으로 대체하지 않는다.
// 소리·안내 및 아직 별도 복원이 필요한 regen 하위 효과는 호출자가 명시적으로 공급한다.
struct RawPriestWorldEffects {
    std::function<void(o::Sid,float,float,std::uint32_t)> pop;
    std::function<void(o::Sid,std::uint32_t)> unpop,repop,destroy;
    std::function<void(float,float,std::uint8_t)> claimGround;
    std::function<void(o::Sid)> ownerNotice,notifySurface;
    std::function<void()> dependentFound;
    std::function<bool()> reserveRegular;
    o::PriestShieldHooks shield;
    std::function<void(o::Sid,float,float)> fallSound;
    o::PriestRegenHooks regen;
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
    std::unique_ptr<Impl> impl_;
};
}
