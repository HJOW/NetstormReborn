# Windows 실행 파일 이름과 배포 경로 — 단위580

cpppj의 Windows 산출물 이름을 `Netstorm.exe`로 변경했다. CMake 대상 `NetstormCpp`와 정적 라이브러리 `netstorm`은 유지하므로 기존 `--target NetstormCpp` 명령을 계속 쓴다. `OUTPUT_NAME Netstorm`으로 실행 파일만 바꾸며 `bin/Release/Netstorm.exe`와 `lib/Release/netstorm.lib`는 서로 다른 폴더에 생성된다. 현재 Release 구성은 별도 실행 파일 PDB를 생성하지 않는다.

현재 PC와 마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**으로 일치한다. 이번에는 디컴파일 산출물을 수정하지 않았다. 사용자 재개 요청으로 앞선579 뒤의 일시 중지를 해제하고 전체 목표를 이어간다.

## 반영 범위

- CMake 출력 이름과 명령행 도움말의 프로그램 이름을 변경했다.
- 배포 스크립트의 기본 파일, ZIP 항목, 두 런처, 안내문과 BUILD_INFO를 변경했다.
- `tools/`에서 이름을 직접 참조하는 도구와 `dotnetpj/tools/export_cpp_world.py`의 기본 오라클 경로를 변경했다. 변경된 Python 파일은 합계96개이며 개별 게임·공략·MP 검사는 실행하지 않았다.
- 국·영 README, cpppj README와 빌드 안내의 사용 명령을 변경했다. 과거 보고서와 `hist/`·`extracted/`의 원시 실행 기록은 당시 파일 이름으로 보존했다.

압축파일 최상위 `Netstorm.exe`는 클론이다. `game/Netstorm.exe`는 아이콘·커서·로딩 그림을 읽는 원본 자료이며 실행하지 않는다. 두 파일의 ZIP 경로와 해시를 각각 대조했다. 원본과 클론은 프로세스 이름이 같으므로 실행·종료 도구는 전체 경로와 자신이 생성한 PID로 구별한다.

## 검증

기본 배포 스크립트를 `-NoBuild` 없이 실행했다. CMake 재구성, `NetstormCpp` Release 증분 빌드와 ZIP 생성이 종료0이었고 빌드 경고·오류는0이다. 실제 EXE와 정적 라이브러리는 별도 폴더에 생성됐다. ZIP은1365항목, 런타임 DLL3개를 포함하며 사용자 설정·진행 파일은 제외했다. 압축 안의 클론 해시는 빌드 산출물과, 원본 자료 해시는 `originals/Netstorm.exe`와 일치했다.

**581 후속 정정:**580은 물리 산출물과 메뉴를 확인했지만 실행 파일의 **import library** 경로가 정적 `netstorm.lib`와 겹치는 것은 검출하지 못했다. 새 게임 코드를 반영하는581에서 MSBuild가 정적 라이브러리를 링커 입력 추적에서 제외해 기존 EXE를 유지하는 현상을 확인했다. `ARCHIVE_OUTPUT_NAME NetstormCpp`로 import library를 분리한 뒤 실제 입력 추적과 라이브러리 수정 시각 변경→게임 재링크를 검증했다. 현재 수정과 최종 배포본은 [581 문서](cpp-chat-local-submit-20261030.md)를 따른다.580의 최초 로그/ZIP/해시는 당시 증거로 보존한다.

변경된 Python96파일은 AST 구문 검사를 통과했다. 압축 해제본의 `--help`는 종료0이고 새 이름을 표시한다. 두 런처의 실행 문자열과 BUILD_INFO의 파일 이름도 검사했다.

첫 실제 런처 실행은 정상 종료했지만 새 설치 안내의 **Main Menu** 선택을 자동 입력에서 누락해 메뉴 도달 단언에 실패했다. 환영 안내 캡처와 최초 스크립트·로그는 보존했다. 기대값을 약화하지 않고 새 압축 해제 경로에서 해당 버튼 입력을 추가했다. 두 번째 실제 런처 실행은 **4.092초 / 종료0**, 최종 phase0과 Campaign 메뉴를 확인했다. 메뉴 캡처도 보존했다. 게임·원본 동작 수정은 없고 이 입력 교정은 검사 드라이버에만 해당한다.

제품 SHA-256은 `511baeb9adf8efe67ed62461ac2e8c952f4c3b8d14ee3964c7acfee4fd456a2f`, ZIP은 `ef90ad4adaa91bfea664aa22890cf41b0129c1762e05dff59a8876ff97f52c4b`다. 등록 C++ 검사는3454개로 유지하며 이번에 전체 C++·34경로·자연 완주·Windows11 실기·신규MP 검사는 하지 않았다. 포획/희생/특정 공격/승리 재현과 공중3종 전용 검사도 실행하지 않았다.

원시는 `extracted/cpp-executable-name-580-20261030/`, 집계는 [executable-name-runtime-20261030.json](../../cpppj/executable-name-runtime-20261030.json)에 있다. 원본/CD/정밀 자료/AGENTS.md의1861파일 해시를 작업 전후 비교한다. 활성 빌드 폴더에 남은 예전 `NetstormCpp.exe`는 새 배포에 포함하지 않는다.

## 다음 작업

실행 파일 이름 요구는 cpppj에서 완료했다. dotnetpj의 이름 변경과 전체 복원은 별도 목표 상태를 유지한다. 다음은579의 실제 Chat 제출·로컬 명령 callback·서식/송신 연결과 기존 UI/Edit·시나리오·수명·호환성 잔여다. 싱글플레이 완료 전 MP를 먼저 확장하지 않으며 후순위MP의 무시험 사전 구현 방침을 유지한다.
