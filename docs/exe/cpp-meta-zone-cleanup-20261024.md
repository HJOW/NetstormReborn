# State 정리 자식0042b1b0: 메타맵 구역 기록표

현재 결과는 **실제 부모38관측(정상28/명시 자식 중단10)·신규 격리3검사/실패0**이다. 첫 MSVC 빌드·실행은 모두 종료0이며 `/W4` 경고·오류는 없다. 신규 모듈은 기존 저장소를 빌리는 최소 부모이고, 실제 마커 삭제 자식 `00479520`과 Client 장기 owner·전체 State 정리는 아직 연결하지 않았다. 공유 기존 소스·CMake·빌드·lib·GUI·LEFT·git은 변경하거나 실행하지 않았다.

## 실제 저장소와 관문

현재 PC와 마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**이다. 보호 원본10.78 SHA는 `a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236`이다. [함수표](../../extracted/refined/originals/functions.tsv:799), [정밀 C](../../extracted/refined/originals/Netstorm.c:34526), 실제 PE를 읽고 `0042b1b0`의68BYTE 전체를 확인했다. 호출자는 공통 State 정리의 `004b2ec2`, 수신 데이터 부모 `004b4250` 안의 `004b428e`다.

| 원문 주소 | 실제 역할 | 현재 대응/경계 |
| --- | --- | --- |
| `00531760` | PE 초기값1인 DWORD. 비영 진입에서 자식 호출을 생략하고 표 전체를 지운 뒤0을 쓴다. | 같은 실제 참조를 빌린다. bool 축소·세션마다1재설정·새 숨은0 owner는 없다. |
| `00546908` | 51×144BYTE=7344BYTE=0x72c DWORD의 고정 메타맵 구역 기록표. | 새 모듈은 span만 빌린다. 같은 실제 Client owner는 현재 미연결이다. |
| `00479520(index)` | signed record+0>0이고 record+4c의 SID가 비0일 때 현재 Pool의 해당 Form 가상`+10(0)`을 호출한 뒤 같은 record+4c를0에 쓴다. | 원문 자식은 정적 분류만 완료했다. 실제 가상소멸/Pool 수명은 선택 도달 시 필수 hook이다. |

진입 플래그0에서는 `index=0..50`의 자식51회를 수행한 다음 표를 낮은 주소부터 DWORD 단위로 지운다. callback이 플래그를1이나 전체 비트1로 바꿔도 이 분기는 다시 선택하지 않으며, 마지막에 플래그를0으로 쓰지도 않는다. 자식에서 중단하면 전체 표 지우기는 시작하지 않고 앞선 자식의 부분 효과가 남는다. 플래그 비영 분기의 플래그0쓰기는 표1836DWORD가 모두 완료된 뒤다.

`MetaZone`이라는 새 이름은 주소의 현재 용도를 설명하는 편의 이름이다. 원래 클래스명 `Ch`를 확정한 것이 아니다. 메타맵 참가 부모 `004b8810`은 `ZoneLocked`를 검사하고 이7344BYTE 전체를 임시 보관·복구하며, 수신 부모도 같은144BYTE 기록을 채운다. 이 근거는 일반 허용 표나 단독 미션의 다섯 State 블록과 구별된다.

## 기존 모듈과 주소 대조

- [RawTotalMade](../../cpppj/src/o/RawTotalMade.h)의 허용 표/`missionRestrictions`는 `005ca0d0`의 첫255DWORD이고 세 집계 표는 `005c94d0/005c98d0/005c9cd0`이다. 이번7344BYTE 표와 같지 않다. 기존 같은 표를 빌리는 Session·Header 경로는 변경하지 않았다.
- [Player Core 기록](../../cpppj/src/o/RawPlayerCoreRecords.h)은 `00595360`의9×16BYTE와 별도 `005953f0`의10BYTE다. [CoreData](../../cpppj/src/o/RawPlayerCoreData.h)의 현재 Player/캐시도 이번 구역표와 별도다.
- [RawLocalMissionState](../../cpppj/src/o/RawLocalMissionState.h)의144BYTE×5개는 `005c8630/005c8758/005c86c8/005c87e8/005c8878`이다. 한 기록 폭이144라는 이유로 메타맵51구역과 같은 owner로 취급하지 않는다.
- [RawMetadisplayDirty](../../cpppj/src/o/RawMetadisplayDirty.h)는 현재 월드 미니맵의 셀 큐·비트맵·카메라 캐시이며 `00546908`의 구역표가 아니다. 현재 제품에서 같은7344BYTE owner는 찾지 못해 신규 모듈에 별도 소유자를 추가하지 않았다.

CD10.72의 자동 SourceMap 후보는 `0045a230`이다. 그러나 실제 [CD 정밀 C](../../extracted/refined/originalCD/Netstorm.c:79244)는 다른 표와 stride를 순회해 조건부 자식을 부르는 몸체이며10.78의 최초 플래그/51기록/7344BYTE지우기와 같은 알고리즘이 아니다. 후보 주소만 남기고 같은 C++ 구현을 CD 복원으로 표시하지 않는다. 이번 CD 실행은0이다.

## 제품 전 독립 원문 관측

[도구](../../tools/decomp_meta_zone_cleanup.py)가 제품 세 파일이 없는 상태에서 [최초 TSV](../../cpppj/tests/fixtures/meta-zone-cleanup-x86.tsv)와 [최초 보고서](../../cpppj/meta-zone-cleanup-native.json)를 먼저 고정했다. 최초 실행 로그는 `extracted/cpp-meta-zone-cleanup-20261024-native-first.log`다. 첫 실행부터 정상이며 도구·기대 교정이나 재작성은 없다.

19입력×두 x87 제어어의 **38관측 / 정상28 / 명시 자식 중단10 / 실행기 명령 계수60,398 / 정밀도 차이0**이다. 계수는 선택된 하위 진입 경계를 포함한다. `00479520`은 필수 외부 장치로 두고 현재 index·호출 순서·flag/record 변경·중단만 명시했다. 그 자식의 실제 marker 가상소멸을 실행하거나 성공으로 대체한 관측은 아니다.

모든 입력에서 실제7344BYTE와 뒤16BYTE canary를 기록한다. 0/1/비영·극단 DWORD 플래그, 서로 다른 비영 초기 메모리, 첫/중간/마지막 자식의 flag 변경·record+4c변경과 중단을 포함한다. 실제 rep stosd의 각 DWORD 주소·폭·0값·1836회 순서, 비영 분기의 마지막 flag0쓰기, 정상 cdecl/비휘발 레지스터/FPU와 canary 보존을 별도로 감사했다. 표를 지우는 시점과 진입 분기 유지가 원문 실행 결과로 고정됐다.

## 신규 API와 검사

[RawMetaZoneCleanup.h](../../cpppj/src/o/RawMetaZoneCleanup.h)·[본문](../../cpppj/src/o/RawMetaZoneCleanup.cpp)은 `MetaZoneCleanupGlobals{firstReset&, records span}`과 `MetaZoneCleanupHooks.clearMarker(index)`를 받는 `ClearMetaZoneRecords`를 제공한다. 고정 표 view는 호출 중 주소를 바꾸지 않으며, 실제 flag·표의 수명은 외곽이 보장해야 한다. 플래그0에서는 표가 전부0이어도 실제 자식51회를 생략하지 않는다. 미연결 hook은 첫 선택 위치에서 명시 오류이며 최초 비영 분기는 그 hook을 요구하지 않는다.

[MetaZoneCleanupTests](../../cpppj/tests/MetaZoneCleanupTests.cpp)는 다음 세 검사를 포함한다.

1. `MetaZoneCleanup_ReplaysIndependentParent38`: 최초38행의 전체 표/canary·현재 flag·정상/중단·자식 index·callback trace를 대조한다. 실제 Native store 수·명령 계수는 독립 감사 항목이며 C++ 쓰기 trace를 계측한 것으로 가장하지 않는다.
2. `MetaZoneCleanup_FirstBranchAndRequiredChildKeepPartialEffects`: 비영 DWORD의 최초 자식 생략, 두 번째 호출의 필수 누락, 자식의 flag/record 부분 변경 후 예외를 확인한다.
3. `MetaZoneCleanup_ShortPhysicalPreservesStorePrefixAndChildTiming`: 짧은 호스트 span은 해당 DWORD에서만 거부하고 완료된 prefix와 비영 최초 flag를 보존한다. 진입0에서는 짧은 표를 이유로 자식51회보다 앞서 실패하지 않는다. 이것은 명시 호스트 물리 계약 검사이며 원본 OS fault를 실행한 결과와 구별한다.

별도 `extracted/cpp-meta-zone-cleanup-20261024-isolated/`에서 MSVC14.51.36231·SDK10.0.26100.0, `/std:c++20 /EHsc /utf-8 /W4 /permissive-` 단일 프로세스로 컴파일했다. `/MP`·공용 obj/lib 링크·제품 실행은 없다. 첫 `build1.log`·`test1.log`·`result1.json`을 보존했다. 신규 등록은 `.cpp` 하나·검사 세 개와 `NETSTORM_META_ZONE_CLEANUP_FIXTURE`→최초 TSV이며 추가 제품 모듈 의존은 없다.

동료 `dialog_lifetime`이 신규 세 파일과 정밀 C를 읽고 진입 분기·7344BYTE 뒤 flag쓰기·51회 자식·부분효과에 명백한 차이를 찾지 못했다. short-span 오류는 Native pagefault와 다른 호스트 계약이라는 점도 확인했다. 읽기 검토이며 추가 실행은 아니다. 최종 SHA·UTF-8·구문·문서 링크·최초 자료 검증은 `extracted/cpp-meta-zone-cleanup-20261024-isolated/verification.json`에 고정했다. 실제 Client의 장기 표·초기 플래그1·마커 자식이 연결될 때까지 공통 State 정리 완료로 표현하지 않는다.
