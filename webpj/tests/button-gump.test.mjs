// 원본252입력의 전체 상태/콜백과 실제 캡처·idle·팝업 수명 연결을 대조한다. (2026-11-14 WB04-E416)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, bitsToFloat, floatToBits } from './raw-support.mjs';
import { ButtonScene, BUTTON_TOKENS as T } from './button-gump-support.mjs';
import { ButtonGump, bindButtonGump } from '../www/ui/button-gump.js';
import { GumpBackgroundFlags as F } from '../www/render/gump-visual.js';

test('ButtonGump_Native252: 원본 전체368BYTE·다섯 표/전역·반환·콜백 당시 전체 상태가 같다', () => {
    const rows = readRows(path.join(REPO_ROOT, 'cpppj/tests/fixtures/buttongump-x86.tsv')).filter((row) => row[0] === 'originals'); assert.equal(rows.length, 252);
    // 각 행 안의 연속 action만 같은 실제 저장소에서 수행한다.
    for (const row of rows) { assert.equal(row.length, 13); const s = new ButtonScene(); s.load(row); assert.equal(s.nodesSnapshot(), row[3], `입력노드${row[1]}`); assert.equal(s.snapshot(), row[4], `입력전역${row[1]}`); s.rig(); const results = [];
        // 기대 동작은 계산하지 않고 원문이 명시한 공개 호출만 재생한다.
        for (const action of row[2].split(';')) { const [op, ...args] = action.split(','); const [token, a, b, c] = args.map(Number); let value = '-';
            if (op === 'Construct') value = s.button.constructBase(token, a | 0, b | 0, c); else if (op === 'Value') s.button.setValue(token, a); else if (op === 'Pressed') s.button.setPressed(token, a); else if (op === 'Enabled') s.button.setEnabled(token, a !== 0); else if (op === 'Pulse') s.button.setPulse(token, a !== 0); else if (op === 'Blink') s.button.setBlink(token, a); else if (op === 'Handle') value = s.button.handle(token, a, b, c); else if (op === 'Broadcast') s.button.broadcast(token, a, b, c); else if (op === 'Idle') s.button.idle(token); else if (op === 'Contains') value = Number(s.button.contains(token, [bitsToFloat(a), bitsToFloat(b)])); else if (op === 'Local') value = s.collection.screenToLocal(token, { x: b | 0, y: c | 0 }).map(floatToBits).join(','); else assert.fail(op); results.push(`${op}:${value}`);
        }
        assert.equal(results.join(';'), row[7], `반환${row[1]}`); assert.equal(s.nodesSnapshot(), row[8], `노드${row[1]}`); assert.equal(s.snapshot(), row[9], `전역${row[1]}`); assert.equal(s.calls.join('|') || '-', row[10], `콜백${row[1]}`);
    }
});

test('ButtonGump 실제 이벤트→캡처→프레임 통지/idle→뗌과 같은 배경 눌림/표시 상태', () => {
    const s = new ButtonScene().basic(); const n = s.records.get(T[1]).node; assert.equal(s.input.route(0x10000, 1, 1), T[1]); assert.equal(s.life.focus, T[1]); assert.equal(n.pressed, 1); assert.equal(n.flags & 0x180, 0x180); assert.ok(s.records.get(T[5]).backgroundFlags & F.PRESSED); const visual = s.button.visualState(T[1], 37, 14); assert.deepEqual([visual.value, visual.pressed, visual.rect.left], [0, 1, 0]); s.frame.broadcast(0, 100, 100); assert.equal(n.pressed, 0); assert.equal(s.life.focus, T[1]); s.frame.broadcast(0, 1, 1); assert.equal(n.pressed, 1); assert.equal(s.input.route(0x80010000, 1, 1), T[1]); assert.equal(s.life.focus, 0); assert.equal(n.pressed, 0); assert.equal(n.flags & 0x180, 0); assert.equal(s.records.get(T[5]).backgroundFlags & F.PRESSED, 0); const previous = s.calls.slice(); s.frame.idle(); assert.deepEqual(s.calls, previous);
});

test('ButtonGump pulse 해제 뒤 현재 idle 후반의 반복 통지를 보존하며 별도 시계 조회를 사용한다', () => {
    const s = new ButtonScene().basic(); const n = s.records.get(T[1]).node; n.pressed = 1; s.button.setPulse(T[1], true); s.frame.idle(); assert.equal(n.buttonFlags & 0x10, 0); assert.equal(n.flags & 0x80, 0); assert.ok(s.calls.some((v) => v.startsWith('Parent,'))); assert.equal(n.repeatDeadline, 1.05); s.calls.length = 0; Object.assign(n, { buttonFlags: 0, pressed: 1, firstDeadline: 0, repeatDeadline: 0 }); s.button.idle(T[1]); assert.ok(s.calls.some((v) => v.startsWith('Parent,'))); assert.equal(n.repeatDeadline, 1.05); assert.equal(s.calls.filter((v) => v.startsWith('Now,')).length, 3);
});

test('ButtonGump 실제 팝업 Kill은 visible/void와 무관하며 같은 지연 큐와 캡처를 갱신한다', () => {
    const s = new ButtonScene().basic(); const n = s.records.get(T[1]).node; n.next = T[2]; const popup = s.records.get(T[2]).node; popup.flags = 0x4008; s.life.focus = T[2]; s.button.handle(T[1], 0x10000, 1, 1); assert.ok(popup.flags & 0x10000); assert.equal(s.life.killCount, 1); assert.equal(s.life.kill[0], T[2]); assert.equal(s.life.focus, T[1]); assert.equal(n.pressed, 1);
});

test('ButtonGump 지역 좌표·현재 콜백/부분 실패와 비선택/필수 가상 계약', () => {
    const s = new ButtonScene().basic(); const n = s.records.get(T[1]).node; s.records.get(T[0]).node.x = 100; n.x = 10.25; assert.deepEqual(s.collection.screenToLocal(T[1], { x: 11, y: 1 }), [0.75, 1]); const p = s.records.get(T[2]).node; Object.assign(p, { flags: p.flags | 0x2000, object: 2, x: -0.75, y: 0.25 }); n.parent = T[2]; assert.deepEqual(s.collection.screenToLocal(T[1], { x: 18, y: -18 }), [1, 1]); const hooks = s.hooks(); hooks.redraw = (token) => { s.records.get(token).node.value = 7; throw new Error('의도한 콜백 예외'); }; const button = new ButtonGump(s.create, s.state, s.factory, s.collection, s.events, s.input, s.lifetime, hooks); assert.throws(() => button.setValue(T[1], 1), /콜백 예외/); assert.equal(n.value, 7); n.parent = T[1]; assert.throws(() => s.collection.screenToLocal(T[1], { x: 1, y: 1 }), /순환/); assert.throws(() => s.button.handle(0, 0x10000, 1, 1)); assert.throws(() => new ButtonGump(s.create, s.state, s.factory, s.collection, s.events, s.input, s.lifetime));
    n.buttonFlags = 1; assert.equal(s.button.handle(T[1], 0x10000, 1, 1), 0); const calls = []; const event = { handle: (...args) => { calls.push(['event', ...args]); return 3; } }; const input = { handle: (...args) => { calls.push(['input', ...args]); return 4; } }; const frame = { idle: (token) => calls.push(['idle', token]), broadcast: (...args) => calls.push(['broadcast', ...args]) }; bindButtonGump(event, input, frame, () => null); assert.equal(event.handle(9, 8, 7, 6), 3); assert.equal(input.handle(5, 4, 3, 2), 4); frame.idle(1); frame.broadcast(4, 3, 2, 1); assert.deepEqual(calls, [['event', 9, 8, 7, 6], ['input', 5, 4, 3, 2], ['idle', 1], ['broadcast', 4, 3, 2, 1]]); assert.throws(() => bindButtonGump({}, {}, {}));
});
