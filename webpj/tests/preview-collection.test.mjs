// 커서 미리보기 수집기 render/preview-collection.js 를 원본 10.78 관측(previewcollection-x86.tsv originals 520행)과 cpppj `PreviewCollectionTests.cpp` 의 쌍 제한/부분 쓰기 계약으로 검사한다.
//  - 관측 한 행 = 실제 미리보기 기계어의 완성 입력 한 건이다: 상태 21값(잠금·조각 개수·표면/섬 비트·zOrder·부모 Gump·부모 깊이 반환·배치 가능·배치 좌표·받침 타입 A/B·SP offset 둘·시각·강조·카운트다운·SP 조회·로컬 소유자·첫 풀·첫 순열·SP 반환),
//    한도, 클립 전 영역 네 float, 조각 목록, 희소 spot, 완성 raw 슬롯(50바이트 전체). 기대는 반환 수·작업 칸 64개 전체 바이트·강조 시각·카운트다운·SP getter 진입 횟수·작업 배열 앞뒤 0xa5 보호 항목이며 기대 쌍/깊이는 계산하지 않는다.
//    풀과 spot 은 수집 전후 바이트가 같아야 하고 두 레이어의 토큰은 `object()` 가 같은 풀 SID 로 되돌린다. original1037·originalCD 행(각 520)은 웹 범위 밖(10.78 전용)이라 읽지 않는다.
//  - cpppj 시험 이식: `PreviewCollection_PairLimitAndPartialWrites`(soft 쌍 제한·실패 후보의 14바이트 부분 기록·실제 span 넘침 방지). 실제 커서 전역520행·SP 재진입/계약 시험도 함께 대조한다(WB04-E404).
//  - 웹 계약: 토큰 범위/풀 검증, `object()` 거부 조건, 훅 우선(`now`·SP offset·첫 풀/순열)과 state 독립 값, SP 저장소 훅 연결(초기화 전 거부), 공통 수집기 묶음 연결, spot 좌표의 float 상수와 64비트 절삭(비유한은 0).
// 실제 게임·브라우저 실행이 아니다. (2026-11-13 WB04-E386)
import { CursorPreviewBindingScene } from './cursor-preview-binding-support.mjs';
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { PreviewCollection, PreviewCollectionState, bindPreviewCollection, cursorPreviewSid, makePreviewSpHooks } from '../www/render/preview-collection.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-13 WB04-E386)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'previewcollection-x86.tsv'));
// 독립 원본의 raw 풀 토큰 기준과 작업 칸 수이다. (2026-11-13 WB04-E386)
const PREVIEW_POOL = 0x15002000;
const CAPACITY = 64;
const ENTRY = 16;
const hex = (bytes) => Buffer.from(bytes.buffer, bytes.byteOffset, bytes.byteLength).toString('hex');
const bitsFloat = (bits) => { const view = new DataView(new ArrayBuffer(4)); view.setUint32(0, bits >>> 0, true); return view.getFloat32(0, true); };

// 한 관측 행을 재생하고 불일치 설명 목록을 돌려준다. (2026-11-13 WB04-E386)
function replay(pool, row, shared = false) {
    const fields = row[2].split(',');
    const v = fields.slice(0, 21).map((field) => Number(field) >>> 0);
    const state = new PreviewCollectionState({
        lockDepth: v[0], pieceCount: v[1] | 0, type: 1, parentGump: v[4], legal: v[6], x: bitsFloat(v[7]), y: bitsFloat(v[8]), specialTypeA: v[9], specialTypeB: v[10],
        scrambledOffset: v[11], copiedOffset: v[12], now: v[13], highlight: v[14], countdown: v[15], querySp: v[16], localPlayer: v[17],
        firstSpPoolPresent: v[18] !== 0, firstSpIndex: (v[19] << 16) >> 16,
    });
    if (row[3] !== '-') state.pieces = row[3].split(',').map(Number);
    const spots = new Uint8Array(65536);
    if (row[4] !== '-') for (const input of row[4].split(';')) { const [index, value] = input.split(',').map(Number); spots[index] = value; }
    if (row[5] !== '-') {
        for (const slot of row[5].split(';')) {
            const [sid, bytes] = slot.split('=');
            const data = Uint8Array.from(Buffer.from(bytes, 'hex'));
            assert.equal(data.length, pool.layout.stride);
            pool.physical(Number(sid)).set(data);
        }
    }
    const types = [{ flags2: 0, zOrder: 0 }, { flags2: v[2], zOrder: v[3] | 0 }];
    // 풀 전체 저장소(모든 슬롯 연속 버퍼)의 수집 전 사본이다. 수집은 풀을 바꾸지 않는다.
    const whole = new Uint8Array(pool.physical(2).buffer);
    const before = Uint8Array.from(whole);
    const beforeSpots = Uint8Array.from(spots);
    const owners = [];
    const hooks = { gumpDepth: (token) => { assert.equal(token, v[4]); return v[5]; }, readSp: (owner) => { owners.push(owner); return v[20]; } };
    const collector = new PreviewCollection(pool, types, spots, state, hooks, PREVIEW_POOL);
    if (shared) { const actual = new CursorPreviewBindingScene(); actual.collection(state); collector.bindCursor(actual.source(pool)); state.lockDepth = 1; state.type = 0xffffffff; state.pieceCount = 0; state.pieces = []; }
    const guarded = new Uint8Array((CAPACITY + 2) * ENTRY).fill(0xa5);
    const work = guarded.subarray(ENTRY, ENTRY + CAPACITY * ENTRY);
    const limit = Number(fields[21]) | 0;
    const count = collector.collect({ left: bitsFloat(Number(fields[22])), top: bitsFloat(Number(fields[23])), right: bitsFloat(Number(fields[24])), bottom: bitsFloat(Number(fields[25])) }, work, limit);
    const guards = hex(guarded.subarray(0, ENTRY)) + hex(guarded.subarray((CAPACITY + 1) * ENTRY));
    const feedback = row[8].split(',').map(Number);
    const issues = [];
    const check = (name, actual, expected) => { if (actual !== expected) issues.push(`${name}: ${actual} != ${expected}`); };
    check('count', count, Number(row[6]));
    check('work', hex(work), row[7]);
    check('highlight', state.highlight >>> 0, feedback[0]);
    check('countdown', state.countdown >>> 0, feedback[1]);
    check('guards', guards, row[10]);
    const sp = /"0040ec50":(\d+)/.exec(row[9]);
    check('spReads', owners.length, sp ? Number(sp[1]) : 0);
    for (const owner of owners) check('owner', owner, v[17]);
    check('pool', Buffer.compare(before, whole), 0);
    check('spots', Buffer.compare(beforeSpots, spots), 0);
    // 두 레이어의 실제 raw 객체 토큰은 같은 풀 번호로 돌아간다.
    for (let i = 0; i < count; i += 1) {
        const token = new DataView(work.buffer, work.byteOffset + i * ENTRY, ENTRY).getUint32(8, true);
        check('object', collector.object(token), (token - PREVIEW_POOL) / pool.layout.stride);
    }
    return issues;
}

test('커서 미리보기 수집기: 원본 10.78 관측 520행의 부분 출력·원래 두 깊이·강조 상태·SP 진입·보호 항목이 모두 일치한다', { skip: rows === null }, () => {
    const originals = rows.filter((row) => row[0] === 'originals');
    assert.equal(originals.length, 520);
    const pool = new SidPool(24000, { server: false });
    for (const row of originals) {
        assert.deepEqual(replay(pool, row), [], `PreviewCollection ${row[1]}`);
    }
});

// 최소한의 커서 목록·타입·raw 슬롯 장면이다(cpppj PreviewScene). (2026-11-13 WB04-E386)
function scene() {
    const pool = new SidPool(24000, { server: false });
    const types = [{ flags2: 0, zOrder: 0 }, { flags2: 0, zOrder: 7 }];
    const spots = new Uint8Array(65536);
    const state = new PreviewCollectionState({ type: 1, pieceCount: 1, pieces: [5], legal: 1, specialTypeA: 82, specialTypeB: 94, x: 20, y: 25, now: 99, highlight: 77 });
    const place = (sid, type, x, y) => {
        const raw = pool.physical(sid);
        raw.fill(0);
        raw[10] = type;
        raw[11] = 255;
        put(raw, 14, floatToBits(x));
        put(raw, 18, floatToBits(y));
    };
    place(5, 1, 20, 25);
    return { pool, types, spots, state, place };
}
const entryField = (work, index, offset, width = 1) => (width === 1 ? work[index * ENTRY + offset] : new DataView(work.buffer, work.byteOffset).getUint32(index * ENTRY + offset, true));
const throwsOf = (action) => { try { action(); } catch { return true; } return false; };

test('커서 미리보기 수집기: soft 쌍 제한·실패 후보의 14바이트 부분 기록·실제 span 넘침 방지를 구분한다(PreviewCollection_PairLimitAndPartialWrites)', () => {
    const s = scene();
    const collector = new PreviewCollection(s.pool, s.types, s.spots, s.state);
    const work = new Uint8Array(ENTRY * 4).fill(0xa5);
    // 한도 1 이어도 두 칸이 있으면 한 쌍을 쓴다.
    assert.equal(collector.collect({}, work.subarray(0, ENTRY * 2), 1), 2);
    assert.deepEqual([entryField(work, 0, 15), entryField(work, 1, 15), entryField(work, 2, 14)], [0, 2, 0xa5]);
    // 칸이 한 개뿐이면 거부하고 첫 칸의 x/y/token/depth 14바이트만 남는다. 카운트다운은 줄지 않는다.
    s.state.countdown = 3;
    work.fill(0xa5);
    assert.ok(throwsOf(() => collector.collect({}, work.subarray(0, ENTRY), 1)));
    const view = new DataView(work.buffer);
    assert.deepEqual([view.getFloat32(0, true), view.getInt16(12, true), work[14], work[15], work[ENTRY + 14], s.state.countdown], [20, 7, 0xa5, 0xa5, 0xa5, 3]);
    // 받침 타입이고 배치 불가이면 건너뛰어 0 개이며 토큰만 남는다(SID 5 × 50).
    s.place(5, 82, 20, 25);
    s.state.legal = 0;
    work.fill(0xa5);
    assert.equal(collector.collect({}, work.subarray(0, ENTRY), 1), 0);
    assert.deepEqual([view.getUint32(8, true), work[14]], [250, 0xa5]);
    // 잠금이 걸렸거나 개수가 음수이면 빈 출력에서도 0 이다.
    s.state.lockDepth = 1;
    s.state.pieces = [65535];
    assert.equal(collector.collect({}, new Uint8Array(0), 100), 0);
    s.state.lockDepth = 0;
    s.state.pieceCount = -1;
    assert.equal(collector.collect({}, new Uint8Array(0), 100), 0);
});

test('커서 미리보기 수집기 계약: 생성 검증과 object() 거부, 훅이 state 의 독립 값보다 우선하고 SP 읽기는 조회가 켜져 있을 때만이다', () => {
    const s = scene();
    assert.throws(() => new PreviewCollection(s.pool, [], s.spots, s.state), /Preview collection tables/);
    assert.throws(() => new PreviewCollection(s.pool, s.types, new Uint8Array(65535), s.state), /Preview collection tables/);
    assert.throws(() => new PreviewCollection(s.pool, s.types, s.spots, s.state, {}, 0xffffffff), /token range/);
    const collector = new PreviewCollection(s.pool, s.types, s.spots, s.state, {}, 1000);
    assert.equal(collector.object(1000 + 5 * 50), 5);
    for (const bad of [999, 1000, 1000 + 7, 1000 + 24000 * 50, 0xffffffff]) assert.throws(() => collector.object(bad), RangeError, String(bad));
    // 피드백: offset 가 다르면 현재 시각, 같으면 그대로(훅 값이 state 값보다 우선).
    const sp = [];
    const hooks = { scrambledOffset: () => 5, copiedOffset: () => 5, now: () => 0x1234, querySp: () => 1, readSp: (owner) => { sp.push(owner); return floatToBits(7000.5); } };
    s.state.scrambledOffset = 1;
    s.state.copiedOffset = 2;
    s.state.countdown = 2;
    s.state.localPlayer = 3;
    const withHooks = new PreviewCollection(s.pool, s.types, s.spots, s.state, hooks, 0);
    withHooks.collect({}, new Uint8Array(ENTRY * 2), 5);
    assert.deepEqual([s.state.highlight, s.state.countdown, sp], [0x1234, 1, [3]]);
    // 7000 이하·NaN 은 강조하지 않는다. 조회가 꺼지면 SP 를 읽지 않는다.
    for (const bits of [floatToBits(7000), floatToBits(6999), 0x7fc00000]) {
        s.state.highlight = 77;
        s.state.countdown = 1;
        new PreviewCollection(s.pool, s.types, s.spots, s.state, { ...hooks, readSp: () => bits }, 0).collect({}, new Uint8Array(ENTRY * 2), 5);
        assert.equal(s.state.highlight, 77, String(bits));
    }
    sp.length = 0;
    s.state.countdown = 1;
    new PreviewCollection(s.pool, s.types, s.spots, s.state, { ...hooks, querySp: () => 0 }, 0).collect({}, new Uint8Array(ENTRY * 2), 5);
    assert.deepEqual(sp, []);
    // 첫 풀이 없거나 첫 순열이 −1(WORD 0xffff)이면 강조 시각을 현재 시각으로 쓴다.
    for (const [pool, index] of [[false, 0], [true, -1]]) {
        s.state.highlight = 77;
        s.state.countdown = 0;
        s.state.firstSpPoolPresent = pool;
        s.state.firstSpIndex = index;
        new PreviewCollection(s.pool, s.types, s.spots, s.state, { now: () => 4242, scrambledOffset: () => 1, copiedOffset: () => 1 }, 0).collect({}, new Uint8Array(ENTRY * 2), 5);
        assert.equal(s.state.highlight, 4242, `${pool}/${index}`);
    }
});

test('커서 미리보기 수집기 계약: 깊이 우선순위(표면 0xffffffff < 섬 −20)와 부모 Gump 깊이−9, 받침 배치 칸 spot 의 float 상수·64비트 절삭을 따른다', () => {
    const depthOf = (flags2, zOrder, parentGump = 0, gumpDepth = 0) => {
        const s = scene();
        s.types[1] = { flags2, zOrder };
        s.state.parentGump = parentGump;
        const work = new Uint8Array(ENTRY * 2);
        new PreviewCollection(s.pool, s.types, s.spots, s.state, { gumpDepth: () => gumpDepth }, 0).collect({}, work, 2);
        const view = new DataView(work.buffer);
        return [view.getInt16(12, true), view.getInt16(ENTRY + 12, true)];
    };
    assert.deepEqual(depthOf(0, 7), [7, 6]);
    assert.deepEqual(depthOf(0, -3), [-3, -4]);
    assert.deepEqual(depthOf(4, 7), [-1, -2]);
    assert.deepEqual(depthOf(0x400000, 7), [-20, -21]);
    assert.deepEqual(depthOf(4 | 0x400000, 7), [-20, -21]);
    assert.deepEqual(depthOf(0, 7, 9, 100), [91, 90]);
    assert.deepEqual(depthOf(4, 7, 9, 100), [-1, -2]);
    assert.throws(() => { const s = scene(); s.state.parentGump = 3; new PreviewCollection(s.pool, s.types, s.spots, s.state).collect({}, new Uint8Array(ENTRY * 2), 2); }, /parent depth/);
    // 받침(82) 조각: 배치 칸 spot(20+0.9999→20, 25→25 → 25*256+20)의 비트 2 가 서 있으면 건너뛰고, 아니면 본체 깊이 20·보조 깊이는 원래 깊이−1 이다.
    const pedestal = (spotValue, x = 20, y = 25) => {
        const s = scene();
        s.place(5, 82, 20, 25);
        s.state.x = x;
        s.state.y = y;
        s.spots[Math.trunc(y + 0.9999) * 256 + Math.trunc(x + 0.9999)] = spotValue;
        const work = new Uint8Array(ENTRY * 2);
        const count = new PreviewCollection(s.pool, s.types, s.spots, s.state).collect({}, work, 2);
        const view = new DataView(work.buffer);
        return [count, view.getInt16(12, true), view.getInt16(ENTRY + 12, true)];
    };
    assert.deepEqual(pedestal(0), [2, 20, 6]);
    assert.deepEqual(pedestal(1), [2, 20, 6]);
    assert.deepEqual(pedestal(2)[0], 0);
    assert.deepEqual(pedestal(3)[0], 0);
    assert.deepEqual(pedestal(0, 30.25, 40.5), [2, 20, 6]);
    assert.equal(pedestal(2, 30.25, 40.5)[0], 0);
    // 비유한·범위 밖 좌표는 CRT 불확정 정수의 하위 0 이라 spot 0 번 칸을 본다.
    for (const bad of [Number.NaN, Infinity, -Infinity, 2 ** 70]) {
        const s = scene();
        s.place(5, 82, 20, 25);
        s.state.x = bad;
        s.state.y = bad;
        s.spots[0] = 2;
        assert.equal(new PreviewCollection(s.pool, s.types, s.spots, s.state).collect({}, new Uint8Array(ENTRY * 2), 2), 0, String(bad));
    }
    // spot 주소가 65536 이상이면 거부한다(y 가 256 이상).
    const far = scene();
    far.place(5, 82, 20, 25);
    far.state.y = 300;
    assert.throws(() => new PreviewCollection(far.pool, far.types, far.spots, far.state).collect({}, new Uint8Array(ENTRY * 2), 2), /spot address/);
});

test('커서 미리보기 수집기 계약: SID 변환·목록 범위·현재 목록 재조회와 SP 저장소 훅·공통 수집기 묶음 연결', () => {
    assert.equal(cursorPreviewSid(65535), 65535);
    assert.throws(() => cursorPreviewSid(65536), /호스트 SID 범위/);
    assert.equal(cursorPreviewSid(-1 >>> 0 & 0xffff), 65535);
    const s = scene();
    const collector = new PreviewCollection(s.pool, s.types, s.spots, s.state);
    // 개수가 목록보다 크면 목록 범위 오류이고 SID 0 은 거부한다.
    s.state.pieceCount = 2;
    assert.throws(() => collector.collect({}, new Uint8Array(ENTRY * 4), 4), /Preview collection list/);
    s.state.pieceCount = 1;
    s.state.pieces = [0];
    assert.throws(() => collector.collect({}, new Uint8Array(ENTRY * 2), 2), /zero SID/);
    s.state.pieces = [70000];
    assert.throws(() => collector.collect({}, new Uint8Array(ENTRY * 2), 2), /호스트 SID 범위/);
    // SP 저장소 훅: 초기화 전에는 거부, 초기화 뒤 readSp 가 저장소 읽기이고 다른 훅은 보존한다.
    assert.throws(() => makePreviewSpHooks({ initialized: false, get: () => 0 }), /prepared SP store required/);
    const gump = () => 1;
    const hooks = makePreviewSpHooks({ initialized: true, get: (owner) => owner * 2 }, { gumpDepth: gump });
    assert.deepEqual([hooks.gumpDepth === gump, hooks.readSp(4)], [true, 8]);
    // 공통 수집기 묶음 연결: preview 가 이 수집기로 위임한다.
    const bound = {};
    bindPreviewCollection(bound, collector);
    s.state.pieces = [5];
    const work = new Uint8Array(ENTRY * 2);
    assert.equal(bound.preview({}, work, 2), 2);
    assert.equal(work[14], 4);
});

test('커서 미리보기 수집기 계약: 상태 기본값, 토큰 범위 경계, 표면/섬 비트의 정확한 마스크, 첫 순열 WORD 부호 확장', () => {
    assert.deepEqual({ ...new PreviewCollectionState() }, {
        lockDepth: 0, type: 0, parentGump: 0, specialTypeA: 0, specialTypeB: 0, legal: 0, pieceCount: 0, pieces: [], x: 0, y: 0, scrambledOffset: 0, copiedOffset: 0,
        now: 0, highlight: 0, countdown: 0, querySp: 0, localPlayer: 0, firstSpPoolPresent: true, firstSpIndex: 0,
    });
    // 토큰 범위: 기준 + 용량×50 이 정확히 2^32 이면 허용, 하나 더 크면 거부한다.
    const s = scene();
    const capacity = s.pool.capacity;
    assert.doesNotThrow(() => new PreviewCollection(s.pool, s.types, s.spots, s.state, {}, 0x100000000 - capacity * 50));
    assert.throws(() => new PreviewCollection(s.pool, s.types, s.spots, s.state, {}, 0x100000000 - capacity * 50 + 1), /token range/);
    // 표면(4)/섬(0x400000) 비트만이 우선한다: flags2 의 다른 낮은 비트(1·2·8·0x200000)만 있으면 zOrder 그대로이다.
    const depthWith = (flags2) => {
        const t = scene();
        t.types[1] = { flags2, zOrder: 7 };
        const work = new Uint8Array(ENTRY * 2);
        new PreviewCollection(t.pool, t.types, t.spots, t.state).collect({}, work, 2);
        return new DataView(work.buffer).getInt16(12, true);
    };
    for (const flags2 of [1, 2, 3, 8, 0x200000, 0x800000, 0xfffffffb & 0x7fffffff & ~0x400000]) assert.equal(depthWith(flags2), 7, `flags2 ${flags2.toString(16)}`);
    // 첫 순열은 WORD 부호 확장이다: 0xffff·0x1ffff 는 −1 로 읽어 강조하고 0x7fff·0x8000 은 그렇지 않다(0x8000 은 −32768).
    for (const [index, highlighted] of [[0xffff, true], [0x1ffff, true], [-1, true], [0x7fff, false], [0x8000, false], [0xfffe, false], [0, false]]) {
        const t = scene();
        t.state.highlight = 77;
        t.state.firstSpIndex = index;
        new PreviewCollection(t.pool, t.types, t.spots, t.state, { now: () => 4242 }, 0).collect({}, new Uint8Array(ENTRY * 2), 5);
        assert.equal(t.state.highlight, highlighted ? 4242 : 77, `index ${index.toString(16)}`);
    }
});

test('커서 미리보기 수집기 계약: 쌍 제출 직전·직후 현재 값을 다시 읽는 경계(목록 축소·SID 교체·타입 범위·spot 주소 65536)와 오류 문구', () => {
    const run = (mutate) => {
        const s = scene();
        mutate(s);
        return s;
    };
    const message = (s, work = new Uint8Array(ENTRY * 4), limit = 4, hooks = {}) => new PreviewCollection(s.pool, s.types, s.spots, s.state, hooks, 0).collect({}, work, limit);
    // 출력 칸이 모자라면 오류 문구가 출력 부족이다(두 번째 조각은 칸이 없음).
    const second = run((s) => { s.state.pieceCount = 2; s.state.pieces = [5, 5]; });
    assert.throws(() => message(second, new Uint8Array(ENTRY * 2), 4), /Preview collection output/);
    // 타입 번호가 표 길이와 같으면 타입 범위 오류이다.
    assert.throws(() => message(run((s) => { s.state.type = 2; })), /Preview collection type/);
    // spot 주소가 정확히 65536 이면 거부한다(x 칸 0, y 칸 256).
    const edge = run((s) => { s.place(5, 82, 20, 25); s.state.x = -0.5; s.state.y = 255.5; });
    assert.throws(() => message(edge), /spot address/);
    // x 가 65536 이상이면 32비트 하위 값이 아니라 큰 주소 그대로 거부한다.
    const wide = run((s) => { s.place(5, 82, 20, 25); s.state.x = 70000.5; s.state.y = 0; });
    assert.throws(() => message(wide), /spot address/);
    // 훅이 목록을 줄이거나 SID 를 바꾸면 쌍 제출 때 현재 목록을 다시 읽는다.
    const shrink = run((s) => { s.state.countdown = 1; s.state.querySp = 1; });
    assert.throws(() => message(shrink, new Uint8Array(ENTRY * 2), 2, { readSp: () => { shrink.state.pieces = []; return 0; } }), /Preview collection list/);
    const swapped = run((s) => { s.place(6, 1, 30, 40); s.state.countdown = 1; s.state.querySp = 1; });
    const work = new Uint8Array(ENTRY * 2);
    message(swapped, work, 2, { readSp: () => { swapped.state.pieces = [6]; return 0; } });
    const view = new DataView(work.buffer);
    // 본체는 처음 SID(5)의 위치/토큰, 보조 레이어는 바뀐 현재 SID(6)의 토큰을 쓴다(위치는 처음 읽은 값).
    assert.deepEqual([view.getFloat32(0, true), view.getUint32(8, true), view.getFloat32(ENTRY, true), view.getUint32(ENTRY + 8, true)], [20, 250, 20, 300]);
    for (const current of [0, 24000]) {
        const bad = run((s) => { s.state.countdown = 1; s.state.querySp = 1; });
        assert.throws(() => message(bad, new Uint8Array(ENTRY * 2), 2, { readSp: () => { bad.state.pieces = [current]; return 0; } }), /current SID/, `현재 SID ${current}`);
    }
    // 토큰 되돌리기: 기준 바로 아래와 기준 − 50 은 거부하고 기준 자체(0 번)도 거부한다.
    const collector = new PreviewCollection(second.pool, second.types, second.spots, second.state, {}, 1000);
    assert.throws(() => collector.object(999), /^RangeError: Preview collection token$/);
    assert.throws(() => collector.object(1000), /token SID/);
    assert.throws(() => collector.object(950), /^RangeError: Preview collection token$/);
    // SP 훅이 없는데 카운트다운과 조회가 켜져 있으면 전용 오류이다. 공통 묶음 연결은 한도를 그대로 넘긴다.
    const noHook = run((s) => { s.state.countdown = 1; s.state.querySp = 1; });
    assert.throws(() => message(noHook, new Uint8Array(ENTRY * 2), 2), /Preview collection SP hook/);
    const bound = {};
    const target = run(() => {});
    bindPreviewCollection(bound, new PreviewCollection(target.pool, target.types, target.spots, target.state, {}, 0));
    assert.equal(bound.preview({}, new Uint8Array(ENTRY * 2), 1), 2);
});

// 같은 원문 입력을 실제30DWORD와 별도 표시 판정에 공급한다. 독립 중복값은 반대로 둔다. (2026-11-14 WB04-E404)
test('PreviewCollection_PatchNativeSharedCursor: 실제 커서 전역의520행이 독립 상태와 같은 원본 바이트/피드백을 낸다', { skip: rows === null }, () => {
    const originals = rows.filter((row) => row[0] === 'originals'); assert.equal(originals.length, 520); const pool = new SidPool(24000, { server: false });
    // 기대 출력은 기존 원본 관측에서만 읽는다.
    for (const row of originals) assert.deepEqual(replay(pool, row, true), [], row[1]);
});

// 실제 SP 재진입 뒤 현재 목록/count와 저장한 위치/깊이를 구별한다. (2026-11-14 WB04-E404)
test('PreviewCollection_SharedCursorCurrentReadsAndContracts: SP가 바꾼 현재SID·개수와 다른 풀/판본 계약을 지킨다', () => {
    const w = scene(); w.place(6, 1, -10, -30); w.state.pieces.push(6); w.state.pieceCount = 2; w.state.querySp = 1; w.state.countdown = 1;
    const actual = new CursorPreviewBindingScene(); actual.collection(w.state);
    const hooks = { readSp: (owner) => { assert.equal(owner, actual.drop.localPlayer); actual.lifecycle.previews[0] = 6; actual.lifecycle.previewCount = 1; w.place(5, 1, 70, 80); w.types[1].zOrder = 100; return floatToBits(0); } };
    const collector = new PreviewCollection(w.pool, w.types, w.spots, w.state, hooks); collector.bindCursor(actual.source(w.pool)); w.state.lockDepth = 1;
    const work = new Uint8Array(ENTRY * 8); assert.equal(collector.collect({}, work, 8), 2); const view = new DataView(work.buffer);
    assert.deepEqual([view.getUint32(8, true), view.getUint32(ENTRY + 8, true)], [250, 300]);
    assert.deepEqual([view.getFloat32(ENTRY, true), view.getFloat32(ENTRY + 4, true), view.getInt16(12, true), view.getInt16(ENTRY + 12, true)], [20, 25, 7, 6]);
    assert.equal(actual.drop.legal, 0); assert.equal(actual.placement.placementAllowed, 1);
    assert.throws(() => collector.bindCursor(actual.source(new SidPool(24000))), /pool\/edition/);
    const cd = { edition: 'cd1072', capacity: 24000 }; const cdCollector = new PreviewCollection(cd, w.types, w.spots, w.state);
    assert.throws(() => cdCollector.bindCursor(actual.source(cd)), /pool\/edition/);
});
