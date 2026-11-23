# webpj 대응표 partial 세 행 승격 (WB04-E1292, 2026-11-24)

[LEFT_JOBS](../LEFT_JOBS.webpj.md#next) '대응표 partial 행 정리'의 닫는 조건이 이미 충족된 4-D 세 행을 메모에 반영하고 `partial`에서 `port`로 올렸다. 코드는 바꾸지 않았다. 판정의 근거는 모두 이전 단위에서 직접 확인한 결과이며 이 단위는 메모·기준 문서만 정리했다.

| 행 | 올린 근거 | 문서 |
| --- | --- | --- |
| `RawCampaignSessionLogin.inc` (389줄) | 마지막 메모(E823)의 "원격 로그인·Native 경계 미검증"은 E1263 점검 4D-3a가 낡게 만들었다. 단독(캠페인·데모·스커미시)이 도달하는 로그인 경로(Begin/Pump·27h·Ready·State8/9·서버/클라이언트 Login·출하 Fort 32종·Chat/동맹 등록)는 E703~E772·E823에서 연결·검증했고, `BeginGeneral` 가지는 일반(LAN) 설치 모드(`CampaignInstallMode::GeneralBattle`)이며 원격 owner Fort 훅·원격 로그인·Native 경계는 단독에서 도달하지 않는다. | [E1263 점검](webpj-left-jobs-audit-20261123.md) · [로그인 32커밋 판정](webpj-session-login-commit-audit-20261118.md) |
| `ClientCampaignTransport.cpp` (378줄) | Client 기본 수신기 표(`makeClientCampaignTransport`·`installClientPacketDefaults`)는 단독 내부 전송에 그 패킷이 오지 않는다. 실제 Chromium 스커미시 40초 관찰에서 분배기에 도착한 종류는 1 두 개뿐이고 18종 중 어느 것도 도착하지 않아(E1274) 앱에 설치하지 않아도 미설치와 같다. 메모의 "InstallNative…PacketReceivers 고정 수신기 묶음 남음"은 E1193~E1221에서 닫혔다. | [도달성 판정](webpj-client-default-receivers-scope-20261123.md) |
| `UberReconnect.inc` (33줄) | 재접속·LAN은 웹 제외이다. 앱이 `pollClientConnectionHealth`를 부르지 않아 생기는 차이는 하트비트(서버 3초/클라 10초)와 그 상태 안내뿐이며 단독 미션(내부 전송)에서 관측 효과가 없다는 E1263 점검의 판정(4D-7b)으로 E1223 감사의 "단독 관측 여부 미확인"을 닫는다. 하트비트 자체를 실행하지 않는 것은 설계 결정이다. | [E1263 점검](webpj-left-jobs-audit-20261123.md) · [연결 감시](webpj-client-connection-health-20261119.md) |

## 집계 변화

`node webpj/tools/port-map.mjs`와 `python -X utf8 tools/port_inventory.py` 재집계: partial 12→9, `port` 263→266, 이식 대상 1,007단위 중 port 이상 995(98.8%) → 998(99.1%). 남은 partial 줄 수는 12,748 → 11,959이다(세 행 789줄 감소 + `UberGump.cpp`가 E1291의 방향키 분기로 11줄 늘어 2,703 → 2,714). `client/` 구역 422/433 → 425/433.

## 남은 partial 9행

`RawCampaignSession`(메모 마지막 E1241, 잔여 문장 대조 필요)·`CampaignClientServices`(ricochet 비결정성 적응 판정)·`RawClockSynchronization`(CRT 동률 RTT 원본 필요)·`ClientRendererFrame`(메인 메뉴 Kencloud)·`UberGump`(Kencloud 출력)·`RawButtonControls`(예산 칸 원본 우클릭·'Chat Window' 우클릭)·`RawMenuChat`(채팅 로그 미션 간 유지 결정)·`CampaignObjectMenuClient`(Archer·운반체 실제 확인)·`RawCampaignRitual`(AddRitualRegular의 shared 플래그 판정). 행별 닫는 조건은 LEFT_JOBS 0.2절 둘째 표에 있다.
