# webpj Renderer 앞단 3종 — RawRendererPrelude·RawRendererClouds·RawCameraPresent (WB04-E383, 2026-11-13, 5-A 열두 번째 단위)

5-A 렌더러 프레임 본체 중 원본 관측 fixture 가 있는 세 작은 client 단위 — **RawRendererPrelude**(cpppj `client/RawRendererPrelude.cpp/.h`, 기준선 111줄)·**RawRendererClouds**(76줄)·**RawCameraPresent**(45줄) — 를 `render/raw-renderer-prelude.js`·`render/raw-renderer-clouds.js`·`render/raw-camera-present.js` 로 옮겼다. 세 파일 모두 기준선 뒤 cpppj 변경이 없다(base `1f00a25a`). 이들은 원본 Renderer(10.78 `004994b0`) 안쪽의 순수 계산이며 호스트 화면·장치는 훅으로 비워 두었다. 호출자(`Renderer` 본체·`ClientRendererFrame`)는 다음 항목이다.

## 1. 규칙

- **공통 규약:** 원본이 효과(훅) 뒤에도 같은 저장소를 다시 읽는 현재 참조는 `{ value }` 상자로 받는다(상자는 실제 저장소를 빌린 것이라 매번 현재 값을 읽는다). DWORD 는 `>>> 0`, signed 비교는 `| 0` 으로 읽는다. 훅은 도달한 위치에서만 요구하고, 선택됐는데 없으면 `Error` 로 거부한다(앞선 효과는 남는다).
- **프레임 앞단 `prepareRendererFrame(state, globals, hooks)`:** ① 화면 flags(`mode`)가 이전 flags(`scan[18]`)와 다르면 구름 dirty 표시(`markCloudDirty`)와 전체 화면 구름 dirty(`queueCloudDirty`)를 하고 이전 flags 를 현재 값으로 갱신한다. ② 세계 redraw 요청(`force`)이 있으면 같은 두 효과를 한 번 더 한다(요청은 지우지 않음). ③ `mode` 에 구름 비트(8)가 있고 override(`overrideActive`)가 꺼져 있으면 — 구름 갱신 비트(32)에서 `updateClouds` 후 하위 BYTE 를 다시 읽고, 시차 비트(16)가 아니면 16행 표(`scan[1..16]`)에서 현재 index 의 행부터 30행 구간의 첫 dirty 행(`rowDirty`)의 30행 사각형을 지도 clip 으로 잘라 일반 dirty 에 추가(`invalidate`)하고 index 를 `(index+1)%16` 으로 옮긴다(표는 `scan[17]` 비트 0 으로 최초 한 번만 초기 높이 기준 값으로 구성). 이어 구름 dirty 표시, 시차+플래그 조합(16·1 켜짐, 2 꺼짐)이면 `invalidateBackground` 와 `backgroundRedraw = 1`. ④ 별도 구름 dirty 개수(`drawCounter`)를 0 으로 쓰고, 세계 요청이 있으면 일반 dirty 개수를 0 으로 쓴 뒤(`clearDirty` 는 호스트 표를 쓸 때만) 새 전체 사각형을 추가한다. `scan` 은 19 DWORD 인접 물리 배열이라 index −1(인접 칸)·16·17 의 읽기를 그대로 두고 범위 밖은 `RangeError`.
- **구름 `appendRendererCloudDirty`·`refreshRendererCloudBackground`·`drawRendererBackdrop`:** 4칸 저장은 진단이 켜져 있고 개수가 4 이상이면 진단 행(0x40c)을 알린 뒤 count 를 다시 읽고, 4 이상이면 저장 없이 끝내며 음수 개수는 앞선 미제공 물리 영역 쓰기(`RangeError`)이다. 저장은 호출자 사각형의 **사본**이다. 시차 갱신은 현재 count(signed)만큼 칸마다 카메라·분모를 다시 읽어 Y→X 순서의 IDIV 몫으로 시차 표면(`draw(rect, x, y, true)`)을 요청하고 정상 종료에만 count 를 0 으로 쓴다 — 분모 0 이나 INT_MIN÷−1 은 실제 IDIV 오류(`RendererCloudDivideError`), 4칸을 넘는 읽기는 `RangeError` 로 구별한다. 배경 선택은 묶음 숨김이면 검정으로 먼저 지우고, UI 전용(dirty 비트 4)이거나 덮여 있으면 끝내며(효과 뒤 현재 관문 재조회), 구름 비트가 없으면 날씨색, 시차+플래그 조합이면 색 255, 그 밖에는 시차 비트에 따라 카메라÷분모 또는 카메라 원점으로 구름을 그린다. 사각형은 left/top 만 호출 직전 값으로 읽고 right/bottom 은 먼저 보존한 값을 쓴다.
- **카메라 Present `presentCameraTarget(target, globals, hooks)`:** 진단이 정확히 1 이면 flip 장치도 요구하지 않고 반환. 아니면 `flip` → 목표 픽셀 두 축을 현재 한계(flip 이 바꿀 수 있음)로 `ftol(max(0, min(값, 한계)))` 제한 → 기본 세계 한계(X: 칸×16−폭, Y: 칸×11−높이, 하위 32비트 곱/뺄셈 뒤 signed)로 다시 제한 → `commit`. 현재 view·snapshot·zoom 은 이 본체가 바꾸지 않는다.

## 2. 웹 범위

- 앱 수명에 연결하지 않았다(`PORT_MAP` session 0). 소비자인 `Renderer`·`ClientRendererFrame` 이 다음 5-A 항목이며, 구름 시험 `RendererCloud_FrameGlobalsShareActualPreludeQueue`(실제 `ClientRendererFrameGlobals` 의 4칸 공유)와 카메라 시험 `CameraPresent_SharedDiagnosticAndDeferredCurrentView` 의 실제 `ClientGameplayGlobals.PatchDiagnostic()` 공유는 그 전역 소유자를 옮기는 단위에서 다시 건다(이 단위는 `{ value }` 상자와 기존 `ClientCameraGlobals` 로 같은 순서를 확인).
- `ClipScreenRect` 는 기존 웹 `render/indexed-surface.js` 의 `clipRect` 를, 구름 dirty 표시는 기존 `core/raw-kencloud-render.js` 의 `markKencloudDirty` 를 재사용한다.

## 3. 검사

- **원본 독립 관측(10.78) 3종이 모두 첫 실행에서 일치:** `renderer-prelude-x86.tsv` **720행**(별개 화면/Renderer/구름·스캔 상태 한 건씩 — 인접 scan 19·현재 전역 11·카운터/구름 요청 5 의 35 DWORD 와 M/F/O/U/Q/A/B 호출 순서), `renderer-clouds-x86.tsv` **695행**(정상 671·실제 IDIV 오류 24 — 4칸 16·전역 9·clip 4·개수·오류 표식과 A/F/C/P 호출 순서), `camera-present-x86.tsv` **178행**(진단·폭·높이·세계 칸·한계·목표와 flip/commit 시점 목표). 하위 효과는 명시한 변형 번호의 호출 직후에만 입력으로 공급하고 기대 분기를 다시 계산하지 않는다.
- cpppj 계약 시험 이식: `RendererPrelude_SelectedDevicesAndPartialFailure`·`RendererCloud_SelectedDevicesAndPhysicalBounds`·`CameraPresent_DiagnosticAndSelectedDeviceGuards`·`CameraPresent_SharedDiagnosticAndDeferredCurrentView`.
- 웹 계약: 스캔이 19칸이 아니면 거부·index 범위 밖/인접 읽기·행 표 최초 구성 값과 한 번만 구성·override 반환 1 도 활성·지도 clip 왼쪽 0/양수·행 구간 signed 덧셈 경계(INT_MAX−30)·세계 요청 시 clearDirty 순서·구름 저장/훅 인자의 사본·색 값(검정 5·날씨색 0xfedcba98·255)과 dirty 비트 4 만 UI 전용·덮임 비트·세계 한계 32비트 감김과 한 축만 걸리는 경계.
- 시험 파일 `raw-renderer-prelude.test.mjs` 5건·`raw-renderer-clouds.test.mjs` 4건·`raw-camera-present.test.mjs` 4건.

## 4. 변형 확인과 남은 경계

- 자동 연산자·상수 변형 152개와 손으로 쓴 구조 변형 65개를 적용했다(prelude 89+26, clouds 43+22, camera present 20+17). 1차 생존 중 중요한 것 — `overrideActive` 반환값 1, 지도 clip 왼쪽 0/양수, 행 구간 덧셈 경계(31), 배경 지우기 색의 비트 이동, dirty 비트 마스크 — 은 시험을 더해 모두 잡았다.
- 남은 동등 변형: ① `row < add(row, 30)` 의 `<=` (행+30 이 행과 같아질 수 없어 같은 결과), ② 구름 시차 갱신의 Y/X 나눗셈 순서 맞바꿈(같은 분모로 둘 다 나누므로 어느 쪽이 먼저 오류를 내도 같은 오류 종류), ③ `RendererCloudDivideError` 가 `RangeError` 를 상속하는 JS 관례(동작이 아니라 형식), ④ 카메라 `limit` 안의 `cameraFtolLow`(입력이 항상 int32 정수라 같은 값).
- 다음 5-A 항목: 렌더러 프레임 본체의 남은 부분 — `RendererDiagnostics`(110)·`Renderer`(partial 494)·`ClientRendererFrame`(396)·`ScreenPresentation`·`Screen`(partial adapt 818).
