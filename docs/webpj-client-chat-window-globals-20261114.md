# webpj 채팅 창 장기 소유자 ClientChatWindowGlobals (WB04-E453, 2026-11-14)

기준선 `1f00a25a`의 `ClientChatWindowGlobals.h`를 `www/core/client-chat-window-globals.js`로 옮겼다. 실제 `ChatWindowState`·`StyleTextBufferState`·`ChatNameCompletionState`·무시 목록·수신 마스크/뉴스 시각·입력줄 전환·UI 대여 표식·문자열 양방향 표를 같은 Client 수명에 둔다. 창/Gump 교체가 하위 상태를 초기화하지 않는다. JS getter는 하위 객체의 같은 참조를 빌리며 객체 교체는 거부한다(원문 subobject 주소 보호의 웹 표현).

`store`는 빈 문자열과 NUL 뒤 바이트도 전체 키로 구별한다. 최초 토큰 `0x71000000`부터 같은 내용은 재사용하고 `0x7ffffffe` 다음에 새 값만 거부한다. 한도에서도 기존 값 조회는 가능하며 실패 시 표를 바꾸지 않는다.

## 기준선 뒤 판정

`8efc76eb`·`3f766a97`까지 읽었고 base는 `3f766a97`이다. 후속 `newsKey` 128BYTE와 `AllianceRequestState`를 **② 상태 정의로 반영**했다. 후자는 `raw-player-participation.js`에 상태 클래스만 추가했다. counter9/clock9쌍/initialized는 같은 112BYTE BSS를 사용하며 PE 설정의 signed 초기값 60초/3회는 별도로 둔다.

`UberGump.cpp`의 일반 `HandleAllianceRequestInput` 앞에 단독 관문이 없으므로 이름만 보고 웹 제외로 단정하지 않았다. 입력 helper·협상 수신·실제 단독 도달 판정은 기존 3.6절 ③ 6단계 잔여다. State19 뉴스 읽기/쓰기와 현재 뉴스 수신의 소비자 연결도 ClientChatRuntime/입력 조립에서 판정하며, 이번 생성자는 네트워크·Config 읽기나 송신을 실행하지 않는다.

## 검사와 경계

전용 PE fixture와 cpppj 직접 검사는 없다. 신규 소스 계약 검사 **5건 통과**: 실제 하위 상태 종류/BSS·연속 clock 물리·DWORD 감김·창 교체 뒤 주소/값 유지·빈/NUL 문자열·마지막 토큰/실패 효과·실제 RawChatRelay와 같은 마스크 공유를 확인했다. 기존 Player 참여 원본 608+288행을 포함한 집중 **15건도 통과**했다.

Node v26.13.0·v22.23.3 전체 각각 **4,678건 통과, 실패 0**이다. 대응표 984행 오류 0, fixture manifest 검사도 통과했다. Node v24는 설치되지 않아 미실행이다. 실제 Client/RawMenuControls 조립과 게임 실행은 후속이며 실제 미션 플레이 완료가 아니다.
