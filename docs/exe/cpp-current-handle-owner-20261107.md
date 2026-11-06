# 1011 — 현재 사용자 이름의 Client 수명

2026-11-07, HJOW-Athlon. 마지막 전체 정밀 디컴파일은 같은 호스트의2026-11-03이다. 10.78의PTR_DAT_00540ca8과004b2df0/004b54e0/004b8cb0을 대조했다. 일반 World 정리004b2df0은 현재 이름을 지우지 않고, 저장/재접속은 같은 전역 이름 포인터를 읽고 쓴다. 실제 Client의 이름이 Session 지역 배열이면 World 교체에서 이 수명을 잃으므로 후속 재접속 caller 전에 소유자를 바로잡았다.

ClientSubscriberGlobals가 기존 두 Subscriber DWORD와 별도로 같은 현재 handle64BYTE를 소유한다. 새 Client에서만 처음 빈 이름으로 구성하고 복사/이동 금지는 유지한다. 실제 RawCampaignSession은 기존 services.subscriberGlobals의 같은 배열 참조를 빌리며 해당 서비스를 쓰지 않는 독립 Session만 기존 지역64BYTE를 소유한다. 서비스 선언은 참조보다 먼저이며 shared owner가 모든 소비자보다 오래 산다. 원본32비트 할당 주소/ABI 자체 일치는 주장하지 않는다.

Fort Subscriber Load/이름 변경·미션 설치·현재 이름 조회/포획 안내·일반 로그인·저장 writer 등 기존 소비자는 같은 handle 참조를 사용하므로 각각의 호출/쓰기 위치를 바꾸지 않았다. Session 생성/반납에서 Client 이름을0으로 만들지 않으며 일반 State7의 기존 정리 전 이름 인계/범위 검사도 유지한다. 새 설정 읽기/이름 복사 동기화나 Player 이름을 현재 이름 대신 사용하는 경로를 추가하지 않았다.

**StateReconnect/ForceServerDeath/004b8cb0의 실제 caller와 복구State7 선택/인계는 아직 남는다.** 이 수명 보완을 실제 재접속 동작 검증 완료로 쓰지 않는다. 일반 프레임486bc0/스펠·전체 decoder·메타·Outpost/checking6·한국어 후속·Edit 시나리오 UI도 이어간다.

Release 제품 빌드 종료0·경고/오류0이다. 제품11.01은11,859,968바이트, SHA-256 `1c828e3c564b9bc3e3a688585f55befac317934dd346557d9211750ee4507ccc`다. 보고서는 `cpppj/current-handle-owner-20261107.json`이다. 신규 MP/단위/게임/통신 검사·fixture 생성0이며 보호2800파일은 모두 불변이다. 실제 이름 변경/미션 재진입/재접속을 실행 확인한 결과가 아니다.
