# webpj Fort 단일 랜덤 배치와 edgeFarm 군집(RawFortRandomPlacement · RawFarmCluster) — WB04-E285 (2026-11-11, 4-B)

3.1절 4-B 의 **RawFortRandomPlacement**(212줄)를 `core/raw-fort-random-placement.js` 로, **RawFarmCluster**(134줄)를 `core/raw-farm-cluster.js` 로 옮겼다. RawFarmCluster 는 3.1절 목록에서 훨씬 뒤(지형·섬 부품 뒤)에 있지만, 단일 배치의 원본 검증 fixture(`farmcluster-x86.tsv`, 군집이 단일 배치 전체를 호출함)가 군집을 요구해서 **목록 순서를 앞당겨 함께 옮겼다**(RawFortRandomPlacement 단독 fixture 는 cpppj 에도 없음). 원본 `0040db90`(단일 배치)·`0040e560`(군집).

## 1. 기준선 뒤 변경 판정 (3.6절)

두 단위 모두 기준선(`1f00a25a`) 뒤 `f7f58f5a 20261109-83 CP 단일 난수 배치와 너겟 배치 부모 및 현재 전역 장기 연결` 한 커밋의 변경이다: `FortRandomPlacementState` 에 `sharedIslandType`/`sharedEdgeFarmType` 포인터와 `IslandType()`/`EdgeFarmType()` 접근자가 생겼고(Native 는 실제 TYPE 전역의 같은 주소를 빌리고 독립 입력은 기존 필드를 씀), 단일 배치의 모서리 조회·edgeFarm 존재 검사·교체 Retype 과 군집의 중심/주변 요청이 그 접근자로 **호출 위치마다 현재 TYPE 을 다시 읽도록** 바뀌었다. 웹은 `FortRandomPlacementState` 에 같은 의미의 `sharedIslandType()`/`sharedEdgeFarmType()` 함수와 `currentIslandType()`/`currentEdgeFarmType()` 를 두어 반영했다(**① 자동 반영**, PORT_MAP base `f7f58f5a`).

## 2. 규칙

- **단일 배치 `place(localRandom, request)`:** request = `{ type, x, y, region, mode, force }`. ① 반올림 spot(x+0.5, y+0.5 의 CRT 절삭)이 256×256 안인지 확인(밖이면 거부)하고 spot 지도 하위 6비트가 0 이면 0. ② 같은 좌표에 현재 edgeFarm 타입 객체가 이미 있으면 0. ③ 공간 검사(`mayPlace`)가 거절하면 0. ④ force 가 아니면 청크의 지역 번호(signed BYTE)가 요청 지역과 다를 때 0. ⑤ 요청 TYPE 이 지면 프레임(0x200000)이면 지면 객체(isle genus)를 찾아(없으면 assert 거부): mode 가 켜져 있으면 모서리 검사(NW→NE→SE→SW 단락 평가)에 실패할 때 **1(새 SID 가 아님)**, 통과하면 Unpop(0) → Retype(현재 edgeFarm) → HP → Repop(64) 로 지면 객체를 교체해 그 SID 를 돌려준다. mode 가 꺼져 있으면 지면의 현재 프레임을 가져온다. ⑥ 임의 프레임(0x100000)이면 고정 P/P → 변환 P/P → P/A → P/임의 변형의 첫 비어 있지 않은 집합(최대 256개)에서 전역 난수 1회로 고른다(후보 없음 −1). ⑦ 프레임 −1 은 assert 거부. ⑧ factory 생성(type, 0) → 프레임 직접 쓰기(가상 SetFrame 없음) → HP → owner(force 면 0, 아니면 지역 owner, 덮어쓰기 전역이 있으면 그 값) → Pop(flags 0). `localRandom` 인자는 원본 단일 몸체에서 쓰이지 않는다.
- **`retype`:** 새 객체가 void 상태이고 두 TYPE 의 foot 이 같을 때만 raw 타입 BYTE 를 바꾸고 현재 생성자만 재실행한다(postCreate 는 부르지 않음).
- **군집 `place(area, region, quantity)`:** 지역 record 시드(청크를 y 바깥/x 안쪽으로 훑어 처음 만나는 island 번호가 region 인 청크의 record BYTE+3 × 77)로 별도 local 난수를 만들고 목표 = 수량 × 20. archipelago 이고 지역 0 이면 사각형을 섬 확장 단위(span×16)만큼 넓히고 목표를 (목표/9 signed) × (span×2+2)² 로 바꾸며 force 1·반지름 2. 높이→폭 순서의 local 난수로 중심을 고르고(limit < 1 이면 난수를 소비하지 않음) 단일 배치(mode 1, 현재 edgeFarm TYPE)를 시도. 성공하면 목표를 줄이고 실패 한도(30000)를 재설정한 뒤 중심 주변을 x 바깥/y 안쪽으로(중심 포함) 시도하며 성공할 때만 줄이고 0 이면 멈춘다. 최초 중심은 수량 0 이어도 시도한다. 좌표는 float32 로 저장한다.
- `FarmClusterState` 의 archipelago 는 `sharedArchipelago()`(Login.bb4 와 다른 bb0 DWORD)를 직접 빌릴 수 있고 반대 bool 사본은 무시한다. 섬 확장 단위도 `sharedArchipelagoSpan()` 로 매번 읽는다.
- `makeFortRandomPlacementHooks`/`makeFortSinglePlacementHook`/`makeFarmClusterPlacementHook`: 같은 pool 의 pipeline·factory·reward·finish/cluster 훅 연결을 조립한다(다른 pool 은 거부).

## 3. 검사 (`tests/farm-cluster.test.mjs` 4건)

- `farmcluster-x86.tsv` **originals 104행**: 실제 단일 풀(32768)·hash0·spot 지도·타입(isle 97 · edgeFarm 121)·청크 지역·두 난수에서 군집을 실행해 배치 시도 횟수 · 단일 진입/반환 DWORD trace 의 Adler · 원시 효과(공간 L · 지역 owner I · 생성 C · 생성자 K · owner O · Unpop U · Repop R · Pop P) 순서 Adler · 변경된 지면과 새 객체의 전체 raw · 전역/local 난수 상태가 모두 같다. **첫 실행에 모두 통과했다.** 같은 104행을 공유 archipelago(bb0)로도 재생했다(cpppj `GameAux_FarmClusterUsesSharedArchipelagoNative104`). CD 104행은 웹 목표(10.78) 밖이다.
- 계약 5건(`farm-cluster.test.mjs` 총 7건 중): ① 구성 거부(필수 효과 누락·다른 월드·어댑터 pool 불일치·표/spot 크기), ② 단일 배치 경계 — 반올림 spot 위치(`x+0.5`)·spot 하위 6비트 마스크·청크 좌표 순서(x/16, y/16)·지면 없음 assert·mode 0 의 지면 프레임 읽기·후보 256 상한과 전역 난수 limit·프레임 코드 패스 순서(P/P → P/A → P 방향 아무 변형)·후보 없음 −1 assert·owner 0/덮어쓰기/force/지역 불일치, ③ 모서리 규칙의 네 방향·보드 끝(x=255 의 오른쪽)·Retype 의 void/foot assert·교체한 자리 재배치(level 1 로 옮겨져 0)·CRT 좌표 범위 거부(메시지로 구별), ④ 군집 경계 — 지역 시드의 signed 비교와 청크 순회 순서(y 바깥)·limit<1 의 난수 비소비·실패 한도 재설정(10 실패 뒤 성공 → 총 시도 30019)·현재 edgeFarm TYPE 재조회·float32 저장.
- 변형 확인: `raw-fort-random-placement.js` 41개, `raw-farm-cluster.js` 30개(+추가 6개). fixture 104행과 첫 계약만으로는 단일 배치 15개·군집 6개가 살아남았다(균일한 spot 지도·청크 지도·모서리 모드 1 위주라 구별하지 못함). 위 계약 ②③④로 모두 잡았고 **남은 1개는 동치 변형**이다: finder 의 표면 건너뛰기 플래그 제거(`find` 플래그) — 이 장면의 Rehash 정책상 교체된 지면이 level 1 로 옮겨져 level 0 에 edgeFarm 타입 객체가 남을 수 없다.

## 4. 남은 경계

- 실제 `ChunkMap`(partial)은 최소 계약(`xLength`/`yLength`/`at(x,y) → { island, record }`)으로만 사용했다. 4-B 의 ChunkMap 항목에서 실제 지도와 연결하고 이 군집 재생을 다시 닫는다.
- FinishOwner(E221)는 이미 `place` 훅으로 이 단일 배치를 받는다. `RawFortWorldLoad`·`NormalTerrainWorldBindingTests`·`FortWorldLoadBindingTests` 의 월드 연결 시험은 해당 항목에서 소비한다.
- 3.1절 4-B 는 16/30 이다(14 + RawFortRandomPlacement + RawFarmCluster). 다음은 **RawFortWorldLoad**(120줄 + client RawFortWorldLoadBinding 40줄)이다.
