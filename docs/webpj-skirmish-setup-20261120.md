# 스커미시 설정·화면·시작 자료 규칙 (WB04-E1101)

AGENTS.md 12차(스커미시)의 순수 규칙을 웹으로 옮겼다. cpppj `client/SkirmishSetup.cpp`·`SkirmishStartIslands.inc`의 설정 저장, Tell 대상 해석, 화면 본문, 시작 대본과 시작 섬 선택, 빈 월드 fort 생성을 [skirmish-setup.js](../webpj/www/core/skirmish-setup.js) 한 모듈로 만들었다. 이 단위는 DOM·설정 저장소·세션에 의존하지 않는 계산만 담고, 화면과 월드에 붙이는 일은 뒤 단위(E1102 이후)에서 한다.

## 구현

- **설정.** `createSkirmishSettings`는 AI 1명·모두 보통·적·금지 없음이다. `loadSkirmishSettings(get)`/`saveSkirmishSettings(set, settings)`는 cpppj와 같은 키(`skirmishAiCount`, `skirmishAi2Diff`~`skirmishAi8Diff`, `skirmishAi2Ally`~, `skirmishBans`)로 왕복하며, 범위 밖 AI 수는 1..7로 줄이고 알 수 없거나 중복인 금지 이름은 버리고 5개까지만 받는다. 저장소가 `undefined`를 줘도 기본값이다.
- **Tell 대상.** `parseSkirmishTarget`은 `Patch1101.Skirmish`·`Count`·`Ai(n)`·`Bans(page)`·`Set(...)`·`Start`를 대소문자 무시로 읽고 인자는 쉼표로 나눈다. 모르는 이름은 `none`이다. `applySkirmishAction`은 `count`·`diff`·`ally`·`ban`(같은 항목을 다시 고르면 해제, 최대 5개, 후보 밖 번호는 무시)·`clearbans`를 적용하고 다음에 보일 대상(`Patch1101.Skirmish`, `…Ai(n)`, `…Bans(n)`)을 돌려준다.
- **화면 본문.** `buildSkirmishScreen`이 첫 화면(AI 수·AI별 `난이도 / 진영`·금지 개수·Start/Cancel(UCampaign으로 복귀))과 AI 수 고르기, AI 한 명의 난이도·진영, 금지 유닛(쪽당 12개, `[X]/[ ]`·내부 이름 병기, 다음/이전 쪽, Clear all, 5개가 차면 안내 문구)을 Tell 문법 문자열로 만든다. 문구는 영어와 한국어(언어 7), 일본어(언어 5)가 있고 금지 유닛 이름은 번역 함수가 받는다. 일본어는 13차에서 다시 점검한다.
- **시작 자료.** `buildSkirmishLaunch(settings, seed)`는 사람 1명 + AI N명(최대 8명)의 시작 섬을 22개 후보에서 서로 다르게 뽑고, 참가자 수로 맵 크기(2명=1, 3~4명=2, 5명 이상=3)를 정하고, 3×3 영역 패턴 중 하나와 회전·씨앗으로 중앙섬 기록 6바이트(`[패턴|회전<<6, 0x09, 0, 씨앗, 0, 0]`)를 만들며, AI별 이름·기술·시작 돈·동맹 목록과 승패 문구가 있는 미션 대본을 쓴다. 난수는 표준 mt19937(`Mt19937`)이고 번호표 섞기는 뒤에서부터의 Fisher–Yates다. cpppj는 `std::shuffle`을 쓰므로 같은 씨앗의 정확한 섬 배치는 다를 수 있다(무작위 배치라 허용, 대신 구현 순서를 시험으로 고정했다).
- **빈 월드 fort.** `makeSkirmishEmptyWorldFort`는 섹션 35개 가운데 Chaff(4)·Territory(6)·Terr 00~19(15~34)만 비우고(빈 청크 256개/영역 표 0/버전만) 나머지(기술·덱·돈)는 기준 fort 그대로 둔 새 바이트를 만든다. 서명이 `F`가 아니거나 섹션 길이가 2 미만·초과면 길이 0의 결과다.

## 검증

- 새 `tests/skirmish-setup.test.mjs` 9건: cpppj `SkirmishSetupTests`의 대상 해석·동작·왕복·대본·빈 fort·화면 시험을 옮겼고, mt19937 표준 기준값(씨앗 5489의 첫 값 3,499,211,612와 10,000번째 값 4,123,659,995), 시험 쪽에서 독립으로 다시 계산한 난수 소비 순서(번호표 섞기 21회 → 패턴 → 회전 → 씨앗), 200개 씨앗에서 시작 섬 8개가 항상 서로 다름과 모든 후보가 뽑힘, 중앙 패턴이 항상 3×3임을 더했다.
- 변형 확인 20여 종(문구 언어 분기·AI 수 하한·금지 개수 상한 두 곳·맵 크기 경계·섞기 범위·패턴 선택 범위·중앙섬 플래그·동맹 `!== 0`(동맹 동작과 저장 읽기 두 곳)·쪽 범위 줄임·이전 쪽 조건·섹션 길이 하한·영역 표 채움·`{n}` 치환·32비트 범위·난수 소비 순서)이 모두 시험 실패로 잡힌다. ASCII 소문자화의 `[A-Z]`→`[A-Za-z]` 변형만은 소문자를 다시 소문자로 바꾸는 동등 변형이다.

## 남은 일 (12차 나머지)

- **E1102 UI 연결.** Campaign 목록(`UCampaign`)에 Skirmish 항목을 Back 앞에 한 번만 넣고(cpppj `ProductTellBody`), Tell 대상 `Patch1101.Skirmish*`를 가로채 이 모듈의 화면을 보이고, 설정을 브라우저 저장소에 왕복한다.
- **시작 섬 fort 번들.** 후보 fort 12개(FWOS9·Imp1·Imp3·LCA·Razorianwar6·Thundere·TpVpath·school5·sh3_n_2·sw_m2·tnron42·tnron51)는 `netstorm.tarc`의 32개 fort에 없고 `originals/d`에 있으므로 `prepare-assets`로 웹 자료에 추가해야 한다.
- **월드 생성.** `RawIslandPartition`·`RawCentralIsland`·`RawBattleIsland`(이식 대상, 이 모듈의 `islands`·`central`·`mapSize`가 입력)와 무작위 간헐천, 빈 월드 fort로 하는 미션 시작(`BeginMission("Skirmish")`)을 Session에 연결하고 AI 난이도/동맹이 실제 AI에 적용되는지 N프레임으로 확인한다.
