# 녹화 분석 클론 반영 작업 인수인계

작성일: 2026-10-02. 작업 환경: Windows `HJOW-Athlon`.

## 현재 상태와 실행 제한

네 번째 녹화의 옵션·도움말·우클릭·주요 조작키를 클론에 반영했고 테스트 코드를 작성했다.
사용자가 **“테스트 실행은 건너뛰어”**라고 지시한 이후에는 빌드·단위 테스트·GUI 검사를 추가로 실행하지 않았다.
아래 명령은 이후 검증 작업을 재개할 때 사용할 인수인계 자료이며 이번 작업에서 실행하지 않았다.
구현·테스트 코드 작성은 완료했고, 최종 파일 상태의 테스트 실행은 남아 있다.

상세 동작과 원본 근거는 [클론 적용 기록](../screens/recorded-ui-clone-20261002.md),
남은 작업 상태는 [LEFT_JOBS.md](../../LEFT_JOBS.md)의 맨 위 절을 참고한다.

## 작업 내역

| 영역 | 반영한 내용 | 주요 파일 |
| --- | --- | --- |
| 옵션 | 켜진 항목·현재 값에만 파란 마크, 토글·값 선택 뒤 메뉴 전체 닫힘, 메인/미션 공용 메뉴, 음량·해상도 설정 저장 | `MainMenuView.cs`, `OriginalUiSkin.cs`, `NetstormGame.cs` |
| 음향 | Wind Noise 반복 재생과 Sound On·음량 연동, Speaker Swap L/R의 PCM 채널 교환, 이전 설정 파일의 기본값 유지 | `AudioPlayer.cs`, `Core/Audio/PcmChannels.cs`, `Core/Display/DisplaySettings.cs` |
| 도움말 | 원본 전체 영문 본문, 앵커 별칭·첫 중복 정의·미션 조건문·내부 링크·인라인 그림 처리 | `Assets/HelpTopics.cs`, `Assets/HelpDocument.cs` |
| 도움말 화면 | 공용 창, 고정 능력치 머리, 본문·스크롤바 드래그와 휠, Back의 위치 복원, OK, 지식 격자로 복귀 | `HelpWindow.cs`, `FortMapViewer.Knowledge.cs` |
| 우클릭·생산 | 사제 건설·지식·About, 워크샵 Available/Current Production·등록·업그레이드·회수, 등록한 유닛만 생산 사이드바에 표시 | `FortMapViewer.ContextMenu.cs`, `FortMapViewer.PlayUi.cs` |
| 규칙 | 워크샵 업그레이드 비용(Sun 800 SP, Wind·Rain·Thunder 각 1,000 SP) 표시·차감, 템플 공급 Golem의 워크샵 등록 제외 | `Core/Simulation/BattleSession.PlayerActions.cs`, `Core/Simulation/BattleSession.Commands.cs`, `Core/Rules/ProductionDeck.cs` |
| 조작 | 주요 원본 키·카메라 저장/복원·Alt/중간 버튼 스크롤, 플레이 입력과 개발 입력 분리, 팝업의 지도 클릭·가장자리 스크롤 차단 | `FortMapViewer.InputControls.cs`, `FortMapViewer.Bridges.cs`, `FortMapViewer.Session.cs`, `DisplayManager.cs` |
| 검사 도구 | 클론 내부 키·우클릭·드래그 입력, 상태 확인·PNG 저장, 메뉴/미션/워크샵 검사 모드 | `UiAutomation.cs`, `tools/clone_help_options_smoke.ps1`, `tools/clone_ui_smoke.ps1` |

표의 `Assets/`·`Core/` 접두사는 각각 `src/Netstorm.Assets/`·`src/Netstorm.Core/`를 가리킨다.
접두사가 없는 C# 파일은 `src/Netstorm.Game/` 아래에 있다.
README와 [조작키 안내](../gameplay/input-controls.md), [캠페인 1-1 안내](../gameplay/campaign-one.md),
[클론 UI 문서](../screens/clone-ui.md)도 갱신했다.

## 작성한 테스트 코드

| 파일 | 검사 대상 |
| --- | --- |
| `tests/Netstorm.Assets.Tests/HelpDocumentTests.cs` | 앵커 별칭·중복의 첫 정의, info 메타데이터, 링크·그림·미션 조건문·인라인 공백, Back/스크롤, 실제 원본 127개 목적지와 녹화의 목차 링크 |
| `tests/Netstorm.Core.Tests/PcmChannelsTests.cs` | 좌·우 PCM 표본 교환·재교환·잘못된 버퍼 거부, 새 음향 설정 저장과 이전 파일 기본값 |
| `tests/Netstorm.Core.Tests/CampaignOneTests.cs` | Sun Workshop의 800 SP 차감, 업그레이드 뒤 등록, 다른 소유자의 명령 거부와 잔액 유지 |
| `tests/Netstorm.Core.Tests/GameRuleTests.cs` | Golem의 Available 제외·등록 거부·칸 유지, 기존 원소·중복·슬롯·파괴 후 재등록 규칙 |
| `tools/clone_help_options_smoke.ps1 -Mode Menu` | F1·링크·Esc·드래그·Back/OK, 옵션 마크 확인용 캡처, 음량·Speaker Swap·Sound On 저장 값 확인 |
| `tools/clone_help_options_smoke.ps1 -Mode Mission` | F2/F7, 사제 건설 팝업, 미션 도움말/옵션, F6 지식 상세창 복귀, Shift+F9 정지/재개 |
| `tools/clone_help_options_smoke.ps1 -Mode Workshop` | 워크샵 완공, Rain Generator·Whirlibase 등록, Available/Current Production·생산 사이드바·Cost 메뉴의 상태 확인과 캡처 |
| `tools/clone_ui_smoke.ps1` | 기존 1-1·1-2 진입, 브리핑·생산 커서·다리·떠나기, 해상도·전체화면·음량 저장 회귀 검사 |

Mission 모드에는 PNG의 오른쪽 위 타이머 영역을 비교해 정지 중 픽셀 유지와 재개 후 변화를 자동 판정하는 코드를 추가했다.
이 비교는 고정 1024×768 화면의 `(924,0)`부터 100×18 영역을 사용하고 Windows PowerShell의 `System.Drawing`을 읽는다.
UI 스크립트의 `wait`는 프레임 수이고 Workshop 준비 명령의 `wait 25`는 게임 시간 25초다.

GUI 스크립트는 메뉴 상태와 저장 값, 타이머를 검사하지만 파란 마크의 모든 픽셀·메뉴 글자·생산 목록 개수까지 자동 판독하지 않는다.
이 부분은 아래 기대 결과와 PNG를 대조해야 한다. 실제 오디오 출력은 PCM 단위 테스트만으로 검증되지 않는다.

## 사용자 지시 이전에 실행한 검증

다음은 테스트 생략 지시 **이전**의 결과다. 최종 파일 전체를 다시 실행한 결과와 구분한다.

- 클론 Release 빌드 성공, 경고·오류 0. 테스트 빌드에서는 기존 `TextResourceTests.cs`의 CA2014 경고가 있었다.
- Assets 197개·Core 246개, 합계 443개 통과·실패/건너뜀 0. 인라인 그림·공백 보완 뒤 도움말/음향 관련 9개도 다시 통과했다.
- Golem 등록 제외와 새 단언을 추가한 뒤 Core 246개를 다시 실행해 통과했다. 이후 `TempleSupplied`를 enum 끝으로 옮겨 기존 값들을 보존한 변경은 단위 테스트를 다시 실행하지 않았다.
- Menu 검사와 별도 입력 파일의 미션·워크샵 UI 흐름이 통과했다. 워크샵의 Available 3개에서 Rain Generator 등록 뒤 2개가 되었고, Whirlibase 등록 뒤 두 생산 항목이 표시됐다. 두 등록 동안 잔액은 2,200 SP였다.
- 별도 PNG 비교로 Shift+F9 정지 중 타이머 동일·재개 뒤 변경을 확인했다. 기존 캠페인·해상도·전체화면·음량 회귀 검사도 통과했다.

**미실행:** 통합 스크립트에 추가한 `-Mode Mission`·`-Mode Workshop`과 마지막 타이머 자동 판정 코드.
해당 입력 흐름을 별도 파일로 실행했던 기록은 있지만 새 스크립트의 모드 분기·설정 준비·결과 판정 전체는 실행하지 않았다.
최종 코드와 이 스크립트를 함께 다시 검증해야 한다.

## 이후 실행할 검증

Windows에서 .NET 10 SDK와 프로젝트 자산을 준비하고 저장소 루트에서 실행한다.
GUI 검사는 클론 실행 창을 만들지만 OS의 다른 창에 클릭·키 입력을 보내지는 않는다.
아래 명령은 보류된 검증의 재개용이며 **현재 실행하지 않았다**.

```powershell
dotnet build Netstorm.sln -c Release
dotnet test tests/Netstorm.Assets.Tests/Netstorm.Assets.Tests.csproj -c Release
dotnet test tests/Netstorm.Core.Tests/Netstorm.Core.Tests.csproj -c Release

powershell -File tools/clone_help_options_smoke.ps1 -Mode Menu -OutputDirectory extracted/screens/ui-handoff-20261002/menu
powershell -File tools/clone_help_options_smoke.ps1 -Mode Mission -OutputDirectory extracted/screens/ui-handoff-20261002/mission
powershell -File tools/clone_help_options_smoke.ps1 -Mode Workshop -OutputDirectory extracted/screens/ui-handoff-20261002/workshop
powershell -File tools/clone_ui_smoke.ps1 -OutputDirectory extracted/screens/ui-handoff-20261002/regression
```

모드마다 다른 출력 폴더를 사용한다. 같은 폴더를 재사용하면 `commands.txt`·로그·설정 파일을 덮어쓴다.
각 폴더의 `stdout.log`·`stderr.log`·PNG·`settings/settings.json`을 함께 확인한다.
새 도움말/옵션 검사 스크립트는 클론 종료 코드와 45초 제한을 확인하며 실행 전 `NETSTORM_SETTINGS_DIR` 값을 종료 시 복원한다.

| 확인할 사항 | 기대 결과 |
| --- | --- |
| 옵션 켜짐/꺼짐·음량 목록 | 파란 원은 켜진 항목·선택 값에만 보이며 마우스 행 강조와 독립적이다. 선택 후 전체 메뉴가 닫히고 다시 열면 저장 값이 보인다. |
| 도움말 링크·드래그 | 태그 경계의 단어가 붙지 않고 인라인 그림과 글자가 겹치지 않는다. 본문만 스크롤 영역에서 잘리고 능력치 머리는 고정된다. 드래그가 링크 클릭으로 오인되지 않고 Back은 스크롤을 복원한다. |
| 지식 격자 | 카드 상세창의 Back은 격자로, OK는 지도까지 돌아온다. 같은 클릭이 뒤쪽 지도에 적용되지 않는다. |
| 워크샵 | Golem은 Available에 없다. Rain Generator 등록 뒤 목록이 3→2개가 되고 Whirlibase까지 등록하면 현재 생산·사이드바에 두 항목이 보인다. 등록 자체는 SP를 차감하지 않는다. |
| 워크샵 업그레이드 | 업그레이드 비용은 건설 비용과 같다. Sun Workshop은 메뉴 표시와 실제 잔액 감소가 800 SP이고, Wind·Rain·Thunder Workshop은 각각 1,000 SP다. 소유권 거부도 유지된다. |
| 주요 단축키 | F2/F7 표시, P/R 사제 선택, F4/H 본거지, F5 사제 중심, Ctrl+F5/N 수송체 순환, D/E 직전 생산/다리, Tab 최근 배치, 숫자·Shift+숫자 카메라, Alt/중간 버튼 스크롤을 수동 확인한다. |
| 팝업과 시간 | 도움말·옵션·우클릭 중 세션은 진행한다. 브리핑·승패 창의 기존 정지는 유지된다. 팝업 중 지도 클릭·가장자리 이동이 차단된다. Shift+F9/Pause의 정지·재개가 정상이다. |
| 화면·언어 | 한국어/영어, 4:3·16:9·16:10, 창/전체화면에서 메뉴와 도움말이 잘리지 않는다. GUI 스크립트의 고정 좌표는 수동 해상도 검사에 그대로 재사용하지 않는다. |
| 실제 음향 | Wind Noise·Sound On·음량·Speaker Swap을 청취하고 저장 후 재시작 상태도 확인한다. 채널 교환 시 효과음/바람 캐시가 갱신되고 음악 곡은 처음부터 다시 시작하지 않아야 한다. 이미 대기 중인 음악 버퍼에는 이전 채널 순서가 잠시 남을 수 있다. |

## 알려진 범위와 자료 보존

- 도움말 본문은 원본 영어 전체를 제공한다. 한국어 본문 번역은 후속 작업이다.
- Auto-Demo·Tell Tips at Startup·Pass Server Diagnostic, 채팅/온라인·일부 단축키, 모든 원본 우클릭 계층·Player 하위 항목은 아직 구현하지 않았다. 공격 유닛의 미확정 Damage도 기존 `?`다.
- Wind Noise의 원본 재생 시점·배율과 실제 오디오 장치의 좌우 교환은 실측하지 않았다.
- 원본 포획·의식 구간의 입력 지연과 커서 반영 지연 확인은 이번 클론 UI 작업과 별개로 남는다.
- 원본 게임·녹화를 새로 실행하지 않았고 `AGENTS.md`, 원본 자산, 네 번째 녹화, analyzeManager를 변경하지 않았다.
- 기존 검증 PNG·입력 파일·로그는 Git 제외 `extracted/screens/ui-analysis-20261002/`에 있다. `mission.*`, `workshop.*`, `regression/` 및 01~28번 PNG를 보존한다. 이 파일들은 문서 커밋만으로 다른 환경에 전달되지 않는다.
- 기준 녹화 세션은 `playingVideos/20261001T145330210Z-425636a2e82c/`였다(2026-10-02 원자료 삭제, 축소 보관본 `extracted/record-play-archive-20261002/`). 사용자가 삭제한 세 번째 세션과 혼동하지 않는다. 이후 반영은 [녹화 최종 판독](../videos/record-play-final-20261002.md)을 따른다.
