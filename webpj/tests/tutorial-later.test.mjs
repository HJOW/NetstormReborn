// Early Missions 3~6 단계 판정기 raw-tutorial-later.js(PATCH 004c3f00/004c40f0/004c42b0/004c45e0)를 원본 10.78 관측과 cpppj `TutorialLaterTests` 계약으로 대조한다.
//  - 원본 관측 tutorial-later-x86.tsv 533개 고유 입력(미션 3~6): 30개 명시 입력(미션 번호·단계·사건 DWORD 셋·생산 목록·사제/Graph/지역/주변 반환·제작 집계·선택·타이머·정지 깊이·재진입 변이 등)에서
//    공통 게이트 14필드 비트·정지 깊이·현재 TYPE 재조회와 외부 하위 호출 순서·반환·옵션 8 BYTE·허용 표·선택·일회성 경고 표식 둘·현재 owner 가 한 비트도 다르지 않다.
//    Advance/Pause 는 E231 `RawMissionFrameGate`, 허용/제작 집계는 E237 `RawTotalMade`, 일회성 알림 필드는 E234 `createMissionStartState` 를 쓴다.
//  - 계약: 누락된 생산 하위/잘못된 입력은 단계 변경이나 옵션 쓰기 전에 명시 거부한다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-10 WB04-E239: 튜토리얼 3~6 단계 판정기 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidPostPopState } from '../www/core/squid-postpop-state.js';
import { RawMissionFrameGate, createMissionFrameGateState } from '../www/core/raw-mission-frame-gate.js';
import { createMissionStartState } from '../www/core/raw-mission-start.js';
import { RawTotalMade } from '../www/core/raw-total-made.js';
import { RawTutorialLater } from '../www/core/raw-tutorial-later.js';

// 저장된 독립 기계어 출력을 한 번만 읽는다(11칸). 첫 줄 머리는 건너뛴다. (2026-11-10 WB04-E239)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'tutorial-later-x86.tsv'));
const observations = rows ? rows.filter((row) => row[0] !== 'input') : null;
// 중복 제거한 독립 원본 입력 수이다. (2026-11-10 WB04-E239)
const INPUTS = 533;
// 원본 PE 에서 읽은 명시 타입 전역 입력(주소 → 현재 TYPE 번호)이다. (2026-11-10 WB04-E239)
const TYPE_GLOBALS = new Map([[0x541174, 71], [0x541178, 72], [0x541184, 75], [0x541188, 76], [0x54118c, 77], [0x541190, 78], [0x541244, 123], [0x541258, 128], [0x541264, 131], [0x541268, 132],
    [0x54126c, 133], [0x541288, 140], [0x54128c, 141], [0x5412ec, 165]]);

// 비트 변환용 공유 뷰이다. (2026-11-10 WB04-E239)
const view = new DataView(new ArrayBuffer(8));
const doubleOf = (bits) => { view.setBigUint64(0, bits, true); return view.getFloat64(0, true); };
const doubleBits = (value) => { view.setFloat64(0, value, true); return view.getBigUint64(0, true).toString(); };
const floatBits = (value) => { view.setFloat32(0, value, true); return String(view.getUint32(0, true)); };
const u32 = (value) => Number(BigInt.asUintN(32, value));
const s32 = (value) => Number(BigInt.asIntN(32, value));
const hexOf = (bytes) => Array.from(bytes, (value) => value.toString(16).padStart(2, '0')).join('');

// 공통 미션 14필드를 실제 폭으로 직렬화해 native 전체 상태와 비교한다. (2026-11-10 WB04-E239)
function gateText(state) {
    return [
        String(state.stage >>> 0), String(state.stageDirty), String(state.tutorialNumber), String(state.demo), doubleBits(state.startSeconds), doubleBits(state.nextStageSeconds),
        floatBits(state.delaySeconds), String(state.lockFrames >>> 0), String(state.started), doubleBits(state.demoEndSeconds), doubleBits(state.timerSeconds),
        doubleBits(state.tellAtSeconds), String(state.scriptPaused), String(state.clearFrames >>> 0),
    ].join(',');
}

// 실제 native 와 같은 외부 입력/선택 효과만 공급한다. 판정/제작/부류/게이트는 제품 몸체이다. (2026-11-10 WB04-E239)
class LaterEnvironment {
    constructor(input, gate) {
        this.input = input; // 원본 30개 명시 입력(BigInt)이며 callback 변경도 여기에만 적용한다.
        this.gate = gate; // 제품이 수정하는 유일한 현재 단계/타이머이다.
        this.owner = 1;
        this.depth = 0;
        this.selected = 0; // 명시 외부 Player/정지 깊이/선택 입력이다.
        this.trace = []; // 실제 현재 TYPE 재조회와 외부 하위 호출의 순서이다.
        this.changed = new Set(); // 각 외부 효과의 첫 입력 변경만 실행한다.
    }

    // 호출한 외부 경계가 현재 입력을 변경하는 정해진 재진입 장면이다. (2026-11-10 WB04-E239)
    effect(name) {
        this.trace.push(name);
        if (this.changed.has(name)) return;
        this.changed.add(name);
        const mutation = this.input[29]; // 명시 입력이 선택한 효과만 실행한다.
        if (mutation === 1n && name.startsWith('F')) this.owner = 2;
        else if (mutation === 2n && name.startsWith('I')) this.input[5] = 0n;
        else if (mutation === 3n && name === 'S') this.selected = 2;
        else if (mutation === 4n && name.startsWith('T')) this.gate.stage = 0x44; // 'D'
        else if (mutation === 5n && name === 'K') { this.depth = 3; this.gate.scriptPaused = 7; }
    }

    // 조회한 원본 전역 주소와 반환한 현재 타입을 native 메모리 읽기에 대조한다. (2026-11-10 WB04-E239)
    type(address) {
        this.effect(`Y${address}`);
        if (!TYPE_GLOBALS.has(address)) throw new Error(`입력에 없는 타입 전역 ${address.toString(16)}`);
        return TYPE_GLOBALS.get(address);
    }

    // 직접 단계 판정에 도달하지 않는 큰 공통 프레임 효과는 성공으로 채우지 않는다. (2026-11-10 WB04-E239)
    gateHooks() {
        const fail = (message) => () => { throw new Error(message); };
        return {
            gameSeconds: () => doubleOf(this.input[26]),
            paused: () => { this.effect('Q'); return this.depth !== 0; },
            pause: () => { this.effect('P'); this.depth += 1; },
            resume: fail('직접 튜토리얼3~6에서 다른 프레임 하위 선택'), checkOutcome: fail('직접 튜토리얼3~6에서 다른 프레임 하위 선택'), tellStage: fail('직접 튜토리얼3~6에서 다른 프레임 하위 선택'),
            tellAt: fail('직접 튜토리얼3~6에서 다른 프레임 하위 선택'), endDemo: fail('직접 튜토리얼3~6에서 다른 프레임 하위 선택'),
            dialogsClear: fail('직접 튜토리얼3~6에서 프레임 UI 선택'), knowledgeOpen: fail('직접 튜토리얼3~6에서 프레임 UI 선택'),
            guideOpen: fail('직접 튜토리얼3~6에서 프레임 UI 선택'), blockingGumpOpen: fail('직접 튜토리얼3~6에서 프레임 UI 선택'),
            tutorial: fail('직접 튜토리얼3~6의 중복 분배'),
        };
    }

    // 원본에서 대체한 실제 외부 하위와 같은 반환·인자·효과 순서를 쓴다. (2026-11-10 WB04-E239)
    hooks() {
        const input = this.input;
        return {
            findPriest: (player) => { const result = u32(input[player === 1 ? 13 : 14]); this.effect(`F${player}`); return result; },
            graphFor: (sid) => { const result = u32(input[BigInt(sid) === input[13] ? 15 : 16]); this.effect(`G${sid}`); return result; },
            productionContains: (type) => { const result = (input[5] & (1n << BigInt(type % 32))) !== 0n; this.effect(`C${type}`); return result; },
            indicateTechnology: (type) => { this.effect(`I${type}`); },
            regionFor: (x, y) => { assert.ok(x === 12 && y === 14); const result = s32(input[18]); this.effect('R'); return result; },
            nearby: (x, y, mask) => { assert.ok(x === 12 && y === 14); const result = u32(input[20]); this.effect(`N${mask}`); return result; },
            priestCaptured: (sid) => { const result = input[17] !== 0n; this.effect(`H${sid}`); return result; },
            selected: () => { const result = this.selected; this.effect('S'); return result; },
            gameSeconds: () => doubleOf(input[26]),
            resetSelection: () => { this.selected = 0; this.effect('Z'); },
            clearSelection: () => { this.selected = 0; this.effect('X'); },
            tell: (section) => { const result = input[22] !== 0n; this.effect(`T${section}`); return result; },
            closeTransientGumps: () => { this.effect('K'); },
        };
    }
}

// 한 행의 실제 저장소·상태·게이트·장부를 입력대로 구성한다. (2026-11-10 WB04-E239)
function buildScene(values) {
    const state = createMissionFrameGateState();
    state.stage = s32(values[1]);
    state.stageDirty = 7;
    state.started = 1;
    state.startSeconds = 2;
    state.lockFrames = s32(values[28]);
    state.timerSeconds = doubleOf(values[25]);
    const environment = new LaterEnvironment(values, state);
    environment.depth = u32(values[27]);
    environment.selected = u32(values[21]);
    const pool = new SidPool(32768);
    const types = Array.from({ length: 188 }, () => ({ flags2: 0 }));
    const players = new Uint8Array(1620);
    const raw = pool.physical(1);
    raw[10] = u32(values[9]) & 0xff;
    raw[0x22] = u32(values[10]) & 0xff;
    put(raw, 14, floatToBits(12));
    put(raw, 18, floatToBits(14));
    pool.physical(2)[10] = u32(values[12]) & 0xff;
    types[u32(values[9])].flags2 = u32(values[11]);
    put(players, 180 + 0x78, u32(values[19]));
    const counts = new SquidPostPopState();
    counts.globalCounts().fill(u32(values[6]));
    counts.localCounts().fill(u32(values[7]));
    counts.globalCounts()[123] = u32(values[8]);
    const restrictions = new Uint32Array(256);
    restrictions[165] = 9;
    const made = new RawTotalMade(types, counts, restrictions);
    const gate = new RawMissionFrameGate(state, environment.gateHooks());
    const mission = createMissionStartState();
    mission.fieldC0 = u32(values[23]);
    mission.fieldC4 = u32(values[24]);
    const options = Uint8Array.from([3, 4, 5, 6, 7, 8, 9, 10]);
    const commands = { rangeCommand: 0x1230001, placedCommand: 0x1230002, containedCommand: 0x1230003, technologyCommand: 0x1230004, captureCommand: 0x1230005, ascendCommand: 0x1230006 }; // native 와 같은 서로 다른 실제 명령 입력이다.
    const body = new RawTutorialLater(pool, types, players, made, gate, mission, {
        battleOptions: options, get localOwner() { return environment.owner; }, type: (address) => environment.type(address), ...commands,
    }, environment.hooks());
    return { state, environment, restrictions, mission, options, body };
}

test('튜토리얼 3~6 단계 판정기: 원본 10.78 533개 고유 입력의 게이트 14필드·정지·하위 호출 순서·반환·옵션·허용·선택·일회성 경고·owner 가 모두 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    let compared = 0;
    for (const row of observations) {
        assert.equal(row.length, 11);
        const values = row[0].split(',').map(BigInt);
        assert.equal(values.length, 30);
        const scene = buildScene(values);
        const event = [u32(values[2]), u32(values[3]), u32(values[4])];
        const result = scene.body.handle(Number(u32(values[0])), event);
        const label = `행 ${compared} 입력 ${row[0]}`;
        assert.equal(gateText(scene.state), row[1], `${label} 게이트`);
        assert.equal(scene.environment.trace.join(','), row[3], `${label} 효과 순서`);
        assert.equal(scene.environment.depth, Number(row[2]), `${label} 정지 깊이`);
        assert.equal(result, Number(row[4]), `${label} 반환`);
        assert.equal(hexOf(scene.options), row[5], `${label} 옵션`);
        assert.equal(scene.restrictions[165], Number(row[6]), `${label} 허용`);
        assert.equal(scene.environment.selected, Number(row[7]), `${label} 선택`);
        assert.equal(scene.mission.fieldC0, Number(row[8]), `${label} +c0`);
        assert.equal(scene.mission.fieldC4, Number(row[9]), `${label} +c4`);
        assert.equal(scene.environment.owner, Number(row[10]), `${label} owner`);
        compared += 1;
    }
    assert.equal(compared, INPUTS);
});

test('튜토리얼 3~6 단계 판정기 계약: 누락된 생산 하위/잘못된 입력은 단계 변경이나 옵션 쓰기 전에 명시 거부한다', () => {
    const state = createMissionFrameGateState();
    const environment = new LaterEnvironment(new Array(30).fill(0n), state);
    const pool = new SidPool(32768);
    const types = Array.from({ length: 188 }, () => ({ flags2: 0 }));
    const players = new Uint8Array(1620);
    const counts = new SquidPostPopState();
    const made = new RawTotalMade(types, counts, new Uint32Array(256));
    const gate = new RawMissionFrameGate(state, environment.gateHooks());
    const mission = createMissionStartState();
    const options = new Uint8Array(3);
    const command = 1;
    const inputs = { battleOptions: options, localOwner: 1, type: (address) => environment.type(address), rangeCommand: command, placedCommand: command, containedCommand: command, technologyCommand: command, captureCommand: command, ascendCommand: command };
    for (const name of Object.keys(environment.hooks())) assert.throws(() => new RawTutorialLater(pool, types, players, made, gate, mission, inputs, { ...environment.hooks(), [name]: undefined }), TypeError, name);
    assert.throws(() => new RawTutorialLater(pool, types, players, made, gate, mission, { ...inputs, battleOptions: new Uint8Array(2) }, environment.hooks()), TypeError);
    assert.throws(() => new RawTutorialLater(pool, types, new Uint8Array(10), made, gate, mission, inputs, environment.hooks()), TypeError);
    assert.throws(() => new RawTutorialLater(pool, types, players, made, gate, mission, { ...inputs, type: undefined }, environment.hooks()), TypeError);
    const body = new RawTutorialLater(pool, types, players, made, gate, mission, inputs, environment.hooks());
    assert.throws(() => body.handle(3, [0, 0]), TypeError);
    assert.throws(() => body.handle(2, [0, 0, 0]), TypeError);
    assert.ok(state.stage === 65 && options[2] === 0 && environment.trace.length === 0);
});

test('튜토리얼 3~6 단계 판정기 계약: 명령 modifier 상위 비트, 일회성 경고의 재실행 방지, 현재 Player 번호·사건 SID TYPE 의 물리 범위 거부', () => {
    // 필요한 입력만 갖춘 최소 장면이다: 단계·사건·owner·표식을 바꿔 가며 쓴다.
    const make = ({ stage, owner = 1, fieldC0 = 0, fieldC4 = 0, productionOn = false, tellOpens = false }) => {
        const state = createMissionFrameGateState();
        state.stage = stage;
        state.started = 1;
        state.lockFrames = 0; // 안내 잠금이 없어야 Advance 가 예약된다.
        const pool = new SidPool(32768);
        const types = Array.from({ length: 188 }, () => ({ flags2: 0 }));
        const counts = new SquidPostPopState();
        const made = new RawTotalMade(types, counts, new Uint32Array(256));
        const gate = new RawMissionFrameGate(state, { gameSeconds: () => 0, paused: () => false, pause: () => {}, resume() {}, checkOutcome() {}, tellStage() {}, tellAt() {}, endDemo() {}, dialogsClear: () => true, knowledgeOpen: () => false, guideOpen: () => false, blockingGumpOpen: () => false, tutorial: () => 0 });
        const mission = createMissionStartState();
        mission.fieldC0 = fieldC0;
        mission.fieldC4 = fieldC4;
        const tells = [];
        const commands = { rangeCommand: 0x1230001, placedCommand: 0x1230002, containedCommand: 0x1230003, technologyCommand: 0x1230004, captureCommand: 0x1230005, ascendCommand: 0x1230006 };
        const body = new RawTutorialLater(pool, types, new Uint8Array(1620), made, gate, mission, { battleOptions: new Uint8Array(8), localOwner: owner, type: (address) => TYPE_GLOBALS.get(address), ...commands }, {
            findPriest: () => 0, graphFor: () => 0, productionContains: () => productionOn, indicateTechnology: () => {}, regionFor: () => 0, nearby: () => 0, priestCaptured: () => false,
            selected: () => 0, gameSeconds: () => 0, resetSelection: () => {}, clearSelection: () => {}, tell: (name) => { tells.push(name); return tellOpens; }, closeTransientGumps: () => {},
        });
        return { state, pool, mission, body, tells, types };
    };
    // 6 'B': 사거리 표시 명령에 modifier 상위 비트(0x04000000)가 섞여도 같은 명령으로 인정해 Advance 한다(단계 B → C).
    const modifier = make({ stage: 0x42 });
    modifier.body.handle(6, [0x1230001 | 0x04000000, 0, 0]);
    assert.equal(modifier.state.stage, 0x43);
    // 5: 전역 제작 수가 없으면 Vortex 경고를, 단계가 B 를 넘고 생산 목록이 비면 Factory 경고를 한 번씩 낸다. 표식이 이미 켜져 있으면 다시 내지 않는다.
    const first = make({ stage: 0x46 });
    first.body.handle(5, [0, 0, 0]);
    assert.deepEqual(first.tells, ['VortexDestroyed', 'FactoryDestroyed']);
    assert.ok(first.mission.fieldC0 === 1 && first.mission.fieldC4 === 1);
    const again = make({ stage: 0x46, fieldC0: 1, fieldC4: 1 });
    again.body.handle(5, [0, 0, 0]);
    assert.deepEqual(again.tells, []);
    // 3 'E': 배치 완료 명령에서 현재 Player 번호가 9 이상이면 지역 조회 뒤 범위 오류로 거부한다(사건 SID 1 의 TYPE 이 71 이어야 이 검사에 도달).
    const placed = make({ stage: 0x45, owner: 9 });
    placed.pool.physical(1)[10] = 71;
    assert.throws(() => placed.body.handle(3, [0x1230002, 1, 0]), RangeError);
    // 3 'G': 사건 SID 의 TYPE 바이트가 표 밖(200)이면 부류 조회에서 RangeError 이다.
    const genus = make({ stage: 0x47 });
    genus.pool.physical(1)[10] = 200;
    assert.throws(() => genus.body.handle(3, [0x1230003, 1, 0]), RangeError);
});

