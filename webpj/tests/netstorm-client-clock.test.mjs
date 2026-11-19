// 앱 프레임과 탭 가시성에서 실제 Client 게임 시계의 수명을 검사한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { Netstorm } from '../www/netstorm.js';
import { ClientFrameClock } from '../www/core/client-frame-clock.js';
import { ClientLocalStateGlobals } from '../www/core/client-local-state-globals.js';
import { ClientTimeSource } from '../www/core/client-time-source.js';

// 실제 앱 메서드는 표시 시계와 별도로 게임 프레임을 고정하고 숨긴 시간은 게임 시간에서 제외한다.
test('Netstorm Client 시계: 프레임·탭 숨김·반복 통지', () => {
    let milliseconds = 1000;
    const source = new ClientTimeSource({ hostMilliseconds: () => milliseconds, hostUnixSeconds: () => 1700000000 });
    const app = new Netstorm(), local = new ClientLocalStateGlobals(), document = { hidden: false };
    app._root = { ownerDocument: document }; app._canvas = {}; app._clientLocal = local; app._gameClock = new ClientFrameClock(source, local.gameTicks());
    app._render = () => {}; app._startFrameLoop = () => {};
    milliseconds = 1025; app._onFrame(25); const frame = app._gameClock.time();
    assert.deepEqual(frame, { game: 0.025, wall: 0.025, delta: 0.025, number: 1 });
    milliseconds = 1030; document.hidden = true; app._onVisibilityChange(); app._onVisibilityChange();
    assert.equal(app._gameClock.paused(), true);
    milliseconds = 2030; document.hidden = false; app._onVisibilityChange(); app._onVisibilityChange();
    assert.equal(app._gameClock.paused(), false);
    milliseconds = 2040; app._onFrame(1040);
    assert.equal(app._gameClock.time(), frame); assert.ok(Math.abs(frame.game - 0.04) < 1e-12);
    assert.equal(frame.wall, 1.04); assert.equal(frame.number, 2);
});

// 탭 복귀는 이미 다른 이유로 걸린 게임 정지까지 해제하지 않는다.
test('Netstorm Client 시계: 기존 중첩 정지 보존', () => {
    let milliseconds = 0;
    const source = new ClientTimeSource({ hostMilliseconds: () => milliseconds, hostUnixSeconds: () => 1700000000 });
    const app = new Netstorm(), local = new ClientLocalStateGlobals(), document = { hidden: false };
    app._root = { ownerDocument: document }; app._clientLocal = local; app._gameClock = new ClientFrameClock(source, local.gameTicks()); app._startFrameLoop = () => {};
    app._gameClock.pushGamePause(); document.hidden = true; app._onVisibilityChange();
    milliseconds = 1000; document.hidden = false; app._onVisibilityChange();
    assert.equal(app._gameClock.paused(), true); app._gameClock.popGamePause();
    assert.equal(app._gameClock.paused(), false); assert.equal(app._gameClock.gameSeconds(), 0);
});
