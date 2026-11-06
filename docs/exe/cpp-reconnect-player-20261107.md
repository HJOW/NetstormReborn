# 1009 — 실제63h 재접속 수신과 참가 순서/객체 정리

2026-11-07, HJOW-Athlon. 마지막 전체 정밀 디컴파일은 같은 호스트의2026-11-03이다. 10.78의004a7510/0040b380·004a6dc0/004a6920/004a6ab0/004a6f70·0040eec0/004ac200을 정밀 소스/보호 PE와 대조했다. CD 대응은004c18e0(strong0.578), observer0040b7a0(weak1.0), CheckLogin004c17f0, Reply004c0f30, Joined004c10e0, Ready004c1e30, TYPE flags004abae0이며 구현 판본은10.78이다.

기존 RawMissionLogin/같은 Player1620BYTE에 version1/정책0/고정8BYTE의63h 수신을 등록했다. observer40b380은 빈 함수라 새 효과를 추가하지 않는다. 원문의 Reconnecting 로그 뒤 owner1..8에서 payload subscriber를 찾으며 subscriber0/현재 flags에 임의 관문을 추가하지 않는다. 미발견에서는 현재 checking일 때 실제 표현식0/Serverboard.cpp1b2 진단 뒤 반환한다. 이 식/파일/행은 PE4a7565..757d의 실제 인자다.

발견한 owner는 기존 실제 CheckLogin/선택 응답을 사용한다. CheckLogin 거부 뒤에도 원문은 후반을 계속하므로 새 조기 반환을 넣지 않았다. 현재 자기 subscriber이면 해당 flags18을 켜고 같은 Joined가 ID/주소·flags3·전체 Player metadata를 전송한다. 실제 fortCount/lastLoginPlayer를0으로 만든 뒤 참여 flags2 기록을 세며 PE의 앞서 선택한 owner번호와 다음+5c의 signed 비교를 유지한다. 이를 최대 예측카운터 비교로 정규화하지 않았다.

같은 Session은 client SID5..14999를 고정 순서로 방문한다. flags11의7이 없고 TYPE>=70이며 현재+40 bit1인 객체는 원문0059ab50/번호×23의 정수 비용을 기존 TypeCostUnits로 복호화해 실제 로컬 SP/AI 지갑/표시 부모에 환불한다. 그 효과 뒤 현재 TYPE+ec flags4000/현재+40 bit8·현재 가상+80 signed양수를 다시 조회해 보존 여부를 정하고, 그 밖에는 실제 현재 가상+10/인자0의 World.Destroy를 호출한다. 별도 SID 반납/가짜 count/자기 목록 사본을 만들지 않는다.

정리 뒤 현재 로컬 Player flags40을 켜고 기존 실제 Ready6e를 대상에게 전송한다. 현재 Subscribers가1이거나 Subscribers-Disconnected가1이면 같은 Begin을1로 만든다. 끝4437f0은 빈 함수다. 모든 조회/지갑/TYPE/가상/Player/전송은 기존 실제 장치를 쓰며 별도 성공 목록을 만들지 않았다.

현재63h 등록/전체 수신 몸체·선택 checking 꼬리를 연결해 checking 잔여는6(부모2/부분1/MP3), 완료부모40/부분5=45+잔여6/기준51로 갱신했다. 메타맵 두 VT+20/004b02d0·Descend004dc220·004b8cb0·나머지MP는 계속 잔여다. **State15 부모/63h 송신·StateReconnect/ForceServerDeath 입력·실제 서버 선택/복구State7 인계는 아직 남는다.** 수신 연결을 실제 서버 이전/재접속 동작 완료로 쓰지 않는다.

첫 Release는 log 콜백 선언 누락 한 원인의C2039/C2672 두 오류로 종료1이었다. 선언과 실제 로그 연결을 보완한 최종 Release는 종료0·경고/오류0이다. 제품11.01은11,855,360바이트, SHA-256 `49a7fecc29fb731509751e6a96f5b72d100048a5c3f3f11c2ea2c239525769bd`다. 보고서는 `cpppj/reconnect-player-20261107.json`이다. 신규 MP/단위/게임/통신 검사·fixture 생성0이며 보호2800파일은 모두 불변이다. 실제63h/객체 삭제/환불/사제·승리 조건을 실행한 결과가 아니다.
