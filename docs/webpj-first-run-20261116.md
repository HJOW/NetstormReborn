# 웹 최초 실행 상태 재반영 (WB04-E616)

원본의 무옵션 최초 실행 분석과 cpppj1314 확정 커밋 `bd2941f4`를 웹에 반영했다.
기존 `soundVolume=3`, `musicVolume=2`는 옛 사용자 값이므로 최초 기본값으로 사용하지 않는다.
`Settings`의 두 fallback과 `AudioSystem`의 두 기본 단계를 모두5로 교정했다.
저장 형식/버전을 바꾸거나 기존 localStorage 설정·진행을 초기화하지 않는다.

## 대조 범위

- [원본 최초 실행](exe/original-first-run-20261116.md)·[cpppj1314](exe/cpp-first-run-1314-20261116.md)가 근거다.
- 기준선 `225553df` 뒤 cpppj 변경은 ClientAudio.cpp +16, ClientAudio.h +12/−3,
  ClientMain.cpp +3/−14, UberGump.cpp +8/−5다. 음량 기본값/옵션 표시 교정은② 재반영했다.
  `ReadClientAudioOptions` 함수 추출 자체는①이다. 웹은 이미 Settings의 같은 값을
  옵션 표시와 오디오에 전달한다. PORT_MAP의 세 대응 행 base를 `bd2941f4`로 올렸다.
- 기존 Welcome→Welcome2 인라인 Config 저장·시작 Tips·`$Checked` 해석은 유지했다.
  브라우저 검사로 이 부품들이 실제 앱 수명에서도 연결되는지 확인했다.
- 해상도·전체화면·Edit·MP·폐기된 온라인 검증을 웹에 추가하지 않았다.

## 검증

`webpj/e2e/first-run.spec.js`는 Playwright가 새로 만든 독립 Chromium 저장소에서 실행한다.
내부 상태는 관찰에 사용하고 안내/메뉴/음량 선택은 실제 Canvas 마우스 입력으로 진행한다.

1. 두 localStorage 키가 없는 상태에서 클릭 시작→Welcome(OK)→Welcome2를 확인한다.
   Welcome2의 두 버튼과 Main Menu를 누르기 전 `welcomeGiven=1` 저장을 확인한다.
2. 소리·음악·바람·가장자리 스크롤·Auto-Demo·팁 ON, 스피커 교환 OFF,
   두 음량 하위 메뉴에서 Volume5만 체크됨을 확인한다.
3. UI로 효과음3·음악2를 선택하고 Struggle For Freedom을 연다.
   첫 행만 활성·나머지5행 잠김·전부 체크 없음이며 잠긴2번 클릭은 무반응이다.
4. 검사 입력 `DoneTheWarBegins=1`을 저장한 뒤 페이지를 다시 연다.
   클릭 시작→Tips 분기, 사용자 음량3·2 유지, 자기 완료 체크/다음 행 해금과 나머지 잠금을 확인한다.
   이 입력은 저장 수명 검사이며 실제 미션 완료/승리를 재현한 것으로 세지 않는다.
5. 처리되지 않은 브라우저 예외0이다. 새 브라우저 검사1건이 통과했다.

최초 집중 검사는35건 중33건 통과·2건 실패였다. 한 검사는 새 기본값5와 같은 값5를
쓰기 때문에 저장이 생기지 않았고, 다른 검사는 음악2로 촬영한 기존 스크린샷에
새 기본값5를 비교했다. 변경을 검증하는 입력은3으로, 스크린샷 입력은 명시적인
음악2로 교정했다. 기존 스크린샷·원본 fixture·제품의 저장 동작은 바꾸지 않았다.
브라우저 검사의 첫 실행은 검사 코드가 없는 `settings` 공개 getter를 참조해 실패했으며
기존 `_settings` 관찰로 교정한 뒤 통과했다.

2026-11-17 최종 Node v22.23.3·v26.13.0 전체는 각각5,906건 통과(실패·취소·건너뜀0, 종료0)했다. v24는 기존 지시대로 생략·호환 가정이다. Chromium 전체2/2 통과·대응표 오류0·git diff --check 통과다.
실행 로그는 `extracted/webpj-e616-node22.log`, `webpj-e616-node26.log`,
집중 최초 로그는 `extracted/webpj-first-run-focus.log`다. 새 fixture는0개다.

## 남은 경계

실제 미션 완료 효과/시나리오 승리·전체 동일성·청취 검증은 이번 범위가 아니다.
현재 세션 부품의 앱 수명 연결은 여전히0이며 실제 미션 플레이도 미완료다.
다음은 4-D D 생성자1815 Transmission 합성의 선행 `RawDaisProcessData` 이식이다.
공통/DN 인계·cpppj·AGENTS·보호 원본/CD·README 개요는 수정하지 않았다.
