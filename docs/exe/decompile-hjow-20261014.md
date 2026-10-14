# HJOW-Athlon 10.78·10.72 재디컴파일과 cpppj 빌드 검증

작업일: **2026-10-14**. 현재 PC는 **HJOW-Athlon**이다. 시작 전 `LEFT_JOBS.md`에는 마지막 전체 디컴파일 PC가 **VM-W11-Codex**로 기록되어 있었다. AGENTS의 다른 PC 재디컴파일 규칙에 따라 10.78 패치판과 CD 10.72판을 이 PC에서 다시 분석하고 cpppj Release 빌드를 검증했다.

## 전체 디컴파일

Ghidra **12.1.4** 헤드리스 분석을 사용했다. 원본 실행 파일은 읽기만 했다.

| 판본 | 입력과 SHA-256 | 디컴파일 출력 | 결과 |
| --- | --- | --- | --- |
| 패치 10.78 | `originals/Netstorm.exe` — `A305414CFE0B94494BB32FEF2B359E920E9DABA225E0821B76AEA1F9F8E7D236` | `extracted/decomp/Netstorm.c` · `functions.tsv` | 성공 **4,506**, 실패 **0** |
| CD 10.72 | `originalCD/NETSTORM.EXE` — `613500A394B3062E914604EB7D09896D678192BE24545D0E575082A87C851233` | `extracted/originalCD/decomp/NETSTORM.c` · `functions.tsv` | 성공 **3,711**, 실패 **0** |

각 판본의 C 함수 표식 수와 `functions.tsv` 행 수가 디컴파일 수와 일치하며, C 파일에 실패 함수 주석은 없었다. 실행 후에도 두 보호 PE의 SHA-256은 위 기준값과 같았다.

실행한 명령:

```powershell
powershell -ExecutionPolicy Bypass -File tools\ghidra\run_decomp.ps1 -Edition originals -GhidraDir C:\Tools\ghidra_12.1.4_PUBLIC
powershell -ExecutionPolicy Bypass -File tools\ghidra\run_decomp.ps1 -Edition originalCD -GhidraDir C:\Tools\ghidra_12.1.4_PUBLIC
```

이번 작업은 기본 전체 디컴파일을 갱신했다. `extracted/refined/` 정밀 프로젝트와 함수별 정밀 export는 재생성하지 않았다. 최신 정밀 분석 절차와 결과는 [10월 13일 기록](decompile-hjow-20261013.md)에 있다.

## cpppj Release 검증

저장소의 빌드 안내에 따라 구성, Release 빌드, CTest를 실행했다.

```powershell
cmake -S cpppj -B cpppj/build -DCMAKE_BUILD_TYPE=Release
cmake --build cpppj/build --config Release --parallel
ctest --test-dir cpppj/build --build-config Release --output-on-failure
```

- CMake 구성과 전체 Release 빌드가 성공했다. `netstorm.lib`, `NetstormCpp.exe`, `netstorm_tests.exe`를 생성했으며 빌드 출력에 컴파일 경고나 오류는 없었다.
- CTest 등록 대상 **1/1 통과**(194.68초). 내부 검사 **1,088개**, 실패 체크 **0개** (`cpppj/build/Testing/Temporary/LastTest.log`).
- 원본 게임, 클론 게임, GUI 창은 실행하지 않았다. C++ 소스와 테스트 코드는 변경하지 않았다.

마지막 전체 디컴파일 PC는 **HJOW-Athlon, 2026-10-14**다. 별도 `extracted/` 로그는 Git에 포함되지 않는다.
