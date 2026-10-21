# 원본 월드 코어 W03 여덟째 조각 — 일반 피해(Damageable Hit·BaseHit)

2026-10-21, 작업 PC **VM-W11-Codex**. [이식 계획](dotnet-cpp-port-plan-20261016.md) 5절 **W03(원본 월드 코어 ② 삭제·수명)** 의 여덟째 조각이다.
여섯째·일곱째 조각이 피해가능객체의 **사전·사후 파괴**를 이식했으므로, 이번 조각은 그 사이의 **피격** 경로를 이었다.
피격 몸체가 필요로 하는 **HP 쓰기**·**레벨 피해 보정**·**burning 판정** 몸체도 함께 옮겼다.
근거는 커밋된 cpppj 소스(`RawDamageableHit.cpp`·`RawSquidHitPoints.cpp`·`RawPriestDamage.cpp`·`RawDamageableBurning.cpp`)와
원본 fixture(`damageablehit-x86.tsv`)뿐이다.
새 디컴파일·원본 관찰·원본/클론 게임·창 실행은 없다. `cpppj/`·공통 `LEFT_JOBS.md` 는 수정하지 않았다.
이 조각은 **L1 모듈 단계**이며 L2 이상으로 합산하지 않는다. 실제 플레이는 달라지지 않는다.

## 1. 구현

| 파일 | 내용 | 원본 근거 / cpppj 근거 |
| --- | --- | --- |
| `RawDamageableHit.cs` (신규) | `DamageableHitState`(레벨·서버 권한·주 섬·경고 시각·사망 기록), `DamageableHitHooks`(방어 판정·삭제·소유자 통지·burning·unrealize·섬 조회·실제 시계·경고 알림), `RawDamageableHit`(`BaseHit`·`Hit`) | `RawDamageableHit.cpp` 의 방어·레벨·HP·서버 사망 또는 client HP1·소유자 통지 순서 |
| `RawDamageableBurning.cs` (신규) | `DamageableBurningState`(검사·process 14·bulf 135), `DamageableBurningHooks`(find·create·destroy·bulf 소리), `RawDamageableBurning`(`Refresh`) | `RawDamageableBurning.cpp` 의 free/피격 효과 0 자연 반환·저 HP 생성·기존 유지·회복 후 삭제 |
| `RawPriestDamage.cs` (신규) | `PriestDamageState`(NoDamage·권한·레벨 보정·소유자별 레벨), `RawPriestDamage.Adjust`(레벨 피해 보정) | `RawPriestDamage.cpp` 의 `Adjust`(패치 레벨 차 +20 / CD +8, Challenge rank 표) |
| `RawSquidHitPoints.cs` (신규) | `RawSquidHitPoints.Set`(26 번 자리에 signed 4바이트 HP 쓰기) | `RawSquidHitPoints.cpp` 의 004adca0 |
| `OriginalTypeRecord.cs` (확장) | 타입 필드 `DamageEffect`(원본 +0xd4) | `RiftTypeRecord.damageEffect`. 0 이면 타격/화상 효과가 없고 burning 판정도 건너뛴다 |
| `X86RawDamageableHitTests.cs` (신규) | 원본 10.78 몫 2,923행 재생 1건 | `cpppj/tests/DamageableHitTests.cpp` 의 `HitScene`·`Burning`·`Hooks`·`Run` 열과 같다 |
| `RawDamageableHitTests.cs` (신규) | 합성 5건(미연결 효과·죽음/abstract 표적·잘못된 번호·burning 경계·HP 쓰기 폭) | `DamageableHitTests.cpp` 의 `DamageableHit_MissingDefenseAndDeleteRejected` 와 같은 구성 |

### 순서와 규칙

`RawDamageableHit.Hit(victim, damage, attackerOwner, attacker)` 는 원본 순서대로 다음을 실행한다.

1. 표적의 HP 보유 여부를 확인한다(제단은 extra bit 0, 그 밖은 타입 플래그1의 `0x10`). 없으면 거짓을 돌려준다.
2. **섬 경고**: 표적 소유자가 현재 로컬 소유자와 같고, 그 좌표의 지면이 1×1 섬이며, 그 섬 번호가 현재 주 섬 번호와 같을 때만
   경고 시각이 지났으면 경고 이벤트·소리·알림을 낸다. 경고 시각은 실제 시계 + 경고 간격(30초)으로 갱신한다.
   실제 시계는 비교와 갱신에서 두 번 읽는다.
3. 표적 extra bit 4(0x4)가 켜져 있으면 unrealize 를 부른다.
4. **BaseHit**: 죽음 assert → free/dead 자연 반환 → abstract·매림 자연 반환(진단 전역이 켜졌으면 assert) →
   무적 전역이면 피해 0 → 가상 방어 판정 → 레벨 피해 보정 → HP 차감 → 서버이고 HP 가 0 이하이면 삭제,
   아니면 HP 1 로 보존하고 소유자 통지를 낸다.
5. 살아 있으면 burning 갱신(burning 판정)을 부른다.

서버 삭제 플래그는 `공격자 소유자 << 16 | 0x100000` 이고, 공격자 타입 플래그2의 `0x100`(유닛 공격)이 켜져 있으면
`공격자 소유자 << 16 | 0x4100000` 이 된다.

레벨 피해 보정은 소유자별 레벨이 다를 때만 적용한다. 피해자 레벨이 더 낮으면
`(공격자 - 피해자 - 20) × 피해 / 20`, 더 높으면 `피해 × 20 / (피해자 - 공격자 - 20)` 이다.

burning 판정은 피격 효과가 0 이면 자연 반환하고, 현재 HP 가 최대 HP 의 절반(최대 × 2 ÷ 4)보다 작으면
기존 burning Form 을 유지하거나 새로 만들고, 그렇지 않으면 기존을 삭제한다.

## 2. 검증 결과

전체 Release 경고 0·오류 0(`--no-incremental` 강제 전체 빌드), 단위 **1,268개 통과**(Assets 363 + Core 905; 이번 +6).

| 검사 | 내용 |
| --- | --- |
| `damageablehit` 10.78 몫 2,923행 재생 | 입력 1 이 0 이면 BaseHit, 1 이면 Hit 경로로 실행한다. 매 행마다 생존 여부·외부 경계 순서·피해자 슬롯 50바이트·사망 기록 8바이트·경고 시각 8바이트가 일치한다 |
| 합성 5건 | 미연결 방어 판정·미연결 삭제 경계·죽음 assert·abstract 자연 반환·held assert·할당되지 않은 번호·타입 표 밖 번호·burning 조회/생성 경계·HP 쓰기 폭을 확인한다 |

`damageablehit-x86.tsv` 는 총 5,846행(2 판본 × BaseHit 1,160 + Hit 1,763)이며 10.78 몫 2,923행만 재생했다.
`originalCD` 행은 읽지 않는다(기준 판본은 10.78 뿐이다).

10.78 몫에서 실제로 잡히는 경계는 방어 판정(`R:`) 619행·소유자 통지(`N:`) 277행·서버 삭제(`D:`) 54행·
burning 조회(`B:`) 344행·burning 생성(`A:`) 120행·unrealize(`U:`) 144행·burning 삭제(`X:`) 52행·
섬 조회(`I:`) 18행·실제 시계(`W:`) 3행·경고 알림(`V:`/`S:`/`F:`/`L:`) 각 2행이다.
살아남은 결과 1,861행·사망 1,062행이고 사망 기록이 남는 행이 54행이다.

**변이 확인:** 구현 9곳을 하나씩 망가뜨려 8곳이 실패로 잡혔다.

| 변이 | 잡힌 검사 |
| --- | --- |
| 서버 삭제 플래그의 소유자 마스크 제거 | 원본 재생 |
| 공격자 유닛 공격 플래그(`0x100`) 검사 제거 | 원본 재생 |
| HP 1 보존(클램프) 제거 | 원본 재생 |
| 레벨 피해 보정 기준값 20 → 10 | 원본 재생 |
| 섬 경고 시각 비교 제거 | 원본 재생 |
| 경고 간격 30초 → 1초 | 원본 재생 |
| burning 저 HP 기준(최대 × 2 ÷ 4) → ÷ 2 | 원본 재생 |
| abstract(extra bit 1) 검사 제거 | 원본 재생 |
| 가상 방어·삭제 경계 누락 시 정상 완료로 바꾸기 | 합성 |

소스는 모두 원복해 같은 검사가 다시 통과함을 확인했다.

**잡히지 않은 1곳과 이유**

- 사망 판정 경계 `next < 1` 을 `next < 0` 으로 바꾸어도 결과가 같다. 이 fixture 에는
  HP 와 피해가 정확히 같아 남은 HP 가 0 이 되는 입력(HP 1·피해 24 처럼 음수가 되는 입력만 있다)이 없다.

## 3. 이번에 하지 않은 것 (경계)

- **사제(Priest) 피격 몸체는 옮기지 않았다.** `RawPriestDamage` 에서는 레벨 피해 보정과 전역만 옮겼고,
  사제 고유 피격(회복 취소·HP1 감시 해제·DependForm 감시)은 다음 조각에서 옮긴다.
- `MakeDamageableGunHitHooks`(가상 표 +0x98 진입 주소로 Base/Damageable 을 분배)와
  `MakePriestGunHitHooks` 는 **분배 어댑터**라 아직 옮기지 않았다. 파생 분배 허브 조각에서 옮긴다.
- CD 판의 지면 조회 보정(`RawPathSurfaceAt`)은 10.78 경로에서 쓰이지 않아 옮기지 않았다.
- 종속 조회(`contained`)·내용물 수명, Form 수명, 신전 삭제, 삭제 보상(SP·지갑·샘 풀) 계산은 아직 옮기지 않았다.
- CD 10.72 몫, 파생 분배 허브·실제 세션 연결. 이 조각은 L1 모듈 단계이며 L2 이상으로 합산하지 않는다.

## 4. 다음 조각

사제 피격(`RawPriestDamage.Hit`) → 종속 조회·내용물(`contained`·`contentlifetime`) →
Form 수명(`formlifecycle`·`formtake`·`form-packet`) → 신전 삭제 → 삭제 보상(SP·지갑·샘 풀) 계산 →
파생 분배 허브·실제 세션 연결.
시작할 때 `python dotnetpj/tools/cpp_sync.py` 를 먼저 실행하고 쓸 fixture 를 `--update --add` 로 등록한다.
