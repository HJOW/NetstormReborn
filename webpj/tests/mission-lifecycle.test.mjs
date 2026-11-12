// Guide 의 Tutorial 가상 단계 0..6 분배와 월드 진입 Pause(mission-lifecycle.js, cpppj `client/MissionLifecycle`)·MissionFirst(mission-guide-input.js 첫 부분)를 원본 10.78 관측과 cpppj 시험, 웹 계약으로 확인한다.
//  - 원본 관측: mission-lifecycle-x86.tsv 831행(진입 Pause / 실제 Guide 반복 × flags/순번/현재/pending/창/Tell 결과/정지 깊이/자기 정지/종료 요청/반복 수/기한 비트)의 현재 활성·정지 깊이·미션 208BYTE·외부 호출 순서를 전부 대조한다.
//    실제 기존 몸체(RawMissionGuide·RawMissionFrameGate)와 새 단계 분배를 같은 상태에 연결한다.
//  - cpppj 시험 이식: MissionLifecycleTests 의 Native831 과 ActualPendingAndEmptyEntry 의 월드 진입 Pause 부분(현재 없음 무동작, 현재가 있는데 pause 없음 거부). 같은 시험의 client `State` 대기열 부분은 State 단위 이식 때 옮긴다.
//  - 웹 계약: 단계 0..6 각각의 효과 순서·flags 쓰기, 단계 5 의 성공 비트 선택과 팝업 닫기→자기 정지 순서, 효과 누락의 도달 위치 거부(단계 5 는 두 효과를 확인한 뒤 닫기), 범위 밖 단계, MissionFirst 의 순번 0 분기.
// 실제 게임·브라우저 실행이 아니다. (2026-11-12 WB04-E341)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { put, readRows } from './raw-support.mjs';
import { createMissionFrameGateState, RawMissionFrameGate, pauseMissionFrameForDialog } from '../www/core/raw-mission-frame-gate.js';
import { createMissionStartState } from '../www/core/raw-mission-start.js';
import { RawMissionGuide } from '../www/core/raw-mission-guide.js';
import { missionFirst } from '../www/core/mission-guide-input.js';
import { pauseMissionWorldEntry, runTutorialMissionStage } from '../www/core/mission-lifecycle.js';

// 원본 관측 행이다. 파일이 없으면 재생 검사를 건너뛴다. (2026-11-12 WB04-E341)
const ROWS = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'mission-lifecycle-x86.tsv'));
// 격리된 가상 표 번호이며 호스트 함수 주소가 아니다. (2026-11-12 WB04-E341)
const LIFECYCLE_TABLE = 0x12002000;

// 소문자 16진 문자열로 바꾼다. (2026-11-12 WB04-E341)
const hex = (bytes) => Array.from(bytes, (byte) => byte.toString(16).padStart(2, '0')).join('');
// 문자열의 각 글자를 한 바이트로 보는 ASCII 16진이다. (2026-11-12 WB04-E341)
const textHex = (text) => hex(Array.from(text, (ch) => ch.charCodeAt(0)));
// 예외가 나는지 본다. (2026-11-12 WB04-E341)
function throws(action, kind) {
    try { action(); } catch (error) { return kind ? error instanceof kind : true; }
    return false;
}
// 64비트 정수의 비트를 double 로 읽는다(원문 열은 u64 십진수라 BigInt 로 읽는다). (2026-11-12 WB04-E341)
function doubleOfBits(bits) {
    const view = new DataView(new ArrayBuffer(8));
    view.setBigUint64(0, bits, true);
    return view.getFloat64(0, true);
}

// 한 행의 12개 입력과 상태/관문/Guide 를 만들고 외부 효과를 기록하는 장면이다. cpppj LifecycleScene 에 대응한다. (2026-11-12 WB04-E341)
class LifecycleScene {
    // input = 연산 / flags / 순번 / 현재 / pending / 창 / Tell 결과 / 깊이 / 자기정지 / 종료요청 / 반복 / 기한 비트이다.
    constructor(values) {
        const fields = values.split(',').map((field) => BigInt(field));
        assert.equal(fields.length, 12);
        this.input = fields;
        this.state = createMissionStartState();
        this.gate = createMissionFrameGateState();
        this.state.flags = Number(fields[1]);
        this.state.sequence = Number(fields[2]);
        this.active = fields[3] !== 0n;
        this.depth = Number(fields[7]);
        this.finish = Number(fields[9]);
        this.gate.nextStageSeconds = doubleOfBits(fields[11]);
        this.gate.scriptPaused = Number(fields[8]);
        this.trace = '';
        this.frameGate = new RawMissionFrameGate(this.gate, this.frameHooks());
        this.guide = new RawMissionGuide(this.guideHooks());
    }

    // 외부 효과 한 번의 순서를 이어 붙인다.
    add(value) {
        if (this.trace) this.trace += ';';
        this.trace += value;
    }

    // 원본 현재 포인터 읽기마다 같은 관측을 남긴다.
    current() {
        this.add(this.active ? 'C,1' : 'C,0');
        return this.active ? this.state : null;
    }

    // 도달하지 않는 공통 프레임 경계는 성공으로 채우지 않고 명시 거부한다.
    frameHooks() {
        const unreachable = (text) => () => { throw new Error(text); };
        return {
            gameSeconds: () => 100.125,
            paused: () => { this.add('Q'); return this.depth !== 0; },
            pause: () => { this.add('H'); this.depth += 1; },
            resume: unreachable('수명 원문에서 미선택 공통 프레임 효과'),
            checkOutcome: unreachable('수명 원문에서 미선택 공통 프레임 효과'),
            tellStage: unreachable('수명 원문에서 미선택 공통 프레임 효과'),
            tellAt: unreachable('수명 원문에서 미선택 공통 프레임 효과'),
            endDemo: unreachable('수명 원문에서 미선택 공통 프레임 효과'),
            dialogsClear: unreachable('수명 원문에서 미선택 UI 조회'),
            knowledgeOpen: unreachable('수명 원문에서 미선택 UI 조회'),
            guideOpen: unreachable('수명 원문에서 미선택 UI 조회'),
            blockingGumpOpen: unreachable('수명 원문에서 미선택 UI 조회'),
            tutorial: unreachable('수명 원문에서 미선택 Early 판정'),
        };
    }

    // 실제 큰 Init/Start/Frame 의 명시 경계는 독립 원문과 같은 요청만 공급한다.
    stageHooks() {
        return {
            initialize: () => {
                this.add('I');
                this.state.flags = (this.state.flags | 1) >>> 0;
                pauseMissionWorldEntry(() => this.current() !== null, () => this.frameGate.pauseForDialog());
            },
            tellGlobal: (name, mode) => { assert.equal(mode, 1); this.add(`G,${textHex(name)}`); },
            start: () => { this.add('S'); this.gate.started = 1; RawMissionGuide.advance(this.state); },
            frame: () => {
                this.add('F');
                if (this.finish) { this.state.flags = (this.state.flags | 1) >>> 0; this.finish = 0; }
            },
            showStage: () => this.add('Show'),
            tellMission: (name) => { this.add(`T,${textHex(name)}`); return Number(this.input[6]); },
            closeTransient: () => this.add('K'),
            pause: () => this.frameGate.pauseForDialog(),
        };
    }

    // 원문 현재 포인터/관문/해제의 명시 입력을 같은 Guide 에 연결한다.
    guideHooks() {
        return {
            current: () => this.current(),
            pending: () => { this.add('P'); return Number(this.input[4]); },
            dialogsClear: () => { this.add('D'); return this.input[5] !== 0n; },
            stage: (target, sequence) => { assert.equal(target, this.state); runTutorialMissionStage(target, sequence, this.stageHooks()); },
            postpone: () => this.frameGate.postpone(),
            release: () => { this.add('L'); this.active = false; },
            invalidStage: () => this.add('A'),
        };
    }

    // 독립 실제 결과와 같은 208바이트/현재/정지/trace 를 직렬화한다.
    run() {
        // 입력이 지정한 별도 호출 수만큼 정상 Guide 또는 진입 Pause 를 실행한다.
        for (let call = 0n; call < this.input[10]; call += 1n) {
            if (this.input[0]) pauseMissionWorldEntry(() => this.current() !== null, () => this.frameGate.pauseForDialog());
            else this.guide.run();
        }
        const raw = new Uint8Array(0xd0);
        // 원본 준비의 무관한 stale 바이트도 유지한다.
        for (let index = 0; index < raw.length; index += 1) raw[index] = (index * 37 + 41) & 0xff;
        put(raw, 0, LIFECYCLE_TABLE);
        put(raw, 0x54, this.state.flags);
        put(raw, 0x58, this.state.sequence);
        put(raw, 0x80, 0x3f000000);
        put(raw, 0x88, this.gate.started);
        put(raw, 0xb8, this.gate.scriptPaused);
        const view = new DataView(new ArrayBuffer(8));
        view.setFloat64(0, this.gate.nextStageSeconds, true);
        // 기한의 낮은/높은 BYTE 를 모두 보존한다.
        for (let index = 0; index < 8; index += 1) raw[0x78 + index] = view.getUint8(index);
        return `${this.active ? 1 : 0}|${this.depth}|${hex(raw)}|${this.trace === '' ? '-' : this.trace}`;
    }
}

test('원본 관측 831행: 현재 활성·정지 깊이·미션 208BYTE·외부 호출 순서가 모두 일치한다', { skip: ROWS === null }, () => {
    assert.equal(ROWS.length, 831);
    // 기대값은 새로 합성하지 않고 최초 PE 결과를 그대로 사용한다.
    for (const row of ROWS) {
        assert.equal(row.length, 2);
        assert.equal(new LifecycleScene(row[0]).run(), row[1]);
    }
});

test('cpppj MissionLifecycle_ActualPendingAndEmptyEntry 의 월드 진입 Pause: 현재가 없으면 무동작이고 있는데 pause 가 없으면 거부한다', () => {
    pauseMissionWorldEntry(() => false, undefined);
    assert.ok(throws(() => pauseMissionWorldEntry(() => true, undefined)));
    assert.throws(() => pauseMissionWorldEntry(undefined, () => {}), /현재 Guide 조회 누락/u);
    assert.throws(() => pauseMissionWorldEntry(() => true, undefined), /Pause 연결 누락/u);
    // 현재 조회는 한 번만 하고 있을 때에만 pause 를 한 번 부른다.
    let reads = 0;
    let pauses = 0;
    pauseMissionWorldEntry(() => { reads += 1; return true; }, () => { pauses += 1; });
    assert.equal(reads, 1);
    assert.equal(pauses, 1);
});

// 효과 호출 순서를 기록하는 단순 단계 장면이다. (2026-11-12 WB04-E341)
function stageScene(flags, sequence = 0) {
    const state = { ...createMissionStartState(), flags, sequence };
    const log = [];
    const hooks = {
        initialize: () => log.push('I'),
        tellGlobal: (name, mode) => log.push(`G${name},${mode}`),
        start: () => log.push('S'),
        frame: () => log.push('F'),
        showStage: () => log.push('Show'),
        tellMission: (name) => { log.push(`T${name}`); return scene.shown; },
        closeTransient: () => log.push('K'),
        pause: () => log.push('P'),
    };
    const scene = { state, log, hooks, shown: 0 };
    return scene;
}

test('단계 0: 순번 0 이면 flags 값 1 만 요청하고 아니면 현재 안내를 다시 표시한다', () => {
    const first = stageScene(0x10, 0);
    runTutorialMissionStage(first.state, 0, first.hooks);
    assert.deepEqual(first.log, []);
    assert.equal(first.state.flags, 0x11);
    const later = stageScene(0x10, 3);
    runTutorialMissionStage(later.state, 0, later.hooks);
    assert.deepEqual(later.log, ['Show']);
    assert.equal(later.state.flags, 0x10);
    // 순번 0 은 안내 연결이 없어도 되지만 그 밖에는 거부한다.
    const bare = stageScene(0, 0);
    runTutorialMissionStage(bare.state, 0, {});
    assert.ok(throws(() => runTutorialMissionStage(stageScene(0, 1).state, 0, {})));
});

test('단계 1~4: 큰 몸체에 위임하고 단계 2 만 init Tell 뒤 flags 값 1 을 쓴다', () => {
    const scene = stageScene(0x20, 4);
    runTutorialMissionStage(scene.state, 1, scene.hooks);
    runTutorialMissionStage(scene.state, 3, scene.hooks);
    runTutorialMissionStage(scene.state, 4, scene.hooks);
    assert.deepEqual(scene.log, ['I', 'S', 'F']);
    assert.equal(scene.state.flags, 0x20);
    runTutorialMissionStage(scene.state, 2, scene.hooks);
    assert.deepEqual(scene.log, ['I', 'S', 'F', 'Ginit,1']);
    assert.equal(scene.state.flags, 0x21);
    assert.equal(scene.state.sequence, 4);
});

test('단계 5: 성공 비트(값 4)로 Success/Failure 를 고르고 Tell 이 비영이면 팝업 닫기 → 정지 뒤 flags 값 1 을 쓴다', () => {
    const win = stageScene(4);
    win.shown = 7;
    runTutorialMissionStage(win.state, 5, win.hooks);
    assert.deepEqual(win.log, ['TSuccess', 'K', 'P']);
    assert.equal(win.state.flags, 5);
    const lose = stageScene(0xfffffffb);
    lose.shown = 0;
    runTutorialMissionStage(lose.state, 5, lose.hooks);
    assert.deepEqual(lose.log, ['TFailure']);
    assert.equal(lose.state.flags, 0xfffffffb);
    // 다른 비트(8, 값 4 아님)만 있으면 실패이다.
    const other = stageScene(0xfb | 8);
    runTutorialMissionStage(other.state, 5, other.hooks);
    assert.deepEqual(other.log, ['TFailure']);
});

test('단계 6 은 무동작이고 범위 밖 단계는 거부한다', () => {
    const scene = stageScene(0x30, 6);
    runTutorialMissionStage(scene.state, 6, scene.hooks);
    assert.deepEqual(scene.log, []);
    assert.equal(scene.state.flags, 0x30);
    assert.ok(throws(() => runTutorialMissionStage(scene.state, 7, scene.hooks), RangeError));
    assert.ok(throws(() => runTutorialMissionStage(scene.state, -1, scene.hooks), RangeError));
});

test('선택된 단계가 요구하는 효과의 누락만 도달한 위치에서 거부한다', () => {
    const base = () => stageScene(0, 2);
    for (const [stage, name] of [[1, 'initialize'], [2, 'tellGlobal'], [3, 'start'], [4, 'frame'], [5, 'tellMission']]) {
        const scene = base();
        delete scene.hooks[name];
        assert.ok(throws(() => runTutorialMissionStage(scene.state, stage, scene.hooks)), `${stage}:${name}`);
        // 거부는 flags 를 바꾸지 않는다.
        assert.equal(scene.state.flags, 0);
    }
    // 단계 5 에서 Tell 이 0 이면 팝업/정지 효과가 없어도 된다.
    const quiet = base();
    quiet.shown = 0;
    delete quiet.hooks.closeTransient;
    delete quiet.hooks.pause;
    runTutorialMissionStage(quiet.state, 5, quiet.hooks);
    assert.equal(quiet.state.flags, 1);
    // Tell 이 비영이면 두 효과를 모두 확인한 뒤 닫기를 부르므로 정지가 없을 때 팝업도 닫지 않는다.
    const half = base();
    half.shown = 1;
    delete half.hooks.pause;
    assert.ok(throws(() => runTutorialMissionStage(half.state, 5, half.hooks)));
    assert.deepEqual(half.log, ['TFailure']);
    assert.equal(half.state.flags, 0);
    const noClose = base();
    noClose.shown = 1;
    delete noClose.hooks.closeTransient;
    assert.ok(throws(() => runTutorialMissionStage(noClose.state, 5, noClose.hooks)));
    assert.deepEqual(noClose.log, ['TFailure']);
});

test('MissionFirst: 순번 0 이면 안내 없이 flags 값 1 만 켜고 다른 필드는 보존한다', () => {
    const state = { ...createMissionStartState(), flags: 0x80000010, sequence: 0, tellAtIndex: 5, fieldC0: 6, fieldC4: 7 };
    let shown = 0;
    missionFirst(state, () => { shown += 1; });
    assert.equal(shown, 0);
    assert.deepEqual(state, { ...createMissionStartState(), flags: 0x80000011, sequence: 0, tellAtIndex: 5, fieldC0: 6, fieldC4: 7 });
    state.sequence = 1;
    missionFirst(state, () => { shown += 1; });
    assert.equal(shown, 1);
    assert.equal(state.flags, 0x80000011);
    assert.ok(throws(() => missionFirst(state, undefined)));
    assert.ok(throws(() => missionFirst(state, 5)));
});

test('자기 정지 도우미(pauseMissionFrameForDialog)와 장면의 정지 깊이가 같은 상태를 쓴다', () => {
    const gate = createMissionFrameGateState();
    let depth = 0;
    pauseMissionFrameForDialog(gate, () => depth !== 0, () => { depth += 1; });
    assert.equal(depth, 1);
    assert.equal(gate.scriptPaused, 1);
    // 이미 자기 정지가 있으면 재진입을 생략한다.
    pauseMissionFrameForDialog(gate, () => depth !== 0, () => { depth += 1; });
    assert.equal(depth, 1);
});
