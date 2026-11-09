# webpj ColorSort primary(RawColorSortPrimary) 이식 — WB04-E205

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절 "그림·캡처" 묶음의 네 번째 부분이다. ColorSort 시작(E77 `raw-color-sort-startup.js`, 43b430)이 0x3800 할당 뒤 부르는 primary 생성자 439de0 을 옮겼다. 팔레트 256색마다 R/G/B 와 H/L/S double 을 담는 56바이트 기록을 원본 저장 순서·binary64 연산 순서 그대로 만들고, 소비자가 읽는 최초 명도 double 을 재계산 없이 빌려 준다.

- cpppj 기준선 `1f00a25a`. 소스·시험·fixture 모두 기준선 뒤 변경 없음. 근거 커밋 `88834c49`(소스·시험·fixture 같음).
- 원본 fixture: `colorsort-primary-x86.tsv` 80행(제어어 027f 53비트 40행·037f 64비트 40행, SHA-256 `ddc522d4…`).
- 선행: E77 `raw-color-sort-startup.js` 의 `construct(대상, logical)` hook 이 이 생성자의 실제 연결 자리다(아직 연결 전).

## 1. 옮긴 것

`www/core/raw-color-sort-primary.js`(DOM 없음, 10.78 기본 53비트/nearest)

- `new RawColorSortPrimary(allocationBytes)`: 원본 malloc 직후의 실제 초기 0x3800 바이트를 그대로 복사해 소유한다. 미작성 padding(+52..+55)을 0 으로 채우지 않으며 저장소(`bytes`)의 주소는 바뀌지 않는다.
- `construct(logicalRgbx, hooks)`(439de0)
  1. 첫 전체 순회: 항목마다 R(+24) → G(+32) → 번호(+48 DWORD) → B(+40) 순서로 저장한다. 값은 RGB 바이트 × 원문 503608 상수(binary64 `0x3f70101010101010`)다. B 바이트는 번호를 쓰기 전에 캡처한다(입력이 같은 raw 를 빌리는 alias 행에서 결과가 달라진다).
  2. 둘째 전체 순회: 저장된 R/G/B double 에서 H(+0)·L(+8)·S(+16)를 계산해 저장한다.
     - 합을 먼저 구하고 명도 = 합 × 0.5 다.
     - 채도의 분모는 명도 ≤ 0.5 면 합, 아니면 (2 − 최대) − 최소(뺄셈 순서 보존)다.
     - 색상은 최대 채널별 식 × 60, 음수면 +360 이다.
     - 회색은 S = 0·H = −1 이다.
  - `hooks.checkpoint(누적수, offset, 크기)`: 진입 때 (0,0,0), 각 store 완료 뒤 부른다. 콜백 예외는 이미 쓴 바이트를 되돌리지 않고 그대로 전파한다(부분 효과 관측).
  - 논리 입력은 매번 현재 값으로 읽는다. 같은 저장소의 alias 를 사본으로 바꾸지 않는다.
- `constructed`: 부모가 성공 반환한 기록만 읽게 하는 host 표식이다. 재생성은 진입 때 false 로 돌린다.
- `lightness(index)`: 완료한 +index×56+8 의 double 을 비트 그대로 읽는다. 미완료면 `Error`, 번호가 범위 밖이면 `RangeError` 다.
- x87 64비트(037f) 결과는 지원하지 않는다. cpppj 와 같이 기본 027f(53비트)만 제품으로 옮긴다. JS 수 연산이 binary64 nearest 라 027f 결과와 같다.

## 2. 검증

`tests/color-sort-primary.test.mjs`(3건, cpppj `ColorSortPrimaryTests` 3건)

- 027f 40행: 각 행의 초기 할당·논리 RGBX(또는 같은 raw 의 alias 구간)·중단 store 수만 공급한다. 다음을 대조한다.
  - 저장소 주소 유지와 raw 0x3800 전체
  - 실행 뒤 논리 입력(alias 반영)
  - store 관측열(offset:크기:바이트) 전체
  - 완료/중단 여부: 정상 18행은 1792 store, 중단 22행은 지정 수
  - 정상 행 256색 명도 getter 의 double 비트(4608개)
- 037f 감사: 두 정밀도 정상 결과 18쌍의 필드별 차이가 H 409·S 451·나머지 0 임을 확인한다(제품 기대를 53비트로 치환하지 않는다).
- 손검사: 초기 할당 크기 계약, 생성 전 getter 거부, 같은 저장소 재생성, getter 범위, 짧은 논리 입력 거부, 재생성 뒤 미작성 padding 유지.

최초 실패와 교정: 없다. 모듈과 시험 모두 첫 실행에 통과했다. 비교가 실제로 작동하는지 확인하려고 명도 경계(≤ → <)·고명도 분모의 뺄셈 순서((2−최대)−최소 → 2−(최대+최소))·B 바이트 선캡처를 각각 바꾸면 027f 대조가 모두 실패했고, 확인 뒤 되돌렸다.

## 3. 경계와 남은 것

- E77 `initializeColorSort` 의 `construct` hook 에 이 생성자를 연결하고 primary DWORD/할당·검색 객체 수명과 묶는 일은 ColorSort 검색 생성·qsort 본체(client `ClientColorSortInput` 등)와 함께 순서5 표시 연결 때 한다(PORT_MAP session 0).
- 037f(64비트 확장 정밀도) 제어어 경로는 범위 밖이다.

## 4. 다음 시작점

WB04-E206 은 3.1절에서 고른다. "그림·캡처" 묶음의 남은 단위(RawPlainAssetDraw·RawPlainUnitCapture·RawPlainWorkshopCapture·RawColorMapCache(adapt))는 Session 시험 안에서만 쓰이거나 전용 시험이 없으므로, 다른 묶음(비행·이동 유닛·Dude 생성·경로·총/피해/삭제 수명·다리·자원/생산/건설 명령·의식/신전·공통 수명)에서 cpppj 전용 fixture 시험이 있는 단위를 먼저 권한다.
