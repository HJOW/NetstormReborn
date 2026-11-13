# webpj DrawWater 진단 사각형 표시 (WB04-E380, 2026-11-13, 5-A 아홉 번째 단위)

5-A 의 시각 효과 소단위 중 client **DrawWater**(cpppj `client/DrawWater.cpp/.h`, 99줄)를 `core/draw-water.js` 로 옮겼다. 원본 게임에 숨어 있는 진단 명령(`DrawWater`)으로, 켜면 화면에 보이는 모든 후보 객체 둘레에 세 겹 사각형을 그린다. 같은 파일에 기준선(`1f00a25a`) 뒤 cpppj `20261111-123 CP M7.3·M7.4`(마지막 변경 `c254e771`)가 더한 **`PathMarkerRectangles`**(원문 `00434970` 경로 표식 세 겹 사각형)도 있어 함께 옮겼다 — 새로 만든 기능이 아니라 원본 함수의 복원이라 3.6절 판정은 "이식 대상"이다. 이름표를 보내는 기본 개발자 메뉴 항목은 원본에도 없어 만들지 않는다.

## 1. 규칙

- **명령 이름표:** `DRAW_WATER_COMMAND = 0x0200001a`(00542760). `drawWaterScriptCode(name)` 는 이름의 첫 NUL 까지를 ASCII 소문자로 바꿔 `drawwater` 와 같으면 명령 코드, 아니면 `0xffffffff`(비 ASCII 대문자는 건드리지 않음, 파생 별칭 없음).
- **사건 분배(`dispatchDrawWater(event, enabled, worldRedraw)`):** 명령 코드(하위 26비트 `0x03ffffff` 마스크)가 아니면 `false`. 뗌(release, `0x80000000`) 사건이면 아무것도 쓰지 않고 `true`. 그 밖에는 현재 비영 전체 DWORD 를 `0`, 영이면 `1` 로 토글하며 **worldRedraw=1 을 flag 쓰기보다 먼저** 쓴다. 두 값은 `{ value }` 상자다.
- **가시 영역(`drawWaterVisibleArea`, 00497530):** snapshot 이 아니라 **현재 Renderer 보기**의 월드 float(`+0/+4`)와 정수 칸 크기(`+8/+c`), 전체 화면 폭/높이를 읽는다. `top = ftolLow(world.y)`, `bottom = top + 높이/칸세로`, `left = ftolLow(world.x)`, `right = left + 폭/칸가로` — 나눗셈은 signed IDIV(0 방향 절삭)이고 더하기는 DWORD 감김이다. 칸 크기 0 이나 `INT_MIN / −1` 은 정상 영역으로 숨기지 않고 `RangeError`(원문 나눗셈 예외), 계산 순서는 위·아래가 왼쪽·오른쪽보다 먼저다.
- **finder 영역(`drawWaterFinderArea`, 0041def0):** 각 좌표를 float32 로 저장하고, 오른쪽/아래만 `+0.99999`(float 상수 `0x3f7fff58`)를 더해 ftol 한 값을 다시 float32 로 저장한 뒤 최종 ftol 한다. 큰 정수의 float32 반올림(2²⁴+1 → 2²⁴)과 음수의 0 방향 절삭(−5 + 0.99999 → −4)이 그대로 남는다. 보드 제한은 finder 의 `begin` 이 한다.
- **표시(`RawDrawWater.draw`, 00434dd0):** flag 는 입구에서만 검사한다(도중에 끄거나 색을 바꿔도 현재 호출은 계속). 기존 `RawSquidFinder`(4단계 해시, 체인 순서, free/dead/void 를 새로 거르지 않고 buried 만 제외)의 모든 후보에 대해 snapshot 의 `project` 로 화면점을 구하고 `반크기 = trunc(칸크기/2)` 를 빼 **초기 사각형** `{x−halfX, y−halfY, x−halfX, y−halfX}`(가로 0 폭, 아래쪽은 원문대로 가로 반크기)를 만들어 `clip` 장치로 알린다. 이어 **세 번** `rectangle(사각형, 현재 color)` 를 부르되 호출 직전에 color 를 다시 읽고, 매 호출 뒤 네 변을 한 픽셀씩 확장(`left−1, top−1, right+1, bottom+1`, DWORD 감김)한다. 후보가 0개여도 flag 가 비영이면 마지막에 `present` 를 한 번 부른다. 장치는 도달한 것만 요구한다(없으면 `Error`). `enabled`·`color` 는 `{ value }` 상자다.
- **네 선(`drawWaterRectangle`, 004a2270):** 위·아래·왼쪽·오른쪽 순서로 VFX 선(`render/vfx-line.js` E379)을 그린다. 오른쪽/아래는 `right−1`, `bottom−1`(DWORD 감김)이라 0폭·역전 사각형도 생략하지 않으며 뒤 선이 같은 모서리를 다시 쓴다. 색은 하위 BYTE.
- **경로 표식(`pathMarkerRectangles`, 00434970):** 오른쪽 아래 = 투영점, 왼쪽 위 = 투영점 − 칸 크기. 먼저 한 픽셀씩 줄인 뒤(`right−1, bottom−1, left+1, top+1`) 기록하는 세 겹이다(DWORD 감김).

## 2. 웹 범위

- renderer/snapshot 은 `render/camera-view.js` 의 `CameraView`(64바이트 상태)다. `RawCameraView` 전체(초기화·설정·복사)는 5-B 몫이라 이 단위는 읽기만 한다. 소비자(`RendererDiagnostics` 의 Fire 진단, 렌더러 프레임의 후단, 게임 메뉴 명령 표 `GameMenuCommands`, 경로 표식 소비자)는 후속이다. `GameMenuCommandValue(0x542760) == kDrawWaterCommand` 확인은 `GameMenuCommands`(todo) 이식 때 한다. DOM 을 쓰지 않는다.
- cpppj 시험의 "Renderer dirty 가 비어도 후단은 전체 viewport 출력을 한다" 부분은 `Renderer`(5-A 후속)가 필요해 옮기지 않았다(웹 `draw` 는 dirty 를 읽지 않고 장치 `present` 만 부른다).

## 3. 검사 (`tests/draw-water.test.mjs` 9건)

- **원본 독립 관측(10.78):** `draw-water-x86.tsv` **76행**(여섯 합성 일반 객체를 실제 네 단계 해시에 건 장면 0·1(buried 섞임)·2(화면 밖), 월드 float/칸 크기/snapshot 원점, flag·색·칸 크기·객체 buried 변이)의 96×80(행 padding 100) 전체 표면, 최종 clip, flag, 색, 장치 호출 추적(`Begin/Clip/Rect/Lock/Unlock/Present`)이 **첫 실행에서 일치**했다. `draw-water-input-x86.tsv` **384행**(Guide/Gump/release/정지/Console/모드 조합)과 `draw-water-names-x86.tsv` **9행**(`DrawWater,0>`·`drawwater,3>`·`DRAWWATER,section>`의 이름·인자·고리 위치, 기존 `queueScriptCommand` 로 재생)도 첫 실행에서 일치했다.
- **웹 계약:** 사건 분배(비명령 false·release true·상위 비트 무시), flag 0 이면 빈 보기/빈 장치에서도 아무 일 없음, 후보 0개의 `present` 요구와 장치 도달 시점(clip → rectangle 순서), 가시 영역의 나눗셈 예외 5종과 음수·NaN·32비트 초과 월드·음수 칸 크기, `INT_MIN`/−1 이 아닌 경우의 정상 나눗셈, finder 영역의 float32 반올림·음수 snap, 네 선의 모서리 겹침·0폭 사각형·색 하위 BYTE, 초기 사각형의 음수 홀수/양수 홀수 반크기와 아래쪽 가로 반크기, 경로 표식 세 겹과 감김.

## 4. 변형 확인과 남은 경계

- 자동 연산자·상수 변형 72개와 손으로 쓴 구조 변형 31개(+반크기 3개 재확인)를 적용했다. 1차에서 자동 6개·손 3개가 남았는데 `Math.trunc` → `Math.floor`(음수 홀수 칸 크기) 두 개는 fixture 76행에 음수 홀수 칸 크기가 없어서 보이지 않았다 → 초기 사각형 시험을 더해 잡았다.
- 남은 동등 변형 6개: ① `RELEASE` 마스크의 최하위 비트(명령 코드는 짝수라 항상 0), ② `ArrayBuffer(4)`→5 와 `UPPER_SNAP` 의 1 ulp(정수 입력의 절삭 결과가 같음), ③ 이름 NUL 위치의 `<`/`0→1`(NUL 이 맨 앞이면 둘 다 미등록), ④ finder 최종 `cameraFtolLow(snapped)`(snapped 는 이미 int32 범위의 float32 정수라 항상 같음). 나눗셈 예외의 `&&` → `||` 는 `INT_MIN` 과 −1 이 아닌 쪽 조합 시험으로 잡았다.
- 다음 5-A 항목: `ThunderProcess`(34줄; 실제 World/Host/Form 필요)·`ConstructionVisualRenderer`(59줄; `DrawSubmission` 선행)·`SpellFrameVisibility.h`(11줄) → 렌더러 프레임 본체(`RendererDiagnostics` 가 이 모듈의 가시 영역·네 선을 그대로 재사용).
