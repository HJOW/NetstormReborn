# webpj Gun 표적(RawGunTarget) 이식 — WB04-E82

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 여든두 번째 부분이다. GunProcess(캐논/아처)의 실제 표적 검색과 MayShoot 계열(`o/RawGunTarget`, 0046b6f0·0046af80·0046ac00·0046b070·00469190)을 cpppj 기준선 `1f00a25a` 에서 옮겼다. 썬 바리케이드 생성(`RawFenceCreate`)·표식 삭제(`RawFenceMarkerClear`)·지면 표적·차단기 등이 쓰는 중심/발자국 계산의 공통 선행 단위이다. 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이라 CD 10.72 분기(수량 폭 2바이트, 거리 float 저장 위치 차이 등)와 10.37 행은 옮기지 않았다.

## 1. 옮긴 것

`www/core/raw-gun-target.js` — `RawGunTarget(pool, hash, types, targetTypes, shapes, state, hooks)`

- `findTarget(x, y, type, direction, owner)`: 현재 타입 발자국 중심(`x − footX/2`)과 공격 모드 프레임으로 문맥을 만들어 group 0(Archer)이면 center±trunc(range) 범위의 일반 `RawSquidFinder`(flags 5: 표면 건너뜀+타입별 확장 높이)로, group 1(Cannon)이면 0·2·4·6 방향 사정선마다 새 finder 로 검색한다. 같은 점수는 첫 후보를 유지하고 ground 후보에는 +1000, 후보 자기 동맹이 없으면 원본 assert, 그 밖의 group 도 원본 assert 이다. 진단 출력(`showRanges`)이 켜져 있으면 최종 표적 발자국을 `drawRange(rect, true)` 로 낸다.
- `mayTarget`(PATCH 는 SID 0/TYPE 0 도 읽고 HP 보유에서 거부): Damageable(Dais 는 extra bit0, 일반은 flags1 bit4)·서버 예약 번호 5..14999·소유자 범위·동맹 마스크(`friendlyFire` 가 꺼졌을 때, 현재 Player+54 조회기 가능)·extra&0x11·사제 수량(≤1)·동일 위치 뒤 `mayShoot`. `mayShoot`: group 0 사거리(+윈드 아처 관측각, 끝점 포함 circular 구간)·group 1 사정선과 확장 발자국 접촉·group 3 직접 허용·그 밖은 checking 켜진 flyer 가 아니면 원본 assert. `mayFire`: 썬 캐논이면 현재 프레임이 그 방향으로 쏠 수 있는지 먼저 검사(프레임 24..27 은 별도 방향).
- `cannonRay`: SHP 포구(+20/+22)와 타입 핫스폿으로 정한 사거리 사각형을 float 저장 순서(`Math.fround`)로 계산하고 보드에 clamp 하며, 폭/높이 2 미만의 얇은 ray 는 floor/ceil+0.0001/0.1 로 확장한다. 테마 1·2 캐논은 지정 방향으로만 쏘고, 플레이어 bit20 이면 진단 출력만 생략한다. `footprint(target, extended)`·`center`·`directionFrame`·`cannonDirection` 도 공개한다.
- 모든 float 연산은 원본의 float 저장 위치와 double 승격 순서(거리 float 저장, `0.9999f` ceil 보정, `57.295827908797776` 환산, 윈드 관측각 정규화는 `raw-board-angle.js` 재사용)를 따른다.

## 2. 검증

- `tests/gun-target.test.mjs`(3건): `guntarget-x86.tsv` 10.78 1727행 — 검색/MayTarget/MayShoot/MayFire/CannonRay/Footprint 여섯 종류, 원본 타입 71/128/129/140/148/83, 방향 모드·테마·사거리·후보 좌표·extra·flags·수량·소유자·동맹 허용·친구 발사·장면 종류(발자국/확장 높이/후보 순서)·진단 출력 조합에서 결과 SID/판정, 사정선·발자국 네 float 비트, 진단 출력 요청(`D:대상여부:float 비트`)이 같다. 같은 입력을 값형 동맹 배열과 현재 Player 조회기(`allyMaskRead`/`playerFlagsRead`, 잘못된 fallback 이 쓰이면 결과가 달라지도록 반대 입력을 남김) 두 방식으로 재생하고, 진단 출력 경계 누락 거부·플레이어 bit20 의 ray 진단 생략·판본/타입 크기/소유자 범위 거부를 확인한다.

최초 실패와 교정: 새 모듈은 위 fixture 1727행(두 방식 합 3454회 재생)과 계약 검사를 모두 첫 실행에 통과했다.

## 3. 경계와 남은 것

- `MakeGunTargetHooks`(GunProcess 의 `findTarget`/`shoot` 연결)는 `RawGunProcess` 가 아직 없어 옮기지 않았다. 보호 원본 SHP 자산으로 `sunCannon` 사정선 48행을 대조하는 `GunTarget_ActualCannonAssetRay48`(`cannon-asset-ray-x86.tsv`)은 SHP 포구 헤더를 읽는 자산 시험 장치가 선행이라 이번 범위 밖이다(PORT_MAP 상태 partial).

## 4. 다음 시작점

`RawFenceCreate`(표식 M 106·포스필드 S 306, 이번 모듈이 선행), `RawFenceMarkerClear`, `RawGunGroundTarget`, 차단기(`GunBlocker`), `RawGunProcess`(+`MakeGunTargetHooks`), `RawCarrierArrival`, `RawAssetPostPop`, 사제 배치 계열(선행 `RawCanonPixelShape`·`RawCanonPlacement`), 후보/지형 권한(`RawCanonPlacementPermission`), `RawSquidTransmission`, `RawDudeSpawn`·`RawDudeNotice`→`RawDudeDrop`, `RawShapeToBuffer`(+ 캡처 계열), `RawIslandRuntime` 계열, 그 밖의 작은 몸체이다.
