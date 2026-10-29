# dotnetpj R3h-3b-2 — 다중 줄 Widget 생성·입력·수명

2026-10-29, HJOW-Athlon. 기준은 `5765d120253d`의 cpppj `ScrollingTextGumpWidgets.h/.cpp`다. 앞 단위에서 실제 Slider를 연결하고 로컬 커밋한 뒤 같은 작업 턴에서 이어 진행했다. 기존 StyleTextBufferStore/ScrollingTextGump/ScrollGumpWidgets/기본 Gump가 같은 실제 저장소를 사용한다.

## 1. 구현

생성은 기본 등록→실제128칸 소유 버퍼→전체 문맥/크기/부분 필드→overlay→숨긴9×100 Slider와 자식 생성→우측 mode1 부착→맨 아래 요청→기본 태그 플래그 순서다. 미작성 Unwritten/DragY와 다른 textFlags 비트는 보존한다. 원본 new 실패 뒤 부분 필드/버퍼/overlay도 임의로 되돌리지 않는다. 원본 생성 우변 콜백이 끝난 뒤 현재 노드를 조회해 버퍼/Slider 토큰을 쓰도록 분리했다.

부착은 실제 기본 트리 뒤 현재 Slider Push/높이/표시·우측 재부착, 기존 scroll 요청과 폭 dirty를 처리한다. 이전 폭 NaN은 패치에서 변경으로 저장하고 CD에서는 무순서 비교로 생략한다. SetSelectable의 전체 비0와 SetSlider/SetScrollable의 최하위 비트 차이, SetWrap의 noWrap 단독 저장·dirty/Redraw 누락을 보존했다.

실제 Slider change는 현재 serial을 검증하고 step은5/-5/실제 페이지 높이의 상대 요청을 소비한다. 상대 요청은 대기 요청 대신 현재 적용 scroll을 사용한다. refresh 뒤에도 마우스/커서 분류를 계속한다. 왼쪽 태그 비0은 부모 태그 통지, 태그0은 캡처/프레임 드래그이며 오른쪽은 조각이 있으면 태그0도 통지한다. 커서는 태그16→스크롤 가능15→일반1 순서다. 방송은 scroll 허용 비트와 전달 y delta만 사용하며 폴링을 추가하지 않는다.

버퍼 교체/소멸은 현재 소유 버퍼의 실제 Destroy/free 뒤 현재 포인터0을 쓰고 원본 소유 DWORD를 그대로 저장한다. 다른 dirty/스크롤/태그는 바꾸지 않는다. 같은 소유 토큰 재설치도 소멸 뒤 같은 포인터를 남기는 원본 계약을 유지하며 후속 잘못된 조회는 명시 오류로 거부한다. 빌린 버퍼의 포인터/상태는 기본 scalar 소멸에서도 남는다. 실제 자식 우선 수명은 Slider/화살표/배경/손잡이→소유 버퍼→본문 노드 순서다.

현재 클래스 분류를 실제 가상 Attach/부모·재귀 사건/직접 캡처/프레임/scalar 분배에 연결했다. 검사 장면은 기존 공통 Slider 장면의 같은 가상 분배를 확장하며 별도 근사 노드를 만들지 않는다.

## 2. 검증 상태와 근거

- 신규17개/기존 Slider21개, 집중 **38개 통과**다. 실제 두 판본 생성/부분 필드/128칸/overlay/Slider, 표시 글리프 픽셀/실제 Slider의 range56·page40/부모5픽셀 이동, step/page/serial, 태그·커서·직접 캡처·드래그, 설정 비트/줄바꿈, 버퍼 소유/교체/free 콜백·부분 실패·반납 순서와 판본별 NaN 폭 분기를 검사했다.
- 최초6실패는 검사 도우미가 마지막NUL 없는 BYTE 범위를 음수 길이로 추가한 입력 오류였다. 실제 API의 명시 길이를 전달하도록 도우미만 고쳐36개를 통과했다. 추가 NaN 판본 검사 중 CD1건과 첫 전체 검사는 이미 붙은 노드를 Push 없이 재부착한 검사 전제 오류로 실패했다. cpppj GumpTree.cpp의 CD void 계약을 확인하고 검사에 Push를 보완한 뒤 최종38개/전체가 통과했다. 제품 코드/기대값을 바꾸지 않았으며 최초·추가 실패 로그와 첫 전체 실패를 보존했다.
- 독립 원본 실행 fixture를 추가하지 않았고 **L1로 세지 않는다.** 기존 fixture99/reflected-commit은 유지하고 포트 맵만200행(L1 97·port103)으로 갱신했다. 이전 코드/검사 은퇴0, 실제 게임/메뉴 캡처 비교0이다.
- 최종 전체 **1,783개 통과(Assets363·Core1030·Client390), 실패/건너뜀0**, Release 전체 빌드 경고/오류0이다. 변경8파일의 UTF-8·한국어 주석/문서와 보호 경로 변경0을 확인했다. Git 제외 로그는 `extracted/dotnet-scrolltext-widget-compile-20261029.log`, `dotnet-scrolltext-widget-focused-initial-20261029.log`, `dotnet-scrolltext-widget-focused-cd-precondition-20261029.log`, `dotnet-scrolltext-widget-focused-20261029.log`, `dotnet-scrolltext-widget-tests-20261029.log`(첫 실패), `dotnet-scrolltext-widget-tests-final-20261029.log`, `dotnet-scrolltext-widget-build-20261029.log`다.

## 3. 다음

다음은 R3h-4 MenuGump 레이아웃/컨트롤/위젯/입력/Draw와 BindMenuLayoutStyleText를 실제 항목 소유 필드에 연결하는 일이다. 이후 실제 메뉴/전체 GameAssets/ConfigInterface/음향/GIF/호스트 조립과 R4 이후 월드/세션/HUD·전체 미션·Edit를 잇는다. 전체 목표는 계속 진행 중이다.

이번 검사는 명시 Draw 대상의 헤드리스 조립이며 실제 Renderer 정렬/분배나 메뉴 캡처 동일성을 대신하지 않는다. cpppj·원본/CD·AGENTS.md·공통 인계는 수정하지 않았고 게임/별도 창·금지된 실제 재현·멀티플레이 검사를 하지 않았다. 외부 TODO.md는 커밋에서 제외한다.
