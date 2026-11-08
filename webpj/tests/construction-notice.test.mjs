// 건설 배치 통지 raw-construction-notice.js(RawConstructionNotice·ConstructionNoticeState·두 어댑터)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: constructionnotice-x86.tsv 의 `originals` 537행 — 통지 값(타입·flags1·group·좌표·인자·방향·플레이어·flags·품질·시각·SID 개수)·전역(로컬 플레이어·권한·moveLocal·Dais)·
//    객체/사제 상태·접근점·이동 성공 여부·좌표 변동·이력 초기 순번에서 하위 효과 순서(Q 정리/P 배치/H 배치 시 이력/B 건설 사제/R 소유 사제/T contained 타입/X contained 삭제/I 이동 불가/
//    A 접근점/M 이동/F 환불/D 삭제/S 건설 시작/W 갱신)와 최종 이력 40바이트·순번이 같다. `originalCD`·`original1037` 행은 다른 판본이라 사용하지 않는다.
//  - 계약: 잘못된 통지(표 밖 타입·빈 목록·20개·SID 0·풀 밖)는 정리/이력 전에 거부, 필수 경계 누락 거부, 공유 이력 장부는 배치 전에 갱신(독립 값형은 불변), 배치 효과가 바꾼 로컬 플레이어/권한은
//    뒤쪽 분기에서 다시 읽음, 실제 wire 경로(actualHandle)는 같은 입력의 typed 경로와 같은 효과 순서·진입 때 고정한 TYPE 레코드·소비 위치마다 현재 패킷 읽기, 어댑터의 풀 불일치 거부.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E134: 건설 배치 통지 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, bitsToFloat, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { createConstructionHistory } from '../www/core/construction-history-key.js';
import { createConstructionPacketHeader, writeConstructionPacket } from '../www/core/raw-construction-packet.js';
import { ConstructionNoticeState, RawConstructionNotice, makeConstructionNoticeConfirmHooks, makeConstructionNoticeHooks } from '../www/core/raw-construction-notice.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E134)
const allRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'constructionnotice-x86.tsv')) ?? [];
const rows = allRows.filter((row) => row[0] === 'originals');

// 판본별 관측 행 수, 합성 사제 타입 번호, 풀 크기이다. (2026-11-08 WB04-E134)
const ROWS = 537;
const PRIEST_TYPE = 158;
const CAPACITY = 24000;

// 예외가 나는지 본다. (2026-11-08 WB04-E134)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 이력 40바이트(좌표 비트 little endian)를 소문자 16진수로 만든다. 기대값은 독립 원본 출력에서만 읽는다. (2026-11-08 WB04-E134)
function historyHex(points) {
    let text = '';
    // 가장 최근부터 다섯 좌표를 x, y 순서로 쓴다.
    for (const point of points) {
        for (const value of [point.x, point.y]) {
            const bits = floatToBits(value);
            for (let byte = 0; byte < 4; byte += 1) text += ((bits >>> (8 * byte)) & 0xff).toString(16).padStart(2, '0');
        }
    }
    return text;
}

// raw 풀·타입 표·전역과 효과 관찰을 행마다 새 입력으로 준비하는 장면이다. (2026-11-08 WB04-E134)
class Scene {
    constructor() {
        this.pool = new SidPool(CAPACITY, { server: false });
        this.types = Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, group: 0 }));
        this.state = new ConstructionNoticeState();
        this.events = [];
        this.priestCalls = 0;
        this.moveCalls = 0;
        this.values = [];
        this.notice = null;
    }

    // 사건을 원본 실행기와 같은 문자열(`글자:필드:…`, 필드가 없으면 `글자:`)로 잇는다. (2026-11-08 WB04-E134)
    emit(letter, ...fields) {
        this.events.push(fields.length === 0 ? `${letter}:` : `${letter}:${fields.join(':')}`);
    }

    // 입력 36개로 타입 표·전역·이력·통지·객체 슬롯을 준비한다. (2026-11-08 WB04-E134)
    prepare(input) {
        const v = input.split(',').map(Number);
        assert.equal(v.length, 36);
        this.values = v;
        for (const type of this.types) Object.assign(type, { flags1: 0, flags2: 0, group: 0 });
        this.events = [];
        this.priestCalls = 0;
        this.moveCalls = 0;
        this.state = new ConstructionNoticeState();
        this.types[v[0]].flags1 = v[1];
        this.types[v[0]].group = v[2] | 0;
        this.types[v[17]].flags2 = v[19];
        this.types[PRIEST_TYPE].flags2 = v[23];
        Object.assign(this.state, { localPlayer: v[13], authority: v[14] !== 0, moveLocalBuilder: v[15] !== 0, daisType: v[16], historyCounter: v[34] });
        // 초기 이력 비트는 실행기의 입력과 같다. 갱신 결과를 미리 계산하지 않는다.
        for (let index = 0; index < 10; index += 1) {
            const value = bitsToFloat((0x3f800000 + index * 0x01020304) >>> 0);
            if (index % 2) this.state.history[Math.floor(index / 2)].y = value;
            else this.state.history[Math.floor(index / 2)].x = value;
        }
        this.notice = { type: v[0], x: bitsToFloat(v[3]), y: bitsToFloat(v[4]), argument: v[5], direction: v[6], player: v[7], flags: v[8], quality: v[9], timeLow: v[10], timeHigh: v[11],
            sids: Array.from({ length: v[12] }, (_, index) => 50 + index) };
        this.node(50, v[0], 0);
        this.node(v[20], PRIEST_TYPE, v[22]);
    }

    // 원본 실행기에 공급한 객체 raw 슬롯과 좌표를 쓴다. (2026-11-08 WB04-E134)
    node(sid, type, flags) {
        const raw = this.pool.physical(sid);
        raw.fill(0);
        raw[10] = type;
        raw[11] = flags;
        put(raw, 14, this.values[25]);
        put(raw, 18, this.values[26]);
    }

    // 몸체 밖 효과만 관찰하며 배치/이동 경계의 합성 raw 변경도 원본 실행기와 같이 공급한다. (2026-11-08 WB04-E134)
    hooks() {
        const v = this.values;
        const bits = floatToBits;
        return {
            clear: (r) => this.emit('Q', r.type, bits(r.x), bits(r.y), r.argument, r.direction, r.player),
            place: (r) => {
                this.emit('P', r.type, bits(r.x), bits(r.y), r.argument, r.direction, r.player, r.sids.length, r.sids.join(','), r.abstract, r.quality);
                this.emit('H', historyHex(this.state.historyPoints()), this.state.historyCounterNow());
                const raw = this.pool.physical(r.sids[0]);
                raw[10] = v[17];
                raw[34] = v[18];
            },
            moveBuilder: (player, sid, low, high) => this.emit('B', player, sid, low, high),
            findPriest: (player) => {
                const sid = v[this.priestCalls++ === 0 ? 20 : 21];
                this.emit('R', player, sid);
                return sid;
            },
            containedType: (type) => { this.emit('T', type, v[35]); return v[35]; },
            removeContained: (sid, type) => this.emit('X', sid, type, 0, 1),
            immobile: (sid) => { this.emit('I', sid, v[24]); return v[24] !== 0; },
            approach: (object, x, y, flags) => {
                const index = flags === 7 ? 27 : 29;
                this.emit('A', object, bits(x), bits(y), flags, v[index], v[index + 1]);
                return { x: bitsToFloat(v[index]), y: bitsToFloat(v[index + 1]) };
            },
            tryMove: (priest, x, y, object) => {
                const success = v[this.moveCalls === 0 ? 31 : 32];
                this.emit('M', priest, bits(x), bits(y), object, success);
                if (this.moveCalls === 0 && v[33]) {
                    const raw = this.pool.physical(priest);
                    put(raw, 14, bits(15.25));
                    put(raw, 18, bits(16.5));
                }
                this.moveCalls += 1;
                return success !== 0;
            },
            refund: (type, owner) => this.emit('F', type, owner),
            destroy: (sid, flags) => this.emit('D', sid, flags),
            start: (sid, flags) => this.emit('S', sid, flags),
            refresh: () => this.emit('W'),
        };
    }

    // 현재 장면으로 통지 처리기를 만든다. (2026-11-08 WB04-E134)
    handler(hooks = this.hooks()) {
        return new RawConstructionNotice(this.pool, this.types, this.state, hooks);
    }
}

test('통지: 원본 10.78 관측 537행의 효과 순서·배치 시점 이력·최종 이력·순번이 같다', { skip: rows.length === 0 }, () => {
    assert.equal(rows.length, ROWS);
    const scene = new Scene();
    // 각 행은 독립된 입력이다.
    for (const row of rows) {
        assert.equal(row.length, 5);
        scene.prepare(row[1]);
        scene.handler().handle(scene.notice);
        assert.equal(scene.events.join(';'), row[2], row[1]);
        assert.equal(historyHex(scene.state.history), row[3], row[1]);
        assert.equal(scene.state.historyCounter, Number(row[4]), row[1]);
    }
});

test('통지: 잘못된 통지는 예측 삭제나 이력 갱신 전에 거부하고 필수 경계 누락·다른 판본을 거부한다', () => {
    const scene = new Scene();
    scene.prepare(rows[0]?.[1] ?? '100,0,1,1101266944,1101660160,7,6,3,0,3,305419896,1074270772,1,3,0,0,165,100,8,16384,70,71,0,2097152,0,1095237632,1096548352,1100480513,1101529087,3214934016,3208642560,0,1,0,2271560481,144');
    const handler = scene.handler();
    const history = historyHex(scene.state.history);
    // 누락된 타입·빈 목록·상한 초과·0·풀 밖 번호를 각각 검사한다.
    for (let kind = 0; kind < 5; kind += 1) {
        const invalid = { ...scene.notice };
        if (kind === 0) invalid.type = 255;
        else if (kind === 1) invalid.sids = [];
        else if (kind === 2) invalid.sids = new Array(20).fill(50);
        else invalid.sids = [50, kind === 3 ? 0 : CAPACITY];
        assert.ok(throws(() => handler.handle(invalid)), `kind ${kind}`);
        assert.deepEqual(scene.events, []);
        assert.equal(historyHex(scene.state.history), history);
        assert.equal(scene.state.historyCounter, scene.values[34]);
    }
    for (const name of ['clear', 'place', 'moveBuilder', 'findPriest', 'immobile', 'approach', 'tryMove', 'refund', 'destroy', 'start', 'refresh', 'containedType', 'removeContained']) {
        assert.ok(throws(() => scene.handler({ ...scene.hooks(), [name]: undefined })), name);
    }
    assert.ok(throws(() => new RawConstructionNotice(scene.pool, scene.types.slice(0, 171), scene.state, scene.hooks())));
    assert.ok(throws(() => new RawConstructionNotice({ edition: 'cd1072' }, scene.types, scene.state, scene.hooks())));
    assert.equal(handler.pool, scene.pool);
    // 서버 확정 어댑터는 같은 풀의 처리기만 받는다.
    assert.ok(throws(() => makeConstructionNoticeConfirmHooks(new SidPool(CAPACITY, { server: false }), handler, {})));
    const confirm = makeConstructionNoticeConfirmHooks(scene.pool, handler, { keep: 1 });
    assert.equal(confirm.keep, 1);
    confirm.handle(scene.notice);
    assert.ok(scene.events.length > 0);
});

test('통지: 이력은 배치 전에 갱신하고 배치 뒤 로컬 플레이어 변화는 Builder 분기에서 다시 읽는다', () => {
    const scene = new Scene();
    scene.prepare('100,0,1,1101266944,1101660160,7,6,3,0,3,305419896,1074270772,1,3,0,0,165,100,8,16384,70,71,0,2097152,0,1095237632,1096548352,1100480513,1101529087,3214934016,3208642560,0,1,0,2271560481,144');
    scene.types[scene.notice.type].flags1 = 0x8010;
    scene.state.localPlayer = scene.notice.player;
    scene.state.moveLocalBuilder = false;
    const hooks = scene.hooks();
    const place = hooks.place;
    // 배치 경계에서 로컬 플레이어가 바뀌는 상황을 공급한다.
    hooks.place = (request) => { place(request); scene.state.localPlayer = 5; };
    scene.handler(hooks).handle(scene.notice);
    assert.ok(scene.events.join(';').includes(';B:3:50:305419896:1074270772;W'));
    assert.equal(floatToBits(scene.state.history[0].x), floatToBits(scene.notice.x));
    assert.equal(scene.state.historyCounter, 0);
});

test('통지: 공유 이력 장부는 배치 전에 갱신하고 독립 값형 사본은 변경하지 않는다', () => {
    const scene = new Scene();
    scene.prepare('100,0,1,1101266944,1101660160,7,6,3,0,3,305419896,1074270772,1,3,0,0,165,100,8,16384,70,71,0,2097152,0,1095237632,1096548352,1100480513,1101529087,3214934016,3208642560,0,1,0,2271560481,144');
    scene.types[scene.notice.type].group = 2;
    scene.notice.flags |= 1;
    scene.state.localPlayer = scene.notice.player;
    const shared = createConstructionHistory();
    shared.points[0] = { x: 70, y: 80 };
    shared.index = 4;
    scene.state.sharedHistory = shared;
    const fallback = historyHex(scene.state.history);
    const fallbackCounter = scene.state.historyCounter;
    const hooks = scene.hooks();
    const place = hooks.place;
    let observed = false;
    hooks.place = (request) => {
        assert.equal(shared.index, 0);
        assert.equal(floatToBits(shared.points[0].x), floatToBits(scene.notice.x));
        assert.equal(floatToBits(shared.points[0].y), floatToBits(scene.notice.y));
        assert.deepEqual(shared.points[1], { x: 70, y: 80 });
        observed = true;
        place(request);
    };
    scene.handler(hooks).handle(scene.notice);
    assert.ok(observed);
    assert.equal(historyHex(scene.state.history), fallback);
    assert.equal(scene.state.historyCounter, fallbackCounter);
    assert.equal(scene.state.historyPoints(), shared.points);
    assert.equal(scene.state.historyCounterNow(), 0);
});

test('통지: 배치 효과가 바꾼 권한(공유 boss)을 자동 건설 시작 관문에서 다시 읽는다', () => {
    const scene = new Scene();
    scene.prepare('100,0,1,1101266944,1101660160,7,6,3,0,3,305419896,1074270772,1,3,0,0,165,100,8,16384,70,71,0,2097152,0,1095237632,1096548352,1100480513,1101529087,3214934016,3208642560,0,1,0,2271560481,144');
    const type = scene.types[scene.notice.type];
    Object.assign(type, { flags1: 0x10, flags2: 0, group: 2 });
    scene.values[17] = scene.notice.type;
    scene.notice.flags = 0;
    scene.notice.sids = [50];
    let boss = true;
    let nextBoss = false;
    scene.state.sharedAuthority = () => boss;
    const hooks = scene.hooks();
    const place = hooks.place;
    let started = 0;
    hooks.place = (value) => { place(value); boss = nextBoss; };
    hooks.start = (sid, flags) => { assert.equal(sid, 50); assert.equal(flags, 0); started += 1; };
    const notice = scene.handler(hooks);
    notice.handle(scene.notice);
    assert.equal(started, 0);
    assert.equal(boss, false);
    nextBoss = true;
    scene.state.authority = false;
    notice.handle(scene.notice);
    assert.equal(started, 1);
    assert.equal(boss, true);
});

// 값형 통지를 실제 wire 패킷으로 직렬화하고 SID 읽기 함수로 배치 요청의 목록을 복원해 typed 경로와 같은 효과 순서인지 본다. (2026-11-08 WB04-E134)
function wireEvents(scene) {
    const hooks = scene.hooks();
    const place = hooks.place;
    hooks.actualPlace = (request, count, reader) => {
        // 실제 배치는 선언 개수만큼 현재 wire 칸을 읽는다.
        const sids = Array.from({ length: count }, (_, index) => reader(index));
        place({ ...request, sids });
    };
    const packet = new Uint8Array(512);
    writeConstructionPacket(scene.notice, createConstructionPacketHeader(), packet);
    scene.handler(hooks).actualHandle(packet);
    return scene.events.join(';');
}

test('통지: 실제 wire 경로(actualHandle)가 같은 입력의 typed 경로와 같은 효과 순서·이력을 만든다', { skip: rows.length === 0 }, () => {
    const scene = new Scene();
    const typed = new Scene();
    let compared = 0;
    // 관측 행 전부를 바이트 경로로 재생한다(SID 목록은 wire 에서 선언 개수로 읽는다).
    for (const row of rows) {
        scene.prepare(row[1]);
        typed.prepare(row[1]);
        // 패킷은 8비트 필드만 담으므로 8비트를 넘는 입력이 있으면 비교 대상에서 뺀다(이 관측에는 없다).
        const n = scene.notice;
        if (n.type > 255 || n.argument > 255 || n.direction > 255 || n.player > 255 || n.flags > 255 || n.quality > 255 || n.sids.length === 0 || n.sids.length > 19) continue;
        // 실제 wire 경로의 전역은 같은 값을 공유 getter 로 읽는다.
        scene.state.sharedLocalPlayer = () => scene.state.localPlayer;
        scene.state.sharedDaisType = () => scene.state.daisType;
        scene.state.sharedPriestType = () => scene.state.priestType;
        typed.handler().handle(typed.notice);
        assert.equal(wireEvents(scene), typed.events.join(';'), row[1]);
        assert.equal(historyHex(scene.state.history), historyHex(typed.state.history), row[1]);
        assert.equal(scene.state.historyCounter, typed.state.historyCounter, row[1]);
        compared += 1;
    }
    assert.equal(compared, ROWS);
});

test('통지: 실제 wire 경로는 진입 때 고정한 TYPE 레코드를 쓰고 소비 위치마다 현재 패킷을 다시 읽는다', () => {
    const scene = new Scene();
    scene.prepare('100,16,1,1101266944,1101660160,7,6,3,0,3,305419896,1074270772,1,3,0,0,165,100,8,16384,70,71,0,2097152,0,1095237632,1096548352,1100480513,1101529087,3214934016,3208642560,0,1,0,2271560481,144');
    const packet = new Uint8Array(512);
    writeConstructionPacket(scene.notice, createConstructionPacketHeader(), packet);
    const hooks = scene.hooks();
    const place = hooks.place;
    hooks.actualPlace = (request, count, reader) => place({ ...request, sids: Array.from({ length: count }, (_, index) => reader(index)) });
    // 정리(clear)가 패킷의 TYPE 을 바꿔도 진입 때 선택한 같은 레코드를 읽고, 이후 소비 위치(권한 뒤 첫 SID)는 현재 바이트를 읽는다.
    const clear = hooks.clear;
    hooks.clear = (request) => { clear(request); packet[4] = 101; packet[27] = 51; };
    scene.state.authority = true;
    // 배치 뒤 첫 객체가 사제 이동 대상이 아니게 해 자동 건설 시작 분기로 가게 한다.
    scene.types[100].flags2 = 0;
    scene.handler(hooks).actualHandle(packet);
    const events = scene.events.join(';');
    assert.ok(events.startsWith('Q:100:'), events);
    // 배치 요청의 type 은 clear 뒤 현재 패킷 바이트(101)이고, 자동 건설 시작은 현재 첫 SID(51)로 간다.
    assert.ok(events.includes(';P:101:'), events);
    assert.ok(events.includes(';S:51:0;W:'), events);
    // actualPlace 가 없으면 효과 전에 거부한다.
    const bare = scene.hooks();
    assert.ok(throws(() => scene.handler(bare).actualHandle(packet)));
    // 패킷 TYPE 이 타입 표 밖이면 효과 전에 거부한다.
    packet[4] = 255;
    assert.ok(throws(() => scene.handler(hooks).actualHandle(packet)));
});

test('통지: 정리·배치·환불·이동 불가 어댑터는 같은 풀의 실제 모듈만 받는다', () => {
    const scene = new Scene();
    scene.prepare('100,0,1,1101266944,1101660160,7,6,3,0,3,305419896,1074270772,1,3,0,0,165,100,8,16384,70,71,0,2097152,0,1095237632,1096548352,1100480513,1101529087,3214934016,3208642560,0,1,0,2271560481,144');
    const log = [];
    const clear = { pool: scene.pool, clear: (request) => log.push(`clear:${request.type}`) };
    const place = {
        pool: scene.pool,
        place: (request) => log.push(`place:${request.type}`),
        actualPlace: (request, count) => log.push(`actual:${request.type}:${count}`),
        refund: (type, player) => log.push(`refund:${type}:${player}`),
    };
    const priest = { pool: scene.pool, immobile: (sid) => { log.push(`immobile:${sid}`); return false; } };
    const base = scene.hooks();
    const other = new SidPool(CAPACITY, { server: false });
    assert.ok(throws(() => makeConstructionNoticeHooks(other, clear, place, base)));
    assert.ok(throws(() => makeConstructionNoticeHooks(scene.pool, { ...clear, pool: other }, place, base)));
    assert.ok(throws(() => makeConstructionNoticeHooks(scene.pool, clear, { ...place, pool: other }, base)));
    assert.ok(throws(() => makeConstructionNoticeHooks(scene.pool, clear, place, base, { ...priest, pool: other })));
    const connected = makeConstructionNoticeHooks(scene.pool, clear, place, base, priest);
    connected.clear({ type: 1 });
    connected.place({ type: 2 });
    connected.actualPlace({ type: 3 }, 4, () => 0);
    connected.refund(5, 6);
    assert.equal(connected.immobile(7), false);
    assert.deepEqual(log, ['clear:1', 'place:2', 'actual:3:4', 'refund:5:6', 'immobile:7']);
    // priest 가 없으면 이동 불가 조회는 기존 경계를 유지한다.
    const noPriest = makeConstructionNoticeHooks(scene.pool, clear, place, base);
    assert.equal(noPriest.immobile, base.immobile);
});
