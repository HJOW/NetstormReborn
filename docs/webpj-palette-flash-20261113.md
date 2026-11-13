# webpj 팔레트 계열 PaletteColors·PaletteFlash·ScreenPaletteApply (WB04-E376, 2026-11-13, 5-A 다섯 번째 단위)

5-A 의 팔레트 항목 중 원본 관측 fixture 가 있는 세 단위 — client **PaletteColors**(112줄)·**PaletteFlash**(132줄)·**ScreenPaletteApply.h**(28줄) — 를 `core/palette-colors.js`·`core/palette-flash.js`·`core/screen-palette-apply.js` 로 옮겼다(`ParticlePaletteMemory` 는 입자 본체 `RawCampaignParticles` 와 함께 다룬다). 세 파일 모두 기준선 뒤 cpppj 변경이 없다(base `1f00a25a`). 이 단위의 fixture 대조는 웹 `assets/palette.js` 의 파일 해석(`GamePalette.parse` 0x308 `.COL`/0x400 RGBQUAD, `buildScreenPalette` 의 0번 검정·255번 흰색 고정)과 `findPaletteColor` 도 처음으로 원본 8관측에 직접 대조한다.

## 1. 규칙

- **`buildPaletteColorTable(logical)` (004a4850):** 논리 팔레트(R|G<<8|B<<16 DWORD 256개)에서 원본 호출 순서의 **58개** 목표 RGB 의 색 번호를 `findPaletteColor`(채널 차이 제곱 합·float32 최소 거리·동률은 앞 번호)로 구한다. 앞 9개가 `basic`(검정·빨강(255,22,22)·초록(22,255,22)·파랑(22,22,255)·자홍·노랑·주황(240,61,0)·흰색·회색), `weather` = 새 검색 없이 `[노랑, 파랑, 빨강, 해 원소 갈색(181,140,111)=named[9]]`(바람·비·천둥·해). 같은 RGB 를 다른 전역에 적는 호출(17,66,60 둘 / 255,255,255 / 128,128,128)도 원본 순서대로 별도 항목으로 남긴다. 결과는 동결한다. 10.72(CD) 50색 변형은 옮기지 않았다.
- **`PaletteFlash` (번개, 00470d40~00471820):** 번개가 바꾸는 색 범위는 기본 팔레트의 구름 색 228~245(18색), 단계 64. `thunderOffset(요청)` 은 0 이면 기본 밝기 64, `thunderDuration(전체화면)` 은 전체화면 0.5초·창 모드 1/60초(창 모드는 진행만 하고 표시하지 않음). `init()` 은 벽시계로 마지막 시각을 정하고 간격 = 지속/64 로 둔 뒤 현재 팔레트를 **두 번** 읽어(원래 색·변경 색) 변경 색을 만들되, offset 0 은 목표 색(`solid`)을 채우고 그 밖은 각 RGB 에 offset 을 signed 32비트로 더해 256 이상이면 255 로 제한한다(음수는 아래로 제한하지 않고 byte 로 감김; 예약 바이트 보존). 초기화에서는 표시하지 않는다. `frame()` 은 경과 > 간격(엄격)일 때마다 — 지연되면 한꺼번에 — 각 색 세 채널을 원래 값 쪽으로 1씩 접근시키고 단계를 줄이며 `last` 에 간격을 반복 더하고(원본 반올림 순서), 단계가 1 미만이거나 64 초과이면 전체화면에서만 저장 팔레트 전체를 복구하고 종료(false), 아니면 전체화면에서만 부분 팔레트를 적용하며 계속(true). `cancel()` 은 초기화된 전체화면 효과의 저장 팔레트 복구(중복 취소 무시). 시각은 게임 정지와 무관한 벽시계이며 화면 모드는 매 프레임 다시 읽는다.
- **`applyScreenPaletteClear(clear, flags, loaded, hooks)`:** 파일 팔레트를 읽은 뒤 `loaded.value = 1`. `clear ≠ 0 && flags ≠ 0` 일 때만 lock → fillBlack → unlock → waitVerticalBlank → presentPreservingClip → flip 을 원문 순서로 부른다(도달한 장치가 없으면 부분 효과와 함께 오류; 선택하지 않은 경로는 장치를 요구하지 않는다). 중첩 깊이·buffer/pitch·clip 보존은 Screen 소유자의 몫이다.

## 2. 웹 범위

- 10.72(CD)/10.37 분기는 옮기지 않는다(`PaletteFlash` 의 CD/10.37 진행에는 초기화되지 않은 스택을 읽는 원본 버그가 있어 복원 목표와 같다고 취급하지 않는다는 cpppj 결정 그대로). 소비자(`Kencloud` 번개 프로세스 등록·`Screen` 의 팔레트 적용/복구·GDI/DirectDraw 표면)는 5-A 후속과 4-D 에서 연결한다. DOM 을 쓰지 않는다.

## 3. 검사 (`tests/palette-colors.test.mjs` 2건 · `tests/palette-flash.test.mjs` 6건 · `tests/screen-palette-apply.test.mjs` 2건)

- **원본 독립 관측(10.78):** `palettecolors-x86.tsv` originals **8행**(파일 크기 0x308/0x400 × 재사용)의 논리 팔레트 RGB(플래그 바이트 0/255=0·나머지 4 는 웹 logical 에 없어 RGB 만 비교)·저장 팔레트 BGR·기본 9색·날씨 4색·58개 (R,G,B,색 번호) 항목이 첫 실행에서 모두 일치했다. `paletteflash-x86.tsv` originals **256행**(entry 80·init 168·cancel 8)의 초기 변경 색·원래 색·간격 double 비트·진행 뒤 변경 색·마지막 시각 double 비트·남은 단계·초기화 여부·장치 호출 순서(apply/restore/kill)가 첫 실행에서 일치했다(NaN 은 NaN 으로 비교). `screen-palette-clear-x86.tsv` **166행**(PE 접두 관측: 중단 원인·13필드·64바이트 물리·trace)이 첫 실행에서 일치했다. 장치 대역(`Device`)은 cpppj `ScreenPaletteApplyTests` 의 명시 장치를 그대로 옮겼다.
- **웹 계약:** 합성 팔레트에서 독립 최근접 검색과 같음·기본/날씨 별칭·중복 RGB 항목·결과 동결·길이 거부; 번개 상수와 요청 규칙·초기값·밝기 경계(정확히 255/256·음수·offset 0 목표 색·예약 바이트 보존)·정지한 게임과 무관한 실시간 진행(간격의 정확한 배수에서 엄격 비교로 한 단계 적게)·화면 모드 매 프레임 재조회·최신 저장 팔레트 복구와 중복 취소 무시·불완전한 장치 묶음/색 범위 거부; clear 관문의 자연 생략과 필수 장치 누락 오류·호출 순서.

## 4. 변형 확인과 남은 경계

- 세 모듈의 연산자·상수 자동 변형 187+59+9+11 = 266회를 적용했다. 1차에서 `PaletteFlashState` 의 초기값 7개(빈 색 0·간격/마지막/지속 0)가 남아 초기값 시험으로 잡았고 그 밖은 모두 잡혔다(남은 동등 변형 없음).
- 시험이 먼저 실패한 곳은 시험 쪽이었다(0.125초에서 16단계를 기대했으나 경과가 간격과 정확히 같아지는 마지막 한 단계는 엄격 비교 때문에 진행되지 않아 15단계). 모듈은 관측 대조에서 바뀌지 않았다.
- 다음 5-A 항목: 시각 효과 소단위(`VFXEllipse`·`VFXLine`·`VFXTransform`·`DrawWater`·`ThunderProcess`·`ConstructionVisualRenderer`·`SpellFrameVisibility.h`·`SpriteHitPointBar`) → 렌더러 프레임 본체 순이다. `ParticlePaletteMemory`(adapt; 원본이 색 수 바로 뒤 인접 DWORD 를 읽는 호스트 메모리 경계)는 `RawCampaignParticles` 와 함께 결정한다.
