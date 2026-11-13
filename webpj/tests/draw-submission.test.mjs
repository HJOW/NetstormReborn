// 공통 표시 제출 render/draw-submission.js 를 원본 10.78 관측(drawsubmission-x86.tsv originals 322행)과 cpppj `DrawSubmissionTests.cpp` 의 계약 시험으로 대조한다.
//  - 관측 한 행 = 상위 Renderer(004994b0)의 병합·정렬·분배 한 번이다: 네 수집기의 완성 입력 바이트(UI·건설·일반·미리보기)와 묶음 저장소(토큰=개수=전체 항목), 입력 옵션 여섯(case·dirtyFlags·정렬 여부·묶음 숨김·debug 표시),
//    기대는 수집기 호출 기록(이름:항목 위치:남은 한도:좌표 비트)·병합 후 바이트·정렬 후 바이트·분배 호출 기록(S 항목 바이트·U/C 토큰·P 항목 바이트·D)·작업 배열 앞뒤 보호 바이트(0xa5)이다.
//    수집기·화면 출력은 명시 응답 경계이고 병합/CRT 정렬/분배는 이 모듈이 낸 결과를 원본 바이트와 비교한다. original1037·originalCD 행은 웹 범위 밖(10.78 전용)이라 읽지 않는다.
//  - cpppj 계약 시험: 손상된 수집 반환·묶음 초과 거부, 사용하는 종류만 필수 경계를 요구하고 알려지지 않은 종류도 debug 관찰은 유지, 5,000 초과 버퍼는 원본 배열 용량까지만 공급하고 용량 0 에서도 수집기를 원본 순서로 부름.
//  - 웹 계약: 묶음의 tail 바이트 보존, 개수 상자는 그리기 중 변경 반영, 정렬은 항목 전체(kind·tail)를 보존하고 비유한 좌표를 교환 전에 거부.
// 실제 게임·브라우저 실행이 아니다. (2026-11-13 WB04-E382)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';
import { collectDrawEntries, dispatchDrawEntries, sortRenderEntries, DRAW_SUBMISSION_CAPACITY } from '../www/render/draw-submission.js';

// 원본 관측 행이다. (2026-11-13 WB04-E382)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'drawsubmission-x86.tsv'));
const ENTRY = 16;
const GUARD = 0xa5;

// 16진 문자열 한 줄을 16바이트 항목 목록으로 바꾼다('-' 는 빈 목록). (2026-11-13 WB04-E382)
const unhex = (text) => (text === '-' ? new Uint8Array(0) : Uint8Array.from(Buffer.from(text, 'hex')));
// 바이트를 소문자 16진 문자열로 바꾼다(빈 목록은 '-', 원본 직렬화와 같다). (2026-11-13 WB04-E382)
const packed = (bytes) => (bytes.length === 0 ? '-' : Buffer.from(bytes.buffer, bytes.byteOffset, bytes.byteLength).toString('hex'));
// 사각형 float 네 개를 비트 직렬화한다(원본 관찰과 같은 순서). (2026-11-13 WB04-E382)
const floatBits = (value) => { const view = new DataView(new ArrayBuffer(4)); view.setFloat32(0, value, true); return view.getUint32(0, true); };
const bitsFloat = (bits) => { const view = new DataView(new ArrayBuffer(4)); view.setUint32(0, bits >>> 0, true); return view.getFloat32(0, true); };
const worldText = (rect) => `${floatBits(rect.left)},${floatBits(rect.top)},${floatBits(rect.right)},${floatBits(rect.bottom)}`;
const screenText = (rect) => `${rect.left},${rect.top},${rect.right},${rect.bottom}`;
// 입력 칸 목록을 정수로 읽는다(16진 `0x…`·10진 모두). (2026-11-13 WB04-E382)
const numbers = (text) => text.split(',').map((field) => Number(field));

// 한 관측 행을 재생해 수집·정렬·분배 결과를 원본 기록과 비교한다. (2026-11-13 WB04-E382)
function replay(row) {
    const input = numbers(row[1]);
    const supplied = row[2].split('|');
    const source = supplied.slice(0, 4).map(unhex);
    // 묶음 저장소: 헤더 토큰·최초 개수·전체 내부 항목. 개수는 상자라 분배 중 변경을 그대로 반영한다.
    const storage = supplied.slice(4).map((text) => {
        const fields = text.split('=');
        return { token: Number(fields[0]), count: { value: Number(fields[1]) }, entries: unhex(fields[2]) };
    });
    const groups = storage.map((group) => ({ token: group.token, entries: group.entries, count: group.count }));
    const request = { screen: { left: 0, top: 0, right: 0, bottom: 0 }, visible: null, unclipped: null, dirtyFlags: input[1], groupRowLimit: 0 };
    // 좌표 환산은 복원 범위 밖이라 원본이 계산한 수집기 인자를 경계 입력으로 쓴다.
    for (const event of row[7].split(';')) {
        const fields = event.split(':');
        const coordinates = numbers(fields[fields.length - 1]);
        if (fields[0] === 'ui') request.screen = { left: coordinates[0] | 0, top: coordinates[1] | 0, right: coordinates[2] | 0, bottom: coordinates[3] | 0 };
        const world = () => ({ left: bitsFloat(coordinates[0]), top: bitsFloat(coordinates[1]), right: bitsFloat(coordinates[2]), bottom: bitsFloat(coordinates[3]) });
        if (fields[0] === 'construction') request.visible = world();
        if (fields[0] === 'preview') request.unclipped = world();
        if (fields[0] === 'group') request.groupRowLimit = Number(fields[1]);
    }
    const guarded = new Uint8Array((DRAW_SUBMISSION_CAPACITY + 2) * ENTRY).fill(GUARD);
    const work = guarded.subarray(ENTRY, ENTRY + DRAW_SUBMISSION_CAPACITY * ENTRY);
    let collected = '';
    let drawn = '';
    const add = (text, event) => (text ? `${text};${event}` : event);
    // 하나의 명시 수집 응답을 복사하고 원본과 같은 포인터 위치/남은 용량/좌표를 관찰한다.
    const copy = (index, name, rectangle, output, limit) => {
        collected = add(collected, `${name}:${(output.byteOffset - work.byteOffset) / ENTRY}:${limit}:${rectangle}`);
        assert.ok(source[index].length <= output.length);
        output.set(source[index]);
        return source[index].length / ENTRY;
    };
    const hooks = {
        ui: (rect, output, limit) => copy(0, 'ui', screenText(rect), output, limit),
        construction: (rect, output, limit) => copy(1, 'construction', worldText(rect), output, limit),
        objects: (rect, output, limit) => copy(2, 'objects', worldText(rect), output, limit),
        preview: (rect, output, limit) => copy(3, 'preview', worldText(rect), output, limit),
        groups: (rect, time, covered) => {
            collected = add(collected, `group:${time}:${worldText(rect)}`);
            covered.value = input[0] % 2 !== 0;
            return groups;
        },
    };
    const result = collectDrawEntries(request, work, hooks);
    const entries = work.subarray(0, result.count * ENTRY);
    const issues = [];
    const check = (name, actual, expected) => { if (actual !== expected) issues.push(`${name}: ${actual} != ${expected}`); };
    check('count', result.count, Number(row[3]));
    check('covered', result.backgroundCovered ? '1' : '0', row[4]);
    check('collected', collected, row[7]);
    check('merged', packed(entries), row[5]);
    if (input[2]) sortRenderEntries(entries);
    check('sorted', packed(entries), row[6]);
    const state = { hideGroups: input[3] !== 0, debugAfterEach: input[4] !== 0 };
    let toggled = false;
    const dispatch = {
        sprite: (entry) => {
            drawn = add(drawn, `S:${packed(entry)}`);
            // 첫 묶음의 첫 항목을 그릴 때 그 묶음 개수를 바꿔 매 차례 현재 개수를 읽는지 본다(case 4: 1, case 5: 4).
            if (storage.length && entry.buffer === storage[0].entries.buffer && entry.byteOffset === storage[0].entries.byteOffset && (input[0] === 4 || input[0] === 5)) storage[0].count.value = input[0] === 4 ? 1 : 4;
        },
        group: (token) => {
            const found = groups.find((group) => group.token === token);
            if (!found) throw new Error('원본 묶음 토큰 누락');
            return found;
        },
        ui: (token) => {
            drawn = add(drawn, `U:${token}`);
            if (input[0] === 9 && !toggled) { state.hideGroups = !state.hideGroups; state.debugAfterEach = !state.debugAfterEach; toggled = true; }
        },
        construction: (token) => { drawn = add(drawn, `C:${token}`); },
        preview: (entry) => { drawn = add(drawn, `P:${packed(entry)}`); },
        afterEach: () => { drawn = add(drawn, 'D'); },
    };
    dispatchDrawEntries(entries, state, dispatch);
    const guard = packed(guarded.subarray(0, ENTRY)) + packed(guarded.subarray((result.count + 1) * ENTRY, (result.count + 2) * ENTRY));
    check('drawn', drawn === '' ? '-' : drawn, row[8]);
    check('guard', guard, row[12]);
    return issues;
}

test('공통 표시 제출: 원본 10.78 관측 322행의 병합·정렬·분배·작업 배열 보호 바이트가 모두 일치한다', { skip: rows === null }, () => {
    const originals = rows.filter((row) => row[0] === 'originals');
    assert.equal(originals.length, 322);
    let replayed = 0;
    for (const row of originals) {
        assert.equal(row.length, 13);
        const issues = replay(row);
        assert.deepEqual(issues, [], `공통 표시 제출 ${row[1]} 불일치`);
        replayed += 1;
    }
    assert.equal(replayed, 322);
});

// 이미 쓴 수집기 기록을 한 줄로 만드는 도우미이다. (2026-11-13 WB04-E382)
const entry = (fields = {}) => {
    const bytes = new Uint8Array(ENTRY);
    const view = new DataView(bytes.buffer);
    view.setFloat32(0, fields.x ?? 0, true);
    view.setFloat32(4, fields.y ?? 0, true);
    view.setUint32(8, fields.token ?? 0, true);
    view.setInt16(12, fields.depth ?? 0, true);
    bytes[14] = fields.kind ?? 0;
    bytes[15] = fields.tail ?? 0;
    return bytes;
};
const request = () => ({ screen: { left: 0, top: 0, right: 0, bottom: 0 }, visible: { left: 0, top: 0, right: 0, bottom: 0 }, unclipped: { left: 0, top: 0, right: 0, bottom: 0 }, dirtyFlags: 0, groupRowLimit: 0 });
const throwsOf = (action) => { try { action(); } catch { return true; } return false; };

test('공통 표시 제출: 손상된 수집 반환과 묶음 초과를 다음 단계로 넘기지 않는다(DrawSubmission_RejectsInvalidCountsAndGroupOverflow)', () => {
    const work = new Uint8Array(ENTRY * 2);
    let called = false;
    const hooks = { objects: () => { called = true; return 0; } };
    hooks.ui = () => -1;
    assert.ok(throwsOf(() => collectDrawEntries(request(), work, hooks)));
    assert.equal(called, false);
    hooks.ui = () => 3;
    assert.ok(throwsOf(() => collectDrawEntries(request(), work, hooks)));
    // 정수가 아닌 반환도 거부한다.
    for (const bad of [0.5, NaN, undefined, '1']) {
        hooks.ui = () => bad;
        assert.ok(throwsOf(() => collectDrawEntries(request(), work, hooks)), String(bad));
    }
    assert.equal(called, false);
    const count = { value: 1 };
    const member = new Uint8Array(ENTRY);
    const group = [{ token: 7, entries: member, count }];
    hooks.ui = () => 2;
    hooks.groups = () => group;
    assert.ok(throwsOf(() => collectDrawEntries(request(), work, hooks)));
    assert.equal(called, false);
    delete hooks.ui;
    group[0].entries = new Uint8Array(0);
    assert.ok(throwsOf(() => collectDrawEntries(request(), work, hooks)));
    group[0].entries = new Uint8Array(ENTRY - 1);
    assert.ok(throwsOf(() => collectDrawEntries(request(), work, hooks)));
    group[0].entries = member;
    group[0].count = null;
    assert.ok(throwsOf(() => collectDrawEntries(request(), work, hooks)));
});

test('공통 표시 제출: 사용하는 종류만 필수 경계를 요구하고 알려지지 않은 종류도 상위 debug 관찰은 유지한다(DrawSubmission_DispatchContractsAndUnknownKinds)', () => {
    const one = entry();
    const state = { hideGroups: false, debugAfterEach: false };
    const hooks = {};
    assert.ok(throwsOf(() => dispatchDrawEntries(one, state, hooks)));
    one[14] = 1;
    state.hideGroups = true;
    assert.ok(!throwsOf(() => dispatchDrawEntries(one, state, hooks)));
    state.hideGroups = false;
    assert.ok(throwsOf(() => dispatchDrawEntries(one, state, hooks)));
    const count = { value: 2 };
    const members = new Uint8Array(ENTRY);
    hooks.group = () => ({ token: 0, entries: members, count });
    let drawn = 0;
    hooks.sprite = () => { drawn += 1; };
    // 개수 2 가 저장소(1개)를 넘으면 첫 항목을 그린 뒤 거부한다.
    assert.ok(throwsOf(() => dispatchDrawEntries(one, state, hooks)));
    assert.equal(drawn, 1);
    count.value = -1;
    assert.ok(!throwsOf(() => dispatchDrawEntries(one, state, hooks)));
    assert.equal(drawn, 1);
    one[14] = 255;
    state.debugAfterEach = true;
    assert.ok(throwsOf(() => dispatchDrawEntries(one, state, hooks)));
    let after = 0;
    hooks.afterEach = () => { after += 1; };
    dispatchDrawEntries(one, state, hooks);
    assert.deepEqual([after, drawn], [1, 1]);
    // 묶음 개수 상자가 없으면 묶음을 그리지 않고 거부한다.
    one[14] = 1;
    hooks.group = () => ({ token: 0, entries: members, count: null });
    assert.throws(() => dispatchDrawEntries(one, state, hooks), /개수 저장소 누락/);
});

test('공통 표시 제출: 5,000 초과 버퍼도 원본 배열 용량까지만 공급하고 용량 0 에서도 연결된 수집기를 원본 순서로 부른다(DrawSubmission_CapacityAndEmptyBuffer)', () => {
    const work = new Uint8Array(ENTRY * 5001);
    work[work.length - 1] = 71;
    const limits = [];
    const hooks = {
        ui: (rect, output, limit) => { limits.push(limit); assert.equal(output.length / ENTRY, limit); return limit; },
        construction: (rect, output, limit) => { limits.push(limit); assert.equal(output.length, 0); return 0; },
    };
    hooks.objects = hooks.construction;
    hooks.preview = hooks.construction;
    const result = collectDrawEntries(request(), work, hooks);
    assert.deepEqual([result.count, work[work.length - 1], limits], [5000, 71, [5000, 0, 0, 0]]);
    limits.length = 0;
    assert.equal(collectDrawEntries(request(), new Uint8Array(0), hooks).count, 0);
    assert.deepEqual(limits, [0, 0, 0, 0]);
});

test('공통 표시 제출 계약: 묶음은 첫 항목의 x·y·depth 와 헤더 토큰으로 kind 1 항목을 만들고 tail·보호 바이트는 쓰지 않는다', () => {
    const work = new Uint8Array(ENTRY * 3).fill(0x5a);
    const first = entry({ x: 1.5, y: -2.25, token: 0x11223344, depth: -7, kind: 3, tail: 9 });
    const second = entry({ x: 8, y: 8, depth: 99 });
    const covered = [];
    const result = collectDrawEntries(request(), work, {
        groups: (rect, limit, flag) => { flag.value = true; covered.push(limit); return [{ token: 0xdeadbeef, entries: Uint8Array.from([...first, ...second]), count: { value: 2 } }]; },
    });
    assert.deepEqual([result.count, result.backgroundCovered], [1, true]);
    const view = new DataView(work.buffer);
    assert.deepEqual([view.getFloat32(0, true), view.getFloat32(4, true), view.getUint32(8, true), view.getInt16(12, true), work[14], work[15]], [1.5, -2.25, 0xdeadbeef, -7, 1, 0x5a]);
    assert.ok(work.subarray(ENTRY).every((byte) => byte === 0x5a));
    // 묶음 수집기의 두 번째 인자(groupRowLimit)는 요청 값이다.
    const row = request();
    row.groupRowLimit = 31;
    collectDrawEntries(row, work, { groups: (rect, limit) => { covered.push(limit); return []; } });
    assert.deepEqual(covered, [0, 31]);
    // dirtyFlags 비트 2 는 UI 이후 수집을 모두 건너뛴다(비트 1·8 은 건너뛰지 않는다).
    const calls = [];
    const probe = { ui: () => { calls.push('ui'); return 0; }, construction: () => { calls.push('construction'); return 0; }, groups: () => { calls.push('groups'); return []; }, objects: () => { calls.push('objects'); return 0; }, preview: () => { calls.push('preview'); return 0; } };
    for (const [flags, expected] of [[4, ['ui']], [5, ['ui']], [1, ['ui', 'construction', 'groups', 'objects', 'preview']], [8, ['ui', 'construction', 'groups', 'objects', 'preview']], [0xfffffffb, ['ui', 'construction', 'groups', 'objects', 'preview']]]) {
        calls.length = 0;
        const r = request();
        r.dirtyFlags = flags;
        collectDrawEntries(r, new Uint8Array(ENTRY), probe);
        assert.deepEqual(calls, expected, `dirty ${flags}`);
    }
});

test('공통 표시 정렬: 항목 전체 16바이트(kind·tail 포함)를 깊이 내림차순·y·x 오름차순으로 옮기고 비유한 좌표는 교환 전에 거부한다', () => {
    const list = [entry({ x: 5, y: 1, depth: 1, token: 1, kind: 3, tail: 0x41 }), entry({ x: 1, y: 1, depth: 9, token: 2, kind: 0, tail: 0x42 }), entry({ x: 0, y: 0, depth: 1, token: 3, kind: 2, tail: 0x43 }), entry({ x: 0, y: 1, depth: 1, token: 4, kind: 4, tail: 0x44 })];
    const bytes = Uint8Array.from(list.flatMap((item) => [...item]));
    assert.equal(sortRenderEntries(bytes), bytes);
    const view = new DataView(bytes.buffer);
    // 깊이 9 가 먼저, 같은 깊이 1 은 y 오름차순 → x 오름차순이다.
    assert.deepEqual([0, 1, 2, 3].map((index) => view.getUint32(index * ENTRY + 8, true)), [2, 3, 4, 1]);
    assert.deepEqual([0, 1, 2, 3].map((index) => [bytes[index * ENTRY + 14], bytes[index * ENTRY + 15]]), [[0, 0x42], [2, 0x43], [4, 0x44], [3, 0x41]]);
    // 비유한 좌표가 뒤에 있어도 앞 항목을 먼저 바꾸지 않는다.
    const bad = Uint8Array.from([...entry({ x: 3, y: 0, depth: 0, token: 1 }), ...entry({ x: 2, y: 0, depth: 0, token: 2 }), ...entry({ x: NaN, y: 0, depth: 0, token: 3 })]);
    const before = Uint8Array.from(bad);
    assert.throws(() => sortRenderEntries(bad), RangeError);
    assert.deepEqual(bad, before);
    // 항목 하나·빈 목록은 그대로이다.
    sortRenderEntries(new Uint8Array(0));
    const single = entry({ x: 1, token: 5 });
    sortRenderEntries(single);
    assert.equal(new DataView(single.buffer).getUint32(8, true), 5);
});

test('공통 표시 제출 계약: 분배는 알려진 종류별로 정확한 훅만 부르고 묶음 숨김·개수 변경을 현재 값으로 읽으며 항목 보기는 같은 저장소를 가리킨다', () => {
    const list = [entry({ kind: 0, token: 10 }), entry({ kind: 2, token: 20 }), entry({ kind: 3, token: 30 }), entry({ kind: 4, token: 40 }), entry({ kind: 1, token: 50 })];
    const bytes = Uint8Array.from(list.flatMap((item) => [...item]));
    const log = [];
    const members = Uint8Array.from([...entry({ token: 1 }), ...entry({ token: 2 }), ...entry({ token: 3 })]);
    const count = { value: 3 };
    const tokenOf = (item) => new DataView(item.buffer, item.byteOffset, item.byteLength).getUint32(8, true);
    const hooks = {
        sprite: (item) => {
            assert.equal(item.length, ENTRY);
            log.push(`S${tokenOf(item)}`);
            // 첫 멤버를 그릴 때 개수를 2 로 줄인다 — 이후 반복은 현재 개수를 따른다.
            if (tokenOf(item) === 1) count.value = 2;
            if (item.buffer === bytes.buffer) assert.equal(item.byteOffset % ENTRY, 0);
        },
        ui: (token) => log.push(`U${token}`),
        construction: (token) => log.push(`C${token}`),
        preview: (item) => log.push(`P${tokenOf(item)}`),
        group: (token) => { log.push(`G${token}`); return { token, entries: members, count }; },
        afterEach: () => log.push('D'),
    };
    dispatchDrawEntries(bytes, { hideGroups: false, debugAfterEach: true }, hooks);
    assert.deepEqual(log, ['S10', 'D', 'U20', 'D', 'C30', 'D', 'P40', 'D', 'G50', 'S1', 'S2', 'D']);
    log.length = 0;
    dispatchDrawEntries(bytes, { hideGroups: true, debugAfterEach: false }, hooks);
    assert.deepEqual(log, ['S10', 'U20', 'C30', 'P40']);
});

test('공통 표시 제출 계약: 묶음이 남은 용량을 정확히 채우는 것은 허용하고 하나 더 많으면 거부하며 필수 콜백 누락은 전용 오류 문구로 알린다', () => {
    const member = entry({ x: 1, y: 2, depth: 3 });
    const group = (token) => ({ token, entries: member, count: { value: 1 } });
    // 작업 2칸 중 UI 가 1칸을 쓰고 묶음 1개가 마지막 칸을 정확히 채운다.
    const exact = collectDrawEntries(request(), new Uint8Array(ENTRY * 2), { ui: () => 1, groups: () => [group(7)] });
    assert.equal(exact.count, 2);
    assert.throws(() => collectDrawEntries(request(), new Uint8Array(ENTRY * 2), { ui: () => 1, groups: () => [group(7), group(8)] }), RangeError);
    assert.throws(() => collectDrawEntries(request(), new Uint8Array(ENTRY), { groups: () => [group(7), group(8)] }), /용량 초과/);
    // 수집 반환이 남은 용량과 같은 것은 허용, 하나 더 많은 것은 거부한다.
    assert.equal(collectDrawEntries(request(), new Uint8Array(ENTRY * 2), { ui: () => 2 }).count, 2);
    assert.throws(() => collectDrawEntries(request(), new Uint8Array(ENTRY * 2), { ui: () => 3 }), /반환 개수/);
    assert.throws(() => collectDrawEntries(request(), new Uint8Array(ENTRY * 2), { ui: () => -1 }), /반환 개수/);
    // 분배: 쓰는 종류마다 빠진 콜백을 같은 오류 문구로 거부한다.
    const state = { hideGroups: false, debugAfterEach: false };
    for (const kind of [0, 1, 2, 3, 4]) {
        const one = entry({ kind });
        assert.throws(() => dispatchDrawEntries(one, state, {}), /콜백 누락/, `kind ${kind}`);
    }
    const sprites = [];
    const groupOnly = { group: () => ({ token: 0, entries: member, count: { value: 1 } }) };
    assert.throws(() => dispatchDrawEntries(entry({ kind: 1 }), state, groupOnly), /콜백 누락/);
    assert.throws(() => dispatchDrawEntries(entry({ kind: 1 }), state, { sprite: (item) => sprites.push(item) }), /콜백 누락/);
    assert.equal(sprites.length, 0);
    assert.throws(() => dispatchDrawEntries(entry({ kind: 200 }), { hideGroups: false, debugAfterEach: true }, {}), /콜백 누락/);
});
