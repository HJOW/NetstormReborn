// DrawWater(진단 사각형 표시) core/draw-water.js 를 원본 10.78 관측과 cpppj 시험, 웹 계약으로 대조한다.
//  - draw-water-x86.tsv 76행: 여섯 합성 일반 객체를 실제 네 단계 해시에 걸고(장면 0 화면 안·1 buried 섞임·2 화면 밖) Renderer/snapshot·칸 크기·flag·현재 색 변이(색/칸 크기/flag 끄기/객체 buried 로 바꾸기)를
//    둔 입력의 전체 96×80 표면(행 padding 100)·최종 clip·flag·색·장치 호출 추적(Begin/Clip/Rect/Lock/Unlock/Present)이 원본과 같다.
//  - draw-water-input-x86.tsv 384행: Guide/Gump/release/정지/Console/모드 조합에서 일반 사건 분배의 DWORD 토글과 worldRedraw 쓰기.
//  - draw-water-names-x86.tsv 9행: 등록 이름(대소문자 무관)·숫자/문자열 인자를 기존 스크립트 명령 큐로 보낸 사건 코드/인자/고리 위치. (GameMenuCommands 의 전체 표는 아직 없어 이름표는 drawWaterScriptCode 로 대신한다.)
//  - cpppj 시험 이식: flag 0 이면 장치/나눗셈에 도달하지 않음, 칸 크기 0 은 IDIV 예외, 경로 표식 세 겹 사각형과 DWORD 감김.
//  - 웹 계약: 가시 영역/ finder 영역 직접 값(음수 월드, 큰 정수의 float32 반올림, 위쪽 snap 의 0 방향 절삭), 네 선 모서리 겹침, 장치 도달 시점.
// 실제 게임·브라우저 실행이 아니다. (2026-11-13 WB04-E380: 5-A DrawWater 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { InputQueue } from '../www/core/input-queue.js';
import { ScriptStringRing, queueScriptCommand } from '../www/ui/script-command.js';
import { CameraView } from '../www/render/camera-view.js';
import {
    DRAW_WATER_COMMAND, RawDrawWater, dispatchDrawWater, drawWaterFinderArea, drawWaterRectangle, drawWaterScriptCode, drawWaterVisibleArea, pathMarkerRectangles
} from '../www/core/draw-water.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-13 WB04-E380)
const fixture = (name) => readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', name));
const drawRows = fixture('draw-water-x86.tsv');
const inputRows = fixture('draw-water-input-x86.tsv');
const nameRows = fixture('draw-water-names-x86.tsv');

// Native 표면과 같은 폭/행 padding 이다. (2026-11-13 WB04-E380)
const WIDTH = 96;
const HEIGHT = 80;
const PITCH = 100;

// 16진 문자열을 바이트 배열로(한 글자 한 바이트 문자열이 필요하면 text) 바꾼다. (2026-11-13 WB04-E380)
const unhex = (text) => Uint8Array.from(Buffer.from(text, 'hex'));
const hexText = (text) => Array.from(Buffer.from(text, 'hex'), (byte) => String.fromCharCode(byte)).join('');
// 사각형 직렬화이다. (2026-11-13 WB04-E380)
const rectText = (rect) => `${rect.left}:${rect.top}:${rect.right}:${rect.bottom}`;

// 원본과 같은 여섯 합성 일반 객체와 실제 네 단계 해시, 장치 추적을 가진 입력 한 건이다. (2026-11-13 WB04-E380)
function createDrawing(input) {
    const drawing = {
        pool: new SidPool(24000, { server: false }), hash: new SquidHash(), types: Array.from({ length: 256 }, () => ({ footX: 0, footY: 0 })),
        renderer: new CameraView(), snapshot: new CameraView(), enabled: { value: Number(input[0]) }, color: { value: 0x1000002d }, mutation: Number(input[7]),
        rects: 0, presents: 0, clip: { left: 5, top: 7, right: 91, bottom: 73 }, pixels: new Uint8Array(PITCH * HEIGHT).fill(199), trace: [],
    };
    const cellX = Number(input[1]);
    const cellY = Number(input[2]);
    for (const view of [drawing.renderer, drawing.snapshot]) {
        view.data.setInt32(8, cellX, true);
        view.data.setInt32(12, cellY, true);
    }
    drawing.renderer.data.setFloat32(0, Number(input[3]), true);
    drawing.renderer.data.setFloat32(4, Number(input[4]), true);
    drawing.snapshot.data.setInt32(28, Number(input[5]), true);
    drawing.snapshot.data.setInt32(32, Number(input[6]), true);
    const scene = Number(input[8]);
    const ids = [50, 60, 61, 62, 63, 64];
    const states = [0, 1, 2, 4, 8, 6];
    // 입력 순서로 같은 버킷 머리에 연결한다. 순회/포함 결과는 제품이 계산한다.
    ids.forEach((id, i) => {
        const x = scene === 2 ? 100 + i : 20 + (i % 3) + 0.25;
        const y = scene === 2 ? 110 + i : 20 + Math.floor(i / 3) + 0.5;
        const raw = drawing.pool.physical(id);
        raw.fill(0);
        raw[10] = 100 + i;
        raw[11] = states[i];
        raw[40] = scene === 1 && i % 2 ? 8 : 0;
        put(raw, 14, floatToBits(x));
        put(raw, 18, floatToBits(y));
        drawing.types[100 + i].footX = 1 + (i % 3);
        drawing.types[100 + i].footY = 1 + (i % 2);
        put(raw, 4, drawing.hash.bucket(i % 4, x, y), 2);
        drawing.hash.setBucket(i % 4, x, y, id);
    });
    const add = (value) => drawing.trace.push(value);
    drawing.hooks = {
        clip: (rect) => { add(`Clip:${rectText(rect)}`); drawing.clip = rect; },
        rectangle: (rectangle, ink) => {
            add(`Rect:${rectText(rectangle)}:${ink}`);
            drawing.rects += 1;
            // Native 경계에서 공급한 현재 필드 변이를 같은 호출 위치(첫 사각형)에 적용한다.
            if (drawing.rects === 1) {
                if (drawing.mutation === 1) drawing.color.value = 0x123456a1;
                else if (drawing.mutation === 2) { drawing.snapshot.data.setInt32(8, 7, true); drawing.snapshot.data.setInt32(12, 13, true); }
                else if (drawing.mutation === 3) drawing.enabled.value = 0;
                else if (drawing.mutation === 4) drawing.pool.physical(63)[40] = 8;
            }
            const saved = drawing.clip;
            drawing.clip = { left: 0, top: 0, right: WIDTH, bottom: HEIGHT };
            add(`Lock:${rectText(drawing.clip)}`);
            drawWaterRectangle(drawing.pixels, PITCH, HEIGHT, drawing.clip, rectangle, ink);
            add(`Unlock:${rectText(drawing.clip)}`);
            drawing.clip = saved;
        },
        present: () => { add('Present:0:0:96:80'); drawing.presents += 1; },
    };
    drawing.draw = () => {
        // Native constructor 의 입력만 기록하고 전체 순회는 같은 제품 몸체가 한다.
        if (drawing.enabled.value) {
            const area = drawWaterVisibleArea(drawing.renderer, WIDTH, HEIGHT);
            add(`Begin:${floatToBits(area.left)}:${floatToBits(area.top)}:${floatToBits(area.right)}:${floatToBits(area.bottom)}:0`);
        }
        new RawDrawWater(drawing.pool, drawing.hash, drawing.types).draw(drawing.renderer, drawing.snapshot, WIDTH, HEIGHT, drawing.enabled, drawing.color, drawing.hooks);
    };
    return drawing;
}

// 변경 구간만 기록한 기대 표면을 만든다('-' 은 변화 없음). (2026-11-13 WB04-E380)
function expectedPixels(changed) {
    const result = new Uint8Array(PITCH * HEIGHT).fill(199);
    if (changed === '-') return result;
    // 관측한 실제 변경 BYTE 만 원위치에 놓는다.
    for (const run of changed.split(';')) {
        const [offset, bytes] = run.split(':');
        result.set(unhex(bytes), Number(offset));
    }
    return result;
}

test('DrawWater: 원본 76행의 전체 finder·투영·사각형·표면 픽셀과 현재 필드 변이가 호출 순서까지 같다', { skip: !drawRows && '원본 fixture 없음' }, () => {
    assert.equal(drawRows.length, 76);
    // 서로 다른 Renderer/snapshot·음수 half·buried/상태·현재 색/칸/flag 변이를 포함한다.
    for (const row of drawRows) {
        assert.equal(row.length, 6);
        const input = row[0].slice(1, -1).split(',');
        assert.equal(input.length, 9);
        const drawing = createDrawing(input);
        drawing.draw();
        const expected = expectedPixels(row[1]);
        const mismatch = drawing.pixels.findIndex((value, index) => value !== expected[index]);
        assert.equal(mismatch, -1, `${row[0]} 픽셀 ${mismatch} 실제 ${drawing.pixels[mismatch]} 기대 ${expected[mismatch]}`);
        assert.equal(rectText(drawing.clip), row[2], row[0]);
        assert.equal(drawing.enabled.value, Number(row[3]), row[0]);
        assert.equal(drawing.color.value, Number(row[4]), row[0]);
        assert.equal(drawing.trace.length === 0 ? '-' : drawing.trace.join(';'), row[5], row[0]);
        // flag 를 반복 중 끈 입력도 현재 원문 호출은 끝까지 진행한다.
        assert.equal(drawing.presents, Number(input[0]) ? 1 : 0, row[0]);
    }
});

test('DrawWater: 일반 사건 분배 384행의 DWORD 토글과 worldRedraw 쓰기가 Guide/Gump/release 조합에서 같다', { skip: !inputRows && '원본 fixture 없음' }, () => {
    assert.equal(inputRows.length, 384);
    // 선행 Guide/Gump 의 현재 소비를 일반 분배보다 먼저 적용한다.
    for (const row of inputRows) {
        assert.equal(row.length, 14);
        const enabled = { value: Number(row[8]) };
        const redraw = { value: Number(row[9]) };
        if (Number(row[5]) === 0 && Number(row[6]) === 0) assert.equal(dispatchDrawWater({ code: Number(row[0]), x: Number(row[1]) | 0, y: Number(row[2]) | 0 }, enabled, redraw), true);
        assert.equal(enabled.value, Number(row[10]), row.join(' '));
        assert.equal(redraw.value, Number(row[11]), row.join(' '));
        assert.equal(row[12], '1');
        assert.equal(row[13], '0');
    }
    const enabled = { value: 7 };
    const redraw = { value: 92 };
    assert.equal(dispatchDrawWater({ code: 'W'.charCodeAt(0), x: 0, y: 0 }, enabled, redraw), false);
    assert.deepEqual([enabled.value, redraw.value], [7, 92]);
});

test('DrawWater: 이름 9행이 기존 스크립트 명령 큐로 숫자/문자열 인자와 고리 위치까지 같고 Water 는 미등록이다', { skip: !nameRows && '원본 fixture 없음' }, () => {
    assert.equal(nameRows.length, 9);
    // 기대 이름/인자/고리 결과는 실제 원문 fixture 에서만 읽는다.
    for (const row of nameRows) {
        assert.equal(row.length, 9);
        const queue = new InputQueue();
        const strings = new ScriptStringRing();
        let unknown = 0;
        queueScriptCommand(hexText(row[1]), queue, strings, { lookup: drawWaterScriptCode, unknown: () => { unknown += 1; } });
        assert.equal(unknown, 0);
        assert.equal(strings.index, Number(row[7]));
        const [codeText, expected] = row[6].split(',');
        const event = queue.pop(false);
        assert.equal(queue.empty, true);
        assert.equal(event.code, Number(codeText));
        assert.equal(event.x, event.y);
        const parameter = event.y >>> 0;
        if (expected[0] === 'n') assert.equal(parameter, Number(expected.slice(1)));
        else assert.equal(strings.resolve(parameter), hexText(expected.slice(1)));
    }
    assert.equal(drawWaterScriptCode('Water'), 0xffffffff);
    // 명령 이름은 첫 NUL 까지이며 ASCII 대소문자만 무시한다.
    assert.equal(drawWaterScriptCode('DRAWWATER'), DRAW_WATER_COMMAND);
    assert.equal(drawWaterScriptCode('drawWater\0junk'), DRAW_WATER_COMMAND);
    assert.equal(drawWaterScriptCode('drawwater '), 0xffffffff);
    assert.equal(drawWaterScriptCode(''), 0xffffffff);
    assert.equal(drawWaterScriptCode('drawwatEr'), DRAW_WATER_COMMAND);
    assert.equal(drawWaterScriptCode('drawwatİr'), 0xffffffff);
});

// ─────────────────────────────────────────────────────────────────────────────
// cpppj 시험 이식과 웹 계약
// ─────────────────────────────────────────────────────────────────────────────

// 월드 float·칸 크기를 가진 보기이다. (2026-11-13 WB04-E380)
function viewOf(worldX, worldY, cellX, cellY) {
    const view = new CameraView();
    view.data.setFloat32(0, worldX, true);
    view.data.setFloat32(4, worldY, true);
    view.data.setInt32(8, cellX, true);
    view.data.setInt32(12, cellY, true);
    return view;
}

test('DrawWater: flag 0 은 장치·나눗셈에 도달하지 않고 비영 flag 의 후보 0개도 present 를 요구한다', () => {
    const pool = new SidPool(24000, { server: false });
    const hash = new SquidHash();
    const types = [{ footX: 0, footY: 0 }];
    // 칸 크기 0 인 빈 보기와 빈 장치도 flag 0 에서는 아무 일도 없다.
    new RawDrawWater(pool, hash, types).draw(new CameraView(), new CameraView(), 0, 0, { value: 0 }, { value: 0 }, {});
    // 비영 flag 는 후보가 없어도 present 한 번을 요구한다(장치가 없으면 미연결 오류).
    const view = viewOf(19.25, 19.5, 16, 11);
    assert.throws(() => new RawDrawWater(pool, hash, types).draw(view, view, WIDTH, HEIGHT, { value: 1 }, { value: 0 }, {}), /미연결/);
    let presents = 0;
    new RawDrawWater(pool, hash, types).draw(view, view, WIDTH, HEIGHT, { value: 0xffffffff }, { value: 0 }, { present: () => { presents += 1; } });
    assert.equal(presents, 1);
    // 후보가 있는 입력에서는 clip → rectangle 순서로 요구하고 장치가 없으면 도달한 첫 장치에서 거부한다.
    const drawing = createDrawing(['1', '16', '11', '19.25', '19.5', '304', '209', '0', '0']);
    const hooks = drawing.hooks;
    const pool2 = new RawDrawWater(drawing.pool, drawing.hash, drawing.types);
    assert.throws(() => pool2.draw(drawing.renderer, drawing.snapshot, WIDTH, HEIGHT, drawing.enabled, drawing.color, { rectangle: hooks.rectangle, present: hooks.present }), /미연결/);
    assert.equal(drawing.rects, 0);
    assert.throws(() => pool2.draw(drawing.renderer, drawing.snapshot, WIDTH, HEIGHT, drawing.enabled, drawing.color, { clip: hooks.clip, present: hooks.present }), /미연결/);
    assert.equal(drawing.rects, 0);
});

test('DrawWater: 초기 사각형은 snapshot 칸 크기/2 를 0 방향 절삭으로 빼고 아래쪽에는 가로 반크기를 쓴다', () => {
    // 첫 후보(SID 50, 월드 20.25/20.5)의 투영점은 (20, 17)이다. 칸 크기가 음수 홀수여도 반크기는 0 방향으로 절삭한다.
    const drawing = createDrawing(['1', '16', '11', '19.25', '19.5', '304', '209', '0', '0']);
    drawing.snapshot.data.setInt32(8, -3, true);
    drawing.snapshot.data.setInt32(12, -5, true);
    drawing.draw();
    const clips = drawing.trace.filter((entry) => entry.startsWith('Clip:'));
    assert.equal(clips[0], 'Clip:21:19:21:18');
    assert.equal(drawing.trace.find((entry) => entry.startsWith('Rect:')).startsWith('Rect:21:19:21:18:'), true);
    // 양수 홀수 칸 크기: 7/2 = 3, 13/2 = 6 → (20 − 3, 17 − 6, 17, 17 − 3).
    const odd = createDrawing(['1', '16', '11', '19.25', '19.5', '304', '209', '0', '0']);
    odd.snapshot.data.setInt32(8, 7, true);
    odd.snapshot.data.setInt32(12, 13, true);
    odd.draw();
    assert.equal(odd.trace.find((entry) => entry.startsWith('Clip:')), 'Clip:17:11:17:14');
});

test('DrawWater: 가시 영역은 칸 크기 0·INT_MIN / −1 을 나눗셈 예외로 거부하고 음수 월드·indefinite 월드를 ftol 하위값으로 읽는다', () => {
    assert.throws(() => drawWaterVisibleArea(new CameraView(), 96, 80), /IDIV/);
    assert.throws(() => drawWaterVisibleArea(viewOf(1, 1, 16, 0), 96, 80), /IDIV/);
    assert.throws(() => drawWaterVisibleArea(viewOf(1, 1, 0, 11), 96, 80), /IDIV/);
    assert.throws(() => drawWaterVisibleArea(viewOf(1, 1, -1, 11), -0x80000000, 80), /IDIV/);
    assert.throws(() => drawWaterVisibleArea(viewOf(1, 1, 16, -1), 96, -0x80000000), /IDIV/);
    // INT_MIN 도 −1 이 아닌 칸 크기로는 정상 나눗셈이고, −1 칸 크기도 INT_MIN 이 아닌 크기로는 정상이다.
    assert.deepEqual(drawWaterVisibleArea(viewOf(0, 0, 16, 11), -0x80000000, -0x80000000), { left: 0, top: 0, right: -134217728, bottom: -195225786 });
    assert.deepEqual(drawWaterVisibleArea(viewOf(5, 6, -1, -1), 96, 80), { left: 5, top: 6, right: -91, bottom: -74 });
    // 일반 값: 분수 원점은 0 방향으로 절삭, 오른쪽/아래는 나눗셈 몫을 더한다.
    assert.deepEqual(drawWaterVisibleArea(viewOf(19.25, 19.5, 16, 11), 96, 80), { left: 19, top: 19, right: 25, bottom: 26 });
    assert.deepEqual(drawWaterVisibleArea(viewOf(-3.75, -0.5, 16, 11), 96, 80), { left: -3, top: 0, right: 3, bottom: 7 });
    assert.deepEqual(drawWaterVisibleArea(viewOf(10, 20, -16, -11), 96, 80), { left: 10, top: 20, right: 4, bottom: 13 });
    assert.deepEqual(drawWaterVisibleArea(viewOf(Number.NaN, Number.POSITIVE_INFINITY, 16, 11), 96, 80), { left: 0, top: 0, right: 6, bottom: 7 });
    assert.deepEqual(drawWaterVisibleArea(viewOf(0, 0, 16, 11), -80, 15), { left: 0, top: 0, right: -5, bottom: 1 });
    // 32비트를 넘는 월드 좌표는 하위 DWORD 로 감기고, 합은 DWORD 감김이다.
    assert.deepEqual(drawWaterVisibleArea(viewOf(3e9, 0, 16, 11), 96, 80).left, -1294967296);
    assert.equal(drawWaterVisibleArea(viewOf(2147483520, 2147483520, 1, 1), 96, 80).right, (2147483520 + 96) | 0);
});

test('DrawWater: finder 영역은 float32 로 두 번 저장하고 위쪽 snap 은 음수에서 0 방향 절삭한다', () => {
    assert.deepEqual(drawWaterFinderArea({ left: 19, top: 19, right: 25, bottom: 26 }), { left: 19, top: 19, right: 25, bottom: 26 });
    // 큰 정수는 float32 반올림을 거친다(2^24 + 1 → 2^24, 2^25 + 3 → 2^25 + 4).
    assert.deepEqual(drawWaterFinderArea({ left: 16777217, top: 33554435, right: 16777217, bottom: 33554435 }), { left: 16777216, top: 33554436, right: 16777216, bottom: 33554436 });
    assert.deepEqual(drawWaterFinderArea({ left: -16777217, top: -33554435, right: -16777217, bottom: -33554435 }), { left: -16777216, top: -33554436, right: -16777215, bottom: -33554436 });
    // 위쪽(오른쪽/아래) 좌표만 +0.99999 를 더한 뒤 절삭한다: −5 → −4.00001 → −4, −1 → 0, 0 → 0.
    assert.deepEqual(drawWaterFinderArea({ left: -5, top: -5, right: -5, bottom: -5 }), { left: -5, top: -5, right: -4, bottom: -4 });
    assert.deepEqual(drawWaterFinderArea({ left: -1, top: -1, right: -1, bottom: -1 }), { left: -1, top: -1, right: 0, bottom: 0 });
    assert.deepEqual(drawWaterFinderArea({ left: 0, top: 0, right: 0, bottom: 0 }), { left: 0, top: 0, right: 0, bottom: 0 });
    assert.deepEqual(drawWaterFinderArea({ left: 2147483647, top: -2147483648, right: 2147483647, bottom: -2147483648 }), { left: -2147483648, top: -2147483648, right: -2147483648, bottom: -2147483648 });
});

test('DrawWater: 네 선은 위·아래·왼쪽·오른쪽 순서로 right/bottom − 1 모서리까지 그리고 감김을 보존한다', () => {
    const pixels = new Uint8Array(PITCH * HEIGHT);
    const full = { left: 0, top: 0, right: WIDTH, bottom: HEIGHT };
    drawWaterRectangle(pixels, PITCH, HEIGHT, full, { left: 2, top: 3, right: 6, bottom: 5 }, 77);
    const lit = [];
    pixels.forEach((value, index) => { if (value) lit.push(`${index % PITCH},${Math.floor(index / PITCH)}`); });
    assert.equal(lit.join(' '), '2,3 3,3 4,3 5,3 2,4 3,4 4,4 5,4');
    // 한 칸 사각형은 한 점이고, 0폭 사각형(left==right)은 오른쪽 변이 왼쪽 밖(−1)으로 감겨 가로 선이 왼쪽으로 뻗는다.
    const point = new Uint8Array(PITCH * HEIGHT);
    drawWaterRectangle(point, PITCH, HEIGHT, full, { left: 9, top: 9, right: 10, bottom: 10 }, 5);
    assert.equal(point.reduce((count, value) => count + (value ? 1 : 0), 0), 1);
    assert.equal(point[9 * PITCH + 9], 5);
    const zero = new Uint8Array(PITCH * HEIGHT);
    drawWaterRectangle(zero, PITCH, HEIGHT, full, { left: 4, top: 6, right: 4, bottom: 8 }, 9);
    const zeroLit = [];
    zero.forEach((value, index) => { if (value) zeroLit.push(`${index % PITCH},${Math.floor(index / PITCH)}`); });
    assert.equal(zeroLit.join(' '), '3,6 4,6 3,7 4,7');
    // 색은 하위 BYTE 만 쓴다.
    const color = new Uint8Array(PITCH * HEIGHT);
    drawWaterRectangle(color, PITCH, HEIGHT, full, { left: 1, top: 1, right: 3, bottom: 3 }, 0x1000002d);
    assert.equal(color[1 * PITCH + 1], 0x2d);
});

test('DrawWater: 경로 표식은 오른쪽 아래 투영점에서 칸 크기만큼 왼쪽 위로 잡은 세 겹 사각형이며 DWORD 감김을 보존한다', () => {
    const rectangles = pathMarkerRectangles([100, 80], [16, 11]);
    assert.deepEqual(rectangles[0], { left: 85, top: 70, right: 99, bottom: 79 });
    assert.deepEqual(rectangles[1], { left: 86, top: 71, right: 98, bottom: 78 });
    assert.deepEqual(rectangles[2], { left: 87, top: 72, right: 97, bottom: 77 });
    assert.equal(rectangles.length, 3);
    // 음수/경계 좌표도 signed DWORD 감김으로 정의된 값을 만든다.
    const wrapped = pathMarkerRectangles([-2147483648, 2147483647], [-1, 5]);
    assert.equal(wrapped[0].right, 2147483647);
    assert.equal(wrapped[0].bottom, 2147483646);
    assert.equal(wrapped[0].left, -2147483646);
    assert.equal(wrapped[2].bottom, 2147483644);
    assert.equal(wrapped[2].top, 2147483647 - 5 + 3);
    assert.equal(wrapped[2].right, 2147483645);
    assert.equal(wrapped[1].left, -2147483645);
    assert.equal(wrapped[1].top, 2147483647 - 5 + 2);
    // 칸 크기 0 이면 왼쪽/위가 투영점과 같아 매 겹 한 픽셀씩 서로 가까워진다.
    const flat = pathMarkerRectangles([10, 20], [0, 0]);
    assert.deepEqual(flat.map((rect) => [rect.left, rect.top, rect.right, rect.bottom]), [[11, 21, 9, 19], [12, 22, 8, 18], [13, 23, 7, 17]]);
});
