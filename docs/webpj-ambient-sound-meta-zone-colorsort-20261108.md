# webpj 주변음·메타구역 기록 정리·ColorSort 시작(RawAmbientSound·RawMetaZoneCleanup·RawColorSortStartup) 이식 — WB04-E77

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 일흔일곱 번째 부분이다. 게임 정지/음향 장치와 별개로 도는 주변음 부모(`o/RawAmbientSound`, 004180f0/004180d0), 메타맵 51구역 고정 기록표 정리(`o/RawMetaZoneCleanup`, 0042b1b0/00479520)와 ColorSort 시작 부모 두 개(`o/RawColorSortStartup`, 43b430/43b300)를 cpppj 기준선 `1f00a25a` 에서 옮겼다. 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이라 CD 10.72 분기와 10.37 행은 옮기지 않았다.

## 1. 옮긴 것

`www/core/raw-ambient-sound.js` — `stepAmbientSound(globals, random, hooks)`·`resetAmbientSoundDeadline(globals, wallSeconds)`

- `globals = { deadline, kind, forced }` 는 State/Edit 와 공유하는 같은 기한·종류·강제 값의 현재 값 뷰이다(프레임 사본을 만들지 않음). 매 호출이 벽시각을 한 번 읽어 기한이 미래이거나 비순서(NaN)면 그것만 하고 돌아간다.
- 기한이 지났으면 벽시각을 **한 번 더** 읽어 `현재 + 120.0` 을 기한에 난수보다 먼저 저장하고, 강제 값이 signed 로 0 이하일 때만 게임 난수(3000)/1000 으로 종류를 새로 고른다(양수면 이전 종류를 유지; 범위 밖 종류는 아무것도 재생하지 않음). 종류 0 은 `thunderQuietDistant.wav` 를 재생한 뒤 **소리 요청이 복귀한 다음** 현재 모달을 다시 읽어 0 일 때만 섬광(0)을 요청하고, 1·2 는 난수(20000)−10000 을 좌우 값으로 `eagleScream.wav`/`whale.wav` 를 재생한다.
- `resetAmbientSoundDeadline`: 현재 벽시각 + 10.0 을 기한에 저장한다(004180d0).

`www/core/raw-meta-zone-cleanup.js` — `clearMetaZoneRecords(globals, hooks)`·`clearMetaZoneMarker(records, index, hooks)`·`META_ZONE_COUNT/STRIDE/BYTES`

- `clearMetaZoneRecords`(0042b1b0): 진입 때 첫 플래그가 비영이면 표(51×144=7344바이트)를 DWORD 단위로 0 으로 쓰고 플래그를 0 으로 써 돌아간다(자식 호출 없음). 진입 때 0 이면 마커 자식(index 0..50)을 51회 모두 부른 뒤 표를 0 으로 쓴다(callback 이 플래그를 바꿔도 진입 때 정한 분기를 유지하고 바뀐 플래그는 보존). 물리가 부족하면 그 DWORD 에서 거부하며 앞서 완료한 접두 쓰기는 남긴다.
- `clearMetaZoneMarker`(00479520): index 51 이상 거부, signed 상태 DWORD 0 이하·+76 SID DWORD 0 이면 삭제 없이 반환, 아니면 원시 DWORD SID 와 flags 0 으로 실제 가상 삭제를 부른 뒤 복귀 뒤 같은 DWORD 한 칸만 0 으로 쓴다(삭제 실패 뒤 SID 를 앞당겨 지우지 않음).

`www/core/raw-color-sort-startup.js` — `initializeColorSort(logical, globals, hooks)`·`initializeColorSortGradient(globals, hooks)`

- `initializeColorSortGradient`(43b300): 현재 gradient 가 비영이면 해제 후 0, 0x5020 할당 뒤에만 현재 count 를 읽어 색띠 생성(this, 회색 RGB double 셋, [1,0,0], count, 2.4)의 반환 주소를 대입한다(할당 0 이면 0).
- `initializeColorSort`(43b430): 로드 플래그 0 이면 진단(451) 뒤 계속, primary·search 를 각각 비영일 때 해제 후 0, 0x3800 할당→생성자(this, logical)→primary, 0x5008 할당 비영이면 검색 생성→첫 DWORD 에 vtable 0x506658 직접 쓰기→qsort(search+8, 256, 80, 0x43a1e0), 캡처한 할당 주소를 search 에 대입(정렬 뒤 전역이 바뀌어도), count 10, 색띠 부모 실행.
- 실제 생성·정렬·색띠 계산은 훅(필수 경계)이며 선택한 경계의 누락은 도달 시 거부하고 선행 효과는 되돌리지 않는다.

## 2. 검증

- `tests/ambient-sound.test.mjs`(3건): `ambient-sound-hardware-x86.tsv` 10.78 636행(재예약·반복 수·모달/강제/기한 변이 조합, 결합된 공유 바람 `raw-ambient-wind.js` 포함)의 최종 기한 비트·종류·강제 값·모달·난수 상태·시계 호출 수·호출 기록·바람 float 비트가 같고, 공유 기한/현재 시계 계약(미래 기한은 첫 조회만, 재예약은 난수보다 먼저 저장)과 선택하지 않은 장치 비요구·도달 장치 누락 거부를 확인한다.
- `tests/meta-zone-cleanup.test.mjs`(4건): `meta-zone-cleanup-x86.tsv` 38행(정상 28·명시 중단 10)의 표 7344+canary 16바이트 전체·플래그·자식 호출 번호·콜백 기록이 같고, 첫 비영 관문/미연결 자식/짧은 물리의 DWORD 경계 접두 보존, 마커 상태/SID 관문과 삭제 복귀 뒤 같은 DWORD 만 0 으로 쓰기를 확인한다.
- `tests/color-sort-startup.test.mjs`(2건): `colorsort-startup-x86.tsv` 116행(정상 100·부분 중단 16)의 완료 분류·현재 전역 5개·세 할당 prefix와 source 부분 쓰기·하위 호출 인자(색띠 60바이트 포함)가 같고, NULL 할당의 자연 분기와 선택된 누락 진단의 이전 상태 보존을 확인한다.

최초 실패와 교정: 새 모듈은 위 fixture 790행과 계약 검사를 모두 첫 실행에 통과했다.

## 3. 경계와 남은 것

- 실제 Client 쪽의 벽시각·모달·번개·음향 이름표 연결과 State/Edit 기한 소유자(`ClientLocalStateGlobals`), 마커 가상 삭제와 `RawIsSidRobust` 연결, ColorSort 생성·정렬·색띠 계산(`RawColorSortPrimary` 등)은 해당 파일 이식 때 연결한다.

## 4. 다음 시작점

`RawFenceRebuild`와 Fence 생성 부모, `RawCarrierArrival`, `RawAssetPostPop`, 사제 배치 계열(선행 `RawCanonPixelShape`·`RawCanonPlacement`), 후보/지형 권한(`RawCanonPlacementPermission`), `RawSquidTransmission`, `RawDudeSpawn`·`RawDudeNotice`→`RawDudeDrop`, `RawColorSortPrimary`, 그 밖의 작은 몸체이다.
