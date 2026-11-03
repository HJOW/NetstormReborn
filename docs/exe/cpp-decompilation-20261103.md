# 2026-11-03 양판본 정밀 디컴파일과 cpppj Release 컴파일

## 실행 범위와 환경

`AGENTS.md`와 `LEFT_JOBS.md`의 사전 확인 지시에 따라 기존 게임 양판본의 정밀 디컴파일과 `cpppj` Release 컴파일을 수행했다. 호스트는 **HJOW-Athlon (192.168.0.94)**, 날짜는 **2026-11-03**이다. 디컴파일에는 Ghidra **12.1.4_PUBLIC** 및 `tools/ghidra/refine_all.ps1`을 사용했다.

디컴파일은 원본 실행 파일을 Ghidra headless로 읽는 정적 분석이며 게임을 실행하지 않았다. 결과는 Git 제외 경로 `extracted/refined/`에 생성했다. 원본 실행 파일의 SHA-256은 패치판 `A305414CFE0B94494BB32FEF2B359E920E9DABA225E0821B76AEA1F9F8E7D236`, CD판 `613500A394B3062E914604EB7D09896D678192BE24545D0E575082A87C851233`이다.

## 정밀 디컴파일 결과

| 판본 | 최종 함수 수 | 디컴파일 성공 | 실패 | 등급 A/B/C/D | 함수 포인터 표 |
| --- | ---: | ---: | ---: | --- | ---: |
| originals, 10.78 패치판 | 5,549 | 5,549 | 0 | 2,326 / 3,152 / 40 / 31 | 1,652 |
| originalCD, 10.72 CD판 | 5,823 | 5,822 | 1 | 2,315 / 2,939 / 484 / 85 | 1,121 |

함수 대응은 **2,496쌍**(강 1,738 / 중 707 / 약 51), 호출 규약 일치율 **97.2%**, 반환 스택 크기 일치율 **97.9%**다. CD판 디컴파일 실패 1건은 미해결로 남겼다. Ghidra 로그에는 GIF 데이터 오류와 일부 함수의 p-code 흐름 경고가 있었다. 패치판과 CD판의 정밀 결과가 모두 생성됐고, 해당 경고를 성공 함수로 잘못 집계하지 않았다.

## cpppj Release 컴파일

실행한 명령:

```powershell
cmake --build cpppj/build --config Release --parallel 4 -- /p:UseMultiToolTask=true /p:MultiProcMaxCount=4 /p:EnforceProcessCountAcrossBuilds=true
```

MSBuild **18.11.1**로 빌드가 종료 코드 **0**으로 완료됐다. 생성·갱신된 결과는 다음과 같다.

- `cpppj/build/bin/Release/Netstorm.exe` — 10,320,384바이트
- `cpppj/build/bin/Release/NetstormSmacker.dll` — 37,376바이트
- `cpppj/build/lib/Release/netstorm.lib` — 303,950,916바이트

현재 CMake 설정은 `NETSTORM_BUILD_TESTS=OFF`다. 검사 실행 파일은 이 빌드에서 생성되지 않았고, 검사나 게임 실행도 하지 않았다. 실제 게임과 별도 GUI 창은 열지 않았다.

## 인수인계

이번 준비는 cpppj 사전 분석·컴파일만 수행했다. 제품은 기존 815/11.01이며 구현 단위816은 미착수다. 원본과의 전체 동일성·자연 완주 및 남은 기능은 기존 `LEFT_JOBS.md`의 인계 범위를 따른다. 양판본 비교 과정에서 확인된 CD판 디컴파일 실패 1건은 후속 원문 분석 시 미해결 상태로 취급한다.
