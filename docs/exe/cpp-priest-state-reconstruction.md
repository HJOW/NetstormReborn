# 사제 HP·지면 상태 조회와 HP 변경 복원

2026-10-08, 기준 판본 **10.78**. 마지막 디컴파일 수행 PC: **HJOW-Athlon**(IP 192.168.0.94). AGENTS.md와 두 인계 문서를 읽고 현재 호스트가 마지막 내보내기 PC와 같음을 확인했다. 새 `prieststate` 목록을 같은 PC에서 읽기 전용으로 내보냈다(10.78 **11개**, CD/추가 10.37 각각 **12개**). 원본 게임/복사본·클론 창 실행·보호 파일/AGENTS.md 변경·커밋/푸시 없음.

[`RawPriestState`](../../cpppj/src/o/RawPriestState.h)는 현재 HP와 지면 기반 이동 불가 상태, 사제 HP setter의 전체 분기 순서를 복원한다. HP setter의 **Unpop/Repop 공간 효과는 외부 훅**이다. 사제 전체 Pop·회복 이벤트·보호막·낙하·preDestroy·비표면 GUI 연결은 후속이다.

| 동작 | 10.78 | CD / 추가 10.37 |
| --- | --- | --- |
| 이동 불가 wrapper | `00427030` | `004e5150` |
| HP·지면 상태 조회 | `00492090` | `0040d7b0` |
| 사제 HP setter | `004942b0` | `0040ca40` |
| raw HP 저장 helper | `004adca0` | `004aed20` |
| HP 전환 좌표 보정 helper | `0041d7a0` | `00440a80` |
| 최대 HP getter | `004adcb0` | `004aed30` |
| 사제 vtable | `0050f210` | `005003e0` |
| 후속 이벤트 처리기(이번에 실행하지 않음) | `00494580` | `0040ccd0` |

## 조회 순서와 판본 차이

현재 HP는 raw +26에 있다. 패치는 **signed DWORD**, CD/10.37은 **signed WORD**이며 옆의 stale 두 바이트를 읽거나 쓰지 않는다. 최대 HP는 기존 `SquidReward::MaxHitPoints`를 재사용한다. 사제 HP 1/4 모드와 signed 정수 나눗셈을 유지하며 보상 지급/SP 저장을 호출하지 않는다.

지면 조회는 genus `0x200000`이 없으면 false다. 현재 HP가 최대 HP/2보다 작으면 좌표를 읽지 않고 true다. 나머지 경로에서는 x/y에 원본 float **0.9999f**를 더한 뒤 0방향으로 절삭한다. x87 중간 합을 float으로 다시 저장하지 않는 점을 double 계산으로 보존했다. 해당 spot의 `& 6`이 0이거나 extra `0x20`이면 true다. 외부 wrapper는 extra `0x20`을 먼저 검사하므로 genus·HP·좌표를 읽지 않고 true일 수 있다.

## HP setter의 절반 경계

1. 기존 현재 HP와 최대 HP/2를 읽고 새 값을 원본 폭으로 저장한다. **CD는 저장한 signed WORD 값으로** 경계를 판단한다. 경계의 같은 쪽이면 가상 호출 없이 끝난다.
2. 권한이 없으면 기존 상태가 저HP일 때 `half-1`, 그 외에는 `half`로 제한해 경계 전환을 막는다.
3. 권한이 있고 저HP에서 회복하는 전환이면 현재 소유자의 Player +0x7c/CD +0x74를 확인한다. 0이면 이전 HP를 복원한다. `recoveryAllowed`는 이 필드가 0이 아닌 조건의 이름이며 게임 내 업그레이드 의미는 아직 확정하지 않았다.
4. 허용된 전환은 **Unpop(sid, 0)**을 호출한다. 그 뒤 현재/최대 HP를 다시 읽어 저HP이면 현재 x/y에 float **0.99999f**를 더하고 0방향 절삭한 float으로 저장한다. 이어 **Repop(sid, 0)**을 호출한다. 이 배율은 지면 조회의 **0.9999f와 다르다.** Unpop 훅이 HP/좌표를 바꾸는 입력도 별도로 대조했다.

같은 풀의 HP 모듈·판본별 타입 수·256×256 spot을 요구한다. 예약/free/contained SID와 자산 밖 타입은 거부한다. setter는 실제 사제 타입 **158**과 vtable을 확인한다. 필요한 소유자 범위·훅 누락은 HP 쓰기 전에 거부하지만, **Unpop 뒤 잘못된 좌표를 발견하는 경우에는 이미 HP 저장/Unpop이 실행되었으므로 원자적 복구를 보장하지 않는다.** 지면을 실제로 읽는 경로는 유한하고 범위 안인 좌표만 지원한다. 사용하지 않는 NaN 좌표는 short circuit 검사로 읽지 않음을 확인했다.

## 독립 기계어 대조

최종 Release **경고/오류 0**, CTest 내부 **289개·실패 0**(기존 284 + 새 5, 92.95초). 감사 **36종 모두 통과**. 새 20,256개를 더해 누적 독립 x86 **179,782개**다. 최초 C++ 대조에서 HP 전환 좌표의 0.99999f 보정값 누락을 발견해 고친 뒤 전체 검사를 다시 통과했다. 로그는 `extracted/prieststate-build-final.log`·`prieststate-ctest-final.log`·`prieststate-audits.json`이다.

[`decomp_prieststate_oracle.py`](../../tools/decomp_prieststate_oracle.py)는 세 실제 PE의 조회/setter와 타입/HP getter·`_ftol` 명령을 실행한다. **조회는 대체 함수가 없고, setter는 vtable +0x48/+0x4c의 공간 효과 두 개만 기록 대체**한다. 각 몸체의 정상 반환/스택, x87 제어 워드 유지·빈 스택, 허용된 raw HP/좌표 밖 쓰기 금지를 검사한다. assert/OS 호출은 0이다.

판본마다 **6,752개**, 총 **20,256개**다(지면 조회 **8,160**, wrapper **8,160**, setter **3,936**). 두 x87 정밀도 `0x027f`/`0x037f`의 결과가 같아야 하며 사례 수에는 두 실행을 중복 집계하지 않는다. HP/2와 1/4 모드·음수/홀수 최대 HP·DWORD/WORD 부호 경계·genus·extra·spot·거의 정수인 좌표·권한/회복 허용·소유자 0/8·외부 HP/좌표 변경을 대조한다. 체커보드 spot 입력은 절삭/거의 올림과 한 축의 잘못된 칸 조회를 구별한다.

[`PriestStateTests.cpp`](../../cpppj/tests/PriestStateTests.cpp)는 저장된 [`prieststate-x86.tsv`](../../cpppj/tests/fixtures/prieststate-x86.tsv)의 반환값·호출 순서/인자·호출 시점 HP/좌표·raw 슬롯 전체를 C++과 비교한다. 원본/Python/Ghidra 없이 CTest에서 재생 가능하다. 별도 보호 검사는 사용하지 않는 좌표의 short circuit, 필요한 지면 조회의 범위 거부, 누락된 훅/소유자/가상 표의 쓰기 전 거부를 확인한다. [`recovery-prieststate-evidence.json`](../../cpppj/recovery-prieststate-evidence.json)에 호스트·PE/도구/목록/내보내기/fixture SHA와 실제/대체 진입 수를 남겼다.

```powershell
powershell -ExecutionPolicy Bypass -File tools/ghidra/export_functions.ps1 -Name prieststate
python -X utf8 tools/decomp_prieststate_oracle.py
python -X utf8 tools/decomp_prieststate_oracle.py --verify
cmake --build cpppj/build --config Release --parallel 4
ctest --test-dir cpppj/build -C Release --output-on-failure
```

2026-10-08 후속(`HJOW-Athlon`): [회복 이벤트 0x25a](cpp-priest-regen-reconstruction.md)를 이 HP setter·기존 ProcessForm/Kernel에 연결했다. 실제 회복/중립 조건/난수와 외부 효과의 호출 순서를 독립 x86 4,539개로 대조했다. 공간/표시·중립 동작·패치 검증/측정의 하위 몸체와 전체 사제 이벤트/Pop은 후속이다. 일반 사제 Pop 허용은 전용 공간/보호막/낙하/postPop/preDestroy가 갖춰진 뒤 진행한다. SharedRegular 낙하 0x25b를 일반 Regular로 대체하지 않는다. 기존 장시간 변이·최대 지도·SID 소진/로드 재시도 검사는 계속 인계한다.
