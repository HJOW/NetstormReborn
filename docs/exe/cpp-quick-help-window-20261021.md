# 전용 Quick Help 창 전체 생성·배치·본문/그림과 HTML 부모

단위256의 시작 커밋은 `e990ec83`다. 현재 PC와 마지막 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-20**로 일치한다. AGENTS와 양 인수인계를 다시 읽었고 새 디컴파일·원본/CD·AGENTS 변경은 없다. 직전 단위는 전용 파생 HTML/동적 정보/링크·지식/공장 공용 요구 에너지를 구현·기록·커밋한 진행 턴이다. 전체11미션 목표는 진행 중이다.

## 실제 전용 창 처리기

`QuickHelpWindow`는 실제 `004c5380/58a0/5970/5d60`의 전체 생성·정리·표시 갱신·배치를 옮긴다. `QuickHelpContentNode`가 기존 제어용 `QuickHelpWindowNode`를 상속해 같은 기본/버튼/학습 필드와 나머지 그림/장식/정보/HTML/세 배경/스트림을 함께 보관한다. 창 종류 BYTE19·가상 표514ec4·450×350·flags800/등록 serial을 설치한다. 기존 `RawMenuControls`/Client 가상 분배 연결은 아직 후속이다.

기본 생성 뒤 현재 스크립트 깊이를 조회한다. 양수이면 같은 마지막 popup에 명령02000095/현재 스크립트 이름을 쓴다. 장식·RGB(55,51,54)의 현재 팔레트 색을 쓰는 안쪽 배경·03e40000 그림/본문 배경·HTML 부모·돌 버튼 순서를 유지한다. 기본 버튼은 **OK**,학습 TYPE가 있는 별도 작업용 창은 **Sacrifice**이며 이때만 Cancel도 만든다. Back은 항상 만든다. 005acd24는 돌 버튼 생성의 unused 인자에 전달하며 마지막 문맥 인자는0이다. 기존 StoneButtonGump 헤더의 실제 다섯 인자 순서를 따른다. OK/Sacrifice 버튼을 글자에 맞춘 뒤 기본 폭을 double1.2배로 늘리는 저장 시점도 유지한다.

도움말 파일은 캐시가 null일 때만 Quick Help Spec을 읽어 VFS에서 로드한다. 성공은 원본 바이트 끝에 LF를 하나 더 쓰며 null/빈 성공 파일을 구별한다. 실패는 원본 등급9 진단을 남긴다. decompile의 길이0 표기와 달리 실제 파일 길이 출력 포인터를 읽어 끝에 LF/NUL을 쓰는 어셈블리/실행을 따랐다.

Attach는 기본 부착→그림이 있으면157×126 배경과153×122 그림→Back/OK 또는 Sacrifice/Cancel→큰 본문 배경/안쪽 배경→HTML 부모→정보 pane 순서다. 필수 작업 버튼의 기본 높이를 ftol로 절삭하고 선택 버튼들의 현재 최대 폭도 절삭해 통일한다. 중심 시작점은 **(창 폭 - 버튼 개수×공통 폭 - 버튼 개수×10)/2의 절삭값**이다. 간격을 (개수-1)로 바꾸면 원본보다5픽셀 어긋나므로 그대로 유지한다. 정보 버퍼의 완료 줄 수가 정확히0일 때만 visible을 끈다. 원본 x87의 현재 계산값으로 기본/배율 크기를 각각 저장하고 Push 뒤 현재 필드를 다시 읽는다.

Reset은 현재 그림→HTML 표시 자식→이전 정보 pane을 지연 삭제하고 새 다중 줄 정보 pane을 (0,0,font5,현재 색)으로 만든다. Show의 type0/빈 주제는 battle 우선 pyrosphereHelp,그다음 challengeHelp,그 외 introHelp를 고른다. type0/비빈 주제의 지역80바이트 한계를 유지한다. 기존 실제 파생 HTML이 같은 HTML 부모 배열/링크/정보 pane을 작성하고,infoMode00565ec4를 변환 뒤0으로 돌린다. 본문을 Config 치환한 뒤 비었을 때만 No help information available을 번역한다. 선택 TYPE의 **현재 helpFrame+120은 치환 뒤** 읽으며 -1/TYPE 없음/두 제외 TYPE와 실제 ImageGump(type,frame,8)를 구별한다. 치환 전 빌린 본문 배열 주소를 이후에도 유지한다.

원본의 picture 포인터가10000보다 큰지 보는 진단은 호스트에서 해당 토큰의 실제 저장소 존재 검사로 표현한다. 호스트 식별값을 PE 포인터 크기와 비교하지 않는다. 본문 NUL/8192범위·파일명256범위·new 실패는 명시 예외다. 원본이8192바이트 지역 치환 버퍼의 미사용 꼬리까지 복사하는 부분은 표시 C문자열/NUL까지 보존하며,미사용 스택 쓰레기 바이트를 새로 합성하지 않는다.

## 같은 HTML 부모와 빈 요청

`HtmlBodyGump`는 실제 `0046c9c0/ca50/ca70/cac0/cbc0/cc30`이다. 부모는 본문+a4/링크+20a4의 두8192바이트 영역을 소유하고 현재 다중 줄 자식+a0은 공통 Gump 수명에 맡긴다. 생성은 두 첫바이트만0으로 쓰고 나머지 기존 바이트를 보존하며 현재 화면의 signed 절반 크기를 쓴다. Parse도 두 첫바이트를 먼저0으로 쓰고,기존 출력이 없는 파서에만 두 영역/8192한도를 설치한 뒤 같은 실제 파서를 실행한다.

SetText는 이전 표시 자식을 Kill→새 다중 줄 생성→부착→원문 StyleText→scroll0→Slider1→scrollable1 순서다. 부모가 void가 아니면 Push 뒤 현재 x/y로 재부착한다. Attach는 기본 부착 뒤 현재 표시 자식을 Push하고 부모 폭-가로 여백/같은 높이로 맞춘다. 스타일/글꼴/버퍼/Slider 계산은 기존 처리기가 맡으며 별도 글자 렌더러를 만들지 않는다.

기존 `QuickHelpRequest`는 type0/빈 문자열까지 짧은 접미 범위 거부로 막아 기본 Help 요청이 실패했다. 실제 빈 요청의 **lookup0→원래 빈 주제 이력→전체 Show의 기본 주제→현재 위치 재부착**를 별도 원본3경로로 확인해 빈 입력만 허용했다. 빈 입력에서 원본의 지역 버퍼 앞 접미 비교를 호스트에서 재현하지 않고 정상 레지스터/스택 관측의 효과를 보존한다. 비빈 짧은 주제의 기존 범위 거부·80바이트 한계·기존985행/기대값은 유지한다. TYPE0 이름의0 반환은 새 원문에서 명시 lookup 경계이며 전체 타입 조회를 새로 실행했다고 주장하지 않는다.

## 제품 변경 전 원문과 검증 범위

첫 `decomp_quick_help_window.py`는 **84고유 입력·84정상 최상위 반환·162878명령**이다. 생성16(학습/캐시/스크립트/배율),배치48(그림/정보/선택 버튼/다른 크기·배율·줄0),HTML 표시4(기존 자식/void),본문 갱신16(type0/1·빈/지정 주제·그림 -1/3·기존 자식)을 교차했다. 모든 노드의 전체0xa0 기본 필드와 창0xd0,현재 캐시/표식·GUI/문자열 호출 순서를 읽었다.

추가 `decomp_quick_help_window_paths.py`는 **14입력·14정상 반환·90207명령**이다. 파일 실패1·battle/challenge 기본 주제와 빈 치환8·두 그림 제외2·실제 빈 요청3이다. 합계98입력·253085명령이다. 실제 창/HTML 부모·기존 파생/기본 HTML/링크·ftol/CRT/스택 probe를 실행했다. 기본/자식 GUI 생성·부착/Push/텍스트/스크롤/그림·번역/Config/파일/색·lookup0/이력은 명시 경계다. 원본 게임/OS/장치·실제 화면 실행이 아니다. 예외 규약·스택·보존 레지스터·x87 제어어를 확인했다. 기존 원문을 다시 만들거나 기대값을 바꾸지 않았다.

초기 부모 Oracle 구성 중 가상 Push 경계 목록 초기화와 원본의 순수 스택 probe 범위를 보완한 준비 실패는 원문 쓰기 전에 끝났으며 정상 반환으로 세지 않는다. 최초84 도구/fixture/JSON과 추가14는 각 제품/빈 요청 수정 전에 `extracted/quick-help-window-20261021-first-capture/`에 보존했다.

첫 fixture SHA-256은 `b7bf52caf2efcb58b27470bd89a9b1e67506a42961e45bb22928ed16985d2fe2`,추가14는 `97c519ae05783122fc023b1aec17f5059241b57751e50e8963af545a2855586d`다. 기존 요청985/파생 HTML86/제어150/지식25의 독립 원문은 변경하지 않았다.

새 C++검사 `QuickHelpWindow_Native98`은 전체98행을 실제 새 창/HTML 부모·기존 파생 HTML/링크/QuickHelpRequest와 같은 노드/배열/현재 표식으로 대조하도록 작성했다. GUI·Config/VFS/색/글자 측정은 원문과 같은 명시 경계다. 실제 공통 트리/픽셀/장치·Client 입력 검사가 아니며 **사용자 지시대로 컴파일만 하고 실행하지 않는다.** C++검사·CTest·전체 회귀·게임/창 실행과 자연 완주는 후속 통합 실제 플레이로 이월한다. 빌드/정적 감사·현재 핀 역변환은 `cpppj/quick-help-window-session.json`과 `tools/cpp_quick_help_window_audit.py --verify`에 기록한다.

첫/최종 전체 Release는 **경고0·오류0·종료0**,최종 로그 `extracted/cpp-quick-help-window-20261021-build2.log`다. 첫 원문/추가 경로·기존 요청985 보존 감사와 새 전체 창/기존 HTML·Help 제어·지식·미션 수명 정적 감사가 종료0이다. 현재 핀 **100보고서·110항목**의 전체 JSON을 시작 커밋으로 역변환해 과거 files/evidence/원문/기대값/실행·바이너리/기존 이력을 보존했다. 기존 RawTutorialLater·TYPE 요구 에너지 client/CMake 역사 핀 차이는 새 통과로 기록하지 않는다.

다음 최우선은 이 처리기들을 **실제 RawMenuControls/RawButtonControls의 공통 등록표·가상 Attach/입력/Draw/scalar·같은 StyleText/버퍼/장식/배경/ImageGump/돌 버튼에 조립**하고 Client Help02000017/themeHelp/F1을 활성화하는 작업이다. 기존 OpenQuickHelpWindow·DispatchQuickHelpWindow·QuickHelpHistory·같은 PopupState를 함께 사용하고 링크 라우터4a00c0/학습492a30의 실제 소비자도 연결해야 한다. 현재 사용자에게 전용 화면이 열리지는 않는다. 전체11미션 자연 완주와 전체 Init/State/공장·소멸·전략AI·지형/포획/희생/대피·외관/음향 동일성 검증도 남는다.
