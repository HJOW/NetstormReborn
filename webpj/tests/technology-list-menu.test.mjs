// 원본 기술 목록 367입력/전체 메뉴 100입력과 조회 시점/고정 저장 용량을 대조한다. 실제 제단·생산·희생·MP를 실행하지 않는다. (2026-11-14 WB04-E450)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import { TechnologyListMenu } from '../www/ui/technology-list-menu.js';

// unsigned DWORD로 기록된 입력 행을 복원한다.
const numbers = (text) => text.split(',').map(Number);
// 독립 원본 자료의 제목 줄과 빈 끝 줄을 제외한다.
const fixture = (name) => fs.readFileSync(new URL(`../../cpppj/tests/fixtures/${name}`, import.meta.url), 'utf8').trimEnd().split(/\r?\n/).slice(1).map((line) => line.split('\t'));

// cpp 검사와 같은 명시 TYPE/장치만 만들며 기대 결과는 fixture에서 직접 읽는다.
function scene(row, all = false) {
    const types = Array.from({ length: 80 }, (_, index) => ({ level: index % 4 - 1, theme: index % 6, group: index % 11 ? 0 : 10 }));
    const trace = []; const event = (name, ...values) => trace.push([name, ...values.map((value) => value >>> 0)].join(':'));
    const hooks = {
        typeCount: () => row[3] | 0, type: (number) => { assert.ok(number < types.length); return types[number]; }, priestType: () => 5, templeProduct: () => 9,
        currentTechnology: () => { event('T'); return 50; }, level: (target) => { assert.equal(target, 10); event('L', target); return (row[2] + 1) >>> 0; },
        owns: (parent, number) => { event('O', parent, number); return row[4] && number % 3 === 1 ? number : 0; },
        translate: (text) => { assert.equal(text, '%s (Level %d)'); event('X'); return text; },
        description: (number) => { event('D', number); if (row[6]) types[number].level = 7; return `T${number}`; },
        format: (pattern, description, level) => { assert.equal(pattern, '%s (Level %d)'); event('F', Number(description.slice(1)), level); return `${description} (Level ${level})`; },
        learnCommand: () => 0x020000d5, item: (label, command, type, enabled) => { assert.ok(label.startsWith('T')); event('I', command, type, enabled ? 1 : 0); },
        assertion: (line) => event('A', line),
    };
    if (all) Object.assign(hooks, {
        reference: (target) => { assert.equal(target, 10); event('C'); event('B', target); }, themeName: (theme) => { event('E', theme); return `E${theme}`; }, separator: () => event('S'),
        translate: (text) => { event(`X:${text}`); return text; },
        formatString: (pattern, value) => { event(`FS:${pattern}:${value}`); return pattern.replace('%s', value); },
        formatNumber: (pattern, value) => { event(`FN:${pattern}`, value); return pattern.replace('%d', String(value)); },
        item: (label, command, parameter, enabled) => event(`I:${label}`, command, parameter, enabled ? 1 : 0),
        surface: (target) => {
            assert.equal(target, 10); const data = new DataView(new ArrayBuffer(4)); data.setUint32(0, row[7], true); const x = Math.trunc(data.getFloat32(0, true) + 0.9998999834060669);
            data.setUint32(0, row[8], true); const y = Math.trunc(data.getFloat32(0, true) + 0.9998999834060669); return x < 0 || y < 0 || x > 255 || y > 255 ? 0 : row[9];
        },
        poolCapacity: () => 64, homeIsland: () => () => row[10], island: (surface) => { event('J', surface); if (row[12]) row[10] = 99; return row[11]; },
        doNothingCommand: () => 0x02000089, upgradeCommand: () => 0x020000d4, nuggetType: () => 154,
    });
    return { hooks, types, trace };
}

test('TechnologyListMenu_IndependentNative: 원문 367입력의 전체 반환/저장 개수/40DWORD/메뉴 경계 순서 일치', () => {
    const rows = fixture('technology-list-menu-x86.tsv'); assert.equal(rows.length, 367);
    // 고정 용량 초과와 설명 getter의 level 변경 입력도 모두 독립 자료로 비교한다.
    for (const [input, expectedTrace, expected] of rows) {
        const row = numbers(input); const r = scene(row); const menu = new TechnologyListMenu(r.hooks); const output = new Uint32Array(40).fill(0xa5a5a5a5); const stored = { value: 0 }; let result = 0;
        if (row[0] === 2) menu.appendTheme(10, row[1] | 0);
        else if (!row[0]) result = menu.countKnown(output.subarray(0, row[5]), stored, row[1] | 0, row[2] | 0);
        else result = menu.countCandidates(output.subarray(0, row[5]), stored, row[1] | 0, row[2] | 0);
        assert.equal(r.trace.join(';') || '-', expectedTrace, input); assert.deepEqual([result >>> 0, stored.value >>> 0, ...output], numbers(expected), input);
    }
});

test('TechnologyListMenu_AllIndependentNative: 원문 100입력의 전체 메뉴 효과/문구/활성 상태 일치', () => {
    const rows = fixture('technology-all-menu-x86.tsv'); assert.equal(rows.length, 100);
    // packed 대상의 상위 WORD는 전체 메뉴 진입에서만 제거한다.
    for (const [input, expected] of rows) { const r = scene(numbers(input), true); new TechnologyListMenu(r.hooks).appendAll(0x1234000a); assert.equal(r.trace.join(';'), expected, input); }
});

test('NULL 목록과 빈 TYPE는 stored를 바꾸지 않으며 후보 수집은 owns를 요구하지 않는다', () => {
    let reads = 0; const menu = new TechnologyListMenu({ currentTechnology: () => { reads += 1; return 50; }, typeCount: () => 0, level: () => 0 }); const stored = { value: 7 };
    assert.equal(menu.countCandidates(null, stored, 0, -1), 0); assert.equal(stored.value, 7); assert.equal(reads, 1); menu.appendTheme(10, 1); assert.equal(reads, 4);
    const r = scene([1, 5, 2, 80, 0, 40, 0]); delete r.hooks.owns; assert.ok(new TechnologyListMenu(r.hooks).countCandidates(new Uint32Array(1), { value: 0 }, 5, 2) > 1);
});

test('장치 누락은 도달한 위치에서 실패하고 선행 번역/설명/서식 효과를 보존한다', () => {
    const r = scene([2, 1, 2, 80, 1, 40, 0]); delete r.hooks.item;
    assert.throws(() => new TechnologyListMenu(r.hooks).appendTheme(10, 1), /실제 장치 미연결/);
    const trace = r.trace.join(';'); assert.match(trace, /;X;D:/); assert.match(trace, /;F:/); assert.doesNotMatch(trace, /;I:/);
    assert.throws(() => new TechnologyListMenu({}).countKnown(null, { value: 0 }, 0, 4), /실제 장치 미연결/);
});

test('stored는 초기화하지 않으며 음수는 실제 저장 시 거부하고 현재 TYPE 수는 반복 조건마다 읽는다', () => {
    let count = 3; const hooks = { currentTechnology: () => 7, typeCount: () => count, type: () => ({ group: 0, theme: 0, level: 0 }), priestType: () => 99, owns: () => { count = 1; return 1; } };
    const menu = new TechnologyListMenu(hooks); const output = new Uint32Array(4).fill(99); const stored = { value: 2 };
    assert.equal(menu.countKnown(output, stored, 0, 0), 1); assert.equal(stored.value, 3); assert.deepEqual([...output], [99, 99, 0, 99]);
    assert.throws(() => menu.countCandidates(output, { value: -1 }, 0, 0), RangeError);
    assert.equal(menu.countCandidates(null, { value: -1 }, 0, 0), 1);
});

test('전체 DWORD 대상·currentTechnology 선행 조회·알려진 기술 비활성화와 40칸 초과 순환을 구분한다', () => {
    const r = scene([2, 5, 2, 80, 1, 40, 0]); r.hooks.level = (target) => { assert.equal(target, 0xf123000a); return 3; };
    const rows = []; r.hooks.item = (label, command, type, enabled) => rows.push({ type, enabled });
    new TechnologyListMenu(r.hooks).appendTheme(0xf123000a, 5); assert.ok(rows.length > 40); assert.ok(rows.some((row) => !row.enabled)); assert.ok(rows.some((row) => row.enabled));
    assert.ok(rows.some((row, index) => rows.findIndex((other) => other.type === row.type) < index));
});
