// 10.78 일반 전투 종료 관문의 현재 Player·동맹과 선택 효과를 확인한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { createHash } from 'node:crypto';
import { RawBattleCompletion } from '../www/core/raw-battle-completion.js';

// 각 검사가 독립된 9 Player와 현재 옵션·동맹을 쓰도록 만든다.
function scene() {
    const globals = { players: new Uint8Array(9 * 180), options: new Uint8Array(87), alliances: new Uint32Array(81), battle: true, missionPost: false, boss: true, scriptBreak: 0, loginEnd: 0 };
    const calls = [], dead = new Set(), hooks = { serverConnection: () => { calls.push('server'); return true; }, priestDead: (owner) => { calls.push(owner); return dead.has(owner); } };
    const write = (owner, offset, value) => new DataView(globals.players.buffer, globals.players.byteOffset).setUint32(owner * 180 + offset, value, true);
    const gate = new RawBattleCompletion(globals, hooks);
    return { globals, hooks, calls, dead, write, gate };
}

// 참가자와 양방향 동맹, 관전자 및 사제 생존의 원본 순서를 확인한다.
test('전투 종료: 팀·상호 동맹·관전자와 현재 사제 조회', () => {
    const { globals: g, calls, dead, write, gate } = scene();
    write(1, 0x2c, 11); write(2, 0x2c, 22);
    g.alliances[1 * 9 + 2] = 1; assert.equal(gate.allAllied(), false);
    assert.equal(gate.teamFinished(), false); assert.deepEqual(calls, ['server', 1, 2]);
    g.alliances[2 * 9 + 1] = 1; calls.length = 0; assert.equal(gate.finished(), true); assert.deepEqual(calls, ['server']);
    g.options[0x3e + 2] = 1; calls.length = 0; assert.equal(gate.teamFinished(), true); assert.deepEqual(calls, ['server', 1, 2]);
    g.options[0x3e + 2] = 0; g.alliances[1 * 9 + 2] = 0; dead.add(2); assert.equal(gate.teamFinished(), true);
    g.missionPost = true; calls.length = 0; assert.equal(gate.finished(), false); assert.deepEqual(calls, []);
});

// 생존 판정 뒤 변경된 기권 비트와 로그인 기한을 다시 읽는다.
test('전투 종료: 기권 비트의 현재 행과 boss별 분기', () => {
    const { globals: g, hooks, calls, write, gate } = scene();
    write(1, 0x2c, 11); write(1, 0x50, 1); write(2, 0x2c, 22); write(2, 0x50, 1);
    hooks.priestDead = (owner) => { calls.push(owner); write(owner, 0x50, 0x81); return false; };
    assert.equal(gate.flagFinished(1), false); assert.deepEqual(calls, ['server', 1, 2]);
    g.boss = false; calls.length = 0; assert.equal(gate.flagFinished(1), true); assert.deepEqual(calls, ['server', 1, 2]);
    g.loginEnd = 1; calls.length = 0; assert.equal(gate.flagFinished(1), false); assert.deepEqual(calls, ['server']);
    g.loginEnd = 0; g.boss = true; assert.equal(gate.flagFinished(0), true);
});

// 미접속 판정은 서버→로컬 사제→boss 관문과 각 행의 현재 집계를 유지한다.
test('전투 종료: 미접속 플레이어와 자연 반환', () => {
    const { globals: g, hooks, calls, dead, write, gate } = scene();
    write(1, 0x2c, 11); write(1, 0x50, 1); write(2, 0x2c, 22); write(2, 0x50, 0x80);
    assert.equal(gate.unloggedFinished(1), false); assert.deepEqual(calls, ['server', 1]);
    dead.add(1); calls.length = 0; assert.equal(gate.unloggedFinished(1), true); assert.deepEqual(calls.slice(0, 2), ['server', 1]);
    g.boss = false; calls.length = 0; assert.equal(gate.unloggedFinished(1), false); assert.deepEqual(calls, ['server', 1]);
    hooks.serverConnection = () => false; calls.length = 0; assert.equal(gate.unloggedFinished(1), false); assert.deepEqual(calls, []);
});

// 실제 저장소 재조회가 잘린 경우 조용히 다른 기록을 사용하지 않는다.
test('전투 종료: Player 물리 검증과 현재 저장소 재조회', () => {
    const { globals, hooks, write, gate } = scene();
    assert.throws(() => new RawBattleCompletion({ ...globals, players: new Uint8Array(1) }), /Player/);
    write(1, 0x2c, 11); hooks.currentPlayers = () => new Uint8Array(1);
    assert.throws(() => gate.allAllied(), /Player DWORD/);
});

// 동일 합성 입력을 cpppj C++ 판정기로 실행한 반환·조회 순서 4096행의 정규화 SHA-256과 비교한다.
test('전투 종료: cpppj 판정기 합성 상태 4096건과 조회 순서', () => {
    const rows = [];
    // C++ 관측 도구와 같은 seed마다 독립 Player·동맹 상태를 만든다.
    for (let sample = 0; sample < 4096; sample += 1) {
        let random = sample + 0x14b37;
        // 원본 DWORD 감김과 관측 도구의 상위 16비트를 재현한다.
        const next = () => { random = (Math.imul(random, 1664525) + 1013904223) >>> 0; return random >>> 16; };
        const players = new Uint8Array(9 * 180), options = new Uint8Array(87), alliances = new Uint32Array(81), dead = [];
        // 참가자, 연결·기권, 관전자, 사제 값을 원본 관측 입력과 같은 순서로 만든다.
        for (let owner = 1; owner < 9; owner += 1) {
            players[owner * 180 + 0x2c] = next() % 4 !== 0 ? owner : 0;
            players[owner * 180 + 0x50] = (next() % 2 !== 0 ? 1 : 0) | (next() % 2 !== 0 ? 0x80 : 0);
            options[0x3e + owner] = Number(next() % 5 === 0); dead[owner] = next() % 2 !== 0;
        }
        // 방향별 동맹 81개를 각각 생성한다.
        for (let index = 0; index < 81; index += 1) alliances[index] = Number(next() % 4 === 0);
        const globals = { players, options, alliances, battle: sample % 10 !== 0, missionPost: sample % 11 === 0, boss: (sample & 1) !== 0, scriptBreak: Number(sample % 13 === 0), loginEnd: Number(sample % 7 === 0) };
        let trace = 0; const hooks = { serverConnection: () => { trace = (Math.imul(trace, 33) + 10) >>> 0; return sample % 9 !== 0; }, priestDead: (owner) => { trace = (Math.imul(trace, 33) + owner) >>> 0; return dead[owner]; } };
        const gate = new RawBattleCompletion(globals, hooks);
        const values = [gate.allAllied(), gate.teamFinished(), gate.flagFinished(0), gate.flagFinished(1), gate.finished(), gate.unloggedFinished(sample % 8 + 1)];
        rows.push(`${sample} ${values.map(Number).join(' ')} ${trace}\n`);
    }
    assert.equal(createHash('sha256').update(rows.join('')).digest('hex'), 'd68863e7b9725c579d1862e83fe50d30fa4dd17aa402605f7634be50e7b560f4');
});

// 일반 프레임이 공유하는 변경 가능한 결과와 시간을 준비한다.
function frameScene() {
    const s = scene(), events = [];
    const frame = { players: s.globals.players, server: false, phase: 14, localOwner: 1, captureEnd: 0, drawDeadline: 0, settleFrames: 5, result: 99, gameSeconds: 10 };
    const hooks = { wallSeconds: () => { events.push('wall'); return 10; }, assertion: (...args) => events.push(['assert', ...args]), currentDialog: () => { events.push('dialog'); return 0; }, deleteSavedState: () => events.push('delete'), rejectDraw: () => events.push('reject'), queueMainMenu: () => events.push('menu') };
    return { ...s, frame, frameHooks: hooks, events };
}

// 비전투에서도 진단은 실행되고 대기 BYTE는 signed 비교 뒤 감긴다.
test('전투 종료 프레임: 진단·캡처 기한·signed 대기', () => {
    const { globals, write, gate, frame, frameHooks, events, calls } = frameScene();
    globals.battle = false; write(1, 0x7c, 12);
    gate.frame(frame, frameHooks);
    assert.deepEqual(events, [['assert', 'p->firstTempleSid == p->debugFirstTempleSid', '.\\Dais.cpp', 0x69e]]); assert.deepEqual(calls, []);
    write(1, 0x80, 12); globals.battle = true; events.length = 0; frame.captureEnd = 9; frame.settleFrames = 255;
    gate.frame(frame, frameHooks);
    assert.equal(frame.captureEnd, 0); assert.equal(frame.settleFrames, 0); assert.deepEqual(events, ['wall']); assert.deepEqual(calls, ['server']);
});

// 팀 종료의 확정은 현재 무승부 비트 제거 뒤 저장 삭제·결과·메뉴 순서다.
test('전투 종료 프레임: 팀 승리와 모달 차단', () => {
    const { globals: g, write, gate, frame, frameHooks, events } = frameScene();
    write(1, 0x2c, 1); write(2, 0x2c, 2); write(1, 0x50, 0x181); write(2, 0x50, 0x81);
    g.alliances[1 * 9 + 2] = 1; g.alliances[2 * 9 + 1] = 1;
    frameHooks.currentDialog = () => 1; gate.frame(frame, frameHooks);
    assert.equal(frame.result, 99); assert.equal(frame.settleFrames, 5); assert.deepEqual(events, []);
    frameHooks.currentDialog = () => { events.push('dialog'); return 0; }; gate.frame(frame, frameHooks);
    const view = new DataView(g.players.buffer); assert.equal(view.getUint32(180 + 0x50, true), 0x101); assert.equal(view.getUint32(360 + 0x50, true), 1);
    assert.equal(frame.result, 1); assert.equal(frame.settleFrames, 0); assert.deepEqual(events, ['dialog', 'delete', 'menu']);
});

// 결과가 나지 않은 게임의 무승부 기한은 NaN에서 발동하지 않고 만료 시 거절한다.
test('전투 종료 프레임: 무승부 기한과 거절 순서', () => {
    const { globals: g, write, gate, frame, frameHooks, events } = frameScene();
    write(1, 0x2c, 1); write(2, 0x2c, 2); write(1, 0x50, 1); write(2, 0x50, 1); g.boss = false;
    frame.drawDeadline = Number.NaN; gate.frame(frame, frameHooks); assert.ok(Number.isNaN(frame.drawDeadline)); assert.deepEqual(events, []);
    frame.drawDeadline = 10; gate.frame(frame, frameHooks);
    assert.equal(frame.drawDeadline, 0); assert.deepEqual(events, [['assert', 'boss', '.\\Dais.cpp', 0x70e], 'reject']);
});

// cpppj 프레임을 실행한 4096개 상태의 결과·flags·효과 순서 해시와 대조한다.
test('전투 종료 프레임: cpppj 합성 상태 4096건과 효과 순서', () => {
    const rows = [];
    // 원본 관측 도구와 같은 seed로 입력과 현재 장치를 준비한다.
    for (let sample = 0; sample < 4096; sample += 1) {
        let random = sample + 0x14b37, trace = 0;
        // DWORD 난수와 효과 순서의 감김을 보존한다.
        const next = () => { random = (Math.imul(random, 1664525) + 1013904223) >>> 0; return random >>> 16; };
        const mark = (value) => { trace = (Math.imul(trace, 33) + value) >>> 0; };
        const players = new Uint8Array(9 * 180), options = new Uint8Array(87), alliances = new Uint32Array(81), dead = [];
        // 같은 순서로 각 Player의 현재 기록을 생성한다.
        for (let owner = 1; owner < 9; owner += 1) {
            players[owner * 180 + 0x2c] = next() % 4 !== 0 ? owner : 0;
            players[owner * 180 + 0x50] = (next() % 2 !== 0 ? 1 : 0) | (next() % 2 !== 0 ? 0x80 : 0);
            options[0x3e + owner] = Number(next() % 5 === 0); dead[owner] = next() % 2 !== 0;
        }
        // 방향별 동맹과 일부 진단 불일치를 같은 위치에 설정한다.
        for (let index = 0; index < 81; index += 1) alliances[index] = Number(next() % 4 === 0);
        if (sample % 17 === 0) players[180 + 0x7c] = 1;
        const globals = { players, options, alliances, battle: sample % 10 !== 0, missionPost: sample % 11 === 0, boss: (sample & 1) !== 0, scriptBreak: Number(sample % 13 === 0), loginEnd: Number(sample % 7 === 0) };
        const hooks = { serverConnection: () => { mark(10); return sample % 9 !== 0; }, priestDead: (owner) => { mark(owner); return dead[owner]; } };
        const frame = { players, server: sample % 3 === 0, phase: sample % 8 === 0 ? 4 : 14, localOwner: sample % 8 + 1, captureEnd: sample % 5 === 0 ? 8 : 0, drawDeadline: sample % 7 === 0 ? 5 : 0, settleFrames: sample % 4 === 0 ? 255 : sample % 4 === 1 ? 4 : 5, result: 99, gameSeconds: 10 };
        const devices = { wallSeconds: () => { mark(30); return 10; }, assertion: (_expression, _file, line) => mark(line), currentDialog: () => { mark(31); return Number(sample % 7 === 1); }, deleteSavedState: () => mark(32), rejectDraw: () => mark(33), queueMainMenu: () => mark(34) };
        new RawBattleCompletion(globals, hooks).frame(frame, devices);
        const flags = [];
        // 처리 뒤 실제 Player flags BYTE를 관측한다.
        for (let owner = 1; owner < 9; owner += 1) flags.push(players[owner * 180 + 0x50]);
        rows.push(`${sample} ${frame.result} ${frame.settleFrames} ${frame.captureEnd} ${frame.drawDeadline} ${trace} ${flags.join(' ')}\n`);
    }
    assert.equal(createHash('sha256').update(rows.join('')).digest('hex'), '966e5ea23cdda8a3e888db611a89f38d4e17fc4460391b9d4798acc84355b7d2');
});
