# 사제의 실제 참여 상태와 포획 fallback — 2026-10-15

현재 PC와 마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon, 2026-10-15**로 일치한다. 작업 전에 AGENTS.md와 양 LEFT_JOBS를 다시 확인했다. 기존 full/refined의 원문과 보호10.78 PE를 읽었으며 재디컴파일·원본 게임·GUI·OS 호출은 없다. AGENTS·보호 원본·dotnetpj를 수정하지 않았다. CMake 등록·공용 빌드·LEFT 갱신·커밋은 통합 담당자가 수행한다.

## 복원한 동작

`RawPriestParticipation`은 원본 `00494f10`과 `00495060` 전체를 기존 같은 월드의 사제 목록·raw 풀·타입 표·4단계 해시와 연결한다. 포획됐다는 외부 bool이나 결과 SID를 받아 성공으로 처리하지 않는다.

| 순서 | 실제 원문 | 복원한 읽기와 결과 |
|---|---|---|
| 1 | `004921f0` | 기존 `RawPriestLookup`을 그대로 호출한다. signed 활성 개수와 목록 순서를 따르고 현재 owner BYTE가 player DWORD와 같고 dead bit2가 없는 첫 SID를 즉시 반환한다. free·void·타입·extra 필터를 추가하지 않는다. |
| 2 | `0040eb40`, `00494f10` | live 결과가0일 때만 SID2부터 풀을 순회한다. 초기 end를 보존하며 free bit1만 건너뛴다. dead·void·contained와 예측 영역을 별도로 제외하지 않는다. |
| 3 | `004ac200`, `004ac9f0` | 현재 후보 타입의 genus `flags2 & 0x30000`이 있으면 기존 `RawCarrierPreDestroy::FindContained(parent,0x20,currentPriestType,0)`를 호출한다. 내부의 기존 `RawContainedFinder`가 headWORD6→nextWORD4와 현재 containedType을 사용하고 첫 mask/kind 일치를 반환한다. |
| 4 | `00494f10` | 반환된 종속의 `+0xc BYTE & 0x7f`가 전체 player DWORD와 같으면 **Carrier SID를 즉시 반환**한다. 그 종속의 원소유자가 다르면 같은 Carrier의 두 번째 일치 종속을 다시 찾지 않는다. |
| 5 | `00448bb0`, `004b1fa0` | Carrier 성공이 없고 후보의 현재 타입이 currentAltarType이면 현재 좌표를0방향 절삭하고 currentDaisType의 flags2에 따른 일반 `RawSquidFinder`를 실행한다. 실제4단계 버킷과 buried bit8을 사용하고 해당 타입 첫 결과를 반환한다. |
| 6 | `00494f10` | 첫 Dais가 있고 dead bit2가 없으며 **Altar 자체의 `+0xc WORD 전체`가 player DWORD와 같으면** Altar SID를 저장하고 계속 순회한다. 끝까지 Carrier가 없으면 **마지막 일치 Altar**를 반환한다. 첫 Dais가 죽었으면 뒤의 다른 Dais로 대체하지 않는다. |
| 7 | `00495060` | 다시 실제 `Find`를 수행한다. 결과0이면2, 결과의 현재 타입이 currentPriestType이면0, 나머지는1이다. 현재 타입 읽기 이후 player1..8에만 기존 `PriestPostPopState::ownerState`를 쓴다. player0/9/음수 비트도 조회는 수행하며 쓰기만 생략한다. |

Altar의 `DaisAt`은 현재 Session에 아직 설치되지 않은 알터 전체 의식/해방 객체를 필수 생성하지 않도록, 기존 `RawSquidFinder`의 전체 typed 조회를 직접 조합하는 생성자를 제공한다. 실제 `RawAltarSacrifice`가 설치된 월드는 마지막 인자에 그 객체를 전달하면 **기존 `DaisAt` 몸체를 직접 호출**한다. 두 경로 모두 동일 풀을 검사하며 외부 선택 결과나 포획 bool을 공급받지 않는다. 일반 typed 조회의 좌표 helper `0041d770`은 거의 올림이 아닌 실제0방향 절삭이다.

## 후속 실제 로그인 종료에 연결하는 방법

```cpp
RawPriestParticipation participation(
    core.pool, core.hash, core.types, priestState.priests, actualCarrier,
    {currentPriestType, currentAltarType, currentDaisType});
participation.Refresh(owner);
```

각 타입 인자는 실제 장기 `uint32_t` 저장소의 참조다. 목록은 사제 PostPop/PreDestroy와 같은 `priestState.priests.priests`, 결과는 같은 `priestState.priests.ownerState`를 사용한다. `Pool()`과 `Priests()`로 동일성을 읽기 전용 확인할 수 있다. 후속 `004b67c0` 로그인 종료는 준비된 같은 월드의 이 모듈을 호출해야 한다. 실제 Session의 설치/API와 보호 월드 검사는 [별도 상세](cpp-session-priest-participation-20261015.md)를 따른다. 자연 로그인 종료·Guide·Stage3를 완료로 표시하지 않는다.

일반 모듈의 안전 계약은 기존 하위 몸체와 같다. 손상된 양수 목록 개수/풀 밖 SID/종속 순환/현재 타입 범위/비정상 좌표는 실제 접근할 때 예외로 거부한다. live 첫 성공 뒤의 손상된 꼬리와 fallback 전역은 읽지 않는다. 동시 Reset이나 다른 스레드의 raw 저장소 재확보는 지원 계약에 넣지 않는다.

## 원본 기계어 관찰

`tools/decomp_priest_participation.py`는 보호10.78 PE의 아래 **19개 전체 함수**만 허용한다. 기능/조회/기본 가상 필터/CRT를 Python 결과로 대체하지 않는다. Ghidra full/refined `functions.tsv`의 불연속 몸체 범위를 사용하며 그 밖의 실행은 즉시 실패한다.

`00494f10`, `00495060`, `004921f0`, `0040eb40`, `004ac200`, `004ac9f0`, `00448bb0`, `004b2270`, `004b1b20`, `004b1fa0`, `004b16d0`, `004b1810`, `0041d770`, `0049a840`, `0049a900`, `00434380`, `004b2b00`, `004e49c0`, `0044daa0`.

원본과 C++ 모두 **24,000개 슬롯 전체**를 사용한다. 합성 입력의 raw 슬롯·목록·현재 타입·타입 속성·버킷 등록만 준비하며, 판정값은 기계어에서 읽는다. 두 x87 제어워드 `0x027f/0x037f`마다 Find/Refresh를 각각 실행해 정상 반환·cdecl 스택·보존 레지스터·x87 균형을 검사한다. 제품 쓰기는 Refresh의 ownerState1..8 DWORD만 허용하고 풀·타입·해시·목록 전체의 불변을 확인한다. 풀/타입/해시/목록 읽기 횟수는 native 통계로 저장하지만 C++의 개별 메모리 명령 수가 같다는 근거로 사용하지 않는다.

무저장 smoke **8입력·32정상 반환·19개 전체 함수 모두 진입** 이후, 전체 fixture **304개 고유 입력·두 정밀도·1,216정상 반환**을 생성하고 `--verify` 종료0을 확인했다. 실제19개 전체 함수 모두 진입했고 대체0·OS0·GUI0이다. 실제 읽기 통계는 풀17,005,120·타입3,600·해시1,896·목록416이다. C++ 빌드 결과는 아래 마감 기록으로 구분한다. 기존 live lookup fixture를 새 관찰 수에 중복 합산하지 않는다.

입력은 live의 타입/free/contained/extra와 dead 구별, DWORD 소유자 폭, mask/kind/현재 containedType, 포획 BYTE의 상위 비트, 첫 일치 종속 이후 원소유자 불일치, 첫 Carrier와 마지막 Altar 우선순위, Dais의 free/dead/void/contained/buried, 네 해시 단계, 풀 마지막 SID23999, signed live 개수, 현재 타입0/다른 타입을 포함한다.

### 검사와 제한

`PriestParticipationTests.cpp`의3개 검사는 전체304행의 반환·ownerState9DWORD·전체 raw/목록 checksum 및 불변·실제4단계 해시 불변을 대조하고, live 조기 성공 뒤의 읽기 생략/DWORD 소유자와 다른 풀/종속 순환 거부를 추가로 검사한다. Carrier 삭제 효과는 빈 성공 콜백으로 넘기지 않고 호출되면 명시 예외를 낸다. CTest는 저장된 fixture만 읽으며 PE/Python/게임을 실행하지 않는다.

CD10.72의 `0040ef50/0040f0e0`은 같은 순서를 가진다. Carrier 포획 소유자는 WORD를 읽고 `&0x7f`를 적용하며, Altar WORD 비교는 `0040ef20`, 결과 쓰기는 `0040d9c0` 하위 함수로 분리돼 있다. BYTE/WORD에서 낮은7비트의 결과는 같다. C++ 하위 모듈은 판본별 owner/extra 배치를 구분하지만 **새 fixture는10.78 전용**이며 CD 실행 대조를 새로 수행했다고 주장하지 않는다.

304개 입력이 실제 미션의 자연 진행을 입증하지는 않는다. 실제 Pop 등록·게임/GUI·로그인 전체·자동 Stage3는 이 단위의 관찰 범위에 포함되지 않는다. Native 보고서 `cpppj/priest-participation.json`의 `files`는 원본 관찰 출처이며 이후 통합 소스의 역사 스냅샷과 구분한다.

## 마감 기록

- 전체 native 생성·`--verify`: **304개·1,216정상 반환·종료0**.
- CMake 등록·최종 Release 빌드: **경고0·오류0**. 새 독립3개와 실제 Session2개를 포함한 **집중60개 실패0**이 통과했다. `extracted/goal-mission-entry-final-{build,focused}.log`를 보존했다.
- 전체 CTest: **1/1·내부1,283개 실패0·216.33초**. `extracted/goal-mission-entry-{ctest,tests}.log`를 보존했다.
