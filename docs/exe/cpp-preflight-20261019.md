# cpppj 사전 준비 — 현재 PC 정밀 디컴파일·Release 빌드·전체 회귀

작업일은 **2026-10-19**, PC는 **HJOW-Athlon / 192.168.0.94**다. 작업 전 `AGENTS.md`, `LEFT_JOBS.md`, `LEFT_JOBS.dotnetpj.md`를 읽었다. 직전 기록의 마지막 정밀 디컴파일 PC는 **VM-W11-Codex / 2026-10-19**라 달랐으므로, cpppj 작업 전에 패치판 10.78과 CD판 10.72를 현재 PC에서 다시 만들었다. 이 PC는 AGENTS.md의 원본 실행 예외에 해당하지만, 이번에는 원본·클론 게임을 실행하지 않았다.

## 정밀 디컴파일

`tools/ghidra/refine_all.ps1`로 두 판본을 분석·복구하고, 호출 규약 힌트를 상호 적용한 뒤 대응표·신뢰도 등급·최종 C 파일을 생성했다. 기존 `extracted/refined/` 결과 41개(138,034,702바이트)는 덮어쓰기 전에 `extracted/refined-backup-before-HJOW-Athlon-20261019/`로 옮겨 보존했다.

| 판본 | 입력 EXE SHA-256 | 함수표 행 | C 내보내기 결과 | 함수표 SHA-256 | 원시 C SHA-256 | 최종 C SHA-256 |
| --- | --- | ---: | --- | --- | --- | --- |
| 패치 10.78 | `A305414CFE0B94494BB32FEF2B359E920E9DABA225E0821B76AEA1F9F8E7D236` | 5,549 | 성공 5,549·실패 0 | `3EA34D7CD978AAA5C141A629264FEA5E8B2F78C4E6B0B4E4633F982A2583F658` | `0B5D53B15FDB2EF7AFBCCF61AF666709726A96FB3B972B6818F4A42EFCD31972` | `AF7CBF0B68E9D8E5AEB7068382C88D66201D3906C2926C54909AE14DE4AD5646` |
| CD 10.72 | `613500A394B3062E914604EB7D09896D678192BE24545D0E575082A87C851233` | 5,823 | 성공 5,822·실패 1 (`FUN_0049f99f`) | `453AA383C27BAF8EEFA00D97BF44051D18460150556511C2EB7A46F111E92DEF` | `DF7FE932EEFEAEAEF6109237A16FEB0EA0BCFDBC6CA45375D2EC899B02A67F2C` | `9306F621576571C85280F9E25F024E3EC8B7B68C5C8EC6731C1F109DC3F25EBB` |

신뢰도 대응은 2,496쌍(강 1,738·중 707·약 51), 호출 규약 일치는 2,313/2,379(97.2%), `ret N` 일치는 2,339/2,389(97.9%)다. CD판 실패는 기존과 같은 `FUN_0049f99f`의 입력 varnode 보정 실패다. 함수표에는 헤더가 포함된다.

첫 래퍼 실행은 패치판 내보내기 뒤 Java 종료 경고를 Windows PowerShell이 `NativeCommandError`로 처리해 CD 단계 전에 끝났다. 그 결과는 `extracted/refine-hjow-athlon-20261019-run.log`와 함께 남겨 두고, stdout/stderr를 별도 파일로 받는 재실행에서 전체 절차 종료 코드 0을 확인했다. 최종 결과 파일은 이전 산출물 백업과 바이트가 같았다. 이전 기록 `decompile-vmw11-20261019.md`에 적힌 CD 최종 C 해시 `F8CF1CA2…`는 실제 보존 산출물과 달라, 확인된 해시 `9306F621…`로 정정했다.

사용 도구는 Ghidra 12.1.4, Zulu OpenJDK 25(build 25+36-LTS), Python 3.13.1이다. 입력 EXE의 전후 SHA-256은 표와 동일하다.

## Release 빌드와 회귀

다음 Release 빌드가 종료 코드 0으로 완료됐다. 첫 전체 빌드는 CMake가 기존 생성 정보를 다시 구성한 뒤 `netstorm.lib`, `NetstormCpp.exe`, `netstorm_tests.exe`를 모두 다시 빌드했다. 마지막 증분 빌드도 종료 코드 0이다.

```powershell
cmake --build cpppj/build --config Release --parallel
```

전체 회귀 검사도 종료 코드 0이다. CTest 한 스위트가 통과했고 내부 검사 1,757개, 실패한 검사 0개를 기록했다(257.22초).

```powershell
ctest --test-dir cpppj/build --build-config Release --output-on-failure
```

빌드한 실행 파일 SHA-256은 `NetstormCpp.exe`가 `00541A088D84CE10F6FD224226B27DFDDF3E253722314DDE764287EFDBAFD891`, `netstorm_tests.exe`가 `ABB064ED4BEF475FEA3E38D590721C9BB9EBE655D4CE73AEBACCAFAE0BD917`다. 실행 파일·원시 검사 외에 코드 수정은 없고, 원본·클론 게임 창도 띄우지 않았다.

전체 재실행 stdout/stderr와 CTest 기록은 Git 제외 경로 `extracted/refine-hjow-athlon-20261019-rerun-stdout.log`, `extracted/refine-hjow-athlon-20261019-rerun-stderr.log`, `extracted/cpp-preflight-20261019-ctest.log`에 있다.
