# webpj VFX 변환 커널 VFXTransform (WB04-E378, 2026-11-13, 5-A 일곱 번째 단위)

5-A 의 시각 효과 소단위 중 client **VFXTransform**(cpppj `client/VFXTransform.cpp/.h`, 149줄)을 `render/vfx-transform.js` 로 옮겼다. 기준선 뒤 cpppj 변경이 없다(base `1f00a25a`). 원본 VFX 변환 커널의 회전 없는(angle=0) 16.16 고정소수점 확대·축소·반전 두 가지 — 일반 출력 `drawScaledVfxImage`(004028cb/CD 004662ab)와 대상 색 변환 출력 `drawTranslatedVfxImage`(0040351d) — 를 8비트 표면에 복원한다. `VFXLine`(일반 선의 클립 Cut/Inverse)·`DrawWater`·`ThunderProcess`·`ConstructionVisualRenderer`·`SpellFrameVisibility.h` 는 다음 항목이다.

`render/indexed-surface.js` 는 `IndexedSurface.drawImage` 의 본문을 독립 함수 `drawIndexedImage(pixels, pitch, height, image, left, top, clip, colorMap, shadow)` 로 분리했다(동작 변경 없음; 클래스 메서드는 이 함수를 부른다). 변환 커널의 단위 배율 직접 출력 분기가 클래스 없이 같은 코드를 쓰기 위해서다.

## 1. 규칙

- **공통 검증 순서:** 표면 용량(`pitch ≥ 1`, `height ≥ 1`, `pitch × height ≤ 픽셀 수`) → 이미지 용량(폭·높이 1~32768, 칸 수 = `indices` 길이 = `opacity` 길이) → 프레임(특수 레코드 아님, `rect` 크기가 이미지와 일치) 순으로 거부한다. 원문의 `invalid_argument`/`out_of_range` 는 모두 `RangeError`(메시지로 구분)로 옮긴다. `drawTranslatedVfxImage` 는 색 변환표가 없으면 `TypeError`.
- **단위 배율 일반 출력:** `drawScaledVfxImage` 에서 두 배율이 정확히 0x10000 이면 변환 없이 `drawIndexedImage`(위치 `position + rect.left/top` 의 32비트 감김 합, 색 변환표가 있으면 적용)로 직접 그린다. 이 경로는 255 도 불투명 색으로 유지하고 WORD 범위 검사를 하지 않는다. `drawTranslatedVfxImage` 는 단위 배율도 아래 변환 경로를 거친다.
- **꼭짓점 변환:** 프레임 `rect` 의 네 값이 signed WORD(−32768~32767)여야 한다. `scaleCoordinate(c, s)` = `floor((c << 16) × s / 2³²)`(곱의 하위 16비트가 항상 0 이라 원문의 `+0x8000` 편향은 올림을 만들지 않으므로 내림 몫과 같다. 결과는 32비트 정수로 자른다). 끝점은 `position + scaleCoordinate(...)` 의 32비트 감김 합이며 네 끝점(firstX·lastX·firstY·lastY)이 모두 WORD 범위여야 한다.
- **한 행 생략·간격:** `firstY == lastY` 이면 아무것도 그리지 않는다(한 행 그림의 비단위 변환, translated 의 한 행 단위 배율 포함). 축마다 `max − min ≤ 32767` 이어야 한다.
- **샘플링:** 보이는 사각형 = `[left, top, right+1, bottom+1) ∩ clip ∩ 표면`(비면 반환). 기울기 `textureStep(±(크기−1), 간격)` = 0 방향 절삭 `(Δ × 65536) / 간격`(가로 간격 0 이면 0). 시작 열은 `간격 있음 ? (lastX > firstX ? 0 : 마지막 열) : (lastY > firstY ? 마지막 열 : 0)`(같은 x 의 두 변 중 오른쪽 변을 먼저 택하는 `<=` 비교), 시작 행은 `lastY > firstY ? 0 : 마지막 행`. 좌표는 `(인덱스 << 16) + 0x8000` 에서 시작해 32비트 DWORD 감김으로 누산하고, **클리핑된 첫 픽셀까지도 같은 기울기로 건너뛴 위치에서 시작**한다(`advance(start, step, 잘린 칸 수)`). 표본 칸은 `(v >>> 16) × 폭 + (u >>> 16)`.
- **픽셀 연산:** 일반 출력은 불투명 칸의 색(색 변환표가 있으면 `colors[색]`)이 255 가 아니면 쓴다(255 는 임시 표면의 투명). translated 출력은 `opacity` 이고 소스 색이 255 가 아닌 칸에서만 현재 대상 색을 `colors[대상 색]` 로 바꿔 쓰며, **결과가 255 여도 그대로 저장**한다.

## 2. 웹 범위

- 소비자(`SpriteDraw` 의 축소 분기·`GumpDraw`·`ImageGump`·`CampaignAuxiliaryDraw`)는 후속이다. cpppj 시험의 Sprite 네 모드(그림자·죽음·추상·밝기 160) 재생은 `SpriteDraw` 가 이식된 뒤 같은 translated fixture 로 다시 확인한다. 64비트 곱이 Number 정밀도를 넘는 `scaleCoordinate` 만 BigInt 를 쓰고 나머지는 Number/`Math.imul` 이다. DOM 을 쓰지 않는다.

## 3. 검사 (`tests/vfx-transform.test.mjs` 14건)

- **원본 독립 관측(10.78):** `scaled-pixels-x86.tsv` **240행**(실제 SHP RLE 12종 × 위치 (12,10)/(−1,1) × pane 두 가지 × 배율 0.5·0.99998·1.0·1.5·2.0, 32×24 표면)과 `translated-pixels-x86.tsv` **296행**(합성 288 + 실제 간헐천 8, 32×24/256×192 표면, 배율 0.5·1.0·1.5)의 전체 표면이 **첫 실행에서 일치**했다. 이 값은 웹이 쓰는 `ShapeDatabase.decode/frame` 의 RLE 해석도 함께 확인한다.
- **독립 참조 모델:** 원본 관측은 배율이 5종뿐이고 음수 배율·감김 위치·부분 clip 이 없어, 같은 알고리즘을 BigInt 32/64비트 산술로 다시 쓴 모델과 4000 라운드(1~12 칸 무작위 그림, 배율 14종 — 음수 반전, 분수, 단위 근처, 확대 —, 위치 32비트 감김 직전/직후 5%, 부분·뒤집힌 clip)를 대조한다. WORD/간격 위반은 같은 `RangeError` 와 표면 불변을 확인한다. `scaleCoordinate` 는 C++ 표기(0 방향 나눗셈 뒤 나머지 보정)로 쓴 독립 식과 2만 무작위 + 가장자리 136쌍, `textureStep` 은 BigInt 몫과 5만 무작위 + 63쌍을 직접 대조한다.
- **계약:** 표면 용량 오류와 정확히 맞는 길이, 1×1/1×N/N×1 표면과 높이 32768 그림, 이미지/프레임/표면 오류를 한 조건씩만 틀려 메시지로 구분, WORD 경계값(−32768·32767) 허용과 한 끝점만 벗어난 입력 거부, 간격 정확히 32767 허용/32768 거부(가로·세로), 한 행 그림 생략, 단위 배율 255 보존/비단위 255 투명/색 변환 결과 255 건너뜀, translated 의 마스크 255·투명 칸·결과 255 저장, 음수 배율 반전과 배율 0(세로 접힘은 생략, 가로 접힘은 한 열).

## 4. 변형 확인과 남은 경계

- 자동 연산자·상수 변형 87개와 손으로 쓴 구조 변형 35개(편향·시프트 폭·누산 횟수·시작 행/열·반전 부호·clip 무시·출력 모드 등)를 적용했다. 1차에서 28개가 남았는데, 대부분 이미지/프레임 오류의 한 조건씩 격리, WORD 경계값, 정확히 32767 간격, 1×N 표면 시험을 더해 잡았다.
- 남은 동등 변형 12개: ① `BigInt.asIntN(32, …)` 폭 33(몫이 항상 31비트 안), ② `BigInt(TEXTURE_BIAS)` 제거(곱의 하위 16비트가 0 이라 올림 없음), ③ 보이는 사각형이 빈 경우의 조기 반환 `||`/`>=` 3개(조기 반환이 없어도 반복이 0 번), ④ 같은 끝점 비교 `>` → `>=` 5개(`lastX == firstX` 는 간격 0 분기에서, `lastY == firstY` 는 앞의 생략에서 도달 불가), ⑤ 간격 0 일 때 `stepX` 0 → 1(첫 열 뒤에는 읽지 않음), ⑥ `advance` 의 `>>> 0`(뒤에서 모두 `>>> 16` 과 감김 덧셈에만 쓰여 Number 정밀도 안에서 같음).
- 다음 5-A 항목: `VFXLine`(partial; 일반 선의 클립 Cut/Inverse·DIV 범위)·`DrawWater`(101줄)·`ThunderProcess`(34줄; 실제 Form/Host 필요)·`ConstructionVisualRenderer`(59줄)·`SpellFrameVisibility.h` → 렌더러 프레임 본체.
