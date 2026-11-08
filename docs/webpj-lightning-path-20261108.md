# webpj 빔 경로(RawLightningPath) 이식 — WB04-E117

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백열일곱 번째 부분이다. Lightning.cpp 의 공유 빔 경로 생성자(00471b20/00471ca0)·시작 입자(004720f0)·진행(00472180)과 재귀 가지 생성(00471fc0)을 cpppj 기준선 `1f00a25a` 의 `o/RawLightningPath` 에서 옮겼다. 선행은 `SidPool`·`GameRandom` 뿐이다. 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이다.

## 1. 옮긴 것

`www/core/raw-lightning-path.js` — `LightningParticle`, `LightningParticles(capacity, owner)`, `LightningPathFields`, `RawLightningPath(pool, random, state, center)`

- `LightningPathFields`: 원본 64바이트 경로 이미지. 생성자가 쓰지 않는 +24..+34 와 미사용 바이트도 원문처럼 보존한다(+34/+3c 만 호스트 토큰). 오프셋(16진 아님, 십진): 시작점 0/4, 끝점 8/12, 차 벡터 16/20, 거리 24, 각 28, 굵기 32, step 상한 36, step 40, 진행 44, 흔들림 각 48, 마지막 입자 토큰 52, 완료 56, 소유자 60.
- `LightningParticles`: 24바이트 입자(좌표 x,y·굵기·흔들림 가중치·10배 시간 수명·이전 입자 토큰)의 고정 용량(정식 빔은 120개)과 Append 꼬리 `total`·소유 빔 토큰 `owner`. 경로 생성은 죽은 입자를 재사용하지 않는다.
- `initialize(fields, source, target, owner)`(00471b20): 양끝 중심에 픽셀 배율(scaleX/scaleY)을 곱해 snapshot 한다. source 가 반더면 y−76, 키 큰 유닛이면 y−48, 맨 오브 워면 y−48·x−5 로 보정하고, target 은 키 큰/맨 오브 워면 y−48, 그 밖은 y−16 이다. 이어 차 벡터·거리(√)·각(`atan2(−dy, dx)`)·굵기 2·완료 0·소유자를 쓴다. 경로는 이후 따라갈 SID 를 저장하지 않는다. `initializePoints(fields, from, to, owner)`(00471ca0)는 이미 정수화된 픽셀 양끝을 직접 저장하고 타입/중심/SID/배율 보정을 읽지 않는다.
- `seed`(004720f0): 흔들리지 않는 시작점 입자를 추가하고 step 0.15·진행 0·기본 각도를 쓴다.
- `step(fields, particles, progress, lifeOffset)`(00472180): 완료 표시가 있으면 아무것도 하지 않는다. 진행(≤1)까지 반복하며 step 난수를 두 번 뽑아 재귀 가지를 만들고(주점은 이전 주점에, 가지는 방금 만든 점에 연결; 굵기가 임계 0.12 를 넘고 난수 < 85 이면 길이·굵기를 반으로 줄여 재귀), 진행이 1 이 되면 끝점 입자를 한 번 추가하고 완료 표시를 쓴다. 모든 float 는 원본 저장 경계(fstp)마다 단정도로 저장하고, 중간 계산은 넓은(double) 값을 쓴다. 손상된 꼬리/다른 소유 빔은 상태를 바꾸기 전에 거부하고, 용량 초과(원본 assert)는 예외로 보고한다.

## 2. 검증

- `tests/lightning-path.test.mjs`(3건): `lightning-path-x86.tsv` 216행 전부(10.78 단일 판본).
  1. 종류 0(생성자 72행) — source/target 의 다른 높이 보정·배율·퇴화 점·미사용 바이트의 64바이트 경로 이미지·꼬리·난수 상태와 120입자 전체(24바이트 × 120).
  2. 종류 1(생성+Seed+Step 144행) — 재귀 가지·난수 두 번 소비·진행/후퇴/완료 후 재호출의 모든 중간 상태와 120입자 전체.
  3. 계약: 판본/배율/중심 조회 오류, 소유자 0·범위 밖 SID, 소유자 불일치/용량 0·용량 초과 입자, 직접 좌표 생성자의 필드, 비유한 진행 거부.

최초 실패와 교정: 새 모듈은 216행 fixture 를 첫 실행에 통과했다(`Math.sin/cos/atan2` 가 단정도 저장 뒤 원본과 같은 비트를 낸다).

## 3. 경계와 남은 것

- 중심 조회(004adea0)는 호출자가 공급하는 함수이다. 번개 시각 프로세스(`RawLightningFrame`)·수명(`RawLightningLifetime`: `CreateLightning` 이름 wrapper 24행 포함)·화면 빔(`RawLightningScreen`)은 별도 단위이다.

## 4. 다음 시작점

번개 수명/프레임/화면(`RawLightningLifetime`·`RawLightningFrame`), `RawArcherTypeData`, `RawGunProcess`(+`MakeGunTargetHooks`), `RawLocalGeyser`, `RawGunImpact`(`Clamp`는 `raw-meltdown.js` 비공개 함수와 같은 식), Missile 계열(`RawMissile`), `RawPathProcess`(커널/Form 장면 선행)·`RawPathStart`·`RawPathData`·`RawPathCleanup`, `RawBalloonFrame`, `RawPacketDispatch`, 사제 배치 계열, `RawIslandRuntime` 계열, 분산 SP 정책(`ReadPlayerSpAmount`/`SetPlayerSpAmount`), 그 밖의 작은 몸체이다.
