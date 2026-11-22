// 송금 숫자 창(Send X Storm Power)의 생성 순서·배치·입력 제한·확정/취소/위임을 가짜 장치로 고정한다(cpppj `SaveFortNameDialogTests.cpp` 의 SendMoneyDialog 계약).
// 가짜 장치는 호출 순서를 기록하고, 각 위젯의 크기를 고정값으로 돌려준다. (2026-11-22 WB04-E1168)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { XlatTable } from '../www/assets/xlat.js';
import { SaveFortNameDialog, SaveFortNameDialogNode, SEND_MONEY_DIALOG_COMMAND, SEND_MONEY_DIALOG_VTABLE, SEND_MONEY_INITIAL_TEXT, SEND_MONEY_MAX_LENGTH, sendMoneyExcludedCharacters } from '../www/ui/save-fort-name-dialog.js';

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
        excluded: (token, text) => calls.push(['excluded', token, text]), background: (token, width, height) => { Object.assign(nodes.get(token), { baseWidth: width, baseHeight: height }); calls.push(['background', token, width, height]); },
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
    return { dialog: new SaveFortNameDialog(state, hooks), calls, nodes, hooks, state };
}

test('송금 숫자 창: 생성 순서와 번역 순서(제목 → OK → Cancel)', () => {
    const { dialog, calls } = scene(); const token = dialog.showMoney(2);
    assert.deepEqual(calls.map((call) => call[0]), ['allocate', 'decoration', 'allocate', 'textEntry', 'excluded', 'attach', 'allocate', 'background', 'attach', 'translate', 'allocate', 'title', 'attach', 'fitTitle', 'push',
        'attach', 'allocate', 'translate', 'stone', 'allocate', 'translate', 'stone', 'push', 'push', 'fitStone', 'fitStone', 'attach', 'attach', 'attach', 'depth', 'modal']);
    assert.deepEqual(calls.filter((call) => call[0] === 'translate').map((call) => call[1]), ['Send X Storm Power', 'OK', 'Cancel']);
    assert.deepEqual(calls.filter((call) => call[0] === 'allocate').map((call) => call[1]), [0xd8, 0x110, 0xb8, 0xa4, 0x170, 0x170]); assert.equal(token, 10);
});

test('송금 숫자 창: 원문 필드와 입력 한도·금지 문자', () => {
    const { dialog, calls, nodes } = scene(); const token = dialog.showMoney(0x12345678), root = nodes.get(token), entry = nodes.get(root.entry);
    assert.equal(root.vtable, SEND_MONEY_DIALOG_VTABLE); assert.equal(root.command, SEND_MONEY_DIALOG_COMMAND); assert.equal(root.command, 68); assert.equal(root.fortMode, 0x12345678); assert.equal(root.canCancel, 1);
    assert.equal(entry.maxLength, SEND_MONEY_MAX_LENGTH); assert.equal(entry.maxLength, 10); assert.equal(entry.multiline, 0);
    assert.deepEqual(calls.find((call) => call[0] === 'textEntry'), ['textEntry', root.entry, 160, 18, SEND_MONEY_INITIAL_TEXT]); assert.equal(SEND_MONEY_INITIAL_TEXT, '0');
    const excluded = calls.find((call) => call[0] === 'excluded')[2]; assert.equal(excluded, sendMoneyExcludedCharacters());
    assert.equal(excluded.length, 245); assert.equal(excluded.charCodeAt(0), 1); assert.equal(excluded.charCodeAt(244), 255); assert.ok(!/[0-9]/u.test(excluded));
    for (const digit of '0123456789') assert.ok(!excluded.includes(digit)); assert.ok(excluded.includes('/') && excluded.includes(':') && excluded.includes('q'));
});

test('송금 숫자 창: 배치(입력·테두리·제목·두 버튼·가운데 정렬·깊이)', () => {
    const { dialog, calls, nodes, state } = scene(); const token = dialog.showMoney(3), root = nodes.get(token);
    const attach = (child) => calls.filter((call) => call[0] === 'attach' && call[1] === child).map((call) => call.slice(2));
    // 입력 160×18 은 가로 (200-160)/2=20, 세로 45. 테두리는 8·4 더 크고 (−4, −2) 어긋난다.
    assert.deepEqual(attach(root.entry), [[20, 45, token, 0]]); const border = calls.find((call) => call[0] === 'background'); assert.deepEqual(border.slice(2), [168, 22]);
    assert.deepEqual(attach(border[1]), [[16, 43, token, 0]]);
    // 제목은 입력 x(20), y 10 에 붙이고 fit·push 뒤 정렬 4 로 같은 위치를 방식 2 로 재부착한다.
    assert.deepEqual(calls.find((call) => call[0] === 'title'), ['title', root.title, 'Send X Storm Power', 160]); // 제목 폭은 창 기본 폭 200×0.8 의 정수이다.
    assert.deepEqual(attach(root.title), [[20, 10, token, 0], [20, 10, token, 2]]); assert.equal(nodes.get(root.title).alignFlags, 4);
    // 두 버튼은 y 45+18+15=78, 합 40+56+10=106 → 가운데 시작 (200-106)/2=47, 취소는 47+40+10=97.
    assert.deepEqual(attach(root.ok), [[47, 78, token, 0]]); assert.deepEqual(attach(root.cancel), [[97, 78, token, 0]]);
    // 높이 100+4, 폭 max(200, 140+32)=200. 화면 640×480 의 가운데 (220, 188) 에 루트(1) 자식으로 붙는다.
    assert.equal(root.baseHeight, 104); assert.equal(root.height, 104); assert.equal(root.baseWidth, 200); assert.equal(root.width, 200);
    assert.deepEqual(attach(token), [[220, 188, state.entries[0], 0]]);
    // 원문은 현재 깊이에서 76 을 뺀 값을 AddDepth 의 변화량으로 넘긴다(가짜 장치는 그 변화량을 더하므로 -100 + (-100-76) = -276).
    assert.deepEqual(calls.at(-2), ['depth', token, -176]); assert.equal(root.depth, -276); assert.deepEqual(calls.at(-1), ['modal', token]);
    // push 는 이미 가려지지 않은(flags&8 없음) 두 버튼과 제목에만 부른다.
    assert.deepEqual(calls.filter((call) => call[0] === 'push').map((call) => call[1]), [root.title, root.ok, root.cancel]);
});

test('송금 숫자 창: 제목이 넓으면 폭이 제목+32 로 늘고 버튼도 그 폭에서 가운데', () => {
    const { dialog, nodes, calls } = scene({ titleWidth: 300, screen: [1024, 768] }); const token = dialog.showMoney(2), root = nodes.get(token);
    assert.equal(root.baseWidth, 332); assert.equal(root.width, 332); assert.equal(root.baseHeight, 104);
    // 버튼 위치는 폭이 넓어지기 전 200 기준으로 계산된다(원문 Buttons 가 창 폭 확장보다 먼저이다).
    assert.deepEqual(calls.filter((call) => call[0] === 'attach' && call[1] === root.ok).map((call) => call.slice(2)), [[47, 78, token, 0]]);
    assert.deepEqual(calls.filter((call) => call[0] === 'attach' && call[1] === token).map((call) => call.slice(2)), [[346, 332, 1, 0]]);
});

test('송금 숫자 창: 확정(OK 단추 serial·Enter)은 accept 가 비영일 때만 Kill 하고 항상 1', () => {
    const accepted = scene(); const token = accepted.dialog.showMoney(4), root = accepted.nodes.get(token), ok = accepted.nodes.get(root.ok).serial; accepted.calls.length = 0;
    assert.equal(accepted.dialog.handle(token, 0x0200002c, ok, 0), 1); assert.deepEqual(accepted.calls, [['accept', token], ['kill', token]]);
    accepted.calls.length = 0; assert.equal(accepted.dialog.handle(token, 0x02000038, 0, 0), 1); assert.deepEqual(accepted.calls, [['accept', token], ['kill', token]]);
    // 높은 비트(수정키·뗌 등 0x03ffffff 밖)는 사건 번호 비교에서 제외된다.
    accepted.calls.length = 0; assert.equal(accepted.dialog.handle(token, 0xfc000000 | 0x02000038, 0, 0) , 1); assert.deepEqual(accepted.calls.map((call) => call[0]), ['accept', 'kill']);
    // accept 가 0 을 돌려주면 창을 유지하고(Kill 없음) 그래도 1 이며 기본 Help 로 가지 않는다.
    const refused = scene({ accepts: 0 }); const retoken = refused.dialog.showMoney(4); refused.calls.length = 0;
    assert.equal(refused.dialog.handle(retoken, 0x02000038, 0, 0), 1); assert.deepEqual(refused.calls, [['accept', retoken]]);
});

test('송금 숫자 창: 취소 단추 serial 은 cancel 호출 뒤 비영일 때만 Kill 하고 다른 사건·다른 serial 은 기본 처리기에 위임', () => {
    const { dialog, calls, nodes } = scene(); const token = dialog.showMoney(5), root = nodes.get(token), cancel = nodes.get(root.cancel).serial, ok = nodes.get(root.ok).serial; calls.length = 0;
    assert.equal(dialog.handle(token, 0x0200002c, cancel, 0), 1); assert.deepEqual(calls, [['cancel', token], ['kill', token]]);
    // OK 가 아닌 다른 serial 의 클릭·다른 사건은 위임 결과(7)를 그대로 돌려준다.
    calls.length = 0; assert.equal(dialog.handle(token, 0x0200002c, 9999, 3), 7); assert.deepEqual(calls, [['baseHandle', token, 0x0200002c, 9999, 3]]);
    calls.length = 0; assert.equal(dialog.handle(token, 0x02000012, ok, 0), 7); assert.deepEqual(calls.map((call) => call[0]), ['baseHandle']);
    // 취소 callback 이 0 이면 Kill 하지 않는다(원문은 취소 불가 창 용).
    const stay = scene({ cancels: 0 }); const retoken = stay.dialog.showMoney(5), reroot = stay.nodes.get(retoken); stay.calls.length = 0;
    assert.equal(stay.dialog.handle(retoken, 0x0200002c, stay.nodes.get(reroot.cancel).serial, 0), 1); assert.deepEqual(stay.calls, [['cancel', retoken]]);
});

test('송금 숫자 창: 필요한 실제 장치가 없으면 그 자리에서 거부하고 입력 창 모양이 아니면 거부', () => {
    for (const name of ['allocate', 'decoration', 'textEntry', 'entry', 'excluded', 'background', 'title', 'stone', 'fitTitle', 'fitStone', 'push', 'attach', 'depth', 'modal', 'translate']) {
        const { dialog } = scene({ omit: [name] }); assert.throws(() => dialog.showMoney(1), (error) => error instanceof Error && /미연결|Cannot|not a function/u.test(error.message), name);
    }
    for (const name of ['accept', 'kill', 'baseHandle', 'cancel']) {
        const complete = scene(); const token = complete.dialog.showMoney(1), root = complete.nodes.get(token); const missing = scene({ omit: [name] }); const retoken = missing.dialog.showMoney(1), reroot = missing.nodes.get(retoken);
        const event = name === 'cancel' ? [0x0200002c, missing.nodes.get(reroot.cancel).serial] : name === 'baseHandle' ? [0x02000012, 0] : [0x02000038, 0];
        assert.throws(() => missing.dialog.handle(retoken, event[0], event[1], 0), /미연결/u, name); assert.ok(root);
    }
    assert.throws(() => new SaveFortNameDialog(null, {}), /등록표/u); assert.throws(() => new SaveFortNameDialog({ screenWidth: 1, entries: [] }, {}).node(1), /미연결/u);
    const noEntry = scene(); noEntry.hooks.entry = () => null; const broken = new SaveFortNameDialog(noEntry.state, noEntry.hooks); assert.throws(() => broken.showMoney(1), /편집 자식/u);
    const wrong = scene(); wrong.hooks.node = () => ({}); const bad = new SaveFortNameDialog(wrong.state, wrong.hooks); assert.throws(() => bad.showMoney(1), /입력 창 실제 부모 없음/u);
});

test('송금 숫자 창: 창이 요청하는 모든 문구가 한국어 번역표에 있다', () => {
    // 창이 번역 장치에 넘기는 원문(제목·OK·Cancel)을 모아 번역표에 모두 있는지 본다. 제목은 이 단위(E1168)에서 번역을 더했다.
    const korean = new XlatTable(fs.readFileSync(path.join(REPO_ROOT, 'webpj', 'translations', 'xlat.korean'), 'utf8'));
    const { dialog, calls } = scene(); dialog.showMoney(2);
    const keys = calls.filter((call) => call[0] === 'translate').map((call) => call[1]); assert.deepEqual(keys, ['Send X Storm Power', 'OK', 'Cancel']);
    for (const key of keys) assert.ok(korean.has(key), `한국어 번역 누락: ${key}`);
    assert.equal(korean.translate('Send X Storm Power'), '스톰 파워 보내기');
});
