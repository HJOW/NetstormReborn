# webpj 스톤 타워 표적 통지(RawGunBlocker) 이식 — WB04-E87

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 여든일곱 번째 부분이다. 10.78 스톤 타워 배치가 주변 기존 Gun(아처/캐논)의 표적 교체를 알리는 후보 수집(`o/RawGunBlocker`, 00469590)과 통지(0046ae80)를 cpppj 기준선 `1f00a25a` 에서 옮겼다. E82 의 `raw-gun-target.js`(끝점 중심)와 E86 의 `raw-fence-target.js`(선분 교차)가 선행이다. 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이다.

## 1. 옮긴 것

`www/core/raw-gun-blocker.js` — `RawGunBlocker(pool, hash, types, targetTypes, targeting, process, state, hooks)`

- `collect(owner, point, capacity = 20)`: 새 타워 발자국(스톤 타워 타입)의 중심을 snap(`+0.99999f` 절삭)한 칸을 중심으로 아처 최대 거리 반경의 일반 finder(flags 5)로 group 0(아처) 후보를 순회한다. dead/free 비트는 따로 배제하지 않고, extra bit0 이거나 `friendlyFire` 가 꺼진 상태에서 동맹이면 건너뛰며, 중심 거리가 반경 이하(포함)이고 현재 표적이 있으며 `거리 < 기존 표적까지 점수`(지상 +1000)이고 포구→기존 표적 선분이 타워 사각형의 두 대각선 중 하나와 교차하면 목록에 넣는다. 이어 0·2·4·6 캐논 방향마다 타워 사각형에서 `cannonRange` 만큼 뻗은 ray 영역의 finder 를 새로 시작해 group 1 후보 중 현재 방향이 `(direction−4)&7` 인 것을 같은 방식(제곱거리 < 기존 표적 점수)으로 모은다. capacity 는 호출자의 signed 용량(통지 20·AI 평가 30)이며 가득 찬 목록에서도 원본 후보 순회를 끝내고 중복도 지우지 않는다.
- `notify(blocker)`: boss 일 때만, 현재 타워의 소유자/원점으로 수집한 뒤 아직 존재하는(현재 표적이 있는) 각 Gun 에 `setTarget(gun, blocker)` 를 같은 순서로 부른다(중복 통지도 보존; 도중에 삭제된 Gun 은 건너뜀).
- 점수/교차는 원본과 같이 표적 발자국(확장 높이 포함)의 반대쪽 edge midpoint 까지의 제곱 거리를 쓰며(아처는 float 저장 뒤 지상 가산, 캐논은 x87 전체 제곱거리), 교차는 E86 의 `RawFenceTarget.intersection` 을 재사용한다.
- `hooks = { currentTarget(sid)→표적|undefined(표적 0 도 실제 값), mode(sid), muzzle(sid)→[x, y], setTarget(gun, blocker) }` 는 기존 Form/Kernel/방향/포구/표적 지정 경계이다.

## 2. 검증

- `tests/gun-blocker.test.mjs`(4건): `gun-blocker-x86.tsv` 690행(수집/통지·group 모드·총 flags/extra·동맹·친구 발사·방향 오프셋·현재 표적 없음·추상·용량·역순·타워 좌표/발자국/확장 높이·boss)의 수집 목록과 통지 순서가 같고(비어 있지 않은 결과 100행 초과), 원본 20개 한도에 못 미치는 장면의 AI 용량 30 재생, 최대 반경 20 의 아래/동일/위 경계 6행(`gun-blocker-boundary-x86.tsv`)이 같다. 계약: boss 가 아니면 통지하지 않고 필수 경계 누락·판본/타입 크기 거부.

최초 실패와 교정: 시험 장면이 free/dead 상태 독립 입력의 슬롯에 `allocated` 로 쓰다가 "할당되지 않은 SID 쓰기"로 거부되어, 원본 시험처럼 상태와 무관한 `physical` 슬롯 대여로 고쳤다(제품 코드는 변경 없음).

## 3. 경계와 남은 것

- 기존 Gun 의 현재 표적/방향/포구(`GunProcess`·`RawGunMode`)와 `setTarget`(0046a960) 본체의 실제 연결은 `RawGunProcess` 이식 때 한다. `GunProcessState` 는 웹에서는 `{ boss, friendlyFire, allies, allianceRead? }` 값 객체로 갈음했다.

## 4. 다음 시작점

`RawGunGroundTarget`·`RawGunProcess`(+`MakeGunTargetHooks`), Missile 계열, `RawTempleDraw`/캡처 계열(자산 시험 장치), `RawCarrierArrival`, `RawAssetPostPop`, 사제 배치 계열(선행 `RawCanonPixelShape`·`RawCanonPlacement`), 후보/지형 권한(`RawCanonPlacementPermission`), `RawSquidTransmission`, `RawDudeSpawn`·`RawDudeNotice`→`RawDudeDrop`, `RawManaBoltArrival`(시험 장면 선행), `RawIslandRuntime` 계열, 그 밖의 작은 몸체이다.
