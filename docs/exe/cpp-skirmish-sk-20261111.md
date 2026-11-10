# SK: 12차 스커미시 모드 — Campaign 설정 화면과 생성 미션 (제품 1293, 2026-11-11)

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
| 랜덤 섬 배치·간헐천 | **간헐천:** 생성 대본에 `randGeysers=1`(멀티플레이어의 전체 간헐천 생성 `GenerateWorldGeysers`)·`moreGeysers=1`. **섬 배치:** 기존 다인용 fort 가운데 소유자가 1~N+1이고 AI N명 대본이 있는 맵(`SkirmishMapList.inc`, 318개: AI 1명 68·2명 47·3명 65·4명 50·5명 22·6명 16·7명 50)에서 **무작위로 하나를 고른다.** → 아래 한계 참고. |
| 설정 저장 | `skirmishAiCount`/`skirmishAi{2..8}Diff`·`...Ally`/`skirmishBans`를 설정 파일에 저장한다(다음 실행에도 유지). |

## 2. 시작 흐름(생성 미션)

1. `BuildSkirmishLaunch(설정, 난수)`가 AI 수에 맞는 맵 이름과 **미션 대본(english)**을 만든다: `missionType="Tutorial"`, `myStartMoney=5000`, `myTech="all"`, 금지가 있으면 `techAllowed="deny;타입;..."`, `myAllyList`(플레이어 편 AI), AI 2..N+1의 `Name/Tech/StartMoney/GeyserAttachments/Collectors/StuffRefreshRate/TimeBetweenMoves/BridgeDrawRate/AllyList`(같은 편끼리 동맹 → 플레이어 편 대 적 편), 브리핑 `Go!`, `[Succeeded][BadTeamDead]`·`[Failed][GoodTeamDead]`(승패 구역).
2. `UberGump::StartSkirmish`가 고른 맵의 fort를 기존 읽기 장치로 읽고, 대본·fort를 `Skirmish` 이름의 **가상 파일 reader**(`skirmishSource_`)로 만든 뒤 `BeginMission("Skirmish")`를 부른다. 원본/사용자 폴더에는 아무 파일도 쓰지 않는다.
3. `CreateMission`이 `skirmishRun`을 1로 두어(다른 미션은 0) AI 생성 때 `AiCreateHooks::difficulty`가 AI별 난이도를 쓴다.

## 3. 한계(사용자 결정 필요)

- **섬 배치는 "무작위 맵 선택"이다.** AGENTS의 "멀티플레이어 게임처럼 랜덤 배치"를 만족하는 진짜 아키펠라고 생성(플레이어 영토 배치·중앙 섬)은 MP 세계 생성 경로(`RawCentralIsland`·플레이어 fort 로드)에 묶여 있고 MP 실행 검증이 불가하여 연결하지 않았다. 후보 맵은 기존 캠페인/튜토리얼 지도라 일부는 미리 지어 둔 기지가 있다. 필요하면 진짜 랜덤 생성으로 교체한다.
- 후보 맵 표는 기본 게임 자료(`originals/d`)에서 만든 것이며 사용자 커스텀 맵은 포함하지 않는다.
- 금지 유닛의 영어 이름 일부(블로커/바리케이드/지상·공중 유닛의 속성 짝)는 게임 안 표기를 이름 규칙에서 추정했다. 효과는 내부 타입 이름으로 적용되므로 영향이 없고, 틀린 이름은 화면에서 내부 이름(`- windBattery`)으로 구별한다.
- 금지가 AI의 건설에도 적용되는지(전역 사용표)는 실제 전투로 확인하지 않았다.

## 4. 검증

| 검사 | 결과 |
| --- | --- |
| 단위 `netstorm_skirmish_setup_tests` 6건 | 통과: 대상 해석/동작 적용(최대 5개)/저장 왕복/시작 대본(AI 수·동맹·금지·같은 난수 같은 결과)/화면 본문(영·한·일)/Campaign 항목 |
| 실제 조작(영어, `extracted/cpp-resume-1278/sk3/`) | Campaign에 `Skirmish` 항목 표시 → 설정 화면 → AI 수 3 → AI 1을 `Hard`+`Ally`, AI 2를 `Easy`로 → 금지 `Outpost` 선택(`Banned units: 1/5`) → `Start` → 브리핑 `Go!` → **미션 `Skirmish`(Tutorial) 전투 진입**, 종료 코드 0·게임 폴더 불변. |
| 실제 AI 값(`raw_ai_settings`) | 소유자 2(Hard·Ally): bridge 0.5·stuff 0.5·moves 1.0 / 소유자 3(Easy): 4·4·8, 채취 유닛 6→3 / 소유자 4(Normal): 1·1·2, 채취 6. 기본값 1/1/2(생성 대본)에서 AI별 난이도가 옵션 난이도 대신 적용됨. |
| 한국어·일본어 | 설정 화면(`스커미시`/`スカーミッシュ`)과 일본어 Credits가 모두 표시된다. 일본어는 공백이 없어 자동 줄바꿈이 안 되므로 `<br>`로 짧게 끊어 오른쪽 잘림을 없앴다(`extracted/cpp-resume-1278/skp-*`·`cr-japanese`). |

## 5. 시작 중 발견·교정한 결함

- `HtmlParser`는 `>`/`<` 문자를 태그로 읽어 `Next page >`가 `Next page`로 보이고 `< Previous page`는 사라질 위험이 있었다 → 쪽 이동 라벨에서 꺾쇠를 뺐다.
- 첫 시작에서 종료: 미션 fort 적재가 `client_.Files().ReadOnly(path)`로 **디스크 stat**을 호출해 가상 `Skirmish.fort`에서 원문 Basefile.cpp 진단 1166을 던졌다 → 가상 fort는 읽기 전용으로 취급(`IsSkirmishFortFile`).
