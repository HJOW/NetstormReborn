# 사제 소유자 재정의 복원

2026-10-08, 마지막 디컴파일 수행 PC **VM-W11-CODEX**. 같은 PC의 기본 Ghidra 프로젝트에서 `priestowner`를 세 판본에 각각 읽기 전용으로 내보냈다. 1차 기준은 10.78이며 원본 게임/복사본·클론 창을 실행하지 않았다.

[`RawPriestOwner`](../../cpppj/src/o/RawPriestOwner.h)는 **Priest의 소유자 지정 재정의 전체**를 복원한다. 원래 소유자와 로컬 표식 비트의 쓰기, 공통 `SquidOwner::Set` 호출 여부, 무상태 알림 순서를 보존한다. [`MakePriestOwnerDispatch`](../../cpppj/src/o/RawPriestOwner.h)를 기존 섬/종유석 소유자 분배기와 합성할 수 있다. GUI walker의 raw 전환·사제 postPop·포획 플레이는 후속이다.

## 원본 주소·필드

| 내용 | 10.78 | CD/추가 10.37 |
| --- | --- | --- |
| 사제 소유자 지정(`+0x74`) | `00491790` | `0040bda0` |
| 사제 가상 표 | `0050f210` | `005003e0` |
| 공통 소유자 지정 | `004adf00` | `004aefa0` |
| 단어 변경 알림 | `004214a0` | `00448c10` |
| 요새 읽기 중첩 값 | `005c89b8` | `005178d4` |
| 요새/전투 로컬 번호 | `00540cac` / `00540c70` | `0050f6d4` / `0050f6c8` |

실제 두 판본의 priest 타입은 **158**이다. raw `+12` WORD의 **low 7비트는 원래 소유자**, 비트 7은 위 모드별 로컬 번호와 같을 때 켜는 표식이다. WORD의 high byte(`+13`)는 보존한다. **현재 소유자**는 별도 필드이며 패치 `+34`, CD `+32` BYTE다.

## 처리 순서와 판본 차이

1. **비전투 또는 요새 읽기 중**에는 0이 아닌 요청값으로 원래 소유자의 low 7비트를 바꾸고 알림을 부른다. 기존 표식과 high byte는 보존한다. **전투 중이며 읽기 중첩 값이 0**이면 요청값을 무시하고 저장된 원래 소유자를 사용한다.
2. 적용할 번호가 정해지면 **공통 소유자 지정**을 부른다. 이 호출은 현재 소유자 BYTE를 바꾸며 원래 소유자 WORD와 구별된다.
3. 읽기 중첩 값이 0이거나 전투 중이면 로컬 번호 일치 여부로 비트 7을 켜거나 끈 뒤 다시 알림을 부른다. 읽기 중 비전투에서는 이 단계가 생략되어 이전 표식을 보존한다.

세부 차이도 원본 그대로 유지한다.

- **패치판:** 원래 소유자를 갱신하는 분기의 요청 0은 `newPlayerId != INVALID_PLAYER_ID` assert다. 범위 밖의 0이 아닌 요청은 WORD의 low 7비트와 최종 표식/알림을 처리하지만 **공통 지정은 생략**한다. 음수로 해석되는 DWORD도 같은 경로다.
- **CD/10.37:** 같은 분기의 요청 0은 WORD와 현재 소유자 지정을 생략한다. 마지막 표식 처리는 모드에 따라 수행한다. 0이 아닌 범위 밖 번호로 공통 지정에 들어가면 일반 모드에서 assert다.
- **전투 중 요청 무시:** 요청 0이나 `0xffffffff`도 저장된 원래 소유자를 쓰므로, 무시되는 요청을 미리 검사하여 거부하지 않는다.
- **공통 알림:** 패치판은 다리 타입에서만 단어 범위를 진단하고, CD는 슬롯 주소를 반환한다. 실제 사제에서는 상태를 바꾸지 않는다. 표시 무효화나 포획 이벤트로 확대 해석하지 않는다.

호스트는 free/예약 SID와 타입/가상 표 불일치를 효과 전에 거부한다. 원본 assert 입력은 변경 전 예외로 바꾼다. 기존 `SquidOwner`의 지원 플레이어 범위는 0~8이며, challenge의 9~39는 이 모듈에서도 지원하지 않는다. 현재 challenge 모드의 0 거부만 보호 검사로 확인했다.

## 독립 제한 x86 대조

[`decomp_priestowner_oracle.py`](../../tools/decomp_priestowner_oracle.py)는 기존 공통 소유자 실행기의 PE/메모리/허용 범위 검사를 재사용한다. 새 [`priestowner-functions.json`](../../tools/ghidra/priestowner-functions.json)은 판본마다 사제 재정의와 알림 **2개 함수**만 추가한다.

**대체 함수가 없다.** 사제 몸체 전체·공통 소유자 지정·타입 조회·알림을 실제 명령으로 실행하고 정상 반환/thiscall 스택 복구를 검사한다. 공통 지정과 알림의 진입 시점에 인자/WORD/현재 소유자를 관찰한다. 쓰기는 WORD와 현재 소유자 BYTE, 스택에만 허용한다. 게임/OS/API/업데이터 실행은 없다.

| 판본 | 입력 수 | 공통 지정 진입 | 알림 진입 | assert |
| --- | ---: | ---: | ---: | ---: |
| 10.78 | 3,528 | 1,584 | 5,256 | 0 |
| CD | 1,944 | 1,584 | 2,448 | 0 |
| 추가 10.37 | 1,944 | 1,584 | 2,448 | 0 |
| 합계 | **7,416** | 4,752 | 10,152 | **0** |

요새/전투 네 조합, 읽기 중첩 0/1/3, 원래 소유자 0/1/2/5/8/127와 표식·high byte, 요청 0/1/3/8/9/127/128/`0x80000000`/`0xffffffff`, 두 로컬 번호·현재 소유자·state/extra를 포함한다. 입력 격자에서 원본 assert 경로를 제외했으므로 판본별 행 수가 다르다. 함수는 정수 필드만 계산한다.

[`priestowner-x86.tsv`](../../cpppj/tests/fixtures/priestowner-x86.tsv)의 **사건 순서·인자·호출 시점 WORD/소유자·슬롯 전체**가 C++과 일치한다. [`recovery-priestowner-evidence.json`](../../cpppj/recovery-priestowner-evidence.json)은 PE/기존·새 내보내기/생성 도구/입력 목록/fixture SHA, 실제 진입/명령 수와 assert/대체/OS 호출 0을 기록한다. 일반 CTest는 원본 파일/Python/Ghidra 없이 저장 fixture만 사용한다.

## 실제 raw 모듈 통합·검증

[`PriestOwnerTests.cpp`](../../cpppj/tests/PriestOwnerTests.cpp)는 실제 사제 생성자와 `SquidOwner`를 두 판본에서 연결한다. 로딩 중 원래 소유자 지정 → 전투 중 요청 무시 → 로컬 번호 일치 표식 설정 → 비전투 로딩 중 표식 보존을 순차 검사한다. 사제 분배기와 기존 섬/종유석 분배기 합성, 공통 fallback, 누락된 효과·원본 assert·잘못된 raw 슬롯의 변경 전 거부도 검사한다.

**Pop/postPop·Graph·사제 생성 플레이·표시/GUI는 실행하지 않았다.** 이 검사는 소유자 지정과 실제 생성자/공통 소유자 연결의 근거다. GUI의 사제 이동은 아직 기존 어댑터다.

Release 최종 빌드 **경고/오류 0**, CTest 내부 **272개·실패 0**(기존 266 + 새 6, 62.21초). 관련 감사 **4종 모두 통과**(priestowner/owner/bridgeconnect/pathanimation). 새 x86 7,416개를 더해 누적 **145,006개**다. 최초 빌드의 새 소스 namespace 닫기 누락을 수정한 뒤 최종 빌드/회귀를 통과했다. 로그는 `extracted/priestowner-export.log`, `priestowner-generate.log`, `build-priestowner-final.log`, `ctest-priestowner.log`에 있다.

```powershell
powershell -ExecutionPolicy Bypass -File tools/ghidra/export_functions.ps1 -Name priestowner
python -X utf8 tools/decomp_priestowner_oracle.py
python -X utf8 tools/decomp_priestowner_oracle.py --verify
cmake --build cpppj/build --config Release
ctest --test-dir cpppj/build --build-config Release --output-on-failure
```

## 다음 구현

다음은 **사제 postPop `004950f0`/CD `0040c110`**이다. 최초 등록에서 사제 목록에 추가하고 regen Regular(`0x25a`)가 없으면 붙이며, 원래/현재 소유자가 같으면 플레이어 상태를 초기화한다. 이후 carrier postPop(`00427720`/CD `004e6360`)을 호출한 뒤 지면·방향에 따라 낙하/경로 처리를 한다. 회복 payload의 타입/HP 조회·float 나눗셈과 판본별 실제 판단을 별도로 대조한다.

preDestroy `004919b0`/CD `0040c330`도 목록·종속 처리와 carrier 훅에 연결해야 한다. 소유자 모듈만으로 공통 Pop에 사제 가상 표를 허용하면 전용 효과가 빠진다. 해당 lifecycle/process 검증 뒤 raw 유닛 월드로 옮기고 PathProcess 보행 진행/종료, 낙하·파편/소리·배치·건설·경제·전투·승패를 이어 간다.
