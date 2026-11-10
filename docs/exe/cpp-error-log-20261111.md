# 오류/예외 공통 구조와 상세 로그 (7차 E1~E3, M7.5) — 작업1269

2026-11-11 / HJOW-Athlon. **오류·예외·비정상 종료 직전의 상세 정보를 `%USERPROFILE%\Saved Games\Netstorm\logs\yyyyMMddHHmmss.log`에 남기는 공통 구조를 구현했다.** 원본에는 대응 코드가 없는 새 장치이며 M7.5(공통 assertion 파일 중앙화, 초기 registry 진단 포함)도 같은 구조로 마감했다. 새 디컴파일은 없다(마지막 디컴파일 2026-11-03/HJOW-Athlon, 현재 같은 호스트). MP 실행은 0이다.

## 구조 (`platform/ErrorLog.h/.cpp`)

- **경로/이름:** `UserDataRoot::Logs()`(루트\logs, 소문자)와 `yyyyMMddHHmmss.log`(지역 시각). 읽기 전용 설치여도 설치 폴더에 쓰지 않는다. 사용자 폴더를 정하지 못하면 파일 없이 계속하며 설치 폴더로 되돌아가지 않는다.
- **지연 생성:** 정상 활동(`Note`, `kInfo`)은 최근 1024줄 메모리 기록에만 남는다. `kWarning` 이상이 처음 나올 때 폴더와 파일을 만들고 UTF-8 BOM·머리말(제품 버전·프로세스 ID·실행 파일·작업 폴더·명령줄·OS)·**이전 활동 기록**을 먼저 쓴 뒤 해당 줄을 기록한다. 이후에는 낮은 심각도도 이어 쓰고 `kError` 이상은 즉시 디스크에 반영한다. 같은 초에 다시 열어도 덮어쓰지 않고 이어 쓴다(`FILE_APPEND_DATA`).
- **표준 스트림 복제:** `std::clog`/`std::cerr`의 모든 줄을 원래 출력과 함께 활동 기록으로 복제한다(콘솔 출력은 그대로).
- **예외 풀이:** 형식 이름·`what()`·`std::nested_exception` 원인 사슬(깊이 8)·`std::string`/`const char*`/`int`/알 수 없는 예외를 구별한다.
- **비정상 종료 핸들러(`InstallCrashHandlers`):** `std::terminate`, 처리되지 않은 SEH(`SetUnhandledExceptionFilter`), 잘못된 CRT 인자, 순수 가상 호출, `abort()` 신호를 로그로 남긴다. SEH 기록에는 예외 코드 이름·주소(모듈+RVA)·스레드 ID·접근 종류/대상 주소·x64 레지스터·x64 언와인드 스택 추적(`모듈+0x오프셋`)이 들어간다. 파일 쓰기는 미리 복사한 경로·스택 버퍼·`WriteFile`을 사용하고(핸들러 안 힙 할당 최소화) 스택 오버플로를 위해 스레드 스택 128KB를 확보한다. 핸들러는 기록 후 기본 종료 동작으로 넘긴다.
- **assertion 중앙화(M7.5):** `Client`의 기본 AssertDisplay 장치(`ClientMain.cpp`)와 초기 registry `keyValue` 진단(Storage 이전)이 모두 `ErrorLog::RecordAssertion`을 거쳐 같은 파일에 `[WARN ] [assertion] 파일:줄 표현식`으로 남는다. 원문의 모달 assertion 창은 무인/MCP 실행을 멈추므로 기본은 표시하지 않으며, 환경 변수 `NETSTORM_ASSERT_WINDOW=1`일 때만 확인/취소 창을 띄운다(취소는 치명 기록 후 종료). 원문 4e0620의 `assert.html`/32비트 스택 주소 동일성은 복원하지 않았다(사용자 홈·x64 장치로 대체).
- **최상위 경계:** `main()`이 시작에서 구성하고, 모든 `std::exception`/그 외 예외를 `kFatal`로 기록한 뒤 GUI 시작 실패 안내와 stderr에 로그 경로를 덧붙인다. 창 프로시저의 예외는 Win32 경계를 넘기지 않고 기록한 뒤 보관했다가 메시지 펌프(`Run` 루프/`PumpMessages`)에서 같은 예외를 다시 던진다. 메모리 할당 실패 핸들러(원문 435f50)는 종료(_Exit) 전에 치명 기록을 남긴다.
- **개발 검사:** `Netstorm.exe --log-crash-test <throw|terminate|access|divzero|abort|stack|invalid|purecall>`가 의도적으로 비정상 종료한다(오류 보고 대화상자는 끈다).

## 검증

Release/x64 빌드 **341.9초/종료0**, 경고 1건(의도적 무한 재귀 함수의 C4717)은 `#pragma warning`으로 제거 후 증분 빌드 25.0초/경고·오류 0. `netstorm_error_log_tests` **14/14**(파일 이름 규칙, 소문자 logs, 정상 활동의 파일·폴더 비생성, 첫 경고의 파일·머리말·이전 활동 순서, 이후 이어쓰기, 예외 중첩 풀이, 비표준 예외, assertion, 한글 UTF-8, 링 1024 제한, 미구성 안전, 4스레드 400줄 무손상, 같은 초 이어쓰기, logs 밖 쓰기 없음). **실제 프로세스 시험**(`extracted/cpp-resume-1269/crash-test.py`, 임시 `NETSTORM_USER_DATA`): 8종 모두 비정상 종료 코드(throw 1, terminate/abort/invalid/purecall 3, access 0xC0000005, divzero 0xC0000094, stack 0xC00000FD)에서 로그 파일 정확히 1개·14자리 이름·기대 문구(ACCESS_VIOLATION 쓰기 0x0·레지스터·`Netstorm.exe+0x…` 스택 등) 확인, `--version` 정상 실행은 logs 폴더를 만들지 않았고 설치 폴더 목록도 불변이었다. 첫 시험의 `throw` 실패는 "자세한 로그" 안내를 로그 파일에서 찾은 시험 스크립트 오류(실제는 stderr/오류 창 문구)로 교정했다.
회귀: 변경 후 `netstorm_menu_session_tests`(6건)·`netstorm_main_popup_tests`(3건)·`netstorm_construction_owner_tests`(2건)의 실패 이름이 1268 직후와 완전히 같다. 최종 EXE의 정상 Tutorial4 진입→생산/건설 요청→600프레임 대기→종료 코드 0(게임 시각 13.5→23.9초)을 확인했고 이 실행도 로그 파일을 만들지 않았다. 포획/희생·특정 적 공격·승리·공중3종·MP/Join/Test Battle 실행은 0이다. 보호 원본/CD/fixture/AGENTS 2,808파일은 불변이다.

## 한계

- 던져진 예외는 스택이 풀린 뒤 잡으므로 **던진 위치의 스택 추적은 없다**(형식·메시지·중첩 원인·직전 활동 기록으로 추적). 처리되지 않은 SEH/terminate/abort는 스택 추적이 있다.
- 스택 추적은 모듈+RVA 형태이며 심볼 이름은 링커 맵/PDB로 별도 해석한다.
- ARM64 레지스터 덤프는 구현하지 않았다(주소·스택 추적은 공통). 실제 ARM 실기 확인은 하지 않았다.
- 배포 ZIP과 읽기 전용 설치의 실제 GUI 로그 생성은 이번에 확인하지 않았다(경로가 사용자 홈임은 단위 검사와 설치 폴더 불변으로 확인).
