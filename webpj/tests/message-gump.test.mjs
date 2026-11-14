// 원본 Pause324·시간90·Warning50의 전체192BYTE와 효과 순서, 후속 안내의 현재 이름/매개변수를 대조한다.
// 후속 안내 검사는 소스 계약이며 원본 직접 관측이나 실제 게임/멀티플레이 실행이 아니다. (2026-11-14 WB04-E452)
import './nan-exact.mjs';
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { MessageGump, MessageGumpNode, formatWarningMessage } from '../www/ui/message-gump.js';
import { GumpCreateState } from '../www/ui/gump-create.js';
import { decodeGumpNode, encodeGumpNode } from './gump-node-support.mjs';
import { readGameTimeSeconds } from '../www/core/game-time-display.js';

// 원문 격리 노드/미작성 필드의 DWORD다.
const TOKEN = 0x12401000; const CANARY = 0xa5a5a5a5;
// 원문 바이트 문자열과 double의 hex 표시다.
const hex = (text) => Buffer.from(text, 'latin1').toString('hex');
const doubleHex = (value) => { const data = Buffer.alloc(8); data.writeDoubleLE(value); return data.toString('hex'); };
// 독립 fixture 행만 읽는다. 제품 출력으로 기대값을 만들지 않는다.
const rows = (file) => fs.readFileSync(path.join(REPO_ROOT, 'cpppj/tests/fixtures', file), 'utf8').trimEnd().split(/\r?\n/).filter((line) => line && !line.startsWith('#')).map((line) => line.split('\t'));

// 실제 Text/이름/번역/Style/Fit의 명시 효과만 구성한다.
function scene() {
    const node = Object.assign(new MessageGumpNode(), decodeGumpNode('a5'.repeat(160))); const trace = [];
    node.head = node.reserved = node.trailing = CANARY; node.kind = node.player = node.parameter = CANARY | 0; node.deadline = Buffer.from('a5'.repeat(8), 'hex').readDoubleLE();
    const state = { challenge: 0, translation: 0, mutation: 0, wall: 0, source: null };
    const event = (value) => trace.push(value);
    const mutate = (phase) => { if (state.mutation === phase) { if (phase === 1) { node.player = 3; node.deadline = 42.5; } else { node.player = 8; node.parameter = 123; } } };
    const hooks = {
        node: (token) => { assert.equal(token, TOKEN); return node; },
        constructText: (token) => { assert.equal(token, TOKEN); event('Text'); node.vtable = 0x5144d4; node.flags = 0x400001; node.command = 45; node.serial = 41; node.head = 0; node.textContext.set([0, 0, 0, 0, 0, 0, 1, 1]); },
        challenge: () => state.challenge, playerName: (player) => { event(`PlayerName,${player >>> 0}`); mutate(1); }, challengePlayerName: (player) => { event(`ChallengeName,${player >>> 0}`); mutate(1); },
        translate: (source) => { event(`Translate,${hex(source)}`); mutate(2); return ['~E~wGame Paused!', '', '~E~yPause\xff!'][state.translation]; },
        style: (token, text) => { assert.equal(token, TOKEN); event(`Style,${hex(text)}`); node.head = 0x12408000; },
        fit: (token, x, y) => { assert.equal(token, TOKEN); event(`Fit,${x},${y}`); node.baseWidth = node.width = 51.25; node.baseHeight = node.height = 13.5; },
        wallSeconds: () => { event(`Wall,${doubleHex(state.wall)}`); return state.wall; }, idle: (token, enabled) => { assert.equal(token, TOKEN); assert.equal(enabled, true); event('Idle,1'); node.flags |= 0x80; }, warningSound: () => event('Sound'),
    };
    const snapshot = () => { const data = Buffer.alloc(192); data.set(Buffer.from(encodeGumpNode(node), 'hex')); data.writeUInt32LE(node.head, 0xa0); data.writeInt32LE(node.kind, 0xa4); data.writeInt32LE(node.player, 0xa8); data.writeUInt32LE(node.reserved, 0xac); data.writeDoubleLE(node.deadline, 0xb0); data.writeInt32LE(node.parameter, 0xb8); data.writeUInt32LE(node.trailing, 0xbc); return data.toString('hex'); };
    return { node, state, trace, hooks, snapshot, create: new GumpCreateState() };
}

test('MessageGump_PauseNative324: 원문 324행의 전체192BYTE/효과 일치', () => {
    const data = rows('message-gump-x86.tsv'); assert.equal(data.length, 324);
    // 두 모드·번역·외부 변경 입력을 각각 독립 노드에서 실행한다.
    for (const [input, expected, events] of data) {
        const [challenge, player, parameter, translation, mutation] = input.split(',').map(Number); const r = scene(); Object.assign(r.state, { challenge, translation, mutation });
        assert.equal(new MessageGump(r.create, r.hooks).construct(TOKEN, 11, player, parameter), TOKEN); assert.equal(r.snapshot(), expected, input); assert.equal(r.trace.join(';'), events, input);
    }
});

test('GameTimeDisplay_MessageNative90: CRT 시계/현재 원점·음수 IDIV·기한 0.1·전체192BYTE 일치', () => {
    const data = rows('game-time-message-x86.tsv'); assert.equal(data.length, 90);
    // 실제 clock 계산기와 같은 stamp/원점 getter를 연결한다.
    for (const row of data) {
        const r = scene(); const now = Number(row[0]); const origin = Number(row[1]); const language = Number(row[3]); const stamp = { value: 0x87654321 }; r.state.wall = Buffer.from(row[2], 'hex').readDoubleLE();
        r.hooks.gameTimeSeconds = () => readGameTimeSeconds(stamp, () => origin, () => { r.trace.push(`Clock,${now}`); return now; });
        r.hooks.translate = (source) => { r.trace.push(`Translate,${hex(source)}`); return ['~E~yGame Time~r:~w %d:%02d:%02d', '~ETime %d/%02d/%02d', ''][language]; };
        new MessageGump(r.create, r.hooks).construct(TOKEN, 16, 1, 0); assert.equal(r.snapshot(), row[4]); assert.equal(stamp.value, Number(row[5]));
        assert.equal(r.trace.join(';'), row[6].split(';').filter((part) => !part.startsWith('Format,')).join(';'));
    }
});

test('MessageGump_WarningNative50: NULL/빈/첫NUL/바이트 정밀도·전체192BYTE 일치', () => {
    const data = rows('warning-message-x86.tsv'); assert.equal(data.length, 50);
    // 본문은 별도 실제 바이트 뷰이며 현재 번역/음향 뒤에 읽힌다.
    for (const row of data) {
        const r = scene(); const input = ['Warning', '', 'A\0tail', '~B~E~yWarning\xff', null][Number(row[0])]; r.state.wall = Buffer.from(row[2], 'hex').readDoubleLE();
        r.hooks.translate = (source) => { r.trace.push(`Translate,${hex(source)}`); return ['%s', '~E~w[%s]', '', '%.3s', '%% %s'][Number(row[1])]; };
        new MessageGump(r.create, r.hooks).construct(TOKEN, 17, 0, 0, input === null ? null : Buffer.from(input, 'latin1')); assert.equal(r.snapshot(), row[3]); assert.equal(r.trace.join(';'), row[4]);
    }
});

test('Warning 실제 서식은 NULL/첫 NUL/폭/정밀도/%%와 255BYTE 한도를 보존한다', () => {
    assert.equal(formatWarningMessage('%s', null), '(null)'); assert.equal(formatWarningMessage('%s', ''), ''); assert.equal(formatWarningMessage('%s\0%q', 'A\0tail'), 'A');
    assert.equal(formatWarningMessage('%%', null), '%'); assert.equal(formatWarningMessage('%-5.3s', 'abcdef'), 'abc  '); assert.equal(formatWarningMessage('%s', 'x'.repeat(255)).length, 255);
    assert.throws(() => formatWarningMessage('%s', 'x'.repeat(256)), RangeError); assert.throws(() => formatWarningMessage('%s %s', 'one'), /실제 인자/); assert.throws(() => formatWarningMessage('%*s', 'one'), /실제 인자/);
});

test('Warning 번역 뒤 현재 BYTE를 읽고 누락 Idle의 선행 기한과 생성 필드를 유지한다', () => {
    const r = scene(); const source = new Uint8Array(32); source.set(Buffer.from('before\0')); let shown;
    r.hooks.warningSound = () => { source.set(Buffer.from('sound\0')); r.node.player = 8; r.node.parameter = 41; r.node.deadline = 700; };
    r.hooks.translate = () => { source.set(Buffer.from('translated\0')); return '[%s]'; }; r.hooks.style = (token, text) => { shown = text; }; delete r.hooks.idle;
    assert.throws(() => new MessageGump(r.create, r.hooks).construct(TOKEN, 17, 0, 0, source), /실제/); assert.equal(shown, '[translated]'); assert.equal(r.node.player, 8); assert.equal(r.node.parameter, 41); assert.equal(r.node.deadline, 10); assert.equal(r.node.kind, 17); assert.equal(r.trace.some((part) => part.startsWith('Fit')), false);
});

test('지원 밖 종류/선택된 이름 미연결은 원문 공통 생성 전후의 같은 경계에서 멈춘다', () => {
    const r = scene(); const message = new MessageGump(r.create, r.hooks); assert.throws(() => message.construct(TOKEN, 19, 0, 0), /미복원/); assert.equal(r.trace.length, 0);
    r.state.challenge = 1; delete r.hooks.challengePlayerName; assert.throws(() => new MessageGump(r.create, r.hooks).construct(TOKEN, 11, 1, 0), /실제 장치/); assert.deepEqual(r.trace, ['Text']);
});

// 후속 10.78 안내의 순수 UI 장치다. 실제 Player/동맹/전송/게임은 만들지 않는다.
function statusScene() {
    const r = scene(); const names = Array.from({ length: 10 }, (_, index) => new Uint8Array(Buffer.from(`P${index}\0`))); const shown = []; const lookups = [];
    Object.assign(r.hooks, { playerText: (owner) => { lookups.push(owner); return names[owner]; }, challengePlayerText: (owner) => { lookups.push(-owner); return names[owner]; }, battle: () => 1, translate: (pattern) => pattern, style: (token, value) => shown.push(value) });
    return { ...r, names, shown, lookups };
}

test('후속 안내: 응답 지연/접속/이탈/시작/종료의 색·음향·영구/10초 기한은 소스 계약과 같다', () => {
    const expectations = [[1, '~wServer not responding', 0, true], [2, '~E~uP4~w not responding', 0, true], [3, '~E~gP4~w is present', 10, false], [5, '~E~gP4~w has disconnected', 10, true], [6, '~E~gP4~w wishes to declare a Draw', 10, false], [9, '~E~wGame started!', 10, false], [10, '~E~wGame completed!', 10, false]];
    // 기대 문자열은 PE 색 표/소스의 정적 서식에서 별도로 적었다.
    for (const [kind, text, deadline, sound] of expectations) {
        const r = statusScene(); new MessageGump(r.create, r.hooks).construct(TOKEN, kind, 4, 0); assert.deepEqual(r.shown, [text]); assert.equal(r.node.deadline, deadline); assert.equal(r.trace.includes('Sound'), sound);
    }
    const neutral = statusScene(); neutral.hooks.battle = () => 0; new MessageGump(neutral.create, neutral.hooks).construct(TOKEN, 3, 0, 0); assert.deepEqual(neutral.shown, []); assert.equal(neutral.node.deadline, 10);
});

test('동맹 안내는 번역 뒤 현재 두 번째 이름/첫 이름 BYTE를 읽고 음향 뒤 선택 분기를 유지한다', () => {
    const r = statusScene(); r.hooks.translate = (pattern) => { r.names[1].set(Buffer.from('A\0')); r.node.parameter = 2; return pattern; };
    new MessageGump(r.create, r.hooks).construct(TOKEN, 7, 1, 4); assert.deepEqual(r.lookups, [1, 2]); assert.deepEqual(r.shown, ['~E~bA~w and ~gP2~w are now Allies']);
    const broken = statusScene(); broken.hooks.warningSound = () => { broken.node.kind = 6; };
    new MessageGump(broken.create, broken.hooks).construct(TOKEN, 8, 1, 4); assert.deepEqual(broken.shown, ['~E~bP1~w and ~gP4~w have broken their Alliance']); assert.equal(broken.node.kind, 6);
});

test('송금은 최초 amount, 자원 지급은 번역 뒤 현재 amount를 쓰며 두 안내의 음향을 구분한다', () => {
    const money = statusScene(); money.hooks.translate = (pattern) => { money.node.parameter = 99; return pattern; };
    new MessageGump(money.create, money.hooks).construct(TOKEN, 4, 1, -7); assert.deepEqual(money.shown, ['~E~bP1~w sent you -7 Storm Power']); assert.equal(money.node.deadline, 10); assert.ok(money.trace.includes('Sound'));
    const injection = statusScene(); injection.hooks.translate = (pattern) => { injection.node.parameter = 99; return pattern; };
    new MessageGump(injection.create, injection.hooks).construct(TOKEN, 18, 1, -7); assert.deepEqual(injection.shown, ['~E~wResource Injection of 99 sp was received']); assert.equal(injection.trace.includes('Sound'), false);
});

test('일반 안내의 Idle 미연결은 wall 조회 전이며 기존 Warning/시간의 선택 실패와 구분한다', () => {
    const r = statusScene(); delete r.hooks.idle; assert.throws(() => new MessageGump(r.create, r.hooks).construct(TOKEN, 9, 0, 0), /실제 장치/); assert.equal(r.node.deadline, 0); assert.equal(r.trace.some((part) => part.startsWith('Wall')), false);
    const invalid = statusScene(); invalid.hooks.translate = () => '%d'; assert.throws(() => new MessageGump(invalid.create, invalid.hooks).construct(TOKEN, 7, 1, 4), TypeError);
    const noNul = statusScene(); noNul.hooks.playerText = () => new Uint8Array([65]); assert.throws(() => new MessageGump(noNul.create, noNul.hooks).construct(TOKEN, 7, 1, 4), RangeError);
});
