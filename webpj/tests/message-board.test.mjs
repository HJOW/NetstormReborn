// 메시지 표 MessageBoard(20칸 serial 표·억제·중복 검색·생성/삭제·경고 시간)를 기준선 소스 계약과 손 계산 값으로 검증한다.
// cpppj에도 직접 시험이 없어 원본 직접 관측 수치가 아니다. 순서/경계는 기준선 소스(MessageBoard.cpp)에서 옮겼다. 실제 게임·브라우저 실행이 아니다. (2026-11-14 WB04-E445)
import test from 'node:test';
import assert from 'node:assert/strict';
import { MessageBoard, MessageBoardRecord, MessageBoardState, warningDeadline, warningExpired } from '../www/ui/message-board.js';

// 기록 틀이다. 레코드는 serial → 레코드 표에 있고, 생성은 100번부터 토큰/serial 을 만든다(serial = 토큰+1000). 화면 폭 1024, 메시지 폭 300 이다.
function rig({ withCreateText = false, createOnly = true, suppressedHook = null } = {}) {
    const log = []; const records = new Map(); const state = new MessageBoardState(); state.screenWidth = 1024; let next = 100;
    const make = (kind, player, parameter) => { const record = new MessageBoardRecord(); record.token = next; record.serial = next + 1000; record.kind = kind; record.player = player; record.parameter = parameter; next += 1; records.set(record.serial, record); return record; };
    const hooks = {
        lookup: (serial) => { log.push(['lookup', serial]); return records.get(serial) ?? null; }, kill: (token) => { log.push(['kill', token]); },
        width: (token) => { log.push(['width', token]); return 300; }, attach: (token, x, y) => { log.push(['attach', token, x, y]); },
    };
    if (createOnly) hooks.create = (kind, player, parameter, text) => { log.push(['create', kind, player, parameter, text]); return make(kind, player, parameter); };
    if (withCreateText) hooks.createText = (kind, player, parameter, text) => { log.push(['createText', kind, player, parameter, text]); return make(kind, player, parameter); };
    if (suppressedHook) hooks.suppressed = suppressedHook;
    return { board: new MessageBoard(state, hooks), state, hooks, log, records, make };
}

test('MessageBoardRecord: 새 레코드의 토큰/serial/종류/플레이어/인자는 모두 0 이다', () => {
    const record = new MessageBoardRecord(); assert.deepEqual([record.token, record.serial, record.kind, record.player, record.parameter], [0, 0, 0, 0, 0]);
});

test('MessageBoardState: 첫 칸은 첫 호출 표식 0xffffffff 이고 나머지 19칸은 0 이며 억제/화면 폭은 0 이다', () => {
    const state = new MessageBoardState(); assert.equal(state.serials.length, 20); assert.equal(state.serials[0], 0xffffffff); assert.ok(Array.from(state.serials).slice(1).every((value) => value === 0));
    assert.deepEqual([state.suppressed, state.screenWidth], [false, 0]); assert.ok(new MessageBoardRecord() instanceof MessageBoardRecord);
});

test('생성자: 조회/삭제/생성(또는 텍스트 생성)/폭/부착 중 하나라도 없으면 TypeError 다', () => {
    const good = rig().hooks;
    for (const name of ['lookup', 'kill', 'width', 'attach']) { const hooks = { ...good }; delete hooks[name]; assert.throws(() => new MessageBoard(new MessageBoardState(), hooks), { name: 'TypeError', message: /연결 누락/ }, name); }
    const none = { ...good }; delete none.create; assert.throws(() => new MessageBoard(new MessageBoardState(), none), TypeError); assert.throws(() => new MessageBoard(new MessageBoardState(), undefined), TypeError);
    assert.ok(new MessageBoard(new MessageBoardState(), { ...none, createText: () => null }));
});

test('show 생성: 첫 호출에서 표를 0 으로 채우고 첫 빈 칸에 생성한 뒤 serial 을 쓰고 화면 오른쪽에서 폭만큼 떨어진 x 와 칸×12 의 y 에 부착한다', () => {
    const r = rig(); r.board.show(3, 1, true, 5, '안녕'); assert.deepEqual(r.log, [['create', 3, 1, 5, '안녕'], ['width', 100], ['attach', 100, 724, 0]]);
    assert.equal(r.state.serials[0], 1100); assert.ok(Array.from(r.state.serials).slice(1).every((value) => value === 0)); r.log.length = 0;
    r.board.show(4, 1, true, 5, 'b'); assert.deepEqual(r.log.slice(-1), [['attach', 101, 724, 12]]); assert.deepEqual([r.state.serials[0], r.state.serials[1]], [1100, 1101]);
    r.state.screenWidth = 100; r.log.length = 0; r.board.show(5, 1, true, 5, 'c'); assert.deepEqual(r.log.slice(-1), [['attach', 102, -200, 24]]);
});

test('첫 호출 초기화와 x 감김: 표식이 있으면 모든 칸을 0 으로 채우고(남은 칸 조회 없음), 화면 폭−폭의 uint32 감김을 signed 로 부착한다', () => {
    const r = rig(); r.state.serials[1] = 999; r.board.show(3, 0, true, 0, 'x'); assert.equal(r.log.some((entry) => entry[0] === 'lookup'), false); assert.equal(r.state.serials[1], 0);
    const wrap = rig(); wrap.state.screenWidth = -2147483648; wrap.board.show(3, 0, true, 0, 'x'); assert.deepEqual(wrap.log.at(-1), ['attach', 100, 2147483348, 0]);
});

test('show 검색 순서: 비어 있지 않은 칸만 조회하고 stale serial 은 같은 칸에서 0 으로 지우며 그 칸이 첫 빈 칸이 된다', () => {
    const r = rig(); r.board.show(1, 0, true, 0, 'a'); r.board.show(2, 0, true, 0, 'b'); r.records.delete(1100); r.log.length = 0;
    r.board.show(9, 0, true, 0, 'new'); assert.deepEqual(r.log.filter((entry) => entry[0] === 'lookup'), [['lookup', 1100], ['lookup', 1101]]); assert.deepEqual(r.log.at(-1), ['attach', 102, 724, 0]);
    assert.deepEqual([r.state.serials[0], r.state.serials[1], r.state.serials[2]], [1102, 1101, 0]);
});

test('중복: 종류·플레이어·인자가 모두 같으면 검색이 멈추고 앞에 빈 칸이 없으면 생성하지 않으며, 앞에 빈 칸이 있으면 새로 만든다(원문대로 중복 방지가 아니다)', () => {
    const r = rig(); r.board.show(3, 1, true, 5, 'A'); r.board.show(4, 1, true, 5, 'B'); r.log.length = 0;
    r.board.show(3, 1, true, 5, 'dup'); assert.deepEqual(r.log, [['lookup', 1100]]); assert.deepEqual([r.state.serials[0], r.state.serials[1]], [1100, 1101]);
    r.board.show(3, 1, false, 5, ''); assert.equal(r.state.serials[0], 0); r.log.length = 0;
    r.board.show(4, 1, true, 5, 'dup2'); assert.deepEqual(r.log.filter((entry) => entry[0] === 'create').length, 1); assert.equal(r.state.serials[0] !== 0, true); assert.equal(r.state.serials[1], 1101);
    // 다른 한 필드만 달라도 일치가 아니다.
    for (const [kind, player, parameter] of [[5, 1, 5], [4, 2, 5], [4, 1, 6]]) { const q = rig(); q.board.show(4, 1, true, 5, 'x'); q.log.length = 0; q.board.show(kind, player, true, parameter, 'y'); assert.equal(q.log.some((entry) => entry[0] === 'create'), true, `${kind},${player},${parameter}`); }
});

test('삭제(enabled=false): 일치 레코드를 삭제 요청하고 그 칸을 0 으로 쓰며, 일치가 없으면 삭제하지 않고 stale 칸만 정리한다', () => {
    const r = rig(); r.board.show(3, 1, true, 5, 'A'); r.board.show(4, 1, true, 5, 'B'); r.log.length = 0;
    r.board.show(4, 1, false, 5, ''); assert.deepEqual(r.log, [['lookup', 1100], ['lookup', 1101], ['kill', 101]]); assert.deepEqual([r.state.serials[0], r.state.serials[1]], [1100, 0]);
    r.log.length = 0; r.board.show(99, 1, false, 5, ''); assert.equal(r.log.some((entry) => entry[0] === 'kill'), false); assert.equal(r.state.serials[0], 1100);
    // 일치 뒤의 stale 칸은 미리 정리하지 않는다.
    const s = rig(); s.board.show(1, 0, true, 0, 'a'); s.board.show(2, 0, true, 0, 'b'); s.board.show(3, 0, true, 0, 'c'); s.records.delete(1102); s.board.show(1, 0, false, 0, ''); assert.equal(s.state.serials[2], 1102);
});

test('Warning(종류 17): 같은 필드가 일치해도 검색을 멈추지 않아 계속 새로 만들고, 삭제 요청은 일치로 보지 않는다', () => {
    const r = rig(); for (let index = 0; index < 3; index += 1) r.board.show(17, 2, true, 7, 'warn'); assert.deepEqual(Array.from(r.state.serials).slice(0, 4), [1100, 1101, 1102, 0]);
    r.log.length = 0; r.board.show(17, 2, false, 7, ''); assert.equal(r.log.some((entry) => entry[0] === 'kill'), false); assert.deepEqual(Array.from(r.state.serials).slice(0, 3), [1100, 1101, 1102]);
});

test('억제: 억제 중이면 종류 11/16 만 통과하고 나머지는 표 초기화/조회 전에 반환하며, 억제 훅이 있으면 매 호출 현재 값을 읽는다', () => {
    const r = rig(); r.state.suppressed = true; r.board.show(3, 0, true, 0, 'x'); assert.deepEqual(r.log, []); assert.equal(r.state.serials[0], 0xffffffff);
    for (const kind of [11, 16]) { const q = rig(); q.state.suppressed = true; q.board.show(kind, 0, true, 0, 'x'); assert.equal(q.log.some((entry) => entry[0] === 'create'), true, String(kind)); }
    for (const kind of [10, 12, 15, 17]) { const q = rig(); q.state.suppressed = true; q.board.show(kind, 0, true, 0, 'x'); assert.deepEqual(q.log, [], String(kind)); }
    let value = true; let calls = 0; const hooked = rig({ suppressedHook: () => { calls += 1; return value; } }); hooked.board.show(3, 0, true, 0, 'x'); assert.deepEqual(hooked.log, []); value = false; hooked.board.show(3, 0, true, 0, 'x'); assert.equal(calls, 2);
    assert.equal(hooked.log.some((entry) => entry[0] === 'create'), true); const override = rig({ suppressedHook: () => false }); override.state.suppressed = true; override.board.show(3, 0, true, 0, 'x'); assert.equal(override.log.some((entry) => entry[0] === 'create'), true);
});

test('가득 참: 20칸이 모두 차면 새 메시지를 생성하지 않는다', () => {
    const r = rig(); for (let index = 0; index < 20; index += 1) r.board.show(index, 0, true, 0, 'x'); assert.equal(Array.from(r.state.serials).filter((value) => value).length, 20);
    r.log.length = 0; r.board.show(99, 0, true, 0, 'full'); assert.deepEqual(r.log.map((entry) => entry[0]), Array(20).fill('lookup')); assert.equal(r.state.serials[19] !== 0, true);
    const last = rig(); for (let index = 0; index < 19; index += 1) last.board.show(index, 0, true, 0, 'x'); last.board.show(77, 0, true, 0, 'last'); assert.deepEqual(last.log.at(-1), ['attach', 119, 724, 228]);
});

test('NULL 문자열: 텍스트 생성 장치가 있으면 null/undefined 와 빈 문자열을 구별해 그대로 넘기고(create 보다 우선), 없으면 NULL 은 Error 이며 빈 문자열은 create 로 간다', () => {
    const both = rig({ withCreateText: true }); both.board.showText(1, 0, true, 0, null); both.board.showText(2, 0, true, 0, undefined); both.board.showText(3, 0, true, 0, ''); both.board.show(4, 0, true, 0, 'z');
    assert.deepEqual(both.log.filter((entry) => entry[0].startsWith('create')), [['createText', 1, 0, 0, null], ['createText', 2, 0, 0, null], ['createText', 3, 0, 0, ''], ['createText', 4, 0, 0, 'z']]);
    const plain = rig(); assert.throws(() => plain.board.showText(1, 0, true, 0, null), { name: 'Error', message: /NULL 포인터/ }); assert.throws(() => plain.board.showText(1, 0, true, 0, undefined), /NULL 포인터/);
    plain.board.showText(2, 0, true, 0, ''); assert.deepEqual(plain.log.find((entry) => entry[0] === 'create'), ['create', 2, 0, 0, '']);
    const onlyText = rig({ withCreateText: true, createOnly: false }); onlyText.board.show(1, 0, true, 0, 'only'); assert.deepEqual(onlyText.log[0], ['createText', 1, 0, 0, 'only']);
    // NULL 이라도 억제/중복/삭제 부모는 그대로 지난다(삭제는 텍스트가 필요 없다).
    const del = rig(); del.board.show(5, 0, true, 0, 'x'); del.board.showText(5, 0, false, 0, null); assert.equal(del.state.serials[0], 0);
});

test('warningDeadline/warningExpired: 기한은 벽 시각+10 초(double)이고 0 은 영구이며 도달(≥)하면 만료, NaN 은 거짓이다', () => {
    assert.equal(warningDeadline(100.25), 110.25); assert.equal(warningDeadline(0), 10); assert.equal(warningDeadline(1e9 + 0.1), 1e9 + 0.1 + 10);
    assert.equal(warningExpired(0, 1e12), false); assert.equal(warningExpired(-0, 1e12), false); assert.equal(warningExpired(110, 109.999), false); assert.equal(warningExpired(110, 110), true); assert.equal(warningExpired(110, 200), true);
    assert.equal(warningExpired(NaN, 5), false); assert.equal(warningExpired(5, NaN), false); assert.equal(warningExpired(-5, -5), true); assert.equal(warningExpired(-5, -6), false);
});
