# webpj 아이스 반사(RawIceRicochet)·Missile 필드 이미지 이식 — WB04-E124

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백스물네 번째 부분이다. 아이스 Missile 의 반사점(00482080)과 후보 선택/후속 생성 요청(00482190)을 cpppj 기준선 `1f00a25a` 의 `o/RawIceRicochet` 에서 옮기고, 이를 위해 `o/RawMissile` 의 128바이트 필드 이미지(`MissileFields`)만 먼저 분리했다(`RawMissile` 본체의 생성·이동·피해는 아직 이식하지 않음). 선행은 [RawArcherAnimation](webpj-archer-animation-20261108.md)(`directionAngle`)·`RawSquidFinder`·`SquidHash`·`SquidReward`·`GameRandom` 이다. 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이다.

## 1. 옮긴 것

`www/core/raw-missile-fields.js` — `MissileFields`, `MISSILE_FIELD_SIZE`

- 원본 128바이트 Missile 객체 이미지(`bytes`)와 SID 읽기: Form(+4)·표시 자산 부모(+8)·발사 자산(+14 16진)·피해 표적(+18 16진)의 낮은 16비트. 그 밖의 필드는 소비자가 `bytes` 를 직접 읽는다.

`www/core/raw-ice-ricochet.js` — `RawIceRicochet(pool, types, missileTypes, random, hooks)`, `makeIceRicochetWorldHooks`

- `point(fields, output)`(00482080): 표시 자산 부모의 현재 위치와 원점(필드 +38/+3c 16진)이 같으면 `output` 과 난수를 건드리지 않고 false 이다. 다르면 원점→현재 방향각(`RawArcherAnimation.directionAngle`)을 구하고, 난수 두 번(100 중 50 이상이면 부호 −, 65536 분수 × 120 + 30 도 편차)으로 각을 정규화(음수 보정은 float 역수 `1/360f`, 360 초과는 fmod)해 `cos/sin(각) × 사거리 + 현재 위치` 를 [1, 255] 로 clamp(`std::clamp` 와 같은 비교 순서, NaN 통과)한 두 float 를 쓰고 true 이다.
- `generate(fields)`(00482190): 난수 99 중 19 이하이면 끝낸다. 아니면 반사점을 구하고(변위 0 이면 호출 전 지역 8바이트를 `initialPointBytes` 훅으로 받아 점으로 사용; 훅이 없으면 거부), 보드 유효 범위(0 < x,y < 256, NaN 거부)가 아니면 끝낸다. 점의 거의 올림(+0.99999f) 칸 3×3 사각형으로 일반 finder(`begin`/`next`, flags 0)를 돌려 HP 보유(`hasHitPoints` 훅)·서버 예약 번호(5..14999) 제외·raw+40 비트 1 제외·비행(0x120000) 제외·현재 표적 제외 후보 중 점에서 가장 가까운 것(동률은 최초)을 고르고, 표적이 없어도(target 0) 후속 Missile 생성 요청(속도 = 필드 +30, 표시 TYPE = 표시 자산 TYPE, 효과 0, 반사 1, 포구 = 표시 자산 위치, 코너 −10000,−10000)을 `createProjectile` 훅에 넘긴다. 표시 슬롯은 처음 잡은 주소를 live 뷰로 두고 순회 중 바뀐 바이트를 그대로 읽으며, 표적·속도·부모는 매번 필드에서 다시 읽는다.
- `makeIceRicochetWorldHooks(pool, hash, types, reward, hooks)`: 같은 풀의 실제 일반 finder(`RawSquidFinder`)와 `SquidReward.hasHitPoints` 를 `begin`/`next`/`hasHitPoints` 훅에 묶는다(풀이 다르면 거부, 후속 생성 훅은 그대로 보존).

## 2. 검증

- `tests/ice-ricochet.test.mjs`(4건):
  1. `ice-ricochet-x86.tsv` 1182행 전부(10.78 단일 판본) — 종류(반사점 / 후보 선택·후속 요청)·난수 시드·원점·현재 위치·사거리·현재 표적·속도·후보 노드·변이(표적/속도/부모/표시 슬롯 변경)로 외부 효과 순서(`B` finder 시작/`N` 다음/`J` 후속 Missile 인자 12개), 반사점 8바이트, 128바이트 필드 이미지, 공유 난수 최종 상태가 같다.
  2. `ice-ricochet-scratch-x86.tsv` 213행 전부 — 변위 0/비영 변위·NaN/경계·후보/현재 필드와 호출 전 지역 점 8바이트 입력으로 같은 출력과 입력 호출 횟수(변위 0 의 확률 관문 통과 때만 1회)가 같다.
  3. 계약: 후속 Missile 수명 누락 거부(효과 없음), 지역 점 입력 누락은 확률 관문 뒤 변위 0 에서만 거부하고 앞선 난수/필드를 보존(난수 시드 20 은 상태 0x0014003f, 시드 1 은 정상), 판본/표 크기 거부.
  4. 실제 해시 체인/HP(`SquidReward`)가 원문의 같은 후보(15000번)로 같은 후속 인자·필드·난수를 만들고 SP 저장소를 초기화하지 않으며, 다른 풀의 보상은 거부.

최초 실패와 교정: 새 두 모듈은 fixture 1395행을 첫 실행에 통과했다.

## 3. 경계와 남은 것

- 후속 반사 Missile 의 생성·Pop·Kernel 부착(`createProjectile`)은 `RawMissile` 이식 때 연결한다. 변위 0 의 지역 점 8바이트는 원문 스택의 호출자 지역점이므로 임의 성공 좌표로 대체하지 않고 훅 입력으로만 받는다.

## 4. 다음 시작점

`RawMissile`(생성·이동·피해·반사 연결), `RawDudeSpawn`·`RawDudeNotice`→`RawDudeDrop`(실제 자산 필요), `RawWindAviaryProcess`·`RawWindFlyer`, 비행체(`RawFlyerProcess`·`RawFlyerFrame`·`RawFlyerTarget`), 번개 수명/프레임/화면, `RawArcherTypeData`, `RawGunProcess`, `RawLocalGeyser`, `RawGunImpact`, `RawPathProcess`(커널/Form 장면 선행)·`RawPathStart`·`RawPathData`·`RawPathCleanup`, `RawBalloonFrame`, `RawPacketDispatch`, 사제 배치 계열, `RawIslandRuntime` 계열, 분산 SP 정책, 그 밖의 작은 몸체이다.
