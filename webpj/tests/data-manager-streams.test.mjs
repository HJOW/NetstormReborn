// 10.78 MemStream·DataManager 소멸·연결 목록(cpppj RawMemStream·RawDataManagerLifetime·RawDataManagerList, 원본 0044cc20..0044d0f0)의 웹 대응을 검사한다(4-B, WB04-E274).
//  - 고정 관측 `datamanager-streams-x86.tsv` 404행: 전체 716 바이트(노드 4개·버퍼 4개·원문) 위에서 할당/쓰기/압축/추가/제거를 실행하고, 매 외부 효과(할당 A·반납 F·진단 D·노드 반납 N) 직전의
//    전체 메모리와 실행 뒤 전체 메모리를 대조한다. variant 는 원본 관측의 명시 변이(할당 중 필드 변경 · 반납 뒤 manager 변경 · 진단 뒤 pos 변경 · 겹치는 원문)이다.
//  - 고정 관측 `datamanager-lifetime-x86.tsv` 392행: 전체 412 바이트(manager 16 + 노드 5개) 위에서 MemStream 소멸과 DataManager 소멸의 free 순서·외부 효과 뒤 head 재조회를 대조한다.
//  - 손 시험(cpppj DataManagerStreamsTests·DataManagerLifetimeTests): 실제 폭 부족 거부, 0 길이·빈 목록은 외부 장치를 요구하지 않음, 진단 뒤 수리된 같은 선행 노드를 다시 읽음.
// Template 저장소 소비자(CampaignTemplates_*)는 다음 항목(Template)에서 다룬다. 실제 게임·브라우저 실행이 아니다. (2026-11-10 WB04-E274)
import test from 'node:test';
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import { dirname, resolve } from 'node:path';
import { MEM_STREAM_ASSERTION, allocateMemStream, compactMemStream, copyMemStream, writeMemStream } from '../www/core/raw-mem-stream.js';
import { DATA_MANAGER_BASE_VTABLE, destroyDataManager, destroyMemStream } from '../www/core/raw-data-manager-lifetime.js';
import { appendDataManagerStream, removeDataManagerStream } from '../www/core/raw-data-manager-list.js';

// 저장소 안의 고정 관측 폴더이다. (2026-11-10 WB04-E274)
const FIXTURES = resolve(dirname(fileURLToPath(import.meta.url)), '../../cpppj/tests/fixtures');

// 주석(#)과 빈 줄을 뺀 고정 관측 행의 칸 배열이다. (2026-11-10 WB04-E274)
function readRows(name) {
    return readFileSync(`${FIXTURES}/${name}`, 'utf8').split(/\r?\n/).filter((line) => line !== '' && line[0] !== '#').map((line) => line.split('\t'));
}

// 소문자 16진수 문자열 ↔ 바이트 배열. (2026-11-10 WB04-E274)
function hexBytes(hex) {
    return Uint8Array.from(hex.match(/../g) ?? [], (pair) => Number.parseInt(pair, 16));
}
function bytesHex(bytes) {
    return Array.from(bytes, (byte) => byte.toString(16).padStart(2, '0')).join('');
}

// 비정렬 DWORD 읽기/쓰기. (2026-11-10 WB04-E274)
function get(bytes, at) {
    return (bytes[at] | (bytes[at + 1] << 8) | (bytes[at + 2] << 16) | (bytes[at + 3] << 24)) >>> 0;
}
function put(bytes, at, value) {
    for (let index = 0; index < 4; index += 1) bytes[at + index] = (value >>> (index * 8)) & 255;
}

// 스트림 관측의 불투명 주소와 전체 716 바이트 안의 위치이다. 노드/버퍼 사이 8 바이트씩이 양쪽 경계이다. (2026-11-10 WB04-E274)
const STREAM = { nodes: 0x12100100, buffers: 0x12200100, nodeStride: 76, bufferStride: 80, bufferStart: 332, sourceStart: 652, nodeStart: 28 };

test('DataManager 스트림: 할당·쓰기·압축·추가·제거 404행의 전체 메모리와 외부 효과 순서가 원본 관측과 같다', () => {
    const rows = readRows('datamanager-streams-x86.tsv');
    assert.equal(rows.length, 404);
    const kinds = new Map();
    for (const row of rows) {
        assert.equal(row.length, 4);
        const input = row[0].split(',');
        const bytes = hexBytes(row[1]);
        assert.equal(bytes.length, 716);
        const manager = bytes.subarray(8, 20);
        const kind = input[0][0];
        const count = Number(input[2]);
        const select = Number(input[3]);
        const length = Number(input[8]);
        const variant = Number(input[9]);
        let events = '';
        const selected = STREAM.nodes + (kind === 'P' && count ? count - 1 : select) * STREAM.nodeStride + 8;
        // 불투명 노드 핸들의 실제 60 바이트이다.
        const node = (handle) => {
            const offset = handle - STREAM.nodes - 8;
            if (offset < 0 || offset % STREAM.nodeStride || offset / STREAM.nodeStride >= 4) throw new RangeError('원문 실제 노드 범위');
            const start = STREAM.nodeStart + (offset / STREAM.nodeStride) * STREAM.nodeStride + 8;
            return bytes.subarray(start, start + 60);
        };
        // 불투명 버퍼 핸들의 실제 64 바이트이다. 반납 뒤에도 관측용 바이트는 지우지 않는다.
        const buffer = (handle) => {
            const offset = handle - STREAM.buffers - 8;
            if (offset < 0 || offset % STREAM.bufferStride || offset / STREAM.bufferStride >= 4) throw new RangeError('원문 실제 버퍼 범위');
            const start = STREAM.bufferStart + (offset / STREAM.bufferStride) * STREAM.bufferStride + 8;
            return bytes.subarray(start, start + 64);
        };
        // 외부 효과 직전 전체 716 바이트를 기록한다.
        const event = (type, value) => {
            if (events) events += ';';
            events += `${type},${value},${bytesHex(bytes)}`;
        };
        const memory = {
            buffer,
            allocate: (size) => {
                event('A', size);
                if (variant === 1) {
                    put(node(selected), 12, Math.floor(size / 2));
                    put(node(selected), 4, 0x87654321);
                }
                return STREAM.buffers + 3 * STREAM.bufferStride + 8;
            },
            release: (handle) => {
                event('F', handle);
                if (variant === 2) {
                    put(node(selected), 4, 0x87654321);
                    put(node(selected), 8, 0x1234);
                    put(manager, 8, 0x456789ab);
                }
            },
            assertion: (reason) => {
                event('D', reason === MEM_STREAM_ASSERTION.existingData ? 0x14 : 0x24);
                if (kind === 'W' && variant === 1) put(node(selected), 4, 5);
            },
        };
        const list = {
            lifetime: { stream: node, releaseBuffer: memory.release, releaseStream: (handle) => event('N', handle) },
            compact: (handle) => compactMemStream(node(handle), memory),
            missingPredecessor: () => { throw new Error('정상 원문404행에 없는 링크 진단'); },
        };
        switch (kind) {
        case 'A': allocateMemStream(node(selected), length, memory); break;
        case 'W': writeMemStream(node(selected), variant === 3 ? buffer(STREAM.buffers + 8).subarray(1) : bytes.subarray(STREAM.sourceStart, STREAM.sourceStart + 64), length, memory); break;
        case 'C': compactMemStream(node(selected), memory); break;
        case 'P': appendDataManagerStream(manager, select ? STREAM.nodes + 3 * STREAM.nodeStride + 8 : 0, list); break;
        case 'R': removeDataManagerStream(manager, selected, list); break;
        default: throw new Error('원문 입력의 동작 종류 오류');
        }
        assert.equal(bytesHex(bytes), row[2], row[0]);
        assert.equal(events || '-', row[3], row[0]);
        kinds.set(kind, (kinds.get(kind) ?? 0) + 1);
    }
    // 동작별 행 수이다: 할당 48·쓰기 168·압축 48·추가 108·제거 32.
    assert.deepEqual([...kinds].sort(), [['A', 48], ['C', 48], ['P', 108], ['R', 32], ['W', 168]]);
});

// 소멸 관측의 불투명 주소와 전체 412 바이트 안의 위치이다. (2026-11-10 WB04-E274)
const LIFETIME = { nodes: 0x12101000, buffers: 0x12200000, stride: 76, count: 5 };

test('DataManager 소멸: MemStream·DataManager 392행의 전체 메모리와 free 순서·외부 효과 뒤 head 재조회가 원본 관측과 같다', () => {
    const rows = readRows('datamanager-lifetime-x86.tsv');
    assert.equal(rows.length, 392);
    for (const row of rows) {
        assert.equal(row.length, 4);
        const input = row[0].split(',');
        const bytes = hexBytes(row[1]);
        assert.equal(bytes.length, 32 + LIFETIME.stride * LIFETIME.count);
        const manager = bytes.subarray(8, 24);
        const variant = Number(input[5]);
        let trace = '';
        // 현재 불투명 노드만 같은 60 바이트로 해석한다.
        const stream = (handle) => {
            const index = (handle - LIFETIME.nodes - 8) / LIFETIME.stride;
            assert.ok(Number.isInteger(index) && index >= 0 && index < LIFETIME.count, `노드 ${handle}`);
            const start = 32 + index * LIFETIME.stride + 8;
            return bytes.subarray(start, start + 60);
        };
        // 외부 효과 전에 전체 메모리를 기록하고 관측한 변이만 적용한다.
        const release = (kind, handle) => {
            if (trace) trace += ';';
            trace += `${kind},${handle},${bytesHex(bytes)}`;
            const index = kind === 'B' ? Math.floor((handle - LIFETIME.buffers) / 0x1000) : (handle - LIFETIME.nodes - 8) / LIFETIME.stride;
            const target = bytes.subarray(32 + index * LIFETIME.stride + 8, 32 + index * LIFETIME.stride + 68);
            if (variant === 1 && kind === 'B') {
                put(target, 0, 0xabcdef01);
                put(target, 4, 0x87654321);
                put(target, 0x38, 0x98765432);
            }
            if (variant === 2 && kind === 'N') {
                put(manager, 4, 0);
                put(manager, 8, 0x87654321);
            }
            if (variant === 3 && kind === 'B') {
                const following = get(manager, 4);
                if (following) put(stream(following), 0x38, 0);
            }
        };
        const hooks = { stream, releaseBuffer: (handle) => release('B', handle), releaseStream: (handle) => release('N', handle) };
        if (input[0] === 'S') destroyMemStream(stream(LIFETIME.nodes + 8), hooks.releaseBuffer);
        else destroyDataManager(manager, hooks);
        assert.equal(bytesHex(bytes), row[2], row[0]);
        assert.equal(trace || '-', row[3], row[0]);
    }
});

test('MemStream·DataManager: 실제 폭이 부족하면 쓰기 전에 거부하고 0 길이와 빈 목록은 외부 장치를 요구하지 않는다', () => {
    const storage = new Uint8Array(24).fill(0xa5);
    const before = Uint8Array.from(storage);
    assert.throws(() => allocateMemStream(storage.subarray(0, 15), 0, {}), RangeError);
    assert.deepEqual(storage, before);
    // 길이 0 할당: 버퍼 조회 없이 16 바이트만 0 으로 쓰고 뒤 바이트는 남긴다.
    const stream = storage.subarray(0, 16);
    put(stream, 0, 0);
    allocateMemStream(stream, 0, { allocate: (size) => { assert.equal(size, 0); return 0; } });
    assert.ok(stream.every((byte) => byte === 0) && storage[16] === 0xa5);
    writeMemStream(stream, new Uint8Array(0), 0, {});
    compactMemStream(stream, {});
    assert.deepEqual([get(stream, 4), get(stream, 8)], [0, 0]);
    // 빈 목록 추가: 새 노드 조회·압축 없이 head/current 만 쓰고 vptr 는 그대로이다.
    const manager = new Uint8Array(12);
    put(manager, 0, 0x87654321);
    appendDataManagerStream(manager, 0x1234, {});
    assert.deepEqual([get(manager, 0), get(manager, 4), get(manager, 8)], [0x87654321, 0x1234, 0x1234]);
    // head 가 비면 제거는 핸들 0 노드 조회를 거부한다.
    put(manager, 4, 0);
    assert.throws(() => removeDataManagerStream(manager, 0, {}), /노드 조회 미연결/);
    // 소멸: 15·11 바이트는 쓰기 전 거부, 빈 버퍼와 빈 목록은 장치 없이 반환하고 vptr 를 쓴다.
    const small = new Uint8Array(16).fill(0xa5);
    const smallBefore = Uint8Array.from(small);
    assert.throws(() => destroyMemStream(small.subarray(0, 15), undefined), RangeError);
    assert.throws(() => destroyDataManager(small.subarray(0, 11), {}), RangeError);
    assert.deepEqual(small, smallBefore);
    put(small, 0, 0);
    destroyMemStream(small, undefined);
    assert.ok(small.every((byte) => byte === 0));
    put(small, 8, 0x87654321);
    put(small, 12, 0x12345678);
    destroyDataManager(small, {});
    assert.deepEqual([get(small, 0), get(small, 4), get(small, 8), get(small, 12)], [DATA_MANAGER_BASE_VTABLE, 0, 0x87654321, 0x12345678]);
});

// 쓰기의 용량·end 비교는 signed 이다: 고정 관측은 양수 범위뿐이라 음수 pos/end 를 손으로 확인한다(변형 확인으로 추가). 대입은 새 버퍼에 DWORD+나머지 바이트를 복사하고 end 를 원문 end 로 남긴다.
test('MemStream: 쓰기의 용량·end 비교는 signed 이고 대입은 pos 0·원문 end 로 끝난다', () => {
    // alloc 0, pos -16, length 8: signed 합 -8 은 alloc 0 보다 작지 않아 진단이 없다(unsigned 면 진단). 대상 위치가 버퍼 밖이라 거부된다.
    const stream = new Uint8Array(16);
    put(stream, 0, 1);
    put(stream, 4, 0xfffffff0);
    put(stream, 12, 0);
    let diagnosed = 0;
    assert.throws(() => writeMemStream(stream, new Uint8Array(8), 8, { buffer: () => new Uint8Array(64), assertion: () => { diagnosed += 1; } }), RangeError);
    assert.equal(diagnosed, 0);
    // end -16 은 새 pos 8 보다 작아 end 가 8 로 올라간다(unsigned 면 그대로).
    const grow = new Uint8Array(16);
    const buffer = new Uint8Array(16);
    put(grow, 0, 1);
    put(grow, 8, 0xfffffff0);
    put(grow, 12, 16);
    writeMemStream(grow, Uint8Array.of(1, 2, 3, 4, 5, 6, 7, 8), 8, { buffer: () => buffer });
    assert.deepEqual([get(grow, 4), get(grow, 8)], [8, 8]);
    assert.deepEqual([...buffer.subarray(0, 8)], [1, 2, 3, 4, 5, 6, 7, 8]);
    // 대입: 기존 버퍼(핸들 5)를 반납하고 원문 end(10)로 새 버퍼(핸들 7)를 할당해 DWORD 두 개와 나머지 2 바이트를 복사한다. pos 는 0, alloc/end 는 10 이다.
    const memory = new Map([[3, Uint8Array.from([9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 11, 12])], [7, new Uint8Array(12).fill(0xcc)]]);
    const target = new Uint8Array(16);
    const source = new Uint8Array(16);
    put(target, 0, 5);
    put(target, 4, 3);
    put(target, 8, 4);
    put(target, 12, 6);
    put(source, 0, 3);
    put(source, 4, 99);
    put(source, 8, 10);
    put(source, 12, 12);
    const log = [];
    copyMemStream(target, source, {
        buffer: (handle) => memory.get(handle),
        release: (handle) => log.push(`F${handle}`),
        allocate: (size) => { log.push(`A${size}`); return 7; },
        assertion: () => log.push('D'),
    });
    assert.deepEqual(log, ['F5', 'A10']);
    assert.deepEqual([get(target, 0), get(target, 4), get(target, 8), get(target, 12)], [7, 0, 10, 10]);
    assert.deepEqual([...memory.get(7)], [9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 0xcc, 0xcc]);
});

test('연결 목록 제거: 선행 링크 진단 뒤 수리된 같은 선행 노드를 다시 읽는다', () => {
    const manager = new Uint8Array(12);
    const nodes = [new Uint8Array(60), new Uint8Array(60), new Uint8Array(60)];
    put(manager, 4, 1);
    put(manager, 8, 9);
    put(nodes[1], 4, 17);
    put(nodes[1], 8, 19);
    let released = 0;
    let diagnosed = 0;
    removeDataManagerStream(manager, 2, {
        lifetime: { stream: (handle) => nodes[handle - 1], releaseStream: (handle) => { released = handle; } },
        // 진단이 노드 1 의 next 를 2 로, 노드 2 의 next 를 3 으로 수리한다.
        missingPredecessor: () => {
            diagnosed += 1;
            put(nodes[0], 0x38, 2);
            put(nodes[1], 0x38, 3);
        },
    });
    assert.deepEqual([diagnosed, released, get(manager, 4), get(manager, 8)], [1, 2, 1, 9]);
    assert.deepEqual([get(nodes[0], 0x38), get(nodes[1], 0x38), get(nodes[1], 4), get(nodes[1], 8)], [3, 3, 0, 0]);
});
