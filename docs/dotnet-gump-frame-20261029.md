# dotnetpj R3d-2 — Gump 프레임 통지·idle·재그리기

2026-10-29, HJOW-Athlon. 기준 cpppj는 `9fcc08d7153f`의 `src/client/GumpFrame.h/.cpp`다. 기존 생성/등록/트리/수명 노드와 표를 그대로 사용한다.

## 1. 구현

`GumpFrame.Broadcast`는 등록 루트를 제외한1~999칸을 읽으며, visible/notify/비 dead 조건을 통과한 노드에만 통지한다. 필터에 실패한 비영 칸도 encountered를 늘리며 현재 signed count를 다음 슬롯마다 다시 읽는다. callback이 count를 줄이거나 뒤 표를 바꾸면 다음 처리에 바로 반영된다.

Idle은 호출 시작 판본의500칸 또는80칸을 순서대로 방문하고 현재 비영 토큰을 가상 함수로 전달한다. visible/dead/void/idle 플래그나 부모 visible을 다시 검사하지 않는다. 지나간 칸은 콜백이 채워도 되돌아가지 않는다.

RedrawNode는 현재0x42000 조건에 따라 redraw(false)를 호출하고, 그 뒤 현재0x20이 켜졌으면 visible 자식만 재귀 처리한다. 하위 dead/void는 추가 필터가 아니다. 하위 next는 재귀 뒤 현재 값으로 읽지만 RedrawRoots의 최상위 next는 요청 전에 보관한다. 실제 등록 루트를 사용하며 수집 override나 루트 자신의 visible은 사용하지 않는다.

## 2. 검증

- `gumpframe-x86.tsv`810행(3판본×270)을 Link로 연결했다. 전체 노드160바이트·현재 개수·등록 표1000칸·idle 활성 구간과 콜백 직전 전체 상태를 원본 기대값과 대조했다. fixture 목록92개·포트 맵152행이며 전체 reflected-commit은 유지한다.
- 집중9개가 통과했다. 원본 재생3개와 C++ 독립 계약6개다. 실제 idle 콜백 중 지연 삭제/등록 해제/기본 소멸자/저장소 해제, CD/패치 idle 길이, 현재 signed count, 통지 필터와 비영 칸 수, 최상위/하위 링크 조회 차이, 순환/누락, 예외 전 쓰기 보존을 확인했다.
- notify 비트가 있는 칸만 encountered에 세는 변이에 세 판본 모두 원본에 없는 추가 Broadcast 호출을 검출해 실패했다. 변이는 제거했으며 기대값 수정은 없다.
- 원복 후 전체 **1,574개 통과(Assets363·Core1030·Client181), 실패/건너뜀0**, Release 전체 빌드 경고/오류0을 확인했다. 변경8파일의 UTF-8과 보호 경로 변경0도 확인했다. 실제 실행 캡처 비교는0개이며 이 단위는 L1과 헤드리스 수명 통합 검증이다.
- 원시 로그는 Git 제외 `extracted/dotnet-gump-frame-focused-20261029.log`·`dotnet-gump-frame-mutation-20261029.log`·`dotnet-gump-frame-tests-20261029.log`·`dotnet-gump-frame-build-20261029.log`에 있다. 기대값 수정은 없다.

## 3. 다음 작업

다음은 `GumpDraw`의 임시 클립/복구·현재 픽셀 표면, `GumpVisual`과 기본 배경/위젯이다. 이어서 글꼴/GIF·메뉴와 실제 호스트 프레임/입력 큐를 조립한다. 독립 프레임 처리의 L1/수명 통합을 실제 메뉴 또는 전체 게임 플레이 완료로 세지 않는다. 기존 근사 코드/검사는 은퇴시키지 않았다.

cpppj·공통 인계·원본/CD·AGENTS.md는 변경하지 않았다. 게임/창·금지된 실제 재현·멀티플레이 검사는 수행하지 않았고 외부 TODO.md 변경은 커밋에 포함하지 않는다.
