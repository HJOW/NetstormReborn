# webpj 운반물 내려놓기 위치(RawCarrierDropPoint) 이식 — WB04-E95

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 아흔다섯 번째 부분이다. 운반물(Carrier)이 내용물을 내려놓을 위치(426a70)·발자국 탐색 helper(49bfe0)·좌표 snap(41d7a0)을 cpppj 기준선 `1f00a25a` 의 `client/RawCarrierDropPoint` 에서 옮겼다. 선행 모듈은 없다(SidPool 만 사용). 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이다.

## 1. 옮긴 것

`www/core/raw-carrier-drop-point.js` — `RawCarrierDropPoint(pool, types, state, hooks)`

- `find(carrier, type)`(426a70): 운반물 슬롯의 현재 좌표(+14/+18 float)를 snap 한 점을 만든다. TYPE flags2 에 0x100 이 있으면 외부 중심(`hooks.center`)을 snap 해 바로 반환한다(배치 검사를 부르지 않음). 아니면 현재 점이 소유자(+34 BYTE)로 허용되는지(`hooks.mayPlace`) 검사해 허용이면 그 점을 그대로 반환한다(유효성 재검사 없음). 아니면 짝수 방향(0·2·4·6) 네 개를 먼저, 홀수 방향 네 개를 이어 기준점에 방향 오프셋을 더한 점으로 `findFootprint` 를 부르고, 반환점이 두 축 모두 0 초과·현재 지도 한계 미만이면 채택한다. 소유자는 각 `findFootprint` 호출 직전에 슬롯에서 다시 읽는다(배치 검사가 슬롯을 바꾸면 이후 방향에 반영). 맞는 방향이 없으면 현재 실패점을 반환한다.
- `findFootprint(type, argument, x, y, flags, owner)`(49bfe0): 대상 TYPE 의 현재 `footX/footY` 를 x 바깥/y 안쪽 순서로 순회하며 `mayPlace({type, argument, x+열, y+행, flags, owner, mode 0})` 가 처음 허용하는 점을 반환한다. 반복 조건마다 `footX/footY` 를 다시 읽고(배치 검사가 바꾸면 반영), 실패한 열을 건너뛰지 않으며, 없으면 현재 실패점을 반환한다.
- `RawCarrierDropPoint.snap(point)`(41d7a0): 각 좌표에 단정도 bias(0.99999f)를 53비트 x87 정밀도(JS 배정도)로 더하고 CRT signed64 절삭의 하위 DWORD 를 signed32 로 해석해 float 에 저장한다. 비유한·signed64 범위 밖 값은 CRT 불확정 정수 0 이다.
- 현재 값만 읽는다: `state = { mapLimit(signed32), invalidPoint([x, y]) }`, `types[type] = { flags2, footX, footY }`. 도달한 경로에서 훅이 없으면 오류이고(실패점으로 바꾸지 않음), CD(10.72) 풀·빈 TYPE 표·TYPE/SID 범위 오류는 거부한다.

## 2. 검증

- `tests/carrier-drop-point.test.mjs`(3건): `carrier-drop-point-x86.tsv` 998행(10.78 전용 단일 판본). 입력 18 DWORD(연산 0 Find / 1 FindFootprint / 2 Snap·좌표·소유자·TYPE flags2·footX/footY·중심 x/y·지도 한계·실패점·성공 순번·결과·첫 호출 재진입 변이 9종·인자·flags·소유자)로 ① Find 609 + FindFootprint 133 = 742행의 결과 점 비트·배치/중심 훅 호출 순서(P/C)·현재 전역 여섯 값(지도 한계·실패점·flags2·footX·footY)·운반물 슬롯 전체 50바이트 raw 가 같고, ② Snap 256행의 기본 53비트 열이 같으며(64비트 정밀도 열과 다른 31행도 구분), ③ 누락 훅·판본·TYPE/SID 범위 오류와 foot 0 의 실패점 반환 계약이 같다.

최초 실패와 교정: 새 모듈은 위 fixture 998행과 계약 검사를 모두 첫 실행에 통과했다.

## 3. 경계와 남은 것

- 전체 배치 검사(`RawCanonPlacementPipeline.MayPlace`, 0049b510)와 실제 발자국 중심(`RawGunTarget.Center`, 004adea0)은 외부 훅이다. 실제 운반물 메뉴(`RawCarrierMenu`)가 이식 전이므로 메뉴 연결(`BindCarrierMenuDropPoint`)과 전체 배치 파이프라인 훅 조립(`MakeCarrierDropPointHooks`)은 해당 단위에서 한다. cpppj 의 `RealPipelineCenterAndMenu` 통합 검사도 그때 옮긴다.

## 4. 다음 시작점

`RawBalloonFrame`·`RawPathRuntime` 계열(보행/경로), `RawCarrierMenu`+`RawCarrierInput`(메뉴 연결 포함), `RawGunGroundTarget`·`RawGunProcess`(+`MakeGunTargetHooks`), Missile 계열, `RawTempleDraw`/캡처 계열(자산 시험 장치), `RawCarrierArrival`, `RawAssetPostPop`, 사제 배치 계열(선행 `RawCanonPixelShape`·`RawCanonPlacement`), 후보/지형 권한(`RawCanonPlacementPermission`), `RawSquidTransmission`, `RawDudeSpawn`·`RawDudeNotice`→`RawDudeDrop`, `RawManaBoltArrival`(시험 장면 선행), `RawIslandRuntime` 계열, 그 밖의 작은 몸체이다.
