# 가장자리 입력과 실제 카메라 프레임 연결 — 단위500

현재 PC와 마지막 전체 정밀 디컴파일 PC는 HJOW-Athlon이며, 마지막 전체 수행일은2026-10-23이다. 정밀 산출물은 변경하지 않았다. [496 입력](cpp-edge-scroll-core-20261027.md), [497 공유 진단](cpp-patch-diagnostic-owner-20261027.md), [498 Present 제한](cpp-camera-present-target-20261027.md), [499 현재 카메라 적용](cpp-camera-commit-core-20261027.md)을 실제 같은 Client에 연결했다. [원문 DIB 관측](../../cpppj/screen-scroll-native-20261027.json)과 [통합 보고서](../../cpppj/edge-scroll-runtime-integration-20261027.json)를 함께 본다.

## 실제 호출과 소유권

`UberGump::Input`은 Guide의 폴링 사건 통지 직후, 기존 Overlay/일반 입력 큐 처리 전에 `ProcessEdgeScroll`을 호출한다. 같은 pointerBoundary flags, 장기 edgeScrollSince, 현재 화면 테두리/폭/높이, 실제 OS 폴링, 현재 모달/메뉴 serial/직계 popup, 현재 wall 시각을 공급한다. 원문 초기 edgeScroll1·speed20을 유지하며 설정이 없으면 현재값을 보존한다. 요청은 현재 Renderer view 기준 목표만 쓰고 같은 LocalStateGlobals.MetaRebuild를 증가시킨다.

`Client::Frame`은 실제 화면 출력 뒤, 마지막 snapshot 복사 앞에서 `PresentCameraTarget`→`CommitCameraTarget`을 실행한다. 독립 검사 사본 대신 같은 Renderer dirty vector와 Renderer+5c, Client의 Renderer view/snapshot/target/005318ec, 현재 World viewport와 선택 마우스, 실제 Cursor Lock/Unlock·Gump RedrawRoots·Screen Scroll·소프트웨어 커서 dirty를 연결했다. 적용 뒤 표면/입자/위치 음향에 새 원점을 알리고 다음 장면을 예약한다. Renderer+5c와 worldRedraw는 다음 실제 Draw/Present에서 소비된다. 진단을6으로 강제하는 쓰기는 추가하지 않았다.

00531928 세계 크기는 PE 초기256의 Client 장기 signed 전역이며 kernel 초기화 뒤 원문의 명시 store를 수행한다. Native Renderer 초기화와 현재 Session의 CursorManaGlobals.mapLimit가 같은 주소를 빌린다. Session 반납으로 주소/값을 재생성하지 않는다.

ESC 메뉴를 연결하면서 기존 SP checksum 주소 오류도 수정했다. `GameMenuTrackingOffset()`은 실제 SpStore가 사용하는 현재 추적 주소를 반환한다. Client 공유 SP를 사용하는데 Session의 별도 patch.sequence에 checksum+3을 쓰던 경로를 제거했다. 독립 Session의 기존 추적 계약은 유지한다. 별도 검사에서 세계 크기의 두 실제 Session 주소 공유와 SP의 세 번 메뉴 준비를 확인했다.

## 화면 장치와 호환성 경계

`Screen::Scroll`은 같은 현재8비트 DIB에서 겹친 행을 원문 방향으로 옮긴다. 양수 카메라 이동은 원본 오른쪽/아래 픽셀을 왼쪽/위로 복사하고, 아래로 옮길 때는 뒤쪽 행부터 처리한다. 실제 pitch와 padding을 유지한다. `tools/decomp_screen_scroll.py`로 보호4a1ad0의 DIB 분기를357입력 실행해357정상 반환의 전체144BYTE를 관측했다. Lock/Unlock과 CRT memmove 바이트 복사는 명시 경계이며 기존 원문/최초 gold와 커밋 fixture의 해시가 같다. C++은 이357행 및 별도 좌표 계산357조합을 대조한다.

Windows10/11 호환 출력은495의 borderless/DIB 장치를 계속 사용한다. 원본 독점 DirectDraw flip은 요청하지 않고 현재 bit4의 자연 반환을 사용하며, 비DIB 시차 장치는 선택하지 않는다. 실제 창 출력은 다음 Present에서 완성된 프레임을 내보낸다. 다른 앱이 활성일 때 바깥 포인터가 배경 카메라를 움직이지 않도록 edge enabled를 차단하는 호환성 처리를 추가했다. 원본과 다른 이 이유는 코드 주석에도 남겼다. 이번 실제 OS 검증은Windows10 19045/HJOW-Athlon이며 Windows11/다른 모니터·DPI 실기를 대신하지 않는다.

NoWorld에서 커서 깊이가 비영인 원문 단락은 같은 깊이 증가/감소로 연결했다. World가 없는데 깊이0의 활성 preview를 재생성하는 경계는 미복원 예외를 유지한다. 일반 메인 메뉴/전환/재시작 검사에서 이 예외가 발생하지 않았다. 선택되지 않은 비DIB DirectDraw 경계도 명시 예외로 유지하며 복원 완료로 합산하지 않는다.

## 실제 실행과 최초 실패 보존

최종 게임 SHA256은 `edbdec749c9cd0923355f5b531769676b8114ce022e58e501976ca941a75cf41`이다. 같은 파일의 짧은 실제 실행8건을 확인했다. 입력 도구는 자기 PID의 실제 표시 창에 OS 포인터/왼쪽 버튼/Shift와 Win32 클릭/ESC를 전달하며, 게임 폴링 공급자를 덮어쓰지 않는다. UI 스크립트는 관측/캡처만 사용한다. 미션은 실제 Play Mission을 클릭해 page=mission과 World/Kernel/프레임 진행을 요구한다.

| 실제 사례 | 확인 결과 |
| --- | --- |
| 1024×768 전체화면 | 우측178→300픽셀 구간 이동, 중앙 정지, 왼쪽 버튼 유지 중 정지, ESC 메뉴 열린 동안 정지, 닫은 뒤 위쪽 이동, Shift 세로만/해제 뒤 대각선 이동 |
| edgeScroll0 | 실제 우측 포인터에서도 원점/요청 개수 유지 |
| 창 모드 | 모드138에서 실제 우측 포인터의 가장자리 이동 없음 |
| 1366×768 | 모드139·pitch1368에서 우측 이동 |
| 1280×800 | 모드139에서 우측 이동 |
| 소프트웨어 커서 | 모드235에서 우측 이동·실제 출력 캡처 |
| 메인 Options | 창→전체화면→800×600→창→전체화면→Quit 정상 |
| 새 프로세스 재실행 | 저장된800×600·모드139로 main 정상 진입/종료 |

모든 성공 실행은 종료0이며 stage clear를 시도하지 않았다. 최신 도구로 수행한 창/wide/ten/soft 네 실행에서는 종료 뒤 ClipCursor가 전체 데스크톱[0,0,1920,1080]으로 해제된 것도 기록했다. full의 실제 출력은 좌측 덱·하단 미니맵과 이동한 월드가 표시되며 양쪽 여백이 검정인 것을 확인했다. 이는 짧은 입력/표시 검증이며 전체 원본 외관·34스테이지 자연 완주 판정은 아니다.

최초 Source2 full은 게임 종료0이지만 입력 도구가 보고서 재쓰기 중 mtime을 조기 소비해25개 중8개 시점만 처리했다. 또 일반 보고서 분리기가 raw_camera_frame 안의 frame까지 잘랐다. 이를 성공으로 세지 않고 최초 로그/전체 TSV를 보존했다. 실제 frame 줄에만 고정한 분리, 부분 쓰기 재읽기, 모든 계획 시점 필수 확인으로 도구를 수정했다. 게임 상태를 바꾸지 않는 camera: 간결 관측을 추가해 큰 월드 덤프 재쓰기도 줄였다. 최종 Source3 full은25시점/필요11입력을 모두 관측했다.

Source3 첫 window도 게임 종료0이지만 도구의 STARTUPINFO가 창을 숨겨 입력을 보내지 못했다. 최초 실패를 유지하고 실제 창을 표시하도록 도구만 수정한 뒤 새 디렉터리에서 재실행했다. 이를 게임 결함이나 기능 통과로 합산하지 않았다. 검증 중 제품/검사 소스와 실행 파일은 변경하지 않았다.

## 검증과 남은 작업

Release build1~3은 모두 경고/오류0이다. 신규6검사를 포함한 집중44고유/88관측·실패0, 기존3,323등록 유지·현재3,329등록이다. 전체 CTest 결과는 아래 최종 기록을 본다. 보호3,217파일 중3,215개와 AGENTS 전체가 바이트 동일하며, 기존 CampaignSessionTests761,649BYTE·tests/CMake166,147BYTE·src/CMake16,645BYTE 접두부를 유지했다. 최종 빌드 전후 소스/검사2,888파일과 실행 파일 해시도 같다. 원시 자료/실패/실제 캡처는 `extracted/cpp-edge-runtime-500/`에 보존한다.

전체 C++3,329개/실패0, CTest1개/실패0(347.33초)을 확인하고 최초 LastTest.log를 별도 보존했다. 커밋 전 fetch/pull은 종료0·추가 변경 없음이며 원격 push는 하지 않는다.

가장자리 입력/Present 카메라의 실제 연결은 끝냈다. 다음에는 미니맵/카메라·덱/QWASZX/ESC의 남은 실제 연결과 와이드 해상도 선택 메뉴·DD 조정 UI, 편집/시나리오 잔여를 점검한다. Graph 재로드 차이의 원본 전체 대조와34스테이지 동작 동일성은 계속 남는다. 스테이지 완주/사용자 개입 검증과 Test Battle english 부재 개선은 계속 미룬다. 멀티플레이어는 사용자 결정대로 이 목표 뒤 디컴파일만 참고해 테스트 없이 사전 구현한다.
