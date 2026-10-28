# R0-4 SP 묶음 — SP 가산 비트와 초기화 명시 할당 L1 대조 (2026-10-28)

작업 PC `HJOW-Athlon`. [LEFT_JOBS.dotnetpj.md 0절](../LEFT_JOBS.dotnetpj.md#dotnet-current-plan-20261028)
R0-4 영향표 A의 첫 묶음(`sp-add-bits`·`sp-initialization`·`fort-sp-write`) 중 앞의 둘을 처리했다.
`fort-sp-write`(`RawFortSpWrite` 신규 이식)는 다음 단위로 남긴다. cpppj·원본·AGENTS.md는 바꾸지 않았다.

## 1. 옮긴 cpppj 변경 (HEAD `3d038e51` 기준)

| cpppj | C# | 바뀐 내용 |
| --- | --- | --- |
| `src/o/SquidReward.cpp` | `Netstorm.Core/Original/RawSquidReward.cs` | `SpMirrorFtol` 신설: 비유한·64비트 범위 밖 증감에서 예외 대신 하위 DWORD 0. `AddSp`의 저장값 유한 검사를 제거하고 지갑 변환을 `SpMirrorFtol(delta)`로 바꿈. `BindPatchWallet` 연결(실제 세션 지갑 처리기. 연결되면 부착·비활성·최솟값 규칙을 따지지 않음) |
| `src/o/SpStore.cpp` | `Netstorm.Core/Original/RawSpStore.cs` | 명시 할당 공급자 생성자(`Action<Span<byte>>`, 36바이트=9 DWORD). `Initialize`가 행마다 공급자 입력(또는 fill)으로 같은 현재 행을 발행하고, 공급자가 던지면 앞선 행을 유지한 채 중단. `FirstPool`은 첫 행 할당 직후에 참이 됨 |

미적용(다음 단위·R4로 연기):

- `fort-sp-write` 198행 — `RawFortSpWrite`가 C#에 없으므로 신규 이식(`PORT_MAP.tsv` 행 추가 포함)으로 처리한다.
- 공유 전역 소유(`Percent`·`GeyserPool`·`PriestQuarterHp`의 장기 주소 빌리기, `TrackingOffsetCurrent` 쓰기 참조,
  `RawHitPoints.cs`의 판본별 extra 오프셋) — L1 결과가 바뀌지 않으므로 R4 세션 조립 때 연결한다.
  `PORT_MAP.tsv`의 `RawHitPoints.cs` 행은 그대로 둔다.

## 2. 연결한 fixture와 검사

- `sp-add-bits-x86.tsv` 505행(머리 1 + 자료 504) — `X86RawSquidRewardTests.SpAddBitsMatchNative`.
  `cpppj/tests/RewardSpBitsTests.cpp`와 같은 출발(난수 1·재시드 77·소유자 0/1/3·두 x87 관문)으로 AddSp 1회를 재생하고
  끝 저장값·지갑 정수·`get;set;wallet;refresh`를 대조한다.
- `sp-initialization-x86.tsv` 49행(머리 1 + 자료 48) — `X86RawSpStoreTests.SpInitializationMatchNative`.
  `cpppj/tests/ClientSpGlobalsTests.cpp`의 48행 재생과 같이 시드·사전 초기화 횟수·중단 위치로 `Initialize`를 반복하고
  중단 여부·할당 수·초기화 여부·offset·추적 offset·난수 상태·첫 배열 여부·1152바이트 전체를 대조한다.

`Netstorm.Core.Tests.csproj`에 두 fixture를 `Link`로 연결하고
`python dotnetpj/tools/cpp_sync.py --update --add sp-add-bits:Core --add sp-initialization:Core`로
`cpp-fixture-manifest.tsv`에 등록했다(66개. reflected-commit은 영향 검토가 끝나지 않아 올리지 않는다).
`--ports-reflect HEAD --ports-file RawSquidReward.cs --ports-file RawSpStore.cs`로 두 행의 base를 올렸다(바뀐 쌍 35→33개).

## 3. 검증

- `dotnet build dotnetpj/Netstorm.sln -c Release`: 경고 0·오류 0.
- `dotnet test dotnetpj/Netstorm.sln -c Release`: **1,398개(Assets 363 + Core 990 + Client 45) 통과**.
  기존 `RewardRejectsBadInputsBeforeWriting`의 "비유한 저장값 거부" 기대는 원본 동작(허용)으로 고쳤다.
- 변별력: `sp-add-bits` 504행 중 180행이 비유한 저장 SP이며 옛 코드는 그 행들에서 예외를 던진다.
  `sp-initialization`의 중단 20행은 옛 `Array.Fill` 초기화(공급자 없음)로는 재현할 수 없다.
