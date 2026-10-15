# VM-W11-Codex 10.78·10.72 정밀 디컴파일 재생성

작업일: **2026-10-15**. 현재 호스트는 **VM-W11-Codex**다. 시작 시 `LEFT_JOBS.md`의 마지막 정밀 디컴파일 PC는 **HJOW-Athlon**으로 기록되어 있어, cpppj 원본 분석에 앞서 현재 PC에서 양 판본의 정밀 디컴파일을 재생성했다.

## 실행 및 입력

Ghidra **12.1.4**의 헤드리스 분석을 사용했다. 다음 명령으로 패치판 10.78과 CD판 10.72를 순서대로 분석하고, 판본 간 대응·호출 규약 힌트·최종 C 파일까지 생성했다.

```powershell
$env:PYTHONUTF8='1'
powershell -ExecutionPolicy Bypass -File tools\ghidra\refine_all.ps1
```

| 판본 | 입력 | SHA-256 |
| --- | --- | --- |
| 패치 10.78 | `originals/Netstorm.exe` | `A305414CFE0B94494BB32FEF2B359E920E9DABA225E0821B76AEA1F9F8E7D236` |
| CD 10.72 | `originalCD/NETSTORM.EXE` | `613500A394B3062E914604EB7D09896D678192BE24545D0E575082A87C851233` |

## 결과

Ghidra 후처리와 `tools/decomp_refine.py`의 최종 생성이 모두 종료 코드 0으로 끝났다. 최종 결과는 Git 제외 경로인 `extracted/refined/`에 있다.

| 판본 | 최종 C 표식 | `functions.tsv` 함수 행 | 디컴파일 결과 |
| --- | ---: | ---: | --- |
| 패치 10.78 | 5,549 | 5,549 | 성공 5,549, 실패 0 |
| CD 10.72 | 5,823 | 5,823 | 성공 5,822, 실패 1 |

CD판의 한 건은 기존 기록과 같은 `FUN_0049f99f`의 `Low-level Error: Cannot properly adjust input varnodes`다. 오류는 `raw.c`, 함수 사실표, 최종 `NETSTORM.c`에 남아 있다. 결과의 함수 개수는 각 판본에서 C 함수 표식 수와 함수표 행 수가 일치한다.

양 판본 함수 대응은 **2,496쌍**(강 1,738·중 707·약 51)이다. 호출 규약 일치는 **2,313/2,379(97.2%)**, `ret N` 일치는 **2,339/2,389(97.9%)**다. 최종 산출물은 다음 경로에서 cpppj 분석에 사용할 수 있다.

- 패치판: `extracted/refined/originals/Netstorm.c`
- CD판: `extracted/refined/originalCD/NETSTORM.c`
- 판본 대응: `extracted/refined/match.tsv`

원본 두 실행 파일의 SHA-256은 표의 값과 일치했다. 원본 파일은 읽기만 했다. 게임·GUI 실행, 빌드, 테스트, cpppj 소스 수정은 하지 않았다. 정밀 프로젝트와 분석 산출물은 Git 제외 경로에 있으므로 이 커밋은 인수인계 기록만 포함한다.

현재 및 마지막 정밀 디컴파일 PC는 **VM-W11-Codex, 2026-10-15**다.
