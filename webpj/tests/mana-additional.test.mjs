// 마나 추가 Draw(00473710)의 출력·현재 SHP 읽기·출력 뒤 raw/TYPE 변경 관측을 원본 x86 fixture 로 재생한다(cpppj ManaAdditionalTests).
//  - mana-additional-x86.tsv 756행: raw·좌표·TYPE→SHP 표·그림자 출력 뒤 변경(없음/프레임·TYPE/프레임·TYPE·SHP·WORD)과 출력열·SHP 읽기열·최종 raw·최종 SHP 표.
//  - 각 행은 입력과 변경 규칙만 공급하고 모듈이 계산한다. 기대 열은 비교에만 쓴다. 거부 경로는 웹 계약 검사다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E203: 신전·마나 추가 그림 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put } from './raw-support.mjs';
import { drawManaAdditional } from '../www/core/raw-mana-draw.js';

// 원본 x86 fixture 이다(머리 주석 행 제외). (2026-11-09 WB04-E203)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'mana-additional-x86.tsv')) ?? [];

// 예외가 지정 종류로 나는지 본다. (2026-11-09 WB04-E203)
function throws(action, kind = Error) {
    try { action(); } catch (error) { return error instanceof kind; }
    return false;
}

// 16진 문자열의 바이트 배열이다. (2026-11-09 WB04-E203)
const bytesOf = (text) => Uint8Array.from(Buffer.from(text, 'hex'));

// 원문 관측과 같은 부호 없는 DWORD 인수열이다(불 값은 1/0). (2026-11-09 WB04-E203)
const numbers = (...values) => values.map((value) => String(Number(value) >>> 0)).join(',');

// "번호=토큰;…" 표를 읽는다. (2026-11-09 WB04-E203)
function shapesOf(text) {
    const result = new Map();
    // 실제 번호/토큰을 재계산하지 않는다.
    for (const field of text.split(';')) {
        const [number, shape] = field.split('=');
        result.set(Number(number), Number(shape));
    }
    return result;
}

// 현재 SHP 표를 번호 순서로 직렬화한다(cpppj std::map 순서). (2026-11-09 WB04-E203)
const encoded = (shapes) => [...shapes.entries()].sort((a, b) => a[0] - b[0]).map(([number, shape]) => `${number}=${shape}`).join(';');

test('ManaAdditional: 원본 756행의 그림자/본체 출력·현재 SHP 읽기·변경 반영이 같다', { skip: rows.length === 0 && 'mana-additional-x86.tsv 없음' }, () => {
    let count = 0;
    // 각 사례는 독립 raw/TYPE/출력 저장소로 실행한다.
    for (const row of rows) {
        assert.equal(row.length, 13);
        const raw = bytesOf(row[1]);
        const selected = raw[10];
        const shapes = shapesOf(row[4]);
        const draws = [];
        const reads = [];
        const hooks = {
            shape(type) {
                assert.ok(shapes.has(type), `SHP 표에 없는 TYPE ${type}`);
                const shape = shapes.get(type);
                reads.push(numbers(type, shape));
                return shape;
            },
            draw(request) {
                draws.push(numbers(request.shadow ? 1 : 0, request.shape, request.frame, request.x, request.y));
                const mutation = Number(row[5]);
                if (request.shadow && mutation) {
                    put(raw, 36, Number(row[7]));
                    raw[10] = Number(row[6]);
                    if (mutation === 2) {
                        shapes.set(selected, Number(row[8]));
                        raw[12] = 0;
                        raw[13] = 0;
                    }
                }
            },
        };
        drawManaAdditional(raw, Number(row[2]) | 0, Number(row[3]) | 0, hooks);
        assert.deepEqual([draws.join(';'), reads.join(';'), Buffer.from(raw).toString('hex'), encoded(shapes)], [row[9], row[10], row[11], row[12]], `행 ${row[0]}`);
        count += 1;
    }
    assert.equal(count, 756);
});

test('ManaAdditional: 짧은 raw·SHP/출력 소비자 누락을 거부한다(웹 계약 검사)', () => {
    const raw = new Uint8Array(50);
    raw[10] = 103;
    const hooks = { shape: () => 1, draw() {} };
    assert.ok(throws(() => drawManaAdditional(new Uint8Array(39), 0, 0, hooks), TypeError));
    assert.ok(throws(() => drawManaAdditional(raw, 0, 0, { draw() {} }), TypeError));
    assert.ok(throws(() => drawManaAdditional(raw, 0, 0, { shape: () => 1 }), TypeError));
    // WORD 상태가 0 이면 본체 하나만, 아니면 그림자(+10, 32비트 감김) 뒤 본체를 낸다.
    const calls = [];
    drawManaAdditional(raw, 0x7ffffffb, -3, { shape: () => 9, draw: (request) => calls.push(request) });
    assert.deepEqual(calls.map((request) => request.shadow), [false]);
    calls.length = 0;
    raw[13] = 1;
    drawManaAdditional(raw, 0x7ffffffb, -3, { shape: () => 9, draw: (request) => calls.push(request) });
    assert.deepEqual(calls.map((request) => [request.shadow, request.x, request.y]), [[true, -0x7ffffffb, 7], [false, 0x7ffffffb, -3]]);
});
