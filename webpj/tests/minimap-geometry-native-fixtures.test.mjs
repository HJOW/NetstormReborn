// 미니맵 순수 기하(MinimapGeometry, 원본 00480270/004803f0/00480440/004805e0/00480660)의 원본 10.78 독립 관측 대조: 다섯 진입점(rebuild·move·idle·attach·draw)의 1,028행(callback 변경이 없는 관측)을
// 실행 뒤 state 5필드·view 8필드의 원시 비트와 원문 경계(중심 목표 float 비트·블릿 원점/크기·카메라 외곽 사각형·재구성 억제)에 대해 cpppj `MinimapGeometryTests` 와 같은 입력 구성으로 비교한다.
// 앞 20열(입력)만 순수 API 에 공급하고 뒤 7열(관측)은 비교에만 쓴다. callback 재조회·부분 중단 26행은 외부 호출 순서라 이 순수 API 의 범위 밖이므로 cpppj 와 같이 계수만 하고 제외한다.
// 원본 게임을 실행하지 않는다. (2026-11-15 WB04-E489)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import { MinimapGeometryState, clampMinimapAttachment, dragMinimap, minimapTarget, planMinimapDraw, rebuildMinimap } from '../www/ui/minimap-geometry.js';

// 공유 128×128 bitmap 의 한 변(native blit 의 source 주소 행 폭)과 진입점 이름/기대 행 수이다.
const BITMAP_STRIDE = 128; const OPERATIONS = ['rebuild', 'move', 'idle', 'attach', 'draw']; const EXPECTED_PURE_ROWS = [254, 36, 216, 272, 250];
// float32 와 비트 사이의 공용 보기이다.
const f32 = new Float32Array(1); const u32 = new Uint32Array(f32.buffer);
const bitsOf = (value) => { f32[0] = value; return u32[0]; }; const floatOf = (bits) => { u32[0] = bits >>> 0; return f32[0]; };
// 십진 입력의 하위 32비트와 부호 있는 해석이다.
const number = (text) => Number(BigInt.asUintN(32, BigInt(text))); const signed = (value) => value | 0;
// 실행 뒤 전체 기하 필드/입력 view 를 독립 stateout/globalsout 과 같은 순서의 문자열로 만든다.
const stateOutput = (state) => `${bitsOf(state.width)}:${bitsOf(state.height)}:${state.cropX >>> 0}:${state.cropY >>> 0}:${state.scrolling >>> 0}`;
const viewOutput = (view) => `${view.camera[0] >>> 0}:${view.camera[1] >>> 0}:${view.viewport.left >>> 0}:${view.viewport.top >>> 0}:${view.viewport.right >>> 0}:${view.viewport.bottom >>> 0}:${view.pixelsX >>> 0}:${view.pixelsY >>> 0}`;
// 원문 trace 에서 같은 이름의 실제 외부 호출 한 묶음의 인자(`name:a:b...`)를 찾는다(정확히 하나여야 한다).
function nativeEvent(row, name) {
    const found = row[23].split(';').filter((event) => event.startsWith(`${name}:`)); assert.equal(found.length, 1, `${row[0]} ${name}`);
    return found[0].split(':');
}
// 원문이 중심 경계에 넘긴 두 float32 비트를 순수 API 의 목표와 비교한다.
function checkCenter(row, target, problems) { const event = nativeEvent(row, 'center'); if (event.length !== 3 || bitsOf(target[0]) !== number(event[1]) || bitsOf(target[1]) !== number(event[2])) problems.push('center'); }
// 원문 source 주소/대상/크기와 exclusive 카메라 프레임을 전체 Draw 계획과 비교한다.
function checkDraw(row, plan, problems) {
    const blit = nativeEvent(row, 'blit'); const frame = nativeEvent(row, 'frame'); if (blit.length !== 9 || frame.length !== 6) { problems.push('draw-shape'); return; }
    const offset = signed(Math.imul(plan.source.y, BITMAP_STRIDE) + plan.source.x);
    if (plan.destination.x !== signed(number(blit[1])) || plan.destination.y !== signed(number(blit[2])) || offset !== signed(number(blit[3]))) problems.push('blit-origin');
    if (number(blit[4]) !== BITMAP_STRIDE || blit[5] !== '0' || blit[6] !== '0') problems.push('blit-stride');
    if (plan.source.x !== signed(number(row[4])) || plan.source.y !== signed(number(row[5])) || plan.size.x !== signed(number(blit[7])) || plan.size.y !== signed(number(blit[8]))) problems.push('blit-size');
    if (plan.camera.left !== signed(number(frame[1])) || plan.camera.top !== signed(number(frame[2])) || plan.camera.right !== signed(number(frame[3])) || plan.camera.bottom !== signed(number(frame[4]))) problems.push('frame');
}

test('MinimapGeometry 원본 10.78 1,028행: 다섯 진입점의 state/view 원시 비트·중심 목표·블릿 계획·카메라 외곽·재구성 억제가 일치한다', () => {
    const table = fs.readFileSync(new URL('../../cpppj/tests/fixtures/minimap-geometry-x86.tsv', import.meta.url), 'utf8').split(/\r?\n/).filter((line) => line.length > 0 && !line.startsWith('#')).map((line) => line.split('\t'));
    assert.equal(table.length, 1054);
    const failures = []; let compared = 0; let excluded = 0; const counts = [0, 0, 0, 0, 0];
    for (const [index, row] of table.entries()) {
        assert.equal(row.length, 27, `행 ${index}`);
        // callback 재조회·부분 중단이 있는 행은 순수 API 가 소유하지 않는 외부 호출 순서이다.
        if (row[19] !== '-') { excluded += 1; continue; }
        const state = new MinimapGeometryState(); state.width = floatOf(number(row[2])); state.height = floatOf(number(row[3])); state.cropX = signed(number(row[4])); state.cropY = signed(number(row[5])); state.scrolling = number(row[6]);
        const view = { camera: [number(row[7]), number(row[8])], viewport: { left: signed(number(row[9])), top: signed(number(row[10])), right: signed(number(row[11])), bottom: signed(number(row[12])) }, pixelsX: signed(number(row[13])), pixelsY: signed(number(row[14])) };
        const localX = floatOf(number(row[15])); const localY = floatOf(number(row[16])); const destination = { x: signed(number(row[17])), y: signed(number(row[18])) };
        const operation = OPERATIONS.indexOf(row[1]); assert.ok(operation >= 0, row[1]); counts[operation] += 1; const problems = [];
        if (row[1] === 'rebuild') { const rebuilt = rebuildMinimap(state, view); if (rebuilt !== (row[23] === 'dirty:0')) problems.push('rebuilt'); }
        else if (row[1] === 'move') checkCenter(row, minimapTarget(state, localX, localY), problems);
        else if (row[1] === 'idle') checkCenter(row, dragMinimap(state, localX, localY), problems);
        else if (row[1] === 'attach') clampMinimapAttachment(state);
        else checkDraw(row, planMinimapDraw(state, view, destination), problems);
        if (stateOutput(state) !== row[21]) problems.push('state');
        if (viewOutput(view) !== row[22]) problems.push('view');
        if (row[20] !== '1' || row[24] !== '0') problems.push('completion');
        compared += 1;
        if (problems.length) failures.push(`${index}/${row[0]}: ${problems.join(',')}`);
    }
    assert.deepEqual(failures.slice(0, 20), []);
    assert.equal(compared, 1028); assert.equal(excluded, 26); assert.deepEqual(counts, EXPECTED_PURE_ROWS);
});
