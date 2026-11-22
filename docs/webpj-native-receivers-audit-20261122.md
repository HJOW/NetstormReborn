# 고정 Native 패킷 수신기 네 묶음의 웹 등록 대조 (WB04-E1193)

재기준선 뒤 대응표에 `todo`로 남아 있던 `NativeLoginPacketReceivers`(15종)·`NativeGameplayPacketReceivers`(26종)·`NativeCarrierPacketReceivers`(15종)·`NativeConstructionPacketReceivers`(8종) — 원문이 Client 장기 기본 표에 설치하는 고정 Native 수신기 64행 — 가 웹에서 어떻게 대신되는지 실제 실행으로 대조했다.

## 방법

1. cpppj 네 파일의 수신기 표(`{종류, 원문 진입 주소, 선언 길이, observer}`) 64행을 정규식으로 추출했다.
2. 임시로 `core/raw-packet-dispatch.js`에 진단을 넣고(대조 뒤 되돌림) 실제 Chromium 에서 Tutorial2 를 시작해 분배기 등록 표를 읽었다(타입 1..137 중 수신기가 있는 종류와 `length`·`version`·`observer`·`packetObserver`).
3. 게임 조작이 많은 기존 e2e 20건(Tutorial2 자연 진행·건설·사제 이동·객체 명령·송금/동맹·채팅·유닛 메뉴)을 같은 진단으로 돌려 "수신기 없는 패킷 종류(10 이상)가 들어오면 오류 기록"이 한 번도 나오지 않음을 확인했다.

## 결과

- **등록:** 64종 모두 웹 분배기에 수신기가 등록돼 있다(실행 중 등록 표 70종).
- **선언 길이·버전:** 64행 모두 원문 표의 선언 길이와 일치하고 버전은 1이다.
- **observer:** 원문의 observer 는 `Empty`(표식만 있는 빈 observer) 대부분과 실제 변형을 하는 일곱 종류(`Login`·`Client`·`Path`·`Developer`·`Construction`·`Effect`·`Timer`·`Meltdown`)이다. 웹은 `Empty`와 일부(Login 0x27·Effect 0x5f·Meltdown 0x75·Timer 0x7d)를 분배기에 observer 로 등록하지 않는다. 실제 변형이 있는 네 종은 가변 길이 패킷의 송신 때 선언 길이를 확정하는 일이며, 웹은 송신 지점이 대신한다: Meltdown 은 `raw-campaign-session-meltdown.js`가 `RawMeltdown.finalizePacket`을, Timer 는 `raw-campaign-session-transmission.js`가 `finalizeBombTimerPacket`을 호출한다. **Effect(0x5f)·Login(0x27)의 송신 확정은 웹 송신 지점에서 찾지 못했다**(단독 로컬 수신은 선언 길이 `0xffff` 가변이라 파싱에 영향이 없고 e2e 오류도 없었다).
- **결론:** 등록·길이·버전은 대응이 확인됐고 observer 네 종만 확정되지 않아 네 행을 `todo`에서 `partial`(웹 모듈 표기)로 올렸다. 완료로 세지 않으므로 남은 양은 그대로다(118단위·29,882줄).

## 남은 점

- Effect·Login observer 의 송신 확정이 단독 웹에서 필요한지(폭탄 효과 송신 패킷, 로그인 패킷의 선언 길이)를 원본 관측과 대조해 닫는다.
- 점검에 쓴 진단은 임시 코드였고 되돌렸다. 같은 점검이 필요하면 `RawPacketDispatch.prototype.validate` 래핑(console.error 한 줄)으로 다시 만들 수 있다.
