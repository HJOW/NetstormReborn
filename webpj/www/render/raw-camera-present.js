// 실제 Present 말미(10.78 49a081..49a1c9)의 진단 관문·두 목표 제한과 뒤 적용 호출의 순서를 옮긴 모듈이다(cpppj `client/RawCameraPresent.cpp/.h`, 10.78 전용). 화면 장치 출력과 현재 논리 카메라 데이터를 구별한다.
//  - presentCameraTarget(target, globals, hooks): ① 진단(005318ec)이 정확히 1 이면 이후 호출을 모두 건너뛴다(flip 장치를 요구하지도 않는다). ② 화면 flip(004a16d0)을 한다.
//    ③ 목표 픽셀 두 축을 현재 한계(flip 이 바꿀 수 있음)로 `ftol(max(0, min(값, 한계)))` 제한한다. ④ 세계 크기(00531928)로 만든 기본 세계 한계(X: 칸×16−폭, Y: 칸×11−높이, 하위 32비트 곱/뺄셈 뒤 signed 로 해석)로 다시 제한한다
//    — 현재 zoom 의 Renderer limit 과는 별도이다. ⑤ 같은 Renderer 의 00497960 전체를 부르는 commit 을 호출한다(현재 view 를 이 본체에서 대신 바꾸지 않는다).
//  - target = `ClientCameraGlobals.target()`({ limits: Int32Array(2), pixels: Int32Array(2), zoom }). 목표 쓰기만으로 현재 view·snapshot 을 바꾸지 않는다.
//  - globals = { diagnostic, width, height, worldCells } `{ value }` 상자(실제 전역을 빌려 flip 효과가 바꾼 현재 값도 반영). hooks = { flip(), commit() } — 도달한 위치에서만 요구하며 없으면 Error 로 거부한다(앞선 효과는 남는다).
// dirty 출력/clear 와 4a17e0 은 이 본체 앞의 호출자가 수행한다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-13 WB04-E383: 5-A 렌더러 프레임 앞단으로 추가, 기준선 뒤 cpppj 변경 없음)
import { cameraFtolLow } from './camera-view.js';

// 실제 호출 지점에 도달한 장치만 요구하며 진단의 조기 반환을 앞서 바꾸지 않는다. (2026-11-13 WB04-E383)
function need(callback) {
    if (typeof callback !== 'function') throw new Error('카메라 Present 현재 장치 미연결');
    return callback;
}

// 원문은 signed 최소값→ROM0 과 비교→FTOL 순서이다. int32 입력은 double 에서 정확히 표현된다. (2026-11-13 WB04-E383)
const limit = (value, maximum) => cameraFtolLow(Math.max(0, Math.min(value, maximum)));

// IMUL/SHL/SUB 의 하위 32비트 결과를 signed 상한으로 해석한다. (2026-11-13 WB04-E383)
const worldLimit = (cells, scale, screen) => (Math.imul(cells | 0, scale) - (screen | 0)) | 0;

// 진단을 한 번만 읽고 flip 뒤의 현재 한계·목표·세계/화면 크기를 순서대로 쓴다. 사용: presentCameraTarget(camera.target(), { diagnostic, width, height, worldCells }, { flip, commit }). (2026-11-13 WB04-E383)
export function presentCameraTarget(target, globals, hooks = {}) {
    if ((globals.diagnostic.value >>> 0) === 1) return;
    need(hooks.flip)();
    target.pixels[0] = limit(target.pixels[0], target.limits[0]);
    target.pixels[1] = limit(target.pixels[1], target.limits[1]);
    // 현재 zoom 의 Renderer limit 과 별도로 원문 기본 16/11 세계 한계를 적용한다.
    const maximumX = worldLimit(globals.worldCells.value, 16, globals.width.value);
    const maximumY = worldLimit(globals.worldCells.value, 11, globals.height.value);
    target.pixels[0] = limit(target.pixels[0], maximumX);
    target.pixels[1] = limit(target.pixels[1], maximumY);
    need(hooks.commit)();
}
