// 앱의 Login 설정·시계·Player 관문이 현재 Client 소유자를 사용하는지 검사한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { Netstorm } from '../www/netstorm.js';
import { ClientFrameClock } from '../www/core/client-frame-clock.js';
import { ClientTimeSource } from '../www/core/client-time-source.js';

// 지정 포트는 장기 전역, 다른 키는 기본값을 보존하는 설정에서 읽는다.
test('Netstorm Login 장치: 현재 설정·시계·표시 갱신', () => {
    const app = new Netstorm(); app._initializeClientRuntime(false);
    let milliseconds = 1000;
    app._gameClock = new ClientFrameClock(new ClientTimeSource({ hostMilliseconds: () => milliseconds, hostUnixSeconds: () => 1700000000 }), app._clientLocal.gameTicks());
    const values = new Map([['timeout', 47]]);
    app._gameData = { readInt: (key, fallback) => ({ value: values.has(key) ? values.get(key) : fallback }) };
    const login = app._campaignLoginServices();
    app._clientGameplay.gameServerPort().value = 0xfffffffe;
    assert.equal(login.configInt('gameServerPort', 30), -2); assert.equal(login.configInt('timeout', 30), 47); assert.equal(login.configInt('missing', 30), 30);
    values.set('timeout', 18); assert.equal(login.configInt('timeout', 30), 18);
    milliseconds = 1125; assert.equal(login.clock(), 1125); assert.equal(login.wallSeconds(), 0.125); assert.equal(login.gameSeconds(), 0.125);
    app._gameClock.pushGamePause(); milliseconds = 2125; assert.equal(login.wallSeconds(), 1.125); assert.equal(login.gameSeconds(), 0.125);
    app._mainMenu = { dirty: false }; app._displayDirty = false; login.refresh();
    assert.equal(app._mainMenu.dirty, true); assert.equal(app._displayDirty, true);
});

// 원본 전투 완료 관문은 단독 미션·비전투·대본 중단에서 Session을 조회하지 않는다.
test('Netstorm Login 장치: Player UI의 현재 전투 완료 관문', () => {
    const app = new Netstorm(); app._initializeClientRuntime(false);
    const login = app._campaignLoginServices();
    assert.throws(() => login.playerUiBusy(), /Client 수명/);
    app._surfaceStorage = { ownerMode: { battle: false } }; app._clientGameplay.world.missionPost = false;
    app._clientLocal.scriptBreak().value = 0;
    assert.equal(login.playerUiBusy(), false);
    app._surfaceStorage.ownerMode.battle = true; app._clientGameplay.world.missionPost = true;
    assert.equal(login.playerUiBusy(), false);
    app._clientGameplay.world.missionPost = false; app._clientLocal.scriptBreak().value = 1;
    assert.equal(login.playerUiBusy(), false);
    app._clientLocal.scriptBreak().value = 0; assert.throws(() => login.playerUiBusy(), /World 누락/);
    let finished = false; app._campaignWorld = { session: () => ({ battleFinished: () => finished }) };
    assert.equal(login.playerUiBusy(), false); finished = true; assert.equal(login.playerUiBusy(), true);
});

// 초기화는 호출 시점의 논리 화면·세계 크기와 같은 Client의 두 전역 상자를 사용한다.
test('Netstorm Login 장치: 실제 화면과 Client 카메라 초기화', () => {
    const app = new Netstorm(), login = app._campaignLoginServices();
    assert.throws(() => login.initializeRenderer(), /수명 누락/);
    app._initializeClientRuntime(false); app._layout = { logicalWidth: 1024, logicalHeight: 768 };
    const camera = app._clientGameplay.camera(), redraw = app._clientKencloud.worldRedraw(), revision = app._clientGameplay.mapRevision();
    revision.value = 0xffffffff; redraw.value = 8;
    login.initializeRenderer();
    assert.deepEqual([...camera.target().limits, ...camera.target().pixels, camera.target().zoom], [3072, 2048, 0, 0, 65536]);
    assert.deepEqual([redraw.value, revision.value], [1, 0]);
    assert.deepEqual(camera.snapshot().bytes, camera.rendererView().bytes);
    app._layout = { logicalWidth: 1366, logicalHeight: 768 }; app._clientGameplay.worldCells().value = 128;
    camera.requestPixels(20, 30); redraw.value = 0; login.initializeRenderer();
    assert.deepEqual([...camera.target().limits, ...camera.target().pixels], [682, 640, 0, 0]);
    assert.deepEqual([redraw.value, revision.value], [1, 1]);
    app._layout = { logicalWidth: 0, logicalHeight: 768 };
    assert.throws(() => login.initializeRenderer(), /화면 크기/); assert.equal(revision.value, 1);
    app._layout = { logicalHeight: 768 };
    assert.throws(() => login.initializeRenderer(), /정수 크기/); assert.equal(revision.value, 1);
});

// 현재 HUD 명령과 모드는 실제 Gump의 숨김 노드 및 캡처에 원본 순서로 전달된다.
test('Netstorm Login 장치: 같은 Client의 HUD 모드와 Gump 수명', () => {
    const app = new Netstorm(), login = app._campaignLoginServices();
    assert.throws(() => login.uiMode(2), /Gump 수명 누락/);
    app._initializeClientRuntime(false); app._layout = { logicalWidth: 100, logicalHeight: 80 }; app._initializeClientGump();
    const gump = app._clientGump, events = [];
    const child = gump.createBase(0, 10, 10, 0x800, (event, first, second) => { events.push([event, first, second]); return 1; });
    gump.attach(child, 0, 0); gump.node(child).flags &= ~1;
    try {
        app._clientLocal.hudModeCommand().value = 0x02000042;
        login.uiMode(2); assert.deepEqual(events, [[0x02000042, 2, 0]]);
        app._clientLocal.hudModeCommand().value = 0x02000043;
        gump.input().capture(child); login.uiMode(3);
        assert.deepEqual(events.at(-1), [0x02000043, 3, 0]); assert.equal(gump.lifetimeState().focus, 0);
    } finally { gump.dispose(); }
    assert.throws(() => login.uiMode(2), /반납한 Client Gump/);
});
