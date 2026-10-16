# cpppj 작업 일시 중단 인수인계 — 2026-10-16

사용자가 서브 AI 중단·대기, 회귀 검사, 인수인계 문서와 로컬 커밋 뒤 목표 중단을 요청했다. `construction_audit`, `energy_finish`, `session_audit`는 모두 **interrupted** 상태이며 추가 작업을 지시하지 않았다. 이 문서 이후에는 사용자 재개 지시가 있을 때까지 구현을 계속하지 않는다.

현재 PC는 **HJOW-Athlon**이고 마지막 전체 정밀 디컴파일 PC/날짜는 **HJOW-Athlon, 2026-10-15**다. 보호 원본·AGENTS.md·dotnetpj를 수정하지 않았고 게임/GUI 실행과 원격 push는 없다. 기존 게임의 Windows 10/11 전체화면 DirectDraw 오류 수정은 목표 범위에서 제외한다.

## 중단 시 보존하는 변경

직전 완료 커밋은 `9a8a0f8` (`20261015-66 CP 단독 미션 로그인 상태 전환과 비동기 수신 순서 복원 검증`)이다. 해당 시점의 전체 기준선은 **1,309개 검사, 실패0**이다. 이전 중단 기록인 [2026-10-15 인수인계](cpp-pause-handoff-20261015.md)의 상태/Start 초안 설명은 아래 새 결과로 갱신한다.

- `RawLocalMissionState`는 직전 커밋에서 이미 CMake 등록과 원본33개 입력 대조를 마쳤다. 더 이상 미등록 초안이 아니다. [검증 기록](cpp-local-mission-state-20261015.md)을 따른다.
- `RawCampaignMissionStartBindings`와 신규 `CampaignMissionStartTests`를 CMake에 등록했다. 같은 실제 Session의 SP·기술·현재 사제·AI 생성/초기 갱신을 같은 MissionScript/프레임 게이트에 연결한다. AI 이름은 현재 Script의 UTF-8을 원본 Windows-1252로 변환한다. 미등록 능력은 실제 assertion/log 소비자를 요구하며, 소비자가 반환하면 원본처럼 계속 진행한다.
- `RawVortexPreDestroy`는 원본 신전 Pre `004dca50`과 목록 중복 전체 제거 `0040ea00`을 복원한다. 같은 Player 1,620바이트, 현재 신전/거주 구역/outpost 목록, loading 전역을 참조한다. 목록의 stale 꼬리, 효과 이후 현재 owner/좌표/extra 재조회, 섬 진단과 지역 갱신 및 Player의 신전 참조 정리 순서를 보존한다.
- Session에 같은 공통 삭제를 반환하는 `RawSurfaceWorld::CommonDeletion()`과 실제 Damageable 해방/Pre/Post를 조립했다. 네 신전 VT의 공통 Destroy/신전 Pre/Damageable Post를 현재 가상 분배기에 연결하고 사제에도 같은 Damageable Pre를 공급한다. 미복원 붕괴·폭발·Dude/받침 드롭은 선택 시 명시 오류를 유지한다.

## 원본 대조와 현재 실패

신전 Pre의 새 native 관찰은 **102개 고유 입력**이다. 원본 `004dca50`/`0040ea00`만 실행했으며 섬·지역·Damageable·UI는 명시 외부 경계다. 전체 Player, 세 객체, 세 목록의 count와 물리 DWORD, loading, 호출 순서를 C++에서 대조한다. 정상 반환 스택·보존 레지스터·쓰기 주소 guard도 관찰했다. 지면 getter `0046f530`의 실제 `ret 8`을 확인하여 관찰기의 초기 ABI를 교정했다. 잘못된 ABI 실행 결과를 정상 관찰 수에 포함하지 않는다.

`VortexPreDestroy_Native102`와 필수 효과/저장소 검사는 모두 통과했다. `tools/decomp_vortex_pre.py --verify`의 SHA/고유 입력 감사도 종료0이다. `cpppj/vortex-pre.json`과 `cpppj/tests/fixtures/vortex-pre-x86.tsv`를 따른다. 이 102개를 실제 미션 시작이나 자연 완주 횟수로 세지 않는다.

보호8미션 통합 검사는 **아직 실패**다. 최초 실행에서는 Tutorial1/2 뒤 Tutorial3의 실제 신전 삭제 미연결에서 실패했다. 신전 생명주기 연결 뒤 Tutorial1~4의 Load/Start와 기대 SP·기술·AI 요청 검사를 통과하여 Tutorial5까지 진행했으나, **`Pop 파생 가상 후처리는 아직 지원하지 않습니다`** 예외로 중단했다. Tutorial5의 Load와 Start 중 어느 호출인지 아직 분리하지 않았다. 순회가 멈추므로 Tutorial6와 뒤의 두 캠페인이 이 검사에서 통과했다고 주장하지 않는다.

별도 두 Start 검사는 실제 TheWarBegins의 현재 AI 이름 인코딩/미등록 능력 진단과 필수 UI 효과 실패 시 부분 쓰기 보존을 확인하여 통과했다. 이 두 검사를 캠페인 자연 플레이 검증으로 해석하지 않는다. 실패를 예상 성공으로 바꾸거나 해당 검사를 제외하지 않고 중단 커밋에 보존한다.

초기 로그는 `extracted/goal-mission-start-binding-initial-focused.log`, 신전 연결 뒤 집중 로그는 `extracted/goal-vortex-start-first-focused.log`다. 후자는 **5개 검사 중 1개 실패**이며 기존 전체1,309개 통과 기준선과 구분한다.

## 중단 회귀 검사

최종 고정 소스를 Release로 다시 빌드하여 **종료0·경고0·오류0**을 확인했다. 빌드가 실제 종료된 다음 전체 검사를 시작했다. 전체 내부 검사는 **1,314개 중 1,313개 통과, 1개 실패**이며 실패는 위 `CampaignMissionStart_ProtectedEightMissions`의 Tutorial5 Pop 후처리 예외 한 건이다. 직전 기준선의 기존 검사는 모두 통과했다. CTest는 이 전체 실행 파일을 검사 하나로 등록하므로 결과는 **0/1 통과·종료8**, 검사202.91초/전체202.95초다. 전체 회귀 통과로 표시하지 않는다.

실행 로그는 `extracted/goal-pause-20261016-{build,ctest,tests,audit}.log`다. 신전102개와 기존 요구에너지2,352개 자료의 SHA/고유 입력 감사, 새 Python 구문 검사, 변경 파일 UTF-8 및 `git diff --check`는 통과했다. 기존 요구에너지 제품/fixture는 변경하지 않고 CMake3파일의 현재 SHA와 등록 변경 이력만 갱신했다. 새 [검증 명세](../../cpppj/mission-start-session.json)에 현재 소스/로그 SHA와 성공·실패 범위를 함께 기록한다.

## 재개 지점

1. 위 실패 검사를 그대로 재현한 뒤 Tutorial5의 Load/Start 경계를 분리한다. 누락된 실제 객체의 타입과 VT를 확인하고 원본 PostPop 몸체 또는 기존 복원된 분배를 연결한다. 예외를 숨기거나 검사를 약화하지 않는다.
2. 보호8미션 Start 검사와 기존 전체 검사를 모두 통과시킨다. 현재 Start 바인딩은 헤드리스 Session 통합이며 실제 GUI/UberGump 경로에 설치된 상태가 아니다.
3. 같은 실제 Session/UberGump에 로컬 Zone→0x27→수신 큐→worldLoad/Ready40→벽시각 로그인 종료→Guide→Stage3를 연결한다. 상태9 SendLogin callback에서 로그인 응답을 즉시 분배하지 말고 phase10 확정 뒤 실제 큐에서 분배한다. 로그인 타이머 종료가 pending0을 쓰는 것으로 바꾸지 않는다. Guide seq3은 Start이고 seq6은 해제다.
4. 일반 Construct/빌더23/manaBolt39 이동, 실제 생산·구매·연구, 다리/수확/운반/희생, 자연 승패·저장·다음 미션과 그래픽·음향 동일성 검증은 여전히 남아 있다. 사제 해방의 실제 placement 연결과 Player별 Factory/공급원 초기 capacity도 후속 확인 대상이다.

Early Missions1~6 및 캠페인1-1~1-2 자연 완주와 원본 동일성 목표는 **미달성**이다. 이번 커밋은 실패와 재개 지점을 보존하는 중단 체크포인트이며 목표 완료가 아니다. AGENTS.md를 커밋 직전 다시 읽고 **`20261016-01 CP 신전 삭제 복원과 미션 시작 회귀 실패 및 중단 인수인계`**로 로컬 커밋한 뒤 목표를 `paused`로 전환한다.
