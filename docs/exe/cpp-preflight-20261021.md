# cpppj 사전 작업 — VM-W11-CODEX 재디컴파일 및 Release 빌드

작업일은 **2026-10-21**, 시작 커밋은 `9859f159`, 현재 PC는 **VM-W11-CODEX**다. `AGENTS.md`, `LEFT_JOBS.md`, `LEFT_JOBS.dotnetpj.md`를 읽었다. `LEFT_JOBS.md`의 마지막 정밀 디컴파일 호스트는 **HJOW-Athlon / 2026-10-20**이므로 지침에 따라 현재 PC에서 10.78 및 CD 10.72 두 판본을 처음부터 다시 처리했다. 이 요청은 사전 작업 범위로 진행했으며, 일시 정지된 cpppj 구현이나 통합 플레이는 재개하지 않았다.

## 정밀 디컴파일

재생성 전에 현재 `extracted/refined/`의 파일 41개, 총 138,067,500바이트를 다음 Git 제외 백업에 복사하고 파일 수와 총 바이트를 대조했다.

`extracted/refined-backup-before-VM-W11-CODEX-20261021/`

전체 절차는 `tools/ghidra/refine_all.ps1`로 실행했다. 10.78판 함수 5,549개, CD 10.72판 함수 5,823개가 표에 기록됐다. CD판의 `FUN_0049f99f` 한 함수 디컴파일 실패는 기존 기록과 동일하다. 두 판본의 함수 대응은 2,496쌍(강 1,738 · 중 707 · 약 51), 호출 규약 일치는 2,313/2,379(97.2%), `ret N` 일치는 2,339/2,389(97.9%)다.

Ghidra 작업은 종료 코드 0으로 끝났다. 분석 중 네 GIF 자원 파싱 `Invalid GIF data` 오류와 여러 비정상 분기/생성자 P-code 경고가 로그에 남았지만, 새 결과는 보존한 기존 내보내기와 비교해 각 판본의 `functions.tsv`, `raw.c`, `conventions.tsv`, `missing.tsv`, `gaps.tsv`, `switches.tsv`, 최종 C 파일 및 대응/등급 파일의 SHA-256이 모두 같았다. 새 작업으로 코드 출력 내용이 바뀌지는 않았다.

| 판본 | 입력 EXE SHA-256 | 함수표 SHA-256 | 원시 C SHA-256 | 최종 C SHA-256 |
| --- | --- | --- | --- | --- |
| 패치 10.78 | `A305414CFE0B94494BB32FEF2B359E920E9DABA225E0821B76AEA1F9F8E7D236` | `3EA34D7CD978AAA5C141A629264FEA5E8B2F78C4E6B0B4E4633F982A2583F658` | `0B5D53B15FDB2EF7AFBCCF61AF666709726A96FB3B972B6818F4A42EFCD31972` | `AF7CBF0B68E9D8E5AEB7068382C88D66201D3906C2926C54909AE14DE4AD5646` |
| CD 10.72 | `613500A394B3062E914604EB7D09896D678192BE24545D0E575082A87C851233` | `453AA383C27BAF8EEFA00D97BF44051D18460150556511C2EB7A46F111E92DEF` | `614884BEA530C070BFB8B5C6629F6C4394D54BADD00E6E73712C9ECDF672DB76` | `F8CF1CA26B0B272C19AD6CC46BE3D21A2654BD3FF697B367265678A224EFFDBB` |

입력 EXE의 재작업 후 SHA-256도 표와 같아 원본 바이너리는 바뀌지 않았다. 전체 로그는 `extracted/refine-vmw11-codex-20261021.log`에 있다.

## Release 컴파일

다음 명령은 종료 코드 0으로 끝났다. 경고와 오류는 각각 0건이며 라이브러리, 실행 파일, 테스트 실행 파일을 컴파일했다.

```powershell
cmake --build cpppj/build --config Release --parallel
```

- `cpppj/build/bin/Release/NetstormCpp.exe` — SHA-256 `D5967860BE87C7854E9CAF7D350F6D807DEBDA9B83DD2FDF8CCE607098FA7EC7`
- `cpppj/build/bin/Release/netstorm_tests.exe` — SHA-256 `F1F2D7079CD56F57BA062642BD233D1CC8A6F821FDB388AB92F1977B5E54CE0F`

테스트 이월 지시에 따라 CTest, 테스트 실행 파일, 게임은 실행하지 않았다. `AGENTS.md`, `originals/`, `originalCD/`, cpppj 소스, dotnetpj 파일은 수정하지 않았다. 빌드 로그는 Git 제외 경로 `extracted/cpp-build-vmw11-codex-20261021.log`에 있다. 마지막 정밀 디컴파일 수행 PC는 이제 **VM-W11-CODEX / 2026-10-21**로 기록한다.

이 사전 작업 단위는 **20261021-23 CP** 로컬 커밋으로 기록한다. 원격 저장소에는 push하지 않는다. 단위261 뒤 일시 정지된 구현 목표는 계속 미완료·정지 상태로 두며, 재개 시 `LEFT_JOBS.md`의 미완료 항목에서 이어간다.
