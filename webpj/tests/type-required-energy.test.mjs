// TYPE 요구 에너지(mana) 후처리와 AI 연결 getter 를 원본 10.78 관측으로 대조한다(WB04-E26, cpppj TypeRequiredEnergyTests 이식).
//  - type-required-energy-x86.tsv 2352행: 음수/98 level·문자표 NUL·제외/group10·명시 문자열의 실제 PE 결과.
//  - 경계: 순서 있는 mana 대입·AST 비문자열·9바이트 초과·비ASCII·표 밖 theme 거부, AI getter 의 188/116 계약과 현재 TYPE 재조회.
// 보호 자산 116개 정의로 188TYPE 전체를 읽는 `Protected188Bindings` 는 자산 적재 단위에서 대조한다. (2026-11-08 WB04-E26: TYPE 요구 에너지 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';
import { bindAiTypeRequiredEnergy, typeRequiredEnergy } from '../www/core/raw-type-required-energy.js';

// 원본 관측 행이다. 파일이 없으면 검사를 건너뛴다. (2026-11-08 WB04-E26)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'type-required-energy-x86.tsv'));
// 원본 바이트 16진을 JS 문자열(글자 하나당 바이트 하나)로 되돌린다. (2026-11-08 WB04-E26)
const bytesOf = (hex) => Buffer.from(hex, 'hex').toString('latin1');

// AST 문자열 입력은 명시 대체 경계이며 후처리 기대값은 실제 기계어에서만 읽는다.
test('TypeRequiredEnergy: 원본10.78 2352행(level·문자표 NUL·제외/group10·명시 문자열)', { skip: !rows && '원본 fixture 없음' }, () => {
    let count = 0;
    for (const row of rows) {
        if (row[0] === 'group') continue;
        assert.equal(row.length, 6);
        const type = { group: Number(row[0]) | 0, flags2: Number(row[1]), level: Number(row[2]) | 0, theme: Number(row[3]) | 0 };
        count += 1;
        assert.equal(typeRequiredEnergy(type, { properties: [{ name: 'mana', value: row[4] }] }), bytesOf(row[5]), `행 ${count} ${row.slice(0, 5).join('/')}`);
    }
    assert.equal(count, 2352);
});

// 같은 이름 속성은 순서대로 대입하고 다른 이름은 건너뛴다.
test('TypeRequiredEnergy: 순서 있는 대입과 경계 거부', () => {
    const type = { group: 0, flags2: 0, level: 1, theme: 3 };
    const definition = { properties: [{ name: 'mana', value: 'S' }, { name: 'other', value: 1 }, { name: 'MANA', value: 'WRTS' }] };
    assert.equal(typeRequiredEnergy(type, definition), 'wrts');
    // 문자열이 아닌 mana 는 뒤의 정상 대입이 있어도 그 속성에서 거부한다.
    assert.throws(() => typeRequiredEnergy(type, { properties: [{ name: 'mana', value: 1 }, { name: 'mana', value: 's' }] }), TypeError);
    // 9바이트를 넘으면 원본 assert 후 손상 경계이다.
    assert.throws(() => typeRequiredEnergy(type, { properties: [{ name: 'mana', value: '0123456789' }] }), RangeError);
    // 비ASCII 는 원본 CRT tolower 경계를 복원하지 않았다.
    assert.throws(() => typeRequiredEnergy(type, { properties: [{ name: 'mana', value: 'é' }] }), TypeError);
    // 문자표 밖 theme 는 거부하고 표의 NUL 칸(theme 11)은 빈 문자열이 된다.
    assert.throws(() => typeRequiredEnergy({ ...type, theme: 12 }, null), RangeError);
    assert.throws(() => typeRequiredEnergy({ ...type, theme: -1 }, null), RangeError);
    assert.equal(typeRequiredEnergy({ group: 0, flags2: 0, level: 0, theme: 11 }, null), '');
    // 제외 플래그와 group 10 은 기본 보충을 하지 않는다.
    assert.equal(typeRequiredEnergy({ group: 10, flags2: 0, level: 1, theme: 3 }, null), '');
    assert.equal(typeRequiredEnergy({ group: 0, flags2: 0x200000, level: 2, theme: 0 }, null), '');
    // level 2 는 level>=0 의 글자에 이어 글자와 's' 를 한 번 더 붙인다(원문 순서 그대로).
    assert.equal(typeRequiredEnergy({ group: 0, flags2: 0, level: 2, theme: 4 }, null), 'wws');
});

// 내부/빈 TYPE 은 정의 없이 zero 초기 입력에서, 자산 구간은 정의를 읽고, getter 는 현재 TYPE 변경을 반영한다.
test('AI 요구 에너지 getter: 188/116 계약과 현재 TYPE 재조회', () => {
    const types = Array.from({ length: 188 }, () => ({ group: 0, flags2: 0, level: -2, theme: 0 }));
    const definitions = Array.from({ length: 116 }, (_, index) => ({ properties: [{ name: 'mana', value: index === 0 ? 'WR' : '' }] }));
    const getter = bindAiTypeRequiredEnergy(definitions, types);
    // 자산 70 은 정의의 명시 문자열, 71 은 빈 문자열이라 level -2 에서 보충도 없다.
    assert.equal(getter(70), 'wr');
    assert.equal(getter(71), '');
    // 내부 TYPE 69 는 정의 없이 level 에서 보충한다. getter 생성 뒤의 변경을 읽는다.
    types[69].level = 2;
    types[69].theme = 0;
    assert.equal(getter(69), 'wws');
    assert.throws(() => getter(188), RangeError);
    assert.throws(() => bindAiTypeRequiredEnergy(definitions, types.slice(0, 187)), TypeError);
    assert.throws(() => bindAiTypeRequiredEnergy(definitions.slice(0, 115), types), TypeError);
});
