# webpj가 요청한 cpppj 작업 처리 (2026-11-09)

2026-11-09, HJOW-Athlon. 사용자 지시("LEFT_JOBS.webpj.md를 읽고 webpj 작업을 위해 cpppj 쪽 작업이 필요한 것을 지금 진행", CP 세션 일시 중지 상태)에 따라, webpj 인계와 단위 문서에 "cpppj 몫"으로 남아 있던 항목을 모아 처리했다. **cpppj·dotnetpj의 기존 작업을 역행하지 않도록** 원본 근거가 확인된 차이만 고치고, 나머지는 판정만 기록했다.

- 기준 커밋: `12c664d2`(20261109-119 CP, 제품 1226). 작업 시작 시 작업 트리 깨끗함.
- 디컴파일: 새로 수행하지 않았다. 기존 산출물 `extracted/refined/originals/Netstorm.c`·`extracted/refined/originalCD/NETSTORM.c`(마지막 디컴파일 HJOW-Athlon 2026-11-03, 현재 PC와 같음)를 읽기만 했다.
- 보호 원본/CD·AGENTS.md는 수정하지 않았다. dotnetpj 파일은 수정하지 않았다(4절의 재반영 대상만 기록).

## 1. 처리 목록

| 출처 | 항목 | 판정 | 처리 |
| --- | --- | --- | --- |
| webpj 3.4절(WB04-E207) | `DudeNoticeTests` 원본 재생 2건 실패 | 시험 장면 결함 | 종속 표식(Burning 14·경로 24)에 판본별 실제 ProcessForm 생성자 VT(0x512098/0x505990) 부여 |
| webpj 4절(WB04-D5c) | `process_attach_guards` 실패 | 시험 기대가 원문 순서와 다름 | 원문 `0041bd20` 순서(타입 진단→부모 상태→Kernel 등록→Form)대로 전송 flags 거부의 부분 효과(등록1·회전8)를 단언 |
| webpj 4절(WB04-D5c) | `ProcessForm_RejectsInvalidNamedFormsBeforeRegistration` 실패 | 시험 기대가 원문 순서와 다름 | `…AfterRegistration`으로 이름을 바꾸고 원문 부분 효과(등록·패치 회전), 예약 머리 허용을 단언 |
| webpj 4절(WB04-D5c) | `rawProcessRemoval_native_mixed_form_filter` 실패 | 시험 입력 결함(raw 노드 VT 0) | raw 노드에 실제 ContentForm VT / 10.78 지원 자산 VT(0x00501dd0) 부여 |
| webpj 4절(WB04-D5b) | `CursorPreviewDword_ContentComparisonNative56KeepsAllBits` | CP가 `cfbd6420`에서 이미 수정 | 추가 수정 없음. 새 빌드로 통과 확인(2절) |
| webpj 오라클 문서 5.1 ③·4절 | General Help 스크롤 화살표가 붉은 갈색 | **cpppj 제품 결함** | 3.1절. 원문대로 현재 팔레트의 검정 색 번호를 화살표 색으로 사용 |
| webpj 오라클 문서 5.1 ④ | Demo 버튼 Tell 이름 `DEMOVILLE` | **cpppj 제품 결함** | 3.2절. 두 판본 원문과 같은 `Demoville` |
| webpj 오라클 문서 5.1 ② | Options 펼침 메뉴에 `Pause - Shift-F9` 없음 | 상태 차이(코드는 원문과 같음) | 수정 없음. 3.3절 |
| webpj 메인 메뉴 문서 | 돌 버튼 질감 프레임 0 | CP가 이미 수정(A01·프레임1) | 수정 없음 |
| webpj Tell 대화상자 문서 | Tell 본문 글꼴 슬롯 5·설명 색 | 기준선 cpppj가 이미 원본과 같음 | 수정 없음(M1 오라클 Demo·Credits·Campaign 차이가 구름뿐) |
| webpj 3.5절 1번·dotnetpj 8절 요청 1 | 실제 Session 상태 덤프 명령 | 새 검사 기능 | ui-script `session-dump` 단계([형식](cpp-session-state-dump-20261109.md)) |
| webpj 5절 L3·dotnetpj 8절 요청 2 | 같은 입력으로 같은 상태가 나오는 실행 | 새 검사 기능(덤프 확인 중 필요성 확인) | `--fixed-frame-ms N` 고정 시각 실행. 기본 OS 시계 두 실행은 게임 시각부터 달라 덤프 비교가 불가능했다([형식 문서 3절](cpp-session-state-dump-20261109.md)) |

## 2. 시험 결함 수정과 검증

- 빌드: Release/x64 `NetstormCpp`·`netstorm_pop_context_tests`·`netstorm_main_popup_tests` 4병렬, 최종 종료 0·경고 0. 통합 `netstorm_tests`(단일 exe)는 빌드하지 않았다(새 시험 파일 2개는 등록만 했다).
- 고친 시험: `DudeNotice_PatchNativeTaskAndRetreat`·`DudeNotice_CdNativeTaskAndRetreat`·`process_attach_guards`·`ProcessForm_RejectsInvalidNamedFormsAfterRegistration`(옛 이름 …BeforeRegistration)·`rawProcessRemoval_native_mixed_form_filter` 모두 통과. `CursorPreviewDword_ContentComparisonNative56KeepsAllBits`도 통과(`cfbd6420`의 CP 수정 확인).
- 시험 기대값은 약화하지 않았다. DudeNotice·혼합 목록은 기대값(원본 fixture)을 그대로 두고 입력 장면의 가상 표만 실제 생성자 값으로 채웠고, 부착 두 시험은 원문 `0041bd20`(10.78 `Netstorm.c` 22002행 부근)의 순서에서 나오는 부분 효과를 그대로 단언한다.
- **회귀 비교(같은 PC, 같은 실행 방식):** 제 변경을 잠시 stash한 기준 빌드와 비교했다.

| 실행 파일 | 기준(HEAD `12c664d2`) | 이번 변경 |
| --- | --- | --- |
| `netstorm_main_popup_tests` | 167건 중 3건 실패(검사 4) | 167건 중 같은 3건 실패(검사 4) |
| `netstorm_pop_context_tests` | 1165건 중 28건 실패(검사 53) | 1169건(새 4건) 중 23건 실패(검사 43) — 차이는 고친 5건뿐, 새 실패 0 |

남은 실패(`CampaignSaveGoMain_ActualManagerGateAndSelectedBattleStop`·`CampaignNativeFortContinuation_*` 2건, pop_context의 "현재 Squid Pop의 실제 전송 미연결"·"Factory 진단 장치 미연결"·`ConstructionData_*`·`RawPriestWorldBindings_*` 등 23건)은 기준에도 똑같이 있는 기존 실패이며 이번 작업 범위 밖이다(CP M6 장기 연결 진행 중 항목).

고정 시각 모듈(`ClientTimeSource`) 추가 뒤 최종 빌드(경고 0)로 다시 돌린 결과도 같다: `netstorm_pop_context_tests` 1171건(새 `SessionStateDump` 4·`ClientTimeSource` 2 포함) 중 실패 23건으로 기준 실패 목록과 비교해 새 실패 0, `netstorm_main_popup_tests` 167건 중 같은 3건. 덤프 형식 변경이 없는 webpj `port-map`·`fixture-manifest` 검사 8건도 통과했다.

## 3. 제품 수정

### 3.1 스크롤 화살표 색

- 원문: `ScrollingText` 생성(149767행)이 `FUN_004a64b0(slider,9,100,DAT_005acd24,DAT_005a4058)`을 부른다. Slider 생성자는 화살표 4개를 `FUN_004a63b0(…,방향,param_4,param_5)`로 만들고, 화살표 생성자는 `param_6`(=`DAT_005a4058`)을 돌 버튼 TextContext[2]에 넣어 삼각형 색으로 쓴다. 색 표 초기화(130623·130642행)에서 `DAT_005a4058=FUN_004a2820(0,0,0)`(검정 근사 색 번호), `DAT_005acd24=FUN_004a2820(0xb5,0x8c,0x6f)`(갈색)이다. TextEntry의 Slider(66438·66448행)도 같은 두 색을 넘긴다.
- cpppj: TextEntry 경로는 `brown/black`을 넘겼지만 `RawMenuControls`의 ScrollingText 상태(`titleState.sliderType/sliderFrame`)는 fortGump TYPE과 프레임 1을 넘겨, 삼각형이 팔레트 1번(99,38,22)으로 그려졌다.
- 수정: `RawMenuControls::SetPalette`가 현재 팔레트 색 표의 갈색(named 9 = 005acd24)과 검정(basic 0 = 005a4058)을 채운다. `UberGump`는 메뉴 생성 직후 `SetPalette`를 부르고 매 Draw도 같은 팔레트로 다시 부르므로 ScrollingText 생성 전에 값이 정해진다. 손잡이 보조 칸(detailType/detailFrame)은 Draw에서 쓰지 않으며 돌 버튼의 다섯째 인자는 원문에서도 미사용이다. 헤더 주석도 "TYPE/프레임"이 아니라 색 번호임을 적었다.

### 3.2 Demo 버튼 Tell 이름

- 원문: 10.78 메인 메뉴 생성자(164564행)와 10.72(179341행) 모두 `"Demoville"`을 넘긴다. `DEMOVILLE` 문자열은 두 실행 파일에 없다(webpj 오라클 문서 5.1 ④의 읽기 전용 검색과 같은 결과).
- 수정: `UberGump.cpp`의 Demo 버튼 명령을 `{"Tell","Demoville"}`로 바꿨다. Tell 섹션 조회는 대소문자를 가리지 않으므로 화면은 같고, 기록 이름(`global.priorTell` 등)이 원본과 같아진다. `tell.english`의 섹션 머리 `[DEMOVILLE]`은 자료이므로 그대로다.

### 3.3 Options의 Pause 행(수정 없음)

원문 Options 구성(160842행 부근)은 `(DAT_00594fa4 != 0) || (DAT_005c85a4 != 0)`일 때만 `Pause - ~iShift-F9~.` 행을 넣고, cpppj `RawGameMenus`도 같은 조건이다. `DAT_00594fa4`는 State 전환(`004b2df0`)에서 `DAT_00594fa0`으로 설정되므로, 원본 스크린샷은 미션을 거친 뒤 메인 메뉴로 돌아온 상태에서 찍힌 것으로 판단한다. 최초 실행 메인 메뉴에서 행이 없는 cpppj는 원문 코드와 같다. `Language >`·`Anti-Aliasing`·`FSR 1.0 Upscaling`은 6·7차의 의도된 추가다.

## 4. 다른 프로젝트 영향

- **webpj:** 웹은 이미 원본 쪽(검정 화살표·`Demoville`·원문 부착 순서·실제 VT 장면)을 따르므로 코드 변경이 없다. 저장된 M1 오라클(`extracted/webpj-oracle/1f00a25a`, 기준선 빌드)은 바뀌지 않으므로 `m1-scenarios.json`의 기록 차이(③ 화살표 15+15픽셀, ④ `page` 이름)는 기준선을 올려 오라클을 다시 모을 때 0이 된다.
- **dotnetpj(재반영 대상, 이 작업에서 수정하지 않음):** `HostMainMenu.cs`가 `("Tell","DEMOVILLE")`을, `HostHtmlText.cs`가 `SliderType=menu.BackgroundType, SliderFrame=1`을 cpppj에서 그대로 옮겼고 `HostStartupSceneTests`가 `DEMOVILLE`을 단언한다. cpppj가 원문 기준으로 바뀌었으므로 다음 DN 단위에서 같은 근거로 재반영한다.
- **cpppj:** 시험 4건은 입력 장면과 원문 순서 기대값만 바꿨고 제품 소스는 바꾸지 않았다. 제품 동작 변경은 3.1·3.2의 두 가지뿐이다. 검사 기능(`session-dump`·`--fixed-frame-ms`)은 지정할 때만 동작하고, 시각 읽기를 모은 `ClientTimeSource`는 기본 모드에서 이전과 같은 `timeGetTime()`/`std::time()`을 읽는다(메인 루프 바쁜 대기 조건에 고정 모드 검사만 더했다). `UberGump.h`에는 공개 읽기 메서드 하나(`SessionStateDump`)만 더했다.

## 5. 실제 실행 확인 (HJOW-Athlon, AGENTS.md 예외 호스트)

빈 사용자 루트(`NETSTORM_USER_DATA`)·`assets/game-data`·1024×768·`--mute-audio`로 실행했다. 자연 완주·전투·포획/희생 검사는 하지 않았다.

| 확인 | 결과 |
| --- | --- |
| General Help 스크롤 화살표(오라클 기록 영역 `[715,61,721,67)`·`[715,341,721,347)`) | 두 영역 모두 삼각형 15픽셀이 (0,0,0) 검정, 붉은 갈색(99,38,22) 0픽셀 |
| Demo 버튼 뒤 보고서 `page` | `Demoville` |
| 메인 메뉴 `session-dump` | `world 0` 머리·난수·Kernel 0·`end` |
| 1-1 `thewarbegins` 미션 덤프(프레임 90·240) | 슬롯 1941/1943·객체 1881·프로세스 35/34·Player 9·Graph 251·TotalMade 16행·`end` |
| 같은 단계표 두 실행 비교 | 기본 시계는 다수 행이 다름, `--fixed-frame-ms 25`는 `sp` 소유자 0(미초기화 힙)만 다름 |
