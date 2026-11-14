# webpj 카메라 적용 RawCameraCommit (WB04-E463, 2026-11-15)

기준선 `1f00a25a`의 `RawCameraCommit.cpp/.h`(원문 `497960`)를 `www/render/raw-camera-commit.js`로 옮겼다. 목표(`RawCameraTargetState`)를 현재 Renderer view와 전역 snapshot에 적용하는 본체이며, E333이 옮긴 `ClientCameraGlobals`(두 64BYTE·목표·분모)와 `CameraView`(`setPixels`) 위에 놓인다. Present 제한과 실제 Frame 호출 위치는 원본처럼 호출자(후속 단위)가 소유한다.

## 이식한 것

`commitCameraTarget(state, globals, hooks)`:

- **배율 분기**(`target.zoom !== view.zoom`): 첫 `redrawRoots` 뒤에 목표 배율을 읽어 view/snapshot의 칸 크기(`IMUL` 하위 32비트 뒤 signed `SAR 16`)·Q16 배율·float 비트를 쓰고, 두 `setPixels`와 두 번째 `redrawRoots`, `worldRedraw = 1`으로 끝난다(다른 효과는 요구하지 않는다).
- **이동 분기**: 위치가 다르면 `lockCursor(0)`·`redrawRoots`, 그 뒤의 목표/현재 값으로 이동량(`dx`,`dy`)을 구해 `scroll` 세 조각(viewport, `x>0`이면 왼쪽 띠, `height<화면 높이`이면 아래 띠)을 호출한다. 시차(16)가 켜지면 DIB(2)가 꺼진 경우 현재/목표 원점을 **분모로 나눈 차이**(`IDIV`, 0으로 나눔과 `INT_MIN/-1`은 `RangeError`로 거부)만큼 배경을 이동하고 두 노출 띠를 배경 변경 영역으로 더하며, DIB(2)가 켜지면 이동이 있을 때만 전체 viewport를 변경 영역으로 더한다. 두 `setPixels` 뒤 이동이 없으면 끝나고, 있으면 `updateMouse`, 현재 변경 표를 매 반복의 현재 개수로 순회해(플래그 1은 보존, 2는 이동 전후 합집합, 그 밖은 이동한 사각형) 옮긴 뒤 노출 띠를 `addDirty`로 더하고 `unlockCursor`·`redrawRoots`·`redraw = 1`을 쓴다.
- 노출 띠(`exposed`)는 원문 순서를 그대로 쓴다: 음수 길이이면 끝 좌표 자체를 NEG하며(길이가 DWORD 감김으로 음수가 되는 경우), 가로 띠 다음 세로 띠다.
- 모든 외부 효과 뒤에는 같은 객체의 현재 속성을 다시 읽는다(`globals`는 읽기 때마다 현재 속성을 보는 객체, `redraw`/`worldRedraw`는 `{ value }` 상자, `dirty`는 Renderer의 살아 있는 표). 선택된 경계만 요구하고 미연결은 `Error`다.

## 기준선 뒤 판정

기준선 뒤 cpppj 변경 없음(base `1f00a25a`). CD 판본 분기가 없어 판본 인자도 없다.

## 검증

- 원본 10.78 관측 **106행**(`camera-commit-x86.tsv`)의 두 64BYTE·목표/전체 dirty·현재 표/전역·외부 호출 순서가 첫 실행에서 모두 일치했다. 즉시 이동 **168행**(`camera-immediate-x86.tsv`, 433aa0 부모의 worldRedraw/revision 쓰기→목표 쓰기→497960)도 첫 실행 일치로, E333의 `centerImmediately`가 기다리던 `commit` 장치가 이 모듈이다. manifest `camera-commit`·`camera-immediate`로 연결했고 기존 fixture를 수정하거나 다시 실행하지 않았다.
- cpppj `CameraCommitTests` 4건(106행, 이동 효과가 뒤 목표를 바꿔도 이동량은 앞에서 보존, 선택 장치/별도 배율 분기, 선택된 시차 분기에서만 IDIV 거부)과 `CameraImmediateTests` 둘(168행, 장치 없을 때 선행 쓰기 보존)을 옮겼다.
- 이월한 두 시험도 닫았다: E392의 `RendererPresentation_ActualCameraCommitSharesDirtyAndRequest`(실제 적용이 Renderer의 전체 출력 요청과 변경 표를 같은 저장소로 씀)와 E393의 `ScreenScroll_CameraUsesActualRendererDirtyAndNextDraw`. 후자의 cpppj 시험은 Draw 직후 `CameraRedraw()==0`을 단언하지만 기준선 `Renderer.cpp:296`은 요청을 **Present**에서만 소비하므로 cpppj 시험 결함이다(웹은 Present 뒤 0을 단언하고 cpppj는 고치지 않았다).
- 독립 시험 5건: 시차 분모 `-1`(정상 나눗셈)과 `INT_MIN/-1` 거부·0 쪽 절삭, 한 칸 이동 노출 띠와 감김으로 음수가 된 길이(`dx`/`dy` = `INT_MIN`)의 NEG 순서(손 계산), viewport 가장자리 조각(`x>0`·`height<화면`), 화면 플래그는 시차 16/DIB 2만 읽음(1·4·8·0x20·17·0x30·18·19), 연속된 모든 항목 이동·플래그 1/2/6 처리. 이 중 마지막은 fixture가 항목의 짝수/홀수 위치에서 플래그 1(버려짐)과 0이 번갈아 놓여 한 칸 건너뛰기 변형을 가리던 공백을 메운다.
- 변형 확인: 자동 변형 98개 중 생존 6개(`ArrayBuffer` 크기, 비어 있지 않은 길이에서의 `< 0`/`<= 0`/`< 1` 동치 두 쌍, 플래그 1을 먼저 건너뛰므로 같은 `& 2`/`& 3`)와 수동 의미 변형 21개 중 생존 1개(효과 없는 `const early = 0` 추가)는 모두 동등 변형이다. 첫 실행에서 어긋난 곳은 시험 기대값 두 곳(손 계산한 열 수 `39`, 탭 구분 열)뿐이었고 모듈은 고치지 않았다.

## 남은 것

실제 Client 프레임 말미의 `Present` 제한과 커서/Gump/화면 장치(`lockCursor`·`updateMouse`·`scroll` 등)의 결선, `CameraHome`·`CameraViewCommands`·`EdgeScroll`의 입력 쪽 호출은 5-B/6단계다.
