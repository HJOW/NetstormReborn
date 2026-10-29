# dotnetpj 일반 HTML 창 수명과 전체 배치 — 2026-10-29

이 단위는 `HtmlWindowGump`와 `HtmlWindowFit`을 새 `Netstorm.Client` 경로에 옮긴다. 전용 QuickHelp 창과 달리 일반 HTML 창은 실제 장식 기반, 8192BYTE 링크 저장소, 버튼20행, 본문과 보조창, 요청 치수, 메뉴 열, 기한/OnExit 명령을 소유한다. 전체 메뉴의 실행 진입이나 전체 화면 동일성을 완료한 단위는 아니다.

## 근거와 범위

- 시작 HEAD `5509df94a6c6dbbfebce316c844f87726ad2598b`, 호스트 `HJOW-Athlon`. 마지막 전체 정밀 디컴파일도 같은 호스트의2026-10-23 자료다. 새 디컴파일은 하지 않았다.
- `cpppj/src/client/HtmlWindowGump.h/.cpp`, `HtmlWindowFit.h/.cpp`, `RawMenuHtml.inc`의 기존 실제 장치 연결, 대응 Native/Client 검사를 읽었다. cpppj 소스나 원본 자료를 수정하지 않았다.
- 작업 도중 외부 CP 커밋 `f9a8cfa54009d437fa2ba1630de5284b582bd5ac`가 들어왔다. 공통 인계와 데모 실행 보고서만 변경하며 이 단위의 cpppj 근거 소스는 바뀌지 않았다. 포트 맵의 새4행과 기존 장식2행 기준은 이 커밋이다. 그 외 반영 범위는 올리지 않았다.

## 구현

- `GumpDecorationNode`의 상속을 열어 HTML 창이 같은 실제 기본/장식 객체를 쓰도록 했다. 장식 필드를 별도 복사하지 않는다.
- `HtmlWindowGumpNode`에 실제20행을 고정 저장하고 현재 count를 signed로 비교한다. 생성자는 두 명령 인자와 첫 NUL 뒤8191BYTE를 덮지 않는다. 각 행은 콜백 도중에도 같은 객체다.
- 버튼 생성/가상 부착/Instant/현재 count 증가, Kill 뒤 현재 count 재조회, 본문 LF 보충의1008BYTE 지역 버퍼 한도, 실제 새 ScrollingText/Slider/StyleText 생성과 기존 본문/보조창 반납을 보존했다.
- 입력은 release 비트를 제거한 사건 종류로 분기하며 count와 무관하게20행의 현재 serial을 검색한다. 명령과 두 동일 인자를 큐에 쓴 뒤 Kill한다. 태그 링크는 Kill 뒤 실제 저장소 반납 전에 소비한다.
- Idle은 기한0만 즉시 거부한다. 벽 시각 효과 뒤 현재 기한/명령을 읽으며0명령도 큐에 쓴다. 실제 소멸은 count감소→OnExit큐→현재 보조창조회/Kill→벽시각→기본소멸→필요시객체free 순서다.
- 전체 Fit은 현재 돌 버튼 최대 폭과 간격, 자연 본문 측정, float32 폭20단계 증가, 높이 정체/종횡비/화면 제한, Slider/스크롤 전환, 현재 메뉴1/2열의 부착/pop, 현재 버튼/본문 부착, 현재 화면 또는 뷰 중앙, 최종깊이-80을 원문 순서로 수행한다. `_ftol`의 signed64 절삭 하위DWORD와 정수 감김, NaN 비교, x87 계산값에서 각각 저장하는 float32 위치를 유지했다.
- 실제 장식·돌 버튼·다중 줄 파서/Widget·기본 트리/수명 장치 어댑터, 메뉴 스택 어댑터, 사건/캡처/프레임Idle/scalar/가상Attach 분배를 작성했다. 이전 다른 파생의 처리기는 보존한다.

## 검증과 원시 기록

독립 원본 fixture는 읽기 전용 `Link`로 연결했으며 제품 기대값으로 다시 생성하지 않았다.

| 자료 | 실제 관측 행 | SHA-256 앞부분 |
| --- | ---: | --- |
| html-window-gump-x86.tsv | 250 | `2acaf260f0f4` |
| html-window-idle-v2-x86.tsv | 88 | `b0309464c7c8` |
| html-window-fit-x86.tsv | 160 | `adaeea8c65db` |

총498행에서 생성/입력/소멸의24DWORD·전체20행·8192BYTE해시·24자식·현재전역·반환/부분중단·효과순서와 Fit의26개전체노드/20행/메뉴스택/현재전역/측정횟수·효과순서를 대조했다. Native3개는 최종 통과했다. 최초3개 실패는 검사 판독기가 일반 TSV 머리행을 실행 입력으로 읽어서 발생했고, 머리행을 제외한 뒤 전수가 일치했다. 제품/fixture를 이 실패에 맞춰 바꾸지 않았다.

추가7개는 실제 공통 등록/트리/입력/수명, 장식/20돌버튼/각배경, 동적 본문교체/소유버퍼/Slider/보호캐시글꼴/픽셀, 두열메뉴스택, 현재OnExit/보조창, 원시큐/Idle/태그/물리마지막행, 한도/부분실패를 검증한다. 배경/모서리 RLE와 화면/뷰 전역은 명시 검사 입력이며 전체 게임 자산 소유자를 조립한 것은 아니다.

최초 실제 검사 코드의 없는 버퍼 속성명과 `var` 다중 선언 컴파일 오류를 고쳤다. 픽셀 검사는 원문 가상 이동이 창을 현재640×480 뷰 중앙으로 되돌리는 것을 놓쳐64×128 표면 바깥에서 그리던 입력을 고쳤다. 현재 작은 뷰와 명시 색 태그를 넣어 같은 실제 전체Fit/그리기에서 픽셀·guard를 확인했다. 전체Fit이나 화면 중앙 동작을 검사 편의로 바꾸지 않았다.

- 집중:10개 통과, 실패/건너뜀0.
- 전체: **1,983개(Assets363 + Core1030 + Client590) 통과**, 실패/건너뜀0. 같은 최종 코드의 Release 전체 빌드도 경고0/오류0·종료0이다.
- 포트259행: L1 139/port120. 새4행은 독립 관찰 범위의 L1이다. 기존 장식은 원래 port 판정을 유지했다.
- fixture목록123개. `reflected-commit`은 `f95a8db28ceae6c036fa0d65c1f7626bcc0c31a3`을 유지한다. 목록의 행 수는 일반 TSV 머리행도 포함한다.
- 이 PC의 Git 제외 로그: `extracted/dotnet-html-window-compile-initial-20261029.log`, `dotnet-html-window-native-initial-20261029.log`, `dotnet-html-window-native-recheck-20261029.log`, `dotnet-html-window-focused-initial-20261029.log`, `dotnet-html-window-focused-recheck-20261029.log`, `dotnet-html-window-focused-second-20261029.log`, `dotnet-html-window-focused-third-20261029.log`, `dotnet-html-window-focused-final-20261029.log`, `dotnet-html-window-tests-20261029.log`, `dotnet-html-window-build-20261029.log`. 최초 실패 로그는 보존했다.

## 남은 작업

`HtmlWindowMetadata`/`HtmlWindowFactory`의 해석/두번파서·임시문자열 고리/현재전역과 전체 소유 연결, MessageGump/대화창, RawMenuControls 상위 등록/모달/링크 소비자와 전체 자산/ConfigInterface/음향/호스트 조립이 남는다. 실제 전체 메뉴 화면 비교 D1, R4 이후 전체 미션/월드/HUD/Edit·후순위 멀티플레이 사전 구현도 남는다.

이번에는 게임/별도창·원본/PE실행·금지된플레이재현·멀티플레이검사를 하지 않았다. 기존 코드/검사 은퇴0, 전체 메뉴/게임 캡처0이다. 모듈의 L1과 실제 하위 장치 연결을 전체 게임 동일성이나 전체 플레이 가능의 증거로 세지 않는다.
