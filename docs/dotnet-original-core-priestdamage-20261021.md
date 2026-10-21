# 원본 월드 코어 W03 아홉째 조각 — 사제 피격(Priest Hit)과 HP 변경

2026-10-21, 작업 PC **VM-W11-Codex**. [이식 계획](dotnet-cpp-port-plan-20261016.md) 5절 **W03(원본 월드 코어 ② 삭제·수명)** 의 아홉째 조각이다.
여덟째 조각의 일반 피해가 **일반 자산**의 HP 쓰기를 다뤘으므로, 이번 조각은 사제(Priest) 고유 경로를 이었다.
사제 HP 변경(공간 해제·등록·거의 올림 좌표)과 HP 1 감시 쌍 끊기까지 포함한 몸체다.
근거는 커밋된 cpppj 소스(`RawPriestDamage.cpp`·`RawPriestState.cpp`·`RawPathAnimation.cpp`)와 원본 fixture(`priestdamage-x86.tsv`)뿐이다.
새 디컴파일·원본 관찰·원본/클론 게임·창 실행은 없다. `cpppj/`·공통 `LEFT_JOBS.md` 는 수정하지 않았다.
이 조각은 **L1 모듈 단계**이며 L2 이상으로 합산하지 않는다. 실제 플레이는 달라지지 않는다.

## 1. 구현

| 파일 | 내용 | 원본 근거 / cpppj 근거 |
| --- | --- | --- |
| `RawPriestDamage.cs` (확장) | `PriestDamageState`·`PriestDamageHooks`·`IPriestDependentHost` 추가, `RawPriestDamage` 를 인스턴스 몸체로 확장(`Hit`) | `RawPriestDamage.cpp` 의 회복 취소·프레임 복귀·HP 1 제한·소유자 통지·HP 1 감시 해제 순서 |
| `RawPriestState.cs` (신규) | `PriestHitPointMode`·`PriestHitPointHooks`·`RawPriestState`(`CurrentHitPoints`·`GroundImmobile`·`Immobile`·`SetHitPoints`) | `RawPriestState.cpp` 의 00492090 HP 변경·00427030 이동 불가 판정 |
| `RawPathAnimation.cs` (신규) | `Side`·`Direction`(프레임 코드의 방향 문자를 'A' 기준 번호로) | `RawPathAnimation.cpp` 의 방향 판정(사제 held 방향 10~14 판정에 쓰인다) |
| `X86RawPriestDamageTests.cs` (신규) | 원본 10.78 몫 1,712행 재생 1건 | `cppjp/tests/PriestDamageTests.cpp` 의 `DamageScene`·`BurningHooks`·`PriestHooks`·`Run` 열과 같다 |
| `RawPriestDamageTests.cs` (신규) | 합성 5건(회복 금지·권한 없음·가상 표 오류·공간 효과 누락·감시 쌍 연결) | `PriestDamageTests.cpp` 의 `PriestDamage_MissingNonzeroBurningRejected` 와 같은 구성 |

### 순서와 규칙

`RawPriestDamage.Hit(victim, damage, attackerOwner, attacker)` 는 원본 순서대로 다음을 실행한다.

1. 죽음 비트(패치판)는 assert 경로이고, free 슬롯·타입 범위 밖은 거부한다.
2. 무적 전역이면 피해 0 으로 만든다.
3. 회복 Regular(0x25d) 이 있으면 회복을 취소한다. 없으면 현재 프레임의 방향 번호가 10~14 면 취소한다.
4. 취소하면 Regular 0x25c·0x25d 를 없애고 기본 프레임으로 되돌린다.
5. 레벨 피해 보정을 적용한다(공격자·피해자 소유자별 레벨 차 기준).
6. HP 를 차감해 1 로 보존한 뒤 HP setter 로 쓴다.
7. 소유자 통지와 burning 갱신을 부른다.
8. 권한이 있고 HP 가 1 이면 감시 쌍을 끊고 거짓을 돌려준다.

`RawPriestState.SetHitPoints` 는 HP 를 쓴 뒤 **절반 경계 전환**이 있을 때만 공간 해제·등록을 수행한다.
권한이 없으면 경계값(`half - (wasLow ? 1 : 0)`)으로 되돌리고, 저 HP 에서 회복이 막혔으면 원래 HP 를 되살린다.
저 HP 로 남은 경우에만 현재 좌표에 HP 전환 전용 배율 `0.99999` 를 더해 거의 올림한 값을 다시 쓴다.

## 2. 검증 결과

전체 Release 경고 0·오류 0(`--no-incremental` 강제 전체 빌드), 단위 **1,274개 통과**(Assets 363 + Core 911; 이번 +6).

| 검사 | 내용 |
| --- | --- |
| `priestdamage` 10.78 몫 1,712행 재생 | 입력 0 은 레벨 보정만(336행), 1 은 사제 Hit(1,152행), 2 는 burning 판정만(224행) 실행한다. 매 행마다 결과·외부 경계 순서·사제 슬롯 50바이트·보정 버퍼 4바이트가 일치한다 |
| 합성 5건 | 회복 금지·권한 없음·가상 표 오류·공간 효과 누락·사제 죽음/free·필수 하위 효과 누락·감시 쌍 연결 누락을 실행 전에 거부한다 |

`priestdamage-x86.tsv` 는 총 3,424행(2 판본 × 3 몸체)이며 10.78 몫 1,712행만 재생했다.
`originalCD` 행은 읽지 않는다(기준 판본은 10.78 뿐이다).

10.78 몫에서 실제로 잡히는 경계는 소유자 통지(`N:`) 1,152행·회복 취소(`C:`) 576행·프레임 복귀(`F:`) 576행·
공간 해제(`U:`) 90행·공간 등록(`R:`) 90행·burning 조회(`B:`) 112행·burning 생성(`A:`) 24행·
burning 삭제(`X:`) 32행·bulf 소리(`S:`) 12행이다.

**변이 확인:** 구현 9곳을 하나씩 망가뜨려 모두 실패로 잡혔다.

| 변이 | 잡힌 검사 |
| --- | --- |
| held 방향 판정 `10~14` → `9~15` | 원본 재생 |
| HP 1 보존(클램프) 제거 | 원본 재생 |
| 회복 취소 Regular 0x25d 제거 | 원본 재생 |
| HP 1 감시 해제 판정 `HP == 1` → `HP == 2` | 원본 재생 |
| 권한 없을 때 경계값 되돌리기 제거 | 원본 재생 |
| HP 전환 좌표의 거의 올림 제거 | 원본 재생 |
| 절반 경계 전환 판정 제거 | 원본 재생 |
| 회복 허용 표(`recoveryAllowed`) 검사 제거 | 합성 |
| 공간 효과 콜백 누락 허용 | 합성 |

소스는 모두 원복해 같은 검사가 다시 통과함을 확인했다.

## 3. 이번에 하지 않은 것 (경계)

- **감시 쌍(DependForm) 몸체는 옮기지 않았다.** `IPriestDependentHost` 연결 지점만 마련했고,
  실제 감시 쌍 생성·통지·제거는 Form 수명 조각에서 옮긴다.
- `MakePriestGunHitHooks`·`MakePriestDamageWorldHooks` 는 분배·월드 연결 어댑터라 파생 분배 허브 조각으로 미뤘다.
- CD 판의 signed WORD HP 폭과 `RawPathSurfaceAt` 지면 조회 보정은 10.78 경로에서 쓰이지 않아 옮기지 않았다.
- 종속 조회(`contained`)·내용물 수명, Form 수명, 신전 삭제, 삭제 보상(SP·지갑·샘 풀) 계산은 아직 옮기지 않았다.
- CD 10.72 몫, 파생 분배 허브·실제 세션 연결. 이 조각은 L1 모듈 단계이며 L2 이상으로 합산하지 않는다.

## 4. 다음 조각

종속 조회·내용물(`contained`·`contentlifetime`) → Form 수명(`formlifecycle`·`formtake`·`form-packet`) →
신전 삭제 → 삭제 보상(SP·지갑·샘 풀) 계산 → 파생 분배 허브·실제 세션 연결.
시작할 때 `python dotnetpj/tools/cpp_sync.py` 를 먼저 실행하고 쓸 fixture 를 `--update --add` 로 등록한다.
