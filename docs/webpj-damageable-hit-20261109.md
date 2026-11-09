# webpj 일반 자산 피격(RawDamageableHit) 이식 — WB04-E210

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절 "총·피해·삭제 수명" 묶음의 첫 부분이다. 일반 자산이 총/투사체 피해를 받을 때 거치는 두 가상 피격 몸체(BaseHit 004add10·Damageable Hit 0044c8e0)를 옮겼다. 사제 피격(E67)과 같은 레벨 보정·HP setter·burning 판정을 공유한다.

- cpppj 기준선 `1f00a25a`. 소스·시험·fixture 모두 기준선 뒤 변경이 없다(`git diff --numstat 1f00a25a HEAD` 0줄).
- 근거 커밋: 소스 `c3cfc98f`(헤더 `8b467ca0`), 시험·fixture `5b10ce82`.
- 원본 fixture: `damageablehit-x86.tsv` originals 2923행(SHA-256 `3d11d4352baddf6f…`). originalCD 2923행(10.72)은 범위 밖이다.
- 공유 장치
  - E67 `raw-priest-damage.js`: `PriestDamageState`(무적·보정 차단·레벨)와 `RawPriestDamage.adjust`(레벨 피해 보정)
  - E67 `raw-damageable-burning.js`: `RawDamageableBurning.refresh`(HP 절반 미만 burning 생성/삭제)
  - E2 `raw-squid-word.js`: `setRawSquidHitPoints`(HP DWORD setter)
  - D5b `squid-reward.js`: `SquidReward`(TYPE 최대 HP, burning 판정이 읽음)
- 새 공유 장치는 만들지 않았다.

## 1. 옮긴 것

`www/core/raw-damageable-hit.js`(DOM 없음, 10.78 전용)

- `DamageableHitState(levels)`: levels 는 사제 피격과 공유하는 `PriestDamageState` 다. 그 밖의 전역은 다음과 같다.
  - 사망 권한 server, held 진단 checking(전체 DWORD, `sharedChecking` getter)
  - local 소유자, 주 섬 번호 40칸
  - 경고 마감(double)과 간격(float 30초)
  - 서버 사망 직전 기록 두 DWORD(killedVictim·killerOwner)
  - 현재 저장소 getter(serverRead·localOwnerRead·primaryIslandRead)
- `RawDamageableHit(pool, types, state, hooks)`
  - `baseHit(victim, damage, attackerOwner, attacker)` — 004add10
    1. dead 면 원본 assert, free 면 참이다.
    2. extra(+40) 비트 0/3(held)이면 checking 일 때 assert, 아니면 참이다.
    3. 무적이면 피해 0 이다. 가상 방어(+0x9c)가 거부하면 참이다.
    4. 현재 소유자 기준 레벨 보정 피해를 현재 signed DWORD HP 에서 감김 뺄셈한다.
    5. 결과가 1 미만이고 서버면: 사망 전역을 기록하고 destroy(공격자 소유자<<16 | 0x100000)를 부른다. 공격자 TYPE flags2 에 0x100(유닛)이 있으면 | 0x4100000 이다. 거짓을 반환한다.
    6. 아니면 HP 를 최소 1 로 쓰고, 현재 소유자에게 통지한 뒤 참이다.
  - `hit(victim, damage, attackerOwner, attacker)` — 0044c8e0
    1. HP 보유가 아니면 거짓이다. 다이스 genus(flags2 0x400000)는 extra 비트 0, 그 밖은 TYPE flags1 0x10 으로 판정한다.
    2. local 소유자의 주 섬 피격 경고를 처리한다(아래).
    3. extra 비트 2 면 unrealize(가상+0x88)를 부른다.
    4. baseHit 를 부르고, 살아 있으면 burning 을 갱신한다.
  - 섬 피격 경고
    1. 피해자 소유자가 local 이 아니면 생략한다.
    2. 현재 좌표의 지면 SID 를 조회한다. 지면이 섬 genus(2)이고 3×3 섬(0x1000000)이 아닐 때만 계속한다.
    3. 섬 번호는 다리 genus(4)면 −1, 아니면 지면 +8 WORD 다. 소유자의 주 섬과 같을 때만 계속한다.
    4. 경고 마감 ≤ 실제 시각이면 event 12 → islandUnderAttack.wav → islandUnderAttack 알림 순서로 낸다.
    5. 실제 시각을 다시 읽어 마감 = 시각 + float 간격으로 저장한다(실제 시각 호출 두 번).
- `makeDamageableGunHitHooks(damage, entries, hooks)`: 현재 raw vtable 이 entries 의 실제 +0x98 진입과 같을 때 분배한다. 0x44c8e0 이면 hit, 0x4add10 이면 baseHit 이다. 그 밖은 기존 `hooks.damage` 로 넘기고, 없으면 오류다.
- `makeDamageableHitBurningHooks(pool, burning, hooks)`: 같은 풀의 burning 판정을 refresh 에 연결한다. 다른 풀이면 거부한다.
- CD 10.72 분기(signed WORD HP·다른 오프셋)는 옮기지 않았다.

## 2. 검증

`tests/damageable-hit.test.mjs`(5건)

- 원본 2923행(cpppj `DamageableHit_PatchNativeBaseAndDerived`): cpppj HitScene 과 같은 장면이다.
  - SID 5..70 을 실제 할당하고, 피해자 50 의 raw 를 독립 패턴으로 채운다. 공격자 60·지면 70 을 둔다.
  - 실제 `SquidReward`·`RawDamageableBurning` 위에 관찰 hooks 를 단다.
  - 대조 칸: 반환·효과 순서·피해자 50바이트·사망 전역 8바이트·경고 마감 double 8바이트.
- 계약
  - `MissingDefenseAndDeleteRejected`: 가상 방어·삭제 누락은 오류이며 사망 전역을 쓰지 않는다.
  - `CurrentVtableDispatchAndFallback`(10.78 판만): 현재 vtable 의 BaseHit 진입 분배(HP 150→126)와 다른 vtable 의 fallback.
  - 웹 추가: 섬+다리 genus(6) 지면의 번호 −1 분기. 판본/TYPE 표/빈 분배 표/다른 풀 burning 거부.

최초 실패와 교정: 없다. 모듈과 원본 대조 2923행 모두 첫 실행에 통과했다. 비교가 실제로 작동하는지 보려고 사본을 다음과 같이 바꿔 돌렸다. 확인 뒤 모두 원본으로 되돌렸다(cmp 일치).

- 원본 대조가 실패한 변형: 유닛 공격 사망 flags 0x4100000→0x4000000, held 마스크 9→8, 경고 간격 30→30.5, 경고 사건/소리 순서, 다이스 HP 보유 비트 0→1.
- 원본 대조가 통과한 변형: 다리 genus 섬 번호 −1→0. fixture 의 다리 행(genus 4)은 섬 비트(2)가 없어 번호 비교 전에 반환하므로 구별되지 않는다. 그래서 섬+다리 genus(6) 계약 검사를 추가했다. 그 변형이 이 검사에서 실패하는 것을 확인했다.

## 3. 경계와 남은 것

- 실제 호출자 연결은 하지 않았다. Gun/Missile 피해 분배(`makeMissileGunHitHooks`·`makePriestGunHitHooks` 와의 연결 순서), 캠페인 피해 vtable 표(CampaignDamageVtables), 지면 조회·실제 시계·경고 UI·소유자 통지의 World/Client 연결은 해당 단위와 순서4~5 몫이다(PORT_MAP session 0).
- 섬+다리 genus(6)의 번호 −1 분기는 cpppj 원문 계약으로만 확인했다. 원본 관측은 없다.
- CD 10.72 는 범위 밖이다.

## 4. 다음 시작점

WB04-E211 은 3.1절에서 고른다. 같은 묶음에서 기준선 뒤 변경이 없는 RawGunGroundTarget(`GunGroundTargetTests`, 손 시험)·RawForcefieldLifecycle(`ForcefieldLifecycleTests`)을 먼저 권한다. RawDamageablePreDestroy·RawDamageablePostDestroy·RawDamageableRelease·RawGunImpact·RawDiscRegular 는 기준선 뒤 변경이 있어 옮길 때 3.6절 판정을 함께 한다. "비행·이동 유닛" 묶음의 RawFlyerFrame/RawFlyerTarget 도 선행 RawFlyerProcess 에 기준선 뒤 변경이 있어 판정이 먼저다.
