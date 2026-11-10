# webpj SquidPostPop 마감(표시 열·Native 진입·몸체 연결 점검) — WB04-E258

2026-11-10, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절 단계 3-D의 둘째(마지막) 항목이다. 공통 후처리 처리기 `SquidPostPop`([E41](webpj-squid-postpop-20261108.md): 가상 분배·사전 검사·공통 장부·비용·`setBody`/`set*Prefix`)이 E41 에서 남긴 세 가지를 닫는다: ① 표시 dirty 열의 원본 대조 ② 기준선 뒤 cpppj 의 장기 Client(Native) 진입 ③ 3-A~3-C 에서 옮긴 몸체의 연결 점검. 게임·브라우저 실행과 디컴파일은 하지 않았다(Node 단위 검사만).

## 1. 기준선 뒤 변경 판정 (3.6절)

`git diff --numstat 1f00a25a HEAD`는 `SquidPostPop.cpp +18/−1`·`.h +17/−0`·`SquidPostPopPatch.cpp +4/−4`(+36 줄)이며, 커밋은 `be567d46`(20261108-187, 공통 자원 소유권)·`417f22d2`(-220, 장기 Host 수명·자원 PostPop 접합)·`7ba0130b`(20261109-16)·`ffb485bc`(-84)·`56f3c80f`(-88)·`d0e7fe1d`(-89)·`396aa6a0`(-92)·`3c5f5092`(20261110-34)·`b1bf342d`(-44) 등이다. 기존 fixture(`postpop`·`bridgepostpop`·`rawgraph`·`regiongraph`·`graphremove`·`graphrecovery`)와 cpppj 시험은 기준선 뒤 바뀐 적이 없고 10.78 typed 결과는 기준선과 같다.

- **② 재반영(단독 결과를 바꾸지 않는 장기 owner 진입):**
  - `SquidPostPopState.sharedSuppressed`/`readSuppressed()` — 실제 Client 의 현재 metamap 관문(00594fc8). 공통 후처리 진입이 독립 `suppressed` 대신 이것을 읽는다.
  - `bindDefaultPatchDevices({ aiNotice, localOwner, recheckSelection })` — 장기 기본 장치. 현재 Session 이 개별 연결한 통지/로컬 owner/선택 재검사가 있으면 그것이 먼저이고 없을 때만 기본 장치를 쓴다. 셋 중 하나라도 없으면 거부한다.
  - `bindDefaultResourcePostPop(fn)` + `dispatch` 선택 분기 — 기본 장치가 연결되고 현재 파생 몸체가 없는 자원 가상 표(0x508cc8·Factory 0x509148/0x509210·0x5040c0·0x504198·buried 0x50a748·0x50e888·0x509470, 자산 전용 표, Carrier/Damageable 직접 표)를 장기 자원 후처리로 보내고 미설치면 그 위치에서 실패한다.
  - `releaseRuntimeBindings()` — 현재 World/Session 의 파생 몸체 열네 종과 AI 통지/로컬 owner/선택 재검사/Teleport 접두만 반납한다. 장기 장부·섬/표면 접두·기본 장치·기본 자원 후처리·Graph 는 보존한다.
  - `SquidPostPopList.matchesCurrentSource(owner, slot)` — 같은 실제 저장소/행 정체성의 기존 getter 묶음만 재사용한다(`bindCurrent` 의 `sourceOwner`/`sourceSlot`).
- **①(해당 없음):** `runtimePin_`(호출 동안 파생 C++ 객체 수명 유지)은 JS 가 가비지 수집이라 대응이 없다.
- 행 base 는 판정한 `b1bf342d`로 올렸다. 호출자(client Session)와 기본 장치/자원 후처리의 실제 몸체 연결은 4-D 몫이다.

## 2. 표시 dirty 열 대조 (`tests/squid-postpop-display.test.mjs`)

E41 의 `postpop-x86.tsv` 재생은 마지막 두 열(전체 갱신 여부·변경 사각형 목록)을 SquidDisplay 단위로 미뤘다. 이 단위가 cpppj `SquidPostPop_X86_CostListsTotalsAndActiveLifecycle` 과 같은 장면 — 모든 TYPE 의 SHP 프레임 하나(크기 {11,17,12,16}·{19,23,−3,5}), 카메라 0/0 배율 65536, 화면 640×480, `SquidDisplay`+`DirtyRegions` 싱크를 `SquidPop`·`SquidUnpop` 에 연결 — 에서 10.78 `originals` PostPop 384·Pop 72·Unpop 36행 전부의 장부(비용·알림·깊이·세 표·목록·슬롯)에 더해 **전체 갱신 여부와 변경 사각형(왼쪽·위·오른쪽·아래·flags, 병합/순서 포함)**을 재생한다. 처음부터 통과했다(첫 실행 실패 1건은 시험 쪽 — 공통 객체 가상 표를 `allocate` 직후 슬롯에서 읽으려 했는데 웹 풀은 생성자를 실행하지 않아 0 이었다. 상수 0x501dd0 으로 교정).

## 3. Native 진입 시험 (`tests/squid-postpop-native.test.mjs`)

위 ② 항목 각각을 직접 계약으로 시험한다: 공유 metamap 이 독립 값을 이긴다(범위 밖 SID 도 억제 시 읽지 않음·깊이 32비트 감김), 기본 장치 셋 필수·개별 연결 우선·반납 뒤 보존, 기본 자원 후처리가 선택되는 표 열한 개와 선택되지 않는 조건(기본 장치 없음·현재 파생 있음·공통 표)·미설치 실패, 반납이 지우는 것과 남기는 것, 목록 정체성.
변형 시험 26개 중 25개를 탐지했고, 남은 1개(기본 자원 표 집합의 0x509470)는 동등 변형이다 — 그 표는 자산 전용 표 집합(`RawAssetPostPop.hasDedicatedVtable`)에도 들어 있어 어느 쪽 분기든 같다.

## 4. 3-A~3-C 몸체 연결 점검

`setBody(kind, body)` 열네 종과 접두 설정기가 실제 몸체와 이어지는지 각 종류의 시험 위치를 확인했다.

| 종류 | 연결을 확인한 시험 |
| --- | --- |
| `asset`·`carrier`(+Damageable 직접) | `asset-post-pop.test.mjs`(E254, 실제 가상 진입 `postPopVirtual`) |
| `factory` | `factory-post-pop.test.mjs` |
| `bomb`(접두)·`priest`·`rune`·`forcefield`(접두) | `bomb-pre-post-pop`·`priest-post-pop-tail`·`rune-post-pop`·`forcefield-lifecycle(-integration)` |
| `dude`·`player` | `dude-spawn(.test/-support)`·`player-world.test.mjs` |
| `buried` | **신규** `squid-postpop-native.test.mjs`(최초 flag: 추첨→QA→두 예약→공통 몸체 한 번, 해제 뒤 미복원 거부) |
| `carrierSteal` | **신규** 같은 파일(크랩 가상 표 둘, 예약 0x492 뒤 Damageable 공통 장부 한 번) |
| 섬/표면/Teleport 접두 | `squid-postpop.test.mjs`(가상 분배)·`surface-lifecycle.test.mjs`(실제 접두 연결) |
| `balloon`·`flyer`·`dais` | 몸체 미이식(7-P1·7-P3 이월) — `setBody` 종류 이름과 `handles/postPop` 계약만 있고 PORT_MAP "남음"으로 둔다 |

## 5. 경계와 다음 시작점

- `SquidPostPop`은 PORT_MAP `partial`(base `b1bf342d`)을 유지한다. 남음: 벌룬·비행(Flyer/SunFlyer)·Dais 몸체의 `setBody` 연결(각 몸체 이식 때), 실제 World 의 몸체 등록과 기본 장치/자원 후처리/AI 통지의 실제 몸체(4-D). 3-D 완료 기준(아직 없는 몸체는 PORT_MAP "남음:"으로 두고 partial 유지)을 충족한다.
- 3-D 는 2/2 로 끝났고 다음은 3-E(기준선 뒤 cpppj 변경 판정 83단위, 순서3 마감 관문)다.
