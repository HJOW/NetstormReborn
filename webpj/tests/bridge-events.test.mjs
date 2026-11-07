// 다리 이벤트 처리기·끝 칸 변환·글자별 프레임 구간 표 raw-bridge-events.js(RawBridgeEvents)를 원본 10.78 관측 bridgeevent-x86.tsv와 cpppj 계약으로 대조한다.
// 행 종류: Frames(합성 프레임 코드 변형 3개)·Table(글자 A~P 첫 프레임/개수/글자 수 306행)·Handler(이벤트 처리기 1,207행 이상)·End(끝 칸 변환 직접 호출 726행). 원본(originals) 10.78 행만 재생한다
// (original1037·originalCD 행은 범위 밖). 이웃 탐색·표면 알림·생성·destroy·소유자·Pop 은 원본 실행기와 같은 사건 기록 훅이다.
// 실제 다리 타입의 프레임 60개는 bridgedecay-x86.tsv 의 "Frames 0" 행에서 읽는다. (2026-11-08 WB04-E43: RawBridgeEvents 이식과 함께 추가)
import assert from 'node:assert/strict';
import path from 'node:path';
import test from 'node:test';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { BRIDGE_EVENT_END, BRIDGE_EVENT_KEEP, BRIDGE_VTABLE, RawBridgeEvents, makeBridgeNeighborHooks, makeBridgeRegularHandler } from '../www/core/raw-bridge-events.js';
import { RawSquidNeighbors } from '../www/core/raw-squid-neighbors.js';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { REPO_ROOT } from './helpers.mjs';
import { adler32, bitsToFloat, floatToBits, get, put, readRows } from './raw-support.mjs';

// 분석 도구와 같은 합성 풀 크기·입력 번호·가상 표 기록값·다리/다른 타입 번호이다. 실제 게임 월드 번호 배치를 뜻하지 않는다. (2026-11-08 WB04-E43)
const CAPACITY = 24000;
const BRIDGE = 50;
const NEWBORN = 60;
const VTABLE = 0x11010000;
const BRIDGE_TYPE = 82;
const OTHER_TYPE = 83;
const OWNER_OFFSET = 34;
const FRAME_OFFSET = 36;
const FLAG_OFFSET = 40;

// fixture 파일 경로이다. (2026-11-08 WB04-E43)
const fixture = (name) => path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', name);

// hex 문자열을 바이트 배열로 만든다. (2026-11-08 WB04-E43)
const fromHex = (text) => Uint8Array.from(text.match(/../g) ?? [], (pair) => parseInt(pair, 16));

// 프레임 코드 바이트 배열(코드당 4바이트: 방향 글자·변형 글자·번호·플래그)을 코드 목록으로 만든다. (2026-11-08 WB04-E43)
function codesOf(bytes) {
    const codes = [];
    // 네 바이트가 한 코드이다.
    for (let index = 0; index + 3 < bytes.length; index += 4) codes.push({ side: bytes[index], variant: bytes[index + 1], number: bytes[index + 2], flags: bytes[index + 3] });
    return codes;
}

// 반환 float 를 분석 도구와 같은 칸으로 쓴다. NaN 은 정규화로 비트가 달라지므로 한 이름이다. (2026-11-08 WB04-E43)
const resultText = (value) => (Number.isNaN(value) ? 'nan' : String(floatToBits(value)));

// 분석 도구의 EventOracle 입력/대체 효과를 같은 계약으로 재현하는 장면이다: 풀 슬롯 5~70 을 확보해 합성 번호 50·60·70 을 준비한다. (2026-11-08 WB04-E43)
class Scene {
    constructor(frameBytes) {
        this.pool = new SidPool(CAPACITY, { server: false });
        // 클라이언트 영역의 앞 슬롯들을 할당한다.
        for (let id = 5; id <= 70; id += 1) assert.equal(this.pool.allocate(2), id);
        this.state = { authority: true, debugKeep: false, bridgeType: 0 };
        this.variants = new Map([...frameBytes].map(([number, bytes]) => [number, new FrameCodeTable(codesOf(bytes))]));
        this.variant = 0;
        this.neighbor = 0;
        this.events = [];
        this.bridge = new RawBridgeEvents(this.pool, this.state, this.hooks());
    }

    // 프레임 DWORD 를 부호 없는 수로 읽는다.
    rawFrame(sid) { return get(this.pool.slot(sid), FRAME_OFFSET, 4); }

    // 외부 효과를 분석 도구의 대체 함수와 같은 사건 문자열로 기록한다.
    hooks() {
        return {
            // 타입 번호는 다리/다른 타입 두 개만 합성했고 프레임 표는 같다.
            frames: (type) => {
                assert.ok(type === BRIDGE_TYPE || type === OTHER_TYPE);
                return this.variants.get(this.variant);
            },
            // 임시 프레임이 쓰인 채로 호출된다. 그 프레임 값을 기록하고 입력의 첫 이웃을 돌려준다.
            firstNeighbor: (sid) => {
                this.events.push(`F:${sid}:${this.rawFrame(sid)}:0`);
                return this.neighbor;
            },
            notifySurface: (sid) => this.events.push(`N:${sid}`),
            // 새 객체의 헤더(vtable·타입·상태 0)만 만든다. 실제 할당/postCreate 는 대체 경계이다.
            create: (type) => {
                this.events.push(`C:${type}:0`);
                const raw = this.pool.physical(NEWBORN);
                put(raw, 0, VTABLE, 4);
                raw[10] = type;
                raw[11] = 0;
                return NEWBORN;
            },
            destroy: (sid, flags) => this.events.push(`D:${sid}:${flags}`),
            setOwner: (sid, owner) => this.events.push(`O:${sid}:${owner}`),
            pop: (sid, x, y, flags) => this.events.push(`P:${sid}:${floatToBits(x)}:${floatToBits(y)}:${flags}`),
        };
    }

    // 행의 입력(권한·디버그·전역 타입·변형·프레임·위치·단어·부모·소유자·플래그·이웃)으로 슬롯과 전역을 만든다.
    prepare(row) {
        this.events = [];
        this.state.authority = Number(row[2]) !== 0;
        this.state.debugKeep = Number(row[3]) !== 0;
        this.state.bridgeType = Number(row[4]);
        this.variant = Number(row[5]);
        this.neighbor = Number(row[13]);
        // 두 슬롯을 0으로 지우고 다리 슬롯만 채운다. 새 슬롯은 생성 대체가 헤더를 쓴다.
        for (const sid of [BRIDGE, NEWBORN]) this.pool.physical(sid).fill(0);
        const raw = this.pool.physical(BRIDGE);
        put(raw, 0, VTABLE, 4);
        raw[10] = BRIDGE_TYPE;
        raw[11] = 0;
        put(raw, 8, Number(row[10]), 2);
        put(raw, 12, Number(row[9]), 2);
        put(raw, 14, Number(row[7]), 4);
        put(raw, 18, Number(row[8]), 4);
        raw[OWNER_OFFSET] = Number(row[11]);
        put(raw, FRAME_OFFSET, Number(row[6]), 4);
        raw[FLAG_OFFSET] = Number(row[12]);
    }
}

// 저장된 독립 기계어 출력을 한 번만 읽는다. (2026-11-08 WB04-E43)
const rows = readRows(fixture('bridgeevent-x86.tsv'));
const frameBytes = new Map(rows.filter((row) => row[0] === 'Frames').map((row) => [Number(row[1]), fromHex(row[2])]));

test('bridge-events: Frames 변형 3개와 원본 10.78 Handler 이벤트 처리기·End 끝 칸 변환 관측을 대조한다', () => {
    assert.equal(frameBytes.size, 3);
    const scene = new Scene(frameBytes);
    let handler = 0;
    let end = 0;
    // 한 줄이 한 입력이다.
    for (const row of rows) {
        if (row[0] === 'Frames' || row[0] === 'Table' || row[1] !== 'originals') continue;
        assert.equal(row.length, 22);
        scene.prepare(row);
        let result = '-';
        if (row[0] === 'Handler') {
            result = resultText(scene.bridge.handle(BRIDGE, Number(row[14]), 0, bitsToFloat(row[15])));
            handler += 1;
        } else {
            scene.bridge.convertEnd(BRIDGE, bitsToFloat(row[16]), bitsToFloat(row[17]));
            end += 1;
        }
        const events = scene.events.length ? scene.events.join(';') : '-';
        const label = `${row[0]} 이벤트 ${row[14]} 프레임 ${row[6]} 위치 ${row[7]}/${row[8]}`;
        assert.equal(result, row[18], `${label} 반환`);
        assert.equal(events, row[19], `${label} 사건`);
        assert.equal(adler32(scene.pool.slot(BRIDGE)), Number(row[20]), `${label} 다리 슬롯`);
        assert.equal(adler32(scene.pool.slot(NEWBORN)), Number(row[21]), `${label} 새 슬롯`);
    }
    assert.equal(handler, 1207);
    assert.equal(end, 726);
});

test('bridge-events: 글자별 첫 프레임·개수·글자 수 표가 원본 306행과 일치한다', () => {
    let count = 0;
    // 한 줄이 한 프레임 코드 배열이다.
    for (const row of rows) {
        if (row[0] !== 'Table') continue;
        assert.equal(row.length, 6);
        const table = new FrameCodeTable(codesOf(row[2] === '-' ? new Uint8Array() : fromHex(row[2])));
        const first = row[3].split(',').map(Number);
        const counts = row[4].split(',').map(Number);
        assert.equal(first.length, 16);
        assert.equal(counts.length, 16);
        let kinds = 0;
        // 글자 'A'~'P' 16개를 하나씩 비교한다.
        for (let letter = 0; letter < 16; letter += 1) {
            const run = table.run(0x41 + letter);
            assert.equal(run.first, first[letter], `Table ${row[1]} 글자 ${letter} 첫 프레임`);
            assert.equal(run.count, counts[letter], `Table ${row[1]} 글자 ${letter} 개수`);
            if (run.count > 0) kinds += 1;
        }
        assert.equal(kinds, Number(row[5]));
        count += 1;
    }
    assert.equal(count, 306);
    assert.throws(() => new FrameCodeTable([]).run('Q'), RangeError);
    assert.throws(() => new FrameCodeTable([]).run('@'), RangeError);
});

test('bridge-events: 실제 다리 타입의 끝 칸 변환이 고르는 프레임(L·M·N·O 의 첫 프레임은 보통, 단단한 J·K 는 번호 20 뿐)', () => {
    const real = readRows(fixture('bridgedecay-x86.tsv')).find((row) => row.length === 3 && row[0] === 'Frames' && row[1] === '0');
    const codes = codesOf(fromHex(real[2]));
    const table = new FrameCodeTable(codes);
    assert.equal(codes.length, 60);
    assert.deepEqual(table.run('J'), { first: 35, count: 6 });
    assert.deepEqual(table.run('K'), { first: 41, count: 6 });
    // 끝 글자 네 개의 첫 프레임과 개수를 확인한다.
    for (const [letter, first] of [['L', 47], ['M', 50], ['N', 53], ['O', 56]]) {
        const run = table.run(letter);
        assert.deepEqual(run, { first, count: 3 });
        assert.equal(codes[run.first].number, 1);
        assert.equal(codes[run.first].flags, 0);
    }
    let hard = 0;
    // J·K 프레임 가운데 0x40 이 켜진 것은 번호 20 뿐이다.
    for (const code of codes) {
        if (code.side !== 0x4a && code.side !== 0x4b) continue;
        assert.equal((code.flags & 0x40) !== 0, code.number === 20);
        hard += (code.flags & 0x40) !== 0 ? 1 : 0;
    }
    assert.equal(hard, 2);
});

test('bridge-events: 훅 누락·권한/디버그 정지·form 타입·프레임 범위·끝 글자 부재는 변경 전에 거부하거나 자연 반환한다', () => {
    const scene = new Scene(frameBytes);
    assert.throws(() => new RawBridgeEvents(scene.pool, scene.state, { ...scene.hooks(), pop: undefined }), TypeError);
    assert.throws(() => new RawBridgeEvents(scene.pool, scene.state, { ...scene.hooks(), frames: undefined }), TypeError);
    const row = ['Handler', 'originals', '1', '0', '82', '0', '0', String(floatToBits(20.5)), String(floatToBits(21.5)), '0', '0', '1', '0', '70'];
    // 권한이 없으면 두 이벤트 모두 0(종료)이고 있으면 -1(유지)이다. 처리하지 않는 이벤트는 payload 그대로이다.
    scene.prepare(row);
    scene.state.authority = false;
    assert.equal(scene.bridge.handle(BRIDGE, 0x2691, 0, 1.5), BRIDGE_EVENT_END);
    assert.equal(scene.bridge.handle(BRIDGE, 0x2692, 0, 1.5), BRIDGE_EVENT_END);
    assert.equal(scene.bridge.handle(BRIDGE, 0x1234, 0, 1.5), 1.5);
    assert.deepEqual(scene.events, []);
    scene.state.authority = true;
    assert.equal(scene.bridge.handle(BRIDGE, 0x2691, 0, 1.5), BRIDGE_EVENT_KEEP);
    assert.deepEqual(scene.events, ['D:50:0']);
    // 디버그 유지가 켜지면 끝 칸 변환은 아무것도 하지 않는다.
    scene.prepare(row);
    scene.state.debugKeep = true;
    scene.bridge.convertEnd(BRIDGE, 20, 21);
    assert.deepEqual(scene.events, []);
    // form 타입은 거부한다.
    scene.prepare(row);
    scene.pool.physical(BRIDGE)[10] = 0x45;
    assert.throws(() => scene.bridge.convertEnd(BRIDGE, 20, 21), /form/);
    // 현재 프레임이 표 밖이면 거부한다.
    scene.prepare(row);
    put(scene.pool.physical(BRIDGE), FRAME_OFFSET, 9999, 4);
    assert.throws(() => scene.bridge.convertEnd(BRIDGE, 20, 21), RangeError);
    // 이웃 탐색이 던져도 임시로 쓴 프레임은 되돌려 놓는다.
    scene.prepare(row);
    const throwing = new RawBridgeEvents(scene.pool, scene.state, { ...scene.hooks(), firstNeighbor: () => { throw new Error('이웃 조회 중단'); } });
    // J/K 방향 글자 프레임으로 두어 이웃 조회까지 도달시킨다.
    const letterJ = frameBytes.get(0);
    const firstJ = codesOf(letterJ).findIndex((code) => code.side === 0x4a);
    put(scene.pool.physical(BRIDGE), FRAME_OFFSET, firstJ, 4);
    assert.throws(() => throwing.convertEnd(BRIDGE, 20, 21), /이웃 조회 중단/);
    assert.equal(get(scene.pool.slot(BRIDGE), FRAME_OFFSET, 4), firstJ);
});

test('bridge-events: Regular 처리기는 가상 표로 다리 처리기/fallback/payload 를 고른다', () => {
    const scene = new Scene(frameBytes);
    const plain = 61;
    put(scene.pool.physical(plain), 0, 0x501dd0, 4);
    put(scene.pool.physical(BRIDGE), 0, BRIDGE_VTABLE, 4);
    scene.prepare(['Handler', 'originals', '1', '0', '82', '0', '0', String(floatToBits(20.5)), String(floatToBits(21.5)), '0', '0', '1', '0', '70']);
    put(scene.pool.physical(BRIDGE), 0, BRIDGE_VTABLE, 4);
    const handler = makeBridgeRegularHandler(scene.pool, scene.bridge);
    // 다리 클래스는 다리 처리기를, 아니면 base 처럼 payload 를 돌려준다.
    assert.equal(handler(BRIDGE, 0x2691, 0, 0.5), BRIDGE_EVENT_KEEP);
    assert.deepEqual(scene.events, ['D:50:0']);
    assert.equal(handler(plain, 0x2692, 0, 0.5), 0.5);
    const calls = [];
    const withFallback = makeBridgeRegularHandler(scene.pool, scene.bridge, (parent, event, count, payload) => { calls.push([parent, event, count, payload]); return 7; });
    assert.equal(withFallback(plain, 0x2692, 3, 0.25), 7);
    assert.deepEqual(calls, [[plain, 0x2692, 3, 0.25]]);
    assert.equal(withFallback(BRIDGE, 0x2691, 0, 0.5), BRIDGE_EVENT_KEEP);
    assert.equal(calls.length, 1);
});

test('bridge-events: 이웃 훅은 같은 타입 표의 프레임 조회와 실제 flag 0 첫 이웃을 연결한다', () => {
    const pool = new SidPool(CAPACITY, { server: false });
    for (let id = 5; id <= 70; id += 1) pool.allocate(2);
    const hash = new SquidHash();
    const spots = new Uint8Array(65536);
    const types = Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, footX: 1, footY: 1 }));
    const frames = Array.from({ length: 188 }, () => []);
    const codes = codesOf(frameBytes.get(0));
    Object.assign(types[BRIDGE_TYPE], { flags1: 0x802, flags2: 4 });
    Object.assign(types[OTHER_TYPE], { flags1: 0x28000803, flags2: 2 });
    frames[BRIDGE_TYPE] = codes;
    frames[OTHER_TYPE] = codes;
    // 다리(50)는 (20,21), 표면 칸(70)은 서쪽 (19,21)에 둔다. 프레임 번호는 둘 다 3번 코드이다.
    const place = (sid, type, x, y, frame) => {
        const raw = pool.physical(sid);
        raw.fill(0);
        put(raw, 0, VTABLE, 4);
        raw[10] = type;
        put(raw, 14, floatToBits(x), 4);
        put(raw, 18, floatToBits(y), 4);
        put(raw, FRAME_OFFSET, frame, 4);
        put(raw, 4, hash.bucket(0, x, y), 2);
        hash.setBucket(0, x, y, sid);
    };
    place(BRIDGE, BRIDGE_TYPE, 20, 21, 2);
    place(70, OTHER_TYPE, 19, 21, 2);
    const neighbors = new RawSquidNeighbors(pool, hash, spots, types, frames);
    const effects = { notifySurface() {}, create() {}, destroy() {}, setOwner() {}, pop() {} };
    const hooks = makeBridgeNeighborHooks(neighbors, effects);
    assert.equal(hooks.firstNeighbor(BRIDGE), neighbors.first(BRIDGE));
    assert.equal(hooks.frames(BRIDGE_TYPE).codes.length, codes.length);
    assert.equal(hooks.frames(BRIDGE_TYPE).run('J').first, new FrameCodeTable(codes).run('J').first);
    // 모든 훅이 연결된 처리기를 만들 수 있다.
    assert.doesNotThrow(() => new RawBridgeEvents(pool, { authority: true, debugKeep: false, bridgeType: BRIDGE_TYPE }, hooks));
});
