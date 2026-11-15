# 메인 메뉴 State 3 이식 (WB04-E534)

5-C의 "메인 메뉴·타이틀 상태" 중 `client/NativeMainMenuState`(원문 004b6dd0의 State 3 선택 부모)를
`webpj/www/core/native-main-menu-state.js`로 옮겼다. 기준선 `225553df`와 현재 cpppj 소스가 같음을 확인했다.
`RawLocalMissionState`의 `parentState`에 꽂는 부모이며 호스트 메뉴 표시와 Native State 공통 확정을 분리한다.

## 옮긴 범위

- `installNativeMainMenuState`: current가 3이 아니면 거부 → current/immediate/next의 DWORD 33을 0 → `intro` 설정이 0이면
  State 39 예약과 `immediate[35] = 1`(같은 DWORD)로 확정 → 공통 앞단(출처 정규화)·배경 첫 BYTE 0 → 승패(victory/defeat + 음악) 또는
  titleMenu 배경 → outcome 0 → 출처가 있으면 저장 삭제·fanfare·출처 배경 → 이전 Battle14 통지 → 공통 준비·HUD1 →
  pending/현재 Fort 이름 첫 BYTE 0 → 제목 갱신 → current가 3이 아니면 `changedState` 결과를 그대로 반환(Guide/안내/tail 중복 없음) →
  Guide 소멸 → Welcome 또는 Blank(+startup이 거짓이고 firstMenu가 참일 때 Tips, 참조 모드만 UserNotUpdated) → firstMenu 0 →
  편집이면 editorPlayers → tail → Commit.
- `readNativeFortEntrySource`(State 1/3 공통 앞단, signed 이전 phase ≤ 6 또는 11이면 출처 0), `notifyNativeFortEntryLeave`
  (현재 battle과 이전 phase 14일 때만, 호출 순간 재조회), `prepareNativeEditorEntry`(current 1/2의 출처/첫 BYTE/통지; 웹은 Edit 제외라
  소비자가 없으나 cpppj 계약 보존을 위해 옮김).
- 제품은 서비스 종료한 인터넷 검증(Not Validated 안내)을 쓰지 않는다(AGENTS.md). `referenceValidationNotice`는 원문 대조 장치만 true이다.

## 시험

`webpj/tests/native-main-menu-state.test.mjs` 3건:

1. 원본 관측 `native-main-menu-state-x86.tsv` **82관측**(41 입력 × 두 제어 워드): 종료 종류·다섯 144BYTE 상태 블록 전체·현재 DWORD 여섯·
   현재/pending/배경 이름 전체·활동 기한 double·외부 호출 순서를 실제 `RawLocalMissionState`와 함께 대조했다. 공통 tail
   (기존 Editor checkpoint 뒤의 HUD·주변음 기한·음악·팔레트·next 4, `ExistingEditorAfterCheckpoint`는 Edit라 웹 제외)는 시험 장치로
   같은 28줄 순서를 재현했다. **첫 실행에서 82관측 전부 일치**했다.
2. 공통 앞단의 signed phase 판정(−1·6·11·7·12)·32비트 출처 보존·Battle14 통지의 호출 순간 재조회와 장치 누락 거부.
3. Editor 진입 앞단의 current 1/2 계약과 State 3 부모의 current 3 계약 거부.

`webpj/tests/fixtures/manifest.json`에 `native-main-menu-state` 항목(원본 10.78, 82관측)을 추가했다.

## 남은 경계

- 인트로 → 메뉴 State 3 → Poll 4의 결합 시험(cpppj `NativeIntroStateTests` 두 번째)은 이 모듈과 E532를 함께 써서 재현할 수 있으나
  실제 메뉴 표시 장치(`tail`)가 필요해 5-C 메뉴 조립에서 대조한다. 앱 수명에는 연결하지 않았다(PORT_MAP `port`).
