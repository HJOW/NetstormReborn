# LAN 재접속 상태17·18과 실제 서버 인수 — 단위1369

2026-11-26, HJOW-Athlon. 마지막 디컴파일 PC도 HJOW-Athlon이다. 기존 C의004b6dd0과 보호 PE4b7cd5/4b7cf3 정적 근거(`state-17-static-1367.json`)를 재사용했다. 새 디컴파일·원본 실행/수정은 없다.

State17은 원문 레벨1d 로그 `Reconnection accepted` 뒤 즉시14를 요청한다. State18은 `Reconnection denied` 로그 뒤 sourceType0/next3을 쓴다. 둘 다 기존144BYTE 공통 확정을 사용한다. 새 결과 로그 콜백을 현재 Client의 실제 진단 장치에 연결했다. 메뉴3은 기존 상위 부모가 다음 Poll에 처리하며 로그인 전용 Poll이 가로채지 않는다. 거절을 성공으로 바꾸거나 새 ACK를 만들지 않는다.

기존 집중 대상 `netstorm_cd_presence_tests.exe`에 추가한 수락·거절·로그 누락/예외3사례와 기존 상태/시계 관련 경계를 포함해14/14 통과했다. 전체 다섯 상태 블록의 stale DWORD 보존·로그가 관측하는 이전 phase/출처·즉시14 순서·거절의 다음3 예약·예외 전 부분 쓰기를 확인했다. 실제 거절 패킷 실행으로 세지 않는다.

실제 TCP 두 참가자 시작 후 봇이 자기 호스트만 강제 종료했다. 상대는15→16→17→14를 진행하고 공유 서버/boss 권한이0/0에서1/1로 바뀌었다. 인수 뒤 로컬owner2/PlayerKnown1·7814객체·추가120프레임 실행·상대 정상 종료0을 확인했다. 호스트 종료1은 의도한 장애 주입이며 정상 종료로 세지 않는다. [인수 뒤 상대 화면](../screens/cpp-lan-reconnect-1369-20261126/bot.png).

첫 인수 실행도 상태17/14·권한1/1·상대 종료0에 도달했으나 종료 전 stderr에서 수락 문구를 요구해 봇 판정은 실패했다. 최종 봇은 표준 `tracelevel=29`를 선택하고 정상 종료의 CRT flush 뒤 실제 `Server takeover`/`Reconnection accepted` 로그를 확인했다. 제품 로그 수준/필터는 바꾸지 않았다. [봇 사용법](../testing/cpp-lan-opponent-guide.md). `raw_network_role` 보고 행은 같은 장기 권한의 두 값을 읽기만 한다.

Release 성공. EXE SHA-256 `84caa62941047c1234e157e2eb918ce4670d39f819591a09d3c84af0daf58e5f`. 보호3212파일/기존 fixture 불변. [검증 JSON](../../cpppj/lan-reconnect-1369-20261126.json). 원시는 `extracted/cpp-lan-1364`의 `build-reconnect-1369.log`, `tests-reconnect-1369.log`, `bot-takeover-1`/`2`다. 최신 ZIP1362에는1363~1369가 아직 없다.

다음1370은 단순 생산/건설과 개별 객체·SP 변경의 실제 동기화다. 이탈 뒤 새 참가자 재접속·최종 ZIP도 남았다. 물리 LAN·포획/희생·특정 공격/승리·명시 사용자 확인은 실행하지 않았다. DN 분배기에도17/18이 없어 InvalidState로 가는 정적 차이를 [DN-MP-04](../dotnet-cpp-lan-followup-20261126.md)에 반영했다. DN 코드/실행/진행률은 그대로다.
