# 스커미시 Campaign 항목·설정 화면·설정 저장 연결 (WB04-E1102)

E1101에서 옮긴 [스커미시 규칙](webpj-skirmish-setup-20261120.md)을 앱에 연결했다. Campaign 목록에 Skirmish 항목이 생기고, 누르면 원본 Tell 창과 같은 부품으로 설정 화면(AI 수·AI별 난이도/동맹·금지 유닛·시작/취소)이 열리며, 선택은 브라우저 저장소에 남는다. 아직 월드 생성이 연결되지 않아 **항목은 기본으로 꺼져 있고** 주소에 `?skirmish`를 붙이거나 앱 옵션 `skirmish: true`를 줄 때만 보인다. 월드 생성까지 연결한 단위에서 기본값을 켠다.

## 구현

- **Campaign 항목.** [product.js](../webpj/www/assets/product.js)의 `productTellBody`가 `skirmish` 옵션이 참일 때 `UCampaign` 본문의 첫 `$Button=` 앞(없으면 끝)에 `$Menu=Skirmish,Tell,Patch1101.Skirmish` 한 줄을 한 번만 넣는다(한국어는 `스커미시`). cpppj `ProductTellBody`와 같다. `GameData`가 `skirmish` 옵션을 받아 `presentTellBody`에서 전달한다.
- **Tell 섹션 틀.** Patch1101 설정 객체(`productCreditsText`)에 `[Skirmish]`·`[SkirmishCount]`·`[SkirmishAi]`·`[SkirmishBans]` 자리 틀을 더했다. Tell의 `객체.섹션` 조회가 섹션을 찾아야 창이 열리기 때문이며(cpppj `kPatchCredits`와 같다), 실제 본문은 아래에서 바꿔 끼운다.
- **화면 본문.** `createTellWindow`에 `bodyFor(대상 전체 이름, 제품 표기를 적용한 본문)` 훅을 더했다. 앱의 `_skirmishScreenBody`가 대상이 첫 화면·AI 수·AI 한 명·금지 유닛이면 현재 설정과 언어(영어 0·한국어 7), 메뉴 번역표(금지 유닛 이름)로 `buildSkirmishScreen` 본문을 돌려준다. 같은 HTML 창·메뉴·버튼 경로를 그대로 쓴다.
- **설정 변경과 시작.** `_openTell`이 `Patch1101.SkirmishSet(...)`를 창 없이 처리한다: 설정을 읽어 `applySkirmishAction`을 적용하고 저장한 뒤 다음 화면(`…Ai(n)`, `…Bans(n)`, 첫 화면)을 다시 연다. `Patch1101.SkirmishStart`는 시작 함수(`options.startSkirmish`, 없으면 안내만 남기고 시작하지 않음)를 부르며, 시작하지 못하면 단추가 닫은 설정 첫 화면을 다시 열어 화면 밖으로 떨어지지 않게 한다.
- **설정 저장.** [settings.js](../webpj/www/platform/settings.js)에 `skirmishAiCount`·`skirmishAi2~8Diff`·`skirmishAi2~8Ally`·`skirmishBans` 16개 키와 새 종류 `text`(영숫자·밑줄·쉼표만, 200자 이하)를 더했다. 기존 `localStorage` 항목에 키가 없으면 기본값(AI 1명·보통·적·금지 없음)이므로 버전은 올리지 않았고, Reset All은 항목 전체를 지우므로 그대로 같이 초기화된다.

## 검증

- 새 `tests/skirmish-ui.test.mjs` 4건(실제 메뉴 자료): 제품 본문의 Skirmish 항목(켠 판본만·한 번만·첫 버튼 앞·버튼이 없으면 끝·한국어), 실제 Campaign 목록에서 Skirmish를 눌러 설정 화면이 열리고 AI 수 3·AI 2 어려움/동맹·금지 유닛 2개를 마우스 좌표로 고르면 설정 저장소에 쓰이고 새 앱이 그 값을 읽음, 시작 단추가 현재 설정으로 시작 함수를 부르고 취소가 Campaign 목록으로 돌아감, 시작 함수가 없으면 시작하지 않고 설정 화면을 다시 엶, 꺼진 판본은 항목이 없고 한국어는 번역된 화면이며 금지 유닛 이름이 번역 함수를 거침. `tests/popup.test.mjs`의 설정 정의 시험은 31개 키·`text` 종류 경계를 확인하도록 고쳤다.
- 변형 확인 15종(중복 방지·켠 판본 조건·버튼 없을 때 위치·Patch1101 자리 틀·설정 저장·화면 종류 목록·언어 번호·본문 대체 훅·시작 분기·시작 실패 시 재표시·text 정규식·길이 상한·GameData 전달 등)이 시험 실패로 잡힌다. 앱이 `GameData.build`에 `skirmish`를 넘기는 한 줄은 Node 시험이 `GameData`를 직접 만들어 가려지므로 Chromium 시험이 확인한다.
- 새 `e2e/skirmish.spec.js` 3건(Chromium): 기본 판본의 Campaign 목록에는 Skirmish가 없음; `?skirmish`에서 Skirmish → 설정 화면 → AI 수 3·AI 2 어려움/동맹·금지 유닛 2개 → 저장소 값(`skirmishAi3Diff=3` 등) → Start(시작 전 안내만 남기고 설정 화면 유지) → Cancel로 Campaign 복귀 → 다시 열면 저장된 설정이 보임; 한국어(`?lang=ko`)의 `스커미시` 항목·`인공지능 수: 1`·`시작/취소` 버튼.

## 남은 일 (12차 나머지)

- 시작 섬 fort 12개 번들 추가(E1103), `RawIslandPartition`·`RawCentralIsland`·`RawBattleIsland` 이식과 빈 월드 fort 미션 시작·무작위 간헐천·AI 난이도/동맹 적용(E1104~), 항목 기본값 켜기.
- 한국어 금지 유닛 이름의 실제 번역표 값 확인(현재는 번역 함수를 거치는 것까지만 시험하고, 표에 없는 이름은 영어 그대로 나온다).
