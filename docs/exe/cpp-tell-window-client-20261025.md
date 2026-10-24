# Tell 전체 HTML 부모·공통 입력·Guide 연결 — 2026-10-25

단위414는 기존 Tell의 별도 대화상자 표시·메타·OnExit 수명을 실제 전체 HTML 공장과 공통 Gump 수명으로 교체한다. 현재/마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**이다. 새 전체 디컴파일·원본 직접 실행은 없고 실제 클론 GUI는11회다. 전체 목표는 활성/미완료다.

## 원문과 실제 소유자

[TellSection](../../cpppj/src/client/TellSection.cpp)의 `ShowTellInConfig`는 원본4cf730의 지역 `param`을 파라미터 해석·본문 조회·실제 두 번 HTML 파서와 메타·공장 반환까지 유지한다. 창 생략 mode의 모든 비트를 보존하고 폭/높이0을 공장에 전달한다. 섹션 없음과 존재하는 빈 본문을 구별하며 공장 실패에서도 지역 등록부를 반납한다. BLANK는 호출자 경로에 남긴다.

[UberGump](../../cpppj/src/client/UberGump.cpp)는 `ParseDialogPage`·별도 본문 배치·메뉴 후처리·수동 대화상자 그림·페이지 개수와 OnExit 표를 제거하고 실제 `RawMenuControls::ShowHtmlWindow`에 연결한다. 실제 메타와 메뉴·돌 버튼·Fit·modal·원시 입력 큐를 사용한다. 본문 없는0 반환이나 Guide.Init의 창 생략 mode1이 기존 부모를 교체하지 않는다. 실제 부모 Kill은 즉시 현재 모달을 반납하지만 인스턴스와 OnExit는 후반 공통 Cleanup에 남는다. 기존 메인 버튼 재합성은 이 수명을 보존한다.

Guide의 dialogsClear는 원문4c8e90과 같은 **565dd4 현재 모달 포인터**를 읽는다. GuideOpen은 소멸 전 HTML 인스턴스 수를 읽으므로 이 둘을 합치지 않는다. 로딩 안내는 Guide.Init 앞의 가짜 대기 화면에서 실제 동기 로드 호출 안으로 옮겼다. 원문4c8dc0처럼 반납 시 Kill 뒤 transient serial을0으로 쓰며 created serial은 보존한다. 이 변경은 GUI3에서 실제로 발견한 첫 미션 시작 교착을 해결한다.

공식 캠페인 파일은 현재 원본 디스크의 `d`가 아닌 TAFF 안에 있다. 원문4588a0/4587f0의 기본 디스크→기본 TAFF→보조 디스크/TAFF 및 첫 대소문자 무시 중복 제거를 확인하고 [BaseFile](../../cpppj/src/o/BaseFile.cpp)에 현재 VFS의 `MatchFinder`를 연결했다. 현재 등록 TAFF 이름 인덱스 순서·원문 상대 경로 바이트와 최초 대소문자를 유지하며 정렬은 상위 Tell에서 한다. 단일 TAFF 풀만 있는 현재 VFS의 별도 보조 TAFF 풀은 미복원 경계다. 기존 정렬 `Match`와 디스크 전용 `MatchDisk`의 의미는 바꾸지 않는다.

10.78 메인 Campaign target은 원문4d0120의 고정 `UCampaign`이다. 이를 고르기 위해 영구 `tell` Config를 등록하지 않으므로 첫 등록 객체를 읽는 원문 조회 순서도 가리지 않는다. 10.72는 기존 별도 분기를 남기며 전체 HTML 동일성을 주장하지 않는다.

## 관측과 검사

기존 선택 Tell858행과 전체 공장106행의 Native 자료를 읽기 전용으로 재생했다. **964행 일치·두 정밀도 차이0**이며 새 fixture를 생성하거나 바꾸지 않았다. 각 관측의 Config/하위 HTML/Gump/장치 경계는 기존 보고서대로다. 이번 FileFinder 연결은 정적 원문 대조·실제 OS/TAFF와 GUI 검사이며 FileFinder 본체 전체의 Native 실행 증거로 확대하지 않는다.

[실제 공장 결합 검사](../../cpppj/tests/HtmlWindowFactoryClientTests.cpp)는 지역param·mode0/1/ffffffff·빈 본문/조회 실패/예외·실제 메뉴 클릭의 큐 인자·모달 즉시 반납/OnExit 지연·재합성·중첩 부모와 버튼 Point·현재 모달 복귀·공식 TAFF8행을 검증한다. [VFS 검사](../../cpppj/tests/MenuTests.cpp)는 실제 디스크·TAFF·보조 순서, 첫 중복, 대소문자·literal 경로와 외부 절대 경로를 확인한다. 안내 기존 결합에는 transient serial0/created 보존을 추가했다.

최종 Release 빌드·집중 검사·전체 검사 결과와 제품/검사 SHA는 [제품 보고서](../../cpppj/tell-window-client-runtime-20261025.json)에 기록한다. 원시 빌드/집중/전체/Native/보호 감사는 `extracted/cpp-tell-window-414-*`다. 빌드2의 검사 컴파일은 수정 반영을 위해 자신의 프로세스 트리를 확인한 뒤 중단했으므로 전체 성공으로 세지 않는다. 빌드3도 게임 target은 완료했지만 검사 컴파일의 파일별 순차 실행을 확인하고 CMake9676/부모14948 경로/명령 검증 뒤 중단했다. 최종 빌드4는 작업별 `/MP4`와 설치된 VS18 `UseMultiToolTask/CL_MPCount/EnforceProcessCountAcrossBuilds`를 사용하며 영구 빌드 설정은 바꾸지 않았다.

빌드4의 최초 VFS 집중 검사에서 새 임시 경로 보호 검사가 끝 구분자를 포함한 부모와 포함하지 않은 부모를 다르다고 보아 두 검사가 생성 전에 실패했다. 실제 임시 부모 정규화를 교정한 빌드5는 검사 소스만 바꿨고 제품 SHA는 같다. 최종 집중 **공장13+Tell26+버튼7+BaseFile4+안내8=58회/실패0**, 공장/Tell에 중복된 신규4개를 제외하면 **고유54개**다. 최초 실패 로그를 보존한다.

최종 전체 CTest는 **2796개/실패0·318.93초**다. 실행 제품 SHA256은 `081f387468ae5e06e6cb61a7189102e69535e92abbac81f5be0cce46a208ae07`, 검사 SHA256은 `4e3f62b899856ae9ed50b07ecb01809ab249bb89ffc9bd53cf7df6974f8af807`다. 보호 **originals1394/CD425/AGENTS/정밀 소스=1821개**의 최종 감사와 모든 소스·참조·GUI·검사 원시 SHA는 제품 보고서에 기록한다.

실제 GUI1은 공식 항목0 실패를 보존했다. GUI2는 수정된 목록/Back이 통과했고 GUI3에서 Guide 시작 교착을 확인했다. 빌드3의 GUI4는 정상 클릭으로 Tutorial1 실제 월드와 A. 안내에 도달했다. GUI5/6은 게임 종료0과 MORE/OK가 작동했지만 자동 미션 단계 진행을 기대한 검사 조건이 잘못되어 실패했다. 원문 `[A.] MORE,Tell,A1.`와 `[A1.] OK,DoNothing,0`에 맞춘 GUI7은 **A.→A1.→Mission/paused0/HTML0/현재 모달0**을 확인했다. GUI8은 같은 실행 파일에서 **Campaign8행(구분선2)→Early6행→Back→Back→메인/HTML0**을 확인했다. 최종 빌드4 제품에서 GUI9/10으로 두 경로를 다시 검증했다. 상태 주입·kernel-tell·미션 단계 강제 변경은 없다. 실행마다 원본 설정/전체화면 표식의 존재와 바이트를 복원했다. GUI7의 실제 준비 화면도 이미지로 확인했다.

최종 제품의 GUI11은 첫 안내 뒤 원문 지시대로 **F4**를 눌렀고, 강제 단계 쓰기 없이 현재 단계65→66/안내 **B.**/실제 모달1로 진행했다. 이 화면도 이미지로 확인했다. 첫 안내를 닫은 사실과 다음 게임 입력으로 실제 Guide가 진행한 사실을 구별해 남긴다.

## 남은 범위

전체 Uber 루트의 배경 GIF 생성/교체·HUD dock/resize와 원본 MainMenu/Edit 진입은 미완료다. Tell의 현재 배경은 공유 이름을 읽는 기존 GIF 표시 어댑터다. Production/Minimap·State/Core/Pool/Win32 접합, 모든 Early/캠페인/Demo의 사용자 완주·Edit/언어별 시나리오 전체·393 Wide SaveOptions와 완전 동일성은 계속 작업한다. 선택된 Input/전송/기술 장치의 미연결도 남으며 임의 커스텀 스크립트 전체 지원을 주장하지 않는다.
