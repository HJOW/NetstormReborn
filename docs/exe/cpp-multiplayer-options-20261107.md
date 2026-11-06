# 997 — Multiplayer Options 실제 공장 연결

2026-11-07, HJOW-Athlon. 마지막 전체 정밀 디컴파일은 같은 호스트의2026-11-03이다. 10.78의004cc4d0/004cc970/004ccf30/004cfc80과10.72의004c9350/004c98b0/004c73e0을 대조했다. PE 정적 해석은 `extracted/cpp-resume-997/options-x86.txt`와 `options-factory-x86.txt`에 보존했다.

250×145 옵션 창의 기본 Gump, 장식/제목·네 정보 행·상대 화살표·Connect/Cancel/Sub ID 돌 버튼을 기존 같은 저장소·문자 편집·트리/serial/모달 수명으로 조립했다. PE에서 확인한 signed GT61과 최초 Attach mode2, 하단 정렬/깊이·각 Attach 끝의 Unnamed 이름 입력 예약 순서를 반영했다. 원문에 있는 미부착 텍스트 입력 자식은 표시하거나 새 소멸 정책을 덧붙이지 않았다. 그 생성자가 this를 문자열로 읽는 부분은 VT00515c88의 NUL 전3BYTE로 한정해 재현한다. 갱신은 공통 Gump의 dead10000/파괴중4 상태를 확인한다.

실제 State6 결과0의 옵션 공장을 연결했다. 저장 게임 선택은 기존 LoadMultiplayer 목록/ReadFort 경로, 이름 변경은 실제 ChangeFortSubscriber, 주소 입력은 기존 HTML Input/편집 노드를 사용한다. 원문 b9/c4/95 명령은 같은 일반 입력 큐로 넘긴다. 서버 선택은 폐기된 공식 서버 목록 대신 명시 로컬 TCP/IP 주소를 사용자 설정에 저장하는 호환 확장이다. 프로토콜은 요구된 TCP/IP로 고정하고 구 SPX/IPX 선택을 비활성화했다. 외부 서버/클라이언트 유효성 검사는 연결하지 않는다.

Connect의 실제 일반 State7/LAN 시작 소비자와 메인 메뉴 Multiplayer 진입은 아직 남는다. 미연결 소비자는 해당 지점에서 명시 거부하며 연결/Ready 성공값을 만들지 않는다. 일반 전투 전체·roster/파생 writer/decoder·메타/저장/재접속/Outpost/checking8은 계속 진행할 잔여다.

첫 Release 빌드는 optional<string_view> 인수의 C2664 한 건으로 종료1이었다. 명시 string_view로 교정한 최종 빌드는 종료0·경고/오류0이다. 제품11.01은11,688,960바이트, SHA-256은 `41d0f1cd9bd8c659b677c41876fc91d4c4523c80fd8120ab5e4fae332b58158c`다. 보고서는 `cpppj/multiplayer-options-20261107.json`이다.

신규 MP/단위/게임/통신 검사와 fixture 생성은 모두0이다. 보호2800파일은 모두 불변이다. 이 코드 연결·정적 대조·컴파일 결과를 옵션 창 실제 화면이나 LAN 동작 검증으로 세지 않는다.
