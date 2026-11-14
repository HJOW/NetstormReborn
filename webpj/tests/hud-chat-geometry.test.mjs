// ChatWindow 기하 HudChatGeometry(전체 자식 재부착·도우미/입력줄 표시 전환·문자 Enter 분배·프레임말 전환 소비)를 기준선 소스 계약과 손 계산 값으로 검증한다.
// cpppj에도 직접 시험이 없어 원본 직접 관측 수치가 아니다. 순서/경계는 기준선 소스(HudChatGeometry.cpp)에서 옮겼다. 실제 게임·브라우저 실행이 아니다. (2026-11-14 WB04-E445)
import test from 'node:test';
import assert from 'node:assert/strict';
import { GumpCreateNode } from '../www/ui/gump-create.js';
import { HudChatGeometry, HudChatNode, consumeChatInputToggle, dispatchChatInputKey } from '../www/ui/hud-chat-geometry.js';
import { InputCode } from '../www/core/input-queue.js';

// 기록 틀이다. root 1, 배경 2, 본문 글 3, 도우미 4, 입력줄 5(보임). root 800×300·왼쪽 여백 10, 입력줄 높이 20, 도우미 BYTE 1, 입력줄 위치 (7,8), root 위치 (3,4)다.
function rig({ visible = true, helper = 1 } = {}) {
    const log = []; const root = new HudChatNode(); const nodes = new Map([[1, root]]); for (const token of [2, 3, 4, 5]) nodes.set(token, new GumpCreateNode());
    root.background = 2; root.text = 3; root.helper = 4; root.console = 5; root.baseWidth = 800; root.baseHeight = 300; root.leftPadding = 10; root.helperVisible = helper; root.x = 3; root.y = 4;
    nodes.get(5).baseHeight = 20; nodes.get(5).x = 7; nodes.get(5).y = 8; nodes.get(5).flags = visible ? 1 : 0; for (const token of [2, 3, 4, 5]) nodes.get(token).scale = 1;
    const state = { current: 1, challenge: 0 };
    const hooks = {
        chat: (token) => (token === 1 ? root : null), node: (token) => nodes.get(token) ?? null,
        baseAttach: (...args) => log.push(['baseAttach', ...args]), push: (token) => log.push(['push', token]), attach: (...args) => log.push(['attach', ...args]),
        bottomIndex: (token) => { log.push(['bottomIndex', token]); return 7; }, requestBottom: (token, line) => log.push(['requestBottom', token, line]),
        challenge: () => state.challenge, current: () => state.current,
    };
    return { geometry: new HudChatGeometry(hooks), hooks, root, nodes, log, state };
}

test('HudChatNode: 새 노드의 네 자식/여백/접두 128바이트/도우미 BYTE 는 0 이다', () => {
    const node = new HudChatNode(); assert.deepEqual([node.console, node.text, node.helper, node.background, node.capacity, node.leftPadding, node.helperVisible], [0, 0, 0, 0, 0, 0, 0]);
    assert.equal(node.inputPrefix.length, 128); assert.ok(node instanceof GumpCreateNode);
});

test('생성자: chat/node 조회가 없으면 TypeError 이고 나머지 장치는 도달할 때만 요구한다', () => {
    assert.throws(() => new HudChatGeometry({ node: () => null }), { name: 'TypeError', message: /같은 노드/ }); assert.throws(() => new HudChatGeometry({ chat: () => null }), TypeError); assert.throws(() => new HudChatGeometry(undefined), TypeError);
    const bare = new HudChatGeometry({ chat: () => null, node: () => null }); assert.throws(() => bare.attach(1, 0, 0, 0, 0), { name: 'Error', message: /공유 root 없음/ });
});

test('attach 순서/값: 기본 부착 → 배경 Push·크기 → 도우미 높이 → 배경 부착 → 본문 줄 조회·Push·크기·맨 아래 요청·부착 → 도우미 → 입력줄 순서와 좌표를 손 계산 값으로 따른다', () => {
    const r = rig(); r.geometry.attach(1, 1.5, 2.5, 9, 4);
    assert.deepEqual(r.log, [['baseAttach', 1, 1.5, 2.5, 9, 4], ['push', 2], ['attach', 2, 10, 0, 1, 0], ['bottomIndex', 3], ['push', 3], ['requestBottom', 3, 7], ['attach', 3, 0, 0, 2, 0], ['push', 4], ['attach', 4, 10, 285, 1, 0], ['push', 5], ['attach', 5, 27, 280, 1, 0]]);
    const [background, text, helper, input] = [2, 3, 4, 5].map((token) => r.nodes.get(token));
    assert.deepEqual([background.baseWidth, background.width, background.baseHeight, background.height], [790, 790, 280, 280]); assert.deepEqual([text.baseWidth, text.width, text.baseHeight, text.height], [790, 790, 280, 280]);
    assert.deepEqual([helper.baseHeight, helper.height], [15, 15]); assert.deepEqual([input.baseWidth, input.width, input.baseHeight], [790, 790, 20]);
});

test('attach 입력줄/도우미 관문: 입력줄이 숨겨지면 본문 높이는 root 전체이고 도우미는 0, 도우미 BYTE 가 0 이어도 도우미 높이만 0 이다', () => {
    const hidden = rig({ visible: false }); hidden.geometry.attach(1, 0, 0, 0, 0); assert.deepEqual([hidden.nodes.get(2).baseHeight, hidden.nodes.get(4).baseHeight, hidden.nodes.get(4).height], [300, 0, 0]); assert.deepEqual(hidden.log.at(-3), ['attach', 4, 10, 300, 1, 0]);
    const noHelper = rig({ helper: 0 }); noHelper.geometry.attach(1, 0, 0, 0, 0); assert.deepEqual([noHelper.nodes.get(2).baseHeight, noHelper.nodes.get(4).baseHeight], [280, 0]); assert.deepEqual(noHelper.log.at(-3), ['attach', 4, 10, 300, 1, 0]);
    const both = rig({ visible: false, helper: 0 }); both.geometry.attach(1, 0, 0, 0, 0); assert.equal(both.nodes.get(4).baseHeight, 0);
    // 입력줄 표시 비트만 본다(다른 플래그 비트는 무관).
    const flags = rig({ visible: false }); flags.nodes.get(5).flags = 0x1e; flags.geometry.attach(1, 0, 0, 0, 0); assert.equal(flags.nodes.get(2).baseHeight, 300); flags.nodes.get(5).flags = 0x1f; flags.geometry.attach(1, 0, 0, 0, 0); assert.equal(flags.nodes.get(2).baseHeight, 280);
});

test('attach 배율/중간값: 표시 크기는 float32 저장 전 배정도 중간값에 배율을 곱한 뒤 float32 로 저장하며, 도우미는 15×배율, 입력줄은 폭만 복사한다', () => {
    const r = rig(); r.root.baseWidth = 16777216; r.root.leftPadding = -1; r.nodes.get(2).scale = 1.5; r.nodes.get(3).scale = Math.fround(0.3); r.nodes.get(4).scale = 0.5; r.nodes.get(5).scale = 2; r.geometry.attach(1, 0, 0, 0, 0);
    assert.equal(r.nodes.get(2).baseWidth, 16777216); assert.equal(r.nodes.get(2).width, Math.fround(16777217 * 1.5)); assert.equal(r.nodes.get(2).width, 25165826); assert.notEqual(r.nodes.get(2).width, Math.fround(16777216 * 1.5));
    assert.equal(r.nodes.get(2).height, Math.fround(280 * 1.5)); assert.equal(r.nodes.get(3).width, Math.fround(16777216 * Math.fround(0.3))); assert.equal(r.nodes.get(4).height, 7.5);
    assert.deepEqual(r.log.filter((entry) => entry[0] === 'attach' && entry[1] === 4), [['attach', 4, -1, 285, 1, 0]]); assert.deepEqual([r.nodes.get(5).baseWidth, r.nodes.get(5).width], [16777216, 33554432]); assert.deepEqual(r.log.at(-1), ['attach', 5, 16, 280, 1, 0]);
    // 왼쪽 여백은 부호 있는 정수라 음수도 그대로 좌표에 쓰인다.
    const neg = rig(); neg.root.leftPadding = -5; neg.geometry.attach(1, 0, 0, 0, 0); assert.deepEqual(neg.log.filter((entry) => entry[0] === 'attach').map((entry) => entry[2]), [-5, 0, -5, 12]);
});

test('attach 재조회: Push 콜백이 root 의 자식/여백을 바꾸면 이후 단계는 새 값을 읽는다', () => {
    const r = rig(); const other = new GumpCreateNode(); other.baseHeight = 50; other.scale = 1; other.flags = 1; r.nodes.set(6, other);
    r.hooks.push = (token) => { r.log.push(['push', token]); if (token === 4) { r.root.console = 6; r.root.leftPadding = 20; } };
    r.geometry.attach(1, 0, 0, 0, 0); assert.deepEqual(r.log.slice(-2), [['push', 6], ['attach', 6, 37, 250, 1, 0]]); assert.deepEqual(r.log.filter((entry) => entry[0] === 'attach').map((entry) => entry[1]), [2, 3, 4, 6]);
});

test('attach 오류: 장치 누락은 도달한 위치에서 TypeError, 저장소/자식 없음은 Error 이며 이미 부른 효과는 취소되지 않는다', () => {
    for (const [name, last] of [['baseAttach', null], ['push', 'baseAttach'], ['attach', 'push'], ['bottomIndex', 'attach'], ['requestBottom', 'push'], ]) {
        const r = rig(); delete r.hooks[name]; const geometry = new HudChatGeometry(r.hooks); assert.throws(() => geometry.attach(1, 0, 0, 0, 0), { name: 'TypeError' }, name); if (last) assert.equal(r.log.at(-1)[0], last, name);
    }
    assert.throws(() => rig().geometry.attach(0, 0, 0, 0, 0), { name: 'Error', message: /공유 root 없음/ }); assert.throws(() => rig().geometry.attach(9, 0, 0, 0, 0), /공유 root 없음/);
    const mismatch = rig(); mismatch.hooks.node = (token) => (token === 1 ? new GumpCreateNode() : mismatch.nodes.get(token)); assert.throws(() => new HudChatGeometry(mismatch.hooks).attach(1, 0, 0, 0, 0), /공유 root 없음/); assert.deepEqual(mismatch.log, []);
    const noChild = rig(); noChild.root.helper = 0; assert.throws(() => noChild.geometry.attach(1, 0, 0, 0, 0), { name: 'Error', message: /실제 자식 없음/ }); assert.deepEqual(noChild.log.map((entry) => entry[0]), ['baseAttach', 'push']);
});

test('setHelperVisible: 0 이 아닌 값은 BYTE 1, 0 은 0 이며 재부착/크기 변경 없이 root 만 확인한다', () => {
    const r = rig({ helper: 0 }); for (const [value, expected] of [[5, 1], [0, 0], [0x80000000, 1], [1, 1], [-1, 1]]) { r.geometry.setHelperVisible(1, value); assert.equal(r.root.helperVisible, expected, String(value)); }
    assert.deepEqual(r.log, []); assert.throws(() => r.geometry.setHelperVisible(0, 1), /공유 root 없음/); assert.throws(() => r.geometry.setHelperVisible(7, 1), /공유 root 없음/);
});

test('setInputVisible: 토큰 0 은 무시하고, root·입력줄 Push 뒤 값/challenge 로 표시 비트를 정하고 입력줄(현재 좌표, parent 0)과 root 를 재부착한다', () => {
    const none = rig(); none.geometry.setInputVisible(0, 1); assert.deepEqual(none.log, []);
    const show = rig({ visible: false }); show.nodes.get(5).flags = 0x30; show.geometry.setInputVisible(1, 2); assert.equal(show.nodes.get(5).flags, 0x31); assert.deepEqual(show.log, [['push', 1], ['push', 5], ['attach', 5, 7, 8, 0, 0], ['attach', 1, 3, 4, 0, 0]]);
    const zero = rig(); zero.state.challenge = 0; zero.nodes.get(5).flags = 0x7; zero.geometry.setInputVisible(1, 0); assert.equal(zero.nodes.get(5).flags, 0x6);
    zero.state.challenge = 9; zero.geometry.setInputVisible(1, 0); assert.equal(zero.nodes.get(5).flags, 0x7); const challenged = rig({ visible: false }); challenged.state.challenge = 0x100; challenged.geometry.setInputVisible(1, 0); assert.equal(challenged.nodes.get(5).flags, 1);
    const hide = rig(); hide.geometry.setInputVisible(1, 0); assert.equal(hide.nodes.get(5).flags, 0);
    // 값 1 도 0 이 아니라 표시(challenge 와 무관)다.
    const one = rig({ visible: false }); one.state.challenge = 0; one.geometry.setInputVisible(1, 1); assert.equal(one.nodes.get(5).flags, 1);
    const lacking = rig(); delete lacking.hooks.challenge; assert.throws(() => new HudChatGeometry(lacking.hooks).setInputVisible(1, 0), { name: 'TypeError', message: /challenge/ }); assert.deepEqual(lacking.log.map((entry) => entry.join(':')), ['push:1', 'push:5']);
    const lackingOk = rig(); delete lackingOk.hooks.challenge; new HudChatGeometry(lackingOk.hooks).setInputVisible(1, 3); assert.equal(lackingOk.nodes.get(5).flags, 1);
    assert.throws(() => rig().geometry.setInputVisible(9, 1), /공유 root 없음/);
});

test('setInputVisible 재조회: 입력줄/root 좌표는 각 Push·부착 콜백 뒤에 다시 읽는다', () => {
    const r = rig(); r.hooks.attach = (...args) => { r.log.push(['attach', ...args]); if (args[0] === 5) { r.root.x = 40; r.root.y = 50; } }; r.geometry.setInputVisible(1, 1);
    assert.deepEqual(r.log.slice(-2), [['attach', 5, 7, 8, 0, 0], ['attach', 1, 40, 50, 0, 0]]);
});

test('toggleInputVisible: 현재 Chat 의 입력줄만 Push 하고 표시 비트를 뒤집은 뒤 같은 입력줄만 재부착하며(root Push/전체 기하 없음) Push 뒤 현재 root 를 다시 읽는다', () => {
    const r = rig(); r.nodes.get(5).flags = 0x11; r.geometry.toggleInputVisible(); assert.equal(r.nodes.get(5).flags, 0x10); assert.deepEqual(r.log, [['push', 5], ['attach', 5, 7, 8, 0, 0]]);
    r.log.length = 0; r.geometry.toggleInputVisible(); assert.equal(r.nodes.get(5).flags, 0x11);
    const none = rig(); none.state.current = 0; none.geometry.toggleInputVisible(); assert.deepEqual(none.log, []);
    assert.throws(() => new HudChatGeometry({ chat: () => null, node: () => null }).toggleInputVisible(), { name: 'Error', message: /현재 포인터 조회/ });
    // Push 가 현재 Chat 을 없애면 두 번째 현재 조회가 0 이라 Error 다.
    const gone = rig(); gone.hooks.push = (token) => { gone.log.push(['push', token]); gone.state.current = 0; }; assert.throws(() => gone.geometry.toggleInputVisible(), /공유 root 없음/); assert.equal(gone.nodes.get(5).flags, 1);
});

test('inputVisible: 현재 Chat 이 없으면 0(자식/노드 조회 안 함), 있으면 입력줄의 표시 비트만 돌려준다', () => {
    const r = rig(); r.nodes.get(5).flags = 0x1f; assert.equal(r.geometry.inputVisible(), 1); r.nodes.get(5).flags = 0x1e; assert.equal(r.geometry.inputVisible(), 0);
    let lookups = 0; const quiet = new HudChatGeometry({ chat: () => { lookups += 1; return null; }, node: () => { lookups += 1; return null; }, current: () => 0 }); assert.equal(quiet.inputVisible(), 0); assert.equal(lookups, 0);
    assert.throws(() => new HudChatGeometry({ chat: () => null, node: () => null }).inputVisible(), /현재 포인터 조회/); const bad = rig(); bad.state.current = 9; assert.throws(() => bad.geometry.inputVisible(), /공유 root 없음/);
});

test('dispatchChatInputKey: 뗌·다른 글자는 먼저 거부하고, 입력줄이 보이면 처리하지 않으며, 보이지 않으면 표시 → 열기를 호출하고 true 를 돌려준다', () => {
    const calls = []; const visible = () => { calls.push('visible'); return false; }; const show = () => calls.push('show'); const open = () => calls.push('open');
    assert.equal(dispatchChatInputKey({ code: 13 }, visible, show, open), true); assert.deepEqual(calls, ['visible', 'show', 'open']);
    for (const code of [14, 12, 0, InputCode.release | 13, 0x80000000 | 13, 0x1000e]) { calls.length = 0; assert.equal(dispatchChatInputKey({ code }, visible, show, open), false, String(code)); assert.deepEqual(calls, [], String(code)); }
    // 수정자/출처 비트는 문자 Enter 를 막지 않는다(마스크 0x03ffffff 밖).
    for (const code of [InputCode.control | 13, InputCode.shift | 13, InputCode.alt | 13, 0x04000000 | 13, 0x40000000 | 13]) { calls.length = 0; assert.equal(dispatchChatInputKey({ code }, visible, show, open), true, code.toString(16)); }
    // 마스크 안의 상위 비트는 글자 값에 포함된다.
    calls.length = 0; assert.equal(dispatchChatInputKey({ code: 0x02000000 | 13 }, visible, show, open), false); assert.equal(dispatchChatInputKey({ code: 0x00010000 | 13 }, visible, show, open), false);
    calls.length = 0; assert.equal(dispatchChatInputKey({ code: 13 }, () => { calls.push('visible'); return 1; }, show, open), false); assert.deepEqual(calls, ['visible']);
});

test('dispatchChatInputKey 오류: 입력줄 조회가 없으면 Error 이고(거부된 입력은 조회 없이 false), 표시/열기 중 하나라도 없으면 조회 뒤 Error 다', () => {
    assert.throws(() => dispatchChatInputKey({ code: 13 }, undefined, () => {}, () => {}), { name: 'Error', message: /입력줄 조회/ }); assert.equal(dispatchChatInputKey({ code: 14 }, undefined, undefined, undefined), false);
    assert.equal(dispatchChatInputKey({ code: 13 }, () => true, undefined, undefined), false);
    assert.throws(() => dispatchChatInputKey({ code: 13 }, () => false, undefined, () => {}), { name: 'Error', message: /표시\/Chat 열기/ }); assert.throws(() => dispatchChatInputKey({ code: 13 }, () => false, () => {}, undefined), /표시\/Chat 열기/);
});

test('consumeChatInputToggle: 요청이 0 이면 아무것도 하지 않고, 아니면 전환을 부른 뒤 0 으로 쓰며 전환이 던지면 요청을 지우지 않고 장치가 없으면 Error 다', () => {
    let calls = 0; const idle = { value: 0 }; consumeChatInputToggle(idle, () => { calls += 1; }); assert.equal(calls, 0); consumeChatInputToggle({ value: 0 }, undefined);
    const request = { value: 5 }; consumeChatInputToggle(request, () => { calls += 1; }); assert.deepEqual([calls, request.value], [1, 0]);
    const negative = { value: -1 }; consumeChatInputToggle(negative, () => { calls += 1; }); assert.deepEqual([calls, negative.value], [2, 0]);
    const failing = { value: 3 }; assert.throws(() => consumeChatInputToggle(failing, () => { throw new Error('실패'); }), /실패/); assert.equal(failing.value, 3);
    const missing = { value: 1 }; assert.throws(() => consumeChatInputToggle(missing, undefined), { name: 'Error', message: /전환 장치/ }); assert.equal(missing.value, 1);
});
