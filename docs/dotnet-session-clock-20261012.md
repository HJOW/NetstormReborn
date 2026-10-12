# dotnetpj 실제 세션 시계·정지 소유권 연결

후속 [초기 브리핑/안내 재개 프레임](dotnet-dialog-frames-20261013.md)에서 즉시 브리핑/즉시 Resume를 10/7개의 실제 화면 프레임 판정으로 바꿨다. 이 문서의 숫자 시계/소유권 연결은 유지하며 후속 변경의 동작 대조는 아직 미수행이다.

작업 PC: **VM-W11-CODEX**(`10.0.0.17`), 기록일: **2026-10-12**. cpppj ButtonGump 정적 구현 단계 완료 뒤 사용자 요청에 따라 dotnetpj를 진행했다. AGENTS.md·두 LEFT_JOBS를 읽었고 기존 cpppj 변경을 보존했다. 공통 마지막 디컴파일 PC는 **VM-W11-CODEX, 2026-10-12**이며 이번 단계의 새 디컴파일/원본 실행은 없다.

9-1 A의 후속으로 **기존 `OriginalGameClock`을 실제 `BattleSession`·MonoGame 지도 프레임과 연결했다.** 수동 정지와 스크립트 안내의 소유권을 구분하고, 일반 F6 지식창의 실행 상태를 세션·화면 제한·표시 연출에서 일치시켰다. **구현과 Release 컴파일 완료이며 이번 변경의 동작 테스트·GUI·독립 PE 실행 대조는 미수행이다.**

## 연결 경로

```text
WINMM/FrameWallClock → 화면 시작의 고정 밀리초
                         ├─ FrameLoopClock → 벽시각/그리기 제한
                         └─ OriginalGameClock → BattleSessionFrameClock
                                                  ├─ 중첩 정지 사유
                                                  └─ BattleSession.AdvanceTo(절대 게임 시각)
                                                       → 명령·다리 스캔·남은 규칙 틱
```

`NetstormGame`이 한 `OriginalGameClock`을 화면 루프와 지도에 공유한다. 각 Update 시작에 실제 펼친 밀리초를 한 번 읽고 지도 입력·정지 전환·세션 진행에 같은 값을 제공한다. 그리기 제한은 기존대로 벽시각을 새로 조회하며 게임 정지 누계가 벽시각 계산을 바꾸지 않는다.

`BattleSessionFactory.Create`에 시작 프레임 시각을 전달하고 다리 스캔을 **현재 게임 시각 +10초**로 초기화한다. 실제 앱은 원본 epoch가 들어 있는 절대 게임 초를 쓴다. 기존 헤드리스 호출의 기본값 0과 `Advance(경과 초)`는 유지한다. 새로운 `AdvanceTo`는 전달받은 시각을 그대로 저장해, 차이를 구한 뒤 다시 더하면서 원본 double 비트가 달라지는 재계산을 피한다.

`BattleSessionFrameClock`은 원본 게임 시각으로 실행 프레임을 한 번 진행하고, 공유 시계가 정지해 있으면 **`ProcessCommands`만 호출**한다. 정지 프레임에서 다리 스캔·규칙 틱을 진행하지 않는다. 이동/건설/생산 등 나머지 규칙 타이머의 `Seconds = Tick /24`, 따라잡기 8틱 상한과 화면 보간은 여전히 후속이다.

## 정지 소유권

| 소유권/화면 | 동작 |
| --- | --- |
| `Manual` | Pause·Shift-F9·옵션 Pause, 개발 뷰어의 Space |
| `ScriptDialog` | 튜토리얼·캠페인 스크립트 안내/브리핑 |
| `ViewerIdle` | 미션·배치·다리·전투 시험이 모두 꺼진 개발 뷰어의 기존 시간 정지 |
| F6 직접 지식창·일반 도움말·미션 메뉴 | 기존 게임 시계를 계속 진행 |
| 브리핑 위의 지식창/도움말 | 뒤의 `ScriptDialog` 소유권 유지 |

각 비트는 처음 생길 때 한 번 Pause하고 없어질 때 한 번 Resume한다. 매 프레임 같은 창을 보고해도 정지 깊이를 계속 늘리지 않는다. 수동 정지와 스크립트 안내가 함께 있으면 각각의 소유권이 있으며 한쪽을 닫아도 다른 쪽이 유지된다.

소유권 교체는 **새 요청을 모두 등록한 뒤 종료한 요청을 해제**한다. 사유 A가 사유 B로 바뀌는 순간 깊이가 0으로 내려가며 정지 누계가 불필요하게 갱신되는 일을 피한다. `FortMapViewer.Dispose`는 자기 소유권만 해제한다. 새 미션 뷰어를 만든 뒤 이전 뷰어를 Dispose하는 기존 전환 순서에서도 새 뷰어의 정지를 보존한다.

지도 Update의 진입/`finally`에서 사유를 동기화한다. 튜토리얼·지식창·컨텍스트 메뉴·작은 화면 등 조기 반환도 사유 변경을 반영한다. 수동 토글과 실제 세션 진행 직전에도 동기화한다. 기존 UI가 참조하는 `GameClockPaused`는 이제 공유 숫자 시계의 정지 깊이를 읽는다.

## F6 지식창 규칙

[기존 정지 분석](gameplay/dialog-pause.md)의 원본 정지 호출은 스크립트 `004c2a56`, 영상, 개발자 Pause `004d82b5`, assert 창이다. 직접 F6 지식창은 시계를 멈추지 않는다. [1-1 녹화 기록](videos/the-war-begins-record-play-20260930.md)에서도 지식창 동안 SP 증가·전투 소리가 확인되어 있다. 이번 단계에서 원본 게임을 새로 실행한 근거가 아니다.

기존 클론은 F6 지식창에서 `UpdateSession`으로 시간을 진행하면서, `GameClockPaused`에 `KnowledgeOpen`을 넣어 그리기 제한에는 정지라고 보고했다. 이번 연결은 **F6 직접 지식창의 세션 진행과 maxFPS 제한을 실행 상태로 일치시킨다.** 브리핑이 뒤에 있으면 그 브리핑의 정지를 계속 사용한다. 지식창마다 새 Pause를 추가하지 않는다.

걷기/낙하/취소 연출의 기존 경과시간 계산은 유지하고 실행 여부를 같은 `GameClockPaused`로 판정한다. 이는 애니메이션/효과의 원본 타이머 이식 완료를 뜻하지 않는다. 게임 UI에는 구현 진단 값을 추가하지 않았고 기존 자동 진단 문자열에 `pause-depth`·`pause-reasons`를 추가했다.

## 호스트 시간 경계

- `OriginalGameClock`의 DWORD·epoch·중첩 정지 숫자 구현은 변경하지 않았다.
- 큰 음의 시각 차이(DWORD 주기의 절반 초과)가 나타나면 세션 프레임 축에 한 주기 **4,294,967.296초**를 더한다. 작은 double 역행에서는 세션의 직전 시각을 유지한다. 원본 숫자 결과는 `RawGameSeconds`로 그대로 읽을 수 있다.
- 새 세션 시작의 원본 시각이 음수인 경계에서는 기존 세션 API의 비음수 계약에 맞춰 0부터 연결한다. 정상 비음수 시각은 그대로 전달한다.
- `RunTicks`·CLI wait·다리 시험의 명시적 합성 앞당기기가 세션 시각을 바꾸면, 다음 실시간 프레임에서 그 차이를 세션 오프셋으로 보존한다. 원본 시계 자체를 앞당기지 않는다.

이 경계 처리는 호스트의 세션 연속성 처리다. 여러 주기 동안의 관찰 공백·모든 극단 double/중첩 정지 조합은 이번 단계에서 동작 대조로 확인하지 않았다. 벽시계·카메라·일반 UI의 실제 경과 시간 전달은 앞 단계 방식을 따른다.

## 변경 파일과 빌드

- `dotnetpj/src/Netstorm.Core/Simulation/BattleSessionFrameClock.cs`: 정지 소유권·원본 시계/세션 연결.
- `BattleSession.cs` / `BattleSessionFactory.cs`: 절대 프레임 API·시작 시각 주입.
- `NetstormGame.cs` / `FortMapViewer.cs` / `FortMapViewer.Session.cs`: 공유 숫자 시계·고정 눈금·전체 반환 경로·수동 토글·뷰어 종료 연결.
- `FortMapViewer.Knowledge.cs`: 세션 갱신을 새 시계 경로로 호출.
- `FortMapViewer.Units.cs` / `FortMapViewer.Effects.cs`: 동일 정지 판정·자동 진단 값.

**Release 솔루션 빌드 성공, 경고 0·오류 0**(최초 복원 포함 125.47초, 주석 정리 후 최종 증분 빌드 14.98초). 실제 게임과 기존 두 테스트 프로젝트를 컴파일했다. 이 PC에 `obj/project.assets.json`이 없어 첫 `--no-restore` 빌드가 NETSDK1004로 중단됐고, 기존 패키지 버전을 복원한 뒤 성공했다. 새 패키지/버전 변경은 없다. 빌드 로그는 UTF-8의 `extracted/dotnet-session-clock-20261012-{build,build-final,build-incremental}.log`다.

**새 테스트를 추가하거나 실행하지 않았다.** 앞 단계 **990개** 단위 통과 및 dotnet 원본 관찰 누적 **53,471개**는 이전 결과이며 이번 변경의 검증 결과로 합산하지 않는다. fixture·원본 근거 JSON·cpppj 소스는 이번 단계에서 수정하지 않았다. AGENTS.md·보호 원본도 변경하지 않았고 게임/GUI 실행·커밋/푸시는 없다.

```powershell
dotnet build dotnetpj/Netstorm.sln -c Release
```

## 후속 작업

1. 새 연결의 중첩/반복 정지, 정지 중 명령, 사유 교체·부모 브리핑의 지식창/도움말, 미션 전환의 깊이/누계·다리 스캔·프레임 비트를 C++/보존 PE 관찰과 대조한다. F6의 maxFPS 동작과 기존 GUI 회귀도 확인한다.
2. 초기 브리핑/안내 재개 10/7프레임은 위 후속에서 구현했다. 실제 창 등록/삭제 수명·스크립트 클래스별 대응과 동작 대조, 후속 튜토리얼 단계의 시간 지연은 남았다.
3. 이동·전투·건설·생산·수확·의식·AI·튜토리얼·애니메이션의 24Hz 틱·8틱 상한·두 시각·정지 중 새 경로 보간을 이행한다. FPS 표본/Capture·OS 초기화/우선순위, D 대기 방식도 남았다.
4. 9-1 C의 기존 결정 → B → D/E/F, Failed/TryAgain 치수·Graph/표면 identity·실제 캠페인 완주/진행 저장을 이어간다. **9-1 A 전체 완료로 처리하지 않는다.**
