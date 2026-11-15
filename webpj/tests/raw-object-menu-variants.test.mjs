// 파생 객체 메뉴 raw-object-menu-variants.js(RawObjectMenuVariants.open·명령 표)를 원본 10.78 관측 1,024입력과 cpppj 계약으로 대조한다(WB04-E542).
//  - object-menu-variants-x86.tsv 1,024행: 여덟 +a8 본체(0041c210·00422000·00462390·00462550·00485600·0048eca0·004dcca0·00420b30)의 입력 11개(소유자·로컬·타입·모드·HP·최대 HP·예약·개수·Storm Power·AI·변경)와
//    콜백 변경 조합에서 전체 외부 효과·인자·순서.
//  - 계약: 명령 ID 표(9쌍)·미등록 본체 거부·판본 거부·선택 경계 누락 거부.
// 실제 게임·브라우저 실행이 아니다. (2026-11-15 WB04-E542)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { RawObjectMenuVariants, isObjectMenuVariantCommand, objectMenuVariantCommandValue } from '../www/core/raw-object-menu-variants.js';

// 원본 관측 fixture 이다. 없으면 해당 검사를 건너뛴다. (2026-11-15 WB04-E542)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'object-menu-variants-x86.tsv'));
// C 문자열의 원본 바이트를 비교 가능한 16진으로 남긴다. (2026-11-15 WB04-E542)
const encoded = (text) => Buffer.from(text, 'latin1').toString('hex');
// 비정렬 값을 낮은 바이트부터 쓴다. (2026-11-15 WB04-E542)
function put(raw, offset, value, width = 4) {
    for (let index = 0; index < width; index += 1) raw[offset + index] = (value >>> (index * 8)) & 0xff;
}

// 기대값은 fixture 에서만 읽고 현재 슬롯/모드와 외부 반환을 명시 입력으로 공급하는 독립 장면이다. (2026-11-15 WB04-E542)
class VariantScene {
    // 입력을 해석하고 별도 메뉴 기대 목록 없이 실제 슬롯을 만든다(5..50번 슬롯 확보).
    constructor(values) {
        this.pool = new SidPool(24000, { server: true });
        this.trace = '';
        // 각 원문 필드의 부호를 유지한다.
        this.input = values.split(',').map((text) => Number(text));
        assert.equal(this.input.length, 11);
        for (let number = 5; number <= 50; number += 1) assert.equal(this.pool.allocate(2), number);
        const raw = this.raw();
        put(raw, 10, this.input[2] >>> 0, 1);
        put(raw, 34, this.input[0] >>> 0, 1);
        put(raw, 26, this.input[4] >>> 0);
        put(raw, 12, this.input[8] >>> 0, 2);
    }

    // 독립 검사의 현재 슬롯이다(제품의 생존 판정을 합성하지 않는다).
    raw() { return this.pool.allocated(50); }

    // 외부 효과 하나를 전체 순서에 추가한다.
    add(value) { this.trace = this.trace ? `${this.trace};${value}` : value; }

    // native 관측기와 같은 명시 callback 변경만 적용한다.
    change(where) {
        const input = this.input;
        if (input[10] === 1 && where === 'translate') this.raw()[34] = 2;
        if (input[10] === 2 && where === 'count') this.raw()[10] = 122;
        if (input[10] === 3 && where === 'ai') this.raw()[34] = 2;
        if (input[10] === 4 && where === 'process') input[3] |= 1;
        if (input[10] === 5 && where === 'translate') put(this.raw(), 26, 0);
    }

    // 실제 메뉴 효과만 기록하며 반환은 원문에 공급한 독립 입력이다.
    hooks() {
        const input = this.input;
        return {
            global: (address) => {
                const code = objectMenuVariantCommandValue(address);
                if (code !== null) return code;
                switch (address) {
                case 0x594fa4: return (input[3] & 4) >>> 0;
                case 0x5c85a4: return (input[3] & 1) >>> 0;
                case 0x55a480: return (input[3] & 2) >>> 0;
                case 0x540c70: return input[1] >>> 0;
                default: throw new Error('명시 파생 메뉴 다른 전역');
                }
            },
            localOwner: () => { this.add('L'); return input[1] >>> 0; },
            maximumHp: (sid) => { assert.equal(sid, 50); this.add('H,50'); return input[5] | 0; },
            eventZero: (sid) => { assert.equal(sid, 50); this.add('P,50,0'); this.change('process'); return input[6] !== 0; },
            hasBrain: (sid) => { assert.equal(sid, 50); return this.raw()[34] !== 0 && input[9] !== 0; },
            aiMenu: (owner) => { this.add(`AI,${owner}`); this.change('ai'); },
            translate: (text) => { this.add(`T,${encoded(text)}`); this.change('translate'); return text; },
            format: (pattern, value) => {
                this.add(`F,${encoded(pattern)},${value >>> 0}`);
                assert.ok(pattern.includes('%d'), '고정 %d 파생 메뉴 서식 없음');
                return pattern.replace('%d', String(value));
            },
            helpWord: () => 'About',
            count: (sid) => { assert.equal(sid, 50); this.add('C,50'); this.change('count'); return input[7] >>> 0; },
            item: (label, command, parameter, include, enabled) => this.add(`I,${encoded(label)},${command >>> 0},${parameter >>> 0},${include >>> 0},${enabled >>> 0},0`),
            header: (text) => this.add(`A,${encoded(text)}`),
            separator: () => this.add('X'),
            object: (sid) => { assert.equal(sid, 50); this.add('B,50'); },
            damageable: (sid) => { assert.equal(sid, 50); this.add('D,50'); },
            salvage: (sid, type, count) => { assert.equal(sid, 50); this.add(`S,50,${type},${count}`); },
        };
    }
}

test('파생 객체 메뉴: 원본 10.78 1,024입력의 여덟 +a8 본체 전체 외부 효과·인자·순서가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    assert.equal(rows.length, 1024);
    let count = 0;
    // 각 행은 새 현재 상태에서 같은 공개 본체를 실행한다.
    for (const row of rows) {
        assert.equal(row.length, 3);
        const scene = new VariantScene(row[1]);
        new RawObjectMenuVariants(scene.pool, scene.hooks()).open(50, parseInt(row[0], 16));
        assert.equal(scene.trace, row[2], `${row[0]} ${row[1]}`);
        count += 1;
    }
    assert.equal(count, 1024);
});

test('파생 객체 메뉴 계약: 명령 ID 표 9쌍과 허용 판정·미등록 본체/미연결 경계/풀 판본 거부', () => {
    const table = [[0x542ad4, 0x030000f8], [0x542b5c, 0x02000125], [0x542b60, 0x02000126], [0x542b64, 0x02000127], [0x542a2c, 0x030000ce],
        [0x5427ac, 0x0200002d], [0x5428c8, 0x03000074], [0x542754, 0x02000017], [0x542974, 0x020000a0]];
    for (const [address, code] of table) {
        assert.equal(objectMenuVariantCommandValue(address), code, address.toString(16));
        assert.equal(isObjectMenuVariantCommand(code), true, code.toString(16));
    }
    assert.equal(objectMenuVariantCommandValue(0x542ad5), null);
    assert.equal(objectMenuVariantCommandValue(0), null);
    assert.equal(isObjectMenuVariantCommand(0x02000000), false);
    assert.equal(isObjectMenuVariantCommand(0x542ad4), false);
    const scene = new VariantScene('1,1,75,0,51,100,0,7,65535,0,0');
    const body = new RawObjectMenuVariants(scene.pool, scene.hooks());
    // 현재 +a8 에 없는 본체는 기본 메뉴로 바꾸지 않고 거부한다.
    assert.throws(() => body.open(50, 0x123456), /파생\+a8 미연결: 1193046/);
    assert.throws(() => body.open(0, 0x41c210), RangeError);
    // 선택한 분기의 경계(damageable)가 없으면 성공으로 대체하지 않는다.
    const missing = new VariantScene('1,1,75,0,51,100,0,7,65535,0,0');
    const hooks = missing.hooks();
    delete hooks.damageable;
    assert.throws(() => new RawObjectMenuVariants(missing.pool, hooks).open(50, 0x41c210), /미연결/);
});
