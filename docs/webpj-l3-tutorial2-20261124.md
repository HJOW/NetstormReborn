# webpj L3: Tutorial2 시작~신전 완공 안내 창 (WB04-E1305, 2026-11-24)

Tutorial1 L3([완료 기록](webpj-l3-tutorial1-complete-20261124.md))와 같은 틀로 **Tutorial2**(건설·생산)의 시작부터 신전 완공 안내 창까지 1200프레임을 cpppj 와 대조했다. 이 단위에서 **웹을 한 곳 고쳤다**(아래). 고친 뒤 체크포인트 6곳(화면)·2곳(덤프)이 화면 0픽셀·덤프 일치(환경 의존 행 `sp`·`player:*` 제외)이고 Tutorial1 L3·L2·고정 시각 시험도 그대로 통과한다.

## 틀

- 폴더 `tests/fixtures/cpppj/candidate-225553df/tutorial2-l3/`(단계표 `steps.json`·출처 `oracle.json`·체크포인트 `.png`/`.tsv`), 웹 시험 `e2e/l3-tutorial2.spec.js`.
- 적용·비교 본문은 `e2e/l3-common.js`(Tutorial1 스펙에서 옮김: 입력 프레임 규칙·체크포인트 비교)이고 두 스펙이 같이 쓴다. `runL3({ compare: false })` 는 비교 없이 체크포인트 결과만 돌려주어 어긋남 조사용 임시 단계표를 `L3_DUMP_DIR` 로 덤프할 때 쓴다.
- 오라클 수집 도구 `tools/l3-collect.mjs <폴더 이름>`(이전에는 임시 스크립트였다): 단계표를 cpp 기준선으로 돌려 화면·덤프·출처 파일을 쓴다.

## 입력(1024×768, Tutorial2)

| 프레임 | 동작 | 인자 | 결과 |
| --- | --- | --- | --- |
| 95·120 | click-if | `MORE` | 'Buildings, Part 1' 안내 두 쪽 |
| 150 | click-if | `OK` | 'Create a Temple'(BACK·OK) 닫기 |
| 180 | right | `screen:520,381` | 사제 우클릭 메뉴(High Priest Level I: Construct·View Netstorm Knowledge·About·Player), 190 화면 `m190` |
| 200 | click | `screen:570,403` | Construct 하위 메뉴(Temple), 210 화면 `m210` |
| 220 | click | `screen:745,403` | Temple 하위 메뉴(Build Wind Temple for 5000), 230 화면 `m230` |
| 240 | click | `screen:900,407` | 커서가 신전 모양이 됨(섬 밖이라 빨간 틀), 270 화면 `g270` |
| 280 | click | `screen:430,420` | 섬 안에 놓음: 사제가 걸어가 건설 시작, SP 10000→5000, 400 화면·덤프 `c400`(건설 그림자) |
| 1200 | snapshot·session-dump | `c1200` | 신전 완공: 'Temple Complete'(섬 가장자리 색이 내 색으로 바뀜, F7 안내) 창으로 정지, 단계 B |

`mission_gate` 두 번째 값은 `A`=65 → `B`=66 이다. 시작 SP 는 10000 이고 생산 창은 비어 있다(신전 뒤 다리 조각이 생긴다).

## 발견·수정: 생산 패널(HUD)이 Guide 시작 단계보다 먼저 만들어져야 한다

화면은 처음부터 모두 0픽셀이었으나 `c400` 덤프에서 `pool`·슬롯 11·12·13·28 이 달랐다. 슬롯 11~13 은 생산 패널의 Gump 폼 두 개(+ 기술 내용물 Contained 하나)이고 cpp 는 **폼 11·12 → Contained 13**, 웹은 **Contained 11 → 폼 12·13** 순서였다(미션 5프레임부터 계속). Tutorial1 은 시작 기술이 비어 있어 드러나지 않았다.

호출 스택을 찍어 보니 웹은 같은 프레임에서 `_clientGuide.run()`(안의 시작 단계 `addTechnology` → Contained 생성)을 먼저 돌리고 그 뒤에야 `_prepareCampaignProduction`·`_prepareCampaignProductionUi`(폼 생성)를 돌렸다. cpp 는 가이드 미션에서 `FinishMissionWorldLoad` 가 HUD 를 만들고 Guide 의 시작 단계(`StartMissionWorld`)는 그 뒤 루프 8번에서 돈다. **수정:** `_onFrame` 에서 생산 준비·UI 설치를 `Guide.run()` 앞으로 옮겼다(`www/netstorm.js`). 이 순서를 가정한 단위 시험 하나(`campaign-request-frame.test.mjs`, 시작의 하단 채팅 숨김 명령이 가짜 채팅 부모에 간다고 본 단언)는 실제 채팅 창으로 가는지 확인하도록 고쳤다(HUD 가 먼저 만들어지므로 현재 채팅 창이 실제 창이다).

## 남은 일

신전 완공 뒤: 'Temple Complete' 이어지는 안내, 워크샾 건설, 연구 생산, 발사기 배치·에너지 범위·회수, 미션 완료 창(Tutorial2 단계 C~I). 다음 단위에서 단계표를 늘린다.
