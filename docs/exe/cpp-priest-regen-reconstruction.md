# 사제 회복 이벤트 0x25a 복원과 Kernel 연결

2026-10-08, 기준 판본 **10.78**. 마지막 디컴파일 수행 PC: **HJOW-Athlon**(IP 192.168.0.94). AGENTS.md와 두 인계 문서를 읽고 마지막 디컴파일 PC와 현재 호스트가 같음을 확인했다. 새 `priestregen` 목록을 같은 PC에서 읽기 전용으로 내보냈다(10.78 **10개**, CD/추가 10.37 각각 **11개**). 이전 `prieststate`·`owner` 내보내기도 재사용한다. 정밀 디컴파일을 다시 하지 않았으며 원본 게임/복사본·클론 창 실행·보호 파일/AGENTS.md/dotnetpj 변경·커밋/푸시 없음.

[`RawPriestRegen`](../../cpppj/src/o/RawPriestRegen.h)은 사제 처리기 `00494580`/CD `0040ccd0`의 **회복 0x25a 분기**를 복원하고, 기존 [HP setter](cpp-priest-state-reconstruction.md)와 [Regular/Kernel](cpp-process-reconstruction.md)을 연결한다. HP 변경·중립 조건·패치 난수/추적 필드 쓰기는 실제 구현이다. **공간/표시·damageable·중립 동작·패치 추적 검증/측정의 하위 몸체는 외부 훅**이다. 전체 사제 이벤트 처리기·Pop/postPop/preDestroy·보호막/낙하·GUI의 완성은 후속이다.

| 동작 | 10.78 | CD / 추가 10.37 |
| --- | --- | --- |
| 회복 이벤트를 포함한 처리기 | `00494580` | `0040ccd0` |
| 실제 HP setter | `004942b0` | `0040ca40` |
| 최대 HP getter | `004adcb0` | `004aed30` |
| 사제 타입 최대 HP | `0049fce0` | `00444720` |
| 중립 후처리 가능 조건 | `00491a80` | `0040c400` → `0040c3d0`/`0040c3b0` |
| 회복 시 가상 표시 요청 | vtable `+0x88` | vtable `+0x88` |
| damageable 후처리(외부) | `0044c060` | `00462480` |
| 타입의 공간 자료 조회(외부) | `00488fe0(type)` | `0047d020(type)` |
| 점유 조회(외부) | `004b1a90(sid, shape, 0, 0)` | `004eb310(sid, shape, 0, 0)` |
| 중립 동작(외부) | `00491db0` | `0040d030` |
| 난수 / 추적 검증 / 측정 | `004558c0` / `00491680` / `0040ee90` | 없음 |

## 회복과 후처리 순서

패치는 먼저 기존 **GameRandom::Next(9)**를 한 번 소비한다. CD에서는 난수를 소비하지 않는다. 이후 현재 HP가 객체 최대 HP보다 작을 때 다음을 실행한다.

1. **사제 타입 최대 HP / 6**을 현재 HP에 더한다. HP 1/4 모드는 기존 `SquidReward`를 통해 적용하며 정수 나눗셈은 0방향 절삭이다.
2. 후보 값이 객체 최대 HP보다 크면 최대 HP로 제한한다. 원본의 DWORD ADD는 signed 32비트 감기이므로 C++ 부호 오버플로를 피한 비트 계산으로 보존했다. 큰 HP에서는 감긴 값을 먼저 비교한다.
3. 실제 `RawPriestState::SetHitPoints`를 실행한다. CD는 실제 저장 폭인 signed WORD로 경계를 평가한다. 비권한 제한/소유자 회복 거부도 기존 setter에 맡긴다.
4. HP가 거부되거나 증가량이 0이어도 **회복을 시도한 경로이면 +0x88 → damageable 후처리**를 부른다. 처음부터 HP가 가득 차 있으면 두 호출을 생략한다.

패치 전용 추적 검증을 호출한 뒤, 두 판본 모두 중립 조건을 검사한다. **권한 있음·차단 전역 두 개가 0·현재 소유자 0·현재 HP가 최대 HP/2 이상**일 때 타입의 공간 자료를 조회하고, 점유 결과가 0이면 중립 동작을 요청한다. 이때 HP/소유자/타입·전역은 **앞선 가상/외부 효과 뒤 현재 값**으로 읽는다. HP가 가득 차 있어도 중립 후처리는 실행할 수 있다. 두 차단 전역의 구체적인 게임 내 의미와 중립 동작 하위 효과는 아직 확정/복원하지 않았다.

payload의 float 비트는 그대로 반환하고 count는 읽지 않는다. 따라서 기존 Regular는 양수 payload로 다시 예약한다. 0·음수·NaN payload의 Regular 처리 규칙은 기존 모듈을 따른다. 회복량을 payload나 호출 횟수에 곱하지 않는다.

## 패치 전용 추적 전역의 폭

`PriestRegenPatchState`는 현재 확인한 raw 필드에 이름을 붙인 것이며 UI/게임 규칙의 전체 의미를 주장하지 않는다.

- `00540cb8`: Next(9) 결과.
- `0054d4e0`: signed DWORD에 3을 더하고 9600보다 크면 9600을 **한 번** 뺀다. 32비트 감기도 유지한다.
- `00568af8`이 위 sequence와 다르면 `005318ec`에 현재 sample을 쓴다.
- `005c89c4`가 0이 아니면 외부 x87 측정 결과가 **7000.0f**보다 클 때 sample을 쓰고, DWORD를 하나 줄인다. NaN 비교는 거짓이다.
- `00594fd0`이 0이거나 **`00531890`의 signed WORD가 -1**이면 sample을 쓴다. sentinel을 DWORD로 읽지 않는다.

외부 `00491680`은 SP/추적 상태에 접근하므로 호출 시점과 뒤의 전역 재조회만 연결한다. 그 함수의 SP 초기화/검증 전체나 `0040ee90`의 실제 측정 몸체는 이번 구현에 포함하지 않는다. 측정 훅은 double로 표현 가능한 결과를 공급하며 x87 전체 수치 영역을 복제하지 않는다.

## 분배·검사와 원본 대조 경계

최종 Release **경고/오류 0**, CTest 내부 **294개·실패 0**(기존 289 + 새 5, 90.47초). 감사 **37종 모두 통과**. 새 4,539개를 더해 누적 독립 x86 **184,321개**다. 최초 대조에서는 CD판의 호출 없는 관찰을 `-`로 비교하는 검사 표현을 고쳤고, 중립 분기 입력을 독립 교차로 보강한 뒤 전체 검사를 다시 통과했다. 로그는 `extracted/priestregen-export.log`·`priestregen-generate-final.log`·`priestregen-build-final.log`·`priestregen-ctest-final.log`·`priestregen-audits-final.json`이다.

`MakePriestRegenHandler`는 **실제 사제 vtable의 0x25a만** 기존 ProcessHost에 연결한다. 다른 사제 사건은 fallback이 없으면 미복원 예외다. 비사제는 명시적 fallback 또는 기존 base의 payload 반환을 사용하므로 다리 분배기와 합성할 수 있다. 일반 `SquidPop`의 사제 허용 범위는 바꾸지 않았다.

풀 불일치·필요한 외부 효과 누락은 생성 시 거부한다. 직접 실행의 잘못된 이벤트·raw SID/타입/vtable은 난수/HP 변경 전에 거부한다. 외부 훅 실행 뒤의 오류는 원자적 복구를 보장하지 않으며, 외부 효과를 빈 훅으로 공급한다고 전체 사제 공간/표시가 복원되는 것은 아니다.

[`decomp_priestregen_oracle.py`](../../tools/decomp_priestregen_oracle.py)는 실제 세 PE에서 **회복 분기의 진입부터 정상 반환까지** 실행한다. HP getter/setter·좌표 보정·중립 조건·패치 난수와 전역 쓰기는 실제 명령이다. **Unpop/Repop·+0x88·damageable·공간 자료/점유·중립 동작·패치 검증/측정**만 기록 대체한다. 합성 x87 호출부는 측정 입력을 올리고 최종 float 반환 비트를 관찰하는 데만 사용한다. 게임/OS는 실행하지 않는다.

판본마다 **1,513개**, 총 **4,539개**다(기본 HP/폭/권한/소유자 격자 1,440 + 표시 뒤 변경 9 + 중립 조건 독립 교차 64). 두 x87 정밀도의 결과가 같으며 실행 횟수를 사례 수에 중복 집계하지 않는다. HP 폭·1/4 모드·최대 HP/6·절반 경계·signed ADD 감기·가득 찬 HP·비권한/회복 거부·중립 차단/점유·외부 HP/소유자/추적 변경·난수 0 시드·추적 순환/감기·WORD sentinel·측정 경계/NaN·payload 비트를 대조한다. 모든 판본에서 공간 자료/점유/중립 동작 진입이 실제로 관찰되어야 감사가 통과한다.

[`priestregen-x86.tsv`](../../cpppj/tests/fixtures/priestregen-x86.tsv)의 반환 비트·사건 순서/인자/호출 시점 HP/소유자/좌표·추적 전역·최종 난수·raw 슬롯 전체를 C++과 비교한다. [`recovery-priestregen-evidence.json`](../../cpppj/recovery-priestregen-evidence.json)에 호스트·PE/도구/목록/내보내기/fixture SHA, 정상 반환/thiscall 스택/x87 복구와 assert/OS 호출 0을 기록했다.

[`PriestRegenTests.cpp`](../../cpppj/tests/PriestRegenTests.cpp)는 저장 관찰 재생 외에 두 판본 server/client **실제 사제 생성자 → postPop prefix의 회복 예약 → ProcessForm/Kernel → 실제 HP setter**를 검사한다. HP 80→113→146→179→200, 첫 절반 경계의 Unpop/Repop 요청, 원래 payload 8.25의 재예약, 가득 찬 뒤 예약 유지, form 삭제/해제를 확인한다. 공간 효과는 여기서도 기록 훅이며 실제 공간 Pop/GUI를 실행한 검사는 아니다.

```powershell
powershell -ExecutionPolicy Bypass -File tools/ghidra/export_functions.ps1 -Name priestregen
python -X utf8 tools/decomp_priestregen_oracle.py
python -X utf8 tools/decomp_priestregen_oracle.py --verify
cmake --build cpppj/build --config Release --parallel 4
ctest --test-dir cpppj/build -C Release --output-on-failure
```

다음은 사제 preDestroy `004919b0`/CD `0040c330`의 목록 제거·종속/Carrier 호출을 복원하거나, 보호막 `00493d30`/CD `0040bfb0`·낙하 `004941f0`/CD `0040c880`의 공간 효과를 진행하는 것이다. SharedRegular 0x25b를 일반 Regular로 대체하지 않는다. 기존 장시간 변이·최대 지도·SID 소진·로드 재시도 검사는 계속 인계한다.
