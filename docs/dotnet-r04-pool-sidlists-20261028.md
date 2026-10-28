# R0-4 세 번째 묶음 — 풀 정리와 SID 목록 L1 대조 (2026-10-28)

작업 PC `HJOW-Athlon`. [LEFT_JOBS.dotnetpj.md 0절](../LEFT_JOBS.dotnetpj.md#dotnet-current-plan-20261028)
R0-4 영향표 A의 세 번째 묶음(`squid-pool-reset`·`state-sid-lists`)을 처리했다. cpppj·원본·AGENTS.md는 바꾸지 않았다.

## 1. 옮긴 cpppj (HEAD `1ddc4ba0` 기준)

| cpppj | C# | 내용 |
| --- | --- | --- |
| `src/o/SidPool.cpp` | `Netstorm.Core/Original/RawSidPool.cs`, `Netstorm.Core/Simulation/SidPool.cs` | `ClearDeletionHistory` 신설(other 20쌍→client 20쌍 순서로 0). 서버 판정을 생성자 공유 값(`Func<bool>`, Pool보다 오래 산다)으로 읽고 미연결이면 기존 입력을 쓴다. R4 세션 조립 때 실제 전역에 연결한다 |
| `src/o/RawSquidPoolReset.{h,cpp}` | `Netstorm.Core/Original/RawSquidPoolReset.cs`(신규) | `RawPathPoint`(2 float, 경로 런타임 이식 때 이동), `Finish`(기록 정리→실패 Y→X에 -10000), `Reset`(하위 Reset 정상 반환 때만 정리, 실패하면 뒤 효과 없음) |
| `src/o/RawStateSidLists.{h,cpp}` | `Netstorm.Core/Original/RawStateSidLists.cs`(신규) | `Register`(첫 빈 칸)·`Unregister`(모든 일치 0)·`ResetRegistered`(0이 아닌 헤더의 개수만 쓰기, 미연결 쓰기는 거부), `RawStateSidLists`(8 목록 용량·9 헤더 등록·`ResetCount`·`PrepareGeneralWorld`) |

미적용(R4 이후로 연기): Client·Gameplay·World가 필요한 정리 4종(대여 관문·공유 실패점·개수 정리·prepared 반납·사제 접두·다중선택),
CD 판본 풀 레이아웃(36바이트, C#은 패치 범위라 해당 6행을 건너뛴다).

## 2. 연결한 fixture와 검사

- `squid-pool-reset-x86.tsv` 13행(머리 1 + 자료 12) — `X86RawSquidPoolResetTests.SquidPoolResetReplaysBothNativeParentsPatch6`.
  패치 6행만 재생한다(CD 6행은 C# 패치 범위가 아니라 건너뛴다). 기록 hex·카운터 9열·정리 뒤 기록·실패 좌표 비트·자취 형식을 대조한다.
- 계약 3개 — `SquidPoolResetLowerPreservesHistoryParentClearsAndReallocates`(하위 Reset 보존·부모 정리·재확보),
  `SquidPoolResetFailureAndHistoryOnlyKeepOtherState`(실패 원자성·기록만 정리),
  `SquidPoolSharedServerBindingSwitchesAuthority`(공유 판정 전환, 두 풀 클래스).
- `state-sid-lists-x86.tsv` 67행(머리 1 + 자료 66) — `X86RawStateSidListsTests` 4개.
  66행 재생(등록·해제·초기화 콜백과 표·헤더 전체 비교), 정적 용량·9헤더, 쓰기 실패·등록·해제 경계, 개수 정리·fresh를 대조한다.

`Netstorm.Core.Tests.csproj`에 두 fixture를 `Link`로 연결하고
`python dotnetpj/tools/cpp_sync.py --update --add squid-pool-reset:Core --add state-sid-lists:Core`로
`cpp-fixture-manifest.tsv`에 등록했다(71개. reflected-commit은 영향 검토가 끝나지 않아 올리지 않는다).
`--ports-add`로 85·86번째 행을 추가하고 `--ports-reflect HEAD --ports-file RawSidPool.cs --ports-file SidPool.cs`로
두 행의 base를 올렸다(바뀐 쌍 32개 유지).

## 3. 검증

- `dotnet build dotnetpj/Netstorm.sln -c Release`: 경고 0·오류 0.
- `dotnet test dotnetpj/Netstorm.sln -c Release`: **1,413개(Assets 363 + Core 1005 + Client 45) 통과**.
- 변별력: 기록 정리 한쪽만 지우는 변이에 재생 6행과 기록 정리 계약 2건 실패를 확인하고 원복했다.
