// 사제 604 의식 프레임 raw-priest-ritual-frame.js(RawPriestRitualFrame)를 원본 10.78 관측과 대조한다.
//  - 원본 관측: priest-ritual-frame-x86.tsv(10.78 전용 단일 판본) 1050행 — 모드(0 실제 사제 코드표 / 1 선언한 짧은 코드 하나)·현재 프레임·side 글자·605 존재·변이 단계 → 반환 float 4바이트, 호출 기록(E 605 조회 / S 위치 소리 / F 프레임 지정), 사제 슬롯 50 raw.
//    모드 0 은 원본 `netstorm.tarc` 의 사제 타입 정의에서 만든 실제 코드표를 쓴다. 픽셀 장치는 명시 경계이다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E70: 사제 의식 프레임 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT, readData } from './helpers.mjs';
import { readRows, put, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { TaffArchive } from '../www/assets/taff.js';
import { decodeOriginalText } from '../www/assets/text-encoding.js';
import { TypeTable } from '../www/assets/type-table.js';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { RawPriestRitualFrame } from '../www/core/raw-priest-ritual-frame.js';

// 원본 관측 행이다(단일 판본 파일). 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E70)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'priest-ritual-frame-x86.tsv'));

// 원본 독립 입력 수, 관찰 사제 SID, 사제 타입 번호와 두 번째 타입 번호이다. (2026-11-08 WB04-E70)
const ROWS = 1050;
const PRIEST = 50;
const PRIEST_TYPE = 158;
const OTHER_TYPE = 133;

// 예외가 나는지 본다. (2026-11-08 WB04-E70)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E70)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 단정도 float 의 네 바이트 hex 이다. (2026-11-08 WB04-E70)
const floatHex = (value) => {
    const view = new DataView(new ArrayBuffer(4));
    view.setFloat32(0, value, true);
    return hex(new Uint8Array(view.buffer));
};

test('PriestRitualFrame: 원본 10.78 1050행의 반환 float·호출 기록·사제 슬롯 50 raw 가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    const table = TypeTable.fromArchive(new TaffArchive(readData('netstorm.tarc')), decodeOriginalText);
    const actual = FrameCodeTable.fromDefinition(table.definitions[PRIEST_TYPE - 70]);
    const pool = new SidPool(24000, { server: true });
    let count = 0;
    // 원본과 같은 TYPE/frame 입력으로 매 행을 새로 만든다.
    for (const row of rows) {
        assert.equal(row.length, 4);
        const input = row[0].split(',').map(Number);
        assert.equal(input.length, 5);
        const [mode, current, side, exists, mutation] = input;
        const frames = Array.from({ length: 188 }, () => new FrameCodeTable([]));
        frames[PRIEST_TYPE] = mode ? new FrameCodeTable([{ side, variant: 0x50, flags: 1 }]) : actual;
        frames[OTHER_TYPE] = frames[PRIEST_TYPE];
        const raw = pool.physical(PRIEST);
        raw.fill(0xa5);
        raw[10] = PRIEST_TYPE;
        put(raw, 14, 0x41a40000);
        put(raw, 18, 0x41ac0000);
        put(raw, 36, current);
        const trace = [];
        // 원문과 같은 실제 요청 순서이다.
        const hooks = {
            setFrame: (sid, frame, flags) => { trace.push(`F:${sid}:${frame >>> 0}:${flags}`); put(raw, 36, frame >>> 0); },
            eventExists: (sid, number) => {
                trace.push(`E:${sid}:${number}`);
                if (mutation === 1) { put(raw, 14, 0x41b20000); put(raw, 18, 0xc18c0000); put(raw, 36, 99); }
                return exists !== 0;
            },
            playAt: (x, y, name, flags) => {
                trace.push(`S:${floatToBits(x)}:${floatToBits(y)}:${name}:${flags}`);
                if (mutation === 2) { raw[10] = OTHER_TYPE; put(raw, 36, 77); }
            }
        };
        const result = new RawPriestRitualFrame(pool, frames, hooks).tick(PRIEST);
        assert.equal(floatHex(result), row[1], `행 ${count}: ${row[0]}`);
        assert.equal(trace.length === 0 ? '-' : trace.join(';'), row[2], `행 ${count}: ${row[0]}`);
        assert.equal(hex(raw), row[3], `행 ${count}: ${row[0]}`);
        count += 1;
    }
    assert.equal(count, ROWS);
});

test('PriestRitualFrame: 판본/표 크기/누락한 필수 효과는 첫 상태 변경 전에 거부하고 범위 밖 타입을 거부한다', () => {
    const pool = new SidPool(24000, { server: true });
    const frames = Array.from({ length: 188 }, () => new FrameCodeTable([]));
    const hooks = { setFrame: () => {}, eventExists: () => false, playAt: () => {} };
    assert.doesNotThrow(() => new RawPriestRitualFrame(pool, frames, hooks));
    assert.ok(throws(() => new RawPriestRitualFrame(pool, frames.slice(1), hooks)));
    assert.ok(throws(() => new RawPriestRitualFrame(pool, frames, { ...hooks, playAt: undefined })));
    assert.ok(throws(() => new RawPriestRitualFrame({ edition: 'cd1072' }, frames, hooks)));
    const raw = pool.physical(PRIEST);
    raw.fill(0);
    raw[10] = 200;
    assert.ok(throws(() => new RawPriestRitualFrame(pool, frames, hooks).tick(PRIEST)));
});
