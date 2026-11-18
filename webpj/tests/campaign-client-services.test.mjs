// 캠페인 Start Client 장치의 선택 순서와 현재 대상 재조회를 검사한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { makeCampaignMissionStartEffects, bindCampaignAiClientServices, prepareCampaignMissionStarting, captureCampaignMissionStarting, runCampaignMissionStarting } from '../www/core/campaign-client-services.js';
import { GameData, ARCHIVE_PATH, SETUP_PATH } from '../www/assets/game-data.js';
import { encodeOriginalText, decodeWindows1252 } from '../www/assets/text-encoding.js';
import { RawChatChannels } from '../www/core/raw-chat-channels.js';
import { readData } from './helpers.mjs';
import { ClientLocalStateGlobals } from '../www/core/client-local-state-globals.js';
import { ClientGameplayGlobals } from '../www/core/client-gameplay-globals.js';
import { ClientKencloudGlobals } from '../www/core/client-kencloud-globals.js';
import { ClientHtmlWindowGlobals } from '../www/core/client-html-window-globals.js';
import { RawSurfaceStorage } from '../www/core/raw-surface-storage.js';
import { CampaignSessionAssets } from '../www/core/raw-campaign-session-owners.js';
import { PriestPlacementAssets } from '../www/core/priest-placement-assets.js';
import { GameAssets } from '../www/assets/game-assets.js';
import { GamePalette } from '../www/assets/palette.js';
import { realCaptureAssets } from './capture-assets-support.mjs';
import { createMissionFortLoadRequest, writeMissionFortRequest, captureMissionFortZone } from '../www/core/mission-fort-request.js';
import { restoreLocalMissionStarting } from '../www/core/raw-local-mission-state.js';

// UI·시계·World 대상은 각 효과가 호출되는 시점의 실제 공급자에서 다시 조회한다.
test('캠페인 Client Start 효과: 현재 시계·화면·World 경계', () => {
    const calls = [], clock = { game: 12.5 }, menu = { refresh: () => calls.push('menu') };
    const client = { time: () => clock, renderer: () => ({ invalidateAll: () => calls.push('renderer') }), menu: () => menu };
    const world = { session: () => ({ generateWorldGeysers: () => calls.push('geysers') }), resetCampaignCamera: () => calls.push('camera') };
    assert.throws(() => makeCampaignMissionStartEffects(client, world, null), /명령 Gump 조회 미연결/);
    const effects = makeCampaignMissionStartEffects(client, world, () => calls.push('gump')), bytes = new Uint8Array(600).fill(0xa7);
    assert.deepEqual(calls, []); effects.settingsBytes(3, bytes); assert.ok(bytes.every((value) => value === 0xa7));
    assert.equal(effects.gameSeconds(), 12.5); clock.game = 14.5; assert.equal(effects.gameSeconds(), 14.5);
    effects.refreshSp(); effects.refreshCommandGump(); effects.randomGeysers(); effects.resetCamera();
    assert.deepEqual(calls, ['renderer', 'menu', 'gump', 'geysers', 'camera']);
    client.menu = () => null; effects.refreshSp(); assert.deepEqual(calls.slice(-1), ['renderer']);
});

// 실제 GameData와 채팅 장부에서 현재 설정·언어·이름·난이도를 다시 읽는다.
test('캠페인 Client AI: 실제 설정·로컬 치환·채팅·난이도', () => {
    const files = new Map([[ARCHIVE_PATH, readData(ARCHIVE_PATH)], [SETUP_PATH, readData(SETUP_PATH)]]), data = GameData.build({ files, language: 'english' });
    data.configuration.assign('defaultAiName="Bot"\ndifficulty=3\nskirmishRun=0\n[END]\n');
    assert.equal(data.substituteLocal('{local.1}|{local.2}', 'A', 'B'), 'A|B'); assert.equal(data.get('local.1'), null);
    const registered = data.registry.objects.length, invalid = { toString: () => { throw new Error('인자 변환 실패'); } };
    assert.throws(() => data.substituteLocal('{local.1}', invalid), /인자 변환 실패/); assert.equal(data.registry.objects.length, registered);
    const records = new Uint8Array(4800), chat = new RawChatChannels(records, () => 7); let language = 2;
    const client = { configuration: () => data, languageNumber: () => language, translate: (text) => `T:${text}`, encodeDisplayText: encodeOriginalText, decodeDisplayText: decodeWindows1252, chatChannels: () => chat };
    const ai = {}; bindCampaignAiClientServices(client, ai);
    assert.equal(ai.names.language(), 2); language = 1; assert.equal(ai.names.language(), 1);
    assert.equal(ai.names.translate('Caf\xe9'), 'T:Caf\xe9'); assert.equal(ai.names.substitute('Unit'), 'Unit'); assert.equal(ai.defaultName(), 'Bot');
    ai.participation.addChat(5, 'Bot', 4); assert.equal(records[0x28] & 4, 4); ai.participation.clearChat(4); assert.equal(records[0x28] & 4, 0);
    assert.equal(ai.skirmishPlayers(), 0); assert.equal(ai.difficulty(2, true), 3); assert.equal(ai.difficulty(2, false), 0);
    data.setInt('skirmishRun', 1); data.setInt('skirmishAiCount', 20); data.setInt('skirmishAi2Diff', 1);
    assert.equal(ai.skirmishPlayers(), 8); assert.equal(ai.difficulty(2, false), 1); assert.equal(ai.difficulty(1, true), 3);
    data.setInt('skirmishAiCount', -3); assert.equal(ai.skirmishPlayers(), 2);
    assert.throws(() => ai.names.translate('가'), /원문 바이트 범위/);
});

// 같은 실제 Client 전역과 요청에서 State7 복사·카메라·재접속 지역 값을 확인한다.
test('캠페인 Client State7: 실제 요청의 앞단 캡처와 재접속 지역 값', () => {
    const local = new ClientLocalStateGlobals(), gameplay = new ClientGameplayGlobals(), cloud = new ClientKencloudGlobals(), html = new ClientHtmlWindowGlobals();
    const client = { edition: () => 'patch1078', localStateGlobals: () => local, gameplayGlobals: () => gameplay, kencloudGlobals: () => cloud, htmlWindowGlobals: () => html };
    // 캡처 시점과 복원 시점의 재접속 비트를 독립으로 바꾼다.
    for (let flags = 0; flags < 4; flags += 1) {
        const request = createMissionFortLoadRequest(), state = local.state(), address = Uint8Array.from([122, 108, 111, 99, 97, 108, 44, 42, 0]);
        writeMissionFortRequest('thewarbegins', { next: state.next, sequence: local.zonePending(), localAddress: address, port: { value: 6799 } }, {
            selectFort: (name) => { request.fort = name; }, duplicate: (packet) => captureMissionFortZone(request, packet),
        });
        request.next.set(state.next); request.sequence = local.zonePending().value;
        state.phase.fill(0x10203040); state.phase[0] = 4; state.old.fill(0x50607080); const phase = state.phase.slice(), old = state.old.slice();
        state.next[33] = flags & 1; state.next[2] = (0xfedcba98 + flags) >>> 0; state.next[35] = 0x89abcdef;
        const active = state.next.slice(), next = active.slice(); next[0] = 0; local.gameTicks().value = 0xfedcba98; local.sourceType().value = 0x87654321;
        const pending = local.zonePending().value, camera = gameplay.camera(), denominator = camera.denominator(), title = html.text.background;
        title[0] = 88; title[1] = 89; denominator.defaultValue = 23; denominator.current = flags & 1 ? 23 : 37; cloud.worldRedraw().value = 19;
        camera.snapshot().data.setInt32(28, 11, true); camera.snapshot().data.setInt32(32, 13, true); camera.rendererView().data.setInt32(28, 17, true); camera.rendererView().data.setInt32(32, 19, true); camera.target().pixels.set([29, 31]);
        const snapshot = camera.snapshot().bytes.slice(), view = camera.rendererView().bytes.slice();
        prepareCampaignMissionStarting(client, request);
        assert.deepEqual(state.current, active); assert.deepEqual(state.next, next); assert.deepEqual(state.phase, phase); assert.deepEqual(state.old, old); assert.notEqual(request.next[2], state.current[2]);
        assert.deepEqual(request.starting, { zonePending: pending, gameTicks: flags & 1 ? 0xfedcba98 : 0, sourceType: flags & 1 ? 0x87654321 : 0 });
        assert.equal(denominator.current, 23); assert.equal(cloud.worldRedraw().value, flags & 1 ? 19 : 1); assert.deepEqual(camera.snapshot().bytes, snapshot); assert.deepEqual(camera.rendererView().bytes, view);
        assert.deepEqual([...camera.target().pixels], [29, 31]); assert.deepEqual([...title.subarray(0, 2)], [0, 89]);
        assert.throws(() => captureCampaignMissionStarting(client, request), /앞단 계약/);
        local.gameTicks().value = 41; local.sourceType().value = 43; state.current[33] = (flags >>> 1) & 1;
        restoreLocalMissionStarting(state.current, request.starting, { get gameTicks() { return local.gameTicks().value; }, set gameTicks(value) { local.gameTicks().value = value; }, get sourceType() { return local.sourceType().value; }, set sourceType(value) { local.sourceType().value = value; } });
        assert.equal(local.gameTicks().value, flags & 2 ? request.starting.gameTicks : 41); assert.equal(local.sourceType().value, flags & 2 ? request.starting.sourceType : 43);
    }
});

// 선택된 잘못된 요청은 State 복사와 카메라 변경 전에 거부한다.
test('캠페인 Client State7: 판본·Zone·중복 관문이 앞선 상태를 보존한다', () => {
    const local = new ClientLocalStateGlobals(), gameplay = new ClientGameplayGlobals(), cloud = new ClientKencloudGlobals(), html = new ClientHtmlWindowGlobals();
    const client = { edition: () => 'patch1078', localStateGlobals: () => local, gameplayGlobals: () => gameplay, kencloudGlobals: () => cloud, htmlWindowGlobals: () => html };
    const request = createMissionFortLoadRequest(), state = local.state(); state.next[0] = 7; state.next[32] = 1; request.next[32] = 1; request.zone = new Uint8Array(86);
    const originalNext = state.next.slice(), originalCurrent = state.current.slice();
    // 각 입력은 원본 관문 중 하나만 어긋나며 다른 상태를 쓰지 않는다.
    for (const mutate of [() => { request.consumed = true; }, () => { request.starting = {}; }, () => { request.zone = new Uint8Array(85); }, () => { state.next[0] = 6; }]) {
        mutate(); assert.throws(() => prepareCampaignMissionStarting(client, request), /앞단 계약/); assert.deepEqual(state.current, originalCurrent);
        request.consumed = false; request.starting = null; request.zone = new Uint8Array(86); state.next.set(originalNext);
    }
    client.edition = () => 'cd1072'; assert.throws(() => prepareCampaignMissionStarting(client, request), /앞단 계약/); assert.deepEqual(state.next, originalNext); assert.deepEqual(state.current, originalCurrent);
});

// State7 몸체가 현재 블록을 바꾸어도 즉시 State8은 최초 복사된 인자로 확정한다.
test('캠페인 Client State7 부모: 실제 장기 저장소와 후속 연결 순서', () => {
    const data = realCaptureAssets(), game = new GameAssets(data.table, data.database, GamePalette.parse(readData('d/GIFCLOUD.COL')));
    const local = new ClientLocalStateGlobals(), gameplay = new ClientGameplayGlobals(), cloud = new ClientKencloudGlobals(), html = new ClientHtmlWindowGlobals();
    const assets = new CampaignSessionAssets(game, { read: () => { throw new Error('선택하지 않은 파일'); } }), metadata = new PriestPlacementAssets(game.typeTable, game.shapes, assets.loadedTypes);
    const storage = new RawSurfaceStorage(assets.types, assets.frames, metadata.shapes, { source: game, authority: gameplay.authority(), gameAux: gameplay.gameAux() });
    const time = { game: 12, wall: 100 }, client = { edition: () => 'patch1078', surfaceStorage: () => storage, localStateGlobals: () => local, gameplayGlobals: () => gameplay, kencloudGlobals: () => cloud, htmlWindowGlobals: () => html, time: () => time, wallSeconds: () => 101 };
    const request = createMissionFortLoadRequest(), state = local.state(), address = Uint8Array.from([122, 108, 111, 99, 97, 108, 44, 42, 0]);
    writeMissionFortRequest('thewarbegins', { next: state.next, sequence: local.zonePending(), localAddress: address, port: { value: 6799 } }, { selectFort: (name) => { request.fort = name; }, duplicate: (packet) => captureMissionFortZone(request, packet) });
    request.next.set(state.next); request.sequence = local.zonePending().value; state.next[2] = 0x12345678; state.phase.fill(0x11223344); state.phase[0] = 4; state.old.fill(0x55667788);
    const previous = state.phase.slice(), older = state.old.slice(), calls = [];
    runCampaignMissionStarting(client, () => {
        assert.deepEqual(state.phase, previous); assert.deepEqual(state.old, older); assert.equal(state.current[0], 7); assert.equal(state.immediate[2], 0x12345678);
        captureCampaignMissionStarting(client, request); state.current[2] = 0xfaceb00c; state.current[32] = 0; state.immediate[0] = 8; state.immediate[1] = 20; calls.push('body');
    }, (received, port) => {
        assert.equal(state.phase[0], 7); assert.equal(state.phase[2], 0xfaceb00c); assert.equal(state.current[0], 8); assert.equal(state.current[2], 0x12345678);
        assert.equal(received.length, 20); assert.equal(received[0], 122); assert.equal(port, 6799); calls.push('connect');
    });
    assert.deepEqual(calls, ['body', 'connect']); assert.equal(state.old[0], 7); assert.equal(state.old[2], 0xfaceb00c); assert.equal(state.phase[0], 8);
    assert.equal(state.phase[2], 0x12345678); assert.equal(state.next[0], 9); assert.equal(state.current[0], 0); assert.equal(local.connectEnd().value, 131);
});

// State7 몸체와 후속 State8 연결의 실패는 서로 다른 확정 경계를 보존한다.
test('캠페인 Client State7 부모: 몸체 실패와 연결 실패의 부분 상태', () => {
    const data = realCaptureAssets(), game = new GameAssets(data.table, data.database, GamePalette.parse(readData('d/GIFCLOUD.COL')));
    // 두 실패 지점은 각각 새 Client 장기 소유자에서 검사한다.
    for (const bodyFailure of [true, false]) {
        const local = new ClientLocalStateGlobals(), gameplay = new ClientGameplayGlobals(), html = new ClientHtmlWindowGlobals();
        const assets = new CampaignSessionAssets(game, { read: () => { throw new Error('선택하지 않은 파일'); } }), metadata = new PriestPlacementAssets(game.typeTable, game.shapes, assets.loadedTypes);
        const storage = new RawSurfaceStorage(assets.types, assets.frames, metadata.shapes, { source: game, authority: gameplay.authority(), gameAux: gameplay.gameAux() });
        const state = local.state(); state.next[0] = 7; state.phase.fill(0x12345678); state.phase[0] = 4; state.old.fill(0x90abcdef);
        const prior = state.phase.slice(), older = state.old.slice(); let connects = 0;
        const client = { edition: () => 'patch1078', surfaceStorage: () => storage, localStateGlobals: () => local, gameplayGlobals: () => gameplay, htmlWindowGlobals: () => html, time: () => ({ game: 1, wall: 2 }), wallSeconds: () => 2 };
        assert.throws(() => runCampaignMissionStarting(client, () => { state.immediate[0] = 8; if (bodyFailure) throw new Error('몸체 실패'); }, () => { connects += 1; throw new Error('연결 실패'); }), bodyFailure ? /몸체 실패/ : /연결 실패/);
        if (bodyFailure) { assert.equal(connects, 0); assert.deepEqual(state.phase, prior); assert.deepEqual(state.old, older); assert.equal(state.current[0], 7); assert.equal(state.immediate[0], 8); }
        else { assert.equal(connects, 1); assert.equal(state.phase[0], 7); assert.deepEqual(state.old, prior); assert.equal(state.current[0], 8); assert.equal(state.immediate[0], 0); }
    }
});
