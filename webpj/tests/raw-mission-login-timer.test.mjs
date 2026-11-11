// 로그인 타이머/집계(cpppj `o/RawMissionLoginTimer`, 10.78 004b67c0·004180a0·004b61f0·004b6780·00490970~00490cf0)의 웹 대응 `core/raw-mission-login-timer.js` 를 검사한다(4-C, WB04-E318).
//  - 원본 독립 관측 `mission-login-timer-x86.tsv` 1984행(집계 C·만료 E·Begin B·Abort A·Run R/S × Player 패턴 × 모드 × 연결/호출 도중 변경)의 결과·전체 상태 직렬화(double 비트 포함)·현재 모드 읽기와 외부 효과 순서를 재생한다.
//  - 손 시험(cpppj `MissionLoginTimerTests.cpp` 이식 + 진행 문구): 미ready missionPost 종료가 Begin 만 생략, inactive 는 어떤 경계도 읽지 않음, 누락 하위에서 앞선 쓰기만 남김, 일반 초기화 미복원 거부, Abort/완료의 현재 참조 갱신 순서, 진행 HTML 의 명단·남은 시간·서식 검증.
// 실제 게임·브라우저 실행이 아니다. (2026-11-11 WB04-E318)
import assert from 'node:assert/strict';
import path from 'node:path';
import test from 'node:test';
import { MISSION_LOGIN_TIMER_LOG, MissionLoginPlayerCounts, RawMissionLoginTimer } from '../www/core/raw-mission-login-timer.js';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';

// 원본 관측 파일이다. 파일이 없으면 재생 검사를 건너뛴다. (2026-11-11 WB04-E318)
const ROWS = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'mission-login-timer-x86.tsv'));

// native 의 명시 flags 입력 16개와 현재 모드 실제 읽기 trace 순서의 이름이다. (2026-11-11 WB04-E318)
const LOGIN_FLAGS = [0, 1, 2, 3, 0x40, 0x41, 0x42, 0x43, 0x100, 0x101, 0x102, 0x103, 0x140, 0x142, 0x143, 0xffffffff];
const MODE_NAMES = ['saved', 'server', 'mission', 'post', 'geysers', 'asserts'];

// 소문자 16진 문자열과 double 의 8바이트 메모리(little endian) 직렬화/복원이다. (2026-11-11 WB04-E318)
const hex = (bytes) => Array.from(bytes, (byte) => byte.toString(16).padStart(2, '0')).join('');
function doubleHex(value) {
    const view = new DataView(new ArrayBuffer(8));
    view.setFloat64(0, value, true);
    return hex(new Uint8Array(view.buffer));
}
function readDouble(text) {
    if (text.length !== 16) throw new RangeError('로그인 double 입력 크기 오류');
    const view = new DataView(new ArrayBuffer(8));
    for (let index = 0; index < 8; index += 1) view.setUint8(index, Number.parseInt(text.slice(index * 2, index * 2 + 2), 16));
    return view.getFloat64(0, true);
}
const textHex = (text) => hex(Array.from(text, (char) => char.charCodeAt(0)));
function put(bytes, offset, value) {
    for (let index = 0; index < 4; index += 1) bytes[offset + index] = (value >>> (8 * index)) & 0xff;
}

// 실제 기록/현재 모드/효과의 독립 입력이며 타이머의 성공/대기 결과를 미리 계산하지 않는다. (2026-11-11 WB04-E318)
class LoginTimerScene {
    // 입력 "14개 숫자|end 8바이트|wall 8바이트": 숫자는 pattern/configTimeout/6모드와 begin/connection/trigger/action/value/budget 이다. (2026-11-11 WB04-E318)
    constructor(input) {
        const fields = input.split('|');
        const words = fields[0].split(',');
        if (fields.length !== 3 || words.length !== 14) throw new RangeError('로그인 타이머 입력 크기 오류');
        this.numbers = words.map((word) => Number(word) >>> 0);
        this.players = new Uint8Array(9 * 0xb4);
        this.end = readDouble(fields[1]);
        this.wall = readDouble(fields[2]);
        this.timeout = 0x76543210;
        this.begin = this.numbers[6];
        this.scriptBreak = 0xabcdef01;
        this.phase = 0x11223344;
        this.clock = 0x12345678;
        this.budget = this.numbers[13];
        this.trace = [];
        const n = this.numbers;
        this.modes = [n[2] !== 0, n[3] !== 0, n[4] !== 0, n[5] !== 0, n[7] !== 0, n[8] !== 0];
        // 집계가 읽지 않는 전체 Player stale BYTE 도 native 와 같은 독립 입력으로 유지한다.
        for (let index = 0; index < this.players.length; index += 1) this.players[index] = (index * 37 + n[0] * 11) & 0xff;
        // Player 0 을 포함한 실제 9기록에 명시 subscriber/flags 를 넣는다.
        for (let owner = 0; owner < 9; owner += 1) this.setPlayer(owner, 0, 0);
        this.setPlayer(0, 0xdeadbeef, 0xffffffff);
        const pattern = n[0];
        if (pattern >= 16) {
            const ordinal = pattern - 16;
            const subs = [0, 7, 0xffffffff];
            this.setPlayer(Math.floor(ordinal / 48) + 1, subs[Math.floor(ordinal / 16) % 3], LOGIN_FLAGS[ordinal % 16]);
        } else if (pattern === 1) this.setPlayer(1, 7, 3);
        else if (pattern === 2) this.setPlayer(1, 7, 0x43);
        else if (pattern === 3) { this.setPlayer(1, 7, 3); this.setPlayer(2, 8, 3); }
        else if (pattern === 4) { this.setPlayer(1, 7, 0x43); this.setPlayer(2, 8, 0x102); }
        else if (pattern === 5) this.setPlayer(1, 7, 2);
        else if (pattern === 6) this.setPlayer(1, 7, 0x100);
        else if (pattern >= 7 && pattern <= 9) {
            // 전체 8명이 연결/ready 인 입력과 subscriber 0 의 stale flags 입력을 구분한다.
            for (let owner = 1; owner < 9; owner += 1) this.setPlayer(owner, pattern === 9 ? 0 : owner, pattern === 7 ? 0 : pattern === 8 ? 0x43 : 0xffffffff);
        }
    }

    // 원문 위치의 명시 Player 입력만 쓴다. (2026-11-11 WB04-E318)
    setPlayer(owner, subscriber, flags) {
        put(this.players, owner * 0xb4 + 0x2c, subscriber);
        put(this.players, owner * 0xb4 + 0x50, flags);
    }

    // 효과 직전의 실제 쓰기 순서 관찰값이다. (2026-11-11 WB04-E318)
    snapshot() {
        return `${doubleHex(this.end)},${this.begin},${this.scriptBreak},${this.phase},${this.clock}`;
    }

    // 입력에 지정된 외부 효과만 변경하며 제품의 타이머/집계 분기를 대신 계산하지 않는다. (2026-11-11 WB04-E318)
    mutate(trigger) {
        if (trigger !== this.numbers[10] || this.budget === 0) return;
        this.budget -= 1;
        const action = this.numbers[11];
        const value = this.numbers[12];
        if (action === 1) this.modes[1] = !this.modes[1];
        else if (action === 2) this.modes[3] = !this.modes[3];
        else if (action === 3) this.modes[0] = !this.modes[0];
        else if (action === 4) this.modes[2] = !this.modes[2];
        else if (action === 5) this.begin = value;
        else if (action === 6) this.end = value;
        else if (action === 7) put(this.players, 0xb4 + 0x50, 0x43);
        else if (action === 8) this.phase = value;
        else if (action === 9) this.scriptBreak = value;
    }

    // 같은 상태의 입력 변경은 원문 외부 효과 진입 관찰 뒤에만 수행한다. (2026-11-11 WB04-E318)
    event(text, trigger = 0) {
        this.trace.push(`${text},${this.snapshot()}`);
        this.mutate(trigger);
    }

    // native 가 실제 모드 DWORD 를 읽는 각 위치에 현재 bool 값을 공급한다. (2026-11-11 WB04-E318)
    mode(index) {
        this.trace.push(`G,${MODE_NAMES[index]},${this.modes[index] ? 1 : 0}`);
        return this.modes[index];
    }

    // 현재 모드 조회기를 실제 각 소비 위치에 연결한다. (2026-11-11 WB04-E318)
    reads() {
        return {
            savedGame: () => this.mode(0), server: () => this.mode(1), mission: () => this.mode(2), missionPost: () => this.mode(3), initialGeysers: () => this.mode(4), checkingAssertions: () => this.mode(5),
        };
    }

    // 동일 실제 상태의 참조(속성 접근)를 빌린다. (2026-11-11 WB04-E318)
    state() {
        const scene = this;
        return {
            get end() { return scene.end; }, set end(value) { scene.end = value; },
            get timeout() { return scene.timeout; }, set timeout(value) { scene.timeout = value; },
            get begin() { return scene.begin; }, set begin(value) { scene.begin = value; },
            get scriptBreak() { return scene.scriptBreak; }, set scriptBreak(value) { scene.scriptBreak = value; },
            get requestPhase() { return scene.phase; }, set requestPhase(value) { scene.phase = value; },
            get clock() { return scene.clock; }, set clock(value) { scene.clock = value; },
        };
    }

    // 실제 시간/전송/사제/UI 는 명시 입력 경계이며 타이머의 비교/쓰기/반복은 제품이 수행한다. (2026-11-11 WB04-E318)
    hooks() {
        const scene = this;
        return {
            configInt: (key) => { assert.equal(key, 'loginTimeout'); scene.event('Q'); return scene.numbers[1] | 0; },
            wallSeconds: () => { scene.event('W', 10); return scene.wall; },
            clock: () => { scene.event('C', 1); return 0x87654321; },
            connection: (peer) => { assert.equal(peer, 9); scene.event('N,9', 2); return scene.numbers[9] !== 0; },
            remoteStatus: (peer, first, second) => { assert.deepEqual([peer, first, second], [9, 0, 0]); scene.event('R,9,0,0', 3); },
            broadcast: (packet) => { scene.event(`B,${hex(packet)}`, 4); },
            priestRefresh: (owner) => { scene.event(`P,${owner}`, 5); },
            saveReconnect: () => { scene.event('S', 6); },
            closeProgress: () => { scene.event('X', 7); },
            resetCamera: () => { scene.event('V'); },
            abortBattle: () => { scene.event('A', 11); },
            progress: () => { scene.event('U'); },
            serverAssertion: () => { scene.event('J', 9); },
            log: (kind, text) => { scene.event(`L,${kind === MISSION_LOGIN_TIMER_LOG.Status ? 1 : 0},${textHex(text)}`, kind === MISSION_LOGIN_TIMER_LOG.Status ? 8 : 0); },
        };
    }

    // native 가 읽은 모든 타이머/모드 저장소를 동일 순서로 직렬화한다. (2026-11-11 WB04-E318)
    observe() {
        return `${doubleHex(this.end)}|${this.timeout >>> 0},${this.begin},${this.scriptBreak},${this.phase},${this.clock}${this.modes.map((mode) => `,${mode ? 1 : 0}`).join('')}`;
    }
}

// 1984행 재생: 열은 kind/input/result/state/trace 이다. (2026-11-11 WB04-E318)
test('로그인 타이머: 원본 관측 1984행의 결과·상태·읽기/효과 순서', { skip: ROWS === null }, () => {
    let compared = 0;
    // 기대 결과는 보호 PE 의 실제 출력이며 제품 규칙으로 다시 계산하지 않는다.
    for (const row of ROWS) {
        if (row[0] === 'kind') continue;
        assert.equal(row.length, 5);
        const scene = new LoginTimerScene(row[1]);
        const counts = new MissionLoginPlayerCounts(scene.players);
        const timer = new RawMissionLoginTimer(scene.state(), counts, scene.reads(), scene.hooks());
        let result = '-';
        if (row[0] === 'C') {
            const before = Uint8Array.from(scene.players);
            result = `${counts.subscribers()},${counts.participating()},${counts.disconnected()},${counts.ready()},${counts.loggedIn()}`;
            assert.deepEqual(Array.from(scene.players), Array.from(before));
        } else if (row[0] === 'E') result = String(Number(timer.expired()));
        else if (row[0] === 'B') timer.begin();
        else if (row[0] === 'A') timer.abort();
        else if (row[0] === 'R' || row[0] === 'S') timer.run(row[0] === 'S');
        else assert.fail('로그인 타이머 원문 함수 종류 오류');
        assert.equal(result, row[2], `행 ${compared} 결과`);
        assert.equal(scene.observe(), row[3], `행 ${compared} 상태`);
        assert.equal(scene.trace.length ? scene.trace.join(';') : '-', row[4], `행 ${compared} trace`);
        compared += 1;
    }
    assert.equal(compared, 1984);
});

// 실제 missionPost 는 미ready/미만료에서도 Begin 만 생략한 뒤 사제 상태와 로그인 종료를 실행한다. (2026-11-11 WB04-E318)
test('로그인 타이머: 미ready missionPost 는 Begin 없이 종료', () => {
    const scene = new LoginTimerScene('1,30,0,1,1,1,0,0,0,1,0,0,0,1|0000000000002540|0000000000002440');
    const counts = new MissionLoginPlayerCounts(scene.players);
    const timer = new RawMissionLoginTimer(scene.state(), counts, scene.reads(), scene.hooks());
    assert.deepEqual([counts.ready(), counts.loggedIn()], [0, 1]);
    timer.run(false);
    assert.deepEqual([scene.end, scene.begin, scene.scriptBreak], [0, 0, 0]);
    const text = scene.trace.join(';');
    assert.ok(!text.includes('B,') && !text.includes('U,'));
    assert.ok(text.includes('P,1,') && text.includes('P,8,') && text.includes('S,'));
});

// inactive 0 은 현재 모드/시간/전송/진단 등 어떤 외부 경계도 읽지 않으며 누락 효과가 있어도 선택하지 않는다. (2026-11-11 WB04-E318)
test('로그인 타이머: inactive 는 아무것도 읽지 않음', () => {
    const scene = new LoginTimerScene('1,30,0,1,1,1,0,0,0,1,0,0,0,1|0000000000000080|0000000000002440');
    const counts = new MissionLoginPlayerCounts(scene.players);
    const timer = new RawMissionLoginTimer(scene.state(), counts, scene.reads(), {});
    const before = scene.observe();
    timer.run(false);
    assert.deepEqual([scene.trace, scene.observe()], [[], before]);
});

// Begin 송신 누락과 Abort 하위 누락은 실제 앞선 쓰기만 남기고 성공/후속 호출로 진행하지 않는다. (2026-11-11 WB04-E318)
test('로그인 타이머: 선택 소비자 누락은 앞선 쓰기만 남김', () => {
    const scene = new LoginTimerScene('1,30,0,1,1,1,0,0,0,1,0,0,0,1|0000000000002440|0000000000002440');
    const counts = new MissionLoginPlayerCounts(scene.players);
    const begin = new RawMissionLoginTimer(scene.state(), counts, scene.reads(), { ...scene.hooks(), broadcast: undefined });
    assert.throws(() => begin.begin(), /Begin 실제 송신 미연결/);
    assert.deepEqual([scene.end, scene.begin], [0, 0]);
    const abort = new RawMissionLoginTimer(scene.state(), counts, scene.reads(), { ...scene.hooks(), abortBattle: undefined });
    assert.throws(() => abort.abort(), /Abort 실제 상태 정리 미연결/);
    assert.deepEqual([scene.begin, scene.end, scene.scriptBreak, scene.phase], [0, 0, 1, 4]);
    assert.ok(!scene.trace.join(';').includes('X,'));
});

// 일반 초기화 미복원을 실제 미션 시작 성공으로 가장하지 않고 같은 선택 위치에서 명시 거부한다. 집계/구성 검사. (2026-11-11 WB04-E318)
test('로그인 타이머: 일반 초기화 미복원과 구성의 거부', () => {
    const scene = new LoginTimerScene('2,30,0,1,0,1,0,0,0,1,0,0,0,1|0000000000002440|0000000000002440');
    assert.throws(() => new MissionLoginPlayerCounts(new Uint8Array(scene.players.length - 1)), RangeError);
    assert.throws(() => new MissionLoginPlayerCounts([]), RangeError);
    const counts = new MissionLoginPlayerCounts(scene.players);
    const timer = new RawMissionLoginTimer(scene.state(), counts, scene.reads(), scene.hooks());
    assert.throws(() => timer.run(false), /일반 전투 초기화 부모 미연결/);
    assert.deepEqual([scene.begin, scene.end], [1, 0]);
    const text = scene.trace.join(';');
    assert.ok(text.includes('B,016e080000000080,') && !text.includes('P,1,'));
    for (const name of ['savedGame', 'server', 'mission', 'missionPost', 'initialGeysers', 'checkingAssertions']) {
        assert.throws(() => new RawMissionLoginTimer(scene.state(), counts, { ...scene.reads(), [name]: undefined }, scene.hooks()), TypeError, name);
    }
    assert.throws(() => new RawMissionLoginTimer(scene.state(), {}, scene.reads(), scene.hooks()), TypeError);
    assert.throws(() => new RawMissionLoginTimer(null, counts, scene.reads(), scene.hooks()), TypeError);
    // 집계 record 는 같은 물리 기록을 빌리고 범위를 확인한다.
    assert.equal(counts.record(3).buffer, scene.players.buffer);
    assert.throws(() => counts.record(9), RangeError);
});

// Abort 의 1 쓰기 뒤 실제 하위 효과가 같은 값을 바꾸면 뒷꼬리도 현재 값을 보존하고, 완료 0 쓰기는 사제/로그 효과 뒤에 발생하며 뒤 효과가 바꾼 현재 값을 다시 덮지 않는다. (2026-11-11 WB04-E318)
test('로그인 타이머: Abort·완료의 현재 참조 갱신 순서', () => {
    const scene = new LoginTimerScene('1,30,0,1,1,1,0,0,0,1,0,0,0,1|0000000000002440|0000000000002440');
    const counts = new MissionLoginPlayerCounts(scene.players);
    const owner = { begin: 17, end: 23.5, scriptBreak: 0xffffffff, other: 37.25 };
    let steps = 0;
    const state = {
        get end() { return owner.end; }, set end(value) { owner.end = value; },
        get begin() { return owner.begin; }, set begin(value) { owner.begin = value; },
        get scriptBreak() { return owner.scriptBreak; }, set scriptBreak(value) { owner.scriptBreak = value; },
        requestPhase: 0x11223344, timeout: 0, clock: 0,
    };
    const hooks = {
        ...scene.hooks(),
        abortBattle: () => { assert.deepEqual([owner.scriptBreak, owner.begin, owner.end, state.requestPhase, steps], [1, 0, 0, 4, 0]); steps += 1; owner.scriptBreak = 0xffffffff; owner.begin = 29; state.requestPhase = 7; },
        closeProgress: () => { assert.deepEqual([owner.scriptBreak, owner.begin, state.requestPhase, steps], [0xffffffff, 29, 7, 1]); steps += 1; owner.scriptBreak = 0x80000000; },
    };
    new RawMissionLoginTimer(state, counts, scene.reads(), hooks).abort();
    assert.deepEqual([steps, owner.scriptBreak, owner.begin, state.requestPhase, owner.end, owner.other], [2, 0x80000000, 29, 7, 0, 37.25]);
    // 미연결 하위를 숨기지 않고 앞선 실제 부분 효과만 남긴다.
    owner.scriptBreak = 0;
    owner.begin = 53;
    owner.end = 67.75;
    const missing = new RawMissionLoginTimer(state, counts, scene.reads(), { ...hooks, abortBattle: undefined, closeProgress: () => { steps += 1; } });
    assert.throws(() => missing.abort(), /Abort 실제 상태 정리 미연결/);
    assert.deepEqual([owner.scriptBreak, owner.begin, owner.end, state.requestPhase, steps], [1, 0, 0, 4, 2]);
    // 완료: 미ready/미만료의 기존 missionPost 자연 완료 분기에서 priest 효과 뒤에 0 을 쓰고 closeProgress 가 마지막이다.
    const done = new LoginTimerScene('1,30,0,1,1,1,0,0,0,1,0,0,0,1|0000000000002540|0000000000002440');
    const doneCounts = new MissionLoginPlayerCounts(done.players);
    const current = { scriptBreak: 0xffffffff, end: 10.5, begin: 0 };
    const doneState = {
        get end() { return current.end; }, set end(value) { current.end = value; },
        get begin() { return current.begin; }, set begin(value) { current.begin = value; },
        get scriptBreak() { return current.scriptBreak; }, set scriptBreak(value) { current.scriptBreak = value; },
        requestPhase: 0, timeout: 0, clock: 0,
    };
    let priests = 0;
    let order = 0;
    const doneHooks = {
        ...done.hooks(),
        priestRefresh: (player) => { priests += 1; assert.equal(player, priests); assert.equal(current.scriptBreak, player === 1 ? 0xffffffff : 0x80000000); current.scriptBreak = 0x80000000; },
        saveReconnect: () => { assert.deepEqual([priests, current.scriptBreak, current.begin, current.end, order], [8, 0, 0, 10.5, 0]); order += 1; current.scriptBreak = 0xffffffff; },
        closeProgress: () => { assert.deepEqual([current.scriptBreak, current.end, order], [0xffffffff, 0, 1]); order += 1; current.scriptBreak = 0x80000000; },
    };
    new RawMissionLoginTimer(doneState, doneCounts, done.reads(), doneHooks).run(false);
    assert.deepEqual([priests, order, current.scriptBreak, current.end], [8, 2, 0x80000000, 0]);
    const text = done.trace.join(';');
    assert.ok(!text.includes('B,') && !text.includes('U,'));
});

// 일반 로그인 진행 HTML: 명단(이름 + 상태 문구)·남은 시간·제목·추가 안내와 번역 서식의 네 인자 검증. (2026-11-11 WB04-E318)
test('로그인 타이머: 진행 문구의 명단·남은 시간·서식 검증', () => {
    const players = new Uint8Array(9 * 0xb4);
    const name = (owner, text) => { players.set(Array.from(text, (char) => char.charCodeAt(0)), owner * 0xb4 + 12); players[owner * 0xb4 + 12 + text.length] = 0; };
    const setPlayer = (owner, subscriber, flags) => { put(players, owner * 0xb4 + 0x2c, subscriber); put(players, owner * 0xb4 + 0x50, flags); };
    setPlayer(1, 7, 0);
    setPlayer(2, 8, 2);
    setPlayer(3, 9, 0x102);
    setPlayer(4, 0, 0xffffffff);
    setPlayer(5, 10, 0x43);
    setPlayer(6, 11, 0x3);
    setPlayer(7, 12, 0x2);
    name(1, 'A');
    name(2, 'B');
    name(3, 'C');
    name(5, 'E');
    name(6, 'F');
    name(7, 'G');
    const counts = new MissionLoginPlayerCounts(players);
    const state = { end: 100, timeout: 30, begin: 0, scriptBreak: 0, requestPhase: 0, clock: 0 };
    let server = false;
    const reads = { savedGame: () => false, server: () => server, mission: () => false, missionPost: () => false, initialGeysers: () => false, checkingAssertions: () => false };
    const timer = new RawMissionLoginTimer(state, counts, reads, {});
    const calls = [];
    const translate = (text) => { calls.push(text); return text; };
    const base = timer.progressText(95.5, translate);
    // 명단: 상태 문구 키는 flags 로 정해진다(연결 중/대기/단절/전송 중/Ready).
    assert.ok(base.startsWith('~.~3Starting Game~*~.\n\nA: ~iConnecting~.\nB: ~iPending~.\nC: ~iDisconnected~.\nE: ~iReady~.\nF: ~iTransferring~.\nG: ~iPending~.\n\nTime Remaining: ~i4~.'));
    assert.ok(base.endsWith('Time Remaining: ~i4~.'));
    // 남은 시간이 1 미만이면 지연 안내가 붙고 음수는 0 이다.
    const late = timer.progressText(130, translate);
    assert.ok(late.includes('Time Remaining: ~i0~.') && late.includes('$ButtonInstant=Abort,LeaveBattle,3'));
    // 비정상 차이(무한/NaN/범위 밖)는 하위 DWORD 0 → 남은 시간 0 이다.
    for (const wall of [Infinity, -Infinity, NaN]) assert.ok(timer.progressText(wall, translate).includes('Time Remaining: ~i0~.'), String(wall));
    // 서버이면 제목이 바뀌고, 모두 준비/단절 외 대기가 없고 경과(timeout - 남은)가 10 초를 넘으면 강제 시작 안내가 붙는다.
    server = true;
    const asServer = timer.progressText(95.5, translate);
    assert.ok(asServer.startsWith('~.~3Becoming Server~*~.'));
    assert.ok(!asServer.includes('ServerForceBattleStart'));
    setPlayer(1, 0, 0);
    setPlayer(2, 0, 0);
    setPlayer(3, 0, 0);
    setPlayer(6, 0, 0);
    setPlayer(7, 0, 0);
    state.end = 100;
    assert.ok(timer.progressText(95.5, translate).includes('$ButtonInstant=Start,ServerForceBattleStart,0 '));
    // 번역 서식 검증: 인자 개수·종류·순서가 다르면 거부하고, 폭/정밀도/%% 는 유지한다.
    const withPattern = (pattern) => timer.progressText(95.5, (text) => (text.startsWith('~.~3%s') ? pattern : text));
    assert.throws(() => withPattern('%s %s %d'), TypeError);
    assert.throws(() => withPattern('%s %s %s %s'), TypeError);
    assert.throws(() => withPattern('%s %s %d %s %s'), TypeError);
    assert.throws(() => withPattern('%*s %s %d %s'), TypeError);
    assert.throws(() => withPattern('%s %s %d'), /인자 개수 오류/);
    assert.throws(() => withPattern('%s %s %s %s'), /네 인자의 서식 오류|네 인자 서식 오류/);
    assert.equal(withPattern('100%% [%s|%s|%5d|%.2s]'), '100% [Becoming Server|E: ~iReady~.\n|    4|\n\n]');
    assert.equal(withPattern('[%-20s|%s|%-3d|%s]').slice(0, 24), '[Becoming Server     |E:');
    assert.equal(withPattern('%s%s%03d%s').includes('Server' + 'E: ~iReady~.\n' + '004'), true);
    assert.equal(withPattern('%s%s%+d%s').includes('+4'), true);
    assert.equal(withPattern('%s%s% d%s').includes('~. 4'), false);
    assert.equal(withPattern('%s|%s|%.3d|%s').includes('|004|'), true);
    // 결과가 1024 바이트 이상이면 거부한다.
    assert.throws(() => withPattern(`${'x'.repeat(1024)}%s%s%d%s`), RangeError);
    // 번역기가 없으면 도달한 위치에서 거부한다.
    assert.throws(() => timer.progressText(95.5, undefined), /번역 미연결/);
});

// fixture 가 닿지 않는 경계(변형 확인에서 확인): 로그 종류 값, 일반 초기화/초기 간헐천 경로의 호출 순서와 로그, 진행 명단의 이름 NUL 누락, 강제 시작 조건(연결 중 제외·경과 시간·부호)과 1024 경계. (2026-11-11 WB04-E318)
test('로그인 타이머: 일반 초기화 경로와 진행 문구의 경계', () => {
    assert.deepEqual({ ...MISSION_LOGIN_TIMER_LOG }, { TimerStart: 0, Status: 1 });
    // 일반 초기화(저장 경로 아님·미션 아님)는 사제 상태 앞에서 초기화 부모를 부르고, 초기 간헐천이 켜져 있으면 로그 사이에 생성한다.
    // 입력: pattern 1, 모드 saved 0 / server 1 / mission 0 / post 1, geysers 1.
    const scene = new LoginTimerScene('1,30,0,1,0,1,0,1,0,1,0,0,0,1|0000000000002440|0000000000002440');
    const counts = new MissionLoginPlayerCounts(scene.players);
    const order = [];
    const hooks = {
        ...scene.hooks(),
        initializeGeneral: () => { order.push('init'); },
        createInitialGeysers: () => { order.push('geysers'); },
        log: (kind, text) => { order.push(`log:${kind}:${text}`); },
        priestRefresh: (owner) => { order.push(`priest:${owner}`); },
    };
    scene.begin = 1;
    new RawMissionLoginTimer(scene.state(), counts, scene.reads(), hooks).run(false);
    assert.deepEqual(order.slice(0, 5), ['init', 'log:1:Creating initial geysers\n', 'geysers', 'log:1:Done\n', 'log:1:Seting Priest State\n']);
    assert.equal(order.at(-1), 'log:1:Done\n');
    // 미션 모드이면 일반 초기화와 간헐천 생성을 모두 건너뛴다.
    const mission = new LoginTimerScene('1,30,0,1,1,1,1,1,0,1,0,0,0,1|0000000000002440|0000000000002440');
    const missionOrder = [];
    mission.begin = 1;
    new RawMissionLoginTimer(mission.state(), new MissionLoginPlayerCounts(mission.players), mission.reads(), { ...mission.hooks(), initializeGeneral: () => { missionOrder.push('init'); }, createInitialGeysers: () => { missionOrder.push('geysers'); } }).run(false);
    assert.deepEqual(missionOrder, []);
    // 진행 명단의 이름은 32바이트 안에서 NUL 로 끝나야 한다.
    const players = new Uint8Array(9 * 0xb4);
    put(players, 0xb4 + 0x2c, 7);
    players.fill(0x41, 0xb4 + 12, 0xb4 + 12 + 32);
    const state = { end: 100, timeout: 30, begin: 0, scriptBreak: 0, requestPhase: 0, clock: 0 };
    const reads = { savedGame: () => false, server: () => false, mission: () => false, missionPost: () => false, initialGeysers: () => false, checkingAssertions: () => false };
    assert.throws(() => new RawMissionLoginTimer(state, new MissionLoginPlayerCounts(players), reads, {}).progressText(95.5, (text) => text), RangeError);
    // 강제 시작 안내: 연결 중인 Player 는 대기 수에서 빼고(subscribers − disconnected − pending − connecting == ready), 경과(timeout − 남은) > 10 이어야 하며 부호 있는 비교이다.
    const buildScene = (timeout, remaining) => {
        const forced = new Uint8Array(9 * 0xb4);
        put(forced, 0xb4 + 0x2c, 7);
        put(forced, 0xb4 + 0x50, 0);
        put(forced, 2 * 0xb4 + 0x2c, 8);
        put(forced, 2 * 0xb4 + 0x50, 0x43);
        forced.set([0x41, 0], 0xb4 + 12);
        forced.set([0x42, 0], 2 * 0xb4 + 12);
        const timerState = { end: 100, timeout, begin: 0, scriptBreak: 0, requestPhase: 0, clock: 0 };
        const timer = new RawMissionLoginTimer(timerState, new MissionLoginPlayerCounts(forced), { ...reads, server: () => true }, {});
        return timer.progressText(100 - remaining, (text) => text);
    };
    // 연결 중 1 + Ready 1: 대기 수에서 연결 중을 빼면 ready 와 같아 안내가 붙는다(경과 26 > 10).
    assert.ok(buildScene(30, 4).includes('ServerForceBattleStart'));
    // 경과가 15 이면 > 10 이라 붙고, 20 이 넘는 기준이 아니다.
    assert.ok(buildScene(30, 15).includes('ServerForceBattleStart'));
    assert.ok(!buildScene(30, 20).includes('ServerForceBattleStart'));
    // timeout 보다 남은 시간이 크면 unsigned 로는 크지만 signed 로는 음수라 붙지 않는다.
    assert.ok(!buildScene(5, 20).includes('ServerForceBattleStart'));
    // 큰 timeout(음수 signed)은 32비트 감김 뒤 signed 로 비교한다: 0xee6b2800 − 4 는 음수라 붙지 않는다.
    assert.ok(!buildScene(-294967296, 4).includes('ServerForceBattleStart'));
    // 결과 길이가 정확히 1024 이면 거부하고 1023 이면 허용한다.
    const timer = new RawMissionLoginTimer(state, new MissionLoginPlayerCounts(new Uint8Array(9 * 0xb4)), reads, {});
    const base = timer.progressText(95.5, (text) => (text.startsWith('~.~3%s') ? '%s%s%d%s' : text)).length;
    const fill = (length) => timer.progressText(95.5, (text) => (text.startsWith('~.~3%s') ? `${'x'.repeat(length - base)}%s%s%d%s` : text));
    assert.equal(fill(1023).length, 1023);
    assert.throws(() => fill(1024), RangeError);
});
