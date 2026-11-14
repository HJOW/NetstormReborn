// 생산 내용물 정보 메뉴(inventory-about-menu: 설명·비용·부족 머리·회수·Help·부착·음향)를 기준선 소스 계약과 손 계산 값으로 검증한다.
// 실제 SidPool 위에서 내용물 슬롯(+10 종류/+18 종류 WORD/+20 플래그 WORD)을 쓰고, 나머지 메뉴/번역/비용은 기록 훅이다. cpppj에도 직접 시험이 없어 원본 직접 관측 수치가 아니다. 실제 게임·브라우저 실행이 아니다. (2026-11-14 WB04-E448)
import test from 'node:test';
import assert from 'node:assert/strict';
import { SidPool } from '../www/core/sid-pool.js';
import { InventoryAboutMenu, InventoryAboutState } from '../www/ui/inventory-about-menu.js';

// 기록 틀이다. 내용물 슬롯 5: +10 = 종류 바이트, +18 = 종류 WORD, +20 = 플래그 WORD. 비용은 표에서(기본 100), 구매 가능/편집/Help 단어는 옵션이다.
function rig({ type = 50, kind = 0x1234, flags = 0x20, costs = {}, affordable = true, editor = false, help = 'Help', content = 5 } = {}) {
    const log = []; const pool = new SidPool(32768, { server: false }); const slot = pool.physical(content & 0xffff);
    slot[10] = type; slot[18] = kind & 0xff; slot[19] = kind >> 8; slot[20] = flags & 0xff; slot[21] = flags >> 8;
    const state = new InventoryAboutState();
    const hooks = {
        begin: () => { log.push(['begin']); return 900; }, root: () => { log.push(['root']); return 901; }, templateOwner: () => { log.push(['templateOwner']); return 902; }, editor: () => { log.push(['editor']); return editor; },
        cost: (value) => { log.push(['cost', value]); return costs[value] ?? 100; }, affordable: (sid) => { log.push(['affordable', sid]); return affordable; }, helpWord: () => { log.push(['helpWord']); return help; },
        idle: (menu, enable) => log.push(['idle', menu, enable]), reference: (menu, object) => log.push(['reference', menu, object]), describe: (...args) => log.push(['describe', ...args]),
        translate: (key) => { log.push(['translate', key]); return `<${key}>`; }, print: (pattern, value) => { log.push(['print', pattern, value]); return `${pattern}|${value}`; },
        header: (value) => log.push(['header', value]), sound: (name) => log.push(['sound', name]), entry: (...args) => log.push(['entry', ...args]), attach: (point, root) => log.push(['attach', point.x, point.y, root]),
    };
    return { menu: new InventoryAboutMenu(pool, state, hooks), pool, state, hooks, log, item: { content, object: 0x77 } };
}

test('InventoryAboutState 기본값: 회수 제외 122·제단 164·다이스 165·너겟 154·회수/Help 명령, 상자를 연결하면 현재 값을 읽는다', () => {
    const state = new InventoryAboutState(); assert.deepEqual([state.suppressedClass, state.none, state.dais, state.fullValue, state.salvageCommand, state.helpCommand], [122, 164, 165, 154, 0x0200005d, 0x02000017]);
    assert.deepEqual([state.suppressedClassNow(), state.altarType(), state.daisType(), state.nuggetType()], [122, 164, 165, 154]);
    state.sharedSuppressedClass = { value: 1 }; state.sharedAltarType = { value: 2 }; state.sharedDaisType = { value: 3 }; state.sharedNuggetType = { value: 4 }; assert.deepEqual([state.suppressedClassNow(), state.altarType(), state.daisType(), state.nuggetType()], [1, 2, 3, 4]);
    state.sharedAltarType.value = 9; assert.equal(state.altarType(), 9);
});

test('생성자: 10.78 풀과 모든 필수 훅이 있어야 하고 없으면 TypeError 이다', () => {
    const r = rig(); for (const name of Object.keys(r.hooks)) { const hooks = { ...r.hooks }; delete hooks[name]; assert.throws(() => new InventoryAboutMenu(r.pool, r.state, hooks), { name: 'TypeError', message: /실제 경계 연결/ }, name); }
    assert.throws(() => new InventoryAboutMenu({ edition: 'cd1072' }, r.state, r.hooks), TypeError); assert.throws(() => new InventoryAboutMenu(r.pool, r.state, undefined), TypeError); assert.equal(Object.keys(r.hooks).length, 16);
});

test('open 순서: 새 메뉴·idle·Form 참조·설명·비용 머리·(구매 불가 머리)·회수·Help·부착·음향이 원본 순서이고 1 을 돌려준다', () => {
    const r = rig({ affordable: false }); assert.equal(r.menu.open(r.item, { x: 12, y: 34 }), 1);
    assert.deepEqual(r.log, [['begin'], ['idle', 900, true], ['reference', 900, 5], ['templateOwner'], ['describe', 0x1234, 0, 902, 0xffffffff], ['translate', 'Cost: ~i%d~[IfortGump.3]~.\n'], ['cost', 0x1234],
        ['print', '<Cost: ~i%d~[IfortGump.3]~.\n>', 100], ['header', '<Cost: ~i%d~[IfortGump.3]~.\n>|100'], ['affordable', 5], ['translate', '~r~ENeed more Storm Power to Build!\n'], ['header', '<~r~ENeed more Storm Power to Build!\n>'],
        ['editor'], ['translate', 'Salvage for %d~[IfortGump.3]'], ['cost', 0x1234], ['print', '<Salvage for %d~[IfortGump.3]>', 25], ['entry', '<Salvage for %d~[IfortGump.3]>|25', 0x0200005d, 0x77, true, true, false],
        ['helpWord'], ['entry', 'Help', 0x02000017, 0x1234, true, true, false], ['root'], ['attach', 12, 34, 901], ['sound', 'opengump.wav']]);
});

test('open 회수 관문: 편집 중이거나 0x20 비트가 없거나 종류 바이트가 회수 제외 클래스이면 회수 항목이 없고(편집은 플래그/종류를 보기 전에 판정), 구매 가능이면 부족 머리도 없다', () => {
    const names = (r) => r.log.map((entry) => entry[0]); const has = (r) => r.log.some((entry) => entry[0] === 'entry' && entry[2] === 0x0200005d);
    const editing = rig({ editor: true }); editing.menu.open(editing.item, { x: 0, y: 0 }); assert.equal(has(editing), false); assert.equal(names(editing).includes('affordable'), true); assert.equal(names(editing).filter((name) => name === 'header').length, 1);
    const noBit = rig({ flags: 0x10 }); noBit.menu.open(noBit.item, { x: 0, y: 0 }); assert.equal(has(noBit), false); const lowBit = rig({ flags: 0x01 }); lowBit.menu.open(lowBit.item, { x: 0, y: 0 }); assert.equal(has(lowBit), false); const highBit = rig({ flags: 0x2120 }); highBit.menu.open(highBit.item, { x: 0, y: 0 }); assert.equal(has(highBit), true);
    const suppressed = rig({ type: 122 }); suppressed.menu.open(suppressed.item, { x: 0, y: 0 }); assert.equal(has(suppressed), false); const near = rig({ type: 121 }); near.menu.open(near.item, { x: 0, y: 0 }); assert.equal(has(near), true); const other = rig({ type: 123 }); other.menu.open(other.item, { x: 0, y: 0 }); assert.equal(has(other), true);
    // 회수 제외 클래스는 편집 관문 뒤의 현재 값을 읽는다.
    const live = rig({ type: 77 }); live.state.sharedSuppressedClass = { value: 0 }; live.hooks.editor = () => { live.state.sharedSuppressedClass.value = 77; return false; }; const menu = new InventoryAboutMenu(live.pool, live.state, live.hooks); menu.open(live.item, { x: 0, y: 0 }); assert.equal(live.log.some((entry) => entry[0] === 'entry' && entry[2] === 0x0200005d), false);
});

test('회수 비용: 비용을 float32 로 저장하고 제단/다이스는 0, 너겟은 전액, 그 밖은 1/4 을 0 방향으로 절삭한다(현재 상자 값을 읽는다)', () => {
    const refund = (kind, cost, mutate) => { const r = rig({ kind, costs: { [kind]: cost } }); mutate?.(r.state); r.menu.open(r.item, { x: 0, y: 0 }); return r.log.find((entry) => entry[0] === 'print' && entry[1].includes('Salvage'))[2]; };
    assert.equal(refund(0x1234, 100), 25); assert.equal(refund(0x1234, 7), 1); assert.equal(refund(0x1234, 3), 0); assert.equal(refund(0x1234, -7), -1); assert.equal(refund(0x1234, 16777217), 4194304); assert.equal(refund(0x1234, 0x7fffffff), 536870912);
    assert.equal(refund(154, 100), 100); assert.equal(refund(154, 16777217), 16777216); assert.equal(refund(164, 100), 0); assert.equal(refund(165, 100), 0); assert.equal(refund(166, 100), 25);
    // 상자를 연결하면 그 현재 값이 제단/다이스/너겟 번호가 된다.
    assert.equal(refund(0x1234, 100, (state) => { state.sharedAltarType = { value: 0x1234 }; }), 0); assert.equal(refund(0x1234, 100, (state) => { state.sharedDaisType = { value: 0x1234 }; }), 0); assert.equal(refund(0x1234, 100, (state) => { state.sharedNuggetType = { value: 0x1234 }; }), 100);
    assert.equal(refund(154, 100, (state) => { state.sharedNuggetType = { value: 1 }; }), 25); assert.equal(refund(164, 100, (state) => { state.sharedAltarType = { value: 1 }; }), 25);
    // 제단이면서 너겟이면 합 0 이고 1/4 은 생략한다(0).
    assert.equal(refund(0x1234, 100, (state) => { state.sharedAltarType = { value: 0x1234 }; state.sharedNuggetType = { value: 0x1234 }; }), 0);
});

test('Help 항목: NULL 과 빈 문자열을 구별하고 종류 WORD 를 인자로 쓰며 +18/+20 WORD 는 비정렬 little endian 이다', () => {
    const helpEntry = (r) => r.log.filter((entry) => entry[0] === 'entry' && entry[2] === 0x02000017)[0];
    const none = rig({ help: null }); none.menu.open(none.item, { x: 0, y: 0 }); assert.deepEqual(helpEntry(none), ['entry', null, 0x02000017, 0x1234, true, true, false]);
    const blank = rig({ help: '' }); blank.menu.open(blank.item, { x: 0, y: 0 }); assert.deepEqual(helpEntry(blank), ['entry', '', 0x02000017, 0x1234, true, true, false]); const undefinedHelp = rig(); undefinedHelp.hooks.helpWord = () => undefined; undefinedHelp.menu.open(undefinedHelp.item, { x: 0, y: 0 }); assert.equal(helpEntry(undefinedHelp)[1], null);
    const big = rig({ kind: 0xabcd }); big.menu.open(big.item, { x: 0, y: 0 }); assert.equal(big.log.find((entry) => entry[0] === 'describe')[1], 0xabcd); assert.equal(helpEntry(big)[3], 0xabcd);
});

test('open 슬롯: 내용물 번호는 하위 16비트이고 풀 범위 밖이면 어떤 효과도 하기 전에 RangeError 다', () => {
    const r = rig({ content: 7 }); r.item.content = 0x10007; r.menu.open(r.item, { x: 0, y: 0 }); assert.deepEqual(r.log.find((entry) => entry[0] === 'reference'), ['reference', 900, 0x10007]); assert.equal(r.log.find((entry) => entry[0] === 'affordable')[1], 7);
    const out = rig(); out.item.content = 0xffff; assert.throws(() => out.menu.open(out.item, { x: 0, y: 0 }), RangeError); assert.deepEqual(out.log, []);
});
