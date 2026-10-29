# dotnetpj R3f — 배경 생성·질감·단색·체크 출력

2026-10-29, HJOW-Athlon. 기준 cpppj는 `436deaf700d8`의 `src/client/GumpBackground*`6파일이다. 같은 기본 생성/등록·수집/위치·클립·픽셀 표면을 사용하는 배경 파생을 옮겼다.

## 1. 구현

`GumpBackgroundCreate`는 기본 생성/등록 뒤 배경 가상 표, 현재 signed BYTE 색, flags/type/frame, 실제 SHP 헤더의 signed WORD 치수를 쓴다. 타입0에서는 이전 shape와 타일 치수를 보존한다. CD는 프레임 조회 전에 shape를 저장하고10.78은 조회 뒤 현재 shape를 읽는다. 기본/파생 노드가 같은 객체인지 검사한다.

질감 주기는 실제 출력 그림의 크기가 아니라 SHP 메타 폭/높이-1이다. 화면 좌표의 배수를0 방향으로 나눠 여분 타일까지 행 우선으로 출력한다. 바탕→왼쪽→오른쪽→위→아래의2픽셀 테두리 순서와 기존 비트에 OR하는 명암/눌림, 밝음 우선순위를 보존한다. 원본과 C++의 과대한 반복 안전 경계도 유지했다.

`GumpBackgroundDraw`는 체크→단색→질감 순으로 분기하고 flat 경로는 현재 색의1픽셀 선을 왼쪽→위→오른쪽→아래 순서로 출력한다. 매 질감 pass에서 현재 type/frame의 헤더를 조회한 뒤 클립을 저장한다. 초기 클립 설정/픽셀 출력 예외에도 논리/VFX 클립을 복구하며 이미 바뀐 명암표/픽셀은 남긴다. 디버그 색은 fill 호출 전에 DWORD로 증가한다.

`GumpBackgroundAssets`는 명시 타입/SHP 바인딩으로 실제 헤더를 안정된 객체에 보관한다. 논리 수와 물리 레이어 수를 구분하고 flags1 0x440000의 추가 레이어 범위를 확인한다. `GumpBackgroundPixelTarget`은 실제 RLE 해제·현재 소스 치환표·투명 픽셀·프레임 상대 영역을 같은 `GumpPixelTarget`에 연결한다. 생성 때 저장한 옛 shape/치수를 출력에 쓰지 않는다.

## 2. 검증

- `gumpbackground-x86.tsv`177행(명암표9·질감168)과 `gumpbackgrounddraw-x86.tsv`627행(3판본×209)을 Link로 연결했다. 전자는 전체256바이트 명암표·전체 pass/clip/map/draw 순서를, 후자는 전체 가상/직접 사건·debug/shade·실제 PE fill 픽셀768바이트를 독립 기대값과 대조한다. 후자의 선/SHP 픽셀은 원본 fixture의 명시 경계이며 별도 실제 RLE 검사로 확인했다.
- 집중16개가 통과했다. 기존 C++의 독립 계약에 생성 콜백이 바꾼 현재 frame/shape·판본별 저장 시점·signed 색/치수·예외 전 기본 등록/필드 보존·초기 clip 설정 실패 복구를 추가했다. 출력 픽셀은 화면 격자/눌림/투명 레이어/현재 색표/메타 주기를 손계산 기대값으로 확인했다.
- 밝음과 어두움이 함께 있을 때 어두움을 우선시키는 변이를 원본 기록 대조가 검출했다. 변이는 제거했고 기대값 수정은 없다.
- fixture 목록96개, 포트 맵164행(L1 97·port67)이다. 전체 reflected-commit은 기존 값을 유지한다. 원복 후 전체 **1,607개 통과(Assets363·Core1030·Client214), 실패/건너뜀0**, Release 전체 빌드 경고/오류0이다. 변경12파일의 UTF-8 및 보호 경로 변경0을 확인했다. 실제 실행 캡처 비교는0개다.
- 로그는 Git 제외 `extracted/dotnet-gump-background-focused-20261029.log`·`dotnet-gump-background-mutation-20261029.log`·`dotnet-gump-background-tests-20261029.log`·`dotnet-gump-background-build-20261029.log`다.

## 3. 남은 연결

`GameAssets` 전체 소유 구조는 아직 새 C# 경로에 없으므로 cpppj의 `GameAssets` 전용 생성자와 선택 실자산 Inspector 검사는 그 로더 조립 때 추가한다. 이번에는 같은 SHP와 명시 타입 바인딩을 공급하는 C++ 독립 경로를 옮겼다. 실제 메뉴/미션 로딩이나 화면 일치 완료로 세지 않는다.

다음은 `BitmapFont`·`GumpText`·`GumpTextDraw`의 글리프 출력과 버튼/돌 버튼의 배경 자식·가상 Draw를 같은 수명에 조립한다. 이후 GIF·텍스트/메뉴 위젯·호스트 프레임/입력 큐다. 기존 근사 코드/검사 은퇴0이며 전체 사용자 플레이 목표는 계속 남는다.

cpppj·공통 인계·원본/CD·AGENTS.md는 변경하지 않았다. 게임/창·금지된 실제 재현·멀티플레이 검사는 수행하지 않았고 외부 TODO.md는 커밋 대상에서 제외한다.
