# webpj 사제 회복(RawPriestRegen) 이식 — WB04-E69

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 예순아홉 번째 부분이다. 사제 이벤트 0x25a(원본 00494580)의 실제 HP 회복, 중립 사제 후처리, 패치 전용 SP 추적을 cpppj 기준선 `1f00a25a` 의 `o/RawPriestRegen` 에서 옮겼다. 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이라 CD 10.72 분기(signed WORD HP, 중립 helper 다른 명령, 추적 없음)와 10.37 행은 옮기지 않았다.

## 1. 옮긴 것

`www/core/raw-priest-regen.js` — `PriestRegenPatchState`, `PriestRegenState`, `RawPriestRegen(pool, hp, priest, mode, space, random, state, hooks)`, `bindPriestRegenSpStore`, `makePriestRegenHandler`

- `handle(sid, 0x25a, count, payload)`: 사제 TYPE/가상 표(0x50f210)를 확인한 뒤 난수(9)로 표본을 갱신하고 추적 sequence 를 +3(9600 초과면 한 번 -9600, signed 감김)한다. 이어 E15 `readPriestRegenHitPoints`(현재 HP→최대 HP→증분 `현재 + TYPE 최대/6`을 현재 사제 타입 번호로 읽음)가 값을 돌려주면 E62 `RawPriestState.setHitPoints`(절반 경계 공간 전환 포함)로 지정하고 표시 갱신→Damageable 후처리를 부른다. 패치 감사(`patchAudit`) 뒤 현재 소유자·권한을 다시 읽어, 권한자이고 두 차단 전역이 꺼져 있고 소유자 0 이며 현재 HP 가 최대 HP 의 절반 이상이고 점유가 없으면 중립 행동을 한다. 마지막으로 추적 확인: 관측 sequence 가 현재 sequence 와 다르거나, 남은 측정이 있고 측정이 7000 을 넘거나, 첫 분산 배열이 없거나, 첫 순열 WORD 가 -1 이면 현재 표본을 변경 표본에 기록한다(남은 측정은 하나 줄임). 반환은 payload 그대로이다.
- `PriestRegenPatchState`: 표본·변경 표본(진단)·남은 측정·독립 입력의 관측 sequence/gate/sentinel 과 추적 sequence holder(`tracking`, `{ value }`)를 가진다. 실제 Client 의 장기 표본/진단 DWORD 는 `sharedSample`/`sharedChangedSample` holder 로 빌려 각 원문 store 위치에서 현재 연결된 주소에 쓴다.
- `bindPriestRegenSpStore(store, patch)`: SP 저장소의 추적 holder 가 `patch.tracking` 과 같은 객체여야 하고(다르면 거부) 관측 sequence·첫 배열 존재·첫 순열 WORD 를 호출 시점마다 저장소에서 읽는 묶음을 만든다. `patch` 없이 부르면 공통 시작에서 이미 초기화한 장기 추적 holder 를 직접 빌려 Session 지역 사본을 만들지 않는다.
- `makePriestRegenHandler`: 실제 사제 vtable 의 0x25a 만 연결하고 다른 사제 이벤트는 fallback 이 없으면 미복원 예외, 비사제는 fallback 또는 payload 반환이다.

`www/core/sp-store.js` — `trackingOwner` getter 추가(빌린 추적 holder 자체를 돌려주며 미연결이면 오류). cpppj `TrackingOffsetCurrent()` 가 돌려주는 참조의 동일성 검사를 JS 에서 같은 객체 비교로 옮기기 위한 변경이며 기존 동작은 바꾸지 않았다.

## 2. 검증

- `tests/priest-regen.test.mjs`(9건): `priestregen-x86.tsv` 10.78 1513행(반환 float 비트, 호출 순서 V/D/N/U/R/F/O/A/M, 패치 추적 상태, 난수 상태, 슬롯 50 raw; 실제 HP 증분·`RawPriestState` setter 사용), 빠진 외부 효과/풀/미지원 사건/잘못된 vtable 거부와 fallback, 현재 사제 타입(74/75) 전역으로 증분 분자만 바뀌고 Actor 는 158 유지, 실제 `SquidFactory`·`RawPriestPostPop`·`SquidProcessHost`·`Kernel` 로 server/client 생성자의 HP 80→113→…→200 회복과 재등록(감사 5회, 삭제 뒤 풀 복원), 중립 효과의 SP 회전 뒤 최종 현재 offset 읽기, 공통 시작 추적 DWORD 직접 갱신, observed→측정→첫 포인터→WORD 읽기 순서와 측정 안의 lazy 초기화, NULL 배열이어도 WORD 읽기·미연결/다른 추적 주소/부분 getter 거부, 네 추적 조건이 같은 장기 진단 holder 에만 쓰는지.

최초 실패와 교정: 새 모듈은 위 fixture 1513행과 계약 8건을 첫 실행에 통과했다.

## 3. 경계와 남은 것

- 중립 행동(`neutralAction`)은 원본 00491db0 이며 E66 `RawPriestPanic.run` 이 그 몸체이다. 이 단위에서는 점유 검사(`occupied`)·형태 조회(`shape`)·측정(`measurement`)·패치 감사(`patchAudit`)와 함께 명시 경계로 남겨 두었고, 실제 연결(공황 속도 표·경로 생성기 포함)은 해당 파일 이식과 월드 조립 때 한다.
- 사제 후처리 접두(E63)가 등록한 0x25a Regular 가 이 처리기로 분배되는 흐름은 위 합성 시험에서 확인했다. 앱 수명 연결은 WB05 월드 조립에서 한다.

## 4. 다음 시작점

사제 배치 계열(`RawPriestPlacement*`), `RawAssetPostPop`, 후보/지형 권한(`RawCanonPlacementPermission`), `RawSquidTransmission` 이다.
