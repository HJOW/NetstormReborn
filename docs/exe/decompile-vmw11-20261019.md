# VM-W11-Codex 10.78·10.72 정밀 디컴파일 재생성

작업일은 **2026-10-19**, 현재 호스트는 **VM-W11-Codex**다. 작업 전 `AGENTS.md`, `LEFT_JOBS.md`, `LEFT_JOBS.dotnetpj.md`를 읽었다. `LEFT_JOBS.md`에 적힌 최신 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-16**이어서 현재 호스트와 달랐다. cpppj 작업 전 호스트 일치 규칙에 따라 패치판 10.78과 CD판 10.72를 이 PC에서 다시 디컴파일했다.

## 실행 절차

- Ghidra **12.1.4**, Eclipse Temurin OpenJDK **21.0.12**, Python **3.12.4**를 사용했다.
- `PYTHONUTF8=1`을 설정하고 `tools/ghidra/refine_all.ps1`을 실행했다. 이 절차는 양 판본의 함수 복구·호출 규약 보정·전체 C 내보내기, 판본 간 대응 힌트 계산·적용, 최종 대응표와 등급 생성을 수행한다.
- 기존 `extracted/refined/` 산출물은 덮어쓰기 전에 `extracted/refined-backup-before-VM-W11-Codex-20261019/`로 옮겨 보존했다. 이번 실행 로그는 `extracted/refine-vmw11-codex-20261019-run.log`에 있다. 둘 다 Git 제외 경로다.

## 결과

| 판본 | 입력 SHA-256 | 함수표 행 / 최종 C 함수 | 디컴파일 | 최종 C SHA-256 | 함수표 SHA-256 |
| --- | --- | ---: | --- | --- | --- |
| 패치 10.78 | `originals/Netstorm.exe` — `A305414CFE0B94494BB32FEF2B359E920E9DABA225E0821B76AEA1F9F8E7D236` | 5,549 / 5,549 | 성공 5,549·실패 0 | `AF7CBF0B68E9D8E5AEB7068382C88D66201D3906C2926C54909AE14DE4AD5646` | `3EA34D7CD978AAA5C141A629264FEA5E8B2F78C4E6B0B4E4633F982A2583F658` |
| CD 10.72 | `originalCD/NETSTORM.EXE` — `613500A394B3062E914604EB7D09896D678192BE24545D0E575082A87C851233` | 5,823 / 5,823 | 성공 5,822·실패 1 | `9306F621576571C85280F9E25F024E3EC8B7B68C5C8EC6731C1F109DC3F25EBB` | `453AA383C27BAF8EEFA00D97BF44051D18460150556511C2EB7A46F111E92DEF` |

CD판의 한 실패는 `FUN_0049f99f`의 `Cannot properly adjust input varnodes`다. 기존 정밀 분석에서 확인된 함수 경계와 같다. 판본 대응은 **2,496쌍**(강 1,738·중 707·약 51), 호출 규약 일치는 **2,313/2,379 (97.2%)**, `ret N` 일치는 **2,339/2,389 (97.9%)**다.

실행 전후 원본 exe 해시는 위 표와 같았다. 게임이나 클론 게임은 실행하지 않았고, cpppj 소스도 변경하지 않았다. 현재 호스트의 정밀 결과는 `extracted/refined/`에 있다. 이후 cpppj 작업은 다른 PC에서 이어질 경우 다시 `LEFT_JOBS.md`의 마지막 디컴파일 호스트와 대조해야 한다.
