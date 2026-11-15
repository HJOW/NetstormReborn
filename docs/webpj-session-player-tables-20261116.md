# Session Player·공용 패킷 테이블 결선 (WB04-E576)

`225553df`의 RawCampaignSession.cpp1152..1186을 같은 E574 기반 owner에 연결했다.
현재 캠페인 Core1 선택, missionPost 비용 관문, 실제 Broadcast/Form writer, 장기 SID 목록과
Player 공장·현재 동맹을 구성한다. 전체 RawCampaignSession은 계속 **partial**이며 전체
생성자·외곽 공개·Load·Login은 미완료다. Runtime에 임시 Session을 공개하지 않는다.

## 구현 범위와 수명

`raw-campaign-session-tables.js`의 RawCampaignSessionTables는 기반 생성 때 같은
GameAux.SquidTransmission의 forms/deletion/buffer2048을 빌린다. 새 헤더·삭제 표식·버퍼를
만들거나 Path의 별도16384BYTE 예약과 혼용하지 않는다. 부모는 실제 Player/SP·진단 뒤
`connectPlayerTablesAndPackets()`를 한 번 호출한다. 정상 복귀는 이 구간 완료만 뜻한다.
초기화 전 호출은 거부하고, 실행 도중 실패하면 앞선 실제 쓰기와 결선을 남긴 채 재실행을
거부한다. 이 순서는 원문 생성자의 일부이며 전체 Session 생성자 예외 정리는 후속 부모가 맡는다.

외부 ClientCoreData가 있는 캠페인은 실제 Select(player, metamap, 1, 1)을 실행한다.
E574의 Core0/null 선택을 최종 Core로 사용하지 않는다. 건설 command/place의 비용 차단은
같은 장기 gameplayGlobals.world.missionPost를 현재 조회한다. 원문처럼 이 콜백은 장기
전역 owner만 보관하며 짧은 Session owner를 붙잡지 않는다.

Broadcast는 같은 실제 Player1620BYTE·서비스 peer9·현재 로컬 DWORD·현재 Server를 빌린다.
server 진단은 같은 World.Storage의 AssertDisplay 식/파일/행을 호출한다. 값형 서비스의
assets와 peer 배열은 기반 생성 시 복사하고 함수/장기 owner는 같은 참조를 유지한다.
호출자가 준 assets에 기본 송신 함수를 써 넣지 않는다. 다른 서비스 묶음의 값형 수명은
후속 전체 생성자 소비 구간에서 계속 대조한다.

별도 장치가 없는 assets.sendGunPacket/pathPacket/formPacket은 원문처럼 const Send다.
BindMutableSend를 연결했다는 이유로 기본 const 송신을 mutable로 바꾸지 않는다.
선택된 건설 mutable 송신은 현재 Login이 있으면 그 transport.registry, 없으면 현재
services.connectionRegistry를 선택한다(4129). Registry가 없으면 그 위치에서 오류이며
수신기를 동기 호출하지 않는다. 실제 Login 부모는 아직 없으므로 Login 분기는 정적 이식만
했고 동적 검증은 D2에 남긴다. 이를 위한 가짜 Login 객체는 시험에 넣지 않았다.

dependencyTransmit 기본 장치는 실제 RawFormPacket.writeData의 현재 가상 표/필드와
공용 버퍼를 사용하고 반환 크기13/18만 현재 services.formPacket에 넘긴다. 반환 크기와
헤더 선언 WORD는 별개다. 두 번째 인자는 원문처럼 사용하지 않고, 공용 stale 꼬리도 지우지
않는다. 제공된 네 송신/의존성 장치는 덮어쓰지 않는다. Native 전용 공용 observer 장치는
이번 독립 typed 캠페인에 추가하지 않는다.

사제의 sharedOwnerState/sharedPriests와 자산의 residences/temples/outposts/geysers는
같은 Storage의 실제 표/목록이다. Outpost 목록 참조 자체는 공통 초기화이며 Outpost 게임
동작을 구현한 것은 아니다(후순위 스커미시 대상). 각 owner0..8에서 실제 Player +0/+4/+8을
읽는 BindFactory를 연결한 뒤 자기 mask/대각 동맹 기본값을 쓴다. 전체 루프 복귀 뒤에만
Gun 방향 동맹81DWORD·Player +0x54 DWORD mask·+0x50 **BYTE** flag의 현재 getter를
설치한다. Player의 현재 물리를 목록 사본으로 대체하지 않는다.

## 근거와 검사

RawCampaignSession.cpp/.h·RawGameAuxState.h는 기준선 이후 HEAD 변경0이다. 실제 마지막
변경은 각각 `382ec33fe`·`bda77381e`·`8defb347c`이며 이번 cpppj 수정은 없다.
기준선 생성자와 SendConstructionMutable, 실제 Broadcast/Form/Registry/LocalMissionTransport,
Player 기록·공유 목록·Gun 소비자를 대조했다. CampaignSession_InitSharedTablesBeforeInstall과
ActualLoginUsesClientCurrentCoreSlot도 읽었지만 이들의 전체 Session/Load/Login 검사를
이번 부분 생성자 검사로 통과한 것으로 세지 않는다.

신규10검사는 실제 보호 GameAssets/World·Player/SP에서 다음을 확인한다.

- 실제 Core1의 Player 물리10BYTE와 단계/반납/반복 거부, 미공개 Runtime.
- 실제 건설 명령이 현재 missionPost에 따라 비용 선검사를 실행/생략함.
- 값형 서비스 사본·NULL peer 기본 송신·현재 로컬/Server·원문 진단, 제공된 장치 보존.
- 실제 Form writer13/18BYTE의 전체 출력, 같은 공용 버퍼/현재 헤더·삭제 표식과 stale 꼬리.
- 실제 내부 zlocal의 client9 송신이 observer(1) 한 번→큐→observer(0)/수신으로 진행함.
  이 시험의 수신/기록은 효과 경계이고 전체 미션 Login 구현의 증거는 아니다.
- Player9의 같은 동적 공장 목록으로 append/중복 안정 제거·현재 count/capacity·꼬리 보존.
- 실제 사제 postPop의 같은 장기 목록/owner 쓰기와 자산 공유 목록 조회.
- 실제 Gun 표적 판정의 현재 mask·방향 동맹, BYTE flag 폭과 범위 오류.
- 방송 생성/공장 루프 부분 실패의 실제 앞선 Core·콜백·목록·대각 기본값 보존.

최초 집중37검사는35통과/2실패였다. 둘 다 시험 오류였다. 내부 전송은 패킷 인자를 받는
observePacket 대신 방향만 받는 observe를 사용했고, Gun 문맥에는 필수 centerX/centerY가
빠졌다. 실제 API와 문맥으로 교정한 뒤 Node22(v22.23.3)·Node26(v26.13.0) 집중은 각각
37/37 통과했다. 신규 모듈 줄100%·분기97.44%·함수89.47%이며 실제 Login 분기와 미사용
참조 setter는 후속 부모 검증 대상이다. 로그는 `extracted/wb-e576-focus2-node22.log`·
`extracted/wb-e576-focus2-node26.log`(Git 제외)다.

전체 검사 도중 최종 검토에서 비용 콜백의 캡처를 짧은 owner에서 실제 장기 gameplay owner로
좁혔다. 진행 중이던 두 전체 검사를 중단하고 변경본으로 새로 시작했다. 중단 로그는
`extracted/wb-e576-stopped-node22.log`·`extracted/wb-e576-stopped-node26.log`이며
완료한 전체 검사로 세지 않는다. 변경 뒤 Node26 집중37/37도 다시 통과했다.
최초 완주한 전체 검사는 두 버전 모두5,469통과/2실패였다. 새 모듈의 PORT_MAP 등록을
전체 검사 뒤로 미뤄 대응표 형식/완전성 검사가 같은 누락을 검출했다. 해당 Session 행에
모듈과 근거를 등록하고 두 전체 검사를 다시 실행했다. 실패 로그는
`extracted/wb-e576-map-fail-node22.log`·`extracted/wb-e576-map-fail-node26.log`다.
최종 전체 Node22(v22.23.3)·Node26(v26.13.0)은 각각5,471건 통과,
실패·취소·건너뜀0으로 끝났다. 로그는 `extracted/wb-e576-node22.log`·
`extracted/wb-e576-node26.log`이며 Git 제외다.
대응표 최종 검사는1,160행/오류0이다(`extracted/wb-e576-port-map-final.log`).
port841·partial34·todo131·제외154, session/oracle0과 단계28/43은 그대로다.

새 관측/fixture는 없고 manifest803항목은 유지한다. 원본 게임·cpppj 오라클·브라우저·
디컴파일·공중3종 전용 행동·새 외부 멀티플레이 실행 검사는 하지 않았다. 내부 zlocal은
기존 단독 캠페인 전송 코드이며 OS 소켓이나 외부 접속을 만들지 않는다.

## 다음 시작점

1187 Native 섬 장치 분기 뒤1188의 실제 RawIslandRuntime부터 원문 생성 순서로 이어간다.
사제 효과·SoundProcess·Player/소유권·전투/경로/건설 등 같은 실제 모듈과 등록이 남았다.
이후 외곽 Session 공개와 같은 Template TypeNames→Objects/Contents·Load, 실제 Login·
프레임·종료를 연결해야 한다. L2/session-dump와 실제 미션 화면/자연 플레이는 미검증이다.
