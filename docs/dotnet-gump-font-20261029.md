# dotnetpj R3g-1 — 원본 글꼴·측정·글리프 출력

2026-10-29, HJOW-Athlon. 기준 cpppj는 `e074cad7f385`의 `BitmapFont.h/.cpp`·`GumpText.h/.cpp`·`GumpTextDraw.h/.cpp`다. 실제 글꼴 포인터와 바이트 폭 표를 기존8비트 표면/트리의 출력에 연결했다.

## 1. 구현

`Netstorm.Client/Display/BitmapFont`는 원본 캐시 네 표와256개 독립 VFX를 읽는다. 측정 폭·ABC A·출력 전진 폭을 별개로 두고, 포인터 없음과 특수 빈 프레임도 구별한다. 원본 캐시의 높이/상승/하강, 손상/꼬리/블록 경계와 실제 RLE를 검사한다. 안전 Measure의64비트 합/범위 오류는 원본 Gump 측정의 DWORD 감김과 구분한다.

`FontStore`는 원본의7슬롯/5스타일 중 실제 초기화 조합만 읽는다. 파일 이름의 슬롯 높이/굵기와 생성 때의 bold 굵기를 구별하며, 캐시가 없거나 손상됐으면 같은 메모리 생성 경계로 넘어간다. 파일 공급은 읽기 전용 조회자이며 실제 파일에는 쓰지 않는다. `WindowsBitmapFont.Generate`는 원본 CreateFontA·ANSI ABC·색32/100의 메모리8비트DIB·127픽셀 RLE로 생성한다. DC/글꼴/비트맵/이전 선택을 예외에서도 정리하며 실제 창은 없다. Win32 생성만 호스트로 분리했다. Linux에서 캐시 없는 생성은 후속 장치 경계다.

`GumpText`는 첫 폭 표/별도 논리 슬롯 높이/명시NUL/signed 길이와 출력 쓰기 순서를 보존한다. width/height가 같은 ref일 때10.78은 현재 출력에 그림자 폭/높이를 더하고 CD는 저장 합계를 쓰는 차이도 유지한다. 불법 스타일의 측정 폭과 높이는 서로 다른 슬롯을 선택할 수 있다. Metrics는 논리 높이 대신 캐시 메트릭을 반환한다.

`GumpTextDraw`는 각 패스의 색 설치 뒤 포인터가 있는 글리프만 출력하고 셋째 폭만 DWORD로 전진한다. 특수 빈 프레임은 출력 콜백/전진이 있고 포인터 없음은 전진하지 않는다. 그림자→위/오른쪽/아래/왼쪽 외곽선→본문 순서를 유지한다. 기본/명시 문맥 모두 Position 투영 뒤 읽으며 명시 구조체는 in 참조로 전달한다. 글자100번만 바꾼 전체256바이트 표를 현재 VFX 표에 설치하고 이전 색표는 복구하지 않는다.

## 2. 검증

- `gumpglyph-x86.tsv`786행(입력Font18·원본Case768)과 `gumptext-x86.tsv`933행(Font18·Case915)을 Link로 연결했다. 세 판본의 모든 글꼴 정체성·색/좌표/순서·측정 폭/높이·alias를 원본 기대값 그대로 대조했다. fixture 목록98개, 포트 맵172행이며 전체 reflected-commit은 유지한다.
- 집중17개가 통과했다. 실제 클론 자료의 캐시18개/4,608글리프 전수를 읽었고 해제 픽셀 합302,480이 기존 C++ 독립 판독기 근거와 일치했다. 실제 슬롯 메트릭4종도 대조했다. Windows GDI 생성 글꼴을 같은 글리프 반복/현재 클립에 출력해 `NetStorm` 본문 픽셀40개 초과를 확인했다. 캐시 누락/손상 fallback·안정된 객체·실제 생성 굵기·잘못된 범위 전 무쓰기·투영 후 문맥·손계산 한 픽셀 출력도 검사했다.
- 실제 출력에서 셋째 폭 대신 첫 측정 폭을 쓰는 변이는 원본 세 판본 모두 좌표 불일치로 실패했다. 변이는 제거했으며 fixture 기대값 수정은 없다.
- 원복 후 전체 **1,624개 통과(Assets363·Core1030·Client231), 실패/건너뜀0**, Release 전체 빌드 경고/오류0이다. 변경14파일의 UTF-8·한국어 주석과 보호 경로 변경0을 확인했다. 실제 메뉴 실행/캡처 비교는0개이며 이 단위는 L1/헤드리스 실제 글꼴 출력이다. Renderer 공통 장면 통합과 기존 근사 UI 교체는 아직 남는다.
- 로그는 Git 제외 `extracted/dotnet-gump-font-focused-20261029.log`·`dotnet-gump-font-mutation-20261029.log`·`dotnet-gump-font-tests-20261029.log`·`dotnet-gump-font-build-20261029.log`다.

## 3. 다음 연결

R3g-2에서 `ButtonGump`·`StoneButtonGump`와 `BindStoneButtonText`를 옮겨 실제 생성/라벨 측정/배경 부착/입력/가상 Draw/자식 우선 파괴를 연결한다. C++ `GumpTextTests`의 돌 버튼 통합 부분과 `GumpGlyphTests`의 Renderer 장면 부분은 각 해당 모듈 단위에서 이어야 한다. 이후 GIF·메뉴 위젯·호스트 프레임/입력 큐·전체 자산 로더를 조립한다. 기존 코드/검사 은퇴는0개다.

cpppj·공통 인계·원본/CD·AGENTS.md는 바꾸지 않았다. 게임/창·금지된 실제 재현·멀티플레이 검사를 하지 않았으며 외부 TODO.md는 커밋 대상에서 제외한다.
