# webpj Missile 생성자·프레임(RawMissile) 이식 — WB04-E125

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백스물다섯 번째 부분이다. Missile.cpp 의 투사체 생성자(썬 캐논·썬더·아이스 캐논·썬 디스크 발사기·크로스보우, 10.78 반사 포함)와 프레임(이동·충돌·사거리 종료·피해/갱신·삭제)을 cpppj 기준선 `1f00a25a` 의 `o/RawMissile` 에서 옮겼다. 선행은 직전 단위의 [Missile 필드 이미지/아이스 반사](webpj-ice-ricochet-20261108.md)·[아처 애니메이션](webpj-archer-animation-20261108.md)(요청 구조)·`RawProjectileGeometry`·`SidPool`·`SquidProcess` 이다. 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이라 CD 10.72 의 0x68 필드/BYTE 프레임/CD 분기는 제외했다.

## 1. 옮긴 것

`www/core/raw-missile.js` — `MissileState`, `RawMissile(pool, types, missileTypes, state, hooks)`, `MissileProcess`, `runPathBaseRobust`

- `construct(fields, request)`: 발사 자산의 타입으로 group0(썬 디스크 71·크로스보우 140) / group1(썬 캐논 128·썬더 131·아이스 129) 또는 반사(ricochet ≠ 0, 아이스 반사 표시 130)를 고르고 요청 좌표·속도·거리를 검증한 뒤 필드를 쓴다(가상 표 0x50d630·발사/표적 SID·속도·반사·초기 사각형/이전 좌표 −10000·코너 여부·표적 중심·종점·원점·허용 오차·시작/종료 시각). 원점은 포구(훅) 또는 요청 포구에 SHP/타입 hotspot 보정을 더하고(썬 디스크 71 은 보정 생략), 종점은 반사면 표적 중심, group0 이면 코너/표적 중심, group1 이면 사정선 끝점(훅)이며 모두 [1,255] clamp 한다. 이어 표시 자산을 만들어(훅) 방향 프레임(썬 디스크는 1)·소유자·발사 소리·Pop·비행 효과를 순서대로 수행하고 표시 자산 SID 를 반환한다.
- `frame(fields, alive)`: 진단 모드면 0041be80 의 두 SID 진단을 실행하고, 비행 시각 비율로 현재 위치를 구해 Unpop/Pop 으로 옮기고 표적 사각형(코너면 코너 사각형)과 이전→현재 선분의 겹침으로 충돌을 판정한다. 클라우드 플로터 표적은 공유 난수로 회피할 수 있고(`randomFloat < avoidanceCutoff`이면 통과 표시), 충돌 효과(코너 배리어/아이스 전용 표시와 `iceCannonImpactN.wav`/일반 효과와 충돌 사운드)·아이스 반사 생성·썬더×스톤 타워 알림 큐를 수행하고, 후반 진단 뒤 이전 좌표를 저장한 다음, 충돌이거나 사거리가 끝났으면 **다시 해석한** 발사 자산의 group 에 따라 총 직접 피해(`hit`)/사거리 갱신(`refresh`) 또는 반사 피해(`damage`, DPS×0.6 float, 비행 표적 특례)를 전달하고 종료 효과 뒤 처음 저장한 표시 자산을 삭제한다. 진단이 현재 프로세스를 삭제하면(`alive()` 거짓) 남은 필드를 읽지 않는다.
- `MissileProcess(SquidProcess)`: 생성된 필드와 처리기를 묶은 Kernel 프로세스(`runFrame`·`onAttached`·`dispose`·`fields`).
- `runPathBaseRobust(hooks)`: 0041be80 의 Form/부모 SID 범위 진단(복귀하면 계속, 항상 true). 이후 `RawPathArrivalParent` 도 이 함수를 쓴다.
- `MissileState`: 진단 관문(불리언 또는 함수), 현재 시각, 캐논/디스크/발사기/크로스보우/썬더/스톤 타워/아이스/플로터 타입, 충돌 명령 토큰, 사거리 종료 여유(일반 4·썬더 10), 픽셀 배율, 회피 기준 등 호출 시점에 읽는 전역이다.

## 2. 검증

- `tests/archer-missile.test.mjs`(1건): `archermissile-x86.tsv` 의 `originals` 944행 — SunArcher 71 / sunDisc 83 / 효과 0 의 생성자·프레임 모두의 외부 효과 순서·128바이트 필드 이미지·표시 슬롯 50바이트가 같다.
- `tests/crossbow-missile.test.mjs`(3건): `crossbow-missile-x86.tsv` 703행(10.78 전용) — arrow 81 생성자(hotspot 두 단계·방향 프레임·사운드 우선순위)·프레임(보간·일반/코너 충돌·사거리 종료)·가상 포구 8바이트가 같다. 포구/hotspot/방향 수치는 이식된 `RawProjectileGeometry` 를 그대로 쓴다.
- `tests/gun-missile.test.mjs`(1건): `gunmissile-x86.tsv` 의 `originals` 148행 — 썬 캐논(group 1) 기본 투사체(표시 80·비행 효과 11)의 생성자·프레임.
- `tests/missile-checking.test.mjs`(6건): 합성 입력으로 현재 checking 관문과 Baseprocess 진단 두 위치(첫 진단은 전체 DWORD, 조회 전에 현재 SID/시각 수정 / 후반 진단은 이전 좌표 저장보다 앞서고 최종 발사자만 다시 조회 / 진단 장치 선택 전 이동·빗나감 소비 보존 / 좌표 실패 조기 종료 / 이동 중 flag 해제 / 호스트 자기 삭제 `alive`)를 확인한다.
- `originalCD` 행은 10.72(0x68 필드)라 제외했다.

최초 실패와 교정: 새 모듈은 위 fixture 1795행을 첫 실행에 통과했다(중간에 헬퍼 이름을 바꾸다 `point` 한 곳을 놓친 내 오타를 시험 첫 실행에서 잡아 고쳤다).

## 3. 경계와 남은 것

- 자산 생성·Pop·소리·비행 효과·피해·삭제·사정선·포구·hotspot·치수·방향 프레임·반사 생성은 훅 경계이다. 실제 팩토리/Host 부착(`MakeMissileFactoryHooks`/`CreateMissile`/`MakeGunMissileHooks`)은 `RawGunProcess`·Host 연결 단위로 둔다.
- 아직 이식하지 않은 Missile 관측: 썬더 캐논(`thunder-missile-x86.tsv`)·초기 아이스(`ice-missile-initial-x86.tsv`)·플로터 회피(`missile-floater-x86.tsv`) — 같은 모듈의 해당 분기이며 다음 단위에서 시험으로 연결한다. 호스트 자기 삭제 실제 Kernel 시험(`MissileChecking_ActualHost…`)은 Host 단위에서 확인한다.

## 4. 다음 시작점

Missile 잔여 관측(썬더·초기 아이스·플로터), `RawArcherTypeData`, `RawGunProcess`(+`MakeGunTargetHooks`), `RawLocalGeyser`, `RawGunImpact`, `RawWindAviaryProcess`·`RawWindFlyer`, 비행체(`RawFlyerProcess`·`RawFlyerFrame`·`RawFlyerTarget`), 번개 수명/프레임/화면, `RawPathProcess`(커널/Form 장면 선행)·`RawPathStart`·`RawPathData`·`RawPathCleanup`, `RawBalloonFrame`, `RawPacketDispatch`, 사제 배치 계열, `RawIslandRuntime` 계열, 분산 SP 정책, 그 밖의 작은 몸체이다.
