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
import { ClientPlayerRecords } from '../www/core/client-player-records.js';
import { RawMissionAiSettings } from '../www/core/raw-mission-ai-settings.js';
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
        templateStorage: overrides.templateStorage, playerRecords: overrides.playerRecords, missionOptions: overrides.missionOptions, fortMission: overrides.fortMission, production: overrides.production, newTechnology: overrides.newTechnology,
        sound: { wallSeconds: () => 0, play: forbidden('소리'), playOnceAt: forbidden('위치 소리'), playLoopAt: forbidden('반복 소리'), stop: forbidden('정지'), isPlaying: () => false },
        player: { log: () => trace.push('log') }, construction: overrides.construction, missionOutcome: overrides.missionOutcome,
        ai: { assertion: forbidden('AI 진단'), ...overrides.ai },
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

// 원본 기본 AI 설정의 같은 600BYTE 입력에 명시 이름·기술·초기 SP만 지정한다.
function localAiProfile() {
    const profile = new Uint8Array(600).fill(0xa7);
    new RawMissionAiSettings({ stuffRefreshRate: 3, startingSp: 2500 }, { get: forbidden('AI 설정') }).initialize(profile);
    profile.set(new TextEncoder().encode('Explicit\0'), 0); profile.set(new TextEncoder().encode('sunCannon\0'), 30); new DataView(profile.buffer).setUint32(0x22c, 777, true);
    return profile;
}

// 실제 두 Player 지역에 생성한 간헐천이 같은 AI 장부에 등록되고 AI를 반납한다.
test('CampaignSession Tutorial 1: 로컬 간헐천과 실제 AI 등록', () => {
    const records = new ClientPlayerRecords(), notices = [];
    const options = {
        playerRecords: records, construction: { wallSeconds: () => 1, configInt: () => 0, refresh: () => notices.push('refresh') }, missionOutcome: { demo: () => 0 },
        ai: { allocationBytes: (bytes) => bytes.fill(0x71), participation: { clearChat: () => notices.push('clear'), addChat: () => notices.push('add') } },
    };
    const { s } = scene(options);
    try {
        s.load(); const token = s.createAi(2, localAiProfile()); records.records()[180 + 0x50] = 2; assert.equal(s.prepareAi(2), true);
        const raw = s.aiMemory().bytes(token), view = new DataView(raw.buffer, raw.byteOffset, raw.byteLength), before = view.getUint32(0x32c, true);
        const count = s.world().objects().filter(({ object }) => object.type === 122).length; s.generateLocalGeysers();
        assert.equal(view.getUint32(0x32c, true), before + 6); assert.equal(s.world().objects().filter(({ object }) => object.type === 122).length, count + 6);
        assert.equal(records.records()[180 + 0x50], 2); assert.equal(records.records()[360 + 0x50] & 0x22, 0x20); assert.equal(s.participation().brain(2), token);
        assert.equal(notices.filter((item) => item === 'refresh').length, 6); s.removeAi(2); assert.equal(s.participation().brain(2), 0);
    } finally { s.dispose(); }
});

// 전체 생성은 로컬 생성 뒤 현재 옵션의 세계 배치까지 같은 World·AI 장부에 반영한다.
test('CampaignSession Tutorial 1: 세계 간헐천 전체 생성과 AI 등록', () => {
    const records = new ClientPlayerRecords(), options = new Uint8Array(87), notices = []; options[18] = 1;
    const { s } = scene({
        playerRecords: records, missionOptions: options, construction: { wallSeconds: () => 1, configInt: () => 0, refresh: () => notices.push('refresh') }, missionOutcome: { demo: () => 0 },
        ai: { allocationBytes: (bytes) => bytes.fill(0x71), participation: { clearChat: () => notices.push('clear'), addChat: () => notices.push('add') } },
    });
    try {
        s.load(); const token = s.createAi(2, localAiProfile()); records.records()[180 + 0x50] = 2; assert.equal(s.prepareAi(2), true);
        const raw = s.aiMemory().bytes(token), view = new DataView(raw.buffer, raw.byteOffset, raw.byteLength), before = view.getUint32(0x32c, true);
        const count = s.world().objects().filter(({ object }) => object.type === 122).length; s.generateWorldGeysers();
        assert.equal(view.getUint32(0x32c, true), before + 17); assert.equal(s.world().objects().filter(({ object }) => object.type === 122).length, count + 17);
        assert.equal(s.participation().brain(2), token); assert.ok(notices.includes('refresh')); s.removeAi(2); assert.equal(s.participation().brain(2), 0);
    } finally { s.dispose(); }
});

// 실제 기술 부모에 같은 종류를 한 번만 넣고 반복 획득에도 현재 Battle snapshot과 안내를 수행한다.
test('CampaignSession Tutorial 1: 실제 기술 획득과 반복 snapshot', () => {
    const storage = new CampaignTemplateStorage(), events = [];
    const options = {
        templateStorage: storage, fortMission: { current: () => 0 }, production: { allocationBytes: (bytes) => bytes.fill(0xa5), effects: { configInt: () => 0 } },
        newTechnology: { name: (type) => game.types()[type - 70].definition.name, configure: (key, value) => events.push([key, value]), format: (pattern, type) => pattern.replace('%d', String(type)), tell: (name) => events.push(['tell', name]) },
    };
    const { s } = scene(options), type = game.typeTable.find('sunCannon');
    try {
        s.load(); assert.equal(type, 128); s.learnNewTechnology(type);
        const first = fortSection(storage.template(1), 'Technology').slice(), firstMoney = fortSection(storage.template(1), 'Money').slice();
        assert.deepEqual([...first], [1, 128, 4]); s.learnNewTechnology(type);
        assert.deepEqual(fortSection(storage.template(1), 'Technology'), first); assert.notDeepEqual(fortSection(storage.template(1), 'Money'), firstMoney);
        assert.deepEqual(events, [['global.newTechName', 'suncannon'], ['global.newTechPic', '<!`"128.*`">'], ['tell', 'NewTech'], ['global.newTechName', 'suncannon'], ['global.newTechPic', '<!`"128.*`">'], ['tell', 'NewTech']]);
    } finally { s.dispose(); }
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
