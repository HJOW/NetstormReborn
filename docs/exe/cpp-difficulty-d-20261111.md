# D: 11차 난이도 조절 — 쉬움/보통/어려움 (제품 1292, 2026-11-11)

마지막 디컴파일 PC 호스트명: HJOW-Athlon(변경 없음, 재디컴파일 없음). AGENTS.md(`60226ea93`) 11차 요구를 구현했다.

## 1. 요구와 구현 범위

| 항목 | 구현 |
| --- | --- |
| 옵션 메뉴 | 메인 메뉴 `Options`와 게임 중 `Options` 모두에 `Difficulty >` 하위 메뉴(`Easy`/`Normal`/`Hard`)를 추가했다(`RawGameMenus::Options`의 `compatibilityDifficulty`). 현재 값에 체크가 붙고 값이 없으면 **Normal(기본)**이다. 영어·한국어(`난이도`: 쉬움/보통/어려움)·일본어(`難易度`)를 번역표(`assets/lang/*/xlat.*`)에 넣었다. |
| 저장 | 설정 키 `difficulty`(1쉬움/2보통/3어려움, 없음·0·그 밖의 값은 보통). 선택은 Tell 구간 `Patch1101.SetDifficulty(값)`(`ProductPresentation.cpp`의 Patch1101 객체)이 `<$Config,difficulty=값>`으로 쓰고 `options.cfg`에 저장된다. |
| 보통 | 현행과 **바이트 단위 동일**(`ApplyAiDifficulty`가 입력과 같은 복사본을 돌려준다). |
| 쉬움 | AI 설정의 다리 그리기 주기(`+0x220` BridgeDrawRate)·물건 갱신 주기(`+0x224` StuffRefreshRate)·이동 간격(`+0x228` TimeBetweenMoves)을 **4배**로 늘리고, 채취 유닛 수(`+0x234` Collectors)가 1 이상이면 **절반(내림), 최소 1**로 줄인다. 원래 0이면 늘리지 않는다. |
| 어려움 | 세 주기를 **절반**(2배 빠름)으로 줄인다. 채취 유닛 수는 그대로다. |

주기 필드의 근거는 원문 AI 설정 파서(`004c2f30`대: `ai%sBridgeDrawRate`/`StuffRefreshRate`/`TimeBetweenMoves`/`Collectors`)와 AI Debug 메뉴(`aiBridgeDrawRate` 0/1/2/6, `aiTimeBetweenMoves` 2/3/4)다. 다리 건설 주기는 +0x220(브레인 +0x600에 복사), 유닛 생성 컨트롤은 팩토리 생산·채취 이동 간격인 +0x228과 배치 갱신 +0x224로 해석했다. 주기가 0 이하(예: 다리 주기 0)이면 배율로 바꿀 수 없어 그대로 둔다.

## 2. 적용 위치와 범위

- 계산: `o/AiDifficulty.{h,cpp}`(순수 함수, 설정 600BYTE의 복사본에만 배율 적용).
- 연결: `RawAiCreate::Create`가 `AiCreateHooks::difficulty(owner)`를 조회해 **보통이 아니면 복사본으로 생성자(`00416860`)와 초기 갱신(`00414710`)에 같은 값을 넘긴다**. 입력 설정은 바꾸지 않는다.
- 실제 Session: `createHooks.difficulty`가 **적 AI에만** 난이도를 준다. 데모(`demo!=0`), 로컬 플레이어 자신, `Player+54` 동맹 비트가 켜진 동맹 AI는 보통이다. 난이도 값 공급은 `CampaignAiServices::difficulty`(클라이언트가 `difficulty` 설정을 읽음)다.
- **12차 연결점:** 난이도는 AI(owner) 단위 값으로 공급되므로 스커미시 화면의 AI별 난이도는 같은 `services.ai.difficulty(owner)`를 교체해 쓴다(옵션 난이도 대신 적용).
- 한계: 난이도는 **AI 생성 시점**에 반영한다. 게임 중에 바꾸면 이미 만든 AI는 그대로이고 다음에 만드는 AI부터 적용된다. LAN 호스트 규칙·패킷 형식은 건드리지 않았고 MP 실행은 검증하지 않았다.

## 3. 검증

| 검사 | 결과 |
| --- | --- |
| 단위 `netstorm_ai_difficulty_tests` 4건 | 통과: 값 해석/보통 불변/쉬움(주기×4·채취 절반·최소 1·0 유지·홀수 내림)/어려움(주기 절반)/크기 거부 |
| 실제 미션 `The War Begins!`(`extracted/cpp-resume-1278/dtest-*-thewarbegins/`) | 같은 AI 2의 `raw_ai_settings`: 보통 `bridge 6 / stuff 3 / moves 6`, **쉬움 `24 / 12 / 24`**, **어려움 `3 / 1.5 / 3`**, 채취 유닛 1은 세 난이도 모두 1(최소 1). 세 실행 모두 종료 코드 0, 게임·보호 파일 불변. |
| 옵션 메뉴 화면 | 영어·한국어·일본어 모두 `Difficulty >` 하위 메뉴 표시, 기본 Normal 체크, `Hard` 선택 후 다시 열면 Hard 체크(`extracted/cpp-resume-1278/opt-english`, `optp-korean`, `optp-japanese`). |

사용자 확인 잔여: 쉬움/어려움의 실제 전투 체감(AI가 느려지는지·빨라지는지)은 사제 포획/공격 시나리오를 재현해야 하므로 AI는 실행하지 않았다.
