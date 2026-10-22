# 2026-10-22 양 판본 정밀 디컴파일 및 cpppj Release 빌드

## 실행 이유와 환경

`AGENTS.md`와 두 인수인계 문서를 먼저 확인했다. 시작 시 HEAD는 `697d305b / 20261022-60 WB`이고 작업 트리는 깨끗했다. 현재 PC는 **HJOW-Athlon (192.168.0.94)** 이며, `LEFT_JOBS.md`에 기록된 마지막 전체 정밀 디컴파일 PC는 **VM-W11-Codex / 2026-10-22**였다. 호스트가 달라서 프로젝트 지침대로 양 판본을 다시 분석했다.

실제 게임은 실행하지 않았다. 정밀 디컴파일은 원본 실행 파일을 읽어 Ghidra 프로젝트와 `extracted/refined/` 산출물만 갱신한다. 원본 `originals/`, CD `originalCD/`, `AGENTS.md`는 변경하지 않았다.

## 정밀 디컴파일

실행 명령은 `powershell -ExecutionPolicy Bypass -File tools/ghidra/refine_all.ps1`이다. 앞선 실행에서 Python 콘솔 인코딩 문제가 있었으므로 `PYTHONUTF8=1`, `PYTHONIOENCODING=utf-8`, `PYTHONUNBUFFERED=1`을 설정했다. 전체 스크립트는 종료 코드 0으로 끝났다.

| 판본 | 최종 함수 수 | 디컴파일 성공 | 실패 | 최종 등급 A/B/C/D | 함수 포인터 표 |
| --- | ---: | ---: | ---: | --- | ---: |
| originals, 10.78 패치판 | 5,549 | 5,549 | 0 | 2,326 / 3,152 / 40 / 31 | 1,652 |
| originalCD, 10.72 CD판 | 5,823 | 5,822 | 1 | 2,315 / 2,939 / 484 / 85 | 1,121 |

판본 간 대응은 **2,496쌍**이며 신뢰 등급은 강 1,738, 중 707, 약 51이다. 최종 호출 규약 일치율은 97.2%, 반환 스택 크기 일치율은 97.9%다.

CD판의 디컴파일 실패 1개는 남아 있다. Ghidra 로그에는 두 판본의 GIF 리소스 데이터 오류, 여러 함수의 p-code 흐름 경고, Java `Unsafe` 폐기 예정 경고가 나타났다. 이 메시지에도 전체 headless 절차, 후처리, 힌트 적용, 판본 대응표와 등급 파일 생성은 종료 코드 0으로 완료됐다. 실패한 함수 하나가 복구됐다고 해석하지 않는다.

결과 파일은 Git 제외 경로 `extracted/refined/originals/`와 `extracted/refined/originalCD/` 아래의 `raw.c`, 최종 `Netstorm.c`/`NETSTORM.c`, `functions.tsv`, `grades.tsv`, `conventions.tsv`, `hints.tsv` 및 상위 `match.tsv`에 있다.

## cpppj Release 빌드

`cmake --build cpppj/build --config Release`를 실행했다. CMake는 `cpppj/src/CMakeLists.txt` 변경을 감지해 솔루션을 자동 재생성한 뒤 MSBuild 18.11.0으로 전체 Release 대상을 빌드했다. 명령은 **종료 코드 0**, 컴파일 경고 0, 오류 0으로 끝났다.

결과물은 `cpppj/build/bin/Release/NetstormCpp.exe`와 `cpppj/build/bin/Release/netstorm_tests.exe`다. 검사 코드는 컴파일했지만 CTest/검사는 실행하지 않았다. 원본 게임이나 클론 게임도 실행하지 않았다.
