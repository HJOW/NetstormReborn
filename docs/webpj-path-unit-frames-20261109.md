# webpj 이동 유닛 경로 프레임 3종(RawBalloonFrame·RawBulfFrame·RawWalkerFrame) 이식 — WB04-E209

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절 "비행·이동 유닛" 묶음의 첫 부분이다. 경로 프로세스(E98 `RawPathRuntime`)의 파생 종류 세 개가 매 프레임 부르는 실제 프레임 몸체를 옮겼다.

| cpppj 파일 | 원본 함수 | 대상 유닛 | 웹 모듈 |
| --- | --- | --- | --- |
| `o/RawBalloonFrame.cpp/.h` | 0048ad60 (Balloon42) | 벌룬 | `www/core/raw-balloon-frame.js` |
| `o/RawBulfFrame.cpp/.h` | 0048b760·회전 0048ba30 (Bulf41) | 벌프 | `www/core/raw-bulf-frame.js` |
| `o/RawWalkerFrame.cpp/.h` | 0048b080 (Walker38) | 세일 스케이터(윈드)·크리스탈 크랩(레인) | `www/core/raw-walker-frame.js` |

- cpppj 기준선 `1f00a25a`. 세 파일의 소스·시험·fixture 모두 기준선 뒤 변경이 없다(`git diff --numstat 1f00a25a HEAD` 0줄).
- 근거 커밋
  - Balloon: 소스·fixture `6cbb7fdb`, 시험 `0e66d91b`
  - Bulf: 소스·시험·fixture `fc85f3c9`
  - Walker: 소스·시험·fixture `dc95d35e`
- 원본 fixture(모두 10.78 단일 판본, 두 x87 정밀도 열이 같음)
  - `balloon-path-x86.tsv` 688행(SHA-256 `be32994968c78fb4…`)
  - `bulf-path-x86.tsv` 314행(SHA-256 `6f9480d21e45e3fc…`)
  - `walker-path-x86.tsv` 348행(SHA-256 `bf133ee926e5bb99…`)
- 공유 장치: E98 `raw-path-runtime.js`(생성자 종류·`initializeFlight`·`physicalTime`·`restoreStep`·`nextFrame`·`step`), E62 `raw-path-animation.js`(`pathDirection`), WB03-X `assets/terrain-tiles.js`(`FrameCodeTable`), WB04-A `sid-pool.js`. 새 공유 장치는 만들지 않았다.

## 1. 옮긴 것

세 모듈 모두 DOM 을 쓰지 않고 10.78 전용이다. 원본 `std::shared_ptr<bool>` 자기 삭제 감시자는 `{ value }` 상자(외부 효과가 자신을 삭제하면 false)로 옮겼다. C++ 출력 참조(`std::uint32_t& windCounter`)도 `{ value }` 상자다. 현재 부모 좌표·TYPE·고도는 C++ 의 살아 있는 `Slot()` 대신 필요한 지점마다 `pool.slot()` 사본을 새로 읽고, 쓰기는 `pool.allocated()` 로 한다.

### 1.1 `runBalloonFrame(pool, parent, runtime, now, balloonType, hooks, watch)`

hooks = `halt·step·arrive·unpop·pop·flame`.

1. 완료이거나 halt 면 반환한다.
2. Step 을 부르고 watch 를 확인한다.
3. 남은 시각 `float(time[current] − now)` 를 한 번 저장한다. 구간 길이는 `time[current] − time[current−1]` 이 0.001f 미만이면 0.001f, 아니면 float 이다(NaN 은 원문 비교대로 float 변환).
4. 현재 단계 1(이륙): 고도 = max(Step 직후 BYTE, ftol(44 − 남은×44/길이)). 그 값이 0 이고 현재 TYPE 이 벌룬이면 현재 좌표로 화염을 부르고 watch 를 확인한다.
5. 마지막 구간(착륙): 고도 = min(Step 직후 BYTE, ftol(남은×44/길이)).
6. Step 이 완료면 도착만 부른다.
7. 화염 뒤의 현재 부모 좌표와 Step 좌표가 다를 때만 Unpop(0) → watch → 고도 BYTE 쓰기(고도 미기록 표식 −1 이면 생략) → Pop(0x40).

ftol 은 원본 `_ftol` 하위 DWORD(비유한·±2^63 밖 0)다.

### 1.2 `bulfTurningFrame(frames, angle, counterClockwise)`·`runBulfFrame(pool, parent, runtime, frames, shapes, hooks, watch)`

- 회전 프레임: E(0)/F(1) 구간에서 `float(angle × float(1/360) × (구간 수−1))` 저장 → +0.5 → `_ftol` 하위 DWORD → 구간 첫 번호를 더한 signed 값이다. 방향 2 이상은 거부한다.
- 프레임 몸체(hooks = `immobile·halt·step·arrive·unpop·pop·randomInt·sound`)
  1. 판본·필수 효과(randomInt·sound 제외)를 확인한다. immobile 이면 반환한다(완료 검사보다 먼저).
  2. Step 전 각도를 보관하고, 완료/halt/삭제면 반환한다. Step 뒤 watch 를 확인하고, Step 이 완료면 도착만 부른다.
  3. 현재 TYPE·SHP 프레임 범위를 확인한다(밖이면 오류).
  4. 각도가 바뀌었으면 회전 프레임이다. 각도가 작아졌거나 180도 넘게 커지면 F 구간이다.
  5. 각도가 같고 90도 배수면, 현재 프레임의 walkStep 발자국 거리가 이동 거리 이하일 때 보행 A~D(경로 방향/2) 다음 프레임이다. 현재 프레임이 E/F 면 원본 중간 프레임 3/9/17/30 에서 시작한다.
  6. 다음 프레임이 같으면 위치도 갱신하지 않는다.
  7. Unpop(0) → watch → 프레임 DWORD(+36) 쓰기 → Pop(0x40) → watch.
  8. `(프레임+1) % 4 == 0` 이면 `randomInt(1000, 6000)/1000` 번 `bulfWalkN.wav` 발소리를 현재 부모 좌표로 낸다.

### 1.3 `runWalkerFrame(pool, parent, runtime, now, frames, windType, rainType, windCounter, hooks, watch)`

hooks = `immobile·halt·step·groundStop·arrive·unpop·pop`(모두 필수).

1. immobile/삭제/완료/halt/삭제면 반환한다. Step 뒤 watch 를 확인하고, Step 이 완료면 도착만 부른다.
2. groundStop(Step 좌표)이 참이거나 그 뒤 삭제되면 반환한다.
3. TYPE 별 프레임
   - 윈드: 공유 counter = (counter+1)&3. 각도 90/180/270 이면 +12/+8/+4, 0 이 아닌 그 밖의 각도는 회전 프레임 `ftol(float((1 − angle×float(1/360)) × 19) + 0.5) + 16` 이다.
   - 레인: 예약 시각 ≤ now 일 때만 진행한다. 0/180 도는 B, 90/270 도는 A 구간에서 다음 프레임(구간 밖이면 첫 번호)을 고르고 예약을 now+0.07999999821186066 으로 미룬다. 비직각이면 이동도 하지 않고 반환한다.
   - 그 밖의 TYPE: 원본 진단 553 오류.
4. 좌표와 프레임이 모두 같으면 반환한다. 아니면 Unpop(0) → watch → 프레임 DWORD 쓰기 → Pop(0x40).

## 2. 검증

| 시험 파일 | 검사 | 내용 |
| --- | --- | --- |
| `tests/balloon-path.test.mjs` | 3 | 원본 688행(생성자 할당 성공/실패·비행 Init·프레임): 효과 순서(H/S/F/A/U/P)·경로 100080바이트 Adler-32·부모 50바이트·관찰 flags/stamp. 계약: Step/화염/Unpop 뒤 자기 삭제 경계, 완료면 Step 없음, 착륙 구간 남은 시각의 float 저장 지점 |
| `tests/bulf-path.test.mjs` | 2 | 원본 314행(생성자·프레임·회전 helper): 효과 순서(I/H/S/A/U/P/R/W)·경로 Adler·부모 50바이트·helper 반환값. 계약: 10.72 풀·필수 효과 누락·회전 방향 2 거부, Unpop/Pop 뒤 자기 삭제면 발소리 없음 |
| `tests/walker-path.test.mjs` | 3 | 원본 348행(생성자·프레임, 예약 시각 0·10·10 인접 double·NaN·±무한): 효과 순서(I/H/S/G/A/U/P)·경로 Adler·부모 50바이트·공유 counter. 계약: cpppj `WalkerPath_DeletionBoundaries` 4단계, 10.72 풀·필수 효과 누락 거부·진단 553 |

- 각 시험은 cpppj `BalloonPath_IndependentNativeBodies`·`BulfPath_IndependentNativeBodies`·`WalkerPath_IndependentNativeBodies` 의 장면(슬롯 50 raw, 경로 꼭짓점·방향·시각, 합성 프레임 표, 외부 효과 기록)을 그대로 옮겼다. 원본 fixture 는 복사하지 않고 `cpppj/tests/fixtures` 를 직접 읽는다.
- cpppj `BalloonPathTests` 의 나머지 두 TEST_CASE(그림자 몸체 489행·저장 가상 60)는 E94 `balloon-body.test.mjs` 가 이미 옮겼다.

최초 실패와 교정: 없다. 세 모듈과 원본 대조 1350행 모두 첫 실행에 통과했다. 비교가 실제로 작동하는지 보려고 사본을 다음과 같이 바꿔 돌렸다. 확인 뒤 모두 원본으로 되돌렸다(cmp 일치).

- 원본 대조가 실패한 변형
  - Balloon 이륙 고도 max→min
  - Bulf E/F 복귀 중간 프레임 30→31, 반시계 판정 `> 180` → `>= 180`
  - Walker 레인 주기 0.07999999821186066→0.08, 회전 프레임 시작 16→15
- 원본 대조가 통과한 변형: Balloon 남은 시각의 float 저장 생략. fixture 의 경로 시각(0·1·3·4)과 now 값으로는 구분되지 않는다. 그래서 착륙 구간 남은 시각 0.25−2^-40(float 저장 뒤 0.25 → 고도 11, 저장하지 않으면 10)을 쓰는 계약 검사를 추가했다. 그 변형이 이 검사에서 실패하는 것을 확인했다.

## 3. 경계와 남은 것

- 실제 호출자 연결은 하지 않았다. 각 경로 프로세스의 프레임 가상 슬롯(벌룬 42·벌프 41·워커 38)에서 이 함수를 부르는 RawPathProcess(3.1절 "경로" 묶음)와, Step(0048a330)·Halt(00489ff0)·도착(0048b340)·Fall 사건 조회(004287a0)·지면 정지(0048a7c0)·공간 Unpop/Pop·소리(004a9d70)·공유 난수(004558f0)의 World 연결은 그 단위와 순서4~5 몫이다(PORT_MAP session 0).
- 윈드 공유 counter 의 장기 저장 위치(World 장기 전역)는 아직 없다. 호출자가 `{ value }` 상자를 넘긴다.
- CD 10.72 분기는 옮기지 않았다(cpppj 도 10.78 전용).

## 4. 다음 시작점

WB04-E210 은 3.1절에서 고른다. 같은 "비행·이동 유닛" 묶음의 RawFlyerFrame/RawFlyerTarget(`FlyerFrameTests`·`FlyerTargetTests`), 총/피해 묶음의 RawGunGroundTarget(`GunGroundTargetTests`)·RawDamageableHit(`DamageableHitTests`)가 cpppj 전용 fixture 시험이 있고 기준선 뒤 변경이 없다.
