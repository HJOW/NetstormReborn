# webpj 건설 위치 Tab·손실 위치 U·사제 선택 P/R·다음 유닛 N 키 입력 이식 — WB04-E22

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 스물두 번째 부분이다. cpppj 기준선 `1f00a25a`의 `client/ConstructionHistoryKey`·`client/LostPositionKey`·`o/RawConstructionHistory.h`·`client/PriestSelectionShortcut`·`client/NextUnitSelection`(10.78)을 옮겼다. 모두 일반 키 입력 하나가 선택·카메라에 미치는 순서를 다루며 각각 독립 원본 관측이 있다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다(Node 단위 검사만). 기준선 뒤 cpppj 변경은 없다.

## 1. 옮긴 것

### `www/core/construction-history-key.js`

- 이력 `{points: [{x,y}×5], index}`는 Client 장기의 값형 장부다(`createConstructionHistory`). `initializeConstructionHistory`는 40 BYTE 다섯 좌표만 (−10000, −10000)으로 쓰고 순번은 보존한다. `isConstructionHistoryPointValid`는 `0 < 각 축 < float(signed 세계 크기)`(0/상한/NaN 무효).
- `dispatchConstructionHistoryKey`(Tab=9): 뗌이거나 다른 키면 미소비. 정지면 효과 없이 소비. 아니면 현재 순번 좌표를 먼저 복사하고, 무효면 순번 0과 첫 칸으로 복귀, 유효하면 `center(x,y)` 후 카메라가 바꾼 현재 순번을 다시 읽어 `(순번+1)`(32비트 감김)을 signed로 5로 나눈 나머지를 쓴다. 순번이 5 이상이면 임의 modulo 없이 `RangeError`.
- `dispatchLostPositionKey`(U/u, 004d98e2..004d990d): 수정키·outside는 거부 조건이 아님. 뗌이면 미소비, 정지면 소비, 정지 callback이 바꾼 현재 좌표/세계 크기로 유효점일 때만 `center`.

### `www/core/selection-shortcuts.js`

- `selectPriestByKey`(P/R, 004d9884..004d98e1): 조회 전에 선택 SID를 캡처 → 참여 조회 → 후보가 다르면 반복 카운터 0 → 같고 카운터가 비0이면 현재 owner/viewCommand로 일반 큐에 (명령, owner, owner) 예약 후 끝 → 아니면 `select(후보,0,1)` 후 현재 카운터 +1(32비트 감김). 카운터는 Client 수명의 DWORD다.
- `selectNextMovingUnit`(N, 004da880): free만 건너뛰는 한 번 순환 검색(소유자 BYTE=캡처한 DWORD, TYPE flags2 `0x30000`), 후보가 없으면 끝, 있으면 `select`→`center`(첫 선택 성공 여부와 무관)→정지면 끝→Shift가 비0이면 후보가 이동 가능하고 현재 선택이 살아 있는 이동 유닛일 때 `multiple(후보,1)`, 아니면 `select` 한 번 더. dead/내용물 상태는 추가로 제외하지 않고 넓은 DWORD SID를 WORD로 먼저 자르지 않는다.

## 2. 검증 (`tests/selection-keys.test.mjs`, 7건)

| 검사 | 근거 |
| --- | --- |
| Tab 키 511행(초기화 3 + Tab 508, 전체 40 BYTE 이력·순번·호출·Center 중단) + 지연 관문/첫 칸 복귀 부분 효과 | `construction-history-key-x86.tsv` |
| U 키 1022행(모달·정지·두 축 경계·수정키·다른 사건·부분 중단) + 현재 참조/지연 경계 | `lost-position-key-x86.tsv` |
| P/R 키 220행(조회 전 캡처·장기 카운터·일반 큐) + 부분 실패/감김 | `priest-selection-shortcut-x86.tsv` |
| N 키 410행(8개 슬롯 상태·owner·TYPE flags·순환·free·dead 전체) | `next-unit-selection-x86.tsv` |

최초 실패와 교정: U 키 1022행 중 674번째에서 시험 장면이 신호 NaN 좌표(`0x7f812345`)를 JS 수로 바꿨다가 되돌리면서 비트가 `0x7fc12345`(조용한 NaN)로 바뀌어 불일치했다. 모듈이 아니라 시험 장면의 표현 문제여서, 장면이 좌표를 원시 32비트로 보관하고 모듈 호출 때만 수로 바꾸도록 고쳤다. 모듈은 좌표를 변경하지 않으므로 영향이 없다. **연결 시 주의:** 실제 Client에서 손실/건설 좌표를 보관할 때 NaN 비트를 보존해야 하는 경로는 `Float32Array` 대신 원시 `Uint32Array` 비트로 두는 편이 안전하다(정상 좌표에는 영향 없음).

## 3. 경계

- cpppj `PriestSelectionShortcut_CurrentOwnerQueueAndCounterLifetime`(`ClientUserSelectionGlobals`의 카운터가 World reset에도 유지됨)·`NextUnitSelection_ActualSelectionAndPauseOrder`(실제 `Select`/`ToggleMultiple` 연결)·`ConstructionHistoryKey`의 실제 Client 수명 검사는 해당 Client/UI 단위에서 대조한다.
- `RawConstructionHistory.h`의 상태 구조체는 이 모듈의 `createConstructionHistory`로 옮겼다. 배치 통지가 순번을 0으로 쓰는 부분은 건설 통지 단위의 몫이다.

## 4. 다음 시작점

`RawGraph`/`RawGraphLive`, 공통 `SquidPostPop` 본체, `RawSquidPoolReset`, `RawBridgeConnect`, 그리고 Player 나머지(`RawPlayerWorld`·`RawPlayerTechnology`·`RawPlayerRegular`·`RawPlayerTemplateState` 등).
