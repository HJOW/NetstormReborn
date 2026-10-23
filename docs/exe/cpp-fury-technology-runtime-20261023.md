# Fury War 원소 기술과 HTML 부모 반납 — 단위376

현재/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**이다. 작업 전에 AGENTS.md·LEFT_JOBS.md·LEFT_JOBS.dotnetpj.md를 재확인했다. 새 전체 디컴파일·기존 정밀 소스 변경은 없다. 단위374의 Fury War `bc8fury76`만 원소 선택창 `@1`에 남던 진입 실패를 대상으로 한다.

## 변경 근거와 연결

원본 `00492ae0`은 타입0부터 현재 타입 개수를 매 반복 재조회한다. group10·theme5를 제외하고 선택 theme인 기록을 찾으면 `CurrentTechnology`→`FindContained(mask4,type,0)`를 호출한다. 없는 항목만 현재 기술 부모를 다시 조회한 뒤 `CreateContained(parent,type,0,4,0x10,0)`로 만든다. 기존 `Ensure(owner,type)`는 타입 assertion·명시 owner getter의 계약이 달라 사용하지 않았다. 새 `RawThemeTechnology`와 `ApplyThemeTechnology`는 기존 세션의 같은 Player/TYPE/사제 Carrier finder/contained 생성자를 연결한다. 보호 명령 `00542798=02000028`의 실제 첫 인자를 사용하고 release는 기존 일반 입력처럼 거부한다. 기술 함수에는 안내창 닫기 효과가 없다.

원본 `004c79b0`은 버튼 명령을 같은40칸 큐에 예약한 뒤 부모를 Kill한다. 일반/Instant 모두 동일하며 차이는 자식의 release/press 발화 시점이다. 새 `FireDialogCommand`를 논리 HTML 버튼·timeout·부모 Enter 사건에 연결했다. 메타를 읽을 때 고정한 정수 command/parameter를 선택 시 다시 이름 조회하거나 문자열 고리에 넣지 않는다. 현재 자식 돌 버튼을 실제 Kill하고 serial/입력 영역을 제거해 같은 드레인의 후속 사건도 다시 선택하지 못한다. 소멸/OnExit는 후반 Cleanup에 남는다.

`004cf120/004cf1c0`의 창 개수는 Kill 직후에도 소멸 전까지 유지된다. 따라서 OnExit0인 부모도 retired 표에 보관한다. 명령 전용 Tell은 임시 새 부모만 반납하고 기존 표시 부모를 닫지 않는다. 섹션 존재와 실제 창 생성 결과를 분리해 무본문 Tell을 Guide 자기 정지 성공으로 간주하지 않는다. 실제 자기 정지는 기존7프레임 게이트가 반납한다.

독립 원문 범위는 [기술1200입력](cpp-theme-technology-20261023.md), [HTML258입력/두 정밀도516회](cpp-dialog-command-lifetime-20261023.md)에 기록했다. 네 실제 UI 검사의 원문 기대와 범위는 [검증 도구 문서](cpp-fury-technology-smoke-20261023.md)를 따른다.

## 검증 기록

Release 빌드1·최종 빌드2는 종료0/관측 컴파일 경고·오류0이다. 최종 제품 SHA256은 `d53095e8f6f3cea34b9f086023d89dc06d046053f917a859185e8131e8d111bd`다. 빌드 원시는 `extracted/cpp-fury-technology-20261023-build1.log`, `...-build2.log`에 보존한다. 관련 **58검사/실패check0**(ThemeTechnology4/Dialog34/RawButtonControls7/KnowledgeMenu2/MissionGuide7/GuideTransitions2/MissionControl2)이 통과했다. 신규 기술 fixture1200입력과 HTML fixture258입력의 원본/SHA 확인 도구도 종료0이다.

실제 `...-final`의 Wind·Thunder·Sun과 `...-rain-retry`의 Rain은 각18조건을 통과했다. 네 최종 기술 수는 초기 outpost/windBattery 포함7·9·8·9이며 기존 SID164/165를 보존한다. 선택 직후90프레임은 아직 자기 정지 중이고108·133프레임에 해제/게임 시각 증가를 확인했다. 실제 초기/선택/최종 표시를 확인했으며 원본 픽셀 대조로 확대하지 않는다. 두 묶음 모두 originals1394파일 전후SHA가 같다.

`...-dialog-exit`은 실제 해상도 확인 No의800프레임 미션 유지와 Yes→SetSize2→OnExit QuitApp99의 정상 조기 종료/800프레임 미도달을 통과했다. Yes의800×600 실제 저장값을 복원 전에 확인했다. `...-game-menu-retry`는1024×768/1280×720의 실제 ESC/Game/View·목표 재열기·Leave 취소·재시작을,1280 확장의 음량/해상도No/QWASZX·바깥 월드 선택을 통과했다. 각 실행의 전체 원본SHA가 같다.

최종 같은 제품으로 `...-solo-all`의 Early6/훈련9/캠페인16 **31/31**, `...-demos`의 실제 메인 메뉴 Demo3 **3/3**이 모두 종료0/상한 미도달/Guide4/비정지·실제월드/로딩반납을 통과했다. Demo는 원본15초 안내의 자연 timeout·demo1·로컬 AI도 확인했다. 총34경로 진입 관측이며 사용자 완주/승패 증명은 아니다. 두 도구의 전체 원본SHA 동일을 확인했고, 마지막에는 최초 Fury 이전의1394파일 전체 목록과 현재 목록을 다시 비교해 같음을 확인했다.

전체 `ctest --test-dir cpppj/build -C Release --output-on-failure`는 종료0/323.07초, 내부 **2433 test(s), 0 failed check(s)**다. `...-ctest.log`와 `...-ctest-internal.log`에 보존한다. 이후 제품/검사 C++를 수정하지 않았다. 독립 검토자가 네 테마와 전체34경로의 원시 TSV·실행 SHA·화면·기술 불변을 별도로 확인했다. 엄격UTF-8·Python AST·새 문서 링크·공백·보호 경로·실행 프로세스 정리도 점검한다.

## 보존한 실패와 설정 복원

최초 Fury Rain은0.897초에 `Unable to open d/options.cfg`로 종료1이며 게임 상태/화면에 도달하지 못했다. 별도 Rain 재실행은 성공했지만 첫 실패의 원인은 미확정이다. 최초 `...-game-menu` probe는 게임 자체 종료0 뒤 Python의 허용 설정 복원 `Path.write_bytes`가 `OSError(22, Invalid argument)`로 실패했다. 이때 차이는 허용 options.cfg 하나뿐이었다. git HEAD의 해당 파일 SHA가 실행 전SHA `7c986d379a533456fb34d19f7937d1a028812c4560389d43cb663f1bd10cd7d4`와 같음을 확인한 뒤 정확한 바이트를 복구했고 전체1394파일의 동일성을 확인했다. `...-game-menu/recovery.json`에 남겼으며 최초 실패 결과는 덮어쓰지 않았다.

검사 보호 도구에 선택적 영구 원시 백업/원래 부재 표식, 복원 실패의 진단과5회 제한 재시도, 한 파일 실패 뒤에도 다른 허용 설정 복원, 최종 실패 전파를 추가했다. 새 Fury/메뉴/전체 진입 검사는 출력 폴더의 settings-backup에 실행 전 바이트를 남긴다. 실제 원본을 사용하지 않는 실패 주입2검사로 일시 실패 복구·원래 부재 복구·최종 실패 전파/다른 파일 복구/백업 보존을 통과했다. 이 보완은 제품의 간헐적인 최초 SaveOptions 실패 원인을 해결했다는 뜻이 아니다.

## 남은 범위

일반 HTML 부모는 아직 전체 공통 Gump 객체로 이행하지 않은 논리 어댑터다. HTML 메뉴 행의 실제 MenuGump ID 첫 인자·입력 트리·물리 Enter 변환 경로는 이번 버튼/timeout의 독립 원문 검증과 구별한다. [다음 일반 메뉴 조각](cpp-tell-menu-identity-next-20261023.md)은 실제 등록 부모·열별 메뉴·즉시 MenuJustClosed Bubble과 첫 Compose 소유권 분리를 요구한다. [Edit 잔여 감사](cpp-editor-gap-audit-20261023.md)는 Temp/Objects 전체 저장·generic Editor 설치·실제 작업본 저장 연결을 다음 기반으로 정리했다. 사제 희생 뒤 기술 지급 본문·전체 사용자 완주/승패·HUD dock·Edit/custom map/scenario·배포와 전체 원본 동일성은 후속 목표다. 네 원소의 진입 성공은 전체 게임 완료를 뜻하지 않는다.
