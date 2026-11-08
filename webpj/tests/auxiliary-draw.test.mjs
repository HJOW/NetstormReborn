// 메타 배율/추상색 추가 Draw raw-auxiliary-draw.js(drawScaledMetadisplay·drawAbstractAdditional)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: auxiliary-draw-x86.tsv 1728행(Scaled 864·Abstract 864; 두 x87 정밀도 동일 결과) — 완성 50바이트 슬롯·기준점 비트·SHP 토큰 → 출력 요청 `추상,SHP,프레임,x,y,배율,배율`(unsigned), SHP 조회 `TYPE,토큰`, 슬롯 불변.
//    숨김 0x4000·분수 WORD·정수 배율·0 은 1배 분기와 고정 추상색 요청을 포함한다.
//  - 계약: 숨김 조기 반환은 프레임/SHP 장치 없이도 성립한다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E84: 메타 배율/추상색 추가 Draw 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';
import { drawScaledMetadisplay, drawAbstractAdditional } from '../www/core/raw-auxiliary-draw.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E84)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'auxiliary-draw-x86.tsv'));

// 독립 입력 수(전체/배율/추상색)이다. (2026-11-08 WB04-E84)
const ROWS = 1728;
const SCALED_ROWS = 864;
const ABSTRACT_ROWS = 864;

// 예외가 나는지 본다. (2026-11-08 WB04-E84)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 완성 50바이트 객체를 메모리 순서로 읽는다. (2026-11-08 WB04-E84)
const bytes = (text) => Uint8Array.from(Buffer.from(text, 'hex'));

// 원문 DWORD 인수와 같은 unsigned 표기이다. (2026-11-08 WB04-E84)
const numbers = (...values) => values.map((value) => Number(value) >>> 0).join(',');

test('AuxiliaryDraw: 원본 10.78 1728입력의 숨김/분수/고정 배율과 고정 치환표 요청·SHP 조회가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    let count = 0;
    let scaled = 0;
    let abstract = 0;
    // 각 사례를 독립 raw/호출 관측 저장소로 실행한다.
    for (const row of rows) {
        assert.equal(row.length, 8);
        const raw = bytes(row[2]);
        const original = Uint8Array.from(raw);
        const draws = [];
        const reads = [];
        const hooks = {
            shape: (type) => { reads.push(numbers(type, row[5])); return Number(row[5]); },
            draw: (value) => { draws.push(numbers(value.abstract ? 1 : 0, value.shape, value.frame, value.x, value.y, value.scale, value.scale)); }
        };
        const x = Number(row[3]) | 0;
        const y = Number(row[4]) | 0;
        if (row[1] === 'Scaled') { drawScaledMetadisplay(raw, x, y, hooks); scaled += 1; } else { assert.equal(row[1], 'Abstract'); drawAbstractAdditional(raw, x, y, hooks); abstract += 1; }
        const label = `${row[1]}/${row[0]}`;
        assert.equal(draws.length === 0 ? '-' : draws.join(';'), row[6], label);
        assert.equal(reads.length === 0 ? '-' : reads.join(';'), row[7], label);
        assert.deepEqual(raw, original, label);
        count += 1;
    }
    assert.ok(count === ROWS && scaled === SCALED_ROWS && abstract === ABSTRACT_ROWS);
});

test('AuxiliaryDraw: 숨김 조기 반환은 프레임/SHP 장치 없이 성립하고 도달한 출력 분기의 누락 장치만 거부한다', () => {
    const hidden = new Uint8Array(30);
    hidden[27] = 0x40;
    drawScaledMetadisplay(hidden, 0, 0, {});
    const visible = new Uint8Array(50);
    assert.ok(throws(() => drawScaledMetadisplay(visible, 0, 0, {})));
    assert.ok(throws(() => drawAbstractAdditional(visible, 0, 0, { shape: () => 1 })));
    assert.ok(throws(() => drawScaledMetadisplay(new Uint8Array(20), 0, 0, { shape: () => 1, draw: () => {} })));
    // 분수 0·정수 0 은 1배(0x10000), 정수 배율 3 과 분수 0x8000 은 0x38000 이다.
    const seen = [];
    const hooks = { shape: () => 7, draw: (value) => seen.push(value.scale) };
    drawScaledMetadisplay(visible, 1, 2, hooks);
    visible[26] = 3;
    visible[8] = 0x00;
    visible[9] = 0x80;
    drawScaledMetadisplay(visible, 1, 2, hooks);
    assert.deepEqual(seen, [0x10000, 0x38000]);
});
