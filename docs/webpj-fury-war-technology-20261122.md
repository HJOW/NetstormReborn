# Fury War 원소 기술 지급(CheatAllTechnology) 연결 (WB04-E1189)

## 발견

영어 대본 82개를 모두 읽어 스크립트 명령을 찾던 중 `bc8fury76.english`(Priest Training의 **Fury War**)가 `<$CheatAllTechnology,원소>`를 쓰는 것을 확인했다. 웹 앱의 스크립트 명령 처리(`_runScriptCommand`)는 `config`·`warning`만 알고 이 명령은 "아직 구현되지 않았습니다" 안내만 남겨, **원소 선택 창(Wind/Rain/Thunder/Sun)에서 골라도 기술이 하나도 지급되지 않았다.**

- `[@1]` 원소 선택 창의 단추가 `[W]`·`[R]`·`[T]`·`[S]` 섹션을 열고, 각 섹션 본문이 `<$CheatAllTechnology,0..3>`(0 바람·1 비·2 천둥·3 태양)이다.
- AI 사제 희생 안내 `[ai2priestdead]`~`[ai5priestdead]`("His tech is now yours!")도 같은 명령으로 상대 원소 기술을 준다.
- 82개 대본 중 이 명령을 쓰는 곳은 이 파일뿐이다(`GetTechnology`·`TechnologyAll` 등 다른 기술 명령은 대본에 없다).

## 원본 동작(cpppj)

보호 명령표 `00542798`의 `CheatAllTechnology`(0x02000028)가 입력 큐로 들어오면 `UberGump`가 `ApplyThemeTechnology(world.Session(), event.x)`를 부르고 생산 메뉴를 다시 만든다(`rebuild_`). `ApplyThemeTechnology`는 Session 의 타입 표·현재 기술 부모·내용물 검색/생성을 `RawThemeTechnology`(원문 `00492ae0`)에 빌려 선택 원소의 기술을 타입 0 부터 차례로 만든다(group 10·theme 5 제외, 이미 가진 기술 건너뜀).

## 웹 구현

- `core/get-technology-input.js`: `CHEAT_ALL_TECHNOLOGY_COMMAND`(0x02000028)와 `applyThemeTechnology(session, theme)`를 더했다. Session 의 `core().types`·`currentTechnologyParent()`·`priests().carrier.findContained`·`createContained`를 `RawThemeTechnology` 훅에 연결한다(이미 이식돼 있던 지급 몸체를 쓴다).
- `netstorm.js`: `_runScriptCommand`가 `cheatalltechnology`를 받아 `_applyThemeTechnology(원소)`를 부른다. 미션 월드가 없으면 안내만 남기고 지급하지 않는다(원본은 logic_error). 지급 뒤 다음 프레임의 생산 내용물 갱신(`updateProductionContents`)이 생산 패널을 새로 채우며 화면을 다시 그린다.
- 시험: `tests/apply-theme-technology.test.mjs` 5건(가짜 Session: 명령 번호, 타입 순서·건너뛰기 규칙, 재지급 무변화, signed theme, 경계 누락 거부), `e2e/fury-technology.spec.js`(Chromium: Fury War 를 시작해 원소 선택 창이 뜨고 Rain 선택 전 비 기술이 없다가 선택 뒤 비 원소 기술 7종이 모두 생기고 다른 원소는 그대로. 명령 처리를 끄면 `Expected 7 / Received 0`으로 실패함을 확인한 변형 시험 포함).

## 남은 점

- 실제로 AI 사제를 희생시켜 `ai?priestdead` 안내가 뜨는 흐름은 자연 플레이가 필요해 확인하지 않았다(같은 명령·경로를 쓴다).
- `GetTechnology`(0x0200009f) 입력은 대본에 쓰이지 않아 앱에 연결하지 않았다.
