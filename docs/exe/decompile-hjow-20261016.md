# HJOW-Athlon 10.78·10.72 정밀 디컴파일 재생성

작업일은 **2026-10-16**, 현재 호스트는 **HJOW-Athlon** (`192.168.0.94`)이다. 작업 전 `AGENTS.md`, `LEFT_JOBS.md`, `LEFT_JOBS.dotnetpj.md`를 읽었다. `LEFT_JOBS.md`의 마지막 전체 정밀 디컴파일은 **VM-W11-Codex / 2026-10-16**로 기록되어 있어, cpppj 작업에 앞서 호스트 일치 규칙에 따라 양 판본을 현재 PC에서 다시 만들었다.

## 실행과 입력

Ghidra **12.1.4** (빌드 2026-09-21), Python **3.13.1**, Zulu OpenJDK **25**와 `tools/ghidra/refine_all.ps1`을 사용했다. Ghidra 설정은 최소 Java 21이며, 헤드리스 실행은 종료 코드 **0**으로 끝났다.

```powershell
$env:PYTHONUTF8='1'
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\tools\ghidra\refine_all.ps1 -Python (Get-Command python).Source
```

스크립트는 양 판본을 새 정밀 프로젝트로 분석하고 누락 함수를 복구·분기와 호출 규약을 보정한 뒤, 상대 판본 대응 힌트를 적용해 다시 내보냈다. 최종 판본 대응표와 등급을 생성했다. 실행 로그는 Git 제외 경로 `extracted/decompile-hjow-20261016-run.log`에 있다. 실행 전 기존 `extracted/refined/`를 `extracted/refined-backup-before-HJOW-Athlon-20261016/`로 보존했다.

| 판본 | 입력 SHA-256 | 함수표 행 / 최종 C 표식 | 디컴파일 결과 | 최종 C SHA-256 | 함수표 SHA-256 |
| --- | --- | ---: | ---: | --- | --- |
| 패치 10.78 | `originals/Netstorm.exe` — `A305414CFE0B94494BB32FEF2B359E920E9DABA225E0821B76AEA1F9F8E7D236` | 5,549 / 5,549 | 성공 5,549·실패 0 | `AF7CBF0B68E9D8E5AEB7068382C88D66201D3906C2926C54909AE14DE4AD5646` | `3EA34D7CD978AAA5C141A629264FEA5E8B2F78C4E6B0B4E4633F982A2583F658` |
| CD 10.72 | `originalCD/NETSTORM.EXE` — `613500A394B3062E914604EB7D09896D678192BE24545D0E575082A87C851233` | 5,823 / 5,823 | 성공 5,822·실패 1 | `9306F621576571C85280F9E25F024E3EC8B7B68C5C8EC6731C1F109DC3F25EBB` | `453AA383C27BAF8EEFA00D97BF44051D18460150556511C2EB7A46F111E92DEF` |

양 입력 파일의 해시는 실행 전후로 각각 위 값과 같아 원본은 읽기만 했다. 최종 C 함수 표식 수와 함수표 행 수도 양 판본 모두 일치한다. CD판의 유일한 실패는 `FUN_0049f99f` (`0049f99f–004a0045`)의 `Cannot properly adjust input varnodes`이며 기존 정밀 분석에서 확인한 경계와 같다.

판본 대응은 **2,496쌍**(강 1,738·중 707·약 51), 호출 규약 일치는 **2,313/2,379 (97.2%)**, `ret N` 일치는 **2,339/2,389 (97.9%)**다. 힌트 적용 후 보완된 함수는 10.78에서 8개, CD판에서 9개였다.

10.78 최종 C와 두 함수표의 해시는 이전 VM 기록과 같다. CD 10.72의 함수표도 이전 기록과 같지만, 최종 C는 이전 VM 기록의 `F8CF1CA26B0B272C19AD6CC46BE3D21A2654BD3FF697B367265678A224EFFDBB`와 다른 위 해시다. 두 기록의 Java/Python 버전이 각각 21.0.12·3.12.4와 25·3.13.1로 달라 출력 차이의 원인은 아직 분리되지 않았다. 현재 생성물은 현재 PC의 정밀 분석 산출물로 보존하며, CD판 C의 바이트 동일성을 이전 VM 산출물과 확인했다고 주장하지 않는다.

이번 작업은 정적 분석만 수행했다. 원본/클론 게임과 GUI 실행, 빌드, 테스트, cpppj·dotnetpj 소스 변경은 없다. Git 제외 산출물은 `extracted/refined/`에 있으며 현재 및 마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon, 2026-10-16**이다.
