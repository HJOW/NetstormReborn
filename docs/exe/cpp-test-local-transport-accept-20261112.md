# 1304 — zlocal accept 설계·Pop 송신 장치에 맞춘 cpppj 시험 교정 (2026-11-12, HJOW-Athlon)

webpj WB04-E323(RawGeyserMined)·E324(LocalMissionTransport) 이식 중 발견한 **cpppj 시험 결함 두 종류**를 사용자 요청(2026-11-12)으로 교정했다. **제품 코드는 바꾸지 않았다**(바뀐 것은 `cpppj/tests/` 시험 6개와 집중 대상 1개뿐이며 `Netstorm.exe` 는 1303 과 같다). 게임 실행·디컴파일은 하지 않았고 마지막 디컴파일 호스트(HJOW-Athlon)와 현재 호스트가 같다.

## 1. 원인

| 결함 | 원인 커밋 | 증상 |
| --- | --- | --- |
| A. Pop 송신 장치 미연결 | `bd36f6e64`(2026-11-04 `20261104-27 CP Activate와 원문 상태 패킷 writer 및 공통 버퍼 연결`) — 서버 영역 객체 Pop 의 첫 활성화가 `SquidPop::BindTransmission` 의 송신 writer 를 필수로 요구 | `rawGeyserMined_real_packet_factory_destroy_and_space` 가 emptyGeyser(서버 영역) Pop 에서 `현재Squid Pop의실제전송미연결` 로 중단 |
| B. zlocal 서버 끝 생성 시점 | `783b12763`(2026-11-06 `20261106-53 CP 공유 전송 등록표와 실제 월드 콜백 수명 접합`) — `LocalMissionTransport` 서버 끝을 `Open` 이 아니라 `Poll` 의 `AcceptPending`(클라이언트 접속 뒤 첫 빈 pending)에서 만든다 | `Open` 직후 pending[1]/서버 flags 0x24/시계 2회를 기대하거나 `Open` 직후 `AssignConnection(-1,…)` 을 부르는 시험 14건이 실패(재배치는 `로컬 실제 pending-1 재배치 계약 오류`) |

두 커밋 모두 기준선 `1f00a25a`(제품 985) 이전이며, 시험 파일은 그 뒤 갱신되지 않았다(LocalMissionTransport·ConnectionRegistration·ConnectionActivityTransport 시험의 마지막 변경은 2026-10-18~26). 2026-11-08 빌드의 통합 `netstorm_tests.exe` 에서도 같은 실패가 나왔다.

## 2. 교정 (시험만, 기대값을 약화하지 않고 현재 설계의 정확한 값으로)

| 파일 | 시험 | 교정 |
| --- | --- | --- |
| `GeyserMinedTests.cpp` | `rawGeyserMined_real_packet_factory_destroy_and_space` | 같은 `SquidPop` 에 송신 요청 기록 장치를 묶고, client 영역 간헐천 Pop 은 송신 없음·고갈 뒤 10.78 은 서버 영역 emptyGeyser 한 번(flags 9)·10.72 Pop 경로는 송신 없음을 단언 |
| `LocalMissionTransportTests.cpp` | `LocalMissionTransport_ServerOpenAndClientConnectAreSeparate` | 열기 뒤 서버 flags 0·시계 0·pending 없음, 접속 시각 101/102, Poll 뒤 서버 flags 0x04·수신 106·heartbeat 104·클라이언트 수신 107·시계 107·pending[1] 이 서버 끝 |
| `ConnectionRegistrationTests.cpp` | `ConnectionRegistration_LocalMissionSharesPendingOwnerAndClientNine` | 열기 뒤 pending 없음 → 접속 → `Poll` 뒤 pending[1] 의 서버 끝 토큰으로 재배치·정리 단언(나머지 그대로) |
| `ConnectionActivityTransportTests.cpp` | `…ClientObjectPresenceIsSeparateFromFlags`·`…FailedConnectKeepsPartialEffectsAndUnpublishedPresence` | 열기의 시계 0 회를 반영해 시계 수 2 씩·시각 2 씩 줄임(실패한 접속의 부분 쓰기 101, 재시도 103/104) |
| `BattleLeaveTests.cpp` | `BattleLeave_ActualLocalQueueAndRegistration` | `Open` 과 `AssignConnection(-1,1)` 사이에 `Poll()` |
| `CampaignSessionTests.cpp` | `SessionPickupWire`(시험 10건이 공유) | `Open` 과 `AssignConnection(-1,1)` 사이에 `Poll()`. 이 Poll 은 두 끝의 접속 통지(type 1)만 소비한다(HEAD 의 접속 통지 수신기는 로컬 끝에서 보낼 복제 패킷이 없어 효과가 없다) |

손 계산 근거(B): 열기(시계 0) → `Connect`(101·102) → `Poll` 의 accept(103·104) → 서버 접속 통지(105) → 집합 로그인(106) → 클라이언트 접속 통지(107). webpj 이식 시험(`webpj/tests/local-mission-transport.test.mjs`)이 같은 값을 독립으로 얻었다.

**새 집중 대상** `netstorm_local_transport_tests`(`tests/CMakeLists.txt` 끝, `FocusedTestMain` + LocalMissionTransport·ConnectionRegistration·ConnectionActivityTransport·BattleLeave 시험): 이 시험들은 통합 `netstorm_tests`(개체 840개)에만 있어 교정 검증에 1시간 이상이 들었다. 기존 `netstorm_pop_context_tests`(GeyserMined)·`netstorm_world_generation_tests`(CampaignSession)는 그대로 쓴다.

## 3. 검증 (Release 집중 빌드, 게임 실행 없음)

같은 작업 트리에서 교정 전 소스로 세 집중 대상을 빌드(609초, 종료 0)·실행한 뒤 교정하고 다시 빌드(120초, 종료 0)·실행했다. `netstorm.lib` 은 1303 빌드 그대로였다(제품 소스 변경 없음).

| 대상 | 교정 전 | 교정 뒤 |
| --- | --- | --- |
| `netstorm_local_transport_tests`(새 대상, 17건) | 실패 5: `LocalMissionTransport_ServerOpenAndClientConnectAreSeparate`·`ConnectionRegistration_LocalMissionSharesPendingOwnerAndClientNine`·`ConnectionActivityTransport_ClientObjectPresenceIsSeparateFromFlags`·`…FailedConnectKeepsPartialEffectsAndUnpublishedPresence`·`BattleLeave_ActualLocalQueueAndRegistration`(18 CHECK) | **17/17 통과**, 실패 0 |
| `netstorm_pop_context_tests`(1175건) | 실패 25(46 CHECK) — 4.2절 기존 25건과 같은 수이며 그중 하나가 `rawGeyserMined_real_packet_factory_destroy_and_space` | 실패 **24**(45 CHECK). 교정 전에만 실패한 것은 rawGeyserMined 하나, 교정 뒤에만 실패한 것 0 |
| `netstorm_world_generation_tests` `SessionPickupWire` 10건(이름 필터로 하나씩) | 9건 실패(모두 `로컬 실제 pending-1 재배치 계약 오류`), `CampaignForwardBattle_ActualSavedStoneExplosionLifetime` 통과 | **9건 통과**, `CampaignDevastation_ActualEnemyCaptureAndDropFromCast` 만 다른 지점에서 실패(4절) |
| `netstorm_world_generation_tests` 전체 실행 | `CampaignSession_GeneratorBodyShadowAndPicking`·`CampaignSession_AiCarrierActionsUseActualWorldContentsAndLifetime` 실패 뒤 `CampaignSharedKernel_*` 구간(1894~1954행)에서 비정상 종료(OK 57건) | 교정 전과 같음(실패 목록 동일, OK 57건, 같은 위치 비정상 종료). 교정한 2416행보다 앞이라 이번 변경과 무관한 기존 문제 |

## 4. 남은 것 — NB18 과 충돌하는 시험 (결정 필요)

`CampaignDevastation_ActualEnemyCaptureAndDropFromCast`(2026-10-20 작성)는 설정 실패가 풀리자 3961행 `link.transport.Poll()==1`(포획 요청 0x4f 없음)·3962행 `captured.value` 에서 실패한다. 임시 확인 줄(빌드·실행 뒤 제거)로 도착 시점에 시전자(TYPE 133)가 기도 내용물(SID 15057, contained Form TYPE 6)을 들고 있음을 확인했다. **10차 NB18 수정(1290, `RawCarrierPickup::RefusesHeldPriest` — mask 1 내용물을 든 운반체의 사제 줍기 요청/수신 거부)**이 이 포획을 정상적으로 막는 것이며, 시험은 NB18 이전 동작(기도를 든 채 적 사제 포획)을 기대한다. 시험 장면을 바꿀지(NB18 거부를 단언하고 포획/내려놓기는 내용물 없는 운반체로 검증) NB18 범위를 너겟으로 좁힐지는 제품 판단이라 이번 단위에서는 고치지 않고 LEFT_JOBS 결정 D8 로 남겼다. → **1305 에서 해소**(5절).

## 5. 1305 — 결정 D8 반영: NB18 유지, 시험 교정 (2026-11-12)

사용자가 권고안("시험을 고치는 게 맞다")을 택했다. 제품 코드(NB18 `RawCarrierPickup::RefusesHeldPriest`)는 그대로 두고 `CampaignDevastation_ActualEnemyCaptureAndDropFromCast` 만 고쳤다.

- 시전·폭발·피해·시전 기도 내용물 유지까지의 앞부분은 그대로다.
- **거부 단언:** 기도 내용물을 든 시전자가 행동 불가가 된 적 사제 좌표에 도착한 뒤 `Flush`·`Poll` 하면 소비 프레임 0·서버 기록 1개(앞선 시전 0x50 뿐)이고, 시전자 안에 포로(TYPE 158 contained)가 없으며 적 사제는 free/dead 가 아니고 Player 2 의 사제 참여 값이 바뀌지 않는다.
- **포획/내려놓기 검증:** 내용물이 없는 새 운반체(같은 TYPE 133, 소유자 1)를 `(x-1, y-1)` 에 놓고(내용물 mask 1 없음 확인) 같은 경로 도착 도우미로 적 사제에 보내, 기존 단언(포획 요청 1프레임·포로 Parent·인자 보존·참여 값, 0x50 내려놓기·복원 사제 TYPE/소유자·포로 정리·잔액 불변·기도 내용물은 시전자에 그대로, 서버 기록 0x50·0x4f·0x50)을 이 운반체 기준으로 검사한다. 이동은 두 운반체가 같은 람다(`StartCarrierMovement` → 도착 시각 프레임 반복 → 좌표·경로 종료 확인)를 쓴다.
- 거부 단언은 NB18 이 없으면 시전자가 포획해 `Poll()==1` 이 되므로(1304 이전 시험의 기대) NB18 회귀를 잡는다. 이 판단은 1304 의 임시 확인(도착 때 든 내용물이 mask 1 기도 내용물)에 근거한다.

**검증:** `netstorm_world_generation_tests` 증분 빌드 64초(종료 0). `--filter CampaignDevastation_ActualEnemyCaptureAndDropFromCast` **통과(실패 CHECK 0)**, `SessionPickupWire` 10건 **모두 통과**, 전체 실행은 1304 와 같은 기존 실패 2건·OK 57건·같은 위치 비정상 종료(이번 변경과 무관).
