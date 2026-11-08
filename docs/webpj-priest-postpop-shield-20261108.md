# webpj 사제 후처리 접두·삭제 준비·보호막 조회/생성(RawPriestPostPop·RawPriestPreDestroy·RawPriestForcefield·RawPriestShield) 이식 — WB04-E63

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 예순세 번째 부분이다. 사제(Priest)의 생성 이후 수명 부품 네 개를 옮겼다. cpppj 기준선 `1f00a25a` 의 `o/RawPriestPostPop`(원본 004950f0 접두: 목록·회복 예약·소유자 상태), `o/RawPriestPreDestroy`(004919b0 삭제 준비), `o/RawPriestForcefield`(004918e0 보호막 조회), `o/RawPriestShield`(00493d30 보호막 확보)이다. 네 파일 모두 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이라 CD 10.72 분기(CD inline 목록, 소유자 쓰기 helper, 구성 시 효과 사전 요구)는 옮기지 않았다. 앞 단위(E62)의 사제 소유자·상태 부품과 E60 의 Carrier 후처리를 후반 조합(다음 단위 `RawPriestPostPopTail`)이 이어 쓴다.

## 1. 옮긴 것

`www/core/raw-priest-post-pop.js` — `RawPriestPostPop(pool, hp, state, hooks, priestType)`, `PriestPostPopState`, `resetPriestOwnerState`, `makePriestPostPopProcessHooks`, `PRIEST_REGEN_EVENT`(0x25a)

- `prefix(sid, flags)`: extra 의 abstract/buried 비트(9)가 있으면 아무 효과도 없다. 최초 flag 에서 사제 목록에 중복 없이 추가한다(00414450: 처음 읽은 signed 개수가 signed 용량 미만일 때만, 현재 개수로 중복 검색, 처음 읽은 개수 위치에 저장한 뒤 현재 개수를 증가; 음수/짧은 배열 저장은 호스트 물리 경계로 거부). 이미 예약된 회복이 없고 예약이 성공하면 `(현재 사제 TYPE HP / 6) * 50 / 객체 최대 HP`(float; 정수 나눗셈의 0 은 −0.0 이 되지 않는다) payload 로 사건 0x25a 를 등록한다. 예약 효과 뒤 현재 원래 소유자 WORD 하위 7비트와 현재 소유자 BYTE 가 같고 유효 소유자 1..8 이면 소유자 상태 한 칸을 0 으로 쓴다.
- PATCH 는 사전 검사 없이 현재 물리 슬롯을 읽고 도달한 장치만 그 시점에 요구한다(`validate` 는 명시 사전 검사). 현재 사제 TYPE(005412d0)은 getter 로 호출마다 다시 읽는다. `resetPriestOwnerState` 는 범위 밖 번호에서 진단 뒤 쓰기를 생략한다. 사제 목록·소유자 상태는 `shared*` 로 실제 Session 의 장기 저장소를 빌릴 수 있다.

`www/core/raw-priest-pre-destroy.js` — `RawPriestPreDestroy(pool, state, hooks)`, `makePriestPreDestroyHooks`

- `preDestroy(sid, flags)`: 사제 TYPE/가상 표/free·contained 상태를 확인한 뒤(dead/void 는 실행) 일반이면 목록 개수를 확인하고 Carrier 사전 검사 → 목록에서 이 SID 를 모두 지워 활성 DWORD 만 압축(비활성 꼬리 보존) → 같은 소유자 보호막 조회 → 1 이상 풀 범위 안이면 보호막 삭제(flags 0) 요청 → 항상 Carrier 삭제 준비 순서. 추상/매장 사제는 목록을 읽지 않고 Carrier 만 호출한다.
- `makePriestPreDestroyHooks`: 삭제 훅의 `preDestroy` 사건 가운데 사제만 가로채고 나머지는 fallback 으로 넘긴다.

`www/core/raw-priest-forcefield.js` — `RawPriestForcefield(pool, hash, types, state)`, `PriestForcefieldState`, `makePriestForcefieldHooks`

- `find(priest)`: 사제 좌표를 0 방향 절삭한 한 칸에서 일반 finder 순서로 같은 타입(현재 보호막 TYPE DWORD)·같은 소유자 BYTE 의 첫 SID(없으면 0). 후보의 free/dead/void 는 제외하지 않는다(매장은 finder 가 제외). 손상 사제와 유한 signed int 범위를 넘는 좌표는 거부한다. `type()` 은 생성이 쓰는 현재 TYPE 이다.

`www/core/raw-priest-shield.js` — `RawPriestShield(pool, lookup, state, hooks)`, `PriestShieldState`, `makePriestShieldCreationHooks`

- `ensure(priest)`: 기존 보호막이 있으면 모두 생략. 없으면 현재 보호막 TYPE 으로 flags 2 생성(유효 자산 SID 가 아니면 거부) → 사제 좌표 캡처(NaN/−0 비트 보존) → 소유자 지정 → 가상 Pop(0) → 소리 예약 성공 시 `priestForceField.wav` 재생 → 소유자/로컬/로딩/알림 시각(+0/−0 만, NaN 제외) 조건이 맞으면 `ourPriestImmobile.wav` 소리와 `PriestImmobile` 안내(번역 없이)를 요청. `makePriestShieldCreationHooks` 는 같은 풀의 `SquidFactory.create`/`SquidOwner.set` 에 연결한다.

## 2. 검증

- `tests/priest-post-pop.test.mjs`(9건): `priestpostpop-x86.tsv` 10.78 Prefix 2392행(목록 결과·소유자 상태 9칸·검색/확보/생성 기록·슬롯 50바이트). 계약: 사전 검사와 접두 분리, 확보 실패 시 HP 조회/생성 생략, 물리 슬롯 읽기, signed 목록 관문과 현재 개수 재조회(공유 목록 연결 포함), 현재 사제 TYPE DWORD 와 1/4 비교, 도달한 장치만 요구, 데이터 전용 정리의 진단 복귀, 예약 뒤 현재 공유 표 선택.
- `tests/priest-pre-destroy.test.mjs`(2건): `priestdestroy-x86.tsv` 10.78 2184행, 잘못된 raw/목록/미연결 하위 거부와 분배기. `tests/priest-forcefield.test.mjs`(2건): `priestforcefield-x86.tsv` 10.78 2976행(Find/Pre, 일곱 슬롯 전체), 손상 입력 거부와 읽기 전용 조회. `tests/priest-shield.test.mjs`(3건): `priestshield-x86.tsv` 10.78 832행(double 시각 비트 포함), 실제 Factory/소유자 연결로 한 번만 생성·삭제 준비가 보호막을 찾음, 누락 경계/다른 풀 거부.

최초 실패와 교정: 모듈은 대부분 fixture 를 첫 실행에 통과했다. 접두 fixture 834행에서 회복 payload 가 원본의 부호 비트(−0.0)와 달랐다 — 원본은 정수 나눗셈 결과 0 을 정수로 곱하므로 분자가 +0.0 이고 음수 최대 HP 로 나눌 때만 −0.0 이 되는데, JS 의 `Math.trunc` 가 −0 을 만들었다. 정수로 되돌려(`| 0`) 해결했다. 공유 목록 계약 검사에서 검사 쪽 호출 기록이 독립 목록 개수를 읽던 점은 현재 목록 개수를 읽도록 고쳤다(cpppj `Priests().Count()`).

## 3. 경계와 남은 것

- 실제 ProcessHost 에 연결한 접두(`priest_postpop_prefix_binds_real_process_host_and_owner`)와 Damageable/Carrier/종속 form/Kernel 정리 통합(`priest_predestroy_cleans_real_regen_form_kernel_and_common_bookkeeping`)은 해당 부품(Kernel 호스트 장면·Damageable 삭제 준비)의 통합 단위에서 대조한다. 훅 어댑터(`makePriestPostPopProcessHooks`)는 호스트의 `findEvent`/`addRegular` 에 맞춘다.
- 보호막의 Pop·소리·안내 하위, 가상 destroy, 실제 World 연결은 WB05 에서 한다.

## 4. 다음 시작점

`RawPriestPostPopTail`(접두→Carrier→이동 불가 조회→낙하/보호막), 사제 Pop/Activate 통합(`PriestPopTests`), `RawAssetPostPop`, 후보/지형 권한(`RawCanonPlacementPermission`) 이다.
