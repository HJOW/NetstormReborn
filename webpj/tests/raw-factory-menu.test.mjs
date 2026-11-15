// Factory 메뉴 raw-factory-menu.js(RawFactoryMenu.open·currentProduction·slots·formatFactoryMana·bindFactoryMenuControls)를 원본 10.78 관측 76행과 cpppj 시험 계약으로 대조한다(WB04-E545).
//  - factory-menu-x86.tsv 76행: Open 39(공통 Factory +ac 004754b0 → 파생 작성 → 현재 생산/지식/업그레이드의 전체 작성 순서·인자·동률 정렬)·Mana 32(점·활성 개수·알 수 없는 ASCII)·Slots 5(0·음수·양수).
//  - 계약: 거부(타 소유자/Outpost/잘못된 가상 표/저장소 크기), 지식 후보 범위, 컨트롤 위임.
// 실제 게임·브라우저 실행이 아니다. (2026-11-15 WB04-E545)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { bitsToFloat, readRows } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { ContainedFinderState } from '../www/core/raw-contained-finder.js';
import { SquidPostPopList } from '../www/core/squid-postpop-state.js';
import { RawObjectMenuOpen } from '../www/core/raw-object-menu-open.js';
import { RawFactoryMenu, bindFactoryMenuControls, createFactoryMenuState, formatFactoryMana } from '../www/core/raw-factory-menu.js';

// 원본 관측 fixture 이다. 없으면 해당 검사를 건너뛴다. (2026-11-15 WB04-E545)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'factory-menu-x86.tsv'));
// 독립 원본 관측의 명시 TYPE 플래그 입력이다. (2026-11-15 WB04-E545)
const FLAGS = [0x800, 0x8000000, 0x10000, 0x4000000, 0x20000000, 0x40000, 0x20000, 0];
// 부호 입력도 원문 하위 DWORD 로 읽는다. (2026-11-15 WB04-E545)
const number = (text) => Number(text) >>> 0;
// UTF8/스타일 문자열의 바이트를 원본 관측 형식(16진)으로 보존한다. (2026-11-15 WB04-E545)
const textHex = (text) => Buffer.from(text, 'latin1').toString('hex');
// 빈 문자열 표식과 원본 16진 바이트를 읽는다. (2026-11-15 WB04-E545)
const textBytes = (hex) => (hex === '-' ? '' : Buffer.from(hex, 'hex').toString('latin1'));
// 입력 목록을 해석하며 생산/지식의 기대 순서를 계산하지 않는다. (2026-11-15 WB04-E545)
const list = (text) => (text === '-' ? [] : text.split(',').map((value) => Number(value) >>> 0));
// 낮은 바이트부터 정해진 폭만 쓴다. (2026-11-15 WB04-E545)
function put(raw, offset, value, width = 4) {
    for (let index = 0; index < width; index += 1) raw[offset + index] = (value >>> (index * 8)) & 0xff;
}
// 번역/printf 는 원본 관측과 같은 명시 외부 경계이다(고정 %s/%d 만). (2026-11-15 WB04-E545)
function print(text, args) {
    let result = '';
    let argument = 0;
    // 고정 서식의 문자열/정수만 원래 순서로 소비한다.
    for (let index = 0; index < text.length; index += 1) {
        if (text[index] !== '%') { result += text[index]; continue; }
        index += 1;
        assert.ok(argument < args.length);
        if (text[index] === 's') { assert.equal(typeof args[argument], 'string'); result += args[argument]; } else { assert.equal(text[index], 'd'); assert.equal(typeof args[argument], 'number'); result += String(args[argument] | 0); }
        argument += 1;
    }
    assert.equal(argument, args.length);
    return result;
}

// 실제 풀/검색기와 현재 TYPE/지식 조회 입력만 준비하는 장면이다. (2026-11-15 WB04-E545)
class FactoryMenuScene {
    // 명시 raw 입력을 만들며 게임 진행이나 기대 출력은 만들지 않는다.
    constructor(row) {
        this.pool = new SidPool(24000, { server: true });
        this.types = Array.from({ length: 188 }, () => ({ theme: 0, flags2: 0, description: '' }));
        this.contained = new ContainedFinderState();
        this.excluded = new SquidPostPopList();
        this.state = createFactoryMenuState();
        this.trace = '';
        this.taken = 0;
        // 실제 원본과 같은 client SID 를 순서대로 확보한다.
        for (let index = 5; index < 151; index += 1) assert.equal(this.pool.allocate(2), index);
        const type = number(row[1]);
        const level = number(row[5]);
        const theme = number(row[6]);
        this.owner = number(row[3]);
        this.editor = number(row[7]) !== 0;
        this.cost = number(row[8]) | 0;
        this.money = bitsToFloat(number(row[9]));
        this.profile = number(row[14]);
        this.tech = list(row[10]);
        this.blocked = list(row[12]);
        this.production = list(row[13]);
        const raw = this.pool.allocated(50);
        raw.fill(0);
        put(raw, 0, type === 85 ? 0x509148 : 0x509210);
        raw[10] = type;
        raw[11] = (level << 5) & 0xff;
        raw[34] = number(row[2]);
        raw[40] = number(row[4]);
        const products = list(row[11]);
        put(raw, 6, products.length ? 100 : 0, 2);
        // 중복 생산도 각각의 실제 Form 을 만들고 next 순서만 입력한다.
        for (let index = 0; index < products.length; index += 1) {
            const form = this.pool.allocated(100 + index);
            form.fill(0);
            form[10] = 6;
            put(form, 4, index + 1 < products.length ? 101 + index : 0, 2);
            put(form, 18, products[index], 2);
            put(form, 20, 0x10, 2);
        }
        // 현재 TYPE 입력은 원본 관측의 번호/테마/플래그/설명과 같다.
        for (let index = 0; index < this.types.length; index += 1) {
            this.types[index].theme = index === type ? theme | 0 : index % 4;
            this.types[index].flags2 = FLAGS[index % FLAGS.length];
            this.types[index].description = this.profile ? (index % 2 ? 'Same' : 'same') : `Type${index}`;
        }
        this.excluded.entries = this.blocked.slice();
        this.excluded.count = this.blocked.length;
        this.hooks = {
            format: print,
            localOwner: () => this.owner,
            editor: () => this.editor,
            currentTechnology: () => 9,
            technologyType: (parent, index) => { assert.equal(parent, 9); return index < this.tech.length ? this.tech[index] : 0; },
            capacity: (sid) => (this.pool.slot(sid)[11] >> 5) + 2,
            level: (sid) => this.pool.slot(sid)[11] >> 5,
            allowed: (value) => !this.blocked.includes(value),
            inProduction: (value) => this.production.includes(value),
            cost: () => this.cost,
            money: () => this.money,
            description: (value) => this.types[value].description,
            mana: () => '',
            helpWord: () => 'Help',
            beginFancy: () => this.add('F'),
            title: (value) => this.add(`T,${textHex(value)}`),
            header: (value) => this.add(`H,${textHex(value)}`),
            takeMenu: () => { this.taken += 1; return 100 + this.taken; },
            submenu: (value, child) => this.add(`S,${textHex(value)},${child}`),
            item: (value, command, parameter, include, enabled) => this.add(`I,${textHex(value)},${command},${typeof parameter === 'string' ? `str:${textHex(parameter)}` : parameter},${include ? 1 : 0},${enabled ? 1 : 0},0`),
            baseMenu: (sid) => this.add(`B,${sid}`),
        };
    }

    // 각 실제 호출 사이에만 구분자를 넣고 빈 호출 목록은 별도로 비교한다.
    add(value) { this.trace = this.trace ? `${this.trace};${value}` : value; }

    // 현재 참조를 빌린 메뉴 처리기를 만든다.
    menu() { return new RawFactoryMenu(this.pool, this.types, this.contained, this.excluded, this.state, this.hooks); }
}

test('Factory 메뉴: 원본 10.78 Open 39입력의 공통 +ac→생산/지식/업그레이드 작성 순서·인자·동률 정렬이 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    let count = 0;
    // 원본 출력은 계산하지 않고 같은 입력의 실제 새 몸체만 실행한다.
    for (const row of rows.filter((r) => r[0] === 'Open')) {
        assert.equal(row.length, 17);
        const scene = new FactoryMenuScene(row);
        const menu = scene.menu();
        const open = new RawObjectMenuOpen(scene.pool, scene.types, {
            beginFancy: scene.hooks.beginFancy,
            summary: (sid) => scene.add(`Q,${sid}`),
            populate: (sid) => menu.open(sid),
            attach: (sid, x, y) => scene.add(`A,${sid},${x >>> 0},${y >>> 0}`),
        });
        const result = open.openFactory(50, { code: 0, x: 37, y: -11 });
        assert.equal(result, number(row[15]), `Open ${count}`);
        assert.equal(scene.trace || '-', row[16], `Open ${count}: ${row.slice(1, 15).join(',')}`);
        count += 1;
    }
    assert.equal(count, 39);
});

test('Factory 마나 문자·슬롯: 원본 10.78 Mana 32·Slots 5입력(점·활성 개수·알 수 없는 ASCII·0/음수 슬롯)이 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    let mana = 0;
    let slots = 0;
    // 전체 76행 중 문자열 함수의 독립 결과 37개만 재생한다.
    for (const row of rows) {
        if (row[0] === 'Mana') {
            assert.equal(formatFactoryMana(textBytes(row[1]), Number(row[2]) | 0), textBytes(row[3]), `Mana ${mana}: ${row[1]}`);
            mana += 1;
        }
        if (row[0] === 'Slots') {
            const scene = new FactoryMenuScene(['Open', '85', '1', '1', '2', '0', '3', '0', '800', '0', '-', '-', '-', '-', '0']);
            assert.equal(scene.menu().slots(Number(row[1]) | 0), textBytes(row[2]), `Slots ${slots}`);
            slots += 1;
        }
    }
    assert.equal(mana, 32);
    assert.equal(slots, 5);
});

test('Factory 메뉴 계약: 타 소유자/Outpost 는 기본 메뉴만, 잘못된 가상 표·저장소·범위 오류 거부, 지식 후보 행/문자열 범위', () => {
    const open = ['Open', '85', '1', '1', '2', '0', '3', '0', '800', '1158250496', '75,76,77,78,79,80,81', '-', '-', '-', '0'];
    // 다른 소유자의 Factory 는 생산/지식 없이 기본 Damageable 메뉴만 호출한다.
    const other = new FactoryMenuScene(open);
    other.pool.allocated(50)[34] = 2;
    other.menu().open(50);
    assert.equal(other.trace, 'B,50');
    // Outpost(126)도 마찬가지이다.
    const outpost = new FactoryMenuScene(open);
    outpost.pool.allocated(50)[10] = 126;
    put(outpost.pool.allocated(50), 0, 0x509210);
    outpost.menu().open(50);
    assert.equal(outpost.trace, 'B,50');
    // 알려지지 않은 파생 가상 표/범위 밖 SID/저장소 크기는 거부한다.
    const broken = new FactoryMenuScene(open);
    put(broken.pool.allocated(50), 0, 0x509000);
    assert.throws(() => broken.menu().open(50), /가상 표 미복원/);
    assert.throws(() => broken.menu().open(0), RangeError);
    assert.throws(() => new RawFactoryMenu(broken.pool, broken.types.slice(1), broken.contained, broken.excluded, broken.state, broken.hooks), TypeError);
    // 지식 후보의 마나 9·설명 81 바이트 한도와 NUL 이후 절단.
    const long = new FactoryMenuScene(open);
    long.hooks.mana = () => '0123456789';
    assert.throws(() => long.menu().open(50), RangeError);
    const trimmed = new FactoryMenuScene(open);
    trimmed.hooks.description = (value) => `${trimmed.types[value].description}\0ignored`;
    assert.doesNotThrow(() => trimmed.menu().open(50));
    assert.ok(!trimmed.trace.includes(textHex('ignored')));
    // 선택한 경계(기본 메뉴)가 없으면 성공으로 대체하지 않는다.
    const missing = new FactoryMenuScene(open);
    delete missing.hooks.baseMenu;
    assert.throws(() => missing.menu().open(50), /미연결/);
});

test('Factory 메뉴 계약: 마나 문자 범위·길이 진단·None 선조회·업그레이드의 편집 지불·후보 200행·설명 81바이트·NUL/한도', () => {
    const open = ['Open', '85', '1', '1', '2', '0', '3', '0', '800', '1158250496', '75,76,77,78,79,80,81', '-', '-', '-', '0'];
    // 마나 문자열은 ASCII 127 까지만 받고, 토큰 출력이 250바이트에 닿으면 원본 진단으로 거부한다(25×10바이트).
    assert.doesNotThrow(() => formatFactoryMana(String.fromCharCode(127), -1));
    assert.throws(() => formatFactoryMana(String.fromCharCode(128), -1), RangeError);
    assert.doesNotThrow(() => formatFactoryMana('>'.repeat(24), -1));
    assert.throws(() => formatFactoryMana('>'.repeat(25), -1), RangeError);
    // 첫 NUL 에서 멈춘다.
    assert.equal(formatFactoryMana('ww\0ww', 1), '~[I103.11]~[I103.0]');
    // 슬롯 문자열은 양수/음수에서도 먼저 None 번역을 조회한다.
    const translated = [];
    const scene = new FactoryMenuScene(open);
    scene.hooks.format = (key, args) => { translated.push(key); return print(key, args); };
    const menu = scene.menu();
    assert.equal(menu.slots(3), formatFactoryMana('???', -1));
    assert.equal(menu.slots(-2), '');
    assert.deepEqual(translated, ['None', 'None']);
    // 편집 모드는 비용이 SP 보다 커도 업그레이드 가격을 지불 가능한 색으로 만든다.
    const label = (editor) => {
        const poor = new FactoryMenuScene(['Open', '85', '1', '1', '2', '0', '3', editor ? '1' : '0', '1000', '0', '75,76', '-', '-', '-', '0']);
        poor.menu().open(50);
        return poor.trace;
    };
    assert.ok(label(false).includes(textHex('Upgrade costs ~r1000~[IfortGump.3]')), label(false));
    assert.ok(label(true).includes(textHex('Upgrade costs 1000~[IfortGump.3]')), label(true));
    // 후보는 최대 200행이고 설명은 81바이트까지이며 NUL 뒤는 버린다.
    const candidates = (count) => {
        const many = new FactoryMenuScene(open);
        many.hooks.technologyType = (parent, index) => (index < count ? 8 : 0);
        many.menu().open(50);
        return many;
    };
    assert.doesNotThrow(() => candidates(200));
    assert.throws(() => candidates(201), RangeError);
    const describe = (text) => {
        const one = new FactoryMenuScene(open);
        one.hooks.technologyType = (parent, index) => (index < 1 ? 8 : 0);
        one.hooks.description = () => text;
        one.menu().open(50);
        return one.trace;
    };
    assert.doesNotThrow(() => describe('d'.repeat(81)));
    assert.throws(() => describe('d'.repeat(82)), RangeError);
    assert.ok(describe('Abc\0ignored').includes(`I,${textHex('Abc')},`));
    // 번역 결과의 NUL 뒤는 버리고 255바이트까지만 허용한다.
    const formatted = (text) => {
        const one = new FactoryMenuScene(open);
        const base = one.hooks.format;
        one.hooks.format = (key, args) => (key === 'Knowledge Available\n' ? text : base(key, args));
        one.menu().open(50);
        return one.trace;
    };
    assert.ok(formatted('K\0z').includes(`T,${textHex('K')};`));
    assert.doesNotThrow(() => formatted('x'.repeat(255)));
    assert.throws(() => formatted('x'.repeat(256)), RangeError);
});

test('Factory 메뉴 계약: 설명의 NUL 뒤 바이트는 81바이트 한도에 넣지 않는다', () => {
    const open = ['Open', '85', '1', '1', '2', '0', '3', '0', '800', '1158250496', '75,76,77,78,79,80,81', '-', '-', '-', '0'];
    const scene = new FactoryMenuScene(open);
    scene.hooks.technologyType = (parent, index) => (index < 1 ? 8 : 0);
    scene.hooks.description = () => `Abc\0${'x'.repeat(100)}`;
    assert.doesNotThrow(() => scene.menu().open(50));
    assert.ok(scene.trace.includes(`I,${textHex('Abc')},`), scene.trace);
});

test('Factory 메뉴 연결: 컨트롤/메뉴 위임과 정적 문자열 인자 인코딩', () => {
    const calls = [];
    const record = (name, result) => (...args) => { calls.push([name, ...args]); return result; };
    const widgets = { beginFancy: record('beginFancy') };
    const controls = { addTitle: record('addTitle'), addHeader: record('addHeader'), addSubmenu: record('addSubmenu'), addText: record('addText') };
    const menus = { takeCurrent: record('takeCurrent', 55) };
    const hooks = {};
    bindFactoryMenuControls(hooks, widgets, controls, menus, (text) => { calls.push(['encode', text]); return 9000; });
    hooks.beginFancy();
    hooks.title('T');
    hooks.header('H');
    assert.equal(hooks.takeMenu(), 55);
    hooks.submenu('S', 7);
    hooks.item('A', 1, 5, true, false);
    hooks.item('B', 2, 'researchHelp', false, true);
    assert.deepEqual(calls, [['beginFancy'], ['addTitle', 'T'], ['addHeader', 'H'], ['takeCurrent'], ['addSubmenu', 'S', 7, true, false], ['addText', 'A', 1, 5, true, false, false],
        ['encode', 'researchHelp'], ['addText', 'B', 2, 9000, false, true, false]]);
    const bare = {};
    bindFactoryMenuControls(bare, widgets, controls, menus, undefined);
    assert.throws(() => bare.item('x', 1, 'researchHelp', true, true), /미연결/);
});
