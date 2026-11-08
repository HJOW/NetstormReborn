# webpj 골렘 Regular(RawCarrierRegular) 이식 — WB04-E91

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 아흔한 번째 부분이다. 골렘(Carrier)의 Regular Handle(`o/RawCarrierRegular`, 42a750)을 cpppj 기준선 `1f00a25a` 에서 옮겼다. E72·E73 의 Carrier 몸체(task·낙하·집기·내용물 통지·전달)가 이 처리기의 사건 0x5105(낙하)와 하위 효과로 이어진다. 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이다.

## 1. 옮긴 것

`www/core/raw-carrier-regular.js` — `RawCarrierRegular(pool, types, details, boss, hooks)` (`boss` 는 현재 권한 getter, `details[n] = { defaultFrame, castTime }`)

- `handles(parent)`: 실제 +5c 가 42a750 인 여섯 VT(0x503d80·0x503e50·0x503f20·0x503ff0·0x5040c0·0x50e568)만 직접 받는다. `handler(fallback)`: 파생 부모는 caller 분배로 넘기며 fallback 이 없으면 거부한다.
- `handle(parent, event, count, payload)`: signed 0x494 미만에서 — **0x493**(시전 기한): 현재 내용물(`find` mask 1)의 종류 TYPE 의 casttime×10(32비트 감김)이 현재 횟수(signed)보다 작으면 0(빈 결과도 물리 0번 슬롯에서 종류를 읽음). **0x491**(교대 표시): 기한 `1.5f/payload`(float 저장)와 횟수(float)를 비교하며 mask 0x20 → 1 순서로 조회(조회마다 기한 검사), 내용물 종류의 flags2 가 0x208100 이 아니면 0, 현재 횟수 홀짝에 맞는 쪽이면 새 미리보기를 만든다(기본 프레임 +36 직접 쓰기·+35 = 부모−1·extra|=8·단어/소유자 지정·부모 좌표를 [1, 255] clamp 하고 y 에서 0.8/1.2f 를 뺀 위치로 Pop·타이머는 할당 성공 때만 생성). 둘 다 없으면 0, 아니면 부모 표시 갱신. **0x492**(회피, boss 만): 다리 칸/코드 비트 판정에 걸리고 이동 프로세스가 없으면 회피(1.0, 0xffffffff).
- **0x5105** 는 낙하 반환값, 그 밖의 사건과 위 분기 끝은 실제 Damageable 기본 처리기로 넘어간다. 선택하지 않은 효과는 요구하지 않고(내용물 없음/만료 조기 반환 포함) 도달한 하위의 누락만 거부한다.

## 2. 검증

- `tests/carrier-regular.test.mjs`(3건): `carrier-regular-x86.tsv` 837행(사건·개수·payload·내용물 존재·새 TYPE flags2·casttime·좌표 상하한/NaN/무한·타이머 할당 성공/실패·boss·회피/이동/경로·외부 변경 8종 조합)의 반환 float 비트·외부 효과 순서(F/C/W/O/P·X/A/T/R/E/M/H/V/D/B)·여섯 슬롯 전체 raw 가 같고, `carrier-regular-vtables.tsv` 83행에서 직접 분배 6·파생 fallback 77 이다. 계약: 내용물 없음/만료는 생성·표시 경계를 요구하지 않고, 기본/낙하 경계 누락·CD 풀·권한 getter 아님을 거부한다.

최초 실패와 교정: 새 모듈은 위 fixture 920행과 계약 검사를 모두 첫 실행에 통과했다.

## 3. 경계와 남은 것

- 내용물 조회(`4ac9f0`)·새 미리보기 생성(`4af530`)·가상 소유자/Pop·타이머(`428700`)·회피(`429010`)·낙하(`426ef0`)·Damageable(`44b900`)의 실제 본체 연결은 해당 단위(E72·E73 의 낙하 포함)에서 한다.

## 4. 다음 시작점

`RawCarrierSteal`·`RawCarrierDrop`(Carrier 계열), `RawGunGroundTarget`·`RawGunProcess`(+`MakeGunTargetHooks`), Missile 계열, `RawTempleDraw`/캡처 계열(자산 시험 장치), `RawCarrierArrival`, `RawAssetPostPop`, 사제 배치 계열(선행 `RawCanonPixelShape`·`RawCanonPlacement`), 후보/지형 권한(`RawCanonPlacementPermission`), `RawSquidTransmission`, `RawDudeSpawn`·`RawDudeNotice`→`RawDudeDrop`, `RawManaBoltArrival`(시험 장면 선행), `RawIslandRuntime` 계열, 그 밖의 작은 몸체이다.
