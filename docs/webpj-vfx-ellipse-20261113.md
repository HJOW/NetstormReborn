# webpj VFX 타원 윤곽 VFXEllipse·선택 체력 막대 부모 SpriteHitPointBar (WB04-E377, 2026-11-13, 5-A 여섯 번째 단위)

5-A 의 시각 효과 소단위 중 원본 관측 fixture 가 있는 두 단위 — client **VFXEllipse**(40줄)와 **SpriteHitPointBar**(48줄) — 를 `render/vfx-ellipse.js`·`core/sprite-hit-point-bar.js` 로 옮겼다. 두 파일 모두 기준선 뒤 cpppj 변경이 없다(base `1f00a25a`). `VFXLine`(일반 선의 클립 알고리즘 Cut/Inverse, 122줄)·`VFXTransform`·`DrawWater`·`ThunderProcess`(실제 World/Host/Form 필요)·`ConstructionVisualRenderer`·`SpellFrameVisibility.h` 는 다음 항목이다.

## 1. 규칙

- **`drawVfxEllipse(pixels, pitch, height, clip, center, radiusX, radiusY, color)` (00405071/CD 00468a51 단색 경로):** 표면 용량(`pitch ≥ 1`, `height ≥ 1`, `pitch × height ≤ 픽셀 수`) 오류는 `RangeError`, clip 은 표면 전체와 교차(반개방)해 비면 돌아온다. 중심은 pane 원점 감산/재가산이 상쇄된 절대 좌표, 색은 하위 BYTE 만 쓴다. **한 반경이 0 이면** 기존 VFX 선(`render/vfx-line.js`)으로 (중심−반경)→(중심+반경) 선을 그린다. 그 밖은 32비트 DWORD 감김 산술로: `a = rx²`, `b = ry²`, `x = 0`, `y = ry`, `dx = 0`, `dy = 2·a·ry`, `결정 = (a ≫ 2) + b − a·ry`; **첫 구간** `signed(dx − dy) < 0` 인 동안 네 대칭점(+x+y, +x−y, −x+y, −x−y 순서, 중복 쓰기 포함)을 찍고 `결정 ≥ 0`(signed)이면 `y−−, dy −= 2a, 결정 −= dy`, 그다음 `x++, dx += 2b, 결정 += dx + b`; 사이에서 `결정 += SAR(SAR(a − b) + (a − b) − dx − dy)`(SAR 은 부호 비트 보존 산술 시프트); **둘째 구간**은 do/while 로 한 번은 실행한 뒤 `signed(y) ≥ 0` 일 때까지: 네 점을 찍고 `결정 < 0` 이면 `x++, dx += 2b, 결정 += dx`, 이어 `y−−, dy −= 2a, 결정 −= (dy − a)`. 음수 반경은 부호 있는 DWORD 비트(`2³² − |r|`)로 같은 정수 경로를 탄다(EvilRay). 비수렴/과도한 입력은 100만 번 반복 상한에서 거부(`RangeError`).
- **`readSpriteHitPointBar(colors, hooks)` (원문 49898c):** 결과 색을 `colors[0]` 으로 시작, 현재 HP(`hooks.current`)와 최대 HP(`hooks.maximum`)를 읽어 `current ≤ trunc(maximum/2)` 이면 `colors[1]`; **다시** 두 값을 읽어 `current ≤ trunc(maximum/4)` 이면 `colors[2]`; 마지막으로 한 번 더 읽은 값이 `{ current, maximum }` 이 된다. 읽기 순서는 항상 H M H M H M, 호출 사이에 캐시하지 않고 색 배열도 사본을 만들지 않는다(진단/장치가 값을 바꿔도 이어서 읽음). 나눗셈은 signed 32비트 0 방향 절삭. 도달한 읽기에서만 장치를 요구한다. 10.72(CD)의 clamp 경로는 옮기지 않았다.

## 2. 웹 범위

- 소비자(`SpriteDraw` 의 선택 분기·`RawCampaignParticles`·미리보기)는 후속이다. `VFXEllipse` 의 선 경로는 웹 기존 `drawVfxLine`(분수 위상 순회; 클립을 자르지 않고 전체 선을 걸으며 클립 밖만 건너뜀)을 쓰므로 반경 0 인 거대한 좌표에서는 걷기 비용이 크다 — 그 경우를 위해 `VFXLine` 의 클립 알고리즘을 다음 항목에서 원문대로 옮긴다. DOM 을 쓰지 않는다.

## 3. 검사 (`tests/vfx-ellipse.test.mjs` 7건 · `tests/sprite-hit-point-bar.test.mjs` 3건)

- **원본 독립 관측(10.78):** `vfx-ellipse-x86.tsv` **270행**(64×48 표면, 배경 199, 앞뒤 32바이트 sentinel `0x7d`; 클립/중심/반경(음수 포함)/색 입력 — EvilRay 음수 반경, 두 정수 구간, 퇴화 선, 화면 밖 중심, 한 픽셀 pane)의 전체 화면이 **첫 실행에서 일치**했고 sentinel 은 변하지 않았다. `sprite-hit-point-bar-x86.tsv` originals **252행**의 결과 (색,현재,최대)·읽기 추적·끝 HP·최대 HP getter 호출 수가 **첫 실행에서 일치**했다. originalCD 행(clamp)은 사용하지 않는다.
- **웹 계약:** 원본 관측이 9개의 작은 반경 쌍(최대 8×7)에만 닿아 두 번째 구간의 결정 변수 갱신 등이 변형으로 드러나지 않았다 → **독립 BigInt 참조 구현**(32비트 감김을 매 연산 명시, 같은 C++ 알고리즘의 다른 표기)과 반경 1~64 전 조합(4096)·여러 중심/클립(1~24 조합 × 6)·음수 반경을 대조하는 시험을 더했다. 그 밖에: 표면 용량 오류와 정확히 맞는 길이, 1×1/1×N/N×1 표면, 너비/높이 0 인 클립은 큰 반경이어도 반복 없이 돌아옴(너비/높이 1 인 클립은 반복 상한에 걸림), 반경 0 선(가로/세로/한 점)과 4중 대칭, `sar` 직접 확인. HP 막대: 이하(포함) 임계, 홀수 최대, 음수 최대의 0 방향 절삭(바닥 나눗셈과 구별), int32 극단 값, H M H M H M 순서, 값과 색 재읽기, 장치 누락 거부와 부분 효과.

## 4. 변형 확인과 남은 경계

- 타원 90개(+경계 시험 후 재확인 8·SAR 2)·HP 막대 18개의 연산자·상수 변형을 적용했다. 1차에서 타원 22개와 HP 5개가 남았는데, 타원의 두 번째 구간 `dx`/`decision` 갱신 변형은 원본 관측의 작은 반경으로는 결정 부호가 뒤집히지 않아 보이지 않았고(독립 BigInt 참조 구현으로 모두 잡음), 표면 용량/빈 클립/`SIGN` 은 경계 시험과 `sar` 직접 시험으로 잡았다.
- 남은 동등 변형: 타원 `ITERATION_LIMIT`/`iterations` 의 정확한 경계(1,000,000 번째 점)·HP 막대의 결과 객체 초기값 0(곧바로 읽은 값으로 덮음)과 `(value | 0)` 의 최하위 비트(짝수 최대의 절삭이 후속 임계에서 항상 덮임 — 음수 최대에서는 절반 임계가 참이면 1/4 임계도 참이고, 양수 최대에서는 최하위 비트가 몫을 바꾸지 않음).
- 다음 5-A 항목: `VFXLine`(partial; 일반 선의 클립 Cut/Inverse·DIV 범위 검사)·`VFXTransform`(151줄)·`DrawWater`(101줄; 관측 fixture draw-water 3종)·`ThunderProcess`(34줄; 실제 Form/Host 필요)·`ConstructionVisualRenderer`(59줄)·`SpellFrameVisibility.h` → 렌더러 프레임 본체.
