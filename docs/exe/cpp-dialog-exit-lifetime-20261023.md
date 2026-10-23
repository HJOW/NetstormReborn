# 실제 대화 OnExit 수명과 해상도 종료 — 2026-10-23

기준은 보호10.78 PE와 `extracted/refined/originals/Netstorm.c`이며 현재/마지막 전체 정밀 디컴파일 PC는 VM-W11-Codex / 2026-10-23이다. 기존 산출물을 재사용하고 새 전체 디컴파일/보호 원문 수정은 하지 않았다. 직전 메뉴 단위369는 5adcf532 / 20261023-37 CP다.

## 원문과 실제 연결

`DialogExitCommand`는 004c83b0의 regex 캡처 이후 현재 명령 조회→code0 진단→첫 바이트 isdigit→atol 또는 즉시80칸 문자열 고리→부모+21e0/+21e4 저장 순서를 복원한다. 인자에 RawTextToken/Trim/따옴표 해제를 끼우지 않고80/256 물리버퍼의 정상 NUL 범위만 받는다. codeffffffff를 추가 거부하지 않는다. 소멸004cf1c0의 같은52f90(code,param,param) 예약 효과는 상태를 먼저 비운 뒤 한 번만 실행한다. 독립204입력의 두정밀도408회/실제5266842명령/정밀도 차이0과 제품 대조를 사용한다. regex엔진 전체나 HTML부모 소멸 전체가 새 모듈과 동일하다는 확대는 하지 않는다.

`DialogMetadataHooks.resolveOnExit`는 첫 HTML 효과 뒤 OnExit 종류의 raw 캡처를 즉시 해석한다. 기존 resolveAction/page.onExit의 Trim된 표현은 보존한다. 기존 Trim은 바깥 쌍따옴표도 제거하므로 legacy의99와 실제raw고리의"99"를 구별한다. 같은 종류가 여러 개면 마지막 값을 보유하고 숨긴 메타는 고리를 소비하지 않는다. 첫 HTML의 끝LF도 실제 regex캡처에서 남을 수 있다. `$OnExit`의 영문/숫자도 004c8d50 내용 관문에 포함되므로 SetSize2는 hasContent=true다. 내용없는 새HTML부모는 OnExit0이며 기존 부모를 root replace로 삭제하지 않는다.

UberGump의 페이지 교체/Blank/월드 반납은 현재 해석값을 지연 소멸 표로 이동한다. Resize/Compose는 페이지를 소멸시키지 않는다. Client 정상 루프의 SaveOptions·미니맵 갱신 뒤, Kernel 앞 CleanupDialogs에서 기존 InputQueue에 예약한다. 따라서 MenuJustClosed를 소비하는 같은 입력 드레인에서 QuitApp이 바로 실행되지 않는다.

원문 정상 순서는 입력4d62b0→SaveOptions441de0→…→미니맵481320→GumpCleanup4662b0→GumpIdle464170→…→Kernel471a30이다(C47219~47244). Quit 반환과 WM_CLOSE/WM_DESTROY/WM_QUIT에는 별도 설정 저장이 없다. 즉시OnExit를 실행하면 SetSize2의 SCREENW/H 저장 전에 종료할 수 있다. 54dc58은 Moviegump 갱신 잠금이며 GameClock pause55b4b0과 구별한다. 현재 이 단위는 논리 페이지의 소멸 위치만 연결하며 전체 공통Gump/영상 잠금 통합은 후속이다.

해석 시점의 현재 Tell/Config/Warning/Guide/Knowledge/GameMenu 명령표와 원본 좁은바이트 경계를 사용한다. 실제 사용자 QuitApp 요청만 읽는 QuitRequested가 CLI 검사 프레임 제한과 구별되어, 사용자 종료 뒤 의도한 늦은 검사 단계가 남아도 정상 종료를 관측할 수 있다. 임의 창 닫기/프레임 제한/오류의 미완료 진단은 유지한다.

## 검증

빌드1은 종료0/경고·오류0이며 새 검사의 legacy 따옴표 기대3을 발견하여 제품 파서/원본 fixture는 바꾸지 않고 기대를 정정했다. 등록되지 않은 DialogScript 필터 요청1도 실제 Dialog 필터로 바로잡았다. 최종 Release 빌드2는 종료0/경고·오류0, Dialog27/ScriptCommand3/GameMenu7/MissionControl2의 관련39검사 실패check0이다. 최종 C++/검사 소스는 이후 동결했다.

실제 GUI2의 probe·No·Yes가 모두 종료0/시간제한 미도달/전체 originals 해시 동일로 통과했다. No는800프레임 관측까지1024×768/미션/비정지를 유지하고1000프레임 제한까지 실행했다. Yes는선택전65프레임 확인 뒤70프레임 실제Yes를 눌러800프레임 보고/캡처 없이 정상 종료했고, 허용 설정을 복원하기 전에 실제 암호화options.cfg를 읽어800×600 저장을 확인했다. 다른 설정값은 기록하지 않았다.

GUI1도 같은 제품의 No·Yes·저장까지 관측했지만, 프레임 제한 때만 생성되는 오디오 보고서를 조기Quit에서도 요구한 도구 기대 때문에 실패했다. 제품 변경 없이 Yes의 음향보고서 부재만 명시 미관측으로 바꾸고 재실행했다. probe/No는 실제 장치준비/음소거깊이1을 계속 필수 대조한다. 종료와 저장의 필수 주장은 완화하지 않았다.

최종 전체 CTest1은 **2408검사·실패check0·종료0/189.91초**이며 내부 LastTest.log의2408/0도 확인했다. 최종 빌드/검사 뒤 제품·검사 C++는 변경하지 않았다. 원문 프레임 차이의 후속 조사/구체안은 [Gump 프레임 순서](cpp-gump-frame-order-20261023.md)에 별도로 남겼다.

## 잔여

전체 HTML부모의 공통Gump/지연 Kill·완전한 OnExit 명령표, Moviegump update pause, 기존 공통UI Cleanup/Idle 프레임 위치와 HUD dock/Chat/NSInfo 연결은 후속이다. Early Missions·전체캠페인·데모3·Edit/비Multiplayer 메뉴의 사용자 완주와 원본 전체 동일성 목표는 계속 활성이다.

주요자료는 `extracted/cpp-dialog-exit-20261023-{build1.log,build2.log,ctest1.log,LastTest.log,scope.json}`과 `gui1/gui2`다. 제품 SHA256은 `0456a0280eb7e877869ede7fc36cefe305ee81cd553fbfc6721b5ce8f8681082`, 검사 SHA256은 `10a1694ceb477fdd346b93d4b0369ccd5c67ff03c583739265439426cd45701f`다. 모든 실제GUI 실행 뒤 전체 originals 해시/허용 설정 복원을 확인했으며, 원본 게임 자체나 원격push는 실행하지 않았다.
