// 공통 후처리 처리기 squid-postpop.js(SquidPostPop 가상 분배·공통 장부·비용)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
// 원본(originals) 10.78 행만 재생한다(original1037·originalCD 행은 범위 밖). 재생하는 관측:
//  - postpop-x86.tsv: 비용·목록·통계 장부 + Pop/Unpop·재등록의 전체 슬롯·장부·Totalmade 세 표(표시 dirty 열은 SquidDisplay 단위에서 대조),
//  - bridgepostpop-x86.tsv: 다리 접두 80행(flags/extra/연결 요청 순서),
//  - rawgraph-x86.tsv·regiongraph-x86.tsv·graphremove-x86.tsv·graphrecovery-x86.tsv: Pop/PostPop/Unpop/Release와 RawGraph 영역·Add·삭제·재구성이 함께 도는 전체 슬롯·기록·스택·해시·spot·풀·장부.
// E40 의 raw-graph.test.mjs 가 남긴 Pop 연동 대조를 이 단위에서 마친다. (2026-11-08 WB04-E41: SquidPostPop 처리기 본체 이식과 함께 추가)
import assert from 'node:assert/strict';
import path from 'node:path';
import test from 'node:test';
import { GRAPH_RECOVERY } from '../www/core/graph.js';
import { RawGraph } from '../www/core/raw-graph.js';
import { RAW_POP_RESULT } from '../www/core/raw-squid-pop-parent.js';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidPop } from '../www/core/squid-pop.js';
import { SquidPostPop, bridgePrefix } from '../www/core/squid-postpop.js';
import { SquidPostPopState } from '../www/core/squid-postpop-state.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { REPO_ROOT } from './helpers.mjs';
import { adler32, bitsToFloat, put, readRows } from './raw-support.mjs';

// 표면 타입 flags1 비트, 공통 슬롯 vtable, 최소 SID 풀 크기(원본 client 영역 경계를 포함)이다. (2026-11-08 WB04-E41)
const SURFACE = 0x800;
const COMMON_VTABLE = 0x501dd0;
const BRIDGE_POOL = 24000;
const WORLD = 256;

// fixture 파일 경로이다. (2026-11-08 WB04-E41)
const fixture = (name) => path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', name);

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E41)
const hex = (bytes) => Array.from(bytes, (byte) => byte.toString(16).padStart(2, '0')).join('');

// hex 문자열을 바이트 배열로 만든다. (2026-11-08 WB04-E41)
const fromHex = (text) => Uint8Array.from(text.match(/../g) ?? [], (pair) => parseInt(pair, 16));

// 부호 있는 16비트로 해석한다. (2026-11-08 WB04-E41)
const int16 = (value) => (((value & 0xffff) << 16) >> 16);

// 6 BYTE 레코드 배열을 직렬화한다. (2026-11-08 WB04-E41)
function recordBytes(records) {
    const bytes = new Uint8Array(records.length * 6);
    // 각 행의 세 WORD 를 차례로 쓴다.
    records.forEach((record, index) => {
        put(bytes, index * 6, record.surfaces & 0xffff, 2);
        put(bytes, index * 6 + 2, record.inUse & 0xffff, 2);
        put(bytes, index * 6 + 4, record.reserved & 0xffff, 2);
    });
    return bytes;
}

// DWORD 배열을 리틀엔디언 바이트로 직렬화한다. (2026-11-08 WB04-E41)
function wordBytes(words) {
    const bytes = new Uint8Array(words.length * 4);
    // 활성 영역 밖 stale DWORD 도 포함한다.
    Array.from(words).forEach((value, index) => put(bytes, index * 4, value >>> 0, 4));
    return bytes;
}

// 6 BYTE 레코드 바이트를 레코드 배열로 해석한다. (2026-11-08 WB04-E41)
function recordsFromBytes(bytes) {
    const records = [];
    // 세 WORD 를 부호 있는 값으로 읽는다.
    for (let index = 0; index < bytes.length / 6; index += 1) {
        const word = (field) => int16(bytes[index * 6 + field * 2] | (bytes[index * 6 + field * 2 + 1] << 8));
        records.push({ surfaces: word(0), inUse: word(1), reserved: word(2) });
    }
    return records;
}

// fixture 의 "x,y,z;x,y,z" 또는 "-" 문자열을 정수 행으로 나눈다. (2026-11-08 WB04-E41)
const parseRows = (text) => (text === '-' ? [] : text.split(';').map((entry) => entry.split(/[,\s]+/).filter(Boolean).map(Number)));

// 188개 기본 타입(HP 91·깊이 7·발자국 1×1·비용 1.25)과 빈 프레임 표를 만든다. (2026-11-08 WB04-E41)
const makeTypes = () => Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, footX: 1, footY: 1, cost: 1.25, maxHitPoints: 91 }));
const makeFrames = () => Array.from({ length: 188 }, () => []);

// 예외가 났는지만 본다. (2026-11-08 WB04-E41)
function throws(operation) {
    try { operation(); } catch { return true; }
    return false;
}

// 슬롯을 raw 입력으로 채운다(원문 Node). n = [x,y,footX,footY,flags1,flags2,side,variant,frame,state,extra,graph,level]. (2026-11-08 WB04-E41)
function node(pool, sid, number, n, next = 0) {
    const bytes = pool.physical(sid);
    bytes.fill(0);
    put(bytes, 0, COMMON_VTABLE);
    put(bytes, 4, next, 2);
    put(bytes, 8, 23, 2);
    bytes[10] = number;
    bytes[11] = n[9];
    const view = new DataView(new ArrayBuffer(8));
    view.setFloat32(0, n[0], true);
    view.setFloat32(4, n[1], true);
    for (let index = 0; index < 4; index += 1) {
        bytes[14 + index] = view.getUint8(index);
        bytes[18 + index] = view.getUint8(4 + index);
    }
    bytes[30] = n[11];
    bytes[33] = n[12];
    bytes[34] = 1;
    put(bytes, 36, n[8], 4);
    bytes[40] = n[10];
}

// Totalmade 세 표에 넣은 오래된 DWORD 입력(원문 fixture 의 고정 시드)이다. (2026-11-08 WB04-E41)
function seedCounts(state, seed) {
    // 세 표를 서로 다른 값으로 채운다.
    for (let index = 0; index < 256; index += 1) {
        const value = (seed + Math.imul(index, 0x9e3779b1)) >>> 0;
        state.localCounts()[index] = value;
        state.localSecondaryCounts()[index] = (value + 17) >>> 0;
        state.globalCounts()[index] = (value + 34) >>> 0;
    }
}

// 장부 한 행(비용·알림·깊이·세 표·목록·슬롯)을 대조한다. 표시 dirty 열(start+9, start+10)은 SquidDisplay 단위에서 대조한다. (2026-11-08 WB04-E41)
function checkLedger(row, start, state, pool, sid, label) {
    assert.equal(state.totalCost, Number(row[start]), `${label} 비용`);
    assert.equal(state.productionDirty, Number(row[start + 1]), `${label} 알림`);
    assert.equal(state.depth, Number(row[start + 2]), `${label} 깊이`);
    [state.localCounts(), state.localSecondaryCounts(), state.globalCounts()].forEach((table, index) => {
        assert.equal(adler32(wordBytes(table)), Number(row[start + 3 + index]), `${label} 표${index}`);
    });
    const counts = row[start + 6].split(/[,;]/).map(Number);
    assert.equal(counts.length, 11);
    assert.equal(state.providers.count, counts[0], `${label} 공급원 수`);
    assert.equal(state.factories.count, counts[1], `${label} 공장 수`);
    const data = [...state.providers.entries, ...state.factories.entries];
    // 중립 owner 0 의 보존 메모리와 모든 소유자 목록을 포함한다.
    state.ownerFactories.forEach((list, index) => {
        assert.equal(list.count, counts[index + 2], `${label} owner${index} 공장 수`);
        data.push(...list.entries);
    });
    assert.equal(adler32(wordBytes(data)), Number(row[start + 7]), `${label} 목록`);
    assert.equal(hex(pool.slot(sid)), row[start + 8], `${label} 슬롯`);
}

test('postpop: 비용·목록·통계 장부와 Pop/Unpop·재등록을 원본 10.78 관측과 대조한다', () => {
    let begins = 0;
    let posts = 0;
    let pops = 0;
    let unpops = 0;
    let active = false;
    let pool;
    let hash;
    let post;
    let pop;
    let unpop;
    let state;
    let types;
    let spots;
    let sid = 0;
    let number = 74;
    // Begin/Type/State/Fill 은 명시적인 준비 입력이며 원본 월드 로딩을 실행한 수치가 아니다.
    for (const row of readRows(fixture('postpop-x86.tsv'))) {
        if (row[0] === 'Begin') {
            active = row[1] === 'originals';
            if (!active) continue;
            types = makeTypes();
            spots = new Uint8Array(65536);
            pool = new SidPool(32768, { server: false });
            hash = new SquidHash();
            sid = pool.allocate(2);
            assert.equal(sid, Number(row[3]));
            begins += 1;
            continue;
        }
        if (!active) continue;
        if (row[0] === 'Type') {
            number = Number(row[1]);
            Object.assign(types[number], { flags1: Number(row[2]) >>> 0, flags2: Number(row[3]) >>> 0, cost: bitsToFloat(row[4]) });
        } else if (row[0] === 'State') {
            state = new SquidPostPopState();
            const [providerCapacity, factoryCapacity, ownerCapacity, initial] = [1, 2, 3, 4].map((index) => Number(row[index]));
            state.providers.entries = Array.from({ length: providerCapacity }, (_, index) => (index === 0 ? sid : 0x60000000 + index));
            state.factories.entries = Array.from({ length: factoryCapacity }, (_, index) => (index === 0 ? sid : 0x60000000 + index));
            state.providers.count = Math.min(initial, providerCapacity);
            state.factories.count = Math.min(initial, factoryCapacity);
            // 소유자 0도 원본 메모리 보존 검사에 포함한다.
            state.ownerFactories.forEach((list, owner) => {
                list.entries = Array.from({ length: ownerCapacity }, (_, index) => 0x70000000 + owner * 100 + index);
                list.count = Math.min(initial, ownerCapacity);
            });
            state.totalCost = Number(row[5]) | 0;
            state.productionDirty = Number(row[6]) >>> 0;
            seedCounts(state, Number(row[7]) >>> 0);
            state.localOwner = Number(row[8]);
            state.graphsEnabled = Number(row[9]) !== 0;
            state.suppressed = Number(row[10]) !== 0;
            state.depth = Number(row[11]) >>> 0;
            post = new SquidPostPop(pool, types, state);
            pop = new SquidPop(pool, hash, spots, { postPop: post });
            unpop = new SquidUnpop(pool, hash, spots);
        } else if (row[0] === 'Fill') {
            hash.reset();
            const bytes = pool.physical(sid);
            bytes.fill(0);
            put(bytes, 0, COMMON_VTABLE);
            bytes[10] = number;
            bytes[11] = 4;
            put(bytes, 14, 0x41a00000);
            put(bytes, 18, 0x41a00000);
            bytes[34] = Number(row[1]);
            bytes[40] = Number(row[2]);
            bytes[30] = 17;
        } else if (row[0] === 'PostPop') {
            post.postPop(sid, Number(row[1]) >>> 0);
            checkLedger(row, 2, state, pool, sid, `PostPop ${row[1]}`);
            posts += 1;
        } else if (row[0] === 'Pop') {
            assert.equal(pop.pop(sid, types[number], 1, 1, bitsToFloat(row[2]), bitsToFloat(row[3]), Number(row[1]) >>> 0), RAW_POP_RESULT.registered);
            checkLedger(row, 4, state, pool, sid, `Pop ${row[1]}`);
            pops += 1;
        } else if (row[0] === 'Unpop') {
            unpop.unpop(sid, types[number], Number(row[1]) >>> 0);
            checkLedger(row, 2, state, pool, sid, `Unpop ${row[1]}`);
            unpops += 1;
        } else assert.fail(`알 수 없는 행 ${row[0]}`);
    }
    assert.equal(begins, 2);
    assert.equal(posts, 384);
    assert.equal(pops, 72);
    assert.equal(unpops, 36);
});

test('bridgepostpop: 다리 접두의 flags/extra/연결 요청 순서를 원본 10.78 80행과 대조한다', () => {
    let count = 0;
    // 각 행의 flags/extra 입력을 독립 슬롯에서 검사한다.
    for (const row of readRows(fixture('bridgepostpop-x86.tsv'))) {
        assert.equal(row.length, 5);
        if (row[0] !== 'originals') continue;
        const pool = new SidPool(BRIDGE_POOL, { server: false });
        // 원본 fixture 의 번호 50 을 client 영역에 확보한다.
        for (let id = 5; id <= 50; id += 1) assert.equal(pool.allocate(2), id);
        const raw = pool.physical(50);
        raw[40] = Number(row[2]);
        const expected = Uint8Array.from(raw);
        expected[40] = Number(row[3]);
        let calls = '';
        bridgePrefix(pool, 50, Number(row[1]) >>> 0, (target) => {
            calls += `L:${target}:0:0;`;
            // 최초 등록 비트는 연결 함수를 부르기 전에 이미 기록되어야 한다.
            assert.equal(pool.slot(target)[40], expected[40]);
        });
        const common = row[4].indexOf('B:');
        assert.ok(common >= 0);
        assert.equal(calls, row[4].slice(0, common));
        assert.deepEqual(pool.slot(50), expected);
        count += 1;
    }
    assert.equal(count, 80);
});

// 기존 Pop/PostPop/Unpop/Release 연동 fixture 한 파일을 재생한다. 열 14·15(표시 dirty)는 SquidDisplay 단위에서 대조한다. 반환은 비교한 연산 수이다. (2026-11-08 WB04-E41)
function replayPopGraphRows(name, { regionFixture, globalFixture = false, recovery = GRAPH_RECOVERY.reject, checkingEnabled = false, allowed = null }) {
    let active = false;
    let pool;
    let hash;
    let graph;
    let post;
    let pop;
    let unpop;
    let state;
    let types;
    let frames;
    let spots;
    let ids = [];
    let calls = 0;
    const diagnosticCounts = { reference: 0, parent: 0 };
    // 기대값은 독립 PE 출력이다. 다른 판본 구간은 다음 Begin 까지 건너뛴다.
    for (const row of readRows(fixture(name))) {
        if (row[0] === 'Begin') {
            active = row[1] === 'originals';
            if (!active) continue;
            types = makeTypes();
            frames = makeFrames();
            spots = new Uint8Array(65536);
            pool = new SidPool(32768, { server: false });
            hash = new SquidHash();
            const expected = row[3].split(',').map(Number);
            ids = expected.map(() => pool.allocate(2));
            assert.deepEqual(ids, expected);
            continue;
        }
        if (!active) continue;
        if (row[0] === 'Setup') {
            hash.reset();
            spots.fill(0);
            if (globalFixture) {
                // 이 fixture 는 free 슬롯도 입력한다. 다음 Setup 은 같은 풀/번호를 새로 준비한다.
                pool = new SidPool(32768, { server: false });
                for (const id of ids) assert.equal(pool.allocate(2), id);
            }
            const nodes = parseRows(row[1]);
            assert.equal(nodes.length, ids.length);
            // 지도는 발자국 전체가 아니라 실제 기준점 버킷/체인을 입력한다.
            nodes.forEach((n, index) => {
                types[74 + index] = { ...types[74 + index], flags1: n[4], flags2: n[5], footX: n[2], footY: n[3] };
                frames[74 + index] = [{ side: n[6], variant: n[7], number: 1, flags: 0 }, { side: 65, variant: 80, number: 2, flags: 0 }];
                const level = regionFixture ? n[12] : 0;
                const mapped = !(n[9] & 4) && (regionFixture || n[12] === 0);
                node(pool, ids[index], 74 + index, n, mapped && regionFixture ? hash.bucket(level, n[0], n[1]) : 0);
                if (mapped) hash.setBucket(level, n[0], n[1], ids[index]);
            });
            // 내부 spot 후보를 별도로 준비한다.
            for (const [x, y, value] of parseRows(row[2])) spots[y * WORLD + x] = value;
            state = new SquidPostPopState();
            state.totalCost = 53;
            state.depth = 5;
            state.localOwner = 1;
            state.graphsEnabled = true;
            seedCounts(state, 31337);
            const stack = regionFixture
                ? Uint32Array.from({ length: 4096 }, (_, index) => (Number(row[4]) + index) >>> 0)
                : new Uint32Array(fromHex(row[4]).buffer.slice(0));
            graph = new RawGraph(pool, hash, spots, types, frames, { records: recordsFromBytes(fromHex(row[3])), stack, recovery });
            if (checkingEnabled) {
                graph.bindDiagnostics({
                    checking: () => 1,
                    assertion: (expression, file, line) => {
                        assert.equal(file, '\\Ns\\O\\Graph.cpp');
                        if ((line === 0x59 && expression === 'inUse') || (line === 0x5a && expression === 'numSurfaces > 0')) diagnosticCounts.reference += 1;
                        else {
                            assert.ok(allowed.has(`${line}:${expression}:${file}`), `허용되지 않은 진단 ${line}:${expression}`);
                            diagnosticCounts.parent += 1;
                        }
                    },
                });
            }
            post = new SquidPostPop(pool, types, state, graph);
            pop = new SquidPop(pool, hash, spots, { postPop: post });
            pop.bindDiagnostics({
                types,
                trace: (level, text) => {
                    assert.ok(level === 29 && text.startsWith('ERROR: gridSid ') && text.endsWith('\n'));
                },
            });
            unpop = new SquidUnpop(pool, hash, spots);
            continue;
        }
        const args = parseRows(row[1]);
        if (row[0] === 'Pop') {
            // void 입력의 결과는 실제 x86 출력의 void 해제 여부로 읽는다. 패치 충돌은 부분 쓰기를 남긴다.
            const expectedRoot = fromHex(row[3].split(';')[0]);
            const expected = (expectedRoot[11] & 4) ? RAW_POP_RESULT.overlap : RAW_POP_RESULT.registered;
            const [flags, x, y] = args[0];
            assert.equal(pop.pop(ids[0], types[74], 1, 1, x, y, flags >>> 0), expected, `${name} Pop #${calls}`);
        } else if (row[0] === 'PostPop') post.postPop(ids[0], args[0][0] >>> 0);
        else if (row[0] === 'Region') graph.invalidateRegion(ids[0]);
        else if (row[0] === 'Add') graph.add(ids[0]);
        else if (row[0] === 'Detach') graph.detach(ids[0], args[0][0] !== 0, args[0][1]);
        else if (row[0] === 'Unpop') unpop.unpop(ids[0], types[74], args[0][0] >>> 0);
        else if (row[0] === 'Release') pool.release(ids[0]);
        else if (row[0] === 'Rebuild') graph.rebuild(args[0][0] !== 0);
        else if (row[0] === 'Allocate') assert.equal(graph.allocate(), Number(row[2]));
        else if (row[0] === 'Flood') assert.equal(graph.flood(ids[0], args[0][0]), Number(row[2]));
        else assert.fail(`${name}: 알 수 없는 행 ${row[0]}`);
        const label = `${name} ${row[0]} ${row[1]} #${calls}`;
        // 전역 재구성 fixture 는 checksum 외에 255개 레코드의 모든 바이트도 직접 비교한다.
        if (row.length === 17) assert.equal(hex(recordBytes(graph.records())), row[16], `${label} 전체 표`);
        row[3].split(';').forEach((expected, index) => assert.equal(hex(pool.slot(ids[index])), expected, `${label} 슬롯 ${index}`));
        assert.equal(adler32(recordBytes(graph.records())), Number(row[4]), `${label} 기록`);
        assert.equal(adler32(wordBytes(graph.floodStack())), Number(row[5]), `${label} 스택`);
        const parts = [0, 1, 2, 3].map((level) => new Uint8Array(hash.entries(level).buffer));
        const heads = new Uint8Array(parts.reduce((sum, part) => sum + part.length, 0));
        let offset = 0;
        // 4단계 해시 전체를 원본 배열 순서로 이어 붙여 대조한다.
        for (const part of parts) {
            heads.set(part, offset);
            offset += part.length;
        }
        assert.equal(adler32(heads), Number(row[6]), `${label} 해시`);
        assert.equal(adler32(spots), Number(row[7]), `${label} spot`);
        assert.equal(adler32(pool.bytes()), Number(row[8]), `${label} 풀`);
        assert.equal(state.totalCost, Number(row[9]), `${label} 비용`);
        assert.equal(state.depth, Number(row[10]), `${label} 깊이`);
        [state.localCounts(), state.localSecondaryCounts(), state.globalCounts()].forEach((table, index) => {
            assert.equal(adler32(wordBytes(table)), Number(row[11 + index]), `${label} 통계 ${index}`);
        });
        calls += 1;
    }
    return { calls, diagnostics: diagnosticCounts };
}

test('rawgraph: 일반 표면 Pop·PostPop·Flood·Add 를 원본 10.78 행과 대조한다', () => {
    assert.equal(replayPopGraphRows('rawgraph-x86.tsv', { regionFixture: false }).calls, 768);
});

test('regiongraph: 일반/다리/섬 Pop 의 영역 통지와 Add 를 원본 10.78 행과 대조한다', () => {
    assert.equal(replayPopGraphRows('regiongraph-x86.tsv', { regionFixture: true }).calls, 512);
});

test('graphremove: 삭제의 일반 finder·분할 정책과 다리·섬 Unpop/Release 를 원본 10.78 행과 대조한다', () => {
    assert.equal(replayPopGraphRows('graphremove-x86.tsv', { regionFixture: true }).calls, 386);
});

test('graphrecovery: 상위 호출 안의 전체 재구성·이후 flood/감소/지도/통계를 원본 10.78 행과 대조한다', () => {
    assert.equal(replayPopGraphRows('graphrecovery-x86.tsv', { regionFixture: true, globalFixture: true, recovery: GRAPH_RECOVERY.fullPool }).calls, 64);
});

test('regiongraph(checking): 같은 영역 관측을 실제 checking 경로에도 재생하며 진단은 원문 목록에 있는 것만 허용한다', () => {
    // 로그를 제외한 실제 원문 assertion 만 진단 카탈로그에 넣는다.
    const allowed = new Set();
    for (const row of readRows(fixture('graph-live-checking-x86.tsv'))) {
        for (const event of row[9].split('|')) if (event.startsWith('a:')) allowed.add(event.slice(2));
    }
    const result = replayPopGraphRows('regiongraph-x86.tsv', { regionFixture: true, checkingEnabled: true, allowed });
    assert.equal(result.calls, 512);
    assert.ok(result.diagnostics.reference > 0);
});

// 장면 하나를 만드는 공통 보조: 풀·해시·spot·상태·처리기와 슬롯 50 이다. (2026-11-08 WB04-E41)
function bridgeScene({ types = makeTypes() } = {}) {
    const pool = new SidPool(BRIDGE_POOL, { server: false });
    for (let id = 5; id <= 50; id += 1) pool.allocate(2);
    const state = new SquidPostPopState();
    state.suppressed = true;
    const body = new SquidPostPop(pool, types, state);
    return { pool, state, body, raw: pool.physical(50), types };
}

test('SquidPostPop: 최초 Pop 의 비용/통계는 재등록에서 자동 중복 증가하지 않고 직접 flags 1 은 다시 집계한다', () => {
    const types = makeTypes();
    Object.assign(types[74], { flags1: 0x10000000, flags2: 0x10000, cost: 3.75 });
    const pool = new SidPool(32768, { server: false });
    const sid = pool.allocate(2);
    put(pool.physical(sid), 0, COMMON_VTABLE);
    pool.physical(sid)[10] = 74;
    pool.physical(sid)[34] = 1;
    const state = new SquidPostPopState();
    state.localOwner = 1;
    state.providers.entries = [0];
    const post = new SquidPostPop(pool, types, state);
    const hash = new SquidHash();
    const spots = new Uint8Array(65536);
    const pop = new SquidPop(pool, hash, spots, { postPop: post });
    const unpop = new SquidUnpop(pool, hash, spots);
    assert.equal(pop.pop(sid, types[74], 1, 1, 20, 20), RAW_POP_RESULT.registered);
    assert.equal(state.totalCost, 3);
    assert.equal(state.globalCounts()[74], 1);
    assert.equal(state.depth, 0);
    unpop.unpop(sid, types[74]);
    assert.equal(pop.pop(sid, types[74], 1, 1, 30, 30), RAW_POP_RESULT.registered);
    assert.equal(state.totalCost, 3);
    assert.equal(state.providers.count, 1);
    assert.equal(state.productionDirty, 1);
    post.activate(sid, 1);
    assert.equal(state.totalCost, 6);
    assert.equal(state.providers.count, 1);
    assert.equal(state.productionDirty, 2);
    assert.equal(state.localCounts()[74], 2);
    assert.equal(state.localSecondaryCounts()[74], 2);
    assert.equal(state.globalCounts()[74], 2);
    assert.equal(state.depth, 0);
});

test('SquidPostPop: 미복원 효과/손상 목록 실패도 앞선 Pop 의 공간 변경을 되돌리지 않는다', () => {
    const types = makeTypes();
    for (const type of types) type.cost = 0; // 이 장면의 원문 입력은 비용 0 이다.
    types[74].flags2 = 0x10000;
    types[74].flags1 = 0x10000000;
    const pool = new SidPool(32768, { server: false });
    const sid = pool.allocate(2);
    put(pool.physical(sid), 0, COMMON_VTABLE);
    pool.physical(sid)[10] = 74;
    pool.physical(sid)[34] = 1;
    const state = new SquidPostPopState();
    state.providers.entries = [0];
    state.providers.count = 2;
    const post = new SquidPostPop(pool, types, state);
    const hash = new SquidHash();
    const spots = new Uint8Array(65536);
    const pop = new SquidPop(pool, hash, spots, { postPop: post });
    // 각 실패를 독립 void 입력에서 시작한다.
    const prepare = () => {
        const raw = pool.physical(sid);
        raw[11] = 4;
        raw[40] = 0;
        hash.reset();
        state.depth = 0;
    };
    // 후처리 진입 전에 완료한 공간/firstPop/void 변경을 확인한다.
    const partial = () => {
        const raw = pool.slot(sid);
        assert.deepEqual([raw[22], raw[23], raw[24], raw[25]], [0x40, 1, 0xdc, 0]);
        assert.equal(raw[11], 0);
        assert.ok(raw[40] & 0x80);
        assert.equal(hash.bucket(1, 20, 20), sid);
        assert.equal(state.depth, 1);
    };
    assert.ok(throws(() => pop.pop(sid, types[74], 1, 1, 20, 20)));
    partial();
    prepare();
    state.providers.count = 0;
    state.aiAttached[1] = true;
    assert.ok(throws(() => pop.pop(sid, types[74], 1, 1, 20, 20)));
    partial();
    assert.equal(state.totalCost, 0);
    prepare();
    state.aiAttached[1] = false;
    state.pendingPlacement = true;
    assert.ok(throws(() => pop.pop(sid, types[74], 1, 1, 20, 20)));
    partial();
    // 선택 재검사 실패 앞의 등록/통계는 이미 기록됐다.
    assert.equal(state.globalCounts()[74], 1);
    assert.equal(state.productionDirty, 2);
    assert.equal(state.providers.count, 1);
    prepare();
    state.pendingPlacement = false;
    state.suppressed = true;
    assert.equal(pop.pop(sid, types[74], 1, 1, 20, 20), RAW_POP_RESULT.registered);
    assert.equal(state.globalCounts()[74], 1);
    assert.equal(state.depth, 0);
    // 표면 그래프 생성은 거부하고 noGraph 리셋 경로만 허용한다.
    const graphTypes = types.map((type) => ({ ...type }));
    graphTypes[74].flags1 = SURFACE;
    const graphState = new SquidPostPopState();
    graphState.graphsEnabled = true;
    const graphPost = new SquidPostPop(pool, graphTypes, graphState);
    assert.ok(throws(() => graphPost.validate(sid, 1)));
    graphPost.validate(sid, 0x2000001);
    // 비유한 비용은 통계/깊이/슬롯에 쓰기 전에 거부한다.
    graphTypes[74].cost = Number.NaN;
    const invalidCost = new SquidPostPop(pool, graphTypes, graphState);
    const registered = hex(pool.slot(sid));
    assert.ok(throws(() => invalidCost.activate(sid, 0x2000001)));
    assert.equal(graphState.depth, 0);
    assert.equal(hex(pool.slot(sid)), registered);
    const other = new SidPool(32768, { server: false });
    const wrong = new SquidPostPop(other, types, state);
    assert.ok(throws(() => new SquidPop(pool, hash, spots, { postPop: wrong })));
});

test('SquidPostPop: 정수 비용표는 trunc(cost + type*23) - type*23 이며 현재 전역 비용에 더해 32비트로 감긴다', () => {
    const types = makeTypes();
    types[5].cost = -2.5;
    types[70].cost = 2147480000.5;
    const pool = new SidPool(32768, { server: false });
    const state = new SquidPostPopState();
    const body = new SquidPostPop(pool, types, state);
    state.totalCost = 10;
    assert.equal(body.totalCost(5), 7);
    state.totalCost = 2147483000;
    // 합이 signed 32비트를 넘으면 원본 ftol 의 low DWORD 로 감긴다.
    assert.equal(body.totalCost(70), (Math.fround(2147480000.5) + 2147483000) | 0);
    assert.throws(() => body.totalCost(188), RangeError);
    types[6].cost = Number.POSITIVE_INFINITY;
    assert.throws(() => new SquidPostPop(pool, types, state).totalCost(6), TypeError);
    // 로딩 비용표를 빌리면 현재 TYPE 의 비용 변경과 분리된다.
    const loaded = makeTypes();
    loaded[7].cost = 4.9;
    body.bindLoadedCosts(loaded);
    state.totalCost = 0;
    assert.equal(body.totalCost(7), 4);
    body.bindLoadedCosts(null);
    assert.equal(body.totalCost(7), 1);
});

test('다리 접두: 선택 플래그가 없으면 슬롯/연결 장치를 읽지 않고, 최초 비트 2 는 연결 누락보다 먼저 쓴다', () => {
    const pool = new SidPool(BRIDGE_POOL, { server: false });
    // 미도달 SID 를 먼저 검증하지 않는다.
    for (const flags of [0, 2, 0x20000]) bridgePrefix(pool, 65000, flags, null);
    // 최초 비트 2 저장은 연결 누락보다 먼저이며 현재 물리 수명/TYPE 를 추가 거부하지 않는다.
    for (const life of [1, 8, 9]) {
        const scene = new SidPool(BRIDGE_POOL, { server: false });
        const raw = scene.physical(50);
        raw[10] = 255;
        raw[11] = life;
        raw[40] = 0x10;
        const expected = Uint8Array.from(raw);
        expected[40] = 0x12;
        assert.ok(throws(() => bridgePrefix(scene, 50, 1, null)));
        assert.deepEqual(raw, expected);
    }
    // flags 4 의 abstract 1 은 장치 없이 반환하고 그 외는 쓰기 없이 장치를 요구한다.
    const raw = pool.physical(50);
    raw[40] = 1;
    bridgePrefix(pool, 50, 4, null);
    assert.equal(raw[40], 1);
    raw[40] = 0;
    assert.ok(throws(() => bridgePrefix(pool, 50, 4, null)));
    assert.equal(raw[40], 0);
});

test('다리 접두: 연결 복귀 뒤 현재 슬롯을 재검사하지 않고, 연결 중단과 자기 해제에도 앞선 쓰기를 보존한다', () => {
    const pool = new SidPool(BRIDGE_POOL, { server: false });
    const raw = pool.physical(50);
    raw[40] = 0;
    let calls = 0;
    bridgePrefix(pool, 50, 1, (sid) => {
        assert.equal(sid, 50);
        assert.equal(raw[40], 2);
        calls += 1;
        raw[10] = 255;
        raw[11] = 9;
        raw[40] = 0;
        put(raw, 0, 0, 4);
    });
    assert.deepEqual([calls, raw[11], raw[40], raw[10]], [1, 9, 0, 255]);
    // 연결 장치의 명시 중단 뒤 최초 쓰기/장치 쓰기를 되돌리지 않는다.
    raw[40] = 0;
    assert.ok(throws(() => bridgePrefix(pool, 50, 1, () => {
        assert.equal(raw[40], 2);
        raw[11] = 9;
        throw new Error('명시 다리 연결 중단');
    })));
    assert.deepEqual([raw[40], raw[11]], [2, 9]);
});

test('가상 분배: 다리/Teleport/섬/표면은 실제 VT 로 접두를 고르고 현재 TYPE·free 를 추가 관문으로 만들지 않는다', () => {
    // 다리: 연결 장치 설치 여부와 무관하게 VT 로 분배하고 비최초는 공통(suppressed)으로만 간다. 최초는 비트 2 저장 뒤 장치 누락으로 멈춘다.
    const bridge = bridgeScene();
    put(bridge.raw, 0, 0x5034c8, 4);
    bridge.raw[10] = 255;
    bridge.raw[11] = 9;
    bridge.raw[40] = 0;
    assert.ok(bridge.body.handlesBridge(50));
    bridge.state.depth += 1;
    bridge.body.postPopVirtual(50, 0);
    assert.equal(bridge.state.depth, 0);
    bridge.state.depth += 1;
    assert.ok(throws(() => bridge.body.postPopVirtual(50, 1)));
    assert.equal(bridge.raw[40], 2);
    assert.equal(bridge.state.depth, 1);
    // 명시 사전 검사는 무효과이며 실제 가상 몸체의 앞에 삽입되지 않는다.
    bridge.raw[10] = 74;
    bridge.raw[11] = 0;
    bridge.raw[40] = 0;
    bridge.state.depth = 0;
    const before = hex(bridge.raw);
    assert.ok(throws(() => bridge.body.validate(50, 1)));
    assert.equal(hex(bridge.raw), before);
    assert.equal(bridge.state.depth, 0);
    // Teleport: flags 0/2/4 는 현재 TYPE/free 와 무관하게 접두를 생략한다.
    for (const flags of [0, 2, 4]) {
        const scene = bridgeScene();
        scene.state.depth = 1;
        put(scene.raw, 0, 0x5070c0, 4);
        scene.raw[10] = 255;
        scene.raw[11] = 9;
        scene.raw[40] = 8;
        const saved = hex(scene.raw);
        scene.body.postPopVirtual(50, flags);
        assert.equal(scene.state.depth, 0);
        assert.equal(hex(scene.raw), saved);
    }
    // 섬/표면: 비최초는 접두 장치 없이 공통으로 가고, 최초는 장치 누락으로 깊이와 물리를 유지한 채 멈춘다.
    for (const vtable of [0x506d98, 0x506f08]) {
        const scene = bridgeScene();
        put(scene.raw, 0, vtable, 4);
        scene.raw[10] = 255;
        scene.raw[11] = 9;
        for (const flags of [0, 2, 4]) {
            scene.state.depth = 1;
            scene.body.postPopVirtual(50, flags);
            assert.equal(scene.state.depth, 0);
            assert.equal(scene.raw[11], 9);
        }
        scene.state.depth = 1;
        const saved = hex(scene.raw);
        assert.ok(throws(() => scene.body.postPopVirtual(50, 1)));
        assert.equal(scene.state.depth, 1);
        assert.equal(hex(scene.raw), saved);
    }
});

test('가상 분배: 접두가 자기 연결을 해제해도 캡처를 유지하고, 접두가 바꾼 현재 TYPE 의 통계와 앞선 효과를 보존한다', () => {
    // 섬/표면/Teleport: 접두가 자기 연결을 바꿔도 실행 중 callable 은 끝까지 유효하다.
    for (const kind of ['island', 'surface', 'teleport']) {
        const scene = bridgeScene();
        scene.state.depth = 1;
        put(scene.raw, 0, kind === 'island' ? 0x506d98 : kind === 'surface' ? 0x506f08 : 0x5070c0, 4);
        scene.raw[10] = 74;
        const setter = { island: 'setIslandPrefix', surface: 'setSurfacePrefix', teleport: 'setTeleportPrefix' }[kind];
        let ran = 0;
        scene.body[setter]((sid, flags) => {
            assert.equal(flags, kind === 'teleport' ? 3 : 1);
            scene.body[setter](null);
            ran += 1;
            scene.raw[10] = 255;
        });
        scene.body.postPopVirtual(50, kind === 'teleport' ? 3 : 1);
        assert.equal(ran, 1);
        assert.equal(scene.state.depth, 0);
        assert.equal(scene.raw[10], 255);
    }
    // Teleport 접두의 명시 중단은 앞선 쓰기를 남기고 깊이를 줄이지 않는다.
    const thrown = bridgeScene();
    thrown.state.depth = 1;
    put(thrown.raw, 0, 0x5070c0, 4);
    thrown.body.setTeleportPrefix(() => {
        thrown.raw[34] = 8;
        throw new Error('명시 생성 중단');
    });
    assert.ok(throws(() => thrown.body.postPopVirtual(50, 1)));
    assert.deepEqual([thrown.raw[34], thrown.state.depth], [8, 1]);
    // 접두가 바꾼 현재 TYPE 의 공통 통계 뒤 선택 재검사 실패까지 앞선 효과를 보존한다.
    const late = bridgeScene();
    late.state.suppressed = false;
    late.state.depth = 1;
    late.state.pendingPlacement = true;
    late.state.localOwner = 1;
    late.raw.fill(0);
    put(late.raw, 0, 0x5070c0, 4);
    late.raw[10] = 74;
    late.raw[34] = 1;
    let calls = 0;
    late.body.setTeleportPrefix(() => {
        calls += 1;
        late.raw[10] = 75;
    });
    assert.ok(throws(() => late.body.postPopVirtual(50, 0x2000001)));
    assert.equal(calls, 1);
    assert.equal(late.state.globalCounts()[75], 1);
    assert.equal(late.state.globalCounts()[74], 0);
    assert.equal(late.state.depth, 1);
    // Teleport 의 명시 사전 검사는 raw 수명 거부이며 실제 가상 몸체 앞에 삽입되지 않는다.
    const explicit = bridgeScene();
    explicit.state.depth = 1;
    put(explicit.raw, 0, 0x5070c0, 4);
    explicit.raw[10] = 74;
    explicit.raw[11] = 1;
    let ran = 0;
    explicit.body.setTeleportPrefix(() => { ran += 1; });
    assert.ok(throws(() => explicit.body.validate(50, 1)));
    assert.equal(ran, 0);
    explicit.body.postPopVirtual(50, 1);
    assert.deepEqual([ran, explicit.state.depth], [1, 0]);
});

test('가상 분배: 등록된 파생 몸체는 자기 postPop 을 부르고 공통 깊이를 이중으로 줄이지 않으며 풀 불일치/미등록은 거부한다', () => {
    const scene = bridgeScene();
    scene.state.suppressed = false;
    const calls = [];
    const body = (name, vtable) => ({
        pool: scene.pool,
        handles: (sid) => (scene.pool.slot(sid)[0] | (scene.pool.slot(sid)[1] << 8) | (scene.pool.slot(sid)[2] << 16)) === (vtable & 0xffffff),
        validate: () => calls.push(`${name}:validate`),
        postPop: (sid, flags) => { calls.push(`${name}:${sid}:${flags}`); scene.state.depth -= 1; },
    });
    scene.body.setBody('dude', body('dude', 0x112233));
    put(scene.raw, 0, 0x112233, 4);
    scene.raw[10] = 70;
    scene.raw[11] = 0;
    scene.state.depth = 1;
    assert.ok(scene.body.handlesDerived(50));
    scene.body.postPopVirtual(50, 5);
    assert.deepEqual(calls, ['dude:50:5']);
    assert.equal(scene.state.depth, 0);
    // 활성화는 사전 검사 → 깊이 증가 → 분배 → 깊이 복귀 확인이다.
    scene.body.setBody('dude', body('dude', 0x112233));
    calls.length = 0;
    scene.state.suppressed = true;
    scene.body.activate(50, 0);
    assert.deepEqual(calls, ['dude:validate', 'dude:50:0']);
    assert.equal(scene.state.depth, 0);
    // 다른 풀의 파생 몸체와 알 수 없는 종류는 등록 단계에서 거부한다.
    const other = new SidPool(BRIDGE_POOL, { server: false });
    assert.throws(() => scene.body.setBody('dude', { pool: other, handles: () => true }), TypeError);
    assert.throws(() => scene.body.setBody('unknown', null), RangeError);
    // 해제하면 그 VT 는 지원하지 않는 가상 후처리가 된다.
    scene.body.setBody('dude', null);
    scene.state.depth = 1;
    assert.ok(throws(() => scene.body.postPopVirtual(50, 0)));
    assert.equal(scene.state.depth, 1);
    // 생성자의 풀/그래프/타입 크기 검사.
    assert.throws(() => new SquidPostPop(scene.pool, makeTypes().slice(1), scene.state), RangeError);
    const graph = new RawGraph(scene.pool, new SquidHash(), new Uint8Array(65536), makeTypes(), makeFrames());
    assert.throws(() => new SquidPostPop(other, makeTypes(), scene.state, graph), TypeError);
    const mismatched = makeTypes();
    mismatched[10].footX = 5;
    assert.throws(() => new SquidPostPop(scene.pool, mismatched, scene.state, graph), TypeError);
    const linked = new SquidPostPop(scene.pool, makeTypes(), scene.state, graph);
    assert.throws(() => linked.validateSpace(new SquidHash(), new Uint8Array(65536)), TypeError);
});
