# webpj 투사체 충돌 효과 RawGunImpact (WB04-E363, 2026-11-13, 7-P2 다섯째 항목)

3.1절 7-P2 다섯째 항목 "RawGunImpact 136줄(+12)"을 `core/raw-gun-impact.js` 로 옮겼다. 투사체가 표적이나 배리어에 부딪힐 때의 시각 효과(anim 160)를 만들고 Regular 로 프레임을 진행·삭제하는 몸체이다. 선행인 `RawMissile`(E361)과 `RawAssetRegular`(3-C)가 끝나 있어 이번에 착수했고, 기준선 뒤 변경 `20261109-99 CP`(충돌 애니메이션 장기 공유)를 함께 반영했다(PORT_MAP base `f2f9e837`).

## 1. 규칙

- **구성:** `new RawGunImpact(pool, frames, setter, random, state, hooks)` — 풀은 PATCH, `frames` 는 188칸 `FrameCodeTable`(타입별 A..P 글자 구간), `setter` 는 같은 풀의 `SquidFrame`, `random` 은 공유 `GameRandom`, `state` 는 `GunImpactState`(억제 double·억제 DWORD·anim 타입 160·장기 억제 double 을 빌리는 `{ value }` 상자 `sharedSuppressedTime`), 훅은 `processCount`·`create`·`pop`·`regular`·`destroy`·`notifyDudes` 이다. 풀이 다른 지정기·길이가 188이 아닌 표·160이 아닌 anim 타입은 거부하고, 선택하지 않은 훅은 요구하지 않는다.
- **`visual(x, y, variant, interval)` (0045a750):** 억제(현재 소유자의 double 이 0 이 아니거나 NaN 이거나, 억제 DWORD 가 비영)면 아무것도 하지 않고 반환한다. 음수 0 은 0 과 같아 생성한다. 그 뒤 Kernel 개수가 30001 이상이면 반환하고(개수 조회 훅이 없으면 거부), 글자 구간 조회(글자 번호 ≥ 16 은 `RangeError`) → 생성/Pop/Regular 훅 존재 확인 → 좌표 제한 → anim 160 생성(생성된 슬롯의 raw 가상 표가 anim(0x509758)이 아니거나 TYPE 이 160 이 아니면 거부) → **첫 프레임**(그 글자 구간의 첫 번호, 빈 구간은 −1)을 슬롯의 DWORD(+36)로만 쓴 뒤 → `pop(sid, 제한한 x, 제한한 y, 0)` → `regular(sid, variant, interval)` 순서이다.
- **`ground(x, y)` (00470ac0):** Dude 반응 훅이 없으면 아무것도 하지 않고 거부한다. 그 뒤 좌표 제한 → **주변 Dude 반응(제한한 좌표)** → 공유 난수 `next(100)` 한 번(반응 뒤, 억제 중에도 소비) → 변형 3·간격 `fround(0.02)` 의 `visual` 이다.
- **`handle(sid, event, count, payload)` (0045a6e0):** 현재 raw 가상 표가 anim 이 아니면 `TypeError`. 현재 슬롯 TYPE 의 글자 구간에서 count 를 signed 로 보아 `0 ≤ count < 개수` 면 `setter.set(sid, 첫 번호 + count, 0)` 후 payload 를 돌려주고, 그 밖(음수 포함)은 삭제 훅 `destroy(sid, 0)` 후 −1 이다(삭제 훅이 없으면 거부). `static handleIn(pool, frames, setter, destroy, sid, event, count, payload)` 는 같은 본문으로, Native 장기 소비자가 생성/난수 상태 없이 쓴다(슬롯 TYPE 이 188 이상이면 글자 구간 오류).
- **`handles(sid)` / `handler(fallback)`:** SID 가 비어 있지 않고 풀 안이며 raw 가상 표가 anim 일 때만 참이다(타입 번호로 추측하지 않음). `handler` 는 anim 만 직접 처리하고 다른 부모는 기존 처리기로 넘기며 처리기가 없으면 거부한다.
- **`clamp(x, y)` (0041d0d0):** 하한 1, 상한 255, NaN 은 보존한다(10.78).
- **훅 공장:** `makeGunImpactRuntimeHooks(pool, factory, kernel, host, hooks)` 는 실제 Factory 의 생성, Kernel 크기(개수), Host 의 Regular 부착(flags 0x50, 실패하면 거부)을 연결한 새 묶음을 돌려준다(Factory/Host 풀이나 Kernel 용량이 다르면 거부, 나머지 훅 보존). `makeMissileGunImpactHooks(impact, hooks)` 는 Missile 의 `impact`→`visual`, `hitEffect`→`ground`, `makeAssetGunImpactHooks(impact, hooks)` 는 자산 Damageable 의 `damageGround`→`ground` 로 연결한다(새 묶음).

## 2. 웹 범위와 기준선 뒤 변경

- 10.72(CD)의 BYTE 프레임·Kernel 한도 3001·NaN→255 상한 규칙은 범위 밖이다. 기준선 뒤 `20261109-99 CP` 의 억제 double 소유자 상자(`sharedSuppressedTime`)·정적 `handleIn`·`handler` 를 반영했다. 소유자 상자는 읽기와 쓰기 모두 같은 소유자로 가고 지역 fallback 은 건드리지 않는다.
- Dude 반응·Pop·삭제 훅의 구현과 Native 장기 소비자(`handleIn`) 조립, 앱 수명 연결은 후속 단계 몫이다. 이 모듈은 DOM 을 쓰지 않는다.

## 3. 검사 (`tests/gun-impact.test.mjs` 14건)

- **원본 독립 관측:** `gunimpact-x86.tsv` 의 10.78(originals) 1,137행 — 생산 Visual 72·anim Handle 1,008·좌표 제한 27·anim 생성자 1·Kernel 개수 5(0/2/30000/30001/39999 중 5개)·지면 Ground 24 가 반환 비트·호출 순서(Q 개수/C 생성/P Pop/D 갱신/U 해제/A·B Regular 부착/Qd Dude/D 삭제)·anim 슬롯 50바이트·별도 Host 의 Regular 의미 상태(Form·부모·시각 비트·이벤트·payload 비트·횟수)·공유 난수 상태·좌표 8바이트까지 **첫 실행에서 전부** 일치했다.
- **cpppj 시험 이식(5건):** `GunImpact_PatchNativeAnimationAndProducer`·`_MissingCreationDeleteAndDudesRejected`·`_SharedSuppressionReadsCurrentOwnerWithoutCopy`·`_LegacyAggregateKeepsOwnedSuppression`·`_ActualAssetsFactoryPopRegularAndDeletion`(웹 준비 자료의 실제 anim 88프레임·실제 SHP 로 Factory/Pop/Regular/장부/종속 삭제 사슬이 22프레임을 진행하고 지운다).
- **웹 계약(9건):** 생성자 검사, 좌표 제한(1~2·255~256 사이·±Infinity·−0·NaN), 억제 조합과 Kernel 한도(30000/30001)·경계 누락의 생성 전 거부·글자 번호 16/17, 첫 프레임 쓰기(빈 구간 −1·다른 바이트 보존)·Pop 인자·Regular 인자·잘못된 생성 슬롯 거부, Ground 순서(Dude→난수→visual)·억제 중 난수 소비·Dude 훅 누락, Handle 의 구간 안/밖/음수/표 밖 TYPE(187은 빈 구간·188 이상은 오류)·삭제 훅 누락·정적 본문, handles/handler 분배, 훅 공장 셋.
- 시험이 먼저 실패한 곳은 시험 쪽 두 건이었다(사건 `D:50:` 호출 여부를 가정한 단정, 구간 안 호출을 거부 단정으로 잘못 쓴 줄). 모듈은 관측 대조에서 바뀌지 않았다.

## 4. 변형 확인과 남은 경계

- 연산자·상수 변형 87 + 의미 변형 65 = 152회를 적용했다. 1차에서 15개, 2차에서 13개가 남았고 그중 5개는 **시험의 공백**(1과 2·255와 256 사이 좌표, 글자 번호 16의 오류 문구, TYPE 경계 188, Regular 부착 flags, Dude 반응과 난수 소비의 순서, 슬롯 0의 판정, Missile impact 의 간격 인자)이라 보강해 잡았다.
- 남은 동등 변형 10개(이유 7가지): 좌표 제한의 `x < 1`/`y < 1` 을 `<=` 로(경계값 1 은 그대로여서 같은 값)·`> 255` 를 `>=` 로(255 는 그대로)·글자 구간 조회의 `type >= frames.length`(anim 타입이 160으로 고정되어 도달하지 않음)·난수 한도 `next(100)` 을 101 로(상태 전진은 한도와 무관, 결과를 쓰지 않음)·Ground 가 이미 제한한 좌표 대신 원래 좌표를 `visual` 에 넘기는 것(`visual` 이 다시 제한하며 제한은 멱등)·`count | 0` 을 `count >>> 0` 으로(구간 개수가 2^31 을 넘지 않아 음수와 큰 양수 모두 구간 밖).
- 다음 7-P2 항목은 **RawDiscRegular**(70줄 +29, 썬 디스크 Regular)이다.
