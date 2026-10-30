# Home 계열의 전체 즉시 카메라 적용 — 단위633

현재/마지막 전체 정밀 디컴파일 PC는 HJOW-Athlon/2026-10-23이다. AGENTS.md와 공통/DN 인계를 다시 읽고632 72dda3ce의 깨끗한 작업본에서 진행했다. 전체 목표는 active다.

## 원문 부모와 실제 장치

433aa0은 공유0059a8b0 증가→497f20 목표/005c85dc 증가→실제497960을 차례로 호출한다.433ba0은 world float 좌표에서 전체 footX/footY를 뺀 뒤 이 부모를 호출하고 현재TYPE/Carrier를 다시 조회한다. 초기4340d0의 실제 깊이 증가/감소와 마지막 후보 선택도 같은 부모를 사용한다. 미니맵/사건의497f20 직접 요청과 달리 flip이나 Present의 가장자리 제한을 추가하지 않는다.

ClientCameraGlobals::CenterImmediately로 부모를 연결했다.CameraHome은 좌표를 먼저 픽셀로 바꾸거나 별도 카운터를 증가시키지 않고 저장된 world float만 부모에 전달한다.Session은 같은 Client worldRedraw/LocalStateGlobals/Camera owner를 사용한 뒤 명시497960 수신자를 호출한다.GameWorld의 Home/사제보기/명시SID/초기Reset은 CommitRequestedCamera→실제Client 장치로 이어지며, 실제 적용 결과가 바뀐 뒤에만 표면/입자/위치 음향을 공개한다.

UberGump의497960 장치 연결을 CommitCameraNow로 분리했다. 화면 출력 후 ApplyCameraFrame은 기존 flip/목표 제한 뒤 같은 본체를 호출한다. 즉시 Home은 이 제한/flip을 거치지 않는다. 커서 Lock/Unlock·Screen.Scroll·dirty 표/두setter·Gump RedrawRoots를 생략하지 않으며 실제 서비스가 없으면 두setter/noop로 성공을 만들지 않는다.

초기 Reset이 현재 도킹 영역을 읽도록 Client 서비스의 같은 MapView getter를 World 생성 시점부터 빌린다. 독립 검사도 명시 전체 viewport/OS 경계와 실제497960을 사용한다. State의 독립 카운터 입력과 실제Session의 장기 Client 카운터를 구별한다. 이전 비Session 어댑터와 Scroll/Resize의 ApplyCameraPixels는 이번 부모로 일괄 변경하지 않았다.

## 독립 원문·C++ 검증

실제433aa0/497f20/4e49c0/497960/497490/4834c0을 실행했다. 정상·음수 목표/NaN/무한, DIB2/18, 두 배율, callback 뒤 현재 목표/TYPE/표/viewport 변경, 카운터 감김의168개 입력을 두 별도 프로세스/x87 PC027f/037f에서 관측해 최종672개가 같다. 전체 Renderer/snapshot16DWORD·target6·dirty5·현재전역11·실제 효과 순서와 commit 진입 때 목표/두 공유 카운터를 보존했다. 커서/Gump/Screen/dirty 추가/마우스/진단만 명시 외부 경계이며 게임/OS/MP 실행은 없다.

초기v1은 도구 정리 전, v2/v3은 현재전역11개까지 두 정밀도 결과에 함께 넣기 전 관측이다. 최종은v4/v5이며 다섯 TSV 바이트는 같다. 준비 관측을 최종672개에 합산하지 않는다.10.72의 부모/store 순서는 별도 분석 전 전체 완료로 세지 않는다. 기존 두 판본 초기 시야 fixture는 같은 명시 원문 부모 장치를 통해 경계 관측만 계속 대조했다.

집중70/실패0·등록3551이며 build-v1/v2는 경고/오류0이다. 새 C++3개는168행의 전체 결합, 장치 누락의 원문 선행 쓰기/명시 거부, 실제World의 같은 세 owner/감김/이전뷰→실제commit→한 번의 음향 공개·동일 위치·dead 첫 Home 후보를 확인한다. 초기 시야/현재TYPE·Carrier preview/깊이·카메라 Present/Commit/요청/Pan·기존미니맵·화면·같은World 표시/수명 회귀를 포함한다. 테스트 helper는 제품의 기본 서비스가 아니라 명시 비GUI 장치다.

## 실제 표시 시점과 Edit 입력 교정

원본에 없는 읽기 전용 report-presented를 추가했다. 기존 Present 뒤·프레임말미 카메라 적용 앞에서 실제 현재 상태만 보고한다. 일반 실행/게임 순서는 유지한다. 최종 같은 한글 배포본의 세 화면비(1024×768 창,1280×720 전체화면,1280×800 창)에서 실제 TEST02 Go!·미니맵·F4/F5·재시작·이탈을 확인했다. 표시 시점 Home은 현재view/목표가 같고, 같은 표시 시점 미니맵은 목표와 현재view가 다르다가 프레임말미에 적용된다. 재시작 뒤 새Session/HUD와 초기메뉴 재생성/반납도 확인했다.

구형 Edit 도구의 최초1회는 TEST02를 찾지 못해 설치 전에 종료했다. 추가 첫 probe는 시작 안내의 OK를 누르지 않아 UserNotUpdated에서 종료했다. 정상 OK/Main Menu를 거친 별도 probe-v2는 실제 기본 목록이 원문 상한50개(A..FWOS5)이고 TEST02가 없다는 것과 Cancel 복귀를 확인했다. 처음 안내 창 문제로 설명한 뒤 목록 관측으로 직접 원인을 구별했다. 전체카탈로그에 원문mayEditMissions=0을 적용한 다음 실행은180초 상한에서 끝나지 않았고 원인은 미확정이다. 별도probe-v3의 외부 절대경로는 기존 이관 정책에서 거부됐다. 이 거부 규칙을 유지하고 자료루트 내부 상대FileSpec에 두 원문Fort만 복사한probe-v4에서 목록2개/실제Editor설치/종료0을 확인했다. 기존 도구도 정상 Main Menu와 같은 두 원문 작업맵의 상대fortSpec/SOLOSpec·원문커스텀필터를 명시했다. 기본 목록/분류/상한이나 게임 상태를 바꾸지 않고 같은 보호 TEST02의 실제 Edit→TestBattle 두 화면비/비정지 시각/현재Session·모드/커서를 확인했다. 이두맵성공을전체카탈로그필터문제해결로세지않는다. 목록 무제한/스크롤은 AGENTS.md의5차 작업으로 남는다.

최종 주요5창/37표시 직후 버퍼↔실제 창DC 전체RGB가 같다. 별도6회(초기Edit입력실패·첫probe안내미처리실패·기본목록/Cancel성공·전체필터180초상한·외부절대경로거부·상대2맵Editor설치성공)는 주요5개와 합산하지 않는다. 초기 성공Home3을 같은 EXE/소스/ZIP 그대로 보존하고 루트내상대작업namespace의 새Edit2를 합쳤다. 게임 상태/카메라를 직접 쓰지 않았고 원본 파일은 수정하지 않았다.

## 배포·현재 기준

게임 SHA-256 `d68fa89be7130a74804c8f2616946d4327d6b51cd79a14308095cad494c9bfe1`, ZIP `402206d4eb1749fcf19b5ab394771ddea8a843e3132159a712e41454a2cbdbd3`다. 배포는 `extracted/cpp-camera-immediate-633-20261031/NetstormReborn-cpppj-633.zip`이며1376압축항목/1358자산의 실행 뒤 사본·압축·현재빌드/보호 원천 바이트가 같다. 기존 보호2628개는 불변이고 새fixture를 포함한 현재 보호2629/fixture770을 다음 기준으로 쓴다.

[완결 집계](../../cpppj/camera-immediate-runtime-20261031.json), 원시 `extracted/cpp-camera-immediate-633-20261031/`를 따른다. 모든 빌드/검사/게임 프로세스가 종료했다. 마지막 전체34는 같은632제품의34/34로 유지하며 매 단위34/전체CTest를 반복하지 않는다.

## 다음634와 남은 범위

다음634는 우선 전체카탈로그의커스텀필터 경로180초상한/원인미확정을 원문 분류·이관·파싱/반복 범위와 대조한다. 좁은2맵시험으로전체Edit문제를대체하지않는다. 실제 입력 pan/가장자리는 이미 목표-only를 사용한다. 남은 Scroll/Resize/이전비Session 어댑터 및 SetParticleView의 선택적 setter 경로를 원문 호출 위치/현재viewport와 분류하고, 실제Client에 남은 부분 연결이 있는지 확인한다. 먼저 본체/호출자 근거를 확보하며 이번 Home 부모로 일괄 치환하지 않는다.

내용물/격자/grid/debug·전체UI/Edit/시나리오·파일/수명/외관/음향·Windows호환성·MP/Outpust/Challenge의 무시험 사전 구현·4~7차·전체 원본 동일성은 남는다. 기존 VFX 범위와612 스택미정값 한계를 유지한다. 자연완주/포획/희생/특정 적 공격/승리·공중3종 전용·신규MP/Windows11/PCM/물리키/DPI/포커스 검사는 하지 않았다.
