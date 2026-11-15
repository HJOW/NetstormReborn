// 게임 메뉴막대 raw-game-menu-bar.js(RawGameMenuBar.open·bindGameMenuBarControls)를 원본 10.78 관측 510행과 cpppj 시험 계약으로 대조한다(WB04-E539).
//  - game-menu-bar-x86.tsv 510행: editor/battle/cheat/mission/검산/기술 도움말/폭/Left/변형 9개 DWORD 입력에서 최종 검산 DWORD·생성순번·하위 호출·번역·항목·폭·부착의 전체 순서.
//  - 계약: 없는 하위는 빈 성공이 아니라 명시 거부, 같은 검산 상자 소유, 컨트롤 연결의 위임.
// 실제 게임·브라우저 실행이 아니다. (2026-11-15 WB04-E539)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { floatToBits, readRows } from './raw-support.mjs';
import { GameMenuBarChild, GameMenuBarValue, RawGameMenuBar, bindGameMenuBarControls, createGameMenuBarState } from '../www/core/raw-game-menu-bar.js';

// 원본 관측 fixture 이다. 없으면 해당 검사를 건너뛴다. (2026-11-15 WB04-E539)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'game-menu-bar-x86.tsv'));
// 부호 입력도 원문 하위 DWORD 로 읽는다. (2026-11-15 WB04-E539)
const number = (text) => Number(text) >>> 0;
// 원본 문자열 바이트를 기대값과 같은 16진으로 기록한다. (2026-11-15 WB04-E539)
const encoded = (text) => Buffer.from(text, 'latin1').toString('hex');

// 하위 부모/번역/GUI 효과는 독립 원문 관측과 같은 명시 장치이며 제품 계산을 대신하지 않는 장면이다. (2026-11-15 WB04-E539)
class GameMenuBarScene {
    // 각 원시 DWORD 를 변환 없이 읽고 출력 기대는 fixture 에만 둔다.
    constructor(values) {
        this.input = values.split(',').map(number);
        assert.equal(this.input.length, 9);
        // 독립 원문 장치의 같은 추적 DWORD 소유자이다. 제품은 실제 기존 소유 주소를 빌리며 별도 검산 사본을 갖지 않는다.
        this.tracking = { value: this.input[4] | 0 };
        this.state = createGameMenuBarState(this.tracking);
        this.state.menuSerial = 0xa5a5a5a5;
        this.takes = 0;
        this.serial = 77;
        this.trace = '';
    }

    // 실제 외부 효과를 한 번 기록한다.
    add(event) { this.trace = this.trace ? `${this.trace};${event}` : event; }

    // 각 실제 접근 시점의 같은 원문 전역을 읽는다.
    read(value) {
        switch (value) {
        case GameMenuBarValue.Editor: return this.input[0];
        case GameMenuBarValue.Battle: return this.input[1];
        case GameMenuBarValue.Cheat: return this.input[2];
        case GameMenuBarValue.Mission: return this.input[3];
        case GameMenuBarValue.Width: return this.input[6];
        case GameMenuBarValue.Left: return this.input[7];
        case GameMenuBarValue.Root: return 0x01001234;
        default: throw new Error('관측 밖 메뉴 전역');
        }
    }

    // 원문 실행의 명시 장치만 같은 제품 훅에 제공한다.
    hooks() {
        const input = this.input;
        return {
            read: (value) => this.read(value),
            prepare: () => this.add('P'),
            translate: (text) => { this.add(`T,${encoded(text)}`); return text; },
            about: () => 'About NetStorm',
            horizontal: () => { this.add('H'); return 0x01001000; },
            vertical: () => this.add('V'),
            separator: () => this.add('X'),
            take: () => { this.add('Q'); this.takes += 1; return this.takes === 1 ? 7 : 0x01001000; },
            child: (kind) => {
                this.add(`C,${kind}`);
                // 실제 도달 하위만 관측한다.
                if ((input[8] & 1) && kind === 0) input[0] ^= 1;
                if ((input[8] & 2) && kind === 3) { input[1] ^= 1; this.tracking.value = 9599; }
                return kind === 5 ? ((input[8] & 32) ? 0 : 8) : kind + 2;
            },
            submenu: (text, child) => this.add(`S,${encoded(text)},${child}`),
            item: (text, code, parameter, include) => this.add(`I,${encoded(text)},${code},${encoded(parameter)},${include},1,0`),
            technicalHelp: () => { this.add('D'); if (input[8] & 4) { input[2] ^= 1; input[3] ^= 1; } return input[5]; },
            width: (token, width) => { this.add(`W,${token},${floatToBits(width)}`); if (input[8] & 8) input[7] = 123; },
            attach: (token, x, y, parent, mode) => {
                this.add(`A,${token},${floatToBits(x)},${floatToBits(y)},${parent},${mode}`);
                if (input[8] & 16) this.serial = 991;
            },
            serial: (token) => { assert.equal(token, 0x01001000); return this.serial; },
        };
    }

    // 실제 부모가 남긴 최종 두 DWORD 와 실제 순서만 직렬화한다.
    run() {
        const menu = new RawGameMenuBar(this.state, this.hooks());
        assert.equal(menu.open(), 0x01001000);
        assert.equal(this.takes, 2);
        assert.equal(this.state.checksum, this.tracking);
        return `${this.tracking.value >>> 0}|${this.state.menuSerial}|${this.trace}`;
    }
}

test('게임 메뉴막대: 원본 10.78 510입력의 검산 DWORD·생성순번·하위/번역/항목/폭/부착 순서가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    assert.equal(rows.length, 510);
    let count = 0;
    // 각 독립 행은 새로운 장치와 실제 공개 제품 본체를 사용한다.
    for (const row of rows) {
        assert.equal(row.length, 3);
        assert.equal(new GameMenuBarScene(row[1]).run(), row[2], `행 ${count}: ${row[0]} ${row[1]}`);
        count += 1;
    }
    assert.equal(count, 510);
});

test('게임 메뉴막대 계약: 없는 하위는 빈 성공으로 바꾸지 않고 선택한 경계의 누락은 명시 거부한다', () => {
    const scene = new GameMenuBarScene('0,1,0,1,0,1,1024,85,0');
    const hooks = scene.hooks();
    delete hooks.child;
    // 첫 Game 하위 경계에서 막히며 그 전의 준비/가로/번역 효과는 이미 일어난 채로 남는다.
    assert.throws(() => new RawGameMenuBar(scene.state, hooks).open(), /하위 부모 연결 누락/);
    assert.equal(scene.trace, 'P;H;T,47616d65');
    for (const [name, pattern] of [['prepare', /Player 준비/], ['horizontal', /가로 생성/], ['translate', /번역/], ['read', /전역 조회/]]) {
        const broken = new GameMenuBarScene('0,0,0,0,0,0,1024,85,0');
        const brokenHooks = broken.hooks();
        delete brokenHooks[name];
        assert.throws(() => new RawGameMenuBar(broken.state, brokenHooks).open(), pattern, name);
    }
    // 검산 상자 없이는 상태를 만들 수 없다.
    assert.throws(() => createGameMenuBarState(null), /검산 상자/);
});

test('게임 메뉴막대 계약: 현재 전역 읽기 순서(Cheat 단락·Left 먼저 Width·Left 재조회)와 폭의 단정도 반올림', () => {
    // 읽기는 원본 접근 순서대로이며 Cheat 가 거짓이면 Mission 은 읽지 않는다. 원본 fixture 는 읽기 순서를 관측하지 않아 직접 확인한다.
    const run = (values) => {
        const scene = new GameMenuBarScene(values);
        const hooks = scene.hooks();
        const reads = [];
        const widths = [];
        const baseRead = hooks.read;
        const baseWidth = hooks.width;
        hooks.read = (value) => { reads.push(value); return baseRead(value); };
        hooks.width = (token, width) => { widths.push(width); baseWidth(token, width); };
        new RawGameMenuBar(scene.state, hooks).open();
        return { reads, widths };
    };
    const { Editor, Battle, Cheat, Mission, Width, Left, Root } = GameMenuBarValue;
    assert.deepEqual(run('0,0,0,1,0,1,1024,85,0').reads, [Editor, Battle, Cheat, Left, Width, Left, Root]);
    assert.deepEqual(run('0,0,1,1,0,1,1024,85,0').reads, [Editor, Battle, Cheat, Mission, Left, Width, Left, Root]);
    // 폭은 단정도로 반올림해 넘긴다(2^24 보다 큰 정수는 정확히 표현되지 않는다).
    assert.deepEqual(run('0,0,0,0,0,1,16777217,0,0').widths, [16777216]);
    // Width−Left 는 DWORD 감김 뒤 signed 해석이다(Left 가 더 크면 음수 폭).
    assert.deepEqual(run('0,0,0,0,0,1,100,300,0').widths, [-200]);
});

test('게임 메뉴막대 연결: 컨트롤/위젯/메뉴 처리기로 위임하고 숫자 0 과 문자열 고리를 구별하며 사라진 노드를 거부한다', () => {
    const calls = [];
    const record = (name) => (...args) => { calls.push([name, ...args]); return `${name}-result`; };
    const widgets = { beginHorizontal: record('beginHorizontal'), beginVertical: record('beginVertical'), attach: record('attach') };
    const controls = { addSeparator: record('addSeparator'), addSubmenu: record('addSubmenu'), addText: record('addText') };
    const menus = { takeCurrent: record('takeCurrent'), setWidth: record('setWidth') };
    const nodes = new Map([[5, { serial: 321 }]]);
    const hooks = {};
    bindGameMenuBarControls(hooks, widgets, controls, menus, (token) => nodes.get(token) ?? null, (text) => { calls.push(['store', text]); return 900; });
    assert.equal(hooks.horizontal(), 'beginHorizontal-result');
    assert.equal(hooks.vertical(), undefined);
    assert.equal(hooks.take(), 'takeCurrent-result');
    hooks.separator();
    hooks.submenu('Game', 7);
    hooks.item('Help', 0x02000017, 'F1Help', 1);
    hooks.item('Technical', 0x02000049, '', 0);
    hooks.width(7, 640);
    hooks.attach(7, 8, 0, 0x1234, 1);
    assert.equal(hooks.serial(5), 321);
    assert.deepEqual(calls, [['beginHorizontal'], ['beginVertical', 0], ['takeCurrent'], ['addSeparator'], ['addSubmenu', 'Game', 7, true, false],
        ['store', 'F1Help'], ['addText', 'Help', 0x02000017, 900, true, true, false], ['addText', 'Technical', 0x02000049, 0, false, true, false],
        ['setWidth', 7, 640], ['attach', 7, 8, 0, 0x1234, 1]]);
    // 사라진 노드는 성공으로 바꾸지 않는다.
    assert.throws(() => hooks.serial(99), /노드 소멸/);
    // 문자열 고리가 없으면 문자열 인자 항목을 만들 수 없다.
    const bare = {};
    bindGameMenuBarControls(bare, widgets, controls, menus, () => null, undefined);
    assert.throws(() => bare.item('x', 1, 'F1Help', 1), /문자열 고리 연결 누락/);
    assert.doesNotThrow(() => bare.item('x', 1, '', 1));
    assert.ok(GameMenuBarChild.Cheat === 5);
});
