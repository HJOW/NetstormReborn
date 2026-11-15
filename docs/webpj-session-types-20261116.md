# Session Carrier·Balloon·TYPE 보충 표 (WB04-E579)

`225553df` RawCampaignSession.cpp1268..1317을 E574~E578의 같은 기반 owner에 연결했다.
실제 CarrierPostPop→BalloonBody→188행 공통 표→보호116정의의 파생 표 순서다.
전체 Session은 계속 **partial**이다. 나머지 생성자·외곽 공개·같은 Template의 전체
Load/로그인/프레임은 잔여이며 임시 Session을 Runtime에 공개하지 않는다.

## 연결 범위

`connectBodiesAndTypeTables()`는 Player/소유권 구간 뒤 한 번 실행한다. Carrier는 같은
현재 TYPE·carrierState·공통 PostPop과 실제 사제 CheckCarrier/World.SetSupportOwner를
사용한다. Balloon은 같은 풀/TYPE/지점 표·현재 Boss·Carrier/사제 CarrierPreDestroy와
실제 World Create/Pop/Unpop/Destroy를 사용하고 공통 PostPop에 등록한다.
Native 장기 Balloon 대여 경로는 이번 독립 typed 캠페인 분기 밖이다.

클라우드 플로터 Regular 예약은 실제 미부착 객체 하나를 확보한다. JS 생성자는 필드만
초기화하므로 빈 객체를 먼저 만들고 `initializeDetached(event,payload)`로 같은 객체를
소비한다. 실제 Host 부착 후 현재 double 시각으로 ResetTask하며 두 번째 객체를 만들지
않는다. 중복/누락 예약과 부착한 객체의 재초기화는 거부한다. JS RangeError는 예약 실패로
돌려주지만 런타임의 복구 불가능한 OOM까지 C++ nothrow와 동일하다고 주장하지 않는다.

원문 멤버의 TYPE 파생 벡터는 기반 자료 생성 시188행0으로 구성하고 이 단계에서 채운다.
행과 벡터 객체를 교체하지 않는다. 모든 행의 signed theme·같은 uint32 비트 테마와 빌더
sourceFrameIndex·원소·A..P 첫 프레임을 현재 Core TYPE/FrameCodeTable에서 읽는다.
미지정 Run의-1을0으로 바꾸지 않는다. 보호116정의는 원래 블록 순서로 읽는다.

- 사정거리·속도·피해·발사 간격은 float32 저장, 높이와 useairdamage는 double에서 int 절삭.
- Gun의 L/M/N/O·빌더 A..P·Archer 방향수와 프레임 수를 각각 구분한다.
- Missile의 frameCount에는 원문대로 Archer 방향수가 들어간다. 피해 세 값은 같은 Hit 행에서 복사한다.
- 정확한 fireSound/impactSound 키의 비어 있지 않은 문자열만 같은 SoundList에서 조회한다.
- 투사체 포구는 실제 물리36BYTE SHP 헤더+32/+34의 signed WORD다. 화면 hotspot과 구별한다.

실패하면 앞서 구성한 몸체/행/소리 이름을 남기고 반복 실행을 거부한다. 종료 시 현재 World
프로세스를 먼저 정리한 뒤 Balloon 등록과 미소비 예약을 반납한다. 다른 TYPE 값이나 외부
Kernel 프로세스를 초기화하지 않는다. 전체 Session 소멸자 구현 완료를 뜻하지 않는다.

## 근거와 검사

Session.cpp/.h와 RawBalloonBody.cpp/.h는 기준선 이후 HEAD 변경0이며 cpppj 수정은 없다.
정적 Squid 생성자 표의 TYPE146→0042b030→00504198과 보호 rainBalloon 에셋을 대조했다.
CloudFloaterTests/BalloonPathTests의 기존 원본 fixture 검사는 전체 회귀에서 유지한다.
새 원본 관측/fixture는 없으며 manifest803항목을 유지한다.

신규9검사는 실제 보호 GameAssets·World/Core·Host·SHP/SoundList에서 다음을 확인한다.

- 앞 단계/반복/반납 관문, 같은 벡터/행,0초기값과 signed theme 비트 및 미공개 Runtime.
- 모든188행/116정의의 파생 표와 기존 실제 투사체 기하 빌더의 전체 일치.
- 마지막 AST 키·float 저장·int 절삭/불확정값·fireSound와 잘못된 별칭 구별.
- 실제 TYPE71 썬 디스크 발사기의 동일 SHP 포구 소비.
- 실제 TYPE146 클라우드 플로터 Create/Pop과 같은 Regular3/float0.1/현재 시각.
- 같은 예약 객체의 실제 Host 등록과 부착 후 재조회한 double 시각.
- 미소비 예약과 현재 프로세스 반납, 기존 벡터 값 보존.
- SHP 읽기 실패 앞의 소리 조회/몸체/표 쓰기 순서와 부분 접두.
- 잘못된 숫자 AST의 선행 자산 검증과 실제 전역/Player 대여 반환.

최초 신규9검사는6통과/3실패였다. 시험에서 mcloud를 클라우드 플로터로 잘못 선택했고,
잘못된 range/orientations 문자열이 선행 TYPE 로더를 통과한다고 가정했다. 실제 정적
생성자 표의 rainBalloon/TYPE146과 선행 검증 시점으로 시험을 교정했다. 제품 검증은
약화하지 않았다. Node22·Node26 집중73검사는 모두 통과했다. 새 모듈 줄100%·분기98.21%·
함수85.19%이며 미실행 외부 World 효과를 억지 호출하여 범위를 부풀리지 않았다.
로그 `extracted/wb-e579-focus2-node22.log`·`extracted/wb-e579-focus2-node26.log`는 Git 제외다.

전체 Node22(v22.23.3)·Node26(v26.13.0)은 각각5,504건 통과, 실패·취소·건너뜀0이다.
로그 `extracted/wb-e579-node22.log`·`extracted/wb-e579-node26.log`는 Git 제외다.
대응표 검사1,160행/오류0(`extracted/wb-e579-port-map-pre.log`)이다. port841·partial34·todo131·제외154,
session/oracle0·단계28/43은 유지한다. 원본/클론 게임·브라우저·오라클·디컴파일·공중3종
전용 생성/행동·새 원격 멀티플레이 실행은 하지 않았다.

## 다음 시작점

1318 GunMode→GunTarget→EnergySupply/CursorMana/Range부터 원문 생성 순서로 이어간다.
총/미사일·피해·경로·건설과 나머지 실제 모듈/등록을 구성한 뒤 외곽 Session 공개와
같은 Template TypeNames→Objects/Contents·Load·로그인·프레임/종료로 진행한다.
L2/session-dump와 실제 미션 화면·자연 플레이는 미검증이다.
