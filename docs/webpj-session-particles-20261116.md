# Session 소유자색·파편 캡처·입자 결선 (WB04-E584)

`225553df` RawCampaignSession.cpp1427..1462와 멤버703..710을 같은 기반 owner에 연결했다.
원문391의 건물 캡처 자격·1978의 BridgeLifetime·2015의 Flyer와
RawCampaignDevastation.inc1..9의 네 자격 함수도 이 경로에 필요한 부분만 옮겼다.
Session.cpp/.h·Devastation.inc·RawCampaignParticles.cpp/.h·RawShapeToBuffer.cpp/.h·
GameAssets.cpp/.h는 기준선 뒤 HEAD 변경0이다. CPP 수정/디컴파일은 없다.

## 같은 실제 소유 관계

- 지역188토큰과 TYPE별9×256색 표를 실제 다리/73/144에 원문 순서로 만든다. 공통
  OwnerColorTableState가 있으면 같은 토큰 배열을 빌리고, 캡처마다 현재 owner→색 번호를
  다시 읽어 그 행을 선택한다. 초기/범위 오류를 빈 항등 표로 대체하지 않는다.
- 실제 BridgeDraw/ShapeToBuffer가 같은 Pool·현재 TYPE/SHP·보호 RLE를 사용한다.
  addTypeHotspot은 원문처럼 true다. Devastation16VT는 기본 그림-1과 기존 E583 추가 그림을
  쓰고, 기존 도움말 프레임 제외 규칙을 유지한다. 다리 lifeNumbers0의 추가 그림도 유지한다.
- 정확한 건물/유닛 캡처 자격이면 현재 buildingCapture를 선택한다. 아직 설치 전이면
  원문 오류를 낸다. 다른 타입을 일반 이미지로 우회하지 않는다. factoryDraw도 현재 슬롯이다.
- 파편 create/pop/depend/destroy는 같은 World/의존 Form 목록을 사용한다. CRT와 Game 난수도
  기존 owner 객체다. CampaignParticles는 실제 팔레트와 현재 카메라 snapshot을 빌린다.
  Streak TYPE은 GameAux의 현재 칸이며 BuildArrival도 같은 칸을 읽는다.
- 빔 파일명은 실제 SoundList에서 찾는다. EvilRay의 소리 번호는 현재 missileTypes의
  firingSound/impactSound이고 stepSound가 아니다. 위치 재생과 assertion은 호출 때 장치를
  선택하며 상세 문구는 같은 Client 상세 owner에 기록한다.
- 종료는 World의 프로세스를 먼저 비운 뒤 실제 입자 시스템을 역순 반납한다. 실패 접두와
  반복/반납 후 결선 거부도 유지한다. 외곽 Session/Load 완료를 게시하지 않는다.

웹 GameAssets는 기존 로더와 분리된 집계라 팔레트 getter가 빠져 있었다. 선택한 실제
GamePalette를 세 번째 인수로 연결하고 같은 객체를 빌리도록 보완했다. 기본 경로는 CPP의
`d/gifcloud.col`이다. 기존 이름/SHP 전용 두 인수 호출은 유지하되 팔레트를 요구하는 경로는
누락 시 실패한다. 숨은 I/O나 합성 팔레트는 추가하지 않았다.

## 근거와 검사

CPP의 CampaignTempleShrapnel_Native181CapturePixels와
CampaignSession_ActualDropIslandSupportAndParticles는 전체 Load·전용 건물 캡처·파편
실행을 요구하므로 후속 Session 설치 뒤 재대조할 항목이다. 이번 결선은 해당 완료를
뜻하지 않는다. RawCampaignParticlesTests와 기존 ShapeToBuffer/파생 캡처의 원본 fixture
검사는 집중/전체 검사에서 유지한다. 새로운 fixture나 원본 관측은 만들지 않았다.
manifest는804항목(original-x86766 등)으로 그대로다. E583의 추가 그림32행과 하위 모듈의
원본 관측은 각 기존 소비 검사에서 대조하며, 신규13검사는 실제 소유 관계의 소스 계약이다.

신규 검사는 실제 COL·116TYPE/SHP·World를 사용한다. 팔레트 동일 객체/누락·실패 접두,
지역/실제 공통 소유자 색표의 현재값·범위, 다리 전체 mapped RLE/기준점,16VT와 현재 signed
프레임/실제 plain RLE·기존 도움말 제외, 정확한 전용 캡처 선택, 실제 Pop/의존 두 Form/
미짝 삭제, 현재 소리/진단 장치, 장기 카메라·Streak 칸을 확인했다. 실제 불꽃·빔도 같은
Host에 부착하고 첫 프레임의 SoundList 위치 재생 요청/dirty 통지·종료 Kernel0을 확인했다.
표시 장치는 문서화된 suppressed/invalidate 계약으로 dirty를 기록하고, 음향도 위치 재생
callback을 기록한다. 실제 브라우저 출력/소리 청취는 하지 않았으며 별도 월드는 없다.

최초12검사는10통과/2실패였다. TYPE168의 frame0을 불투명하다고 가정했으나 실제 투명
프레임이어서 SHP의 기존 비도움말 불투명 프레임을 입력으로 선택했다. 의존 관계는 Kernel
프로세스라고 가정했으나 실제 두 Depend Form이므로 같은 Pool의 두 슬롯과 Has/Remove를
검증했다. 수정 뒤12건 모두 통과했다. 실제 입자 수명13번째 검사를 더했을 때 생성 즉시
소리를 기대해12통과/1실패였고, 원문 Lightning 첫 프레임 순서로 진행한 뒤13건이 통과했다.
제품 몸체나 기존 단언을 약화하지 않았다.

최종 원문 대조에서 지역 토큰의`std::array::at`를 TypedArray 대입으로 옮긴 경계 차이를
발견했다. 실제 다리 TYPE188/최대 DWORD의 실패 접두 검사를 추가한 결과 누락 예외로
12통과/1실패였고, 첫 토큰 쓰기 전 범위 거부를 보완했다. 전체 검사는 보완 뒤 다시 실행했다.
보완 전 두 버전의5,561건 통과 로그는`extracted/wb-e584-before-bounds-node22.log`·
`wb-e584-before-bounds-node26.log`로 보존하며 최종 집계와 구별한다.

Node22(v22.23.3)·Node26(v26.13.0) 집중78건이 모두 통과했다. GameAssets는 줄/분기/함수
각100%, 신규 Session 입자 모듈은 줄100%·분기88.12%·함수100%다. 전체는 두 버전 각각
5,561건 통과, 실패·취소·건너뜀0이다. 대응표1,160행·오류0이다. 집중/전체 로그는
`extracted/wb-e584-focus-node22.log`·`wb-e584-focus-node26.log`·
`wb-e584-node22.log`·`wb-e584-node26.log`, 대응표는`wb-e584-port-map-pre.log`다(Git 제외).

전체 검사 도중 신규 검사의 파일시스템 입력을 실제 보존명`d/GIFCLOUD.COL`로 맞췄다.
CPP의 소문자 기본 VFS 키는 유지했다. 수정 전 전체 실행은 직접 중단하고 두 버전 모두
다시 끝까지 실행했다. 중단 로그는`extracted/wb-e584-before-case-node22.log`·
`wb-e584-before-case-node26.log`이며 완료/통과로 집계하지 않았다.

## 남은 경계와 다음 시작점

RawCampaignSession은 계속 partial이다. Devastation.inc는16VT 자격만 옮겨 partial이며
시전/효과/통신 몸체는7단계 잔여다. 전체 이식 상태는 port842·partial35·todo129·제외154,
client port300·partial32·todo113이다. 완료 줄 수111,687은 바뀌지 않았고4-D는29/44다
(보관 완료13개 포함, 현행 체크 목록16/31).

다음은 생성자1463의 TeleportPrefix/현재 TYPE 생성자 ABI 선택부터다. 후속 Archer/Missile·
이동/건물·건물 전용 캡처/건설/연소·전체 모듈·외곽 공개·Load/실제 Login/프레임과 앱 연결은
잔여다. 원본/클론 게임·브라우저/오라클·실제 주문 시전·공중3종 전용 생성/생산/행동·새
원격 멀티플레이 실행 검사는 하지 않았다.
