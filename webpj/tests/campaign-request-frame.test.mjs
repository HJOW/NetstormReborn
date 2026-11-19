// 미션 요청 프레임이 실제 ClientState 큐의 Poll 전 수명과 Fort 이미지를 보존하는지 검사한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { ClientState } from '../www/core/client-state.js';
import { runCampaignRequestFrame } from '../www/core/campaign-request-frame.js';
import { AppState, Netstorm } from '../www/netstorm.js';
import { GameAssets } from '../www/assets/game-assets.js';
import { GamePalette } from '../www/assets/palette.js';
import { GameData } from '../www/assets/game-data.js';
import { buildMenuResources } from '../www/assets/menu-assets.js';
import { MainMenuScreen } from '../www/ui/main-menu.js';
import { IndexedSurface } from '../www/render/indexed-surface.js';
import { IndexedPresenter } from '../www/render/presenter.js';
import { AudioSystem } from '../www/audio/audio-system.js';
import { Settings } from '../www/platform/settings.js';
import { CampaignRenderCollection } from '../www/render/campaign-render-collection.js';
import { CampaignRendererDrawing } from '../www/render/campaign-renderer-drawing.js';
import { RawContainedFinder } from '../www/core/raw-contained-finder.js';
import { ClientInventoryGrid } from '../www/ui/client-inventory-grid.js';
import { InventoryItemNode } from '../www/ui/inventory-gump.js';
import { DATA_ROOT, readData, readDataMap } from './helpers.mjs';
import { realCaptureAssets } from './capture-assets-support.mjs';

// 같은 큐와 현재 이미지를 사용하는 Guide 경계다.
function guideFixture() {
    const pending = new ClientState(), current = { value: { fort: 'BridgeTheGap', consumed: false } };
    return { pending, current, guide: { pending: () => pending, fortRequest: () => current.value } };
}

// Poll이 만든 요청은 다음 프레임에 넘기고 기존 요청은 Poll 뒤 정확히 한 번 실행한다.
test('미션 요청 프레임: Poll 전 요청만 실제 이미지로 소비한다', () => {
    const { pending, current, guide } = guideFixture(), events = [];
    const load = (request) => { assert.equal(request, current.value); events.push('load'); };
    assert.equal(runCampaignRequestFrame(guide, () => { events.push('poll'); pending.post({ command: 'LoadMissionFort', argument: 'BridgeTheGap' }); }, load), false);
    assert.equal(pending.pending(), true);
    assert.equal(runCampaignRequestFrame(guide, () => events.push('poll'), load), true);
    assert.equal(pending.pending(), false); assert.deepEqual(events, ['poll', 'poll', 'load']);
    assert.equal(runCampaignRequestFrame(null, () => events.push('no-guide'), load), false);
    assert.equal(events.at(-1), 'no-guide');
});

// Poll이 기존 요청을 취소하거나 교체하면 새 요청을 현재 프레임에 소비하지 않는다.
test('미션 요청 프레임: 취소·교체와 Poll 실패의 실제 큐 수명을 지킨다', () => {
    const { pending, guide } = guideFixture(), first = { command: 'LoadMissionFort', argument: 'BridgeTheGap' };
    pending.post(first);
    assert.throws(() => runCampaignRequestFrame(guide, () => { throw new Error('Poll 실패'); }, () => {}), /Poll 실패/);
    assert.equal(pending.pending(), true);
    assert.equal(runCampaignRequestFrame(guide, () => { pending.cancel('LoadMissionFort'); pending.post(first); }, () => { throw new Error('조기 실행'); }), false);
    assert.equal(pending.pending(), true);
    assert.equal(runCampaignRequestFrame(guide, () => { pending.cancel('LoadMissionFort'); }, () => { throw new Error('취소 실행'); }), false);
    assert.equal(pending.pending(), false);
});

// 이미 소비된 이미지·다른 이름·지원하지 않는 전환은 실제 설치 장치를 부르지 않는다.
test('미션 요청 프레임: 현재 이미지 불일치와 미연결 전환을 거부한다', () => {
    const { pending, current, guide } = guideFixture(); let calls = 0;
    // 서로 다른 세 잘못된 이미지는 매번 새 큐 요청을 소비한다.
    for (const value of [null, { fort: '다른 Fort', consumed: false }, { fort: 'BridgeTheGap', consumed: true }]) {
        current.value = value; pending.post({ command: 'LoadMissionFort', argument: 'BridgeTheGap' });
        assert.throws(() => runCampaignRequestFrame(guide, () => {}, () => { calls += 1; }), /이미지 수명 불일치/);
    }
    pending.post({ command: 'LoadMissionWorld', argument: '' });
    assert.throws(() => runCampaignRequestFrame(guide, () => {}, () => { calls += 1; }), /다른 전환 미연결/);
    current.value = { fort: 'BridgeTheGap', consumed: false }; pending.post({ command: 'LoadMissionFort', argument: 'BridgeTheGap' });
    assert.throws(() => runCampaignRequestFrame(guide, () => {}, null), /설치 장치 미연결/);
    assert.equal(calls, 0); assert.equal(pending.pending(), false);
});

// 실제 Tutorial1 Guide가 예약한 요청을 앱의 같은 State7·World·로컬 로그인에 전달한다.
test('미션 요청 프레임: 실제 Tutorial1 요청이 앱 Session의 로그인까지 도달한다', async () => {
    const assets = realCaptureAssets(), game = new GameAssets(assets.table, assets.database, GamePalette.parse(readData('d/GIFCLOUD.COL')));
    const manifest = JSON.parse(fs.readFileSync(path.join(DATA_ROOT, 'manifest.json'), 'utf8'));
    const files = readDataMap(manifest.files.filter((entry) => entry.bundle === 'menu').map((entry) => entry.path));
    const data = GameData.build({ files, language: 'english' });
    const app = new Netstorm(); app._initializeClientRuntime(false); app._gameData = data;
    // 실제 미션 재생기와 Web Audio 경계로 로그인 경로를 연결한다.
    const node = () => ({ connect() {}, disconnect() {} });
    const context = { destination: {}, createGain: () => ({ ...node(), gain: { value: 0 } }), createChannelSplitter: node, createChannelMerger: node,
        createBufferSource: () => ({ ...node(), start() {}, stop() {} }), decodeAudioData: async (bytes) => ({ bytes: bytes.byteLength }) };
    app._settings = new Settings(); app._audio = new AudioSystem(context);
    app._assetLoader = { manifest: { files: manifest.files.filter((entry) => entry.path === 'sound/button.wav') },
        loadFile: async (entry) => Uint8Array.from(fs.readFileSync(path.join(DATA_ROOT, ...entry.path.split('/')))) };
    app._root = { ownerDocument: { title: 'before' } }; app._resources = buildMenuResources(files);
    app._layout = { logicalWidth: 1024, logicalHeight: 768, visible: false }; app._surface = new IndexedSurface(1024, 768); app._presenter = new IndexedPresenter(null);
    app._mainMenu = new MainMenuScreen({ resources: app._resources }); app._mainMenu.resize(1024, 768); app._initializeClientGump();
    app._initializeClientWorldStorage(game, data.files); app._syncClientCloudOptions(); app._initializeClientPalette(game); app._initializeClientPaletteCache(game); app._initializeClientGuide();
    app._canvas = {}; app._render = () => {}; app._startFrameLoop = () => {}; app._applyCursor = () => {};
    const guide = app._clientGuide;
    try {
        // 실제 Campaign 목록의 Early Missions와 첫 Tutorial 행을 눌러 Guide를 시작한다.
        app._setState(AppState.MAIN_MENU); assert.equal(app._openTell('UCampaign'), true);
        const campaignPoint = app._mainMenu.controlPoint('Early Missions'); assert.ok(campaignPoint);
        app._mainMenu.pointerDown(campaignPoint.x, campaignPoint.y);
        const firstMission = app._mainMenu.dialog.menu.items.find((item) => item.command?.name === 'MissionBegin' && item.command.argument === 'Tutorial1');
        assert.ok(firstMission); const missionPoint = app._mainMenu.controlPoint(firstMission.label); assert.ok(missionPoint);
        app._mainMenu.pointerDown(missionPoint.x, missionPoint.y); assert.ok(guide.current());
        const startedRuntime = guide.current().runtime;
        assert.equal(app._beginCampaignFromDialog('Tutorial2'), false); assert.equal(app._beginCampaignFromDialog('Tutorial1'), false);
        assert.equal(guide.current().runtime, startedRuntime);
        assert.ok(guide.current().script.section('A.').includes('NetStorm!')); assert.equal(app._openTell('A.'), true);
        assert.ok(app._mainMenu.dialog); app._mainMenu.setDialog(null);
        guide.run(); guide.run();
        const request = guide.fortRequest(); assert.equal(request.fort, 'BridgeTheGap'); assert.equal(guide.pending().pending(), true);
        app._onFrame(0);
        assert.equal(request.consumed, false); assert.equal(guide.pending().pending(), true); assert.ok(app._clientSoundPrepare);
        await app._clientSoundPrepare; app._onFrame(16);
        assert.equal(request.consumed, true); assert.equal(guide.pending().pending(), false);
        assert.throws(() => app._prepareCampaignMissionStart(), /수명 오류/);
        assert.ok(app._missionLoadingWindow); assert.equal(app._mainMenu.dialog, app._missionLoadingWindow);
        app._mainMenu.render(new IndexedSurface(1024, 768));
        const session = app._campaignWorld.session();
        const notices = [], gump = app._clientGump;
        const parent = gump.createBase(0, 20, 20, 1, (...event) => { notices.push(event.slice(0, 3)); return 0; });
        const child = gump.createBase(0, 10, 10, 1, () => 0); gump.attach(parent, 0, 0); gump.attach(child, 0, 0, parent); app._clientChatWindows.window.current = child;
        // 실제 패킷·State는 앱 프레임에서만 진행하고 준비 전에는 Guide를 기다린다.
        for (let frame = 0; frame < 10 && !session.localConnectionReady(); frame += 1) app._onFrame(frame * 16);
        assert.equal(session.localConnectionReady(), true); assert.equal(session.world().storage, app._surfaceStorage);
        const runtime = guide.current().runtime; assert.equal(app._campaignWorld.connecting, false);
        assert.ok(runtime.start && runtime.tutorial); assert.equal(runtime.state.sequence, 4); assert.equal(runtime.gate.started, 1);
        // 실제 경로 발소리는 같은 Client 재생기에 원본 위치와 반복·우선순위를 전달한다.
        const player = app._clientSoundPlayer, originalPlay = player.playOnceAt, steps = [];
        try { player.playOnceAt = (...args) => steps.push(args); app._campaignGameplayServices().motion.path.stepSound(42, 10.5, 20.25, 7); }
        finally { player.playOnceAt = originalPlay; }
        assert.deepEqual(steps, [[10.5, 20.25, 7, 0, 0]]);
        // 실제 Player 정기 진단은 같은 Client 로그 버퍼를 사용한다.
        app._campaignGameplayServices().player.log(1, 'Player test');
        assert.ok(app._clientTrace.log.buffer.some((byte) => byte !== 0));
        // 미구현 사건을 추월하지 않고 맨 앞 이동 통지만 현재 Guide와 Gump에 전달한다.
        const inputQueue = app._clientInputQueue; inputQueue.pushCommand(0x02000050, 1, 2); inputQueue.pushCommand(0x02000044, 3, 4);
        app._runCampaignInputFrame(); assert.equal(inputQueue.size, 2); assert.equal(inputQueue.pop(false).code, 0x02000050);
        app._runCampaignInputFrame(); assert.equal(inputQueue.empty, true);
        // 실제 앱 포인터 폴링은 World의 현재 마우스 좌표를 입력 큐 처리 전에 갱신한다.
        const pointerState = app._input.pointer, previousPointer = { ...pointerState };
        Object.assign(pointerState, { inside: true, x: 123, y: 234, buttons: 0 }); app._runCampaignInputFrame();
        assert.deepEqual(app._clientGameplay.selection.mouse, { x: 123, y: 234 }); Object.assign(pointerState, previousPointer);
        assert.ok(notices.some(([code, first, second]) => code === 0x02000088 && first === 0 && second === 0));
        assert.equal(session.sp(session.localOwner()), Number(guide.current().script.get('myStartMoney')));
        const camera = app._clientGameplay.camera(); assert.deepEqual(camera.rendererView().pixels, camera.snapshot().pixels);
        const soundView = app._campaignSoundView(); assert.equal(soundView.cameraX, camera.snapshot().pixels.x); assert.equal(soundView.cameraY, camera.snapshot().pixels.y);
        assert.deepEqual({ left: soundView.left, top: soundView.top, right: soundView.right, bottom: soundView.bottom }, app._campaignWorld.mapView());
        assert.equal(app._clientRenderer.cameraRedraw.value, 1);
        // 로그인된 같은 Session의 사제 선택과 해제를 실제 입력 서비스에 전달한다.
        assert.ok(app._campaignUserInput); assert.throws(() => app._prepareCampaignUserInput(), /수명 오류/);
        const priest = session.findPriestParticipation(session.localOwner()); assert.ok(priest);
        app._campaignWorld.select(priest); assert.equal(app._campaignWorld.selected(), priest);
        app._campaignWorld.resetSelection(); assert.equal(app._campaignWorld.selected(), 0);
        const priestSlot = session.core().pool.slot(priest), priestRaw = new DataView(priestSlot.buffer, priestSlot.byteOffset, priestSlot.byteLength);
        const priestPoint = camera.snapshot().project(priestRaw.getFloat32(14, true), priestRaw.getFloat32(18, true));
        app._campaignUserInput.event({ code: 0x00010000, x: priestPoint.x - 16, y: priestPoint.y - 11 });
        assert.equal(app._campaignWorld.selected(), priest); app._campaignWorld.resetSelection();
        // 같은 실제 앱의 레이아웃 변경이 새 Screen·Renderer와 현재 World 지도 범위에 순서대로 반영된다.
        app._root.clientWidth = 1366; app._root.clientHeight = 768; app._aspect = '16:9'; app._canvas = { style: {} }; app._applyCursor = () => {};
        const beforeRenderer = app._clientRenderer, frameGlobals = app._clientRendererFrame, cameraDevice = app._campaignCameraDevice(); frameGlobals.scan()[3] = 17; frameGlobals.cloudDirtyCount().value = 2;
        app._applyLayout();
        assert.equal(app._clientPaletteScreen.width, 1366); assert.notEqual(app._clientRenderer, beforeRenderer);
        assert.equal(app._clientRendererFrame, frameGlobals); assert.equal(frameGlobals.scan()[3], 17); assert.equal(frameGlobals.cloudDirtyCount().value, 2);
        assert.equal(app._campaignCameraDevice(), cameraDevice);
        assert.deepEqual(app._campaignWorld.mapView(), { left: 0, top: 28, right: 1366, bottom: 768 });
        app._campaignWorld.takeChanged(); app._campaignWorld.cameraApplied(); assert.equal(app._campaignWorld.takeChanged(), true);
        // 현재 Screen의 모드 변경과 강제 갱신은 같은 구름 큐에 각 한 칸씩 넣고 실제 Renderer의 이전 dirty를 전체 화면으로 교체한다.
        const redraw = app._clientKencloud.worldRedraw(); redraw.value = 1;
        app._clientRenderer.clearDirty(); app._clientRenderer.invalidate({ left: 10, top: 20, right: 30, bottom: 40 });
        app._prepareCampaignRendererFrame();
        assert.equal(frameGlobals.scan()[18], app._clientPaletteScreen.flags); assert.equal(frameGlobals.cloudDirtyCount().value, 0);
        assert.deepEqual(frameGlobals.cloudRectangles()[2], { left: 0, top: 0, right: 1366, bottom: 768 });
        assert.deepEqual(frameGlobals.cloudRectangles()[3], { left: 0, top: 0, right: 1366, bottom: 768 });
        assert.deepEqual(app._clientRenderer.cameraDirtyEntries(), [{ rect: { left: 0, top: 0, right: 1366, bottom: 768 }, flags: 0 }]);
        assert.equal(redraw.value, 1); redraw.value = 0;
        // 실제 월드 객체의 현재 풀 좌표와 같은 카메라 snapshot으로 부착 Gump를 투영한다.
        const object = app._campaignWorld.objects()[0]; assert.ok(object);
        const attached = gump.createBase(0, 10, 10, 1, () => 0); gump.attach(attached, 3, 4);
        const attachedNode = gump.node(attached); attachedNode.flags |= 0x2000; attachedNode.object = object.id;
        const slot = session.core().pool.slot(object.id), raw = new DataView(slot.buffer, slot.byteOffset, slot.byteLength);
        const snap = camera.snapshot(), savedCamera = snap.bytes.slice(), projected = snap.project(raw.getFloat32(14, true), raw.getFloat32(18, true));
        assert.deepEqual(gump.collection().position(attached), { x: projected.x + 3, y: projected.y + 4 });
        const origin = snap.pixels; snap.setPixels(origin.x + 17, origin.y + 11);
        assert.deepEqual(gump.collection().position(attached), { x: projected.x + 3 - 17, y: projected.y + 4 - 11 });
        snap.bytes.set(savedCamera); gump.remove(attached);
        // 현재 Tutorial1의 실제 지면 묶음·객체·입자·커서를 같은 원본 표시 목록에 모은다.
        const collectedUi = [], collector = new CampaignRenderCollection(app._campaignWorld, camera.snapshot(), 1366, 768, (clip, output, limit) => {
            collectedUi.push({ clip, limit }); const view = new DataView(output.buffer, output.byteOffset, output.byteLength);
            view.setFloat32(0, 1, true); view.setFloat32(4, 2, true); view.setUint32(8, 123, true); output[14] = 2; return 1;
        });
        const pixels = new Uint8Array(1366 * 768), fullClip = { left: 0, top: 0, right: 1366, bottom: 768 };
        assert.throws(() => collector.submit(pixels, 1366, 768, fullClip), /수집 누락/);
        const all = collector.collect({ left: 0, top: 0, right: 1366, bottom: 768 }, 0), kinds = [];
        for (let at = 14; at < all.entries.length; at += 16) kinds.push(all.entries[at]);
        assert.equal(kinds[0], 2); assert.ok(kinds.includes(1)); assert.ok(kinds.includes(0));
        const groupAt = kinds.indexOf(1) * 16, group = collector.group(new DataView(all.entries.buffer, all.entries.byteOffset).getUint32(groupAt + 8, true));
        assert.ok(group.count.value > 0); assert.equal(collectedUi.length, 1);
        const renderedUi = []; collector.submit(pixels, 1366, 768, fullClip, { drawUi: (token) => renderedUi.push(token) });
        assert.deepEqual(renderedUi, [123]); assert.ok(pixels.some((value) => value !== 0));
        const hiddenPixels = new Uint8Array(pixels.length); collector.collect(fullClip, 0);
        collector.submit(hiddenPixels, 1366, 768, fullClip, { drawUi: () => {}, hideGroups: () => true }); assert.notDeepEqual(hiddenPixels, pixels);
        const uiOnly = collector.collect({ left: 0, top: 0, right: 20, bottom: 20 }, 4);
        assert.equal(uiOnly.entries.length, 16); assert.equal(uiOnly.entries[14], 2); assert.equal(uiOnly.covered, false);
        assert.throws(() => collector.submit(pixels, 1366, 768, fullClip), /분배 콜백 누락/);
        // 실제 Screen DIB와 구름·Renderer·표시기를 같은 미션 화면에 연결한다.
        const screen = app._clientPaletteScreen, renderer = app._clientRenderer;
        const drawing = new CampaignRendererDrawing({ world: app._campaignWorld, screen, camera: camera.snapshot(), globals: frameGlobals,
            clouds: app._clientKencloud, files: data.files, config: data, random: app._clientRandom.game,
            denominator: camera.denominator(), tint: () => 0 });
        app._layout = { ...app._layout, visible: false }; redraw.value = 1; const beforePresent = renderer.presentCount;
        // 실제 Client 트리의 기본 Gump 출력기가 같은 Renderer kind 2와 DIB를 통과한다.
        let uiDraws = 0; gump.bindDraw(parent, (_token, target) => { uiDraws += 1; target.fill({ left: 1, top: 1, right: 5, bottom: 5 }, 241); });
        app._drawCampaignRendererFrame();
        assert.ok(renderer.presentCount > beforePresent); assert.equal(redraw.value, 0); assert.equal(renderer.cameraDirtyEntries().length, 0);
        assert.ok(uiDraws > 0); assert.equal(app._surface.pixels[1366 + 1], 241); gump.bindDraw(parent, null);
        assert.ok(app._surface.pixels.some((value) => value !== 0));
        // 빈 지면의 구름은 지도 영역에만, UI 전용 dirty는 배경을 건드리지 않는다.
        screen.clear(fullClip, 55); drawing.collect(fullClip, 0); const sky = screen.lock();
        try { drawing.background(fullClip, 0, 0); assert.ok(sky.subarray(screen.pitch * 28).some((value) => value !== 55)); assert.equal(sky[0], 55); }
        finally { screen.unlock(); }
        screen.clear(fullClip, 55); drawing.collect(fullClip, 4); const uiPixels = screen.lock(), beforeUi = uiPixels.slice();
        try { drawing.background(fullClip, 4, 0); assert.deepEqual(uiPixels, beforeUi); } finally { screen.unlock(); }
        // 시차 경로는 분모 객체의 현재 current 칸을 출력마다 다시 읽는다.
        const movingCamera = camera.snapshot().copy(); movingCamera.setPixels(90, 60);
        const parallaxScreen = new Proxy(screen, { get(target, key) { if (key === 'flags') return 0x18; const value = Reflect.get(target, key, target); return typeof value === 'function' ? value.bind(target) : value; } });
        const offsets = [], divisor = { current: 3 };
        const parallax = new CampaignRendererDrawing({ world: app._campaignWorld, screen: parallaxScreen, camera: movingCamera, globals: frameGlobals,
            clouds: { draw: (_screen, area) => offsets.push(area.slice(4)) }, files: data.files, config: data, random: app._clientRandom.game, denominator: divisor, tint: () => 0 });
        parallax.collect(fullClip, 0); parallax.background(fullClip, 0, 0); divisor.current = 5; parallax.background(fullClip, 0, 0);
        assert.deepEqual(offsets, [[30, 20], [18, 12]]);
        assert.throws(() => app._prepareCampaignMissionStart(), /수명 오류/); assert.throws(() => app._prepareCampaignTutorial(), /수명 오류/);
        assert.doesNotThrow(() => session.checkMissionOutcome());
        const outcome = app._campaignGameplayServices().missionOutcome;
        assert.equal(outcome.section('A.'), true); assert.equal(outcome.tellBlocked(), true); app._mainMenu.setDialog(null);
        assert.equal(outcome.tellBlocked(), false); assert.equal(outcome.section('없는 안내'), false); assert.equal(outcome.demo(), runtime.gate.demo);
        // 실제 앱의 같은 Kernel은 안내와 정지 중에도 프레임마다 정확히 한 번 실행된다.
        let visits = 0; const probe = app._clientKernel.add({ runFrame: () => { visits += 1; } });
        try {
            for (let frame = 0; frame < 15 && !app._mainMenu.dialog; frame += 1) { const before = visits; app._onFrame(200 + frame * 16); assert.equal(visits, before + 1); }
            assert.ok(app._mainMenu.dialog); assert.equal(runtime.gate.scriptPaused, 1); assert.equal(app._gameClock.paused(), true);
            const pausedVisits = visits; app._onFrame(480); assert.equal(visits, pausedVisits + 1);
            app._mainMenu.setDialog(null); const beforeResume = visits; for (let frame = 0; frame < 7; frame += 1) app._onFrame(500 + frame * 16);
            assert.equal(visits, beforeResume + 7); assert.equal(runtime.gate.scriptPaused, 0); assert.equal(app._gameClock.paused(), false);
        } finally { app._clientKernel.remove(probe); }
        // 실제 생산 유지 장치가 현재 기술 내용물에 다리를 공급한다.
        const production = app._campaignProduction, technology = session.currentTechnologyParent(); assert.ok(production); assert.ok(technology);
        const bridges = () => { const finder = new RawContainedFinder(session.core().pool, session.productionFinder()); let count = 0;
            // 같은 기술 부모의 실제 내용물만 센다.
            for (let sid = finder.begin(technology); sid; sid = finder.next()) { const raw = session.core().pool.slot(sid); if ((raw[20] | (raw[21] << 8)) & 0x40) count += 1; }
            return count; };
        const previousBridges = bridges(); production.panel.bridgeUntil = 0; app._runCampaignProductionFrame();
        assert.equal(production.panel.slots, session.battleOption(1) * 2 + 2); assert.equal(bridges(), Math.min(previousBridges + 1, production.panel.slots));
        assert.throws(() => app._prepareCampaignProduction(), /수명 오류/);
        // 실제 앱 프레임은 월드를 표시하고 Tell을 그 위에 합성하며 닫은 뒤 DIB 원본을 복구한다.
        let presentations = 0; app._presenter.present = () => { presentations += 1; };
        app._ctx = {}; app._layout = { ...app._layout, visible: true }; app._render = Netstorm.prototype._render.bind(app);
        app._mainMenu.setDialog(null); app._onFrame(700); assert.ok(presentations > 0);
        assert.equal(app._openTell('A.'), true); app._onFrame(716); assert.ok(app._mainMenu.dialog);
        // DIB pitch에는 행 패딩이 있을 수 있으므로 실제 화면 열만 비교한다.
        const differsFromScreen = () => { const bytes = screen.lock();
            try { for (let y = 0; y < screen.height; y += 1) for (let x = 0; x < screen.width; x += 1) if (app._surface.pixels[y * screen.width + x] !== bytes[y * screen.pitch + x]) return true; return false; }
            finally { screen.unlock(); } };
        assert.equal(differsFromScreen(), true);
        app._mainMenu.setDialog(null); app._onFrame(732);
        assert.equal(differsFromScreen(), false); assert.equal(app._mainMenu.dirty, false);
        // 미션에서는 가려진 메뉴 돌 버튼을 호출하지 않고 열린 Tell 창에는 포인터를 전달한다.
        let downs = 0, ups = 0; app._mainMenu.pointerDown = () => { downs += 1; }; app._mainMenu.pointerUp = () => { ups += 1; };
        app._canvas = { focus() {}, setPointerCapture() {}, getBoundingClientRect: () => ({ left: 0, top: 0, width: 1366, height: 768 }) };
        app._retryAudioOnGesture = () => {}; const pointerEvent = { button: 0, buttons: 1, pointerId: 1, clientX: 100, clientY: 100 };
        app._onPointerDown(pointerEvent); app._onPointerUp(pointerEvent); assert.equal(downs, 0); assert.equal(ups, 0);
        assert.equal(app._openTell('A.'), true); app._onPointerDown(pointerEvent); app._onPointerUp(pointerEvent); assert.equal(downs, 1); assert.equal(ups, 1);
        // 실제 기술 내용물의 다리를 같은 앱 커서에 집고 취소해 원본 위치·글꼴·선택 수명을 확인한다.
        app._mainMenu.setDialog(null); Object.assign(app._input.pointer, { inside: true, x: 400, y: 300, buttons: 0 });
        const finder = new RawContainedFinder(session.core().pool, session.productionFinder()); let bridge = 0;
        // 첫 실제 다리 슬롯을 고른다.
        for (let sid = finder.begin(technology); sid; sid = finder.next()) { const raw = session.core().pool.slot(sid); if ((raw[20] | (raw[21] << 8)) & 0x40) { bridge = sid; break; } }
        assert.ok(bridge); const cursor = session.cursor(), drop = session.cursorDropSettings(); cursor.take(bridge, 0);
        assert.equal(drop.preview, bridge); assert.equal(drop.lockDepth, 0); assert.ok(cursor.lifecycle.previewCount > 0);
        const playerRecords = app._clientPlayers.records(); const temple = new DataView(playerRecords.buffer, playerRecords.byteOffset + session.localOwner() * 180 + 0x7c, 4).getUint32(0, true);
        const templeBytes = session.core().pool.slot(temple), templeRaw = new DataView(templeBytes.buffer, templeBytes.byteOffset, templeBytes.byteLength);
        const home = camera.snapshot().project(templeRaw.getFloat32(14, true), templeRaw.getFloat32(18, true)); let legal = null;
        // 실제 배치 판정으로 신전 주변의 유효한 화면점을 찾는다.
        for (let y = home.y - 150; y <= home.y + 150 && !legal; y += 11) {
            for (let x = home.x - 200; x <= home.x + 200; x += 16) {
                if (x < 0 || y < 28 || x >= 1366 || y >= 768) continue;
                cursor.update(x, y); if (drop.legal) { legal = { x, y }; break; }
            }
        }
        assert.ok(legal); const beforeConstruction = session.localConnectionPackets().length;
        const beforeBridges = session.world().objects().filter(({ object }) => object.type === session.core().bridgeState.bridgeType).length;
        app._campaignUserInput.event({ code: 0x00010000, ...legal });
        assert.equal(drop.preview, 0); assert.equal(drop.lockDepth, 2); session.pumpLocalConnection();
        const actualBridges = session.world().objects().filter(({ object }) => object.type === session.core().bridgeState.bridgeType);
        assert.deepEqual(session.localConnectionPackets().slice(beforeConstruction), [0x3e]); assert.ok(actualBridges.length > beforeBridges);
        // 실제 Session의 GumpForm이 같은 기술 SID에 부착되고 Gump 제거 시 반납된다.
        const bound = gump.createBase(0, 10, 10, 1, () => 0); gump.attach(bound, 200, 200); gump.bindObject(bound, technology);
        const form = gump.node(bound).form; assert.equal(session.gumpFormHost().parent(form), technology);
        assert.equal(session.gumpFormHost().owner(form), gump.node(bound).id);
        gump.remove(bound); assert.ok(session.core().pool.slot(form)[11] & 1);
        // 실제 Form의 부모 삭제 통지는 등록 ID를 통해 같은 Gump를 지연 삭제한다.
        const notified = gump.createBase(0, 10, 10, 1, () => 0); gump.attach(notified, 210, 210); gump.bindObject(notified, technology);
        const notifiedForm = gump.node(notified).form; session.gumpFormHost().notify(notifiedForm);
        assert.ok(gump.node(notified).flags & 0x10000); assert.ok(session.core().pool.slot(notifiedForm)[11] & 1);
        gump.lifetime().cleanup(); assert.equal(gump.node(notified), null);
        // 파생 가상 통지는 같은 등록 토큰과 실제 부모 SID를 받아 처리한다.
        const derived = gump.createBase(0, 10, 10, 1, () => 0); gump.attach(derived, 215, 215); gump.bindObject(derived, technology);
        const received = []; gump.bindObjectGone(derived, (token, object) => { received.push([token, object]); gump.lifetime().kill(token); });
        session.gumpFormHost().notify(gump.node(derived).form); assert.deepEqual(received, [[derived, technology]]);
        gump.lifetime().cleanup(); assert.throws(() => gump.bindObjectGone(derived, null), /토큰/u);
        // World 반납 전에는 잔여 Form을 실제 삭제하고 새 대여를 허용한다.
        const outstanding = gump.createBase(0, 10, 10, 1, () => 0); gump.attach(outstanding, 220, 220); gump.bindObject(outstanding, technology);
        const outstandingForm = gump.node(outstanding).form; gump.releaseFormHost();
        assert.equal(gump.node(outstanding), null); assert.ok(session.core().pool.slot(outstandingForm)[11] & 1); gump.setFormHost(session.gumpFormHost());
        // 실제 기술 내용물을 공통 Gump 트리의 원본 생산 격자 항목으로 수집한다.
        const gridState = { cursorPoint: { x: 260, y: 260 }, cursorFlags: 0x40 };
        const grid = new ClientInventoryGrid(gump, session, gridState, {
            now: () => app._gameClock.wallSeconds(), baseHandle: () => 0,
            allocateItem: (size) => { assert.equal(size, 0xc0); const item = new InventoryItemNode(); return gump.allocateDerived(item, () => 0, (token, flags) => gump.lifetime().deleteScalar(token, flags)); },
        });
        const gridToken = grid.create(2, 2, 0x40), gridNode = gump.node(gridToken); gridNode.fixed = 1; gridNode.acceptedCursor = 0x40; gridNode.baseWidth = gridNode.width = 72; gridNode.baseHeight = gridNode.height = 72;
        gump.bindObject(gridToken, technology); gump.attach(gridToken, 250, 250); let items = 0;
        // 실제 내용물과 같은 풀에서 각 항목의 다리 플래그를 확인한다.
        for (let item = gridNode.child; item; item = gump.node(item).next) { const raw = session.core().pool.slot(gump.node(item).content); assert.ok((raw[20] | (raw[21] << 8)) & 0x40); items += 1; }
        assert.ok(items > 0); assert.equal(gump.handle(gridToken, 0x0200005f, 0, 0), 0x40);
        assert.equal(gump.handle(gridToken, 0x0200003f, 0, 0), 1); gridState.cursorPoint.x = 322; assert.equal(gump.handle(gridToken, 0x0200003f, 0, 0), 0);
        gump.remove(gridToken);
    } finally { guide.dispose(); app._releaseCampaignWorld(); app._surfaceStorage.dispose(); app._clientSoundDevice.dispose(); app._missionAudioAssets.dispose(); }
});
