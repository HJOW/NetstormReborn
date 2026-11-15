// Session1318..1339의 실제 총·공급·표시 연결을 보호 자산/World/Host에서 검사한다(WB04-E580).
// 전체 Load/Runtime·원본 게임·공중3종 전용 행동·새 원격MP 검사는 수행하지 않는다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { GameAssets } from '../www/assets/game-assets.js';
import { TypeTable } from '../www/assets/type-table.js';
import { SoundList } from '../www/audio/sound-list.js';
import { RawCampaignSessionOwners } from '../www/core/raw-campaign-session-owners.js';
import { createCursorManaProcess } from '../www/core/raw-cursor-mana.js';
import { createCursorRangeProcess } from '../www/core/raw-cursor-range.js';
import { realCaptureAssets } from './capture-assets-support.mjs';
import { get, put, floatToBits, bitsToFloat } from './raw-support.mjs';

// 같은 보호 자료만 읽으며 시험마다 실제 소유 자료/월드를 새로 구성한다.
const data = realCaptureAssets(); const game = new GameAssets(data.table, data.database);
const files = { read() { throw new Error('생성자 구간의 VFS 읽기 금지'); } };
// 이 구간에서 호출되지 않아야 하는 외부 장치다.
function forbidden() { throw new Error('선택되지 않은 실제 장치'); }
// 필수 소리 경계만 공급한다. 새 합성 월드나 Session 대체 객체는 사용하지 않는다.
function scene(extra = {}, source = game) { return new RawCampaignSessionOwners(source, files, 'thewarbegins', { unixSeconds: () => 1700000000, soundNames: new SoundList(), sound: { wallSeconds: () => 0, playLoopAt: forbidden, playOnceAt: forbidden, play: forbidden, stop: forbidden, isPlaying: forbidden }, ...extra }); }
// 실제 TYPE 보충 구간까지 원문 순서로 연결한다.
function beforeViews(o) { o.initializePlayersAndSp(); o.connectPlayerTablesAndPackets(); o.connectIslandsAndPriests(); o.connectPlayersAndOwnership(); o.connectBodiesAndTypeTables(); }
// 이번 생성 구간까지 한 번 연결한다.
function connect(o) { beforeViews(o); o.connectGunAndCursorViews(); }
// 실제 Rain 제네레이터를 생성하고 현재 공급 장부의 시험 입력으로 등록한다. 후속 자산 PostPop 연결을 완료했다고 세지 않는다.
function provider(o, owner = 1) {
    const sid = o.world.create(76, 2); const raw = o.core.pool.allocated(sid); raw[34] = owner; put(raw, 14, floatToBits(20)); put(raw, 18, floatToBits(20)); o.core.bookkeeping.currentProviders().append(sid); return sid;
}

test('Session 총/커서 구성: 같은 좌표·반경·30칸 목록과 앞 단계/반복/반납 관문을 유지한다', () => {
    const o = scene();
    try {
        assert.equal(o.energySources.capacity(), 30); assert.equal(o.energySources.readCount(), 0); assert.equal(o.energySources.entries.every((value) => value === 0), true);
        assert.equal(o.cursorRangeGlobals.coordinates, o.cursorManaGlobals); assert.equal(o.cursorRangeGlobals.targeting, o.state.gunTargetState);
        o.gameplayGlobals.worldCells().value = 128; o.gameplayGlobals.manaRadius().value = 19.125; assert.equal(o.cursorManaGlobals.mapLimit, 128); assert.equal(o.cursorManaGlobals.radiusLimit, 19.125);
        o.cursorManaGlobals.mapLimit = 96; o.cursorManaGlobals.radiusLimit = 13.123456789; assert.equal(o.gameplayGlobals.worldCells().value, 96); assert.equal(o.gameplayGlobals.manaRadius().value, Math.fround(13.123456789));
        assert.throws(() => o.connectGunAndCursorViews(), /초기화 전/); assert.equal(o.gunMode, null); connect(o); assert.equal(o.gunViewModules.ready, true);
        assert.equal(o.gunMode.pool, o.core.pool); assert.equal(o.gunTarget.pool, o.core.pool); assert.equal(o.energySupply.pool, o.core.pool);
        assert.equal(o.cursorMana.globals, o.cursorManaGlobals); assert.equal(o.cursorRange.globals, o.cursorRangeGlobals); assert.equal(o.cursorMana.cursor, o.cursorDrop); assert.equal(o.cursorRange.clock, o.core.processState);
        assert.throws(() => o.connectGunAndCursorViews(), /반복/); assert.throws(() => o.gameplayGlobals.requireSession(), /미연결/);
    } finally { o.dispose(); }
    assert.throws(() => o.connectGunAndCursorViews(), /반납/);
});

test('Session 총 검색: 현재 모드 타입·실제 프레임과 같은 Gun 표/중심을 읽는다', () => {
    const o = scene();
    try {
        connect(o); const parent = o.world.create(140, 2); const raw = o.core.pool.allocated(parent); raw[30] = 3; assert.equal(o.gunMode.get(parent), 6);
        o.state.gunState.windArcherType = 71; raw[10] = 71; raw[30] = 2; assert.equal(o.gunMode.get(parent), 4);
        raw[40] |= 8; put(raw, 36, 0); const side = o.core.frames[71].codes[0].side; const mode = side * 2 - 152; assert.equal(o.gunMode.get(parent), mode < 0 || mode > 7 ? 0 : mode);
        const cannon = o.world.create(128, 2); const body = o.core.pool.allocated(cannon); put(body, 14, floatToBits(100)); put(body, 18, floatToBits(110));
        const center = o.cursorMana.hooks.center(cannon); assert.deepEqual(center, o.gunTarget.center(cannon)); put(body, 14, floatToBits(120)); assert.notDeepEqual(o.cursorRange.hooks.world.center(cannon), center);
        o.gunTypes[128].directionFrames[0] = 7; assert.equal(o.cursorRange.hooks.directionFrame(cannon, 0), 1);
        const rain = o.world.create(129, 2); o.gunTypes[129].directionFrames[0] = 7; assert.equal(o.cursorRange.hooks.directionFrame(rain, 0), 7); o.gunTypes[128].range = 12.5; assert.equal(o.cursorRange.hooks.range(128), 12.5);
        assert.deepEqual(o.cursorRange.hooks.footprint(cannon), o.gunTarget.footprint(cannon, false));
    } finally { o.dispose(); }
});

test('Session 에너지: 실제 장기 공급 목록·같은 선택 배열/개수와 방향별 동맹을 사용한다', () => {
    const o = scene();
    try {
        connect(o); const sid = provider(o); const providers = o.core.bookkeeping.currentProviders(); assert.equal(providers, o.world.storage.sidLists.providers); assert.notEqual(providers, o.core.bookkeeping.providers);
        const selected = o.energySources.entries; assert.deepEqual(o.energySupply.evaluate('이전', 19, 19, 75, 1), { value: 1, output: 'S' }); assert.equal(o.energySources.count, 1); assert.equal(selected[0], sid);
        assert.equal(o.energySupply.takeSource(), sid); assert.equal(o.energySources.entries, selected); assert.equal(selected[0], 0); assert.equal(o.energySources.count, 1);
        o.alliances.fill(0); assert.equal(o.energySupply.evaluate('', 19, 19, 75, 2).value, 0); o.alliances[1 * 9 + 2] = 1; assert.equal(o.energySupply.evaluate('', 19, 19, 75, 2).value, 1);
        o.alliances[11] = 0; o.alliances[19] = 1; assert.equal(o.energySupply.evaluate('', 19, 19, 75, 2).value, 0);
        providers.removeAll(sid); o.core.bookkeeping.providers.entries = [sid]; o.core.bookkeeping.providers.count = 1; assert.equal(o.energySupply.collect(19, 19, 75, 1), '');
    } finally { o.dispose(); }
});

test('Session 에너지 현재 전역: editor·special·mission과 같은 반경 상태를 매번 조회한다', () => {
    const o = scene();
    try {
        connect(o); const sid = provider(o); o.alliances.fill(0); assert.equal(o.energySupply.collect(19, 19, 75, 2), ''); o.globals.editor = 1; assert.equal(o.energySupply.collect(19, 19, 75, 2), 'r'); o.globals.editor = 0;
        o.energyState.radiusSquared = 0; assert.equal(o.energySupply.covers(sid, [], 19, 19, 0, 0), false); o.energyState.radiusSquared = 484; assert.equal(o.energySupply.covers(sid, [], 19, 19, 0, 0), true);
        o.energyState.ignore = 1; o.energyState.userCheat = 1; o.globals.missionPost = 0; o.playerState.special = false; assert.equal(o.energySupply.evaluate('유지', 19, 19, 75, 2).value, 0);
        o.globals.missionPost = 1; assert.deepEqual(o.energySupply.evaluate('유지', 19, 19, 75, 2), { value: 1, output: '유지' }); o.globals.missionPost = 0; o.playerState.special = true; assert.deepEqual(o.energySupply.evaluate('특수', 19, 19, 75, 2), { value: 1, output: '특수' });
    } finally { o.dispose(); }
});

test('Session 에너지 요구: 같은 보호 정의와 현재 TYPE의 테마/레벨을 재조회한다', () => {
    const definitions = data.table.definitions.slice(); definitions[0] = { ...definitions[0], properties: [...definitions[0].properties, { name: 'mana', value: '' }] }; const source = new GameAssets(new TypeTable(definitions), data.database); const o = scene({}, source);
    try {
        connect(o); provider(o); const current = o.core.types[70]; current.flags2 = 0; current.group = 0; current.level = 0; current.theme = 0;
        assert.deepEqual(o.energySupply.evaluate('', 19, 19, 70, 1), { value: 0, output: 'w' }); current.theme = 1; assert.deepEqual(o.energySupply.evaluate('', 19, 19, 70, 1), { value: 1, output: 'R' });
        current.level = 1; assert.deepEqual(o.energySupply.evaluate('', 19, 19, 70, 1), { value: 0, output: 'Rs' });
    } finally { o.dispose(); }
});

test('Session 마나 표시: 실제 Host·World의 같은8표시와 현재 시각/반경·종료 수명을 사용한다', () => {
    const o = scene();
    try {
        connect(o); const parent = o.world.create(123, 2); const raw = o.core.pool.allocated(parent); put(raw, 14, floatToBits(100)); put(raw, 18, floatToBits(110)); o.core.processState.now = 17.25; o.cursorManaGlobals.radiusLimit = 15;
        const process = createCursorManaProcess(o.core.processes, o.cursorMana, parent); o.cursorDrop.cursorGump = process.token;
        assert.equal(o.core.kernel.get(process.token), process); assert.equal(o.core.processes.fromForm(process.form), process); assert.equal(get(process.fields.bytes, 0x84), 8); assert.equal(bitsToFloat(get(process.fields.bytes, 0x8c)), 15);
        const markers = Array.from({ length: 8 }, (_, index) => get(process.fields.bytes, 12 + 4 * index));
        // 같은8개 실제 마나 표시의 TYPE/WORD/extra를 확인한다.
        for (const sid of markers) { const marker = o.core.pool.allocated(sid); assert.equal(marker[10], 103); assert.equal(marker[12], 1); assert.equal(Boolean(marker[40] & 8), true); }
        o.core.processState.now += 0.125; process.runFrame(); assert.notEqual(get(o.core.pool.slot(markers[0]), 14), floatToBits(1)); const token = process.token; process.notify(3, 0, 0);
        o.core.processState.now += 0.125; process.runFrame(); o.core.processState.now += 0.125; process.runFrame(); assert.equal(o.core.kernel.get(token) === null, true); assert.equal(o.cursorDrop.cursorGump, 0);
    } finally { o.dispose(); }
});

test('Session 원형 사정거리: 같은 Gun 범위·A Run·실제8표시와 현재 Host 종료를 사용한다', () => {
    const o = scene();
    try {
        connect(o); const parent = o.world.create(71, 2); const raw = o.core.pool.allocated(parent); put(raw, 14, floatToBits(100)); put(raw, 18, floatToBits(110)); o.gunTypes[71].range = 18.75;
        const process = createCursorRangeProcess(o.core.processes, o.cursorRange, parent); o.cursorDrop.cursorGump = process.token; assert.equal(get(process.fields.bytes, 0x1a4), 8); assert.equal(bitsToFloat(get(process.fields.bytes, 0x1b0)), 18.75);
        const markers = Array.from({ length: 8 }, (_, index) => get(process.fields.bytes, 20 + 4 * index));
        // 같은8개 실제 사정거리 표시를 별도 표시 사본과 구별한다.
        for (const sid of markers) assert.equal(o.core.pool.allocated(sid)[10], 104);
        o.core.processState.now += 0.125; process.runFrame(); const token = process.token; process.notify(3, 0, 0);
        o.core.processState.now += 0.125; process.runFrame(); o.core.processState.now += 0.125; process.runFrame(); assert.equal(o.core.kernel.get(token) === null, true); assert.equal(o.cursorDrop.cursorGump, 0);
    } finally { o.dispose(); }
});

test('Session 사정거리 입력: 실제 signed SHP·현재 Run과 명시 자산/범위 관문을 유지한다', () => {
    const o = scene();
    try {
        connect(o); const h = o.cursorRange.hooks; const header = game.shapes.squidHeader(58, 0); const v = new DataView(header.buffer, header.byteOffset, header.byteLength); assert.deepEqual(h.firingOffset(128, 0), [v.getInt16(32, true), v.getInt16(34, true)]);
        assert.deepEqual(h.frameRun(104, 1), o.core.frames[104].run('B')); assert.throws(() => h.range(188), /TYPE 범위/); assert.throws(() => h.range(-1), /TYPE 범위/);
        assert.throws(() => h.firingOffset(69, 0), /자산 범위/); assert.throws(() => h.firingOffset(186, 0), /자산 범위/); assert.throws(() => h.firingOffset(128, -1), /자산 범위/); assert.throws(() => h.frameRun(188, 0), /Run 범위/); assert.throws(() => h.frameRun(104, 16), /Run 범위/);
    } finally { o.dispose(); }
});

test('Session 직선 사정거리: 실제 썬 캐논의 모드·방향·SHP 포구로 표시를 생성하고 이동한다', () => {
    const o = scene();
    try {
        connect(o); const parent = o.world.create(128, 2); const raw = o.core.pool.allocated(parent); put(raw, 14, floatToBits(100)); put(raw, 18, floatToBits(110)); o.gunTypes[128].range = 20;
        const process = createCursorRangeProcess(o.core.processes, o.cursorRange, parent); const token = process.token; const count = get(process.fields.bytes, 0x1ac); assert.ok(count > 0 && count <= 100); assert.equal(get(process.fields.bytes, 12), 0);
        o.core.processState.now += 0.5; process.runFrame(); let visible = 0;
        // 실제 생성 목록 중 SID0이 아닌 같은 표시의 공간 등록을 확인한다.
        for (let index = 0; index < 100; index += 1) { const sid = get(process.fields.bytes, 20 + 4 * index); if (sid && !(o.core.pool.slot(sid)[11] & 4)) visible += 1; }
        assert.ok(visible > 0); process.notify(3, 0, 0); process.runFrame(); assert.equal(o.core.kernel.get(token) === null, true);
    } finally { o.dispose(); }
});

test('Session 사정거리 서비스: 제공 진단의 값 복사와 실제 World 표시 효과를 보존한다', () => {
    const events = []; const extra = { cursorRangeDiagnostic: (...args) => events.push(args) }; const o = scene(extra); extra.cursorRangeDiagnostic = forbidden;
    try {
        connect(o); o.cursorRange.hooks.diagnostic(29, '현재 진단'); assert.deepEqual(events, [[29, '현재 진단']]); const sid = o.cursorMana.hooks.create(103, 2); o.cursorMana.hooks.setWord(sid, 0x12345678); assert.equal(get(o.core.pool.allocated(sid), 12, 2), 0x5678);
        o.cursorMana.hooks.position(sid, 20, 30, 0); assert.equal(get(o.core.pool.slot(sid), 14), floatToBits(20)); o.cursorRange.hooks.world.detach(sid, 0); o.cursorRange.hooks.world.unpop(sid, 0); o.cursorRange.hooks.world.destroy(sid, 0);
    } finally { o.dispose(); }
});

test('Session 총/커서 부분 실패: 앞선 실제 Gun은 남기고 공급/커서 뒤 생성과 반복을 거부한다', () => {
    const o = scene();
    try {
        beforeViews(o); o.alliances = new Uint32Array(8); assert.throws(() => o.connectGunAndCursorViews(), /에너지 실제 월드/); assert.ok(o.gunMode); assert.ok(o.gunTarget); assert.equal(o.energySupply, null); assert.equal(o.cursorMana, null); assert.equal(o.cursorRange, null); assert.equal(o.gunViewModules.ready, false); assert.throws(() => o.connectGunAndCursorViews(), /반복/);
    } finally { o.dispose(); }
    const use = o.gameplayGlobals.acquire(); use.release(); const players = o.playerRecords.acquire(); players.release();
});

test('Session 표시 종료: 살아 있는 실제 마나/사정거리 프로세스를 논리 반납 전에 정리한다', () => {
    const o = scene(); connect(o); const parent = o.world.create(71, 2); const mana = createCursorManaProcess(o.core.processes, o.cursorMana, parent); const range = createCursorRangeProcess(o.core.processes, o.cursorRange, parent); const tokens = [mana.token, range.token]; o.cursorDrop.cursorGump = range.token;
    assert.equal(o.core.kernel.size, 2); o.dispose();
    // 반납 전 저장한 실제 두 Kernel 토큰이 더 이상 등록되지 않았는지 확인한다.
    for (const token of tokens) assert.equal(o.core.kernel.get(token) === null, true);
    assert.equal(o.core.kernel.size, 0); assert.equal(o.cursorDrop.cursorGump, 0); assert.equal(o.world.storage.borrowed, false); o.dispose(); assert.throws(() => o.gunViewModules.bind(), /초기화 전|반복/);
});
