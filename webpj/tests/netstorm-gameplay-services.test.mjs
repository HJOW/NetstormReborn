// 앱의 미션 소유권·자산 장치가 현재 Client 수명을 사용하는지 검사한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { Netstorm } from '../www/netstorm.js';
import { InputQueue } from '../www/core/input-queue.js';

// 설정은 호출마다 현재 객체를 읽고 생산 갱신은 숨겨진 실제 Gump에도 전달한다.
test('Netstorm 미션 장치: 현재 설정과 생산 갱신', () => {
    const app = new Netstorm(), services = app._campaignGameplayServices();
    assert.throws(() => services.ownership.themeMode(), /설정 수명 누락/);
    assert.throws(() => services.ownership.refreshProduction(), /Gump 수명 누락/);
    app._initializeClientRuntime(false); app._layout = { logicalWidth: 100, logicalHeight: 80 }; app._initializeClientGump();
    const events = [], gump = app._clientGump;
    const child = gump.createBase(0, 10, 10, 0x800, (event, first, second) => { events.push([event, first, second]); return 1; });
    gump.attach(child, 0, 0); gump.node(child).flags &= ~1;
    try {
        app._gameData = { readInt: (key, fallback) => ({ value: key === 'theammode' ? -1 : fallback }) };
        assert.equal(services.ownership.themeMode(), 0xffffffff);
        app._gameData = { readInt: (_key, fallback) => ({ value: fallback }) };
        assert.equal(services.ownership.themeMode(), 1);
        services.ownership.refreshProduction(); services.assets.postPop.refreshProduction();
        assert.deepEqual(events, [[0x02000003, 0, 0], [0x02000003, 0, 0]]);
    } finally { gump.dispose(); }
    assert.throws(() => services.ownership.refreshProduction(), /반납한 Client Gump/);
});

// 개발자 명령과 자산 명령은 현재 입력 고리를 사용하며 교체한 큐를 다시 조회한다.
test('Netstorm 미션 장치: 현재 입력 큐와 명령 정수 폭', () => {
    const app = new Netstorm(), services = app._campaignGameplayServices();
    app._clientInputQueue = null; assert.throws(() => services.developerQueue(1, 2, 3), /입력 큐 수명 누락/);
    const first = new InputQueue(), second = new InputQueue(); app._clientInputQueue = first;
    services.developerQueue(0x03000075, -1, 2); app._clientInputQueue = second;
    services.assets.queueCommand(0x02000042, 0xffffffff, -2);
    assert.deepEqual(first.pop(false), { code: 0x03000075, x: -1, y: 2 });
    assert.deepEqual(second.pop(false), { code: 0x02000042, x: -1, y: -2 });
});

// 카메라는 현재 Screen을, 표면은 현재 Renderer의 전체·부분 변경 표를 사용한다.
test('Netstorm 미션 장치: 화면 크기와 교체된 실제 변경 표', () => {
    const app = new Netstorm(), services = app._campaignGameplayServices();
    assert.throws(() => services.cameraScreenSize(), /Screen 수명 누락/);
    assert.throws(() => services.surfaceDisplay.suppressed(), /Renderer 수명 누락/);
    assert.throws(() => services.surfaceDisplay.invalidate({ left: 1, top: 1, right: 5, bottom: 5 }, 0), /Renderer 수명 누락/);
    app._clientPaletteScreen = { width: 1024, height: 768 }; app._initializeClientRenderer(1024, 768);
    const first = app._clientRenderer;
    assert.deepEqual(services.cameraScreenSize(), { x: 1024, y: 768 }); assert.equal(services.surfaceDisplay.suppressed(), true);
    first.clearDirty(); app._displayDirty = false;
    services.surfaceDisplay.invalidate({ left: 1, top: 2, right: 5, bottom: 7 }, 4);
    assert.equal(app._displayDirty, true); assert.deepEqual(first.cameraDirtyEntries(), [{ rect: { left: 1, top: 2, right: 5, bottom: 7 }, flags: 4 }]);
    app._clientPaletteScreen = { width: 1366, height: 768 }; app._initializeClientRenderer(1366, 768);
    const second = app._clientRenderer; assert.notEqual(second, first);
    assert.deepEqual(services.cameraScreenSize(), { x: 1366, y: 768 }); assert.equal(services.surfaceDisplay.suppressed(), true);
    second.clearDirty(); services.surfaceDisplay.invalidate({ left: 1025, top: 3, right: 1030, bottom: 9 }, 0);
    assert.equal(second.cameraDirtyEntries().length, 1); assert.equal(first.cameraDirtyEntries().length, 1);
});
