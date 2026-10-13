# Damageable 삭제 후처리와 선행 드롭

현재/마지막 디컴파일 PC는 **HJOW-Athlon, 2026-10-14**다. 작업 전에 최신 AGENTS.md와 두 LEFT_JOBS를 다시 확인했다. 앞 단위11의 IslandBuilder/공유 디코더 소스는 동결했다. 이 문서는 다음 단위12용 독립 원본 관찰 및 C++ 초안 범위이며 기존 완료수나 단위11 공유 검사에 합산하지 않는다.

## 실제 부모와 첫 helper

부모 주소는 **0044b9e0 / CD00461b40**, 선행 드롭 helper는 **0044b830 / CD00461970**이다. 정밀 전체 C의 원래 경로는 `extracted/refined/originals/raw.c`, `extracted/refined/originalCD/raw.c`다. 현재 좁은 내보내기는 `extracted/damageablepostdestroy/{edition}/{creation.c,functions.tsv}`의 **38/35 함수**다. 최초37/34 자료는 root가 `extracted/damageablepostdestroy-before-plain-snap`에 보존했다.

첫 helper를 기존 `RawDamageableRelease::Release`로 재사용할 수 있다는 초기 추정은 **폐기했다**. 기존 Release는 0044b626~0044b80c / CD0046174e~0046195a의 contained 해방이며, 0044b830/CD00461970는 Dude 생성 수를 결정하는 별도 몸체다. 기존 Release를 수정하거나 다시 구현하지 않는다.

원본 PostDestroy 순서는 다음과 같다.

1. `flags&0x300000`이면 현재 genus를 읽고 드롭 helper를 호출한다.
2. ordinary(`extra&9==0`)·typeFlags1 `0x400`·같은 칸의 genus `0x1000000`·현재 genus의 `0x10000000` 부재 및 rainBlocker/flags 조건을 만족하면 실제 섬 정리를 요청한다.
3. 전투·현재 HasHP·ordinary이면 현재 group0/1 또는 vortex 조건에서 주변 HP 대상에 피해 Regular를 만든다.
4. 현재 genus의 `0x130000`이 없으면 ±2칸 일반 finder의 다리를 금 간 상태/지연 낙하/단단한 상태/권한에 따라 약화하거나 삭제한다.
5. 마지막은 항상 **004b0840/CD004adbe0** 공통 Squid postDestroy다.

이 부모를 공통 post 한 번으로 대체하면 선행 드롭·섬 정리·주변 피해·다리 약화가 사라진다. DaisPostDestroy **00449580/CD00481620**, Altar thunk **00448b60/CD00483660**의 마지막 필수 경계이므로 새 `RawDamageablePostDestroy::PostDestroy(sid,flags)`를 분배한다.

## decompiled C와 실제 기계어의 차이

### 드롭의 셋째 인자

기계어는 두 판본 모두 **Drops(sid,genus,1)** 을 호출한다. patch0044ba25가1을 push하고 0044ba30이genus, 0044ba31이SID를 push해 0044ba32에서0044b830을 호출한 뒤 `add esp,0xc`로 정리한다. CD00461b6e/7a/86/87도 같은 순서다.

정밀 C는 push1을 Flags2 getter의 무시 인자로 표시하고 첫 helper를 두 인자로 호출하는 것처럼 보인다. 실제 Post 경로는 난수 위치 **00450b30/CD004e8d20**이며 task5103 생성으로 이어진다. Residence 위치 **00450c20/CD004e8e00**의 task5104는 helper의 다른 진입 범위다.

### 피해 Regular의 필드

원본은 Regular constructor **00496e80/CD0048efd0**에 event6699·후보 SID·payload0을 넘긴 뒤, 반환 객체의 **+0x20 count**에 `attackerOwner*65536 + maxHP/2 + 1`을 직접 쓴다. vortex genus0x200이면 피해량은400이다. 이 값은 payload나 event가 아니다.

공격자 owner는 삭제 flags의16~19비트다. packed 값은 낮은16비트를 OR하는 방식이 아닌 DWORD **ADD**이므로 피해65537이면 owner1에서 count131073으로 carry가 남는다. `0x200000`은 주변 피해를 생략하고 `0x400000`은 피해량 계산 뒤 실제 finder/생성을 생략한다. 다리 후처리와 마지막 공통 post는 별도로 판단한다.

### 두 좌표 절삭 helper

특정 칸 genus 조회의 plain snap은 **0041d770/CD00440a50**이다. 반경 finder는 **0041d7a0/CD00440a80**이며 실제 기계어에 fadd가 있다. 두 판본의 상수 **00502310/CD005017a0** 비트는 **0x3f7fff58 = 0.99999f**다. 0.9999f로 바꾸거나 중간 float 저장을 추가하지 않는다.

## 선행 드롭의 실제 분기

부모 SID가0이면 반환한다. abstract bit0이 켜지거나 genus `0x174200`이 없으면 생성하지 않는다. 기본1개→genus0x4200이면5개→genus0x20000이면2개 순서로 결정한다. 현재 특수 타입135이면3개로 바꾸며 현재 Dude 수가20보다 크면 정수 절반으로 줄인다. 계산한 반복 수는 생성 callback 뒤 다시 계산하지 않는다.

patch 제외 타입은 **133/156/144/138/187**, CD는 앞의 **133/156/144/138** 네 개다. 실제 PE의 대응 DWORD 전역을 확인했다. 타입135 특수 전역과 Regular event6699도 두 PE에서 직접 확인했다.

드롭 생성의 하위 몸체는 별도다. 10.78의00450a20 실제 생성은 기존 복원에 없으며, 현재 `RawDudeNotice`의 ResidencePoint 및 task5107 경로가 이를 완료하지 않는다. CD는004e8b30 생성·004e8ba0 후속을 호출하며 10.78의 생성 뒤 task쓰기와 호출 구성이 다르다. **이 하위 생성·Factory/HP/Frame/Pop/Owner를 noop 또는 성공값으로 대체한 완료를 주장하지 않는다.** 현재 필수 `dropDude` 연결은 해당 원본 생성을 공급하거나 요청 시 미복원으로 거부해야 한다.

## 기존 몸체 재사용

| 새 부모의 필요 효과 | 기존 구현과 연결 계약 |
|---|---|
| HasHP/MaxHP | 같은 Pool의 `SquidReward.HasHitPoints/MaxHitPoints` |
| 실제 발자국 중심 | 같은 Pool/Hash의 `RawSquidNeighbors.Center` |
| 네 단계 후보·현재 raw next | 새 사본 목록 없이 기존 `RawSquidFinder` |
| 섬 정리0046e6c0/CD0048f960 | 같은 월드의 **RawConstructionEffects.DropIsland** |
| 금 간 frame 검색 | 기존 **Bridge::CrackedFrame** |
| 지연 낙하0x2692 조회 | 실제 **HasScheduledBridgeFall** |
| 마지막 공통 post | 같은 삭제/장부의 **SquidDestroyLifecycle.PostDestroy** |

새 부모는 현재 type/span과 raw 상태를 각 효과 뒤 다시 읽는다. HP/중심 어댑터가 보존하는 metadata는 연결 뒤 고정해야 하며, 원본 현재 타입 **번호**가 변경되는 경로는 재조회한다. 타입 표 사본을 mutable 원본 전역으로 취급하지 않는다.

다리 약화는 현재 bridgeCrackLife−1을 WORD life 비트에 쓰고 실제 공간 통지를 먼저 호출한다. 그 뒤 현재 타입/frame으로 기존 금 간 검색을 실행하고 성공 시 patch DWORD/CD BYTE 프레임을 쓰고 실제 +88 표시를 요청한다. 현재 금 간 flags0x20과 단단한 flags0x40을 구분한다. 금 간 다리에 지연 낙하가 없으면 boss에서만 가상 Destroy0을 요청한다.

모든 하위 효과8개가 필수다. C++ constructor는 누락된 생성/섬 정리/Regular/낙하조회/공간통지/표시/가상삭제/공통post를 거부한다. Root가 후속12에서 실제 `RegularProcess::SetInitialCount(uint32_t)` API를 기존 전체 ResetTask lineage에 연결하기 전에는 **실제 Host 생성+count 쓰기 완료는 아니다**. 부모가 callback에 packed count를 전달한 것과 실제 Host 객체의 +20 효과를 구분한다.

## 독립 원본 관찰

`decomp_damageablepostdestroy_oracle.py`는 두 PE의 제한된 실제 부모/선행 helper와 타입 getter·HasHP/MaxHP·중심·일반 finder·다리 판정/WORD/frame 쓰기를 실행한다. Regular constructor 반환 뒤 +20 쓰기도 부모 원본 명령이다. Dude 생성, DropIsland, allocator/Regular constructor, 지연 낙하 조회, 공간/표시/가상삭제 및 공통 post는 **명시 외부 경계**다. 기존 완료 몸체를 새 native 완료로 중복 계수하지 않는다.

고정 입력은 삭제 flags·battle/boss·extra·현재 타입/genus/group/HP/발자국·현재 Dude 수/이벤트/life 전역·경계변경·좌표다. 전체 현재 부모/후보 SID50/60~65의 **원본 stride 바이트**, 경계 요청 순서/인자, 원본이 쓴 packed count를 저장했다. fixture 마지막 assert 횟수는 모두0이다.

| 판본 | 고유 정상 | assert | 두 정밀도 복귀 |
|---|---:|---:|---:|
| 10.78 | 2246 | 0 | 4492 |
| CD10.72 | 2166 | 0 | 4332 |
| 합계 | **4412** | **0** | **8824** |

x87 control0x027f/0x037f에서 같은 전체 raw/경계 효과와 보존 레지스터/스택을 확인했다. 게임/GUI/OS 실행은0이다. 이 숫자는 단위12의 C++ 대조와 최종 통합이 확인되기 전 **준비된 원본 관찰 수**이며 단위11 완료에 더하지 않는다.

첫 smoke는 해시 전역 주소 및 합성 가상 표시 경계 주소 오류로 각각 중단됐다. 당시 전체 도구는 `extracted/damageablepostdestroy-smoke-initial-tool.py`, `...smoke-repaired-initial-tool.py`와 최초 두 로그에 보존했고 최종25×두 판본 smoke가 성공했다. 표본/실패를4412에 더하지 않는다.

최초 전체4412에서 effectprofile6의 현재 owner 변경 입력이 빠진 것을 발견했다. 이전 전체 tool/h/cpp/report/fixture는 `hist/damageablepo-7d76b`에 보존했다. `if mutation==6: source.owner=3` 한 줄을 공급하고 재관찰했다. **20행만 변경**, 나머지 **4392개 전체 원본 행 동일**이며 생산 부모 C++는 전체 바이트 불변이다. 재관찰4412는 중복 합산하지 않는다.

`python tools/analyze_damageablepostdestroy.py`가 전체 현재 SHA·양판본 input 고유성/정상 계수/두 정밀도 반환·이전 전체 보고서/도구·한 줄 정확 역변환·현재 C++ 검사 소스를 감사하며 통과했다.

## C++ 검사 초안과 남은 작업

현재 `DamageablePostDestroyTests.cpp`의 검사2개는 원본4412행을 같은 실제 HP/중심/finder/기존 금 간 검색으로 재생하고, 전체 raw·원본 외부 경계 요청·packed count carry/payload0·필수 생성 누락 거부를 대조한다. 경계 관찰용 callback은 전체 하위 생성의 완료 근거가 아니다. **현재 공유 C++ 등록/실행 전**이며 실제 Host·전체 Dais/Altar 삭제 소비자도 대기한다.

Path 담당은 실제 Dais400000/Altar48 삭제의 `flags&0x300000==0` 범위부터 같은 RawSquidDestroy/Pool/장부의 마지막 공통post를 연결할 예정이다. 그 범위와 battle/HasHP/genus에 따른 피해/다리 분기의 실행 여부를 실제 metadata로 확인해야 한다. 이후 초기 count API와 실제 Host의 DamageableHandle6699→현재 가상 HP 피해까지 연결한다. Dude 생성/task5103/5104 및 전체 GameWorld/캠페인1-1·시각/음향/경제/생산/운반의 완료는 여전히 후속이다. CMake/LEFT/공유 빌드/통합 계수와 커밋은 root 전담이다.

## 단위12 실제 초기 count 연결의 후속 출처

Root는 기준11 **5bdb5be** 이후 `RegularProcess::SetInitialCount(uint32_t)`를 추가하고 기존39자료/전체몸체 역변환을 별도 lineage로 보존했다. 이 API는 새 Regular의 +20 count만 직접 쓰고 event/payload/time/Form을 보존한다. 위 절의 API 대기 설명은 첫4412 관찰 당시 상태이며 현재 API 준비가 완료됐다.

새 `DamageablePostDestroyHostTests.cpp`의 **TEST_CASE1개**는 기존2검사와 분리했다. 같은 실제 Factory로 생성한 레지던스/후보, HP Max/중심/finder/Hash, 단일 Pool/Kernel/ProcessHost에서 packed count를 새 Regular에 쓴다. 두 판본×최대HP100/131073의4장면에서 event6699/payload0/현재double시각17.25/Form을 확인하고, 시각 전에는 실행하지 않으며 첫 Kernel 호출에 원본 packed count65587/131073을 전달하는지 확인한다. handler는 **명시 관찰 후0반환**이며 실제 ProcessForm Unpop/Kernel 제거/번호 반환과 삭제깊이0 복구를 확인한다. 실제 DamageableHandle/지면 피해/가상 HP까지 완료됐다고 주장하지 않는다.

기존 Post h/cpp·기존2검사·4412 fixture·두 보고서·두 도구 **전체8파일**을 `hist/damageablepo-a3bd8`에 그대로 보존했다. 현재와 보존된 전체바이트는 동일하며 `analyze_damageablepost_host_binding.py`가 현재 Root count lineage·새consumer·기존전체 의존성과 연결한다. **새native0, 생산 부모 변경0, 고유4412 유지**다. 새 검사와 기존2검사의 공유 빌드 결과는 아직 대기한다. 첫4412 native보고서의 실제Host 미완료 범위는 원문으로 보존하고 현재 소비자 확인 범위는 이 후속 보고서에만 연결했다.
