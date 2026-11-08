# webpj 사제 후반 후처리와 사제 Pop 통합(RawPriestPostPopTail) 이식 — WB04-E64

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 예순네 번째 부분이다. 사제 postPop(원본 004950f0) 전체 wrapper 의 후반 `o/RawPriestPostPopTail` 을 cpppj 기준선 `1f00a25a` 에서 옮겼다. 앞 단위들이 만든 부품(E60 Carrier 후처리, E62 사제 상태·경로 애니메이션, E63 사제 접두·보호막)을 접두 → Carrier → 이동 불가 조회 → 낙하/보호막 순서로 조립하고, 실제 `SquidPop`/`SquidPostPop` 의 가상 분배(`0x50f210`)로 사제 Pop 전체 흐름까지 원본 관측과 대조했다. 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이라 CD 10.72 분기(CD 구성 시 효과 사전 요구, BYTE 프레임·inline 지면)는 옮기지 않았다.

## 1. 옮긴 것

`www/core/raw-priest-post-pop-tail.js` — `RawPriestPostPopTail(pool, prefix, carrier, state, frames, spots, hooks)`, `makePriestPostPopTailShieldHooks`

- `postPop(sid, flags)`: `prefix.prefix` → `carrier.postPop` → Carrier 뒤 현재 extra 의 abstract/buried 비트(9)가 있으면 끝 → `state.immobile`(00427030)이 거짓이면 현재 보호막을 조회/삭제(`clearShield`)하고 끝 → 이동 불가이고 낙하 재등록 flag(0x800)가 없으면 현재 좌표에 0.9999 를 더해 절삭한 칸의 spot & 6(지면)이 없을 때 현재 프레임의 방향(signed side − 'A')이 낙하 방향 9 가 아니면 낙하 요청(`fall`, 004941f0) → 마지막으로 `ensureShield`(00493d30). 낙하 효과가 extra/지면을 바꿔도 원본이 선택한 이동 불가 분기의 보호막 요청을 유지한다.
- 풀/프레임 표(188칸)/spot 지도(65536칸) 연결 오류는 첫 접두 효과 전에 거부하고, 낙하/보호막 훅은 도달할 때만 요구한다. `validate(sid)` 는 접두의 명시 사전 검사에 위임한다.
- `makePriestPostPopTailShieldHooks(pool, shield, lookup, destroy, fall)`: `ensureShield` 는 E63 의 `RawPriestShield.ensure`, `clearShield` 는 `RawPriestForcefield.find` 결과가 1 이상 풀 범위 안일 때 `destroy(found, 0)` 이다.

## 2. 검증

- `tests/priest-post-pop-tail.test.mjs`(7건): `priestpostpoptail-x86.tsv` 10.78 2048행(전체 wrapper의 호출 기록 C/D/S/X·슬롯 50바이트·목록·소유자 상태 9칸; 실제 Carrier 본체·사제 상태·접두를 그대로 사용하고 Carrier 확인/지면/낙하/보호막 경계만 기록), `priestpop-x86.tsv` 10.78 1280행(실제 `SquidPop.pop` → 공통 firstPop/Activate/가상 분배 → 사제 wrapper 의 기록·슬롯·해시 머리·목록·소유자·깊이). 계약: 연결 오류 거부와 PATCH 구성의 도달 전 효과 비요구, 낙하 flag 의 지면/프레임 조회 생략과 Carrier 뒤 extra 차단, 가상 표만으로의 분배와 명시 `validate` 분리, 도달한 보호막/낙하만 그 시점 거부하며 앞선 부분 효과 보존, signed 초과 목록의 등록만 생략하는 실제 Pop.

최초 실패와 교정: 새 모듈은 위 fixture 3328행 전부를 첫 실행에 통과했다.

## 3. 경계와 남은 것

- 회복 Form/Kernel·낙하(`RawPriestFall`)·프레임 단계 이동·ProcessHost 를 통째로 잇는 통합 장면(cpppj `PriestPostPopTail_ComposesProcessBooksAndShieldLifetimeRequests`, `PriestPop_ComposesFallFrameSpaceAndRegularLifetime`)은 해당 부품(Kernel 호스트 장면, `RawPriestFall`, `RawCarrierCheck`, `RawFrameAdvance`)의 이식 단위에서 대조한다.
- `SquidPostPop.setBody('priest', tail)` 의 실제 World 등록과 사제 생성/배치(`RawPriestPlacement*`)는 WB05 월드 조립과 각 파일 단위에서 한다.

## 4. 다음 시작점

`RawPriestFall`·`RawCarrierCheck`·`RawFrameAdvance`(낙하/보행 프레임), 사제 배치 계열(`RawPriestPlacement*`), `RawAssetPostPop`(Gun·제네레이터·거주 구역·바리케이드·간헐천·신전 분배), 후보/지형 권한(`RawCanonPlacementPermission`) 이다.
