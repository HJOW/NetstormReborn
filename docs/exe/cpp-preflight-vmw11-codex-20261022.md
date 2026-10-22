# cpppj 사전 준비 — VM-W11-Codex 정밀 디컴파일 및 Release 컴파일

작업일은 **2026-10-22**, PC는 **VM-W11-Codex**다. 작업 전 `AGENTS.md`, `LEFT_JOBS.md`, `LEFT_JOBS.dotnetpj.md`를 확인했다. 직전 기록의 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-21**로 현재 PC와 달라, 지침에 따라 패치판 10.78과 CD판 10.72를 모두 다시 디컴파일했다. 이번 사용자 요청은 cpppj 사전 준비 범위이며, 일시 중지된 구현 목표나 실제 플레이 검증을 재개하지 않았다.

## 기존 산출물 보존

재생성 전 `extracted/refined/`의 기존 결과를 `extracted/refined-backup-before-VM-W11-Codex-20261022/`에 복사했다. 원본과 백업은 각각 **41개 파일·137,969,202바이트**로 일치한다.

## 정밀 디컴파일

저장소의 전체 절차 `tools/ghidra/refine_all.ps1`을 실행했다. 환경은 Ghidra **12.1.4**, Temurin OpenJDK **21.0.12.1**, Python **3.12.4**, Windows PowerShell **5.1**이다. Python 콘솔 인코딩 문제를 피하기 위해 UTF-8 환경 변수를 설정했다. 전체 절차 종료 코드는 **0**이다.

```powershell
$env:PYTHONUTF8 = '1'
$env:PYTHONIOENCODING = 'utf-8'
& .\tools\ghidra\refine_all.ps1
```

| 판본 | 함수 | 결과 | 최종 C SHA-256 |
| --- | ---: | --- | --- |
| 패치판 10.78 | 5,549 | 성공 5,549 · 실패 0 | `AF7CBF0B68E9D8E5AEB7068382C88D66201D3906C2926C54909AE14DE4AD5646` |
| CD판 10.72 | 5,823 | 성공 5,822 · 실패 1 (`FUN_0049f99f`) | `F8CF1CA26B0B272C19AD6CC46BE3D21A2654BD3FF697B367265678A224EFFDBB` |

판본 대응은 **2,496쌍**(강 1,738 · 중 707 · 약 51)이며, 호출 규약 힌트 8개(패치판)와 9개(CD판)를 적용했다. CD판의 단일 실패는 기존 기록과 같은 `FUN_0049f99f`의 `Cannot properly adjust input varnodes`다.

원본 입력의 SHA-256은 작업 전후 동일하다.

- `originals/Netstorm.exe`: `A305414CFE0B94494BB32FEF2B359E920E9DABA225E0821B76AEA1F9F8E7D236`
- `originalCD/NETSTORM.EXE`: `613500A394B3062E914604EB7D09896D678192BE24545D0E575082A87C851233`

정밀 결과와 재생성 전 백업은 모두 Git 제외 경로 `extracted/`에 있다. 디컴파일 로그는 `extracted/refine-vmw11-codex-20261022.log`에 보관했다.

## Release 컴파일

다음 명령은 CMake 구성을 갱신한 뒤 **종료 코드 0**으로 끝났다. 빌드 로그에 컴파일 단위 330개가 기록됐고, 경고·오류는 확인되지 않았다.

```powershell
cmake --build cpppj/build --config Release --parallel
```

| 산출물 | SHA-256 |
| --- | --- |
| `cpppj/build/lib/Release/netstorm.lib` | `2DBFF2FBC0E9F1844F911FB1DA2D1D86B9170FE270072701495F29DB7EA17DE5` |
| `cpppj/build/bin/Release/NetstormCpp.exe` | `CBF8CDD974F44DD85B64FFC13606032729E4A52FEC81B1E610896914E435A985` |
| `cpppj/build/bin/Release/netstorm_tests.exe` | `F9BAB5C7A7B812B47D1A17F3C016761CBE717B20D3FA6BDF2F743C948145A0C9` |

테스트 실행 파일은 컴파일했지만 CTest·단위 테스트·게임 실행은 하지 않았다. cpppj 소스와 원본 게임/CD 파일은 수정하지 않았다.

## 인수인계

최신 정밀 디컴파일 PC는 **VM-W11-Codex / 2026-10-22**다. 이번 사전 준비는 구현 목표의 재개가 아니다. 일시 중지된 cpppj 다음 작업은 `00417bd0` 다리 전략의 정밀 범위·분배 및 하위 함수 소유 관계 확인부터 이어간다. 전체 미션 플레이와 Edit/custom Fort·시나리오 수명 및 표시·음향 동일성 목표도 계속 미완료다.
