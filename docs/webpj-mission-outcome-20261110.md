# webpj 미션 승패 조건(RawMissionOutcome) 이식 — WB04-E233

2026-11-10, HJOW-Athlon. 미션의 플레이어/팀 승패 조건과 한 번만 발생하는 스크립트 사건(Check 004c36c0, Event 004c3940, 사제 사망/포획 004922e0)을 옮겼다. 플레이어 1~8의 신전 절반 파괴·신전 파괴·사제 사망·사제 포획·사제 구출 사건과 우리 팀/적 팀 전멸·로컬 실패 사건을 번호 순서로 검사하고, 사건마다 DWORD 잠금으로 한 번만 처리하는 규칙이다. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절의 "순서4 이후로 넘기는 `o/` 단위"에 속하며 기준선 뒤 변경이 없고 원본 관측이 있어 E231 미션 공통 프레임 문에 이어 옮겼다. 선행 단위 `RawPriestLookup`(E66)·`SquidReward`(최대 HP)·`PriestPostPopState`(E63)를 그대로 쓴다.

- cpppj 기준선 `1f00a25a`. 소스(`RawMissionOutcome.cpp/.h`)·시험(`MissionOutcomeTests`)·fixture(`missionoutcome-x86.tsv`) 모두 기준선 뒤 변경이 없다(`git diff --numstat 1f00a25a HEAD` 를 저장소 루트에서 확인, 출력 없음).
- 근거 커밋: 소스 `da6c61ae`(2026-10-25), fixture `a5afdafb`(2026-10-13).
- 원본 fixture: `missionoutcome-x86.tsv` 에서 10.78(`originals`) 364행(CD 10.72 `originalCD` 364행은 웹 범위 밖). 입력 19칸 중 앞 15칸이 로컬 번호·데모·Tell 차단·섹션 발견 여부·콜백 변이 번호·영역 소유자 반환·조회 인자·플레이어 9명의 활성 플래그/신전 SID/사제 상태·방향별 9×9 동맹 DWORD·43개 사건 잠금·사제 목록·슬롯 raw 노드이고, 출력은 종료 시점 입력/잠금 전체 문자열·효과 호출 순서(와 그 당시 상태)·반환값이다.
- 공유 장치: `raw-priest-lookup.js`(소유 사제 조회), `squid-reward.js`(`maxHitPoints`), `raw-priest-post-pop.js`(`PriestPostPopState`·`resetPriestOwnerState`), `squid-postpop-state.js`·`sp-store.js`·`random.js`(보상 구성).

## 1. 옮긴 것

`www/core/raw-mission-outcome.js`(DOM 없음, 10.78 전용)

- `createMissionOutcomeState()`: 독립 입력 상태(플레이어 0~8의 활성 플래그/신전 SID, 방향별 9×9 동맹, 플레이어별 5개 잠금, 팀 전멸 둘과 실패 잠금, 로컬 번호 1·데모 0·Tell 차단 0).
- `RawMissionOutcome(pool, priests, state, hooks)`: 10.78 풀과 필수 효과 5개(`maxHitPoints`·`regionOwner`·`section`·`closeTransientGumps`·`onSection`)가 있어야 생성된다(누락은 TypeError). 범위 밖 번호 진단 `invalidPlayerAssert` 는 선택이다.
- `check()`: 활성(Player+0x50 & 0x22)인 플레이어마다 번호 순서로 `ai<번호>TempleHalfDead`·`TempleDead`·`PriestDead`·`PriestCaptured`·`PriestSaved` 사건을 검사한다. 신전 절반 파괴는 신전이 없거나 HP < 최대 HP/2(0 방향 절삭, HP 는 최대 HP 조회 **전에** 읽으며 정확히 절반이면 아님), 신전 파괴는 없음 또는 raw dead 비트만 보고 free/void 는 보지 않는다. 사제가 죽지 않았으면 로컬 번호와 두 방향 동맹이 모두 비영일 때 우리 팀, 아니면 적 팀으로 센 뒤 `GoodTeamDead`·`BadTeamDead`·`Failed`(로컬 사제 사망)를 검사한다. 신전 파괴만으로 승리하지 않는다. 로컬 번호가 8 초과이면 동맹 조회 전에 RangeError.
- `event(target, key, condition, name)`: 조건이 참이고 잠금이 0 이면 데모일 때 `Demo` 접두를 붙인 이름으로 Tell 차단 DWORD 가 0 일 때만 사건 섹션을 실행하고(발견되면 임시 Gump 정리 → 미션 가상 통지) **콜백이 끝난 뒤** 잠금에 1 을 쓴다. 섹션이 없어도 차단이 없으면 잠금을 확정한다. 재진입을 막지 않는다. C++ 의 잠금 참조는 `(target, key)` 쌍이며 target 은 함수일 수도 있다(콜백이 잠금 표를 갈아 끼워도 현재 것에 쓰기 위해).
- `priestDead(player)`: 범위 밖은 진단(인자는 부호 없는 DWORD) 뒤 true, 비활성은 true, 활성 소유자는 ownerState 가 2 일 때만 true. `priestCaptured(player)`: 범위 밖 true, 비활성 false, 활성은 ownerState 가 1 일 때만 true. 공유 소유자 상태 표를 빌렸으면 그 표를 매번 읽는다.
- `bindCurrentInputs({ players, allies, localPlayer, demo, tellBlocked })`: 실제 9 Player 원문(9×180 BYTE)·방향별 81 DWORD·세 getter 가 정확히 있어야 연결하고(아니면 이전 연결 유지), 연결 뒤에는 독립 입력 상태를 읽지 않고 조회마다 현재 원문을 읽는다.
- `makeMissionOutcomeHooks(pool, reward, hooks)`: 같은 풀의 `SquidReward.maxHitPoints` 만 연결하고 다른 풀이면 TypeError.
- CD 10.72 분기(HP 2바이트·Player 기록 폭 0xac·신전 위치 0x74·범위 진단 없음)는 웹 범위 밖이라 옮기지 않았다.

## 2. 검증

`tests/mission-outcome.test.mjs`(6건)

- 원본 관측 재생: 10.78 364행 전부(독립 입력 상태). 행마다 새 풀·사제 상태·플레이어 9명·동맹·43개 잠금·사제 목록·raw 노드·TYPE 최대 HP 를 입력대로 구성하고 종료 시점 전체 문자열·효과 순서와 당시 상태·반환값이 같은지 대조한다. 콜백이 입력을 바꾸는 변이 1~4(섹션 뒤 로컬/활성/사제/신전 변경, 정리 뒤 Tell 차단/데모, 통지 뒤 잠금/사제, 영역 뒤 로컬 변경)를 포함한다.
- 원본 관측 재생(실연결): 같은 fixture 에서 콜백 변이 행과 단방향 0/비영 동맹 조합 8행을 실제 Player 원문 기록·방향별 동맹 DWORD·현재 전역 getter 만 읽는 연결로 다시 재생한다. 이때 독립 입력 상태를 일부러 오염(로컬 9·데모 7·차단 9)해 오래된 사본을 읽으면 실패하게 했다.
- cpppj 계약 포팅 3건: 같은 사건 콜백의 재진입이 최종 잠금 쓰기 전에는 실행 가능(`SSCVCV`), 연결 누락·다른 풀 보상 거부, 공유 소유자 상태 표의 2/1/0 변경을 즉시 읽고 데이터 reset 이 같은 주소에 적용(사제 목록·fallback 표 불변).
- 웹 계약 1건(원본 fixture 에 없는 경계): 범위 밖 번호 −1 의 진단 인자가 4294967295, 음수 최대 HP −3 과 HP −2 에서 절반 −1(0 방향 절삭)이라 `ai1TempleHalfDead` 가 발생, 로컬 번호 9 는 동맹 조회 전에 거부, 실연결 입력의 길이 초과와 세 getter 각각의 누락 거부.

최초 실패와 교정: 모듈은 처음부터 일반 재생·실연결 재생이 모두 통과했다. 시험 쪽 결함 하나를 고쳤다(실연결 관찰용 신전 DWORD 조합에서 `|` 와 `>>>` 의 우선순위로 부호가 틀렸다. 비정렬 DWORD 읽기 도우미를 썼다). 모듈 사본을 바꿔 돌려 비교가 작동하는지 확인했다. 확인 뒤 원본으로 되돌렸다(diff 일치). 처음 변형 61건 중 9건이 살아남아 5건은 웹 계약 1건을 더해 잡았고 4건은 동등 변형이다.

- 실패한 변형: 활성 마스크·dead 비트·필드 위치(상태/HP/좌표/활성/신전)·기록 폭, 번호 범위 양끝·진단 유무, 사망/포획의 비활성·상태 값 판정, 사건의 조건·잠금·데모 접두·차단·섹션 뒤 정리/통지 순서·잠금 값·콜백 뒤 쓰기, 신전 절반의 없음 처리·부호 있는 HP·`≤`, 신전 파괴의 없음 처리·dead 비트, 구출의 없음 처리·비교·좌표 y, 순회 범위(시작/끝)·비활성 건너뛰기·이름 접두·사건 순서·구출 제거·팀 집계 게이트·동맹 `&&`/`||`·우리/적 팀 교환·실패 판정, 실연결 읽기(플레이어 플래그/신전/동맹/로컬/데모/차단)와 길이/getter 거부, 필수 효과 검사·보상 풀 검사·최대 HP 연결(각 1건 모두 실패).
- 적용되지만 통과한 변형(동등 변형): ① 구출 판정에서 로컬 번호를 슬롯 범위 검사보다 먼저/나중에 읽기 — 슬롯 읽기에 부작용이 없다. ② 동맹 두 방향의 `&&` 평가 순서 교환 — 두 조회 모두 순수 읽기라 결과가 같다. ③ 모듈의 슬롯 범위 검사 제거 — `SidPool.slot` 이 같은 RangeError 를 던진다. ④ `at()` 첨자 검사 제거 — 로컬 번호 8 초과 검사가 먼저 걸려 `check()` 경로에서 도달하지 않는다.

## 3. 경계와 남은 것

- 실제 미션/세션에 연결하지 않았다. 사건 섹션 실행·임시 Gump 정리·미션 가상 통지·영역 소유자 조회와 `RawMissionFrameGate`(E231)의 `checkOutcome` 연결, 호출 지점(프레임 문 안의 004c36c0)은 순서4·6 몫이다(PORT_MAP session 0).
- CD 10.72 364행과 CD 분기는 웹 범위 밖이다.

## 4. 다음 시작점

WB04-E234 는 3.1절에서 고른다. 기준선 뒤 변경이 없는 단위(`RawTerrainGrowth`·`RawTotalMade`·`RawMissionStart`·`RawObjectTraceMain`)를 권한다.
