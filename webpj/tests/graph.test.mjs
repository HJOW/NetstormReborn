// 표면 이웃 탐색(SurfaceFinder)과 Graph 정수 연산(할당·flood·병합·삭제 분리·전체 재구성)을 원본 10.78 관측과 보호 경로로 대조한다(WB04-E12, cpppj GraphTests 이식).
//  - graph-x86.tsv 의 originals 두 구간(Begin originals 639/895): Add/Flood/Allocate/Free/Remove 각 512행씩 모두 1280개 검사 지점에서 표 255행 해시, 4096 DWORD 스택 해시, 객체별 번호/상태를 대조한다.
//    originalCD 두 구간(10.72)은 제외한다.
//  - 계약: 크기 동률은 첫 이웃의 그래프, 여러 칸 발자국은 한 객체로 셈, WORD 감김, 소진/손상 입력은 부분 변경 없이 거부, 마지막 연결 무리는 기존 번호에 남음, 독립 무리 249개의 전체 재구성,
//    기준점 내부 표면은 번호를 따로 초기화하지 않음.
// (2026-11-08 WB04-E12: 표면 이웃 탐색과 Graph 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, adler32 } from './raw-support.mjs';
import { SurfaceFinder, TYPE_FLAG1_SURFACE } from '../www/core/surface-finder.js';
import { Graph, GRAPH_COUNT, GRAPH_TABLE_SIZE } from '../www/core/graph.js';
import { WORLD_CELLS } from '../www/assets/terrain-mask.js';

// 원본 관측 행이다. 파일이 없으면 검사를 건너뛴다. (2026-11-08 WB04-E12)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'graph-x86.tsv'));

// 'a,b,c;d,e,f' 형식의 행을 부호 있는 정수 배열의 배열로 읽는다('-' 는 빈 목록). (2026-11-08 WB04-E12)
function parseRows(text) {
    if (text === '-') return [];
    return text.split(';').map((row) => row.split(',').map((value) => Number(value)));
}

// 표 레코드의 세 WORD(surfaces·inUse·reserved)를 리틀엔디언으로 직렬화해 Adler-32 를 계산한다(reserved 포함). (2026-11-08 WB04-E12)
function recordsHash(graph) {
    const bytes = new Uint8Array(GRAPH_TABLE_SIZE * 6);
    graph.records().forEach((record, index) => {
        [record.surfaces, record.inUse, record.reserved].forEach((value, field) => {
            bytes[index * 6 + field * 2] = value & 0xff;
            bytes[index * 6 + field * 2 + 1] = (value >> 8) & 0xff;
        });
    });
    return adler32(bytes);
}

// 스택의 사용하지 않는 흔적까지 DWORD 폭으로 직렬화해 Adler-32 를 계산한다. (2026-11-08 WB04-E12)
function stackHash(graph) {
    const stack = graph.floodStack();
    const bytes = new Uint8Array(stack.length * 4);
    stack.forEach((value, index) => { for (let byte = 0; byte < 4; byte += 1) bytes[index * 4 + byte] = (value >>> (8 * byte)) & 0xff; });
    return adler32(bytes);
}

// 작은 표면의 전체 발자국 번호 지도를 독립 입력으로 만든다(뒤 입력 객체가 같은 칸을 덮어쓴다). (2026-11-08 WB04-E12)
function makeMap(objects) {
    const map = new Uint16Array(WORLD_CELLS * WORLD_CELLS);
    for (const object of objects) {
        // 발자국의 행과 열을 채운다.
        for (let y = object.y - object.height + 1; y <= object.y; y += 1) for (let x = object.x - object.width + 1; x <= object.x; x += 1) map[y * WORLD_CELLS + x] = object.id;
    }
    return map;
}

// 실제 표 hex 를 WORD 세 개의 필드(부호 있는 16비트)로 읽는다. (2026-11-08 WB04-E12)
function parseRecords(hex) {
    const records = [];
    for (let index = 0; index < GRAPH_TABLE_SIZE; index += 1) {
        const fields = [0, 1, 2].map((field) => {
            const position = index * 12 + field * 4;
            const value = parseInt(hex.slice(position, position + 2), 16) | (parseInt(hex.slice(position + 2, position + 4), 16) << 8);
            return (value << 16) >> 16;
        });
        records.push({ surfaces: fields[0], inUse: fields[1], reserved: fields[2] });
    }
    return records;
}

// 합성 한 칸 섬 표면이다(실제 프레임 입력과 분리). (2026-11-08 WB04-E12)
const node = (id, x, y, width = 1) => ({ id, x, y, width, height: 1, flags1: TYPE_FLAG1_SURFACE, flags2: 8, frame: { side: 65, variant: 80, number: 1, flags: 0 }, dead: false, buried: false });

// 같은 합성 입력으로 탐색기와 Graph 를 만든다(spots 는 기본 빈 지도). (2026-11-08 WB04-E12)
function build(objects, members, { spots = new Uint8Array(WORLD_CELLS * WORLD_CELLS), records = [] } = {}) {
    const finder = new SurfaceFinder(objects, makeMap(objects), spots);
    return { finder, graph: new Graph(finder, members, records) };
}

// 오류가 호출자에게 보고되는지 확인한다. (2026-11-08 WB04-E12)
const throwsAny = (operation) => { try { operation(); } catch { return true; } return false; };

// 두 판본의 Add/flood/할당/반납/감소가 남긴 모든 멤버와 버퍼를 대조한다(10.78 구간만).
test('Graph: 원본10.78 Add·Flood·Allocate·Free·Remove 1280개 검사 지점', { skip: !rows && '원본 fixture 없음' }, () => {
    let section = '';
    let begins = 0;
    let checks = 0;
    let finder = null;
    let graph = null;
    for (const row of rows) {
        if (row[0] === 'Begin') { section = row[1]; if (section === 'originals') begins += 1; continue; }
        if (section !== 'originals') continue;
        if (row[0] === 'Setup') {
            const objects = parseRows(row[1]).map((v) => ({ id: v[0], x: v[1], y: v[2], width: v[3], height: v[4], flags1: v[5], flags2: v[6], frame: { side: v[7], variant: v[8], number: 1, flags: 0 }, dead: (v[10] ?? 0) !== 0, buried: (v[11] ?? 0) !== 0 }));
            const spots = new Uint8Array(WORLD_CELLS * WORLD_CELLS);
            // 명시한 내부 비트만 준비한다.
            for (const v of parseRows(row[2])) spots[v[1] * WORLD_CELLS + v[0]] = v[2];
            // membership 순서도 fixture 의 번호와 같게 둔다.
            const members = parseRows(row[3]).map((v) => ({ id: v[0], graph: v[1], state: v[2] }));
            finder = new SurfaceFinder(objects, makeMap(objects), spots);
            graph = new Graph(finder, members, parseRecords(row[4]));
            continue;
        }
        const args = row[1] === '-' ? [] : parseRows(row[1])[0];
        const label = `검사 ${checks} ${row[0]} ${row[1]}`;
        if (row[0] === 'Add') graph.add(args[0]);
        else if (row[0] === 'Flood') assert.equal(graph.flood(args[0], args[1]), Number(row[2]), `${label} 변경 수`);
        else if (row[0] === 'Allocate') assert.equal(graph.allocate(), Number(row[2]), `${label} 번호`);
        else if (row[0] === 'Free') graph.free(args[0]);
        else if (row[0] === 'Remove') graph.remove(args[0]);
        else assert.fail(`알 수 없는 작업 ${row[0]}`);
        assert.equal(recordsHash(graph), Number(row[3]), `${label} 표`);
        assert.equal(stackHash(graph), Number(row[4]), `${label} 스택`);
        // graph/state 는 모든 객체별로 직접 비교한다.
        for (const v of parseRows(row[5])) {
            assert.equal(graph.number(v[0]), v[1], `${label} 객체 ${v[0]} 번호`);
            assert.equal(graph.state(v[0]), v[2], `${label} 객체 ${v[0]} 상태`);
        }
        checks += 1;
    }
    assert.deepEqual([begins, checks], [2, 1280]);
});

// 크기 동률은 y/x 순서의 위쪽 그래프를 택하고 두 노드짜리 왼쪽 무리를 합친다.
test('Graph: 크기 동률은 첫 이웃을 택하고 연결된 패배 무리를 병합한다', () => {
    const objects = [node(5, 20, 20), node(6, 19, 20), node(7, 21, 19), node(8, 20, 19), node(9, 18, 20)];
    const members = [{ id: 5, graph: 254, state: 0 }, { id: 6, graph: 1, state: 0 }, { id: 7, graph: 0, state: 0 }, { id: 8, graph: 0, state: 0 }, { id: 9, graph: 1, state: 0 }];
    const records = Array.from({ length: GRAPH_TABLE_SIZE }, () => ({ surfaces: 0, inUse: 0, reserved: 0 }));
    records[0] = { surfaces: 2, inUse: 1, reserved: 71 };
    records[1] = { surfaces: 2, inUse: 1, reserved: 83 };
    const { graph } = build(objects, members, { records });
    graph.add(5);
    assert.equal(graph.inUse(), 1);
    assert.equal(graph.records()[0].surfaces, 5);
    assert.deepEqual([graph.number(5), graph.number(6), graph.number(9)], [0, 0, 0]);
    assert.equal(graph.records()[1].surfaces, 0);
    assert.deepEqual([graph.records()[0].reserved, graph.records()[1].reserved], [71, 83]);
});

// 여러 칸 발자국도 한 표면 객체로 세며 WORD 증가/감소는 16비트로 감긴다.
test('Graph: 여러 칸 발자국은 한 객체이고 WORD 는 16비트로 감긴다', () => {
    const objects = [node(5, 30, 30, 3)];
    const members = [{ id: 5, graph: 254, state: 0 }];
    const { finder, graph } = build(objects, members);
    graph.add(5);
    assert.deepEqual([graph.records()[0].surfaces, graph.inUse()], [1, 1]);
    graph.remove(0);
    assert.deepEqual([graph.records()[0].surfaces, graph.inUse(), graph.number(5)], [0, 0, 0]);
    const records = Array.from({ length: GRAPH_TABLE_SIZE }, () => ({ surfaces: 0, inUse: 0, reserved: 0 }));
    records[0] = { surfaces: 32767, inUse: 1, reserved: 123 };
    const wrap = new Graph(finder, members, records);
    assert.equal(wrap.flood(5, 0), 1);
    assert.equal(wrap.records()[0].surfaces, -32768);
    assert.equal(wrap.flood(5, 254), 1);
    assert.deepEqual([wrap.records()[0].surfaces, wrap.records()[0].reserved], [32767, 123]);
});

// 소진/손상/미확보 membership 은 원본의 부분 변경을 호출자에게 전파하지 않는다.
test('Graph: 잘못된 입력과 소진은 변경 전에 거부한다', () => {
    const objects = [node(5, 20, 20), node(6, 21, 20)];
    const members = [{ id: 5, graph: 254, state: 0 }];
    const { finder, graph: missing } = build(objects, members);
    const before = recordsHash(missing);
    const stack = stackHash(missing);
    assert.ok(throwsAny(() => missing.add(5)));
    assert.deepEqual([recordsHash(missing), stackHash(missing), missing.number(5)], [before, stack, 254]);
    assert.ok(throwsAny(() => missing.flood(5, 251)));
    assert.ok(throwsAny(() => missing.free(253)));
    assert.ok(throwsAny(() => missing.remove(254)));
    const records = Array.from({ length: GRAPH_TABLE_SIZE }, () => ({ surfaces: 0, inUse: 0, reserved: 0 }));
    // 정상 레코드만 소진시키며 sentinel/미사용 레코드를 보존한다.
    for (let index = 0; index < GRAPH_COUNT; index += 1) records[index] = { surfaces: 1, inUse: 1, reserved: 123 };
    const full = new Graph(finder, members, records);
    const fullBefore = recordsHash(full);
    assert.ok(throwsAny(() => full.allocate()));
    assert.deepEqual([recordsHash(full), full.inUse()], [fullBefore, 251]);
});

// 마지막 연결 무리는 원래 번호에 남고 소진/잘못된 감소 수는 모든 표/스택/번호를 보존한다.
test('Graph: 삭제 준비는 마지막 무리를 남기고 소진 시 원자적으로 거부한다', () => {
    const dead = { ...node(5, 20, 20), dead: true };
    const objects = [dead, node(6, 19, 20), node(7, 21, 20), node(8, 18, 20)];
    const members = [{ id: 5, graph: 0, state: 2 }, { id: 6, graph: 0, state: 0 }, { id: 7, graph: 0, state: 0 }, { id: 8, graph: 0, state: 0 }];
    const records = Array.from({ length: GRAPH_TABLE_SIZE }, () => ({ surfaces: 0, inUse: 0, reserved: 0 }));
    records[0] = { surfaces: 4, inUse: 1, reserved: 71 };
    records[1].reserved = 83;
    const connections = [6, 7];
    const { finder, graph } = build(objects, members, { records });
    graph.detach(5, connections);
    assert.deepEqual([graph.number(5), graph.state(5)], [0, 2]);
    assert.deepEqual([graph.number(6), graph.number(8), graph.number(7)], [1, 1, 0]);
    assert.deepEqual([graph.records()[0].surfaces, graph.records()[1].surfaces], [1, 2]);
    assert.deepEqual([graph.records()[0].reserved, graph.records()[1].reserved], [71, 83]);
    const hash = recordsHash(graph);
    const stack = stackHash(graph);
    assert.ok(throwsAny(() => graph.detach(5, connections, false, 2)));
    assert.deepEqual([recordsHash(graph), stackHash(graph)], [hash, stack]);
    // 첫 새 무리를 만들 번호가 없으면 복사본의 부분 flood 도 원본에 반영하지 않는다.
    for (let index = 0; index < GRAPH_COUNT; index += 1) records[index] = { surfaces: 4, inUse: 1, reserved: 71 };
    const full = new Graph(finder, members, records);
    const fullBefore = recordsHash(full);
    const oldStack = stackHash(full);
    assert.ok(throwsAny(() => full.detach(5, connections, true)));
    assert.deepEqual([recordsHash(full), stackHash(full)], [fullBefore, oldStack]);
    assert.deepEqual([5, 6, 7, 8].map((id) => full.number(id)), [0, 0, 0, 0]);
});

// 예약 0번·임시 1번 때문에 한 번의 전체 재구성은 독립 무리 249개까지 확보할 수 있다.
test('Graph: 전체 재구성은 독립 무리 249개까지 확보하고 초과하면 원자적으로 거부한다', () => {
    const objects = [];
    const members = [];
    // 각 표면은 세 칸 간격으로 떨어져 원본 탐색에서도 연결되지 않는다.
    for (let index = 0; index < 250; index += 1) {
        objects.push(node(5 + index, 20 + (index % 25) * 3, 20 + Math.floor(index / 25) * 3));
        members.push({ id: 5 + index, graph: 254, state: 0 });
    }
    const spots = new Uint8Array(WORLD_CELLS * WORLD_CELLS);
    const records = Array.from({ length: GRAPH_TABLE_SIZE }, () => ({ surfaces: 0, inUse: 0, reserved: 0 }));
    // 소진과 reserved 보존을 함께 확인한다.
    for (let index = 0; index < GRAPH_COUNT; index += 1) records[index] = { surfaces: 7, inUse: 1, reserved: 0x1234 + index };
    const { graph: tooMany } = build(objects, members, { spots, records });
    const before = recordsHash(tooMany);
    const stack = stackHash(tooMany);
    assert.ok(throwsAny(() => tooMany.rebuild(true)));
    assert.ok(throwsAny(() => tooMany.allocateWithRecovery()));
    assert.deepEqual([recordsHash(tooMany), stackHash(tooMany), tooMany.number(5), tooMany.number(254)], [before, stack, 254, 254]);
    objects.pop();
    members.pop();
    const { graph } = build(objects, members, { spots, records });
    assert.equal(graph.allocateWithRecovery(), 1);
    assert.equal(graph.inUse(), 251);
    assert.deepEqual([graph.records()[0].reserved, graph.records()[1].surfaces], [0x1201, 0]);
    // SID 오름차순으로 무리 번호 2..250 을 확보하고 각 크기를 1 로 만든다.
    for (let index = 0; index < 249; index += 1) {
        assert.equal(graph.number(5 + index), index + 2);
        assert.equal(graph.records()[index + 2].surfaces, 1);
        assert.equal(graph.records()[index + 2].reserved, records[index + 2].reserved);
    }
});

// 원본은 기준점 내부 표면을 첫 수집에서 제외하지만 그 번호를 별도로 초기화하지 않는다.
test('Graph: 재구성은 내부 표면의 번호를 보존하고 소속에 없는 flood 대상을 거부한다', () => {
    const objects = [node(5, 20, 20, 3), node(6, 40, 40), node(7, 21, 20)];
    const members = [{ id: 5, graph: 3, state: 0 }, { id: 6, graph: 254, state: 0 }];
    const spots = new Uint8Array(WORLD_CELLS * WORLD_CELLS);
    spots[20 * WORLD_CELLS + 20] = 8;
    const { finder, graph } = build(objects, members, { spots });
    assert.ok(finder.originInterior(5));
    graph.rebuild(true);
    assert.deepEqual([graph.number(5), graph.number(6)], [3, 2]);
    spots[20 * WORLD_CELLS + 20] = 0;
    const { graph: missing } = build(objects, members, { spots });
    const before = recordsHash(missing);
    const stack = stackHash(missing);
    assert.ok(throwsAny(() => missing.rebuild(true)));
    assert.deepEqual([recordsHash(missing), stackHash(missing), missing.number(5), missing.number(6)], [before, stack, 3, 254]);
});
