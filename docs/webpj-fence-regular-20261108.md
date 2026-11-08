# webpj 썬 바리케이드 Regular(RawFenceRegular) 이식 — WB04-E89

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 여든아홉 번째 부분이다. 썬 바리케이드(Fence)의 실제 Regular Handle(`o/RawFenceRegular`, 00457a50)을 cpppj 기준선 `1f00a25a` 에서 옮겼다. E81(재생성 제어)·E83(표식/포스 필드 생성·표식 삭제)·E88(이웃 검색)이 이 처리기가 훅으로 부르는 하위 몸체이다. 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이다.

## 1. 옮긴 것

`www/core/raw-fence-regular.js` — `RawFenceRegular(pool, frames, setter, hooks)`

- `handles(sid)`: 실제 가상 표 0x509470 인 부모만 분배한다(TYPE 이름만 같은 다른 부모는 거부). `handler(fallback)`: 다른 부모는 기존 처리기로 전달하고, 없으면 거부한다.
- `handle(sid, event, count, payload)`: 보호 비트 20(extra&0x20)이면 사건 번호보다 먼저 공통 Damageable 로 위임. **사건 0**(애니메이션): 현재 프레임의 글자 구간 크기(`pathDirection` → 글자별 `run().count`)로 `(프레임+1) % 크기`(구간 첫 번호를 더하지 않는 절대 나머지; 0 나눗셈/INT_MIN÷−1 거부)를 실제 `SquidFrame.set` 으로 지정하고, 소유자 캐시(+42 DWORD)가 현재 소유자(+34)와 다르면 이웃 정리, 하위 연결 수를 조회해 연결 캐시(+46)와 다르면 **한 번 더** 조회해 기록하고 이웃 정리한 뒤 0.1f 를 반환한다. **사건 1** 이웃 스냅샷 갱신, **사건 2** 재생성 위임 뒤 0, **사건 3**: 소유자 캐시를 갱신하지 않는 이웃 정리 뒤 정의되지 않은 x87 반환(음수 indefinite NaN `0xffc00000`)을 돌려준다. 그 밖의 사건은 Damageable 위임.
- 이웃 정리: 네 방향(0·2·4·6)을 먼저 모아 스냅샷으로 삼고(효과 뒤 다시 검색하지 않음), 사건 0 만 소유자 캐시를 쓴 뒤 각 이웃마다 기존 표식 삭제 → 삭제 효과 뒤 **현재** 좌표 네 float 로 연결선 삭제를 하고, 마지막에 사건 2(0.1f) 예약을 시도한다(할당 실패는 그 예약만 생략, 이후 연결 수 조회는 계속). 선택된 하위의 누락은 앞선 쓰기를 보존한 채 거부한다.

## 2. 검증

- `tests/fence-regular.test.mjs`(4건): `fence-regular-x86.tsv` 1372행(애니메이션 A 1305: 두 독립 변경 분기·각 할당 실패·현재 재조회 / 위임 E 18 / Damageable D 49)과 `fence-event3-x86.tsv` 301행(실제 원본 호출자까지 실행한 사건 3)의 부모+네 이웃 전체 250바이트·반환 float 비트(NaN 포함)·하위 호출 순서가 같다. 계약: 연결 수 훅 누락은 프레임 쓰기를 보존한 채 거부, 예약 훅 누락에서도 사건 3 의 소유자 캐시는 변하지 않음, 보호 bit20 은 Damageable, 다른 가상 표는 fallback 없이 거부/있으면 위임, 판본/풀 혼합 거부.

최초 실패와 교정: 새 모듈은 위 fixture 1673행과 계약 검사를 모두 첫 실행에 통과했다.

## 3. 경계와 남은 것

- cpppj 의 `FenceEvent3_ActualHostRetainsNanAndRunsNewRebuild`(실제 `SquidProcessHost` 에 처리기를 연결해 사건 3 의 NaN 재예약·새 사건 2 수명·count 감김을 확인)는 Host 재예약 합성 시험이라 이번 모듈 범위 밖이다(실제 Host 의 NaN 시각/payload 보존은 `squid-process.js` 단위가 소유). 하위 몸체(이웃 `raw-fence-neighbors.js`·표식 삭제 `raw-fence-marker-clear.js`·연결선 삭제 `raw-fence-line.js`·재생성 `raw-fence-rebuild.js`·이웃 스냅샷 `raw-fence-lifetime.js`)와의 실제 Session 조립은 해당 단위에서 한다.

## 4. 다음 시작점

`RawGunGroundTarget`·`RawGunProcess`(+`MakeGunTargetHooks`), Missile 계열, `RawTempleDraw`/캡처 계열(자산 시험 장치), `RawCarrierArrival`, `RawAssetPostPop`, 사제 배치 계열(선행 `RawCanonPixelShape`·`RawCanonPlacement`), 후보/지형 권한(`RawCanonPlacementPermission`), `RawSquidTransmission`, `RawDudeSpawn`·`RawDudeNotice`→`RawDudeDrop`, `RawManaBoltArrival`(시험 장면 선행), `RawIslandRuntime` 계열, 그 밖의 작은 몸체이다.
