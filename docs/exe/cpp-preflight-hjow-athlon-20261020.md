# cpppj 사전 준비 — HJOW-Athlon 정밀 디컴파일 및 Release 컴파일

작업일은 **2026-10-20**, PC는 **HJOW-Athlon**이다. 작업 전에 `AGENTS.md`, `LEFT_JOBS.md`, `LEFT_JOBS.dotnetpj.md`를 확인했다. 인수인계의 직전 정밀 디컴파일 PC는 `VM-W11-Codex / 2026-10-20`였으므로 현재 PC와 달라 두 판본을 모두 다시 정밀 디컴파일했다. 이 작업은 cpppj 사전 준비만 수행했으며, 보류 중인 구현 목표를 재개하지 않았다.

## 기존 결과 보존

재생성 전 `extracted/refined/`를 `extracted/refined-backup-before-HJOW-Athlon-20261020/`에 복사했다. 백업 전후 파일 수와 전체 크기를 확인했으며, **41개 파일·138,067,470바이트**로 일치한다.

## 정밀 디컴파일

저장소의 전체 절차를 Python UTF-8 모드로 실행했다. Ghidra 12.1.4, Zulu OpenJDK 25.0.1, Python 3.13.1 환경이다.

```powershell
$env:PYTHONUTF8 = '1'
powershell -ExecutionPolicy Bypass -File tools\ghidra\refine_all.ps1
```

| 판본 | 함수 | 디컴파일 결과 | 최종 C SHA-256 | `raw.c` SHA-256 | `functions.tsv` SHA-256 |
| --- | ---: | --- | --- | --- | --- |
| 패치 10.78 | 5,549 | 성공 5,549 · 실패 0 | `AF7CBF0B68E9D8E5AEB7068382C88D66201D3906C2926C54909AE14DE4AD5646` | `0B5D53B15FDB2EF7AFBCCF61AF666709726A96FB3B972B6818F4A42EFCD31972` | `3EA34D7CD978AAA5C141A629264FEA5E8B2F78C4E6B0B4E4633F982A2583F658` |
| CD 10.72 | 5,823 | 성공 5,822 · 실패 1 (`FUN_0049f99f`) | `9306F621576571C85280F9E25F024E3EC8B7B68C5C8EC6731C1F109DC3F25EBB` | `DF7FE932EEFEAEAEF6109237A16FEB0EA0BCFDBC6CA45375D2EC899B02A67F2C` | `453AA383C27BAF8EEFA00D97BF44051D18460150556511C2EB7A46F111E92DEF` |

함수 대응은 **2,496쌍**(강 1,738 · 중 707 · 약 51)이며 호출 규약 일치는 2,313/2,379(97.2%), `ret N` 일치는 2,339/2,389(97.9%)다. 대응 근거로 10.78판에 힌트 8개, CD판에 9개를 적용했다. CD판의 `FUN_0049f99f` 실패는 기존 사전 준비와 같은 한 건으로, 나머지 전체 절차와 최종 파일 생성은 완료됐다.

원본 입력 파일의 해시는 기존 기준과 일치한다.

- `originals/Netstorm.exe`: `A305414CFE0B94494BB32FEF2B359E920E9DABA225E0821B76AEA1F9F8E7D236`
- `originalCD/NETSTORM.EXE`: `613500A394B3062E914604EB7D09896D678192BE24545D0E575082A87C851233`

## Release 컴파일

다음 빌드 명령을 실행했다. CMake가 최신 `cpppj/src/CMakeLists.txt`를 감지해 구성을 갱신했고, 최종 확인 실행은 **종료 코드 0**이었다.

```powershell
cmake --build cpppj/build --config Release --parallel
```

빌드 산출물:

- `cpppj/build/lib/Release/netstorm.lib` — SHA-256 `5F91999D49B5666270A0B55296E0B48FD835899BCD18C3E5B1B4FDF698EF886B`
- `cpppj/build/bin/Release/NetstormCpp.exe` — SHA-256 `9646D905E8989B0B6C276C725640E16E27EA2D5159E5B89C89CCAFDE57D1786E`
- `cpppj/build/bin/Release/netstorm_tests.exe` — SHA-256 `F0AC09139476F48D075D74AF5438BA79B890321B8E757F5C78606A51C2B4523F`

테스트 실행 파일은 빌드됐지만 CTest와 단위 테스트, 원본 게임 및 cpppj 게임 실행은 하지 않았다. 원본 디렉터리와 cpppj 소스도 수정하지 않았다. 생성된 정밀 디컴파일 결과와 백업은 Git 제외 경로 `extracted/`에 둔다.

## 인수인계

현재 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-20**이다. 다음 cpppj 구현은 기존 인수인계의 미완료 작업에서 이어가며, 이번 준비는 구현 목표나 실제 플레이 검증을 재개하지 않는다.
