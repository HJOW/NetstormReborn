// Player 메뉴 raw-player-menu.js(RawPlayerMenu.open·textParameter·formatPlayerMenuText·bind*)를 원본 10.78 관측 292입력과 cpppj 시험 계약으로 대조한다(WB04-E544).
//  - player-menu-x86.tsv 292행: Player 부모 전체(대상·모드·flags·사제·동맹·숨김·허용·SP·무시/귓속말·게임 수·주소·변경)의 반환값·정적 208바이트(asksend 두 버퍼)·모든 조회/GUI 효과 순서.
//  - 계약: 같은 정적 토큰이 갱신된 asksend 문자열을 읽음, 필요한 조회 누락 거부, 고정 서식(%s/%d/%%)·256바이트 한도, 컨트롤/객체 메뉴 연결.
// 실제 게임·브라우저 실행이 아니다. (2026-11-15 WB04-E544)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { bitsToFloat, readRows } from './raw-support.mjs';
import { RawPlayerIdentity } from '../www/core/raw-player-identity.js';
import { PlayerMenuAssertion, RawPlayerMenu, bindObjectPlayerMenu, bindPlayerMenuControls, createPlayerMenuState, formatPlayerMenuText } from '../www/core/raw-player-menu.js';

// 원본 관측 fixture 이다. 없으면 해당 검사를 건너뛴다. (2026-11-15 WB04-E544)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'player-menu-x86.tsv'))?.filter((row) => row[0] !== 'target') ?? null;
// 원문 13개 독립 입력 열이다. (2026-11-15 WB04-E544)
const Input = Object.freeze({ Target: 0, Modes: 1, Flags: 2, Priest: 3, Allied: 4, Hide: 5, Allow: 6, Money: 7, Ignored: 8, Whisper: 9, Games: 10, Address: 11, Mutation: 12 });
// 원본 문자열의 바이트 16진이다. (2026-11-15 WB04-E544)
const textHex = (value) => Buffer.from(value, 'latin1').toString('hex');
// 낮은 바이트부터 정해진 폭만 쓴다. (2026-11-15 WB04-E544)
function put(bytes, offset, value, width = 4) {
    for (let index = 0; index < width; index += 1) bytes[offset + index] = (value >>> (index * 8)) & 0xff;
}
// 원문 문자열/NUL 만 쓰며 뒤의 표식을 지우지 않는다. (2026-11-15 WB04-E544)
function put0(bytes, offset, value) {
    for (let index = 0; index < value.length; index += 1) bytes[offset + index] = value.charCodeAt(index);
    bytes[offset + value.length] = 0;
}

// 실제 기록/현재 getter·GUI 경계만 조립하며 예상 메뉴는 계산하지 않는 장면이다. (2026-11-15 WB04-E544)
class PlayerScene {
    // 고정 원문 입력을 채우며 원본 반환/메뉴 기대값은 사용하지 않는다.
    constructor(row) {
        this.input = Array.from({ length: 13 }, (_, index) => Number(row[index]) >>> 0);
        this.players = new Uint8Array(1620);
        this.meta = new Uint8Array(40 * 460);
        this.chat = new Uint8Array(4800);
        this.alliances = new Uint32Array(81);
        this.hidden = new Uint8Array(9);
        this.priests = new Uint32Array(9);
        this.taken = 0;
        this.trace = '';
        const input = this.input;
        this.owner = input[Input.Target] === 0 ? 1 : 2;
        this.subscriber = input[Input.Target] === 0 ? 0xfedcba98 : input[Input.Target] === 1 ? 0x80000002 : 0xdeadbeef;
        const owner = this.owner;
        put(this.players, 180 + 44, 0xfedcba98); put0(this.players, 180 + 12, 'Local1'); this.players[180 + 80] = 0x23;
        put(this.players, 2 * 180 + 44, 0x80000002); put0(this.players, 2 * 180 + 12, 'Alpha'); put(this.players, 8 * 180 + 44, 0xfffffff0); put0(this.players, 8 * 180 + 12, 'LAST');
        put(this.players, 3 * 180 + 44, 0x80000003); put0(this.players, 3 * 180 + 12, 'Buddy'); this.players[3 * 180 + 80] = 0x23;
        this.players[owner * 180 + 80] = input[Input.Flags] & 0xff;
        this.priests[owner] = input[Input.Priest];
        put(this.players, 180 + 84, (input[Input.Allied] & 16) ? (1 << owner) >>> 0 : 0);
        // 숨김 표와 서로 다른 두 방향의 동맹을 독립 입력으로 채운다.
        for (let index = 0; index < 9; index += 1) this.hidden[index] = (input[Input.Hide] & (1 << index)) ? 1 : 0;
        this.alliances[owner * 9 + 3] = (input[Input.Allied] & 1) !== 0 ? 1 : 0;
        this.alliances[3 * 9 + owner] = (input[Input.Allied] & 2) !== 0 ? 1 : 0;
        this.alliances[owner * 9 + 8] = (input[Input.Allied] & 4) !== 0 ? 1 : 0;
        this.alliances[8 * 9 + owner] = (input[Input.Allied] & 8) !== 0 ? 1 : 0;
        // metadata 1/2 와 실제 필드 배치의 독립 버전/주소 입력이다.
        for (let index = 1; index <= 2; index += 1) {
            const entry = this.meta.subarray(index * 460, index * 460 + 460);
            put(entry, 0, index);
            put(entry, 4, index === 1 ? 0xfedcba98 : 0x80000002);
            put0(entry, 38, index === 1 ? 'LocalUser' : 'Meta2');
            entry[100] = 1;
            put(entry, 176, 1, 2);
            put(entry, 178, 78, 2);
            put(entry, 84, input[Input.Games], 2);
            put0(entry, 416, 'udp,127.0.0.1:6800');
        }
        if (input[Input.Address]) put0(this.players, owner * 180 + 48, 'zlocal,127.0.0.1:6800');
        const reads = {
            localOwner: () => 1, subscriber: () => 0xfedcba98, address: () => 0xc0a8005e, name: () => 'LocalUser',
            battle: () => (input[Input.Modes] & 1) !== 0, editor: () => (input[Input.Modes] & 8) !== 0, metamap: () => (input[Input.Modes] & 4) !== 0, unknownName: () => 'SOMEONE',
        };
        this.identity = new RawPlayerIdentity(this.players, this.meta, this.chat, reads);
        this.state = createPlayerMenuState();
        this.state.sendMission.fill(0xa5);
        this.state.sendLobby.fill(0x5a);
        put0(this.state.sendMission, 0, 'OLD_MISSION');
        put0(this.state.sendLobby, 0, 'OLD_LOBBY');
        this.hooks = {
            mission: () => (input[Input.Modes] & 2) !== 0,
            fort: () => (input[Input.Modes] & 16) !== 0,
            allowSend: () => (input[Input.Allow] & 1) !== 0,
            allowOffer: () => (input[Input.Allow] & 2) !== 0,
            allowBreak: () => (input[Input.Allow] & 4) !== 0,
            assertion: (kind) => this.add(`!,${kind === PlayerMenuAssertion.MetamapNumber ? 1 : 0}`),
            parseAddress: (value, port) => this.add(`A,${textHex(value)},${port}`),
            reliability: (index) => { this.add(`R,${index}`); if (input[Input.Mutation] & 8) input[Input.Modes] &= ~4; return 90 + index; },
            ignored: (value) => { this.add(`G,${value}`); return input[Input.Ignored] !== 0; },
            whispered: (value) => { this.add(`W,${value}`); return input[Input.Whisper] & 0xff; },
            money: (value) => {
                this.add(`$,${value}`);
                if (input[Input.Mutation] & 4) { this.players[this.owner * 180 + 80] = 0; input[Input.Allow] &= ~1; }
                return bitsToFloat(input[Input.Money]);
            },
            translate: (key) => {
                this.add(`T,${textHex(key)}`);
                if ((input[Input.Mutation] & 1) && key === 'Allied with ~i%s~.') put0(this.players, 3 * 180 + 12, 'Changed');
                if ((input[Input.Mutation] & 2) && key === 'Version: ~i%d.%d~.\n') put(this.meta, this.owner * 460 + 176, 9, 2);
                return key;
            },
            print: (pattern, args) => formatPlayerMenuText(pattern, args),
            beginFancy: () => this.add('F'),
            beginVertical: () => this.add('V'),
            separator: () => this.add('X'),
            title: (value) => this.add(`D,${textHex(value)}`),
            header: (value) => this.add(`H,${textHex(value)}`),
            item: (value, command, parameter, include, enabled) => this.add(`I,${textHex(value)},${command},${parameter},${include},${enabled}`),
            checked: (checked, value, command, parameter) => this.add(`C,${checked},${textHex(value)},${command},${parameter}`),
            takeMenu: () => { this.taken += 1; const token = 100 + this.taken; this.add(`K,${token}`); return token; },
            submenu: (value, child) => this.add(`S,${textHex(value)},${child}`),
        };
    }

    // 실제 효과를 발생 순서대로 연결한다.
    add(value) { this.trace = this.trace ? `${this.trace};${value}` : value; }

    // 같은 실제 Identity/표/현재 전역/버퍼를 빌린다.
    menu() { return new RawPlayerMenu(this.identity, this.alliances, this.hidden, this.priests, this.state, this.hooks); }

    // 두 정적 버퍼 전체를 원문과 같은 바이트로 읽는다.
    buffers() { return Buffer.from(this.state.sendMission.subarray(0, 104)).toString('hex') + Buffer.from(this.state.sendLobby.subarray(0, 104)).toString('hex'); }
}

test('Player 메뉴: 원본 10.78 292입력의 반환·정적 208바이트·모든 조회/GUI 효과 순서가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    let count = 0;
    // 기대값은 실제 기계어에서 얻었으며 생성/분기 계산을 복제하지 않는다.
    for (const row of rows) {
        assert.equal(row.length, 16);
        const scene = new PlayerScene(row);
        const result = scene.menu().open(scene.subscriber);
        const label = `행 ${count}: target ${row[0]} modes ${row[1]} mutation ${row[12]}`;
        assert.equal(String(result), row[13], label);
        assert.equal(scene.buffers(), row[14], label);
        assert.equal(scene.trace || '-', row[15], label);
        count += 1;
    }
    assert.equal(count, 292);
});

test('Player 메뉴 계약: 같은 정적 토큰은 갱신된 asksend 문자열을 읽고 누락된 효과는 성공으로 바뀌지 않는다', () => {
    const base = rows?.[0] ?? ['1', '3', '0', '0', '0', '0', '1', '1120403456', '0', '0', '5', '0', '0', '', '', ''];
    const scene = new PlayerScene(base);
    const menu = scene.menu();
    assert.notEqual(menu.open(scene.subscriber), 0);
    const token = scene.state.missionTextToken;
    assert.equal(menu.textParameter(token), 'asksend(2)');
    put(scene.players, 2 * 180 + 44, 0);
    put(scene.players, 8 * 180 + 44, scene.subscriber);
    menu.open(scene.subscriber);
    assert.equal(menu.textParameter(token), 'asksend(8)');
    assert.throws(() => menu.textParameter(0x1234), RangeError);
    // 송금 행의 SP 조회 장치가 없으면 거부한다.
    const missing = new PlayerScene(base);
    delete missing.hooks.money;
    assert.throws(() => missing.menu().open(missing.subscriber), /미연결/);
    // 저장소 크기를 잘못 빌리면 만들 수 없다.
    assert.throws(() => new RawPlayerMenu(scene.identity, new Uint32Array(80), scene.hidden, scene.priests, scene.state, scene.hooks), TypeError);
});

test('Player 메뉴 계약: 후보 동맹의 자기 제외·귓속말 BYTE·마지막 금액 1·사제 번호 범위·출력 NUL/한도·주소 조회 호출', () => {
    const base = rows?.[0] ?? ['1', '3', '0', '0', '0', '0', '1', '1120403456', '0', '0', '5', '0', '0', '', '', ''];
    // 전투 모드(1)·비미션(0)·허용 전부·SP 충분한 대상 2 의 기본 장면이다(행 형식은 fixture 와 같다).
    const battle = ['1', '1', '35', '0', '0', '0', '7', '1148846080', '0', '0', '5', '0', '0', '', '', ''];
    // 후보→자기 동맹은 제외한다(원본은 candidate != owner 를 먼저 확인).
    const selfAlly = new PlayerScene(battle);
    selfAlly.alliances[selfAlly.owner * 9 + selfAlly.owner] = 1;
    selfAlly.menu().open(selfAlly.subscriber);
    assert.ok(!selfAlly.trace.includes(textHex('Allied with ~i%s~.')) && selfAlly.trace.includes(textHex('Allied with nobody.\n')), selfAlly.trace);
    // 귓속말 조회는 하위 BYTE 만 쓴다: 0x100 은 0, 0x1ff 는 255(체크 인자)로 읽는다.
    const whisper = (value) => {
        const scene = new PlayerScene(['1', '1', '35', '0', '0', '0', '7', '1148846080', '0', '0', '5', '0', '0', '', '', '']);
        scene.hooks.whispered = () => value;
        scene.menu().open(scene.subscriber);
        return scene.trace;
    };
    assert.ok(whisper(0x100).includes(`C,0,${textHex('Whisper To')},${0x02000027},`), whisper(0x100));
    assert.ok(whisper(0x1ff).includes(`C,255,${textHex('Whisper To')},${0x02000101},`), whisper(0x1ff));
    // 마지막(All) 송금 금액은 1 이다: SP 1.5 는 지불 가능(접두 ~r 없음), 0.5 는 불가능.
    const label = (sp) => {
        const scene = new PlayerScene(['1', '1', '35', '0', '0', '0', '7', '0', '0', '0', '5', '0', '0', '', '', '']);
        scene.hooks.money = () => sp;
        scene.menu().open(scene.subscriber);
        return scene.trace;
    };
    assert.ok(label(1.5).includes(textHex('Send  All~[IfortGump.3]')), label(1.5));
    assert.ok(label(0.5).includes(textHex('Send ~r All~[IfortGump.3]')), label(0.5));
    assert.ok(!label(1.5).includes(textHex('Send ~r All~[IfortGump.3]')), label(1.5));
    // 사제 번호 범위: 1..8 만 유효하고 밖은 진단 뒤 사망/포획으로 취급한다(8 은 유효).
    const range = new PlayerScene(base);
    const menu = range.menu();
    range.players[8 * 180 + 80] = 0x22;
    range.priests[8] = 0;
    assert.equal(menu.dead(8), false);
    assert.equal(menu.captured(8), false);
    assert.equal(range.trace, '');
    assert.equal(menu.dead(0), true);
    assert.equal(menu.captured(9), true);
    assert.equal(range.trace, '!,0;!,0');
    // print 훅이 NUL 뒤에 남긴 바이트는 버리고 256바이트 이상은 거부한다.
    const nul = new PlayerScene(battle);
    nul.hooks.print = (pattern, args) => (pattern === 'Priest: ~i%s~.\n' ? 'AB\0CD' : formatPlayerMenuText(pattern, args));
    nul.menu().open(nul.subscriber);
    assert.ok(nul.trace.includes(`H,${textHex('AB')};`), nul.trace);
    const long = new PlayerScene(battle);
    long.hooks.print = (pattern, args) => (pattern === 'Priest: ~i%s~.\n' ? 'x'.repeat(256) : formatPlayerMenuText(pattern, args));
    assert.throws(() => long.menu().open(long.subscriber), RangeError);
    // 비미션 전투의 주소 조회는 정확히 한 번 실행되고, 미션에서 남의 subscriber 면 실행하지 않는다.
    const spy = (row) => {
        const scene = new PlayerScene(row);
        let calls = 0;
        const original = scene.identity.addressForSubscriber.bind(scene.identity);
        scene.identity.addressForSubscriber = (subscriber) => { calls += 1; return original(subscriber); };
        scene.menu().open(scene.subscriber);
        return calls;
    };
    assert.equal(spy(battle), 1);
    assert.equal(spy(['1', '3', '35', '0', '0', '0', '7', '1148846080', '0', '0', '5', '0', '0', '', '', '']), 0);
});

test('Player 메뉴 서식·연결 계약: 고정 %s/%d/%% 와 256바이트 한도, 컨트롤/객체 메뉴 위임', () => {
    assert.equal(formatPlayerMenuText('Player %s 100%% %d', ['Bob', -5]), 'Player Bob 100% -5');
    assert.equal(formatPlayerMenuText('x', []), 'x');
    assert.throws(() => formatPlayerMenuText('%s', []), /인자 부족/);
    assert.throws(() => formatPlayerMenuText('x', ['extra']), RangeError);
    assert.throws(() => formatPlayerMenuText('%f', [1]), /미복원 서식/);
    assert.throws(() => formatPlayerMenuText('%d', ['text']), TypeError);
    assert.throws(() => formatPlayerMenuText('x'.repeat(256), []), RangeError);
    assert.equal(formatPlayerMenuText('x'.repeat(255), []).length, 255);
    const calls = [];
    const record = (name, result) => (...args) => { calls.push([name, ...args]); return result; };
    const widgets = { beginFancy: record('beginFancy'), beginVertical: record('beginVertical') };
    const controls = { addSeparator: record('addSeparator'), addTitle: record('addTitle'), addHeader: record('addHeader'), addText: record('addText'), addChecked: record('addChecked'), addSubmenu: record('addSubmenu') };
    const menus = { takeCurrent: record('takeCurrent', 77) };
    const hooks = {};
    bindPlayerMenuControls(hooks, widgets, controls, menus);
    hooks.beginFancy();
    hooks.beginVertical();
    hooks.separator();
    hooks.title('T');
    hooks.header('H');
    assert.equal(hooks.takeMenu(), 77);
    hooks.item('I', 5, 6, 0, 1);
    hooks.checked(1, 'C', 7, 8);
    hooks.submenu('S', 9);
    assert.deepEqual(calls, [['beginFancy'], ['beginVertical'], ['addSeparator'], ['addTitle', 'T'], ['addHeader', 'H'], ['takeCurrent'], ['addText', 'I', 5, 6, false, true, false],
        ['addChecked', true, 'C', 7, 8, true, true], ['addSubmenu', 'S', 9, true, false]]);
    // 객체 메뉴의 Player 하위 메뉴는 subscriber 하나만 쓴다(원본 호출자의 여분 두 인자는 버린다).
    const objectHooks = {};
    const opened = [];
    bindObjectPlayerMenu(objectHooks, { open: (subscriber) => { opened.push(subscriber); return 123; } });
    assert.equal(objectHooks.playerMenu(0x12200100, true, false), 123);
    assert.deepEqual(opened, [0x12200100]);
});
