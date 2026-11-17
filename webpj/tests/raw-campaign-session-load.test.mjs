// 실제 Tutorial 1 저장 자료의 Session Load를 검사한다. 플레이·포획·희생·공중 유닛 전용 행동은 실행하지 않는다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { RawCampaignSession } from '../www/core/raw-campaign-session.js';
import { InputQueue } from '../www/core/input-queue.js';
import { GameAssets } from '../www/assets/game-assets.js';
import { GamePalette } from '../www/assets/palette.js';
import { fortSection, parseFort } from '../www/assets/fort.js';
import { ARCHIVE_PATH } from '../www/assets/game-data.js';
import { TaffArchive } from '../www/assets/taff.js';
import { decodeOriginalText } from '../www/assets/text-encoding.js';
import { createFortTypes } from '../www/assets/type-flags.js';
import { TypeTable } from '../www/assets/type-table.js';
import { SoundList } from '../www/audio/sound-list.js';
import { CampaignTemplateStorage } from '../www/core/campaign-template-storage.js';
import { realCaptureAssets } from './capture-assets-support.mjs';
import { loadOriginalForts, readData } from './helpers.mjs';

// 보호된 실제 TYPE·SHP·팔레트와 Tutorial 1 Fort를 읽는다.
const data = realCaptureAssets(), game = new GameAssets(data.table, data.database, GamePalette.parse(readData('d/GIFCLOUD.COL')));
const archive = new TaffArchive(readData(ARCHIVE_PATH));
const entry = loadOriginalForts(archive).find((candidate) => /thewarbegins.fort$/i.test(candidate.source));
const fort = parseFort(entry.bytes, createFortTypes(TypeTable.fromArchive(archive, decodeOriginalText)));
// 예상하지 않은 외부 장치는 호출된 자리에서 중단한다.
const forbidden = (name) => () => { throw new Error(`외부 장치 미연결: ${name}`); };

// 게임 논리는 실제 몸체로 실행하고 UI·음향은 관찰하며 명령은 실제 원본 큐에 담는다.
function scene(overrides = {}, savedFort = fort) {
    const trace = [], queue = new InputQueue();
    const s = new RawCampaignSession(game, { read: forbidden('파일') }, savedFort, 'thewarbegins', {
        unixSeconds: () => 1700000000, soundNames: new SoundList(), carrierRequest: forbidden('전송'), deletion: { selected: () => 0 },
        install: { translateYou: () => Uint8Array.of(89, 111, 117, 0), setConfig: () => trace.push('config'), refreshWindowTitle: () => trace.push('title'), resetCamera: () => trace.push('camera'), ...overrides.install },
        ownership: { refreshProduction: () => trace.push('production'), themeMode: () => 0 },
        assets: { postPop: { playImpactSound: () => trace.push('impact') }, queueCommand: (...args) => queue.pushCommand(...args) },
        templateStorage: overrides.templateStorage, fortMission: overrides.fortMission, production: overrides.production,
        sound: { wallSeconds: () => 0, play: forbidden('소리'), playOnceAt: forbidden('위치 소리'), playLoopAt: forbidden('반복 소리'), stop: forbidden('정지'), isPlaying: () => false },
        player: { log: () => trace.push('log') }, ai: { assertion: forbidden('AI 진단') },
    });
    return { s, trace, queue };
}

// 실제 120BYTE 섬과 저장 객체를 적재한 다음에만 loaded를 공개한다.
test('CampaignSession Tutorial 1: 실제 저장 객체·Graph와 커서까지 Load', () => {
    const { s, trace, queue } = scene();
    try {
        assert.equal(s.loaded, false); s.load(); assert.equal(s.loaded, true);
        const objects = s.world().objects(), priest = objects.find(({ object }) => object.type === 158 && object.owner === 1);
        assert.ok(priest && priest.sid >= 15000); assert.ok(objects.length > 1000); assert.ok(queue.size > 0);
        assert.deepEqual(trace.slice(0, 2), ['config', 'title']); assert.equal(trace.at(-1), 'camera'); assert.ok(trace.includes('production'));
        assert.throws(() => s.load(), /반복|중복/); assert.equal(s.loaded, true);
    } finally { s.dispose(); }
    assert.throws(() => s.load(), /반납/);
});

// 실제 Battle snapshot의 저장 자식과 재적재 객체를 같은 자료로 비교한다. SID는 비교하지 않는다.
test('CampaignSession Tutorial 1: 실제 Battle snapshot과 재적재', () => {
    const storage = new CampaignTemplateStorage();
    const options = { templateStorage: storage, fortMission: { current: () => 0 }, production: { allocationBytes: (bytes) => bytes.fill(0xa5), effects: { configInt: () => 0 } } };
    const first = scene(options); let saved, before;
    try {
        first.s.load(); before = first.s.world().objects(); first.s.snapshotBattle(1);
        const value = storage.template(1); assert.ok(fortSection(value, 'Terr00').length > 1); assert.ok(fortSection(value, 'Terr01').length > 1);
        assert.equal(fortSection(value, 'Territory').length, 120); assert.equal(fortSection(value, 'Money').length, 4);
        saved = { ...value, sections: value.sections.map((bytes) => bytes.slice()) };
    } finally { first.s.dispose(); }
    const second = scene({ ...options, templateStorage: new CampaignTemplateStorage() }, saved);
    try {
        second.s.load(); const after = second.s.world().objects();
        // SID는 적재 순서에 좌우되므로 표시 자료의 다중집합을 비교한다.
        const keys = (objects) => objects.map(({ object }) => `${object.type},${object.owner},${object.frame},${object.x},${object.y}`).sort();
        assert.equal(second.s.loaded, true); assert.equal(before.length, 1875); assert.deepEqual(keys(after), keys(before));
    } finally { second.s.dispose(); }
});

// 중간 외부 장치 오류는 loaded를 켜지 않고 원문의 앞선 설치를 보존한다.
test('CampaignSession Load: 실패 접두를 유지하고 완료를 가장하지 않는다', () => {
    const { s } = scene({ install: { translateYou: forbidden('번역') } });
    try { assert.throws(() => s.load(), /번역/); assert.equal(s.loaded, false); assert.throws(() => s.load(), /반복|중복/); }
    finally { s.dispose(); }
});
