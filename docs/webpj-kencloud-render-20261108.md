# webpj 구름 표시(RawKencloudRender) 이식 — WB04-E119

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백열아홉 번째 부분이다. 구름 배경의 갱신(00470c50)·표시 요청(00470c80)·표시 행 정리(00470ca0)·행 조회(00470ce0)와 두 표시 부모(004711d0/00471410)를 cpppj 기준선 `1f00a25a` 의 `o/RawKencloudRender` 에서 옮겼다. 선행은 직전 단위의 [RawKencloud](webpj-kencloud-20261108.md)와 같은 packed 상태 배열 규약뿐이다(모듈 import 는 없음). 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이다.

## 1. 옮긴 것

`www/core/raw-kencloud-render.js` — `updateKencloud`, `markKencloudDirty`, `cleanKencloudDirty`, `kencloudRowDirty`, `drawKencloud`

기존 Kencloud 소유자의 packed 상태 배열(`Uint32Array(2054)`: 이전 표시 행 512 + 가장자리 주소 + 캐시 주소(515) + 현재 행 표(516..) + 전체/부분 표시 요청(2051/2052))을 복제하지 않고 그대로 읽고 쓴다. 화면 장치(잠금·복사·clip)는 호출자가 훅으로 제공하며 계산·반복·쓰기 순서가 원문 몸체다.

- `updateKencloud(globals, hooks)`(00470c50): 캐시가 0 이면 `build` 로 생성한 뒤 현재 speed 를 다시 읽고(기존 캐시는 최초 speed 만 캡처), 캡처한 횟수만큼 `step` 을 부른다. 하위가 speed 를 바꿔도 이미 캡처한 반복 수는 유지한다.
- `markKencloudDirty(state)`(00470c80): 전체 요청 1 → 부분 0 순서(첫 store 뒤 중단은 그대로 남음). `cleanKencloudDirty(state)`(00470ca0): 실제 이전 표시 행만 순서대로 지우고(현재 행 표시 0·전체 0), 전체 표시가 남긴 부분 플래그도 마지막에 정리한다.
- `kencloudRowDirty(state, first, second)`(00470ce0): 두 값의 합을 signed 나머지(`%512`, 음수는 음의 나머지로 앞선 같은 물리 DWORD 를 읽음)로 행을 구해 그 행이나 전체 요청이 켜져 있으면 1 이다.
- `drawKencloud(globals, area, hooks, alternate)`(004711d0/00471410): 면적 `(오른쪽−왼쪽)×(아래−위)` 의 하위 32비트가 0 이면 장치도 조회하지 않는다. 캐시가 없으면 생성하고 현재 clip 을 저장·교체한 뒤 잠금(`alternate` 로 두 장치 중 선택)하고, 512 타일 격자를 세로로 훑는다. 전체 요청이 없으면 켜진 행마다 가로 타일별 한 줄 blit(+행 표시 유지), 있으면 타일 전체 blit 뒤 부분 요청을 1 로 둔다. 각 세로 타일에서 현재 전체 요청을 다시 읽고 각 복사 전에 현재 캐시를 읽는다. 잠금 해제와 clip 복구는 예외 때 가짜로 실행하지 않는다.
- 모든 덧셈/뺄셈은 x86 uint32 감김을 따르고 타일 경계 비교는 signed 이다. 물리 범위를 넘는 접근은 `RangeError`, 선택된 장치가 없으면 `Error` 로 도달 시점에 거부한다.

## 2. 검증

- `tests/kencloud-render.test.mjs`(2건):
  1. `kencloud-render-x86.tsv` 944행 전부(두 x87 정밀도 열 639/895 의 행 모두 — 10.78 단일 판본). 진입점(update/mark/clean/query/draw/alternate)·캐시·speed·dirty 패턴·전체/부분 요청·면적·원점·하위 변화(mutation, 명시 중단 포함)로 완료 종류, 조회 결과, 2054 DWORD 전체 물리의 FNV-1a 64비트(64비트 곱을 32비트 두 쪽으로 구현), 최종 clip, speed, 하위 호출 순서(trace)가 같다.
  2. 계약: 면적 0 / 기존 캐시 + speed 0 의 자연 생략은 장치를 요구하지 않음, 짧은 물리는 첫 store 뒤 거부되며 앞선 쓰기 유지, 행 −1 은 같은 cache DWORD 의 인접 alias, 선택된 장치 누락은 도달 시점에 거부.

최초 실패와 교정: 새 모듈은 944행 fixture 를 첫 실행에 통과했다.

## 3. 경계와 남은 것

- 실제 화면 잠금/복사(004a14e0/1550/15d0/1650·004a1fe0·clip 004a1580)는 훅 경계이며 별도 화면 단위에서 이식한다. 구름 생성/행 이동은 [RawKencloud](webpj-kencloud-20261108.md) 이다.

## 4. 다음 시작점

번개 수명/프레임/화면(`RawLightningLifetime`·`RawLightningFrame`), `RawArcherTypeData`, `RawGunProcess`(+`MakeGunTargetHooks`), `RawLocalGeyser`, `RawGunImpact`, Missile 계열(`RawMissile`), `RawPathProcess`(커널/Form 장면 선행)·`RawPathStart`·`RawPathData`·`RawPathCleanup`, `RawBalloonFrame`, `RawPacketDispatch`, 사제 배치 계열, `RawIslandRuntime` 계열, 분산 SP 정책, 그 밖의 작은 몸체이다.
