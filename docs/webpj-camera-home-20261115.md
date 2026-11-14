# webpj 초기 카메라 CameraHome (WB04-E464, 2026-11-15)

기준선 `1f00a25a`의 `CameraHome.cpp/.h`를 `www/core/camera-home.js`로 옮겼다. 원본 초기 카메라 선택(`resetCamera`)과 world float 중심 이동(`433ba0/4340d0`), 대본 `<$ViewSpot,(XxY)>`의 인자 해석(`433ad0`, 단위 676)이다. 중심 이동은 E333의 `ClientCameraGlobals.centerImmediately`(433aa0 전체 부모)에, 그 commit 장치는 E463의 `commitCameraTarget`에 이어진다.

## 이식한 것

- `createCameraHomeState()`: 원본 PE의 현재 모드/로컬 번호/화면·월드 크기/카운터 기본값(`localPlayer 1`, 전투 모드, `640×480`, 월드 `4096×2816`).
- `CameraHome(pool, types, state, center, hooks)`: 생성 때 타입 목록·`center` 함수·양수 화면 크기·풀 크기(65536 이하)·억제 깊이 장치 쌍을 확인한다(`TypeError`).
  - `reset()`: SID 2부터 free 비트만 보고 번호 순으로 훑는다. 요새/전투 모드에서 소유자 BYTE(10.78은 `+34`)가 로컬 번호와 같은 사제 genus(`flags2 & 0x200000`)는 마지막 후보가 되고 단정도 합/개수에 누적된다(합/개수는 평균 분기에서만 쓰이며 그 분기는 선택이 없을 때만 실행되어 사실상 도달하지 않지만 원문대로 둔다). Challenge는 현재 타입의 후보를 고르고 로컬 소유자 후보를 만나면 좌표를 읽고 멈춘다. 후보를 고르고도 좌표를 쓰지 않은 Challenge 경로는 원본이 미초기화 스택을 읽으므로 실행을 거부한다(`Error`). 억제 깊이를 증가 직전에 읽어(감김 직전은 `RangeError`) 1 올리고, 중심 이동(Challenge 중심/선택 후보의 `centerOn`/사제 평균/`128,128`) 뒤 현재 깊이에서 1을 내린다. 예외에도 같은 순서로 복구한다.
  - `centerOn(sid)`: free/dead 후보는 자연 반환한다. 현재 좌표에서 TYPE 발자국 **전체**(절반이 아님)를 뺀 float 로 `center`를 호출한 뒤 현재 타입을 다시 읽어 Carrier genus(`flags2 & 0x30000`)이면 내용물 preview 를 요청하고, 장치가 없으면 깊이 0 에서만 미연결로 거부한다. TYPE 번호가 표 크기 이상이면 순회·이동·이동 뒤 재조회에서 `RangeError`.
  - `centerPoint(x, y)`: 433aa0 부모에 world float 를 그대로 넘긴다.
- `parseViewSpot(text)`: 맨 앞 `(` 한 글자만 건너뛰고 `x`와 `)`로 나눈 두 토큰을 `atol`해 float 로 돌려준다(숫자가 아닌 필드는 0). `VIEW_SPOT_COMMAND = 0x020000d9`.

## 기준선 뒤 판정

기준선 뒤 cpppj 변경 없음(base `1f00a25a`). CD 10.72 분기(소유자 `+32`·별도 ftol)와 `originalCD` fixture 행은 웹 범위 밖이라 읽지 않는다.

## 검증

- 원본 10.78 **1,182행**(`camera-home-x86.tsv`)의 적용 원점·억제 깊이·요청/변환 번호가 첫 실행에서 모두 일치했다. 입력은 raw 슬롯 5..127의 번호/타입/상태/소유자/좌표 DWORD이고, 실제 433aa0(`centerImmediately`)과 commit 직전 원점 관측만 명시 장치다(독립 `makePointDevice`). `viewspot-x86.tsv` **34행**의 두 float 비트도 일치했다. manifest `camera-home`·`viewspot`로 연결했고 기존 fixture를 수정하거나 다시 실행하지 않았다.
- cpppj `CameraHomeTests` 4건(두 판본 중 10.78, ViewSpot 34행, 즉시 이동 부모까지의 목표 `1712,584`·카운터, 화면 적용 예외 때 깊이 복구)을 옮겼다.
- 독립 시험 5건: 기본 상태와 생성 검사 9종, 번호 2·3·연속/홀수 번호·마지막 후보·genus 낮은 비트, TYPE 번호=표 크기, 억제 깊이 장치의 읽기/쓰기·외부 변경 뒤 복구·감김 직전·Carrier 표시 미연결, 괄호 없는 ViewSpot.
- 변형 확인: 자동 변형 80개 중 생존 13개는 모두 동등 변형이다: 도달하지 않는 평균 분기와 그 누적값 초기화·`count + 1`(선택이 없을 때만 실행되는 분기에서 `count`는 항상 0), `FREE|DEAD`가 같은 3, `DataView` 길이 4→5, ViewSpot 빈 입력/한 글자의 시작 위치(결과가 같음).
- 첫 실행에서 어긋난 곳은 시험 도구 두 곳뿐이었다(웹 `SidPool`은 65536 이상을 만들 수 없어 풀 크기 검사를 같은 모양의 객체로 확인, 앞 단계에서 놓은 슬롯을 다시 비우지 않은 시험 입력). 모듈은 고치지 않았다.

## 남은 것

`CameraViewCommands`·`UtilityCommandInput`·`RunForIt` 등 실제 소비자(Session/Client)의 호출과 Carrier 내용물 preview 장치는 후속 단계다. 다음은 5-B의 `RawCameraView` partial 마무리와 `EdgeScroll`·`UberGumpCamera`다.
