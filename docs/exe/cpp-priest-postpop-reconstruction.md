# 사제 postPop의 목록·회복 예약 복원

2026-10-08, 기준 판본 **10.78**. 마지막 디컴파일 수행 PC: **VM-W11-CODEX**(현재 호스트 `VM-W11-Codex`, IP 10.0.0.17). 같은 PC의 Ghidra 프로젝트를 읽기 전용으로 열어 `priestpostpop` 목록을 내보냈다. 10.78은 5개, CD/추가 10.37은 각각 6개다. 원본 게임/복사본·클론 창을 실행하지 않았으며 보호 파일·AGENTS.md·dotnetpj를 변경하거나 커밋/푸시하지 않았다.

[`RawPriestPostPop`](../../cpppj/src/o/RawPriestPostPop.h)은 사제 postPop의 **carrier 호출 전 구간**을 복원한다. 전체 가상 postPop은 아직 구현하지 않았으므로 공통 `SquidPop`의 허용 가상 표나 GUI raw 유닛 월드에는 등록하지 않았다. 이전 [소유자 복원](cpp-priest-owner-reconstruction.md)에 이어 실제 사제 생성자·공통/사제 소유자·회복 ProcessForm/Kernel까지 콘솔에서 연결한다.

| 동작 | 10.78 | CD / 추가 10.37 |
| --- | --- | --- |
| 사제 postPop 진입 | `004950f0` | `0040c110` |
| 실행 중단 경계: carrier 진입 전 | `00427720` | `004e6360` |
| 중복 없는 사제 목록 등록 | `00414450`, 목록 `005954c4` | 몸체 inline, 목록 `00549188` |
| 회복 Regular 검색 | `004afb40` | `004ac890` |
| Regular 생성자 | `00496e80` | `0048efd0` |
| 타입/객체 최대 HP | `0049fce0` / `004adcb0` | `00444720` / `004aed30` |
| 소유자 상태 칸 | `00595428 + owner*4` 직접 쓰기 | `0040d9c0`, `005119a0 + owner*4` |

## 구현한 순서

실제 사제 타입은 **158**, 가상 표는 `0050f210`/CD `005003e0`이다. extra는 +40/CD +35이고, `extra & 9`가 0일 때만 아래 효과를 실행한다. 일반 raw 상태 바이트(+11)의 void/contained와는 별개다.

1. `flags & 1`이면 사제 목록의 활성 구간을 검색해 SID가 없을 때 끝에 넣는다. 용량이 가득 차면 그대로 유지한다. 용량 뒤의 같은 SID는 중복으로 세지 않는다. 목록 등록 실패나 중복 여부와 관계없이 회복 검색은 계속한다.
2. 이벤트 **`0x25a`**인 Regular가 이미 있으면 기존 payload/시각을 유지한다. 없으면 원본 `new(0x28)` 확보 경계를 지난 뒤에만 HP를 조회하고 회복을 예약한다. 확보 실패는 HP 조회·생성만 생략한다.
3. 원래 소유자 WORD(+12)의 low 7비트와 현재 소유자 BYTE(+34/CD +32)가 같고 현재 소유자가 **1~8**이면 해당 DWORD 상태 칸을 0으로 만든다. 칸 0과 나머지 칸은 보존한다. 이 동작은 `flags & 1`과 무관하다.

CD는 소유자가 0이어도 일치하면 helper를 호출하지만 helper 자체가 1~8만 쓴다. C++은 최종 쓰기 조건으로 합쳐 동일한 상태를 만든다. 회복 생성 훅 이후 현재 소유자를 다시 읽어 쓰기 순서를 유지한다.

## 회복 payload와 기존 모듈 연결

원본 두 PE의 배율 DWORD는 `0x42480000`, float **50.0**이다. payload는 현재 HP를 사용하지 않는다. 최대 HP 조회는 기존 [`SquidReward`](../../cpppj/src/o/SquidReward.h)의 두 읽기 함수를 공개하여 재사용했다. 사제 최대 HP 1/4 모드와 기존 패치판 debug assert, 객체 최대 HP의 타입 분기를 새로 복제하지 않았다. 보상 지급·SP 저장은 호출하지 않는다.

```text
정수 q = 사제 타입 최대 HP / 6       // 0방향 절삭
float numerator = float(q * 50.0)   // x87 곱셈 후 실제 float 저장
float payload = float(numerator / 객체 최대 HP)
```

원본 `FILD`는 signed int를 정확히 올리므로 곱하기 전에 q를 float으로 줄이지 않는다. 분모도 int를 float으로 줄이지 않는다. C++은 정확히 표현 가능한 곱을 double로 계산한 후 중간 float 저장과 최종 float 저장을 보존한다. HP 200이면 **8.25**, 1/4 모드의 HP 50이면 **8.0**이다. 현재 HP를 42로 고정한 독립 관찰에서도 같은 결과를 얻는다.

`MakePriestPostPopProcessHooks`는 기존 `SquidProcessHost::FindEvent/AddRegular`에 연결한다. 실제 부착은 로컬 flags `0x50`, 타입 46의 ProcessForm, 부모 종속 체인, Kernel 슬롯 및 현재 시각을 사용한다. 원본의 할당 실패 분기는 `reserveRegular` 훅으로 주입할 수 있다. 기본 연결은 확보 성공을 전제로 하며 호스트 메모리 부족은 C++ 할당 예외로 전파한다. 원본 malloc의 메모리 부족 처리 전체를 복제한 것은 아니다.

## 독립 원본 대조

[`decomp_priestpostpop_oracle.py`](../../tools/decomp_priestpostpop_oracle.py)는 세 실제 PE 각각에서 postPop 진입부터 carrier **진입 직전**까지 실행한다. 목록·최대 HP·CD 소유자 쓰기 helper는 실제 명령이다. **Regular 검색·new·Regular 생성자만** 입력/기록 대체한다. carrier 진입의 this/flags와 x87 스택이 비어 있는지 검사하며, 원본 함수 전체의 정상 반환이나 SEH 복구를 주장하지 않는다. 허용 범위 밖 명령과 스택/FS/list/owner 칸 밖 쓰기를 거부한다.

각 판본 **2,392개**, 총 **7,176개**다. 두 x87 정밀도 `0x027f`/`0x037f`에서 flags/extra/state·소유자/원래 WORD·목록 중복/포화·기존 회복·확보 실패·HP 1/4·큰 값/음수 HP를 관찰한다. [`priestpostpop-x86.tsv`](../../cpppj/tests/fixtures/priestpostpop-x86.tsv)의 목록 저장소·모든 소유자 칸·호출 순서/인자·payload 비트·raw 슬롯 전체를 C++과 비교한다. 유효 최대 HP 0, CD의 Pentium FDIV 보정 경로, 사제 전용 이벤트 처리기는 이번 격자에 포함하지 않는다.

[`recovery-priestpostpop-evidence.json`](../../cpppj/recovery-priestpostpop-evidence.json)은 PE/내보내기/도구/입력 목록/fixture SHA와 실제/대체 진입 수, 중단 수 및 assert/OS 호출 0을 기록한다. 실제 최대 HP 진입 수가 예약 수와 같고 타입 HP 진입 수가 그 두 배인지 감사한다. 일반 CTest는 원본/Python/Ghidra 없이 저장 fixture만 읽는다.

## 통합·검증 범위

[`PriestPostPopTests.cpp`](../../cpppj/tests/PriestPostPopTests.cpp)는 세 판본 재생 외에 두 raw 배치의 server/client 생성자·실제 소유자 지정·회복 부착을 합성한다. 최초 예약, 같은 이벤트의 중복 방지, Kernel 실행/재예약, 기존 payload 유지, form 삭제/종속 체인 해제, HP 모드 변경 뒤 새 회복 예약을 검사한다. 이 통합 검사의 부모 이벤트 처리기는 기록/재예약용 훅이며 **사제의 실제 HP 회복 효과를 구현한 것은 아니다**. 잘못된 SID·타입·가상 표·손상 목록·누락 효과·HP 모듈 풀 불일치의 효과 전 거부도 검사한다.

최종 Release **경고/오류 0**, CTest 내부 **278개·실패 0**(기존 272 + 새 6, 57.75초). 관련 감사 **5종 통과**(priestpostpop/priestowner/process/reward/owner). 새 7,176개를 더해 누적 독립 x86 **152,182개**다. 최초 통합 검사의 client 생성에 필요한 flags 2 누락을 고친 뒤 전체 회귀가 통과했다. 로그는 `extracted/priestpostpop-export.log`, `priestpostpop-generate.log`, `build-priestpostpop-final.log`, `ctest-priestpostpop-final.log`에 있다.

```powershell
powershell -ExecutionPolicy Bypass -File tools/ghidra/export_functions.ps1 -Name priestpostpop
python -X utf8 tools/decomp_priestpostpop_oracle.py
python -X utf8 tools/decomp_priestpostpop_oracle.py --verify
cmake --build cpppj/build --config Release
ctest --test-dir cpppj/build --build-config Release --output-on-failure
```

## 다음 구현

carrier postPop(`00427720`/CD `004e6360`)의 비권한 상태 가상 호출 및 damageable/common postPop부터 이어 간다. 그 뒤 지면·방향에 따른 사제 낙하/경로 분기와 회복 이벤트 처리기(`0x25a`)를 복원한다. preDestroy의 목록 제거·종속 처리, 보행 진행/종료도 남는다. 이 전용 흐름을 갖춘 뒤 사제를 공통 Pop·비표면 raw 월드/GUI에 연결해야 한다. 장시간 변이/최대 지도/SID 소진 검사는 기존 15분 이상 작업 인계를 유지한다.
