# 1015 — 실제 프레임의 연결 감시와 자동 서버 이전

2026-11-07 HJOW-Athlon. 마지막 전체 정밀 디컴파일은 같은 호스트의2026-11-03이다. 원문10.78 004b9010/4b6560/40ae00/4d0600과 실제 Client call439ad8/439ae3, PE 상수/분기표를 대조했다. 004b9010/4b6560은 정밀본의CD대응 없음이며 MessageGump의CD대응004c8b40(strong0.297)은 참고이고 구현 판본은10.78이다.

현재phase29/33이면서11연결 객체가 없으면 자연 반환한다. phase14 이외는 실제 wall+1을005c8620에 쓴다. phase14 만료에서는64h/1/5BYTE·끝payload0을 같은 실제 서버9 또는 현재 참가자 broadcast로 보내고, 송신 뒤 현재 server를 다시 읽어 새 wall+3(서버)/+10(client)을 쓴다. 이 기한과 실제00531908을 ClientConnectionActivityGlobals가 두 활동표와 함께 장기 소유한다. PE초기DWORD6과 실제 옵션 해석의1 store를 구분했다. 새 사용자 옵션/DNS/외부 연결은 추가하지 않는다.

그 뒤 현재owner/프레임 고정wall을 캡처한다. 서버는 현재8행 flags1·signed활동 초 차>15·비영활동frame·자기owner제외·실제BattleFinished 거짓 순서로 kind2 안내를 낸다. client는 phase+84가0일 때만 서버활동 차>8·비영frame·미종료를 안내한다. 실제9객체부재 또는 초차>20·unsigned하위DWORD프레임차>20이며 비영수신frame/AutoRestart일 때 같은004b6560을 호출한다. 해당 부모는 flags8행마다 flags1/40을 지우고100을 세운 뒤 기존 실제976BYTE writer를 호출한다. 저장 복귀 뒤1014의 RequestBattleReconnect(false)로 이어지며 같은 상태/Player/현재 이름/전송 owner를 사용한다. 다음World교체는 여전히 다음 Tick이다.

원문kind1/2 MessageBoard/MessageGump를 같은 생성자·현재Player/이름주소·경고음·번역/Style/Fit에 연결했다. kind1의 PE문자열은 `~wServer not responding`이며 kind2의8색은 b/r/w/u/m/y/l/o다. 동맹색g와 달리4번u를 보존한다. 원문처럼두 종류는기한0/자동Idle등록없음이며 새64h수신의 기존 Remove가 같은 종류/Player/parameter0을 지운다. 별도 경고창/타이머/정지를 만들지 않는다.

Client의 영상 갱신 잠금 없는 가지에서 SaveOptions 뒤/Ambient·Wind 앞439ad8에 실제감시를 연결했다. 실제Client의 로그인 타이머는 수신 Pump에서 떼어 같은439ae3 위치로 옮겼다. 독립 Session만 기존 Pump 편의 호출을 유지하고 실제Client는 같은 타이머를 한 번 호출한다. Editor에는 Login을 만들지 않는다. 이 변경을 일반 프레임486bc0/전체 MP 송신·디코더 완료로 쓰지 않는다.

첫 빌드의 없는 RawSurfaceStorage.processState 한 원인/C2039·C2664 오류2를 같은 실제 Gameplay.Authority.server 참조로 교정했다. 두 번째 빌드에서 메타 연결 부재의 bool 자연 반환 누락/C2561 오류1을 false 명시로 교정했다. 최종 Release 제품 빌드 종료0·경고/오류0이다. 제품11.01은11,893,248바이트, SHA-256 `22cbc1b8dcd2254f3e42db60103daf4f7695918e822658934739f3429a8b17dc`다. 보고서는 `cpppj/connection-health-frame-20261107.json`이다. 신규 MP/단위/게임/통신 검사·fixture 생성/실제 장애 저장0이며 보호2800파일 불변이다. 실제 자동 장애 재현·LAN 안정성 검증은 수행하지 않았다.
