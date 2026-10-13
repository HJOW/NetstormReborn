# 실제 Dude 드롭 부모와 생성·작업 소비자

## 현재 범위

- 주 대상은 패치 **10.78**, 비교 대상은 CD **10.72**다. 현재/마지막 정밀 디컴파일 PC는 **HJOW-Athlon, 2026-10-14**다.
- 새 독립 부모 입력은 **308 = 정상306/assert2**다. 패치154 중 정상152/assert2, CD154는 모두 정상이다. 이전 Spawn288 또는 Task780은 재사용한 하위 근거이며 새 부모 관찰 수에 다시 더하지 않는다.
- 실제 드롭 부모·게임 float 난수·진입점·Dude factory/생성자/owner/Pop/최초 후처리/I 프레임/장부·task/Regular 생성자/예약 시각을 원본 명령으로 실행했다. 실제 Dude 생성과 task 예약은 **각 판본144**, 총288사슬이다.
- C++ 새2검사는 같은 실제 `SquidProcessHost`/Kernel/Pool/보호 Dude 자산/공간/표시/최초 후처리를 구성하며 **308전체 대조가 통과했다.** 단위13 최종 Release 경고/오류0·새10/고유집중59·전체1066검사/실패0이다.

## 실제 주소와 두 지연

| 역할 | 패치10.78 | CD10.72 |
| --- | --- | --- |
| 난수 좌표 드롭 부모 | `00450b30` | `004e8d20` |
| 진입점 드롭 부모 | `00450c20` | `004e8e00` |
| 실제 생성 하위 | `00450a20` | `004e8b30` |
| 일반 task 설정 | `00450670` | `004e8ba0` |
| 실제 진입점 | `004505a0` | `004e8a30` |

난수 부모는 현재 x/y에서 발판의 두 난수를 빼고 각 좌표를 float로 저장한다. 게임 난수의 현재 seed와 소비 순서를 보존한다. 발판0에서는 min0.1/max−0.1의 절대 차를 사용하는 원본의 실제 경로가 관찰됐다. 생성 뒤 task **5103**과 지연 **index×0.1f**를 요청한다. 진입점 부모는 기존 실제 `RawDudeNotice::ResidencePoint`를 재사용하며 task **5104**와 지연 **index×0.7f**를 요청한다.

패치 `00501784`/CD `005072e8`의 **`33 33 33 3f`는 float0.7**이다. 디컴파일의 `333...` 기호를 숫자333으로 읽지 않는다. 패치 `005022ec`/CD `005072c8`의 **`cd cc cc 3d`는 float0.1**이다. 기존 Task 독립 격자의 합성 지연333은 이 드롭 상수를 뜻하지 않는다.

양판본은 source free에서 즉시 반환하고 dead source는 계속 처리한다. 패치는 sourceSID0에서 원본 진단으로 중단한다. 진입점 부모의 진단 줄469와 난수 부모의452를 따로 관찰했다. CD의 sourceSID0은 정상 반환이다. 생성 실패 뒤 패치는 즉시 반환하고 CD는 SID0을 일반 task에 넘겨 그 하위 몸체의 정상 반환을 실행한다.

## 원본 격자와 관찰 경계

각 판본154입력은 두 부모, index0/1/7, 발판0×0/3×3, owner0/1/8, source alive/dead, seed0/12345678을 교차하고 capacity0/count가득참/blocked/free/sourceSID0을 추가한다. 현재 게임 시각은 double **16777217.125**다. 결과는 정상/진단·새 SID·호출 순서·실제 Regular의 double시각/event/payload/count·난수 상태·population 전체·raw 슬롯·공통 장부·실제 변경 영역이다.

원본 oracle의 명시적인 하위 경계는 **Masked Regular 조회**, **40바이트 힙**, **ProcessForm 부착** 세 종류다. 실제 새 Regular 생성자와 그 뒤 task 재설정은 원본 몸체다. C++에서는 세 경계를 실제 Host 조회/생성/부착으로 연결한다. 하위 Regular가 생성된 뒤의 이동/task handler, DudePath24, source 전체 파괴, DamageablePostDestroy 부모의 전체 삭제, 캠페인1-1 월드/완주는 이번 완료 범위가 아니다.

`RawDudeDrop.h/.cpp`는 같은 `RawDudeSpawn`/`RawDudeTask`/`RawDudeNotice`/GameRandom 풀을 확인한다. 새 `DudeDropTests.cpp`와 전용 `DudeSpawnSupport.h`는 실제 보호65클러스터/130물리프레임을 소비한다. 기존 `DudeSpawnTests.cpp` 및 최초288보고서는 수정하지 않았다. CMake 등록은 production `RawDudeDrop.cpp`, test `DudeDropTests.cpp`, 매크로 **`NETSTORM_DUDEDROP_FIXTURE`**와 `dudedrop-x86.tsv`가 필요하다.

## 최초 전체 자료와 현재 test 연결

최초308 native report/fixture/tool/production/test/support 및 모든 비보호 출처 **69파일 전체**를 `hist/dudedrop-bef-bac65`에 보존했다. 미컴파일 test의 임시 진단 출력0을 실제 두 부모의452/469로 바꾼 한 조각만 정확 역변환한다. `analyze_dudedrop_draft_lineage.py`와 별도 보고서가 최초 native SHA/원문 불변 및 현재 test SHA를 연결한다. 새 native 입력은0이고 C++ 성공을 의미하지 않는다. 최초 native 보고서의 `cpp_tests_completed=0`과 당시 test SHA는 그대로 보존한다.

```text
python -X utf8 tools/analyze_dudedrop_draft_lineage.py --verify
```

실제 공유 소비자의 최종 개별2검사/308전수는 성공했다. 최초 native 보고서의 당시 상태/Source SHA는 그대로 보존하고 최종 공유 결과는 단위13 종합 근거에 별도로 기록한다.
