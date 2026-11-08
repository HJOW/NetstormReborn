# webpj 사제 소형 부품 네 개(소유 사제 조회·기도·공황 이동·Halt) 이식 — WB04-E66

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 예순여섯 번째 부분이다. 사제 수명 부품 중 호출 경계가 작아 서로 독립적인 네 파일 `o/RawPriestLookup`·`o/RawPriestPrayer`·`o/RawPriestPanic`·`o/RawPriestHalt` 를 cpppj 기준선 `1f00a25a` 에서 옮겼다. 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이라 CD 10.72 분기(BYTE 폭, 다른 소유자 위치, J bool helper, CD 반환 조건)와 10.37 행은 옮기지 않았다.

## 1. 옮긴 것

`www/core/raw-priest-lookup.js` — `RawPriestLookup(pool, state)`

- `find(player)`(004921f0): 활성 개수를 signed 로 읽어 0 이하이면 0, 아니면 공유 사제 목록을 원래 순서로 읽으며 현재 소유자 BYTE 와 전체 DWORD 인자가 같고 dead 비트(2)가 없는 첫 항목을 반환한다. 첫 일치 뒤의 손상된 항목은 읽지 않고, 용량 이상의 SID 는 WORD 로 자르지 않고 접근할 때 거부한다. free/void/타입/extra 는 검사하지 않는다.

`www/core/raw-priest-prayer.js` — `RawPriestPrayer(pool, types, boss, hooks)`

- `command(부모, 명령, 인자)`(4940b0): 상위 6비트를 뺀 명령이 기도(0x030000de)이면 인자의 상위 WORD 를 희망 종류로 하여, 첫 605 Regular 가 없고 현재 방향이 10..14 밖일 때만 8바이트 패킷(1,0x6d,8,0,부모,종류)을 대상 9 로 요청하고 항상 1 을 반환한다. 공황 명령(0x03000103)은 공황을 먼저 실행하고 나머지는 부모 처리기에 위임한다.
- `receive(패킷)`(493f80): 희망 종류(+42)를 쓰고 → 상태 비트 3 이면 끝 → 이미 605 가 있거나 방향 10..14 이면 끝 → 권한자는 이동 프로세스를 제거하고 패킷을 배포 → 예약 성공 시 605 공유 Regular 등록(payload 1) → 의식 단계 4.
- `tick(부모, 개수, payload)`(494580의 605): 개수(+46)를 쓰고 이동/정지를 조회한 뒤 마비이거나 이동 Form 이 있으면 0, 개수가 현재 희망 종류의 기도 횟수와 같을 때만 권한자의 내용물 교체와 기본 프레임 지정 후 0, 아니면 payload 를 그대로 돌려준다. 권한은 getter 로 각 분기에서 다시 읽는다.

`www/core/raw-priest-panic.js` — `RawPriestPanic(pool, speeds, hooks)`

- `run(사제)`(00491db0): 현재 좌표의 섬 신전이 있고 현재 HP 가 최대 HP 보다 작고 신전과 소유자가 같으면 현재 TYPE 속도를 임시로 두 배로 하여 신전 경로를 시작하고 속도를 절삭한 원래 정수로 돌린다(경로가 생기면 끝). 아니면 땅 위 사제는 ±20 임의 후보 좌표가 같은 영역일 때까지, 섬 밖 사제는 첫 후보에서 점 경로를 시작한다(경로 실패여도 반환). 정수 변환이라 속도의 음의 0 을 `| 0` 으로 정규화한다.

`www/core/raw-priest-halt.js` — `PriestHaltState`, `RawPriestHalt(pool, hash, types, spots, check, priest, hitPoints, state, hooks)`

- `halt(sid)`(사제 가상 +38): 최초 좌표를 보관한 뒤 전투 모드이면 좌표(0.9999 를 더해 절삭)의 spot & 6 이 없을 때(권한자 Unpop→Pop 보정) 또는 지면이 있어도 현재 HP 가 최대 HP 의 절반 미만이고 권한자일 때 11 을 반환하고, 그 밖에는 공통 Halt 로 위임한다. 공간 보정 좌표는 최초 좌표를 0.99998999 로 절삭한 칸이다.
- `commonHalt(sid)`(0042a5f0): 실제 `RawCarrierCheck`(E65)가 참이면 12, 아니면 현재 좌표를 단순 절삭한 한 칸의 일반 finder(`RawSquidFinder`)를 돌며 자신이 아닌 운반체 타입(156)의 포획 몸체와 너겟 운반체 타입(187)의 전달 몸체가 12 를 돌려주면 12, 그 밖에는 10 이다.

## 2. 검증

- `tests/priest-lookup.test.mjs`(3건): `priestlookup-x86.tsv` 10.78 1617행(반환 SID, 128슬롯 raw 와 목록의 Adler-32, 조회가 입력을 바꾸지 않음), 조기 일치/손상 개수·SID 의 늦은 거부와 DWORD 소유자 폭, 실제 `SquidFactory`·`SquidOwner`·`RawPriestPostPop`·`RawPriestPreDestroy` 로 등록/중복 억제/소유자 지정/삭제 준비의 목록 압축이 같은 조회에 반영되는 합성.
- `tests/priest-prayer.test.mjs`(2건): `priest-prayer-x86.tsv` 1126행(명령 C·수신 R·진행 T 의 호출 순서, 슬롯 60 raw, 반환 비트, 최종 권한), 선택되지 않은 하위 효과 비요구/도달한 누락 진단/짧은 패킷의 쓰기 전 거부/잘못된 연결 거부.
- `tests/priest-panic.test.mjs`(2건): `priest-panic-x86.tsv` 600행(호출 순서 T/H/V/L/G/R/P, 사제·신전 raw, 두 TYPE 속도 비트, 난수 상태), 연결 오류 거부와 도달한 하위 누락 진단.
- `tests/priest-halt.test.mjs`(2건): `priesthalt-x86.tsv` 10.78 432행(반환 코드, 호출 순서 F/U/P/C/N, 슬롯 50 raw; 실제 `RawCarrierCheck`·`RawPriestState`·`SquidReward`·`RawSquidFinder` 사용), 연결 오류(다른 풀·표/지도 크기·누락 훅) 거부.

최초 실패와 교정: 위 fixture 3775행 중 panic 의 속도 비트 한 곳이 어긋났다(원래 속도를 `Math.trunc` 로 절삭한 값이 음의 0 으로 남아 `float` 로 되돌릴 때 `0x80000000` 이 쓰임). 원본은 정수 변환이라 `| 0` 으로 정규화해 해결했다. halt 행 수는 처음 한 판본의 3분의 1(288)로 착각했으나 판본당 432행이다.

## 3. 경계와 남은 것

- 건설 확정/통지 어댑터(cpppj `PriestLookup_ConstructionConfirmUsesActualGraphAnchor`·`ConfirmNoticePreservesCapturedPriestAndRereadsCurrentOwner`·`SelectionAdaptersRejectForeignPools`)는 `RawConstructionConfirm`·`RawConstructionNotice`·`RawPlayerPlacementAnchor` 이식 때 대조한다.
- 기도의 큰 하위(이동 Form 제거/배포, 의식 단계, 내용물 교체, 공황)·공황의 신전/영역/경로 조회·Halt 의 포획/전달 몸체는 각 파일 이식 때 연결한다. 이 단위의 합성 검사는 명시 경계의 호출 순서와 인자 비트를 원본과 대조한다.

## 4. 다음 시작점

`RawPriestRegen`(회복 분배기와 패치 추적), `RawPriestDamage`, `RawPriestSpawn`, `RawPriestParticipation`/`RawPriestTakenNotice`, 사제 배치 계열(`RawPriestPlacement*`), `RawAssetPostPop` 이다.
