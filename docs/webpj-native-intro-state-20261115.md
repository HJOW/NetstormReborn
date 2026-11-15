# 인트로 State 39/40/41/42 이식 (WB04-E532)

5-C의 "메인 메뉴·타이틀 상태" 중 `client/NativeIntroState`(원문 004b6e62..004b6fa9)를
`webpj/www/core/native-intro-state.js`의 `installNativeIntroState`로 옮겼다. 실제 `RawLocalMissionState`(E320)의
`parentState`에 꽂는 부모 상태이며 기준선 `225553df`와 현재 cpppj 소스가 같음을 확인했다.

## 옮긴 범위

- State 42: 커서 숨김 → 제목 스플래시 → State 41 예약 → `immediate[35]` 0 → 제목 팔레트 복구 → 커서 표시
  (앞서 숨긴 커서/비트맵 효과는 예외에서도 되돌리지 않는다).
- State 39: `current[35]`가 있으면 cleanup → 채팅 입력 숨김 → 채팅 숨김 → fort/server/boss 참 → Core → Renderer → HUD →
  영상 경로 조회, 이름이 있으면 인트로 표식 → 영상 열기(성공 여부를 쓰지 않음) → State 40 예약, 없으면 State 41 예약.
- State 40: 영상이 있으면 `next[0] = 40`, 없으면 State 41. State 41: pending 이름 첫 BYTE만 지우고 State 3 예약.
- 39/40/41/42 밖의 상태는 장치 호출 전에 거부하며 선택하지 않은 장치는 요구하지 않는다. 공통 commit은 `RawLocalMissionState`에 맡긴다.
- fort/server/boss는 `{ value }` 상자(또는 속성 접근자)로 같은 실제 모드를 읽고 쓴다.

## 시험

`webpj/tests/native-intro-state.test.mjs` 2건:

1. 원본 관측 `native-intro-state-x86.tsv` **17행 전부**: 실제 `RawLocalMissionState`와 장치 배선을 만들어 종료 종류(menu/stop/normal)·
   다섯 144BYTE 상태 블록 전체·모드·pending 이름 16BYTE·효과 순서를 대조했다. **첫 실행에서 모두 일치**했다.
2. 범위 밖 상태 거부, 준비 없는 State 39의 State 41 예약, State 40의 `next[0]`, State 41의 이름 BYTE 삭제와 물리 없음 거부,
   준비된 State 39의 선택된 장치 누락 거부(모드 쓰기 전).

`webpj/tests/fixtures/manifest.json`에 `native-intro-state` 항목(원본 10.78, 17행)을 추가했다.

## 남은 경계

- cpppj의 두 번째 시험(인트로 → 메뉴 State 3 → Poll 4)은 `NativeMainMenuState`(5-C의 남은 단위) 이식 때 대조한다.
- 실제 영상 장치(`MovieGump`·`MoviePlayback`, 8단계)와 Client 연결은 아직 없다. 앱 수명에는 연결하지 않았다(PORT_MAP `port`).
