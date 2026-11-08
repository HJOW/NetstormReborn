// 윈드 원주 좌표 raw-wind-circle.js(windCirclePoint)와 소환 주문 raw-wind-spell.js(RawWindSpell)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측 1: wind-circle-point-x86.tsv 232행 — (원주 개수, 번호, 중심 x, y) 입력의 원주 점 두 float 비트가 같다(두 번째 열 기본 53비트 정밀도).
//  - 원본 관측 2: wind-spell-x86.tsv 300행 — 변형(세 주문)·서버·기존 프로세스 존재·spawns·변화로 반환(항상 거짓), 조회/확보/부착의 호출 순서와 인수, 부착한 80바이트 생산기 이미지가 같다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E121: 윈드 원주 점·소환 주문 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, bitsToFloat, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { windCirclePoint } from '../www/core/raw-wind-circle.js';
import { WindAviaryProcessFields } from '../www/core/raw-wind-aviary-state.js';
import { RawWindSpell } from '../www/core/raw-wind-spell.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E121)
const pointRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'wind-circle-point-x86.tsv')) ?? [];
const spellRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'wind-spell-x86.tsv')) ?? [];
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 예외가 나는지 본다. (2026-11-08 WB04-E121)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

test('WindCircle: 원본 기본 53비트 원주 점의 두 float 저장이 232행과 같다', { skip: pointRows.length === 0 && '원본 fixture 없음' }, () => {
    let count = 0;
    // 원본 CRT 기본 53비트의 두 float 저장을 비교한다.
    for (const row of pointRows) {
        const a = row[0].split(',');
        const point = windCirclePoint(bitsToFloat(Number(a[2])), bitsToFloat(Number(a[3])), Number(a[0]) | 0, Number(a[1]) | 0);
        assert.equal(`${floatToBits(point[0])},${floatToBits(point[1])}`, row[1], `Wind 원주 입력 ${row[0]}`);
        count += 1;
    }
    assert.equal(count, 232);
    assert.ok(throws(() => windCirclePoint(0, 0, 0, 0)));
});

test('WindSpell: 세 소환 함수의 생성 필드와 현재 인수 순서가 원본 300행과 같다', { skip: spellRows.length === 0 && '원본 fixture 없음' }, () => {
    let count = 0;
    // 결과를 계산하지 않고 조회/힙/부착의 명시 변화만 공급한다.
    for (const row of spellRows) {
        const words = row[0].split(',');
        const variant = Number(words[0]);
        const mutation = Number(words[4]);
        const env = { server: Number(words[1]) !== 0, now: 100 };
        const counts = new Array(188).fill(Number(words[3]) | 0);
        const state = { processType: 59, spells: [171, 172, 173], counts };
        const pool = new SidPool(24000, { server: false });
        for (let sid = 5; sid <= 50; sid += 1) assert.equal(pool.allocate(2), sid);
        const raw = pool.physical(50);
        raw.fill(0);
        put(raw, 0, [0x5025d8, 0x502e68, 0x502f30][variant]);
        put(raw, 12, 60, 2);
        const effects = [];
        const images = [];
        const hooks = {
            exists: (parent, type) => {
                effects.push(`F:${parent}:${type}:0:0`);
                if (mutation === 1) put(raw, 12, 61, 2);
                return Number(words[2]) !== 0;
            },
            allocate: () => {
                effects.push('N:80');
                const fields = new WindAviaryProcessFields();
                fields.bytes.fill(0xa5);
                if (mutation === 2) {
                    env.server = false;
                    state.processType = 60;
                    env.now = 101.25;
                }
                return fields;
            },
            attach: (fields, parent, type, flags) => {
                effects.push(`A:${type}:${parent}:0:${flags}`);
                images.push(hex(fields.bytes));
                if (mutation === 3) counts[171 + variant] = 1;
                if (mutation === 4) {
                    put(raw, 12, 62, 2);
                    state.processType = 60;
                }
            }
        };
        const logic = new RawWindSpell(pool, env, state, hooks);
        const result = logic.mayTarget(50);
        assert.deepEqual([result ? 1 : 0, effects.length ? effects.join(';') : '-', images.length ? images.join(',') : '-'],
            [Number(row[1]), row[2], row[3]], `Wind 소환 입력 ${row[0]}`);
        count += 1;
    }
    assert.equal(count, 300);
});

test('WindSpell: 미연결 하위·판본/표 크기·모르는 가상 표를 거부한다', () => {
    const pool = new SidPool(24000, { server: false });
    for (let sid = 5; sid <= 50; sid += 1) assert.equal(pool.allocate(2), sid);
    const state = { processType: 59, spells: [171, 172, 173], counts: new Array(188).fill(1) };
    assert.ok(throws(() => new RawWindSpell({ edition: 'cd1072' }, { server: true, now: 0 }, state, {})));
    assert.ok(throws(() => new RawWindSpell(pool, { server: true, now: 0 }, { ...state, counts: [] }, {})));
    const raw = pool.physical(50);
    raw.fill(0);
    const logic = new RawWindSpell(pool, { server: true, now: 0 }, state, {});
    // 모르는 가상 표는 항상 거부한다.
    assert.ok(throws(() => logic.mayTarget(50)));
    put(raw, 0, 0x5025d8);
    put(raw, 12, 60, 2);
    // 서버이면 조회 훅이 필요하다(없으면 거부).
    assert.ok(throws(() => logic.mayTarget(50)));
    // 서버가 아니면 훅을 요구하지 않고 거짓이다.
    assert.equal(new RawWindSpell(pool, { server: false, now: 0 }, state, {}).mayTarget(50), false);
});
