// Renderer 프레임 앞단 render/raw-renderer-prelude.js 를 원본 10.78 관측(renderer-prelude-x86.tsv 720행)과 cpppj `RendererPreludeTests.cpp` 의 선택 장치 시험, 웹 계약으로 검사한다.
//  - 관측 한 행 = 별개의 화면/Renderer/구름·스캔 상태 한 건이다: 입력 19열(화면 flags·이전 flags·세계 요청·override 반환·폭/높이/초기 높이/카메라 Y/지도 clip 네 값·scan index·초기화 표식·몇 번째 행 조회에서 dirty 를 낼지·
//    하위 효과 변형 번호·시차 배경 상태·일반 dirty 개수·구름 dirty 개수), 출력 35 DWORD(scan 19·현재 전역 11·배경/일반 dirty/구름 dirty 개수·구름 전체/부분 표시 요청 2)와 호출 순서(M·F·O·U·Q·A·B + 원시 인수).
//    하위 효과(화면 flags 변경·세계 요청 변경·override 변경·행 index 변경 등)는 명시한 변형 번호의 호출 직후에만 입력으로 공급하고 상위 계산은 이 모듈이 낸 결과를 원본 출력과 비교한다.
//  - cpppj 계약 시험: 구름 비표시/override 조기 생략은 연결하지 않은 하위를 요구하지 않고, 선택된 첫 구름 dirty 표시가 없으면 거기서 멈추며 뒤 counter 0·표 지우기를 가짜로 실행하지 않는다.
//  - 웹 계약: 인접 물리 범위(index 가 표 밖이면 거부)·19칸이 아니면 거부·음수 index 의 signed 나머지·행 표 최초 구성 값과 한 번만 구성.
// 실제 게임·브라우저 실행이 아니다. (2026-11-13 WB04-E383)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';
import { prepareRendererFrame, RENDERER_PRELUDE_SCAN_SIZE } from '../www/render/raw-renderer-prelude.js';
import { markKencloudDirty } from '../www/core/raw-kencloud-render.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-13 WB04-E383)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'renderer-prelude-x86.tsv'));
// 같은 배열 칸을 현재 참조로 빌리는 상자이다. (2026-11-13 WB04-E383)
const cell = (array, index) => ({ get value() { return array[index]; }, set value(next) { array[index] = next; } });
// 사각형 직렬화(부호 없는 DWORD 네 개)이다. (2026-11-13 WB04-E383)
const text = (rect) => [rect.left, rect.top, rect.right, rect.bottom].map((value) => value >>> 0).join(',');

// 한 관측 행을 재생해 출력 35 DWORD 와 호출 순서를 반환한다. (2026-11-13 WB04-E383)
function replay(row) {
    const input = row.slice(0, 19).map(Number);
    const expected = row.slice(19, 54).map(Number);
    const word = (index) => input[index] >>> 0;
    const scan = new Uint32Array(RENDERER_PRELUDE_SCAN_SIZE);
    scan[0] = word(12);
    scan[17] = word(13);
    scan[18] = word(1);
    // 원문 초기화와 무관한 기존 16행 입력이다.
    for (let i = 0; i < 16; i += 1) scan[i + 1] = (i * 7 - 11) >>> 0;
    // 0 mode · 1 force · 2 override 반환 · 3 폭 · 4 높이 · 5 초기 높이 · 6 카메라 Y · 7~10 지도 clip(left/top/right/bottom).
    const globals = new Uint32Array([word(0), word(2), word(3), word(4), word(5), word(6), word(7), word(8), word(9), word(10), word(11)]);
    const storage = new Uint32Array([word(16), word(17), word(18)]);
    const cloud = new Uint32Array(2054);
    cloud[2051] = 17;
    cloud[2052] = 19;
    let queries = 0;
    const events = [];
    const changed = new Set();
    // 명시한 변형 번호의 호출 직후에만 하위 효과를 입력으로 공급한다.
    const effect = (label) => {
        if (changed.has(label)) return;
        changed.add(label);
        const kind = input[15];
        if (kind === 1 && label === 'M') { globals[0] = 0x38; globals[3] = 801; globals[1] = 1; }
        else if (kind === 2 && label === 'F') { globals[0] = 0x29; globals[4] = 611; }
        else if (kind === 3 && label === 'O') globals[0] = 0;
        else if (kind === 4 && label === 'U') globals[0] = 0x19;
        else if (kind === 5 && label === 'Q') { scan[0] = 15; globals[0] = 0x19; }
        else if (kind === 6 && label === 'A') { globals[0] = 0; globals[1] = 0; }
        else if (kind === 7 && label === 'Q') { scan[0] = 2; scan[3] = 100; globals[3] = 704; globals[7] = 17; }
        else if (kind === 8 && label === 'B') { globals[1] = 0; storage[0] = 17; }
    };
    const hooks = {
        markCloudDirty: () => { events.push('M'); effect('M'); markKencloudDirty(cloud); },
        queueCloudDirty: (rect) => { events.push(`F:${text(rect)}`); effect('F'); },
        overrideActive: () => { events.push('O'); effect('O'); return globals[2]; },
        updateClouds: () => { events.push('U'); effect('U'); },
        rowDirty: (y, cameraY) => { queries += 1; events.push(`Q:${y >>> 0},${cameraY >>> 0}`); effect('Q'); return queries === word(14) ? 1 : 0; },
        invalidate: (rect, flags) => { events.push(`A:${text(rect)},${flags}`); effect('A'); },
        invalidateBackground: () => { events.push('B'); effect('B'); },
    };
    prepareRendererFrame(
        { scan, backgroundRedraw: cell(storage, 0), dirtyCount: cell(storage, 1), drawCounter: cell(storage, 2) },
        { mode: cell(globals, 0), force: cell(globals, 1), width: cell(globals, 3), height: cell(globals, 4), initialHeight: cell(globals, 5), cameraY: cell(globals, 6), left: cell(globals, 7), top: cell(globals, 8), right: cell(globals, 9), bottom: cell(globals, 10) },
        hooks,
    );
    return { actual: [...scan, ...globals, storage[0], storage[1], storage[2], cloud[2051], cloud[2052]], expected, events: events.length ? events.join(';') : '-', expectedEvents: row[54] };
}

test('Renderer 앞단: 원본 10.78 관측 720행의 scan·현재 전역·카운터·구름 요청과 호출 순서가 모두 일치한다', { skip: rows === null }, () => {
    assert.equal(rows.length, 720);
    for (const row of rows) {
        assert.equal(row.length, 55);
        const { actual, expected, events, expectedEvents } = replay(row);
        assert.deepEqual(actual, expected, `관측 ${row.slice(0, 19).join(',')}`);
        assert.equal(events, expectedEvents, `호출 ${row.slice(0, 19).join(',')}`);
    }
});

// 최소한의 상태 묶음을 만든다(모든 칸 0, 폭 1024·높이 768). (2026-11-13 WB04-E383)
function bare(overrides = {}) {
    const globals = { mode: { value: 0 }, force: { value: 0 }, width: { value: 1024 }, height: { value: 768 }, initialHeight: { value: 768 }, cameraY: { value: 0 }, left: { value: 84 }, top: { value: 0 }, right: { value: 1024 }, bottom: { value: 768 }, ...overrides };
    const state = { scan: new Uint32Array(19), backgroundRedraw: { value: 7 }, dirtyCount: { value: 9 }, drawCounter: { value: 11 } };
    return { state, globals };
}

test('Renderer 앞단: 구름 비표시/override 조기 생략은 연결하지 않은 하위를 요구하지 않고 선택된 첫 장치가 없으면 거기서 멈춘다(RendererPrelude_SelectedDevicesAndPartialFailure)', () => {
    const { state, globals } = bare();
    prepareRendererFrame(state, globals, {});
    assert.deepEqual([state.drawCounter.value, state.backgroundRedraw.value, state.dirtyCount.value], [0, 7, 9]);
    globals.mode.value = 8;
    state.scan[18] = 8;
    state.drawCounter.value = 17;
    const hooks = { overrideActive: () => 19 };
    prepareRendererFrame(state, globals, hooks);
    assert.deepEqual([state.drawCounter.value, state.scan[0], state.scan[17]], [0, 0, 0]);
    state.drawCounter.value = 23;
    globals.force.value = 1;
    assert.throws(() => prepareRendererFrame(state, globals, hooks), /선택된 현재 장치 미연결/);
    // 선택된 첫 MarkDirty 누락에서 멈추며 뒤 counter 0·표 지우기를 가짜로 실행하지 않는다.
    assert.deepEqual([state.drawCounter.value, state.dirtyCount.value], [23, 9]);
});

test('Renderer 앞단 계약: 행 표는 19칸이어야 하고 index 가 표 밖이면 인접 물리 범위로 거부하며 최초 구성은 한 번만 일어난다', () => {
    const { state, globals } = bare({ mode: { value: 0x28 } });
    state.scan[18] = 0x28;
    const hooks = { overrideActive: () => 0, updateClouds: () => {}, rowDirty: () => 0, invalidate: () => {}, markCloudDirty: () => {}, queueCloudDirty: () => {}, invalidateBackground: () => {} };
    assert.throws(() => prepareRendererFrame({ ...state, scan: new Uint32Array(18) }, globals, hooks), RangeError);
    // 행 표 최초 구성: 초기 높이 768 기준 값과 표식 비트.
    prepareRendererFrame(state, globals, hooks);
    const built = Array.from(state.scan);
    assert.deepEqual(built.slice(1, 17), [0, 738, 120, 618, 30, 708, 150, 588, 60, 678, 180, 558, 90, 648, 210, 528]);
    assert.equal(built[17], 1);
    // 구성된 뒤에는 초기 높이를 바꿔도 다시 만들지 않고 index 만 (index+1)%16 으로 움직인다.
    globals.initialHeight.value = 100;
    const before = Array.from(state.scan);
    prepareRendererFrame(state, globals, hooks);
    assert.deepEqual(Array.from(state.scan).slice(1, 18), before.slice(1, 18));
    assert.equal(state.scan[0], 2);
    // index 가 표 밖이면(물리 19칸 밖) 인접 읽기 범위 오류이다. 음수는 index-1 의 인접 칸(스캔 0)까지만 허용한다.
    state.scan[0] = 18;
    assert.throws(() => prepareRendererFrame(state, globals, hooks), /인접 물리 범위/);
    state.scan[0] = 0xffffffff;
    prepareRendererFrame(state, globals, hooks);
    assert.equal(state.scan[0], 0);
    state.scan[0] = 0xfffffffe;
    assert.throws(() => prepareRendererFrame(state, globals, hooks), /인접 물리 범위/);
});

test('Renderer 앞단 계약: override 반환 1 도 모달 위 모드이고 행 구간 경계(signed 덧셈 감김)와 지도 clip 의 왼쪽 0/양수가 그대로 나온다', () => {
    const calls = [];
    const hooks = {
        overrideActive: () => 1, updateClouds: () => calls.push('U'), rowDirty: () => 1, markCloudDirty: () => calls.push('M'), queueCloudDirty: () => {}, invalidateBackground: () => {},
        invalidate: (rect, flags) => calls.push(`A:${rect.left},${rect.top},${rect.right},${rect.bottom},${flags}`),
    };
    // 반환 1 은 override 활성이라 구름 갱신/행 스캔/구름 dirty 를 모두 건너뛴다.
    const active = bare({ mode: { value: 0x28 } });
    active.state.scan[18] = 0x28;
    prepareRendererFrame(active.state, active.globals, hooks);
    assert.deepEqual(calls, []);
    // 반환 0 이면 구름 갱신 → 행 조회 → dirty 행의 30행 사각형(지도 clip 으로 자름) → 구름 dirty 표시 순이다.
    hooks.overrideActive = () => 0;
    for (const [clipLeft, expectedLeft] of [[0, 0], [84, 84], [5, 5]]) {
        calls.length = 0;
        const { state, globals } = bare({ mode: { value: 0x28 }, left: { value: clipLeft } });
        state.scan[18] = 0x28;
        state.scan[17] = 1;
        state.scan[0] = 0;
        state.scan[1] = 40;
        prepareRendererFrame(state, globals, hooks);
        assert.deepEqual(calls, ['U', `A:${expectedLeft},40,1024,70,0`, 'M'], `clip 왼쪽 ${clipLeft}`);
        assert.equal(state.scan[0], 1);
    }
    // 행 값이 INT_MAX-30 이면 구간(행+30)이 양수라 조회하고, 하나 더 크면 감겨 음수가 되어 조회하지 않는다.
    for (const [row, queried] of [[0x7fffffe1, true], [0x7fffffe2, false]]) {
        calls.length = 0;
        const { state, globals } = bare({ mode: { value: 0x28 } });
        state.scan[18] = 0x28;
        state.scan[17] = 1;
        state.scan[1] = row;
        let asked = 0;
        prepareRendererFrame(state, globals, { ...hooks, rowDirty: () => { asked += 1; return 0; } });
        assert.equal(asked > 0, queried, `행 ${row.toString(16)}`);
    }
});

test('Renderer 앞단 계약: 세계 redraw 요청은 일반 dirty 개수를 지우고(호스트 표를 쓰면 clearDirty) 새 전체 사각형을 추가하며 요청은 지우지 않는다', () => {
    const calls = [];
    const { state, globals } = bare({ force: { value: 1 } });
    state.scan[18] = 0;
    const hooks = {
        markCloudDirty: () => calls.push('M'), queueCloudDirty: (rect) => calls.push(`F:${rect.right},${rect.bottom}`),
        invalidate: (rect, flags) => calls.push(`A:${rect.left},${rect.top},${rect.right},${rect.bottom},${flags}:${state.dirtyCount.value}`),
        clearDirty: () => calls.push(`C:${state.dirtyCount.value}`),
    };
    prepareRendererFrame(state, globals, hooks);
    // 구름 dirty 두 효과(요청) → 개수 0 → clearDirty → 전체 사각형 추가 순이다.
    assert.deepEqual(calls, ['M', 'F:1024,768', 'C:0', 'A:0,0,1024,768,0:0']);
    assert.deepEqual([state.dirtyCount.value, state.drawCounter.value, globals.force.value], [0, 0, 1]);
    // clearDirty 가 없어도(직접 DWORD 표를 빌린 호출자) 같다.
    calls.length = 0;
    state.dirtyCount.value = 9;
    delete hooks.clearDirty;
    prepareRendererFrame(state, globals, hooks);
    assert.deepEqual(calls, ['M', 'F:1024,768', 'A:0,0,1024,768,0:0']);
});
