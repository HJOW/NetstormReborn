// 실제 Session 기반 자료의 Player/Template/Regular/섬 소유권 결선을 대조한다(WB04-E578).
// 실제 보호 자산과 기존 월드/저장소/Host를 사용하며 전체 Session Load나 플레이 완료를 대신하지 않는다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { GameAssets } from '../www/assets/game-assets.js';
import { SECTION_NAMES } from '../www/assets/fort.js';
import { SoundList } from '../www/audio/sound-list.js';
import { RawCampaignSessionOwners } from '../www/core/raw-campaign-session-owners.js';
import { realCaptureAssets } from './capture-assets-support.mjs';
import { put, get, floatToBits, bitsToFloat } from './raw-support.mjs';

// 같은 보호 자산을 빌리며 실제 월드와 Player는 시험마다 새로 소유한다.
const data = realCaptureAssets(); const game = new GameAssets(data.table, data.database);
const files = { read() { throw new Error('이 생성 구간의 VFS 읽기 금지'); } };
// 이 구간에서 선택되지 않아야 하는 외부 효과다.
function forbidden() { throw new Error('선택되지 않아야 하는 효과'); }
// 실제 필수 시각/소리 경계만 공급한다. 이 구간은 재생을 요구하지 않는다.
function scene(extra = {}) { return new RawCampaignSessionOwners(game, files, 'thewarbegins', { unixSeconds: () => 1700000000, soundNames: new SoundList(), sound: { wallSeconds: () => 0, playLoopAt: forbidden, playOnceAt: forbidden, play: forbidden, stop: forbidden, isPlaying: forbidden }, ...extra }); }
// Player/SP→테이블→섬/사제→이번 생성자 구간을 원문 순서로 실행한다.
function connect(o) { o.initializePlayersAndSp(); o.connectPlayerTablesAndPackets(); o.connectIslandsAndPriests(); o.connectPlayersAndOwnership(); return o.playerModules; }
// C++ span처럼 같은 물리 범위인지 비교한다. 같은 저장소의 새 JS 뷰 객체도 허용한다.
function samePhysical(left, right) { return left.buffer === right.buffer && left.byteOffset === right.byteOffset && left.byteLength === right.byteLength; }
// State 단위 입력을 실제 35섹션 manager에 설치한다. 캠페인 파일 Load를 수행한 것으로 세지 않는다.
function template(o, slot, state = new Uint8Array(0), name = 'state.fort') {
    const sections = Array.from({ length: SECTION_NAMES.length }, () => new Uint8Array(0)); sections[SECTION_NAMES.indexOf('State')] = state;
    o.templateStorage.install(slot, { sections }, name);
}

test('Session Player/소유권: 앞 단계 뒤 같은 실제 기록·상태·섬·테마 벡터를 연결한다', () => {
    const o = scene();
    try {
        assert.throws(() => o.connectPlayersAndOwnership(), /초기화 전/); assert.equal(o.player, null); const m = connect(o);
        assert.equal(m.ready, true); assert.equal(o.player.pool, o.core.pool); assert.equal(o.player.records, o.players); assert.equal(o.player.state, o.playerState); assert.equal(o.playerRegular.pool, o.core.pool);
        assert.equal(o.ownership.vortex().pool, o.core.pool); assert.equal(o.ownership.vortex().records, o.players); assert.equal(o.paint.sharedWorldRedraw, o.worldRedraw); assert.equal(o.votes.sharedWorldRedraw, o.worldRedraw); assert.equal(o.votes.sharedMapRevision, o.gameplayGlobals.mapRevision());
        assert.equal(o.themes.length, 188); assert.equal(o.elements.length, 188); assert.equal(o.themes.every((value) => value === 0), true); assert.equal(o.elements.every((value) => value === 0), true);
        assert.throws(() => o.connectPlayersAndOwnership(), /반복/); assert.throws(() => o.gameplayGlobals.requireSession(), /미연결/);
    } finally { o.dispose(); }
    assert.throws(() => o.connectPlayersAndOwnership(), /반납/);
});

test('Session Player152: 실제 생성/Owner/Pop이 같은 Technology 기록과 첫 Regular를 만든다', () => {
    const o = scene();
    try {
        connect(o); const sid = o.player.create(1); const raw = o.core.pool.slot(sid);
        assert.equal(raw[10], 152); assert.equal(raw[34], 1); assert.equal(get(raw, 36), 0); assert.equal(bitsToFloat(get(raw, 14)), 11); assert.equal(bitsToFloat(get(raw, 18)), 1);
        assert.equal(get(o.players, 0xb4 + 0x9c), sid); assert.equal(o.player.currentTechnology(), sid); assert.equal(o.player.technology(1), sid);
        const regular = o.core.processes.findEvent(sid, 1); assert.ok(regular); assert.equal(regular.payload, 5); assert.equal(regular.parent, sid);
    } finally { o.dispose(); }
});

test('Session Player 진단: 현재 AI 장치가 같은 기록을 바꾼 뒤 실제 Technology를 다시 읽는다', () => {
    const o = scene(); const events = [];
    try {
        connect(o); o.playerState.playerKnown = 0; assert.equal(o.player.currentTechnology(), 0); assert.throws(() => o.player.technology(1), /Player 실제 진단 장치/);
        o.services.ai = { assertion: (expression) => { events.push(expression); put(o.players, 0xb4 + 0x9c, 0x12345678); } };
        assert.equal(o.player.technology(1), 0x12345678); assert.deepEqual(events, ['playerIdKnown']);
    } finally { o.dispose(); }
});

test('Session Template: 실제 빈 State 생성이 미정 입력 바이트와 같은 현재 stream/저장소를 사용한다', () => {
    const depths = []; let o; o = scene({ templateStateBytes: (bytes) => { assert.equal(bytes.length, 28); depths.push(o.objectState.loadingDepth); bytes.fill(0xa5); } });
    try {
        connect(o); template(o, 1); o.objectState.loadingDepth = 7; const bytes = o.templateState.get(1);
        const expected = new Uint8Array(28).fill(0xa5); expected[0] = 0xa1; expected.fill(0, 4, 10); expected.fill(0, 12, 18); expected.fill(0, 22, 26);
        assert.deepEqual(bytes, expected); assert.equal(samePhysical(bytes, o.templateStorage.state(1).bytes), true); assert.deepEqual(depths, [7]); assert.equal(o.objectState.loadingDepth, 7);
        assert.deepEqual([o.templateStorage.state(1).position, o.templateStorage.state(1).end], [28, 28]); const current = o.templateState.get(1); assert.equal(samePhysical(current, bytes), true); bytes[1] = 0x23; assert.equal(current[1], 0x23); assert.equal(o.templateStorage.state(1).position, 0); assert.deepEqual(depths, [7]);
    } finally { o.dispose(); }
});

test('Session Template: 짧은 State를 같은 실제 최소 확장 부모로 교체하고 원래 payload를 보존한다', () => {
    const o = scene({ templateStateBytes: forbidden });
    try {
        connect(o); template(o, 2, Uint8Array.of(1, 2, 3, 4, 5)); const old = o.templateStorage.state(2).bytes;
        const bytes = o.templateState.get(2); assert.equal(bytes.length, 28); assert.notEqual(bytes.buffer, old.buffer); assert.deepEqual(Array.from(bytes.slice(0, 5)), [1, 2, 3, 4, 5]); assert.equal(bytes.slice(5).every((value) => value === 0), true);
        assert.equal(samePhysical(bytes, o.templateStorage.state(2).bytes), true); assert.deepEqual([o.templateStorage.state(2).position, o.templateStorage.state(2).end], [28, 28]);
    } finally { o.dispose(); }
});

test('Session Template 실패: manager/미정 바이트/Notify의 현재 관문에서 앞선 cursor/flag를 유지한다', () => {
    const o = scene();
    try {
        connect(o); assert.throws(() => o.templateState.get(1), /1246/); template(o, 1); assert.throws(() => o.templateState.get(1), /미정 바이트 입력 미연결/); assert.equal(o.templateStorage.state(1).position, 0); assert.equal(o.templateStorage.state(1).end, 0);
        template(o, 2, new Uint8Array(28)); put(o.headers, 2 * 20 + 8, 1); assert.doesNotThrow(() => o.templateState.changed(2));
        put(o.headers, 2 * 20 + 8, 0); assert.throws(() => o.templateState.changed(2), /1246/); o.objectState.mission = false; put(o.headers, 2 * 20 + 16, 1); assert.doesNotThrow(() => o.templateState.changed(2));
    } finally { o.dispose(); }
});

test('Session Player Regular: 실제 최초 Regular와 같은 Host로 두 사건을 연결한다', () => {
    const events = []; const o = scene({ player: { log: (...args) => events.push(args) } });
    try {
        connect(o); const sid = o.player.create(1); o.core.ownerMode.battle = false;
        assert.equal(o.playerRegular.handle(sid, 1, 0, 5), 5); assert.deepEqual(events, []); assert.equal(o.playerRegular.handle(sid, 1, 1, 5), 5); assert.deepEqual(events, [[29, 'SORE LOSER TEST: '], [29, 'NOT STARTING YET.\n']]);
        o.core.ownerMode.battle = true; assert.equal(o.playerRegular.handle(sid, 1, 1, 5), 0); assert.equal(o.core.processes.findEvent(sid, 2).payload, 60);
        template(o, 1, new Uint8Array(28)); put(o.headers, 20 + 8, 1); assert.equal(o.playerRegular.handle(sid, 2, 1, 60), 0); assert.equal(o.templateStorage.state(1).bytes[0], 4);
        assert.throws(() => o.playerRegular.handle(sid, 99, 1, 0), /1250/);
    } finally { o.dispose(); }
});

test('Session Player Regular: State getter 뒤 현재 templateOwner를 다시 읽어 Notify한다', () => {
    let o; o = scene({ templateStateBytes: (bytes) => { bytes.fill(0); o.playerState.templateOwner = 2; } });
    try {
        connect(o); const sid = o.player.create(1); o.core.ownerMode.battle = true; template(o, 1); template(o, 2, new Uint8Array(28), '.target'); put(o.headers, 2 * 20 + 8, 1);
        assert.throws(() => o.playerRegular.handle(sid, 2, 1, 60), /1246/); assert.equal(o.playerState.templateOwner, 2); assert.equal(o.templateStorage.state(1).bytes[0] & 4, 4); assert.equal(o.templateStorage.state(2).bytes[0], 0);
    } finally { o.dispose(); }
});

test('Session 소유권: 실제 신전 Owner는 같은 Player 두 DWORD와 개정·생산을 갱신한다', () => {
    let refresh = 0; const o = scene({ ownership: { refreshProduction: () => { refresh += 1; }, themeMode: () => 1 } });
    try {
        connect(o); const sid = o.world.create(123); assert.ok(o.core.pool.slot(sid)[11] & 4); o.ownership.setOwner(sid, 1);
        assert.equal(o.core.pool.slot(sid)[34], 1); assert.equal(get(o.players, 0xb4 + 0x7c), sid); assert.equal(get(o.players, 0xb4 + 0x80), sid); assert.equal(refresh, 1); assert.equal(o.revision, 1);
        const island = o.islands.add(1, 0); o.islands.setBounds(island, { left: 10, top: 10, right: 50, bottom: 50 }); o.islands.setOwner(island, 2);
        assert.equal(get(o.players, 2 * 0xb4 + 0x78), island); assert.equal(o.finishState.playerRegions[2], 127); assert.equal(o.worldRedraw.value, 1); assert.equal(o.revision, 1);
    } finally { o.dispose(); }
});

test('Session 신전 진단: 실제 섬 조회 실패의 상세/단언과 앞선 Player 쓰기를 보존한다', () => {
    const o = scene({ ownership: { refreshProduction: forbidden, themeMode: forbidden } }); const detail = { value: '' }; const events = [];
    try {
        connect(o); const sid = o.world.create(123); const raw = o.core.pool.allocated(sid); raw[11] &= ~4; raw[34] = 1; put(raw, 14, floatToBits(10.75)); put(raw, 18, floatToBits(20.9)); put(o.players, 0xb4 + 0x7c, sid); put(o.players, 0xb4 + 0x80, sid);
        o.world.storage.bindDisplayDiagnostics(detail, (...args) => { events.push(args); throw new Error('진단 경계 중단'); }); assert.throws(() => o.ownership.setOwner(sid, 2), /진단 경계 중단/);
        assert.match(detail.value, /islandList.totalIslandIds=0\n/); assert.match(detail.value, /pos=\(10,20\)\noldPlayerId=1\nnewPlayerId=2\n/); assert.deepEqual(events, [['0', '\\Ns\\O\\Vortex.cpp', 0x235]]);
        assert.equal(get(o.players, 0xb4 + 0x7c), 0); assert.equal(get(o.players, 0xb4 + 0x80), 0); assert.equal(raw[34], 1);
    } finally { o.dispose(); }
});

test('Session 서비스/누락: 제공한 로그·생산 함수를 값형으로 보존하고 선택한 누락만 실패한다', () => {
    const events = []; const player = { log: () => events.push('log') }; const ownership = { refreshProduction: () => events.push('refresh'), themeMode: () => 1 }; const o = scene({ player, ownership });
    try {
        player.log = forbidden; ownership.refreshProduction = forbidden; connect(o); const sid = o.player.create(1); o.core.ownerMode.battle = false; o.playerRegular.handle(sid, 1, 1, 5); assert.deepEqual(events, ['log', 'log']); o.ownership.setOwner(o.world.create(123), 1); assert.equal(events.at(-1), 'refresh');
        assert.equal(player.addRegular, undefined); assert.equal(ownership.otherOwner, undefined);
    } finally { o.dispose(); }
    const missing = scene();
    try { connect(missing); const sid = missing.player.create(1); assert.throws(() => missing.playerRegular.handle(sid, 1, 1, 5), /1251/); const temple = missing.world.create(123); assert.throws(() => missing.ownership.setOwner(temple, 1), /1259/); assert.equal(get(missing.players, 0xb4 + 0x7c), temple); } finally { missing.dispose(); }
});

test('Session 부분 실패: 소유권 구성 거부 전의 실제 Player/Template/Regular를 유지하고 종료 시 반납한다', () => {
    const o = scene();
    try {
        o.initializePlayersAndSp(); o.connectPlayerTablesAndPackets(); o.connectIslandsAndPriests(); o.themes = new Uint32Array(187);
        assert.throws(() => o.connectPlayersAndOwnership(), /같은 Core/); assert.ok(o.player); assert.ok(o.templateState); assert.ok(o.playerRegular); assert.equal(o.ownership, null); assert.equal(o.playerModules.ready, false); assert.throws(() => o.connectPlayersAndOwnership(), /반복/);
    } finally { o.dispose(); }
    assert.equal(o.world.storage.borrowed, false); const p = o.playerRecords.acquire(); p.release(); const g = o.gameplayGlobals.acquire(); g.release();
});
