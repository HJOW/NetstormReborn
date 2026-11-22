# 메타(챌린지) 앞단 세 단위의 웹 범위 판정 (WB04-E1188)

재기준선 문서([E498~E505](webpj-rebaseline-meta-20261115.md))가 "단독 도달을 4-D에서 대조한다"며 `port`로 남겨 둔 메타 앞단 세 단위를 소스로 판정했다. **모두 멀티/메타 전용으로 제외한다.**

| 단위 | 원문 역할 | 단독 웹에서 닿는가 |
| --- | --- | --- |
| `ClientChallengeGlobals` | Client 장기 CS(챌린지) 포인터·메타 참가자/전투 상태 | 단독 미션은 CS 를 만들지 않는다. 웹 `core/raw-campaign-session-login-front.js`가 메타 CS 서비스(`challengeGlobals`)가 있으면 "메타 CS 상태는 웹 범위 밖"으로 거부한다. 이웃 단위(`ClientChallengeImages`·`MetaConnectionRuntime`·`ClientMetaBattleDescription`·`ClientMetaBattleObjects`)가 이미 `exclude-mp` 이다 |
| `ClientMetaPacketReceivers` | 모든 Client 기본표에 메타 참가자 추가/제거/이동(0x20·21·22)·전투 생성/제거(0x23·24)·항해 오류(0x25)·전투 옵션(0x67)·참가자 이름(0x88) 수신기와 버전·길이 관찰 등록 | 등록하는 본문(`ReceiveClientMetaParticipant*`·`ReceiveClientMetaBattle*`·`ReceiveClientMetaSailError`)이 모두 `exclude-mp`(`NativeMetaParticipant*`·`NativeMetaBattle*`·`NativeMetaSail*`) 단위이고, 단독 로컬 서버는 이 패킷을 보내지 않는다. 웹 `core/client-campaign-transport.js`가 이미 "Meta 계열 수신기 웹 제외"로 기록한다 |
| `NativeMetaConnectedState` | 메타 접속 상태 28~33과 `FindMetaBattleMaster`(전투 master 조회), 로그인 거부 이유 번역 | `ClientCampaignTransport.cpp`의 쓰임(`NotifyClientMetaMaster`)도 메타 전투 master 알림뿐이다. 재기준선 문서 자신이 "메타 접속 상태 28~33은 제외한다"고 적었다 |

## 처리

- `tools/port-map.mjs`에 `mp-meta-front`(`exclude-mp`) 규칙을 더하고 세 단위를 이식 대상 todo 에서 제외로 옮겼다(`tests/port-map.test.mjs`의 예외 목록 추가). 남은 양은 124단위·30,699줄 → 121단위·30,202줄이다.
- 같은 인계 항목의 `MetaMenuType`/`NotifyPlayerResults`(`RawCampaignSession.cpp` 4620..4642) 도달 판정은 남아 있다(`NotifyPlayerResults`는 `ClientMain.cpp:631`에서 `playerKnown && ownerMode.battle`일 때 불려 단독 미션과 겹칠 수 있어 별도 확인이 필요하다).
