# 2026-10-23 양 판본 정밀 디컴파일 및 cpppj Release 컴파일

## 실행 이유와 환경

요청 범위는 cpppj 사전 준비(기존 게임 디컴파일 및 cpppj 컴파일)로 한정했다. `AGENTS.md`와 `LEFT_JOBS.md`를 확인했으며, 시작 HEAD는 `ab3ef43d / 20261023-15 CP`, 작업 트리는 깨끗했다. 현재 호스트는 **VM-W11-Codex**이고, 마지막 전체 정밀 디컴파일 호스트는 **HJOW-Athlon / 2026-10-22**였다. 호스트가 달라 10.78 패치판과 10.72 CD판을 다시 분석했다.

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

최신 전체 정밀 디컴파일 호스트를 **VM-W11-Codex / 2026-10-23**으로 갱신했다. 이번 요청은 사전 준비에 한정하며, 단위352에서 중지한 게임 복원 목표를 재개하거나 완료 처리하지 않는다.
