# webpj 공통 Pop(객체 등록) 이식 — WB04-E1

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 첫 부분이다. cpppj 기준선 `1f00a25a`의 `o/SquidPop`·`o/RawSquidPopParent`·`o/RawSquidActivate`·`o/PopVtables.inc`와 `o/RawBuildingSpace`의 Pop 쪽(옆면 쓰기)을 옮겼다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다(Node 단위 검사만).

## 1. 옮긴 것

| 웹 모듈 | cpppj 근거 | 내용 |
| --- | --- | --- |
| `www/core/squid-pop.js` | `o/SquidPop.cpp/.h`, `o/PopVtables.inc` | `SquidPop`(`pop`·`popCurrent`·`supports`·`bindDiagnostics`·`bindTransmission`·`setBuildingSpace`·`setMetadisplayDirty`/`releaseMetadisplayDirty`·`validateFrameBinding`), 10.78 가상 표 26칸 `PATCH_POP_VTABLES`. `pop`은 인자 TYPE 레코드와 프레임 치수를 쓰는 독립 입력, `popCurrent`는 빌린 전체 현재 TYPE 표와 표시 장치를 소비 위치마다 조회하는 실제 World용이다 |
| `www/core/raw-squid-pop-parent.js` | `o/RawSquidPopParent.cpp/.h` | 004b02d0 공통 Pop 부모 `runSquidPopParent`, `isCurrentPopPointValid`, `RAW_POP_RESULT`(변경 없음 0·등록 1·겹침 2) |
| `www/core/raw-squid-activate.js` | `o/RawSquidActivate.cpp/.h` | 004ac560 공통 Activate `runSquidActivate` |
| `www/core/squid-space.js` (보완) | `o/RawBuildingSpace.cpp` | 건물 Pop 옆면 쓰기 `popSide`와 `hasIsland`(WB04-D1의 `unpopSide`와 짝) |

원문 순서: 앞단(무효/비 void/contained → 변경 없음) → 좌표 검사(현재 월드 한 변, 틀리면 (10,10) 보정, metamap이 꺼져 있으면 수준29 로그) → x/y와 화면 좌표 쓰기 → 섬/다리 미니맵 통지 → 건물군 genus 또는 표면이면 발자국 y/x 순서로 spot OR(겹치면 `Overlap`으로 조기 반환하되 앞선 칸·좌표는 되돌리지 않음)·옆면·grid 교체 진단/로그 → 해시 단계 선택(SHP 프레임 크기) → 버킷 머리 삽입(자기 머리 진단) → 섬 번호 → 표시 갱신 → 첫 Pop이면 firstPop → 현재 가상 Activate → 마지막 contained 진단. 진단은 정상 복귀하면 이어서 계속하고 복귀 뒤 현재 TYPE 표·좌표·checking을 다시 읽는다.

수명 없는 `RawCheckingFlag`(비소유 뷰)는 웹에서 현재 DWORD를 돌려주는 getter 함수 계약으로 대신한다(`squid-unpop.js` 등 기존 모듈과 같음).

## 2. 검증

원본 x86 기대값은 `cpppj/tests/fixtures`를 직접 읽는다. 10.78 블록/행만 사용하고 CD 10.72 블록은 범위 밖이다.

| 검사(`tests/squid-pop.test.mjs`) | fixture | cpppj 대응 |
| --- | --- | --- |
| raw 재생 | `pop-x86` 10.78 4블록 **2456단계**: 단계마다 반환값·풀/머리/spot Adler-32·대상 슬롯 50바이트 전체 | SquidPop_X86Fixture_ReplaysActualRawRegistrationLifecycle |
| 가상 표 | `pop-vtables` 10.78 83행(+0x2000 표시 경로) | SquidPop_Vtables_RejectsUnrestoredPostPopOverrides |
| 월드 경계 | `pop-world-bounds-x86` 260행: 반환·getter 읽기 횟수·(10,10) 보정 비트 | PopWorldBounds_Native260… |
| 계약 | 독립 입력 | 가드의 부분 쓰기, getter 자기 해제·x 단락, 앞단 반환, 좌표 로그, 미니맵 뒤 TYPE, 시작점 0x6b7, grid 0x6e8, 옆면 장치 부재, SHP 실패, 겹침, 자기 머리 0x123, firstPop, 마지막 0x71a, 전송 직후 0x35f, Activate 순서 |

최초 실패와 교정: 구현 직후 가장 큰 재생(2456단계)과 260행이 처음부터 통과했다. 처음에 실패한 것은 검사 쪽 기대(단계 수 추정 2300 → 실제 2456, 가상 표 행 수 151 → 10.78만 83, Activate 검사 헬퍼의 깊이 감소 방향)였다.

`%5.1f` 서식은 C의 정확한 반올림(동률은 짝수)을 따른다(JS `toFixed`는 동률에서 올림이라 정확한 십진 전개로 직접 처리). NaN/무한대는 소문자 `nan`/`inf`로 쓴다(호스트 `snprintf`와 같음; 원본 CRT의 `1.#QNAN` 같은 표기는 재현하지 않으며 이 로그는 수준29 디버그 기록이다).

## 3. 3.6절 판정과 경계

대응 행 6개는 모두 기준선 뒤 cpppj 변경이 없다(base `1f00a25a`).

- 표시(`SquidDisplay`)와 공통 postPop(`SquidPostPop`·`SquidPostPopPatch`)은 선택 연결 계약(`display.frameSize/update`, `postPop.activationDepth/postPopVirtual/validateSpace`)만 두고 본체는 WB04-E의 다음 단위다. 연결하지 않으면 표시 갱신은 억제하되 현재 VT의 지원 여부는 확인하고, 공통 깊이는 호출마다의 지역 장부를 쓴다(cpppj 독립 구성과 같음).
- 실제 World(`RawSurfaceWorld`)·Client 접합 장면(`SquidPopParent_ActualWorld…`, `…ActualClientWorldDiagnostic…`)은 WB05에서 옮긴다.
- 다리·섬 받침 전용 postPop 접두(`HandlesDerived`)와 CD 판 Pop은 범위 밖이다.

## 4. 다음 시작점

WB04-E의 다음 단위: `SquidPostPop`(공통 비용/목록/통계 장부, 449+226줄)과 종류별 PostPop(`RawAssetPostPop`·`RawFencePostPop`·`RawNuggetPostPop`·…), `SquidDisplay`/`RawSquidDisplayParent`, `SquidFrame`, 소유자/지역(`SquidOwner`·`RawSquidRegion`), 레벨/HP(`RawSquidLevel`·`RawSquidHitPoints`).
