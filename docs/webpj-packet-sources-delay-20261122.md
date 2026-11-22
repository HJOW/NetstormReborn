# 패킷 원천 장부·지연 큐의 웹 범위 판정 (WB04-E1187)

재기준선 뒤 `todo`로 남아 있던 두 네트워크 단위를 단독 웹 기준으로 판정했다.

## ClientPacketSources(cpppj `client/ClientPacketSources.cpp`) — 웹 대체로 닫음

- **원문 역할:** 도착한 패킷 노드가 어느 실제 Source(TCP·UDP·Composite 내부 끝·Editor·Meta) 장치에서 왔는지 weak 장부(`Resolve`)로 찾고, 배송 동안 그 장치를 `devicePin`(shared_ptr)으로 붙잡으며, 앱 종료 때 남은 listener 를 닫는다(`Shutdown`).
- **웹:** 단독 웹의 전송 장치는 `LocalMissionTransport`(Composite 내부 끝) 하나뿐이라 `Resolve`로 고를 경로가 없다. 장치 수명은 JS 참조(GC)가 대신하므로 `devicePin`·weak 장부는 필요 없고(`core/client-connection-globals.js` 머리 ①·② 판정), 소켓 listener 가 없어 `Shutdown`의 listener 닫기도 해당 없다. 대응표 행은 `adapt`/`port`로 닫고 웹 모듈은 `client-connection-globals.js`·`local-mission-transport.js`로 적었다.

## RawPacketDelay(cpppj `o/RawPacketDelay.cpp`) — 멀티플레이 전용 제외

- **원문 역할:** 실제 TCP/UDP 연결에서 수신 패킷을 `lag` 옵션(1·2·3) 분포와 난수로 지연시키는 FIFO 큐(원문 지연 함수의 signed 분포·ring 확장).
- **판정:** cpppj `ClientCampaignTransport.cpp:322`의 주석대로 **내부(단독) 미션에는 이 큐를 설치하지 않는다**(재진입한 TCP/UDP 에만 쓰임). 웹 `core/client-campaign-transport.js`도 이미 이 훅을 "웹 제외"로 기록하고 있었다. `tools/port-map.mjs`에 `mp-packet-delay`(`exclude-mp`) 규칙을 더해 이식 대상 todo 에서 제외로 옮겼고(`tests/port-map.test.mjs`의 예외 목록에 `o/RawPacketDelay` 추가), 멀티플레이를 구현할 때 함께 옮긴다.

## 남은 양

126단위·30,896줄 → 124단위·30,699줄(4-D 행 73/17,583 → 71/17,386).
