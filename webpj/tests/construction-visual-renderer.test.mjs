// 건설 표시 렌더러 어댑터 render/construction-visual-renderer.js 를 cpppj `ConstructionVisualSystemTests.cpp` 의 client 렌더러 시험 5건(10.78 판)과 웹 계약으로 검사한다.
//  - E194(`construction-visual-system.test.mjs`)가 `o/` 부품만으로 옮기고 미룬 것: RenderCollection_VisibleConstructionSubmission(가시 영역 선별·signed 깊이 정렬·카메라 Draw·용량 제한·정렬 끔)·
//    RenderCollection_SmallTiesAndInvalidSortCoordinates(작은 구간 동률 순서와 비유한 좌표 거부)·DrawSort_ConstructionUsesNativeTieOrder(9항목 동률의 원본 CRT 순서, drawsort-x86.tsv originals 한 행)·
//    DrawSubmission_ActualConstructionMergesWithOtherKinds(UI/묶음/자산/미리보기와 한 번 정렬되며 원래 출력 버퍼를 그림)·DrawSubmission_ConstructionDeletionDuringMixedDispatch(앞선 종류의 Draw 가 토큰을 지워도 현재 슬롯의 새 토큰을 섞지 않음).
//    10.72(CD) 반복은 범위 밖이다. 장면 장치(World)는 `construction-visual-support.mjs` 를 E194 시험과 공유한다.
//  - 웹 계약: ConstructionVisualSystem 과 RenderProcessSystem 두 대상(C++ 오버로드)이 같은 결과, 잘못된 대상 거부, 호출자의 수집기/분배 훅 묶음은 바뀌지 않고 construction 훅은 system 으로 교체,
//    drawVisibleConstruction 은 수집 영역을 그대로 넘기고 dirty 0(건설 수집 항상 실행)이며 작업 개수를 한도로 넘김, 32비트 초과 작업 버퍼 거부, 정렬 끔은 수집 순서 보존.
// 실제 게임·장치 실행이 아니다. (2026-11-13 WB04-E382)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put } from './raw-support.mjs';
import { RenderProcessSystem } from '../www/core/render-process-system.js';
import { RENDER_ENTRY_BYTES } from '../www/core/raw-render-collection.js';
import { drawVisibleConstruction, drawVisibleScene } from '../www/render/construction-visual-renderer.js';
import { World, entryToken, released, throwsOf } from './construction-visual-support.mjs';

// 수집 항목 16바이트의 깊이(+12 int16)와 종류(+14)이다. (2026-11-13 WB04-E382)
const entryDepth = (work, index) => new DataView(work.buffer, work.byteOffset).getInt16(index * RENDER_ENTRY_BYTES + 12, true);
const entryKind = (work, index) => work[index * RENDER_ENTRY_BYTES + 14];
// 항목 한 개(x·y·token·depth·kind·tail)를 16바이트로 만든다. (2026-11-13 WB04-E382)
function makeEntry(token, depth, kind, tail = 0) {
    const bytes = new Uint8Array(RENDER_ENTRY_BYTES);
    const view = new DataView(bytes.buffer);
    view.setUint32(8, token, true);
    view.setInt16(12, depth, true);
    bytes[14] = kind;
    bytes[15] = tail;
    return bytes;
}
// 건설 표시 세 개를 같은 부모에 만들고 부모 zOrder(+35)를 7/0 으로 바꿔 깊이 7·0·-1 로 등록한 장면이다(cpppj 와 같은 순서). (2026-11-13 WB04-E382)
function threeProcesses(world) {
    const first = world.system.add(world.parent, false);
    world.pool.allocated(world.parent)[35] = 7;
    const second = world.system.add(world.parent, false);
    world.pool.allocated(world.parent)[35] = 0;
    const third = world.system.add(world.parent, false);
    world.hp(50);
    world.kernel.runFrame();
    return { first, second, third };
}
const VIEW = { left: 20, top: 20, right: 22, bottom: 23 };

test('건설 표시 렌더러: 가시 영역만 수집해 signed 깊이로 정렬하고 현재 카메라 Draw 에 제출하며 작업 용량과 정렬 끔을 지킨다(RenderCollection_VisibleConstructionSubmission)', () => {
    const world = new World();
    world.createSystem();
    const { first, second, third } = threeProcesses(world);
    const work = new Uint8Array(RENDER_ENTRY_BYTES * 3);
    assert.equal(drawVisibleConstruction(world.system, VIEW, work), 3);
    assert.equal(world.draws.length, 3);
    // 깊이 7 이 먼저, 그다음 0, -1 순이다.
    assert.deepEqual([entryToken(work, 0), entryToken(work, 1), entryToken(work, 2)], [second.registration.token, third.registration.token, first.registration.token]);
    assert.deepEqual([entryDepth(work, 0), entryDepth(work, 1), entryDepth(work, 2), entryKind(work, 0)], [7, 0, -1, 3]);
    // 영역 밖과 경계만 닿는 영역은 수집하지도 그리지도 않는다.
    world.draws.length = 0;
    assert.equal(drawVisibleConstruction(world.system, { left: 100, top: 100, right: 110, bottom: 110 }, work), 0);
    assert.equal(world.draws.length, 0);
    const right = first.registration.right / world.visualState.scaleX;
    assert.equal(drawVisibleConstruction(world.system, { left: right, top: 20, right: right + 1, bottom: 23 }, work), 0);
    assert.equal(world.draws.length, 0);
    // 카메라가 바뀌면 같은 항목이 새 화면 위치에 그려지고, 작업 버퍼가 모자라면 먼저 등록된 두 항목으로 제한한다(잘린 항목은 정렬에 참여하지 않음).
    world.frameState.cameraX = 4;
    world.frameState.cameraY = 5;
    assert.equal(drawVisibleConstruction(world.system, VIEW, work.subarray(0, RENDER_ENTRY_BYTES * 2)), 2);
    assert.equal(world.draws.length, 2);
    assert.deepEqual([entryToken(work, 0), entryToken(work, 1)], [second.registration.token, first.registration.token]);
    assert.deepEqual(world.draws[0], [322, 225]);
    // 정렬을 끄면 수집(등록) 순서를 그대로 쓴다.
    world.draws.length = 0;
    assert.equal(drawVisibleConstruction(world.system, VIEW, work, false), 3);
    assert.deepEqual([entryToken(work, 0), entryToken(work, 1), entryToken(work, 2)], [first.registration.token, second.registration.token, third.registration.token]);
});

test('건설 표시 렌더러: 작은 구간의 동률 두 항목은 등록 순서를 유지하고 비유한 좌표는 그리기 전에 거부한다(RenderCollection_SmallTiesAndInvalidSortCoordinates)', () => {
    const world = new World();
    world.createSystem();
    const first = world.system.add(world.parent, false);
    const second = world.system.add(world.parent, false);
    const work = new Uint8Array(RENDER_ENTRY_BYTES * 2);
    assert.equal(drawVisibleConstruction(world.system, VIEW, work), 2);
    assert.deepEqual([entryToken(work, 0), entryToken(work, 1)], [first.registration.token, second.registration.token]);
    world.draws.length = 0;
    // 부모 x 를 NaN(0x7fc00000)으로 바꾸면 정렬이 교환 전에 거부해 아무것도 그리지 않는다.
    put(world.pool.allocated(world.parent), 14, 0x7fc00000);
    assert.ok(throwsOf(() => drawVisibleConstruction(world.system, VIEW, work)));
    assert.equal(world.draws.length, 0);
});

test('건설 표시 렌더러: 9항목 완전 동률은 원본 10.78 CRT 정렬의 토큰 순서로 제출한다(DrawSort_ConstructionUsesNativeTieOrder, drawsort-x86.tsv originals 한 행)', () => {
    const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'drawsort-x86.tsv'));
    assert.ok(rows, 'drawsort-x86.tsv 없음');
    const world = new World();
    world.createSystem();
    // 같은 부모의 같은 깊이/좌표를 갖는 표시 객체 9개를 등록한다(토큰은 원본 관찰과 같은 1~9 순서).
    for (let index = 0; index < 9; index += 1) world.system.add(world.parent, false);
    const work = new Uint8Array(RENDER_ENTRY_BYTES * 9);
    assert.equal(drawVisibleConstruction(world.system, VIEW, work), 9);
    assert.equal(world.draws.length, 9);
    let found = 0;
    // 원본 저장 관찰의 동률 행에서 최종 토큰만 읽는다. 기대 정렬은 계산하지 않는다.
    for (const row of rows) {
        if (row[0] !== 'originals' || row[1] !== 'Sort' || !row[2].startsWith('9,0,')) continue;
        found += 1;
        for (let index = 0; index < 9; index += 1) {
            const bytes = Buffer.from(row[4].slice(index * 32 + 16, index * 32 + 24), 'hex');
            assert.equal(entryToken(work, index), bytes.readUInt32LE(0), `항목 ${index}`);
        }
    }
    assert.equal(found, 1);
});

test('건설 표시 렌더러: 실제 건설 두 프로세스가 UI/묶음/자산/미리보기와 한 번 정렬되며 공통 분배가 원래 출력 버퍼를 그린다(DrawSubmission_ActualConstructionMergesWithOtherKinds)', () => {
    const world = new World();
    const drawn = [];
    let firstPixels = null;
    const frame = world.frameHooks();
    const draw = frame.draw;
    frame.draw = (request) => { draw(request); drawn.push(request.pixels === firstPixels ? 'C1' : 'C2'); };
    world.createSystem(world.visualHooks(), frame, world.registryHooks());
    const first = world.system.add(world.parent, false);
    firstPixels = first.fields.visual.output;
    world.pool.allocated(world.parent)[35] = 7;
    const second = world.system.add(world.parent, false);
    world.hp(50);
    world.kernel.runFrame();
    // 묶음 내부 항목 두 개(sprite 77·미리보기 종류 78)와 개수 상자.
    const members = Uint8Array.from([...makeEntry(77, 20, 0), ...makeEntry(78, 100, 4, 2)]);
    const count = { value: 2 };
    const groups = [{ token: 500, entries: members, count }];
    const collect = {
        ui: (rect, output, limit) => { assert.equal(limit, 10); output.set(makeEntry(99, 30, 2)); return 1; },
        groups: (rect, rowLimit, covered) => { assert.equal(rowLimit, 17); covered.value = true; return groups; },
        objects: (rect, output, limit) => { assert.equal(limit, 6); output.set(makeEntry(103, 5, 0)); return 1; },
        preview: (rect, output, limit) => { assert.equal(limit, 5); output.set(makeEntry(104, -30, 4, 2)); return 1; },
    };
    const tokenOf = (entry) => new DataView(entry.buffer, entry.byteOffset, entry.byteLength).getUint32(8, true);
    const dispatch = {
        ui: (token) => { drawn.push(`U${token}`); },
        sprite: (entry) => { drawn.push(`S${tokenOf(entry)}`); },
        group: (token) => { assert.equal(token, 500); return groups[0]; },
        preview: (entry) => { assert.equal(entry[15], 2); drawn.push(`P${tokenOf(entry)}`); },
    };
    const work = new Uint8Array(RENDER_ENTRY_BYTES * 10);
    const request = { screen: { left: 0, top: 0, right: 500, bottom: 500 }, visible: { left: 20, top: 20, right: 22, bottom: 23 }, unclipped: { left: 19, top: 19, right: 24, bottom: 24 }, dirtyFlags: 0, groupRowLimit: 17 };
    const result = drawVisibleScene(world.system, request, work, collect, {}, dispatch);
    assert.deepEqual([result.count, result.backgroundCovered, world.draws.length], [6, true, 2]);
    assert.deepEqual(drawn, ['U99', 'S77', 'S78', 'C2', 'S103', 'C1', 'P104']);
    assert.equal(entryToken(work, 2), second.registration.token);
    assert.equal(entryToken(work, 4), first.registration.token);
    // 호출자의 훅 묶음은 바뀌지 않는다(construction 은 사본에만 연결).
    assert.deepEqual(Object.keys(collect), ['ui', 'groups', 'objects', 'preview']);
    assert.deepEqual(Object.keys(dispatch), ['ui', 'sprite', 'group', 'preview']);
    // 정렬을 끄면 수집 순서(UI → 건설 → 묶음 → 일반 → 미리보기)를 그대로 쓴다.
    drawn.length = 0;
    world.draws.length = 0;
    assert.equal(drawVisibleScene(world.system, request, work, collect, {}, dispatch, false).count, 6);
    assert.deepEqual(drawn, ['U99', 'C1', 'C2', 'S77', 'S78', 'S103', 'P104']);
    // dirtyFlags 비트 2 는 UI 만 수집한다.
    drawn.length = 0;
    world.draws.length = 0;
    request.dirtyFlags = 4;
    const uiOnly = drawVisibleScene(world.system, request, work, collect, {}, dispatch);
    assert.deepEqual([uiOnly.count, uiOnly.backgroundCovered, drawn, world.draws.length], [1, false, ['U99'], 0]);
});

test('건설 표시 렌더러: 앞선 종류의 Draw 가 건설 토큰을 지워도 현재 슬롯의 새 토큰을 이전 목록에 섞지 않는다(DrawSubmission_ConstructionDeletionDuringMixedDispatch)', () => {
    const world = new World();
    world.createSystem();
    const first = world.system.add(world.parent, false);
    const old = first.registration.token;
    let replacement = null;
    const collect = { ui: (rect, output) => { output.set(makeEntry(99, 30, 2)); return 1; } };
    const dispatch = {
        ui: () => {
            world.host.kill(first, 0);
            replacement = world.system.add(world.parent, false);
        },
    };
    const work = new Uint8Array(RENDER_ENTRY_BYTES * 2);
    const request = { screen: { left: 0, top: 0, right: 0, bottom: 0 }, visible: VIEW, unclipped: VIEW, dirtyFlags: 0, groupRowLimit: 0 };
    assert.equal(drawVisibleScene(world.system, request, work, collect, {}, dispatch).count, 2);
    assert.equal(world.draws.length, 0);
    assert.ok(replacement && replacement.registration.token !== old && entryToken(work, 1) === old && released(world.released, 1));
    world.system.drawToken(replacement.registration.token);
    assert.equal(world.draws.length, 1);
});

test('건설 표시 렌더러 계약: 건설 표시 시스템과 공통 표시 저장소 두 대상이 같은 결과를 내고 다른 대상은 거부한다', () => {
    const world = new World();
    world.createSystem();
    threeProcesses(world);
    const viaSystem = new Uint8Array(RENDER_ENTRY_BYTES * 3);
    const viaRendering = new Uint8Array(RENDER_ENTRY_BYTES * 3);
    assert.equal(drawVisibleConstruction(world.system, VIEW, viaSystem), 3);
    assert.equal(world.draws.length, 3);
    world.draws.length = 0;
    assert.ok(world.system.rendering instanceof RenderProcessSystem);
    assert.equal(drawVisibleConstruction(world.system.rendering, VIEW, viaRendering), 3);
    assert.equal(world.draws.length, 3);
    assert.deepEqual(viaSystem, viaRendering);
    for (const bad of [null, undefined, {}, { rendering: {} }, { rendering: null }, 5]) {
        assert.throws(() => drawVisibleConstruction(bad, VIEW, viaSystem), TypeError, String(bad));
        if (bad !== null && bad !== undefined && typeof bad === 'object') assert.throws(() => drawVisibleConstruction(bad, VIEW, viaSystem), /수집 대상 오류/, String(bad));
    }
    assert.throws(() => drawVisibleScene({}, {}, viaSystem, {}, {}, {}), /수집 대상 오류/);
});

test('건설 표시 렌더러 계약: drawVisibleConstruction 은 수집 영역을 그대로 넘기고 작업 개수를 한도로 넘기며 construction 훅만 시스템으로 교체한다', () => {
    const calls = [];
    // 공통 표시 저장소의 수집/토큰 Draw 만 가진 가짜 대상이다(instanceof 를 통과하는 빈 객체).
    const fake = Object.create(RenderProcessSystem.prototype);
    fake.collect = (view, output, limit) => { calls.push(['collect', view, output.length, limit]); output.set(makeEntry(5, 3, 3)); output.set(makeEntry(6, 9, 3), RENDER_ENTRY_BYTES); return 2; };
    fake.drawToken = (token) => { calls.push(['draw', token]); };
    const view = { left: 1, top: 2, right: 3, bottom: 4 };
    const work = new Uint8Array(RENDER_ENTRY_BYTES * 4).fill(0x77);
    assert.equal(drawVisibleConstruction(fake, view, work), 2);
    // 한도는 작업 개수(4)이고 깊이 9 가 먼저 그려진다. 수집 밖 바이트는 그대로이다.
    assert.deepEqual(calls, [['collect', view, RENDER_ENTRY_BYTES * 4, 4], ['draw', 6], ['draw', 5]]);
    assert.ok(work.subarray(RENDER_ENTRY_BYTES * 2).every((byte) => byte === 0x77));
    calls.length = 0;
    assert.equal(drawVisibleConstruction(fake, view, work, false), 2);
    assert.deepEqual(calls.slice(1), [['draw', 5], ['draw', 6]]);
    // 호출자가 준 construction 수집기/분배기는 시스템의 것으로 교체된다.
    calls.length = 0;
    let replaced = 0;
    const request = { screen: { left: 0, top: 0, right: 0, bottom: 0 }, visible: view, unclipped: view, dirtyFlags: 0, groupRowLimit: 0 };
    const collection = { construction: () => { replaced += 1; return 0; } };
    const dispatch = { construction: () => { replaced += 1; } };
    assert.equal(drawVisibleScene(fake, request, work, collection, {}, dispatch).count, 2);
    assert.equal(replaced, 0);
    assert.deepEqual([collection.construction !== undefined, dispatch.construction !== undefined], [true, true]);
    // 32비트 signed 개수를 넘는 작업 버퍼는 수집 전에 거부한다.
    calls.length = 0;
    assert.throws(() => drawVisibleConstruction(fake, view, { length: RENDER_ENTRY_BYTES * 2 ** 31 }), RangeError);
    assert.deepEqual(calls, []);
});

test('건설 표시 렌더러 계약: 작업 버퍼가 32비트 signed 최대 개수와 같으면 허용하고 하나 더 많으면 거부하며 건설 수집은 dirty 0 으로 항상 일어난다', () => {
    const view = { left: 1, top: 2, right: 3, bottom: 4 };
    const fake = Object.create(RenderProcessSystem.prototype);
    const calls = [];
    fake.collect = (area, output, limit) => { calls.push([area, output.length, limit]); return 0; };
    fake.drawToken = () => { throw new Error('수집이 없으면 그리지 않는다'); };
    // 가짜 작업 버퍼: 최대 개수(2^31-1)는 통과해 원본 용량 5,000 항목만 수집기에 준다.
    const largest = { length: RENDER_ENTRY_BYTES * (2 ** 31 - 1), subarray: (begin, end) => new Uint8Array(end - begin) };
    assert.equal(drawVisibleConstruction(fake, view, largest), 0);
    assert.deepEqual(calls, [[view, 5000 * RENDER_ENTRY_BYTES, 5000]]);
    const tooLarge = { length: RENDER_ENTRY_BYTES * 2 ** 31, subarray: () => { throw new Error('도달 불가'); } };
    assert.throws(() => drawVisibleConstruction(fake, view, tooLarge), /크기 오류/);
    // 건설 수집은 항상 일어난다(dirty 비트 2 가 서 있었다면 건너뛰었을 것).
    assert.equal(calls.length, 1);
});
