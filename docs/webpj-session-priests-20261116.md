# Session 섬·소리·사제 연결 (WB04-E577)

`225553df` RawCampaignSession.cpp1188..1235의 실제 typed 캠페인 생성 순서를 E574~E576의
같은 owner에 연결했다. RawIslandRuntime→사제 효과→SoundProcessSystem→RawPriestWorldBindings
→Process/Authority/Player/로컬 번호→RawPriestParticipation 순서다. 전체 Session은 여전히
**partial**이다. Player/Template·소유권/피해/이동/건설·나머지 모듈과 외곽 공개/Load는 잔여다.

## 실제 연결과 수명

`connectIslandsAndPriests()`는 같은 Player 테이블 구간이 정상 복귀한 뒤 한 번 실행한다.
부분 실패에서는 앞서 생성/연결한 섬·소리·콜백을 되돌리지 않으며 반복 결선을 거부한다.
이 구간의 ready는 전체 Session/Load나 사제의 모든 순환 참조 완료를 뜻하지 않는다.

섬은 같은 pool/hash·3204BYTE 기록·finishState·장기 거주 구역 목록과 현재 개정 상자를
빌린다. 다시 Reset하지 않는다. worldRedraw는 실제 ClientKencloudGlobals의 같은 상자이며
독립 Session만 지역0 상자를 가진다(원문621). 실제 소유권 모듈이 나중에 설치되면 같은
현재 슬롯에서 SetOwner를 호출한다. MissingIsland/InvalidBounds 진단은 원문 SessionMissing
1189의 명시 오류다. 아직 생성자 뒤의 최종 섬/Player 설치를 앞당기지 않았다.

사제의 Pop/Unpop/Repop/Destroy/받침/비사제 소유자/Find는 실제 같은 World에 연결한다.
OwnerNotice의 기본 몸체와 NotifySurface도 같은 stateless building 통지다. 보호막 진단은
호출 때 현재 services.ai.assertion을 조회하고, Owner 진단은 같은 장기 Storage를 보관한다.
services.priest/shield/regen과 sound는 값형으로 복사해 외부 묶음 재대입을 분리한다.
제공된 ownerNotice/fallSound/외부 Walker 후단과 선택형 장치는 원문 보존 규칙을 따른다.

SoundProcessSystem은 같은 실제 Host·soundState·소리 장치를 쓴다. 보호막 이름 조회는 같은
SoundList, 부착은 실제 Sound47이다. 기본 낙하 소리는 현재 위치·priestFall.wav·flags0을
현재 서비스에 전달한다. 실제 브라우저 재생기는 아직 이 단계에 설치하지 않았다.
측정/번역/Tell/안내 소리 누락은 원문 FillMissing처럼 해당 경계가 선택될 때만 실패한다.

회복은 같은 display의 **현재 raw 물리**를 넘기며 슬롯 사본을 만들지 않는다. 감사 Rotate와
다음 SP 읽기를 분리한다. 독립 SP는 실제 지역 사제 추적 holder, 공통 SP는 Client의 기존
추적 holder를 그대로 사용한다. 피해/Burning·panic·Carrier 낙하는 후속 실제 모듈 슬롯에서
선택하며 설치 전에는 명시 오류다. 원문2723..2729의 너겟 보충 부모도 같은 실제 Bounds와
현재 권한/모드/군도 getter를 연결했다. 현 단계의 construction 미설치는 원문 위치에서
거부한다. 실제 Bounds를 요구하는 후단 실행은 Construction 연결 뒤에 검증한다.

실제 사제 Bindings는 같은 World core·HP·난수·hash0/spot·공장·후처리·프레임을 빌린다.
같은 ProcessHost/권한/Player9와 로컬1을 연결한다. Damageable/Placement 순환 참조와
Regular의 전체 Runtime 분배는 후속 구간에 남았다. 사제 참여는 같은 장기 목록/owner 표와
현재 사제·제단·다이스 TYPE를 빌리며 생성 때 owner 표를 초기화하지 않는다.

JS의 명시 종료에서는 RawSurfaceWorld의 기존 현재 대여/Host 필터로 프로세스만 먼저
반납하고 사제 postPop 등록/Player 뷰를 해제한 뒤 World·Player·게임 전역 대여를 반납한다.
이를 위한 drainOwnedProcessesForShutdown 진입점은 기존 동일 반납 몸체를 재사용한다.
외부 Kernel의 다른 BaseProcess는 남는다. Kernel 최종 반납은 Form 삭제 통지가 아니다.
반복 소리 stop은 원문처럼 실제 Form 삭제 OnFormDestroy에서 검증한다.

## 공유 사제 조회 교정

실제 참여 조회를 연결하자 장기 목록에 사제를 넣어도 find가0을 반환했다. 기존 웹
RawPriestLookup이 state.priests만 읽어 sharedPriests를 무시한 이식 누락이었다.
기준선과 기존 base1f00a25a 모두 C++ state.Priests()를 사용한다(두 기준 사이 소스 변경0).
현재 currentPriests()를 사용하고 기존 독립 구조 입력은 유지하도록 교정했다.
같은 Session 장기 목록/owner 쓰기와 현재 사제 TYPE 회귀 검사를 추가했다. 원본10.78
fixture priestlookup1617행도 기존 입력/반환/바이트 불변 기준으로 다시 통과했다.
연결 fixture는 `cpppj/tests/fixtures/priestlookup-x86.tsv`의 originals1617행이며 파일 SHA256은
`c18f6985bd89a33cb65951fc70253960658170b8f7f40e97f9053446f531c907`이다.

## 근거와 검증

Session·PriestWorldBindings·SoundProcess·IslandRuntime·PriestLookup·RawSurfaceWorld는
기준선 이후 HEAD 변경0이며 cpppj 수정은 없다. 마지막 소스 변경은 Session382ec33fe,
PriestWorldBindings4b6efc0ef, IslandRuntimef0d065dce, PriestLookup1e8e43d9a다.
각 생성자/실제 소비자와 원문 참조 멤버·서비스 값 복사를 정적으로 대조했다.
CampaignSession_PriestParticipationUsesCurrentSharedState와
PriestOwnerStatesUseActualSurfaceStorageAndDataReset의 같은 장기 목록/owner 계약도 읽었다.
전체 CampaignSession의 Load/로그인 검사를 이 부분 검사로 통과했다고 세지 않는다.

신규12검사는 실제 보호 GameAssets·World·Player/SP·Host에서 생성 단계/공유 참조·기존
섬 기록/Client redraw·실제 Sound47 부착/정지·외부 Kernel 보존·서비스 복사·진단/누락·
같은 장기 사제 목록/참여·독립/공통 SP와 감사 회전·같은 표시 물리·부분 실패를 확인한다.
소리/표시/진단의 외부 장치는 인자 관찰 경계이며 실제 브라우저 음향/화면 증거가 아니다.

기존37집중은 먼저 통과했다. 최초 신규11검사는7통과/4실패였다. 제품 오류1개는 위 공유
목록 누락이었다. 시험 오류3개는 원문이 허용하지 않는 좌/상0 섬 경계, 없는 Host.findContained
호출, ClientSpGlobals 대신 store().initialize()를 호출해야 하는 API 차이였다.
교정 뒤51검사 중50통과/1실패였고, 이는 Kernel 반납을 Form 삭제 통지로 오해한 시험 오류였다.
실제 Host.kill과 최종 반납을 나눠 원문 순서로 고쳤다. 표시 참조 검사를 더한 최종 집중은 Node22(v22.23.3)·Node26(v26.13.0)
각각52/52 통과했다. 새 모듈 줄90.91%·분기88.46%·함수55.56%이며 실제 Carrier 후단/
Construction 보충과 아직 전체 설치 전인 World 콜백은 후속 부모 연결에서 검증한다.
로그는 `extracted/wb-e577-focus2-node26.log`·`wb-e577-focus3-node26.log`·
`wb-e577-focus5-node26.log`·`wb-e577-focus5-node22.log`(Git 제외)다.

전체 Node22(v22.23.3)·Node26(v26.13.0)은 각각5,483건 통과, 실패·취소·건너뜀0이다.
로그 `extracted/wb-e577-node22.log`·`extracted/wb-e577-node26.log`는 Git 제외다.
대응표 최종 검사1,160행/오류0(`extracted/wb-e577-port-map-final.log`), port841·partial34·
todo131·제외154, session/oracle0이며 단계28/43은 유지한다.

새 원본 관측/fixture는 없고 기존 manifest803항목은 유지한다. 원본/클론 게임·오라클·
브라우저·디컴파일·공중3종 전용 행동·새 원격 멀티플레이 실행 검사는 하지 않았다.

## 다음 시작점

원문1236의 실제 PlayerWorldHooks/RawPlayerWorld→TemplateState/PlayerRegular→1257의
RawCampaignOwnership부터 같은 owner에 이어 연결한다. 이후 피해·이동·건설·나머지 모듈/
등록과 실제 외곽 공개, 같은 Template TypeNames→Objects/Contents·Load·Login·프레임/
종료가 남았다. L2/session-dump와 실제 미션 화면/자연 플레이는 아직 미검증이다.
