# 한국어 캠페인 미션 대본

캠페인(Struggle For Freedom·A Nation Rises·Complete Victory) 미션 16개의 브리핑·성공·실패 안내를 한국어로 옮겨 웹 한국어 묶음에 포함한다. 미션 대본은 단계 대본이 없는 짧은 파일(머리글 + `[A.]` 브리핑 + `[Succeeded][BadTeamDead]` + `[Failed]`)이며 실행 설정·명령은 영어 원문 그대로이고 제목(`title`)과 적 이름(`aiName`, 게임 안 플레이어 이름으로 표시됨)·안내·버튼 이름만 번역한다. 번역 이름 목록은 [prepare-assets.js](../webpj/tools/prepare-assets.js)와 [korean-assets.js](../webpj/www/assets/korean-assets.js)의 `KOREAN_CAMPAIGN_MISSIONS`가 같이 가지며, 두 목록과 한국어 묶음 매니페스트가 같은지 `tests/korean.test.mjs`가, 원본과의 구조 일치를 `tests/korean-mission.test.mjs`의 표 기반 시험이 확인한다.

## 진행

| 단위 | 미션 | 비고 |
| --- | --- | --- |
| WB04-E1112 | TheWarBegins(전쟁의 시작!)·MasterOfWhirligigs(훨리기그의 지배자)·SaveTheIsland(섬을 구하라!)·FragileFortune(위태로운 보물!) | 적 이름: 간수·훨리기그의 지배자·썬더 백작·윈드 왕자 |

## 번역 규칙

- 건물·유닛 이름은 사용자 지정 용어표를 따른다(워크샾·신전·사제·골렘·썬 캐논·훨리기그·훨리베이스 등). 인물 이름은 음역하고 인용구는 뜻을 옮겼다.
- 버튼: `Review Knowledge` → `지식 보기`, `Play Mission` → `미션 시작`, `Leave Missions` → `미션 나가기`, `Next Mission` → `다음 미션`, `Continue` → `계속`(명령 인자는 그대로).
- 시험은 머리글을 제목·`aiName` 줄만 빼고 원문과 같게 요구하고, 섹션 이름 순서·버튼/설정 명령·그림 태그가 같아야 하며, 한국어 미션 설정에서 제목·`aiName`이 한글이고 `A.`·`Succeeded`·`Failed` 섹션이 한글을 담고 영어 단어(4글자 이상)가 태그·명령·치환 밖에 남지 않음을 확인한다.
- 아직 번역하지 않은 미션은 영어 대체 경로를 유지한다(시험은 `FinalConfrontation`으로 확인; 마지막에 번역하면 다른 미션으로 바꾼다).
- 실제 Chromium에서 한국어 캠페인 미션을 시작해 보는 화면 확인은 하지 않았다(미션 진행은 사용자 확인 범위).
