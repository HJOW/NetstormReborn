# 생산 단축키의 실제 큐와 Gump 연결 — 2026-10-23

현재/마지막 전체 정밀 디컴파일 PC는 **VM-W11-Codex / 2026-10-23**이다. AGENTS.md·공통/닷넷 인수인계를 다시 읽고 `0e66d91b / 20261023-33 CP`의 완료 상태에서 이어갔다. 이전 목표 턴은 객체·청크 저장과 전체2369/0을 남긴 진행이다. 기존 정밀 원문과 보호 PE를 사용했고 새 디컴파일은 하지 않았다. 원본 실행 파일 SHA256은 `a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236`이다. 이 호스트는 AGENTS.md의 게임/별도 창 실행 확인 예외다. 실제 GUI는 새 클론만 실행했으며 보호 원본·CD·AGENTS.md를 변경하지 않았다.

## 실제 제품 연결

기존 UserInput은 QWASZX 문자 분기가 없었다. 하위 생산 root의020000f4 처리는 있었으나 UberGump가 비마우스 사건을 UserInput으로 보낸 뒤 반환하여 해당 명령도 도달하지 못했다. 두 경계를 연결했다. `ProductionShortcuts`는 Q/W/A/S/Z/X→slot0/1/2/3/4/5, Y→4, E→0..5 순서의 실제 예약을 수행한다. 대소문자가 모두 같고 별도 Ctrl/Shift/Alt 거부를 추가하지 않는다. 코드는03ffffff 마스크로 비교한다. 여섯 슬롯보다 현재 공급이 적어도 키층에서 제한하지 않으며 없는 슬롯은 실제 하위 검색이 무동작으로 처리한다.

예약은 현재00542ac4 명령 DWORD를 읽어452fe0(command,slot,0)에 넘기는 원본 경계다. E도 각 예약 직전 getter를 다시 호출한다. 제품의 기본 명령은 보호 PE와 같은 `kProductionBridgeCommand=020000f4`이며 생산 root도 같은 상수를 사용한다. 기존 정밀 원문의 해당 전역 참조는 읽기/비교이며 쓰기는 이 검색 범위에서 찾지 못했다. 실제 InputQueue의 FIFO·코드 마스킹·39개 포화 정책을 그대로 사용한다. 키가 실제 커서를 직접 취득하거나 별도 조각을 생성하지 않는다.

UserInput은 기존 활성 커서 HandleInput 뒤에 일반 키 예약을 호출한다. 원본4d96b6의 Paused 결과가 비영이면4d9bbb로 점프하여 일반 단축키만 건너뛴다. 그 뒤445330과 활성 커서의 좌클릭Drop/우클릭·좌우 회전/ESC가 이어지므로 정지를 이유로 이 커서 경로까지 막지 않는다. helper는 일반 예약의 release/Paused만 거부한다. 기존 일반 Home/사제 등 다른 키 전체의 정지 동작을 이번 모듈로 검증한 것으로 확대하지 않는다.

UberGump는 Guide/Tutorial 입력과 현재 Help 모달 처리를 먼저 유지하고, 이미 큐에 들어온020000f4를 같은 RawButtonControls::Event→GumpInput::Route→공유 Gump 트리에 전달한다. 월드 UserInput의 조기 반환에 도달하기 전에 처리한다. 이 명령 소비에 새 Paused/활성 커서 조건을 넣지 않는다. 기존 RawProductionControls의 실제43ec00 대응 분기는 현재 기술 부모에서 flags40/slot BYTE+23이 처음 일치하는 Form을 찾아 `Cursor.Take(sid,0)`와 getPiece.wav를 실행한다. 비용·재고·추가 정지 조건을 새로 넣지 않았다.

자동 GUI 입력의 `key`/`keyup`에는 한 글자 ASCII를 실제 WM_CHAR 이후처럼 하위 문자 코드로 넣는 경로를 추가했다. 기존 left/up/right/down/home/priest 가상키 별칭은 유지한다. 문자 Q를 가상 Q<<16으로 잘못 넣지 않는다. 이 기능은 검사 스크립트의 입력만 확장하며 게임 상태/조각 수를 직접 쓰지 않는다.

## 독립 원문 관측과 대조 범위

`tools/decomp_production_shortcuts.py`는 제품 C++를 읽지 않고 보호 PE를 Unicorn으로 실행한다. 현재 사건의004d6e61부터 종료 라벨004da477 직전까지다. 실제 Guide463e40, signed 정지460de0, 커서445330, 큐452fe0, fallback CRT strchr도 원문으로 실행한다. 명시 외부 경계는 Guide 하위 가상 입력, Gump 소비 반환, 일반413d50이다. 전체 함수 prologue·OS 폴링·큐 Pop·다음 사건 반복·최종 RET를 실행한 것으로 설명하지 않는다.

입력4020개·x87 제어어027f/037f의 **8040관측**, 실제2382402명령, 정밀도 차이0이다. 모든 현재 사건이 종료 라벨에 도달하고 사건별 스택 변화0을 확인했다. 명령/슬롯은 실제 점프표와 push, 실제 큐 쓰기의 결과이며 표준 매핑을 기대값 생성기로 사용하지 않았다. 대소문자와 modifier, Guide/Gump의 비영 소비, release, signed 정지 깊이0/1/-1, 콘솔, 명령 DWORD, 빈/감긴/가득 찬 고리, 주변 비대상 문자와 연속 사건을 교차했다.

고정17열의 앞9개만 장치 입력이다. `name/events/guideResult/gumpResult/pauseDepth/console/command/read/filled` 뒤에 `completed/readout/writeout/queueout/physicalqueue/trace/stackdelta/nativeinstructions`가 온다. 실제40칸480BYTE도 관측했다. 해시와 범위는 `extracted/cpp-production-shortcuts-20261023-native.json`에 있다.

새 helper와 **직접 제품 대조하는 것은4280행**이다. GuideResult=0, GumpResult=0, console=0이고 모든 입력 사건의 release가0인 행으로, 실제 paused 경계 수가 사건 수와 같은 집합임을 독립 확인했다. 나머지3760행은 앞선 native 관문 증거이며 이 helper의 제품 대조로 계산하지 않는다. `ProductionShortcuts_Native4280ReachableCases`는 실제 예약 인자·명령 getter 횟수·같은 InputQueue 공개 API의 활성FIFO와 포화를 비교한다. 비공개 물리 슬롯이나 read/write 포인터의 제품 동일성까지 주장하지 않는다.

별도 `ReleasePauseAndLazyDevices`는 뗌/정지/비대상 문자에 장치를 요구하지 않는지, E의 현재 명령 재조회와 세 번째 큐 경계에서의 부분 중단을 확인한다. 원문의 이미 꺼낸 사건에서 outside 비트가 마스킹된다는 관측은 앞선 창 활성/폴링의 검증으로 확대하지 않는다. 기존 UserInput의 outside 거부 위치는 유지한다.

## 실제 클론 GUI

`tools/cpp_production_shortcuts_smoke.py`는 원본 설정을 허용된 두 이름만 보관/복구하고 전체 원본 파일 SHA를 전후 비교한다. 실제 thewarbegins 브리핑→Play Mission→자연 공급→문자 키→같은 큐/프레임을 실행하며 조각 순서·수량·게임 시계를 주입하지 않는다. 실행/PID·실행 파일 SHA·입력 계획·원시 로그·모든 상태와 BMP를 새 경로에 남긴다. `--no-audio`이므로 소리 장치/청취는 별도다. 실제 WM_CHAR 이후 입력을 넣었으며 OS 키 메시지 발생 자체를 검증한 것은 아니다.

GUI1은 취득/회전/배치/보충까지 실행됐지만 적 AI의 자연 SP 수입400까지 불변으로 가정한 마지막 검사 때문에 실패했다. 게임 코드는 바꾸지 않고 로컬 플레이어의 다리 비용만 확인하도록 검사했다. GUI2는 실제 브리핑 문자 무시, 초기 slot0만 공급된 때 X의 없는 slot5 무동작, ESC 팝업의 정지 중 Q 무시, 현재 여섯 Form의 q/W/a/S/z/X/Y 대체와 E 전체 예약, 뗌 무시, 활성 커서 교체를 통과했다.

GUI2의 실제 여섯 원본 내용물 SID는 slot0..5=73/88/91/92/93/94다. 각 키의 raw_cursor.preview가 해당 현재 SID와 일치했고 E의 최종 값은 slot5 SID94였다. 모든 취득의 다리 costFlags는512로, 실제 Take flags0에 다리 bit200을 더한 경로다. 회전 방향은0→2→0이고 원래 방향으로 복귀한 첫 조각을 실제 합법 위치에 놓았다. 세 배치 관문은111이며 새 서버 다리 SID15202가 생성됐다. 선택 Form73은 현재 목록에서 사라졌고 새 Form162가 보충되어 여섯 슬롯의 품질2가 회복됐다. 로컬 SP3000은 유지됐고 원본 전체 파일 이름/내용은 동일했다. 첫 조각은 한 칸 모양이므로 이 관측만으로 모든 복잡한 조각의 회전 외관/배치 동일성을 증명하지 않는다.

`extracted/cpp-production-shortcuts-20261023-gui2`의 mature/rotated/placed/refilled BMP를 보존했고 rotated/placed1024×768을 직접 확인했다. 원본 자산의 좌측 여섯 조각·SP·커서 표시와 생성된 다리를 확인했다. 좌측 하단 미니맵과 원본 상단 메뉴는 아직 남아 있으며 이 화면을 전체 UI 동일성으로 계산하지 않는다.

## 검증과 다음 작업

Release 빌드1은 종료0·경고/오류0이고 새2검사는 실패check0이다. GUI2는 클론 종료0·판정 통과·원본 불변이다. 이 검증 이후 제품/검사 C++를 동결했다. 최종 전체 CTest1은 **2371검사·실패check0·종료0/176.58초**이고 LastTest.log 내부2371/0도 확인했다. 로그는 `extracted/cpp-production-shortcuts-20261023-*`다. UTF-8/한국어 주석/원본·도구·TSV SHA/diff/보호 경로를 확인하고 인수인계를 마감해 **20261023-34 CP**로 로컬 커밋한다. 다른 dotnet 세션 파일·원격 push는 없으며 검증 뒤 제품/검사 C++를 변경하지 않았다.

다음 UI 우선 작업은 미니맵의 실제 생성4801b0/표시480810/Draw480660/이동4803f0/드래그Idle480440/셀그림4808d0/프레임Refresh481320을 기존 dirty 큐와 동일 HUD/카메라에 연결하는 것이다. ESC의 Game·조건부Edit·View·Options·조건부Players·About 상단 막대, 덱의 원본 드래그/지도 영역 배치도 남는다. 이후 Temp 동적 노드/rename·version 공유·Terr/Chaff 부모·Money/Mission/Badges/Notify, generic Editor·새 맵/시나리오를 계속 진행한다. 전체 Early Missions·캠페인·데모3 사용자 완주와 원본 동일성, Multiplayer 외 메인 메뉴·GUI·음향·배포 목표를 그대로 유지한다.
