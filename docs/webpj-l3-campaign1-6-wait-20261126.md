# webpj 캠페인1-6 Dissolved Alliance 진입·대기 L3 대조

- 단위: WB04-E1355, 2026-11-26, HJOW-Athlon.
- 기준선: cpppj225553df, EXE SHA-256 `5b52d96d2e53f4674a40991b78ff64c3c7e2ad8275a0a99b22d18d532b77eb46`.

브리핑100프레임과 실제 Play Mission110프레임,400·1200프레임 자연 대기를 같은 단계표로 대조한다. 아군 AI는 켜지 않고 공격·포획·희생·승리 입력을 넣지 않는다.

`campaign1-6-wait-l3/`의 단계표·출처·화면3곳·덤프3곳을 `e2e/l3-campaign1-6-wait.spec.js`에서 대조했다. 화면3곳 모두0픽셀, 덤프3곳은 환경 의존 `sp`·`player:*` 행 밖 차이0이다. 현재 로컬 brain0·시작 표식1·원본 소유자1 금액(400프레임 4000·1200프레임 4000)도 별도로 일치했다. Chromium1/1 통과(5.9m), 자료 추적Node3/3 통과. 자료8개를 등록해 누적1,211개다.

최초 자료 집중 검사는 후속 미션 자료 등록 중 SHA-256·계산 칸이 아직 없어서 형식1건이 실패했다. 모든 등록·계산 완료 뒤 같은 검사3/3이 통과했다. 실패 로그는 webpj/extracted/webpj-e1355-fixture-test-during-registration.log, 재검증 로그는 webpj/extracted/webpj-e1355-fixture-test.log로 보존했다. 제품 코드는 바꾸지 않았다.
