# webpj 경로 런타임(RawPathRuntime) 이식 — WB04-E98

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 아흔여덟 번째 부분이다. 경로(Path) 프로세스의 실제 저장 이미지(100080바이트)·생성자·시각 계산(Prepare)·한 꼭짓점씩 진행하는 Step·비행 Init 과 필드 접근자를 cpppj 기준선 `1f00a25a` 의 `o/RawPathRuntime` 에서 옮겼다. 운반물 이동/표적/선택/탈출(`RawCarrierMove` 등)·벌룬·보행·Dude 가 모두 이 이미지를 읽고 쓰므로 그 선행 단위이다. 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이라 CD(10.72)의 배열 600·float 중간 저장 차이·관찰 시각은 옮기지 않았다.

## 1. 옮긴 것

`www/core/raw-path-runtime.js` — `PATH_LAYOUT`, `RawPathRuntime({ priest, heapFill, flying, worm, balloon, dude, bulf, walker })`

- 이미지 배치(10.78): +0 vtable · +4/+8 Form/부모 · +0x10 꼭짓점 개수 · +0x14 꼭짓점 좌표 float 쌍(8바이트 간격, 최대 5000) · +0x9c58 도착 시각 double 배열 · +0x13898 방향 DWORD 배열 · +0x186b8 다음 프레임 예약 시각 QWORD · +0x186c0 현재 단계 · +0x186c4 각도 float · +0x186c8 완료 · +0x186cc/+0x186d0 도착 대상 두 개 · +0x186d4 helper(+4 가 이동 회피 깊이) · +0x186dc/+0x186e0 패치 전용 목적지 float · +0x186e4/+0x186e8 명령 꼬리 · +0x186ec 사망 감시.
- 생성자: 실제 생성자처럼 vtable(기본 텔레포트 0x50e284, 사제 0x50e458, 파생 종류는 fly39 0x50e2ec·worm40 0x50e4c4·balloon42 0x50e2b8·dude24 0x50e3d8·bulf41 0x50e354·walker38 0x50e320)과 개수·시각 배열 전체·지정 필드만 0 으로 만들고 꼭짓점/방향 배열은 `heapFill` 로 남긴다(미초기화 heap 보존). 파생 종류는 사제와 서로 배타적이다.
- `setPath(points, directions, times)`: 경로 탐색/수신부가 만든 꼭짓점·방향·시각을 실제 append 와 같은 인덱스로 쓴다(직선 경로를 추측해 만들지 않음). 크기 불일치/용량 초과는 거부한다.
- `prepare(now, startTime, speed, turningSpeed, firstDirection)`(00488e20): 늦은 시작(2초 초과)이면 `startTime`, 아니면 `now` 를 첫 시각으로 쓰고 첫 방향을 쓴 뒤 꼭짓점마다 `회전 역수(float 저장)×회전량(float 저장) + 거리×속도 역수(float 저장) + 이전 시각`을 double 로 누적해 저장한다. 빈 경로·속도/회전 속도 0 은 거부한다.
- `step(now, parent, observation, stepSound)`(0048a330): 한 호출에 최대 한 꼭짓점만 넘는다. 끝을 지나면 마지막 꼭짓점·방향 각도·완료를 돌려주고, 도착 시각 전이면 이전/목표 꼭짓점 사이를 보간(비율·각도는 float 저장, 각도는 [0, 360) 보정)하며 보간 각도를 이미지에 쓴다. 도착 시각 이후면 목표 꼭짓점·방향 각도를 돌려주고 현재 단계를 1 올리며, 완료가 아니고 새 단계가 짝수이면 발소리 효과를 요청한다. 관찰 플래그 bit0 을 켜고 관찰 시각을 0 으로 쓴다.
- `initializeFlight(parent, destination, cruiseHeight, currentHeight)`(48af00/488d00): 생성자의 기존 개수를 초기화하지 않고 부모 → (고도가 모자라면) 이륙 → (순항 고도가 있으면) 수평 → 도착 순으로 꼭짓점을 추가한다(고도→칸 배율은 단정도 0.0909…, 마지막 방향은 `RawSquidNeighbors.direction`)한 뒤 현재 단계를 0 으로 만든다.
- 필드 접근: `count/current/angle/finished/targetA/targetB/helper/moveAwayDepth/deathWatchToken/nextFrame`, `direction/time/point(index)`, `physicalTime(-1..)`, `restoreStep/resetCurrent/setFinished/setTargets/setCommandFields/replacePoint/clearHelper/setMoveAwayDepth/setDeathWatchToken/setAttachment/setNextFrame` 는 원본 저장 폭만 쓰고 옆의 값을 보존하며, 범위 밖 읽기/쓰기는 호스트 메모리 접근 대신 오류로 진단한다. `RawPathRuntime.directionDelta` 는 4방향 차이에서 양의 180도를 고르는 최단 signed 회전이다.

## 2. 검증

- `tests/path-runtime.test.mjs`(4건):
  1. `path-constructor-x86.tsv` 32행(PATCH 종류 8종 × heapFill 0/A5 × 잔여 caller 인자 2종의 100080바이트 전체 이미지 반복 BYTE 압축)·`pathruntime-x86.tsv` 의 `originals` ctor 4행·`flypath-x86.tsv` ctor 3행의 FNV32 가 같다.
  2. `pathruntime-x86.tsv` 의 `originals` prepare 24행(꼭짓점 시각 QWORD 4개)과 step 80행(profile 별 연쇄 Step: 반환 위치/각도 비트·완료·발소리 수·현재 단계·각도 필드)이 같다.
  3. `flypath-x86.tsv` init 24행(기존 개수 0/1/2 × 도착 좌표 8종의 비행 꼭짓점 추가 뒤 전체 이미지 FNV32)이 같다.
  4. 계약: 거부해야 하는 입력(빈 경로 Prepare·속도 0·빈 경로 Step·크기 불일치), 방향 차이의 최단 회전, 필드 접근자의 위치/폭, 종류 배타, 상태 이관 범위, 물리 시각 범위.

최초 실패와 교정: 새 모듈은 위 fixture 167행(+계약)을 모두 첫 실행에 통과했다.

## 3. 경계와 남은 것

- `pathruntime-x86.tsv` 의 arrival 12행·frame 60행과 `flypath-x86.tsv` 의 frame 63행은 `RawPathProcess`(커널/Form 위의 RunFrame·도착) 단위에서 대조한다. 보행 모양 1170행(`walk-shape-x86.tsv`)도 마찬가지이다. `originalCD` 행은 10.72 라 범위 밖이다.
- 경로 시작(`RawPathStart`)·수신 복원(`RawPathData`)·탐색(`RawPathSearch` 의 BFS)·정리(`RawPathCleanup`)·재목표(`RawPathRetarget`)와 운반물 이동(`RawCarrierMove`) 등은 이 이미지를 쓰는 후속 단위이다.

## 4. 다음 시작점

`RawPathProcess`(RunFrame·도착; 커널/Form 장면 선행)·`RawPathStart`·`RawPathData`·`RawPathCleanup`·`RawPathRetarget`, 그 위에 `RawCarrierMove`/`Target`/`Choice`/`Escape`(운반물 이동/표적/선택/탈출), `RawBalloonFrame`, `RawGunGroundTarget`·`RawGunProcess`(+`MakeGunTargetHooks`), Missile 계열, `RawTempleDraw`/캡처 계열(자산 시험 장치), `RawCarrierArrival`, `RawAssetPostPop`, 사제 배치 계열(선행 `RawCanonPixelShape`·`RawCanonPlacement`), 후보/지형 권한(`RawCanonPlacementPermission`), `RawSquidTransmission`, `RawDudeSpawn`·`RawDudeNotice`→`RawDudeDrop`, `RawManaBoltArrival`(시험 장면 선행), `RawIslandRuntime` 계열, 그 밖의 작은 몸체이다.
