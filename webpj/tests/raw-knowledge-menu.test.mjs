// 전체 지식 창 raw-knowledge-menu.js(showKnowledgeMenu·showWorldKnowledgeMenu·showKnowledgeSideMenu·knowledgeScriptCode)를 원본 10.78 관측 75행과 cpppj 시험 계약으로 대조한다(WB04-E537).
//  - knowledge-menu-x86.tsv 25관측(Storm 카드): 차단/타입 수/보유/제외/언어/화면/아이콘/번역 profile 입력에서 노드 아홉 필드(단정도 비트)·표식·80BYTE 인자·호출 순서 전체.
//  - knowledge-world-menu-x86.tsv 50관측(World 카드 25·선택 메뉴 25): 같은 열.
//  - 계약: 차단 시 효과 없음·미연결 효과 거부·문자열/행 수 범위·NUL 뒤 문자열 제외·이름표.
// 실제 게임·브라우저 실행이 아니다. (2026-11-15 WB04-E537)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { floatToBits, readRows } from './raw-support.mjs';
import { CHEAT_ALL_TECHNOLOGY_COMMAND, SHOW_TEC_MENU2_COMMAND, SHOW_TEC_SIDE_MENU_COMMAND, SHOW_TECHNOLOGY_COMMAND, createKnowledgeMenuState, knowledgeScriptCode, showKnowledgeMenu, showKnowledgeSideMenu, showWorldKnowledgeMenu } from '../www/core/raw-knowledge-menu.js';

// 원본 관측 fixture 이다. 없으면 해당 검사를 건너뛴다. (2026-11-15 WB04-E537)
const fixture = (name) => readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', name));
const stormRows = fixture('knowledge-menu-x86.tsv');
const worldRows = fixture('knowledge-world-menu-x86.tsv');

// 부호 입력도 원문 하위 DWORD 로 읽는다. (2026-11-15 WB04-E537)
const number = (text) => Number(text) >>> 0;
// 원문 문자열의 바이트를 기대값과 같은 16진으로 직렬화한다. (2026-11-15 WB04-E537)
const encoded = (text) => Buffer.from(text, 'latin1').toString('hex');

// 입력 열과 외부 GUI 조회만 공급하고 필터·정렬·배치 계산은 제품이 수행하는 독립 검사 장면이다. kind: 'Storm' | 'World' | 'Side'. (2026-11-15 WB04-E537)
class KnowledgeScene {
    // 원본 입력의 DWORD 비트와 필터/정렬 타입 필드만 준비한다.
    constructor(values, kind) {
        this.kind = kind;
        this.input = values.split(',').map(number);
        assert.equal(this.input.length, 10);
        this.nodes = [];
        this.marker = 0xa5a5a5a5;
        this.argument = new Uint8Array(80).fill(0xa5);
        this.trace = '';
        // 네이티브 독립 번역 입력과 원본 strlen 이 읽는 실제 마나 바이트이다.
        this.names = ['Unused', 'Wind Cannon', 'Long-name-here', 'Sun', 'Same', 'same', 'Wide Name Here', 'Priest', 'Temple', 'Zero Mana', 'Group10', 'BadTheme'];
        this.mana = ['', '', 'w', 'WW', 'w.r', 'r', 'ww', '', '', '', '', ''];
        // 실제 정렬/사제 제외 입력이다.
        const flags = [0, 0, 0x10000, 0, 0x800, 0x800, 0x40000, 0x200000, 0, 0, 0, 0];
        this.types = Array.from({ length: 12 }, (_, index) => ({
            theme: kind === 'Storm' ? (index === 11 ? 4 : index % 4) : (index === 11 ? 8 : 4 + (index % 4)),
            group: index === 0 || index === 10 ? 10 : 0,
            flags2: flags[index],
        }));
        this.state = createKnowledgeMenuState({ width: this.input[5] | 0, height: this.input[6] | 0, iconWidth: this.input[7] | 0, iconHeight: this.input[8] | 0, language: this.input[4], templeProduct: 8 });
    }

    // 외부 효과 하나의 실제 인자만 기록한다.
    add(value) { this.trace = this.trace ? `${this.trace};${value}` : value; }

    // 실제 지식 창 함수가 호출할 GUI/내용물/번역 경계이다.
    hooks() {
        const input = this.input;
        return {
            blocked: () => input[0] !== 0,
            cancel: () => this.add('Cancel'),
            currentTechnology: () => 9,
            owns: (parent, type) => { assert.equal(parent, 9); return (input[2] & (1 << type)) !== 0; },
            mana: (type) => this.mana[type],
            description: (type) => (input[9] ? (type % 2 ? 'Same' : 'same') : this.names[type]),
            inProduction: () => false,
            excluded: (type) => (input[3] & (1 << type)) !== 0,
            translate: (text) => text,
            begin: () => {
                const index = this.nodes.length;
                this.nodes.push({ flags: 0x4001, scale: 1, baseWidth: 60, width: 60, baseHeight: 20, height: 20, targetWidth: 52.5, x: 0, y: 0, menuFlags: 0 });
                this.add(`V,${index}`);
                return index + 1;
            },
            node: (token) => this.nodes[token - 1],
            title: (text) => this.add(`T,${encoded(text)}`),
            separator: () => this.add('X'),
            item: (text, code, parameter, enabled) => {
                // 원본 정적 themeHelp 주소 0050f050 의 의미만 시험에서 정규화한다. 제품은 별도 수명 토큰이다.
                const value = typeof parameter === 'number' ? parameter : 5304400;
                if (typeof parameter === 'string') assert.equal(parameter, 'themeHelp');
                this.add(`I,${encoded(text)},${code},${value},1,${enabled ? 1 : 0},0`);
            },
            attach: (x, y) => {
                const node = this.nodes[this.nodes.length - 1];
                node.x = Math.fround(x);
                node.y = Math.fround(y);
                this.add(`A,${this.nodes.length - 1},${x >>> 0},${y >>> 0}`);
            },
            push: (token) => this.add(`P,${token - 1}`),
            pop: (token) => this.add(`Q,${token - 1}`),
            width: (token, value) => { const node = this.nodes[token - 1]; node.baseWidth = node.width = value; },
            height: (token, value) => { const node = this.nodes[token - 1]; node.baseHeight = node.height = value; },
            position: (token) => {
                this.add(`B,${token - 1}`);
                const node = this.nodes[token - 1];
                return { x: Math.trunc(node.x), y: Math.trunc(node.y) };
            },
            mark: (value) => { this.marker = value >>> 0; this.argument.fill(0); },
        };
    }

    // 원본이 남긴 같은 필드/표식/호출 순서만 출력한다.
    run() {
        const hooks = this.hooks();
        const tokens = this.kind === 'Side' ? showKnowledgeSideMenu(this.state, hooks)
            : (this.kind === 'World' ? showWorldKnowledgeMenu : showKnowledgeMenu)(this.types.slice(0, this.input[1]), this.state, hooks);
        assert.equal(tokens.length, this.nodes.length);
        // 단정도 값은 소수 문자열로 바꾸지 않고 전체 DWORD 를 비교한다.
        const fields = this.nodes.map((node) => [node.flags, floatToBits(node.baseWidth), floatToBits(node.baseHeight), floatToBits(node.width), floatToBits(node.height),
            floatToBits(node.x), floatToBits(node.y), floatToBits(node.targetWidth), node.menuFlags].join(',')).join('~');
        return `${fields || '-'}|${this.marker}|${Buffer.from(this.argument).toString('hex')}|${this.trace || '-'}`;
    }
}

test('전체 지식 창(Storm): 원본 10.78 25관측의 노드 단정도 비트·표식·80BYTE 인자·호출 순서가 같다', { skip: !stormRows && '원본 fixture 없음' }, () => {
    assert.equal(stormRows.length, 25);
    let count = 0;
    // 개별 행은 새 GUI/타입/표식 입력으로 실행한다.
    for (const row of stormRows) {
        assert.equal(row.length, 2);
        assert.equal(new KnowledgeScene(row[0], 'Storm').run(), row[1], `행 ${count}: ${row[0]}`);
        count += 1;
    }
    assert.equal(count, 25);
});

test('전체 지식 창(World 카드·선택 메뉴): 원본 10.78 50관측이 같다', { skip: !worldRows && '원본 fixture 없음' }, () => {
    assert.equal(worldRows.length, 50);
    let world = 0;
    let side = 0;
    // 입력과 외부 조회만 공급하며 출력 기대는 fixture 만 읽는다.
    for (const row of worldRows) {
        assert.equal(row.length, 3);
        assert.ok(row[0] === 'World' || row[0] === 'Side', row[0]);
        assert.equal(new KnowledgeScene(row[1], row[0] === 'Side' ? 'Side' : 'World').run(), row[2], `${row[0]}: ${row[1]}`);
        if (row[0] === 'Side') side += 1; else world += 1;
    }
    assert.equal(world, 25);
    assert.equal(side, 25);
});

test('전체 지식 창 계약: 이름표·차단·미연결 효과·문자열/행 수 범위·NUL 뒤 문자열', () => {
    // 명령 번호와 이름표(ASCII 대소문자 무시, 별칭 없음).
    assert.equal(SHOW_TECHNOLOGY_COMMAND, 0x020000a0);
    assert.equal(SHOW_TEC_SIDE_MENU_COMMAND, 0x02000132);
    assert.equal(SHOW_TEC_MENU2_COMMAND, 0x02000133);
    assert.equal(CHEAT_ALL_TECHNOLOGY_COMMAND, 0x02000028);
    assert.equal(knowledgeScriptCode('sHoWtEcHnOlOgY'), SHOW_TECHNOLOGY_COMMAND);
    assert.equal(knowledgeScriptCode('CheatAllTechnology'), CHEAT_ALL_TECHNOLOGY_COMMAND);
    assert.equal(knowledgeScriptCode('SHOWTECSIDEMENU'), SHOW_TEC_SIDE_MENU_COMMAND);
    assert.equal(knowledgeScriptCode('showtecmenu2'), SHOW_TEC_MENU2_COMMAND);
    assert.equal(knowledgeScriptCode('GetTechnology'), 0x0200009f);
    assert.equal(knowledgeScriptCode('ShowKnowledge'), 0xffffffff);
    // 메타맵 차단이면 취소·조회·생성 어느 것도 하지 않고 빈 목록을 돌려준다.
    const calls = [];
    const blocked = { blocked: () => true, cancel: () => calls.push('cancel') };
    assert.deepEqual(showKnowledgeMenu([], createKnowledgeMenuState(), blocked), []);
    assert.deepEqual(showWorldKnowledgeMenu([], createKnowledgeMenuState(), blocked), []);
    assert.deepEqual(showKnowledgeSideMenu(createKnowledgeMenuState(), blocked), []);
    assert.deepEqual(calls, []);
    // 선택된 효과가 없으면 성공으로 대체하지 않는다(차단 조회 자체가 없어도 거부).
    assert.throws(() => showKnowledgeMenu([], createKnowledgeMenuState(), {}), /연결 누락/);
    assert.throws(() => showKnowledgeSideMenu(createKnowledgeMenuState(), { blocked: () => false }), /연결 누락/);
    // 원본 행 112BYTE 의 문자열 범위(마나 9·설명 81)와 머리 포함 100개 메뉴 범위를 넘으면 거부하고, 첫 NUL 뒤는 길이에 넣지 않는다.
    const types = [{ theme: 0, group: 0, flags2: 0 }];
    const base = { blocked: () => false, cancel() {}, currentTechnology: () => 1, owns: () => true, inProduction: () => false };
    assert.throws(() => showKnowledgeMenu(types, createKnowledgeMenuState({ templeProduct: 99 }), { ...base, mana: () => '0123456789', description: () => 'a' }), RangeError);
    assert.throws(() => showKnowledgeMenu(types, createKnowledgeMenuState({ templeProduct: 99 }), { ...base, mana: () => '', description: () => 'x'.repeat(82) }), RangeError);
    const hooks = { ...base, mana: () => '012345678\0tail-ignored', description: () => `${'x'.repeat(81)}\0ignored`, excluded: () => true, translate: (text) => text, begin: () => { throw new Error('범위 통과 확인'); } };
    assert.throws(() => showKnowledgeMenu(types, createKnowledgeMenuState({ templeProduct: 99 }), hooks), /범위 통과 확인/);
    const many = Array.from({ length: 97 }, () => ({ theme: 0, group: 0, flags2: 0 }));
    assert.throws(() => showKnowledgeMenu(many, createKnowledgeMenuState({ templeProduct: 999 }), { ...base, mana: () => '', description: () => 'a' }), RangeError);
});

test('전체 지식 창 계약: 설명 줄 나눔 경계(길이 8 이하·위치 1은 나누지 않음, 마지막 공백/하이픈부터 한 번만)와 팝업 표식의 중간 상태', () => {
    // 한 카드(타입 1)의 설명만 바꿔 두 설명 줄을 읽는다. 머리 4행 뒤 첫 카드의 item 은 번호 행, 설명 둘째 행 순서이다.
    const linesFor = (description) => {
        const scene = new KnowledgeScene('0,2,2,0,1,640,480,65,60,0', 'Storm');
        scene.types = [{ theme: 0, group: 10, flags2: 0 }, { theme: 0, group: 0, flags2: 0 }];
        scene.names = ['', description];
        scene.mana = ['', ''];
        scene.run();
        const items = scene.trace.split(';').filter((entry) => entry.startsWith('I,')).map((entry) => Buffer.from(entry.split(',')[1], 'hex').toString('latin1'));
        // 머리 4개(각 3행) 뒤의 카드 행: 번호, 첫 줄, 둘째 줄.
        return items.slice(12);
    };
    assert.deepEqual(linesFor('ab cd-ef'), ['~[I1.#]', '~5ab cd-ef', '']);         // 길이 8 은 나누지 않는다.
    assert.deepEqual(linesFor('ab cd-efg'), ['~[I1.#]', '~5ab cd', '~5efg']);      // 길이 9 는 마지막 공백/하이픈(여기서는 하이픈)부터 한 번만 나눈다.
    assert.deepEqual(linesFor('a bcdefghij'), ['~[I1.#]', '~5a bcdefghij', '']);   // 위치 1 의 공백은 나누지 않는다.
    assert.deepEqual(linesFor('ab cd ef gh'), ['~[I1.#]', '~5ab cd ef', '~5gh']);  // 여러 공백이면 가장 뒤의 것이다.
    // 모든 카드를 만드는 동안 팝업 비트는 꺼져 있고 위치 조회 순회 중에도 꺼져 있으며, 표식 직전에 모두 켜진다.
    const scene = new KnowledgeScene('0,12,0,0,1,640,480,65,60,0', 'Storm');
    const hooks = scene.hooks();
    const seen = [];
    const original = { position: hooks.position, mark: hooks.mark };
    hooks.position = (token) => { seen.push(['position', scene.nodes.map((node) => node.flags & 0x4000)]); return original.position(token); };
    hooks.mark = (value) => { seen.push(['mark', scene.nodes.map((node) => node.flags & 0x4000)]); original.mark(value); };
    showKnowledgeMenu(scene.types.slice(0, 12), scene.state, hooks);
    const positions = seen.filter(([name]) => name === 'position');
    assert.equal(positions.length, scene.nodes.length);
    assert.ok(positions.every(([, flags]) => flags.every((flag) => flag === 0)));
    const marks = seen.filter(([name]) => name === 'mark');
    assert.equal(marks.length, 1);
    assert.ok(marks[0][1].every((flag) => flag === 0x4000));
});
