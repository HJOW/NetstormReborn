# 실제 Client Player/factory 저장소 접합 — 단위391

10.78의 실제 Player 초기화 부모를 세션 생성 경로에 연결하고, 별도 vector에 있던 소유자별 factory SID 목록을 Player의 현재 +0/+4/+8에 연결했다. 전체 State 정리나 Editor 완료를 뜻하지 않는다.

## 기준과 범위

- 기준 커밋은 `7e53489e / 20261024-13 CP`다. 현재/마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**으로 일치한다. 이번에는 기존 정밀 C를 읽었으며 재디컴파일하지 않았다.
- [이전 전체 초기화 복원](cpp-player-initialization-20261024.md)의236BYTE 부모와 실제 동맹 자식을 재사용한다. 최초202관측/fixture/도구는 수정하지 않았고 SHA 검증 종료0이다.
- 추가 Native 관측을 만들지 않았다. 최초 `owner-x86.tsv`의10.78 **1000행**을 실제 새 할당/헤더 연결로 재생하는 검사를 추가했다. 기존 CD10.72/10.37 각1000행도 기존 독립 경로에서 유지한다. 새 CD 실행/전체 CD 복원으로 확대하지 않는다.
- 원문00490050은 Player+8 < +4일 때 +0의 배열에 store한 뒤 +8을 증가시킨다.004900a0은 같은 현재 Player의 배열 remove에 도달한다. 실제 게임의 SID와 호스트 포인터를 혼동하지 않는다.

## 같은 물리와 수명

[ClientPlayerRecords](../../cpppj/src/client/ClientPlayerRecords.h)는 현재 Player base, 실제1620BYTE 기록,9개의120BYTE factory 할당, FortCount/LastLoginPlayer, 방향별81DWORD 동맹을 소유한다. Client 생성자는 아직 Player를 할당하지 않는다. 실제 CampaignClientServices가 같은 소유자를 Session에 전달하고, 독립 Session 검사만 같은 종류의 자체 소유자를 만든다.

opaque32 주소는 실제 생존 블록의 식별자다. 호스트 포인터를32비트로 절삭하지 않는다. 블록은 DWORD 정렬 저장소이며 부모의 malloc/free/new 장치에만 연결한다. factory의 비초기화120BYTE는 제품에서 기존 CurrentAllocationBytes의 현재 CRT 입력을 사용하고, 검사에서만 명시 패턴을 제공한다. Native 힙 주소/실제 OS allocator의 바이트까지 동일하다는 뜻은 아니다.

Session은 첫 고정 Player span 소비자를 만들기 전에 원문 전체 부모→실제 RawPlayerParticipation::SetAlliance를 실행한다. 이때 같은 PlayerWorldState의 known은 최초0이다. 이후 기존 known1 어댑터 시점을 유지한다. 각 행의 region127/자기 동맹을 별도로 다시 쓰던 초기화를 제거했다. Login의 FortCount/LastLoginPlayer와 기존 모든 동맹 소비자는 같은 소유자의 실제 값을 빌린다.

`SquidPostPopList`의 연결된 목록은 매번 현재 Player의 +0 포인터/+4 용량/+8 개수를 읽는다. 기존 독립 vector/count는 연결 시 비어 있어야 하며, 사본을 동기화하지 않는다. 공통 PostPop/소유자 이동/공통 삭제는 같은 Append/RemoveAll을 사용하고, 배치 기준점은 같은 Items를 읽는다. 독립 목록은 기존 vector/count 경로를 유지한다. 새 연결을 쓰는 소비자는 Count/Capacity/Storage/Items를 사용해야 한다.

고정 span 소비자의 전체 교체 대신 단일 `Use` 대여를 두었다. Session Impl에서 world 및 모든 소비자보다 먼저 구성하고 나중에 반납한다. 같은 소유자의 두 번째 살아 있는 Session과 같은 대여의 두 번째 Initialize는 기존 물리를 변경하기 전에 거부한다. 원문 부모 실패의 앞선 부분효과는 되돌리지 않으며 실패한 대여를 재실행하지 않는다. 다음 세션은 이전 소비자가 모두 소멸한 뒤에만 이전 factory9개/Player를 반납하고 새 기록을 구성한다.

현재 UberGump::DropWorld는 Help/메뉴·미니맵·Production·렌더 콜백·ObjectMenu·MissionRuntime·입력 소비자를 먼저 분리하고 실제 Kernel의 GameWorld를 반납한다. 이 기존 순서를 확인했다. 새 대여는 전체 원문 State 정리나 모든 외부 참조의 자동 추적을 대체하지 않는다.

## 검증

configure1/build1은 종료0이고 관측 경고·오류0이다. 신규 소유자4·기존 Native1000행의 bound 재생1·실제 Client 연속 Session1의 **6개 모두 첫 실제 실행에서 통과**했다. 집중 고유35개/검사 실패0이다. 부모202관측 전체 대조·기존 공통 PostPop6·공통 삭제6·실제 Session 용량도 함께 확인했다.

최초 집계기는 `owner_`를 정확한 접두사처럼5개로 기대했지만 실제 검사기는 부분문자열을 선택하여14개가 통과했다. 제품 검사 실패는0이며 집계기만 종료1이었다. 그 최초 검사기·출력을 보존하고, 통과한4+14개를 재실행하지 않은 채 남은5그룹을 이어 실행했다. 최종 고유 수는 출력의 실제 검사 이름으로 확인한다. 이 집계기 오판 때문에 제품/검사 소스나 Native 입력을 바꾸지 않았다.

최종 감사1의 KeyError는 실제 GUI 보고서의 `originalsUnchanged`를 `originals_unchanged`로 읽은 감사기 오류다. 첫 출력을 보존하고 해당 조회를 교정했으며 실제 보고서의 완료/원본 불변/각 실행 SHA를 확인한다. 제품·검사·실제 관측을 바꾸거나 재실행하지 않았다.

동일 제품 SHA **d94c09412226313feedfecce740f612cfc70bb9cd0e1379d06822affc0c1b494**의 실제 GUI 회귀는 다음과 같다.

| 실제 경로 | 관측 결과 | 보존 범위 |
| --- | --- | --- |
| 자연 Early1 | A~G, 세 수확·귀환, SP0→200→400→600, 다음 Tutorial2 Guide4/SP10000 | 초기 완료1→1·주입0·stocktimer1·종료0 |
| 자연 Early2 | A~I, 신전/워크샾, 발사기4→3, 회수1·환불75·SP3075, 다음 Tutorial3 Guide4/SP2000 | 초기 완료1→1·주입0·stocktimer1·종료0 |
| 일반31 | Early6+training9+campaign16의 CLI/실제 Play 버튼→실제 플레이 장면 | 전체31 통과·새 사용자 완주/일반 메뉴 선택 증거로 확대하지 않음 |
| Demo3 | 실제 메뉴→15초 자연 timeout→플레이/로컬 AI | 전체3 통과·세 Demo의 자연 완주 증거로 확대하지 않음 |

각 실행의 제품 SHA와 원본1394 불변을 확인했고 두 자연플레이 goal.bmp를 직접 열어 완료창/SP/좌측 덱/미니맵을 확인했다. 게임은36프로세스이며 위 표는4개 직렬 검사 묶음이다. 오디오는 이 회귀에서 비활성화했다. 빌드의 검사 컴파일과 GUI는 병행했고, 모든 게임과 설정 복구가 끝난 뒤 집중/전체 검사를 시작했다.

검사 SHA는 **d3acf86b690e26b1657202f73ff774b6fbfe5a5dfe04b6015371302bc9f110b0**이다. 최종 CTest1은 **2664검사/실패0·종료0·316.73초(전체316.79초, wrapper316.94초)**다. 검사 전후 제품/검사 SHA가 같고 원본1394/1394동일·보호 경로 추적 변경0이다. [제품 보고서](../../cpppj/client-player-records-runtime-20261024.json)에 현재 소스17·증거43 SHA와 최초 집계기/감사기 중단을 기록했다. 최초 출력은 `extracted/cpp-player-owner-391-*`에 별도로 보존한다. 같은 검사를 이유 없이 반복하지 않았다.

## 남은 경계

- ClientCoreDataGlobals의 current/cache/active와 Login의 지역 current/cache/active는 아직 공유하지 않는다. 따라서 전체 PlayerLinks/Kernel/Gump/State 정리와 Core0→해제→선택적Core1을 실제 부모 순서로 접합하는 작업은 남는다.
- 현재 Session 생성/소멸 어댑터의 모든 모드 전환이 원문 부모 전체와 같다고 주장하지 않는다. 초기화 실패 이후 재시도/부분할당 복구도 별도 원문 근거 없이 합성하지 않는다.
- 기존 비-guided My=1 시점, 기타 State 표/연결/Pool/Zone 정리, 최종 Editor HUD/State/loaded·도구·저장·언어별 시나리오·재로드는 미완료다.
- 전 Early Missions/전체 캠페인/Demo3 사용자 완주와 픽셀·음향·규칙의 완전 동일성은 미완료이며 목표는 활성 상태다. 과거388 제품의 관측을 이번 새 제품의 관측으로 합산하지 않는다.
