# 건설 끝 시각 재설정과 프로세스 검증 마무리

2026-10-11, **HJOW-Athlon**. AGENTS.md·두 LEFT_JOBS를 다시 읽었다. 시작 트리는 깨끗하고 현재/마지막 디컴파일 PC가 같았다. 착수 당시 인계에는 시작/진행/완료가 다음 항목이었지만 저장소 `f468820`에는 해당 코드와 독립 원본 자료가 이미 있었다. 작업 도중 [기존 복원 기록](cpp-construction-process-reconstruction.md)의 635개 검사·16개 변이 검증과 README·LEFT_JOBS 마무리 기록을 확인했다. 이번 단계에서는 끝 시각 변경을 이어서 복원하고 기존 흐름을 포함해 전체 641개를 검증했다.

`constructiontime` **3/9/9개**를 읽기 전용으로 내보냈다. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-11.** 원본/복사본 게임·analyzeManager·클론 창은 실행하지 않았다.

## 실제 재설정 동작

| 흐름 | 10.78 | CD/10.37 |
|---|---|---|
| form을 받는 재설정 전체 | `00443dc0` | `004d0930` |
| 건설 객체의 재설정 | 위 함수에 인라인 | `004d0970` |
| form의 Kernel 번호 조회 | `004ae2d0` | `004af4e0` |
| Kernel 조회 | `00471aa0` | `004ed640` |
| 부모 조회 / 최대 HP | `0041bfb0` / `004adcb0` | `0048f870` / `004aed30` |
| 기본 시간 helper | 직접 현재+10 쓰기 | `004d08a0` |
| 끝 시각 필드 / 변경 비트 | +`0x18` / +8의 bit 4 | 동일 |

두 판본 모두 form raw +`0x12`의 **DWORD 전체**를 Kernel 번호로 읽고, 조회한 프로세스의 건설 객체와 부모를 찾는다. 최대 HP를 조회한 뒤 **현재 게임 시각+10.0**을 끝 시각으로 쓰며 변경 비트 4를 켠다. 시작 시각·소리 항목·부모 HP·다른 변경 비트는 보존한다. 이 경로에서는 생성/소리 재생/화면 갱신/건설 완료를 호출하지 않는다.

패치판은 두 번째 스택 인자와 최대 HP 반환값을 사용하지 않는다. CD판은 `절삭((현재-시작)/(끝-시작)×최대 HP)-인자`를 DWORD 뺄셈으로 계산하고 signed 결과가 1 이하면 1로 제한한다. 하지만 이어 호출하는 `004d08a0`은 부모/HP 인자 둘을 **읽지 않고 현재 시각+10초를 저장**한다. 따라서 이 계산값으로 새 지속 시간을 바꾸거나 부모 HP를 깎으면 원본과 달라진다. Ghidra 본문뿐 아니라 실제 x86 명령과 helper의 `ret 8`까지 확인했다.

## C++ 연결과 계약

[`RawConstructionProcess`](../../cpppj/src/o/RawConstructionProcess.h)에 `ResetEnd(fields,parent,damage)`와 `ResetEndFromForm(form,damage)`를 추가했다. 첫 API는 복원된 계산/쓰기 몸체이며, 두 번째는 실제 host에서 form을 조회해 해당 건설 프로세스의 필드와 부모를 넘긴다. CD의 정수 뺄셈은 unsigned DWORD 감김 뒤 signed 비교로 옮겨 C++ signed overflow를 피한다. `DefaultTime`은 CD helper의 무시되는 인자와 끝 시각 쓰기를 표현한다.

[`SquidProcessHost::FromForm`](../../cpppj/src/o/SquidProcess.h)은 현재 form의 DWORD 번호로 실제 Kernel의 프로세스를 조회한다. 빈 슬롯/범위 밖 번호/다른 BaseProcess는 null이다. 재설정 API는 host 누락, 건설 프로세스가 아닌 결과, 현재 프로세스가 가진 form과 요청 form의 불일치를 필드 쓰기 전에 거부한다. 원본의 null 프로세스 접근을 실행하지 않는 호스트 안전 계약이다. core `ResetEnd`의 필드/부모 참조는 호출자가 유효하게 공급해야 한다.

진행 비율은 유지된 시작 시각과 새 끝 시각으로 계산한다. 새 끝은 재설정 당시 현재 시각+10초이며, 기존 끝 시각에 따라 완료가 늦춰지거나 앞당겨질 수 있다. 실행 중인 실제 `ConstructionProcess`와 같은 `ConstructionProcessState.now`를 공급해야 한다. 별도 게임 시각을 갖는 host/소리 계층은 기존처럼 같은 월드 시각으로 동기화해야 한다.

[`ConstructionTimeTests.cpp`](../../cpppj/tests/ConstructionTimeTests.cpp)의 새 검사 **6개**는 세 PE 관찰을 실제 host 부착/Kernel 조회로 재생한다. 원본과 동일한 form 5/Kernel 번호 7을 만들어 최대 HP 경계 호출·시각 비트·dirty·부모 raw·form 불변을 비교한다. 추가로

- 실제 Kernel 프레임에서 기존 완료 시각에는 프로세스/abstract가 남고, 새 끝 시각에는 실제 Kill/form 삭제 통지/완료가 실행된다.
- 최대 HP 경계가 현재 시각·시작/끝·dirty·소리 필드를 바꾸는 입력을 재조회하거나 보존한다.
- `0x10007` Kernel 번호를 WORD 7로 감지 않는다. 빈 슬롯·다른 종류의 프로세스·오래된 form·host 누락을 거부하고 기존 필드를 유지한다.

실제 부착·form 조회·Kernel·Kill/공통 삭제·건설 진행/완료는 연결했으며, 이 검사의 최대 HP/소리/표시·부모 Unpop/재등록은 명시 경계다. 전체 GUI에서 건설한 관찰은 아니다.

## 독립 원본 관찰과 검증

[`decomp_constructiontime_oracle.py`](../../tools/decomp_constructiontime_oracle.py)는 각 PE의 재설정 전체와 form/부모 getter, CD 진행 계산/뺄셈/CRT/기본 시간 helper를 정상 반환까지 실행한다. **Kernel 조회와 최대 HP만 명시 응답 경계**이며 실제 번호/부모 인자와 호출 횟수를 확인한다.

각 PE **423개**, 새 저장 **1,269개**, 두 x87 정밀도의 정상 반환 **2,538회**다. 유한 합성 시각, 시작/끝이 같은 경우·음수 구간, 최대 HP 0/100/-1, damage DWORD 0/1/100/최상위 비트/전체 비트, 여러 dirty 값과 경계 중 현재 자료 변동을 교차했다. 두 제어값 639/895에서 관찰이 같았고 스택/보존 레지스터/x87 균형·허용 실행/쓰기·부모/form 불변을 확인했다. 원본 assert·게임·OS 호출은 0이다. CD Pentium FDIV 우회 전역은 0이다. [fixture](../../cpppj/tests/fixtures/constructiontime-x86.tsv)와 [근거 JSON](../../cpppj/constructiontime.json)의 SHA·입력/필드/raw 폭·진입/정상 반환/경계 집계도 감사한다.

이번에 완료 검증한 기존 시작/진행/완료 자료 **2,868개**를 포함해 누적은 **484,599개**(480,462 + 2,868 + 1,269)다. 기존 자료를 새 재설정 자료에 중복으로 세지 않는다. 기존 건설 프로세스 12개와 소리 연결 1개, 새 끝 시각 6개를 합쳐 인계 기록 이후 내부 검사는 19개 늘었다.

Release 경고/오류 **0**·CTest 내부 **641개·실패 0**(137.29초, 전체 137.32초), 근거 감사 **99종 모두 통과**. 새 끝 시각 변이 6개와 기존 건설 프로세스 변이 16개, 총 **22개 모두 검출**(`--focused`, 변이 전 사본은 전체 검사 통과).

## 다음 작업

전송용 상태 쓰기/읽기/수신과 가상 쓰기 래퍼는 이후 [건설 상태 복원](cpp-construction-data-reconstruction.md)에서 완료했다. 지정 form 부착·0x37 디스패처, 건설 표시 효과(`00423840`/`00423900`), contained 삭제(`004b1a90`/CD `004eb310`), 타입 준비 소리(+`0x90`), 가상 표시/Unpop/재등록의 실제 연결이 남는다. 이어 하위 경로/접근점·자원 선택/정렬·소유 사제 이동/contained, 타입별 파생 효과·메시지 전파/수신·법적 위치/미리보기·건설 메뉴/커서 GUI를 복원한다. **게임 내 건설과 미션 완주는 아직 불가능하다.**

```powershell
powershell -ExecutionPolicy Bypass -File tools/ghidra/export_functions.ps1 -Name constructiontime
python -X utf8 tools/decomp_constructionprocess_oracle.py --verify
python -X utf8 tools/decomp_constructiontime_oracle.py --verify
cmake --build cpppj/build --config Release
cpppj/build/bin/Release/netstorm_tests.exe --filter ConstructionTime_
ctest --test-dir cpppj/build -C Release --output-on-failure
python -X utf8 tools/cpp_mutation_check.py --focused --only time-form-pid-width
```

로그는 Git 제외 `extracted/constructiontime-*.log`다. 변이는 별도 사본에서만 실행한다. AGENTS.md·보호 원본·dotnetpj 변경, 커밋/푸시는 없다.
