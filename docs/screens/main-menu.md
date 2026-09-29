# 메인 메뉴 화면과 정적 이동 경로

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
| 기본 메뉴 | Campaign | 캡처에 보이는 선택 화면은 `tell.english`의 `[UCampaign]`과 일치 | Campaign 대화상자에 6개 묶음과 Back |
| Campaign | Early Missions, Priest Training, Struggle For Freedom, A Nation Rises, Complete Victory, User Made Campaigns | `[UCampaign]`의 `@{guideSpec}` 메뉴 → 선택 파일의 `[Overview]`; 공식 파일 `offical1~6.english` | 각 선택 화면의 캡처가 있음 |
| 공식 캠페인 목록 | 미션 항목 | 각 `offical*.english`의 `$Checked=...,MissionBegin,<미션>` | 파란 점·흐린 글자 상태는 캡처 시점의 진행 상태 |
| 공식 캠페인 목록 | Back | 각 `offical*.english`의 `Tell,UCampaign` | Campaign 선택 화면으로 돌아가도록 기록됨 |
| 기본 메뉴 | Demo | `tell.english`의 `[DEMOVILLE]` | 데모 3개와 Cancel |
| Demo | The Storm Rages / Guard My Back / Dissolved Alliance | `StartDemo`에 각각 `DemoStormRages` / `GuardMyBack` / `DissolvedAlliance` | 선택 목록은 캡처로 확인; 개별 실행 전환은 이번에 미검증 |
| Demo | Cancel | `Tell,Blank` | 취소 대상이 스크립트에 기록됨 |
| 기본 메뉴 | Credits | `tell.english`의 `[Credits]` | 원판·패치판 크레딧 2개와 Cancel |
| Credits | 10.72 Patch Credits / Original Credits | `Tell,CreditsNew` / `Tell,Creditsold` | 크레딧 **제목**의 10.72는 보유 exe의 버전 증거가 아님 |
| Credits | Cancel | `Tell,Blank` | 취소 대상이 스크립트에 기록됨 |
| 기본 메뉴 | Help | 펼침 메뉴 캡처 | General Help - F1, Technical Help, Version |
| 기본 메뉴 | Edit | `Load Battle Map` 캡처 | 2열 맵 목록, Create New Map, Cancel. 항목 선택 결과는 미검증 |
| 기본 메뉴 | Options | 펼침 메뉴 캡처 | 화면·소리·자동 데모·팁·진단 설정. 세부 항목은 [목록](README.md#1-메인-메뉴-계열-창-제목-netstorm-main-menu) 참조 |

Campaign의 공식 목록은 `offical1~6`이라는 원본 철자를 그대로 쓴다. `offical1~5`의 `$Checked` 마지막 인자에는 앞 미션의 `{Done...}` 값이 들어 있어 잠금 순서를 정한다. 화면의 파란 점은 이 완료값과 대응하는 것으로 **추정**하며, 실제 저장 상태와 표시 조건을 한 번 더 대조해야 한다. User Made Campaigns는 `offical6.english`의 `%{userSpec}`으로 목록을 구성하므로 고정된 팬 캠페인 목록을 게임 UI에 박아 넣지 않는다.

`tell.english`에는 별도로 `[Campaign]`이 `offical6.Overview`를 포함하는 경로도 있다. 기본 메뉴 Campaign 버튼이 `[Campaign]`과 `[UCampaign]` 중 어느 진입점을 쓰는지는 이 스크립트와 정지 캡처만으로 확정할 수 없다.

구버전 [공식 PDF 매뉴얼](../sources/pdf-manual.md#화면-그림과-판본별-메뉴-항목)의 32쪽(인쇄 31) 그림도 큰 제목 아래 버튼을 놓는 실제 메인 메뉴 구성과 맞는다. PDF의 색상 손상은 스캔의 영향이며, 해상도도 다르다(사용자 확인). 옛 메뉴의 기본 항목은 **7개**이며, PDF는 게임 CD가 있을 때만 `Intro`가 나타난다고 설명한다. 이 절의 보유 패치판에는 `Edit`를 포함한 **8개**가 보인다. PDF의 Campaign 네 묶음과 패치판의 여섯 묶음처럼 항목 내용과 위치는 판본별로 구분한다.

## 게임 실행 자료로 이미 확인된 동작

이전 세션의 Wine 캡처·보고서([원본 실행 관찰](README.md))에서 팁 OK → 검증 안내 OK → 기본 메뉴, 기본 메뉴의 F1 → 도움말 창, 약 45초 무입력 후 Auto-Demo 진입을 확인했다. 도움말 ESC는 창을 닫지 않았다. 이는 **이전 세션의 결과**이며 이번 정적 정리에서 게임을 재실행하지 않았다.

남은 동적 확인: Demo 안내창 OK 뒤 ESC/상단 Game 메뉴, Auto-Demo의 정확한 시간 기준, Edit의 맵 선택 결과, Options·Help의 각 항목 전환, Campaign 목록에서 잠금 항목 클릭 처리. 게임 실행 없이 위 캡처와 스크립트가 증명하는 범위를 넘겨 단정하지 않는다.
