# Session 기반 소유 자료와 Player/SP 생성 구간 (WB04-E574)

`225553df`의 `RawCampaignSession.cpp`에서 실제 Session이 소유하는 기반 자료를
`raw-campaign-session-owners.js`로 구성했다. 대응표의 이 단위는 **todo→partial**이다.
전체 생성자·Load·외곽 Session·현재 Runtime 공개는 미완료다. D1 체크는 열어 둔다.

## 범위와 근거

- 원문339..362의 캠페인용 자산/게임 전역 정체성 검사,366..378의 SessionAssets,
  438..950 멤버 선언 중 기반 공유 owner·실제 참조,990..1008 초기화 목록 중 기반 자료를 옮겼다.
- 원문1076..1095의 공통 SP/난수·Player/Core 검증, 현재 Core0 선택, Player 채택/초기화,
  동일 Player 뷰 게시, 독립 SP/추적 초기화, SquidLevel과 Fort/known 관문은
  `initializePlayersAndSp()`로 옮겼다. 앞선 공유 상태 결선 뒤 실제 부모가 한 번 호출해야 한다.
- 이 구간은 생성자 전체와 구별한다.1010..1075의 나머지 공유 상태/연결 활동 결선,
  1098 이후 진단·자산 보충 표·전체 모듈 조립·Connect 함수·Fort 설치는 아직 연결하지 않았다.
  기반 생성자 성공이나 `playersReady`는 전체 Session/미션 성공이 아니다.
- 실제 마지막 소스 변경은 RawCampaignSession.cpp `382ec33fe`, RawGameAuxState.h
  `8defb347c`다. 두 파일과 RawCampaignSession.h는 기준선 이후 HEAD 차이0을 확인했다.
  cpppj 소스·원본·공통/dotnetpj 인계는 수정하지 않았다. 디컴파일·원본/브라우저 실행도 없다.

## 실제 소유와 수명

GameAssets의116자산에서188TYPE과 프레임 코드를 구성하며 현재 mutable TYPE와 최초 비용표를
구별한다. 같은 보호 VFS와 GameAssets 자체는 빌린다. World의 실제 풀·장부·Kernel·표면
저장소를 구성하고 같은 권한/GameAux를 연결한다. 같은 Template의 serialization owner가
World와 FortObjectLoad의 현재 깊이/판본/scope/저장 마스크를 함께 소유한다.

게임 전역 대여→Player/Core owner 및 대여→Template/자산/World→난수/SP/HP 순서를 보존한다.
초기 생성은 Player를 할당하거나 SP를 초기화하지 않는다. 독립 입력은 C++의 CRT0/Game0/mode0을
사용하고 Client의 CRT1/Game55 기본값을 대신 쓰지 않는다. 공통 SP는 실제 같은 난수 owner와
초기화 완료가 필요하며 이미 만든 Player를 채택하면 실제 기록/동맹/공장 주소를 다시 쓰지 않는다.
독립 SP의 추적 상자는 같은 사제 회복 상태에 연결한다. AI 할당 입력은 실제 할당 때만 요구한다.

같은 장기 Template·TypeNames 변환표·Chat·State·연결 활동 owner와 옵션87BYTE·금지256DWORD를
빌린다. 숨김9BYTE는 옵션의0x3e 실제 뷰다. 청크·섬 바이트·Dude 목록·도장 캐시와
FullDeck/개정/로그인 기한/저장 게임도 기존 실제 owner를 직접 사용한다.

Impl 선언에서 조건 없이 빌리는 GameAux 역할8개(무료 기술153·신전 제품133·플레어106·
배리어 표식90/포스 필드170·aviary 프로세스32/59/60)를 추가했다. 일반 fresh/reset/반납이
현재 번호를 덮어쓰지 않는다. 독립 typed 분기의 piece107·Dude provider0·세 flyer73/138/144는
원문 지역 값으로 유지한다. 웹에 없는 Native 분기를 현재 전역으로 임의 승격하지 않는다.

생성 중 예외는 완성한 World→Player 대여→게임 전역 대여를 반납한다. Player/SP 구간의
시각 예외는 이미 만든 Player와 SP 추적 연결을 유지하고 완료를 게시하지 않는다. 같은 생성
구간을 재실행하지 않는다. `dispose`는 기반 반납이며 전체 부모는 먼저 자식/수신기/Runtime
공개를 실제 소멸 순서로 닫아야 한다. State 반납 증빙은 여기서 만들지 않는다.

## 검사

신규15검사는 실제 보호 GameAssets/World/Player/Core/SP를 사용한다. 자산188/116·수정 표의
독립성·같은 Template 깊이와 현재 공유 참조·옵션/금지 뷰·사전 거부와 실제 대여 시점·중복 대여·
생성 중 역순 반납·Player9/방향 동맹81·공통 SP/난수 수명·이전 Core0 선택·부분 실패·AI 실제
바이트/반납·미초기화 채택 거부·현재 SquidLevel 진단을 확인했다. 초기 SP 결과는 같은 실제
기존 시드/SP 부품에 동등 입력을 제공해 비교했으며 새 원본 관측이나 독립 수치 오라클이 아니다.

기존 GameAux 검사는 새8칸까지 포함해79scalar·미션7DWORD·장식4칸을 보호 PE의 실제
주소/폭으로 읽어 비교한다. TYPE 이름 대조는48칸이다. 원본 SHA-256은 기존 manifest의
`a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236`이며 읽기 전용이다.
새 fixture는 추가하지 않았고 manifest803항목의 기존 PE 범위를79칸으로 갱신한다.

첫 집중23검사는20통과/3실패였다.2건은 새8칸을 추가한 뒤 고정 개수71/40을79/48로
갱신하지 않은 시험 오류였다(PE 실제 바이트 비교는 처음부터 일치).1건은 원문 Core0 선택의
현재 포인터 null을 캐시 포인터로 잘못 기대한 시험 오류였다. 원문 RawPlayerCoreData의
0 분기를 확인해 기대값을 고쳤다. 제품 알고리즘 교정은 없었다. 채택 거부/레벨 진단을 더한
집중 Node26은25/25 통과했다. 새 모듈 줄100%·분기97.41%·함수84.85%이며 미선택 가상80
콜백은 전체 부모의 연결 대상이다. 로그 `extracted/wb-e574-focus3-node26.log`(Git 제외).

Node22(v22.23.3) 집중도25/25 통과했다(`extracted/wb-e574-focus4-node22.log`).
대응표1,160행은 오류0이며 port841·partial34·todo131·제외154, session/oracle0이다.
남은165단위와 단계 진행28/43은 그대로다. 전체 Node22(v22.23.3)·Node26(v26.13.0)은 각각
5,449건 통과, 실패·취소·건너뜀0으로 끝났다. 로그는 `extracted/wb-e574-node22.log`·
`extracted/wb-e574-node26.log`이며 Git 제외다.

## 다음 시작점

같은 owner에 나머지 Impl 상태를 추가하고1010..1075의 shared 입력·연결 활동부터 원문 순서로
결선한다. `initializePlayersAndSp` 앞뒤를 정확히 배치한 뒤1098 이후 실제 자산 보충 표와 모든
하위 모듈을 생성한다. 기존 E519·E561~E573 모듈을 같은 core/현재 owner에 설치하고 마지막에
실제 외곽 Session을 공개한다. Load는 TypeNames→Objects/Contents와 전체 후처리를 같은
Template에서 연결해야 한다. 로컬 로그인·프레임·L2/session-dump·미션 화면/플레이는 잔여다.

