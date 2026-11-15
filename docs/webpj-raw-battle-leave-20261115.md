# 전투 이탈 통지와 상태 예약 이식 (WB04-E557)

`client/RawBattleLeave`(원문 10.78 0040aa60 52h 헤더·004a71a0 수신·004b3540 메인 복귀 예약·004b6350 공통 중단·004d803e 양수 이유 부모·004d8035 확인 관문)를 `webpj/www/core/raw-battle-leave.js`로 옮겼다.
기준선 `225553df`와 현재 cpppj 소스가 같음을 확인했다. 같은 Player 저장소·State 블록·패킷 분배 표를 빌린다. 6단계 "AI 잔여·전투 이탈" 항목의 `RawBattleLeave`에 해당한다.

## 옮긴 범위

- `initializeBattleLeavePacket`/`makeBattleLeavePacket`: 52h 통지(버전 1·타입 0x52·길이 5·0)의 앞 4바이트만 쓰고 State 3 가 쓰는 다섯째 바이트를 0으로 둔 5바이트 통지. `sendBattleLeaveForState(session)`은 로컬 9에 예약만 한다(강제 수신/flush 없음).
- `receiveBattleLeave(sender, players)`: 패킷 본문을 읽지 않고 sender(0..8)의 Player(180바이트)+80 DWORD에 비트 2를 합친다(나머지 세 바이트 보존). `registerBattleLeavePacket`은 52h를 빈 observer·버전 1/정책 0·실제 수신기로 등록한다(실제 Native 연결 수신 `canonicalReceive`가 있으면 같은 값을 빌리되 이 웹 분배기에는 없다).
- `requestMainMenuAfterLeave`: next의 첫 DWORD만 3. `prepareBattleAbortState`(004b6350): abortPending=1, 미션 후/편집/메타맵이 모두 아니고 전투이면 (결과 0이면 3) next=6, 그 밖은 결과 0·next=3, 세 블록의 +84를 0으로 지운 뒤 저장 삭제 장치(누락 시 명시 오류)로 넘긴다.
- `prepareBattleLeaveState`(004d803e, signed 양수 이유만): 원격 전투 조건(번호 확정·전투·미션 후 아님·이유 2/3 아님)이면 `remoteLeave`를 필수로 먼저 부르고, 이유 2는 직접 next=3, 이유 3/9는 선행 삭제 뒤 sourceType=0, 그 뒤 공통 중단을 실행한다(두 번째 삭제의 부분 효과도 보존).
- `dispatchBattleRetreatConfirmation`(004d8035): 떠나기 명령(0x020000c5)의 뗌은 소비, signed 이유(y)가 0 이하이면 `ConfirmRetreat` 안내만 열고(Tell 누락은 명시 오류), 양수/다른 명령은 false.
- 읽기 전용 현재 전역은 `{ value }` 상자·현재 값을 돌려주는 함수·단순 값 중 하나로 받는다.

## 시험

`webpj/tests/raw-battle-leave.test.mjs` 7건:

1. `battle-leave-x86.tsv` **16행**(header 3·receive 7·next 3·parent 3: 실제 `installNativeMainMenuState`의 전투 복귀 통지 송신 순서)·`battle-abort-state-x86.tsv` **8행**·`battle-leave-reason-x86.tsv` **22행**·`retreat-confirmation-x86.tsv` **18행**, 모두 **첫 실행 일치** — 처음 한 번은 시험 장면이 읽기 전용 전역에 단순 값을 넘긴 탓에 어긋나 모듈이 단순 값도 받도록 넓혔다.
2. 계약: 52h 등록과 로컬 9 송신, 양수 이유 계약(0 이하 거부·이유 2 직접 예약·삭제 장치 누락), 확인 이벤트 분배(뗌·다른 명령·Tell 필수).
3. 변형 확인에서 드러난 미관측 경계: 공통 중단의 전투 복귀 분기(결과 보존·next 6), 원격 처리 조건 네 가지와 이유 2/3 제외, 수신 Player 상한(물리가 충분해도 sender 9 이상 거부), 저장 삭제 장치 누락 메시지.

**변형 확인:** 상수·오프셋·분기 조건·원격 조건·결과/상태 값·확인 관문·송신 목적지 55가지를 넣어 모두 시험에서 잡혔다(처음 12가지가 살아남았고 3번 시험으로 모두 잡았다). 동등 변형은 없다.

fixture는 manifest에 `battle-leave`(16행)·`battle-abort-state`(8행)·`battle-leave-reason`(22행)·`retreat-confirmation`(18행) 네 항목으로 추가했다.

## 남은 경계

- `LocalMissionTransport` 실제 큐를 지나는 52h 수신(cpppj `BattleLeave_ActualLocalQueueAndRegistration`)과 Session 연결(`SendBattleLeaveForState`가 부르는 실제 `RawCampaignSession.SendLocalPacket`)은 세션 단계에서 한다. 원격 전투 페널티/재접속 처리(`remoteLeave`)와 MP 실행은 웹 제외이다.
