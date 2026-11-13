// 번개 팔레트 프로세스 palette-flash.js(PaletteFlash·thunderOffset·thunderDuration)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - paletteflash-x86.tsv 의 originals 256행(op entry 80·init 168·cancel 8): 생성 요청(밝기/전체화면)·직접 초기화·한 프레임·중단에서 초기화 직후 변경 색, 원래 색, 간격(double 비트),
//    진행 뒤 변경 색, 마지막 시각(double 비트), 남은 단계, 초기화 여부, 장치 호출 순서(apply/restore/kill)가 같다. originalCD/original1037 행은 10.72/10.37 이라 사용하지 않는다.
//  - 계약: 생성자 거부, 벽시계 기준 정지 중 진행/화면 모드 재조회, 중단 시 최신 저장 팔레트 복구와 중복 취소 무시.
// 실제 게임·브라우저 실행이 아니다. (2026-11-13 WB04-E376: 5-A PaletteFlash 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';
import { PaletteFlash, PaletteFlashState, THUNDER_COLOR_COUNT, THUNDER_FIRST_COLOR, THUNDER_STEPS, emptyScreenColor, thunderDuration, thunderOffset } from '../www/core/palette-flash.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-13 WB04-E376)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'paletteflash-x86.tsv'))?.filter((row) => row[0] === 'originals') ?? null;

// fixture 의 double 바이트를 little endian 으로 복원한다(NaN/무한대도 원본 저장 값을 유지). (2026-11-13 WB04-E376)
function readDouble(hex) {
    const view = new DataView(new ArrayBuffer(8));
    for (let index = 0; index < 8; index += 1) view.setUint8(index, parseInt(hex.slice(index * 2, index * 2 + 2), 16));
    return view.getFloat64(0, true);
}

// 두 double 이 같은 값인지 본다: 원문 비트가 같거나, 둘 다 NaN 이면 같다(NaN 의 부호/페이로드는 엔진이 정규화한다). (2026-11-13 WB04-E376)
function sameDouble(actual, expected) {
    if (Number.isNaN(expected)) return Number.isNaN(actual);
    const view = new DataView(new ArrayBuffer(16));
    view.setFloat64(0, actual, true);
    view.setFloat64(8, expected, true);
    return view.getBigUint64(0, true) === view.getBigUint64(8, true);
}

// RGBQUAD 배열을 메모리 순서(blue, green, red, reserved)의 소문자 hex 로 바꾼다. 빈 배열은 '-' 이다. (2026-11-13 WB04-E376)
function colorHex(colors, count) {
    let hex = '';
    for (let index = 0; index < count; index += 1) hex += [colors[index].blue, colors[index].green, colors[index].red, colors[index].reserved].map((value) => value.toString(16).padStart(2, '0')).join('');
    return hex === '' ? '-' : hex;
}

// byte hex 를 부분 팔레트의 메모리 배치(blue, green, red, reserved)로 읽는다. (2026-11-13 WB04-E376)
function readColors(hex, colors, offset, count) {
    for (let index = 0; index < count; index += 1) {
        const at = index * 8;
        const [blue, green, red, reserved] = [0, 2, 4, 6].map((delta) => parseInt(hex.slice(at + delta, at + delta + 2), 16));
        colors[offset + index] = { blue, green, red, reserved };
    }
}

// 32비트 DWORD 를 RGBQUAD 메모리 배치의 색으로 읽는다(little endian: byte0 = blue). (2026-11-13 WB04-E376)
const colorOfDword = (value) => ({ blue: value & 255, green: (value >>> 8) & 255, red: (value >>> 16) & 255, reserved: (value >>> 24) & 255 });

test('PaletteFlash: 원본 10.78 번개 256행의 초기화·진행·중단 결과와 장치 호출 순서가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    let cases = 0;
    // 주석을 건너뛰고 각 독립 입력을 복원 모듈에 전달한다.
    for (const row of rows) {
        assert.equal(row.length, 20);
        const [, op, full, offset, durationHex, start, count, wallHex, nowHex, step, solid, paletteHex, initialHex, originalHex, currentHex, intervalHex, lastHex, remaining, initialized, expectedTrace] = row;
        const fullScreen = Number(full) !== 0;
        const state = new PaletteFlashState({ start: Number(start), count: Number(count), offset: Number(offset) | 0, duration: readDouble(durationHex), solid: colorOfDword(Number(solid)) });
        if (op === 'entry') {
            state.offset = thunderOffset(Number(offset) | 0);
            state.duration = thunderDuration(fullScreen);
        }
        const palette = Array.from({ length: 256 }, emptyScreenColor);
        readColors(paletteHex === '-' ? '' : paletteHex, palette, state.start, state.count);
        let wall = readDouble(wallHex);
        const trace = [];
        let reads = 0;
        const flash = new PaletteFlash(state, {
            wallSeconds: () => wall,
            fullScreen: () => fullScreen,
            readPalette: (first, size, destination) => {
                assert.ok(first === state.start && size === state.count);
                for (let index = 0; index < size; index += 1) destination[index] = { ...palette[first + index] };
                reads += 1;
            },
            applyPalette: (first, size, colors) => { trace.push(`apply:${first}:${size}:${size ? colorHex(colors, size) : ''}`); },
            restorePalette: () => { trace.push('restore'); },
        });
        flash.init();
        const where = `행 ${cases} ${op} (${row.slice(2, 11).join(' ')})`;
        assert.equal(colorHex(state.current, state.count), initialHex, `${where} 초기 변경 색`);
        if (op !== 'entry') state.steps = Number(step);
        assert.equal(reads, 2, `${where} 읽기 횟수`);
        assert.equal(colorHex(state.original, state.count), originalHex, `${where} 원래 색`);
        assert.ok(sameDouble(state.interval, readDouble(intervalHex)), `${where} 간격`);
        if (op === 'cancel') flash.cancel();
        else {
            wall = readDouble(nowHex);
            if (!flash.frame()) trace.push('kill');
        }
        assert.equal(reads, 2, `${where} 진행 뒤 읽기 횟수`);
        assert.equal(colorHex(state.original, state.count), originalHex, `${where} 진행 뒤 원래 색`);
        assert.equal(colorHex(state.current, state.count), currentHex, `${where} 진행 뒤 변경 색`);
        assert.ok(sameDouble(state.interval, readDouble(intervalHex)), `${where} 진행 뒤 간격`);
        assert.ok(sameDouble(state.last, readDouble(lastHex)), `${where} 마지막 시각`);
        assert.equal(state.steps, Number(remaining), `${where} 남은 단계`);
        assert.equal(state.initialized, Number(initialized) !== 0, `${where} 초기화 여부`);
        assert.equal(trace.length ? trace.join('|') : '-', expectedTrace, `${where} 장치 순서`);
        cases += 1;
    }
    assert.equal(cases, 256);
});

test('PaletteFlash: 상수와 생성 요청 규칙(밝기 0 은 기본 64, 전체화면 0.5초/창 1/60초)', () => {
    assert.deepEqual([THUNDER_FIRST_COLOR, THUNDER_COLOR_COUNT, THUNDER_STEPS], [228, 18, 64]);
    assert.equal(thunderOffset(0), 64);
    assert.equal(thunderOffset(30), 30);
    assert.equal(thunderOffset(-5), -5);
    assert.equal(thunderDuration(true), 0.5);
    assert.equal(thunderDuration(false), 1 / 60);
    const state = new PaletteFlashState();
    assert.deepEqual([state.start, state.count, state.offset, state.steps, state.initialized], [228, 18, 64, 0, false]);
    assert.equal(state.original.length, 256);
    assert.equal(state.current.length, 256);
    assert.notEqual(state.original[0], state.current[0]);
    // 초기 값: 빈 색은 모두 0, 간격/마지막/지속과 목표 색도 0, 모든 칸이 0 색이다.
    assert.deepEqual(emptyScreenColor(), { red: 0, green: 0, blue: 0, reserved: 0 });
    assert.deepEqual([state.interval, state.last, state.duration], [0, 0, 0]);
    assert.deepEqual(state.solid, { red: 0, green: 0, blue: 0, reserved: 0 });
    assert.ok(state.original.every((color) => color.red === 0 && color.green === 0 && color.blue === 0 && color.reserved === 0));
    assert.ok(state.current.every((color) => color.red === 0 && color.green === 0 && color.blue === 0 && color.reserved === 0));
    // 초기값 일부만 덮어쓸 수 있다.
    const partial = new PaletteFlashState({ start: 10, duration: 2 });
    assert.deepEqual([partial.start, partial.count, partial.duration], [10, 18, 2]);
});

test('PaletteFlash: 화면 모드는 매 프레임 다시 읽고 정지한 게임과 무관하게 실시간의 64단계를 끝낸다', () => {
    let wall = 0;
    let full = false;
    let writes = 0;
    let restores = 0;
    const state = new PaletteFlashState({ duration: thunderDuration(true) });
    const flash = new PaletteFlash(state, {
        wallSeconds: () => wall,
        fullScreen: () => full,
        readPalette: (first, count, destination) => { for (let index = 0; index < count; index += 1) destination[index] = { blue: 30, green: 20, red: 10, reserved: 99 }; },
        applyPalette: () => { writes += 1; },
        restorePalette: () => { restores += 1; },
    });
    flash.init();
    // 창 모드(생성 뒤 전체화면이 아님)에서는 진행만 하고 표시하지 않는다.
    assert.ok(flash.frame() && writes === 0);
    // 전체화면으로 바뀐 뒤 0.125초: 간격(0.5/64 = 0.0078125)의 정확히 16배라 엄격 비교로 15단계만 진행하고(마지막 경과가 간격과 같음) 표시한다.
    full = true;
    wall = 0.125;
    assert.ok(flash.frame() && writes === 1 && restores === 0);
    assert.equal(state.steps, 49);
    // 0.51초: 남은 단계를 한꺼번에 끝내고 저장 팔레트를 복구하며 종료한다.
    wall = 0.51;
    assert.ok(!flash.frame() && restores === 1 && !state.initialized);
    // 이미 끝난 효과의 취소는 두 번째 복구를 하지 않는다.
    flash.cancel();
    assert.equal(restores, 1);
    // 초기화되지 않은 상태의 frame 은 false 이다.
    assert.equal(flash.frame(), false);
});

// 최신 저장 팔레트를 복구하는 간단한 화면 대역이다: saved = LoadPalette 가 정한 저장 팔레트, shown = 현재 표시 팔레트(SetPalette 가 부분 적용). (2026-11-13 WB04-E376)
function fakeScreen() {
    const screen = {
        saved: Array.from({ length: 256 }, emptyScreenColor),
        shown: Array.from({ length: 256 }, emptyScreenColor),
        load(value) {
            this.saved = Array.from({ length: 256 }, () => ({ blue: value, green: value, red: value, reserved: 0 }));
            this.shown = this.saved.map((color) => ({ ...color }));
        },
    };
    return screen;
}

test('PaletteFlash: 중단은 효과 도중 새로 읽은 최신 저장 팔레트를 복구하고 중복 취소를 무시한다', () => {
    const screen = fakeScreen();
    screen.load(20);
    const state = new PaletteFlashState({ duration: 0.5 });
    let wall = 0;
    let restores = 0;
    const flash = new PaletteFlash(state, {
        wallSeconds: () => wall,
        fullScreen: () => true,
        readPalette: (first, count, destination) => { for (let index = 0; index < count; index += 1) destination[index] = { ...screen.shown[first + index] }; },
        applyPalette: (first, count, colors) => { for (let index = 0; index < count; index += 1) screen.shown[first + index] = { ...colors[index] }; },
        restorePalette: () => { restores += 1; screen.shown = screen.saved.map((color) => ({ ...color })); },
    });
    flash.init();
    assert.ok(flash.frame());
    // 밝아진 구름 색(228~245)만 바뀌고 앞뒤 색은 그대로이다(20 + 64 = 84).
    assert.ok(screen.shown[228].red === 84 && screen.shown[227].red === 20 && screen.shown[246].red === 20 && screen.shown[245].red === 84);
    // 효과 도중 새 저장 팔레트를 읽었으면 저장 팔레트 복구는 새 색을 사용한다.
    screen.load(30);
    wall = 0.001;
    assert.ok(flash.frame());
    flash.cancel();
    flash.cancel();
    assert.ok(restores === 1 && screen.shown[228].red === 30 && screen.shown[227].red === 30);
});

test('PaletteFlash: 불완전한 장치 묶음과 부분 팔레트 범위 밖의 색 범위를 거부한다', () => {
    const complete = { wallSeconds: () => 0, fullScreen: () => false, readPalette: () => {}, applyPalette: () => {}, restorePalette: () => {} };
    assert.throws(() => new PaletteFlash(new PaletteFlashState(), {}), TypeError);
    assert.throws(() => new PaletteFlash(new PaletteFlashState(), null), TypeError);
    for (const name of Object.keys(complete)) assert.throws(() => new PaletteFlash(new PaletteFlashState(), { ...complete, [name]: undefined }), TypeError, name);
    assert.throws(() => new PaletteFlash(new PaletteFlashState({ start: 255, count: 2 }), complete), RangeError);
    assert.throws(() => new PaletteFlash(new PaletteFlashState({ start: 257, count: 0 }), complete), RangeError);
    // 경계: 마지막 색 하나와 빈 범위는 허용한다.
    assert.ok(new PaletteFlash(new PaletteFlashState({ start: 255, count: 1 }), complete));
    assert.ok(new PaletteFlash(new PaletteFlashState({ start: 256, count: 0 }), complete));
});

test('PaletteFlash: 밝기는 signed 32비트 덧셈으로 255 에서 제한하고 음수는 byte 로 감기며 offset 0 은 목표 색을 채우고 예약 바이트를 보존한다', () => {
    const run = (offset, color, solid = { blue: 1, green: 2, red: 3, reserved: 4 }) => {
        const state = new PaletteFlashState({ start: 0, count: 1, offset, duration: 1, solid });
        const flash = new PaletteFlash(state, {
            wallSeconds: () => 0,
            fullScreen: () => true,
            readPalette: (first, count, destination) => { destination[0] = { ...color }; },
            applyPalette: () => {},
            restorePalette: () => {},
        });
        flash.init();
        return state.current[0];
    };
    assert.deepEqual(run(64, { blue: 10, green: 200, red: 192, reserved: 77 }), { blue: 74, green: 255, red: 255, reserved: 77 });
    assert.deepEqual(run(-20, { blue: 10, green: 200, red: 192, reserved: 77 }), { blue: 246, green: 180, red: 172, reserved: 77 });
    assert.deepEqual(run(255, { blue: 0, green: 1, red: 255, reserved: 5 }), { blue: 255, green: 255, red: 255, reserved: 5 });
    assert.deepEqual(run(0, { blue: 9, green: 9, red: 9, reserved: 9 }), { blue: 1, green: 2, red: 3, reserved: 4 });
    // 경계: 합이 정확히 255 와 256 일 때.
    assert.equal(run(1, { blue: 254, green: 255, red: 0, reserved: 0 }).blue, 255);
    assert.equal(run(1, { blue: 254, green: 255, red: 0, reserved: 0 }).green, 255);
    assert.equal(run(1, { blue: 254, green: 255, red: 0, reserved: 0 }).red, 1);
});
