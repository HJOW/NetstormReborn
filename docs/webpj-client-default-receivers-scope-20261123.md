# webpj Client 기본 수신기 표의 도달성 판정 (WB04-E1274, 2026-11-23)

[LEFT_JOBS 점검](webpj-left-jobs-audit-20261123.md)(E1263)이 4D-3b 에 남긴 "Client 기본 수신기 표(`makeClientCampaignTransport`·`installClientPacketDefaults`)가 앱에 설치되지 않는다"를 판정했다. 코드는 바꾸지 않았다.

## 무엇인가

`CLIENT_PACKET_DEFAULTS`(`www/core/client-campaign-transport.js`) 18행은 cpppj `ClientCampaignTransport` 가 Client 장기 분배기에 채우는 기본 등록이다. 종류별로 고정 길이·버전 정책을 맞추고, 원문 빈 C3 수신기 7종(0x0c·0x42·0x4d·0x55·0x56·0x57·0x5a)과 읽기 전용 관찰기(진단)를 설치한다. 즉 **서버에서 Client 로 오는 패킷의 검증·진단 장치**다.

## 판정

- 분배기는 수신기가 없는 종류(10 이상)를 오류 없이 무시한다(`RawPacketDispatch.validate` 가 false → `dispatch` 가 true 로 반환). 기본 등록이 없을 때의 차이는 그 종류의 패킷이 실제로 도착할 때만 생긴다(빈 수신기는 수신·기록 시각만 갱신).
- 웹 단독 미션(캠페인·데모·스커미시)의 내부 전송(`LocalMissionTransport`)은 Session 이 등록한 수신기로만 돈다. **실제 Chromium 에서 스커미시를 시작해 40초 진행하며 분배기의 `dispatch` 로 도착하는 패킷을 센 결과 종류 1 두 개뿐이었고 18종 중 어느 것도 도착하지 않았다**(채팅·기술·경로 등은 앱의 자체 경로로 처리돼 이 분배기를 거치지 않는다).
- 따라서 앱에 기본 표를 설치하지 않아도 단독 미션의 동작은 같다. 멀티플레이(웹 제외)에서만 길이 검증·관찰기 진단이 의미를 가진다. 모듈과 시험은 L1 용도로 남긴다.

## 고정

새 e2e `e2e/internal-transport-packets.spec.js` 가 스커미시 40초 동안 분배기에 도착한 종류가 기본 18종에 하나도 없음을 단언한다(분배기를 감싼 계측이 실제로 패킷을 세는지도 확인). 이 성질이 깨지면(예: Session 이 기본 종류 패킷을 보내기 시작하면) 이 시험이 먼저 알려 주고, 그때 기본 표를 앱 Session 에 설치한다.
