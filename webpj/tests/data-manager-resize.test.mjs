// DataManager 임시 stream 생성·이름 교체·최소 확장·섹션 생성/복제(cpppj `o/RawDataManagerResize`, 원본 0044d070 · 0044d160 · 0044d1b0 · 0044d230 · 0044d290)의 웹 대응 `core/raw-data-manager-resize.js` 를 검사한다(4-B, WB04-E283).
//  - 고정 관측 `datamanager-resize-x86.tsv` 286행: 전체 796BYTE(manager 12 + 노드 4개 + 버퍼 5개 + 이름 2개) 위에서 최소 확장(G)·이름 교체(R)·생성자(그 밖)를 이름 종류와 4개 변이(생성 중 원문 data/end 변경 · 반납 뒤 current 변경 ·
//    할당 중 임시 alloc 변경)로 실행해 반환 핸들·실행 뒤 전체 메모리·외부 효과(비교 Q · 할당 A · 노드 할당 S · 버퍼 반납 F · 노드 반납 N)의 순서와 그때의 전체 메모리를 대조한다.
//  - 고정 관측 `datamanager-create-x86.tsv` 220행: 전체 17380BYTE(manager 둘 + 노드 5개 + 버퍼 6개 + 16384 큰 버퍼 + 이름) 위에서 섹션 생성(N)·복제(C)·MemStream 대입(M)을 변이 4종으로 실행해 매 경계 직전 전체 메모리와 최종 메모리를 대조한다.
//    행의 메모리는 `~값x개수~` 반복 표기로 압축되어 있다.
//  - 손 시험(cpppj DataManagerCreate_EmptyNullAllocationAndRequiredEffects · MemStreamCopy_SelfAssignmentResetsBeforeReadingLength)과 fixture 가 구분하지 못하는 경계(이름 40BYTE·null 이름·크기 0 → 1·signed 최소·누락 장치) 계약.
//    CampaignTemplates_* 시험은 CampaignTemplateStorage(4-D)가 이 모듈을 쓸 때 옮긴다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-11 WB04-E283)
import test from 'node:test';
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import { dirname, resolve } from 'node:path';
import { compactMemStream, copyMemStream } from '../www/core/raw-mem-stream.js';
import { compareDataManagerSectionNames } from '../www/core/raw-data-manager-selection.js';
import {
    DATA_MANAGER_TEMPORARY_NAME, cloneDataManagerStream, constructDataManagerStream, createDataManagerStream, ensureDataManagerStreamSize, renameDataManagerStream,
} from '../www/core/raw-data-manager-resize.js';

// 저장소 안의 고정 관측 폴더이다. (2026-11-11 WB04-E283)
const FIXTURES = resolve(dirname(fileURLToPath(import.meta.url)), '../../cpppj/tests/fixtures');

// 주석(#)과 빈 줄을 뺀 고정 관측 행의 칸 배열이다. (2026-11-11 WB04-E283)
function readRows(name) {
    return readFileSync(`${FIXTURES}/${name}`, 'utf8').split(/\r?\n/).filter((line) => line !== '' && line[0] !== '#').map((line) => line.split('\t'));
}

// 소문자 16진수 문자열 ↔ 바이트 배열. (2026-11-11 WB04-E283)
const hexBytes = (hex) => Uint8Array.from(hex.match(/../g) ?? [], (pair) => Number.parseInt(pair, 16));
const bytesHex = (bytes) => Array.from(bytes, (byte) => byte.toString(16).padStart(2, '0')).join('');

// 같은 바이트의 반복 표기 `~값x개수~`(값은 16진 한 BYTE, 개수는 10진)도 빠짐없이 전체 메모리로 복원한다. (2026-11-11 WB04-E283)
function createBytes(text) {
    const out = [];
    let position = 0;
    // 일반 16진 두 글자와 반복 표기를 원래 순서로 읽는다.
    while (position < text.length) {
        if (text[position] !== '~') {
            out.push(Number.parseInt(text.slice(position, position + 2), 16));
            position += 2;
            continue;
        }
        const end = text.indexOf('~', position + 1);
        const separator = text.indexOf('x', position + 1);
        if (end < 0 || separator < 0 || separator >= end) throw new Error('생성 원문의 반복 표기 누락');
        const value = Number.parseInt(text.slice(position + 1, separator), 16);
        const count = Number.parseInt(text.slice(separator + 1, end), 10);
        for (let index = 0; index < count; index += 1) out.push(value);
        position = end + 1;
    }
    return Uint8Array.from(out);
}

// 비정렬 DWORD 읽기/쓰기이다. (2026-11-11 WB04-E283)
const get = (bytes, at) => (bytes[at] | (bytes[at + 1] << 8) | (bytes[at + 2] << 16) | (bytes[at + 3] << 24)) >>> 0;
function put(bytes, at, value) {
    for (let index = 0; index < 4; index += 1) bytes[at + index] = (value >>> (index * 8)) & 255;
}

// 원문 이름을 첫 NUL 까지 읽은 이진 문자열로 돌려준다. (2026-11-11 WB04-E283)
function nameAt(bytes, position) {
    const end = bytes.indexOf(0, position);
    if (end < 0) throw new RangeError('원문 이름 NUL 누락');
    return String.fromCharCode(...bytes.subarray(position, end));
}

// 이름 문자열의 바이트 16진 표기이다. (2026-11-11 WB04-E283)
const nameHex = (text) => bytesHex(Uint8Array.from(text, (char) => char.charCodeAt(0)));

// 예외가 나는지 본다. (2026-11-11 WB04-E283)
function throws(action, type = Error) {
    try {
        action();
        return false;
    } catch (error) {
        return error instanceof type;
    }
}

// 최소 확장/이름 교체/생성자의 원본 286행: 반환·전체 796BYTE·외부 효과 순서가 같다. 기대값은 fixture 에서만 읽는다. (2026-11-11 WB04-E283)
test('DataManager 최소 확장·이름 교체·생성자: 원본 10.78 286입력의 전체 796BYTE·반환 핸들·외부 효과 순서가 같다', () => {
    const NODES = 0x12100100;
    const BUFFERS = 0x12200100;
    const NODE_STRIDE = 76;
    const BUFFER_STRIDE = 80;
    const BUFFER_START = 332;
    const NAMES_START = 732;
    const rows = readRows('datamanager-resize-x86.tsv');
    assert.equal(rows.length, 286);
    // 각 행은 다른 행의 결과와 독립적인 같은 원본 입력을 적용한다.
    for (const row of rows) {
        assert.equal(row.length, 5);
        const input = row[0].split(',');
        assert.equal(input.length, 7);
        const bytes = hexBytes(row[1]);
        assert.equal(bytes.length, 796);
        const manager = bytes.subarray(8, 20);
        const kind = input[0][0];
        const position = Number(input[2]);
        const nameKind = Number(input[5]);
        const variant = Number(input[6]);
        const minimum = Number(BigInt.asIntN(32, BigInt(input[4])));
        let allocations = 0;
        const events = [];
        const original = NODES + position * NODE_STRIDE + 8;
        const temporary = NODES + 3 * NODE_STRIDE + 8;
        // 같은 raw 노드만 조회한다.
        const node = (handle) => {
            const offset = handle - NODES - 8;
            if (offset % NODE_STRIDE || offset < 0 || Math.floor(offset / NODE_STRIDE) >= 4) throw new RangeError('확장 원문 노드 범위');
            return bytes.subarray(28 + (offset / NODE_STRIDE) * NODE_STRIDE + 8, 28 + (offset / NODE_STRIDE) * NODE_STRIDE + 8 + 60);
        };
        // 반납한 관측용 바이트도 지우지 않는다.
        const buffer = (handle) => {
            const offset = handle - BUFFERS - 8;
            if (offset % BUFFER_STRIDE || offset < 0 || Math.floor(offset / BUFFER_STRIDE) >= 5) throw new RangeError('확장 원문 버퍼 범위');
            return bytes.subarray(BUFFER_START + (offset / BUFFER_STRIDE) * BUFFER_STRIDE + 8, BUFFER_START + (offset / BUFFER_STRIDE) * BUFFER_STRIDE + 8 + 64);
        };
        // 외부 효과 전 현재 796BYTE 를 기록한다.
        const event = (type, value) => events.push(`${type},${value},${bytesHex(bytes)}`);
        const hooks = { selection: { stream: node }, list: { lifetime: {} }, memory: {} };
        hooks.selection.compare = (left, right) => {
            event('Q', `${nameHex(left)}/${nameHex(right)}`);
            return compareDataManagerSectionNames(left, right);
        };
        hooks.memory.buffer = buffer;
        // 원본 allocator 의 명시 입력만 적용해 생성 뒤 data/end/alloc 재조회를 검사한다.
        hooks.memory.allocate = (size) => {
            event('A', String(size));
            const pointer = BUFFERS + (3 + allocations) * BUFFER_STRIDE + 8;
            if (variant === 1 && kind === 'G' && !allocations) {
                put(node(original), 0, BUFFERS + ((position + 1) % 3) * BUFFER_STRIDE + 8);
                put(node(original), 8, 2);
                put(manager, 8, 0xdead1234);
            }
            if (variant === 3 && !allocations) put(node(temporary), 12, size + 1);
            allocations += 1;
            return pointer;
        };
        hooks.memory.release = (pointer) => {
            event('F', String(pointer));
            if (variant === 2) put(manager, 8, 0xdead5678);
        };
        hooks.memory.assertion = () => { throw new Error('정상 확장 원문에 없는 MemStream 진단'); };
        hooks.list.lifetime.stream = node;
        hooks.list.lifetime.releaseBuffer = hooks.memory.release;
        hooks.list.lifetime.releaseStream = (handle) => {
            event('N', String(handle));
            if (variant === 2) put(manager, 8, 0xdead5678);
        };
        hooks.list.compact = (handle) => compactMemStream(node(handle), hooks.memory);
        hooks.list.missingPredecessor = () => { throw new Error('정상 확장 원문에 없는 링크 진단'); };
        hooks.allocateStream = (size) => {
            event('S', String(size));
            assert.equal(size, 60);
            return temporary;
        };
        hooks.missingSource = () => { throw new Error('정상 확장 원문에 없는 원문 누락'); };
        const desired = nameAt(bytes, NAMES_START);
        const prior = nameAt(bytes, NAMES_START + 32);
        let result;
        if (kind === 'G') result = ensureDataManagerStreamSize(manager, desired, minimum, hooks);
        else if (kind === 'R') result = renameDataManagerStream(manager, desired, prior, hooks);
        else result = constructDataManagerStream(temporary, nameKind === 2 ? null : (nameKind === 1 ? prior : desired), minimum, hooks);
        assert.equal(result, Number(row[3]), `반환 ${row[0]}`);
        assert.equal(bytesHex(bytes), row[2], `메모리 ${row[0]}`);
        assert.equal(events.length === 0 ? '-' : events.join(';'), row[4], `효과 ${row[0]}`);
    }
});

// 섹션 생성(N)·복제(C)·대입(M)의 원본 220행: 매 경계 직전 전체 17380BYTE 와 최종 메모리·반환이 같다. (2026-11-11 WB04-E283)
test('DataManager 섹션 생성/복제/대입: 원본 10.78 220입력의 전체 17380BYTE·반환 핸들·모든 경계 직전 상태가 같다', () => {
    const NODES = 0x12100100;
    const BUFFERS = 0x12200100;
    const LARGE = 0x12210000;
    const NODE_START = 56;
    const NODE_STRIDE = 76;
    const BUFFER_START = 436;
    const BUFFER_STRIDE = 80;
    const LARGE_START = 916;
    const NAMES_START = 17316;
    const rows = readRows('datamanager-create-x86.tsv');
    assert.equal(rows.length, 220);
    // 각 행의 입력/기대 바이트는 원본 PE 의 최초 관측만 사용한다.
    for (const row of rows) {
        assert.equal(row.length, 5);
        const inputs = row[0].split(',');
        assert.equal(inputs.length, 6);
        const bytes = createBytes(row[1]);
        assert.equal(bytes.length, 17380);
        const manager = bytes.subarray(8, 20);
        const sourceManager = bytes.subarray(36, 48);
        const kind = inputs[0][0];
        const variant = Number(inputs[5]);
        const original = NODES + 3 * NODE_STRIDE + 8;
        const fresh = NODES + 4 * NODE_STRIDE + 8;
        let smallAllocations = 0;
        let eventIndex = 0;
        const events = row[4] === '-' ? [] : row[4].split(';');
        // 같은 실제 노드 60BYTE 다.
        const node = (handle) => {
            const offset = handle - NODES - 8;
            if (offset % NODE_STRIDE || offset < 0 || Math.floor(offset / NODE_STRIDE) >= 5) throw new RangeError('생성 원문 노드 범위');
            return bytes.subarray(NODE_START + (offset / NODE_STRIDE) * NODE_STRIDE + 8, NODE_START + (offset / NODE_STRIDE) * NODE_STRIDE + 8 + 60);
        };
        // 원문 data 의 +3BYTE 변이도 같은 할당 내부에서 읽는다.
        const buffer = (pointer) => {
            if (pointer >= LARGE + 8 && pointer < LARGE + 8 + 0x4000) {
                const offset = pointer - LARGE - 8;
                return bytes.subarray(LARGE_START + 8 + offset, LARGE_START + 8 + 0x4000);
            }
            const offset = pointer - BUFFERS - 8;
            const index = Math.floor(offset / BUFFER_STRIDE);
            const inside = offset % BUFFER_STRIDE;
            if (offset < 0 || index >= 6 || inside >= 64) throw new RangeError('생성 원문 실제 버퍼 범위');
            return bytes.subarray(BUFFER_START + index * BUFFER_STRIDE + 8 + inside, BUFFER_START + index * BUFFER_STRIDE + 8 + 64);
        };
        // 제품의 현 메모리를 최초 경계 직전의 전체 원문과 즉시 비교한다.
        const event = (type, value) => {
            assert.ok(eventIndex < events.length, `생성 원문에 없는 경계 효과 ${type},${value}`);
            const expected = events[eventIndex].split(',');
            eventIndex += 1;
            assert.equal(expected.length, 3);
            assert.equal(expected[0], type);
            assert.equal(expected[1], value);
            assert.ok(Buffer.from(bytes).equals(createBytes(expected[2])), `경계 ${type},${value}: 전체 메모리 불일치`);
        };
        const hooks = { selection: { stream: node }, list: { lifetime: {} }, memory: {} };
        hooks.selection.compare = (left, right) => {
            event('Q', `${nameHex(left)}/${nameHex(right)}`);
            return compareDataManagerSectionNames(left, right);
        };
        hooks.memory.buffer = buffer;
        // 할당 경계에서 원문을 바꿔 대입 부모의 data/end 재조회를 검사한다.
        hooks.memory.allocate = (size) => {
            event('A', String(size));
            const pointer = size === 0x4000 ? LARGE + 8 : BUFFERS + (4 + smallAllocations++) * BUFFER_STRIDE + 8;
            if ((variant === 1 && size === 0x4000) || (variant === 3 && kind === 'M')) {
                put(node(original), 0, BUFFERS + 2 * BUFFER_STRIDE + 11);
                put(node(original), 8, 2);
            }
            return pointer;
        };
        // 반납 뒤 current/원문 변이는 본문의 후속 쓰기와 구별한다.
        const release = (type, pointer) => {
            event(type, String(pointer));
            if (variant === 2) {
                put(manager, 8, 0xdead5678);
                put(node(original), 0, BUFFERS + BUFFER_STRIDE + 8);
                put(node(original), 8, 3);
            }
        };
        hooks.memory.release = (pointer) => release('F', pointer);
        hooks.memory.assertion = () => { throw new Error('정상 생성 원문에 없는 MemStream 진단'); };
        hooks.list.lifetime.stream = node;
        hooks.list.lifetime.releaseBuffer = hooks.memory.release;
        hooks.list.lifetime.releaseStream = (handle) => release('N', handle);
        hooks.list.compact = (handle) => compactMemStream(node(handle), hooks.memory);
        hooks.list.missingPredecessor = () => { throw new Error('정상 생성 원문에 없는 선행 노드 진단'); };
        hooks.allocateStream = (size) => {
            event('S', String(size));
            assert.equal(size, 60);
            return fresh;
        };
        hooks.missingSource = () => { throw new Error('정상 복제 원문에 없는 원문 누락'); };
        const name = nameAt(bytes, NAMES_START);
        let result = 0;
        if (kind === 'N') result = createDataManagerStream(manager, name, hooks);
        else if (kind === 'C') result = cloneDataManagerStream(manager, sourceManager, name, hooks);
        else copyMemStream(node(fresh), node(original), hooks.memory);
        if (kind !== 'M') assert.equal(result, Number(row[3]), `반환 ${row[0]}`);
        assert.ok(Buffer.from(bytes).equals(createBytes(row[2])), `최종 메모리 ${row[0]}`);
        assert.equal(eventIndex, events.length, `경계 효과 수 ${row[0]}`);
    }
});

// 빈 manager 의 new60 실패는 원본처럼 NULL 을 추가하고 현재 0 을 반환하며 임의 노드를 만들지 않는다(cpppj DataManagerCreate_EmptyNullAllocationAndRequiredEffects). (2026-11-11 WB04-E283)
test('DataManager 섹션 생성: 빈 목록의 NULL 노드 할당은 0 을 추가·반환하고 필요한 장치가 없으면 거부한다', () => {
    const manager = new Uint8Array(12);
    let requests = 0;
    const hooks = { allocateStream: (size) => { requests += 1; assert.equal(size, 60); return 0; } };
    assert.equal(createDataManagerStream(manager, 'State', hooks), 0);
    assert.equal(requests, 1);
    assert.ok(manager.every((byte) => byte === 0));
    assert.ok(throws(() => createDataManagerStream(manager, 'State', {})));
});

// 자기 대입은 free 뒤 같은 첫 16BYTE 가 비워져 길이 0 으로 재할당되는 원본의 결과다(cpppj MemStreamCopy_SelfAssignmentResetsBeforeReadingLength). (2026-11-11 WB04-E283)
test('MemStream 대입: 자기 대입은 반납 뒤 비워진 길이 0 으로 재할당하고 버퍼를 조회하지 않는다', () => {
    const node = new Uint8Array(60);
    node.fill(0xa7, 16);
    put(node, 0, 1);
    put(node, 4, 4);
    put(node, 8, 7);
    put(node, 12, 8);
    let releases = 0;
    let allocations = 0;
    const hooks = {
        release: (pointer) => {
            releases += 1;
            assert.equal(pointer, 1);
            assert.equal(get(node, 4), 4);
            assert.equal(get(node, 8), 7);
        },
        allocate: (size) => {
            allocations += 1;
            assert.equal(size, 0);
            assert.deepEqual([get(node, 0), get(node, 4), get(node, 8), get(node, 12)], [0, 0, 0, 0]);
            return 2;
        },
        buffer: () => { throw new Error('0길이 자기 대입에서 버퍼 조회 금지'); },
    };
    copyMemStream(node, node, hooks);
    assert.deepEqual([releases, allocations, get(node, 0), get(node, 4), get(node, 8), get(node, 12)], [1, 1, 2, 0, 0, 0]);
    assert.ok(node.subarray(16).every((byte) => byte === 0xa7));
});

// 계약: fixture 가 구분하지 못하는 경계 — 이름 40BYTE·null 이름·크기 0 은 1·signed 최소·임시 이름 상수·누락 장치 거부. (2026-11-11 WB04-E283)
test('DataManager 임시 stream·확장 계약: 이름 범위·null 이름·크기 하한·signed 최소·누락 장치 거부', () => {
    // 한 노드(60BYTE)와 그 버퍼만 가진 최소 장면이다. 할당은 바이트 배열 한 개를 돌려준다.
    const scene = () => {
        const nodes = new Map();
        const buffers = new Map();
        let nextBuffer = 100;
        const record = { allocated: [] };
        const hooks = {
            selection: { stream: (handle) => nodes.get(handle) },
            list: { lifetime: { stream: (handle) => nodes.get(handle), releaseBuffer: () => {}, releaseStream: () => {} }, compact: () => {}, missingPredecessor: () => { throw new Error('링크 진단'); } },
            memory: {
                allocate: (size) => { record.allocated.push(size); buffers.set(nextBuffer, new Uint8Array(size)); return nextBuffer++; },
                release: () => {}, buffer: (pointer) => buffers.get(pointer), assertion: () => { throw new Error('MemStream 진단'); },
            },
            allocateStream: () => 0,
        };
        return { nodes, buffers, record, hooks };
    };
    // 이름 39글자는 쓰고 40글자는 거부한다(이름 영역 40BYTE 에 NUL 이 들어가야 한다).
    const a = scene();
    a.nodes.set(1, new Uint8Array(60).fill(0x99));
    assert.equal(constructDataManagerStream(1, 'x'.repeat(39), 8, a.hooks), 1);
    const node = a.nodes.get(1);
    assert.equal(node[16 + 39], 0);
    assert.equal(get(node, 0x38), 0);
    assert.equal(get(node, 12), 8);
    assert.ok(throws(() => constructDataManagerStream(1, 'x'.repeat(40), 8, a.hooks), RangeError));
    // 이름의 첫 NUL 까지만 쓴다. null 이면 첫 이름 BYTE 만 0 이고 나머지 이름 BYTE 는 그대로다.
    node.fill(0x77, 16, 56);
    constructDataManagerStream(1, 'ab\0cd', 8, a.hooks);
    assert.deepEqual([...node.subarray(16, 20)], [0x61, 0x62, 0, 0x77]);
    node.fill(0x77, 16, 56);
    constructDataManagerStream(1, null, 8, a.hooks);
    assert.deepEqual([...node.subarray(16, 18)], [0, 0x77]);
    // 크기 0 이하는 1 로 할당한다(음수 포함).
    a.record.allocated.length = 0;
    for (const size of [0, -5, 1, 28]) constructDataManagerStream(1, 'n', size, a.hooks);
    assert.deepEqual(a.record.allocated, [1, 1, 1, 28]);
    // 노드 조회 장치가 없거나 NULL 핸들이면 거부한다.
    assert.ok(throws(() => constructDataManagerStream(0, 'n', 8, a.hooks)));
    assert.ok(throws(() => constructDataManagerStream(1, 'n', 8, { memory: a.hooks.memory })));
    // 임시 이름은 원문 상수이다.
    assert.equal(DATA_MANAGER_TEMPORARY_NAME, 'T$e$m$p');
    // 최소 확장: end 가 이미 최소 이상(signed)이면 할당 없이 current 를 돌려준다. 음수 최소는 항상 충분하다.
    const b = scene();
    const section = new Uint8Array(60);
    section.set([0x53, 0x74, 0x61, 0x74, 0x65], 16);
    put(section, 8, 32);
    b.nodes.set(5, section);
    const manager = new Uint8Array(12);
    put(manager, 4, 5);
    for (const minimum of [-1, 0, 28, 32]) {
        assert.equal(ensureDataManagerStreamSize(manager, 'State', minimum, b.hooks), 5, `최소 ${minimum}`);
        assert.equal(b.record.allocated.length, 0);
    }
    // 최소가 end 보다 크면 임시 stream 할당이 필요한데 노드 할당이 NULL 이면 노드를 만들지 않고 거부한다.
    assert.ok(throws(() => ensureDataManagerStreamSize(manager, 'State', 33, b.hooks)));
    // 원문 누락 진단 장치가 없으면 거부하고, 있으면 진단 뒤 NULL 노드로 계속하다 거부한다.
    const empty = new Uint8Array(12);
    assert.ok(throws(() => ensureDataManagerStreamSize(empty, 'State', 1, b.hooks)));
    let missing = 0;
    assert.ok(throws(() => ensureDataManagerStreamSize(empty, 'State', 1, { ...b.hooks, missingSource: () => { missing += 1; } })));
    assert.equal(missing, 1);
    assert.ok(throws(() => cloneDataManagerStream(new Uint8Array(12), empty, 'State', b.hooks)));
    // 이름 교체: 대상이 이미 있으면 제거한 뒤 원래 이름을 찾는다(원래 이름이 없으면 NULL 노드라 거부).
    assert.ok(throws(() => renameDataManagerStream(empty, 'New', 'Old', b.hooks)));
});

// 계약: 최소 확장은 원문 길이 0(데이터 포인터 0)에서도 버퍼 조회 없이 끝나고, 복사 뒤 남은 구간을 0 으로 채우며, 용량으로 올리는 end 비교는 signed 이다. (2026-11-11 WB04-E283)
test('DataManager 최소 확장 계약: 원문 길이 0 허용·남은 구간 0 채움·end 올림의 signed 비교', () => {
    // 'State' 섹션 하나(노드 5)가 있는 manager 와 임시 노드(9)를 돌려주는 최소 장면이다. 버퍼 조회 장치는 옵션으로 가로챈다.
    const build = (sourceBytes, onBuffer) => {
        const nodes = new Map();
        const buffers = new Map();
        const section = new Uint8Array(60);
        section.set([0x53, 0x74, 0x61, 0x74, 0x65], 16);
        buffers.set(50, Uint8Array.from(sourceBytes));
        put(section, 0, sourceBytes.length ? 50 : 0);
        put(section, 8, sourceBytes.length);
        put(section, 12, sourceBytes.length);
        nodes.set(5, section);
        nodes.set(9, new Uint8Array(60));
        const manager = new Uint8Array(12);
        put(manager, 4, 5);
        let next = 100;
        const hooks = {
            selection: { stream: (handle) => nodes.get(handle) },
            list: { lifetime: { stream: (handle) => nodes.get(handle), releaseBuffer: () => {}, releaseStream: () => {} }, compact: () => {}, missingPredecessor: () => { throw new Error('링크 진단'); } },
            memory: {
                allocate: (size) => { buffers.set(next, new Uint8Array(size)); return next++; },
                release: () => {},
                buffer: (pointer) => { onBuffer?.(pointer, nodes.get(9), buffers); return buffers.get(pointer); },
                assertion: () => { throw new Error('MemStream 진단'); },
            },
            allocateStream: () => 9,
        };
        return { manager, nodes, buffers, hooks };
    };
    // 원문 길이 0: 데이터 포인터가 0 이어도 원문 버퍼 조회 없이 확장이 끝난다(길이 0 은 조회하지 않는다).
    const zero = build([], (pointer) => assert.notEqual(pointer, 0, '길이 0 원문의 NULL 버퍼 조회 금지'));
    assert.doesNotThrow(() => ensureDataManagerStreamSize(zero.manager, 'State', 1, zero.hooks));
    assert.equal(get(zero.nodes.get(9), 12), 1);
    // 복사 뒤 남은 구간: 꼬리 구간을 조회하는 시점(복사로 pos 가 원문 길이가 된 뒤)에 꼬리를 더럽히고 end 를 0xffffffff 로 바꾸면,
    // 구현은 꼬리를 0 으로 채우고 pos/end 를 용량(28)으로 올린다(signed 로 −1 < 28 이라 end 도 올림).
    let dirtied = 0;
    const tail = build([1, 2, 3, 4], (pointer, target, buffers) => {
        if (pointer === 100 && get(target, 4) === 4) {
            buffers.get(100).fill(0xee, 4);
            put(target, 8, 0xffffffff);
            dirtied += 1;
        }
    });
    ensureDataManagerStreamSize(tail.manager, 'State', 28, tail.hooks);
    assert.equal(dirtied, 1);
    const target = tail.nodes.get(9);
    assert.deepEqual([get(target, 4), get(target, 8), get(target, 12)], [28, 28, 28]);
    assert.deepEqual([...tail.buffers.get(100).subarray(0, 8)], [1, 2, 3, 4, 0, 0, 0, 0]);
    assert.ok(tail.buffers.get(100).subarray(4).every((byte) => byte === 0));
    // 확장한 stream 이 'State' 이름을 얻고 기존 노드를 대체했다.
    assert.equal(String.fromCharCode(...target.subarray(16, 21)), 'State');
});
