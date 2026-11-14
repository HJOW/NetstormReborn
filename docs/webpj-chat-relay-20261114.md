# webpj 채팅 중계 RawChatRelay (WB04-E449, 2026-11-14)

기준선 `1f00a25a`의 `RawChatRelay.cpp/.h`를 `www/ui/raw-chat-relay.js`로 옮겼다. 주소 준비→현재 전투 마스크 해석→서버 수신자 축소→자기 로컬 수신 순서와 같은 mutable 패킷/Client 마스크 대여를 보존한다. signed 이름 길이·본문 마지막 NUL·세 BYTE CRT atol·첫 수신자 DWORD 덮어쓰기도 원문과 같다. LAN의 다른 subscriber 중계는 선택된 지점에서 명시적인 웹 제외 오류다.

기준선 뒤 변경 `16431ec1`·`eeae45c6`은 canonical 장기 수신기의 등록/대여 및 metadata 검증이다. 일반 typed 중계 결과는 같고 3.6절 ③ 4-D의 분배표 owner 승격과 함께 반영한다. 대응표 base는 `eeae45c6`이며 canonical 연결이 완료됐다는 의미는 아니다. 이번에는 기준선의 실제 일반 등록·송신 관찰기·수신 분배를 연결했다.

원본 자료 `cpppj/tests/fixtures/chat-relay-x86.tsv` 132행의 전체 256BYTE 패킷·마스크·중단 원인·주소/수신 효과가 첫 실행부터 일치했다. 해시는 fixture manifest의 `chat-relay` 항목에 있다. 신규 검사 5건은 이 재생과 signed 주소/물리 범위·예외 뒤 선행 쓰기 유지·LAN 제외·실제 RawPacketDispatch의 등록/길이 관찰/수신·접근자를 통한 공유 마스크를 검사한다. 실제 게임/브라우저 실행은 없다.

Node v26.13.0·v22.23.3 전체 각각 **4,652건 통과, 실패 0**이다. 두 버전 병렬 실행이 지연돼 v22 첫 실행은 중단하고 단독 전체 재실행으로 통과했다. 코드 검토에서 C++ 등록의 함수 목록 값 캡처를 명시하도록 보강한 뒤 양 버전 집중 5건을 다시 통과했다(실제 전역은 각 함수가 현재값을 조회한다). 대응표 984행 오류 0이다. Node v24는 설치되지 않아 미실행이다. 앱 수명·ClientChatRuntime·RawMenuControls와의 조립 및 canonical 장기 수신기 연결은 후속이다. 실제 미션 플레이는 아직 불가다.
