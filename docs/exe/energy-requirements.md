# 건설 에너지 요구와 판정 (패치판 exe)

> 근거: `originals/Netstorm.exe`의 `Rifttype.cpp`·`Mana.cpp` 디컴파일(`extracted/decomp/Netstorm.c`) 및 `extracted/tarc/d/*.type`. 정적 분석 결과이며 게임 내 동적 재현은 아직 하지 않았다.

## 타입별 요구값

타입 로더는 `.type`의 `theme`을 타입 구조체 `+0x98`에, `level - 1`을 `+0x94`에 저장한다. `mana` 속성이 있으면 문자열을 `+0xA0`에 복사한다. `FUN_0049b0d0`은 이 문자열이 비었을 때 아래 기본값을 만든다. 실행 파일의 원소 문자 표(`0x540ad8`의 포인터)는 `wrtsWRTS.?>`로, `w`·`r`·`t`·`s`가 Wind·Rain·Thunder·Sun이다.

| `.type` level | Sun 타입 | Rain 타입 | Wind 타입 | Thunder 타입 |
|---|---|---|---|---|
| 1 | `s` | `r` | `w` | `t` |
| 2 | `ss` | `rs` | `ws` | `ts` |
| 3 | `sss` | `rrs` | `wws` | `tts` |

`rainBattery`·`windBattery`·`thunderBattery`는 `level = 1`이어도 모두 **`mana = "s"`**를 명시한다. `outpost`에도 같은 속성이 있다. 로더가 먼저 이 값을 복사하므로 `FUN_0049b0d0`의 기본값 생성은 건너뛴다. 따라서 발전기는 자기 원소가 아닌 **아무 원소 공급원 한 개**를 요구한다.

패치판의 level 1 원소 유닛 중 `bulf`와 `thunderFence`는 `t`, `rainwalker`는 `r`이 된다. 원판 매뉴얼의 Bulf 설명(Thunder 1)과 공식 PDF 매뉴얼의 유닛별 "Energy to Build" 규칙이 이 생성 규칙과 일치한다([PDF 대조표](../sources/pdf-manual.md#유닛별-건설-에너지-energy-to-build)). **사용자도 Bulf는 Thunder 공급원이 반드시 필요하다고 확인했다(2026-09-29).** 이전 기록의 일반식 `자기 원소 × (level - 1) + Sun 1`은 폐기했다. 구현 기준은 보유 패치판 exe와 `.type`의 요구 문자열이다.

## 배치 위치 판정

`FUN_004734d0`(`Mana.cpp`)은 배치할 위치와 타입을 받아 `FUN_004730c0`으로 그 위치를 덮는 공급원들을 수집하고, 타입 `+0xA0`의 요구 문자열을 `FUN_00473330`으로 검사한다. `FUN_00412b90`의 배치 후보 검사와 `FUN_0040f2b0`의 생산 위치 검사에서 이 함수를 호출한다.

`FUN_00473330`은 `w`·`r`·`t`를 같은 문자 공급원에만 대응시킨다. `s`는 아직 사용하지 않은 임의의 원소 공급원에 대응시킨다. 대응한 공급원 문자를 `.`으로 바꾸므로 한 공급원을 요구값 두 개에 중복 사용하지 않는다. `FUN_004730c0`에서는 공급원별 가상 함수로 대상 위치의 범위 포함 여부를 확인하고, 소유자 또는 동맹 관계도 걸러낸다. 이는 [사용자가 확인한 에너지 공급·중복 금지 규칙](../gameplay/elements-energy.md)과 맞는다.

디컴파일에서 `FUN_00473330` 호출 인자가 일부 복원되지 않아, 특수 모드의 우회 조건과 모든 배치 경로까지 확정한 것은 아니다. 일반적인 타입 요구값과 배치 후보 검사 경로는 확인했다. level 1 원소 유닛이 자기 원소 공급원을 요구한다는 점은 사용자 확인(Bulf)으로 확정했으므로 별도 동적 검증 항목에서 뺀다.

## 근거 위치

| 항목 | 주소 / 파일 |
|---|---|
| `mana` 문자열 복사 | `Rifttype.cpp`의 타입 속성 파서, `Netstorm.c` 약 104288행 |
| 기본 요구값 생성 | `FUN_0049b0d0`, 약 102525행 |
| 공급원 수집 | `FUN_004730c0`, 약 80130행 |
| 요구 문자 대응 | `FUN_00473330`, 약 80246행 |
| 배치 검사 연결 | `FUN_004734d0`, 약 80362행 |
| 명시 요구값 | `rainbattery.type`·`windbattery.type`·`thunderbattery.type`·`outpost.type` |
