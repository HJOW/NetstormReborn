// 레인 신전 가상 Draw(004dcd00)의 그림/소리 제출 순서를 원본 x86 fixture 로 대조한다(cpppj RainDrawTests 의 10.78 행).
//  - rain-draw-x86.tsv originals 116행: 현재 프레임(+36 DWORD)·WORD 상태(+12)·제출 순서(F번호·S 소리). originalCD 116행은 범위 밖이다.
//  - 입력 코드 표는 cpppj 시험과 같은 29클러스터(A·B×14·C×13·P) 방향 표이며 기대 순서를 다시 계산하지 않는다. 호출 뒤 raw 가 바뀌지 않았는지도 본다.
//  - cpppj 의 Renderer 합성 두 시험(RainDraw_Renderer*)은 client Renderer 그림 겹침 이식 몫이라 여기서 옮기지 않는다. 거부 경로는 웹 계약 검사다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E203: 신전·마나 추가 그림 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put } from './raw-support.mjs';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { isRainVortex, drawRainVortex } from '../www/core/raw-rain-vortex-draw.js';

// 원본 x86 fixture 의 10.78 행이다. (2026-11-09 WB04-E203)
const rows = (readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'rain-draw-x86.tsv')) ?? []).filter((row) => row[0] === 'originals');

// 예외가 지정 종류로 나는지 본다. (2026-11-09 WB04-E203)
function throws(action, kind = Error) {
    try { action(); } catch (error) { return error instanceof kind; }
    return false;
}

// 원본 .type 순서와 같은 입력 코드 표이다(방향 글자만 Draw 에 필요하다). (2026-11-09 WB04-E203)
function rainFrames() {
    const letters = `A${'B'.repeat(14)}${'C'.repeat(13)}P`;
    return new FrameCodeTable([...letters].map((letter) => ({ side: letter.charCodeAt(0), variant: 0, flags: 0 })));
}

// 10.78 레인 신전 raw 입력이다. (2026-11-09 WB04-E203)
function rainRaw(frame, state) {
    const raw = new Uint8Array(50);
    put(raw, 0, 0x5196f0);
    raw[10] = 124;
    put(raw, 12, state, 2);
    put(raw, 36, frame);
    return raw;
}

test('RainDraw: 원본 10.78 116행의 그림/소리 제출 순서가 같다', { skip: rows.length === 0 && 'rain-draw-x86.tsv 없음' }, () => {
    const frames = rainFrames();
    let count = 0;
    // 같은 입력을 웹 모듈에만 제공한다.
    for (const row of rows) {
        assert.equal(row.length, 4);
        const raw = rainRaw(Number(row[1]), Number(row[2]));
        const before = raw.slice();
        const trace = [];
        drawRainVortex(raw, frames, { frame: (value) => trace.push(`F${value}`), ascendSound: () => trace.push('S') });
        assert.equal(trace.join(','), row[3], `행 ${count} frame ${row[1]} state ${row[2]}`);
        assert.deepEqual(raw, before);
        count += 1;
    }
    assert.equal(count, 116);
});

test('RainDraw: 다른 가상 표·소비자 누락·방향 프레임 범위 밖을 거부하고 콜백 뒤 현재 raw 를 다시 읽는다(웹 계약 검사)', () => {
    const frames = rainFrames();
    const raw = rainRaw(15, 1);
    assert.equal(isRainVortex(raw), true);
    assert.ok(throws(() => drawRainVortex(raw, frames, { frame() {} }), TypeError));
    assert.ok(throws(() => drawRainVortex(raw, frames, { ascendSound() {} }), TypeError));
    put(raw, 0, 0x501d28);
    assert.equal(isRainVortex(raw), false);
    assert.ok(throws(() => drawRainVortex(raw, frames, { frame() {}, ascendSound() {} }), TypeError));
    // 'C' 개수보다 작지만 코드 표 밖인 음수 프레임은 범위 오류다.
    const negative = rainRaw(-1, 1);
    assert.ok(throws(() => drawRainVortex(negative, frames, { frame() {}, ascendSound() {} }), RangeError));
    // 소리 소비자가 현재 프레임을 바꾸면 마지막 추가 그림은 바뀐 값을 쓴다.
    const live = rainRaw(1, 1);
    const trace = [];
    drawRainVortex(live, frames, { frame: (value) => trace.push(value), ascendSound: () => { trace.push('S'); put(live, 36, 3); } });
    assert.deepEqual(trace, [0, 1, 'S', frames.run('C').first + 3]);
});
