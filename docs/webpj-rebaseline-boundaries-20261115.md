# 재기준선 신규 편집·플랫폼 경계 33단위 판정 (4-R ②-B)

기준은 `1f00a25a`와 후보 `225553df`의 `client/`·`o/` 단위다. 아래
33개 모두 이전 기준선에 없고 후보에 생긴 단위임을 Git 트리로 확인했다.
분류는 구현 완료를 뜻하지 않는다. 기존 대응표는 나머지 분류를 끝내기
전까지 바꾸지 않는다.

| 분류 | 단위 | 근거·후속 경계 |
| --- | --- | --- |
| exclude-edit | `EditorScenarioController`, `EditorScenarioFiles`, `EditorScenarioModel`, `ScenarioDocument`, `UberScenario` | 9차 Editor의 시나리오 문서·파일·편집 UI. `ScenarioDocument` 소비자는 Editor 계열이며 웹은 Edit와 9차를 제외한다 |
| exclude-edit | `NativeFortSnapshot` | `UberGump`의 Fort/일반 전투 snapshot 저장에 쓰인다. 웹의 Editor 및 네트워크 Fort 저장은 제외한다 |
| exclude-mp | `LanBattleLobby`, `LocalServerProcess`, `LocalUdpTransport`, `MetaConnectionRuntime`, `MissionNetwork`, `MultiplayerOptionsWindow`, `NativeLocalServerLaunch`, `RawMenuMultiplayer`, `UberLan`, `UberMeta`, `UberReconnect`, `RawLocalServerDiscovery`, `RootServerEndpoint`, `RootServerListDisk` | 로컬 서버·실제 TCP/UDP·메타 대기실·서버 목록과 Multiplayer UI. 웹 내부 미션 전송과 스커미시 규칙은 이 제외에 포함하지 않는다 |
| exclude-mp | `SavedStateFileRead`, `SavedStateFileWrite`, `RawSavedStateWrite` | 일반 전투 재접속 `!state.dat`의 파일 읽기·쓰기. `PollSavedStateWrite`는 battle 관문이며 웹 스커미시에는 네트워크 재접속이 없다. 캠페인 진행 저장은 별도 localStorage 경로를 유지한다 |
| exclude-mp | `NativeJoinTestBattle` | 개발자 2bh 명령이 현재 Fort를 준비하고 메타의 TCP 전투 참가 본문을 호출한다. Editor의 Test Battle과도 별도이며 웹의 미션 내부 연결과 구분한다 |
| exclude-host | `CampaignTestPlay`, `CampaignTestSpectator` | cpppj 명령줄 전용 자동 플레이·관전 도구. cpppj 출력은 L3 비교에 활용하지만 웹 제품의 일반 경로로 이식하지 않는다 |
| exclude-host | `ClientPacketRecord`, `RegisteredMessages` | 사용자 홈의 패킷 기록 파일과 Windows 등록 창 메시지·외부 설정 도구. 7차 브라우저 오류 console 로깅은 별도 요구다 |
| adapt | `ClientTimeSource`, `SessionStateDump` | 전자는 OS 시계와 고정 프레임 시험 시각, 후자는 파일 TSV로 내보내는 비교 관측이다. 4-D L2에서 브라우저 시계·웹 덤프를 같은 입력/출력 계약으로 연결한다 |
| adapt | `DisplayTextInput` | Windows 확정 문자와 표시 바이트의 경계다. 웹 입력 이벤트·한국어/기타 언어 표시 부호화에 맞게 6·10단계에서 연결한다 |
| port | `FrameRateOption` | 60/75/120 값과 선택 해석. 프레임 품질을 다루는 9단계에서 웹 정책에 맞게 반영한다 |
| port | `NativeFortSaveVirtual` | 이름과 달리 단독 `RawCampaignSession`의 가상 레벨·내용물 Robust 조회와 `RawSurfaceReward`·Factory·Ritual이 같은 저장소에서 사용한다. Editor 저장 전용으로 제외하지 않으며 4-D 및 각 소비자에서 연결한다 |

신규 176개 중 E498의 29개와 이번 33개를 판정했고 **114개**가 남는다.
기존 이식 단위 변경 37개의 별도 판정, 범위 규칙 반영 및 기준선 전환도
남아 있다. 도달 여부가 불명확한 메타 패킷 수신기와 공유 객체는 이 묶음에
넣지 않았다.

현행 대응표 검사는 984행·오류 0건이고 Node v22.23.3·v24.22.3·
v26.13.0 전체 검사는 각각 4,966건 통과·실패 0이다. 이 단위는 문서
판정이며 제품 코드 변경이나 게임 실행은 없다.
