// 위험한 다리 위 이동체 대피 raw-bridge-carriers.js(RawBridgeCarriers)를 원본 10.78 독립 관측 bridge-carrier-retreat-x86.tsv 258행과 대조한다.
// 각 행은 장면 입력(연산·권한·종류·이동 Form 유무·위치 시나리오·수명·후보 flags/Graph·시작 결과·checking·후보 순서·행위자 좌표 비트)에서 4개 슬롯을 실제 raw 풀과 4단계 해시에 설치하고,
// 실제 finder 후보 순서·이동 요청·거부·전체 50바이트 슬롯을 원문 기록과 비교한다. 기대 선택점은 계산하지 않고 입력만 구성한다. (2026-11-08 WB04-E46: RawBridgeCarriers 이식과 함께 추가)
import assert from 'node:assert/strict';
import path from 'node:path';
import test from 'node:test';
import { RawBridgeCarriers } from '../www/core/raw-bridge-carriers.js';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { REPO_ROOT } from './helpers.mjs';
import { bitsToFloat, floatToBits, get, put, readRows } from './raw-support.mjs';

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E46)
const hex = (bytes) => Array.from(bytes, (byte) => byte.toString(16).padStart(2, '0')).join('');

// 원문 생성기와 같은 명시 위치 입력(후보 61 의 (x,y)와 62 의 (x,y))이며 선택점은 계산하지 않는다. (2026-11-08 WB04-E46)
const POSITIONS = [[22, 21, 20, 23], [21, 21, 20, 22], [21, 22, 19, 20], [22, 21, 18, 21], [20.125, 21, 20, 22.125], [20, 21, 22, 23]];

// 이 장면은 원문 입력만 구성하고 기대 이동점/사건은 동결 TSV 에서 읽는다. 풀은 새로 만들지 않고 입력 슬롯만 지운다. (2026-11-08 WB04-E46)
function runRow(columns, pool) {
    const c = columns[0].split(',').map((value) => Number(value) >>> 0);
    assert.equal(c.length, 13);
    const hash = new SquidHash();
    const types = Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, footX: 1, footY: 1 }));
    const points = POSITIONS[c[4]];
    const ids = [50, 60, 61, 62];
    if (c[10]) [ids[2], ids[3]] = [ids[3], ids[2]];
    // 동일 타입/상태/좌표/바이트를 설치한 뒤 같은 순서로 해시 머리에 삽입한다.
    for (const id of ids) {
        const raw = pool.physical(id);
        raw.fill(0);
        put(raw, 0, 0x11018000, 4);
        const kind = id === 50 ? 82 : id === 60 ? c[2] : id === 61 ? 70 : 71;
        raw[10] = kind;
        types[kind].footX = 1;
        types[kind].footY = 1;
        types[kind].flags1 = id === 60 ? 0 : id === 61 ? c[6] : 0x800;
        types[kind].flags2 = id === 60 ? 0x10000 : id === 62 ? 2 : 4;
        const x = id === 50 ? 20 : id === 60 ? bitsToFloat(c[11]) : id === 61 ? points[0] : points[2];
        const y = id === 50 ? 21 : id === 60 ? bitsToFloat(c[12]) : id === 61 ? points[1] : points[3];
        put(raw, 14, floatToBits(x), 4);
        put(raw, 18, floatToBits(y), 4);
        raw[30] = id === 60 ? 254 : id === 61 ? c[7] : 1;
        put(raw, 12, id === 50 ? 40 : id === 61 ? c[5] * 8 : 0, 2);
        const level = id === 60 ? 1 : 0;
        put(raw, 4, hash.bucket(level, x, y), 2);
        hash.setBucket(level, x, y, id);
    }
    const boss = c[1] !== 0;
    const checking = c[9] !== 0;
    const events = [];
    const hooks = {
        // 이전 Graph 모듈과 같은 명시 해시 입력을 공급하며 기대 선택을 계산하지 않는다.
        graphFor: (sid) => {
            const raw = pool.slot(sid);
            if (types[raw[10]].flags1 & 0x800) return raw[30];
            const view = new DataView(raw.buffer, raw.byteOffset, raw.byteLength);
            const x = Math.trunc(view.getFloat32(14, true) + Math.fround(0.9999));
            const y = Math.trunc(view.getFloat32(18, true) + Math.fround(0.9999));
            const head = hash.cell(0, x, y);
            if (head && !(types[pool.slot(head)[10]].flags1 & 0x800)) events.push('A:1849');
            return head ? pool.slot(head)[30] : 254;
        },
        movementType: (type) => { events.push(`T:${type}:0:0`); return 46; },
        movementForm: (sid, type) => { events.push(`P:${sid}:${type}:0:0`); return c[3] ? 900 : 0; },
        start: (sid, point) => { events.push(`M:${sid}:${floatToBits(point.x)}:${floatToBits(point.y)}:0:0:0:0:0`); return c[8] !== 0; },
        valid: (point) => point.x > 0 && point.y > 0 && point.x < 256 && point.y < 256,
        assertion: (at) => events.push(`A:${at}`),
        wontWalkOffEdge: (type) => type === 158 || type === 156 || type === 137 || type === 135 || type === 187,
        scan: (area) => events.push(`F:${area.left >>> 0}:${area.top >>> 0}:${area.right >>> 0}:${area.bottom >>> 0}`),
    };
    const carriers = new RawBridgeCarriers(pool, hash, types, () => boss, () => checking, () => 158, () => 156, () => 7, hooks);
    if (c[0]) carriers.check(50);
    else carriers.retreat(60);
    return { events: events.join(';'), slots: ids.length ? [50, 60, 61, 62].map((id) => hex(pool.slot(id))) : [] };
}

test('bridge-carriers: 원본 258행의 후보 순서·이동 요청·거부·전체 슬롯을 같은 물리 입력으로 재생한다', () => {
    const pool = new SidPool(24000, { server: true });
    let count = 0;
    // 원문을 새 생산 몸체의 결과로 다시 작성하거나 행을 생략하지 않는다.
    for (const columns of readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'bridge-carrier-retreat-x86.tsv'))) {
        assert.equal(columns.length, 3);
        const result = runRow(columns, pool);
        assert.equal(result.events, columns[1] === '-' ? '' : columns[1], `행 ${count} (${columns[0]}) 사건`);
        const wanted = columns[2].split(',');
        // 원문이 관측한 네 전체 슬롯도 바이트로 보존 비교한다.
        [50, 60, 61, 62].forEach((id, index) => assert.equal(result.slots[index], wanted[index], `행 ${count} 슬롯 ${id}`));
        count += 1;
    }
    assert.equal(count, 258);
});

test('bridge-carriers: 구성 오류 거부, 누락된 효과는 도달한 지점에서만 실패, 권한/Graph 없음은 자연 반환한다', () => {
    const pool = new SidPool(24000, { server: true });
    const hash = new SquidHash();
    const types = Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, footX: 1, footY: 1 }));
    const constant = (value) => () => value;
    assert.throws(() => new RawBridgeCarriers(pool, hash, types.slice(1), constant(true), constant(0), constant(158), constant(156), constant(7), {}), TypeError);
    // 권한이 없으면 효과를 읽지 않고 반환한다.
    const idle = new RawBridgeCarriers(pool, hash, types, constant(false), constant(0), constant(158), constant(156), constant(7), {});
    idle.retreat(60);
    // 권한이 있으면 Graph 조회 장치가 있어야 한다.
    const missing = new RawBridgeCarriers(pool, hash, types, constant(true), constant(0), constant(158), constant(156), constant(7), {});
    assert.throws(() => missing.retreat(60), /미연결/);
    // 다른 종류의 객체만 있으면 check 는 아무 효과도 부르지 않는다.
    missing.check(50);
});
