# webpj L3: Tutorial2 시작부터 미션 완료 창까지 (WB04-E1305~E1307, 2026-11-24)

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

## 확장: 워크샾 건설과 Production 안내 창 (WB04-E1306, 2026-11-24)

1200프레임 뒤에 'Temple Complete'(MORE)와 'Create a Workshop'(OK)을 넘기고(1220·1300) 사제를 다시 우클릭해(`screen:488,438`, 1360 화면) Construct → Workshop(`725,475`) → Build Sun Workshop for 800(`900,478`, 1460 화면 = 워크샾 모양 커서)을 고른 뒤 섬 안 `screen:580,340` 에 놓는다(1480). 건설 중(1520 덤프·화면: SP 4221 로 줄어드는 중, 800 표시)이고 완공하면 단계가 `C`=67 이 되어 'Production' 안내 창(워크샾 우클릭 → Put Knowledge Into Production)이 열려 정지한다(2600 덤프·화면). 결과: 체크포인트 w1360·w1460·c1520·c2600 이 화면 0픽셀·덤프 일치로 **웹을 고칠 차이는 없었다.**

cpp 로 먼저 관측한 이어지는 구간(아직 단계표에 넣지 않음): 2620 OK → 2700 워크샾 우클릭(`570,350`, 메뉴: View Current Production·Put Knowledge into Production·Upgrade costs 800·Salvage gains 200·About·Player) → 2720 `670,375`(Knowledge Available: Sun Disc Thrower) → 2740 `860,375` 선택하면 생산 창에 발사기가 생기고 단계 `D`=68 'First Weapon, Part 1'(MORE) → 'Part 2'(에너지 아이콘 설명, BACK·OK) 가 열린다. 발사기는 2960 생산 창 `screen:40,180` 클릭으로 집으며 놓을 수 있는 자리는 에너지 범위 안이다(탐색 중).

## 확장: 발사기 네 개·회수·미션 완료 창 (WB04-E1307, 2026-11-24)

2600프레임 뒤를 끝까지 이어 **Tutorial2 시작부터 'Mission Accomplished!' 완료 창까지 5400프레임**을 cpppj 와 대조했다. 체크포인트 덤프 12곳·화면 29곳이 모두 화면 0픽셀·덤프 일치(환경 의존 행 `sp`·`player:*` 제외)이고 **웹을 고칠 차이는 없었다.**

| 프레임 | 동작 | 인자 | 결과(단계 `mission_gate` 두 번째 값) |
| --- | --- | --- | --- |
| 2620 | click-if | `OK` | 'Production' 안내 창 닫기 |
| 2700·2720·2740 | right·click·click | `570,350`·`670,375`·`860,375` | 워크샾 우클릭 → Put Knowledge into Production → Sun Disc Thrower: 생산 창에 발사기가 생기고 'First Weapon, Part 1·2' 안내(2800 덤프·화면, 단계 `D`=68) |
| 2820·2880 | click-if | `MORE`·`OK` | 안내 두 쪽 닫기 |
| 2960·3020 | click | `40,180`·`420,340` | 생산 창에서 발사기를 집어(3000 화면) 에너지 범위 안에 놓음(SP 4200→3900): 'Weapon Displays' 안내 창(3400, 단계 `E`=69) |
| 3420·3460 | click-if·click | `OK`·`420,340` | 놓은 발사기를 눌러 사정·에너지 표시(3500 화면) |
| 3540·3600 | click | `40,180`·`460,340` | 두 번째 발사기(3580 화면) → 'Energy, Part 1'(3700, SP 3600, 단계 `F`=70) |
| 3720·3840 | click-if | `MORE`·`OK` | Energy Part 1·2 안내 닫기 |
| 3920 | click | `430,430` | 신전을 눌러 에너지 범위 표시(3960 화면) → 잠시 뒤 'Energy, Part 3'(4200, 단계 `G`=71) |
| 4220~4420 | click-if·click | `OK`·`40,180`·`420,300`·`40,180`·`460,300` | 세·네 번째 발사기(4340·4400 화면)를 놓아 SP 3000 → 'Energy, Part 4'(4800, 단계 `H`=72) |
| 4820 | click-if | `MORE` | 'Salvaging' 안내 창(4900) |
| 4920·4960·5010 | click-if·right·click | `OK`·`420,340`·`480,349` | 발사기 우클릭 메뉴(5000 화면: Salvage gains 75)에서 회수(5100: 단계 `I`=73 잠금) |
| 5400 | snapshot·session-dump | `c5400` | SP 3075, 'Mission Accomplished!' 완료 창(Leave Tutorials·Next Tutorial)으로 정지 |

발사기를 놓을 수 있는 자리는 cpp 오라클 격자 탐색(63곳, 약 10분)으로 정했다: 신전·워크샾 주변을 뺀 섬 안쪽 24곳이 놓였다(예: (340,300)~(500,380) 일대). 에너지는 신전이 만든다. 이 구간 전체가 첫 비교에서 화면·덤프가 맞았다(우클릭 메뉴 3단계·생산 창 집기·사정 표시·SP 갱신·회수·안내 창 정지 포함). 웹 시험 `e2e/l3-tutorial2.spec.js` 는 약 5분이 걸린다.

## 남은 일

Tutorial2 는 끝났다. Tutorial3~6 과 세 캠페인 묶음 미션도 같은 틀(`e2e/l3-common.js`·`tools/l3-collect.mjs`)로 시작~완료를 대조할 수 있다(미션 이름과 폴더만 바꿈).
