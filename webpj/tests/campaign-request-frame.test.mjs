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
import { ClientInventoryItem } from '../www/ui/client-inventory-item.js';
import { ClientImageGump } from '../www/ui/client-image-gump.js';
import { ClientTextGump } from '../www/ui/client-text-gump.js';
import { makeClientTextMarkup } from '../www/ui/client-text-markup.js';
import { ClientProductionBudget } from '../www/ui/client-production-budget.js';
import { ClientGumpBackground } from '../www/ui/client-gump-background.js';
import { ClientProductionSelection } from '../www/ui/client-production-selection.js';
import { ClientProductionHint } from '../www/ui/client-production-hint.js';
import { ClientProductionPanel } from '../www/ui/client-production-panel.js';
import { ClientHudDivider } from '../www/ui/client-hud-divider.js';
import { createClientProductionAssembly } from '../www/ui/client-production-assembly.js';
import { GumpHelp, GumpHelpState } from '../www/ui/gump-help.js';
import { ClientProductionAbout } from '../www/ui/client-production-about.js';
import { createClientProductionMenu } from '../www/ui/client-production-menu.js';
import { createQuickHelpPicture } from '../www/ui/quick-help-info.js';
import { ClientStyleTextBuffers } from '../www/ui/client-style-text-buffers.js';
import { ClientScrollingText } from '../www/ui/client-scrolling-text.js';
import { ClientScrollControls } from '../www/ui/client-scroll-controls.js';
import { ButtonGumpState } from '../www/ui/button-gump.js';
import { StoneButtonGumpState } from '../www/ui/stone-button-gump.js';
import { ScrollGumpState } from '../www/ui/scroll-gump.js';
import { ScrollGumpWidgetState } from '../www/ui/scroll-gump-widgets.js';
import { ScrollingTextGumpWidgetState } from '../www/ui/scrolling-text-gump-widgets.js';
import { ClientGumpDecoration, createClientDecorationState } from '../www/ui/client-gump-decoration.js';
import { ClientMenuGump } from '../www/ui/client-menu-gump.js';
import { ClientPriestMenu } from '../www/ui/client-priest-menu.js';
import { ClientPriestMenuOpen } from '../www/ui/client-priest-menu-open.js';
import { ClientPlayerMenu } from '../www/ui/client-player-menu.js';
import { ClientObjectMenu } from '../www/ui/client-object-menu.js';
import { ClientObjectMenuInfo } from '../www/ui/client-object-menu-info.js';
import { ClientCarrierMenu } from '../www/ui/client-carrier-menu.js';
import { RawPlayerIdentity } from '../www/core/raw-player-identity.js';
import { StyleTextFragmentKind } from '../www/ui/style-text-fragments.js';
import { PriestPlacementAssets } from '../www/core/priest-placement-assets.js';
import { RawCanonPixelShape } from '../www/core/raw-canon-pixel-shape.js';
import { GumpDrawState, GumpPixelTarget, setGumpDrawClip } from '../www/render/gump-draw.js';
import { GumpBackgroundAssets } from '../www/render/gump-background-draw.js';
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
    app._syncClientHelpWord(); assert.equal(app._clientLocal.helpWord().get(), data.get('helpWord'));
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
        gump.move(parent, 100, 0); let uiDraws = 0; gump.bindDraw(parent, (_token, target) => { uiDraws += 1; target.fill({ left: 101, top: 1, right: 105, bottom: 5 }, 241); });
        app._drawCampaignRendererFrame();
        assert.ok(renderer.presentCount > beforePresent); assert.equal(redraw.value, 0); assert.equal(renderer.cameraDirtyEntries().length, 0);
        assert.ok(uiDraws > 0); assert.equal(app._surface.pixels[1366 + 101], 241); gump.bindDraw(parent, null);
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
        const production = app._campaignProduction, technology = session.currentTechnologyParent(); assert.ok(production?.ui); assert.ok(technology);
        // 같은 Client 비용 표시가 실제 텍스트와 시계로 생성되고 반납된다.
        const beforeCost = app._gameClock.gameSeconds(), costToken = production.ui.showSp(5000, 300, 320, 0), costNode = gump.node(costToken);
        const afterCost = app._gameClock.gameSeconds();
        assert.equal(costNode.vtable, 0x0050688c); assert.ok(costNode.head); assert.equal(costNode.acceleration, -2);
        assert.ok(costNode.stamp >= beforeCost && costNode.stamp <= afterCost); gump.remove(costToken); assert.equal(gump.node(costToken), null);
        const bridges = () => { const finder = new RawContainedFinder(session.core().pool, session.productionFinder()); let count = 0;
            // 같은 기술 부모의 실제 내용물만 센다.
            for (let sid = finder.begin(technology); sid; sid = finder.next()) { const raw = session.core().pool.slot(sid); if ((raw[20] | (raw[21] << 8)) & 0x40) count += 1; }
            return count; };
        const previousBridges = bridges(); production.panel.bridgeUntil = 0; app._runCampaignProductionFrame();
        assert.equal(production.panel.slots, session.battleOption(1) * 2 + 2); assert.equal(bridges(), Math.min(previousBridges + 1, production.panel.slots));
        // 자동 공급 후 화면 격자도 같은 기술 내용물 개수로 갱신된다.
        let visibleBridges = 0; for (let token = gump.node(production.ui.node().bridge).child; token; token = gump.node(token).next) visibleBridges += 1;
        assert.equal(visibleBridges, bridges());
        // 실제 앱의 생산 격자 우클릭은 같은 정보 메뉴와 Help 큐로 이어진다.
        const appItem = gump.node(production.ui.node().bridge).child, appBounds = gump.collection().bounds(appItem), appIcon = gump.node(appItem).child;
        const hitDevice = new ClientImageGump(gump, game, { localOwner: 0, ownerMaps: new Uint32Array(188), shadowOffset: { x: 5, y: 5 } }, () => { throw new Error('적중 검사의 색표 호출'); }, () => 0);
        let appPoint = null;
        // 다리 모양의 중심이 투명할 수 있으므로 실제 불투명 픽셀을 선택한다.
        for (let y = appBounds.top; y < appBounds.bottom && !appPoint; y += 1) for (let x = appBounds.left; x < appBounds.right; x += 1) {
            const local = gump.collection().screenToLocal(appItem, { x, y }); if (hitDevice.hit(appIcon, local[0], local[1])) { appPoint = { x, y }; break; }
        }
        assert.ok(appPoint); assert.equal(gump.handle(appItem, 0x20000, appPoint.x, appPoint.y), 1);
        let appMenu = gump.node(gump.createState().collection.root).child;
        while (appMenu && gump.node(appMenu).command !== 26) appMenu = gump.node(appMenu).next;
        assert.ok(appMenu); const appMenuBounds = gump.collection().bounds(appMenu), helpX = appMenuBounds.left + 8, helpY = appMenuBounds.top + 5;
        gump.broadcast(0, helpX, helpY); assert.equal(gump.input().route(0x10000, helpX, helpY), appMenu);
        app._runCampaignInputFrame(); assert.ok(app._mainMenu.dialog); assert.equal(app._mainMenu.dialog.topic, 'bridgeType');
        // 원본 다리 주제에는 info 태그가 없다. 실제 info 태그가 있는 썬더 팩토리 주제를 같은 큐로 요청한다.
        assert.equal(app._mainMenu.dialog.information, null); app._mainMenu.setDialog(null); gump.lifetime().cleanup();
        const factoryType = game.find('thunderFactory').block + 70; app._clientInputQueue.pushCommand(0x02000017, 0, factoryType); app._runCampaignInputFrame();
        assert.equal(app._mainMenu.dialog.topic, 'thunderFactoryType'); assert.ok(app._mainMenu.dialog.information); assert.ok(app._mainMenu.dialog.information.softLines.length);
        assert.equal(app._mainMenu.dialog.picture?.frameNumber ?? -1, game.types()[factoryType - 70].definition.helpFrame);
        app._mainMenu.setDialog(null);
        // Factory의 정적 Help 토큰도 같은 실제 입력 큐에서 주제로 해석한다.
        const researchTopic = app._clientChatWindows.store('researchHelp');
        app._clientInputQueue.pushCommand(0x02000017, 0, researchTopic); app._runCampaignInputFrame();
        assert.equal(app._mainMenu.dialog.topic, 'researchHelp'); app._mainMenu.setDialog(null);
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
        assert.ok(bridge);
        // 실제 Session TYPE·요구 에너지와 현재 체력 장치를 도움말이 공유한다.
        const bridgeType = session.core().bridgeState.bridgeType, helpType = session.helpTypeInfo(bridgeType), bridgeRecord = session.core().types[bridgeType];
        assert.equal(helpType.theme, bridgeRecord.theme); assert.equal(helpType.flags1, bridgeRecord.flags1); assert.equal(helpType.cost, bridgeRecord.cost);
        const helpPicture = createQuickHelpPicture(game, bridgeType), definition = game.types()[bridgeType - 70].definition;
        assert.equal(helpPicture?.frameNumber ?? -1, definition.helpFrame); if (helpPicture) assert.equal(helpPicture.type, bridgeType);
        assert.equal(createQuickHelpPicture(game, 109), null); assert.equal(createQuickHelpPicture(game, 161), null); assert.throws(() => createQuickHelpPicture(game, 188), /TYPE 범위/u);
        assert.equal(typeof helpType.mana, 'string'); assert.equal(typeof helpType.classification, 'string'); assert.equal(typeof session.hitPoints().typeHitPoints(bridgeType), 'number');
        assert.throws(() => session.helpTypeInfo(188), /TYPE 범위/u);
        const cursor = session.cursor(), drop = session.cursorDropSettings(); cursor.take(bridge, 0);
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
        const metadata = new PriestPlacementAssets(game.typeTable, game.shapes, session.core().types);
        const measure = new RawCanonPixelShape(session.core().pool, session.core().types, metadata.frames, metadata.shapes, metadata.hotspots, metadata.geometry, { scaleX: 16, scaleY: 11 });
        const images = new ClientImageGump(gump, game, { localOwner: 0, ownerMaps: new Uint32Array(188), shadowOffset: { x: 5, y: 5 } }, () => { throw new Error('선택하지 않은 색표'); }, () => 0);
        const itemState = { localOwner: session.localOwner(), editor: 0, stockEnabled: 0, inputDisabled: 0, lastType: 0, bridgeType: session.core().bridgeState.bridgeType, pixelsX: 16, pixelsY: 11, backgroundType: 92, backgroundFrame: 2 };
        const itemBackground = new ClientGumpBackground(gump, game, () => 0, () => screen.shadeMaps(), () => screen.colors(), () => 0);
        let grid; const itemDevice = new ClientInventoryItem(gump, session, game, metadata, measure, images, itemState, { idleItem: (token) => grid.idleItem(token), denied: () => 0,
            background: (target, clip, ...args) => itemBackground.texture(target, clip, ...args) });
        grid = new ClientInventoryGrid(gump, session, gridState, {
            now: () => app._gameClock.wallSeconds(), baseHandle: () => 0, allocateItem: (size) => itemDevice.allocate(size),
        });
        const gridToken = grid.create(2, 2, 0x40), gridNode = gump.node(gridToken); gridNode.fixed = 1; gridNode.acceptedCursor = 0x40; gridNode.baseWidth = gridNode.width = 72; gridNode.baseHeight = gridNode.height = 72;
        const selectedItems = [], selectionSounds = [], selection = new ClientProductionSelection(session, itemDevice, { technology: () => technology,
            templateOwner: () => session.localOwner(), stock: () => false, failure: () => { throw new Error('다리 선택 실패'); },
            flash: () => { throw new Error('다리 예산 부족'); }, sound: (name) => selectionSounds.push(name), changed: () => {} });
        const panel = gump.createBase(0, 80, 200, 1, (event, first, second) => { if (event === 0x02000008) { selectedItems.push([first, second]); return selection.choose(first, second); } return 0; });
        gump.attach(panel, 0, 0); gump.bindObject(gridToken, technology); gump.attach(gridToken, 250, 250, panel); let items = 0;
        // 실제 내용물과 같은 풀에서 각 항목의 다리 플래그를 확인한다.
        for (let item = gridNode.child; item; item = gump.node(item).next) { const raw = session.core().pool.slot(gump.node(item).content); assert.ok((raw[20] | (raw[21] << 8)) & 0x40); items += 1; }
        assert.ok(items > 0); assert.equal(gump.handle(gridToken, 0x0200005f, 0, 0), 0x40);
        const firstItemToken = gridNode.child, firstItem = gump.node(firstItemToken), icon = firstItem.child; assert.ok(icon); assert.equal(gump.node(icon).command, 3);
        const draw = new GumpDrawState(); setGumpDrawClip(draw, { left: 0, top: 0, right: 1366, bottom: 768 });
        const iconPixels = new Uint8Array(1366 * 768).fill(255); gump.draw(icon, draw, new GumpPixelTarget(iconPixels, 1366, 768, draw));
        assert.ok(iconPixels.some((value) => value !== 255)); let hitPoint = null;
        firstItem.decoration = 1; const decoration = new Uint8Array(1366 * 768).fill(255);
        gump.draw(firstItemToken, draw, new GumpPixelTarget(decoration, 1366, 768, draw)); assert.ok(decoration.some((value) => value !== 255)); firstItem.decoration = 0;
        // 실제 아이콘의 불투명 픽셀을 항목 로컬 적중으로 찾아 클릭한다.
        for (let y = 240; y < 340 && !hitPoint; y += 1) for (let x = 240; x < 340; x += 1) {
            if (iconPixels[y * 1366 + x] === 255) continue; const local = gump.collection().screenToLocal(firstItemToken, { x, y });
            if (images.hit(icon, local[0], local[1])) { hitPoint = { x, y }; break; }
        }
        assert.ok(hitPoint); assert.equal(gump.handle(firstItemToken, 0x10000, hitPoint.x, hitPoint.y), 1);
        assert.deepEqual(selectedItems, [[firstItem.content, firstItem.serial]]); assert.equal(itemState.lastType, session.core().bridgeState.bridgeType);
        assert.equal(drop.preview, firstItem.content); assert.deepEqual(selectionSounds, ['getPiece.wav']); cursor.cancel();
        const bridgeSlot = session.core().pool.slot(firstItem.content)[23]; assert.equal(selection.shortcut(bridgeSlot), 1); assert.equal(drop.preview, firstItem.content); cursor.cancel();
        assert.equal(gump.handle(gridToken, 0x0200003f, 0, 0), 1); gridState.cursorPoint.x = 322; assert.equal(gump.handle(gridToken, 0x0200003f, 0, 0), 0);
        gump.remove(gridToken); gump.remove(panel);
        // 실제 Client 글꼴과 TextGump 스타일 체인을 같은 Gump 픽셀에 출력한다.
        const text = new ClientTextGump(gump, app._resources.fonts, () => 0), textToken = text.allocate();
        text.construct(textToken, 0, 0, 0, 241); text.style(textToken, 'Storm Power'); text.fit(textToken); gump.attach(textToken, 350, 350);
        const textPixels = new Uint8Array(1366 * 768).fill(255); gump.draw(textToken, draw, new GumpPixelTarget(textPixels, 1366, 768, draw));
        assert.ok(textPixels.some((value) => value !== 255)); text.clear(textToken); assert.equal(gump.node(textToken).head, 0); gump.remove(textToken);
        // 생산 예산의 실제 색·그림 명령을 한 줄 조각과 같은 화면에 출력하고 수명을 반납한다.
        const pictureColors = Uint8Array.from({ length: 256 }, (_, index) => index);
        const rich = new ClientTextGump(gump, app._resources.fonts, () => 0, makeClientTextMarkup(game, () => screen.colors(), () => pictureColors, () => { throw new Error('예산 문자열에 소리 없음'); }));
        const richToken = rich.allocate(); rich.construct(richToken, 0, 0, 0, 241); rich.style(richToken, '~3~E~w100~[I92.3]'); rich.fit(richToken); gump.attach(richToken, 350, 350);
        const fragments = rich.styleState().fragments; assert.equal([...fragments.values()].filter((part) => part.kind === StyleTextFragmentKind.EXTERNAL).length, 1);
        const richPixels = new Uint8Array(1366 * 768).fill(255); gump.draw(richToken, draw, new GumpPixelTarget(richPixels, 1366, 768, draw));
        rich.clear(richToken); rich.style(richToken, '~3~E~w100');
        const plainPixels = new Uint8Array(1366 * 768).fill(255); gump.draw(richToken, draw, new GumpPixelTarget(plainPixels, 1366, 768, draw));
        assert.ok(richPixels.some((value, index) => value !== plainPixels[index])); rich.clear(richToken); assert.equal(fragments.size, 0); gump.remove(richToken);
        // 실제 Session의 자본을 원본 100/10/1 단계로 표시하고 점멸 수명을 확인한다.
        let budgetNow = 0, budgetChanges = 0, immediate = false;
        const budgetText = new ClientTextGump(gump, app._resources.fonts, () => 0, makeClientTextMarkup(game, () => screen.colors(), () => pictureColors, () => {}));
        const budget = new ClientProductionBudget(gump, budgetText, { now: () => budgetNow, money: () => session.sp(session.localOwner()), known: () => true,
            fort: () => immediate, editor: () => false, backgroundType: () => 92, changed: () => { budgetChanges += 1; } });
        const budgetPanel = gump.createBase(0, 80, 100, 1, () => 0); gump.attach(budgetPanel, 400, 400); const budgetToken = budget.create(budgetPanel);
        const previousCash = session.sp(session.localOwner()); session.setSp(session.localOwner(), 2500); const cash = session.sp(session.localOwner());
        gump.idle(); assert.equal(budget.amount(), 100); assert.ok(budgetChanges >= 2);
        immediate = true; gump.idle(); assert.equal(budget.amount(), cash); immediate = false;
        session.setSp(session.localOwner(), 2200); gump.idle(); assert.equal(budget.amount(), 2490);
        session.setSp(session.localOwner(), 2170); gump.idle(); assert.equal(budget.amount(), 2390);
        session.setSp(session.localOwner(), 2380); gump.idle(); assert.equal(budget.amount(), 2389);
        session.setSp(session.localOwner(), cash); immediate = true; gump.idle(); assert.equal(budget.amount(), cash);
        budget.flash(); budgetNow = 0.16; gump.idle(); assert.equal(gump.node(budgetToken).head, 0);
        budgetNow = 0.32; gump.idle(); assert.notEqual(gump.node(budgetToken).head, 0); gump.remove(budgetPanel); session.setSp(session.localOwner(), previousCash);
        // 실제 글꼴·번역·픽셀의 생산 실패 안내는 4초 뒤 같은 Gump 수명에서 사라진다.
        let hintNow = 0; const hint = new ClientProductionHint(gump, budgetText, { now: () => hintNow, translate: (value) => app._resources.translate(value),
            palette: () => screen.colors(), changed: () => {} });
        const reference = gump.createBase(0, 36, 36, 1, () => 0); gump.attach(reference, 500, 400);
        const notice = hint.show(false, gump.node(reference).serial), noticePixels = new Uint8Array(1366 * 768).fill(255);
        gump.draw(notice, draw, new GumpPixelTarget(noticePixels, 1366, 768, draw)); assert.ok(noticePixels.some((value) => value !== 255));
        hintNow = 4; gump.idle(); assert.ok(gump.node(notice).flags & 0x10000); gump.lifetime().cleanup(); assert.equal(gump.node(notice), null); gump.remove(reference);
        // 실제 생산 유지 상태에 예산·배경·구분선·두 격자를 같은 트리로 조립한다.
        const panelBudget = new ClientProductionBudget(gump, budgetText, { now: () => app._gameClock.wallSeconds(), money: () => session.sp(session.localOwner()),
            known: () => true, fort: () => false, editor: () => false, backgroundType: () => 92, changed: () => app._clientRenderer.invalidateAll() });
        const divider = new ClientHudDivider(gump, { background: itemBackground, backgroundColor: () => 0, dividerColor: () => 55, closePopups: () => gump.closePopups() });
        const globals = app._clientProductionGlobals; globals.backgroundType = 92; globals.backgroundFrame = 0;
        const assembled = new ClientProductionPanel(gump, session, { grid, background: itemBackground, divider, budget: panelBudget, selection,
            technology: () => technology, changed: () => app._clientRenderer.invalidateAll() }, globals, production.panel);
        const assembledToken = assembled.open(), assembledNode = assembled.node(); assert.equal(assembledNode.panel, production.panel); assert.equal(assembledNode.width, 80);
        assert.ok(gump.node(assembledNode.bridge).child); assert.equal(gump.node(assembledNode.bridge).object, technology); assert.equal(gump.node(assembledNode.teleport).object, technology);
        const actualItem = gump.node(gump.node(assembledNode.bridge).child), actualSlot = session.core().pool.slot(actualItem.content)[23];
        assert.equal(gump.handle(assembledToken, 0x020000f4, actualSlot, 0), 1); assert.equal(drop.preview, actualItem.content); cursor.cancel();
        const clickedToken = gump.node(assembledNode.bridge).child, clickedItem = gump.node(clickedToken), clickedIcon = clickedItem.child;
        const panelPixels = new Uint8Array(1366 * 768).fill(255); gump.draw(clickedIcon, draw, new GumpPixelTarget(panelPixels, 1366, 768, draw)); let panelHit = null;
        // 현재 패널의 실제 불투명 아이콘을 적중하고 공통 입력 경로로 누른다.
        for (let y = 26; y < 250 && !panelHit; y += 1) for (let x = 0; x < 80; x += 1) {
            if (panelPixels[y * 1366 + x] === 255) continue; const local = gump.collection().screenToLocal(clickedToken, { x, y });
            if (images.hit(clickedIcon, local[0], local[1])) { panelHit = { x, y }; break; }
        }
        assert.ok(panelHit); gump.input().route(0x10000, panelHit.x, panelHit.y); assert.equal(drop.preview, clickedItem.content); cursor.cancel();
        assembled.close(); assert.equal(gump.node(assembledToken), null); assert.throws(() => assembled.open(), /중복 생성/u);
        // 실제 Client 색표·정책·글꼴을 쓰는 조립 공장도 같은 Session에서 생성한다.
        const help = new GumpHelp(gump.createState(), new GumpHelpState(), { node: (token) => gump.node(token) });
        const complete = createClientProductionAssembly(gump, session, game, app._resources.fonts, screen, app._clientColorSort, globals, production.panel, {
            now: () => app._gameClock.wallSeconds(), translate: (value) => app._resources.translate(value),
            sound: (name) => app._clientSoundPlayer.play(app._clientSoundNames.lookup(name), 0, 0, 0, 0, 0), tell: (section) => app._openTell(section),
            about: () => { throw new Error('생산 정보 메뉴 미연결'); }, changed: () => app._clientRenderer.invalidateAll(), closePopups: () => app._closeCampaignPopups(),
            poll: (code) => app._clientInputPoll.poll(code), cursor: (number) => { app._campaignCursor = number; },
            baseHandle: (...args) => help.handle(...args), menuInstances: () => 0,
        });
        const completeToken = complete.open(), completeNode = complete.node(); assert.ok(gump.node(completeNode.bridge).child);
        const completeItem = gump.node(gump.node(completeNode.bridge).child), completeSlot = session.core().pool.slot(completeItem.content)[23];
        const completePixels = new Uint8Array(1366 * 768).fill(255); gump.draw(completeItem.child, draw, new GumpPixelTarget(completePixels, 1366, 768, draw));
        assert.ok(completePixels.some((value) => value !== 255));
        assert.equal(gump.handle(completeToken, 0x020000f4, completeSlot, 0), 1); assert.equal(drop.preview, completeItem.content); cursor.cancel();
        const bridgeForm = gump.node(completeNode.bridge).form, budgetId = gump.node(completeNode.budget).id;
        complete.resize(700); assert.equal(completeNode.height, 620); assert.equal(gump.node(completeNode.bridge).form, bridgeForm); assert.equal(gump.node(completeNode.budget).id, budgetId);
        complete.resize(768); assert.equal(completeNode.height, 688);
        // 실제 기술 항목의 설명·비용·도움말을 원본 작성 순서로 수집한다.
        const aboutCalls = [], aboutItem = gump.node(gump.node(completeNode.bridge).child), writer = {
            begin: () => { aboutCalls.push(['begin']); return 99; }, root: () => gump.createState().collection.root,
            idle: (...args) => aboutCalls.push(['idle', ...args]), reference: (...args) => aboutCalls.push(['reference', ...args]),
            title: (value) => aboutCalls.push(['title', value]), header: (value) => aboutCalls.push(['header', value]),
            entry: (...args) => aboutCalls.push(['entry', ...args]), attach: (...args) => aboutCalls.push(['attach', ...args]),
        };
        const about = new ClientProductionAbout(session, game, data, writer, { translate: (value) => app._resources.translate(value),
            ownerName: (owner) => { const records = app._clientPlayers.records(), start = owner * 180 + 12, end = records.indexOf(0, start); return String.fromCharCode(...records.subarray(start, end)); },
            helpWord: () => app._clientLocal.helpWord().get(), affordable: (sid) => itemDevice.affordable(sid), sound: (name) => aboutCalls.push(['sound', name]) });
        assert.equal(about.open(aboutItem, { x: 12, y: 34 }), 1); assert.ok(aboutCalls.some(([kind, value]) => kind === 'title' && value));
        assert.ok(aboutCalls.some(([kind, value]) => kind === 'header' && value.includes('Cost:'))); assert.ok(aboutCalls.some(([kind, _label, command]) => kind === 'entry' && command === 0x02000017));
        assert.deepEqual(aboutCalls.at(-1), ['sound', 'opengump.wav']); complete.close();
        // 실제 한 줄 글자 소유자의 조각·그림과 글꼴을 다중 줄 버퍼에서도 공유한다.
        let bufferPictures = null, bufferMarkup = null; const bufferText = new ClientTextGump(gump, app._resources.fonts, () => 0,
            makeClientTextMarkup(game, () => screen.colors(), () => pictureColors, () => {}, undefined, (pictures, markup) => { bufferPictures = pictures; bufferMarkup = markup; }));
        const buffers = new ClientStyleTextBuffers(bufferText, app._resources.fonts, bufferPictures), bufferToken = buffers.create(8), buffer = buffers.binding(bufferToken);
        const words = Uint32Array.of(0, 0, 241, 0, 0, 0, 1, 1); buffer.handler.appendPicture(buffers.pictures(), 92, 3);
        buffer.handler.appendText(Uint8Array.of(65, 10, 0), words); buffer.handler.layout(100);
        assert.equal(buffer.state.lineCount, 1); assert.ok(buffer.state.softCount > 0); assert.ok(buffer.handler.maximumWidth() > 0);
        buffers.destroy(bufferToken); assert.equal(bufferText.styleState().fragments.size, 0);
        // 실제 글꼴·그림·숨긴 Slider가 있는 다중 줄 Gump를 생성하고 화면에 출력한다.
        const buttonState = new ButtonGumpState(), stoneState = new StoneButtonGumpState(), scrollState = new ScrollGumpState();
        Object.assign(buttonState, { valueEvent: 0x02000070, clickEvent: 0x0200002c, rightEvent: 0x02000012, repeatEvent: 0x0200002f, baseColor: 56 });
        Object.assign(scrollState, { clickEvent: 0x0200002c, repeatEvent: 0x0200002f, stepEvent: 0x0200002e, changeEvent: 0x02000036 });
        stoneState.color = 49; stoneState.palette = { black: 0, white: 1, edge: 2 }; stoneState.backgroundType = 92; stoneState.backgroundFrame = 1;
        const scrollAssets = new GumpBackgroundAssets(game), scrollWidget = new ScrollGumpWidgetState(); scrollWidget.defaultType = 92;
        const scroll = new ClientScrollControls(gump, { buttonState, stoneState, scrollState, widgetState: scrollWidget,
            backgroundColor: () => 35, type: (type) => scrollAssets.type(type), frame: (...args) => scrollAssets.frame(...args), output: { fonts: app._resources.fonts, background: itemBackground },
            now: () => app._gameClock.wallSeconds(), poll: (code) => app._clientInputPoll.poll(code), sound: () => {}, baseEvent: () => 0 });
        const scrollWidgetState = new ScrollingTextGumpWidgetState(); Object.assign(scrollWidgetState, { changeEvent: 0x02000036, stepEvent: 0x0200002e });
        const multi = new ClientScrollingText(gump, app._resources.fonts, bufferText, buffers, scroll, bufferMarkup, scrollWidgetState, (number) => { app._campaignCursor = number; });
        const multiToken = multi.allocate(); multi.construct(multiToken, 160, 60, 0, 241); gump.attach(multiToken, 600, 300); multi.style(multiToken, 'A~[I92.3]B\n');
        const multiPixels = new Uint8Array(1366 * 768).fill(255); gump.draw(multiToken, draw, new GumpPixelTarget(multiPixels, 1366, 768, draw));
        assert.ok(multiPixels.some((value) => value !== 255)); multi.setSlider(multiToken, true);
        const sliderNode = gump.node(gump.node(multiToken).slider), scrollPixels = new Uint8Array(1366 * 768).fill(255), scrollTarget = new GumpPixelTarget(scrollPixels, 1366, 768, draw);
        // 두 화살표·돌 배경·손잡이는 각각 실제 픽셀을 남긴다.
        for (const child of [sliderNode.decrement, gump.node(sliderNode.decrement).background, sliderNode.increment, sliderNode.thumb]) {
            scrollPixels.fill(255); gump.draw(child, draw, scrollTarget); assert.ok(scrollPixels.some((value) => value !== 255));
        }
        gump.remove(multiToken); scroll.close(); assert.equal(bufferText.styleState().fragments.size, 0);
        // 같은 Client의 원본 세로 메뉴를 만들고 실제 항목 클릭·명령·소멸을 확인한다.
        const commands = [], menuDecoration = new ClientGumpDecoration(gump, game, itemBackground, createClientDecorationState(92), () => 0, (rect) => app._clientRenderer.invalidate(rect, 0));
        const menus = new ClientMenuGump(gump, { assets: game, fonts: app._resources.fonts, text: bufferText, background: itemBackground, decoration: menuDecoration, title: multi,
            session, config: data, backgroundType: 92, foreground: screen.colors().basic[7], disabled: screen.colors().basic[8], shadow: screen.colors().basic[0],
            sound: (name) => app._clientSoundPlayer.play(app._clientSoundNames.lookup(name), 0, 0, 0, 0, 0), command: (...args) => commands.push(args), invalidate: (rect) => app._clientRenderer.invalidate(rect, 0) });
        const menuToken = menus.beginVertical(); menus.entry('Help', 0x02000017, 82); menus.attach({ x: 700, y: 400 }, menus.root());
        const menuPixels = new Uint8Array(1366 * 768).fill(255); gump.draw(menuToken, draw, new GumpPixelTarget(menuPixels, 1366, 768, draw)); assert.ok(menuPixels.some((value) => value !== 255));
        const menuBounds = gump.collection().bounds(menuToken), menuX = menuBounds.left + 8, menuY = menuBounds.top + 5;
        gump.broadcast(0, menuX, menuY); assert.equal(gump.input().route(0x10000, menuX, menuY), menuToken);
        assert.equal(commands[0][0], 0x02000017); gump.lifetime().cleanup(); assert.equal(gump.node(menuToken), null); scroll.close();
        // 장식 메뉴의 실제 제목·본문·기술 Form과 공통 수명도 함께 확인한다.
        const fancyToken = menus.beginFancy(); menus.title('Bridge'); menus.header('\nCost: ~i100~[I92.3]~.\n'); menus.entry('Help', 0x02000017, 82);
        menus.idle(fancyToken, true); menus.reference(fancyToken, aboutItem.content); menus.attach({ x: 750, y: 420 }, menus.root());
        const fancyNode = gump.node(fancyToken); assert.equal(session.gumpFormHost().parent(fancyNode.form), aboutItem.content);
        const fancyPixels = new Uint8Array(1366 * 768).fill(255), fancyTarget = new GumpPixelTarget(fancyPixels, 1366, 768, draw);
        // 장식·제목·항목은 별도 Gump 수집 항목으로 각각 출력한다.
        for (const token of [fancyNode.fancy, fancyNode.title, fancyToken]) gump.draw(token, draw, fancyTarget);
        assert.ok(fancyPixels.some((value) => value !== 255)); gump.remove(fancyToken); scroll.close(); assert.equal(bufferText.styleState().fragments.size, 0);
        // 사제 건설에 필요한 중첩 메뉴는 같은 controlled 스택과 실제 자식 등록을 사용한다.
        const parentMenu = menus.beginFancy(); menus.title('Construct Building\n');
        const childMenu = menus.beginVertical(); menus.entry('Build', 0x02000060, 0x007b0001);
        assert.equal(menus.itemCount(), 1); assert.equal(menus.takeCurrent(), childMenu);
        menus.submenu('Temple', childMenu, true); menus.separator(); menus.entry('Help', 0x02000017, 82);
        assert.equal(menus.itemCount(), 3); assert.equal(gump.node(parentMenu).items[0].childId, gump.node(childMenu).id);
        assert.equal(gump.node(childMenu).items[0].command, 0x02000060); assert.equal(gump.node(childMenu).items[0].parameter, 0x007b0001);
        menus.attach({ x: 750, y: 420 }, menus.root()); gump.remove(parentMenu); menus.close(); scroll.close();
        assert.equal(gump.node(childMenu), null); assert.equal(bufferText.styleState().fragments.size, 0);
        // 현재 사제의 실제 기술·자금과 중첩 작성기를 결합해 기본 객체 메뉴까지 순서대로 호출한다.
        const priestRoot = menus.beginFancy(); menus.title('Priest\n'); const basePriests = [];
        const priestMenu = new ClientPriestMenu(session, menus, { translate: (value) => app._resources.translate(value),
            baseMenu: (sid) => basePriests.push(sid), tell: (value) => { throw new Error(value); } });
        priestMenu.open(priest); assert.deepEqual(basePriests, [priest]);
        const priestNode = gump.node(priestRoot); assert.ok(priestNode.itemCount > 0);
        assert.ok(priestNode.items.slice(0, priestNode.itemCount).some((item) => item.childId));
        menus.attach({ x: 750, y: 420 }, menus.root()); gump.remove(priestRoot); menus.close(); scroll.close();
        assert.equal(bufferText.styleState().fragments.size, 0);
        // 같은 현재 Player와 동맹·사제 상태를 Client Player 하위 메뉴에 결합한다.
        const identity = new RawPlayerIdentity(app._clientPlayers.records(), new Uint8Array(0), app._clientChatRecords, {
            localOwner: () => app._clientGameplay.player.battleOwner, subscriber: () => app._clientSubscriber.mySubscriber().value,
            address: () => app._clientGameplay.localAddress().value, name: () => { const bytes = app._clientSubscriber.handle(), end = bytes.indexOf(0); return String.fromCharCode(...bytes.subarray(0, end)); },
            battle: () => app._surfaceStorage.ownerMode.battle, editor: () => app._clientGameplay.world.editor, metamap: () => app._surfaceStorage.ownerMode.challenge, unknownName: () => 'SOMEONE',
        });
        const parentPlayer = menus.beginFancy(); menus.title('Object\n');
        const playerMenu = new ClientPlayerMenu(session, identity, menus, { translate: (value) => app._resources.translate(value),
            assertion: (value) => { throw new Error(`Player 진단 ${value}`); }, parseAddress: () => { throw new Error('예상하지 않은 주소'); }, reliability: () => { throw new Error('예상하지 않은 메타맵'); },
            ignored: (subscriber) => app._clientChatWindows.ignored.includes(subscriber), whispered: () => 0 });
        const subscriber = identity.subscriberForOwner(session.localOwner()), playerChild = playerMenu.open(subscriber); assert.ok(playerChild);
        menus.submenu('Player', playerChild); assert.equal(gump.node(parentPlayer).items[0].childId, gump.node(playerChild).id);
        assert.ok(gump.node(playerChild).itemCount); menus.attach({ x: 750, y: 420 }, menus.root()); gump.remove(parentPlayer); menus.close(); scroll.close();
        assert.equal(gump.node(playerChild), null); assert.equal(bufferText.styleState().fragments.size, 0);
        // 같은 실제 사제의 공통 메뉴는 현재 도움말과 Player 하위를 같은 스택에 쓴다.
        const objectRoot = menus.beginFancy(); menus.title('Object\n');
        const objects = new ClientObjectMenu(session, identity, playerMenu, menus, data, { translate: (value) => app._resources.translate(value),
            helpWord: () => app._clientLocal.helpWord().get(), staticText: (value) => app._clientChatWindows.store(value) });
        objects.open(priest); const objectNode = gump.node(objectRoot); assert.equal(objectNode.itemCount, 2);
        assert.equal(objectNode.items[0].command, 0x02000017); assert.equal(objectNode.items[0].parameter, session.core().pool.slot(priest)[10]);
        assert.ok(objectNode.items[1].childId); menus.attach({ x: 750, y: 420 }, menus.root()); gump.remove(objectRoot); menus.close(); scroll.close();
        assert.equal(bufferText.styleState().fragments.size, 0);
        // 실제 사제의 원본 제목·소유 관계·상태를 같은 장식 메뉴에 작성한다.
        const summaryRoot = menus.beginFancy(); const summary = new ClientObjectMenuInfo(session, game, identity, menus, data, {
            translate: (value) => app._resources.translate(value), ownerName: (owner) => identity.nameForOwner(owner) });
        summary.summary(priest); assert.ok(gump.node(summaryRoot).title);
        menus.attach({ x: 750, y: 420 }, menus.root()); gump.remove(summaryRoot); menus.close(); scroll.close();
        assert.equal(bufferText.styleState().fragments.size, 0);
        // 사제 건설 메뉴의 기본 호출을 실제 운반체와 공통 객체 메뉴까지 잇는다.
        const chainRoot = menus.beginFancy(); menus.title('Priest\n');
        const carrierMenu = new ClientCarrierMenu(session, menus, objects, (value) => app._resources.translate(value));
        const chainPriest = new ClientPriestMenu(session, menus, { translate: (value) => app._resources.translate(value),
            baseMenu: (sid) => carrierMenu.open(sid), tell: (value) => { throw new Error(value); } });
        chainPriest.open(priest); const chainNode = gump.node(chainRoot);
        assert.ok(chainNode.items.slice(0, chainNode.itemCount).some((item) => item.command === 0x02000017));
        assert.ok(chainNode.items.slice(0, chainNode.itemCount).filter((item) => item.childId).length >= 2);
        menus.attach({ x: 750, y: 420 }, menus.root()); gump.remove(chainRoot); menus.close(); scroll.close();
        assert.equal(bufferText.styleState().fragments.size, 0);
        // 원본 사제 선택 관문과 메뉴 열기를 실제 Form·화면 좌표에 함께 연결한다.
        const openSounds = [], opener = new ClientPriestMenuOpen(app._campaignWorld, menus, summary, chainPriest, {
            altar: () => { throw new Error('예상하지 않은 제단'); }, delegate: () => { throw new Error('예상하지 않은 제단 위임'); },
            project: (x, y) => { const point = camera.snapshot().project(x, y); return [point.x, point.y]; },
            content: (sid) => session.priests().carrier.findContained(sid, 1, 0, 0), range: (type) => session.core().types[type].range,
            screenWidth: () => screen.width, sound: (value) => openSounds.push(value),
        });
        assert.equal(opener.supports(priest), true); assert.equal(opener.open(priest, { code: 0x20000, x: 750, y: 420 }), 1);
        assert.deepEqual(openSounds, ['opengump.wav']); assert.equal(app._campaignWorld.selected(), priest);
        let openToken = gump.node(gump.createState().collection.root).child;
        while (openToken && gump.node(openToken).command !== 26) openToken = gump.node(openToken).next;
        assert.ok(openToken); assert.equal(session.gumpFormHost().parent(gump.node(openToken).form), priest);
        gump.remove(openToken); menus.close(); scroll.close(); assert.equal(bufferText.styleState().fragments.size, 0);
        // 실제 신전은 현재 환불과 회수 금지 전역을 읽어 원본의 신전 회수 안내를 만든다.
        const damageRoot = menus.beginFancy(); menus.title('Temple\n'); const priorDeny = app._clientGameplay.denySalvage().value;
        app._clientGameplay.denySalvage().value = 0; const damageSid = temple;
        objects.openDamageable(damageSid); const damageNode = gump.node(damageRoot);
        assert.ok(damageNode.items.slice(0, damageNode.itemCount).some((item) => item.command === 0x02000095 && app._clientChatWindows.strings.get(item.parameter) === 'SalvageVortex'));
        menus.attach({ x: 750, y: 420 }, menus.root()); gump.remove(damageRoot); menus.close(); scroll.close();
        app._clientGameplay.denySalvage().value = priorDeny; assert.equal(bufferText.styleState().fragments.size, 0);
        // 실제 생산 정보가 작성한 장식 메뉴의 제목·비용·항목과 Form을 같은 공통 수명에서 확인한다.
        const integratedCommands = [], productionMenu = createClientProductionMenu(gump, session, game, data, app._resources.fonts, screen, app._clientColorSort, itemBackground, 92, {
            baseHandle: () => 0, sound: () => {}, translate: (value) => app._resources.translate(value),
            ownerName: (owner) => { const records = app._clientPlayers.records(), start = owner * 180 + 12, end = records.indexOf(0, start); return String.fromCharCode(...records.subarray(start, end)); },
            helpWord: () => app._clientLocal.helpWord().get(), affordable: (sid) => itemDevice.affordable(sid), now: () => app._gameClock.wallSeconds(),
            poll: (code) => app._clientInputPoll.poll(code), cursor: (number) => { app._campaignCursor = number; },
            command: (...args) => integratedCommands.push(args), invalidate: (rect) => app._clientRenderer.invalidate(rect, 0),
        });
        assert.equal(productionMenu.open(aboutItem, { x: 750, y: 420 }), 1); assert.equal(productionMenu.instances(), 1);
        let productionMenuToken = gump.node(gump.createState().collection.root).child;
        while (productionMenuToken && gump.node(productionMenuToken).command !== 26) productionMenuToken = gump.node(productionMenuToken).next;
        assert.ok(productionMenuToken); const productionMenuNode = gump.node(productionMenuToken);
        assert.equal(session.gumpFormHost().parent(productionMenuNode.form), aboutItem.content); assert.ok(productionMenuNode.title); assert.ok(productionMenuNode.itemCount);
        const integratedBounds = gump.collection().bounds(productionMenuToken), integratedX = integratedBounds.left + 8, integratedY = integratedBounds.top + 5;
        gump.broadcast(0, integratedX, integratedY); assert.equal(gump.input().route(0x10000, integratedX, integratedY), productionMenuToken);
        assert.equal(integratedCommands[0][0], 0x02000017); gump.lifetime().cleanup(); assert.equal(productionMenu.instances(), 0);
        assert.equal(productionMenu.open(aboutItem, { x: 750, y: 420 }), 1); productionMenu.close(); assert.equal(productionMenu.instances(), 0);
        // 사제 건설 명령의 packed TYPE/SID는 현재 Session 커서의 실제 미리보기를 만든다.
        const beforeBuildMoney = session.sp(session.localOwner()); session.setSp(session.localOwner(), 10000);
        app._clientInputQueue.pushCommand(0x02000060, 0, (123 << 16) | priest); app._runCampaignInputFrame();
        const builtPreview = session.cursorDropSettings().preview; assert.ok(builtPreview);
        const previewSlot = session.core().pool.slot(builtPreview); assert.equal(new DataView(previewSlot.buffer, previewSlot.byteOffset, previewSlot.byteLength).getUint16(18, true), 123);
        assert.equal(session.cursorDropSettings().costFlags, 0x160);
        session.cursor().cancel(); session.setSp(session.localOwner(), beforeBuildMoney);
        // 같은 원본 회수 명령은 실제 로컬 패킷을 보내고 수신 전까지 월드를 바꾸지 않는다.
        const salvageSid = actualBridges.at(-1).sid, salvageFlags = session.core().pool.slot(salvageSid)[11], beforeSalvage = session.localConnectionPackets().length;
        const denied = app._clientGameplay.denySalvage().value; app._clientGameplay.denySalvage().value = 0;
        app._clientInputQueue.pushCommand(0x0200005d, 0, salvageSid); app._runCampaignInputFrame();
        assert.deepEqual(session.localConnectionPackets().slice(beforeSalvage), []); assert.equal(session.core().pool.slot(salvageSid)[11], salvageFlags);
        session.pumpLocalConnection(); assert.deepEqual(session.localConnectionPackets().slice(beforeSalvage), [0x54]); assert.ok(session.core().pool.slot(salvageSid)[11] & 3); app._clientGameplay.denySalvage().value = denied;


    } finally { guide.dispose(); app._releaseCampaignWorld(); app._surfaceStorage.dispose(); app._clientSoundDevice.dispose(); app._missionAudioAssets.dispose(); }
});
