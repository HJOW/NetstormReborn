// 운반체 이동 raw-carrier-move.js(RawCarrierMove·writeCarrierMoveCommand)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: carrier-move-x86.tsv 982행(두 정밀도 열이 같음) — 입력 26 DWORD(연산 0 요청/1 수신·TYPE flags2·상태·소유자·보낸 소유자·fort·선택·연결·경로·후보·귀환·간헐천/제단 genus·허용·희생·정지/차단 사건·변이 20종·목적지·경로 목적지·헤더)
//    → 외부 효과 순서(S 선택/C 해제/I 진단/L 연결/M 이동 종류/P 경로/E 사건/Q 후보/R 귀환/D 접근/G genus/Y 간헐천 경로/A 허용/F 희생/B 시작/T 송신), 현재 전역(fort·TYPE flags2·요청 헤더 세 값), 송신 패킷 hex,
//    두 부모 슬롯 raw, 경로 이미지(앞 0x186ec바이트)의 Adler-32.
//  - 계약: 짧은 수신 패킷·미연결 하위의 지연 거부, 공유 작성기의 헤더/float 비트/꼬리 경계/짧은 버퍼 쓰기 전 거부.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E99: 운반체 이동 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, bitsToFloat, floatToBits, adler32 } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { RawPathRuntime } from '../www/core/raw-path-runtime.js';
import { CarrierMoveState, RawCarrierMove, writeCarrierMoveCommand } from '../www/core/raw-carrier-move.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E99)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'carrier-move-x86.tsv'));

// 예외가 나는지 본다. (2026-11-08 WB04-E99)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

const hex = (bytes) => Buffer.from(bytes).toString('hex');
const bits = (value) => floatToBits(value);

// 큰 외부 하위의 명시 입력/변경만 재생하고 제품이 분기·패킷·현재 읽기를 수행하는 장면이다. (2026-11-08 WB04-E99)
function makeScene() {
    const scene = {
        pool: new SidPool(24000, { server: true }),
        types: Array.from({ length: 188 }, () => ({ flags2: 0 })),
        fort: false,
        state: new CarrierMoveState(),
        path: new RawPathRuntime(),
        row: [],
        received: new Uint8Array(18),
        trace: [],
        packet: '-'
    };
    // 원본 실제 인자 순서를 DWORD 로 관측한다.
    const event = (name, args = []) => scene.trace.push(args.reduce((text, value) => `${text}:${value >>> 0}`, name));
    // 원본 관측기에 공급한 외부 변경만 재생한다.
    const mutate = (where) => {
        const row = scene.row;
        const m = row[17];
        const raw50 = scene.pool.physical(50);
        if (m === 1 && where === 'select') scene.types[133].flags2 = 0;
        if (m === 2 && where === 'assert') scene.types[133].flags2 = 0x20000;
        if (m === 3 && where === 'connected') raw50[10] = 134;
        if (m === 4 && where === 'movement') { scene.state.request.protocol = 0xff; scene.state.request.length = 3; }
        if (m === 5 && where === 'path') { put(scene.path.bytes, 0x186dc, row[18]); put(scene.path.bytes, 0x186e0, row[19]); }
        if (m === 6 && where === 'select') put(raw50, 14, bits(24.5));
        if (m === 7 && where === 'select') scene.types[133].flags2 = 0x20000;
        if (m === 8 && where === 'select') scene.state.request.message = 0x55;
        if (m === 9 && where === 'assert') scene.types[133].flags2 = 0x10000;
        if (m === 10 && where === 'connected') scene.types[133].flags2 = 0;
        if (m === 11 && where === 'stop') raw50[34] = 2;
        if (m === 12 && where === 'stop') scene.fort = true;
        if (m === 13 && where === 'move') { put(scene.received, 4, 51, 2); put(scene.received, 6, bits(24.5)); }
        if (m === 14 && where === 'candidate') { put(scene.received, 4, 51, 2); put(scene.received, 10, bits(25.5)); }
        if (m === 15 && where === 'return') { put(scene.received, 4, 51, 2); put(scene.received, 6, bits(26.5)); }
        if (m === 16 && where === 'genus') { put(scene.received, 4, 51, 2); put(scene.received, 10, bits(27.5)); }
        if (m === 17 && where === 'allowed') put(scene.received, 14, 0);
        if (m === 18 && where === 'allowed') put(scene.received, 4, 51, 2);
        if (m === 19 && where === 'candidate') scene.types[133].flags2 = 0x10000;
        if (m === 20 && where === 'return') put(scene.received, 14, 0);
    };
    // 실제 원문 입력만 두 슬롯/Path/헤더에 설치한다.
    scene.prepare = (values) => {
        const row = values;
        scene.row = row;
        assert.equal(row.length, 26);
        scene.trace = [];
        scene.packet = '-';
        scene.fort = row[5] !== 0;
        scene.state.request = { message: row[23], protocol: row[22], length: row[24] };
        // 콜백 대체 부모도 별개 물리 슬롯으로 유지한다.
        for (const sid of [50, 51]) {
            const raw = scene.pool.physical(sid);
            raw.fill(0xa5);
            put(raw, 0, 0x12200000);
            raw[10] = 133;
            raw[11] = row[2];
            raw[34] = row[3];
            put(raw, 14, bits(20.5));
            put(raw, 18, bits(21.5));
        }
        // 경로 접두의 미사용 바이트 보존도 원본 Adler 와 대조한다.
        scene.path.bytes.fill(0xa5);
        put(scene.path.bytes, 0x186dc, row[20]);
        put(scene.path.bytes, 0x186e0, row[21]);
        // 사용 중인 두 TYPE 이외에는 원문처럼 빈 입력이다.
        for (const type of scene.types) type.flags2 = 0;
        scene.types[133].flags2 = row[1];
        scene.types[134].flags2 = 0x20000;
        scene.received = new Uint8Array(18);
        scene.received[0] = row[22] & 0x7f;
        scene.received[1] = row[23];
        put(scene.received, 2, row[24], 2);
        put(scene.received, 4, 50, 2);
        put(scene.received, 6, row[18]);
        put(scene.received, 10, row[19]);
        put(scene.received, 14, row[14]);
    };
    // 큰 하위만 원래 인자로 기록하며 계산 결과는 제품에 맡긴다.
    scene.hooks = {
        selected: () => { event('S'); return scene.row[6]; },
        select: (a, b, c) => { event('C', [a, b, c]); mutate('select'); },
        assertion: (line) => { event('I', [line]); mutate('assert'); },
        connected: (source, destination) => { event('L', [bits(source.x), bits(source.y), bits(destination.x), bits(destination.y)]); mutate('connected'); return scene.row[7] !== 0; },
        movement: (type) => { event('M', [type, 0, 0]); mutate('movement'); return 40; },
        path: (sid, type) => { event('P', [sid, type]); mutate('path'); return scene.row[8] ? scene.path : null; },
        send: (target, bytes) => { scene.packet = hex(bytes); scene.trace.push(`T:${target}:${scene.packet}`); },
        regular: (sid, eventId) => { event('E', [sid, eventId]); mutate(eventId === 0x5105 ? 'stop' : 'move'); return scene.row[eventId === 0x5105 ? 15 : 16] !== 0; },
        candidate: (sid, point) => { event('Q', [sid, bits(point.x), bits(point.y)]); mutate('candidate'); return { sid: scene.row[9], nugget: true }; },
        returnPlace: (point) => { event('R', [bits(point.x), bits(point.y), 0]); mutate('return'); return scene.row[10]; },
        target: (parent, target) => event('D', [parent, target, 0, 0]),
        genusAt: (point, genus) => { event('G', [bits(point.x), bits(point.y), genus]); mutate('genus'); return scene.row[genus === 0x10000000 ? 11 : 12]; },
        geyser: (parent, target) => { event('Y', [parent, target]); return scene.row[25] ? scene.path : null; },
        altarAllowed: (altar, parent) => { event('A', [altar, parent]); mutate('allowed'); return scene.row[13] !== 0; },
        sacrifice: (parent, altar) => event('F', [parent, altar]),
        start: (request) => {
            event('B', [request.parent, bits(request.destination.x), bits(request.destination.y), request.replaceStart, request.helper, request.tailWord, request.tailByte, request.reserved]);
            return scene.row[25] ? scene.path : null;
        }
    };
    // 효과 뒤 현재 전역을 원문과 같은 순서로 읽는다.
    scene.globals = () => `${scene.fort ? 1 : 0},${scene.types[133].flags2 >>> 0},${scene.state.request.protocol},${scene.state.request.message},${scene.state.request.length}`;
    return scene;
}

test('CarrierMove: 원본 10.78 982입력의 요청/수신 효과 순서·전역·송신 패킷·두 슬롯 raw·경로 이미지 Adler 가 두 정밀도 열과 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = makeScene();
    let count = 0;
    // 원본 결과는 읽기만 하며 제품의 실제 효과를 비교한다.
    for (const fields of rows) {
        assert.equal(fields.length, 11);
        scene.prepare(fields[0].split(',').map((value) => Number(BigInt.asUintN(32, BigInt(value)))));
        const move = new RawCarrierMove(scene.pool, scene.types, () => scene.fort, scene.state, scene.hooks);
        const row = scene.row;
        if (!row[0]) move.request(50, { x: bitsToFloat(row[18]), y: bitsToFloat(row[19]) }, row[14]);
        else move.receive(row[4], scene.received);
        const actual = [
            scene.trace.length === 0 ? '-' : scene.trace.join(';'),
            scene.globals(),
            scene.packet,
            `${hex(scene.pool.slot(50))},${hex(scene.pool.slot(51))}`,
            String(adler32(scene.path.bytes.subarray(0, 0x186ec)))
        ];
        // 각 관측 전체를 비교하고 두 정밀도 열이 같은지도 확인한다.
        for (let index = 0; index < actual.length; index += 1) {
            assert.equal(actual[index], fields[index + 1], `행 ${count} 열 ${index}: 입력 ${fields[0]}`);
            assert.equal(fields[index + 1], fields[index + 6]);
        }
        count += 1;
    }
    assert.equal(count, 982);
});

test('CarrierMove: 짧은 수신 패킷과 미연결 하위는 월드 변경 전에 거부한다', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = makeScene();
    scene.prepare('1,65536,1,1,1,0,0,1,0,0,0,0,0,0,0,0,0,0,1102315520,1102839808,0,0,1,64,18,1'.split(',').map(Number));
    const move = new RawCarrierMove(scene.pool, scene.types, () => scene.fort, scene.state, {});
    assert.equal(move.pool, scene.pool);
    move.receive(1, scene.received);
    assert.ok(throws(() => move.receive(1, scene.received.subarray(0, 17))));
    assert.ok(throws(() => move.request(50, { x: 0, y: 0 }, 0)));
    assert.ok(throws(() => new RawCarrierMove({ edition: 'cd1072' }, scene.types, () => false, scene.state, {})));
    assert.ok(throws(() => new RawCarrierMove(scene.pool, scene.types.slice(0, 171), () => false, scene.state, {})));
});

test('CarrierMove: 공유 작성기는 원본 헤더·float 비트·꼬리 경계·짧은 버퍼 쓰기 전 거부를 보존한다', () => {
    const state = new CarrierMoveState();
    state.request = { message: 0x40, protocol: 0xff, length: 2 };
    const bytes = new Uint8Array(23).fill(0xa5);
    assert.equal(writeCarrierMoveCommand(state, 17, { x: bitsToFloat(0x7fc01234), y: -0 }, 0xdeadbeef, bytes), 18);
    assert.equal(bytes[0], 0x7f);
    assert.equal(bytes[1], 0x40);
    assert.equal(get(bytes, 2, 2), 2);
    assert.equal(get(bytes, 4, 2), 17);
    assert.equal(get(bytes, 6), 0x7fc01234);
    assert.equal(get(bytes, 10), 0x80000000);
    assert.equal(get(bytes, 14), 0xdeadbeef);
    assert.ok(bytes.subarray(18).every((value) => value === 0xa5));
    const before = Buffer.from(bytes);
    assert.ok(throws(() => writeCarrierMoveCommand(state, 17, { x: 1, y: 2 }, 0, bytes.subarray(0, 17))));
    assert.deepEqual(Buffer.from(bytes), before);
});
