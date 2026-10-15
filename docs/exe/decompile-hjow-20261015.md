# HJOW-Athlon 10.78·10.72 정밀 디컴파일 재생성

작업일은 **2026-10-15**, 현재 호스트는 **HJOW-Athlon**이다. 시작 시 `LEFT_JOBS.md`의 마지막 정밀 디컴파일 호스트가 **VM-W11-Codex**로 기록되어 있어, `AGENTS.md`의 호스트 확인 규칙에 따라 양 판본을 이 PC에서 재생성했다. `AGENTS.md`와 양쪽 인수인계 문서를 확인했다.

## 입력과 실행

Ghidra **12.1.4**, Python **3.13.1**, PowerShell을 사용했다. `GHIDRA_INSTALL_DIR`은 `C:\Tools\ghidra_12.1.4_PUBLIC`이다.

```powershell
$env:PYTHONUTF8 = '1'
powershell -ExecutionPolicy Bypass -File tools\ghidra\refine_all.ps1 -GhidraDir $env:GHIDRA_INSTALL_DIR
```

이 스크립트는 각 판본을 새 정밀 프로젝트로 가져와 분석하고, 누락 함수·switch·호출 규약을 보정한 뒤 판본 간 함수 대응과 호출 규약 힌트를 계산한다. 힌트를 적용해 두 프로젝트를 다시 내보내고, 대응표·등급표·최종 C 파일을 만든다. 게임 실행은 필요하지 않다.

스크립트는 `extracted/refined/` 안의 같은 이름 프로젝트와 결과 파일을 다시 쓴다. 기존 로컬 결과를 보존하기 위해 실행 전에 `extracted/refined/`를 `extracted/refined-backup-before-HJOW-Athlon-20261015/`로 옮겼다. 기존 결과와 새 정밀 프로젝트는 모두 Git 제외 경로에 있다.

| 판본 | 입력 및 SHA-256 | 최종 C | 함수표와 디컴파일 결과 |
| --- | --- | --- | --- |
| 패치 10.78 | `originals/Netstorm.exe` — `A305414CFE0B94494BB32FEF2B359E920E9DABA225E0821B76AEA1F9F8E7D236` | `extracted/refined/originals/Netstorm.c` | 5,549개 함수, 성공 5,549·실패 0 |
| CD 10.72 | `originalCD/NETSTORM.EXE` — `613500A394B3062E914604EB7D09896D678192BE24545D0E575082A87C851233` | `extracted/refined/originalCD/NETSTORM.c` | 5,823개 함수, 성공 5,822·실패 1 |

각 `raw.c`와 최종 C의 함수 표식 수가 `functions.tsv` 행 수와 일치한다. CD판의 한 실패는 `FUN_0049f99f`의 `Low-level Error: Cannot properly adjust input varnodes`이며 이전 디컴파일 기록과 같은 경계다. 패치판 첫 내보내기는 5,548개였고 판본 간 힌트 적용 뒤 최종 내보내기는 5,549개였다.

판본 간 함수 대응은 **2,496쌍**(강 1,738·중 707·약 51)이다. 호출 규약 일치는 **2,313/2,379 (97.2%)**, `ret N` 일치는 **2,339/2,389 (97.9%)**다. 상대 판본 힌트 적용 뒤 추가로 보완 가능한 함수는 없었다.

## 보존 및 검증

두 원본 PE의 SHA-256은 실행 전과 후에 각각 위 값과 일치한다. 원본 파일은 읽기만 했다. Ghidra가 일부 GIF 리소스와 비어 있는 메모리·생성자 경로에서 경고를 출력했지만, 패치판 최종 실패는 0개이고 CD판 실패는 위 단일 함수다.

실제 게임/GUI 실행, 빌드, 테스트, cpppj·dotnetpj 소스 변경은 없었다. `AGENTS.md`는 수정하지 않았다. 최종 산출물은 cpppj 분석에 `extracted/refined/originals/Netstorm.c`와 `extracted/refined/originalCD/NETSTORM.c`를 사용한다. 현재 및 마지막 정밀 디컴파일 PC는 **HJOW-Athlon, 2026-10-15**다.
