# W05 선행 조각 — 패치판 SP 저장소와 삭제·해체 보상(RawSpStore·RawSquidReward)

2026-10-23, 작업 PC **HJOW-Athlon**. [이식 계획](dotnet-cpp-port-plan-20261016.md)의 **W05(경제·난수) 선행 부분**이다.
[현행 계획](../LEFT_JOBS.dotnetpj.md#dotnet-current-plan-20261022)의 "바로 다음 작업 3번 — W05 선행 부분"을 수행했다.
다음 조각인 내용물 수명(`ContentFormLifetime`)이 요구하는 공유 SP 저장소·`SquidReward`·지갑 가산/알림·난수 소비를 먼저 옮긴 것이다.

근거는 커밋된 cpppj 소스(`SpStore.cpp`·`SpStore.h`·`SquidReward.cpp`·`SquidReward.h`)와 원본 fixture
(`sprotate-x86.tsv`·`reward-x86.tsv`)뿐이다. 새 디컴파일 산출물을 읽지 않았고 원본/클론 게임·창 실행도 없다.
현재 PC는 마지막 전체 정밀 디컴파일 PC(**VM-W11-Codex, 2026-10-22**)와 다르지만 디컴파일 소스를 분석하지 않았으므로
재디컴파일 대상이 아니다. 마지막 디컴파일 PC 기록은 그대로다. `cpppj/`·공통 `LEFT_JOBS.md` 는 수정하지 않았다.
이 조각은 **L1 모듈 단계**이며 L2 이상으로 합산하지 않는다. 실제 플레이는 달라지지 않는다.

## 1. 구현

| 파일 | 내용 | 원본 근거 / cpppj 근거 |
| --- | --- | --- |
| `RawSpStore.cs` (신규) | `RawSpStore`: `Initialize`·`Get`·`Set`·`Rotate`·`BindTrackingOffset`, 32×9 DWORD 배열, offset·순열·준비 상태 | `SpStore.cpp` 의 0040eba0(초기화)·0040ec50(읽기)·0040edb0(쓰기)·00491680(회전) |
| `RawSquidReward.cs` (신규) | `SquidRewardTypes`·`SquidRewardState`·`SquidRewardHooks`·`SquidRewardResult`, `Plan`·`Refund`·`SalvageRefund`·`AddSp`·`CreditWallet`·`TypeCostUnits`·HP 조회 | `SquidReward.cpp` 의 0044c2f0(보상)·0044c240(환불)·0044b220(비용 곡선)·0043cb30(해체 비용)·0040eec0/0041df80(SP 가산)·00413bb0(지갑) |
| `NetstormRandom.SetState` (추가) | SP 저장소 초기화가 재시드 값을 주입하는 setter | `GameRandom::SetState`(SpStore.cpp) |
| `X86RawSpStoreTests.cs` (신규) | `sprotate` 61행 재생 + 합성 2건 | `SpStoreNoticeTests.cpp` 의 `SpStoreNotice_ReplaysExistingNativeOffsets`·계약 검사 |
| `X86RawSquidRewardTests.cs` (신규) | `reward` 10.78 몫 2,400행 재생 + 합성 4건 | `RewardTests.cpp` 의 `Replay`·`reward_sp_store_roundtrip_and_guards`·`reward_rejects_bad_inputs_before_writing` |
| `Netstorm.Core.Tests.csproj` | fixture 링크 2건, `cpp-fixture-manifest.tsv` 갱신(54→**56**개) | `cpp_sync.py --update --add sprotate:Core --add reward:Core` |

### 패치판 SP 저장소 (`RawSpStore`)

- 소유자 SP 32비트를 32개의 9칸 DWORD 배열에 비트로 흩뿌린다. 값의 k번째 비트는 배열 k 의 `(offset+k)&31` 비트다.
- 초기화 순서: 재시드 콜백 → 난수 상태 주입 → offset 추첨(0~99) → 추적 offset 기록 → 32번 순열 교환 →
  **항등 순열 복원** → 배열을 fill 로 채우기. 교환 결과를 저장에 쓰지 않는다.
- `Get` 은 실제 비트만 모으고, `Set` 은 실제 비트를 쓴 뒤 같은 DWORD 의 다른 위치에 난수 잡음 5개를 OR 한다.
  실제 비트 위치는 잡음에서 제외하므로 값은 그대로 읽힌다.
- `Rotate` 는 모든 DWORD 를 왼쪽 3비트 회전하고 offset 도 3 전진시켜 값을 보존한다.
  offset 이 9600 **보다 크면** 9600 을 뺀다(같으면 그대로 둔다).
- `Get`/`Set`/`Rotate` 는 초기화 전이면 먼저 초기화한다. 재시드 콜백이 없으면 초기화에서 거부한다.
  소유자 번호는 0~8 만 받고, 추적 offset 은 초기화 전에 한 번만 연결할 수 있다.

### 삭제·해체 보상 (`RawSquidReward`)

패치판(10.78)은 SP 저장소를 반드시 요구한다. CD 판은 옮기지 않았다.

- `TypeCostUnits`: 로더가 저장한 `ftol(cost + 번호*23)` 에서 번호*23 을 빼 디코딩한다. 범위 밖·비유한 값은 거부한다.
- `CostCurve`: 같은 float 단가를 `n+1` 번 누적한 double 을 `ftol`(0 방향 절삭, 하위 32비트)로 줄인다.
  제단(164)·받침(165)은 개수를 하나 줄이며, 음수 개수는 0 이다. 항 수·누적 범위는 원본 계산 범위로 제한한다.
- `SalvageCost`: 같은 곡선에 0.25 배율을 곱하되 너겟(154)은 곱하지 않는다. 누적 뒤에만 float 로 절삭한다.
- `SalvageRefund`: 성장형 rainBlocker(150)는 0, HP 없는 객체는 절삭 비용 그대로, HP 가 있으면
  `(float)(hp*cost)` 를 최대 HP 로 나눈 뒤 절삭한다. HP 절반 미만 모드·최대 HP 0 은 0 이다.
- `Plan`: 제단은 대상이 아니고, 삭제 보상에서는 rainBlocker(147)를 제외한다. 성장형 rainBlocker 는
  비용만 rainBlocker 것으로 대체하며 가상 개수는 호출마다 다시 묻는다. 삭제 보상 지급액은
  `(int32)(percent*cost)/100`, 수신자 0 이면 0 이다.
- `AddSp`: 현재 SP(비트 패턴 float)에 delta 를 float 으로 더하고, 지갑에는 delta 의 절삭값을 넣는다.
  SP 고정 모드에서는 delta 대신 `194162 - sp` 를 쓴다. 표시 갱신 훅은 가산마다 한 번이다.
- `AddWallet`: AI 미러 비활성·소유자 1~8 밖·AI 미부착이면 무동작. 32비트로 감고, clamp 조건이면 0x4a6 아래로 내려가지 않는다.
- `Refund`: 계획대로 SP 를 가산하고 `geyserPool += cost - refund` 를 32비트로 감는다.

## 2. 검증 결과

전체 Release 경고 0·오류 0(`--no-incremental` 강제 전체 빌드), 단위 **1,303개 통과**(Assets 363 + Core 940; 이번 +8).

| 검사 | 내용 |
| --- | --- |
| `sprotate` 61행 재생 | 명령마다 offset·추적 offset·난수 상태와 **32×9 DWORD 배열 전체**(2,304 hex)를 리틀 엔디언으로 대조하고, 초기화 뒤 순열이 항등인지 확인한다 |
| `reward` 10.78 몫 2,400행 재생 | 매 행마다 타입 3개·슬롯·장부·전역·AI 지갑·SP 9개·재시드/fill 을 입력으로 되돌리고, 샘 풀·지갑 9개·SP 9개·Refund 전후 난수 상태·배열 체크섬·offset·표시 호출 수·가상 개수 호출 수를 대조한다 |
| 합성 6건 | 저장소 계약(재시드 누락·소유자 범위·늦은 추적 연결)·왕복/회전 보존·보상 입력 거부·제단/rainBlocker/성장형 분기·절반 HP/지갑 규칙·SP 고정 모드와 디버그 assert |

- `sprotate-x86.tsv` 는 61행이며 I/S/R 명령과 스냅샷이 1:1 이다. 0·0xffffffff·0xcdcdcdcd fill 과 9600 경계 회전이 포함된다.
  C++ 검사는 offset/tracking 만 봤지만 이번 구현은 fixture 가 담고 있는 **전체 배열 직렬화**까지 비교한다.
- `reward-x86.tsv` 는 총 7,200행(3 판본 × 2,400행)이고 10.78 몫 2,400행만 재생했다.
  해체 환불 705행·삭제 보상 1,695행, 수신자 0~8 전부, 비율 25·100·200·1,000,000·0xffffffff,
  가상 개수 스크립트 1,058행(base 1,342행), 표시 갱신 0/1 이 모두 쓰인다.
  비유한 비용·HP·클램프·AI 부착·고정 SP·디버그 assert 조합도 포함된다.
- `originalCD`·`original1037` 행은 읽지 않는다(기준 판본은 10.78 뿐이다).

**변이 확인:** 구현 18곳을 하나씩 망가뜨려 17곳이 실패로 잡혔다.

| 변이 | 잡힌 검사 |
| --- | --- |
| SP 순열 항등 복원 제거 | 원본 재생 |
| 잡음 비트 수 5→4 | 원본 재생 |
| 회전 걸음 3→1 | 원본 재생 |
| offset 경계 `>`→`>=` | 원본 재생(9600 경계 행) |
| SP 읽기 비트 조건 변경 | 원본 재생 |
| SP 0 비트 쓰기 제거 | 원본 재생 |
| 삭제 보상 `/100` 제거 | 원본 재생 |
| 제단·받침 개수 감소 제거 | 원본 재생 |
| 삭제 보상 rainBlocker 제외 제거 | 원본 재생 |
| 성장형 rainBlocker 비용 대체 제거 | 원본 재생 |
| 수신자 0 지급액 0 규칙 제거 | 원본 재생 |
| 해체 배율 0.25→0.5 | 원본 재생 |
| 너겟 배율 면제 제거 | 원본 재생 |
| SP 고정 모드 제거 | 원본 재생 |
| AI 부착 검사 제거 | 원본 재생 |
| 받침 abstract HP 반전 | 원본 재생 |
| 받침 HP 보유 규칙 제거 | 원본 재생 |

`wallet <= 0x4a6` → `wallet < 0x4a6` 변이는 **동작 차이가 없는 동등 변이**다(같은 값일 때도 결과가 0x4a6 이다).
합성 검사에 같은 최소값 입력을 넣어 이 사실을 확인했다. 소스는 모두 원복해 재통과를 확인했다.

## 3. 이번에 하지 않은 것 (경계)

- **CD 10.72·10.37 보상 경로는 옮기지 않았다.** CD 는 지역 float SP(`cdLocalSp`)와 지갑 전용 지급을 쓰며 저장소가 필요 없다.
- `BindPatchWallet`(실제 PATCH Player/brain 지갑 처리기 연결)과 실제 세션의 지갑·SP 표시 연결은 옮기지 않았다.
  지금은 값형 AI 지갑과 주입된 SP 저장소만 쓴다.
- `SquidReward` 의 `virtualCount` 파생 객체 훅은 시험용 콜백으로 검증했을 뿐, 실제 타입별 가상 표(+0x80) 연결은 없다.
- 내용물 수명(`ContentFormLifetime`) 통합·신전 삭제·삭제 수명주기 연결·실제 세션 연결은 다음 조각이다.
  이 조각은 L1 모듈 단계이며 L2 이상으로 합산하지 않는다.
- 동기화 목록의 반영 커밋은 **`f95a8db2` 그대로** 두었다. HEAD(`4959afe7`, 우리 DN 커밋)까지 cpppj 커밋 66개 가운데
  이 저장소·보상 모듈을 바꾼 커밋은 없지만, 새 fixture 30개가 미이식 기능을 담고 있어
  ["미반영 동작이 남으면 반영 커밋을 무조건 HEAD 로 올리지 않는다"](dotnet-cpp-port-plan-20261016.md#7-cpppj-진행을-따라가는-절차-다른-ai가-cpppj를-계속-진행-중)는 규칙을 지켰다.

## 4. 다음 조각

내용물 수명(`contentlifetime`·`RawContainedCreate`) → 신전 삭제 → 삭제 수명주기와 보상 연결 →
파생 분배 허브·실제 세션 연결. 시작할 때 `python dotnetpj/tools/cpp_sync.py` 를 먼저 실행하고 쓸 fixture 를
`--update --add` 로 등록한다.
