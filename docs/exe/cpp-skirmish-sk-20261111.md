# SK: 12차 스커미시 모드 — Campaign 설정 화면, 생성 미션, MP 규칙 월드 (제품 1293 → MP 규칙 월드 1300, 2026-11-11)

마지막 디컴파일 PC 호스트명: HJOW-Athlon(변경 없음, 재디컴파일 없음). AGENTS.md(`60226ea93`) 12차 요구를 구현했다.

## 1. 요구와 구현

| 요구 | 구현 |
| --- | --- |
| Campaign 메뉴에 Skirmish 추가 | 원문 Campaign 메인 목록(Tell 대상 `UCampaign`)의 마지막 항목 앞(Back 버튼 앞)에 `Skirmish`(한국어 `스커미시`, 일본어 `スカーミッシュ`)를 넣는다(`ProductTellBody`). |
| 별도 UI | 원문 HTML 창(Tell)과 같은 부품으로 **자체 구현**한 네 화면: **설정**(`Patch1101.Skirmish`), **AI 수**(`...SkirmishCount`), **AI별 설정**(`...SkirmishAi(소유자)`), **금지 유닛**(`...SkirmishBans(쪽)`). 본문은 `client/SkirmishSetup`(`BuildSkirmishScreen`)이 설정값으로 매번 생성하고 영어·한국어·일본어 문구를 갖는다. |
| 적 AI 수 | 1~7명(소유자 2~8). 설정 화면에는 선택한 수만큼 AI 줄이 나온다. |
| 금지 유닛(선택, 최대 5개, Outpost 포함) | 27개 후보(발전기·발사기·캐논·블로커·바리케이드·공중 유닛과 생산 건물·지상 유닛·Bulf·**Outpost**). 항목을 고르면 `[X]`로 표시하고 같은 항목을 다시 고르면 해제한다. **6번째는 무시**하고 5개가 되면 안내 문구가 나온다. `Clear all` 제공. 내부 타입 이름을 같이 보여 짝이 틀린 영어 이름이 있어도 구별된다. |
| AI별 난이도와 동맹 | 각 AI마다 난이도(Easy/Normal/Hard, 옵션 난이도 대신)와 진영(Enemy/Ally)을 고른다. 난이도는 11차의 AI 단위 값(`services.ai.difficulty(owner,enemy)`)을 쓰며 **스커미시 실행 중에는 동맹 AI도 자기 난이도**를 쓴다(`skirmishRun`). |
| 시작/취소 | `Start`: 설정으로 미션 대본과 fort를 만들어 로딩 뒤 바로 시작한다. `Cancel`: Campaign 메뉴로 돌아간다. |
| 랜덤 섬 배치·간헐천 | **멀티플레이어 게임 시작과 같은 규칙**(2026-11-11 사용자 지시: "멀티플레이어에서 플레이어들이 준비하고 게임 시작하면 섬과 간헐천이 랜덤으로 배치"). 섬은 §2의 MP 월드 생성 순서로, 간헐천은 `randGeysers=1`로 MP의 전체 간헐천 생성기(`GenerateWorldGeysers`, 옵션 None/Random/Even 규칙·예산)가 만든다. 원문 예산(`WorldGeyserBudget`)은 전투 모드가 아니면 참가자 수를 0으로 세어 예산이 0이므로 스커미시는 **사람+AI 참가자 수**(`CampaignAiServices::skirmishPlayers`)로 같은 공식을 쓴다(12차 신규 연결). |
| 설정 저장 | `skirmishAiCount`/`skirmishAi{2..8}Diff`·`...Ally`/`skirmishBans`를 설정 파일에 저장한다(다음 실행에도 유지). |

## 2. 시작 흐름(생성 미션 + MP 규칙 월드)

1. `BuildSkirmishLaunch(설정, 난수)`가 **미션 대본(english)**과 월드 계획을 만든다. 대본: `missionType="Tutorial"`, `myStartMoney=5000`, `myTech="all"`, 금지가 있으면 `techAllowed="deny;타입;..."`, `myAllyList`(플레이어 편 AI), AI 2..N+1의 `Name/Tech/StartMoney/GeyserAttachments/Collectors/StuffRefreshRate/TimeBetweenMoves/BridgeDrawRate/AllyList`(같은 편끼리 동맹), `randGeysers=1`. 월드 계획: 시작 섬 N+1개(후보 표 `SkirmishStartIslands.inc`에서 서로 다른 것을 무작위로 뽑음), 무작위 중앙섬 기록(3x3 패턴·회전·지형 씨앗), 맵 크기 옵션(2명=1·3~4명=2·5명 이상=3, 링 간격 4/5/6칸).
2. `UberGump::StartSkirmish`가 시작 섬 fort를 읽고(원본 `d/*.fort`), 첫 fort에서 **섬 없는 빈 월드 fort**(`MakeSkirmishEmptyWorldFort`: Chaff/Territory/Terr00..19를 비움)를 만들어 대본과 함께 `Skirmish` 이름의 **가상 파일 reader**(`skirmishSource_`)로 둔 뒤 `BeginMission("Skirmish")`를 부른다. 원본/사용자 폴더에는 아무 파일도 쓰지 않는다.
3. 미션 `Start`(`RawCampaignMissionStartBindings::Start`)가 사제를 찾기 **전에** `prepareWorld` → `RawCampaignSession::BuildSkirmishWorld()`를 부른다(일반 미션은 계획이 없어 아무 일도 하지 않음). MP 로그인과 같은 순서다:
   1. **군도 밀도**: MP `InitializeDensity`와 같은 규칙(밀도 옵션 최소/최대 표에서 반값 범위를 두 번 뽑아 더함). 섬 기록이 100개뿐이라 참가자가 5명 이상이면 5~8%로 낮춘다(실측: 8명 맵에서 5%=섬 66~76개, 10%·15%=91개로 8번째 섬이 100개 한계에 걸림).
   2. **중앙섬 먼저**(`BuildCentralIsland`: 원문 진단 "섬 목록이 비어 있어야 함"과 같은 순서): 원점 배치, 지형 성장, 군도 섬 생성.
   3. **참가자별 전투섬**: 지역 Template 슬롯(owner)에 시작 섬 fort를 읽고 `BuildBattleIsland(owner)`가 방향표 링(`PlayerBounds().Placement`: 소유자 1~8 → 남·북·서·동·대각선 순, 간격 3~6칸)에 놓고 지형을 키운 뒤 그 영역의 객체를 만든다. MP 규칙대로 객체 소유자는 항상 참가자이고(`objectState.mission` 끔), 원본 Chaff는 다시 읽지 않으며(`chaffMode` 끔), 영역 일치 판정이 소유자까지 비교한다(`finishState.chaffMode` 끔: 켜져 있으면 다른 참가자의 같은 번호 영역까지 일치로 봐 객체 읽기가 넘친다). 소유자 후처리(`RawFortFinishOwner::Finish`)는 거주 구역이 없는 참가자에게 거주 구역을 무작위로 놓는다(MP 전투 규칙; 세션의 `finish`에 실제 배치기를 연결).
   4. 미션 모드 네 곳은 끝나면(예외 포함) 되돌린다.
   5. 이어서 `Start`가 사제로 AI를 만들고 `randGeysers`로 간헐천을 만든다.
4. `CreateMission`이 `skirmishRun`을 1로 두어(다른 미션은 0) AI 생성 때 `AiCreateHooks::difficulty`가 AI별 난이도를 쓴다.

## 3. 한계(사용자 결정 필요)

- **시작 섬 모양은 후보 표(22개)에서 고른다.** MP에서는 참가자가 자기 fort의 전투 영역을 가져오지만 스커미시에는 개인 fort가 없어, 원본 fort 중 "사제 1명만 있는 단일 소유자 3x3 이하 영역"을 후보로 썼다(`tools/cpp_skirmish_start_islands.py`가 `--dump-forts` 결과에서 만든다). 즉 섬 **위치**(방향표 링)·**중앙섬/군도**·**간헐천**은 MP 규칙이지만 시작 섬의 **모양**은 이 22개 중 무작위다. 지형 성장으로 외곽은 매번 달라진다. 더 다양한 시작 섬이 필요하면 후보 기준을 넓힐 수 있다.
- 방향표는 원문 고정표(소유자 1~8 → 4/0/6/2/7/3/5/1)라 사람 플레이어는 항상 남쪽(소유자 1)이다. MP도 소유자 번호가 위치를 정한다(접속 순서가 정하던 번호를 스커미시는 고정).
- 금지 유닛의 영어 이름 일부(블로커/바리케이드/지상·공중 유닛의 속성 짝)는 게임 안 표기를 이름 규칙에서 추정했다. 효과는 내부 타입 이름으로 적용되므로 영향이 없고, 틀린 이름은 화면에서 내부 이름(`- windBattery`)으로 구별한다.
- 금지가 AI의 건설에도 적용되는지(전역 사용표)는 실제 전투로 확인하지 않았다.

## 4. 검증

| 검사 | 결과 |
| --- | --- |
| 단위 `netstorm_skirmish_setup_tests` 6건 | 통과: 대상 해석/동작 적용(최대 5개)/저장 왕복/시작 대본(AI 수·동맹·금지·같은 난수 같은 결과)/화면 본문(영·한·일)/Campaign 항목 |
| 실제 조작(영어, `extracted/cpp-resume-1278/sk3/`) | Campaign에 `Skirmish` 항목 표시 → 설정 화면 → AI 수 3 → AI 1을 `Hard`+`Ally`, AI 2를 `Easy`로 → 금지 `Outpost` 선택(`Banned units: 1/5`) → `Start` → 브리핑 `Go!` → **미션 `Skirmish`(Tutorial) 전투 진입**, 종료 코드 0·게임 폴더 불변. |
| 실제 AI 값(`raw_ai_settings`) | 소유자 2(Hard·Ally): bridge 0.5·stuff 0.5·moves 1.0 / 소유자 3(Easy): 4·4·8, 채취 유닛 6→3 / 소유자 4(Normal): 1·1·2, 채취 6. 기본값 1/1/2(생성 대본)에서 AI별 난이도가 옵션 난이도 대신 적용됨. |
| **MP 규칙 월드(1300, `extracted/cpp-resume-1278/sk-mp1`·`sk-mp2`·`sk-mp3`·`sk-z*`)** | 3명 AI(4명): 중앙섬 생성 뒤 SID 약 27700/32768·섬 약 65개, 시작 섬 4개(`Skirmish: owner N <- fort TerrK` 로그), 객체 사제 4·거주 구역 4·간헐천 50, 약 90초 동안 AI가 다리를 0→246조각 건설. 7명 AI(8명): 5~8% 밀도로 10회 연속 성공(섬 62~76개, SID 약 25000 남음). 1명 AI(2명) 성공. 일반 미션(`thewarbegins`)도 진입·대기·종료 정상. 밀도 15% 이상에서 8명이면 섬 기록 100개 한계로 실패해 밀도 규칙을 넣었다. |
| 한국어·일본어 | 설정 화면(`스커미시`/`スカーミッシュ`)과 일본어 Credits가 모두 표시된다. 일본어는 공백이 없어 자동 줄바꿈이 안 되므로 `<br>`로 짧게 끊어 오른쪽 잘림을 없앴다(`extracted/cpp-resume-1278/skp-*`·`cr-japanese`). |

## 5. 시작 중 발견·교정한 결함

- `HtmlParser`는 `>`/`<` 문자를 태그로 읽어 `Next page >`가 `Next page`로 보이고 `< Previous page`는 사라질 위험이 있었다 → 쪽 이동 라벨에서 꺾쇠를 뺐다.
- 첫 시작에서 종료: 미션 fort 적재가 `client_.Files().ReadOnly(path)`로 **디스크 stat**을 호출해 가상 `Skirmish.fort`에서 원문 Basefile.cpp 진단 1166을 던졌다 → 가상 fort는 읽기 전용으로 취급(`IsSkirmishFortFile`).

## 6. MP 규칙 월드 연결 중 발견·교정한 결함(1300)

- 영역 표 읽기를 `RawFortWorldLoad`로 하면 생성자가 모든 훅을 요구해 `Fort World 실제자료/필수child/진단연결누락`으로 종료 → `ReadTerritory`와 같은 규칙(스트림 위치에서 120바이트)으로 직접 읽는다.
- 객체 읽기가 `Fort Objects 직접 BYTE 물리범위`로 종료: 세션이 미션 모드(`chaffMode`)라 영역 일치 판정이 소유자를 비교하지 않아 같은 영역 번호를 쓰는 앞선 참가자의 섬까지 일치로 봤다 → MP 전투 규칙(비Chaff)으로 바꾸고, 이때 호출되는 소유자 후처리의 실제 배치 훅이 세션에 비어 있던 것(`SessionMissing`)을 연결.
- 8명 시작에서 `IslandRuntime Add 원본100기록경계`: 중앙섬+군도가 섬 기록 약 91개까지 차서 → 5명 이상은 군도 밀도 5~8%.
- 약 5% 확률로 `지역 프레임 quartile 난수 범위 오류`로 종료 → 알려진 버그 NB23(드문 섬 생성 종료)의 실제 원인으로 확인해 교정([상태 문서](cpp-known-bugs-status-20261111.md) NB23).
