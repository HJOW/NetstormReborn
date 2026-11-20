# 스커미시 시작과 월드 생성 연결 (WB04-E1105)

12차 스커미시를 끝까지 연결했다. Campaign ▸ Skirmish ▸ 설정을 고르고 Start를 누르면 시작 섬 fort 묶음을 받아 무작위 시작 자료를 만들고, 생성한 미션 대본과 빈 월드 fort로 미션을 시작하며, 세션이 미션 Start 직전에 MP 시작 규칙으로 월드(무작위 중앙섬 → 참가자별 시작 섬 → 세계 간헐천)를 만든다. 브리핑 창의 Go!를 누르면 게임 시계가 흐른다. 항목은 이제 기본으로 켜져 있다(원본 비교 모드이거나 주소에 `?skirmish=0`이면 숨김).

## 구현

- **앱 시작.** `_startSkirmish`(설정 화면의 Start)가 `buildSkirmishLaunch`로 무작위 시작 자료를 만들고, `_beginSkirmish`가 `skirmish` 자료 묶음(E1103)을 받아 `makeSkirmishWorldPlan`(시작 섬마다 fort 바이트)과 `makeSkirmishEmptyWorldFort`(기술·덱·돈만 남기고 섬·객체를 비운 가상 fort)를 만들고, `createSkirmishFileSource`(생성한 대본과 가상 fort 경로만 구분자·대소문자 무시로 서비스하는 파일 읽기 함수)를 Guide에 넘겨 미션 `Skirmish`를 시작한다. 받는 동안 앱이 다시 시작됐거나 메인 메뉴를 떠났으면 그만두고, 시작하지 못한 오류는 `reportGameError('skirmish-start')`로 남긴다. 후보가 모자라면 안내만 남기고 설정 화면으로 돌아간다.
- **Guide 명시 파일 source.** `ClientCampaignGuide.begin(name, demo, mode, source)`가 선택적 명시 읽기 함수를 받아 `MissionScript`(이미 명시 source를 지원)에 넘긴다. 생략하면 이전 미션의 명시 source를 잇고(원문 BeginMission이 이전 FileSource를 넘김) 없으면 기본 자산 읽기를 쓴다. Restart는 `_lastCampaignMission.source`로 같은 가상 파일을 다시 쓴다.
- **실행 표식.** `_markSkirmishRun`이 미션을 시작할 때마다 설정 `skirmishRun`(1/0)을 쓰고 스커미시가 아니면 이전 월드 계획을 비운다. 이미 이식해 둔 AI 서비스가 이 값으로 AI별 난이도(`skirmishAiNDiff`)와 세계 간헐천 예산의 참가자 수(`1 + skirmishAiCount`)를 쓴다. `services.ai.skirmishWorld`가 현재 계획을 세션에 준다.
- **세션 월드 생성.** 새 [raw-campaign-session-skirmish.js](../webpj/www/core/raw-campaign-session-skirmish.js) `RawCampaignSessionSkirmish`가 cpppj `RawCampaignSession.cpp`의 `ReadSkirmishTerritory`·`BuildBattleIsland`·`PopulateCentralResidences`·`BuildCentralIsland`·`BuildSkirmishWorld`를 옮겼고 E1104의 세 부모(`RawIslandPartition`·`RawCentralIsland`·`RawBattleIsland`)를 실제 풀·월드 지도·섬 목록·정상 지형 생성기·Player 배치 범위·Graph에 잇는다. `buildSkirmishWorld`는 객체 소유자·링 배치·Chaff 규칙 네 곳을 MP(비미션)로 잠시 바꾸고(예외가 나도 되돌림), 맵 크기와 군도 밀도(참가자 5명 이상이면 5~8%)를 정한 뒤 중앙섬을 먼저, 이어서 참가자마다 지역 Template 슬롯에 시작 섬 fort를 읽고 전투섬을 만든 뒤 슬롯을 반납한다. `RawCampaignSession.buildSkirmishWorld()`로 공개하고 미션 Start 효과 `prepareWorld`가 부른다(일반 미션은 계획이 없어 아무 일도 하지 않는다).
- **기본값.** 앱 옵션 `skirmish`는 기본 true이며(원본 비교 모드나 `?skirmish=0`이면 false), `?skirmish` 주소 옵션은 더 필요하지 않다.

## 검증

- 새 Chromium `e2e/skirmish-start.spec.js` 2건: 기본 설정(AI 1명)으로 Campaign ▸ Skirmish ▸ Start가 오류 없이 생성 미션 `Skirmish`(missionType Tutorial)를 만들고 브리핑 뒤 Go!로 게임 시계가 3초 넘게 흐르며 정지·창이 없음(계획 참가자 2명·맵 크기 1·중앙섬 기록 플래그 0x09·위치 0), AI 7명·동맹·최고 난이도·금지 유닛 2개 설정으로 8명 계획(맵 크기 3)도 오류 없이 시작됨. 시작 직후와 Go! 뒤 화면을 눈으로 확인했다: 중앙섬에 `Skirmish / 1 AI / Go!` 브리핑, 주위 군도와 간헐천, 사이드바 SP 5000과 미니맵, Go! 뒤 시작 섬의 사제와 건물.
- Node 시험: `skirmish-setup`에 월드 계획(시작 섬 바이트 묶기·후보 누락/빈 바이트/빈 후보 거부·중앙섬 기록 사본)과 가상 파일 source(대본·빈 fort 경로만, 구분자·대소문자 무시, 그 밖 null) 시험을 더했고 변형 5종이 모두 시험 실패로 잡힌다. `skirmish-ui`에 앱 옵션 기본값(켜짐, 원본 비교 모드에서 꺼짐, 명시 옵션 우선)을 더했다. `e2e/skirmish.spec.js`는 `?skirmish=0`에서 항목이 없음과 기본 판본의 설정 화면 흐름으로 고쳤다.
- 세션 연결의 세부 이름·순서는 cpppj 소스와 줄 단위로 대조했으며(`BuildSkirmishWorld`의 밀도 표·5명 이상 규칙·ModeGuard 네 곳·지역 Template 후크), 실제 월드 조립에서 처음 드러난 문제 하나(월드 런타임은 프레임 표를 `FrameCodeTable` 배열이 아니라 `table.codes`로 넘김)를 고쳤다.

## 남은 일 (12차 나머지)

- 스커미시 게임의 실제 진행 검증: AI의 다리 건설·생산·공격과 개별 난이도/동맹이 N프레임 동안 동작하는지, 승리(`BadTeamDead`)·패배(`GoodTeamDead`) 안내와 `MissionAbort`, 금지 유닛 목록이 생산 메뉴에서 빠지는지, 비미션 너겟 보충을 사람 눈으로 확인한다(자연 완주는 1절 사용자 확인 범위).
- 브리핑 창이 떠 있는 동안 카메라는 중앙섬에 있고 Go! 뒤 자기 사제 섬으로 옮겨 간다(화면으로 확인). cpppj와 같은 시점인지는 L3 화면 대조에서 본다.
- 7차: 시작 실패 때 사용자에게 보이는 안내 창은 없다(콘솔 오류만).
