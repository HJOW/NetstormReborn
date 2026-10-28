# R0-4 네 번째 묶음 — 사제 panic·기도와 회복 허용 L1 대조 (2026-10-28)

작업 PC `HJOW-Athlon`. [LEFT_JOBS.dotnetpj.md 0절](../LEFT_JOBS.dotnetpj.md#dotnet-current-plan-20261028)
R0-4 영향표 A의 네 번째 묶음(`priestregen`·`priest-panic`·`priest-prayer`) 중 재생 가능한 panic·기도와
공유 판정 기제를 처리했다. cpppj·원본·AGENTS.md는 바꾸지 않았다.

## 1. 옮긴 cpppj (HEAD `55520f78` 기준)

| cpppj | C# | 내용 |
| --- | --- | --- |
| `src/o/RawPriestState.cpp` | `Netstorm.Core/Original/RawPriestState.cs` | `PriestHitPointMode`에 공유 권한(`Func<bool>`)·장기 Player 기록(`byte[]`)·`GetAuthority()`·`GetRecoveryAllowed(owner)`를 둠. 10.78 stride 0xb4/offset 0x7c 신전 DWORD의 0 아님을 읽고, 미연결이면 기존 bool 배열을 쓴다. `SetHitPoints` 3곳을 새 조회로 바꿈 |
| `src/o/RawPriestDamage.cpp` | `Netstorm.Core/Original/RawPriestDamage.cs` | `PriestDamageState`에 공유 boss와 `GetBoss()`를 두고 HP 1 감시 해제 관문을 바꿈 |
| `src/o/RawPriestPreDestroy.cpp` | `Netstorm.Core/Original/RawPriestPreDestroy.cs` | `PriestPostPopState`에 공유 사제 목록과 `GetPriests()`를 두고 목록 읽기 2곳을 바꿈 |
| `src/o/RawPriestPanic.{h,cpp}` | `Netstorm.Core/Original/RawPriestPanic.cs`(신규) | `DudeNoticeTypeData`(향후 RawDudeNotice로 이동)·`PriestPanicHooks`(7종)·`Run`(신전→HP·최대·소유자→임시 속도 2배→경로, 실패 시 영역 랜덤 루프) |
| `src/o/RawPriestPrayer.{h,cpp}` | `Netstorm.Core/Original/RawPriestPrayer.cs`(신규) | `PriestPrayerType`·`PriestPrayerHooks`(16종)·`Command`(6d 요청·panic 위임·부모 전달)·`Receive`(희망 쓰기→상태·방향·boss 제거·605·604)·`Tick`(count 쓰기→이동·praytime 일치→내용물·기본 프레임). boss는 호출 시점 값을 읽는다 |
| `src/o/RawPriestRegen.{h,cpp}` | `Netstorm.Core/Original/RawPriestRegen.cs`(신규, 미검증) | `PriestRegenState`·`PriestRegenHooks`(spReads 제외)·`Handle`(0x25a만 처리). 분배기·저장소 읽기는 R4에서 연결한다 |

미적용(연기): 사제 회복 0x25a 재생 — `priest-regen-x86.tsv`가 병렬 세션 작업 중 디스크에서 삭제됐고(미커밋·git 복원 불가) 재생 근거가 없어 `PriestRegenPatchX86`을 Skip으로 두었다. Regen 모듈·검사는 이식했으나 미검증이라 이번 커밋에 넣지 않고 작업 트리에 둔다. fixture 복원 후 Skip 해제·`Link`·`--update --add`·`--ports-add RawPriestRegen`·base 갱신을 0.19 단위로 잇는다. `SupportsConstructedVtable` 등 이전 단위 연기 항목과 CD 판본·세션 연결은 R4·R6에서 잇는다.

## 2. 연결한 fixture와 검사

- `priest-panic-x86.tsv` 601행(머리 1 + 자료 600) — `X86RawPriestPanicTests.PriestPanicNative600`.
  같은 슬롯·속도 표·난수로 Run 1회를 재생하고 효과 순서·두 슬롯 전체·속도 비트·난수 상태를 대조한다.
- `priest-prayer-x86.tsv` 1127행(머리 1 + 자료 1126) — `X86RawPriestPrayerTests.PriestPrayerIndependentNative1126`.
  명령·수신·진행 1126행을 재생하고 효과 순서·슬롯 60바이트·반환 비트·최종 권한을 대조한다.
- 계약 — `X86RawPriestStateTests` 3개(기록·폴백·폭 오류, 공유 권한·boss, 공유 목록).

`Netstorm.Core.Tests.csproj`에 두 fixture를 `Link`로 연결하고
`python dotnetpj/tools/cpp_sync.py --update --add priest-panic:Core --add priest-prayer:Core`로
`cpp-fixture-manifest.tsv`에 등록했다(73개. reflected-commit은 영향 검토가 끝나지 않아 올린다).
`--ports-add`로 87·88번째 행을 추가했다(Regen 행은 재생 검증 뒤에 둔다).
`--ports-reflect HEAD --ports-file RawPriestState.cs --ports-file RawPriestDamage.cs --ports-file RawPriestPreDestroy.cs`로
세 파일의 base를 올렸다(바뀐 쌍 27개).

## 3. 검증

- `dotnet build dotnetpj/Netstorm.sln -c Release`: 경고 0·오류 0.
- `dotnet test dotnetpj/Netstorm.sln -c Release`: **1,418개 통과 + 1개 건너뜀**(Assets 363 + Core 1010 + 1 skip + Client 45).
- 변별력: panic 속도 2배 누락 변이에 204행 불일치, 검사 장치 오류 2건(신전 분기 반전·DWORD 오타)을 원본과 대조해 정정했다. Regen 모듈은 재생 없이 이식만 했으므로 변별력 확인을 fixture 복원 뒤로 미룬다.
