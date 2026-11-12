// 미션 시작 진행 안내(mission-loading-message.js)를 원본 10.78 관측과 cpppj 시험, 웹 계약으로 확인한다.
//  - 원본 관측: mission-loading-message-x86.tsv 1540행(확장 printf/임시 버퍼 경계 뒤 본문·Cancel 번역 × 기존/새 부모 6가지 프로필 × 깊이 × 호출 도중 현재 필드 변경 5가지)의 조립 본문·하위 효과 순서·최종 현재 필드(transient/created serial, 부모 깊이/serial)를 전부 대조한다.
//  - cpppj 시험 이식: MissionLoadingMessageTests 의 Native1540 · ConditionalCallbacksAndBounds. 나머지 `StartingMissionDialogMetadata` 는 DialogPage 파서(DialogScript, PORT_MAP todo)가 필요해 그 단위에서 옮긴다.
//  - 웹 계약: 기본 Cancel 단추 조건(`~.` 시작·`$Button` 포함)과 조건부 조회, 첫 NUL 절단, 1024BYTE 경계, 깊이 +1/−1 의 효과 뒤 재조회와 32비트 감김, 신규 경로의 created 재조회, 접근자 속성 상태 객체, 누락 효과의 도달 위치 거부.
// 실제 게임·브라우저 실행이 아니다. (2026-11-12 WB04-E339)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';
import { buildMissionLoadingMessageBody, showMissionLoadingMessage } from '../www/core/mission-loading-message.js';

// 원본 관측 행이다. 파일이 없으면 재생 검사를 건너뛴다. (2026-11-12 WB04-E339)
const ROWS = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'mission-loading-message-x86.tsv'));
// 독립 PE 관측의 두 명시 HTML 노드 토큰이다. (2026-11-12 WB04-E339)
const FIRST_PARENT = 0x12200000;
const SECOND_PARENT = 0x12204000;

// 16진 문자열을 한 글자 한 바이트의 원문 바이트 문자열로 바꾼다(`-` 는 빈 값, UTF-8 재해석 없음). (2026-11-12 WB04-E339)
function loadingBytes(hex) {
    if (hex === '-') return '';
    let text = '';
    // 두 16진 글자를 한 바이트로 돌린다.
    for (let index = 0; index < hex.length; index += 2) text += String.fromCharCode(Number.parseInt(hex.slice(index, index + 2), 16));
    return text;
}

// 관찰한 본문 바이트를 원문과 같은 소문자 16진으로 쓴다(빈 값은 `-`). (2026-11-12 WB04-E339)
function loadingHex(text) {
    if (text === '') return '-';
    return Array.from(text, (ch) => ch.charCodeAt(0).toString(16).padStart(2, '0')).join('');
}

// 예외가 나는지 본다. (2026-11-12 WB04-E339)
function throws(action, kind) {
    try { action(); } catch (error) { return kind ? error instanceof kind : true; }
    return false;
}

test('원본 관측 1540행: 조립 본문·하위 효과 순서·최종 현재 필드가 모두 일치한다', { skip: ROWS === null }, () => {
    assert.equal(ROWS.length, 1540);
    // 각 행은 새 상태/부모 노드로 같은 입력을 받는다.
    for (const row of ROWS) {
        assert.equal(row.length, 15);
        const message = loadingBytes(row[0]);
        const cancel = loadingBytes(row[1]);
        const profile = Number(row[2]);
        const depth = Number(row[3]);
        const mutation = Number(row[5]);
        const state = { transientSerial: 17, createdSerial: 73 };
        const parents = new Map([[FIRST_PARENT, { depth, serial: 17 }], [SECOND_PARENT, { depth, serial: profile === 1 ? 41 : 57 }]]);
        const trace = [];
        let observedBody = '';
        // 호출 도중 현재 필드를 바꾸는 입력(메타/본문 효과 시점)이다.
        const mutate = (token, where) => {
            const node = parents.get(token);
            if (mutation === 1 && where === 'M') { node.depth = 100; node.serial = 99; }
            else if (mutation === 2 && where === 'B') { node.depth = 0; node.serial = 77; }
            else if (mutation === 3 && where === 'M') { state.transientSerial = 888; state.createdSerial = 666; }
        };
        const hooks = {
            cancelText() { trace.push('T,Cancel'); return cancel; },
            findSerial(serial) {
                const found = profile === 0 && serial === 17 ? FIRST_PARENT : ((profile === 1 && serial === 41) || (profile === 4 && serial === 57)) ? SECOND_PARENT : 0;
                trace.push(`L,${serial},${found}`);
                return found;
            },
            updateDepth(token) {
                const node = parents.get(token);
                return { get value() { return node.depth; }, set value(next) { node.depth = next >>> 0; } };
            },
            serial: (token) => parents.get(token).serial,
            metadata(token, box) {
                observedBody = box.text;
                trace.push(`M,${token},${parents.get(token).depth},${loadingHex(box.text)}`);
                mutate(token, 'M');
                if (mutation === 4) box.text = 'AfterMetadata\n';
            },
            body(token, source) {
                observedBody = source;
                trace.push(`B,${token},${parents.get(token).depth},${loadingHex(source)}`);
                mutate(token, 'B');
            },
            create(source, mode, width, height) {
                observedBody = source;
                trace.push(`C,${mode},${width},${height},${loadingHex(source)}`);
                state.createdSerial = profile === 1 ? 41 : profile === 4 ? 57 : 0;
                return profile === 2 ? 0 : 41;
            },
        };
        const result = showMissionLoadingMessage(state, message, hooks);
        assert.equal(loadingHex(observedBody), row[6]);
        assert.equal(result, Number(row[7]));
        assert.equal(state.transientSerial, Number(row[8]));
        assert.equal(state.createdSerial, Number(row[9]));
        assert.equal(parents.get(FIRST_PARENT).depth, Number(row[10]));
        assert.equal(parents.get(FIRST_PARENT).serial, Number(row[11]));
        assert.equal(parents.get(SECOND_PARENT).depth, Number(row[12]));
        assert.equal(parents.get(SECOND_PARENT).serial, Number(row[13]));
        assert.equal(trace.join(';'), row[14]);
    }
});

test('cpppj MissionLoadingMessage_ConditionalCallbacksAndBounds: 조회하지 않는 경계는 요구하지 않고 버퍼 밖만 거부한다', () => {
    assert.equal(buildMissionLoadingMessageBody('~.Body', undefined), '~.Body\n');
    assert.equal(buildMissionLoadingMessageBody('Body$Buttonish', undefined), 'Body$Buttonish\n');
    assert.ok(throws(() => buildMissionLoadingMessageBody('Body', undefined)));
    assert.equal(buildMissionLoadingMessageBody(`~.${'A'.repeat(1020)}\n`, undefined).length, 1023);
    assert.ok(throws(() => buildMissionLoadingMessageBody('A'.repeat(1023), undefined), RangeError));
    const state = { transientSerial: 0, createdSerial: 0 };
    assert.ok(throws(() => showMissionLoadingMessage(state, 'Body', { cancelText: () => 'Cancel' })));
    assert.equal(state.transientSerial, 0);
    assert.equal(state.createdSerial, 0);
});

test('본문 조립: 기본 Cancel 단추와 줄바꿈 규칙(시작 안내 한 줄의 실제 모양)', () => {
    assert.equal(buildMissionLoadingMessageBody('Starting Mission...', () => 'Cancel'), 'Starting Mission...\n$ButtonInstant=Cancel,LeaveBattle,3');
    assert.equal(buildMissionLoadingMessageBody('Line\n', () => 'Cancel'), 'Line\n$ButtonInstant=Cancel,LeaveBattle,3');
    // 빈 본문도 줄바꿈 하나 뒤에 단추가 붙는다.
    assert.equal(buildMissionLoadingMessageBody('', () => 'Stop'), '\n$ButtonInstant=Stop,LeaveBattle,3');
    // `$Button` 검사는 대소문자를 구별한다.
    assert.equal(buildMissionLoadingMessageBody('a$button', () => 'C'), 'a$button\n$ButtonInstant=C,LeaveBattle,3');
    // 본문과 Cancel 번역은 첫 NUL 앞만 쓴다.
    assert.equal(buildMissionLoadingMessageBody('Body\0hidden', () => 'Ca\0ncel'), 'Body\n$ButtonInstant=Ca,LeaveBattle,3');
    // `~.` 는 맨 앞일 때만 단추를 막는다.
    assert.equal(buildMissionLoadingMessageBody('x~.', () => 'C'), 'x~.\n$ButtonInstant=C,LeaveBattle,3');
    assert.equal(buildMissionLoadingMessageBody('~Body', () => 'C'), '~Body\n$ButtonInstant=C,LeaveBattle,3');
    assert.equal(buildMissionLoadingMessageBody('~', () => 'C'), '~\n$ButtonInstant=C,LeaveBattle,3');
});

test('붙인 뒤 1024BYTE 에 닿으면 거부한다(본문은 맞아도 단추 때문에 넘는 경계)', () => {
    // 본문 1000 + 줄바꿈 1 + 단추 15 + 'Cancel' 6 + 14 = 1036 이므로 거부, 1002(본문 987)는 1023 이라 허용.
    assert.ok(throws(() => buildMissionLoadingMessageBody('A'.repeat(1000), () => 'Cancel'), RangeError));
    const fits = buildMissionLoadingMessageBody('A'.repeat(987), () => 'Cancel');
    assert.equal(fits.length, 987 + 1 + 15 + 6 + 14);
    assert.ok(fits.length < 1024);
    assert.ok(throws(() => buildMissionLoadingMessageBody('A'.repeat(988), () => 'Cancel'), RangeError));
});

// 한 부모만 있는 단순 장면이다. 깊이 상자는 노드를 직접 가리킨다. (2026-11-12 WB04-E339)
function simpleScene(node) {
    const calls = [];
    return {
        calls,
        hooks: {
            cancelText: () => 'Cancel',
            findSerial: (serial) => (serial === node.serial ? 1 : 0),
            updateDepth: () => ({ get value() { return node.depth; }, set value(next) { node.depth = next; } }),
            serial: () => node.serial,
            metadata: (_token, box) => { calls.push(`M${node.depth}`); box.text = box.text.replace('$ButtonInstant=Cancel,LeaveBattle,3', ''); },
            body: (_token, text) => { calls.push(`B${node.depth}:${text.length}`); },
            create: () => { throw new Error('기존 부모가 있으면 새로 만들지 않아야 함'); },
        },
    };
}

test('기존 부모 갱신: 깊이는 효과 동안 +1 이고 끝나면 −1 로 돌아오며 메타가 지운 본문이 전달된다', () => {
    const node = { depth: 5, serial: 9 };
    const scene = simpleScene(node);
    const state = { transientSerial: 9, createdSerial: 0 };
    assert.equal(showMissionLoadingMessage(state, 'Hello', scene.hooks), 9);
    // 깊이 5 → 6 으로 메타/본문을 부르고 5 로 되돌린다. 본문은 `Hello\n`(6글자)만 남는다.
    assert.deepEqual(scene.calls, ['M6', 'B6:6']);
    assert.equal(node.depth, 5);
});

test('깊이는 32비트로 감기고 효과 뒤 현재 값에서 감소한다', () => {
    const node = { depth: 0xffffffff, serial: 9 };
    const scene = simpleScene(node);
    scene.hooks.body = () => { assert.equal(node.depth, 0); node.depth = 50; };
    showMissionLoadingMessage({ transientSerial: 9, createdSerial: 0 }, 'x', scene.hooks);
    assert.equal(node.depth, 49);
    const low = { depth: 0, serial: 9 };
    const lowScene = simpleScene(low);
    lowScene.hooks.body = () => { low.depth = 0; };
    showMissionLoadingMessage({ transientSerial: 9, createdSerial: 0 }, 'x', lowScene.hooks);
    assert.equal(low.depth, 0xffffffff);
});

test('상태는 접근자 속성 객체여도 현재 값을 읽고 쓴다(Client 의 비연속 전역을 빌리는 모양)', () => {
    const globals = { transient: 9, created: 0 };
    const view = {
        get transientSerial() { return globals.transient; },
        set transientSerial(value) { globals.transient = value; },
        get createdSerial() { return globals.created; },
        set createdSerial(value) { globals.created = value; },
    };
    const node = { depth: 0, serial: 33 };
    const scene = simpleScene(node);
    scene.hooks.findSerial = (serial) => (serial === 9 ? 1 : 0);
    scene.hooks.serial = () => 33;
    assert.equal(showMissionLoadingMessage(view, 'Hi', scene.hooks), 33);
    assert.equal(globals.transient, 33);
    assert.equal(globals.created, 0);
});

test('새 부모 경로: create 의 반환과 create 가 바꾼 createdSerial 을 서로 바꾸지 않고, created 부모가 없으면 갱신하지 않는다', () => {
    const log = [];
    const state = { transientSerial: 5, createdSerial: 6 };
    const hooks = {
        cancelText: () => 'Cancel',
        findSerial: (serial) => { log.push(`L${serial}`); return 0; },
        create: (body, mode, width, height) => { log.push(`C${mode},${width},${height}`); state.createdSerial = 70; return 71; },
    };
    assert.equal(showMissionLoadingMessage(state, 'Hi', hooks), 71);
    // create 뒤에는 create 가 바꾼 현재 createdSerial(70)을 찾는다.
    assert.deepEqual(log, ['L5', 'C0,400,30', 'L70']);
    assert.equal(state.transientSerial, 71);
    assert.equal(state.createdSerial, 70);
});

test('도달한 효과의 누락은 도달한 위치에서 거부한다', () => {
    const state = { transientSerial: 1, createdSerial: 2 };
    assert.ok(throws(() => showMissionLoadingMessage(state, 'x', { cancelText: () => 'C' })));
    assert.ok(throws(() => showMissionLoadingMessage(state, 'x', { cancelText: () => 'C', findSerial: () => 0 })));
    assert.ok(throws(() => showMissionLoadingMessage(state, 'x', { cancelText: () => 'C', findSerial: () => 1 })));
    assert.ok(throws(() => showMissionLoadingMessage(state, 'x', undefined)));
    assert.equal(state.transientSerial, 1);
});

test('메타가 본문에 NUL 을 남기면 본문 갱신에는 첫 NUL 앞만 전달한다', () => {
    const node = { depth: 0, serial: 9 };
    const scene = simpleScene(node);
    let received = null;
    scene.hooks.metadata = (_token, box) => { box.text = 'Head\0Tail'; };
    scene.hooks.body = (_token, text) => { received = text; };
    showMissionLoadingMessage({ transientSerial: 9, createdSerial: 0 }, 'x', scene.hooks);
    assert.equal(received, 'Head');
});
