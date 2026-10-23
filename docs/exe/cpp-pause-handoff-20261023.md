# 2026-10-23 사용자 요청 목표 일시 정지 인수인계

현재/마지막 전체 정밀 디컴파일 PC는 **VM-W11-Codex / 2026-10-23**이다. 원본/CD/AGENTS/정밀 디컴파일 산출물은 변경하지 않았다. 사용자가 시간이 없어 인수인계 작성·로컬 커밋 후 목표 일시 정지를 요청했다. 전체 미션/캠페인/데모/Edit/HUD 목표는 완료가 아니다. 원격 push는 하지 않는다.

## 완료한 직전 단위

- 단위371 `bb913242 /20261023-39 CP`: 실제 Gump 입력 경계/프레임 순서. 전체 CTest2416/0은 이 커밋의 결과다.
- 단위372 `42021fd2 /20261023-40 CP`: 메인 메뉴 전 두루마리 지도 로딩 표시. 최종 빌드 경고·오류0/관련15검사 통과/실제1024 메뉴 core 회귀 통과. 1024×768·640×480·1280×720 전체RGB 차이0,1280×800은 지도RGB1.0이며 우하단 Windows 워터마크2451픽셀 차이는 별도 한계로 남았다.

## 현재 단위373: 미션 로딩 창 연결 중단

사용자가 확인한 기준 화면은 `extracted/analyzeManager/20261023T082847962Z-44a65267bba2/screens/90f2ef29695deffae34521b73857194f6a42ca44fedc7bad6870f45e802da772.png`이다. Early Missions 첫 미션에서 구름/왼쪽HUD·폭400 돌 창·Starting Mission...·Cancel을 관측했다. 원본 게임은 사용자가 직접 종료했다. 세션 정리 완료/전체 originals 전후 해시 동일이며 원본 게임/녹화 프로세스는 남기지 않았다.

MissionLoadingMessage를 공식 CMake/검사에 등록했다. 원문 cf570의1540입력/두 x87정밀도3080회/1638460명령/정밀도차이0을 사용한다. `python -X utf8 tools/decomp_mission_loading_message.py --verify` 종료0이다. 원문의 일반 HTML 하위 생성·GDI·Fit 전체를 원문 실행했다는 뜻은 아니다.

RawMenuControls에 실제 공통 등록표의 장식 부모/ScrollingText/Instant StoneButton을 연결하고 cf570의 transient serial 재사용을 공급했다. 현재는 기본 미션 안내만 처리한다. 일반 HTML 부모 전체의8192바이트 입력·20버튼·OnExit/Timeout·다중 메뉴 열은 아직 아니다. 짧은 본문 Fit의 실제40/65/30 상수를 사용하며 긴 본문은 명시 예외다. 왼쪽85 지도 영역도 전체 HUD Dock의 현재 사각형으로 연결해야 한다.

UberGump의 Courier 단독 문구를 제거하고 raw 창을 그린다. Client의 기존 Draw/Lock/Present를 PresentScene으로 공통화하여 동기 파일 적재 전에 출력한다. 비 Tutorial은 다음 State로 로드 예약하여 첫 입력/Cancel을 처리하게 바꾸었다. 이 일정 변경과 캠페인/데모 진입의 회귀는 미검증이다. Cancel은 같은 명령 큐에 LeaveBattle(3,3)을 예약하고 부모를 Kill한다.

**현재 제품은 미션 로딩 검증 실패 상태다. 완료 또는 사용 준비로 판정하지 않는다.** build1은 잘못된 alignment 필드와 미사용 body 경고로 실패했고, alignFlags/미사용 변수 제거 후 build2는 종료0·경고/오류0이다. `--filter MissionLoading`은4검사 중 기존 원문 모듈3검사 통과/새 실제 창1검사 실패다. `--filter RawMenuControls`는10검사 중 기존9통과/같은 새 창1실패다. 실패는 `void인 Gump 밀어내기` 예외이며 두 필터의 실패는 같은 검사다. 관련 로그는 `extracted/cpp-mission-loading-20261023-build2.log`, `...tests-message.log`, `...tests-rawmenu.log`에 있다. 수정 제품 GUI/원본 픽셀 비교/새 전체 CTest는 실행하지 않았다.

### 재개 시 첫 작업

1. AGENTS.md와 LEFT_JOBS.md/LEFT_JOBS.dotnetpj.md를 다시 읽고 현재 호스트와 마지막 디컴파일 호스트를 확인한다.
2. RawMenuControls::LoadingMetadata/LoadingBody의 실제 생성→초기 Attach→Push→재부착 순서를 native c83b0/c7aa0/c7d40과 대조한다. 새 버튼이 void 상태인데 LoadingBody가 Push하는 곳이 우선 후보이며 아직 원인 확정은 아니다. native body 역시 생성 직후0,0으로 부모에 붙인 뒤 Fit에서 Push한다. 예외를 숨기거나 GumpTree의 void 검사를 약화하지 않는다.
3. 새 RawMenuControls_MissionLoadingUsesSharedTreeAndCancel 검사와 MissionLoadingMessage3검사를 먼저 통과시킨다. serial 재사용/자식 반납/Cancel(3,3) 명령을 실제 트리로 검증한다.
4. 튜토리얼·전체 캠페인·데모의 공통 진입, Cancel 입력 가능 시점, 부모가 남지 않는지 실제 GUI에서 확인한다. 사용자 확인 원본 캡처와 돌 질감 타일 원점/모서리·문자·버튼/지도 사각형/정수 절삭을 대조한다. 고정 대기시간을 추가해 캡처를 통과시키지 않는다.
5. 이후 관련 회귀와 필요 시 전체 CTest를 실행하고 실제 검증 결과로 인수인계를 갱신한다.

## 함께 보존한 미등록 준비 자료

아래 자료는 이번 중단 커밋에 보존하지만 **공식 CMake 등록/실제 Client 연결 완료가 아니다.** manifest JSON에 경로·바이트·SHA를 기록했다. 과거 개별 초안의 ‘미빌드’ 등은 당시 범위이며 아래 최신 독립 검사 상태와 구별한다.

- HudChatGeometry, HudNsInfo, HudNsInfoCreate, TextEntryBuffer, TextEntryGump, ChatWindow의 소스/검사/원문 fixture/생성기/문서. 이전 독립 Release13CPP/링크 경고·오류0,6필터8검사/원문7482행 통과는 `extracted/cpp-hud-components-20261023-isolated/verification-final.json`과 final-results.json에 있다. 공식 전체 빌드/실제 Client 연결 결과로 세지 않는다.
- TextEntryGump2292행은 실제46ee50 문자열 getter alias를 바로잡은 원문으로 instruction count72행만 달라졌다. Chat180행은18 trace payload alias와 instruction count가 바뀌었고 최종 상태/반환은 같았다. fixture를 제품에 맞춰 변경한 것이 아니다. 개별 초안의 예전 해시/명령 수는 등록 전에 갱신해야 한다.
- NSInfo 계층은 root 직계 background1 + 아래13필수자식=14필수노드다. root 직계14개로 잘못 연결하지 않는다. Player8×4+Battle3을 포함한 최소50노드다. 실제 Chat 등록/clear/quit 파일 append/bottomIndex 등 Client 전역 연결은 남았다.
- MinimapBlit의 생성기/120행 fixture/검사/문서도 미등록이다. crop(-2,1)은 flat 유효 읽기,(-2,0)은 heap guard 밖 미정 경계다. 임의0 또는 clamp로 정상 성공을 만들지 않는다. 전체 Gump 배경/테두리는128 bitmap만으로 대체하지 않는다. 제안 패치는 기존 extracted 인계 자료를 확인한다.

전체 목표의 잔여 캠페인/단위 행동·Edit/custom map·일반 HTML/HUD 결합 등 세부 이력은 LEFT_JOBS.md의 이전 단위 기록을 계속 사용한다. 다음 작업은 위 미션 로딩 실패 수정부터다.
