# 한국어 캠페인 미션 대본

캠페인(Struggle For Freedom·A Nation Rises·Complete Victory) 미션 16개의 브리핑·성공·실패 안내를 한국어로 옮겨 웹 한국어 묶음에 포함한다. 미션 대본은 단계 대본이 없는 짧은 파일(머리글 + `[A.]` 브리핑 + `[Succeeded][BadTeamDead]` + `[Failed]`)이며 실행 설정·명령은 영어 원문 그대로이고 제목(`title`)과 적 이름(`aiName`, 게임 안 플레이어 이름으로 표시됨)·안내·버튼 이름만 번역한다. 번역 이름 목록은 [prepare-assets.js](../webpj/tools/prepare-assets.js)와 [korean-assets.js](../webpj/www/assets/korean-assets.js)의 `KOREAN_CAMPAIGN_MISSIONS`가 같이 가지며, 두 목록과 한국어 묶음 매니페스트가 같은지 `tests/korean.test.mjs`가, 원본과의 구조 일치를 `tests/korean-mission.test.mjs`의 표 기반 시험이 확인한다.

## 진행

| 단위 | 미션 | 비고 |
| --- | --- | --- |
| WB04-E1112 | TheWarBegins(전쟁의 시작!)·MasterOfWhirligigs(훨리기그의 지배자)·SaveTheIsland(섬을 구하라!)·FragileFortune(위태로운 보물!) | 적 이름: 간수·훨리기그의 지배자·썬더 백작·윈드 왕자 |
| WB04-E1113 | ThunderingPower(천둥 같은 힘!)·DissolvedAlliance(깨어진 동맹!)·TheNoose(올가미)·RainVsRain(레인 대 레인!) | 적 이름(`aiName`·`aiNName`): 썬더의 기사·맨 오 워 후작·플레이어·썬더 왕자·레인 공작·레인의 시종장·레인 왕자. ThunderingPower 는 시간 쪽지(`[@20]`~`[@900]`)와 항복·승리 분기를 모두 옮겼다 |
| WB04-E1114 | RunForIt(도망쳐라!)·GuardMyBack(내 등을 지켜 줘)·Surrounded(포위!)·BreakingThrough(돌파!) | 적 이름: 레인의 군주·썬의 대신·썬더 후작·레인의 기사·썬의 군주·썬더의 군주·윈드의 군주·윈드 왕자(+ 데모용 `플레이어`). GuardMyBack 의 대신 전갈(`[Ai2TempleHalf]`·`[Ai3TempleDead]`·`[Ai4TempleDead]`)·`[ai2PriestDead][Failed]`·`[Demo]` 구간과 원문 개발 주석(영어)은 그대로 두었다. 원문 `ai3Name` 은 닫는 따옴표가 없지만 번역본은 닫았다(시험이 허용) |
| WB04-E1115 | ToTheRescue(구출 작전!)·Vicious(잔혹한 힘!)·EnemyTerritory(적진!)·FinalConfrontation(최후의 대결) | 적 이름: 체로크 총독·썬더 왕자·썬더 공작·퓨리의 공작·배신자·윈드 공작·레인 공작·플레이어·썬더/레인/윈드의 군주·스톰 군주. 이것으로 캠페인 미션 16개가 모두 한국어이다(데모 `DemoStormRages`와 보너스 도전 `bc*76` 9개는 남음) |

## 번역 규칙

- 건물·유닛 이름은 사용자 지정 용어표를 따른다(워크샾·신전·사제·골렘·썬 캐논·훨리기그·훨리베이스 등). 인물 이름은 음역하고 인용구는 뜻을 옮겼다.
- 버튼: `Review Knowledge` → `지식 보기`, `Play Mission` → `미션 시작`, `Leave Missions` → `미션 나가기`, `Next Mission` → `다음 미션`, `Continue` → `계속`(명령 인자는 그대로).
- 시험은 머리글을 제목·`aiName`·`aiNName` 줄만 빼고 원문과 같게 요구하고, 섹션 이름 순서·버튼/설정 명령·그림 태그가 같아야 하며, 한국어 대본의 적 이름이 원문과 같은 개수이고 모두 한글이며 미션 설정에서 제목이 한글이고 `A.`·`Succeeded`·`Failed` 섹션이 한글을 담고 영어 단어(4글자 이상)가 태그·명령·치환 밖에 남지 않음을 확인한다.
- 아직 번역하지 않은 미션은 영어 대체 경로를 유지한다(시험은 `bc1menu76`으로 확인; 마지막에 번역하면 다른 미션으로 바꾼다).
- 실제 Chromium에서 한국어 캠페인 미션을 시작해 보는 화면 확인은 하지 않았다(미션 진행은 사용자 확인 범위).
