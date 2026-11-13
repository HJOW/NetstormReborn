// Renderer 구름 4칸·시차 갱신·배경 선택 render/raw-renderer-clouds.js 를 원본 10.78 관측(renderer-clouds-x86.tsv 695행)과 cpppj `RendererCloudTests.cpp` 의 선택 장치/물리 범위 시험, 웹 계약으로 검사한다.
//  - 관측 한 행 = 독립 입력 한 건(함수 번호 0 추가·1 시차 갱신·2 배경 선택, 4칸 개수·진단·mode·묶음 숨김·dirty·덮임·날씨색·검정·카메라 X/Y·분모·clip 네 값·하위 효과 변형 번호)이다.
//    출력 31 DWORD(개수 1·4칸 16·현재 전역 9·clip 4·IDIV 오류 표식 1)와 호출 순서(A 진단·F 지우기·C 일반 구름·P 시차 구름 + 원시 인수)를 비교한다. 정상 671행과 실제 IDIV 오류 24행이다.
//    기대 분기는 다시 계산하지 않고 명시한 하위 효과(개수 변경·모드/날씨색/clip 변경)만 호출 직후 입력으로 공급한다.
//  - cpppj 계약 시험: 4칸 경계의 자연 생략/선택된 진단/제공하지 않은 앞선 메모리 쓰기 구분과 음수 개수의 시차 갱신.
//    `RendererCloud_FrameGlobalsShareActualPreludeQueue`(실제 ClientRendererFrameGlobals 의 4칸 공유)는 그 전역 소유자를 옮기는 ClientRendererFrame 단위에서 한다.
//  - 웹 계약: 저장은 호출자 사각형의 사본이다(원본 객체를 바꿔도 저장 칸이 따라가지 않음)·시차 갱신의 훅 인자도 사본·IDIV 오류와 물리 범위 오류의 구별·카메라/분모는 매 반복에서 다시 읽음.
// 실제 게임·브라우저 실행이 아니다. (2026-11-13 WB04-E383)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';
import { appendRendererCloudDirty, drawRendererBackdrop, refreshRendererCloudBackground, RendererCloudDivideError } from '../www/render/raw-renderer-clouds.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-13 WB04-E383)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'renderer-clouds-x86.tsv'));
// 사각형 직렬화(부호 없는 DWORD 네 개)이다. (2026-11-13 WB04-E383)
const text = (rect) => [rect.left, rect.top, rect.right, rect.bottom].map((value) => value >>> 0).join(',');
// 상자 만들기이다. (2026-11-13 WB04-E383)
const box = (value) => ({ value });

// 한 관측 행을 재생한다. (2026-11-13 WB04-E383)
function replay(row) {
    const input = row.slice(0, 17).map(Number);
    const expected = row.slice(17, 48).map(Number);
    const word = (index) => input[index] >>> 0;
    const integer = (index) => input[index] | 0;
    // 기존 미사용 칸도 원문과 같은 보존 입력이다(16 DWORD 를 네 사각형으로 본다).
    const initial = Array.from({ length: 16 }, (_, i) => (i * 17 - 3) >>> 0);
    const rectangles = [0, 1, 2, 3].map((i) => ({ left: initial[i * 4] | 0, top: initial[i * 4 + 1] | 0, right: initial[i * 4 + 2] | 0, bottom: initial[i * 4 + 3] | 0 }));
    const queueCount = box(word(1));
    const checking = word(2);
    const mode = box(word(3)); const hidden = box(word(4)); const dirty = box(word(5)); const covered = box(word(6)); const tint = box(word(7)); const black = box(word(8));
    const cameraX = box(integer(9)); const cameraY = box(integer(10)); const denominator = box(integer(11));
    const clipped = { left: integer(12), top: integer(13), right: integer(14), bottom: integer(15) };
    const events = [];
    const changed = new Set();
    // 명시 하위 효과만 공급하며 기대 분기를 다시 계산하지 않는다.
    const effect = (label) => {
        if (changed.has(label)) return;
        changed.add(label);
        const mutation = input[16];
        if (label === 'A' && (mutation === 1 || mutation === 2)) queueCount.value = mutation === 1 ? 1 : 0x7fffffff;
        else if (label === 'P' && mutation === 3) queueCount.value = 1;
        else if (label === 'P' && (mutation === 4 || mutation === 5)) { queueCount.value = 3; denominator.value = mutation === 4 ? 7 : 0; cameraX.value = -17; cameraY.value = 23; rectangles[1] = { left: 101, top: 103, right: 107, bottom: 109 }; }
        else if (label === 'P' && mutation === 10) queueCount.value = 0xffffffff;
        else if (label === 'F' && (mutation === 6 || mutation === 7 || mutation === 9)) {
            mode.value = mutation === 6 ? 0 : 24; tint.value = 247; dirty.value = mutation === 9 ? 4 : 0; covered.value = mutation === 9 ? 1 : 0;
            denominator.value = 3; cameraX.value = -17; cameraY.value = 23; Object.assign(clipped, { left: 10, top: 20, right: 30, bottom: 40 });
        }
    };
    const hooks = {
        assertion: (at) => { events.push(`A:${at}`); effect('A'); },
        clear: (rect, color) => { events.push(`F:${text(rect)},${color}`); effect('F'); },
        draw: (rect, x, y, alternate) => { const label = alternate ? 'P' : 'C'; events.push(`${label}:${text(rect)},${x >>> 0},${y >>> 0}`); effect(label); },
    };
    let fault = false;
    try {
        if (input[0] === 0) appendRendererCloudDirty({ rectangles, count: queueCount }, clipped, checking, hooks);
        else if (input[0] === 1) refreshRendererCloudBackground({ rectangles, count: queueCount }, cameraX, cameraY, denominator, hooks);
        else drawRendererBackdrop(clipped, dirty, covered, { mode, hideGroups: hidden, tint, black, cameraX, cameraY, denominator }, hooks);
    } catch (error) {
        if (!(error instanceof RendererCloudDivideError)) throw error;
        fault = true;
    }
    const actual = [queueCount.value >>> 0];
    // 기존 4칸 전체를 비교한다.
    for (const rect of rectangles) actual.push(rect.left >>> 0, rect.top >>> 0, rect.right >>> 0, rect.bottom >>> 0);
    actual.push(mode.value >>> 0, hidden.value >>> 0, dirty.value >>> 0, covered.value >>> 0, tint.value >>> 0, black.value >>> 0, cameraX.value >>> 0, cameraY.value >>> 0, denominator.value >>> 0,
        clipped.left >>> 0, clipped.top >>> 0, clipped.right >>> 0, clipped.bottom >>> 0, fault ? 1 : 0);
    return { actual, expected, events: events.length ? events.join(';') : '-', expectedEvents: row[48], fault };
}

test('Renderer 구름: 원본 10.78 관측 695행(정상 671·실제 IDIV 오류 24)의 4칸·개수·전역·clip·호출 순서가 모두 일치한다', { skip: rows === null }, () => {
    assert.equal(rows.length, 695);
    let faults = 0;
    for (const row of rows) {
        assert.equal(row.length, 49);
        const { actual, expected, events, expectedEvents, fault } = replay(row);
        assert.deepEqual(actual, expected, `관측 ${row.slice(0, 17).join(',')}`);
        assert.equal(events, expectedEvents, `호출 ${row.slice(0, 17).join(',')}`);
        if (fault) faults += 1;
    }
    assert.equal(faults, 24);
});

test('Renderer 구름: 4칸 경계의 자연 생략과 선택된 진단, 제공하지 않은 앞선 메모리 쓰기를 구분한다(RendererCloud_SelectedDevicesAndPhysicalBounds)', () => {
    const rectangles = Array.from({ length: 4 }, () => ({ left: 0, top: 0, right: 0, bottom: 0 }));
    const count = box(4);
    appendRendererCloudDirty({ rectangles, count }, { left: 1, top: 2, right: 3, bottom: 4 }, 0, {});
    assert.equal(count.value, 4);
    assert.throws(() => appendRendererCloudDirty({ rectangles, count }, { left: 1, top: 2, right: 3, bottom: 4 }, 1, {}), /선택된 실제 장치 미연결/);
    assert.equal(count.value, 4);
    count.value = 0xffffffff;
    assert.throws(() => appendRendererCloudDirty({ rectangles, count }, { left: 1, top: 2, right: 3, bottom: 4 }, 0, {}), (error) => error instanceof RangeError && !(error instanceof RendererCloudDivideError));
    assert.equal(count.value, 0xffffffff);
    // 음수 개수는 IDIV/표시도 요구하지 않고 count 0 만 쓴다.
    refreshRendererCloudBackground({ rectangles, count }, box(0), box(0), box(0), {});
    assert.equal(count.value, 0);
});

test('Renderer 구름 계약: 저장 칸과 훅 인자는 호출자 사각형의 사본이고 IDIV 오류와 물리 범위 오류를 구별한다', () => {
    const rectangles = Array.from({ length: 4 }, () => ({ left: 0, top: 0, right: 0, bottom: 0 }));
    const count = box(0);
    const source = { left: 5, top: 6, right: 7, bottom: 8 };
    appendRendererCloudDirty({ rectangles, count }, source, 0, {});
    source.left = 99;
    assert.deepEqual(rectangles[0], { left: 5, top: 6, right: 7, bottom: 8 });
    assert.equal(count.value, 1);
    // 시차 갱신의 훅 인자가 사본이라 훅이 바꿔도 저장 칸은 그대로이다.
    const drawn = [];
    refreshRendererCloudBackground({ rectangles, count }, box(-17), box(23), box(3), { draw: (rect, x, y, alternate) => { drawn.push([x, y, alternate]); rect.left = 77; } });
    assert.deepEqual(drawn, [[-5, 7, true]]);
    assert.equal(rectangles[0].left, 5);
    assert.equal(count.value, 0);
    // 분모 0 / INT_MIN÷−1 은 IDIV 오류이고 count 는 0 이 되지 않는다.
    count.value = 1;
    assert.throws(() => refreshRendererCloudBackground({ rectangles, count }, box(0), box(1), box(0), { draw: () => {} }), RendererCloudDivideError);
    assert.throws(() => refreshRendererCloudBackground({ rectangles, count }, box(0), box(-0x80000000), box(-1), { draw: () => {} }), RendererCloudDivideError);
    assert.equal(count.value, 1);
    // 4칸을 넘는 개수는 칸 뒤 읽기 물리 오류이다.
    count.value = 5;
    const seen = [];
    assert.throws(() => refreshRendererCloudBackground({ rectangles, count }, box(0), box(0), box(1), { draw: () => { seen.push(1); } }), (error) => error instanceof RangeError && !(error instanceof RendererCloudDivideError));
    assert.equal(seen.length, 4);
    // 배경 선택: 시차 없는 일반 구름은 카메라 원점 그대로, 시차는 몫.
    const clipped = { left: 1, top: 2, right: 30, bottom: 40 };
    const calls = [];
    const g = { mode: box(0x08), hideGroups: box(0), tint: box(7), black: box(9), cameraX: box(-17), cameraY: box(23), denominator: box(3) };
    drawRendererBackdrop(clipped, box(0), box(0), g, { draw: (rect, x, y, alt) => calls.push([text(rect), x, y, alt]), clear: () => assert.fail('지우기 없음') });
    g.mode.value = 0x18;
    drawRendererBackdrop(clipped, box(0), box(0), g, { draw: (rect, x, y, alt) => calls.push([text(rect), x, y, alt]), clear: () => assert.fail('지우기 없음') });
    assert.deepEqual(calls, [['1,2,30,40', -17, 23, false], ['1,2,30,40', -5, 7, false]]);
});

test('Renderer 구름 계약: 배경 지우기 색은 검정(묶음 숨김)·날씨색·255 그대로이고 dirty 비트 4 만 UI 전용이며 덮임은 어느 값이든 끝낸다', () => {
    const clears = [];
    const hooks = { clear: (rect, color) => clears.push(color), draw: () => clears.push('draw') };
    const globals = (mode, extra = {}) => ({ mode: box(mode), hideGroups: box(0), tint: box(0xfedcba98), black: box(5), cameraX: box(0), cameraY: box(0), denominator: box(1), ...extra });
    const run = (dirty, covered, g) => { clears.length = 0; drawRendererBackdrop({ left: 0, top: 0, right: 9, bottom: 9 }, box(dirty), box(covered), g, hooks); return [...clears]; };
    // 묶음 숨김은 날씨색과 무관하게 검정 값 그대로(0 이 아닌 DWORD)로 먼저 지운다.
    assert.deepEqual(run(0, 0, globals(0, { hideGroups: box(1) })), [5, 0xfedcba98]);
    assert.deepEqual(run(4, 0, globals(0, { hideGroups: box(2) })), [5]);
    // dirty 는 비트 4 만 UI 전용이다(1·2·8 은 아니다). 덮임은 0 이 아니면 끝낸다.
    for (const dirty of [1, 2, 3, 8, 0xfffffffb]) assert.deepEqual(run(dirty, 0, globals(0)), [0xfedcba98], `dirty ${dirty}`);
    for (const dirty of [4, 5, 0xffffffff]) assert.deepEqual(run(dirty, 0, globals(0)), [], `dirty ${dirty}`);
    for (const covered of [1, 2, 0x80000000]) assert.deepEqual(run(0, covered, globals(0)), [], `covered ${covered}`);
    // 구름 표시 비트가 있고 시차+플래그 조합(16·1 켜짐, 2 꺼짐)이면 255, 그 밖에는 구름을 그린다.
    assert.deepEqual(run(0, 0, globals(0x19)), [255]);
    assert.deepEqual(run(0, 0, globals(0x1b)), ['draw']);
    assert.deepEqual(run(0, 0, globals(0x18)), ['draw']);
    assert.deepEqual(run(0, 0, globals(0x09)), ['draw']);
});
