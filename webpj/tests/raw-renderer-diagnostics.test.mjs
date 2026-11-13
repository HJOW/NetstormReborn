// Renderer 후단 진단 render/raw-renderer-diagnostics.js 를 원본 10.78 관측(renderer-diagnostics-x86.tsv 252행·renderer-diagnostic-keys-x86.tsv 576행)과 cpppj `RendererDiagnosticsTests.cpp` 의 계약으로 검사한다.
//  - 진단 252행(504회 관측): Hash/Fire(모드 0·1)/Spot/Board 와 flag 0/비영 DWORD 를 독립 입력 6열 [종류·flag·fireMode·spot 채움 BYTE·거리표 채움 값·칸 크기 종류]로 재생해 96×80 전체 표면(행 padding 100)·최종 clip·배경 쓰기·호출 순서
//    (Begin·Clip·Rect·Line·Lock·Unlock·Present·Assert)가 원본과 같다. 여섯 합성 객체(SID 50·60~64, 상태 0/1/2/4/8/6, 표면·graph BYTE 입력)를 실제 네 단계 해시에 걸고 순회는 제품이 한다. 기대 좌표/픽셀은 계산하지 않는다.
//  - 키 576행: 7/16/19 키(수식·outside 마스크 포함)와 정지/Console 상태의 DWORD 토글·redraw·Shift-P 의 기존 좌표 로그 지연 호출.
//  - cpppj 계약 시험: flag 0 은 카메라/실제 장치/장기 저장소를 요구하지 않음(`RendererDiagnostics_DisabledAndLongOwnerNoSyntheticActivation` 중 렌더러 쪽; 장기 저장소 `ClientGameplayGlobals` 주소 해석은 그 전역 소유자 단계) .
//    `RendererDiagnostics_SharedPathWorkSurvivesSearchOwnerReplacement`(거리표를 두 탐색기가 이어 빌림)는 RawPathSearch 소유 저장소 시험이라 웹은 E334 path-search 쪽에 이미 있다.
//  - 웹 계약: 선택한 효과에만 장치를 요구, 색은 매번 현재 배열에서 읽음, 물리 범위 오류, Fire 모드의 8방향 거리 WORD 비교와 진단 0x1f9, Spot 의 중복 표식 진단 0xfe, Board/Spot 색 번호.
// 실제 게임·브라우저 실행이 아니다. (2026-11-13 WB04-E384)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { CameraView } from '../www/render/camera-view.js';
import { drawVfxLinePixels } from '../www/render/vfx-line.js';
import { InputCode } from '../www/core/input-queue.js';
import { drawWaterRectangle, drawWaterVisibleArea } from '../www/core/draw-water.js';
import { RawRendererDiagnostics, RendererDiagnosticKind, dispatchRendererDiagnosticKey } from '../www/render/raw-renderer-diagnostics.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-13 WB04-E384)
const fixture = (name) => readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', name));
const diagnosticRows = fixture('renderer-diagnostics-x86.tsv');
const keyRows = fixture('renderer-diagnostic-keys-x86.tsv');
const WIDTH = 96;
const HEIGHT = 80;
const PITCH = 100;
const box = (value) => ({ value });
const unhex = (text) => Uint8Array.from(Buffer.from(text, 'hex'));
// 사각형 직렬화이다. (2026-11-13 WB04-E384)
const rectText = (rect) => `${rect.left}:${rect.top}:${rect.right}:${rect.bottom}`;

// 원본과 같은 여섯 합성 객체·실제 네 단계 해시·메모리 화면·장치 추적을 가진 입력 한 건이다. (2026-11-13 WB04-E384)
function createDrawing(input) {
    const drawing = {
        pool: new SidPool(24000, { server: false }), hash: new SquidHash(), types: Array.from({ length: 256 }, () => ({ footX: 0, footY: 0, flags1: 0 })),
        spots: new Uint8Array(65536).fill(Number(input[3])), distances: Number(input[4]) ? new Int16Array(65536).fill(Number(input[4])) : new Int16Array(0),
        renderer: new CameraView(), snapshot: new CameraView(), kind: Number(input[0]), flag: box(Number(input[1])), mode: box(Number(input[2])), background: box(73), worldCells: box(256),
        clip: { left: 5, top: 7, right: 91, bottom: 73 }, pixels: new Uint8Array(PITCH * HEIGHT).fill(199), colors: [10, 11, 12, 13, 14, 15, 16, 17, 18], trace: [],
    };
    const cell = Number(input[5]) ? [32, 22] : [16, 11];
    drawing.renderer.data.setFloat32(0, 19.25, true);
    drawing.renderer.data.setFloat32(4, 19.5, true);
    for (const view of [drawing.renderer, drawing.snapshot]) {
        view.data.setInt32(8, cell[0], true);
        view.data.setInt32(12, cell[1], true);
    }
    drawing.snapshot.data.setInt32(28, 304, true);
    drawing.snapshot.data.setInt32(32, 209, true);
    const ids = [50, 60, 61, 62, 63, 64];
    const states = [0, 1, 2, 4, 8, 6];
    const graphs = [2, 0, 254, 1, 0, 3];
    // 입력 순서의 실제 해시 머리만 만들고 순회는 제품에 맡긴다.
    ids.forEach((id, i) => {
        const x = 20 + (i % 3) + 0.25;
        const y = 20 + Math.floor(i / 3) + 0.5;
        const raw = drawing.pool.physical(id);
        raw.fill(0);
        raw[10] = 100 + i;
        raw[11] = states[i];
        raw[12] = i & 1;
        raw[30] = graphs[i];
        put(raw, 14, floatToBits(x));
        put(raw, 18, floatToBits(y));
        drawing.types[100 + i].footX = 1 + (i % 3);
        drawing.types[100 + i].footY = 1 + (i % 2);
        drawing.types[100 + i].flags1 = i % 2 ? 0 : 0x800;
        put(raw, 4, drawing.hash.bucket(i % 4, x, y), 2);
        drawing.hash.setBucket(i % 4, x, y, id);
    });
    const add = (value) => drawing.trace.push(value);
    drawing.hooks = {
        screen: {
            clip: (rect) => { add(`Clip:${rectText(rect)}`); drawing.clip = rect; },
            rectangle: (rect, ink) => {
                add(`Rect:${rectText(rect)}:${ink}`);
                const saved = drawing.clip;
                drawing.clip = { left: 0, top: 0, right: WIDTH, bottom: HEIGHT };
                add(`Lock:${rectText(drawing.clip)}`);
                drawWaterRectangle(drawing.pixels, PITCH, HEIGHT, drawing.clip, rect, ink);
                add(`Unlock:${rectText(drawing.clip)}`);
                drawing.clip = saved;
            },
            present: () => { add('Present:0:0:96:80'); },
        },
        line: (first, second, ink) => {
            add(`Line:${first[0]}:${first[1]}:${second[0]}:${second[1]}:${ink}`);
            const saved = drawing.clip;
            drawing.clip = { left: 0, top: 0, right: WIDTH, bottom: HEIGHT };
            add(`Lock:${rectText(drawing.clip)}`);
            drawVfxLinePixels(drawing.pixels, PITCH, HEIGHT, drawing.clip, { x: first[0], y: first[1] }, { x: second[0], y: second[1] }, ink);
            add(`Unlock:${rectText(drawing.clip)}`);
            drawing.clip = saved;
        },
        colors: () => drawing.colors,
        invalidGraph: () => 254,
        distances: () => drawing.distances,
        assertion: (line) => { add(`Assert:${line}`); },
    };
    drawing.draw = () => {
        // 실제 finder 입력값(가시 영역의 float 비트)만 기록하고 후보 순회는 같은 제품 본문이 한다.
        if (drawing.flag.value && drawing.kind === RendererDiagnosticKind.fire) {
            const area = drawWaterVisibleArea(drawing.renderer, WIDTH, HEIGHT);
            add(`Begin:${floatToBits(area.left)}:${floatToBits(area.top)}:${floatToBits(area.right)}:${floatToBits(area.bottom)}:0`);
        }
        new RawRendererDiagnostics(drawing.pool, drawing.hash, drawing.types, drawing.spots).draw(drawing.kind, drawing.renderer, drawing.snapshot, WIDTH, HEIGHT, drawing.flag, drawing.mode, drawing.worldCells, drawing.background, drawing.hooks);
    };
    return drawing;
}

// 변경 구간만 기록한 기대 표면을 만든다('-' 은 변화 없음). (2026-11-13 WB04-E384)
function expectedPixels(changed) {
    const result = new Uint8Array(PITCH * HEIGHT).fill(199);
    if (changed === '-') return result;
    for (const run of changed.split(';')) {
        const [offset, bytes] = run.split(':');
        result.set(unhex(bytes), Number(offset));
    }
    return result;
}

test('Renderer 진단: 원본 252행(Hash·Fire 두 모드·Spot·Board)의 96×80 전체 표면·clip·배경 쓰기·호출 순서가 모두 일치한다', { skip: !diagnosticRows && '원본 fixture 없음' }, () => {
    assert.equal(diagnosticRows.length, 252);
    for (const row of diagnosticRows) {
        assert.equal(row.length, 7);
        const input = row[0].slice(1, -1).split(',');
        assert.equal(input.length, 6);
        const drawing = createDrawing(input);
        drawing.draw();
        const expected = expectedPixels(row[1]);
        const mismatch = drawing.pixels.findIndex((value, index) => value !== expected[index]);
        assert.equal(mismatch, -1, `${row[0]} 픽셀 ${mismatch} 실제 ${drawing.pixels[mismatch]} 기대 ${expected[mismatch]}`);
        assert.equal(rectText(drawing.clip), row[2], row[0]);
        assert.equal(drawing.background.value, Number(row[6]), row[0]);
        assert.equal(drawing.trace.length === 0 ? '-' : drawing.trace.join(';'), row[5], row[0]);
    }
});

test('Renderer 진단 키: 원본 576행의 7/16/19 키 게이트·DWORD 토글·redraw·Shift-P 좌표 로그가 모두 일치한다', { skip: !keyRows && '원본 fixture 없음' }, () => {
    assert.equal(keyRows.length, 576);
    for (const row of keyRows) {
        assert.equal(row.length, 12);
        const initial = Number(row[3]) >>> 0;
        const state = { fire: box(initial), spot: box(initial), board: box(initial), redraw: box(Number(row[4]) >>> 0) };
        let logs = 0;
        // 정지(row[5])와 Console(row[6])은 서로 다르며 Console 이면 이 분배에 도달하지 않는다(원문 상위 관문).
        if (!Number(row[6])) assert.ok(dispatchRendererDiagnosticKey({ code: Number(row[0]) }, Number(row[1]), Number(row[2]), state, () => { logs += 1; }));
        const label = row.slice(0, 7).join(',');
        assert.deepEqual([state.fire.value >>> 0, state.spot.value >>> 0, state.board.value >>> 0, state.redraw.value >>> 0], [Number(row[7]), Number(row[8]), Number(row[9]), Number(row[10])], label);
        assert.equal(logs, row[11].includes('position-log') ? 1 : 0, label);
    }
});

test('Renderer 진단 계약: flag 0 은 카메라와 실제 장치를 요구하지 않고 어떤 전역도 바꾸지 않는다', () => {
    const pool = new SidPool(24000, { server: false });
    const background = box(71);
    const drawing = new RawRendererDiagnostics(pool, new SquidHash(), [], new Uint8Array(0));
    for (const kind of Object.values(RendererDiagnosticKind)) drawing.draw(kind, null, null, 0, 0, box(0), box(0), box(256), background, {});
    assert.equal(background.value, 71);
});

test('Renderer 진단 계약: 선택한 효과에만 장치를 요구하고 Hash 는 Present 없이 배경 쓰기, 나머지는 Present 로 끝난다', () => {
    const base = ['1', '1', '0', '0', '0', '0'];
    // 장치가 없으면 도달한 첫 위치에서 거부한다.
    for (const kind of [0, 1, 2, 3]) {
        const drawing = createDrawing([String(kind), '1', '0', kind === 2 ? '0x40' : '8', '0', '0'].map((value) => (value.startsWith('0x') ? String(Number(value)) : value)));
        assert.throws(() => new RawRendererDiagnostics(drawing.pool, drawing.hash, drawing.types, drawing.spots).draw(drawing.kind, drawing.renderer, drawing.snapshot, WIDTH, HEIGHT, drawing.flag, drawing.mode, drawing.worldCells, drawing.background, { screen: {} }), /실제 경계 미연결/, `kind ${kind}`);
    }
    // Hash 는 present 없이 background 에 1 을 쓴다.
    const hash = createDrawing(['0', '1', '0', '0', '0', '0']);
    hash.hooks.screen.present = () => assert.fail('Hash 는 즉시 Present 하지 않는다');
    hash.draw();
    assert.equal(hash.background.value, 1);
    // Board/Spot/Fire 는 Present 를 한 번 하고 끝난다.
    for (const kind of [1, 2, 3]) {
        const drawing = createDrawing([String(kind), '1', '0', '0', '0', '0']);
        drawing.draw();
        assert.equal(drawing.trace.filter((entry) => entry.startsWith('Present')).length, 1, `kind ${kind}`);
        assert.equal(drawing.background.value, 73, `kind ${kind}`);
    }
    assert.ok(base.length === 6);
});

test('Renderer 진단 계약: 색은 매번 현재 배열에서 읽고 범위 밖이면 거부하며 Spot 의 두 비트 중복은 진단 0xfe 를 알린다', () => {
    // Board: spot 비트 8 → 색 5, 비트 6 → 색 1. 색 배열을 바꾸면 다음 사각형부터 반영한다.
    const board = createDrawing(['3', '1', '0', '8', '0', '0']);
    let reads = 0;
    board.hooks.colors = () => { reads += 1; return board.colors.map((value) => value + 100 * reads); };
    board.hooks.screen.rectangle = (rect, ink) => { board.trace.push(`R:${ink}`); };
    board.draw();
    // 칸마다 색 3번을 매 사각형 직전에 읽는다(읽기 횟수가 사각형 수와 같다).
    assert.equal(board.trace.filter((entry) => entry.startsWith('R:')).length, reads);
    assert.ok(reads > 0);
    const short = createDrawing(['3', '1', '0', '8', '0', '0']);
    short.colors = [1, 2, 3];
    assert.throws(() => short.draw(), /색표 물리 범위/);
    // Spot: 0x40|0x80 이면 중복 표식 진단(0xfe) 뒤 색 0 으로 그린다.
    const spot = createDrawing(['2', '1', '0', String(0x40 | 0x80), '0', '0']);
    spot.draw();
    assert.ok(spot.trace.some((entry) => entry === 'Assert:254'));
    const only = createDrawing(['2', '1', '0', String(0x80), '0', '0']);
    only.draw();
    assert.ok(!only.trace.some((entry) => entry.startsWith('Assert')));
});

test('Renderer 진단 계약: Fire 모드의 연결선은 같은 거리 WORD 와 이웃을 비교하고 거리표가 모자라면 물리 범위로 거부한다', () => {
    const fire = createDrawing(['1', '1', '1', '0', '0', '0']);
    // 거리표가 비어 있으면(미할당) 연결선도 진단도 없다.
    fire.draw();
    assert.ok(!fire.trace.some((entry) => entry.startsWith('Line') || entry.startsWith('Assert')));
    // 같은 값으로 채운 거리표에서는 8방향 모두 같아 객체마다 8개 선을 색 7 로 그린다.
    const same = createDrawing(['1', '1', '1', '0', '5', '0']);
    same.draw();
    const lines = same.trace.filter((entry) => entry.startsWith('Line'));
    assert.equal(lines.length % 8, 0);
    assert.ok(lines.length > 0);
    assert.ok(lines.every((entry) => entry.endsWith(':17')));
    // 객체 좌표(20~22)가 거리표 칸 안이면 진단이 없다(0x1f9 는 finder 가 영역 밖 객체를 돌려주지 않아 정상 입력으로는 도달하지 않는 방어 진단이다).
    assert.ok(!same.trace.some((entry) => entry.startsWith('Assert')));
    // 거리표가 모자라면(현재 칸 위치가 배열 밖) 물리 범위로 거부한다. 서로 다른 이웃 값이면 선을 그리지 않는다.
    const short = createDrawing(['1', '1', '1', '0', '5', '0']);
    short.distances = new Int16Array(100).fill(5);
    assert.throws(() => short.draw(), /원문 배열 물리 범위/);
    const varied = createDrawing(['1', '1', '1', '0', '5', '0']);
    varied.distances = Int16Array.from({ length: 65536 }, (_, index) => index);
    varied.draw();
    assert.ok(!varied.trace.some((entry) => entry.startsWith('Line')));
});

test('Renderer 진단 키 계약: 7키는 보조 특수일 때만 Fire 토글, 10 키는 특수일 때 Shift 없으면 Spot 토글·Shift 면 좌표 로그, 13 키는 Board 토글이며 release 는 소비만 한다', () => {
    const make = () => ({ fire: box(0), spot: box(0), board: box(0), redraw: box(0) });
    const press = (code, special, auxiliary, state, log = null) => dispatchRendererDiagnosticKey({ code }, special, auxiliary, state, log);
    // 관심 밖 키는 소비하지 않는다.
    const none = make();
    assert.equal(press(8, 1, 1, none), false);
    // 7: 보조 특수가 없으면 소비만, 있으면 0→1(redraw 없음)→0(redraw 1).
    let state = make();
    assert.equal(press(7, 1, 0, state), true);
    assert.deepEqual([state.fire.value, state.redraw.value], [0, 0]);
    press(7, 0, 1, state);
    assert.deepEqual([state.fire.value, state.redraw.value], [1, 0]);
    press(7, 0, 1, state);
    assert.deepEqual([state.fire.value, state.redraw.value], [0, 1]);
    // 비영 전체 DWORD 도 켜진 상태이다: 토글하면 0 이 되고 redraw 를 쓴다.
    state = make();
    state.fire.value = 0x80000000;
    press(7, 0, 5, state);
    assert.deepEqual([state.fire.value, state.redraw.value], [0, 1]);
    // 0x10: 특수가 있어야 하고 Shift 없으면 Spot 은 항상 redraw 1, Shift 면 좌표 로그만.
    state = make();
    const logs = [];
    press(0x10, 0, 0, state, (event) => logs.push(event));
    assert.deepEqual([state.spot.value, state.redraw.value, logs.length], [0, 0, 0]);
    press(0x10, 1, 0, state, (event) => logs.push(event));
    assert.deepEqual([state.spot.value, state.redraw.value, logs.length], [1, 1, 0]);
    state.redraw.value = 0;
    press(0x10 | InputCode.shift, 1, 0, state, (event) => logs.push(event));
    assert.deepEqual([state.spot.value, state.redraw.value, logs.length], [1, 0, 1]);
    assert.throws(() => press(0x10 | InputCode.shift, 1, 0, state), /실제 경계 미연결/);
    // 0x13: 특수일 때 Board 토글이며 꺼질 때만 redraw 를 쓴다.
    state = make();
    press(0x13, 1, 0, state);
    assert.deepEqual([state.board.value, state.redraw.value], [1, 0]);
    press(0x13, 1, 0, state);
    assert.deepEqual([state.board.value, state.redraw.value], [0, 1]);
    // release 는 상태를 바꾸지 않고 소비한다.
    state = make();
    for (const code of [7, 0x10, 0x13]) assert.equal(press((code | InputCode.release) >>> 0, 1, 1, state), true);
    assert.deepEqual([state.fire.value, state.spot.value, state.board.value, state.redraw.value], [0, 0, 0, 0]);
});

// 보기 한 쌍(Renderer·snapshot)의 월드 원점과 칸 크기를 바꾼다. (2026-11-13 WB04-E384)
function setView(drawing, worldX, worldY, cellX = 16, cellY = 11) {
    drawing.renderer.data.setFloat32(0, worldX, true);
    drawing.renderer.data.setFloat32(4, worldY, true);
    for (const view of [drawing.renderer, drawing.snapshot]) {
        view.data.setInt32(8, cellX, true);
        view.data.setInt32(12, cellY, true);
    }
}
// 그리기 호출 기록 중 사각형만 모은다. (2026-11-13 WB04-E384)
const rects = (drawing) => drawing.trace.filter((entry) => entry.startsWith('Rect:')).map((entry) => entry.slice(5));

test('Renderer 진단 계약: Board 는 칸 위치의 spot BYTE(y*256+x)만 읽고 비트 8 은 색 5·비트 6 은 색 1 의 세 겹이며 다른 비트는 그리지 않는다', () => {
    const run = (value) => {
        const drawing = createDrawing(['3', '1', '0', '0', '0', '0']);
        drawing.spots[22 * 256 + 21] = value;
        drawing.draw();
        return rects(drawing);
    };
    // 칸 (21,22): 투영 점 (32,33)에서 칸 크기(16,11)를 뺀 사각형을 한 칸씩 좁힌 세 겹이다(snapshot 원점 304,209).
    const strong = run(8);
    assert.deepEqual(strong, ['17:23:31:32:15', '18:24:30:31:15', '19:25:29:30:15']);
    for (const weak of [2, 4, 6]) assert.deepEqual(run(weak), ['17:23:31:32:11', '18:24:30:31:11', '19:25:29:30:11'], `spot ${weak}`);
    // 비트 8 과 6 이 함께 있으면 8 이 이긴다. 비트 1·16·32·64·128 만으로는 그리지 않는다.
    assert.deepEqual(run(8 | 6), strong);
    for (const none of [0, 1, 16, 32, 64, 128, 0xf1]) assert.deepEqual(run(none), [], `spot ${none}`);
});

test('Renderer 진단 계약: Spot 은 표면 BYTE 의 0 비트가 있을 때만 뒤 세 겹을 현재 회색(색 8)으로 그리고 지도 밖 칸은 SID 0 의 표면을 읽는다', () => {
    const drawing = createDrawing(['2', '1', '0', '0', '0', '0']);
    // 최하위 단계 칸 (21,22)에 표면 BYTE 1 인 객체를 걸고 그 칸의 spot 만 0x40(색 7)으로 둔다.
    const raw = drawing.pool.physical(70);
    raw.fill(0);
    raw[12] = 1;
    drawing.hash.setBucket(0, 21, 22, 70);
    drawing.spots[22 * 256 + 21] = 0x40;
    drawing.draw();
    // 앞 세 겹은 색 7(17), 뒤 세 겹은 색 8(18)이다.
    assert.deepEqual(rects(drawing), ['17:23:31:32:17', '18:24:30:31:17', '19:25:29:30:17', '20:26:28:29:18', '21:27:27:28:18', '22:28:26:27:18']);
    // 표면 BYTE 의 0 비트가 없으면(짝수) 앞 세 겹만 그린다.
    raw[12] = 2;
    const plain = createDrawing(['2', '1', '0', '0', '0', '0']);
    plain.pool.physical(70).fill(0);
    plain.pool.physical(70)[12] = 2;
    plain.hash.setBucket(0, 21, 22, 70);
    plain.spots[22 * 256 + 21] = 0x40;
    plain.draw();
    assert.equal(rects(plain).length, 3);
    // 0x80 은 색 0(10)이다. 지도 밖(x<0) 칸은 해시를 읽지 않고 SID 0 슬롯의 표면 BYTE 를 쓴다.
    const outside = createDrawing(['2', '1', '0', '0', '0', '0']);
    setView(outside, -1.5, 20.5, 1000, 1000);
    outside.pool.physical(0)[12] = 1;
    outside.spots[20 * 256 - 1] = 0x80;
    outside.draw();
    assert.deepEqual(rects(outside).map((entry) => entry.split(':')[4]), ['10', '10', '10', '18', '18', '18']);
});

test('Renderer 진단 계약: spot 배열 위치는 y*256+x 이고 배열 밖(음수·65536 이상)은 물리 범위 오류이며 경계 0 과 65535 는 허용한다', () => {
    const at = (worldX, worldY) => {
        const drawing = createDrawing(['3', '1', '0', '0', '0', '0']);
        setView(drawing, worldX, worldY, 1000, 1000);
        return drawing;
    };
    // 한 칸만 보이는 보기(칸 크기 1000)로 경계를 본다.
    at(0.5, 0.5).draw();
    assert.throws(() => at(-1.5, 0.5).draw(), /원문 배열 물리 범위/);
    assert.throws(() => at(0.5, 256.5).draw(), /원문 배열 물리 범위/);
    const last = at(255.5, 255.5);
    last.spots[65535] = 8;
    last.draw();
    assert.equal(rects(last).length, 3);
    // 행 alias: x 가 255 를 넘으면 다음 행의 앞 칸을 읽는다(임의로 정규화하지 않음).
    const alias = at(256.5, 3.5);
    alias.spots[4 * 256 + 0] = 8;
    alias.draw();
    assert.equal(rects(alias).length, 3);
});

test('Renderer 진단 계약: Hash 는 칸 개수를 단계별 한계(세계 칸÷단계−1)로 자르고 아래 한계 0 은 추가하지 않는다', () => {
    const count = (worldX, worldY, worldCells) => {
        const drawing = createDrawing(['0', '1', '0', '0', '0', '0']);
        setView(drawing, worldX, worldY);
        drawing.worldCells.value = worldCells;
        drawing.draw();
        return rects(drawing).length;
    };
    // 가시 영역 (250..256, 250..257): 단계 1 은 6×6, 2 는 3×3, 4 는 2×2, 16 은 1×1 이다.
    assert.equal(count(250.5, 250.5, 256), 36 + 9 + 4 + 1);
    // 세계 칸 64: 모든 단계가 한 칸(63·31·15·3)으로 잘린다.
    assert.equal(count(250.5, 250.5, 64), 4);
    // 아래 한계는 없다: 음수 칸도 그대로 그린다(−3..2 의 6칸 × (−2..3)의 6칸 등).
    assert.ok(count(-2.5, -2.5, 256) > 0);
});

test('Renderer 진단 계약: 상수 비트 — Fire 는 TYPE flags1 의 0x800 만, Spot 은 0x40/0x80 만, Board 는 8 과 6 만 고르고 색 배열 경계와 TYPE 범위를 거부한다', () => {
    // Fire: flags1 의 다른 비트(0x001·0x7ff·0x1000)만 있는 타입은 그리지 않는다.
    for (const flags of [0, 0x001, 0x7ff, 0x1000]) {
        const drawing = createDrawing(['1', '1', '0', '0', '0', '0']);
        for (let i = 0; i < 6; i += 1) drawing.types[100 + i].flags1 = flags;
        drawing.draw();
        assert.equal(rects(drawing).length, 0, `flags ${flags}`);
    }
    // Spot: 0x01·0x20·0x41 은 0x40 이 아닌 값에서 그리지 않는다(0x41 은 0x40 을 포함하므로 그린다).
    const spotRects = (value) => { const drawing = createDrawing(['2', '1', '0', String(value), '0', '0']); drawing.draw(); return rects(drawing).length; };
    for (const none of [0, 1, 0x20, 0x3f]) assert.equal(spotRects(none), 0, `spot ${none}`);
    assert.ok(spotRects(0x41) > 0);
    assert.ok(spotRects(0x81) > 0);
    // 색 배열 길이가 사용 색 번호와 같으면(Board 의 색 5, 길이 5) 물리 범위 오류이다.
    const shortColors = createDrawing(['3', '1', '0', '8', '0', '0']);
    shortColors.colors = [1, 2, 3, 4, 5];
    assert.throws(() => shortColors.draw(), /색표 물리 범위/);
    const exact = createDrawing(['3', '1', '0', '8', '0', '0']);
    exact.colors = [1, 2, 3, 4, 5, 6];
    exact.draw();
    assert.ok(rects(exact).length > 0);
    // TYPE 표가 모자라면(객체 TYPE 100 이 표 길이와 같음) 범위 오류이다(finder 가 먼저 거부하므로 진단 자신의 TYPE 범위 검사는 중복 방어이다).
    const shortTypes = createDrawing(['1', '1', '0', '0', '0', '0']);
    shortTypes.types.length = 100;
    assert.throws(() => shortTypes.draw(), RangeError);
    // 색 개수 0 이면 graph 나머지 선택에서 거부하고, 색이 한 개면 항상 그 색이다.
    const empty = createDrawing(['1', '1', '0', '0', '0', '0']);
    empty.colors = [];
    empty.pool.physical(63)[30] = 1; // graph 0·무효 그래프 객체도 일반 나머지 경로로 보낸다.
    empty.pool.physical(61)[30] = 1;
    assert.throws(() => empty.draw(), /색 개수 0/);
    const single = createDrawing(['1', '1', '0', '0', '0', '0']);
    single.colors = [9];
    single.pool.physical(61)[30] = 1; // 무효 그래프(254) 객체는 색 7 을 요구하므로 일반 그래프로 바꾼다.
    single.draw();
    assert.ok(rects(single).some((entry) => entry.endsWith(':9')));
});

test('Renderer 진단 계약: graph 색은 현재 팔레트 수의 나머지이고 연결선은 여덟 방향 각각의 이웃 칸(x+dx, y+dy)과 중심 칸의 거리 WORD 를 비교한다', () => {
    // 팔레트가 graph 보다 짧으면 나머지로 고른다(graph 5, 팔레트 3 → 색 2).
    const modulo = createDrawing(['1', '1', '0', '0', '0', '0']);
    modulo.colors = [10, 11, 12];
    modulo.pool.physical(50)[30] = 5;
    modulo.pool.physical(61)[30] = 1;
    modulo.pool.physical(63)[30] = 1;
    modulo.draw();
    assert.deepEqual(rects(modulo).map((entry) => entry.split(':')[4]), ['12', '12', '12', '11', '11', '11', '11', '11', '11']);
    // 거리표를 칸마다 다른 값으로 채우고 한 방향의 이웃만 중심(객체 50 의 칸 (20,20))과 같게 한다.
    const directions = [[0, -1], [1, -1], [1, 0], [1, 1], [0, 1], [-1, 1], [-1, 0], [-1, -1]];
    directions.forEach(([dx, dy], direction) => {
        const drawing = createDrawing(['1', '1', '1', '0', '5', '0']);
        drawing.distances = Int16Array.from({ length: 65536 }, (_, index) => index & 0x7fff);
        drawing.distances[(20 + dy) * 256 + (20 + dx)] = drawing.distances[20 * 256 + 20];
        // 객체 50 의 x 를 20.75 로 둬 칸 위치가 반올림(21)이 아니라 0 방향 절삭(20)임을 본다(해시 머리 칸은 그대로 20).
        put(drawing.pool.physical(50), 14, floatToBits(20.75));
        drawing.draw();
        // 객체 50(좌표 20.75,20.5)의 snapshot 투영 점에서 (-8,-6)이 시작점이고 끝점은 방향 증분 (7,5)배이다.
        const point = drawing.snapshot.project(20.75, 20.5);
        const px = point.x - 8;
        const py = point.y - 6;
        // 다른 객체의 중심 칸도 바뀐 칸과 이웃할 수 있으므로 객체 50 의 선만 본다: 정확히 이 방향 하나뿐이다.
        const lines = drawing.trace.filter((entry) => entry.startsWith(`Line:${px}:${py}:`));
        assert.deepEqual(lines, [`Line:${px}:${py}:${px + dx * 7}:${py + dy * 5}:17`], `방향 ${direction}`);
    });
});

test('Renderer 진단 계약: Spot 의 지도 안 경계 칸(x=0·y=0·255)은 해시 표면을 읽고 지도 밖(x=256)은 SID 0 의 표면을 읽는다', () => {
    // 한 칸만 보이는 보기에서 지정한 칸의 spot 을 0x40 으로 두고, 그 칸의 해시 표면 BYTE 와 SID 0 의 표면 BYTE 를 서로 다르게 둔다.
    const outcome = (cellX, cellY, hashSurface, slotZeroSurface) => {
        const drawing = createDrawing(['2', '1', '0', '0', '0', '0']);
        setView(drawing, cellX + 0.5, cellY + 0.5, 1000, 1000);
        drawing.pool.physical(0)[12] = slotZeroSurface;
        const raw = drawing.pool.physical(70);
        raw.fill(0);
        raw[12] = hashSurface;
        // 지도 안 칸이면 그 칸의 해시 머리에 객체를 건다(밖이면 걸지 않는다).
        if (cellX >= 0 && cellX <= 255 && cellY >= 0 && cellY <= 255) drawing.hash.setBucket(0, cellX, cellY, 70);
        drawing.spots[((cellY * 256 + cellX) | 0)] = 0x40;
        drawing.draw();
        return rects(drawing).length;
    };
    // 안쪽 칸: 해시 객체의 표면(1)이 이겨 뒤 세 겹(회색)까지 6개, 해시 표면이 0 이면 SID 0 의 표면과 무관하게 3개이다.
    for (const [x, y] of [[0, 20], [20, 0], [255, 20], [20, 255], [255, 255], [0, 0]]) {
        assert.equal(outcome(x, y, 1, 0), 6, `칸 (${x},${y}) 해시 표면`);
        assert.equal(outcome(x, y, 0, 1), 3, `칸 (${x},${y}) SID 0 표면 무시`);
    }
    // 바깥 칸(x=256 은 spot 배열 alias 로 다음 행 앞 칸): SID 0 의 표면(1)을 읽어 6개, 0 이면 3개이다.
    assert.equal(outcome(256, 20, 0, 1), 6);
    assert.equal(outcome(256, 20, 1, 0), 3);
});

test('Renderer 진단 계약: Spot 의 세로 칸 256 은 지도 밖(SID 0 표면)이어도 spot 배열은 물리 범위 오류이다', () => {
    const drawing = createDrawing(['2', '1', '0', '0', '0', '0']);
    setView(drawing, 20.5, 256.5, 1000, 1000);
    assert.throws(() => drawing.draw(), /원문 배열 물리 범위/);
});
