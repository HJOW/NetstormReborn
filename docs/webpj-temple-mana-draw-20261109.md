# webpj 신전·마나 추가 그림(RawTempleDraw·RawRainVortexDraw·RawManaDraw) 이식 — WB04-E203

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절 "그림·캡처" 묶음의 두 번째 부분이다. 세 신전(윈드·레인·썬더)의 기본 프레임과 전용 추가 그림, 마나의 그림자/본체 추가 그림을 옮기고, 각 cpppj 전용 시험의 원본 fixture 를 그대로 재생했다. 세 처리기는 그림을 직접 그리지 않고 "그림 한 장 제출"·"소리"·"VFX 요청" 소비자에 순서대로 넘긴다(픽셀 출력은 소비자 몫).

- cpppj 기준선 `1f00a25a`. 세 소스와 시험 파일은 기준선 뒤 변경 없음(`git diff --numstat 1f00a25a HEAD` 0줄). 근거 커밋:
  - `o/RawTempleDraw` `6e5a4fc8`(시험 `TempleDrawTests` 같음)
  - `o/RawRainVortexDraw` `20e055e3`(시험 `RainDrawTests` 같음)
  - `o/RawManaDraw` `ab993bdf`(시험 `ManaAdditionalTests` 같음)
- 원본 fixture(기준선 뒤 변경 없음):
  - `temple-draw-x86.tsv` 264행(10.78, SHA-256 `18ee694c…`, 근거 `6e5a4fc8`): 윈드 84·레인 116·썬더 64행
  - `rain-draw-x86.tsv` originals 116행(`d2a78813…`, `20e055e3`). originalCD 116행은 범위 밖이다.
  - `mana-additional-x86.tsv` 756행(`5a69003c…`, `ab993bdf`)
- 공유 장치: WB03-X `assets/terrain-tiles.js` 의 `FrameCodeTable`(cpppj `RiftTypeFrames`), E200 `tests/capture-assets-support.mjs`(실제 TYPE 정의의 baseFrame·프레임 코드 표).

## 1. 옮긴 것

세 모듈 모두 `www/core/`, DOM 없음이다. 슬롯은 50바이트 raw 이며 +0 가상 표·+10 TYPE·+12 WORD 상태·+36 현재 프레임(signed DWORD)을 읽는다.

- `raw-temple-draw.js`
  - `isPatchTemple(raw)`: 윈드 `0x519860`·레인 `0x5196f0`·썬더 `0x519918` 세 표만 받는다.
  - `patchTempleMainFrame(raw, baseFrames)`(4acfe0): 현재 TYPE 의 baseFrame 한 필드(−1 도 그대로)다. 현재 애니메이션이나 가상 +88 을 읽지 않는다.
  - `drawWindThunderTemple(raw, frames, baseFrame, frame)`(4dcf10 윈드 / 4dd0f0 썬더):
    1. 썬더는 현재 프레임을 늘 내고, 윈드는 현재 프레임이 baseFrame(고정 본체 13)보다 앞일 때만 낸다.
    2. WORD 상태가 있고 현재 프레임이 'C' 구간 개수보다 작으면 'C' 첫 번호 + 현재 프레임(32비트 감김)을 한 번 더 낸다.
- `raw-rain-vortex-draw.js`
  - `isRainVortex(raw)`: 10.78 레인 표 `0x5196f0` 만 받는다.
  - `drawRainVortex(raw, frames, hooks)`(004dcd00):
    1. 0번과 현재 프레임을 늘 낸다.
    2. WORD 상태가 있고 현재 프레임이 'C' 구간 개수보다 작으면, 현재 코드 방향 구간의 첫 번호와 같을 때 `ascendSound()`(templeAscend.wav)를 부른다.
    3. 그 뒤 'C' 첫 번호 + 다시 읽은 현재 프레임을 낸다(콜백 뒤 현재 raw 를 다시 읽는다).
  - cpppj 의 판본 인자는 받지 않는다(10.78 전용, CD `0x501d28`·BYTE 프레임 분기는 범위 밖).
- `raw-mana-draw.js` `drawManaAdditional(raw, x, y, hooks)`(00473710)
  1. TYPE 을 한 번 고른다.
  2. WORD 상태가 0 이 아니면 프레임 → 고른 TYPE 의 현재 SHP 순서로 읽고, 두 축 +10(32비트 감김) 그림자 요청을 낸다.
  3. 프레임과 SHP 를 다시 읽어 본체 요청을 낸다. 그림자 소비자가 바꾼 프레임/SHP 는 반영하고, 바뀐 TYPE 으로 다시 고르지 않는다.
  - 소유자/dead/추상 관문을 덧붙이지 않는다.
- 거부 종류: 다른 가상 표·소비자 누락은 `TypeError`(cpppj `invalid_argument`), raw 범위와 방향 프레임 범위는 `RangeError`(`out_of_range`)다.

## 2. 검증

- `tests/temple-draw.test.mjs`(2건): 원본 264행에서 raw 와 실제 TYPE 정의(baseFrame·프레임 코드 표)만 공급해 기본 프레임(부호 없는 DWORD)과 제출 순서(F번호·S 소리, 없으면 `-`)를 대조하고, 호출 뒤 raw 가 그대로인지 본다. 거부 경로는 웹 계약 검사다.
- `tests/rain-draw.test.mjs`(2건): cpppj 와 같은 29클러스터 입력 코드 표(A·B×14·C×13·P)로 원본 10.78 116행의 제출 순서를 대조한다. 웹 계약 검사는 거부·음수 프레임 범위 오류와 소리 콜백 뒤 현재 프레임 재읽기다.
- `tests/mana-additional.test.mjs`(2건): 원본 756행에서 그림자 출력 뒤 변경 규칙(없음 / 프레임·TYPE / 프레임·TYPE·SHP·WORD)을 cpppj 시험처럼 적용하고, 출력열·SHP 읽기열·최종 raw·최종 SHP 표 네 칸을 대조한다. 웹 계약 검사는 거부와 그림자 좌표 감김이다.

최초 실패와 교정: 없다. 모듈과 시험 모두 첫 실행에 통과했다. 비교가 실제로 작동하는지 확인하려고 상승 구간 글자를 'B' 로, 그림자 이동량을 9 로 바꾸면 세 원본 대조 검사가 모두 실패했고, 확인 뒤 되돌렸다.

## 3. 경계와 남은 것

- cpppj `RainDrawTests` 의 Renderer 합성 두 시험(`RainDraw_RendererLayersAndDirtyPixels`·`RainDraw_RendererFollowingFrameExtendsBounds`)은 client `Renderer` 의 후속 그림(followingFrames) 겹침과 dirty 지움을 검사하므로, `Renderer.cpp`(현재 partial) 이식 때 옮긴다.
- 실제 Session 의 Squid 가상 분배(신전·마나 가상 표의 Draw 슬롯 선택), extra 의 abstract/특수 Draw 선택, 실제 templeAscend.wav 재생·VFX 그림자 치환표(557738)·현재 SHP 토큰은 순서5 표시 연결 몫이다(PORT_MAP session 0).
- 같은 묶음의 남은 그림·캡처 단위: RawPlainAssetDraw·RawPlainUnitCapture·RawPlainWorkshopCapture·RawColorSortPrimary·RawMinimapRaster·RawColorMapCache(adapt).

## 4. 다음 시작점

WB04-E204 는 "그림·캡처" 묶음의 RawColorSortPrimary(`ColorSortPrimaryTests`)·RawMinimapRaster(`MinimapRasterTests`)나 3.1절의 다른 묶음에서 고른다. RawPlainAssetDraw·RawPlainWorkshopCapture·RawPlainUnitCapture 는 `CampaignSessionTests` 안에서만 쓰이거나 전용 시험이 없으므로 순서5 Session 연결과 함께 판단한다.
