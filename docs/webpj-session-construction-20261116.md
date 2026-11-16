# Session의 실제 건설·간헐천 부모 (WB04-E603)

`225553df`의 생성자1652/ConnectConstruction2731..2774 독립 typed
분기를 같은 owner에 연결했다. 함께 쓰는 MayPlaceGameplay3584..3593과
mutable Chat/RemoteTell4134..4141의 캠페인 분기도 옮겼다.
RawCampaignSession.cpp·RawSurfaceStorage.cpp·RawSurfaceWorld.h의
기준선 뒤 변경은0이다. Native 장기 건설 owner는 실제 Client 결선에서
남기고 기존 웹 제외 Editor 전송은 추가하지 않았다.

## 같은 물리와 현재 소비자

- 같은 World/Core·HP/SP·Player9·프레임/청크·방송·건설 상태에 기존
  RawCampaignConstructionBindings를 구성한다. 새 Pool/근사 월드·가짜
  성공/임시 생성은 없다. 구성 자체는 게임 난수/객체/패킷을 소비하지 않는다.
- 최근 간헐천5DWORD/count와 PE0053181c의 방향9DWORD를 Storage에
  최초 한 번 확보했다. 전체 간헐천 목록과 다르며 ConnectConstruction은
  현재 count/꼬리를 초기화하지 않는다. 준비 대여/반납은 보존하고 일반
  fresh 대여만 원문 Graph reset 뒤 최근 목록을0으로 쓴다. 방향표는
  일반 fresh에서도 보존한다. PlayerBounds는 현재 옵션BYTE/방향표/
  미션/패턴TYPE/현재 Canon 진단을 조회한다.
- effects/presentation의 값 사본을 유지하며 configInt/벽시각/refresh/
  수량·범위 진단의 누락은 선택된 호출에서만 명시 오류다. 섬 존재는
  같은 현재 IslandList의 bool이며 source의 bool 반환을 보존한다.
- 기본 환불6ch/취소2fh는 E602 writer를 사용하고 설치 관문 뒤 현재
  Login의 mutable 전송에 보낸다. Tell70h는 같은264BYTE 패킷을
  Player1..8 순서로 공유해 앞선 observer 변경과 현재 Login 교체를
  다음 대상에 보존한다. 수신을 동기로 우회하지 않는다.
- 기본 경고 서식은 기존 순수 MessageGump formatter를 재사용한다.
  마지막 로컬 Tell의 표시 해독은 E269의 웹 UTF-16 정책대로
  decodeOriginalText를 쓴다. cpppj 전용4BYTE 글꼴 부호화는 도입하지
  않는다(기존 OriginalText의 의도된 웹 적응).
- 기록 활성은 같은 Client playing DWORD를 빌리고 라벨/통지는 매 호출
  현재 weak context를 다시 잠근다. 사제/내용물 조회는 같은 실제
  Pool/목록이다. 건물/Builder·Graph 접근/이동/시작은 원문 인자와
  현재 게임 시각을 실제 후설치 몸체로 전달한다. 미설치는 명시 오류다.
- MayPlace는 같은 현재 Player/동맹/Graph/100개 섬 기록을 실제 전체
  파이프라인에 공급한다. 파이프라인 생성 자체는 후속 생성 구간이며
  이 단위에서 빈 성공으로 대신하지 않는다.

## 검증·최초 실패와 경계

전용 원본 관측은 없다. 실제 기존 몸체/보호 자산과 명시 소비자 계약을
대조했다. 신규11건 첫 실행은10통과/1실패였고, 시험의 재생 장치 이름
onBoard/notify를 실제 API validPoint/label로 고쳐 보드 밖 라벨
조건에 맞췄다. 제품 재생 몸체는 바꾸지 않았다. 이후 신규11건과
관련42건이 통과했다. 검토 중 섬 존재 훅의 반환도 C++ bool로 명시했다.
Node v22.23.3·v26.13.0 전체는 각각5799건 통과, 실패/취소/
건너뜀0·종료0이다. bool 반환을 명시한 신규11건의 재검사도 통과했다.
대응표1160행/오류0, port846/partial39/todo122·session0,
잔여161단위/34340줄·JS131341줄/검사773파일이다.
Node24는 사용자 지시로 생략하고22/26 통과 시 호환을 가정한다. Source 계약을 원본
관측으로 등재하지 않아 직접 manifest 항목0개/총806항목은 같다.

다음은 생성자1653 ConnectDamageable,1654 IceTower·운반/주문·이동/
건물과 나머지 모듈 생성이다. World.BindRuntime1825·외곽 공개·Load/
Login·프레임·L2/L3와 실제 미션 플레이는 아직 미완료다. 새 서비스만
연결한 시험을 실제 미션/자연 완주로 세지 않았다. 보호 파일/cpppj 수정·
디컴파일·게임/브라우저 실행·실제 포획/희생·특정 적 공격/승리·공중3종
전용 관측은 하지 않았다.

로그: `extracted/wb-e603-initial.log`, `wb-e603-focused.log`,
`wb-e603-focused-final.log`, `wb-e603-node22.log`, `wb-e603-node26.log`.

추가 로그: `extracted/wb-e603-boolean-check.log`, `wb-e603-port-map.log`.
