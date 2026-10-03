# 메인 메뉴 화면과 정적 이동 경로

2026-10-01 클론 구현: 원본 타이틀·구름 위에 74×19px 버튼을 두 줄로 배치하고 작은 돌 캠페인 창과 옵션 펼침 메뉴를 구현했다. Campaign → Struggle For Freedom → 1-1과 Options(해상도·창/전체화면·음량)을 연결했으며 다른 항목·미션은 비활성이다. [UI 외관·검증](clone-ui.md), [플레이 범위](../gameplay/campaign-one.md)를 참고한다. 아래 좌표·동작은 원본 관찰 기록이다.

> 2026-09-29 정리. 기존 `screenShots/mainMenu*.png`와 추출된 `tell.english`, `offical1~6.english`를 읽었다. 이 작업에서는 원본 게임을 실행하지 않았다.

## 좌표 기준

모든 좌표는 원본 **1024×768 클라이언트 영역** 기준이다. `screenShots/`는 창 테두리와 제목 표시줄을 포함한 외부 캡처이므로, 파일별 클라이언트 시작점(대체로 파일 좌상단에서 x=2~4, y=33~35)을 빼고 읽었다. 아래 대화상자 범위는 외곽 장식 기준의 **대략값(±5px)**이다. 클릭 자동화에는 버튼 영역 안쪽 좌표를 쓰고, 실제 입력 결과를 기록해야 한다.

| 화면 | 클라이언트 좌표의 대략적 범위 | 근거 캡처 |
|---|---:|---|
| 타이틀 그림 | (192,144)~(831,623), 640×480 | `mainMenu.png` |
| 기본 버튼 윗줄 | y=311~329, Campaign x=356~430, Multiplayer 435~509, Demo 514~588, Help 593~667 | `mainMenu.png`, 이전 분석 도구 캡처 측정 |
| 기본 버튼 아랫줄 | y=334~352, Edit·Credits·Options·Quit은 윗줄과 같은 x 순서 | `mainMenu.png`, 이전 분석 도구 캡처 측정 |
| Campaign 대화상자 | (395,233)~(631,534) | `mainMenu - Campaign.png` |
| Early Missions 대화상자 | (345,227)~(684,539) | `mainMenu - Campaign - Early Missions.png` |
| A Nation Rises 대화상자 | (383,259)~(644,510) | `mainMenu - Campaign - A Nation Rises.png` |
| Demo 선택 대화상자 | (390,268)~(638,500) | `mainMenu - Demo.png` |
| Credits 선택 대화상자 | (397,306)~(631,465) | `mainMenu - Credits.png` |
| Edit 맵 목록 대화상자 | (342,96)~(687,675) | `mainMenu - Edit.png` |
| Help 펼침 메뉴 | (634,314)~(756,365) | `mainMenu - Help.png` |
| Options 펼침 메뉴 | (553,296)~(718,564) | `mainMenu - Options.png` |

대화상자는 내용에 따라 크기가 바뀐다. 원본의 와이드 화면 자료는 없으므로 클론에서는 공통 화면 중심에 배치하고, 가장자리 HUD와 별도로 계산한다([화면 요구사항](../../LEFT_JOBS.md)).

## 메뉴 이동 경로

`tell.english`의 `$Menu`·`$Button`은 **스크립트에 기록된 대상**이다. 스크린샷의 파일명은 해당 화면이 존재한다는 근거이며, 이번 작업에서 클릭 전환을 다시 재현한 것은 아니다. `tell.english`와 `offical*.english`는 `netstorm.tarc`에서 추출한다([추출 순서](../formats/README.md#추출-순서-처음-받은-사람용)).

| 시작 화면 | 항목 | 스크립트 대상 또는 화면 자료 | 화면에서 확인한 것 |
|---|---|---|---|
| 기본 메뉴 | Campaign | `tell.english`의 **`[UCampaign]`** (실행 확인) | Campaign 대화상자에 6개 묶음과 Back. `$Timeout=120` |
| Campaign | Early Missions, Priest Training, Struggle For Freedom, A Nation Rises, Complete Victory, User Made Campaigns | `[UCampaign]`의 `@{guideSpec}` 메뉴 → 선택 파일의 `[Overview]`; 공식 파일 `offical1~6.english` | 각 선택 화면의 캡처가 있음 |
| 공식 캠페인 목록 | 미션 항목 | 각 `offical*.english`의 `$Checked=...,MissionBegin,<미션>` | 파란 점·흐린 글자 상태는 캡처 시점의 진행 상태 |
| 공식 캠페인 목록 | Back | 각 `offical*.english`의 `Tell,UCampaign` | **Campaign 선택 화면으로 돌아감**(Early Missions에서 실행 확인) |
| 기본 메뉴 | Demo | `tell.english`의 `[DEMOVILLE]` | 데모 3개와 Cancel |
| Demo | The Storm Rages / Guard My Back / Dissolved Alliance | `StartDemo`에 각각 `DemoStormRages` / `GuardMyBack` / `DissolvedAlliance` | Guard My Back 실행 확인: 창 제목 `NetStorm Demo "Guard My Back"`, 안내 창이 15초 뒤 자동으로 닫힘. 나머지 둘은 미실행 |
| Demo | Cancel | `Tell,Blank` | 취소 대상이 스크립트에 기록됨 |
| 기본 메뉴 | Credits | `tell.english`의 `[Credits]` | 원판·패치판 크레딧 2개와 Cancel |
| Credits | 10.72 Patch Credits / Original Credits | `Tell,CreditsNew` / `Tell,Creditsold` | 크레딧 **제목**의 10.72는 보유 exe의 버전 증거가 아님. 각 쪽 More/Back/Cancel, **20초마다 자동으로 다음 쪽**, 마지막 쪽 뒤 메인 메뉴(실행 확인) |
| Credits | Cancel | `Tell,Blank` | 취소 대상이 스크립트에 기록됨 |
| 기본 메뉴 | Help | 펼침 메뉴(커서를 올린 뒤 클릭) | General Help - F1 → 도움말 창 / **Technical Help → 외부 `help\help.exe readme.hlp` 실행**(Windows 10에서 Edge 창이 열림) / **Version → "Version v10.78" 창** |
| 기본 메뉴 | Edit | `Load Battle Map` | 2열 맵 목록, Create New Map, Cancel. **맵을 고르면 편집기 모드**(`NetStorm Editor Game "<맵>"`), 편집기 Game → Main Menu → "Leave Edit Mode" 저장 확인(Yes/No/Cancel). **목록은 이름순 앞 50개(25행×2열)만 보이며** 나머지(예: `TEST01`)는 **Create New Map에 기존 이름을 넣어** 연다([2026-10-03 녹화](../videos/record-play-edit-test01-20261003.md#52-load-battle-map-창)). 편집기 Game → Test Battle → 연결 창 → 브리핑 → 시험 전투 |
| 기본 메뉴 | Options | 펼침 메뉴 | 화면·소리·자동 데모·팁·진단 설정. `>` 하위 메뉴는 클릭해야 열림. Pass Server Diagnostic → Yes/No 확인 창. 세부는 [화면 목록 1.3절](README.md#13-메인-메뉴-전환데모미션편집기-메뉴-조사-2026-09-29-밤-windows-hjow-athlon) |

Campaign의 공식 목록은 `offical1~6`이라는 원본 철자를 그대로 쓴다. `offical1~5`의 `$Checked` 마지막 인자에는 앞 미션의 `{Done...}` 값이 들어 있어 잠금 순서를 정한다. 화면의 파란 점은 이 완료값과 대응하는 것으로 **추정**하며, 실제 저장 상태와 표시 조건을 한 번 더 대조해야 한다. User Made Campaigns는 `offical6.english`의 `%{userSpec}`으로 목록을 구성하므로 고정된 팬 캠페인 목록을 게임 UI에 박아 넣지 않는다.

`tell.english`에는 별도로 `[Campaign]`이 `offical6.Overview`를 포함하는 경로도 있다. 기본 메뉴 Campaign 버튼이 `[Campaign]`과 `[UCampaign]` 중 어느 진입점을 쓰는지는 이 스크립트와 정지 캡처만으로 확정할 수 없다.

구버전 [공식 PDF 매뉴얼](../sources/pdf-manual.md#화면-그림과-판본별-메뉴-항목)의 32쪽(인쇄 31) 그림도 큰 제목 아래 버튼을 놓는 실제 메인 메뉴 구성과 맞는다. PDF의 색상 손상은 스캔의 영향이며, 해상도도 다르다(사용자 확인). 옛 메뉴의 기본 항목은 **7개**이며, PDF는 게임 CD가 있을 때만 `Intro`가 나타난다고 설명한다. 이 절의 보유 패치판에는 `Edit`를 포함한 **8개**가 보인다. PDF의 Campaign 네 묶음과 패치판의 여섯 묶음처럼 항목 내용과 위치는 판본별로 구분한다.

## 게임 실행 자료로 이미 확인된 동작

**옵션 상태 표시 보완(2026-10-02):** 파란 원형 마크는 켜진 항목 또는 하위 목록의 현재 값에만 표시한다. 끄면 없어지고 다시 켜면 나타난다. 마우스 행 강조와는 별개이며, 마크가 없어도 흰 글자 항목은 클릭해 켤 수 있다. [Sound On·Play Music·Auto-Demo·음량의 전후 프레임과 메뉴 동작](options-menu.md). 도움말 본문은 [11주제 한국어 정리](../gameplay/help-text-record-play-20261001.md)와 [원본 전체 본문](../sources/in-game-help.md)에 보존했다.

**2026-10-02 사용자 UI 녹화 분석:** Options의 Resolution 세 값·음악/효과음 Volume 1~5와 선택 표시 변경, Help → General Help → 주제 링크·스크롤·Back·OK를 [네 번째 녹화 노트](../videos/ui-controls-record-play-20261001.md)에 기록했다. 실제 단축키 입력과 내장 도움말 기재를 구분한 [원본 조작키 표](../gameplay/input-controls.md)도 참고한다.

이전 세션의 Wine 캡처·보고서([원본 실행 관찰](README.md))에서 팁 OK → 검증 안내 OK → 기본 메뉴, 기본 메뉴의 F1 → 도움말 창, 약 45초 무입력 후 Auto-Demo 진입을 확인했다. 도움말 ESC는 창을 닫지 않았다. 이는 **이전 세션의 결과**이며 이번 정적 정리에서 게임을 재실행하지 않았다.

**사용자 확인(2026-09-29):**
- Campaign 목록에서 **잠긴(흐린 글자) 미션을 클릭하면 아무 반응이 없다.**
- **안내 창(Demo 안내 창 포함)에서는 ESC가 동작하지 않는다.** 누르면 아무 일도 일어나지 않고 버튼으로만 닫힌다.

클론도 이렇게 구현한다([화면 목록 1.0절](README.md#10-사용자-확인-동작-2026-09-29)).

**Windows 실행 결과(2026-09-29 저녁·밤, `HJOW-Athlon`, [화면 목록 1.2·1.3절](README.md#13-메인-메뉴-전환데모미션편집기-메뉴-조사-2026-09-29-밤-windows-hjow-athlon)):**
- Auto-Demo는 **마지막 입력(마우스 이동 포함)과 대화상자가 닫힌 시점 중 늦은 쪽부터 45초** 뒤에 시작한다. 대화상자가 열려 있는 동안은 시작하지 않는다.
- 로딩 창은 "Starting Mission..." 또는 "Connecting to Game Server - Countdown N"이며 Cancel 버튼이 있다. Auto-Demo의 안내 창은 뜰 때와 안 뜰 때가 있다(조건 미확인).
- 데모·미션 중 ESC는 상단 메뉴 막대를 켜고 끈다. 브리핑 창이 열려 있으면 반응하지 않는다. Exit Demo는 확인 없이 메인 메뉴로, Leave Mission은 확인 창(Main Menu / Replay Mission / Continue Mission)을 거쳐 메인 메뉴로 돌아간다.
- 보유 exe는 Version 창 기준 **10.78**이다.

### 메뉴 흐름도 (실행 확인 범위)

> **클론 방침(사용자 결정 2026-09-30):** 아래 시작 흐름의 "Not Validated" 안내 창(클라이언트 유효성 검사·자동 업데이트 검증 실패 안내)은 **구현하지 않는다.** 멀티플레이(Multiplayer 메뉴 이하)는 후순위로 나중에 구현한다. [network-ports.md](../exe/network-ports.md) 1-1절.

```text
시작 화면 → "Did You Know?" 팁(OK) → "Not Validated"(OK) → 메인 메뉴
메인 메뉴 ─ 45초 무입력 ─→ 로딩 창 → Auto-Demo(The Storm Rages!) ─ ESC → Game → Exit Demo ─→ 메인 메뉴
  ├ Campaign → [UCampaign] 6묶음 ─ Back → 메인 메뉴
  │    └ Early Missions → 미션 목록 ─ Back → Campaign
  │         └ 미션 선택 → 로딩 창 → 미션 화면 + 브리핑(MORE → … → OK) → 플레이
  │              └ ESC → Game → Leave Mission → "Leave Mission?" ─ Main Menu → 메인 메뉴
  │                                                              ├ Replay Mission → 로딩 창 → 같은 미션의 첫 브리핑
  │                                                              └ Continue Mission (미실행)
  ├ Demo → 데모 3종 선택 → 로딩 창 → 데모 + 안내 창(15초 자동 닫힘) ─ ESC → Game → Exit Demo → 메인 메뉴
  ├ Help → General Help - F1(도움말 창, OK) / Technical Help(외부 프로그램) / Version(OK)
  ├ Edit → Load Battle Map → 맵 선택 → 편집기 ─ ESC → Game → Main Menu → "Leave Edit Mode" ─ No → 메인 메뉴
  ├ Credits → 제작진 2종 → 20초마다 다음 쪽 → 마지막 쪽 뒤 메인 메뉴 (Back·Cancel 버튼)
  ├ Options → 펼침 메뉴(하위 메뉴는 클릭으로 열기) / Pass Server Diagnostic → Yes·No 확인 창
  ├ Multiplayer (분석 대상에서 제외, 누르지 않음)
  └ Quit (미실행)
```

동적 확인 현황과 남은 일:
- Auto-Demo 안내 창이 뜨는 조건
- Restart Demo는 [화면 목록 1.5절](README.md#15-시간-계획-분석-메뉴-타이머편집기튜토리얼-1-2026-09-29-밤-windows-hjow-athlon), Replay Mission·Restart Mission은 [1.9절](README.md#19-replay-missionrestart-mission-전환-2026-09-30-windows-hjow-athlon)에서 첫 브리핑 재진입을 확인했다. 두 미션 명령 뒤 배치물 초기화는 별도 확인 대상이다.
- Original Credits의 마지막 쪽 뒤 메인 메뉴 복귀는 [화면 목록 1.8절](README.md#18-original-credits-마지막-전환-2026-09-30-windows-hjow-athlon)에서 확인했다. 각 쪽의 화면 내용은 별도 정리 대상이다.
- Campaign 나머지 묶음의 세부 흐름 (`$Timeout=120`은 1.5절에서 실측)
- 로딩 창 Cancel은 1.5절에서 두 차례 눌렀지만 반응이 없었다. 다른 로딩 경로에서의 동작은 미확인이다.
- Create New Map의 저장 과정, 편집기 Add Island·Set All Bridge (`Create New Map` 입력 창과 Test Battle은 1.5절에서 확인)
- 결과(승리·패배) 화면 이후 흐름
