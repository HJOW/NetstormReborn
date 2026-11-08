# webpj 썬 바리케이드 표식·포스 필드 생성과 기존 표식 삭제(RawFenceCreate·RawFenceMarkerClear) 이식 — WB04-E83

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 여든세 번째 부분이다. 썬 바리케이드(Fence)의 내부 표식 생성(`o/RawFenceCreate`의 `RawFenceMarkerCreate`, 00456460)과 화면의 포스 필드 생성(`RawFenceShieldCreate`, 00455b20 mode 1), 연결된 기존 표식 삭제(`o/RawFenceMarkerClear`, 004566d0)를 cpppj 기준선 `1f00a25a` 에서 옮겼다. E82 의 `raw-gun-target.js`(끝점 중심 계산)와 E81 의 `raw-fence-rebuild.js`(재생성 제어)가 선행이다. 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이라 CD 10.72 분기는 옮기지 않았다.

## 1. 옮긴 것

`www/core/raw-fence-create.js` — `RawFenceMarkerCreate(pool, target, type, hooks)`·`RawFenceShieldCreate(pool, frames, type, hooks)` (`type` 은 각 생성 직전에 다시 읽는 getter)

- `RawFenceMarkerCreate.points(centers)`: 두 끝점 중심(float 4개)에서 원본 방향(동률이면 세로 우선, 한 걸음 ±1)·포함 사각형(부모가 보드 (0,256) 안일 때만 min/max, 아니면 clamp 한 이웃 중심)·4칸(반올림 `+0.5f` 절삭 후 /4, 63 상한) 구간이 바뀔 때마다 한 걸음 이동한 float 지점을 모은다(이동 후 사각형 밖이면 종료). `create(parent, other)`: 끝점 중심을 구해 지점마다 새 fencemark 를 만들고 QA(+12 WORD=부모)를 쓴 뒤 표면 통지 → QB(이웃 번호의 high8 을 +26, low8 을 +30) → 부모의 현재 소유자 지정 → flags 0x40 Pop 순서를 지킨다.
- `RawFenceShieldCreate.points(endpoints)`: mode 0 과 같은 보정이나 정수 절삭 전 float 지점(세로/가로 각각의 `±1/±3` 보정과 `dx+dx` 선행 합)이며 100개 상한에 끝점을 한 번 포함한다. `create(endpoints, firstOwner, secondOwner, firstId, secondId)`: 수직이면 J 수평이면 K 구간의 첫 프레임을 +36 에 직접 쓰고(생성 직후 extra bit20 이면 즉시 종료), 소유자는 두 소유자가 같거나 둘째 SID(signed)가 더 작으면 첫째, 아니면 둘째이며 flags 0x40 으로 Pop 한다(표면 통지는 이 경로에서 부르지 않음).

`www/core/raw-fence-marker-clear.js` — `RawFenceMarkerClear(pool, hash, types, target, markerType, destroy)`

- `bounds(parent, other)`: 이웃 중심을 먼저 읽고 [1, 255] 로 clamp 한 corner 와 부모 중심의 min/max 로 왼쪽/위는 절삭, 오른쪽/아래는 `+0.9999f` 절삭한 정수 검색 영역(비유한 중심은 거부). `clear(parent, other)`: 그 영역을 일반 finder(flags 1)로 현재 순서대로 순회하며 현재 TYPE 이 fencemark 타입(getter)이고 QA 가 부모이거나 packed QB(`(+26 DWORD<<8)|+30`)가 부모인 표식만 가상 삭제(flags 0x40)한다. 후보를 미리 모으지 않고 삭제 효과 뒤의 finder next 와 태그를 읽으며, 삭제 소비자는 삭제에 도달했을 때만 요구한다.

## 2. 검증

- `tests/fence-create.test.mjs`(3건): `fence-create-x86.tsv` 표식(M) 106행·포스 필드(S) 306행 — 부모/이웃 전체 raw, 새 객체들의 전체 50바이트, 외부 효과 호출 순서(Create/Notify/Owner/Pop)가 같다(표식 쪽 외부 효과의 소유자/좌표 변이, 포스 필드 쪽 타입 변이·차단 번째·signed SID 소유자 선택·100개 상한 포함). 계약: 선택되지 않은 소비자(표면 통지/소유자/Pop)는 요구하지 않고 선택 실패는 앞선 새 객체의 QA/프레임 쓰기를 보존, 판본/풀/TYPE 크기 혼합 거부, 비유한 좌표 거부.
- `tests/fence-marker-clear.test.mjs`(2건): `fence-marker-clear-x86.tsv` 259행 — 이웃 위치 장면·QA/packed QB 일치 mask·삭제 효과 변이(태그 변경, 후속 표식 태그 변경, 체인 head 변경)에서 검색 영역, 삭제 요청 순서, 여섯 슬롯 전체 300바이트가 같다. 계약: 삭제 소비자는 도달했을 때만 요구하고 현재 타입이 다른 표식은 태그가 같아도 삭제하지 않는다.
- 같은 fixture 의 재생성(R) 166행은 E81 의 `raw-fence-rebuild.js` 시험이 소비한다(manifest `fence-create` 항목의 소비자가 두 시험으로 늘었다).

최초 실패와 교정: 새 모듈은 위 fixture 671행과 계약 검사를 모두 첫 실행에 통과했다.

## 3. 경계와 남은 것

- 새 객체 생성(`004af530`)·표면 통지(`004214a0`)·가상 소유자 지정(+74)·가상 Pop(+90)·가상 삭제(+10)의 실제 연결은 해당 단위에서 한다. 포스 필드 mode 0 삭제는 E74 의 `raw-fence-line.js` 이다.

## 4. 다음 시작점

`RawGunGroundTarget`·차단기(`GunBlocker`)·`RawGunProcess`(+`MakeGunTargetHooks`), `RawCarrierArrival`, `RawAssetPostPop`, 사제 배치 계열(선행 `RawCanonPixelShape`·`RawCanonPlacement`), 후보/지형 권한(`RawCanonPlacementPermission`), `RawSquidTransmission`, `RawDudeSpawn`·`RawDudeNotice`→`RawDudeDrop`, `RawManaBoltArrival`(시험 장면 선행), `RawShapeToBuffer`(+ 캡처 계열), `RawIslandRuntime` 계열, 그 밖의 작은 몸체이다.
