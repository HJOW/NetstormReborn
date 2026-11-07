# webpj Bomb 전처리·주문 효과 후처리 접두(RawBombPreDestroy·RawBombPostPop) 이식 — WB04-E51

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 쉰한 번째 부분이다. cpppj 기준선 `1f00a25a`의 `o/RawBombPreDestroy`(10.78; 원본 41e960 Point Blast 계열 가상 전처리)와 `o/RawBombPostPop`(원본 0041f870/0041f920/0041fab0 그라비테이션·힐·미티어 최초 Regular 예약 접두)를 옮겼다. 후자는 [E41](webpj-squid-postpop-20261108.md)의 `SquidPostPop.setBody('bomb', …)`에 연결되는 접두 본체다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다(Node 단위 검사만). 기준선 뒤 cpppj 변경은 없다.

## 1. 옮긴 것

`www/core/raw-bomb-pre-destroy.js` — `RawBombPreDestroy`

- `new RawBombPreDestroy(pool, {explosion, sound, base})`: 10.78 풀만 받는다. 훅은 도달한 지점에서만 요구한다(없으면 `TypeError`).
  - `handles(sid)`: 현재 슬롯의 첫 DWORD가 502370(41e960 몸체의 가상 표)일 때만 참. 다른 Bomb 파생이나 입력 TYPE으로 표를 넓히지 않는다.
  - `preDestroy(sid, flags)`: 표가 다르면 거부. 상태 비트 `9`(extra)가 없고 `flags & 0x100000`일 때만 `explosion(sid)`(4605d0) → **효과 뒤 현재 좌표를 다시 읽어** `sound(x, y)`(explosion.wav, 반복 0). 이어 항상 `base(sid, flags)`(4b0950, 비가상 공통 전처리).

`www/core/raw-bomb-post-pop.js` — `RawBombPostPop`

- `new RawBombPostPop(pool, random, {hasRegular, reserveRegular, addRegular})`: 10.78 풀과 공유 `GameRandom`. `pool` getter는 `SquidPostPop.setBody`의 풀 일치 확인용이다.
  - `handles(parent)`: 현재 가상 표가 502c38(그라비테이션)/502cf0(힐)/502db0(미티어)일 때만 참(표시 TYPE과 무관).
  - `prefix(parent, flags)`: 호출 시점의 표를 고정한다(조회 중 표가 바뀌어도 이번 호출은 처음 고른 몸체). `flags & 1` → `hasRegular(parent, 1)`로 중복 예약 조회 → `reserveRegular()` 할당 성공이면 지연을 정해 `addRegular(parent, 1, delay)`. 그라비테이션 지연은 float 0.05. 힐·미티어는 공유 난수 한 번(`next(65536)`)으로 `rand/65536 × |0.05 − low| + low`(low: 힐 0, 미티어 0.01), 힐은 double 0.05를 더한 뒤 한 번 float로 저장한다. 중복이거나 할당 실패면 난수를 소비하지 않고, 뒤 장치(`addRegular`) 누락으로 던져도 앞선 난수 소비는 남는다.

## 2. 검증 (`tests/bomb-pre-post-pop.test.mjs`, 4건)

독립 원본 관측 2종(10.78 단일 관측):

- `bomb-predestroy-x86.tsv` 96행: 상태 비트·폭발 플래그 관문과 폭발/소리 도중 좌표·상태·가상 표를 바꾸는 mutation 1~3에서 효과 문자열(`E`/`S`/`B`)과 50바이트 슬롯.
- `bomb-postpop-x86.tsv` 432행: 세 가상 표 × flags × 조회 × 할당 × 난수 시드의 호출열(`find`/`alloc`/`regular`/`base`)과 최종 난수 상태. 실제 `SquidPostPop.postPopVirtual`(억제된 공통 장부)을 거치므로 공통 깊이가 한 번만 줄고 슬롯이 바뀌지 않는 것도 확인한다.

계약: 판본 불일치, 다른 가상 표 거부, 폭발 플래그 없을 때 폭발/소리 훅 불필요, 선택 장치 지연 요구(난수 소비 순서 포함), 조회 중 표가 바뀌어도 고른 몸체 유지(payload `0x3d4ccccd`), 미지원 표 거부.

최초 실패와 교정: 모듈은 528행 전부를 첫 실행에 통과했다.

## 3. 경계

- 실제 폭발(4605d0)·소리·공통 전처리(4b0950)와 Regular 조회/예약/등록(004afb40/new 0x28/00496e80)은 월드 조립에서 공급한다. `RawBombPreDestroy`는 Point Blast 외 Bomb 파생(공통 전처리를 가진 다른 VT)으로 분배하지 않는다.
- cpppj `BombPostPop_Reserved187UsesSameCarrierPostPopAddress`(포획 `RawCarrierSteal`의 두 VT 분배)는 `RawCarrierSteal` 이식 단위에서 대조한다. `LightningScreenTests.cpp`의 번개 화면 부분은 이 단위 범위 밖(전처리 96행만 사용).

## 4. 다음 시작점

Bomb 나머지(`RawBombStatus`·`RawBombEffectTimer`·`RawBombTimerPacket`·`RawBombRegular`·`RawBombBlast`·`RawBombTargets`·`RawBombEffect`), Player 나머지, 종류별 PostPop 본체, `SquidDisplay`/`SquidFrame`이다.
