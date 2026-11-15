// 게임 메뉴 일반 명령 game-menu-actions.js(GameMenuActions)를 원본 10.78 관측 3종과 cpppj 시험 계약으로 대조한다(WB04-E535).
//  - game-menu-actions-x86.tsv 610행: 독립 현재 전역/Player/Form/SID/큐 장치에서 명령(QuitApp·RequestQuitApp·볼륨·Pause·ViewHome·HideBuildings·ReloadTheams 등)의
//    종료 종류·최종 전역 16개·장치 바이트·큐 커서/물리 큐·본체 효과 순서(부분 중단 26행·QuitApp 즉시 반환 16행·앞단 소비 270행 포함)를 대조한다.
//  - cheat-money-x86.tsv 144행: AddSp(0x02000061)의 현재 관문 읽기 순서·signed→float 비트·중단 시 부분 효과.
//  - full-deck-input-x86.tsv 384행: GetFullDeck 의 Editor/mission 관문과 release, 하위 getFullDeck 호출 60회.
//  - 계약: 대상이 아닌 명령/release/미연결 효과 거부, GetFullDeck 단락 읽기, PlaySound 의 text→sound 고정 인자.
// 실제 게임·브라우저 실행이 아니다. (2026-11-15 WB04-E535)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { floatToBits, readRows } from './raw-support.mjs';
import { GameMenuActionCommand, GameMenuActions, GameMenuReloadList } from '../www/core/game-menu-actions.js';

// 원본 관측 fixture 경로이다. 없으면 해당 검사를 건너뛴다. (2026-11-15 WB04-E535)
const fixture = (name) => readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', name));
const menuRows = fixture('game-menu-actions-x86.tsv');
const moneyRows = fixture('cheat-money-x86.tsv');
const deckRows = fixture('full-deck-input-x86.tsv');

// 두 제어어에서 독립 관측한 고정 자료 수와 장치 주소이다(cpppj 시험과 같다). (2026-11-15 WB04-E535)
const MENU_ROWS = 610;
const MENU_COLUMNS = 21;
const ARENA = 0x01010000;
// fixture 의 전역 순서(현재 전역 주소 16개)이다. (2026-11-15 WB04-E535)
const GLOBALS = [0x5c85a4, 0x594fb8, 0x594fc8, 0x594fa4, 0x595344, 0x540c70, 0x59a8ac, 0x59a8b0,
    0x55b4b0, 0x565dd4, 0x55a714, 0x5e18a4, 0x59534c, 0x5c8464, 0x55a70c, 0x5e189c];
// 보호 PE 에서 읽은 서로 다른 불변 명령 ID 표이다. (2026-11-15 WB04-E535)
const COMMANDS = new Map([
    [GameMenuActionCommand.PlaySound, 0x0200009a], [GameMenuActionCommand.AddSp, 0x02000061],
    [GameMenuActionCommand.QuitApp, 0x02000001], [GameMenuActionCommand.RequestQuitApp, 0x02000094],
    [GameMenuActionCommand.SoundVolume, 0x020000e6], [GameMenuActionCommand.MusicVolume, 0x020000e7],
    [GameMenuActionCommand.Pause, 0x020000cf], [GameMenuActionCommand.ViewHome, 0x020000ae],
    [GameMenuActionCommand.ViewYourPriest, 0x020000b8], [GameMenuActionCommand.HideBuildings, 0x020000f1],
    [GameMenuActionCommand.ReloadTheams, 0x02000123], [GameMenuActionCommand.GetFullDeck, 0x020000cd],
]);
// 명시 효과 중단을 정상 메뉴 반환과 구별한다. (2026-11-15 WB04-E535)
class ExplicitStop extends Error {}

// 부호 입력도 원문 하위 DWORD 로 읽는다. (2026-11-15 WB04-E535)
const number = (text) => Number(text) >>> 0;
// 16진 문자열을 원시 바이트로 바꾼다. (2026-11-15 WB04-E535)
const bytesOf = (text) => Uint8Array.from(text.match(/../g) ?? [], (pair) => parseInt(pair, 16));
// 원시 바이트를 fixture 표기(소문자 16진)로 관측한다. (2026-11-15 WB04-E535)
const hexOf = (bytes) => Buffer.from(bytes).toString('hex');
// 비정렬 DWORD 쓰기/읽기이다. (2026-11-15 WB04-E535)
const store = (bytes, offset, value) => {
    if (offset > bytes.length || bytes.length - offset < 4) throw new RangeError('메뉴 장치 쓰기 범위');
    // 낮은 바이트부터 4바이트를 쓴다.
    for (let index = 0; index < 4; index += 1) bytes[offset + index] = (value >>> (8 * index)) & 0xff;
};
const load = (bytes, offset) => {
    if (offset > bytes.length || bytes.length - offset < 4) throw new RangeError('메뉴 장치 읽기 범위');
    return (bytes[offset] | (bytes[offset + 1] << 8) | (bytes[offset + 2] << 16) | (bytes[offset + 3] << 24)) >>> 0;
};

// 앞 10열의 자료만 소유하고 하위 장치 효과를 실제 입력대로 공급하는 현재 장면이다. (2026-11-15 WB04-E535)
class MenuScene {
    // 관측 뒤 열은 입력으로 쓰지 않고 독립 현재 상태를 준비한다.
    constructor(row) {
        this.devices = bytesOf(row[5]);
        this.queue = new Uint8Array(480);
        this.read = number(row[6]);
        this.write = (this.read + number(row[7])) % 40;
        this.calls = new Map();
        this.effects = [];
        this.stops = new Set();
        this.trace = '';
        const values = row[4].split(':');
        assert.equal(values.length, GLOBALS.length, '메뉴 전역 열 폭');
        this.globals = Uint32Array.from(values, number);
        // 비활성 슬롯까지 독립 원문 입력 표식을 설치한다.
        for (let index = 0; index < 40; index += 1) {
            store(this.queue, index * 12, 0x2000300 + index);
            store(this.queue, index * 12 + 4, 100 + index);
            store(this.queue, index * 12 + 8, 200 + index);
        }
        // 입력 변경만 명시 형식으로 읽는다.
        if (row[8] !== '-') for (const item of row[8].split(';')) {
            const parts = item.split('/');
            assert.equal(parts.length, 3, '메뉴 효과 열 폭');
            this.effects.push(parts);
        }
        // 명시 부분 중단 호출 번호이다.
        if (row[9] !== '-') for (const item of row[9].split(';')) this.stops.add(item);
    }

    // 실제 주소에 대응하는 현재 전역 칸의 번호이다.
    slot(address) {
        const index = GLOBALS.indexOf(address);
        if (index < 0) throw new RangeError('메뉴 전역 주소');
        return index;
    }

    // 현재 포인터가 가리키는 장치 메모리의 남은 폭이다.
    memory(address) {
        if (address < ARENA || address - ARENA >= this.devices.length) throw new RangeError('메뉴 장치 주소');
        return this.devices.subarray(address - ARENA);
    }

    // 같은 경계에 배정한 변경과 중단을 호출 순서대로 공급한다.
    event(name, values = [], body = true) {
        if (body) this.trace = `${this.trace}${this.trace ? ';' : ''}${[name, ...values].join(':')}`;
        const count = (this.calls.get(name) ?? 0) + 1;
        this.calls.set(name, count);
        const trigger = `${name}${count}`;
        // 경계 복귀 전 같은 실제 자료를 변경한다.
        for (const effect of this.effects) {
            if (effect[0] !== trigger) continue;
            const address = number(effect[1]);
            const value = number(effect[2]);
            if (address >= ARENA && address + 4 <= ARENA + this.devices.length) store(this.devices, address - ARENA, value);
            else this.globals[this.slot(address)] = value;
        }
        if (this.stops.has(trigger)) throw new ExplicitStop(trigger);
    }

    // 실제 현재 SID/목록/자료를 조회하는 메뉴 외부 장치이다.
    hooks() {
        const g = this.globals;
        const at = (address) => this.slot(address);
        return {
            command: (name) => COMMANDS.get(name),
            queue: (command, first, second) => {
                this.event('queue', [String(command), String(first), String(second)]);
                // 큐는 본체 밖 명시 장치이며 포화 시 물리 슬롯을 쓰고 write 만 복구한다.
                store(this.queue, this.write * 12, command);
                store(this.queue, this.write * 12 + 4, first);
                store(this.queue, this.write * 12 + 8, second);
                const next = (this.write + 1) % 40;
                if (next !== this.read) this.write = next;
            },
            tell: (text) => this.event('tell', [text]),
            config: (text) => this.event('config', [text]),
            sound: (name, a, b, c, d, e) => this.event('sound', [name, String(a), String(b), String(c), String(d), String(e)]),
            paused: () => {
                this.event('paused', [String(g[at(0x55b4b0)])]);
                return (g[at(0x55b4b0)] | 0) > 0;
            },
            status: (a, b, c, d, text) => this.event('status', [String(a), String(b), String(c), String(d), text]),
            cancelCursor: () => this.event('cancel'),
            pauseClock: () => { g[at(0x55b4b0)] += 1; this.event('pause'); },
            resumeClock: () => { if (g[at(0x55b4b0)]) g[at(0x55b4b0)] -= 1; this.event('resume'); },
            muteSound: () => this.event('mute'),
            resumeSound: () => this.event('unmute'),
            player: (owner) => {
                const address = (g[at(0x59534c)] + owner * 0xb4) >>> 0;
                return address ? this.memory(address) : new Uint8Array(0);
            },
            centerObject: (sid) => this.event('center', [String(sid)]),
            viewPriest: (owner) => this.event('priest', [String(owner)]),
            reloadCount: (list) => g[at(list === GameMenuReloadList.First ? 0x55a714 : 0x5e18a4)] | 0,
            reloadSid: (list, index) => load(this.memory(g[at(list === GameMenuReloadList.First ? 0x55a70c : 0x5e189c)]), index * 4),
            form: (sid) => this.memory((g[at(0x5c8464)] + sid * 50) >>> 0),
            reloadAt: (x, y, zero) => this.event('reload', [String(x), String(y), String(zero)]),
            assertion: (line) => this.event('assert', [String(line)]),
        };
    }

    // 제품/장치가 공유하는 최종 전역을 원문과 같은 순서로 관측한다.
    globalsText() {
        return Array.from(this.globals, (value) => String(value)).join(':');
    }
}

test('게임 메뉴 일반 명령: 원본 10.78 610입력의 현재 전역·장치 바이트·큐·본체 효과 순서·부분 중단이 같다', { skip: !menuRows && '원본 fixture 없음' }, () => {
    let rows = 0;
    let partial = 0;
    let quit = 0;
    let gateSkipped = 0;
    // 관측 열을 장치 초기화/분기 선택에 쓰지 않고 입력 열만으로 재생한다.
    for (const row of menuRows) {
        assert.equal(row.length, MENU_COLUMNS, `열 폭 ${row[0]}`);
        rows += 1;
        const scene = new MenuScene(row);
        const fields = row[1].split(':');
        assert.equal(fields.length, 3, '메뉴 사건 폭');
        const [event, first, second] = fields.map(number);
        const box = (index) => ({ get value() { return scene.globals[index]; }, set value(next) { scene.globals[index] = next >>> 0; } });
        const actions = new GameMenuActions({ read: (address) => scene.globals[scene.slot(address)], hideBuildings: box(scene.slot(0x59a8ac)), redraw: box(scene.slot(0x59a8b0)) }, scene.hooks());
        let completed = true;
        let result = null;
        try {
            scene.event('guide', [], false);
            scene.event('mission', [], false);
            if (number(row[2]) === 0) {
                scene.event('gump', [], false);
                if (number(row[3]) === 0) result = actions.handle(event, first, second);
                else gateSkipped += 1;
            } else gateSkipped += 1;
        } catch (failure) {
            if (!(failure instanceof ExplicitStop)) throw failure;
            completed = false;
        }
        if (!completed) partial += 1;
        if (result === 1) quit += 1;
        const label = `행 ${rows}: ${row[0]}`;
        assert.equal(completed, row[10] === '1', label);
        assert.equal(result === 1, row[11] === '1', label);
        assert.equal(scene.globalsText(), row[12], label);
        assert.equal(hexOf(scene.devices), row[13], label);
        assert.equal(scene.read, number(row[14]), label);
        assert.equal(scene.write, number(row[15]), label);
        assert.equal(hexOf(scene.queue), row[16], label);
        assert.equal(scene.trace === '' ? '-' : scene.trace, row[17], label);
        assert.ok(number(row[20]) > 0, label);
        // 외부 tail/기계 스택은 제품 subset 의 반환값이 아니므로 실제 관측 자료의 프레임 종료만 검사한다.
        if (completed) assert.equal(Number(row[19]), row[11] === '1' ? 12296 : 0, label);
        // 부모 MenuJustClosed/비대상은 미처리이다.
        const code = (event & 0x03ffffff) >>> 0;
        if (code === 0x020000be || code === 0x020003fe) assert.equal(result, null, label);
    }
    assert.equal(rows, MENU_ROWS);
    assert.equal(partial, 26);
    assert.equal(quit, 16);
    assert.equal(gateSkipped, 270);
});

test('SP 메뉴 명령: 원본 10.78 144입력의 관문 읽기 순서·signed→float 비트·부분 효과가 같다', { skip: !moneyRows && '원본 fixture 없음' }, () => {
    let count = 0;
    // 각 사례마다 독립 상태를 만들어 앞선 효과를 섞지 않는다.
    for (const f of moneyRows) {
        assert.equal(f.length, 7);
        const probe = { editor: number(f[0]), mission: number(f[1]), stop: number(f[3]), events: 0, hide: { value: 0 }, redraw: { value: 0 }, added: null, trace: '' };
        // 효과를 적용하기 전 진입을 기록하고 지정 번호에서 중단한다.
        const event = (text) => {
            probe.trace = `${probe.trace}${probe.trace ? ';' : ''}${text}`;
            probe.events += 1;
            if (probe.events === probe.stop) throw new ExplicitStop(text);
        };
        const actions = new GameMenuActions({
            read: (address) => {
                if (address === 0x5c85a4) { event(`E,${probe.editor}`); return probe.editor; }
                if (address === 0x594fa4) { event(`M,${probe.mission}`); return probe.mission; }
                throw new Error('검사 메뉴의 예상밖 전역');
            },
            hideBuildings: probe.hide,
            redraw: probe.redraw,
        }, {
            command: (name) => COMMANDS.get(name),
            addLocalSp: (value) => { const bits = floatToBits(value); event(`A,${bits}`); probe.added = bits; },
        });
        let completed = true;
        try {
            const result = actions.handle(0x02000061, 0, number(f[2]));
            assert.equal(result, 0);
        } catch (failure) {
            if (!(failure instanceof ExplicitStop)) throw failure;
            completed = false;
        }
        const label = `행 ${count}: ${f.join(',')}`;
        assert.equal(completed, f[4] === '1', label);
        assert.equal(probe.trace, f[5], label);
        assert.equal(probe.added !== null, f[6] !== '-', label);
        if (probe.added !== null) assert.equal(probe.added, Number(f[6]), label);
        assert.ok(probe.hide.value === 0 && probe.redraw.value === 0, label);
        count += 1;
    }
    assert.equal(count, 144);
});

test('전체 Deck 명령: 원본 10.78 384입력의 Editor/mission 관문·release·하위 호출 60회가 같다', { skip: !deckRows && '원본 fixture 없음' }, () => {
    let rows = 0;
    let calls = 0;
    // 원문 입력 열과 실제 호출 관측을 읽고 기대 분기를 다시 계산하지 않는다.
    for (const row of deckRows) {
        assert.equal(row.length, 22);
        const globals = row[5].split(':');
        const event = row[2].split(':');
        assert.equal(globals.length, 16);
        assert.equal(event.length, 3);
        const hide = { value: 0 };
        const redraw = { value: 0 };
        let currentCalls = 0;
        const state = {
            read: (address) => {
                if (address === 0x5c85a4) return number(globals[0]);
                if (address === 0x594fa4) return number(globals[3]);
                throw new Error('전체Deck 입력의 다른 전역 조회');
            },
            hideBuildings: hide,
            redraw,
        };
        const actions = new GameMenuActions(state, { command: (name) => COMMANDS.get(name), getFullDeck: () => { currentCalls += 1; } });
        // 앞 Guide/Gump 소비는 기존 전체 부모가 소유하며 여기서 중복 실행하지 않는다.
        if (number(row[3]) === 0 && number(row[4]) === 0) assert.equal(actions.handle(number(event[0]), number(event[1]), number(event[2])), 0, `행 ${rows + 1}`);
        const selected = row[19].split(';').includes('fullDeck');
        assert.equal(currentCalls, selected ? 1 : 0, `행 ${rows + 1}: ${row[1]}`);
        assert.ok(hide.value === 0 && redraw.value === 0 && row[18] === '-');
        calls += currentCalls;
        rows += 1;
    }
    assert.equal(rows, 384);
    assert.equal(calls, 60);
});

test('게임 메뉴 계약: 비대상·release 는 효과 장치를 요구하지 않고 선택한 효과 누락은 성공으로 대체하지 않는다', () => {
    const hide = { value: 0 };
    const redraw = { value: 0 };
    const hooks = { command: (name) => COMMANDS.get(name) };
    const actions = new GameMenuActions({ read: () => 0, hideBuildings: hide, redraw }, hooks);
    // MenuJustClosed(0x020000be)와 비대상(0x020003fe)은 부모 Gump 몫이라 미처리이다.
    assert.equal(actions.handle(0x020000be, 1, 2), null);
    assert.equal(actions.handle(0x020003fe, 1, 2), null);
    // release(상위 비트)는 대상 명령이어도 효과 없이 0 이다.
    assert.equal(actions.handle(0x82000001, 0, 99), 0);
    // QuitApp 의 sound 효과가 미연결이면 미연결 Error 이며 범위 오류(RangeError)로 위장하지 않는다.
    assert.throws(() => actions.handle(0x02000001, 0, 99), (failure) => failure instanceof Error && !(failure instanceof RangeError) && /미연결/.test(failure.message));
    assert.ok(hide.value === 0 && redraw.value === 0);
    // 생성자는 전역 getter 와 명령 조회가 없으면 거부한다.
    assert.throws(() => new GameMenuActions({ hideBuildings: hide, redraw }, hooks), /미연결/);
    assert.throws(() => new GameMenuActions({ read: () => 0, hideBuildings: hide, redraw }, {}), /미연결/);
});

test('게임 메뉴 계약: 현재 전역 읽기 순서와 단락(Pause 는 mission→Editor, ViewHome 은 소유자 두 번 재조회, RequestQuit 은 Editor→Abandon 쌍)', () => {
    const hide = { value: 0 };
    const redraw = { value: 0 };
    const reads = [];
    const values = new Map();
    const calls = [];
    const make = (extra) => new GameMenuActions({ read: (address) => { reads.push(address); return values.get(address) ?? 0; }, hideBuildings: hide, redraw },
        { command: (name) => COMMANDS.get(name), assertion: (line) => calls.push(`assert:${line}`), tell: (text) => calls.push(`tell:${text}`), queue: (...args) => calls.push(`queue:${args.join(',')}`), ...extra });
    // Pause: mission(594fa4) 가 참이면 Editor(5c85a4)는 읽지 않고, 거짓이면 이어서 읽으며 둘 다 거짓이면 효과가 없다.
    values.set(0x594fa4, 1);
    make({ paused: () => false, status: () => {}, cancelCursor: () => {}, pauseClock: () => {}, muteSound: () => {} }).handle(0x020000cf, 0, 0);
    assert.deepEqual(reads, [0x594fa4]);
    reads.length = 0;
    values.clear();
    make({}).handle(0x020000cf, 0, 0);
    assert.deepEqual(reads, [0x594fa4, 0x5c85a4]);
    // ViewHome: 595344 가 거짓이면 594fc8 을 읽지 않고, 소유자 0 은 진단 뒤 계속하며 같은 전역을 다시 읽어 Player 를 요청한다.
    reads.length = 0;
    make({}).handle(0x020000ae, 0, 0);
    assert.deepEqual(reads, [0x595344]);
    reads.length = 0;
    values.set(0x595344, 1);
    const owners = [];
    make({ player: (owner) => { owners.push(owner); return new Uint8Array(0); } }).handle(0x020000ae, 0, 0);
    assert.deepEqual(reads, [0x595344, 0x594fc8, 0x540c70, 0x540c70]);
    assert.deepEqual(owners, [0]);
    assert.deepEqual(calls, ['assert:1895']);
    // RequestQuitApp: Editor 가 거짓이고 Abandon 쌍 중 하나라도 참이면 QuitApp 을 99 로 예약하며, 쌍은 594fb8 뒤 594fc8 순서로 단락 평가한다.
    reads.length = 0;
    calls.length = 0;
    values.clear();
    values.set(0x594fb8, 1);
    make({}).handle(0x02000094, 0, 0);
    assert.deepEqual(reads, [0x5c85a4, 0x594fb8]);
    assert.deepEqual(calls, ['queue:33554433,0,99']);
    reads.length = 0;
    calls.length = 0;
    values.clear();
    make({}).handle(0x02000094, 0, 0);
    assert.deepEqual(reads, [0x5c85a4, 0x594fb8, 0x594fc8]);
    assert.deepEqual(calls, ['tell:QuitAbandon']);
});

test('게임 메뉴 계약: GetFullDeck 은 Editor 읽기에서 단락하고 release 는 읽지 않으며 PlaySound 는 text 뒤 고정 0 다섯 개로 재생한다', () => {
    const hide = { value: 0 };
    const redraw = { value: 0 };
    const reads = [];
    const stop = new ExplicitStop('fullDeck');
    const actions = new GameMenuActions({ read: (address) => { reads.push(address); return 0x80000000; }, hideBuildings: hide, redraw },
        { command: (name) => COMMANDS.get(name), getFullDeck: () => { throw stop; } });
    // Editor 전역이 참이면 mission 전역은 읽지 않고 곧바로 하위 효과로 간다.
    assert.throws(() => actions.handle(0x020000cd, 0, 0), (failure) => failure === stop);
    assert.deepEqual(reads, [0x5c85a4]);
    reads.length = 0;
    assert.equal(actions.handle(0x820000cd, 0, 0), 0);
    assert.deepEqual(reads, []);
    // PlaySound 는 모드/소유자 관문 없이 문자열 해석 뒤 이름과 고정 0 다섯 개를 재생한다(음수 입력은 하위 32비트로 해석).
    const sounds = [];
    const texts = [];
    const player = new GameMenuActions({ read: () => { throw new Error('PlaySound 는 전역 관문이 없다'); }, hideBuildings: hide, redraw }, {
        command: (name) => COMMANDS.get(name),
        text: (address) => { texts.push(address); return 'bridge.wav'; },
        sound: (...args) => sounds.push(args),
    });
    assert.equal(player.handle(0x0200009a, 0, -1), 0);
    assert.deepEqual(texts, [0xffffffff]);
    assert.deepEqual(sounds, [['bridge.wav', 0, 0, 0, 0, 0]]);
    // text 훅 누락은 소리를 재생하기 전에 거부한다.
    const missing = new GameMenuActions({ read: () => 0, hideBuildings: hide, redraw }, { command: (name) => COMMANDS.get(name), sound: (...args) => sounds.push(args) });
    assert.throws(() => missing.handle(0x0200009a, 0, 5), /미연결/);
    assert.equal(sounds.length, 1);
});
