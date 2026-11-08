# webpj 벌룬 그림자 몸체(RawBalloonBody) 이식 — WB04-E94

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 아흔네 번째 부분이다. 벌룬(42)·에어십·클라우드 플로터의 그림자 후처리(`o/RawBalloonBody`: 42ac90, 래퍼 42aed0)·그림자 우선 삭제(42ae80)·클라우드 전용 Regular(42af60)·저장 검사(42ae60)를 cpppj 기준선 `1f00a25a` 에서 옮겼다. E60 의 `raw-carrier-post-pop.js` 와 E56 의 `raw-carrier-pre-destroy.js` 가 선행이다. 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이다.

## 1. 옮긴 것

`www/core/raw-balloon-body.js` — `RawBalloonBody(pool, types, spots, boss, post, pre, hooks)` (`boss` 는 현재 권한 getter, `spots` 는 65536칸 지도 비트)

- `handles(sid)`: 가상 표 0x5040c0(벌룬/에어십)·0x504198(클라우드 플로터)만. `robust(sid)`(저장 가상 60): 최초 signed 프레임을 보관한 뒤 현재 TYPE 의 프레임 수와 비교(`frame < frames(TYPE)`; 호출 효과가 프레임을 바꿔도 처음 읽은 값). `validate(sid, flags)`: 현재 원시 부모(할당·extra&9 없음·자산 TYPE)만.
- `postPop(sid, flags)`: 클라우드 플로터이고 최초 flag 면 먼저 추가 주기 Regular(event 3, 0.1f)를 **할당 성공 때만** 생성한다. 이어 최초 flag·현재 프레임이 TYPE 프레임 수 미만·TYPE flags1&0x400000 이면 그림자를 만들어(프레임 = 프레임 수 + 부모 프레임, extra|=8, 부모 +8 WORD 에 연결, 부모 좌표로 Pop) 첫 그림자 주소를 보관하고, Unpop 후 `높이(+30)×0.0909…` 로 계산한 좌표(`height×0.5+x+0.1f`, `height+y−0.3f`; 중간 float 저장 없이 fstp 에서만 반올림)에 Pop 한다. 첫 Pop 효과 뒤 현재 그림자 WORD 를 **다시 읽어** extra bit4 를 부모와 맞추고 표시 깊이를 정하며(그림자 0xe6·부모 0xe2, 높이 0 이면 현재 칸 bit4 에 따라 부모 −20/−21·그림자 0xec; 좌표가 칸 밖이면 거부), 최초 flag 가 없고 권한이 없으면 전용 check 를 부른 뒤 Carrier 공통 Damageable 후처리를 한 번 실행한다.
- `preDestroy(sid, flags)`: 현재 그림자(+8 WORD)를 먼저 삭제하고 Carrier 기본 전처리를 실행한다. `cloudRegular(sid, event, count, payload, advance, base)`: 클라우드 플로터 전용으로 event 3 만 프레임을 진행(`advance(sid, 1, 0)`)하고 입력 payload 를 돌려주며 count/payload 를 바꾸지 않고, 나머지는 기본 Carrier Regular 에 위임한다.

## 2. 검증

- `tests/balloon-body.test.mjs`(2건): `balloon-body-x86.tsv` 233행 + `balloon-body-height-x86.tsv` 256행(두 x87 정밀도 동일)의 입력 11 DWORD(PreDestroy/PostPop·flags·부모 프레임·그림자 WORD·TYPE flags1·높이·boss·지도 비트·외부 변이 5종·내용물 연결·좌표 0 여부)로 외부 효과 순서(C/P/U/K/X/B/D/R)·부모와 두 그림자의 전체 150바이트 raw·현재 `boss,프레임 수`가 같다(원문 안의 finder 내부 표식 `F:` 는 외부 효과가 아니라 제외). 계약: 저장 검사가 signed 프레임을 먼저 읽고 현재 TYPE 의 그림 개수(음수 포함)와 비교, 클라우드 Regular 의 event 3 진행/나머지 위임, 다른 가상 표·CD 풀·지도 크기 거부.

최초 실패와 교정: 새 모듈은 위 fixture 489행과 계약 검사를 모두 첫 실행에 통과했다.

## 3. 경계와 남은 것

- 벌룬의 이륙/경로/걸음(`RawBalloonFrame`·`RawPathRuntime` 계열; cpppj `BalloonPath_IndependentNativeBodies` 688행)과 Carrier 공통 후처리/전처리의 실제 Damageable·내용물 본체는 해당 단위에서 한다. 그림자 생성(4af530)·Pop/Unpop/삭제 가상 호출은 훅 경계이다.

## 4. 다음 시작점

`RawBalloonFrame`·`RawPathRuntime` 계열(보행/경로), `RawCarrierDropPoint`(client), `RawGunGroundTarget`·`RawGunProcess`(+`MakeGunTargetHooks`), Missile 계열, `RawTempleDraw`/캡처 계열(자산 시험 장치), `RawCarrierArrival`, `RawAssetPostPop`, 사제 배치 계열(선행 `RawCanonPixelShape`·`RawCanonPlacement`), 후보/지형 권한(`RawCanonPlacementPermission`), `RawSquidTransmission`, `RawDudeSpawn`·`RawDudeNotice`→`RawDudeDrop`, `RawManaBoltArrival`(시험 장면 선행), `RawIslandRuntime` 계열, 그 밖의 작은 몸체이다.
