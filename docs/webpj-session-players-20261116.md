# Session Player·Template·소유권 연결 (WB04-E578)

`225553df` RawCampaignSession.cpp1236..1267을 E574~E577의 같은 기반 owner에 연결했다.
실제 PlayerWorldHooks→RawPlayerWorld/후처리→RawPlayerTemplateState→RawPlayerRegular
→RawCampaignOwnership 순서다. 전체 Session은 계속 **partial**이며 나머지 생성자·외곽 공개·
같은 Template의 전체 Load/로그인/프레임은 잔여다. 임시 Session을 Runtime에 공개하지 않는다.

## 연결 범위

`connectPlayersAndOwnership()`는 실제 섬/사제 구간 뒤 한 번 실행한다. 실제 Player152의
Find/Create/Owner/Pop·첫 Regular 예약은 같은 World/Core/Host를 사용하고 기록/상태는
같은 Player1620BYTE/PlayerWorldState다. Player 진단은 호출 때 현재 AI 장치를 선택하며
AlreadyExists와 PlayerKnown의 원문 식을 구별한다. 실제 Player를 생성하는 것은 이 구간의
부작용이 아니라 이후 호출자의 작업이다. 검사에서 선택한 생성도 전체 미션 Load로 세지 않는다.

Template는 같은180BYTE 헤더·현재 Storage.selectState/name·objectState.mission을 빌린다.
빈 State의 미정28BYTE 입력과 실제 Storage.initializeState, 짧은 State의 ensureStateSize를
연결한다. 두 소비자는 실제 manager/섹션/커서의 수명과 갱신을 수행한다. get이 돌려준 JS 뷰
객체가 새로 생겨도 C++ span처럼 같은 buffer/offset/length여야 한다.
CD 전용 loadingDepth 즉시 증감/콜백은 PATCH 분기에 실행하지 않는다. 파일 저장 부모는
현재 생성자에서 연결하지 않으며 원문 후속 연결/웹 저장 판정 구간에 남긴다.

Player Regular는 현재 battle 모드와 같은 Host에 사건1/2를 예약하고, 같은 Template getter/
Changed를 호출한다. getter 효과 뒤 templateOwner를 다시 읽는다. 제공된 로그는 값형으로
보존하며 누락 로그/미지원 사건 진단은 원문 FillMissing/SessionMissing의 실제 선택 지점에서
실패한다. 현재 Runtime의 전체 Regular 분배는 후속 생성자 연결에 남았다.

소유권은 같은 실제 사제 분배·Player 기록·섬·metadata.frames/현재 TYPE 테마 벡터·난수·
도장/투표 상태를 사용한다. 신전 네 VT는 실제 VortexOwner에 가고 다른 파생은 실제 사제
분배를 거쳐 World의 자산 소유자에 연결한다. missingIsland는 같은 현재 Storage에 원문 상세
문자열을 기록하고 단언을 호출한다. 진단 정상 복귀 뒤의 원문 후단을 우회하지 않는다.

기반 자료에 원문636..639의 RegionPaintState/RegionOwnershipState를 추가하고 같은
worldRedraw와 mapRevision 상자를 빌렸다. 원문1004의 themes/elements188칸 벡터는
값 초기화0을 보존한다. 실제 TYPE theme 채움은 원문1284이며 아직 앞당기지 않았다.
services.player/ownership도 생성 시 값형으로 복사한다. 제공한 함수와 그 장기 참조는 유지한다.

부분 실패는 앞서 만든 실제 Player/Template/Regular를 남기고 반복 실행을 거부한다.
종료는 현재 World 프로세스 반납→Player 등록 해제→사제 등록 해제→World/Player/게임
전역 대여 반납 순서다. 기존 E577의 다른 Kernel 소유권 보존 검사도 계속 통과해야 한다.

## 근거와 검사

Session·RawPlayerWorld/TemplateState/Regular·RawCampaignOwnership은 기준선 이후 HEAD
변경0이며 cpppj 수정은 없다. 실제 생성자와 소비자의 enum 식·현재 조회/값 복사·원문 벡터
초기화 시점을 대조했다. Template의 depthChanged는 C++ 구현에서 CD 분기에만 쓰인다.
PlayerWorld_SameSurfaceCoreRecordsAndInitialRegular와 VortexOwner_CampaignSameCoreAndFirstRegion의
같은 Core/Player/첫 지역 계약도 읽었다. 해당 CPP 검사의 CD 분기나 전체 덤프를 이번 부분
연결 검사로 통과했다고 세지 않는다.
기존 PlayerWorld/Template/Ownership 단위의 원본 fixture와 계약 검사는 전체 회귀에서
유지한다. 이번 단위는 새로운 원본 관측이나 fixture를 만들지 않으며 manifest803항목을 유지한다.

신규12검사는 실제 보호 GameAssets·World·Player/Template 저장소·Host에서 다음을 확인한다.

- 앞 구간/반복/반납 관문과 같은 Player/소유권·두 지역 상태·188칸0 벡터·미공개 Runtime.
- 실제 Player152 생성→Owner/Pop→Technology 기록·현재 getter→같은 첫 Regular1/payload5.
- PlayerKnown 진단이 현재 실제 기록에 쓴 뒤 Technology를 재조회함.
- 같은 빈 State28BYTE 미정 입력/개별 초기화, 같은 현재 stream과 PATCH 깊이 불변.
- 짧은 State의 실제 최소 확장/섹션 교체, 원래 payload·0꼬리·커서28 보존.
- manager/미정 입력/Notify 관문의 선택 실패와 앞선 커서/flag 보존.
- 실제 Player Regular의 로그·전투 모드 재조회·Host 사건2/payload60·State bit4/Changed.
- State getter가 바꾼 현재 templateOwner로 Notify하며 앞선 State 쓰기가 남음.
- 실제 신전 Owner의 Player 두 DWORD·생산 갱신/개정, 섬 Owner의 현재 Player 첫 지역.
- 실제 섬 조회 실패의 같은 상세/단언과 그 앞의 Player 신전 포인터0 쓰기.
- 제공 로그/생산 함수의 값 복사와 실제 선택한 누락 오류.
- Ownership 구성 실패의 실제 생성 접두와 종료 시 대여 반납.

최초 신규12검사는11통과/1실패였다. 시험이 두 번째 State getter가 반환한 새 JS 뷰 객체를
다른 저장소로 오해했다. 원문 span의 계약대로 buffer/offset/length와 실제 공유 쓰기로
교정했으며 제품 알고리즘 변경은 없었다. 두 Node 버전의 집중64검사는 모두 통과했다.
신규 모듈 줄100%·분기95.92%·함수96.77%다. 로그는
`extracted/wb-e578-focus2-node26.log`·`extracted/wb-e578-focus2-node22.log`(Git 제외)다.
전체 Node22(v22.23.3)·Node26(v26.13.0)은 각각5,495건 통과, 실패·취소·건너뜀0이다.
로그 `extracted/wb-e578-node22.log`·`extracted/wb-e578-node26.log`는 Git 제외다.
대응표 최종 검사1,160행/오류0(`extracted/wb-e578-port-map-final.log`), port841·partial34·
todo131·제외154, session/oracle0이며 단계28/43은 유지한다.

소리/진단/로그의 외부 장치는 인자 기록 경계다. Template 검사의 빈/짧은35섹션 입력은
실제 manager 서비스를 시험하는 자료이며 캠페인 파일 Load 증거가 아니다. 원본/클론 게임·
브라우저·오라클·디컴파일·공중3종 전용 행동·새 원격 멀티플레이 실행은 하지 않았다.

## 다음 시작점

1268의 같은 CarrierPostPop→BalloonBody→1284 TYPE 파생표(themes/elements 등)부터
원문 생성 순서로 이어간다. 이어 총/미사일·피해·경로·건설과 나머지 실제 모듈/등록을
구성한 뒤 외곽 Session 공개와 같은 Template TypeNames→Objects/Contents·Load·로그인·
프레임/종료로 진행한다. L2/session-dump와 실제 미션 화면·자연 플레이는 미검증이다.
