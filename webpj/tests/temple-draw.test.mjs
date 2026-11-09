// 세 신전(윈드·레인·썬더)의 실제 기본 프레임 반환과 전용 그림/소리 제출 순서를 원본 x86 fixture 로 대조한다(cpppj TempleDrawTests, 10.78).
//  - temple-draw-x86.tsv 264행: 신전 이름·관측 입력·기본 프레임(부호 없는 DWORD)·제출 순서(F번호·S 소리, 없으면 '-')·50바이트 raw.
//  - 각 행은 raw 와 TYPE 정의(baseFrame·프레임 코드 표)만 공급하고 모듈이 계산한다. 기대 열은 비교에만 쓰며, 호출 뒤 raw 가 바뀌지 않았는지도 본다.
//  - 거부 경로는 웹 계약 검사다. 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E203: 신전·마나 추가 그림 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put } from './raw-support.mjs';
import { realCaptureAssets } from './capture-assets-support.mjs';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { isPatchTemple, patchTempleMainFrame, drawWindThunderTemple } from '../www/core/raw-temple-draw.js';
import { drawRainVortex } from '../www/core/raw-rain-vortex-draw.js';

// 원본 x86 fixture 이다. (2026-11-09 WB04-E203)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'temple-draw-x86.tsv')) ?? [];

// 예외가 지정 종류로 나는지 본다. (2026-11-09 WB04-E203)
function throws(action, kind = Error) {
    try { action(); } catch (error) { return error instanceof kind; }
    return false;
}

// 16진 문자열의 바이트 배열이다. (2026-11-09 WB04-E203)
const bytesOf = (text) => Uint8Array.from(Buffer.from(text, 'hex'));

// TYPE 번호로 찾는 188칸 baseFrame 표이다(내장·여분 칸은 -1). (2026-11-09 WB04-E203)
function baseFrames(table) {
    // 실제 로딩 순서 정의의 baseFrame 을 번호 칸에 넣는다.
    return Array.from({ length: 188 }, (_, number) => (number >= 70 && table.definitions[number - 70] ? table.definitions[number - 70].baseFrame : -1));
}

test('TempleDraw: 원본 264행의 기본 프레임과 전용 그림/소리 제출 순서가 같다', { skip: rows.length === 0 && 'temple-draw-x86.tsv 없음' }, () => {
    const { table } = realCaptureAssets();
    const bases = baseFrames(table);
    let count = 0;
    // 모든 실제 기본 반환/원본 Draw 호출을 한 행도 생략하지 않는다.
    for (const row of rows) {
        assert.equal(row.length, 6);
        const raw = bytesOf(row[5]);
        assert.equal(raw.length, 50);
        const before = raw.slice();
        const main = patchTempleMainFrame(raw, bases);
        assert.equal(main >>> 0, Number(row[3]), `기본 프레임 행 ${count}`);
        const number = table.find(row[0]);
        assert.ok(number >= 70, row[0]);
        const frames = FrameCodeTable.fromDefinition(table.definitions[number - 70]);
        const trace = [];
        const frame = (value) => trace.push(`F${value}`);
        if (row[0] === 'rainVortex') drawRainVortex(raw, frames, { frame, ascendSound: () => trace.push('S') });
        else drawWindThunderTemple(raw, frames, main, frame);
        assert.equal(trace.length ? trace.join(',') : '-', row[4], `${row[0]} 행 ${count}`);
        assert.deepEqual(raw, before);
        count += 1;
    }
    assert.equal(count, 264);
});

test('TempleDraw: 다른 가상 표·표 밖 TYPE·소비자 누락을 그리기 전에 거부한다(웹 계약 검사)', () => {
    const { table } = realCaptureAssets();
    const bases = baseFrames(table);
    const frames = FrameCodeTable.fromDefinition(table.definitions[table.find('windVortex') - 70]);
    const raw = new Uint8Array(50);
    raw[10] = table.find('windVortex');
    // 세 정확한 가상 표만 신전이다.
    for (const [table32, accepted] of [[0x519860, true], [0x5196f0, true], [0x519918, true], [0x519860 + 8, false], [0, false]]) {
        put(raw, 0, table32);
        assert.equal(isPatchTemple(raw), accepted);
    }
    put(raw, 0, 0x519860 + 8);
    assert.ok(throws(() => patchTempleMainFrame(raw, bases), TypeError));
    put(raw, 0, 0x519860);
    assert.equal(patchTempleMainFrame(raw, bases), bases[raw[10]]);
    assert.ok(throws(() => patchTempleMainFrame(raw, bases.slice(0, raw[10])), TypeError));
    // 레인 표는 윈드/썬더 전용 Draw 가 받지 않고, 소비자가 없으면 거부한다.
    assert.ok(throws(() => drawWindThunderTemple(raw, frames, 0, null), TypeError));
    put(raw, 0, 0x5196f0);
    assert.ok(throws(() => drawWindThunderTemple(raw, frames, 0, () => {}), TypeError));
    // 슬롯이 짧으면 범위 오류다.
    assert.ok(throws(() => isPatchTemple(new Uint8Array(3)), RangeError));
});
