// 미니맵 기하(minimap-geometry: rebuild·target·drag·clamp·draw plan)를 기준선 소스 계약과 손 계산 값으로 검증한다.
// cpppj에도 직접 시험이 없어 원본 직접 관측 수치가 아니다. 모든 기대값은 기준선 식(x87 절삭·float 중간 저장·signed 32비트 감김)을 손으로 계산했다. 실제 게임·브라우저 실행이 아니다. (2026-11-14 WB04-E448)
import test from 'node:test';
import assert from 'node:assert/strict';
import { MinimapGeometryState, clampMinimapAttachment, dragMinimap, minimapGeometryView, minimapTarget, planMinimapDraw, rebuildMinimap } from '../www/ui/minimap-geometry.js';

// float32 비트와 보기 만들기다. 기본 보기: 지도 영역 (0,0)-(1024,768), 칸 배율 16×11 → 보이는 칸 수 [32, 34].
const bits = (value) => { const view = new DataView(new ArrayBuffer(4)); view.setFloat32(0, value, true); return view.getUint32(0, true); };
const viewOf = (cameraX, cameraY, fields = {}) => minimapGeometryView({ camera: [bits(cameraX), bits(cameraY)], viewport: { left: 0, top: 0, right: 1024, bottom: 768 }, ...fields });
const stateOf = (fields = {}) => Object.assign(new MinimapGeometryState(), fields);

test('MinimapGeometryState/minimapGeometryView 기본값: 크기 128, crop 0, 눌림 0 이고 보기의 칸 배율은 16×11 이다', () => {
    const state = new MinimapGeometryState(); assert.deepEqual([state.width, state.height, state.cropX, state.cropY, state.scrolling], [128, 128, 0, 0, 0]);
    const view = minimapGeometryView(); assert.deepEqual([view.camera, view.viewport, view.pixelsX, view.pixelsY], [[0, 0], { left: 0, top: 0, right: 0, bottom: 0 }, 16, 11]); assert.equal(minimapGeometryView({ pixelsX: 8 }).pixelsX, 8);
});

test('rebuild: 카메라 중심/보이는 칸 수/여백 절반으로 crop 을 정하고(손 계산 34·15), 눌림 중이면 바꾸지 않고 false 다', () => {
    const state = stateOf({ width: 64, height: 64 }); assert.equal(rebuildMinimap(state, viewOf(100, 60)), true); assert.deepEqual([state.cropX, state.cropY], [34, 15]);
    // 지도 영역 왼쪽/위 원점이 중심에 더해진다(160/16=10, 220/11=20 → 중심 (110/2, 80/2) = (55, 40)).
    const shifted = stateOf({ width: 64, height: 64 }); rebuildMinimap(shifted, viewOf(100, 60, { viewport: { left: 160, top: 220, right: 1184, bottom: 988 } })); assert.deepEqual([shifted.cropX, shifted.cropY], [39, 25]);
    const pressed = stateOf({ width: 64, height: 64, cropX: 7, cropY: 8, scrolling: 1 }); assert.equal(rebuildMinimap(pressed, viewOf(100, 60)), false); assert.deepEqual([pressed.cropX, pressed.cropY], [7, 8]);
    const none = stateOf({ width: 64, height: 64, scrolling: 1 }); assert.equal(rebuildMinimap(none, viewOf(0, 0, { pixelsX: 0 })), false);
});

test('rebuild 경계: 0..(128-크기)로 제한하고 카메라 float 는 0 방향으로 절삭하며 비유한 값은 0, 칸 배율 0 이하는 TypeError 다', () => {
    const far = stateOf({ width: 64, height: 64 }); rebuildMinimap(far, viewOf(1000, 1000)); assert.deepEqual([far.cropX, far.cropY], [64, 64]);
    const near = stateOf({ width: 64, height: 64 }); rebuildMinimap(near, viewOf(-1000, -1000)); assert.deepEqual([near.cropX, near.cropY], [0, 0]);
    const trunc = stateOf({ width: 64, height: 64 }); rebuildMinimap(trunc, viewOf(100.9, 60.9)); assert.deepEqual([trunc.cropX, trunc.cropY], [34, 15]);
    const negative = stateOf({ width: 64, height: 64 }); rebuildMinimap(negative, viewOf(-5.5, -5.5)); assert.deepEqual([negative.cropX, negative.cropY], [0, 0]);
    const nan = stateOf({ width: 64, height: 64 }); rebuildMinimap(nan, minimapGeometryView({ camera: [0x7fc00000, 0x7fc00000], viewport: { left: 0, top: 0, right: 1024, bottom: 768 } })); assert.deepEqual([nan.cropX, nan.cropY], [0, 0]);
    // 크기가 128 이면 제한 범위가 0 뿐이다. 크기 100 은 0..28.
    const full = stateOf(); rebuildMinimap(full, viewOf(100, 60)); assert.deepEqual([full.cropX, full.cropY], [0, 0]); const wide = stateOf({ width: 100, height: 100 }); rebuildMinimap(wide, viewOf(1000, 1000)); assert.deepEqual([wide.cropX, wide.cropY], [28, 28]);
    for (const fields of [{ pixelsX: 0 }, { pixelsX: -1 }, { pixelsY: 0 }, { pixelsY: -5 }]) assert.throws(() => rebuildMinimap(stateOf(), viewOf(0, 0, fields)), { name: 'TypeError', message: /칸 배율/ }, JSON.stringify(fields));
    // 큰 카메라 값(2^31)의 절삭은 하위 DWORD(-2^31)이고 중심은 0 방향 나눗셈이다.
    const huge = stateOf({ width: 64, height: 64 }); rebuildMinimap(huge, viewOf(2147483648, 0)); assert.equal(huge.cropX, 0);
});

test('rebuild 여백: 보이는 칸 수/크기에 따라 여백 절반(0.5 곱 뒤 절삭)을 중심에서 뺀다(홀수 여백은 내림)', () => {
    const state = stateOf({ width: 65, height: 63 }); rebuildMinimap(state, viewOf(100, 60)); // 여백 (65-32)*0.5=16.5→16, (63-34)*0.5=14.5→14 → 50-16=34, 30-14=16
    assert.deepEqual([state.cropX, state.cropY], [34, 16]);
});

test('target: crop+지역 좌표(float32)를 두 배해 마지막에 float32 로 저장한다', () => {
    const state = stateOf({ cropX: 10, cropY: -3 }); assert.deepEqual(minimapTarget(state, 3.5, 2.25), [27, -1.5]);
    assert.deepEqual(minimapTarget(stateOf({ cropX: 0, cropY: 0 }), 0.1, 0.2), [Math.fround(Math.fround(0.1) * 2), Math.fround(Math.fround(0.2) * 2)]);
    // 큰 crop 에서 합이 float 로 좁혀지기 전의 배정도 곱을 쓴다.
    assert.equal(minimapTarget(stateOf({ cropX: 16777217, cropY: 0 }), 0.5, 0)[0], Math.fround((16777217 + 0.5) * 2));
});

test('drag: 좌/상 경계(<=0)는 crop 을 4 줄여 0 아래로 가지 않고, 우(>=폭)/하(>=높이-1)는 4 늘려 128-크기를 넘지 않으며 같은 좌표의 목표를 돌려준다', () => {
    const left = stateOf({ width: 64, height: 64, cropX: 10, cropY: 10 }); const target = dragMinimap(left, 0, 30); assert.equal(left.cropX, 6); assert.equal(left.cropY, 10); assert.deepEqual(target, [(6 + 0) * 2, (10 + 30) * 2]);
    const floor = stateOf({ width: 64, height: 64, cropX: 2, cropY: 3 }); dragMinimap(floor, -5, -0.5); assert.deepEqual([floor.cropX, floor.cropY], [0, 0]);
    const right = stateOf({ width: 64, height: 64, cropX: 10, cropY: 10 }); dragMinimap(right, 64, 10); assert.equal(right.cropX, 14); dragMinimap(right, 63.9, 10); assert.equal(right.cropX, 14);
    const ceiling = stateOf({ width: 64, height: 64, cropX: 62, cropY: 62 }); dragMinimap(ceiling, 100, 100); assert.deepEqual([ceiling.cropX, ceiling.cropY], [64, 64]);
    const bottom = stateOf({ width: 64, height: 64, cropX: 10, cropY: 10 }); dragMinimap(bottom, 10, 63); assert.equal(bottom.cropY, 14); dragMinimap(bottom, 10, 62.5); assert.equal(bottom.cropY, 14); dragMinimap(bottom, 10, 62.999); assert.equal(bottom.cropY, 14);
    const inside = stateOf({ width: 64, height: 64, cropX: 10, cropY: 10 }); const kept = dragMinimap(inside, 30.25, 31.5); assert.deepEqual([inside.cropX, inside.cropY], [10, 10]); assert.deepEqual(kept, [(10 + 30.25) * 2, (10 + 31.5) * 2]);
    // 좌/상과 우/하가 동시에 해당하면(폭 0) 두 단계가 차례로 적용되어 -4 뒤 +4 로 원래 값이 된다.
    const both = stateOf({ width: 0, height: 1, cropX: 10, cropY: 10 }); dragMinimap(both, 0, 0); assert.deepEqual([both.cropX, both.cropY], [10, 10]);
});

test('clamp: crop+크기가 128 을 넘는 축만 128-크기로 줄이고 같거나 음수 crop 은 그대로 둔다', () => {
    const over = stateOf({ width: 64, height: 64, cropX: 100, cropY: 65 }); clampMinimapAttachment(over); assert.deepEqual([over.cropX, over.cropY], [64, 64]);
    const same = stateOf({ width: 64, height: 64, cropX: 64, cropY: 10 }); clampMinimapAttachment(same); assert.deepEqual([same.cropX, same.cropY], [64, 10]);
    const negative = stateOf({ width: 64, height: 64, cropX: -50, cropY: -1 }); clampMinimapAttachment(negative); assert.deepEqual([negative.cropX, negative.cropY], [-50, -1]);
    const big = stateOf({ width: 200, height: 128.5, cropX: 0, cropY: 0 }); clampMinimapAttachment(big); assert.deepEqual([big.cropX, big.cropY], [-72, 0]);
    const one = stateOf({ width: 64, height: 64, cropX: 65, cropY: 0 }); clampMinimapAttachment(one); assert.deepEqual([one.cropX, one.cropY], [64, 0]);
});

test('planDraw: 블릿 원점(crop)·대상·블릿 크기(min(128,크기)의 절삭)와 카메라 outline(대상+(중심-crop), 폭/높이는 보이는 칸 수)을 계산한다', () => {
    const state = stateOf({ width: 64, height: 64, cropX: 34, cropY: 15 }); const plan = planMinimapDraw(state, viewOf(100, 60), { x: 10, y: 20 });
    assert.deepEqual(plan, { source: { x: 34, y: 15 }, destination: { x: 10, y: 20 }, size: { x: 64, y: 64 }, camera: { left: 26, top: 35, right: 58, bottom: 69 } });
    const wide = stateOf({ width: 200, height: 127.9, cropX: 0, cropY: 0 }); const second = planMinimapDraw(wide, viewOf(100, 60), { x: 0, y: 0 }); assert.deepEqual(second.size, { x: 128, y: Math.trunc(Math.fround(127.9)) });
    assert.throws(() => planMinimapDraw(state, viewOf(0, 0, { pixelsY: 0 }), { x: 0, y: 0 }), { name: 'TypeError', message: /칸 배율/ });
    // 32비트 덧셈은 감긴다.
    const wrap = planMinimapDraw(stateOf({ width: 64, height: 64, cropX: 0, cropY: 0 }), viewOf(0, 0), { x: 2147483647, y: 0 }); assert.equal(wrap.camera.left, 2147483647); assert.equal(wrap.camera.right, (2147483647 + 32) | 0);
});

test('경계: 칸 배율 1 은 유효하고(0 만 거부), 드래그의 좌/상 판정은 0 이하만 해당한다(0.5 는 경계 안)', () => {
    const state = stateOf({ width: 64, height: 64 }); assert.equal(rebuildMinimap(state, viewOf(100, 60, { pixelsX: 1, pixelsY: 1 })), true);
    const near = stateOf({ width: 64, height: 64, cropX: 10, cropY: 10 }); dragMinimap(near, 0.5, 0.5); assert.deepEqual([near.cropX, near.cropY], [10, 10]); dragMinimap(near, 1, 1); assert.deepEqual([near.cropX, near.cropY], [10, 10]);
    const edge = stateOf({ width: 64, height: 64, cropX: 10, cropY: 10 }); dragMinimap(edge, 0, 0); assert.deepEqual([edge.cropX, edge.cropY], [6, 6]);
    const negativeZero = stateOf({ width: 64, height: 64, cropX: 10, cropY: 10 }); dragMinimap(negativeZero, -0, -0); assert.deepEqual([negativeZero.cropX, negativeZero.cropY], [6, 6]);
});
