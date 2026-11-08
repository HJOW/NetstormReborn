# webpj 캐논 공통 발사(RawCannonFire)·아이스 캐논 발사(RawIceCannonFire) 이식 — WB04-E114

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백열네 번째 부분이다. 썬 캐논·썬더 캐논·반더 타워가 같은 467330 프레임 몸체로 실행하는 공통 발사(Fire)와 아이스 캐논의 80바이트 파생(004685e0/00468660)을 cpppj 기준선 `1f00a25a` 의 `o/RawCannonFire`·`o/RawIceCannonFire` 에서 옮겼다. 선행은 직전 단위의 [RawGunAnimation](webpj-gun-animation-20261108.md)(`GunAnimationFields`)·`SidPool`·`SquidProcess`·`FrameCodeTable`·`GameRandom` 이다. 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이다.

## 1. 옮긴 것

`www/core/raw-cannon-fire.js` — `CannonFireKind`, `CannonFireState`, `RawCannonFire(pool, types, frames, state, hooks, beam)`, `CannonFireProcess`, `readField`/`writeField`

- `initialize(fields, gunForm, target)`: 파생(썬 0x50b318/썬더 0x50b370/반더 0x50b170)의 가상 표·내장 표·dirty 0xff·gun/target WORD·현재 시각·원위치 방향 0 을 쓰고 Form/Parent 는 0 으로 둔다.
- `step(fields, frame)`(004684b0/00468520/00468130):
  - 반더 `frame | 0xc000`(발사·완료 동시).
  - 썬 `frame < 24` 면 `+24 | 0x4000`, 아니면 `-24 | 0x8000`.
  - 썬더는 현재 부모 프레임 코드의 글자 구간을 따라 진행한다. 구간 끝을 넘으면 `처음 | 0x8000`, 끝이면 `| 0x4000`이고, `끝 - growlOffset`/`끝 - roarOffset` 칸에서 `thunderCannonGrowl.wav`/`thunderCannonRoar.wav` 를 현재 부모 위치로 낸다(두 소리가 겹칠 수 있음).
- `launch(fields)`(00466f50): live HP 표적일 때 일반 투사체(Missile 인자 12개, 포구 `-10000,-10000`)를 만들거나, 초기 부모 TYPE 이 반더(`vanderType`)이면 즉시 빔을 쓴다(create → Cloud Floater 만 `RandomFloat(0,1000) < avoidanceCutoff` 회피 → refresh 또는 hit, 반환은 표적이 아직 살아 있는지). 초기 부모/표적 슬롯 주소는 snapshot 이므로 훅 뒤 바뀐 바이트를 그대로 다시 읽는다(`pool.physical` live 뷰).
- `frame(fields)` / `frameWithStep(fields, step)`(00467330): 표적 추적(live 면 위치/중심 재기록) → 시각이 도달하면 가상 step → 부모 프레임 변경(`unpop 0` → DWORD 쓰기 → `repop 0x40`) → 발사 요청 → 완료(통지 info 6/value 7 + 삭제 0x10) 또는 다음 시각(`지연 × (발사 비트 && 부모 TYPE = scaledType 이면 배율)`). `FrameWithStep` 은 아이스 파생이 step 만 바꿔 쓰도록 분리했다.
- `CannonFireProcess(SquidProcess)`: `runFrame` → `frame`, `onAttached` → Form/Parent 기록, `fields`.
- `CannonFireState`: 현재 시각·종류·TYPE·속도·지연·배율·소리 차이·반더/플로터 TYPE·회피 기준을 훅 뒤에도 다시 읽는 가변 상태이다(`currentType = { value }` 이면 매번 빌린 DWORD).

`www/core/raw-ice-cannon-fire.js` — `IceCannonFireFields`(공통 64 + 전용 16), `RawIceCannonFire(pool, types, frames, state, common, iceHooks, beam)`, `IceCannonFireProcess`

- `initialize`: 공통 초기화 뒤 두 가상 표(0x50b3f0/0x50b3ec)·active 1·전환 시각 `현재+4.0` 만 쓴다(+44 padding 과 공통 dirty 는 유지).
- `step`(00468660): 전환 시각이 지났으면(NaN 이면 건너뜀) active 가 켜져 있을 때 active 를 끄고 냉각 표시(`cooling` 훅)를 만든 뒤 `현재(훅 뒤)+6.0` 으로 미루며 입력 프레임을 그대로 돌려주고, 꺼져 있으면 켜고 `현재+4.0`. 그다음 현재 부모(현재 TYPE)의 글자 구간에서 구간 끝을 넘으면 처음 칸, 아니면 문맥(`context`)을 만들고 `mayTarget` 이 거짓이면 `| 0x8000`, 참이면 `| 0x4000`(다음 칸이 끝이고 active 일 때만).
- `frame`: 같은 467330 공통 추적/공간/발사/완료를 위 step 으로 진행한다. `launch`: 공통 발사.
- `IceCannonFireProcess(SquidProcess)`: `runFrame`/`onAttached`/`fields`.

## 2. 검증

- `tests/cannon-fire.test.mjs`(4건):
  1. `cannon-fire-x86.tsv` 5534행 중 썬(프로파일 0)의 Frame/Initialize/Step/Launch.
  2. 같은 파일의 썬더(프로파일 1) 전부 — 반환, 외부 효과 순서(`C` 중심/`U`·`P` 공간/`T` 표적점/`A`·`J` Missile/`S` 소리/`M` 통지/`K` 삭제/`B` 부착), 0x40바이트 필드 이미지, 네 슬롯 200바이트.
  3. `vander-fire-x86.tsv` 의 반더 발사 393행(종류 0~3 — 즉시 빔·회피 난수·난수 상태·현재 시각까지). 이름 wrapper 24행(종류 4 이상)은 `RawLightningLifetime` 이식 때 쓴다.
  4. 계약: Missile 경계 누락을 성공으로 우회하지 않음(효과 없이 거부), 판본/표 크기/현재 TYPE 범위 오류, 빌린 현재 TYPE, 프로세스 보조.
- `tests/ice-cannon-fire.test.mjs`(2건): `ice-cannon-fire-x86.tsv` 3585행 전부(Frame/Initialize/Step/Launch) — 반환, 효과 순서(`E` 냉각/`X` 문맥/`A` 표적 판정 포함), 80바이트 이미지, 네 슬롯. 계약: 냉각·문맥·표적 판정 경계 누락 거부, 초기화 padding, 프로세스 보조.
- 이 fixture 들은 모두 10.78(원본) 단일 판본이다.

최초 실패와 교정: 새 두 모듈은 모든 fixture 행을 첫 실행에 통과했다.

## 3. 경계와 남은 것

- 중심·공간·소리·Missile 생성·즉시 빔 생성/refresh/hit·냉각 표시·문맥·표적 판정·완료 통지·삭제는 훅 경계이다. Host 연결(`MakeCannonFireHostHooks`/`CreateCannonFire`, `MakeIceCannonFireHostHooks`/`CreateIceCannonFire`)은 `RawGunProcess` 이식 뒤에 한다.
- 이름 wrapper(`CreateLightning`) 24행과 번개 모양/수명은 `RawLightningLifetime`·`RawLightningPath` 단위에서 이식한다.

## 4. 다음 시작점

`RawGunProcess`(+`MakeGunTargetHooks`), `RawArcherAnimation`, `RawLocalGeyser`, `RawGunImpact`(`Clamp`는 `raw-meltdown.js` 비공개 함수와 같은 식), Missile 계열(`RawMissile`), `RawPathProcess`(커널/Form 장면 선행)·`RawPathStart`·`RawPathData`·`RawPathCleanup`, `RawBalloonFrame`, `RawPacketDispatch`, 사제 배치 계열, `RawIslandRuntime` 계열, 분산 SP 정책(`ReadPlayerSpAmount`/`SetPlayerSpAmount`), 번개 계열, 그 밖의 작은 몸체이다.
