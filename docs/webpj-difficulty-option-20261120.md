# 옵션 메뉴 Difficulty(난이도) (WB04-E1096)

메인 메뉴 Options와 ESC 게임 메뉴 Options에 `Difficulty` 하위 메뉴(쉬움·보통·어려움)를 더했다. 계산(`ai-difficulty.js`: 쉬움은 적 AI의 다리·갱신·이동 주기 4배와 충분한 SP에서 채취 유닛 절반[최소 1], 어려움은 주기 절반)과 새 미션의 적 AI 생성 때 설정을 읽는 연결(`campaign-client-services.js`의 `ai.difficulty`)은 E554·E555에서 이미 있었고, 이 단위가 사용자가 고르는 메뉴와 저장을 완성한다.

## 구현

- **선택지.** [difficulty-option.js](../webpj/www/core/difficulty-option.js)의 `difficultyChoices(설정값)`이 Easy(1)·Normal(2)·Hard(3)을 만들고 설정이 없거나 범위 밖이면 보통에 체크한다(`aiDifficultyFromValue` 재사용). 라벨은 번역 대상이며 한국어는 쉬움·보통·어려움, 제목은 난이도다. `parseDifficultyTarget`은 Tell 대상 `Patch1101.SetDifficulty(1|2|3)`을 해석한다.
- **저장.** 선택은 `_openTell`이 창 없이 설정 `difficulty`(정수, 기본 0=보통)에 쓰고 저장한다. 설정 층은 같은 값을 게임 데이터에 옮기므로 적 AI 서비스가 읽는 `difficulty` 설정과 같다. 적용은 새 미션의 AI 생성 때이며 진행 중인 미션의 AI에는 영향이 없다(cpppj와 같다).
- **메뉴.** 메인 메뉴 Options의 `Language`·`Frame Rate` 다음, ESC 메뉴 Options의 같은 위치에 `Difficulty`를 둔다(원본 비교용 `includeAll`에는 없음). 설정 키가 15개가 됐다.

## 검증

- 새 `tests/difficulty-option.test.mjs` 3건(선택지·Tell 대상·메뉴 항목과 설정 연동). 설정·메뉴 구조 변경으로 `popup`·`korean`·`frame-rate`·`language-choice` 시험과 `first-run.spec.js`·`frame-rate.spec.js` 기대를 갱신했고 한국어 번역표에 `Difficulty`·`Easy`·`Normal`·`Hard`를 더했다.
- 새 `e2e/difficulty.spec.js`(Chromium): 기본 보통 체크 → 어려움 선택(저장과 게임 데이터 값 3) → 다시 열면 어려움 체크 → 쉬움 선택(값 1); 한국어 화면에서 `옵션 ▸ 난이도`가 쉬움·보통·어려움으로 보이고 저장된 어려움에 체크가 붙는다.

## 남은 경계

실제 AI 미션에서 난이도별로 AI의 다리 건설·생산 주기와 채취 유닛 수가 달라지는 것을 N프레임 동안 비교하지 않았다(계산과 서비스 연결은 Node 시험으로 확인). 스커미시(12차)의 AI별 개별 난이도는 스커미시 화면 구현 때 같은 설정 키 체계(`skirmishAi<n>Diff`)로 이어진다.
