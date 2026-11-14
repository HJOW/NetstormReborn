// 원본 Sky Overview(미니맵)의 crop·카메라 프레임 사각형·바깥 드래그 계산을 옮긴다(원문 00480270/004803f0/00480440/004805e0/00480660, cpppj client/MinimapGeometry.cpp/.h, 기준선 1f00a25a, 10.78 전용).
// x87 의 정수 절삭(CRT _ftol: signed 64비트 절삭 뒤 하위 DWORD, 무효 값은 0)과 float 중간 저장 위치, signed 32비트 감김, std::min/max 의 NaN 처리 순서를 보존한다.
// 화면 픽셀로 바꾸거나 가장자리를 clamp 하는 일은 호출자의 몫이다(실제 497f20 은 호출자가 실행). 전용 PE fixture 는 없다(cpppj 에도 직접 시험이 없다). 기준선 뒤 변경도 없다.
// 사용: const state = new MinimapGeometryState(); rebuildMinimap(state, { camera: [bits0, bits1], viewport: {left,top,right,bottom}, pixelsX: 16, pixelsY: 11 }); planMinimapDraw(state, view, { x, y }).
// (2026-11-14 WB04-E448: RawMenuControls 선행 미니맵 기하 이식)
import { cameraFtolLow } from '../render/camera-view.js';

// 공유 128×128 bitmap 의 한 변이다. 화면의 crop 크기와 구별한다.
const BITMAP_SIDE = 128;

// 실제 minmapGump 의 크기 및 +a0/+a4/+a8 확장 필드다.
export class MinimapGeometryState {
    // 배율 적용 전 원본 +3c/+40 크기(float32)다.
    width = 128; height = 128;
    // 공유 128×128 bitmap 에서 잘라낼 원점(signed 32비트)이다.
    cropX = 0; cropY = 0;
    // 실제 눌림 동안만 rebuild 가 crop 자동 추적을 중단한다.
    scrolling = 0;
}

// 원본 카메라 float 비트(0059a900/904)와 지도 표시 사각형·칸 배율(0059a908/90c, 현재 signed)을 같은 시점에 읽는 보기 기본값이다.
export function minimapGeometryView(fields = {}) {
    return { camera: [0, 0], viewport: { left: 0, top: 0, right: 0, bottom: 0 }, pixelsX: 16, pixelsY: 11, ...fields };
}

// signed 32비트 덧셈/뺄셈(원본 DWORD 감김)이다.
const add = (a, b) => (a + b) | 0;
const subtract = (a, b) => (a - b) | 0;
// 정수 나눗셈은 0 방향 절삭이다.
const divide = (a, b) => Math.trunc(a / b) | 0;
// float 비트(부호 없는 32비트)의 값이다.
function floatOfBits(bits) { const view = new DataView(new ArrayBuffer(4)); view.setUint32(0, bits >>> 0, true); return view.getFloat32(0, true); }
// C++ std::min/std::max 의 NaN 처리(비교가 거짓이면 첫 인자)를 그대로 쓴다.
const stdMin = (a, b) => (b < a ? b : a);
const stdMax = (a, b) => (a < b ? b : a);

// 카메라의 현재 float 절삭과 viewport 원점의 정수 나눗셈을 먼저 더한 뒤 2 로 나눈다. 칸 배율이 0 이하이면 TypeError(C++ invalid_argument)다.
function center(view) {
    if (view.pixelsX <= 0 || view.pixelsY <= 0) throw new TypeError('미니맵의 유효한 현재 칸 배율이 필요합니다');
    return [divide(add(cameraFtolLow(floatOfBits(view.camera[0])), divide(view.viewport.left, view.pixelsX)), 2), divide(add(cameraFtolLow(floatOfBits(view.camera[1])), divide(view.viewport.top, view.pixelsY)), 2)];
}

// 지도 영역 폭/높이를 현재 픽셀 배율의 두 배로 signed 나눈다.
function visible(view) {
    return [divide(subtract(view.viewport.right, view.viewport.left), Math.imul(view.pixelsX, 2)), divide(subtract(view.viewport.bottom, view.viewport.top), Math.imul(view.pixelsY, 2))];
}

// 00480270: 드래그 중(scrolling)에는 아무것도 바꾸지 않고 false, 아니면 현재 카메라 주변 crop 을 계산해 저장하고 true 다. 후보 = 카메라 중심 - ftol((크기 - 보이는 칸 수)×0.5) 를 float 로 좁힌 값을 0..(128-크기)로 제한한 뒤 절삭한다.
export function rebuildMinimap(state, view) {
    if (state.scrolling) return false;
    const middle = center(view); const shown = visible(view); // signed 정수 연산이 끝난 원본 중간값이다.
    const candidateX = subtract(middle[0], cameraFtolLow((state.width - shown[0]) * 0.5)); // 원본은 여백 절반을 먼저 ftol 하고 center 에서 signed DWORD 로 뺀다.
    const candidateY = subtract(middle[1], cameraFtolLow((state.height - shown[1]) * 0.5)); // 두 번째 축도 여백만 먼저 절삭한다.
    state.cropX = cameraFtolLow(stdMax(0.0, stdMin(Math.fround(candidateX), BITMAP_SIDE - state.width)));
    state.cropY = cameraFtolLow(stdMax(0.0, stdMin(Math.fround(candidateY), BITMAP_SIDE - state.height)));
    return true;
}

// 004803f0: signed crop + 지역 좌표(float32)를 확장 정밀도로 두 배한 뒤 마지막에 float32 로 저장하는 카메라 목표 [x, y] 다.
export function minimapTarget(state, localX, localY) {
    return [Math.fround((state.cropX + Math.fround(localX)) * 2.0), Math.fround((state.cropY + Math.fround(localY)) * 2.0)];
}

// 00480440: 좌/상단은 <=0, 우측은 >=폭, 하단은 >=높이-1 인 끝점 포함 비교로 경계 밖이면 crop 을 4픽셀씩 옮긴 뒤 같은 현재 좌표의 목표를 계산한다(좌/상은 0 아래로 내려가지 않고 우/하는 128-크기를 넘지 않는다).
export function dragMinimap(state, localX, localY) {
    const x = Math.fround(localX); const y = Math.fround(localY);
    if (x <= 0) state.cropX = Math.max(0, add(state.cropX, -4));
    if (state.width <= x) state.cropX = cameraFtolLow(stdMin(Math.fround(add(state.cropX, 4)), BITMAP_SIDE - state.width));
    if (y <= 0) state.cropY = Math.max(0, add(state.cropY, -4));
    if (state.height - 1.0 <= y) state.cropY = cameraFtolLow(stdMin(Math.fround(add(state.cropY, 4)), BITMAP_SIDE - state.height));
    return minimapTarget(state, x, y);
}

// 004805e0: 부착 전 crop+크기가 128 을 넘는 축의 crop 만 절삭해 보정한다. 원본은 음수 crop 을 별도로 0 으로 바꾸지 않는다.
export function clampMinimapAttachment(state) {
    if (BITMAP_SIDE < Math.fround(state.cropX) + state.width) state.cropX = cameraFtolLow(BITMAP_SIDE - state.width);
    if (BITMAP_SIDE < Math.fround(state.cropY) + state.height) state.cropY = cameraFtolLow(BITMAP_SIDE - state.height);
}

// 00480660: 실제 bitmap 블릿 원점(crop)·대상 원점·블릿 크기(min(128, 크기)의 ftol)와 카메라 outline 사각형(대상 원점 + (카메라 중심 - crop), 폭/높이는 보이는 칸 수)을 계산한다. 블릿 크기와 outline 크기는 다르게 계산하므로 임의 비례 축소를 넣지 않는다.
export function planMinimapDraw(state, view, destination) {
    const middle = center(view); const shown = visible(view);
    const left = add(destination.x, subtract(middle[0], state.cropX)); const top = add(destination.y, subtract(middle[1], state.cropY)); // 대상 원점에서 crop 을 뺀 outline 원점이다.
    return {
        source: { x: state.cropX, y: state.cropY }, destination: { x: destination.x, y: destination.y },
        size: { x: cameraFtolLow(stdMin(BITMAP_SIDE, state.width)), y: cameraFtolLow(stdMin(BITMAP_SIDE, state.height)) },
        camera: { left, top, right: add(left, shown[0]), bottom: add(top, shown[1]) },
    };
}
