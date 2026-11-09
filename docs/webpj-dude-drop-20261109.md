# webpj Dude 드롭 부모(RawDudeDrop) 이식 — WB04-E208

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절 "Dude 생성" 묶음의 마지막 부분이다. 피해 후처리가 부르는 Dude 드롭 두 부모(00450b30 발자국 안 난수 위치·00450c20 표적 진입점)를 E206 생성·E76 task·E207 진입점 모듈에 연결했다. 이로써 이 묶음(RawDudeSpawn → RawDudeNotice → RawDudeDrop)을 모두 옮겼다.

- cpppj 기준선 `1f00a25a`. 소스·시험·fixture 모두 기준선 뒤 변경 없음. 근거 커밋: 소스·fixture `860030e7`, 시험 `a4b63270`.
- 원본 fixture: `dudedrop-x86.tsv` originals 154행(SHA-256 `7675d8bc…`). originalCD 154행은 범위 밖이다.
- 공유 장치:
  - E206 `raw-dude-spawn.js`, E76 `raw-dude-task.js`, E207 `raw-dude-notice.js`(`residencePoint`)
  - D2 `random.js`(GameRandom)
  - D5c `squid-process.js`·`kernel.js`·`squid-destroy.js`
- 시험 장치: E206 시험의 장면 조립을 `tests/dude-spawn-support.mjs`(cpppj `DudeSpawnSupport.h` 대응)로 분리했다. `dude-spawn.test.mjs` 와 새 `dude-drop.test.mjs` 가 같이 쓴다. E206 시험의 대조 내용은 바꾸지 않았다.

## 1. 옮긴 것

`www/core/raw-dude-drop.js`(DOM 없음, 10.78 전용)

- `RawDudeDrop(pool, types, spawn, task, notice, random)`: 생성/task/진입점이 같은 풀인지 생성 때 확인한다.
- `drop(source, index, random)`
  1. source SID 0 이면 원본 assert(`DudeDropSourceAssertError`)다. source 가 해제(상태 비트 1)면 아무것도 하지 않는다. dead 부모의 정상 PostDestroy 드롭은 계속한다.
  2. 좌표를 정한다.
     - random: 현재 TYPE 발자국(0..256 확인)과 유한 좌표를 확인한다. x 를 먼저, 그다음 y 를 처리한다. 각 축마다 [0.1f, foot−0.1f] 구간에서 상위 16비트 난수(÷65536)로 53비트 중간값을 구해 좌표에서 빼고 float 로 저장한다.
     - 진입점: E207 `residencePoint`(표적 발자국 진입점)를 쓴다.
  3. source 의 현재 소유자로 Dude 를 생성한다. 생성되지 않았거나 새 Dude 가 해제/dead 면 반환한다.
  4. task 를 설정한다. random 은 0x5103·index×0.1f, 진입점은 0x5104·index×0.7f(double 곱 뒤 float 저장)다.
- CD 10.72 분기(SID 0 진단 없이 반환·생성 실패에도 task 호출)는 옮기지 않았다.

## 2. 검증

`tests/dude-drop.test.mjs`(2건)

- 원본 154행: cpppj DropScene 과 같은 장면이다. E206 의 실제 보호 Dude 생성 장면에 다음을 더한다.
  - 부모 89 의 발자국/genus 입력(공유 TYPE 표를 바꾸지 않도록 사본)
  - 실제 Kernel/SquidProcessHost Regular, 공유 GameRandom(seed 입력), RawDudeNotice 진입점, RawDudeTask
  - 준비 SID 5 를 부모 raw 로 바꾸고, 새 Dude 6/Regular 7 의 할당 순서를 쓴다.
  - 출력 19칸을 대조한다: 결과(반환/assert)·새 SID·Regular 조회/생성/부착 호출 순서·최종 Regular·난수 상태·population·슬롯·장부·변경 사각형.
- 웹 계약: 다른 풀의 생성/task 연결 거부와 source 0 assert 다.

최초 실패와 교정: 없다. 모듈과 시험 모두 첫 실행에 통과했다. 비교가 실제로 작동하는지 보려고 다음 변형을 각각 돌렸다.

- 진입점 지연 상수(0.7→0.75)·난수 task 번호(0x5103→0x5102)·난수 구간의 최솟값 더하기(최솟값→최댓값): 원본 대조가 실패했다.
- 새 Dude 해제/dead 반환 검사 생략: 통과했다. fixture 의 새 Dude 가 늘 살아 있어 구별되지 않는 차이다.
- 확인 뒤 모두 되돌렸다.

## 3. 경계와 남은 것

- 실제 호출자인 Damageable PostDestroy(3.1절 "총·피해·삭제 수명" 묶음의 RawDamageablePostDestroy)와 World 의 공유 난수·Dude 생성/진입점 장기 연결은 그 단위와 순서4~5 몫이다(PORT_MAP session 0).

## 4. 다음 시작점

"Dude 생성" 묶음은 끝났다. WB04-E209 는 3.1절에서 고른다. cpppj 전용 fixture 시험이 있고 기준선 뒤 변경이 없는 단위를 먼저 권한다: 비행 프레임 계열 RawBalloonFrame(`BalloonPathTests`)·RawBulfFrame(`BulfPathTests`)·RawWalkerFrame(`WalkerPathTests`)·RawFlyerFrame/RawFlyerTarget, 그리고 RawGunGroundTarget·RawDamageableHit.
