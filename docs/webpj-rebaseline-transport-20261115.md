# 재기준선 신규 전송·진단 경계 26단위 판정 (4-R ②-D)

후보 `225553df`에 추가된 단위 26개를 기준선 `1f00a25a` 및 앞선 세
분류 묶음과 대조했다. 분류는 소스의 웹 범위를 정한 것이며 실제 세션
조립과 단독 도달 검증을 완료했다는 뜻은 아니다.

| 분류 | 단위 | 근거·후속 경계 |
| --- | --- | --- |
| port | `NativeCarrierPacketReceivers`, `NativeConstructionPacketReceivers`, `NativeGameplayPacketReceivers`, `NativeLoginPacketReceivers`, `ClientChatPacketReceiver` | `ClientCampaignTransport`와 Session이 현재 Client에 설치하는 고정 수신기. 내부 단독 미션의 실제 패킷·등록 수명을 4-D에 연결하고 각 입력·채팅 후단은 6·8에서 연결한다 |
| adapt | `ClientPacketSources` | TCP/UDP·Editor와 내부 Composite의 Source 정체성을 공유하는 장부다. 웹에서는 실제 소켓 대신 내부 전송에 맞추되 송신 시 선택·대여 수명은 4-D에서 확인한다 |
| port | `FortCoreDataLoad` | World 없는 메타와 `RawCampaignSession`이 함께 쓰는 현재 CoreData 읽기다. 단독 적재의 호출 여부와 현재 저장소 연결을 4-D에서 검증한다 |
| port | `RawClockSynchronization`, `RawConnectionEnded`, `RawConnectionHealth` | 현재 시계 보정·연결 종료·활동 감시. 4-D의 내부 연결에서 실제 도달과 초기값을 확인하고 서버 이전 등 네트워크 전용 분기는 제외한다 |
| port | `RawPacketCopy`, `RawPacketDelay`, `RawPacketFrame` | 고정 등록·관찰기·내부 프레임과 공유하는 패킷 복사·지연·분배. 4-D에서 실제 단독 경로를 확인하고 TCP/UDP 구현을 가져오지 않는다 |
| port | `RawPeerWorldSync`, `RawSquidCompressedReceive`, `RawSquidMutationReceive`, `RawSquidReceive`, `RawSquidVerification` | 같은 Pool·TYPE·SID와 현재 전송 등록을 이용하는 월드 동기화/수신/검증. 4-C 이월의 내부 미션 송신·수신과 함께 4-D에서 연결한다. 각 검증 패킷의 단독 도달 여부는 그때 확인한다 |
| port | `RawConstructionFailurePackets`, `RawTechnologyTypesPacket` | 건설 실패와 기술 목록의 공통 수신 바이트. 기술 목록은 4-D 기본 등록, 건설 결과는 6의 소비자와 연결한다 |
| port | `RawRemoteTellPacket`, `RawDeveloperCommandPacket` | `RawCampaignSession`이 비용 안내의 70h와 단독·일반 공통 31h를 등록한다. Tell·명령 소비자는 순서 8에서 연결하며 외부 서버 접속을 만들지 않는다 |
| adapt | `ClientBuildReplay`, `RawBuildReplay` | 같은 건설 기록의 파일·재생 수명이다. 순서 6의 원문 입력을 검토하며 웹에서 파일 장치를 대체한다. 실제 지원 경로와 입출력은 소비자에서 검증한다 |
| exclude-host | `RawCdBattleCompletion` | 10.72의 172바이트 Player 전용 분기다. 웹은 10.78만 대상으로 한다 |
| exclude-mp | `RawRootLoginReply` | Root 로그인 서버의 존·친구 목록·버전 응답이다. 단독 미션의 내부 Zone 로그인과 다르며 외부 서버 통신을 추가하지 않는다 |

신규 176개 중 E498~E500의 95개와 이번 26개를 판정했고 **55개**가
남는다. 메타 수신기 일부는 모든 Client 기본표에 설치되므로 아직 일괄
제외하지 않았다. 기존 이식분 변경 37개 판정과 대응표 전환도 남는다.

Git 트리의 신규 단위 26개와 앞선 묶음과의 중복 부재를 확인했다. 현행
대응표는 984행·오류 0건이고 Node v22.23.3·v24.22.3·v26.13.0 전체
검사는 각각 4,966건 통과·실패 0이다. 제품 코드 변경이나 게임 실행은 없다.
