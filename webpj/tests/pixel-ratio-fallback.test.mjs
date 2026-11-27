// 배율 알림이 없는 경우에도 표시 프레임이 배치를 복구하고 게임 프레임 상한/시계를 그대로 유지하는지 검사한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { Netstorm } from '../www/netstorm.js';

// 프레임 상한으로 게임 처리가 생략되는 실제 앱 경계다. DOM 적용만 관찰하고 게임 처리에 도달하면 실패한다.
function scene(ratio, previous) {
    const app = new Netstorm(), view = { devicePixelRatio: ratio }, calls = [];
    app._root = { ownerDocument: { defaultView: view } }; app._canvas = {}; app._layout = { devicePixelRatio: previous };
    app._lastFrameStamp = 1000; app._startFrameLoop = () => calls.push('schedule');
    app._gameClock = { captureFrame() { throw new Error('상한으로 생략된 게임 시계를 진행하면 안 된다'); } };
    app._applyLayout = () => { calls.push('layout'); app._layout = { devicePixelRatio: view.devicePixelRatio }; };
    return { app, view, calls };
}

// Native DPR 읽기는 원본 frameDue 관문보다 앞서고 같은 배율이면 불필요한 재배치를 하지 않는다.
test('DPR 알림 누락: 생략된 표시 기회에서 배치 복구·시계/프레임 상한 유지', () => {
    const s = scene(1.25, 2); s.app._onFrame(1001);
    assert.deepEqual(s.calls, ['layout', 'schedule']); assert.equal(s.app._layout.devicePixelRatio, 1.25); assert.equal(s.app._lastFrameStamp, 1000);
    s.app._onFrame(1002); assert.deepEqual(s.calls, ['layout', 'schedule', 'schedule']);
    s.view.devicePixelRatio = 2; s.app._onFrame(1003); assert.deepEqual(s.calls, ['layout', 'schedule', 'schedule', 'layout', 'schedule']);
});

// 잘못된 기기 값은 computeLayout과 같은1로 해석하고, 루트/배치가 없으면 DOM에 재진입하지 않는다.
test('DPR 표시 fallback: 비정상 입력과 초기화/반납 경계는 반복 재배치하지 않는다', () => {
    const s = scene(1, 1);
    for (const value of [Infinity, NaN, 0, -2, undefined]) { s.view.devicePixelRatio = value; s.app._onFrame(1001); }
    assert.equal(s.calls.filter((name) => name === 'layout').length, 0);
    s.app._root = null; s.app._onFrame(1001); s.app._root = { ownerDocument: { defaultView: s.view } }; s.app._layout = null; s.app._onFrame(1001);
    assert.equal(s.calls.filter((name) => name === 'layout').length, 0);
});
