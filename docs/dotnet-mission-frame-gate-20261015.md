# dotnetpj 공통 미션 프레임 게이트 연결과 시간 소비 목록 (감사 D01/D02)

2026-10-15, 작업 PC **HJOW-X3D**. 근거는 [cpppj 반영 점검 문서](dotnet-cpp-audit-20261015.md)의 5절 제안 순서 1번(현재 검사 기준선 + D01/D02 화면 연결 대조)이다. 마지막 정밀 디컴파일 PC는 공통 인수인계 기록(`LEFT_JOBS.md`)상 같은 **HJOW-X3D, 2026-10-15**이며 이번 작업에서 새 디컴파일·원본 기계어 관찰·원본 게임 실행은 하지 않았다. 원본 근거는 cpppj가 이미 만든 [공통 미션 프레임 계약](exe/cpp-mission-frame-gate-20261015.md)과 `cpppj/tests/fixtures/missionframe-x86.tsv`(원본 10.78 PE 독립 관찰 328개)다.

## 1. 검사 기준선

| 시점 | 값 |
| --- | --- |
| 기준 | 커밋 `16bb9e4`, .NET SDK 10.0.402 |
| 변경 전 | 솔루션 Release 빌드 경고 0·오류 0, 단위 테스트 **990개 통과**(Assets 331 + Core 659) |
| 변경 후 | 솔루션 Release 빌드 경고 0·오류 0, 단위 테스트 **1,012개 통과**(Assets 331 + Core 681), 실패/건너뜀 0 |

기존 990개는 2026-10-12의 값과 같으므로 그 뒤 세션 시계·안내/튜토리얼 프레임 연결(10-12~10-13)은 **단위 테스트가 하나도 없는 상태**였다(`BattleSessionFrameClock`·`MissionDialogFrameGate`·`ConnectTutorialScreenFrames`를 참조하는 테스트 0개). 이번에 처음으로 원본 대조와 실제 연결 경로 검사를 붙였다.

## 2. D02 구현: 원본 004c34c0 전체 순서를 한 게이트로

### 2.1 무엇을 바꿨나

- [`MissionFrameGate`](../dotnetpj/src/Netstorm.Core/Simulation/MissionFrameGate.cs)(신규): 원본 `004c34c0`(공통 프레임)·`004c33f0`(단계 증가)·`004c2a40`(안내 정지)·`004c3170`(재지연)의 완전한 이식이다. 상태 14필드(`MissionFrameGateState`), 외부 경계 13개(`MissionFrameGateHooks`)를 가진다. 경계가 하나라도 비어 있으면 생성하지 못한다(빈 성공 금지).
- 처리 순서: 시작 여부 → 승패 → 안내 기한·10프레임 잠금 → tellAt → 7프레임 재개 → 데모 기한 → 튜토리얼 1~6 분배. 각 훅 뒤에 공유 상태를 다시 읽는다. 경계 비교: tellAt은 경과보다 **엄격히 작을 때만**, 데모 기한은 현재 시각과 **같아도** 실행, 후속 안내 기한은 float 지연을 double로 넓혀 **현재 게임 시각(double)** 에 더한다.
- 구 `MissionDialogFrameGate`(안내 잠금/재개의 부분 구현)는 **삭제**했다. 뷰어와 세션은 새 게이트만 쓴다.
- 튜토리얼 단계 글자와 임시 타이머는 화면 연결 시 **게이트 상태 하나가 소유**한다(`TutorialStages.AttachGateState`). 단계 증가는 게이트의 `Advance()`가 글자·잠금 10·안내 기한·타이머 0을 한 번에 쓴다. 연결 전 헤드리스 합성 경로(`RunTicks` 기반 검사)는 기존 동작을 유지한다.

### 2.2 원본 훅과 C# 연결

| 원본 경계 | 뷰어(`FortMapViewer.CreateMissionFrameHooks`) 연결 |
| --- | --- |
| 게임 시각 `0055b4d0` | `BattleSessionFrameClock.RawGameSeconds`(원본 숫자 시계의 double 초) |
| 정지 조회 `00460de0` | `GameClockPaused` |
| 승패 `004c36c0` | `BattleSession.CheckMissionOutcome()` (`UpdateMissionEvents` 래퍼) |
| 다이얼로그 없음 `004c8e90` | 외부 도움말/옵션·나가기 확인·미션 안내 창이 모두 없음(호출 시점에 읽음) |
| 지식 창 `00445330` / 안내 창 `004c79a0` / 0x4000 자식 `004641d0` | `KnowledgeOpen` / 미션 안내 창 / 지식·컨텍스트·미션 메뉴 |
| 현재 단계 Tell `004c2a90` | `TellCurrentStage()` — 초기엔 브리핑, 이후엔 현재 단계 섹션. 성공하면 루트 팝업을 닫고 `PauseForDialog()` |
| tellAtList `004c3470` / 데모 `00452f90` | **`NotSupportedException`** — 출하 미션에 `tellAtList`가 없고(`docs/gameplay/dialog-pause.md`) 시작 상태가 0이라 호출되지 않는다 |
| 튜토리얼 1~6 분배 | 정지 동기화 뒤 `BattleSession.AdvanceTutorialScreenFrame` (지원하는 번호 1·2만 `Tutorial`이 존재) |
| 정지/재개 깊이 `00460df0/00460e30` | 게이트의 자기 정지 표식만 바꾸고, 실제 시계는 `SynchronizePauseReasons`가 `ScriptDialog` 사유로 반영 |

### 2.3 동작이 달라진 점 (의도된 변경만)

1. 단계 증가의 정지/잠금 판정이 **캐시된 `TutorialFrameLocked` 대신 게이트의 현재 시계 정지·잠금 값**을 읽는다(같은 프레임 안에서 앞선 Tell이 정지를 잡은 경우도 즉시 반영).
2. 승패 판정이 안내 창이 입력을 막아 세션 진행이 생략된 프레임에도 **프레임마다 호출**된다(원본의 첫 단계와 같다). 이미 알린 섹션은 되풀이되지 않는다. 판정 규칙 자체는 D04 대상이며 이번에 바꾸지 않았다.
3. 잠금 판정이 `!= 0`에서 원본의 `> 0`으로 바뀌었다(음수 잠금은 정상 경로에서 나오지 않는다).
4. 연결 시 검사합·자동 진단에 들어가는 게이트 필드가 바뀌었다(`ScriptPaused`·`ClearFrames` 포함). `briefing-frames`·`resume-frames`·`stage-at` 진단 키 이름은 유지했다.
5. 안내가 열리는 모든 경로(`OpenTutorialTell`, 게이트의 현재 단계 Tell)가 `PauseForDialog()`를 부른다. 정지 동기화에 남긴 안전망(열린 안내는 정지 보유)은 재귀하지 않도록 표식만 바꾼다.

## 3. 검증

### 3.1 원본 기계어 대조 (모듈)

`X86MissionFrameGateTests.NativeFrameGate_MatchesAllStateEffectsAndReturns`가 fixture **328행**(Frame 265·Advance 48·Pause 9·Postpone 6)의 **상태 14필드 전체 비트·정지 깊이·외부 조회/효과 호출 순서·반환값**을 모두 대조한다. fixture는 cpppj 파일을 링크로 공유하며(사본 없음) 기대값을 C#에서 계산하지 않는다. 처음 실행에서 328행이 모두 일치했다.

판별력 확인으로 구현을 일부러 틀리게 바꿔 실패하는지 보았다(전부 원복했다): tellAt 경계 `<`→`<=`, 데모 기한 `<=`→`<`, 후속 기한에 현재 시각 float 축소 적용 — 세 경우 모두 이 검사가 실패했다. 그 밖에 13개 경계 누락 거부, null 인자 거부, 상태 기본값(65/1/0.5/10)을 검사한다.

### 3.2 실제 연결 경로 (헤드리스)

`MissionFrameScreenPathTests` 6개가 뷰어와 같은 구성(원본 숫자 시계 + `BattleSessionFrameClock` + 게이트 + 튜토리얼 공유 상태 + 정지 사유 동기화)을 헤드리스로 재생한다.

- 초기 브리핑은 안내 없는 연속 **10프레임째**에 열리고 시계가 정지한다. 외부 창이 열린 프레임은 카운터를 10으로 되돌린다.
- 안내를 닫은 뒤 **7프레임 연속** 막는 창이 없어야 재개된다. 중간에 창이 열리면 0으로 돌아간다.
- 다른 소유자의 정지 중에는 단계 판정은 호출되지만 증가는 거부되고, 거부된 프레임의 사건은 다음 프레임에 재생되지 않는다. 정지가 풀린 뒤 새 사건이 오면 글자·잠금 10·기한(현재 + float 0.5)·타이머 0이 함께 갱신된다.
- 튜토리얼 2 **A~I 전체를 화면 경로로 완주**한다: 각 단계 0.5초 + 10프레임 뒤 안내가 열리고 정지/7프레임 재개를 거치며, 단계 C·F·H의 double 타이머(2/4/2초)는 만료 직전 프레임 < 기한 ≤ 만료 프레임의 포함 경계를 지킨다. 안내 순서는 `A. B. C. NotVortex D. E. F. G. H. I.`다.
- 판별력: 단계 증가의 정지/잠금 무시, 재개 7→6프레임, 잠금 10→9프레임으로 바꾸면 각각 2~3개 검사가 실패했다(원복함). 검사가 게이트의 상수를 되읽어 동어반복이 되지 않도록 원본 값 10/7을 테스트에 직접 적었다.

### 3.3 클론 창 검사 (사용자 승인)

원본 게임은 실행하지 않았다. 이 PC는 창 실행 확인 예외 목록에 없어 실행 전에 사용자 확인을 받았다. 클론 `NetstormClone.exe`만 숨김 창 모드로 실행했다.

| 검사 | 결과 |
| --- | --- |
| `clone_campaign_smoke.ps1` | 통과(잠김/해제 두 설정) |
| `clone_campaign_progress_smoke.ps1` | 통과(1-1 승리 창 → 완료값 저장 → 재실행 → 1-2 해제·브리핑) |
| `clone_help_options_smoke.ps1` | 통과 |
| `clone_loop_smoke.py` | 4개 조합(영어/한국어, 1024×768·1280×720·1280×800, FPS 75/30/무제한) 모두 통과 |
| `clone_construct_smoke.ps1`, `clone_production_smoke.ps1` | 통과(브리핑을 닫고 재개된 뒤 건설·생산) |
| 튜토리얼 2 직접 확인(`--mission tutorial2`) | 브리핑이 열리며 `pause-reasons=ScriptDialog`·`pause-depth=1`, 세 페이지의 OK로 닫은 뒤 `running`·`pause-depth=0`·틱 진행 |

**스모크 스크립트 수정:** 캠페인·진행·UI·녹화 재생 스모크 4개가 클릭 직후 `assert briefing`을 바로 했다. 이는 브리핑이 즉시 열리던 시절 기준이며, **변경 전 커밋(HEAD)을 격리 작업 트리에서 빌드해 같은 명령을 실행해도 똑같이 실패**함을 확인했다(`예상=briefing, 실제=battle`). 10-13의 "10프레임 뒤 브리핑" 변경 때 어긋난 스크립트 문제이므로 해당 6줄에 `wait 15`(1/60초 단위)를 넣었다. `wait 15`를 넣으면 변경 전·후 빌드 모두 통과한다. 임시 작업 트리는 제거했다.

기존 `clone_ui_smoke.ps1`(전체화면 왕복 포함)과 `clone_recordplay_details_smoke.ps1`은 이번에 실행하지 않았다(수정은 위 `wait 15` 한 줄씩).

## 4. 남은 경계 (정직한 한계)

- **승패 판정 규칙(D04)은 그대로다.** 게이트의 첫 단계에 현재 `UpdateMissionEvents`를 연결했을 뿐 Seen 기록·엔티티 부재 기반 판정, 튜토리얼 미션 생략은 원본과 다르다.
- `Postpone`(`004c3170`, 가상+20)은 게이트에 구현·검증됐지만 **상위 호출자를 아직 확인하지 못해 화면에 연결하지 않았다**(기존 인계의 "가상+20 재지연의 상위 호출" 항목 유지).
- `started` 표식은 뷰어 생성 시점에 1로 둔다. 원본은 `004c2b20`의 맨 끝 표식이다(시작 전 프레임 처리의 호출 시점 차이는 확인하지 않았다). tellAt·데모 예약은 시작 헤더에서 읽는 부분이 연결되지 않아 0이다.
- 튜토리얼 단계 판정의 부수효과 순서(D06: H의 현재 명령 ID, G의 마지막 타이머 쓰기, 선택 정리 등), 튜토리얼 1의 간헐천·Graph(D05)는 이번에 바꾸지 않았다.
- 튜토리얼 단계 B 이후의 전환을 **GUI에서 직접 밟아 보지는 않았다**(건설·선택 입력이 필요). 그 경로는 3.2의 헤드리스 완주가 검증한다.
- 10-13 근거 도구 `tools/analyze_dotnet_{dialog,stage}_frames.py`, 그 산출물 `dotnetpj/{dialog,stage}-frames.json`, 경로 정리 이력 `docs/path-map.json`은 삭제한 `MissionDialogFrameGate.cs`를 가리킨다. 당시 시점의 근거 기록이므로 **이력으로 보존하고 재실행하지 않는다**(재실행하면 삭제된 파일 때문에 실패한다).

## 5. D01 — 아직 고정 틱(24Hz·최대 8틱)을 쓰는 시간 소비 목록

이미 실제 화면/게임 시각으로 옮겨진 것: 명령 처리, 다리 붕괴 스캔(프레임 간격), 안내 잠금·재개·0.5초 후속 지연, 튜토리얼 단계 판정과 2/4/2초 타이머(화면 연결 시 double 절대 시각), 그리기 제한, 세션 시계·정지 소유권.

`BattleSession.AdvanceFrame`은 여전히 `FixedTimestep`(기본 `DefaultTicksPerSecond=24`, `DefaultMaxCatchUpTicks=8`)으로 규칙 틱을 만들어 `Step`을 호출하며, 아래 소비자가 틱 정수나 `Tick/24`를 시간으로 쓴다. 이 목록이 D01 이후 작업의 이행 단위다.

| 소비자 | 위치 | 현재 시간 기준 | 이행의 원본 근거 |
| --- | --- | --- | --- |
| 규칙 틱 생성·따라잡기 상한 | `FixedTimestep.cs`, `BattleSession.AdvanceFrame` | 경과 시간 → 고정 틱, 8틱 초과분 폐기 | [Regular/Kernel](exe/cpp-process-reconstruction.md), [미션 게이트](exe/cpp-mission-frame-gate-20261015.md) |
| 다리 트레이 추첨 | `Step`의 `player.Tray.Update(Seconds…)` | `Seconds = Tick/24` | [다리 표·추첨](exe/cpp-bridge-reconstruction.md), D12 난수 |
| 수확 채굴 대기 | `BattleSession.Harvest.cs` `MiningUntilTick` | 틱 | D11 [실제 Path](exe/cpp-raw-path-search.md) |
| 지상 이동·재탐색 | `BattleSession.Movement.cs`(`route.Progress` 틱당 진행) + 화면 보간 | 틱당 진행량 | D11 [Path 시각 준비](exe/cpp-raw-path-data.md) |
| 건설 완료 | `BattleSession.Construction.cs` `CompleteTick` | `Tick + TicksFor(10초)` | D13 [끝 시각](exe/cpp-construction-time-reconstruction.md)(원본도 현재+10초이므로 값은 유지, 호출 시점·기한이 대상) |
| 유닛 생산 완료·재충전 | `BattleSession.Production.cs`, `BattleSession.Commands.cs` `UnitReadyTick` | 틱 | D13 |
| 전투 발사·충돌·번개 표시 | `BattleSession.Combat.cs`(`NextAttackTick`, `AttackStartedTick`, 충돌/번개 수명) | 틱 | D10 [총 시각](exe/cpp-gun-hit-20261013.md) |
| 비행체 출격·재급유·복귀 | `BattleSession.Flyers.cs` `ReadyTick`/`ReturnTick` | 틱 | D10 |
| 희생 의식 | `BattleSession.Sacrifice.cs` `StartTick` | `(Tick-StartTick)/24`로 환산한 초 | 의식 원본 근거는 별도 확인 필요 |
| 얼음 타워 성장 | `BattleSession.Defenses.cs` `RegenerationStartTick` | 틱 | D15·[공통 프레임](exe/cpp-frame-advance-reconstruction.md) |
| 대포·석궁·투사체 애니메이션 | `CannonAnimation`, `SunCannonAnimation`, `CrossbowAnimation`, `ProjectileAnimation`, `GameEntity.Next*AnimationTick` | 틱 | D15 |
| 캠페인 AI 이동 간격 | `BattleSession.CampaignAi.cs` | `Tick % TicksFor(간격)` | D16(임시 AI, 전략 미복원 — 보류) |
| 튜토리얼 헤드리스 타이머 | `TutorialStages.TimerTick` | 틱(**헤드리스 합성 경로 한정**) | 화면 경로는 이미 double 절대 시각 |

이행 원칙(감사 D01): `Seconds` 속성 하나를 바꾸거나 기본 틱 수를 늘리는 것으로 끝내지 않고, 규칙별로 원본의 기한·정밀도·호출 순서로 옮긴다. 0틱·여러 틱·700ms 이상 지연, 반복/중첩 정지, 기한 직전/동일/직후, 미션 교체와 DWORD 감김을 같은 시간열로 재생해 검증한다. 모든 FPS에서 검사합이 같다는 가정은 추가하지 않는다.

## 6. 재현 방법

```powershell
cd dotnetpj
dotnet build Netstorm.sln -c Release
dotnet test Netstorm.sln -c Release
# 이번 단계의 검사만
dotnet test tests/Netstorm.Core.Tests/Netstorm.Core.Tests.csproj -c Release --filter "FullyQualifiedName~MissionFrame"
```
