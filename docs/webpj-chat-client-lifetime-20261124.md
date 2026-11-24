# Client 수명 동안 채팅 로그 유지 — WB04-E1320

2026-11-24, HJOW-Athlon. 사용자가 확정한 ‘원본과 같은 수명’에 따라 미션 종료 때 로그를 지우던 E1169/E1170 적응을 해제했다. 비교 기준은 `225553df`의 `RawMenuChat.inc`·`ClientChatWindowGlobals.h`다. 원본/클론 실행이나 cpppj 변경은 이번 웹 단위에 포함하지 않는다.

로그는 문자열 사본만이 아니라 StyleText의 글자·색·그림 조각 토큰과 버퍼를 가진다. `client-production-menu.js`의 `clientMenuText`가 이 저장소를 `ClientChatWindowGlobals.uiOwner`에 보관하고 다음 미션이 같은 객체를 다시 빌리게 했다. 현재 메뉴 장치와 색표는 미션마다 갱신하고, 메뉴 정리 끝에서 이전 장치 참조를 놓아 이전 Session을 붙잡지 않는다. root·게임·글꼴·화면이 다른 소유자를 섞으면 거부한다. Client를 다시 만들면 새 저장소가 생성된다.

`ClientChatWindow.close()`는 미션의 창·입력줄·도우미만 반납한다. 공유 버퍼를 파괴하거나 로그를 새로 만들지 않는다. 사용되지 않는 `resetLog()`를 제거했다. 로그를 localStorage에 저장하지 않으며 `.clear`는 기존처럼 사용자가 명시적으로 로그를 지운다.

집중 Node26 검사 34/34 통과(공유 전역·ChatWindow 계약·원본 창 fixture). Chromium 3/3 통과: 숨긴 로그/F3/색 출력, 미션 이탈→다음 미션의 같은 로그 객체·조각 머리 토큰·빨강 글자 유지→추가 줄→페이지 재시작 시 0줄, 실제 송수신·`.clear`. 두 번째 미션에서 창은 새로 만들어지고 이전 미션 장치는 null이며 오류가 없다. 기존 적응은 이 보강 e2e의 이탈 뒤 1줄 기대를 만족하지 못한다. 기록은 `webpj/extracted/webpj-e1320-{focused,browser}.log`다.

webpack 번들 3,423,493바이트로 한도 통과(`webpj/extracted/webpj-e1320-build.log`). RawMenuChat 행을 port로 올려 partial은 2행/7,941줄이다. 나머지 옛 메모의 Tab·.flash·우클릭 도움말은 각각 E1173·E1163·E1253에서 닫혔고, serial0/도우미 점멸은 단독 say 패킷 발생원에서 도달하지 않음(`f66401a24`)·파일 로그는 웹 제외 판정을 유지한다. 정기 전체 회귀는 E1316 뒤 세 번째 완료 단위이므로 이번에는 집중 검사와 실제 전환 검사를 수행했다.
