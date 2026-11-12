# webpj 신전 의식 RawTempleRitual (WB04-E365, 2026-11-13, 7-P3 신전·제단·의식 부품 첫 항목)

3.1절 7-P3 첫 항목 "RawTempleRitual 266줄(+18)"을 `core/raw-temple-ritual.js` 로 옮겼다. 사제가 신전의 의식 목표점에 서면 단계별로 사제 동작(K..P 프레임)·위치 음향·0x307 공유 예약을 하고, 후속 모드에서는 강한 사제의 후속과 권한이 있을 때의 룬 생성을 하는 몸체(00447e40)와 그 보조(가상 count·단계·SHP 목표점·룬/사제 조회)이다. 기준선 뒤 `20261109-95 CP`(삭제의 장기 소유자가 쓰는 정적 `FindStrongOwnedPriestIn`·현재 사제 TYPE 상자)와 `20261110-52 CP`(의식계 정기의 장기 연결: 정적 `StageFromCount`·현재 룬 TYPE/권한 상자)를 반영했다(PORT_MAP base `21bb2550`).

## 1. 규칙

- **구성:** `new RawTempleRitual(pool, hash, types, frames, shapes, spots, footprint, hitPoints, actions, state, hooks)` — 풀은 PATCH, `types`·`frames`·`shapes` 는 188칸(TYPE 레코드 `{ flags1, flags2 }`, `FrameCodeTable`, SHP 자료 `{ loaded, frameCount, frames[{ muzzleX, muzzleY }] }`), `spots` 는 지면 지도 65536바이트, `footprint` 는 같은 풀/해시의 `RawSquidNeighbors`, `hitPoints` 는 같은 풀의 `SquidReward`(최대 HP), `actions` 는 같은 풀의 `RawPriestRitualActions`, `state` 는 `TempleRitualState`(권한·사제/룬 TYPE 번호·픽셀 배율과 장기 값을 빌리는 `{ value }` 상자). 하위 연결의 풀/해시가 다르거나 표 크기가 다르면 거부하고, 훅(`create`·`setHitPoints`·`setOwner`·`pop`·`playAt`·`reserveShared`·`addShared`·`count`·`assertion`)은 도달한 것만 요구한다.
- **가상 count (`count`):** 현재 TYPE 의 프레임 표에서 현재 프레임(signed DWORD)의 방향 글자 `N`→1·`M`→2·`L`→3 이 우선이고, 그 밖은 프레임 63 이하 1·64~119 2·120 이상 3 이다. 프레임이 표 밖이면 거부한다. **단계 (`stage`)**: 현재 프레임(−1)을 먼저 저장한 뒤 가상 count 를 한 번 조회하고(`hooks.count`; 없으면 raw 가상 표가 실제 Dais 0x507950 일 때만 직접 count), count 별 첫 프레임 `[1, 3, 64, 120]` 을 뺀 값을 `<11` 0·`<22` 1·`<33` 2·`<44` 3·`≤54` 4·그 위 5 로 나눈다(int32 감김). `stageFromCount` 는 저장한 값만 쓰는 정적 본문이다.
- **목표점:** `stageAnchor(temple, stage)` = 신전 좌표 + (첫 count 구간 프레임 3 + 오프셋 `[0, 11, 22, 33, 44]`)의 SHP **포구 쌍**(헤더 +0x20/+0x22, 표시 기준점이 아님)÷축별 픽셀 배율(각 축을 합산 뒤 **한 번만** 단정도로 저장; 배율이 비유한/0 이면 거부). `stagePoint` = Anchor + 현재 룬 TYPE 의 해당 단계 포구 쌍이고, 단계 4 는 절삭 칸의 지도 값이 `(spot & 6) && !(spot & 8)` 일 때 (−9, −6) 픽셀을 더 보정한다(칸 번호가 0(−0 포함)이면 허용, 음수나 지도 길이 이상이면 거부). SHP 는 로드되어 있어야 하고 프레임이 `frameCount`(`flags1 & 0x440000` 이면 두 배)와 프레임 배열 길이 안이어야 한다.
- **조회:** `runeAt(temple, point)` 는 현재 단계의 룬 포구를 빼고 점 finder 로 첫 현재 룬 TYPE 슬롯을 찾는다(룬 `flags2 & 6` 이 0 이면 표면 층 건너뜀). `findStrongOwnedPriest`(정적 본문 `findStrongOwnedPriestIn` 은 삭제의 장기 소유자도 쓴다)는 신전 발자국을 위·아래·왼쪽·오른쪽으로 한 칸씩 넓혀(우하단은 거의 올림 0.99999) 같은 소유자이고 signed HP ≥ 후보 최대 HP÷2(0 쪽 절삭)인 첫 사제. `hasAbstractRune` 은 발자국의 첫 현재 룬 중 extra 비트 0 이 선 것. 모두 좌표 절삭이 비유한이거나 ±2^31 밖이면 거부한다.
- **룬 생성 (`spawnRune`):** `create`(현재 룬 TYPE, flags 0; 슬롯이 0 이거나 풀 밖이면 거부) → HP 0 → **현재 단계**의 프레임 DWORD(옆 바이트 보존) → 슬롯이 void(비트 4)가 아니면 진단(`isVoid()`, `c:\ns\o\squid.h`, 0x282) 뒤 계속 → extra 비트 0 → 신전의 **현재** 소유자 → **다시 읽은** 현재 단계 Anchor 에 `pop(…, flags 0)`.
- **의식 전체 (`ritual(temple, priest, mode)`):** 단계가 5 면 아무것도 하지 않는다. 사제 위치가 현재 단계 목표점과 **두 축 모두** 같아야 하고, 아니면(단계 4 가 아닐 때만) 다음 단계 목표점과 비교한다. 같은 자리에 이미 룬이 있으면 반환. mode 0: 사제 동작 시작(`actions.begin`) → **현재 사제 위치**에서 단계별 음향(`forWind2/forSun2/forRain2/forThunder2/forStorm2.wav`) → 확보가 되면 신전에 0x307 공유 예약(지연 float `1.96/2.25/1.88/2.22/2.41`). 그 밖의 mode: 강한 사제가 있으면 그 사제를 **현재 단계**로 후속시키고, 현재 권한(상자 포함)이 있고 추상 룬이 없으면 룬 생성. 가상 count 조회는 단계 계산·룬 조회마다 한 번씩이라 mode 0 은 2회, 후속 모드는 3회(생성 시 5회)이다.
- **훅 공장:** `makeTempleRitualHooks(pool, factory, owner, host, hooks)` 는 실제 Factory 생성(호출자가 준 flags 그대로)·공통 HP setter·Owner·Host 공유 Regular 를 연결한 새 묶음을 돌려주고 나머지 훅은 보존하며 풀이 다르면 거부한다.

## 2. 웹 범위

- 10.72(CD)는 범위 밖이다. 위치 음향·Pop·예약 훅의 실제 구현과 `PathCleanup` 의 의식 경계 조립은 후속 단계 몫이다. 이 모듈은 DOM 을 쓰지 않는다.

## 3. 검사 (`tests/temple-ritual.test.mjs` 23건)

- **원본 독립 관측:** `templeritual-x86.tsv` 의 10.78(originals) 3,360행(모드 0/1 각 1,680) — 모드·현재 프레임·방향 글자·사제/룬 프로필·권한·예약 결과·지면 값·위치·효과 뒤 변이를 입력으로 가상 count·단계·Anchor/Point 좌표 비트·룬 조회·강한 사제·추상 룬, 의식 전체의 효과 순서, 네 슬롯의 raw 전체(가상 표 DWORD 제외), 최종 권한이 **첫 실행에서 전부** 일치했다(cpppj `rawTempleRitual_native_stage_positions_queries_and_effects`).
- **cpppj 시험 이식(실제 소비자):** 실제 Factory/Owner → 같은 Frame → 실제 0x307 Host 예약의 mode 0 사슬(`rawTempleRitual_real_frame_and_307_shared_consumer`)이 첫 실행에서 통과했다.
- **웹 계약(21건):** 생성자 검사(PATCH·하위 연결의 풀/해시·표 크기 네 갈래), `stageFromCount` 경계 표(count 별 첫 프레임·감김·범위 밖 count), count/stage(방향 글자 우선·구간 경계 63/64/119/120·가상 조회 한 번·frame −1 은 조회 전에 저장·명시 frame·실제 Dais 직접 count·뒤 바이트 무관), 좌표 절삭 범위, SHP 한계(프레임 수 경계·두 배 비트·배열 길이·로드 여부), 목표점 계산(포구 쌍 vs 표시 기준점·축별 배율·단정도 저장 한 번(142÷3 + 75.86727905273438 은 두 번 저장하면 123.20060729980469)·배율 한쪽만 비유한/0·음수 배율), 단계 4 지면 칸 경계(0·−0·255.5·65536·음수·행 우선), 룬 조회의 층 선택(flags2 비트 1·2)과 룬 TYPE 오류, 사제 조회의 네 방향 한 칸 경계와 홀수·음수 임계 HP, 추상 룬의 거의 올림 경계(정수/작은 소수부), 의식 전체(단계별 음향·지연 float·두 축 일치·다음 단계 점·단계 4·기존 룬·count 조회 횟수), 사제 동작 뒤 위치 재조회, 후속 모드(후속 사제·권한 상자·추상 룬·약한 사제), 룬 생성 세부(추상 비트·void 진단 조건·효과 뒤 단계 재조회·옆 바이트 보존), 훅 누락 거부, 훅 공장 둘, 상태 상자.
- 시험이 먼저 실패한 곳은 모두 시험 쪽이었다(발자국이 좌표 기준 우하단이라 사제 위치 가정이 틀린 입력, 대상 사제와 강한 사제를 한 슬롯으로 둔 입력, 슬롯 상태 비트 1 이 free 라 공통 setter 가 거부하는 입력, 옆 바이트 위치 오기). 모듈은 관측 대조에서 바뀌지 않았다.

## 4. 변형 확인과 남은 경계

- 연산자·상수 변형 198 + 의미 변형 119 = 317회를 적용했다. 1차에서 67개가 남았고 그중 **56개는 시험의 공백**이라 보강해 잡았다(원본 관측은 모두 같은 합성 SHP(포구 = 표시 기준점)·겹치는 위치·정수 좌표·한 슬롯의 사제를 쓰고, 한 축만 어긋난 목표점·해시 층·발자국 경계·오류 문구·훅 호출 횟수·단정도 이중 반올림은 가려져 있었다).
- 남은 동등 변형 11개(이유 7가지): 음수 절삭 한계 −2^31 을 −2^31−1 로(그 사이에 표현되는 float 가 없음)·`DataView` 크기 4→5·`TYPE ≥ types.length || TYPE ≥ frames.length` 의 `||`/경계(두 표 길이가 같아 서로 가려짐)와 한쪽 제거·프레임 DWORD 쓰기의 `8 * index` 를 9 로(단계 5 이하라 상위 바이트가 0)·후속 사제 `found && found < capacity` 의 `&&`→`||`/경계/뒤 조건 제거(finder 는 실제 슬롯만 돌려줌)·`frame < 0`(단계에서 오는 프레임은 음수가 아님).
- 다음 7-P3 항목은 **RawRitualChoice**(27줄)이며, cpppj 에서 MP 6b 입력/수신 사전 구현이라 웹 범위에서 단독 도달하는지 먼저 확인한다.
