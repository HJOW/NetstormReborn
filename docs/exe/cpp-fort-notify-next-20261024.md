# Notify의 첫 파일명 접근과 최소 수정 제안

2026-10-24 읽기 전용 감사와 격리 사본 검증이다. 현재/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**으로 같다. 기존 `RawPlayerTemplateState`·검사·CMake·LEFT·공유 빌드·GUI·git은 변경하지 않았다. [제안 patch](../../extracted/cpp-fort-notify-20261024-proposed.patch)는 공유 소스에 미적용이며 root가 후속 적용 여부를 판단한다.

## 실제 차이와 원문 순서

[원문](../../extracted/refined/originals/Netstorm.c):150825~150857의004bd360은 **155BYTE**다. 처음 명령은 `manager=*this` 뒤 **manager+0x0c의 첫BYTE 읽기**다. 빈 정상 C문자열의 첫NUL은 유효하다. manager0은 그보다 뒤의 manager 진단이 아니라 **주소0x0c의 물리읽기 실패**에 먼저 도달한다. header+8/+10이 비영이어도 이 첫 접근을 건너뛰지 않는다.

1. 첫 파일명BYTE가`.`이면 Template.cpp **0x1c1**을 진단한다.
2. 현재 header+8이 비0이면 현재+c가 비0일 때 **0x1c3**을 진단한다. 복귀 뒤 **+8을 다시 읽고 비0이면 반환**한다. 진단이+c가 아니라+8을0으로 바꿨으면 아래로 계속한다.
3. 현재 inMission00594fa0이 비0이면 **0x1c6**을 진단한다.
4. 현재 manager가0이면 **0x1c7**을 진단한다. 이 진단은 **처음에는 유효했던 manager가 앞 callback에서0으로 바뀐 경우**에도 도달할 수 있다.
5. 현재 header+10 preventSave가 비0이면 반환한다.
6. 현재 manager를 다시 읽어 **같은 manager를 this로0044d8d0에 tail jump**한다. 미연결/실패한 실제 저장을 정상 성공으로 치환하지 않는다.

현재 [Changed](../../cpppj/src/o/RawPlayerTemplateState.cpp):78~89의+8/+c/미션/manager/+10/Save 분기는 위 순서와 같다. 차이는 첫 두 줄이다. `names_(slot).empty()` 선행 throw는 유효한 첫NUL을 금지하며, manager0일 때 names callback에 따라 먼저 이름을 읽거나 뒤 MissingNotify 진단까지 진행할 수 있다. 정상 이름도 names callback을 두 번 호출한다. 이를 그대로 원본 물리 접근이라고 설명할 수 없다.

## patch의 최소 범위

제안은 첫 두 줄을 다음 효과의 여섯 줄로만 바꾼다. **뒤 분기는 그대로**다.

- slot0..8의 호스트 물리 계약을 먼저 검사한다.
- 첫 manager0은 **이름 callback/원문 진단 전에** `out_of_range`로 거부한다. 이는 원본의 실제0xc 미매핑 읽기를 안전한 호스트 경계로 표현한 것이며 원본 진단0x1c7을 먼저 호출하지 않는다.
- 현재 이름을1회 조회하고 **`name.c_str()[0]`**를 읽는다. 빈 `std::string`도 첫NUL이 있으므로 정상이며`.`만 기존 진단을 호출한다.

초기 manager 검사를 뒤 모든 재조회에 반복하지 않는다. 파일명 진단이 manager0/+8비영으로 바꾼 뒤 자연 반환하는 경우도 원문에 있다. 뒤 manager0 진단이 복귀하면 현재 preventSave/Save 경계를 그대로 수행한다. 이름 getter는 같은 현재 manager의 순수 조회여야 하며 임의 callback 부작용을 원본 인라인 메모리 읽기라고 간주하지 않는다.

## 최초 독립 원문과 격리 제품 검증

[도구](../../tools/decomp_fort_notify.py)는 실제155BYTE를 실행하고 첫파일명BYTE 읽기, 진단/0044d8d0 진입, callback 변경과 부분 중단을 기록한다. 초기 manager0에는 **0페이지를 매핑하지 않아 실제 미매핑 읽기**를 관측했다. 진단/SaveFile 본체만 명시 하위 장치이며 OS·게임은 실행하지 않는다. `save:0` 진입 관측은 하위 파일 쓰기 성공이 아니다.

제품 patch 작성 전에 [fixture](../../cpppj/tests/fixtures/fort-notify-x86.tsv)와 [보고서](../../cpppj/fort-notify-native.json)를 고정했다. **798입력×두x87제어어=1,596관측, 정상1,388/명시중단80/첫물리경계128,38,036명령,정밀도차이0**이다. 이름 첫BYTE1,468회·Save진입336회·진단2,074회를 포함한다. fixture SHA256은 **058ffe03a7e6ab151467ca350915d682ab84aab8d56f51083842eb3fac583576**이다.

입력은 빈/점/일반/첫NUL을 가진 비어 있지 않은 문자열, manager0/두 비영manager, 각 관문의DWORD0/1/80000000/ffffffff와 진단 후 manager/+8/미션/prevent/파일명 변경이다. 여러 진단의 순차 변경·+8재조회·진단별 중단·Save진입 중단을 포함한다. 결과는 정상/중단/물리경계, 현재inMission과 **Template180+두manager128BYTE 전체**, 첫이름읽기/진단/저장 인자 trace다. 원본 SEH/unwind 복원을 주장하지 않는다.

[FortNotifyTests.cpp](../../cpppj/tests/FortNotifyTests.cpp)는 최초1,596행 모두와 빈정상이름의 **선택된 Save 장치 누락 실패 유지**, 미선택+8/+10 반환을 검사한다. 등록 매크로는 **NETSTORM_FORT_NOTIFY_FIXTURE**다. 기존 공유 source를 `extracted/cpp-fort-notify-20261024-isolated/source/`에 복사한 뒤 **그 사본에만** 제안 patch를 적용했다. 현재 버전alias ABI로 빌드된 `cpppj/build/lib/Release/netstorm.lib`는 읽기 링크했다. 해당 lib의SHA/시각과 사본·공유 소스의 각SHA는 격리 `verification.json`에 기록한다.

격리 Release는 관측 컴파일/링크 경고·오류0이며 **신규2+기존PlayerRegular3=5검사/실패check0**이다. 신규1,596행/128물리경계/80중단 및 기존 양판본329행씩과 같은 Host0→5→65초 사슬을 통과했다. 로그는 격리 `build.log`, `tests.log`다. 실제 파일 writer 성공을 가짜로 채운 검사가 아니며 신규 fixture의 Save child는 명시 외부 경계다.

## 기존 회귀와 원문 근거의 영향

| 항목 | 영향과 확인 상태 |
|---|---|
| `PlayerRegularTests.cpp`의3검사 | 기존 양판본 fixture는 비영manager·일반/점 이름이다. 기대 변경 없이 격리 patch와 모두 통과했다. 새 빈이름/manager0 입력은 별도 fixture로 보존한다. |
| `player-regular-x86.tsv`658행 | **변경하지 않았다.** SHA `e3ef582f5da9c62f3235eb16f30bd76b97f9db1a5bff0728070f7ac83ccea932`다. 이번은10.78 추가 관측이며 CD의 새 경계 입력을 별도 관측했다고 하지 않는다. |
| `EditorFilesTests.cpp` 두 Notify 연계 검사 | 유효 비영manager/비어 있지 않은 실제작업경로이며 저장 callback·후속 관문은 그대로다. 이번에는 공유 build/test-data 쓰기를 피하기 위해 재실행하지 않았고 root 적용 뒤 관련 파일회귀가 필요하다. |
| State Initialization/Expand 검사 | Get/State 생성·최소 확장 본체와API는 변경하지 않는다. 비어 있는 다른 슬롯 이름 배열을 가진 것 자체는 해당 Notify를 호출한 것이 아니므로 기대를 바꾸지 않는다. |
| `decomp_player_regular_oracle.py`·`analyze_player_regular_native.py` | 과거 report는 당시 제품소스SHA도 고정한다. 최초 report의RawPlayerTemplateState.cpp SHA는 이미 현재 공유소스와 다르다. 과거자료/기대값을 덮어쓰지 않고 이번 새proof로 patch 변경을 연결한다. |
| 실제 Session Notify/파일쓰기 | 빈 정상 이름 허용은 빈 파일 저장 성공 보장이 아니다. 선택된 실제 파일 장치가 실패하면 계속 실패한다. BindFileSave·작업 namespace·inMission 실제 전역 공유는 [Session 바인딩](cpp-fort-snapshot-session-bindings-20261024.md)의 후속이다. |

root가 적용할 때는 제안 patch의 context/기준SHA, 신규 검사 등록과 관련 실제 파일회귀를 확인한다. **공유 소스 원본SHA는01fb2787ae07e232a8dad80e0d2e3e8a034d87319f3d16e2d0dec98587815b15**, 격리 patch 사본은b057fa7ef346d437d95fcb0fdee4e87fae0af831b7f1242e150e4af2fdc7a209다. 저장 억제를 앞당기거나 빈 이름/manager0/누락 Save를 일괄 성공으로 처리하는 추가 변경은 이 제안에 없다.
