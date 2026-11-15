// 연결 감시 raw-connection-health.js(RawConnectionHealth, 원본 004b9010)를 cpppj 소스 계약으로 대조한다(WB04-E559).
// 원본 관측 fixture 가 없는 소단위이므로 상태별 조기 반환·활동 송신 간격·서버/클라이언트 지연 안내·자동 서버 이전 관문과 현재값 재조회·호출 순서·선택 장치 요구를 직접 단언한다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-15 WB04-E559)
import test from 'node:test';
import assert from 'node:assert/strict';
import { pollConnectionHealth } from '../www/core/raw-connection-health.js';

// 완전한 전역·장치 한 벌이다. 상태는 14(진행), 벽시각은 100 으로 고정하며 trace 에 호출 순서를 남긴다. (2026-11-15 WB04-E559)
function makeScene(overrides = {}) {
    const trace = [];
    const phase = new Uint32Array(36);
    phase[0] = 14;
    const players = new Uint8Array(9 * 180);
    const globals = {
        phase,
        heartbeatEnd: { value: 1000 },
        server: { value: false },
        localOwner: { value: 2 },
        autoRestart: { value: 1 },
        activity: { frames: new Uint32Array(22), wallSeconds: new Uint32Array(22) },
        players: () => { trace.push('players'); return players; },
        ...overrides.globals
    };
    const hooks = {
        connectionExists: (index) => { trace.push(`exists,${index}`); return true; },
        wallSeconds: () => { trace.push('wall'); return 100; },
        frameWallSeconds: () => { trace.push('frameWall'); return 100; },
        frameNumber: () => { trace.push('frame'); return 5000; },
        sendHeartbeat: (server) => trace.push(`send,${server}`),
        battleFinished: () => { trace.push('finished'); return false; },
        status: (kind, player) => trace.push(`status,${kind},${player}`),
        saveServerFailure: () => trace.push('save'),
        ...overrides.hooks
    };
    return { trace, phase, players, globals, hooks };
}
// 서버가 아닐 때의 연결 9 활동을 한 줄로 설정한다. wall 은 마지막 수신 벽시각, frames 는 마지막 수신 프레임이다. (2026-11-15 WB04-E559)
function setActivity(scene, id, wall, frame) {
    scene.globals.activity.wallSeconds[id] = wall >>> 0;
    scene.globals.activity.frames[id] = frame;
}

test('연결 확인 상태(29/33)에서 연결 11 이 없으면 아무 효과 없이 반환한다', () => {
    for (const state of [29, 33]) {
        const scene = makeScene({ hooks: { connectionExists: (index) => { scene.trace.push(`exists,${index}`); return false; } } });
        scene.phase[0] = state;
        assert.equal(pollConnectionHealth(scene.globals, scene.hooks), false);
        assert.deepEqual(scene.trace, ['exists,11']);
        assert.equal(scene.globals.heartbeatEnd.value, 1000);
    }
    // 연결 11 이 있으면 계속하되 상태 14 가 아니므로 다음 활동 끝을 새 벽시각+1 로 두고 반환한다.
    const present = makeScene();
    present.phase[0] = 29;
    assert.equal(pollConnectionHealth(present.globals, present.hooks), false);
    assert.deepEqual(present.trace, ['exists,11', 'wall']);
    assert.equal(present.globals.heartbeatEnd.value, 101);
});

test('진행 상태(14)가 아니면 연결 확인 없이 다음 활동 끝을 벽시각+1.0 으로 둔다', () => {
    for (const state of [0, 13, 15, 28, 30, 32, 34]) {
        const scene = makeScene();
        scene.phase[0] = state;
        assert.equal(pollConnectionHealth(scene.globals, scene.hooks), false, `상태 ${state}`);
        assert.deepEqual(scene.trace, ['wall']);
        assert.equal(scene.globals.heartbeatEnd.value, 101);
    }
});

test('활동 송신: 끝이 지났을 때만 송신하고 송신 뒤 서버 권한을 다시 읽어 다음 간격을 새 벽시각에서 정한다', () => {
    // 아직 지나지 않았으면 송신도 끝 갱신도 없다.
    const waiting = makeScene({ globals: { heartbeatEnd: { value: 100.5 } } });
    pollConnectionHealth(waiting.globals, waiting.hooks);
    assert.equal(waiting.trace.includes('send,false'), false);
    assert.equal(waiting.globals.heartbeatEnd.value, 100.5);
    // 끝과 벽시각이 같으면(<=) 송신한다. 클라이언트는 10초 뒤이다.
    const due = makeScene({ globals: { heartbeatEnd: { value: 100 } } });
    pollConnectionHealth(due.globals, due.hooks);
    assert.deepEqual(due.trace.slice(0, 4), ['wall', 'send,false', 'wall', 'frameWall']);
    assert.equal(due.globals.heartbeatEnd.value, 110);
    // 서버는 3초 뒤이다.
    const server = makeScene({ globals: { heartbeatEnd: { value: 50 }, server: { value: true } } });
    pollConnectionHealth(server.globals, server.hooks);
    assert.equal(server.globals.heartbeatEnd.value, 103);
    assert.equal(server.trace.includes('send,true'), true);
    // 송신 인자는 송신 전 서버 권한이고, 간격은 송신 효과 뒤 권한으로 정한다.
    const flip = makeScene({ globals: { heartbeatEnd: { value: 50 } } });
    flip.hooks.sendHeartbeat = (value) => { flip.trace.push(`send,${value}`); flip.globals.server.value = true; };
    pollConnectionHealth(flip.globals, flip.hooks);
    assert.equal(flip.trace.includes('send,false'), true);
    assert.equal(flip.globals.heartbeatEnd.value, 103);
    // 다음 간격은 첫 조회가 아니라 새로 읽은 벽시각에서 계산한다.
    const clock = makeScene({ globals: { heartbeatEnd: { value: 50 } } });
    let tick = 100;
    clock.hooks.wallSeconds = () => { clock.trace.push('wall'); tick += 0.5; return tick; };
    pollConnectionHealth(clock.globals, clock.hooks);
    assert.equal(clock.globals.heartbeatEnd.value, 101 + 10);
});

test('서버: Player 1..8 의 현재 flags 1·15초 초과·활동 프레임·로컬 owner 아님·전투 종료 아님일 때만 종류 2 안내를 낸다', () => {
    const base = () => {
        const scene = makeScene({ globals: { server: { value: true }, heartbeatEnd: { value: 5000 } } });
        for (let player = 1; player < 9; player += 1) { scene.players[player * 180 + 0x50] = 1; setActivity(scene, player, 0, 7); }
        return scene;
    };
    // 활동 초 0 → 경과 100초이므로 로컬 owner(2) 를 뺀 1..8 이 모두 안내 대상이다. 매 반복 현재 Player 를 빌린다.
    const all = base();
    assert.equal(pollConnectionHealth(all.globals, all.hooks), false);
    const notices = all.trace.filter((entry) => entry.startsWith('status'));
    assert.deepEqual(notices, ['status,2,1', 'status,2,3', 'status,2,4', 'status,2,5', 'status,2,6', 'status,2,7', 'status,2,8']);
    assert.equal(all.trace.filter((entry) => entry === 'players').length, 8);
    // 조건별로 하나씩 막으면 그 Player 만 빠진다.
    const cases = [
        ['flags 1 없음', (scene) => { scene.players[4 * 180 + 0x50] = 0; }],
        ['flags 짝수', (scene) => { scene.players[4 * 180 + 0x50] = 2; }],
        ['경과 15초', (scene) => { setActivity(scene, 4, 85, 7); }],
        ['활동 프레임 0', (scene) => { setActivity(scene, 4, 0, 0); }]
    ];
    for (const [label, mutate] of cases) {
        const scene = base();
        mutate(scene);
        pollConnectionHealth(scene.globals, scene.hooks);
        assert.equal(scene.trace.includes('status,2,4'), false, label);
        assert.equal(scene.trace.includes('status,2,5'), true, label);
    }
    // 경과 16초는 안내한다. signed 활동 초(0xffffffff = -1)는 경과를 늘린다.
    const sixteen = base();
    setActivity(sixteen, 4, 84, 7);
    pollConnectionHealth(sixteen.globals, sixteen.hooks);
    assert.equal(sixteen.trace.includes('status,2,4'), true);
    const negative = base();
    setActivity(negative, 4, 0xffffffff, 7);
    negative.globals.activity.wallSeconds[5] = 0x7fffffff;
    pollConnectionHealth(negative.globals, negative.hooks);
    assert.equal(negative.trace.includes('status,2,4'), true);
    assert.equal(negative.trace.includes('status,2,5'), false);
    // 전투가 끝났으면 안내하지 않으며, 판정은 앞선 조건이 모두 참인 Player 에서만 부른다.
    const finished = base();
    finished.hooks.battleFinished = () => { finished.trace.push('finished'); return true; };
    pollConnectionHealth(finished.globals, finished.hooks);
    assert.equal(finished.trace.some((entry) => entry.startsWith('status')), false);
    assert.equal(finished.trace.filter((entry) => entry === 'finished').length, 7);
    // 서버는 연결 확인/자동 서버 이전을 하지 않고 항상 false 이다.
    assert.equal(all.trace.some((entry) => entry === 'save' || entry === 'exists,9'), false);
});

test('서버 Player 물리: 현재 9행이 아니면 첫 반복에서 거부한다', () => {
    const scene = makeScene({ globals: { server: { value: true }, heartbeatEnd: { value: 5000 } } });
    for (const size of [0, 9 * 180 - 1, 9 * 180 + 1]) {
        scene.globals.players = () => new Uint8Array(size);
        assert.throws(() => pollConnectionHealth(scene.globals, scene.hooks), RangeError);
    }
    assert.equal(scene.trace.some((entry) => entry.startsWith('status')), false);
});

test('클라이언트: 서버(9) 활동이 8초를 넘고 활동 프레임이 있고 전투가 끝나지 않았으면 종류 1 안내를 낸다', () => {
    const base = () => { const scene = makeScene({ globals: { heartbeatEnd: { value: 5000 } } }); setActivity(scene, 9, 91, 100); return scene; };
    const notice = base();
    setActivity(notice, 9, 91, 100);
    assert.equal(pollConnectionHealth(notice.globals, notice.hooks), false);
    assert.equal(notice.trace.filter((entry) => entry.startsWith('status')).length, 1);
    assert.equal(notice.trace.includes('status,1,0'), true);
    // 경과 8초(정확히)·프레임 0·전투 종료는 안내하지 않는다. 경과 9초부터 안내한다.
    const exact = base();
    setActivity(exact, 9, 92, 100);
    pollConnectionHealth(exact.globals, exact.hooks);
    assert.equal(exact.trace.some((entry) => entry.startsWith('status')), false);
    const nine = base();
    setActivity(nine, 9, 91, 100);
    pollConnectionHealth(nine.globals, nine.hooks);
    assert.equal(nine.trace.includes('status,1,0'), true);
    const noFrame = base();
    setActivity(noFrame, 9, 0, 0);
    pollConnectionHealth(noFrame.globals, noFrame.hooks);
    assert.equal(noFrame.trace.some((entry) => entry.startsWith('status')), false);
    const finished = base();
    finished.hooks.battleFinished = () => true;
    pollConnectionHealth(finished.globals, finished.hooks);
    assert.equal(finished.trace.some((entry) => entry.startsWith('status')), false);
    // 서버 이전 중(33번 칸)이면 이 블록 전체를 건너뛰어 연결 9 도 조회하지 않는다.
    const migrating = base();
    migrating.phase[33] = 1;
    assert.equal(pollConnectionHealth(migrating.globals, migrating.hooks), false);
    assert.equal(migrating.trace.some((entry) => entry === 'exists,9' || entry.startsWith('status')), false);
});

test('자동 서버 이전: 연결 9 가 없거나(20초 초과·프레임 차 20 초과) 활동 프레임이 있고 자동 재시작이면 저장 뒤 true 이다', () => {
    const base = () => { const scene = makeScene({ globals: { heartbeatEnd: { value: 5000 } } }); return scene; };
    // 연결 9 가 없으면 활동 프레임이 있고 자동 재시작일 때 저장하고 true.
    const missing = base();
    missing.hooks.connectionExists = (index) => { missing.trace.push(`exists,${index}`); return false; };
    setActivity(missing, 9, 100, 4);
    assert.equal(pollConnectionHealth(missing.globals, missing.hooks), true);
    assert.equal(missing.trace.at(-1), 'save');
    assert.equal(missing.trace.includes('frame'), false);
    // 활동 프레임 0 이거나 자동 재시작 0 이면 저장하지 않는다.
    const noFrame = base();
    noFrame.hooks.connectionExists = () => false;
    setActivity(noFrame, 9, 100, 0);
    assert.equal(pollConnectionHealth(noFrame.globals, noFrame.hooks), false);
    assert.equal(noFrame.trace.includes('save'), false);
    const noAuto = base();
    noAuto.hooks.connectionExists = () => false;
    noAuto.globals.autoRestart = { value: 0 };
    setActivity(noAuto, 9, 100, 4);
    assert.equal(pollConnectionHealth(noAuto.globals, noAuto.hooks), false);
    assert.equal(noAuto.trace.includes('save'), false);
    // 연결 9 가 있으면 경과 20초 초과(정확히 20 은 아님)와 unsigned 프레임 차 20 초과가 모두 필요하다.
    const gapCases = [
        [79, 4980, false],
        [79, 4979, true],
        [80, 4979, false],
        [79, 5000, false],
        [79, 5001, true]
    ];
    for (const [wall, frame, expected] of gapCases) {
        const scene = base();
        setActivity(scene, 9, wall, frame);
        assert.equal(pollConnectionHealth(scene.globals, scene.hooks), expected, `wall ${wall} frame ${frame}`);
        assert.equal(scene.trace.includes('save'), expected);
    }
    // 종류 1 안내와 저장은 한 번의 호출에서 둘 다 일어날 수 있다.
    const both = base();
    both.hooks.connectionExists = () => false;
    setActivity(both, 9, 50, 4);
    assert.equal(pollConnectionHealth(both.globals, both.hooks), true);
    assert.equal(both.trace.indexOf('status,1,0') < both.trace.indexOf('save'), true);
});

test('캡처 순서: 활동 송신 뒤 현재 owner 와 프레임 벽시각을 이 순서로 읽고 활동 초는 프레임 벽시각 기준이다', () => {
    const scene = makeScene({ globals: { server: { value: true }, heartbeatEnd: { value: 5000 } } });
    for (let player = 1; player < 9; player += 1) { scene.players[player * 180 + 0x50] = 1; setActivity(scene, player, 0, 7); }
    // 새 벽시각이 아니라 프레임에 고정된 벽시각(frameWall)으로 경과를 계산한다.
    scene.hooks.frameWallSeconds = () => { scene.trace.push('frameWall'); return 14; };
    pollConnectionHealth(scene.globals, scene.hooks);
    assert.equal(scene.trace.some((entry) => entry.startsWith('status')), false);
    // 로컬 owner 는 프레임 벽시각과 같은 시점에 한 번 캡처한다(반복 중 바뀌어도 첫 값을 쓴다).
    const owner = makeScene({ globals: { server: { value: true }, heartbeatEnd: { value: 5000 } } });
    for (let player = 1; player < 9; player += 1) { owner.players[player * 180 + 0x50] = 1; setActivity(owner, player, 0, 7); }
    owner.hooks.battleFinished = () => { owner.globals.localOwner.value = 4; return false; };
    pollConnectionHealth(owner.globals, owner.hooks);
    assert.equal(owner.trace.includes('status,2,2'), false);
    assert.equal(owner.trace.includes('status,2,4'), true);
});

test('선택된 장치만 요구하고 누락은 명시 오류이며 자연 반환 가지는 사용하지 않는 장치를 요구하지 않는다', () => {
    // 상태 14 가 아니면 벽시각 장치만 필요하다.
    const idle = makeScene({ hooks: { connectionExists: undefined, frameWallSeconds: undefined, frameNumber: undefined, sendHeartbeat: undefined, battleFinished: undefined, status: undefined, saveServerFailure: undefined } });
    idle.phase[0] = 13;
    assert.doesNotThrow(() => pollConnectionHealth(idle.globals, idle.hooks));
    idle.hooks.wallSeconds = undefined;
    assert.throws(() => pollConnectionHealth(idle.globals, idle.hooks), /미연결/);
    // 연결 확인 상태에서 연결 조회 장치가 없다.
    const noExists = makeScene({ hooks: { connectionExists: undefined } });
    noExists.phase[0] = 29;
    assert.throws(() => pollConnectionHealth(noExists.globals, noExists.hooks), /미연결/);
    // 서버 Player 순회에서 안내/종료 판정 장치가 필요할 때만 요구한다(조건이 모두 거짓이면 불요).
    const quiet = makeScene({ globals: { server: { value: true }, heartbeatEnd: { value: 5000 } }, hooks: { battleFinished: undefined, status: undefined } });
    assert.doesNotThrow(() => pollConnectionHealth(quiet.globals, quiet.hooks));
    const noFinish = makeScene({ globals: { server: { value: true }, heartbeatEnd: { value: 5000 } }, hooks: { battleFinished: undefined } });
    noFinish.players[3 * 180 + 0x50] = 1;
    setActivity(noFinish, 3, 0, 7);
    assert.throws(() => pollConnectionHealth(noFinish.globals, noFinish.hooks), /미연결/);
    // 클라이언트의 자동 서버 이전에서 저장 장치와 프레임 장치가 필요하다.
    const noSave = makeScene({ globals: { heartbeatEnd: { value: 5000 } }, hooks: { saveServerFailure: undefined, connectionExists: () => false } });
    setActivity(noSave, 9, 100, 4);
    assert.throws(() => pollConnectionHealth(noSave.globals, noSave.hooks), /미연결/);
    const noFrame = makeScene({ globals: { heartbeatEnd: { value: 5000 } }, hooks: { frameNumber: undefined } });
    setActivity(noFrame, 9, 79, 4);
    assert.throws(() => pollConnectionHealth(noFrame.globals, noFrame.hooks), /미연결/);
    // 서버 Player 조회 장치 자체가 없으면 서버 순회의 첫 반복에서 거부한다.
    const noPlayers = makeScene({ globals: { server: { value: true }, heartbeatEnd: { value: 5000 }, players: undefined } });
    assert.throws(() => pollConnectionHealth(noPlayers.globals, noPlayers.hooks), /미연결/);
});

test('읽기 전용 전역은 상자·함수·단순 값으로 받는다', () => {
    const scene = makeScene({ globals: { server: () => true, localOwner: 2, autoRestart: () => 1, heartbeatEnd: { value: 5000 } } });
    for (let player = 1; player < 9; player += 1) { scene.players[player * 180 + 0x50] = 1; setActivity(scene, player, 0, 7); }
    pollConnectionHealth(scene.globals, scene.hooks);
    assert.equal(scene.trace.includes('status,2,2'), false);
    assert.equal(scene.trace.includes('status,2,1'), true);
    const client = makeScene({ globals: { server: false, autoRestart: 1, heartbeatEnd: { value: 5000 } }, hooks: { connectionExists: () => false } });
    setActivity(client, 9, 100, 4);
    assert.equal(pollConnectionHealth(client.globals, client.hooks), true);
});

// 읽기 전용 전역을 함수로 줄 때 반환값(거짓·다른 값 포함)을 읽는다. (2026-11-15 WB04-E559 변형 확인에서 추가)
test('읽기 전용 전역의 함수 형태는 반환값을 읽고 연결 9 조회는 번호 9 를 쓴다', () => {
    // 서버 함수가 거짓이면 클라이언트 경로이다(자동 재시작 함수가 0 이면 저장하지 않는다).
    const noAuto = makeScene({ globals: { server: () => false, autoRestart: () => 0, heartbeatEnd: { value: 5000 } }, hooks: { connectionExists: (index) => { noAuto.trace.push(`exists,${index}`); return false; } } });
    setActivity(noAuto, 9, 100, 4);
    assert.equal(pollConnectionHealth(noAuto.globals, noAuto.hooks), false);
    assert.equal(noAuto.trace.includes('exists,9'), true);
    assert.equal(noAuto.trace.includes('exists,11'), false);
    assert.equal(noAuto.trace.includes('save'), false);
    // 로컬 owner 함수의 값이 서버 순회의 제외 대상이다.
    const owner = makeScene({ globals: { server: () => true, localOwner: () => 3, heartbeatEnd: { value: 5000 } } });
    for (let player = 1; player < 9; player += 1) { owner.players[player * 180 + 0x50] = 1; setActivity(owner, player, 0, 7); }
    pollConnectionHealth(owner.globals, owner.hooks);
    assert.equal(owner.trace.includes('status,2,3'), false);
    assert.equal(owner.trace.includes('status,2,2'), true);
    // 연결 9 가 없을 때 저장하며 번호 9 로 조회한다(연결 11 이 아니다).
    const save = makeScene({ globals: { heartbeatEnd: { value: 5000 } }, hooks: { connectionExists: (index) => { save.trace.push(`exists,${index}`); return index !== 9; } } });
    setActivity(save, 9, 100, 4);
    assert.equal(pollConnectionHealth(save.globals, save.hooks), true);
    assert.equal(save.trace.includes('exists,9'), true);
});
