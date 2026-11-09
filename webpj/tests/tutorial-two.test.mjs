// Secret Workshop(튜토리얼 2) 단계 판정기 raw-tutorial-two.js(PATCH 004c3bb0)를 원본 10.78 관측과 cpppj `TutorialTwoTests` 계약으로 대조한다.
//  - 원본 관측 tutorial-two-x86.tsv 346행(Frame 몸체): 같은 명시 입력(사건·게임 시각 double 비트·정지 깊이·재진입 변이·미션 게이트 14필드·제작 집계 셋·현재 선택·생산 포함·Tell 반환·옵션 8 BYTE·회수 금지·허용 표)에서
//    전체 공유 상태 14필드·정지 깊이·외부 조회/효과 순서(`R`·`S`·`C71`·`Z`·`X`·`Tell`·`K`·`Q`·`P`)·반환값·옵션 8 BYTE·회수 금지·허용 표·현재 선택이 모두 같다.
//    Advance/Pause 는 E231 의 실제 `RawMissionFrameGate`, 허용/제작 집계는 E237 의 실제 `RawTotalMade` 를 쓴다.
//  - 계약: 필수 하위 경계 누락·다른 판본 자료는 실제 효과 전에 거부한다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-10 WB04-E238: Secret Workshop 단계 판정기 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidPostPopState } from '../www/core/squid-postpop-state.js';
import { RawMissionFrameGate, createMissionFrameGateState } from '../www/core/raw-mission-frame-gate.js';
import { RawTotalMade } from '../www/core/raw-total-made.js';
import { RawTutorialTwo } from '../www/core/raw-tutorial-two.js';

// 저장된 독립 기계어 출력을 한 번만 읽는다(27칸). (2026-11-10 WB04-E238)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'tutorial-two-x86.tsv'));
// 중복 제거한 새 PATCH 독립 입력 수이다. (2026-11-10 WB04-E238)
const CASES = 346;

// 비트 변환용 공유 뷰이다. (2026-11-10 WB04-E238)
const view = new DataView(new ArrayBuffer(8));
const doubleFromBits = (text) => { view.setBigUint64(0, BigInt(text), true); return view.getFloat64(0, true); };
const doubleBits = (value) => { view.setFloat64(0, value, true); return view.getBigUint64(0, true).toString(); };
const floatFromBits = (text) => { view.setUint32(0, Number(text) >>> 0, true); return view.getFloat32(0, true); };
const floatBits = (value) => { view.setFloat32(0, value, true); return String(view.getUint32(0, true)); };
const signed = (text) => Number(BigInt.asIntN(32, BigInt(text)));
// 원본 옵션의 16진수 BYTE 를 변환한다(주위 옵션도 보존). (2026-11-10 WB04-E238)
const bytesOf = (text) => Uint8Array.from((text.match(/../g) ?? []).map((pair) => parseInt(pair, 16)));
const hexOf = (bytes) => Array.from(bytes, (value) => value.toString(16).padStart(2, '0')).join('');

// fixture 의 14필드 입력을 그대로 공통 게이트 상태에 전달한다(기대 상태는 계산하지 않는다). (2026-11-10 WB04-E238)
function gateState(text) {
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
// 실제 14필드의 현재 비트를 원본과 같은 순서로 기록한다. (2026-11-10 WB04-E238)
function gateSnapshot(state) {
    return [
        String(state.stage >>> 0), String(state.stageDirty), String(state.tutorialNumber), String(state.demo), doubleBits(state.startSeconds), doubleBits(state.nextStageSeconds),
        floatBits(state.delaySeconds), String(state.lockFrames >>> 0), String(state.started), doubleBits(state.demoEndSeconds), doubleBits(state.timerSeconds),
        doubleBits(state.tellAtSeconds), String(state.scriptPaused), String(state.clearFrames >>> 0),
    ].join(',');
}

// 실제 원본 입력/관찰 경계만 제공한다. 단계 판단과 Advance/Pause/TotalMade 는 제품 몸체를 쓴다. (2026-11-10 WB04-E238)
class TwoEnvironment {
    constructor(mission, counts) {
        this.mission = mission; // Tutorial2 와 게이트가 공유하는 유일한 단계/타이머이다.
        this.counts = counts; // 원본과 같은 postPop 의 누적 제작 표이다.
        this.depth = 0;
        this.selected = 0;
        this.mutation = 0; // 명시 외부 정지 깊이/현재 선택/재진입 입력이다.
        this.now = 0; // 실제 기계어에 입력한 같은 double 게임 시각이다.
        this.production = false;
        this.tellResult = false; // 현재 생산 목록/NotVortex 창 생성의 명시 반환 입력이다.
        this.trace = []; // 외부 조회/효과의 실제 호출 순서이다.
        this.changed = new Set(); // 명시 외부 효과의 첫 호출만 입력을 바꾸는 장부이다.
    }

    // 선택한 외부 효과 뒤 공유 입력을 바꾸며 게이트/Tutorial2 의 분기를 복제하지 않는다. (2026-11-10 WB04-E238)
    effect(kind) {
        this.trace.push(kind);
        if (this.changed.has(kind)) return;
        this.changed.add(kind);
        if (this.mutation === 1 && kind === 'R') this.counts.localSecondaryCounts()[123] = 1;
        else if (this.mutation === 2 && kind === 'Tell') this.production = true;
        else if (this.mutation === 3 && kind === 'Z') this.mission.timerSeconds = 99;
        else if (this.mutation === 4 && kind === 'K') { this.depth = 3; this.mission.scriptPaused = 7; }
    }

    // 공통 프레임 자체를 호출하지 않는 범위의 큰 미션 하위는 명시 예외로 남긴다. (2026-11-10 WB04-E238)
    gateHooks() {
        const fail = (message) => () => { throw new Error(message); };
        return {
            gameSeconds: () => this.now,
            paused: () => { this.effect('Q'); return this.depth !== 0; },
            pause: () => { this.effect('P'); this.depth = (this.depth + 1) >>> 0; },
            resume: fail('Tutorial2 직접 판정은 Resume 미실행'),
            checkOutcome: fail('Tutorial2 직접 판정의 다른 미션 하위 미실행'), tellStage: fail('Tutorial2 직접 판정의 다른 미션 하위 미실행'),
            tellAt: fail('Tutorial2 직접 판정의 다른 미션 하위 미실행'), endDemo: fail('Tutorial2 직접 판정의 다른 미션 하위 미실행'),
            dialogsClear: fail('Tutorial2 직접 판정의 프레임 UI 미실행'), knowledgeOpen: fail('Tutorial2 직접 판정의 프레임 UI 미실행'),
            guideOpen: fail('Tutorial2 직접 판정의 프레임 UI 미실행'), blockingGumpOpen: fail('Tutorial2 직접 판정의 프레임 UI 미실행'),
            tutorial: fail('Tutorial2 직접 판정의 중복 분배 미실행'),
        };
    }

    // 원본 실제 UI/생산/옵션 하위의 명시 인자/반환/순서만 공급한다. (2026-11-10 WB04-E238)
    hooks() {
        return {
            gameSeconds: () => this.now,
            refreshEnergyRange: () => { this.effect('R'); },
            selected: () => { this.effect('S'); return this.selected; },
            productionContains: (type) => { assert.equal(type, 71); this.effect('C71'); return this.production; },
            resetSelection: () => { this.effect('Z'); this.selected = 0; },
            clearSelection: () => { this.effect('X'); this.selected = 0; },
            tell: (section) => { assert.equal(section, 'NotVortex'); this.effect('Tell'); return this.tellResult; },
            closeTransientGumps: () => { this.effect('K'); },
        };
    }
}

test('Secret Workshop 단계 판정기: 원본 10.78 346행의 공유 상태·옵션·금지·허용·선택·정지·효과 순서·반환이 모두 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    let count = 0;
    for (const row of rows) {
        assert.equal(row.length, 27);
        assert.equal(row[0], 'Frame');
        count += 1;
        const pool = new SidPool(32768);
        const types = Array.from({ length: 188 }, () => ({ flags2: 0 }));
        const counts = new SquidPostPopState();
        types[71].flags2 = 0x20;
        types[85].flags2 = 0x4000;
        types[123].flags2 = 0x200;
        pool.physical(1)[10] = 71;
        pool.physical(2)[10] = 123;
        counts.localSecondaryCounts()[123] = Number(row[10]);
        counts.localSecondaryCounts()[85] = Number(row[11]);
        counts.localSecondaryCounts()[71] = Number(row[12]);
        const restrictions = new Uint32Array(256);
        restrictions[85] = Number(row[18]);
        const made = new RawTotalMade(types, counts, restrictions);
        const mission = gateState(row[9]);
        const environment = new TwoEnvironment(mission, counts);
        environment.depth = Number(row[3]) >>> 0;
        environment.selected = Number(row[13]);
        environment.now = doubleFromBits(row[2]);
        environment.production = Number(row[14]) !== 0;
        environment.tellResult = Number(row[15]) !== 0;
        environment.mutation = Number(row[8]);
        const gate = new RawMissionFrameGate(mission, environment.gateHooks());
        const options = bytesOf(row[16]);
        const state = { battleOptions: options, denySalvage: Number(row[17]), workshopType: 85, archerType: 71, salvageCommand: 0x1230000 };
        const body = new RawTutorialTwo(pool, types, made, gate, state, environment.hooks());
        const result = body.handle(Number(row[1]));
        const label = `행 ${count}`;
        assert.equal(result, Number(row[22]), `${label} 반환`);
        assert.equal(gateSnapshot(mission), row[19], `${label} 게이트`);
        assert.equal(environment.depth, Number(row[20]), `${label} 정지 깊이`);
        assert.equal(environment.trace.join(','), row[21], `${label} 효과 순서`);
        assert.equal(hexOf(options), hexOf(bytesOf(row[23])), `${label} 옵션`);
        assert.equal(state.denySalvage, Number(row[24]), `${label} 회수 금지`);
        assert.equal(restrictions[85], Number(row[25]), `${label} 허용 표`);
        assert.equal(environment.selected, Number(row[26]), `${label} 선택`);
    }
    assert.equal(count, CASES);
});

test('Secret Workshop 단계 판정기 계약: 필수 UI/생산/옵션 경계가 없거나 다른 판본 자료이면 실제 효과 전에 거부한다', () => {
    const pool = new SidPool(32768);
    const types = Array.from({ length: 188 }, () => ({ flags2: 0 }));
    const counts = new SquidPostPopState();
    const restrictions = new Uint32Array(256);
    const made = new RawTotalMade(types, counts, restrictions);
    const mission = createMissionFrameGateState();
    const environment = new TwoEnvironment(mission, counts);
    const gate = new RawMissionFrameGate(mission, environment.gateHooks());
    const state = { battleOptions: new Uint8Array(8), denySalvage: 1, workshopType: 85, archerType: 71, salvageCommand: 0x1230000 };
    for (const name of Object.keys(environment.hooks())) assert.throws(() => new RawTutorialTwo(pool, types, made, gate, state, { ...environment.hooks(), [name]: undefined }), TypeError, name);
    assert.throws(() => new RawTutorialTwo(pool, types.slice(0, 171), made, gate, state, environment.hooks()), TypeError);
    assert.throws(() => new RawTutorialTwo(pool, types, made, gate, { ...state, battleOptions: new Uint8Array(4) }, environment.hooks()), TypeError);
    // 연결이 모두 있으면 만들어지고 알 수 없는 단계에서는 아무 효과도 없다.
    mission.stage = 0x5a;
    const body = new RawTutorialTwo(pool, types, made, gate, state, environment.hooks());
    assert.equal(body.handle(0), 0);
    assert.equal(environment.trace.length, 0);
    assert.equal(state.denySalvage, 1);
});

test('Secret Workshop 단계 판정기 계약: 회수 완료 명령은 modifier 상위 비트를 지운 값으로 비교하고 현재 선택 SID 의 TYPE 이 표 밖이면 거부한다', () => {
    const build = () => {
        const pool = new SidPool(32768);
        const types = Array.from({ length: 188 }, () => ({ flags2: 0 }));
        const counts = new SquidPostPopState();
        const made = new RawTotalMade(types, counts, new Uint32Array(256));
        const mission = createMissionFrameGateState();
        const environment = new TwoEnvironment(mission, counts);
        environment.now = 10;
        const gate = new RawMissionFrameGate(mission, environment.gateHooks());
        const state = { battleOptions: new Uint8Array(8), denySalvage: 5, workshopType: 85, archerType: 71, salvageCommand: 0x1230000 };
        return { pool, mission, environment, body: new RawTutorialTwo(pool, types, made, gate, state, environment.hooks()), state };
    };
    // 'H': 상위 비트(0x04000000)가 섞인 회수 완료 명령도 같은 명령으로 인정해 타이머를 지금 + 2초로 예약하고 회수 금지를 0 으로 쓴다.
    const hStage = build();
    hStage.mission.stage = 0x48;
    hStage.body.handle(0x1230000 | 0x04000000);
    assert.equal(hStage.mission.timerSeconds, 12);
    assert.equal(hStage.state.denySalvage, 0);
    // 다른 명령이면 타이머를 건드리지 않는다.
    const other = build();
    other.mission.stage = 0x48;
    other.body.handle(0x1230001);
    assert.equal(other.mission.timerSeconds, 0);
    // 'F': 현재 선택 SID 의 TYPE 바이트가 표 밖(200)이면 RangeError, 선택 SID 가 풀 밖이어도 RangeError.
    const range = build();
    range.mission.stage = 0x46;
    range.environment.selected = 3;
    range.pool.physical(3)[10] = 200;
    assert.throws(() => range.body.handle(0), RangeError);
    range.environment.selected = 40000;
    assert.throws(() => range.body.handle(0), RangeError);
});

