// 미션 요청 프레임이 실제 ClientState 큐의 Poll 전 수명과 Fort 이미지를 보존하는지 검사한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { ClientState } from '../www/core/client-state.js';
import { runCampaignRequestFrame } from '../www/core/campaign-request-frame.js';
import { Netstorm } from '../www/netstorm.js';
import { GameAssets } from '../www/assets/game-assets.js';
import { GamePalette } from '../www/assets/palette.js';
import { GameData } from '../www/assets/game-data.js';
import { buildMenuResources } from '../www/assets/menu-assets.js';
import { MainMenuScreen } from '../www/ui/main-menu.js';
import { IndexedSurface } from '../www/render/indexed-surface.js';
import { IndexedPresenter } from '../www/render/presenter.js';
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
test('미션 요청 프레임: 실제 Tutorial1 요청이 앱 Session의 로그인까지 도달한다', () => {
    const assets = realCaptureAssets(), game = new GameAssets(assets.table, assets.database, GamePalette.parse(readData('d/GIFCLOUD.COL')));
    const manifest = JSON.parse(fs.readFileSync(path.join(DATA_ROOT, 'manifest.json'), 'utf8'));
    const files = readDataMap(manifest.files.filter((entry) => entry.bundle === 'menu').map((entry) => entry.path));
    const data = GameData.build({ files, language: 'english' });
    const app = new Netstorm(); app._initializeClientRuntime(false); app._gameData = data;
    app._root = { ownerDocument: { title: 'before' } }; app._resources = buildMenuResources(files);
    app._layout = { logicalWidth: 1024, logicalHeight: 768, visible: false }; app._surface = new IndexedSurface(1024, 768); app._presenter = new IndexedPresenter(null);
    app._mainMenu = new MainMenuScreen({ resources: app._resources }); app._mainMenu.resize(1024, 768); app._initializeClientGump();
    app._initializeClientWorldStorage(game, data.files); app._syncClientCloudOptions(); app._initializeClientPalette(game); app._initializeClientPaletteCache(game); app._initializeClientGuide();
    app._canvas = {}; app._render = () => {}; app._startFrameLoop = () => {};
    const guide = app._clientGuide, events = [];
    const services = { cameraCommit: (context) => { const pixels = context.camera.target().pixels; context.camera.applyPixelsToRendererAndSnapshot(pixels[0], pixels[1]); },
        surfaceDisplay: { suppressed: () => false, invalidate: () => {} }, ownership: { refreshProduction: () => {}, themeMode: () => 1 },
        assets: { postPop: { playImpactSound: () => {} }, queueCommand: () => {} },
        sound: { wallSeconds: () => 0, play: () => {}, playOnceAt: () => {}, playLoopAt: () => {}, stop: () => {}, isPlaying: () => false },
        ai: { assertion: (value) => { throw new Error(`AI 진단 ${value}`); }, participation: { addChat: () => {}, clearChat: () => {} } } };
    try {
        guide.begin('Tutorial1'); guide.run(); guide.run();
        const request = guide.fortRequest(); assert.equal(request.fort, 'BridgeTheGap'); assert.equal(guide.pending().pending(), true);
        assert.equal(runCampaignRequestFrame(guide, () => events.push('poll'), (current) => {
            assert.equal(current, request); events.push('state7'); app._runCampaignState7(services, { x: 1024, y: 768 });
        }), true);
        assert.deepEqual(events, ['poll', 'state7']); assert.equal(request.consumed, true); assert.equal(guide.pending().pending(), false);
        assert.ok(app._missionLoadingWindow); assert.equal(app._mainMenu.dialog, app._missionLoadingWindow);
        app._mainMenu.render(new IndexedSurface(1024, 768));
        const session = app._campaignWorld.session();
        // 실제 패킷·State는 앱 프레임에서만 진행하고 준비 전에는 Guide를 기다린다.
        for (let frame = 0; frame < 10 && !session.localConnectionReady(); frame += 1) app._onFrame(frame * 16);
        assert.equal(session.localConnectionReady(), true); assert.equal(session.world().storage, app._surfaceStorage);
    } finally { guide.dispose(); app._releaseCampaignWorld(); app._surfaceStorage.dispose(); }
});
