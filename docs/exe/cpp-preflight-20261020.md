# cpppj 사전 준비 — VM-W11-Codex 정밀 디컴파일과 Release 컴파일

작업일은 **2026-10-20**, PC는 **VM-W11-Codex / 10.0.0.17**이다. `AGENTS.md`, `LEFT_JOBS.md`, `LEFT_JOBS.dotnetpj.md`를 다시 읽었다. 인수인계의 최신 정밀 디컴파일 PC는 `HJOW-Athlon / 2026-10-19`였으므로 cpppj 작업 전에 현재 PC에서 10.78 패치판과 CD 10.72판을 다시 준비했다. 이 작업은 사전 준비이며, 일시 중지된 구현 목표나 통합 플레이 검증을 재개하지 않았다.

## 정밀 디컴파일

기존 `extracted/refined/`의 41개 파일(138,001,964바이트)은 재생성 전에 `extracted/refined-backup-before-VM-W11-Codex-20261020/`로 옮겨 보존했다. 전체 절차는 `tools/ghidra/refine_all.ps1`을 사용했다. Ghidra 12.1.4, Temurin OpenJDK 21.0.12+1, Python 3.12.4 환경이다.

첫 전체 실행에서는 두 판본의 Ghidra 분석과 C 내보내기까지 끝났지만, 판본 대응 요약을 출력하는 `tools/decomp_refine.py --hints`가 Windows 기본 CP949 인코딩으로 em dash를 출력하지 못해 종료 코드 1을 반환했다. 생성된 Ghidra 프로젝트를 보존하고 다음과 같이 `-SkipStage1`로 대응 힌트 단계부터 재개했다. UTF-8 설정은 해당 프로세스에만 적용했다.

```powershell
$env:PYTHONUTF8 = '1'
powershell -ExecutionPolicy Bypass -File tools\ghidra\refine_all.ps1 -SkipStage1
```

재개 실행은 종료 코드 0으로 끝났다. 최종 결과는 다음과 같다.

| 판본 | 입력 EXE SHA-256 | 함수 | C 내보내기 | `functions.tsv` SHA-256 | `raw.c` SHA-256 | 최종 C SHA-256 |
| --- | --- | ---: | --- | --- | --- | --- |
| 패치 10.78 | `A305414CFE0B94494BB32FEF2B359E920E9DABA225E0821B76AEA1F9F8E7D236` | 5,549 | 성공 5,549 · 실패 0 | `3EA34D7CD978AAA5C141A629264FEA5E8B2F78C4E6B0B4E4633F982A2583F658` | `0B5D53B15FDB2EF7AFBCCF61AF666709726A96FB3B972B6818F4A42EFCD31972` | `AF7CBF0B68E9D8E5AEB7068382C88D66201D3906C2926C54909AE14DE4AD5646` |
| CD 10.72 | `613500A394B3062E914604EB7D09896D678192BE24545D0E575082A87C851233` | 5,823 | 성공 5,822 · 실패 1 (`FUN_0049f99f`) | `453AA383C27BAF8EEFA00D97BF44051D18460150556511C2EB7A46F111E92DEF` | `614884BEA530C070BFB8B5C6629F6C4394D54BADD00E6E73712C9ECDF672DB76` | `F8CF1CA26B0B272C19AD6CC46BE3D21A2654BD3FF697B367265678A224EFFDBB` |

CD판의 실패는 `FUN_0049f99f` 한 함수이며, 입력 varnode 보정 오류다. 판본 대응은 **2,496쌍**(강 1,738 · 중 707 · 약 51), 호출 규약 일치는 **2,313/2,379 (97.2%)**, `ret N` 일치는 **2,339/2,389 (97.9%)**다. 두 입력 EXE의 작업 전후 SHA-256은 표와 같아 원본 바이너리가 변경되지 않았다. 현재 산출물 41개를 보존했던 이 PC의 이전 결과와 대조했을 때, 내보낸 함수표·원시 C·최종 C와 대응 자료는 같았고 Ghidra 프로젝트 내부 데이터 파일 10개만 재저장됐다.

실행 로그는 Git 제외 경로에 있다: `extracted/refine-vmw11-codex-20261020-stdout.log`, `extracted/refine-vmw11-codex-20261020-stderr.log`, `extracted/refine-vmw11-codex-20261020-resume-stdout.log`, `extracted/refine-vmw11-codex-20261020-resume-stderr.log`.

## Release 컴파일

다음 명령이 종료 코드 0으로 끝났다. CMake가 변경된 `cpppj/src/CMakeLists.txt`를 감지해 생성 파일을 갱신한 뒤 라이브러리·게임 실행 파일·테스트 실행 파일을 컴파일했다.

```powershell
cmake --build cpppj/build --config Release --parallel
```

- `cpppj/build/lib/Release/netstorm.lib`
- `cpppj/build/bin/Release/NetstormCpp.exe` — SHA-256 `7CEF804FABBB23728773CDD63EE2BD869BC074AD21F1236CE1B3221A6F963DD3`
- `cpppj/build/bin/Release/netstorm_tests.exe` — SHA-256 `AC0B34D0B496F566BFEAA505BB65CD8CE8E6B7B70B996B44EA8239F2AED667AF`

사용자의 테스트 이월 지시에 따라 CTest와 게임 실행은 하지 않았다. `AGENTS.md`, `originals/`, `originalCD/`, cpppj 소스는 수정하지 않았다. 다음 cpppj 구현은 사용자가 재개를 지시한 뒤 인수인계의 미완료 항목에서 이어간다.
