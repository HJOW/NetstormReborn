# 정렬 Content 부착의 현재 호출과 늦은 Pop — 단위958

2026-11-06, HJOW-Athlon. 마지막 전체 정밀 디컴파일은 같은 HJOW-Athlon/2026-11-03이다. AGENTS와 공통·DN 인계를 다시 읽고 LEFT_JOBS 3절4번의 공통 Pop/공유 수명 순서로 진행했다. 정밀 산출물과 원본은 변경하지 않았다. 제품은958/11.01이다.

## 발견한 차이와 복원 범위

PATCH10.78의 `004aec10`은 부모/자신의 dead/free 진단과 정상 복귀 재판정 뒤, 같은 그룹의 첫 순서 공백에 삽입하거나 끝에 붙인다. 기존 ContentFormHost는 링크를 쓰기 전에 고정 TYPE·ValidatePop·생산 TYPE·체인을 검사했다. 이 때문에 뒤쪽 Pop에서 실패해야 하는 입력도 링크/부모/contained 변경 없이 먼저 중단됐다.

새 AttachSortedContent 부모는 원문의 링크 순서와 부분 효과를 복원한다. 부모 this는 첫 진단 전에 저장하며, 목록 그룹 WORD20도 순회 전에 저장한다. 각 반복은 현재 content 번호 DWORD00541088을 읽는다. 첫 공백에서는 자기 next를 쓴 뒤 **자기 VT를 이웃 previous getter 전에 저장**하고, 그 저장 VT로 자기 previous setter를 실행한다. 이웃 setter와 자기 getter들은 각 호출 위치의 현재 VT를 사용하며, 자기 previous가 비영이면 getter를 한 번 더 호출한다. append 경로의 세 쓰기 순서도 보존한다.

이후 순서 BYTE23 → 현재 setParent → contained 비트 → 공통 Pop(flags|0x100) → 저장한 부모의 이유1 통지 순서다. Pop 또는 통지가 중단돼도 앞선 쓰기를 되돌리지 않는다. next 자기참조 진단 `004aba70`은 정상 복귀 뒤 저장을 계속한다. 물리 슬롯 범위 밖 접근·끝나지 않는 체인 순환만 호스트 경계에서 중단하며, 손상 입력을 미리 정상화하지 않는다.

현재 실제 ContentFormHost가 이 부모를 호출한다. selected-table previous getter/setter를 공통 contained 장치에 추가해 저장한 VT를 현재 raw TYPE로 대체하지 않는다. 실제 RawCampaignSession은 같은 Client finder의 현재 content 번호와 같은 World TYPE 표를 대여한다. 부모 통지 뒤의 genus 판정도 생성 시 사본 대신 현재 표를 읽는다. 대여하지 않은 독립 API의 사본 계약은 유지한다.

독립 원문 실행의 관측 범위는 **부모 통지 진입까지**다. getter/setter·공통 Pop·부모 통지는 명시 장치이며, 실제 `004aba70`만 함께 실행한다. 전체 원문 ABI 반환·사제 포획 꼬리·모든 파생 Pop 완료의 증거가 아니다. 현재 표 대여 검사는 battle0에서 생산 genus의 실제 변경과 부모 TYPE의 늦은 물리 범위 오류만 관측하며 포획 callback은0회다.

## 독립 관측과 실제 호스트 검사

제품 구현 전에 새 fixture 41행을 고정했다. 빈/공백/끝/혼합 목록 × 세 상태 × 정상/Pop 중단/통지 중단36행과 dead 진단의 미복구/복구·현재 content 번호 변경·getter 중 자기 VT 변경·getter 중단5행이다. 호출 순서와 슬롯0/5/7/9/11의 전체250바이트를 비교한다. fixture를 다시 관측해 차이0이며 제품 결과로 기대값을 생성하지 않았다. 최초 도구의 한자 혼용 주석만 한국어로 교정했고, 최초 도구/보고서와 두 도구 해시는 보존했다. fixture 바이트는 변경하지 않았다.

새 실제 호스트 검사는 수정 전 코드에서 `Form Pop 상태 오류`로 실패해 Pop 장치에 도달하지 못했다. 수정 후에는 void0와 범위 밖 생산 TYPE 두 입력 모두 앞선 링크·부모·contained 쓰기를 마친 채 선택 Pop에 도달하고, 명시 중단 뒤 상태가 남는다. 다른 검사는 부모 통지에서 현재 TYPE를 바꾼 뒤 실제 표 대여/독립 사본의 소비 차이를 확인한다.

| 최종 집중 필터 | 검사 수 | 결과 |
| --- | ---: | --- |
| ContentAttachCurrent_ | 새3 | 통과, 독립41행 포함 |
| FormAttachCurrent_ | 기존5 | 통과, 양판본864행 포함 |
| FormUnpopCurrent_ | 기존4 | 통과, 양판본1728행 포함 |
| FormPopCurrent_SortedContentBorrowsSameFactoryDepth | 기존1 | 통과 |
| SquidContainedUnpop_CommonVirtualGettersKeepWordAndNeighborBytes | 기존1 | 통과 |
| ContentFormAttach_DeadDiagnosticsResumeAtCurrentSlots | 기존1 | 통과 |
| ContentFormAttach_UnrepairedDeadReturnsWithoutLinkWrites | 기존1 | 통과 |

고유16검사=새3/기존13, 실패0이다. 게임과 집중 대상의 최종 Release 빌드 종료0·경고/오류0이다. 최초 검사 빌드의 TestSupport 누락에 따른 macro 컴파일 실패는 include를 추가해 고쳤고, 수정 전 제품 실패를 그 다음 실행으로 분리했다. 첫 제품 후보 빌드는 종료0이나 검사 initializer_list의 int→WORD 경고4건이 있어 명시 WORD 배열로 고친 뒤 다시 빌드하고16검사를 실행했다. 최초 원시를 보존하며 실패/경고를 최종 통과에 합산하지 않는다.

## CD 차이와 남은 순서

CD10.72의 대응 부모는 `004afff0`이다. 슬롯36바이트/순서 BYTE22이며, 그룹 WORD를 반복에서 다시 읽고 자기 VT도 이웃 getter 뒤에 선택한다. next 자기참조 진단은 PATCH의 별도 helper와 달리 인라인이다. 이번 구현은 PATCH 계약이며 CD의 기존 제한 API·선행 Validate·TYPE 기반 이웃 선택을 복원 완료로 주장하지 않는다. CD 전체 이관은 후속10.72 범위다.

다음959도 같은3절4번이다. Process `004b1440`/CD`004af380`의 파생 접두와 SquidProcessHost의 Attach/Reattach 선행 거부·복구, 상위 Factory 할당/외부 heap·실패 수명을 계속 대조한다. 사제/전투 꼬리의 나머지 guard와 전체 공통 Pop/파생 몸체·파일 수명도 잔여다. checking8=부모3/부분2/MP3와 완료부모39/부분4는 불변이다. 이 정렬 부모 하나로4번 전체를 완료하지 않는다.

새 게임/GUI·실제 포획/희생/특정 적 공격/승리·공중3종 전용·신규 MP/패킷/통신·물리 입력/PCM/CD 삽입·전체 CTest/34미션/ZIP은 실행하지 않았다. 일반 생산 Content와 명시 경계만 사용했다.

제품 EXE는 `81de8a948619c1b412b8a007f52b6e87c17d2792bb3399da54a1730af74616c6`, 11,292,160바이트다. 기존 보호2789개/fixture969개 불변이며 새 fixture1개를 더한 다음 기준은2790개/970개다. 원시/다음 보호 기준은 `extracted/cpp-resume-958/`이고 [완료 JSON](../../cpppj/content-attach-current-20261106.json)과 [Native 보고](../../cpppj/content-attach-current-native-20261106.json)를 따른다. 전체 목표는 진행 중이다.
