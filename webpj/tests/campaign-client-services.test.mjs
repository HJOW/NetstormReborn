// 캠페인 Start Client 장치의 선택 순서와 현재 대상 재조회를 검사한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { makeCampaignMissionStartEffects, bindCampaignAiClientServices } from '../www/core/campaign-client-services.js';
import { GameData, ARCHIVE_PATH, SETUP_PATH } from '../www/assets/game-data.js';
import { encodeOriginalText, decodeWindows1252 } from '../www/assets/text-encoding.js';
import { RawChatChannels } from '../www/core/raw-chat-channels.js';
import { readData } from './helpers.mjs';

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
