# 재기준선 신규 메타·전투 경계 55단위 판정 (4-R ②-E)

`1f00a25a`와 후보 `225553df`의 단위 목록에서 앞선 E498~E501의 121개를
뺀 나머지 55개를 검토했다. E505의 재검토를 반영한 분류는 **port 15·adapt 1·exclude-edit 1·exclude-mp
38**이다. `port`는 전체 메타 기능을 웹에 만들라는 뜻이 아니다. 공통 부모의
현재 상태·등록·단독 분기는 보존하고 제외 분기는 실제 도달 경로를 검증해
구분한다. 이식 완료나 세션 연결 완료로 세지 않는다.

| 분류 | 단위 | 근거와 후속 경계 |
| --- | --- | --- |
| port | `ClientChallengeGlobals` | Client 장기 CS 포인터와 초기 상태를 공통 수신기들이 조회한다. 4-D에서는 단독의 현재 0과 같은 소유 수명을 보존하고 메타 할당/참가자 분기는 제외한다 |
| port | `ClientMetaPacketReceivers` | `ClientCampaignTransport`가 모든 Client 기본표에 20h~25h·67h·88h의 버전·길이·관찰기를 설치한다. 4-D에서 공통 등록 계약과 단독 도달을 대조하고 메타 참가자 동작은 제외한다 |
| port | `ClientMetaSpriteRefresh`, `NativeMetaSpriteRefresh` | E505 재판정: 메타 화면 외에 일반 Sprite `0x50cc08` 갱신과 같은 실제 World·Gump 연결을 포함한다. 이 일반 분기는 4-D/5-C에서 이식하고 메타 화면 분기는 제외한다 |
| port | `HudNsInfoMeta`, `NativeNsInfoInput`, `NativePlayerInfoInput`, `RawBattleOptionSlots` | NSInfo/PlayerInfo의 일반 앞단·기본 입력과 공유하는 슬롯 계산이 있다. 5-B·6에서 단독 경로를 확인하며 메타 참가자 송신·편집 분기는 제외한다 |
| port | `NativeBattleReturnState` | State6의 현재 결과·공통 Fort 부모까지 다루므로 4-D·8에서 단독 미션의 도달/복귀를 확인한다. 네트워크 전투 복귀와 Multiplayer Options는 제외한다 |
| port | `NativeMetaConnectedState` | 공통 `ClientCampaignTransport`의 현재 master 조회에 쓰이는 분기가 있어 4-D에서 상태·CS 관문을 확인한다. 메타 접속 상태 28~33은 제외한다 |
| port | `RawBattleCompletion`, `RawBattleResourceFrame`, `RawBattleResourceOptions` | 현재 Player·자원 정기와 종료 관문에 결합된다. 4-D·8 및 12차 스커미시 소비자에서 단독 도달을 확인하고 네트워크 재접속은 제외한다 |
| port | `RawStateBattleOptions`, `RawStateMetaBattleDescription` | 공통 State의 67h 옵션 적용과 71h CS0 관문·관찰기다. 4-D에서 범위/비용·등록 순서와 CS0 조기 반환을 유지한다 |
| adapt | `SystemNewsTime` | 채팅 장기 news 키·시각과 Config 읽기/쓰기. 8의 웹 저장 경계로 대체하며 외부 뉴스 서버 접속을 추가하지 않는다 |
| exclude-edit | `FortSubscriberClient` | Windows 등록 메시지에 반응해 현재 Fort의 등록 ID를 파일 writer에 변경한다. 웹은 Edit와 외부 등록 도구를 제외한다 |
| exclude-mp | `ClientChallengeImages`, `ClientMetaBattleDescription`, `ClientMetaBattleObjects`, `ClientMetaObjectInput`, `ClientMetaPlayerMenu`, `ClientMetaPointerInput`, `ClientMetaSpriteDraw` | 메타 CS Image·전투 설명·객체/참가자 UI와 메타 표시 본문. 71h의 CS0 관문과 공통 패킷 등록은 위 port 부모에 남긴다 |
| exclude-mp | `NativeBattleInfoInput`, `NativeBattleInfoMenus`, `NativeFortPreparation` | 메타 BattleInfo 옵션 UI와 개발자 2bh의 Fort snapshot 준비. 웹 스커미시 설정은 별도 `SkirmishSetup` 이식 대상이다 |
| exclude-mp | `NativeCameraRig`, `NativeIslandRig`, `NativeSpring3d` | 메타 참가자·전투 객체의 카메라/섬 입장 Rig와 spring. 실제 호출자는 메타 참가자 패킷이며 일반 미션 카메라는 기존 카메라 경로다 |
| exclude-mp | `NativeMetaBattleCreate`, `NativeMetaBattleDescription`, `NativeMetaBattleOptions`, `NativeMetaBattleRemove`, `NativeMetaDisplay`, `NativeMetaGeometry`, `NativeMetaObjectInput` | CS 전투·참가자 기록과 메타 화면 본문. 공통 67h/71h 부모의 CS0 분기는 위 port에 남긴다 |
| exclude-mp | `NativeMetaParticipantCreate`, `NativeMetaParticipantMove`, `NativeMetaParticipantName`, `NativeMetaParticipantRemove`, `NativeMetaPlayerMenu`, `NativeMetaProjection`, `NativeMetaSail`, `NativeMetaSailError`, `NativeMetaScene`, `NativeMetaSpriteDraw`, `NativeMetaState` | 메타 참가자·항해·투영·State19 화면과 해당 표시 수명. 단독 미션 및 생성 스커미시는 이 메타 화면으로 진입하지 않는다 |
| exclude-mp | `RawBattleOptionMutation`, `RawBattleOptionText` | 메타 BattleInfo 옵션 편집·번역 표. 슬롯의 공통 입력 계산은 위 `RawBattleOptionSlots`에 남기고 스커미시 금지 유닛은 `SkirmishSetup`에서 처리한다 |
| exclude-mp | `RawBattleReconnect`, `RawChallengeLoginReply`, `RawChallengeState`, `RawOrdinaryBattleStart`, `RawStateBattleStart` | 네트워크 재접속·메타 로그인/CS·일반 전투 초기화/1ah·1bh 참가. `RawMissionLoginTimer`의 일반 초기화는 `!mission` 관문이고 생성 스커미시는 미션 경로에서 자체 월드 계획을 사용한다 |

신규 176개는 E498~E502에서 모두 분류하고 E505에서 두 Sprite 갱신 분류를 정정했다. E502 당시 기존 이식분 변경 37개의
①/②/③ 판정과 필요한 재반영, 이 분류를 도구 규칙과 대응표에 옮기는 일,
manifest 근거 및 기준선 전환은 남는다. 사용하지 않는 메타 콜백을 빈 성공
함수로 대체하지 않고, 실제 단독 경로가 선택하면 명시적으로 확인한다.

다섯 판정표의 단위 열과 두 커밋의 신규 목록을 비교해 누락·중복이 없음을
확인했다. E505의 두 Sprite 갱신 단위 재판정 후 신규 176개의 합계는 port 100·adapt 7·exclude-edit 7·
exclude-mp 57·exclude-host 5다. 기존 기준선 행의 분류·상태는 이 합계에
포함하지 않는다.

현행 대응표 검사는 984행·오류 0건이고 Node v22.23.3·v24.22.3·
v26.13.0 전체 검사는 각각 4,966건 통과·실패 0이다. 이 단위에서 제품
코드를 바꾸거나 게임을 실행하지 않았다.
