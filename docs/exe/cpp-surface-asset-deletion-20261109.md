# 실제 자산 가상 삭제와 장기 Pre/Post 부모 연결 — 1200

M6-n의 World 없는 자산 삭제를 같은 Client 저장소에 연결했다. `RawSurfaceStorage::ProcessFormDispatch`가 실제 자산 가상+10을 `VirtualDestroy`로 전달하고, 현재 VT+14/+18이 선택한 기존 공통·표면·Damageable·간헐천 부모를 호출한다. 다리 가상 삭제의 `false` 반환과 dead·로그·Pre/Post 깊이·공간 해제·SID 반납 순서는 기존 몸체가 결정한다.

실제 Client의 PATCH 삭제 모드에서 현재 삭제 대여가 살아 있으면 기존 Client 장치의 Session/Runtime pin을 유지한다. 대여가 없으면 장기 분배기를 사용한다. 물리 World 대여 증빙이 아직 남아 있다는 이유로 반납된 Session을 요구하지 않는다. 실제 Client는 첫 Editor/Session 전 `EnablePatchDestroyDiagnostics`로 이 PATCH 몸체를 활성화한다. PATCH 몸체를 사용하지 않는 독립 typed 장치의 기존 반환/예외 계약은 유지한다. 너겟 Regular, 표면 연결 객체와 Damageable의 주변 다리 삭제도 같은 진입으로 재귀 호출한다. 임시 World·Session·Player·성공 상수를 추가하지 않았다.

`RawSurfaceAssetDeletion.cpp`와 `AssetDeletionVtables.inc`는 생성자83개 실제 VT의 Pre/Post/부모 통지 주소를 담는다. 효과마다 현재 슬롯의 VT를 다시 읽으므로 앞선 로그·선택 효과가 바꾼 VT나 선택 DWORD를 선행 스냅샷으로 대체하지 않는다. TYPE 번호와 VT의 쌍 일치를 새 조건으로 추가하지 않는다.

| 연결 범위 | 근거와 한계 |
| --- | --- |
| 자산 가상+10 | PATCH83VT의 실제 진입점은 공통`004af780`과 다리`004220f0` 두 가지다. 기존 `RawSquidDestroyDispatch`가 같은 Pool/Graph·현재 권한/editor로 선택한다. |
| Pre/Post 부모 분배 | **83VT 중52개 쌍**은 양쪽 부모를 연결했다. 공통`004b0950/004b0840`, Damageable 가상Pre`0044b4b0`/Post`0044b9e0`, 너겟 thunk`00484f80`, 다리`004221b0/00422300`, 섬Pre`00442240`, noIsland Post`00442640`, 간헐천`00485580/00485540`다. 내부`0044afd0`을 가상Pre`0044b4b0`과 혼동하지 않는다. |
| 부모 삭제 통지 | 77VT의 실제+34는 빈`0041c100 ret8`이다. 이 주소에서만 반환하며 TYPE/상태/통지 인자에 다른 조건을 더하지 않는다. **6개 전용 통지**는 미연결이다. |
| 남은 전용 부모 | **31VT 쌍**은 한쪽 이상 전용 부모가 남았다. 선택되면 앞선 효과를 보존한 채 해당 위치에서 명시 실패한다. 공통 삭제로 가장하지 않는다. |

52는 **기존 Pre/Post 부모 분배를 연결한 수**다. 이 부모가 선택하는 큰 낙하·파편·AI·드롭 생성까지 모두 World 없이 동작함을 뜻하지 않는다. 현재 큰 하위 장치는 그대로 유지했고, 그 하위가 선택되면 실제 Runtime을 요구하는 범위도 남아 있다. 다른 Form의 장기 호스트와 전용 Regular도 별도 작업이다.

아직 남은31쌍은 PE 진입점으로 다음과 같이 구분한다. 다음 구현 시 현행 Session의 같은 원문 대응 몸체와 실제 장기 owner를 대조한다.

| 원문 전용 부모 후보 | VT 수 |
| --- | --- |
| Bomb Pre`0041e960` | 1 |
| Buried Pre`0041f910` | 4 |
| Carrier Pre`00426890` | 5 |
| Balloon Pre`0042ae80` | 2 |
| Dais Pre/Post`0044a4d0/00448b60` | 1 |
| Blocker Pre/Post`0044afd0/00449580` | 1 |
| Dude Post`00450920` | 1 |
| Vortex Pre`00455840` | 1 |
| Fence Pre`00457990` | 1 |
| Flyer Pre`0045e7e0` | 3 |
| Growing Ice Post`0048eb80` | 1 |
| Ice Post`0048edf0` | 3 |
| Forcefield Pre`004906c0` | 1 |
| Priest Pre`004919b0` | 1 |
| Player Post`0049a4a0` | 1 |
| Construction Pre`004dca50` | 4 |

정적 근거는 PATCH83VT와 CD68VT를 읽은 원시 자료 및 [검증 보고서](../../cpppj/surface-asset-deletion-20261109.json)에 있다. PATCH 분배 표332DWORD를 실제 PE와 대조했다. C/전체 함수 범위 PE 추출은 요청21개 중20개를 확보했으며, CD`00449820`은 기존 정밀 함수 메타데이터에 없어 함수 본문 대조 완료로 세지 않았다. 해당 CD 주소는 PE 가상 표에서 읽은 값이다. 이번 구현의 Native 대상은10.78이며 CD 전체 복원을 뜻하지 않는다. 마지막 정밀 디컴파일은 Client 양판본2026-11-03·R전용2026-11-07, 모두 **HJOW-Athlon**이다. 정밀 소스를 변경하거나 원본 게임을 실행하지 않았다.

새 비MP 검사6개는 실제 자산의7개 부모 경로 반납·깊이 균형, 로그 뒤 VT 재조회·전체 선택 DWORD, 선택 기억/해제 한 번, 미연결 전용 Pre의 부분 효과, 다리 가상 삭제 거부, 실제 삭제 대여 없는 물리 lease를 확인한다. 미리보기/매몰 플래그로 큰 효과가 생략되는 입력을 포함하므로 일반 상태의 모든 효과를 검증한 것은 아니다. 기존 관련 부모 검사와 Release 빌드 및 단순 Tutorial4 생산·건설 요청/대기·같은 Client Edit 왕복 결과는 보고서가 기준이다. 최초 검사 컴파일 C3490과 교정 원시를 보존했다. 첫 실행99건 중98통과/독립 typed 삭제1실패도 보존하고, 제품에서 PATCH 활성 모드를 구별해 기존 단언을 바꾸지 않고 교정했다.

사제 포획/희생·특정 적 공격·승리·공중3종·Test Battle·신규MP/패킷/통신 실행 검사는 하지 않았다. 전체34경로 재진입·전체 솔루션 실행·ZIP 갱신도 이번 단위 범위에 포함하지 않는다. [현행 인계](../../LEFT_JOBS.md)의 M6/M7/M8·한국어·시나리오 편집·버그 수정은 계속 남아 있다.
