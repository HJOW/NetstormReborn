// 다이스 추가 그림 raw-dais-draw.js(isPatchDais·drawDaisAdditional)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: dais-draw-x86.tsv 768행(두 x87 정밀도 동일 결과) — 입력 6열(프레임 DWORD·TYPE 169 프레임 수·상태 WORD·로그 표식 세 개) → 497280 소비자에 넘기는 요청 `draw:50:표식:표식:표식:프레임(unsigned)`.
//    함수 자체에 dead/추상 필터가 없으며 슬롯은 변하지 않는다(signed idiv 의 부호/감김을 포함).
//  - 계약: DaisDraw_RejectsUnboundContext(다른 VT/TYPE 이나 손상된 프레임 수/소비자 누락을 일반 다이스 효과로 처리하지 않음).
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E84: 다이스 추가 그림 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put } from './raw-support.mjs';
import { isPatchDais, drawDaisAdditional } from '../www/core/raw-dais-draw.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E84)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'dais-draw-x86.tsv'));

// 독립 입력 수와 다이스의 가상 표이다. (2026-11-08 WB04-E84)
const ROWS = 768;
const DAIS_TABLE = 0x507950;

// 예외가 나는지 본다. (2026-11-08 WB04-E84)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

test('DaisDraw: 원본 10.78 768입력의 signed idiv 부호/감김과 dead/추상 필터 부재가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    let count = 0;
    // 두 원본 정밀도의 동일 결과를 한 행도 생략하지 않는다.
    for (const row of rows) {
        assert.equal(row.length, 3);
        const input = row[0].split(',').map(Number);
        assert.equal(input.length, 6);
        const raw = new Uint8Array(50);
        put(raw, 0, DAIS_TABLE);
        raw[10] = 165;
        raw[11] = input[2] & 0xff;
        raw[40] = (input[2] >>> 8) & 0xff;
        put(raw, 36, input[0]);
        const before = Uint8Array.from(raw);
        let trace = '';
        drawDaisAdditional(raw, input[1] | 0, (type, frame) => {
            assert.equal(type, 169);
            trace = `draw:50:${input[3]}:${input[4]}:${input[5]}:${frame >>> 0}`;
        });
        assert.ok(trace === row[1] && row[1] === row[2], `행 ${count}: 입력 ${row[0]}`);
        assert.deepEqual(raw, before);
        count += 1;
    }
    assert.equal(count, ROWS);
});

test('DaisDraw: 다른 VT/TYPE 이나 손상된 프레임 수를 일반 다이스 효과로 처리하지 않는다', () => {
    const raw = new Uint8Array(50);
    put(raw, 0, DAIS_TABLE);
    raw[10] = 165;
    let draws = 0;
    const draw = () => { draws += 1; };
    assert.ok(isPatchDais(raw));
    assert.ok(throws(() => drawDaisAdditional(raw, 0, draw)));
    assert.ok(throws(() => drawDaisAdditional(raw, 180, undefined)));
    raw[10] = 164;
    assert.ok(!isPatchDais(raw));
    assert.ok(throws(() => drawDaisAdditional(raw, 180, draw)));
    assert.equal(draws, 0);
    assert.ok(throws(() => isPatchDais(new Uint8Array(3))));
    // 음수 프레임의 나머지는 부호를 유지하고 -0 을 만들지 않는다.
    raw[10] = 165;
    put(raw, 36, 0xffffff4c);
    let seen = null;
    drawDaisAdditional(raw, 180, (type, frame) => { seen = frame; });
    assert.ok(Object.is(seen, 0));
});
