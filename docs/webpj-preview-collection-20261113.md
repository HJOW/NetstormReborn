# webpj PreviewCollection — 커서 미리보기 수집기 (WB04-E386, 2026-11-13, 5-A 열다섯 번째 단위)

5-A 타입별 Draw·미리보기 중 client **PreviewCollection**(cpppj `client/PreviewCollection.cpp/.h`, 기준선 214줄)을 `render/preview-collection.js` 로 옮겼다. 기준선 뒤 cpppj 변경이 없다(base `1f00a25a`). 원본 커서 미리보기 수집기(10.78 `00446060`)이며 공통 표시 목록(E382 `render/draw-submission.js`)의 마지막 수집기(kind 4)다. 커서가 들고 있는 조각마다 본체(tail 0)와 보조 레이어(tail 2, 깊이 −1)를 쓴다. **독립 상태 경로**(`PreviewCollectionState` + 훅)를 옮겼고, 실제 커서 전역(`CursorPreviewSource`)에 묶는 `bindCursor` 와 그 경로의 재생 시험 `PreviewCollection_PatchNativeSharedCursor` 는 커서 전역(`ClientCursorGlobals`·`CursorDropState` 등, K 영역) 단위가 옮겨진 뒤 잇는다 — 그래서 PORT_MAP 상태는 `partial` 이다.

## 1. 규칙

- **`collect(view, output, limit)`:** 원본은 `view` 를 읽지 않는다. 잠금 깊이가 0 이 아니면 아무것도 쓰지 않는다. 조각(`pieces[i]`, `i < pieceCount` — 매 반복에서 현재 개수·목록을 다시 읽음)마다 `limit < 1` 이면 멈춘다(그래서 한도 1 에서도 한 쌍을 쓴다). 조각 SID 의 raw 위치(+14/+18)와 토큰(`tokenBase + SID × 50`)을 현재 칸에 **먼저** 쓴다 — 건너뛴 조각도 x/y/token/depth 14바이트는 남고 kind/tail 과 반환 수는 그대로다.
  - **깊이 우선순위:** 선택 타입의 zOrder → 부모 Gump 가 있으면 `gumpDepth(부모) − 9` → TYPE flags2 표면 비트(4)면 `0xffffffff` → 섬 비트(`0x400000`)면 −20 (뒤 규칙이 앞을 덮음).
  - **받침(specialTypeA/B) 조각:** 배치 불가(`legal` 0)면 건너뛴다. 가능하면 배치 칸 spot(`x + (y << 8)`; 좌표는 +0.9999 float 상수(`0x3f7ff972`)를 x87 로 더한 뒤 CRT 64비트 절삭의 하위 DWORD, 비유한·범위 밖은 0)의 비트 2 가 서 있을 때 건너뛰며, 아니면 본체 깊이를 20 으로 둔다. spot 주소가 65536 이상이면 거부한다.
  - **쌍:** 칸이 모자라면 거부한다(`count + 2 > 칸 수`). 쌍마다 패치판 표시 피드백(`patchFeedback`)을 실행한 뒤 본체 kind 4/tail 0, 보조(같은 위치·**현재 SID** 토큰·`깊이 − 1`(spot 에서 20 으로 바뀌기 전의 깊이)·kind 4·tail 2)를 쓰고 `count += 2`, `limit -= 2`.
- **`patchFeedback(owner)`:** 분산 SP offset 과 복사 offset 이 다르면 강조 시각(`highlight`)을 현재 시각으로 쓴다. 카운트다운이 있으면 SP 조회(`querySp`)가 켜져 있을 때 소유자 SP float(비트 해석)이 7000 보다 클 때(순서 비교, NaN 제외) 강조하고 카운트다운을 1 줄인다. 첫 SP 풀이 없거나 첫 순열 WORD 가 −1(부호 확장)이면 강조 시각을 현재 시각으로 쓴다. 각 값은 훅이 있으면 매번 현재 값을, 없으면 state 의 독립 값을 읽는다.
- **`object(token)`:** 대리 토큰을 같은 풀의 SID 로 되돌린다(기준 미만·0·stride 배수 아님·용량 밖은 거부). **`makePreviewSpHooks(store, hooks)`:** 초기화를 끝낸 분산 SP 저장소의 읽기를 `readSp` 로 연결한 훅 사본. **`bindPreviewCollection(hooks, collector)`:** 공통 수집기 묶음의 `preview` 를 이 수집기로 위임. **`cursorPreviewSid(number)`:** 커서 원문 DWORD → 호스트 SID(65535 초과 거부, `RawCursorLifecycle` 단위가 같은 함수를 쓴다).

## 2. 웹 범위

- 앱 수명에 연결하지 않았다(`PORT_MAP` session 0). 10.78 전용(CD `0043da30` 경로·판본별 활성 판정은 옮기지 않음). 항목은 16바이트 `Uint8Array`, state 는 원본 전역의 현재 값(`PreviewCollectionState`)이다.
- 남은 것(partial): `bindCursor`(`CursorPreviewSource` 로 풀·잠금 깊이·선택 타입·부모 Gump·받침 타입·배치 가능·조각 목록 30칸·배치 좌표·로컬 소유자를 복사 없이 읽음)와 `PatchNativeSharedCursor` 재생(같은 520행을 실제 커서 참조에 공급하고 중복 입력은 서로 다르게 둠).

## 3. 검사 (`tests/preview-collection.test.mjs` 7건)

- **원본 독립 관측(10.78):** `previewcollection-x86.tsv` originals **520행**(실제 미리보기 기계어 — 상태 21값·한도·클립 전 영역·조각 목록·희소 spot·완성 raw 슬롯 50바이트 전체)에서 반환 수·작업 칸 64개 전체 바이트·강조 시각·카운트다운·SP getter 진입 횟수·작업 배열 앞뒤 0xa5 보호 항목이 **첫 실행에서 전부 일치**했고, 풀과 spot 은 수집 전후 바이트가 같으며 두 레이어의 토큰은 `object()` 가 같은 풀 SID 로 되돌렸다.
- cpppj `PreviewCollection_PairLimitAndPartialWrites` 이식(soft 쌍 제한·실패 후보의 14바이트 부분 기록·실제 span 넘침 방지).
- 웹 계약(변형 확인에서 발견한 공백을 독립 입력으로 보강): 상태 기본값, 생성 검증과 토큰 범위 경계(정확히 2^32), `object` 거부 문구, 훅이 state 값보다 우선, SP 읽기는 조회가 켜져 있을 때만·7000 이하/NaN 은 강조 없음·훅 누락 오류, 깊이 우선순위 전 조합, 표면/섬 비트 마스크(낮은 비트만 있으면 zOrder 그대로), 받침 spot 좌표(float 상수·비유한→0 칸·65536 이상 거부), 첫 순열 WORD 부호 확장(0xffff·0x1ffff 는 −1), 훅이 쌍 제출 도중 목록을 줄이거나 SID 를 바꾸는 경우의 재조회(본체는 처음 SID·보조는 현재 SID 토큰), SID 변환·목록 범위·공통 묶음 연결의 한도 전달.

## 4. 변형 확인과 남은 경계

- 자동 연산자·상수 변형 137개와 손으로 쓴 구조 변형 43개를 적용했다(1차 생존 47건 → 위 보강 시험으로 대부분 해소, 결과는 LEFT_JOBS 0.1 의 한 줄 요약).
- 남은 동등 변형: ① spot 좌표 float 상수 `0x3f7ff972` 의 1 ulp·`value` 범위 검사(`±2^63`)·`asUintN` 폭(32→16/33) — 2^56 이상 float 은 하위 32비트가 모두 0 이라 CRT 불확정 0 과 같고, 관측 입력의 좌표는 정수 경계에서 1 ulp 이상 떨어져 있음, ② `DataView(ArrayBuffer(4))` 크기, ③ 낮은 비트가 `>>> 0` 으로 걸러지는 SP float 비트 해석.
- 다음 5-A 항목: 타입별 Draw·미리보기의 남은 `SpriteDraw`(partial 470)·`PreviewDraw`(365, 이 수집기의 항목을 그리는 쪽)·`RawCursorPreviews`(185), 입자 본체(`RawCampaignParticles`·`ParticlePaletteMemory`), 렌더러 프레임 본체의 `Renderer`·`ClientRendererFrame`·`ScreenPresentation`·`Screen`.
