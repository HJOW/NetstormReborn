// 객체 메뉴 raw-object-menu.js(RawObjectMenu.open·openDamageable·addSalvage·bind*)를 원본 10.78 관측과 cpppj 시험 계약으로 대조한다(WB04-E541).
//  - object-menu-x86.tsv 의 originals(10.78) 행: base(Open)·damage(OpenDamageable)·salvage(AddSalvage) 각각 입력 14 DWORD(소유자·로컬·raw 타입·명시 타입·모드·금지·flags2·비용·환불·개수·항목 수·HP·변경)와
//    콜백이 슬롯/전역을 바꾸는 17가지 변경 조합에서 전체 효과 호출 순서. (CD 10.72 판본 행은 웹 범위 밖이라 건너뛴다.)
//  - 계약: 도달한 미연결 효과(helpWord NULL·Player 메뉴 없음) 거부, 현재 캐시(helpWordRead)가 값형 fixture 보다 우선, 컨트롤/설정/Identity 연결의 위임.
// 실제 게임·브라우저 실행이 아니다. (2026-11-15 WB04-E541)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { bitsToFloat, readRows } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { RawObjectMenu, bindObjectMenuControls, bindObjectMenuIdentity, createObjectMenuState } from '../www/core/raw-object-menu.js';

// 원본 관측 fixture 이다. 없으면 해당 검사를 건너뛴다. (2026-11-15 WB04-E541)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'object-menu-x86.tsv'));
// 종류마다 원본 10.78(originals) 판본 입력은 239개이다(두 판본 합쳐 478개 중 CD 10.72 판본 239개는 웹 범위 밖). (2026-11-15 WB04-E541)
const ROWS_PER_KIND = 239;
// fixture 14개 입력 열의 원본 의미이다. 부호 있는 환불도 저장은 DWORD 비트이다. (2026-11-15 WB04-E541)
const Input = Object.freeze({ Owner: 0, Local: 1, RawType: 2, PassedType: 3, Modes: 4, Deny: 5, CurrentFlags: 6, PassedFlags: 7, Cost: 8, Refund: 9, Count: 10, Items: 11, Hp: 12, Mutation: 13 });
// 스타일 문자열을 구분자와 충돌하지 않는 원문 바이트(16진)로 기록한다. (2026-11-15 WB04-E541)
const textHex = (text) => Buffer.from(text, 'latin1').toString('hex');

// 실제 원문 fixture 의 효과만 수집하며 예상 메뉴 분기는 만들지 않는 독립 장면이다. (2026-11-15 WB04-E541)
class ObjectScene {
    // 원문 입력 50번 슬롯만 실제 클라이언트 영역에서 확보한다.
    constructor(row) {
        this.pool = new SidPool(24000, { server: true });
        this.types = Array.from({ length: 188 }, () => ({ flags2: 0, cost: 1 }));
        this.input = row.slice(2, 16).map((text) => Number(text) >>> 0);
        this.trace = '';
        this.resolvedOwner = 0;
        const input = this.input;
        // 5..50번은 실제 클라이언트 슬롯이다.
        for (let number = 5; number <= 50; number += 1) assert.equal(this.pool.allocate(2), number);
        const raw = this.pool.allocated(50);
        raw[10] = input[Input.RawType];
        raw[34] = input[Input.Owner];
        // 변경 콜백이 가리킬 TYPE 도 실제 표 안에 준비한다.
        this.types[input[Input.RawType]].flags2 = input[Input.CurrentFlags];
        this.types[input[Input.RawType]].cost = bitsToFloat(input[Input.Cost]);
        this.types[input[Input.PassedType]].flags2 = input[Input.PassedFlags];
        this.state = createObjectMenuState({
            localOwner: input[Input.Local], editor: (input[Input.Modes] & 1) !== 0, special: (input[Input.Modes] & 2) !== 0, chargeBlocked: (input[Input.Modes] & 4) !== 0,
            denySalvage: input[Input.Deny], helpWord: 'AboutType',
        });
        this.hooks = {
            hasHitPoints: (sid) => { this.add(`H,${sid}`); return input[Input.Hp] !== 0; },
            count: (sid) => { this.add(`L,${sid}`); if (input[Input.Mutation] & 1) this.pool.allocated(sid)[10] = 122; return input[Input.Count]; },
            salvageRefund: (sid, type, count) => { this.add(`R,${sid},${type},${count}`); if (input[Input.Mutation] & 2) this.state.denySalvage = 1; return input[Input.Refund] | 0; },
            format: (key, args) => {
                this.add(`T,${textHex(key)}`);
                if (input[Input.Mutation] & 16) this.types[input[Input.PassedType]].flags2 = 0x200;
                let text = key;
                if (args.length) {
                    assert.equal(args.length, 1);
                    assert.ok(text.includes('%d'));
                    text = text.replace('%d', String(args[0]));
                }
                return text;
            },
            itemCount: () => { this.add('Q'); if (input[Input.Mutation] & 32) this.types[input[Input.PassedType]].flags2 = 0x4000; return input[Input.Items]; },
            separator: () => { this.add('X'); if (input[Input.Mutation] & 4) this.state.denySalvage = 0; },
            setConfig: (key, value) => { this.add(`C,${textHex(key)},${textHex(value)}`); if (input[Input.Mutation] & 8) this.pool.allocated(50)[10] = 122; },
            item: (label, command, parameter) => this.add(`I,${textHex(label)},${command},${typeof parameter === 'number' ? `N:${parameter}` : `S:${textHex(parameter)}`}`),
            ownerSubscriber: (owner) => { this.add(`N,${owner}`); this.resolvedOwner = owner; return 0x12200100; },
            // 이전 fixture 의 하위 경계는 고정 opaque 값을 PlayerN label 로 관찰했다. 문자열 API 의 근거로 사용하지 않는다.
            playerMenu: (subscriber, first, second) => { assert.equal(subscriber, 0x12200100); this.add(`P,${textHex(`Player${this.resolvedOwner}`)},${first ? 1 : 0},${second ? 1 : 0}`); return 123; },
            submenu: (label, menu) => this.add(`S,${textHex(label)},${menu}`),
        };
    }

    // 외부 효과의 순서와 빈 결과를 원문 형식에 맞춘다.
    add(text) { this.trace = this.trace ? `${this.trace};${text}` : text; }

    // 생성 중 실제 메뉴 효과를 실행하지 않는다.
    menu() { return new RawObjectMenu(this.pool, this.types, this.state, this.hooks); }
}

// 같은 몸체의 10.78(originals) 입력을 대조한다. (2026-11-15 WB04-E541)
function compare(kind) {
    let count = 0;
    // 기대값은 실제 PE 에서 수집한 전체 효과 문자열이다.
    for (const row of rows) {
        if (row[1] !== kind || row[0] !== 'originals') continue;
        assert.equal(row.length, 17);
        const scene = new ObjectScene(row);
        const menu = scene.menu();
        if (kind === 'base') menu.open(50);
        else if (kind === 'damage') menu.openDamageable(50);
        else menu.addSalvage(50, scene.input[Input.PassedType], scene.input[Input.Count]);
        assert.equal(scene.trace || '-', row[16], `${kind} ${count}: ${row.slice(2, 16).join(',')}`);
        count += 1;
    }
    return count;
}

test('객체 메뉴 base: 원본 10.78 입력의 치트·도움말·SetOwner·Player 하위 메뉴 전체 효과 순서가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    assert.equal(compare('base'), ROWS_PER_KIND);
});

test('객체 메뉴 damage: 원본 10.78 입력의 가상 개수 snapshot·회수 뒤 기본 메뉴 연결이 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    assert.equal(compare('damage'), ROWS_PER_KIND);
});

test('객체 메뉴 salvage: 원본 10.78 입력의 회수/금지·음수 환불·현재 설정·콜백 재진입·특수 생략이 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    assert.equal(compare('salvage'), ROWS_PER_KIND);
});

test('객체 메뉴 계약: 도달한 미연결 효과를 빈 성공으로 처리하지 않고 helpWord 현재 캐시가 값형 입력보다 우선한다', () => {
    const base = rows?.find((row) => row[0] === 'originals' && row[1] === 'base') ?? ['originals', 'base', '1', '1', '158', '123', '0', '0', '0', '0', '1065353216', '45', '3', '1', '1', '0', '-'];
    // helpWord 가 NULL 이면 어떤 항목도 만들기 전에 거부한다.
    const scene = new ObjectScene(base);
    scene.state.helpWord = null;
    assert.throws(() => scene.menu().open(50), /helpWord NULL/);
    assert.equal(scene.trace, '');
    // Player 하위 메뉴 장치가 없으면 소유자 항목에서 거부하고 하위 메뉴를 만들지 않는다.
    scene.state.helpWord = 'AboutType';
    delete scene.hooks.playerMenu;
    assert.throws(() => scene.menu().open(50), /미연결/);
    assert.ok(!scene.trace.includes('P,'));
    // 실제 Client 의 현재 캐시는 값형 fixture 보다 우선하고 읽는 시점마다 최신 값/NULL 을 구별한다.
    const cached = new ObjectScene(base);
    let cache = 'Info';
    cached.state.helpWordRead = () => cache;
    cached.menu().open(50);
    assert.ok(cached.trace.includes(textHex('Info')) && !cached.trace.includes(textHex('AboutType')));
    const nullCache = new ObjectScene(base);
    cache = null;
    nullCache.state.helpWordRead = () => cache;
    assert.throws(() => nullCache.menu().open(50), /helpWord NULL/);
    assert.equal(nullCache.trace, '');
    // 비영 빈 문자열은 NULL 이 아니라 정상 항목 문자열이다.
    const empty = new ObjectScene(base);
    empty.state.helpWordRead = () => '';
    empty.menu().open(50);
    assert.ok(empty.trace.includes('I,,'));
    // 풀 판본/TYPE 표 크기가 맞지 않으면 만들 수 없다.
    assert.throws(() => new RawObjectMenu(scene.pool, scene.types.slice(1), scene.state, scene.hooks), TypeError);
});

test('객체 메뉴 계약: 번역 NUL·255바이트 한도와 실제 현재 전역 getter 가 값형 입력보다 우선한다', () => {
    const salvageRow = ['originals', 'salvage', '1', '1', '158', '123', '0', '0', '0', '0', '1065353216', '45', '3', '1', '1', '0', '-'];
    const run = (adjust) => {
        const scene = new ObjectScene(salvageRow);
        adjust(scene);
        scene.menu().addSalvage(50, 123, 3);
        return scene;
    };
    // 번역 훅이 NUL 뒤에 남긴 바이트는 버리고 256바이트 이상은 원본 스택 버퍼 한도 초과로 거부한다.
    const nul = run((scene) => { scene.hooks.format = () => 'AB\0CD'; });
    assert.ok(nul.trace.includes(`I,${textHex('AB')},`), nul.trace);
    assert.doesNotThrow(() => run((scene) => { scene.hooks.format = () => 'x'.repeat(255); }));
    assert.throws(() => run((scene) => { scene.hooks.format = () => 'x'.repeat(256); }), RangeError);
    // 회수 금지는 값형 입력이 0 이어도 현재 전역 getter 가 1 이면 DenySalvage 안내 항목이 된다(getter 가 우선).
    const denied = run((scene) => { scene.state.denySalvage = 0; scene.state.denySalvageRead = () => 1; });
    assert.ok(denied.trace.includes(`S:${textHex('DenySalvage')}`), denied.trace);
    const allowed = run((scene) => { scene.state.denySalvage = 1; scene.state.denySalvageRead = () => 0; });
    assert.ok(!allowed.trace.includes(`S:${textHex('DenySalvage')}`) && allowed.trace.includes(`N:50`), allowed.trace);
    // 로컬 소유자 getter 도 값형 입력보다 우선하며 소유자가 다르면 항목을 만들지 않는다.
    const other = run((scene) => { scene.state.localOwner = 1; scene.state.localOwnerRead = () => 2; });
    assert.equal(other.trace, '');
    // 편집 getter 가 참이면 값형 입력과 무관하게 Destroy 항목만 편집용으로 만든다.
    const editor = new ObjectScene(salvageRow);
    editor.state.editor = false;
    editor.state.editorRead = () => true;
    editor.menu().open(50);
    assert.ok(editor.trace.startsWith(`I,${textHex('Destroy')},`), editor.trace);
    // 특수/차단 getter 두 개가 모두 참이면 치트 항목 셋이 생긴다.
    const cheat = new ObjectScene(salvageRow);
    cheat.state.specialRead = () => true;
    cheat.state.chargeBlockedRead = () => true;
    cheat.menu().open(50);
    assert.ok(cheat.trace.includes(textHex('CHEAT Destroy')) && cheat.trace.includes(textHex('CHEAT Make Yours')), cheat.trace);
});

test('객체 메뉴 연결: 컨트롤·설정·Identity 에 위임하고 정수 인자와 정적 문자열 인자를 구별한다', () => {
    const calls = [];
    const record = (name, result) => (...args) => { calls.push([name, ...args]); return result; };
    const controls = { addSeparator: record('addSeparator'), addText: record('addText'), addSubmenu: record('addSubmenu') };
    const menus = { itemCount: record('itemCount', 3) };
    const config = { set: record('set') };
    const hooks = {};
    bindObjectMenuControls(hooks, controls, menus, config, (text) => { calls.push(['encode', text]); return 4000 + text.length; });
    assert.equal(hooks.itemCount(), 3);
    hooks.separator();
    hooks.item('Destroy', 0x03000015, 50);
    hooks.item('Set Owner', 0x02000095, 'SetOwner');
    hooks.submenu('Player', 123);
    hooks.setConfig('global.salvageSid', '50');
    assert.deepEqual(calls, [['itemCount'], ['addSeparator'], ['addText', 'Destroy', 0x03000015, 50, true, true, false], ['encode', 'SetOwner'], ['addText', 'Set Owner', 0x02000095, 4008, true, true, false],
        ['addSubmenu', 'Player', 123, true, false], ['set', 'global.salvageSid', '50']]);
    // 정적 문자열 표가 없으면 문자열 인자 항목을 만들 수 없다.
    const bare = {};
    bindObjectMenuControls(bare, controls, menus, config, undefined);
    assert.throws(() => bare.item('x', 1, 'SetOwner'), /미연결/);
    assert.doesNotThrow(() => bare.item('x', 1, 7));
    const identityHooks = {};
    bindObjectMenuIdentity(identityHooks, { subscriberForOwner: (owner) => 0x12200000 + owner });
    assert.equal(identityHooks.ownerSubscriber(5), 0x12200005);
});
