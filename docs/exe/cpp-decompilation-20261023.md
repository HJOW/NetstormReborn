# 2026-10-23 양 판본 정밀 디컴파일 및 cpppj Release 컴파일

## 같은 날 재실행: HJOW-Athlon (2026-10-23)

사용자가 AGENTS.md와 인수인계를 먼저 읽고 디컴파일 및 컴파일을 요청했다. 이 보완 기록은 아래 선행 실행 이후 다른 PC에서 동일 작업을 다시 수행한 최신 결과다. 시작 HEAD는 `e5a68d59`이고 작업 트리는 깨끗했다. 현재 호스트 **HJOW-Athlon (192.168.0.94)** 은 인수인계에 지정된 실행 예외 시스템이며, 당시 마지막 정밀 디컴파일 PC인 **VM-W11-Codex** 와 달라 두 판본을 전부 다시 분석했다.

`tools/ghidra/refine_all.ps1` 은 Ghidra headless로 원본 바이너리를 읽어 정적 분석한다. 실제 게임, 클론 게임, 별도 GUI 창은 실행하지 않았다.

| 판본 | 최종 함수 수 | 디컴파일 성공 | 실패 | 등급 A/B/C/D | 함수 포인터 표 |
| --- | ---: | ---: | ---: | --- | ---: |
| originals, 10.78 패치판 | 5,549 | 5,549 | 0 | 2,326 / 3,152 / 40 / 31 | 1,652 |
| originalCD, 10.72 CD판 | 5,823 | 5,822 | 1 | 2,315 / 2,939 / 484 / 85 | 1,121 |

함수 대응은 **2,496쌍**(강 1,738 / 중 707 / 약 51), 호출 규약 일치율 **97.2%**, 반환 스택 크기 일치율 **97.9%**다. CD판 실패 함수 1개는 미해결로 남겼다. 이전 결과 41개 파일을 실행 전 백업했고, 비교 시 텍스트 산출물은 모두 동일했다. Ghidra 내부 데이터베이스 인덱스 4개가 바뀌고 임시 캐시 파일 6개가 재생성되지 않은 차이는 정적 분석 데이터베이스 내부에 한정된다.

### cpppj Release 빌드

`cmake --build cpppj/build --config Release` 를 실행했다. CMake가 변경된 대상에 맞춰 재구성한 뒤 MSBuild **18.11.0**이 전체 Release 실행 파일 및 검사 실행 파일을 빌드했다. 종료 코드는 **0**이며 관측된 빌드 경고와 오류는 **0개**다. 확인을 위해 같은 빌드 명령을 한 번 더 실행했고, 종료 코드 0으로 모든 세 대상이 정상 최신 상태임을 확인했다.

- `cpppj/build/bin/Release/NetstormCpp.exe` — 6,655,488바이트
- `cpppj/build/bin/Release/netstorm_tests.exe` — 23,471,104바이트
- `cpppj/build/lib/Release/netstorm.lib` — 203,027,918바이트

검사 실행 파일은 컴파일만 했으며 실행하지 않았다. 원본/CD 및 클론 게임은 실행하지 않았고 `originals/`, `originalCD/`, `cpppj` 소스와 AGENTS.md는 변경하지 않았다. 최신 전체 정밀 디컴파일 호스트는 **HJOW-Athlon / 2026-10-23**이다.

## 선행 실행 기록: VM-W11-Codex (2026-10-23)

아래는 같은 날 VM-W11-Codex에서 수행했던 이전 사전 준비 실행 기록이다. 최신 HJOW-Athlon 재실행의 수치와 환경은 위 절을 따른다.

## 실행 이유와 환경

요청 범위는 cpppj 사전 준비(기존 게임 디컴파일 및 cpppj 컴파일)로 한정했다. `AGENTS.md`와 `LEFT_JOBS.md`를 확인했으며, 시작 HEAD는 `ab3ef43d / 20261023-15 CP`, 작업 트리는 깨끗했다. 당시 호스트는 **VM-W11-Codex**이고, 마지막 전체 정밀 디컴파일 호스트는 **HJOW-Athlon / 2026-10-22**였다. 호스트가 달라 10.78 패치판과 10.72 CD판을 다시 분석했다.

실제 게임은 실행하지 않았다. `tools/ghidra/refine_all.ps1`은 원본 실행 파일을 Ghidra에서 정적으로 분석하며, 결과는 Git 제외 경로 `extracted/refined/`에 생성한다. 원본 `originals/`, CD `originalCD/`, `AGENTS.md`는 수정하지 않았다.

## 정밀 디컴파일

다음 환경 변수와 명령을 사용했다.

```powershell
$env:PYTHONUTF8='1'
$env:PYTHONIOENCODING='utf-8'
$env:PYTHONUNBUFFERED='1'
powershell -NoProfile -ExecutionPolicy Bypass -File tools/ghidra/refine_all.ps1
```

| 판본 | 최종 함수 수 | 디컴파일 성공 | 실패 | 최종 등급 A/B/C/D | 함수 포인터 표 |
| --- | ---: | ---: | ---: | --- | ---: |
| originals, 10.78 패치판 | 5,549 | 5,549 | 0 | 2,326 / 3,152 / 40 / 31 | 1,652 |
| originalCD, 10.72 CD판 | 5,823 | 5,822 | 1 | 2,315 / 2,939 / 484 / 85 | 1,121 |

두 판본의 함수 대응은 **2,496쌍**(강 1,738 / 중 707 / 약 51)이다. 호출 규약 일치율은 **97.2%**, 반환 스택 크기 일치율은 **97.9%**다. 전체 단계와 최종 대응·등급 산출이 완료됐다.

Ghidra 로그에는 두 판본의 GIF 리소스 데이터 오류 및 일부 함수의 p-code 흐름 경고가 있었다. CD판의 실패 함수 1개는 미해결로 남겼으며 성공으로 간주하지 않는다.

## cpppj Release 컴파일

```powershell
cmake --build cpppj/build --config Release
```

CMake가 변경된 대상에 맞춰 빌드 시스템을 재구성했고 MSBuild **17.14.60**이 Release 대상을 컴파일했다. 빌드는 종료 코드 **0**이며 관측된 컴파일 경고와 오류는 **0개**다. 생성/갱신된 결과물은 다음과 같다.

- `cpppj/build/bin/Release/NetstormCpp.exe` — 6,210,048바이트
- `cpppj/build/bin/Release/netstorm_tests.exe` — 22,275,584바이트

테스트 실행 파일은 컴파일만 했다. CTest/검사와 원본·클론 게임은 실행하지 않았다.

## 인수인계 경계

그 실행 뒤 최신 전체 정밀 디컴파일 호스트를 **VM-W11-Codex / 2026-10-23**으로 갱신했었다. 이후 HJOW-Athlon에서 다시 수행한 최신 결과와 PC 정보는 위 보완 기록을 따른다. 두 실행 모두 사전 준비에 한정하며, 진행 중이던 게임 복원 목표를 완료 처리하지 않는다.
