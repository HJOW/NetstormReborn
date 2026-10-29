# dotnetpj R3h-4d — 실제 메뉴 생성·자식·부착·소유 수명

2026-10-29, HJOW-Athlon. 장식 단위 `acc8f3dc0246` 뒤 cpppj `MenuGumpWidgets.h/.cpp`와 `GumpFormBinding.h/.cpp`를 기준으로 옮겼다. 현재/마지막 정밀 디컴파일 PC는 HJOW-Athlon이다. `menuwidgets.json`의 정적 근거와 현재 cpppj 몸체를 사용했으며 원본 게임/GUI/PE 에뮬레이터를 실행하지 않았다.

## 1. 구현

MenuGumpWidgets는 기존 실제 배경·장식·다중 줄 제목/소유 버퍼/Slider·항목 레이아웃/StyleText·트리/수명을 사용한다. 생성 입력은 원본 문자열/자식 variant와 flags/command/parameter/condition이다. 문자열 객체는 c_str처럼 안정된 NUL 종료 BYTE를 소유한다. 조건 false도 명령/인자를 먼저 쓰고 문자열/자식 처리를 생략한다. 자식은 실제로 숨겨 부착한 뒤 앞 항목에 현재 등록 ID를 쓴다. 각 항목마다 Reflow를 추가하지 않으며 최종 방향/장식 뒤 한 번 수행한다. 40번째 항목은 쓰기/개수 증가 뒤 원본의 `<40` 계약으로 실패한다.

실제 배경 생성 뒤 원본 부분 필드와 문맥/종류/깊이/개수를 쓴다. 미작성 offsets/목표 크기는 보존하고 전체 높이/항목만0으로 만든다. 각 항목 객체와 Styled 소유 필드의 주소는 유지한다. fancy는 실제0xc4 장식→0xd8 제목을 생성하고 제목 자신의128칸 버퍼/Slider/overlay 생성도 기존 몸체를 사용한다. 별도 자식 부착/깊이/배경 비트를 보존한다. 제목 Reflow의 가상 경계를 실제 RequestScroll/FitContent에 연결했다.

가로/세로/장식 controlled 시작의 index 증가·빈 칸 검사·0xb90 할당/생성·현재 칸 저장·idle·추가 배경 플래그 순서를 구분한다. 콜백 뒤 현재 index를 재조회한다. 부착은 root 비트/parent0 재사용·전체 화면 보정과 패치의 중간 float32 저장/CD 비저장·fancy Y 합 순서·DWORD 차이를 유지한 뒤 실제 기본 Attach에 맡긴다. scalar는 개수 감소→현재 선택 통지→40개 전체 소유 조각 역순 Clear→기본 소멸이며 자식 우선 삭제는 기존 GumpLifetime을 쓴다.

객체 연결은 실제 GumpFormHost의 공용 SID 풀에 연결했다. 메뉴 Form(type3,2) 생성→현재 Gump 등록 ID→객체 종속 부착 뒤 같은 기본 flags/object/form 필드를 쓴다. 객체0은 이전 연결을 지우지 않는다. 객체 삭제 알림은 매번 현재 등록표의 토큰을 읽고 실제 자식 우선 Kill을 수행한다. UI Kill은 실제 같은 풀의 Form 삭제/Unpop/반납을 호출하고 메뉴는 다음 Cleanup에서 소멸한다. 다른 Form 삭제 분배는 보존한다. Core의 RawGumpFormHost는 기존10.78 범위를 그대로 사용하며 CD Form의 추가 복원/서버 전송 경계는 이 단위에서 확장하지 않았다.

## 2. 검증

- 집중 **21개 통과**: 메뉴17개와 실제 Form 연결4개다. 같은 실제 기본 트리/장식/제목/소유 조각·버퍼/Slider/글꼴/팔레트/픽셀/수명을 조립했다. 생성·조건부 부분 필드/이전 항목 자식/미작성 배열·stable 소유, 세 시작 경로/현재 index·부분 실패, 실제 제목 배치/모서리·글리프 픽셀, 전체 화면 보정/생략, 선택 콜백 뒤 전체40개 역순 소유 해제, 실제 자식/버퍼 우선 소멸을 확인했다.
- 실제 객체 연결은10.78에서 명시 Form 알림과 공통 부모 삭제 두 경로를 검사했다. 같은 풀의 owner/parent/종속 머리·반납 상태와 실제 UI 지연 Kill/Cleanup까지 관측했다. 현재 등록표 재조회·범위/판본/타입 오류·다른 Form fallback도 확인했다. 이 검사는 멀티플레이 실행/전송 검사가 아니다.
- 최초 집중 검사17개 중 새 검사4개가 실패했다. 원본 GumpText의 shadowX/행별 shadowY 여백, 제목의 패치 `~E`와 CD 기본 문맥/색 차이를 새 검사 예상에 빠뜨린 것이었다. 기존 L1 GumpText와 cpppj 제어부/측정 순서를 재확인해 새 검사만 고쳤다. 제품 구현/원본 기대값 수정은 없다. 이어 Form4개를 포함한 집중21개가 통과했다.
- 전체 **1,844개(Assets363+Core1030+Client451) 통과**, 실패/건너뜀0이며 Release 전체 빌드는 경고/오류0이다. Git 제외 로그는 `extracted/dotnet-menu-widgets-compile-20261029.log`, `dotnet-menu-widgets-focused-initial-20261029.log`, `dotnet-menu-widgets-focused-20261029.log`, `dotnet-menu-widgets-tests-20261029.log`, `dotnet-menu-widgets-build-20261029.log`다.
- 신규 독립 PE 관찰0·실제 게임/메뉴 캡처0·기존 코드/검사 은퇴0이다. 새 포트4행은 port이며 전체212행(L1 101/port111)이다. fixture101개/reflected-commit은 유지했다. 이 정적 계약/실제 조립 검사를 독립 L1이나 전체 메뉴/플레이 동일성으로 세지 않는다.

## 3. 다음

MenuGumpInput/Draw를 같은 실제 트리/표면/사건에 연결한다. 이후 실제 메인 메뉴/전체 자산/ConfigInterface/음향/GIF/호스트를 조립한다. R4 이후 월드/세션/HUD·전체 미션·Edit·후순위 멀티플레이 사전 구현도 남으며 전체 목표는 계속 진행 중이다.

보호 경로·cpppj·공통 인계·외부 TODO.md를 수정하지 않았다. 실제 사제 포획/희생·특정 적 공격·승리를 재현하거나 멀티플레이를 검사하지 않았다.
