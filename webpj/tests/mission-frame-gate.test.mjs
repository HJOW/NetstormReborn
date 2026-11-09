// 미션 공통 프레임 문 raw-mission-frame-gate.js(RunFrame·Advance·PauseForDialog·Postpone, PATCH 004c34c0 외)를 원본 10.78 관측과 cpppj `MissionFrameGateTests` 계약으로 대조한다.
//  - 원본 관측 missionframe-x86.tsv 328행(Frame·Advance·Pause·Postpone 네 몸체): 같은 명시 입력(사건·게임 시각 double 비트·시계 정지 깊이·다이얼로그/지식/안내/막는 Gump 조회·재진입 변이·14개 상태 필드의 원본 비트)에서
//    전체 상태 14필드의 실제 비트·시계 정지 깊이·외부 조회/효과 순서(`Q,O,D,…,T<번호>`)·반환값이 한 비트도 다르지 않다.
//  - 계약: 모든 큰 하위 경계는 명시해야 하며 누락을 실행하지 않는 정상 미션으로 숨기지 않는다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-10 WB04-E231: 미션 공통 프레임 문 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';
import { RawMissionFrameGate, createMissionFrameGateState, pauseMissionFrameForDialog } from '../www/core/raw-mission-frame-gate.js';

// 저장된 독립 기계어 출력을 한 번만 읽는다(14칸). (2026-11-10 WB04-E231)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'missionframe-x86.tsv'));

// 비트 변환용 공유 뷰이다. (2026-11-10 WB04-E231)
const view = new DataView(new ArrayBuffer(8));
// 원본 입력의 IEEE double 비트(unsigned 64)를 수로 바꾼다. (2026-11-10 WB04-E231)
function doubleFromBits(text) { view.setBigUint64(0, BigInt(text), true); return view.getFloat64(0, true); }
// double 의 실제 비트를 unsigned 64 십진수로 쓴다. (2026-11-10 WB04-E231)
function doubleBits(value) { view.setFloat64(0, value, true); return view.getBigUint64(0, true).toString(); }
// 원본 입력의 float 비트(unsigned 32)를 수로 바꾼다. (2026-11-10 WB04-E231)
function floatFromBits(text) { view.setUint32(0, Number(text) >>> 0, true); return view.getFloat32(0, true); }
// float 의 실제 비트를 unsigned 32 십진수로 쓴다. (2026-11-10 WB04-E231)
function floatBits(value) { view.setFloat32(0, value, true); return String(view.getUint32(0, true)); }
// DWORD 비트를 signed 상태 필드로 되돌린다. (2026-11-10 WB04-E231)
const signed = (text) => Number(BigInt.asIntN(32, BigInt(text)));

// fixture 의 14필드를 실제 원본 상태로 구성한다(기대 결과는 계산하지 않는다). (2026-11-10 WB04-E231)
function missionState(text) {
    const fields = text.split(',');
    assert.equal(fields.length, 14);
    const state = createMissionFrameGateState();
    state.stage = signed(fields[0]);
    state.stageDirty = Number(fields[1]);
    state.tutorialNumber = Number(fields[2]);
    state.demo = Number(fields[3]);
    state.startSeconds = doubleFromBits(fields[4]);
    state.nextStageSeconds = doubleFromBits(fields[5]);
    state.delaySeconds = floatFromBits(fields[6]);
    state.lockFrames = signed(fields[7]);
    state.started = Number(fields[8]);
    state.demoEndSeconds = doubleFromBits(fields[9]);
    state.timerSeconds = doubleFromBits(fields[10]);
    state.tellAtSeconds = doubleFromBits(fields[11]);
    state.scriptPaused = Number(fields[12]);
    state.clearFrames = signed(fields[13]);
    return state;
}

// 전체 상태의 실제 비트를 native fixture 와 같은 순서로 읽는다. (2026-11-10 WB04-E231)
function snapshot(state) {
    return [
        String(state.stage >>> 0), String(state.stageDirty), String(state.tutorialNumber), String(state.demo), doubleBits(state.startSeconds), doubleBits(state.nextStageSeconds),
        floatBits(state.delaySeconds), String(state.lockFrames >>> 0), String(state.started), doubleBits(state.demoEndSeconds), doubleBits(state.timerSeconds),
        doubleBits(state.tellAtSeconds), String(state.scriptPaused), String(state.clearFrames >>> 0),
    ].join(',');
}

// 원본 실행기의 명시 외부 입력/효과만 공급한다. gate 분기 자체의 복제는 하지 않는다. (2026-11-10 WB04-E231)
class MissionFrameEnvironment {
    constructor(state) {
        this.state = state; // 처리기와 실제로 같은 단계/타이머/잠금 저장소이다.
        this.now = 0; // 독립 입력 게임 시각이다. 벽시각이나 합성 틱으로 변환하지 않는다.
        this.depth = 0;
        this.event = 0;
        this.mutation = 0; // 외부 정지 깊이·현재 사건·명시 재진입 입력이다.
        this.dialogs = false;
        this.knowledge = false;
        this.guide = false;
        this.blocking = false; // 실제 UI 조회의 명시 반환 입력이다.
        this.changed = new Set(); // 선택 외부 효과를 한 번만 적용하는 입력 장부이다.
        this.trace = []; // 원본과 비교할 외부 효과/조회 순서이다.
    }

    // 각 외부 효과 첫 호출에서만 원본 실행기와 동일한 입력 변경을 적용한다. (2026-11-10 WB04-E231)
    effect(kind) {
        this.trace.push(kind);
        if (this.changed.has(kind)) return;
        this.changed.add(kind);
        if (this.mutation === 1 && kind === 'O') { this.state.lockFrames = 0; this.state.tutorialNumber = 2; }
        else if (this.mutation === 2 && kind === 'S') { this.state.scriptPaused = 1; this.depth = (this.depth + 1) >>> 0; this.dialogs = false; }
        else if (this.mutation === 3 && kind === 'L') { this.state.lockFrames = 0; this.state.demo = 3; }
        else if (this.mutation === 4 && kind === 'D') this.state.lockFrames = 2;
        else if (this.mutation === 5 && kind === 'G') this.state.scriptPaused = 0;
        else if (this.mutation === 6 && kind === 'E') { this.state.demoEndSeconds = 100; this.state.demo = 0; }
    }

    // 실제 native 의 각 외부 함수 관찰 경계와 같은 이름/현재값을 제공한다. (2026-11-10 WB04-E231)
    hooks() {
        const env = this;
        return {
            gameSeconds: () => env.now,
            paused: () => { env.effect('Q'); return env.depth !== 0; },
            checkOutcome: () => { env.effect('O'); },
            dialogsClear: () => { env.effect('D'); return env.dialogs; },
            knowledgeOpen: () => { env.effect('K'); return env.knowledge; },
            guideOpen: () => { env.effect('G'); return env.guide; },
            blockingGumpOpen: () => { env.effect('B'); return env.blocking; },
            tellStage: () => { env.effect('S'); },
            tellAt: () => { env.effect('L'); },
            endDemo: () => { env.effect('E'); },
            tutorial: (number, current) => { assert.equal(current, env.event); env.effect(`T${number}`); return 0xa000 + number; },
            pause: () => { env.effect('P'); env.depth = (env.depth + 1) >>> 0; }, // 시계 정지 깊이는 unsigned DWORD 이다.
            resume: () => { env.effect('R'); env.depth = (env.depth - 1) >>> 0; },
        };
    }
}

test('미션 공통 프레임 문: 원본 10.78 328행(Frame·Advance·Pause·Postpone)의 전체 상태·정지 깊이·외부 조회/효과 순서·반환값이 모두 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    let count = 0;
    const unique = new Set();
    for (const fields of rows) {
        assert.equal(fields.length, 14);
        assert.ok(!unique.has(fields.slice(0, 13).join('\t')), '고유 입력');
        unique.add(fields.slice(0, 13).join('\t'));
        const state = missionState(fields[9]);
        const environment = new MissionFrameEnvironment(state);
        environment.event = Number(fields[1]);
        environment.now = doubleFromBits(fields[2]);
        environment.depth = Number(fields[3]);
        environment.dialogs = Number(fields[4]) !== 0;
        environment.knowledge = Number(fields[5]) !== 0;
        environment.guide = Number(fields[6]) !== 0;
        environment.blocking = Number(fields[7]) !== 0;
        environment.mutation = Number(fields[8]);
        const gate = new RawMissionFrameGate(state, environment.hooks());
        assert.equal(gate.state, state);
        let result = 0;
        if (fields[0] === 'Frame') result = gate.runFrame(environment.event);
        else if (fields[0] === 'Advance') gate.advance();
        else if (fields[0] === 'Pause') gate.pauseForDialog();
        else if (fields[0] === 'Postpone') gate.postpone();
        else assert.fail(`알 수 없는 미션 프레임 fixture 연산 ${fields[0]}`);
        count += 1;
        const label = `행 ${count} ${fields[0]}`;
        assert.equal(snapshot(state), fields[10], `${label} 상태`);
        assert.equal(environment.depth, Number(fields[11]), `${label} 깊이`);
        assert.equal(environment.trace.join(','), fields[12], `${label} trace`);
        assert.equal(result, Number(fields[13]), `${label} 반환`);
    }
    assert.equal(count, 328);
});

test('미션 공통 프레임 문 계약: 모든 큰 하위 경계를 명시해야 하며 누락을 정상 미션으로 숨기지 않는다', () => {
    const state = createMissionFrameGateState();
    const environment = new MissionFrameEnvironment(state);
    for (const name of Object.keys(environment.hooks())) {
        const hooks = { ...environment.hooks(), [name]: undefined };
        assert.throws(() => new RawMissionFrameGate(state, hooks), TypeError, name);
    }
    // 독립 자기 정지 함수는 도달한 장치만 요구한다.
    assert.equal(state.scriptPaused, 0);
});

test('미션 공통 프레임 문 계약: 초기 상태·승패 확인 시점의 stageDirty·Advance 반환·독립 자기 정지·null 상태 거부', () => {
    // 초기 상태는 원본 전역 초기값이다(단계 65·표식 1·지연 0.5·잠금 10, 나머지 0).
    const state = createMissionFrameGateState();
    assert.deepEqual(state, {
        stage: 65, stageDirty: 1, tutorialNumber: 0, demo: 0, startSeconds: 0, nextStageSeconds: 0, delaySeconds: 0.5, lockFrames: 10, started: 0,
        demoEndSeconds: 0, timerSeconds: 0, tellAtSeconds: 0, scriptPaused: 0, clearFrames: 0,
    });
    // 시작된 프레임은 승패/사제 사건을 확인하기 전에 stageDirty 를 0 으로 쓴다. 시작 전 프레임은 아무 조회도 하지 않는다.
    const environment = new MissionFrameEnvironment(state);
    let dirtyAtOutcome = -1;
    const hooks = { ...environment.hooks(), checkOutcome: () => { dirtyAtOutcome = state.stageDirty; } };
    const gate = new RawMissionFrameGate(state, hooks);
    assert.equal(gate.runFrame(), 0);
    assert.ok(dirtyAtOutcome === -1 && state.stageDirty === 1 && environment.trace.length === 0);
    state.started = 1;
    gate.runFrame();
    assert.equal(dirtyAtOutcome, 0);
    // Advance 는 예약했을 때만 true 이고, 시계 정지 중이거나 잠금이 남았으면 false 이며 상태를 바꾸지 않는다.
    const advancing = createMissionFrameGateState();
    advancing.lockFrames = 0;
    const idle = new MissionFrameEnvironment(advancing);
    idle.now = 3.25;
    const advancer = new RawMissionFrameGate(advancing, idle.hooks());
    assert.equal(advancer.advance(), true);
    assert.deepEqual([advancing.stage, advancing.lockFrames, advancing.nextStageSeconds, advancing.timerSeconds], [66, 10, 3.75, 0]);
    assert.equal(advancer.advance(), false);
    advancing.lockFrames = 0;
    idle.depth = 1;
    assert.equal(advancer.advance(), false);
    assert.equal(advancing.stage, 66);
    // 독립 자기 정지 함수: 정지 조회 누락은 TypeError, 이미 자기 정지를 보유하면 재진입을 생략한다.
    const own = createMissionFrameGateState();
    assert.throws(() => pauseMissionFrameForDialog(own, undefined, () => {}), TypeError);
    let pauses = 0;
    pauseMissionFrameForDialog(own, () => false, () => { pauses += 1; });
    pauseMissionFrameForDialog(own, () => true, () => { pauses += 1; });
    assert.ok(pauses === 1 && own.scriptPaused === 1);
    assert.throws(() => pauseMissionFrameForDialog(createMissionFrameGateState(), () => false, undefined), TypeError);
    assert.throws(() => new RawMissionFrameGate(null, environment.hooks()), TypeError);
});

