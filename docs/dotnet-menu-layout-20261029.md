# dotnetpj R3h-4a — 메뉴 기본·항목 소유·전체 레이아웃

2026-10-29, HJOW-Athlon. 기준 cpppj는 `6c0a1865fb35`의 `MenuGump.h/.cpp`·`MenuGumpLayout.h/.cpp`와 StyleText의 BindMenuLayoutStyleText다. 실제 메뉴/배경 노드·스타일 소유/글꼴·기본 트리/프레임/수명 저장소에 연결했다. MenuGumpControls/Widgets/Input/Draw와 실제 메뉴 화면 조립은 다음 단위다.

## 1. 구현

MenuGumpItem은 원본40바이트 라벨 뒤 다섯 DWORD와 실제 Styled 소유 필드를 갖는다. 고정40항목의 객체 주소를 보존해 콜백 중 현재 필드 변경과 ref head를 같은 저장소에서 사용한다. MenuGump는 장식 Push 뒤 현재 제목/장식/크기를 읽고 float32 좁힘 전 합계로 배율 크기를 계산한다. 방향별 항목 위치·한 축0 확장 시작·실제 idle 등록·판본별 NaN 분기·높이→폭→Pop·다음 호출의 expanding 해제와 idle 유지, controlled 표의 현재 칸/판본별 삭제 순서·콜백 뒤 현재 index 제거·실제 등록표 루트의 객체 부착을 옮겼다.

MenuGumpLayout은40개 한도/선두·연속 구분선 생략, command/parameter/childId의 조건부 일부 쓰기, 자식 숨김/부착의 판본별 순서와 콜백 뒤 현재 항목 수 재조회를 보존한다. 일반 라벨은 strncpy40이며 별도NUL을 추가하지 않는다. 실제60바이트 직렬화로 라벨 뒤 childId 등의 바이트까지 일반 측정기에 공급한다. 스타일은 font0 문맥 복사→실제 소유 Clear→현재 disabled 접두→현재 입력→256바이트 한도→Parse 순서다. BindMenuLayoutStyleText는 실제 항목 필드를 ref로 Clear/Parse/두 번 측정에 연결한다.

전체 재배치는 각 콜백 뒤 현재 개수/문맥/방향/자식 표식/여백을 읽는다. signed DWORD 합/곱/CRT 절삭, 제목 Push/Fit/Layout 후 현재 제목 크기·위치, 세로 누적offset과 가로 공통 높이, 미사용 배열 칸 보존을 유지했다. 전체 GameAssets/메뉴 생성·입력·Draw를 추측해 대신 구현하지 않았다.

## 2. 독립 원본 대조

- `cpppj/tests/fixtures/menugump-x86.tsv`의 **308행**(두 판본 각154): 전체55DWORD×3노드, controlled20칸/idle/루트, 콜백 직전 상태·명시 원본 패치, 반환값이 일치한다.
- `cpppj/recovery-menulayout-observations.tsv`의 **260행**(두 판본 각130): 전체 메뉴2960바이트+기본 자식160바이트×2, 콜백 인자/원문과 같은 CRC32·명시 입력 패치, 반환값이 일치한다. 측정/스타일/제목/가상 경계는 원본 관찰의 외부 응답 입력을 그대로 공급한다. 실제 글꼴/스타일 구현을 그 응답으로 바꾸지 않는다.
- 관찰 파일을 복사/수정하지 않고 csproj Link로 연결했다. 최상위 기존 관찰을 위해 동기화 도구/목록의 읽기 전용 `cpppj-root` 출처를 추가했다. 해당 파일은 현재 LF SHA `8efbd8aa472d...`이며 CRLF로 표현한 SHA `75e30d4ee6c7...`가 기존 menulayout.json에 기록된 값과 같아 줄바꿈 차이임을 확인했다.
- fixture 목록은101개이며 신규 원문에는 머리행까지309/261행을 기록한다. 전체 reflected-commit `f95a8db2...`는 올리지 않았다. 포트 맵은204행(L1 101·port103)으로, 새 메뉴4행만 L1이고 실제 StyleText 바인딩의 기존2행 기준만 갱신했다. 데이터568행 대조를 실제 메뉴/전체 플레이 동일성으로 합산하지 않는다.

## 3. 실제 조립·검증

- 집중 **14개 통과**다. 원본 두 대조4개 외에 실제 스타일 소유 Clear/Parse/측정·두 색 글리프 픽셀/guard·두 방향 재배치, 라벨40BYTE 뒤 필드 측정, disabled 접두의255/256 경계·부분 예외, Clear 콜백의 현재 입력/플래그와 저장 문맥, 실제 트리 부착/수집·controlled 제거, 실제 idle/프레임/기본 재그리기, 한도/부분 필드/손상 입력을 확인했다.
- 최초 집중 검사 컴파일은 잘못 지정한 RenderCollectionEntry 네임스페이스1건(연쇄 진단2개)으로 실패했다. 올바른 기존 타입을 사용하도록 검사만 고쳤다. 검사 글꼴의 문맥 설정은 일반 항목에 없는 캐시를 요구하지 않고 실제 스타일 SetText의 font0 강제를 검증하는 위치에 두었다. 원본 기대값은 변경하지 않았다.
- 전체 **1,797개(Assets363+Core1030+Client404)가 통과**했고 실패/건너뜀0이다. Release 전체 빌드는 경고/오류0이다. Git 제외 로그는 `extracted/dotnet-menu-layout-compile-20261029.log`, `dotnet-menu-layout-native-initial-20261029.log`, `dotnet-menu-layout-focused-initial-20261029.log`, `dotnet-menu-layout-focused-20261029.log`, `dotnet-menu-layout-tests-20261029.log`, `dotnet-menu-layout-build-20261029.log`다.

## 4. 다음

R3h-4b에서 MenuGumpControls/Widgets/Input/Draw를 연결하고 실제 메뉴의 항목·제목·장식·선택·키보드·배경 출력으로 이어간다. 이후 실제 메뉴/전체 자산/ConfigInterface/음향/GIF/호스트와 R4 이후 월드/세션/HUD·전체 미션·Edit를 조립한다. 전체 목표는 계속 진행 중이다.

기존 코드/검사 은퇴0, 실제 게임/메뉴 캡처 비교0이며 명시 Draw 대상의 헤드리스 픽셀 검사가 실제 Renderer 정렬/분배를 대신하지 않는다. cpppj·원본/CD·AGENTS.md·공통 인계는 수정하지 않았다. 게임/별도 창·금지된 실제 재현·멀티플레이 검사를 하지 않았고 외부 TODO.md는 커밋에서 제외한다.
