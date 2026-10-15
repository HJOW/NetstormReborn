# VM-W11-Codex 10.78·10.72 정밀 디컴파일 재생성

작업일: **2026-10-16**. 호스트는 **VM-W11-Codex** (`10.0.0.17`)다. 작업 시작 시 `LEFT_JOBS.md`의 마지막 정밀 디컴파일 호스트는 **HJOW-Athlon / 2026-10-15**였다. cpppj 분석에 앞서 다른 PC 작업 규칙에 따라 두 판본을 다시 정밀 분석했다.

## 절차와 입력

Ghidra **12.1.4 PUBLIC** 헤드리스 분석, Java **21.0.12**, Python **3.12.4**를 사용했다. 아래 저장소 절차가 1단계 자동 분석·누락 함수 복구·호출 규약 추정·양 판본 대응, 2단계 힌트 적용·재내보내기, 최종 C 파일·등급·함수 대응표 생성을 모두 수행했다.

```powershell
$env:PYTHONUTF8='1'; & powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\tools\ghidra\refine_all.ps1 -Python (Get-Command python).Source 2>&1 | Tee-Object -FilePath .\extracted\decompile-vmw11-20261016-run.log; $runExit=$LASTEXITCODE; exit $runExit
```

실제 실행은 Python 경로를 명시했고 종료 코드 **0**이었다. 전체 로그는 Git 제외 파일 `extracted/decompile-vmw11-20261016-run.log`에 남겼다. 입력 바이너리의 확인된 SHA-256은 다음과 같다.

| 판본 | 입력 파일 | SHA-256 |
| --- | --- | --- |
| 패치 10.78 | `originals/Netstorm.exe` | `A305414CFE0B94494BB32FEF2B359E920E9DABA225E0821B76AEA1F9F8E7D236` |
| CD 10.72 | `originalCD/NETSTORM.EXE` | `613500A394B3062E914604EB7D09896D678192BE24545D0E575082A87C851233` |

## 결과

각 최종 C 함수 표식 수가 `functions.tsv`의 데이터 행 수와 일치한다.

| 판본 | 최종 C | 함수 수 | 성공 / 실패 |
| --- | --- | ---: | ---: |
| 패치 10.78 | `extracted/refined/originals/Netstorm.c` | 5,549 | 5,549 / 0 |
| CD 10.72 | `extracted/refined/originalCD/NETSTORM.c` | 5,823 | 5,822 / 1 |

CD판의 유일한 실패는 `FUN_0049f99f` (`0049f99f–004a0045`)의 `Low-level Error: Cannot properly adjust input varnodes`다. 오류는 `raw.c`, 최종 `NETSTORM.c`, 함수표에 남아 있다. 지난 전체 재디컴파일과 같은 함수·오류다.

두 판본 사이의 함수 대응은 **2,496쌍**(강 1,738·중 707·약 51)이다. 대응 함수의 호출 규약 일치는 **2,313/2,379 (97.2%)**, `ret N` 일치는 **2,339/2,389 (97.9%)**다. 10.78의 최종 등급은 A 2,326·B 3,152·C 40·D 31, CD판은 A 2,315·B 2,939·C 484·D 85다.

| 산출물 | SHA-256 |
| --- | --- |
| 10.78 `Netstorm.c` | `AF7CBF0B68E9D8E5AEB7068382C88D66201D3906C2926C54909AE14DE4AD5646` |
| 10.78 `functions.tsv` | `3EA34D7CD978AAA5C141A629264FEA5E8B2F78C4E6B0B4E4633F982A2583F658` |
| 10.72 `NETSTORM.c` | `F8CF1CA26B0B272C19AD6CC46BE3D21A2654BD3FF697B367265678A224EFFDBB` |
| 10.72 `functions.tsv` | `453AA383C27BAF8EEFA00D97BF44051D18460150556511C2EB7A46F111E92DEF` |

두 C 파일과 함수표의 해시는 직전 기록의 최종 산출물과 동일하다. Ghidra가 보고한 GIF 리소스 형식 경고는 양 판본에 각 2건 있으며 함수 디컴파일 집계에는 영향을 주지 않았다. 10.78에서 생성자/분기 주소 관련 경고가 일부 있었지만 최종 디컴파일 실패는 0건이다.

이번 작업은 정적 분석뿐이며 원본 게임은 실행하지 않았다. `originals/`와 `originalCD/`의 바이너리는 읽기 전용으로 다뤘고, cpppj 소스 변경이나 테스트 실행은 하지 않았다. 생성물은 Git 제외 경로 `extracted/refined/`에 보관한다.

현재 마지막 전체 정밀 디컴파일 PC는 **VM-W11-Codex, 2026-10-16**이다.
