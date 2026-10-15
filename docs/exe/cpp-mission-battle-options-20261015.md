# 미션 로그인 옵션의 실제 순수 몸체 (2026-10-15)

현재 PC와 마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon, 2026-10-15**다. 보호 원본과 디컴파일 산출물은 변경하지 않았으며 게임/GUI를 실행하지 않았다.

[`RawMissionBattleOptions`](../../cpppj/src/o/RawMissionBattleOptions.h)는 실제 로그인/클라이언트 수신의 옵션87BYTE, 현재188TYPE, 사용표256BYTE, 현재 범위표3float 및 현재 대상TYPE/파생전역을 빌린다. 성공 표식이나 옵션/TYPE 사본을 만들지 않는다.

| 10.78 주소 | 실제 처리와 현재 API | CD 10.72 대응 후보 |
|---|---|---|
| `004b4860` | 옵션+4의 범위표 선택, 원래 비교 분기, 범위/제곱 쓰기 → `UpdateRange` | `00431630` |
| `0041c8b0` | 옵션+10의 BYTE로 비용1000/2000/3000/5000 조회 → `TypeCost` | `00453750` |
| `0041ca70` | 옵션+0f의 BYTE로0/25/50/75/100/150 조회 → `CurrentValue` | `004537b0` |
| `0041c320` | `005451f8`의64DWORD/256BYTE만0으로 초기화 → `ClearTypes` | `004534a0` |
| `004b4900`의 작은 하위 | 범위→비용 조회→현재 `00541240`TYPE+c4 비용쓰기→파생값 쓰기 → `ApplyDerived` | 부모 후보 `00431680` |
| `0049a840` | 실제 `0059ab20 + type*500` 조회. 제품은 같은 현재 typed188TYPE의 `cost`를 쓴다 | 이번 범위에서 별도 확인하지 않음 |

CD 대응은 현재 정밀 함수표/디컴파일의 대응 후보 기록이며 CD 실행 대조가 아니다. 이번 제품은 PATCH10.78만 받아들인다.

## 현재 저장소와 호출 순서

`MissionBattleOptionState::options`는 실제 `0052f7e0`의87BYTE다. 기존 수신기의 숨김9BYTE는 이 배열+0x3e의 같은 영역이며 다른 별도 배열을 만들면 안 된다. `rangeTable`은 `005425f4`의 현재3float로 기본14/22/30이다. 범위/제곱은 `0052f488/0052f48c`, 파생값은 `005424bc`, 선택TYPE는 현재 `00541240`의 참조다.

`TYPE+c4`는 **실제 `RiftTypeRecord::cost` 비용**이다. 기존 수신 hook의 임시 이름 `requiredEnergy/setEnergyType`만 보고 에너지 사용량 필드에 쓰면 달라진다. `SetSelectedTypeCost`는 쓰기 직전에 현재 선택번호를 읽고 그 typed필드만 바꾼다. `ClearTypes`는256BYTE 사용표의 초기화이며 별도 기술 허용 상태를 만들어 대신하지 않는다.

실제 `RawMissionClientPackets`의0x67 부모에는 다음 작은 소비자를 공급하면 된다. 부모 자체가 먼저 packet 옵션을 복사하고 필요하면 Outpost를 호출한다. 그 뒤 `updateRange=UpdateRange`, `requiredEnergy=TypeCost`, `setEnergyType=SetSelectedTypeCost`, `readOptionValue=CurrentValue` 순서를 보존한다. 부모의 파생값 쓰기는 같은 현재 `value`에 연결해야 한다. 선택된 Outpost/UI 등 큰 외부효과는 이 모듈의 빈 성공으로 대체하지 않는다. `ApplyDerived`는 위 작은 네 작업을 같은 순서로 실행하는 API이며 전체 수신 부모의 복사/외부효과를 구현했다고 주장하지 않는다.

유효 크기는87옵션/188TYPE/256사용표/3float로 고정한다. 손상된 범위/비용/파생값/TYPE 인덱스에서 원본의 인접 스택·전역 임의 읽기를 호스트 범위 밖 접근으로 재현하지 않고 명시 오류로 구분한다. 해당 손상 입력은 정상 원본 대조 수에 합산하지 않는다.

## x87 비교와 관찰기 한계

`004b4860`은 `std::clamp`로 단순 치환하지 않았다. 원문 `fnstsw/test ah/jp`와 `test ah,41/jne`는 unordered 비교가 최소값 대입을 선택하지 않게 한다. 디컴파일 C의 `min <= candidate` 조건을 그대로 옮기면 NaN에서 다른 결과가 나온다. 실제 최소/최대 선택과 저장 순서 및 float제곱 비트를 보존한다.

초기 선택 저장은 `004b4867 FLD m32`→`004b486e FSTP m32`다. 처음 포함한 signaling NaN 입력에서 Unicorn은 `7f800001`을 범위에 그대로 저장했고 곱셈 결과만 quiet로 만들었다. 이를 제품 기대값으로 강제하지 않았다. 현재 CPU에서 x87환경 전체를 저장/복원하고 같은FLD/FSTP쌍을 control word027f로 실행한 별도 진단은 다음을 확인했다.

| 입력 비트 | 현재 CPU 저장 비트 |
|---|---|
| `7f800001` | `7fc00001` |
| `ff800001` | `ffc00001` |
| `7fc12345` | `7fc12345` |
| `ffc12345` | `ffc12345` |
| `80000000` | `80000000` |

해당 진단은 새 도구의 `host_x87_diagnostic`와 보고서에 별도로 남겼고 **원본 함수 정상반환 고유 입력0개**로 구분한다. 원본 게임/OS 진입 코드를 실행하지 않은64bit CPU 명령쌍 진단이며 전체32bit 원본 함수 실행과 합산하지 않는다. 제품은 실제 초기FLD/FSTP의 quiet변환을 명시 보존하며 별도 C++ 두 패턴 검사로 다룬다. 이후 최소/최대 대입은 실제MOV 경로이며 ordered비교가 NaN 피연산자의 대입을 선택하지 않는다. x87 예외상태 플래그나 비정상trap의 일치는 이번 검증 대상이 아니다.

## 독립 원본 대조와 검증 상태

[`관찰 도구`](../../tools/decomp_mission_battle_options.py), [`fixture`](../../cpppj/tests/fixtures/mission-battle-options-x86.tsv), [`SHA 보고서`](../../cpppj/mission-battle-options.json)는 다음 **3,305개 고유 정상반환 입력**을 보존한다.

- 범위3,003개: 기본3선택과 finite/quiet NaN/무한/부호0/양·음 비정규/역전된표의10종3칸×3선택.
- 비용4개, 파생값6개, 사용표 초기화4개.
- 실제0x67 부모288개:3범위×4비용×6파생값×현재TYPE0/1/122/187.

전체 정밀 함수 범위의 실제 명령만 허용하며 **외부 함수 대체0**이다. 부모는 `outpost=0/missionPost=1`의 실제 자연분기를 사용한다. 현재87옵션을 복사하고 실제 작은 하위와 `0049a840`을 실행한다. 각 정상반환에서 cdecl stack/EBX/ESI/EDI/EBP, 실제쓰기주소 guard, 범위와제곱 float비트, 현재 파생값, 전체188비용, 사용표256+양쪽이웃8, 현재옵션87을 보존했다. 함수 진입 순서도 fixture에 남긴다.

[`C++ 검사`](../../cpppj/tests/MissionBattleOptionsTests.cpp)는 원본의 저장 결과 전부를 직접 비교하며 현재 옵션/선택TYPE/범위표의 호출 간 변경과 명시 오류를 추가 검사한다. 원본 진입 trace는 원본 호출 순서 근거로 보존하며 제품에 가짜 관찰 callback을 넣어 새 효과로 만들지 않는다. 정상 원본의 비용 이외 TYPE바이트는 쓰기 guard가 수정 시 실패시키며 제품은 typed cost만 바꾼다.

native 생성/고유성/SHA/UTF-8/후행공백 검사는 통과했다. root가 CMake에 등록해 **Release 경고0·오류0·관련 집중42개 실패0**을 확인했으며 새2개 검사에서3,305행을 전수 대조했다. 전체 회귀 최종 결과는 [중단 인수인계](cpp-pause-handoff-20261015.md)를 따른다. 실제 Session 옵션/사용표 서비스 연결, 로그인 종료부터 Guide/Stage3 및 자연 완주 검증은 미완료다.
