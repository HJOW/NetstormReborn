# dotnetpj R3h-3b-1 — 실제 Slider·돌 화살표·질감 손잡이

2026-10-29, HJOW-Athlon. 기준 cpppj는 `075b0c1424db`의 `ScrollGump.h/.cpp`·`ScrollGumpWidgets.h/.cpp`다. 같은 기본 Gump·실제 트리/사건/캡처/프레임·돌 버튼/배경/픽셀·수명에 연결했다. 실제 메뉴/다중 줄 Widget 전체 생성·입력은 다음 단위다.

## 1. 구현

`ScrollGump`은 min/max/page/position의 signed 비교·DWORD 감김·두 번 위치 적용을 유지한다. 음수 페이지에 임의 하한을 추가하지 않고 void 여부에 따라 가상 Push→현재 x/y Attach 또는 Redraw(false)를 호출한다. 실제 기본 부착 뒤 현재 세 자식을 Push하고 원본 float32 저장 전 배율과 판본별 짧은 축 저장을 사용해 크기/위치를 계산한다. 손잡이 위치 분자는 position-minimum 대신 원본 position이다.

화살표 click/repeat는 현재 serial로 부모 이동 요청을 보내고 미소비 때만 현재 step/position으로 기본 이동한다. 실제 값이 달라진 경우만 change를 통지하며 Handle은 항상0으로 반환한다. 손잡이 step의 안쪽 Bubble 뒤 바깥 Bubble도 원래 serial로 상위 전달을 계속한다. 양끝 포함 손잡이 선택·바깥 page4/3·캡처, 패치 가로 누름/CD 가로 무동작, 세로 release만 프레임 플래그 해제, 방송의 왼쪽 버튼 폴링과 전달 y delta를 보존했다. 세로 드래그의 range*(delta+top)은 DWORD 곱 뒤 원본 CRT 절삭을 한다. 가로 방송은 이동하지 않아도 y delta가 비0이면 원본 change 통지를 보낸다.

`ScrollGumpWidgets`는 원본 new 크기·실제 돌 버튼/질감 배경 생성·부분 초기화·미작성 필드 보존·종류 BYTE 확장을 옮겼다. 기본0축만40으로 바꾸고 감소/증가/손잡이 생성·부착 뒤 실제 range0..100/page10/position50을 적용한다. C++ 대입의 우변 생성 콜백 뒤 현재 노드를 조회하도록 반환 토큰을 지역 변수로 받은 다음 자식 필드에 쓴다. 화살표는 실제 돌 버튼 뒤 현재 기본 글자색/경계로 삼각형 세 선을 출력하고, 손잡이는 실제 기본 배경 뒤 현재 기본 type/frame과 양끝 사각형을+1 없이 배경 출력에 전달한다. scalar는 실제 기본 수명으로 직접 위임한다. 손잡이가 실제 배경을 상속할 수 있도록 GumpBackgroundNode의 sealed만 해제했다.

가상 Attach/부모·재귀 사건/직접 캡처/프레임/Draw/수명 분배를 현재 클래스 조회에 연결하고 다른 기존 파생 처리기를 보존했다. 다중 줄 표시의 네 Slider 경계도 같은 실제 값/범위/페이지/현재 위치에 연결했다.

## 2. 검증과 한계

- 집중 **21개 통과**다. 두 판본/가로·세로·0축 생성, 원본 할당 크기·partial 필드·초기 값/실제 부착, 비0 minimum의 position 분자, signed 범위·음수 페이지·감김/두 번 재적용, 부모 소비/현재 값 변경·재귀 전달, 포함 캡처/페이지·CD 가로·방송/폴링, DWORD 드래그를 고정 입력에 대조했다.
- 실제 돌 버튼/배경/SHP/공통 Draw의 삼각형4방향 끝점·빈 내부, 손잡이 기본 자산/정확한 클립·끝점, 앞뒤 guard를 확인했다. 실제 자식 우선 Cleanup의 두 배경→화살표/손잡이→Slider 반납과 캡처 해제도 검사했다. 같은 기본 저장소의 다중 줄 표시가 실제 Slider에 전체 높이28/page10/position18을 반영함을 확인했다.
- 최초20개 중3개는 기대값 계산 오류로 실패했다. C++ 본문을 다시 읽고 두 원인을 확인했다. 첫째, 동률 INT_MAX 범위는 max가 INT_MIN으로 감긴 뒤 `max < page+position`이 참이므로 position도 두 번 모두 INT_MIN이다. 둘째, Slider.Handle이0을 반환하므로 부모가0으로 응답하면 안쪽 Slider serial 통지 뒤 바깥 Thumb serial 통지도 도달한다. 기대값과 근거 주석만 수정했고 제품 코드/원본 fixture는 바꾸지 않았다. 최초 로그를 보존했다.
- `cpppj/scrollgump.json`은 게임/에뮬레이터를 실행하지 않은 정적 복원 보고서다. 이 단위는 **독립 원본 실행 대조/L1이 아니다.** fixture99/reflected-commit을 유지하고 포트 맵만198행(L1 97·port101)으로 갱신했다. 기존 코드/검사 은퇴0, 실제 게임/메뉴 캡처 비교0이다.
- 전체 **1,766개 통과(Assets363·Core1030·Client373), 실패/건너뜀0**, Release 전체 빌드 경고/오류0이다. 변경9파일의 UTF-8·한국어 주석/문서와 보호 경로 변경0을 확인했다. 로그는 Git 제외 `extracted/dotnet-slider-compile-20261029.log`, `dotnet-slider-focused-initial-20261029.log`, `dotnet-slider-focused-20261029.log`, `dotnet-slider-tests-20261029.log`, `dotnet-slider-build-20261029.log`다.

## 3. 다음

R3h-3b-2에서 `ScrollingTextGumpWidgets`의 실제 생성/128칸 소유 버퍼/Slider 자식/overlay·부착/태그·선택·커서/캡처·드래그/수명을 연결한다. 이어서 MenuGump 전체와 실제 메뉴/전체 자산/음향/GIF/호스트, R4 이후 세션/월드/HUD·전체 미션·Edit를 조립한다. 전체 목표는 계속 진행 중이다.

이번 Draw 검사는 대상을 명시한 헤드리스 조립이다. 공통 Renderer의 실제 정렬/분배(R5)나 메뉴 캡처 동일성을 대신하지 않는다. cpppj·원본/CD·AGENTS.md·공통 인계는 수정하지 않았고 게임/창·금지된 실제 재현·멀티플레이 검사를 하지 않았다. 외부 TODO.md는 커밋에서 제외한다.
