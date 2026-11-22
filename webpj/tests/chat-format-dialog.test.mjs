// 채팅 서식 입력 창(Edit Back/Front Format)의 생성 순서·원문 필드·배치·확정/취소 위임을 가짜 장치로 고정한다(원문 Netstorm.exe 0x4d4580/0x4d42b0/0x4d4620/0x4c9600 계약). (2026-11-22 WB04-E1179)
// 송금 숫자 창과 같은 공통 기반을 쓰므로 가짜 장치 구성은 같고, 차이(제목·글자 제한·가로 표시 순서·모달/깊이 없음·금지 문자 없음)를 확인한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { SaveFortNameDialogNode } from '../www/ui/save-fort-name-dialog.js';
import { CHAT_FORMAT_BACK_MAX_LENGTH, CHAT_FORMAT_BACK_VTABLE, CHAT_FORMAT_COMMAND, CHAT_FORMAT_COPY_LIMIT, CHAT_FORMAT_FRONT_MAX_LENGTH, CHAT_FORMAT_FRONT_VTABLE, ChatFormatDialog, chatFormatKey } from '../www/ui/chat-format-dialog.js';

// 기본 노드 모양(공통 필드만)이다.
const plain = () => ({ baseWidth: 0, baseHeight: 0, width: 0, height: 0, x: 0, y: 0, scale: 1, flags: 0, alignFlags: 0, depth: 0, serial: 0, maxLength: 0, multiline: 0 });

// 기록하는 가짜 장치 한 벌이다. 제목은 폭 titleWidth, 돌 버튼은 OK 40·Cancel 56 이다.
function scene({ titleWidth = 140, omit = [], screen = [640, 480], accepts = 1, cancels = 1 } = {}) {
    const calls = [], nodes = new Map(), state = { screenWidth: screen[0], screenHeight: screen[1], entries: [1] }; let next = 10, serial = 100;
    const labels = new Map();
    const hooks = {
        node: (token) => { const node = nodes.get(token); return node instanceof SaveFortNameDialogNode ? node : null; }, base: (token) => nodes.get(token) ?? null, entry: (token) => nodes.get(token) ?? null,
        allocate: (bytes) => { const token = next++; nodes.set(token, bytes === 0xd8 ? Object.assign(new SaveFortNameDialogNode(), plain()) : plain()); nodes.get(token).serial = serial++; calls.push(['allocate', bytes, token]); return token; },
        decoration: (token, width, height) => { Object.assign(nodes.get(token), { baseWidth: width, baseHeight: height, width, height, scale: 1, depth: -100 }); calls.push(['decoration', token, width, height]); },
        textEntry: (token, width, height, text) => { Object.assign(nodes.get(token), { baseWidth: width, baseHeight: height, width, height }); calls.push(['textEntry', token, width, height, text]); },
        excluded: (token, text) => calls.push(['excluded', token, text]), horizontal: (token, value) => calls.push(['horizontal', token, value]),
        background: (token, width, height) => { Object.assign(nodes.get(token), { baseWidth: width, baseHeight: height }); calls.push(['background', token, width, height]); },
        title: (token, text, width) => { labels.set(token, text); calls.push(['title', token, text, width]); }, stone: (token, text) => { labels.set(token, text); calls.push(['stone', token, text]); },
        fitTitle: (token) => { Object.assign(nodes.get(token), { baseWidth: titleWidth, baseHeight: 20 }); calls.push(['fitTitle', token]); },
        fitStone: (token) => { Object.assign(nodes.get(token), { baseWidth: labels.get(token) === 'OK' ? 40 : 56, baseHeight: 18 }); calls.push(['fitStone', token]); },
        push: (token) => { nodes.get(token).flags |= 8; calls.push(['push', token]); },
        attach: (token, x, y, parent, mode) => { Object.assign(nodes.get(token), { x, y }); calls.push(['attach', token, x, y, parent, mode]); },
        depth: (token, delta) => { nodes.get(token).depth += delta; calls.push(['depth', token, delta]); }, modal: (token) => calls.push(['modal', token]), kill: (token) => calls.push(['kill', token]),
        accept: (token) => { calls.push(['accept', token]); return accepts; }, cancel: (token) => { calls.push(['cancel', token]); return cancels; },
        baseHandle: (token, event, first, second) => { calls.push(['baseHandle', token, event, first, second]); return 7; },
        translate: (text) => { calls.push(['translate', text]); return text; },
    };
    for (const name of omit) delete hooks[name];
    return { dialog: new ChatFormatDialog(state, hooks), calls, nodes, hooks, state };
}

test('서식 창 Back: 생성 순서(가로 표시는 입력창을 붙인 뒤)와 모달·깊이·금지 문자 없음', () => {
    const { dialog, calls } = scene(); const token = dialog.show(false, 'abc');
    assert.deepEqual(calls.map((call) => call[0]), ['allocate', 'decoration', 'allocate', 'textEntry', 'attach', 'horizontal', 'allocate', 'background', 'attach', 'translate', 'allocate', 'title', 'attach', 'fitTitle', 'push',
        'attach', 'allocate', 'translate', 'stone', 'allocate', 'translate', 'stone', 'push', 'push', 'fitStone', 'fitStone', 'attach', 'attach', 'attach']);
    assert.ok(!calls.some((call) => ['modal', 'depth', 'excluded'].includes(call[0])));
    assert.deepEqual(calls.filter((call) => call[0] === 'translate').map((call) => call[1]), ['Please type your back format.', 'OK', 'Cancel']);
    assert.deepEqual(calls.filter((call) => call[0] === 'allocate').map((call) => call[1]), [0xd8, 0x110, 0xb8, 0xa4, 0x170, 0x170]); assert.equal(token, 10);
});

test('서식 창 Front: 가로 표시는 입력창을 붙이기 전, 제목은 front 문구', () => {
    const { dialog, calls } = scene(); dialog.show(true, '');
    assert.deepEqual(calls.slice(0, 6).map((call) => call[0]), ['allocate', 'decoration', 'allocate', 'textEntry', 'horizontal', 'attach']);
    assert.deepEqual(calls.filter((call) => call[0] === 'translate').map((call) => call[1])[0], 'Please type your front format.');
});

test('서식 창: 원문 필드·글자 제한·초기 문자열·입력창 크기', () => {
    for (const front of [false, true]) {
        const { dialog, calls, nodes } = scene(); const token = dialog.show(front, 'hello ~g'), root = nodes.get(token), entry = nodes.get(root.entry);
        assert.equal(root.vtable, front ? CHAT_FORMAT_FRONT_VTABLE : CHAT_FORMAT_BACK_VTABLE); assert.equal(CHAT_FORMAT_BACK_VTABLE, 0x517dd0); assert.equal(CHAT_FORMAT_FRONT_VTABLE, 0x517e38);
        assert.equal(root.command, CHAT_FORMAT_COMMAND); assert.equal(root.command, 59); assert.equal(root.fortMode, 1); assert.equal(root.canCancel, 1);
        assert.equal(entry.maxLength, front ? CHAT_FORMAT_FRONT_MAX_LENGTH : CHAT_FORMAT_BACK_MAX_LENGTH); assert.equal(entry.maxLength, front ? 300 : 500); assert.equal(entry.multiline, 0);
        // 입력창 너비는 틀 200 의 0.8 = 160, 높이 24, 초기 문자열은 호출 인자이다. 가로 표시 값은 1 이다.
        assert.deepEqual(calls.find((call) => call[0] === 'textEntry'), ['textEntry', root.entry, 160, 24, 'hello ~g']);
        assert.deepEqual(calls.find((call) => call[0] === 'horizontal'), ['horizontal', root.entry, 1]);
    }
    assert.equal(CHAT_FORMAT_COPY_LIMIT, 300);
});

test('서식 창: 초기 문자열이 없으면 빈 문자열이다', () => {
    const { dialog, calls } = scene(); dialog.show(false, undefined); assert.equal(calls.find((call) => call[0] === 'textEntry')[4], '');
});

test('서식 창: 배치(입력 y 45·테두리·제목·두 버튼·화면 가운데 부착)', () => {
    const { dialog, calls, nodes, state } = scene(); const token = dialog.show(false, ''), root = nodes.get(token);
    const attach = (child) => calls.filter((call) => call[0] === 'attach' && call[1] === child).map((call) => call.slice(2));
    // 입력 160×24 는 가로 (200-160)/2=20, 세로 45. 테두리는 8·4 더 크고 (−4, −2) 어긋난다.
    assert.deepEqual(attach(root.entry), [[20, 45, token, 0]]); assert.deepEqual(calls.find((call) => call[0] === 'background').slice(2), [168, 28]);
    assert.deepEqual(attach(calls.find((call) => call[0] === 'background')[1]), [[16, 43, token, 0]]);
    // 제목은 입력 x 20, y 10 에 붙인 뒤 정렬 4 로 같은 자리에 모드 2 로 재부착한다.
    assert.deepEqual(attach(root.title), [[20, 10, token, 0], [20, 10, token, 2]]); assert.equal(nodes.get(root.title).alignFlags, 4);
    // 두 버튼은 y = 입력 y 45 + 높이 24 + 15 = 84 이고 틀 높이는 4 늘어나며 폭은 제목 140+32=172 이하라 그대로 200 이다.
    assert.equal(attach(root.ok)[0][1], 84); assert.equal(root.baseHeight, 104); assert.equal(root.baseWidth, 200);
    // 화면 가운데: ((640-200)/2, (480-104)/2) 를 루트(entries[0])에 모드 0 으로 붙이며 이후 모달·깊이 변경은 없다.
    assert.deepEqual(attach(token), [[220, 188, state.entries[0], 0]]);
});

test('서식 창: 긴 제목이면 틀 폭이 제목+32 로 넓어진다', () => {
    const { dialog, nodes } = scene({ titleWidth: 215 }); const root = nodes.get(dialog.show(true, '')); assert.equal(root.baseWidth, 247);
});

test('서식 창: OK·Enter 확정은 accept 가 참일 때만 닫고 Cancel 은 항상 닫는다(공통 기반 처리)', () => {
    const { dialog, calls, nodes } = scene(); const token = dialog.show(false, ''), root = nodes.get(token); const ok = nodes.get(root.ok).serial, cancel = nodes.get(root.cancel).serial;
    assert.equal(dialog.handle(token, 0x0200002c, ok, 0), 1); assert.deepEqual(calls.slice(-2).map((call) => call[0]), ['accept', 'kill']);
    assert.equal(dialog.handle(token, 0x02000038, 0, 0), 1); assert.deepEqual(calls.slice(-2).map((call) => call[0]), ['accept', 'kill']);
    assert.equal(dialog.handle(token, 0x0200002c, cancel, 0), 1); assert.deepEqual(calls.slice(-2).map((call) => call[0]), ['cancel', 'kill']);
    assert.equal(dialog.handle(token, 0x02000001, 0, 0), 7);
    const rejected = scene({ accepts: 0 }); const other = rejected.dialog.show(true, ''); rejected.dialog.handle(other, 0x02000038, 0, 0);
    assert.ok(!rejected.calls.some((call) => call[0] === 'kill'));
});

test('서식 창: 실제 장치가 없으면 그 자리에서 거부한다', () => {
    for (const name of ['horizontal', 'textEntry', 'attach', 'title', 'fitTitle']) assert.throws(() => scene({ omit: [name] }).dialog.show(false, ''), /미연결|not a function/u, name);
});

test('서식 설정 키: 구독자(부호 있는 정수) 뒤에 Back 은 chatFormat, Front 는 chatFormat2', () => {
    assert.equal(chatFormatKey(7, false), '7chatFormat'); assert.equal(chatFormatKey(7, true), '7chatFormat2');
    assert.equal(chatFormatKey(0xffffffff, false), '-1chatFormat'); assert.equal(chatFormatKey(0, true), '0chatFormat2');
});
