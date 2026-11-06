# cpppj 공유 전송과 실제 World 수명 접합 — 2026-11-06, 단위984

제품 **984 / 11.01**은 원문 전역 패킷 등록표와 번호순 복합 폴링을 실제 단독 미션의 Client/World에 연결한다. 982의 TCP 미참조 부품 등록에서 진행한 단위다. **LAN 역할은 컴파일했지만 Multiplayer UI·방 roster·일반 전투는 아직 미접합**이다. 작업 호스트와 마지막 정밀 디컴파일 호스트는 **HJOW-Athlon**, 마지막 정밀 자료 날짜는 **2026-11-03**으로 같고 새 디컴파일은 하지 않았다.

## 원문 구조와 현재 연결

| 원문 10.78 함수 | 실제 반영 |
| --- | --- |
| 004ded50/70/90·004df240 | `PacketRegistrationTable` 하나에 receiver/observer/version/mismatch/length를 두고 Local/TCP/Editor가 공유한다. backend별 시계/기록/close/free는 별도로 유지한다. |
| 004e28b0·004df560·004e2920 | `CompositeMissionTransport`가 TCP→zlocal listener 수락, pending -1..-31 각1회, normal0..21의 ID별 sentinel 반복, normal0..21 flush를 실행한다. backend 전체 Poll 두 개를 연달아 호출하지 않는다. |
| 004e2430·004e02d0·004e18d0 | 내부 전송도 listener Open→client9 Connect→Poll accept로 분리한다. 두 backend가 같은 Registry의 빈 음수 슬롯을 받는다. |
| 004e27b0 | 동일 accepted 노드의 owner 재배치와 buffered bit2를 유지한다. 접속 완료는 실제 내부 Connect/Accept 또는 TCP OS 상태로 읽는다. |
| 004dfc00·004dfa40·004df9f0·004df740 | 기존 단일/집합 분배를 재사용한다. proxy 분배 없이 observer/record/receiver와 실제 depth를 한 번씩 처리한다. |
| 004df440·004e2940 | Registry가 현재 실제 owner의 close/free를 선택한다. TCP 물리 수거는 외곽 호출/콜백/깊이 복귀까지 기다린다. |

현재 `UberGump`는 실제 단독 GameWorld를 shared owner로 만든 뒤, 지연 로그인 시작 전에 `BindConnectionTransport`를 호출한다. `CampaignTransportServices`의 같은 현재 ID API를 Login/Chat/명령 writer가 사용한다. 기존 직접 `Client()/Server()/flags2` 의존을 현재 Registry·실제 status/open/주소 호출로 바꿨다. 미션/owner/Ready 값을 강제로 써서 일반 전투 경계를 넘기지 않는다. Editor는 현재 기존 내부 장치를 유지하며 복합 Editor 바인더는 API/컴파일 범위다.

## 실제 소유권과 정리 경계

등록 대여는 **실제 GameWorld 전체**를 약하게 빌린다. 수신/관찰/record와 송신 중 observer 이후 실제 패킷 복사가 끝날 때까지 강한 pin을 보유한다. 이 owner에는 실제 RawCampaignSession·표시/선택 소비자가 함께 들어 있다. 단순 Login 표식으로 전체 Runtime 수명을 대신하지 않는다.

장기 backend는 Client의 실제 Player 할당·서버 권한·동일 GameClock owner를 보유한다. 시계는 같은 원점/정지 상태의 객체를 shared owner로 옮겼고 모든 기존 사용자는 같은 객체를 읽는다. 기록 훅은 weak World를 재조회한 뒤 현재 Login 기록을 호출하므로 지난 Login의 raw this를 장기 listener에 저장하지 않는다. World→서비스→전송→등록 대여→World의 강한 순환도 만들지 않는다.

Detach는 자기 세대가 바꾼 등록 슬롯만 noexcept snapshot swap으로 복원한다. 실행 중인 이전 세대가 복귀하기 전 새 세대 등록/전역 변경을 거부한다. callback 안에서 전체 Tick이 재진입하면 State/후속 명령 전체를 다음 정상 Tick에 남긴다. 직접 Native 정리·Runtime 반납·World 정리는 첫 파괴 효과 전에 거부하여 같은 Player/Pool/Kernel을 먼저 비우지 않는다.

추가 정적 리뷰에서 record/observer가 depth0 연결을 즉시 Disconnect한 뒤 분배 말미가 같은 close/free를 다시 요청하는 경계를 찾았다. 현재 token·실제 주소·present/alive/retired 상태를 검증해 이미 실제 반납한 같은 끝만 중복 요청을 생략한다. 다른 주소/세대는 오류이며 callback 중 같은 고정 노드의 새 세대 Open도 거부한다. 원문의 depth>0 지연 반납과 depth0 close→free 순서는 유지한다.

이 공유 owner/token·재진입 거부는 Windows C++ 호스트의 수명 경계다. 동시 스레드 안전이나 원본32비트 heap/포인터/SEH 바이트 동일성까지 검증한 것은 아니다.

## 확인한 범위

Release 첫 제품 빌드와 close 재진입 보강 후 증분 빌드 **두 명령 모두 종료0·경고/오류0**이었다. cpppj 소스/CMake31파일을 변경했으며 신규 소스/헤더는9개다. 새 단위·MP·소켓·통신 검사는 작성하거나 실행하지 않았다.

실제 클론 한 번을 격리 게임/사용자 루트로 실행해 **1920×1080 tutorial1 진입→비정지100프레임 대기→WM_CLOSE 종료0**을 확인했다. 앱 내부 입력5개·보고7개, State9→10→13→14와 게임 시각 증가, 내부 연결 두 끝 반납 로그를 보관했다. 미션 PNG도 확인했다. 설치1351파일·기존 보호2800개·cpppj fixture980개는 실제 SHA-256 불변이며 새 fixture는 없다. 이는 LAN/공격/포획/희생/승리·물리 입력 검증이 아니다. 초기 인트로 파일 부재/첫 색 캐시 읽기 진단은 stderr에 보존했으며 종료 코드는0이었다.

현재 EXE는 **11,512,320바이트**, SHA-256 `82f65d6e393dfb3829439de7ff6d7bac4b27397b63100df7f8b8d15cc22d3393`이다. 실제 코드 참조로 `WS2_32.dll`이 링크되지만 `InternalMission` 역할은 TCP backend를 생성하지 않는다. import 존재를 실제 LAN 접속 성공으로 세지 않는다. 공식 서버/DNS fallback/R.exe/root6800/폐기 클라이언트 유효성·7f/80은 연결하지 않았다.

정리된 증거는 [단위 보고서](../../cpppj/shared-transport-lifetime-20261106.json), 로컬 원시는 Git 제외 경로 `extracted/cpp-resume-984/`에 있다. 다른 세션의 WB 변경과 준비 중인 한국어/Scenario 파일은 이 단위에 포함하지 않는다.

## 남은 작업

다음985는 원문004a7800의 일반 Login 부모 분기다. 원격 Fort 이미지/004a7350 월드 동기화/실제 전투섬·중앙섬, 방 subscriber/roster/Zone, 일반 State6/7/8/9/13, Commander, LAN UI, 재접속 저장/메타맵/Outpost/checking8이 남는다. 현재 Multiplayer 메뉴는 여전히 NotImplemented다. 한국어 final-pending과 Scenario 통합 준비본도 실제 반영/검증 후에 완료로 기록한다. 전체 목표는 진행 중이다.
