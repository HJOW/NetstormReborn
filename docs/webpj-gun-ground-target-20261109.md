# webpj 별도 지상 표적 검색(RawGunGroundTarget) 이식 — WB04-E211

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절 "총·피해·삭제 수명" 묶음의 일부다. AI 의 group 3 배치가 쓰는 별도 지상 표적 검색(0046b540 Find·0046b360 MayTarget·Depend 조회)을 E82 Gun 표적(`raw-gun-target.js`)과 같은 해시·풀 위에 옮겼다.

- cpppj 기준선 `1f00a25a`. 소스·시험 모두 기준선 뒤 변경이 없다(`git diff --numstat 1f00a25a HEAD` 0줄).
- 근거 커밋: 소스·시험 `39116d17`(헤더 `8b467ca0`).
- 원본 fixture: 없다. cpppj `GunGroundTargetTests` 10건은 손으로 쓴 계약 시험이다. 같은 PC 의 기존 cpppj 통합 시험 실행 파일(`cpppj/build/bin/Release/netstorm_tests.exe --filter GunGroundTarget`, 2026-11-08 빌드, 게임 실행 아님)에서 10건 통과를 확인했다.
- 공유 장치
  - E82 `raw-gun-target.js`(`mayTarget`·`footprint`, 공통 HP·서버 번호·동맹·사제 수량 필터)
  - WB04-B `raw-squid-finder.js`·`squid-hash.js`
  - E39 `raw-squid-neighbors.js`(`direction`, 4방향)
  - D4 `squid-factory.js`(시험 장면의 `take`)
- 새 공유 장치는 만들지 않았다. 원본 `RawCheckingFlag` 와 전역 참조 묶음은 `GunGroundTargetGlobals` 객체(필드를 조회 위치마다 다시 읽음)로 옮겼다.

## 1. 옮긴 것

`www/core/raw-gun-ground-target.js`(DOM 없음, 10.78 전용)

- `GunGroundTargetGlobals`
  - excludedType(72, 보행 운반체 검색 제외 TYPE)·dependType(1)
  - checking(전체 DWORD, sharedChecking getter)·metamap·boss
  - assertion(식, 줄): 원본 진단 장치다. 반환하면 원래 순서로 계속한다.
- `RawGunGroundTarget(pool, hash, types, targetTypes, targeting, state, globals)`: 같은 PATCH 풀의 RawGunTarget·188칸 표·전역 객체를 확인한다.
  - `find(x, y, type, owner)` — 0046b540
    1. 문맥을 만든다. 방향/프레임은 0, 중심 = 좌표 − 발자국 절반(float)이다.
    2. 중심을 칸으로 바꾼다. +0.99999f 뒤 절삭 → float 저장 → 다시 절삭한다.
    3. 그 칸 ± trunc(사거리) 사각형으로 실제 일반 finder(flags 5, 확장 높이 표)를 시작한다.
    4. 첫 후보를 준비한 뒤 자기 동맹을 검사한다. friendlyFire 가 꺼졌고 자기 비트가 없으면 원본 진단 0x2fc 다.
    5. 후보마다 HP 보유(flags1 0x10)와 `mayTarget` 을 통과한 것만 본다. 확장 발자국에서 반대 방향 경계 중점까지의 double 제곱거리가 float(사거리²)보다 엄격히 작을 때 고른다.
    6. 같은 거리/NaN 은 먼저 고른 것을 유지한다. ground 가산과 사정선 그림은 없다.
  - `mayTarget(query, target)` — 0046b360
    1. 비행/벌룬(flags2 0x120000)은 거부한다.
    2. 보행 운반체(flags2 0x10000)는 문맥 TYPE 이 제외 TYPE 이면 거부한다. 아니면 Depend 체인에 현재 dependType 인 자산 TYPE(<70) Form 이 있을 때 거부한다. 제외 TYPE 판정이 Depend 조회보다 먼저다.
    3. 그 밖은 기존 `RawGunTarget.mayTarget` 이다.
  - Depend 조회(004b22d0/004b1b80/004b1bd0)
    1. checking·비 metamap·dead 부모·비 boss 면 원본 진단 0x158 을 부른다. 그 뒤 현재 head(+6)를 다시 읽는다.
    2. next(+4)를 먼저 저장한 뒤 TYPE<70 이고 dependType 인 첫 Form 을 찾는다. free/dead 자식도 원문대로 통과한다.
    3. 일치 없이 순환하거나 물리 범위 밖이면 명시 오류다.

## 2. 검증

`tests/gun-ground-target.test.mjs`(12건)

- cpppj `GunGroundTargetTests` 10건을 같은 장면(GroundScene)으로 옮겼다.
  - 비서버 SidPool(32768)과 SID 5/6 실제 할당, 서버 대역 15000..15002 의 `SquidFactory.take(80)`, 1단계 해시 버킷 체인, 기존 RawGunTarget 공통 필터를 쓴다.
  - 시험 이름: AirborneAndCommonTargetFilters·WalkerUsesActualDependChainAndCurrentType·ExcludedSourcePrecedesDependencyLookup·DependencyDiagnosticAndCurrentHead·DependencyFirstMatchAndCycleBoundary·ActualHashStrictSquaredRange·ActualHashKeepsFirstEqualCandidate·CurrentAllianceAndDiagnosticContinuation·SnapBiasAndAsymmetricSourceFootprint·NoRangeDrawAndRejectedWalkerDoesNotWin
  - 사정선 그림 hook 은 불리면 실패하게 했다.
- 웹 계약 2건
  - 칸 보정 0.99999f 경계: 중심 소수부 0.00005 면 51칸이라 잡히지 않는다. 일반 ceil 보정 0.9999f 였다면 50칸이라 잡힌다.
  - 판본·다른 풀의 공통 필터·TYPE 표 크기·전역 형식 거부, 진단 장치 누락 오류, 범위 밖 TYPE/NaN 좌표 거부.

최초 실패와 교정: 모듈은 처음부터 cpppj 10건을 통과했다. 비교가 실제로 작동하는지 보려고 사본을 다음과 같이 바꿔 돌렸다. 확인 뒤 모두 원본으로 되돌렸다(cmp 일치).

- 실패한 변형: 등거리 선택 `<`→`<=`, 진단 뒤 head 재조회 생략, 비행/벌룬 마스크 0x120000→0x100000, 제외 TYPE 와 Depend 조회 순서 교환, 사거리² 상한 완화.
- 통과한 변형: 칸 보정 0.99999→0.9999. cpppj 시험은 단순 ceil 과의 차이만 확인해 구별하지 못한다. 그래서 위 0.99999f 경계 계약을 추가했다.
  - 첫 작성 때 대조점 50.50001 을 "50칸"으로 잘못 계산해 원본 모듈에서 그 검사가 실패했다. 중심 소수부가 0.0000114 라 실제로는 51칸이다. 대조점을 50.5(중심 소수부 0)로 고쳤다.
  - 고친 뒤 원본은 통과하고 0.9999 변형은 실패하는 것을 확인했다.

## 3. 경계와 남은 것

- 실제 호출자 연결은 하지 않았다. AI group 3 배치(E173~E175 AI 배치 평가/실행)의 지상 표적 조회와 World 장기 전역(00541178·00541074·005e4794·00594fc8·00540bc4) 연결은 순서4~5 몫이다(PORT_MAP session 0).
- 원본 x86 관측이 없는 단위다. 원본과의 대조는 cpppj 계약 범위에 머문다.

## 4. 다음 시작점

WB04-E212 는 3.1절에서 고른다. 같은 묶음에서 기준선 뒤 변경이 없는 RawForcefieldLifecycle(`ForcefieldLifecycleTests`)을 먼저 권한다. RawDamageablePreDestroy·RawDamageablePostDestroy·RawDamageableRelease·RawGunImpact·RawDiscRegular 와 "비행·이동 유닛" 묶음의 선행 RawFlyerProcess 는 기준선 뒤 변경이 있어 옮길 때 3.6절 판정을 함께 한다.
