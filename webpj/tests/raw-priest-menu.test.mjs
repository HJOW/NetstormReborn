// 사제 건설 메뉴 raw-priest-menu.js(RawPriestMenu.open·selectBuild·hasTheme·frameState·bind*)를 원본 10.78 관측 905행과 cpppj 시험 계약으로 대조한다(WB04-E540).
//  - priestmenu-x86.tsv: Open 329(전체 메뉴 구성의 호출 순서 문자열)·Select 64(두 금액 조회/NaN/float 축소/워크샾 테마 관문의 생성·안내·커서 효과와 조회 횟수)·Theme 256·Frame 256(signed char 프레임 코드).
//  - 계약: 같은 기술 체인의 theme/flags4/flags2 0x800 와 signed 프레임, 미연결 효과 거부, 컨트롤/월드 연결의 위임.
// 실제 게임·브라우저 실행이 아니다. (2026-11-15 WB04-E540)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { bitsToFloat, readRows } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { ContainedFinderState } from '../www/core/raw-contained-finder.js';
import { SquidPostPopList } from '../www/core/squid-postpop-state.js';
import { RawPriestMenu, bindPriestMenuControls, bindPriestMenuWorld, createPriestMenuState } from '../www/core/raw-priest-menu.js';

// 원본 관측 fixture 이다. 없으면 해당 검사를 건너뛴다. (2026-11-15 WB04-E540)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'priestmenu-x86.tsv'));
// 실제 PE 의 메뉴 타입 순서와 fixture mask 대응이다. (2026-11-15 WB04-E540)
const ORDER = [123, 124, 125, 85, 86, 87, 88, 126, 165, 164];
// 부호 입력도 원문 하위 DWORD 로 읽는다. (2026-11-15 WB04-E540)
const number = (text) => Number(text) >>> 0;
// 문자열을 바이트 단위 16진으로 기록하여 NUL/스타일 표식/구분자 충돌을 피한다. (2026-11-15 WB04-E540)
const hex = (text) => Buffer.from(text, 'latin1').toString('hex');
// 명시 입력의 낮은 바이트부터 정해진 폭만 쓴다. (2026-11-15 WB04-E540)
function put(raw, offset, value, width = 4) {
    for (let index = 0; index < width; index += 1) raw[offset + index] = (value >>> (index * 8)) & 0xff;
}
// 번역/printf 는 이 검사의 명시 외부 경계이다. 원본 고정 서식의 %s/%d 만 처리한다. (2026-11-15 WB04-E540)
function menuFormat(text, args) {
    let result = '';
    let argument = 0;
    // 현재 서식의 변환 인자를 순서대로 소비한다.
    for (let index = 0; index < text.length; index += 1) {
        if (text[index] !== '%') { result += text[index]; continue; }
        index += 1;
        assert.ok(index < text.length && argument < args.length);
        if (text[index] === 's') { assert.equal(typeof args[argument], 'string'); result += args[argument]; } else if (text[index] === 'd') { assert.equal(typeof args[argument], 'number'); result += String(args[argument] | 0); } else assert.fail('서식');
        argument += 1;
    }
    assert.equal(argument, args.length);
    return result;
}

// 기계어 oracle 과 같은 명시 입력/관찰 경계를 구성하되 메뉴 조건의 기대 결과는 계산하지 않는 장면이다. (2026-11-15 WB04-E540)
class MenuScene {
    // 실사용 Pool 에 입력 슬롯을 확보하고 UI 효과만 기록한다. 새 게임 실행/메뉴 픽셀 검사가 아니다.
    constructor() {
        this.pool = new SidPool(24000, { server: true });
        this.types = Array.from({ length: 188 }, (_, type) => ({ theme: type % 4, flags2: 0 }));
        this.frames = Array.from({ length: 188 }, () => ({ codes: [{ side: 65 }] }));
        this.players = new Uint8Array(1620);
        this.excluded = new SquidPostPopList();
        this.contained = new ContainedFinderState();
        this.state = createPriestMenuState();
        this.blocked = 0; this.building = 0; this.immobile = 0; this.allowed = 1023; this.contents = 1; this.taken = 0; this.moneyCalls = 0;
        this.amount = 400;
        this.money = [bitsToFloat(0x453b8000)];
        this.trace = '';
        // 원본 같은 client 번호 공간에서 raw 입력 50/Technology 20/내용물 21..24 를 확보한다.
        for (let index = 5; index <= 70; index += 1) assert.equal(this.pool.allocate(2), index);
        // 타입별 설명/테마를 명시 입력으로 준비한다(types 의 theme 는 위 생성자에서 type%4).
        const raw = this.pool.allocated(50);
        raw[10] = 158;
        raw[34] = 1;
        this.hooks = {
            hasEvent: (sid, event) => (event === 0x493 ? this.blocked : event === 0x25d ? this.building : 0) !== 0,
            immobile: () => this.immobile !== 0,
            currentTechnology: () => 20,
            containedType: (parent, mask, type) => ((parent === 20 && (this.contents & 1)) || (parent === 50 && (this.contents & 2)) ? type : 0),
            allowed: (type) => { const found = ORDER.indexOf(type); assert.ok(found >= 0); return (this.allowed & (1 << found)) !== 0; },
            cost: () => this.amount,
            money: () => { const value = this.money[Math.min(this.moneyCalls, this.money.length - 1)]; this.moneyCalls += 1; return value; },
            description: (type) => `Type${type}`,
            format: menuFormat,
            beginFancy: () => this.add('F'),
            beginVertical: () => this.add('V'),
            title: (text) => this.add(`T,${hex(text)}`),
            takeMenu: () => { this.taken += 1; return 100 + this.taken; },
            submenu: (text, child, enabled) => this.add(`S,${hex(text)},${child},${enabled ? 1 : 0},0`),
            item: (text, command, parameter, enabled) => this.add(`I,${hex(text)},${command},${parameter},1,${enabled ? 1 : 0},0`),
            separator: () => this.add('X'),
            baseMenu: (sid) => this.add(`B,${sid}`),
            tell: (key) => this.add(`D,${hex(key)}`),
            createPreview: (parent, type) => { this.add(`C,${parent},${type}`); return 70; },
            takePreview: (sid, flags) => this.add(`P,${sid},${flags}`),
        };
    }

    // 예상 결과가 아닌 실제 호출 효과를 세미콜론 순서로 연결한다.
    add(value) { this.trace = this.trace ? `${this.trace};${value}` : value; }

    // 같은 실제 Technology 종속 체인으로 독립 theme 비트 입력을 표현한다(각 원소마다 flags4 의 기술 Form 한 개를 체인에 넣는다).
    themes(mask) {
        put(this.pool.allocated(20), 6, 21, 2);
        for (let index = 0; index < 4; index += 1) {
            const sid = 21 + index;
            const raw = this.pool.allocated(sid);
            raw[10] = 6;
            put(raw, 4, index < 3 ? sid + 1 : 0, 2);
            put(raw, 18, 170 + index, 2);
            put(raw, 20, (mask & (1 << index)) ? 4 : 0, 2);
            this.types[170 + index].theme = index;
        }
    }

    // 현재 참조를 빌린 메뉴 처리기를 생성하며 저장소를 복사하지 않는다.
    menu() { return new RawPriestMenu(this.pool, this.types, this.frames, this.players, this.excluded, this.contained, this.state, this.hooks); }

    // fixture 의 명시 모드를 원본 각 전역에 적용한다.
    modes(value) {
        this.state.workshopMode = (value & 1) !== 0;
        this.state.special = (value & 2) !== 0;
        this.state.editor = (value & 4) !== 0;
        this.state.chargeBlocked = (value & 8) !== 0;
    }
}

test('사제 메뉴: 원본 10.78 Open 329입력의 전체 메뉴 구성 호출 순서가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    let count = 0;
    // 기계어가 관찰한 독립 입력을 모두 재생한다.
    for (const row of rows.filter((r) => r[0] === 'Open')) {
        assert.equal(row.length, 16);
        const scene = new MenuScene();
        const raw = scene.pool.allocated(50);
        raw[34] = number(row[1]);
        scene.state.localOwner = number(row[2]);
        scene.blocked = number(row[3]);
        scene.building = number(row[4]);
        scene.immobile = number(row[5]);
        scene.frames[158] = { codes: [{ side: Number(row[6]) + 65 }] };
        scene.modes(number(row[7]));
        const flags = number(row[8]);
        // 네 실제 Player 필드의 유무를 독립 입력에서 쓴다.
        [0x7c, 0x84, 0x8c, 0x94].forEach((offset, index) => put(scene.players, raw[34] * 180 + offset, (flags & (1 << index)) ? 1 : 0));
        scene.players[scene.state.localOwner * 180 + 0x71] = 3;
        scene.allowed = number(row[9]);
        scene.themes(number(row[10]));
        scene.contents = number(row[11]);
        scene.amount = Number(row[12]) | 0;
        scene.money = [bitsToFloat(number(row[13]))];
        const excluded = number(row[14]);
        scene.excluded.entries = ORDER.slice();
        scene.excluded.count = 0;
        // 실제 제외 목록은 활성 타입만 같은 순서로 보관한다.
        for (let index = 0; index < ORDER.length; index += 1) if (excluded & (1 << index)) scene.excluded.entries[scene.excluded.count++] = ORDER[index];
        scene.menu().open(50);
        assert.equal(scene.trace || '-', row[15], `Open ${count}: ${row.slice(1, 15).join(',')}`);
        count += 1;
    }
    assert.equal(count, 329);
});

test('사제 건설 선택: 원본 10.78 Select 64입력의 두 금액 조회·NaN·float 축소·워크샾 테마 관문이 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    let count = 0;
    // 선택 결과는 생성/안내/커서 효과 및 실제 조회 횟수로 검증한다.
    for (const row of rows.filter((r) => r[0] === 'Select')) {
        assert.equal(row.length, 9);
        const scene = new MenuScene();
        scene.amount = Number(row[1]) | 0;
        scene.money = [bitsToFloat(number(row[2])), bitsToFloat(number(row[3]))];
        scene.modes(number(row[4]));
        scene.types[123].flags2 = number(row[5]) ? 0x4000 : 0;
        scene.themes(number(row[6]) << 3);
        const result = scene.menu().selectBuild(123, 50);
        assert.equal(result, row[8].includes('P,'), `Select ${count}`);
        assert.equal(scene.moneyCalls, number(row[7]), `Select ${count}`);
        assert.equal(scene.trace || '-', row[8], `Select ${count}`);
        count += 1;
    }
    assert.equal(count, 64);
});

test('사제 메뉴 getter: 원본 10.78 Theme 256·Frame 256입력이 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    let themes = 0;
    let frames = 0;
    // 각 기대값은 원본 getter 몸체의 반환값이며 이 검사에서 계산하지 않는다.
    for (const row of rows) {
        if (row[0] === 'Theme') {
            const scene = new MenuScene();
            scene.themes(number(row[1]));
            const banned = number(row[2]);
            // 원본 타입의 flags2 0x800 을 기술 네 종에 독립으로 쓴다.
            for (let index = 0; index < 4; index += 1) scene.types[170 + index].flags2 = (banned & (1 << index)) ? 0x800 : 0;
            assert.equal(scene.menu().hasTheme(Number(row[3]) | 0), number(row[4]) !== 0, `Theme ${themes}`);
            themes += 1;
        } else if (row[0] === 'Frame') {
            const scene = new MenuScene();
            scene.frames[158] = { codes: [{ side: number(row[1]) }] };
            assert.equal(scene.menu().frameState(50), Number(row[2]) | 0, `Frame ${frames}`);
            frames += 1;
        }
    }
    assert.equal(themes, 256);
    assert.equal(frames, 256);
});

test('사제 메뉴 계약: 같은 기술 체인의 theme/flags4/flags2 0x800 와 signed 프레임 코드·미연결 효과 거부', () => {
    const scene = new MenuScene();
    scene.themes(15);
    const menu = scene.menu();
    assert.ok(menu.hasTheme(0) && menu.hasTheme(3));
    scene.types[173].flags2 = 0x800;
    assert.ok(!menu.hasTheme(3));
    scene.types[173].flags2 = 0;
    put(scene.pool.allocated(24), 20, 0, 2);
    assert.ok(!menu.hasTheme(3));
    put(scene.pool.allocated(24), 20, 4, 2);
    assert.ok(menu.hasTheme(3));
    scene.frames[158] = { codes: [{ side: 255 }, { side: 75 }] };
    assert.equal(menu.frameState(50), -66);
    put(scene.pool.allocated(50), 36, 1);
    assert.equal(menu.frameState(50), 10);
    // 선택한 분기가 요구하는 효과/조회가 없으면 성공으로 대체하지 않는다.
    const broken = new MenuScene();
    delete broken.hooks.baseMenu;
    broken.pool.allocated(50)[34] = 2;
    assert.throws(() => broken.menu().open(50), /미연결/);
    // 범위 밖 SID·잘못된 저장소 크기는 거부한다.
    assert.throws(() => scene.menu().open(0), RangeError);
    assert.throws(() => new RawPriestMenu(scene.pool, scene.types.slice(1), scene.frames, scene.players, scene.excluded, scene.contained, scene.state, scene.hooks), TypeError);
});

test('사제 메뉴 계약: 번역 결과는 첫 NUL 까지·255바이트까지만 허용하고 기도 분기는 막힘 사건을 호출 순간 다시 조회한다', () => {
    // 번역 훅이 NUL 뒤에 남긴 바이트는 원본 문자열이 아니므로 버리고, 256바이트 이상은 원본 스택 버퍼 한도 초과로 거부한다.
    const nulScene = new MenuScene();
    const baseFormat = nulScene.hooks.format;
    nulScene.hooks.format = (key, args) => (key === 'Construct Building\n' ? 'AB\0CD' : baseFormat(key, args));
    nulScene.menu().open(50);
    assert.ok(nulScene.trace.startsWith(`F;T,${hex('AB')};V;`), nulScene.trace.slice(0, 40));
    const limit = (length) => {
        const scene = new MenuScene();
        const base = scene.hooks.format;
        scene.hooks.format = (key, args) => (key === 'Construct Building\n' ? 'x'.repeat(length) : base(key, args));
        scene.menu().open(50);
    };
    assert.doesNotThrow(() => limit(255));
    assert.throws(() => limit(256), RangeError);
    // 막힘(0x493) 사건은 첫 조회(메뉴 진입 관문)에서는 없고 기도 분기의 마지막 조회에서만 생기면 기도 메뉴를 만들지 않는다.
    const calls = [];
    const scene = new MenuScene();
    scene.state.chargeBlocked = false;
    let blockedCalls = 0;
    scene.hooks.hasEvent = (sid, event) => { calls.push(event); if (event !== 0x493) return false; blockedCalls += 1; return blockedCalls >= 2; };
    scene.menu().open(50);
    assert.equal(calls.filter((event) => event === 0x493).length, 2);
    assert.ok(!scene.trace.includes(`T,${hex('Prayer\n')}`), scene.trace);
    // 두 조회 모두 없으면 기도 메뉴가 만들어진다(대조군).
    const control = new MenuScene();
    control.menu().open(50);
    assert.ok(control.trace.includes(`T,${hex('Prayer\n')}`));
});

test('사제 메뉴 연결: 컨트롤은 기존 메뉴 처리기에, 월드는 같은 Pool 의 조회/생성에 위임하고 다른 Pool 은 거부한다', () => {
    const calls = [];
    const record = (name) => (...args) => { calls.push([name, ...args]); return `${name}-result`; };
    const widgets = { beginFancy: record('beginFancy'), beginVertical: record('beginVertical') };
    const controls = { addTitle: record('addTitle'), addSubmenu: record('addSubmenu'), addText: record('addText'), addSeparator: record('addSeparator') };
    const menus = { takeCurrent: record('takeCurrent') };
    const hooks = {};
    bindPriestMenuControls(hooks, widgets, controls, menus);
    hooks.beginFancy();
    hooks.beginVertical();
    hooks.title('T');
    assert.equal(hooks.takeMenu(), 'takeCurrent-result');
    hooks.submenu('S', 5, true);
    hooks.item('I', 0x02000060, 7, false);
    hooks.separator();
    assert.deepEqual(calls, [['beginFancy'], ['beginVertical', 0], ['addTitle', 'T'], ['takeCurrent'], ['addSubmenu', 'S', 5, true, false], ['addText', 'I', 0x02000060, 7, true, false, false], ['addSeparator']]);
    // 월드 연결: 같은 Pool 이어야 하며 이벤트는 프로세스 존재 여부, 내용물 타입은 찾은 슬롯의 +0x12 WORD, 생성은 원본 고정 인자.
    const pool = new SidPool(24000, { server: true });
    const priest = { pool, immobile: (sid) => sid === 9 };
    const processes = { pool, findEvent: (sid, event) => (sid === 5 && event === 0x493 ? { event } : null) };
    const players = { pool, currentTechnology: () => 20 };
    const reward = { pool, typeCostUnits: (type) => type * 10 };
    const slot = pool.allocate(2);
    put(pool.allocated(slot), 18, 0x1234, 2);
    const carrier = { pool, findContained: (parent, mask, type, index) => (parent === 7 && mask === 4 && type === 126 && index === 0 ? slot : 0) };
    const created = [];
    const create = { pool, create: (...args) => { created.push(args); return 70; } };
    const world = {};
    bindPriestMenuWorld(world, pool, priest, processes, players, carrier, reward, create);
    assert.ok(world.immobile(9) && !world.immobile(8));
    assert.ok(world.hasEvent(5, 0x493) && !world.hasEvent(5, 0x25d));
    assert.equal(world.currentTechnology(), 20);
    assert.equal(world.cost(12), 120);
    assert.equal(world.containedType(7, 4, 126), 0x1234);
    assert.equal(world.containedType(7, 4, 99), 0);
    assert.equal(world.createPreview(50, 123), 70);
    assert.deepEqual(created, [[50, 123, 0, 0x20, 0x10, 0]]);
    assert.throws(() => bindPriestMenuWorld({}, pool, priest, { ...processes, pool: new SidPool(24000, { server: true }) }, players, carrier, reward, create), TypeError);
});
