# Session 공유 상태와 생성자 진단 결선 (WB04-E575)

`225553df`의 RawCampaignSession.cpp1010..1121에서 캠페인/독립 typed 경로를
E574의 같은 기반 owner에 결선했다. 전체 RawCampaignSession은 계속 **partial**이며
외곽 공개·전체 생성자·Load는 미완료다. 현재 Runtime을 임시 객체로 공개하지 않는다.

## 범위

새 `raw-campaign-session-state.js`는 기존 실제 상태 클래스를 생성하고 Rune/의식·Carrier/
내용물·피해·건설/배치·경로·총·미션 시작/AI·소리·정기 상태를 같은 Impl 수명으로 보관한다.
기존 raw 소비자가 값형 객체를 받는 GunTarget/GeyserMined/BombEffect/MissionAiSettings/
CarrierDrop은 C++ 헤더의 실제 기본값을 공급한다. 별도 월드나 추정 게임 상태는 만들지 않는다.
기반 owner의 `state`가 Player/SP 전에1010..1075를 결선하고, Player/SP 정상 복귀 뒤
`bindDiagnostics()`가1098..1121을 한 번 결선한다. 구간을 순서대로 조립하는 전체 부모는 잔여다.

현재 Refund/자원/절반 HP·사제/제단/다이스·Rune·레인 바리케이드·너겟을 같은 GameAux/
소유자 모드에서 읽는다. HP는 실제 current TYPE와 최초 로딩 비용표를 구별한다. 회복 표본/
진단과 경로 정책은 같은 장기 상자를 사용한다. MissionStart 출력4칸과 무적/동맹 사격/강제
배치7DWORD 설정, 건설 권한/Server/종류/이력/PlayerKnown/패턴, 군도/여백, FullDeck/보존
장르도 동일 owner를 직접 소비한다. 내용물/Carrier의 로딩 깊이는 같은 Template 저장 전역이다.

웹 소비자에는 함수 getter·`{value}` 상자·일반 숫자 필드가 섞여 있다. 각 실제 읽기 형식에 맞춰
연결했다. 일반 필드만 읽는 GeyserMined에는 현재 getter를 제공하며, 지역 fallback 대입으로
공유 포인터가 가리키는 owner를 덮어쓰지 않는다. RawCheckingFlag의 숫자 getter는 현재
DWORD 전체 비트를 읽는다. 값 대입은 연결을 끊고 독립 입력으로 돌아가며 외부 DWORD에는
쓰지 않는다. 함수 입력도 기존 웹 소비 계약대로 현재 수치로 읽는다.

연결 활동은 같은 Client 두22DWORD 표의 실제 RawConnectionActivity다. 벽시각/프레임/
Server를 실제 순서로 조회하며 미션이면11/16 외 경고 삭제를 억제한다. 선택된 필수 장치가
없으면 앞선 표 쓰기를 남기고 그 지점에서 오류다. 아직 분배기에 등록하거나 전송하지 않았다.

16종 checking, 경로 checking, 동일 Canon 진단 객체와 World의 Mana/Unpop/Pop/Factory/
HP/Building/Graph/기준점 진단을 연결했다. C++ 값 캡처 장치와 호출 때 서비스를 조회하는
장치를 구별한다. BoardAngle은 원문처럼 Storage의 진단 뷰를 사용한다(독립 Storage의 초기
checking0을 임의로 Client checking에 재연결하지 않음). 권한 owner와 asset 전투/로컬 기본도
실제 몸체에 연결했다. Native 전용1051..1062와1100..1102는 웹 typed에 적용하지 않는다.

## 근거와 검사

RawCampaignSession.cpp/.h·RawGameAuxState.h는 기준선 이후 HEAD 변경0이다. 실제
마지막 변경은 각각 `382ec33fe`·`bda77381e`·`8defb347c`이며 새 cpppj 수정은 없다.
본문의 필드/명시 Bind/람다 캡처와 각 웹 소비자의 실제 조회를 정적으로 대조했다.
CampaignSessionTests의 MissionInputsUseSameScrambledSpSlots·ConstructionSupportUsesSameWorld·
InitSharedTablesBeforeInstall도 읽었다. 이들의 전체 Load/생성/패킷 검사를 현재 공유 입력
검사로 통과한 것으로 세지 않으며 실제 Session 조립 뒤 재대조한다.
새 원본 관측/fixture는 추가하지 않았다. 기존 manifest803항목은 유지한다. 원본 실행 파일,
cpppj 오라클·브라우저·디컴파일·공중3종 전용 행동 검사는 실행하지 않았다.

신규12검사는 같은 보호 GameAssets/실제 World·SP에서 다음을 확인한다.

- 같은 경로 시계/사제 역할·피해 상태와 Player/SP 전후 단계, 미공개 Runtime.
- 실제 HP/current TYPE와 최초 비용표, 보상/자원/절반 모드의 현재 읽기/쓰기.
- 실제 채집 소리 뒤 너겟 TYPE/boss 변경 재조회·수량 차감·고갈 삭제 억제.
- 실제 Gun MayTarget가 현재55a490을 읽고, 다른 Gun/배치/피해 소비자도 같은 설정을 봄.
- 실제 MissionStart의 장기 출력4칸과 같은 SP/시작 관문. 기술/UI/카메라는 인자 기록 경계이며
  전체 미션 시작·자연 플레이의 증거가 아니다.
- Rune/Carrier/깊이/회복 표본, 건설 종류/권한/이력/Player/패턴·군도/여백·FullDeck/장르.
- 현재 checking16종·동일 Canon 진단·값 대입 분리, 진단 장치의 캡처/호출 시 조회 차이.
- 실제 연결 활동의 표·시각/Server·미션 경고 억제와 장치 누락의 부분 쓰기.

최초 기존15검사는 통과했다. 신규12건을 더한27건 중21통과/6실패가 나왔다. 제품 오류1개가
2검사에 나타났다: `SquidProcessState`의 웹 Server는 메서드가 아니라 속성이므로
`isServer()`를 `server` 읽기로 교정했다. 나머지4개는 시험 오류였다. 채집/표적2개는 읽기
사본인 `slot()`에 입력을 썼으므로 실제 `allocated()`로 고쳤다.1개는 LocalGeyser의
`readBoss()`를 잘못 호출했고,1개는 HP 진단 조건을 HP0으로 오해했다(실제는 hasHP 플래그).
관련 E574 레벨 시험의 TYPE 입력2곳도 같은 실제 쓰기 뷰로 교정했다.

교정 뒤 Node26 집중27/27 통과, 새 모듈 줄100%·분기97.62%·함수76.71%다.
로그 `extracted/wb-e575-focus1-node26.log`·`wb-e575-focus2-node26.log`·
`wb-e575-focus3-node26.log`(Git 제외). Node22(v22.23.3) 집중도27/27 통과했다
(`extracted/wb-e575-focus4-node22.log`). 전체 Node22(v22.23.3)·Node26(v26.13.0)은
각각5,461건 통과, 실패·취소·건너뜀0으로 끝났다. 로그는 `extracted/wb-e575-node22.log`·
`extracted/wb-e575-node26.log`이며 Git 제외다.
대응표 최종 검사도1,160행/오류0이다(`extracted/wb-e575-port-map-final.log`).
상태는 port841·partial34·todo131·제외154, session/oracle0이며 단계28/43은 그대로다.

## 다음 연결

1122..1151의 웹 제외 모드 뒤 실제 캠페인 Core1 선택(1152)부터 이어간다. 현재
missionPost 비용 관문→실제 Broadcast/Form writer·같은 공통 전송/삭제 헤더→공유 SID 목록/
Player 공장·현재 방향 동맹/Player mask/flag→실제 Islands/사제/Sound/Player/소유권/전투
모듈을 연결해야 한다.1152의 Core1 선택을 E574 Core0 초기화 시험으로 대체하지 않는다.
모든 모듈/등록이 완성된 뒤 외곽 Session을 공개하고 같은 Template TypeNames→Objects/
Contents·Load·로그인·프레임으로 진행한다. L2/session-dump와 실제 미션 화면은 미검증이다.

