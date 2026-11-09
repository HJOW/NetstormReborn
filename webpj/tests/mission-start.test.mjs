// 공통 미션 시작 raw-mission-start.js(PATCH 004c2b20 Start·004c3100 RefreshTellAt)와 같은 fixture 의 AI 설정/생성 관측을 원본 10.78 관측과 cpppj `MissionStartTests` 계약으로 대조한다.
//  - 원본 관측 mission-start-x86.tsv 의 204개 고유 입력: start 48(Start 전체 상태·double 비트·조회/효과 순서)·init 20·apply 64·prepare 8·ability 32(MissionAiSettings 설정 600 BYTE 전체)·create 32(AiCreate 후 Player 180 BYTE·반환 토큰·효과 순서).
//    E32 의 `RawMissionAiSettings` apply/prepare/setAbility 와 E164 의 `RawAiCreate` 를 같은 관측으로 처음 대조한다(PORT_MAP E32 메모의 "미션 시작 override 관측은 MissionStart 단위에서").
//  - 같은 start 48행을 실제 현재 getter 연결(값형 fallback 은 반대 입력으로 오염)로 다시 재생하고, 원본 owner 에 쓴 allowAnyCapture·ordinaryGeysers 가 장기 GameAuxState 에 쓰이는지 확인한다.
//  - 계약: SP/표시/기술/AI 효과 뒤 현재 owner 재조회와 myTech 앞의 owner 보관 순서, 미복원 생성자/초기 갱신 거부와 기존 brain·NoTemple 반환, 선택 효과 누락 거부.
// CD 10.72 분기는 웹 범위 밖이다. 실제 게임·브라우저 실행이 아니다. (2026-11-10 WB04-E234: 미션 시작 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, floatToBits } from './raw-support.mjs';
import { createMissionFrameGateState } from '../www/core/raw-mission-frame-gate.js';
import { RawMissionStart, createMissionStartGlobals, createMissionStartState } from '../www/core/raw-mission-start.js';
import { RawMissionAiSettings } from '../www/core/raw-mission-ai-settings.js';
import { RawAiCreate } from '../www/core/raw-ai-brain.js';
import { PlayerParticipationState, RawPlayerParticipation } from '../www/core/raw-player-participation.js';
import { GameAuxState } from '../www/core/game-aux-state.js';

// 저장된 독립 기계어 출력을 한 번만 읽는다(5칸: 종류·입력·raw·결과·trace). (2026-11-10 WB04-E234)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'mission-start-x86.tsv'));
const observations = rows ? rows.filter((row) => row[0] !== 'kind') : null;

// TSV 의 문자열 바이트를 변경 없이(latin1 문자) 복원한다. (2026-11-10 WB04-E234)
const unhex = (text) => String.fromCharCode(...(text.match(/../g) ?? []).map((pair) => parseInt(pair, 16)));
// 문자열/바이트를 소문자 hex 로 쓴다. (2026-11-10 WB04-E234)
const hexOfText = (text) => Array.from(text, (char) => char.charCodeAt(0).toString(16).padStart(2, '0')).join('');
const hexOfBytes = (bytes) => Array.from(bytes, (value) => value.toString(16).padStart(2, '0')).join('');
// 실제 double 필드를 메모리 순서(little endian)로 직렬화한다. (2026-11-10 WB04-E234)
function doubleHex(value) {
    const view = new DataView(new ArrayBuffer(8));
    view.setFloat64(0, value, true);
    return hexOfBytes(new Uint8Array(view.buffer));
}
// C `strtod` 처럼 앞쪽 십진 부동소수만 읽는다(원본 관측 입력에 쓰이는 형태). (2026-11-10 WB04-E234)
function parseDouble(text) {
    const match = /^\s*[+-]?(\d+\.?\d*|\.\d+)([eE][+-]?\d+)?/.exec(text);
    if (!match) throw new Error(`stod 입력 오류: ${text}`);
    return Number(match[0]);
}

// 모든 입력 저장소/효과는 원본과 동일하게 준비하고 기대 분기를 계산하지 않는다. (2026-11-10 WB04-E234)
class StartScene {
    constructor(input) {
        const fields = input.split('|');
        this.numbers = fields[0].split(',').map(Number); // DWORD 와 음수 입력을 손실 없이 저장한다.
        this.config = new Map(); // 외부 Config 조회가 반환할 문자열 입력이다.
        this.trace = [];
        // 설정 값은 세미콜론을 포함할 수 있으므로 hex 로 보존한다.
        for (let index = 1; index < fields.length; index += 1) {
            if (fields[index] === '') continue;
            const equal = fields[index].indexOf('=');
            this.config.set(fields[index].slice(0, equal), unhex(fields[index].slice(equal + 1)));
        }
        // 원본과 같은 caller 미정 바이트 패턴이며 기대 출력은 아니다.
        this.settings = Uint8Array.from({ length: 600 }, (_, index) => (index * 37 + this.numbers[0]) & 0xff);
        this.aiHooks = {
            get: (key) => this.get(key),
            parseFloat: (value) => { this.add(`F,${hexOfText(value)}`); return parseDouble(value); },
            unknownAbilityAssertion: () => { this.add('A'); },
            unknownAbilityLog: () => { this.add('L'); },
        };
    }

    // 실제 호출을 누적하며 기대 순서를 미리 작성하지 않는다. (2026-11-10 WB04-E234)
    add(value) { this.trace.push(value); }

    // 매 조회의 원본 키와 순서를 기록한다. (2026-11-10 WB04-E234)
    get(key) { this.add(`G,${key}`); return this.config.get(key) ?? ''; }

    traceText() { return this.trace.length ? this.trace.join(';') : '-'; }

    // 기본 글로벌과 실제 설정 처리기를 조립한다. (2026-11-10 WB04-E234)
    settingsRuntime() {
        return new RawMissionAiSettings({ stuffRefreshRate: this.numbers[2], startingSp: this.numbers[3], checkingAsserts: this.numbers[4] !== 0 }, this.aiHooks);
    }

    // 실제 공통 Start 결과를 직렬화한다. 현재 getter 재생은 fallback 만 반대로 두고 같은 독립 입력을 쓴다. (2026-11-10 WB04-E234)
    start(currentInputs = false, aux = null) {
        const gate = createMissionFrameGateState();
        gate.demo = this.numbers[3] >>> 0;
        gate.started = 0;
        gate.startSeconds = 77.25;
        gate.demoEndSeconds = 77.25;
        gate.tellAtSeconds = 77.25;
        const state = createMissionStartState();
        state.flags = 0xffffffff;
        state.sequence = this.numbers[5] >>> 0;
        state.tellAtIndex = this.numbers[6] >>> 0;
        state.fieldC0 = 0xa5a5a5a5;
        state.fieldC4 = 0xa5a5a5a5;
        const globals = createMissionStartGlobals();
        globals.localOwner = this.numbers[0] >>> 0;
        globals.playerKnown = this.numbers[1] >>> 0;
        globals.editor = this.numbers[2] >>> 0;
        const hooks = {
            get: (key) => this.get(key),
            sectionNames: (prefix) => { this.add(`N,${prefix}`); return this.config.get('@sections') ?? ''; },
            set: (key, value) => { this.add(`W,${key},${hexOfText(value)}`); this.config.set(key, value); },
            gameSeconds: () => 16777217.25,
            setSp: (owner, value) => { this.add(`S,${owner},${floatToBits(value)}`); },
            refreshSp: () => { this.add('R'); },
            clearTechnology: (owner) => { this.add(`C,${owner}`); },
            addTechnology: (owner, tech) => { this.add(`T,${owner},${hexOfText(tech)}`); },
            hasPriest: (owner) => { this.add(`P,${owner}`); return ((this.numbers[4] >>> 0) & (1 << owner)) !== 0; },
            createAi: (owner) => { this.add(`I,${owner}`); },
            refreshCommandGump: () => { this.add('Q'); },
            randomGeysers: () => { this.add('Y'); },
            resetCamera: () => { this.add('Z'); },
        };
        const currentOwner = globals.localOwner;
        const currentKnown = globals.playerKnown;
        const currentEditor = globals.editor; // 기존 native 의 실제 입력이며 getter 재생에서도 변경하지 않는다.
        const fallbackOwner = currentOwner === 1 ? 8 : 1;
        const fallbackKnown = currentKnown === 0 ? 1 : 0;
        const fallbackEditor = currentEditor === 0 ? 1 : 0; // 잘못된 fallback 사용을 드러낼 반대 입력이다.
        if (aux) { // 새 owner 에만 실제 Start 출력을 쓴다.
            globals.shared.allowAnyCapture = { get value() { return aux.allowAnyCapture; }, set value(next) { aux.allowAnyCapture = next; } };
            globals.shared.ordinaryGeysers = { get value() { return aux.ordinaryGeysers; }, set value(next) { aux.ordinaryGeysers = next; } };
            globals.allowAnyCapture = 79;
            globals.ordinaryGeysers = 83;
        }
        const runtime = new RawMissionStart(gate, state, globals, hooks);
        if (currentInputs) {
            globals.localOwner = fallbackOwner;
            globals.playerKnown = fallbackKnown;
            globals.editor = fallbackEditor;
            runtime.bindCurrentInputs({ localOwner: () => currentOwner, playerKnown: () => currentKnown, editor: () => currentEditor });
        }
        runtime.start();
        if (currentInputs) assert.ok(globals.localOwner === fallbackOwner && globals.playerKnown === fallbackKnown && globals.editor === fallbackEditor);
        const allowAnyCapture = aux ? aux.allowAnyCapture : globals.allowAnyCapture;
        const ordinaryGeysers = aux ? aux.ordinaryGeysers : globals.ordinaryGeysers;
        const values = [state.flags, state.sequence, state.tellAtIndex, state.fieldC0, state.fieldC4, gate.started, globals.denySalvage >>> 0, globals.denyAscend >>> 0, allowAnyCapture >>> 0, ordinaryGeysers >>> 0];
        return `${values.join(',')}|${doubleHex(gate.startSeconds)}|${doubleHex(gate.demoEndSeconds)}|${doubleHex(gate.tellAtSeconds)}`;
    }

    // 실제 Player 부착 몸체를 재사용하고 원본 생성자 요청만 외부 경계로 관찰한다. (2026-11-10 WB04-E234)
    create() {
        const owner = this.numbers[0];
        const players = new Uint8Array(1620);
        const allies = new Array(81).fill(0);
        const hidden = new Uint8Array(9);
        put(players, owner * 180 + 0xb0, this.numbers[1] >>> 0);
        put(players, owner * 180 + 0x78, 127);
        put(players, owner * 180 + 0x50, this.numbers[5] >>> 0);
        put(players, owner * 180 + 0x2c, this.numbers[6] >>> 0);
        const participation = new RawPlayerParticipation(players, allies, hidden, new PlayerParticipationState(), {});
        this.settings.fill(0);
        put(this.settings, 0x244, this.numbers[2] >>> 0);
        put(this.settings, 0x250, this.numbers[3] >>> 0);
        const hooks = {
            changeColor: (target, color) => { this.add(`O,${target},${color}`); },
            firstRegion: () => 127,
            islandTemple: (region) => { this.add(`V,${region}`); return this.numbers[4] >>> 0; },
            destroyTemple: (_temple, flags) => { this.add(`D,${flags}`); },
            construct: (target, _settings, size) => { this.add(`B,${size}`); this.add(`K,${target}`); return 0x12310000; },
            initialUpdate: (token) => { this.add(`U,${token}`); },
        };
        const token = new RawAiCreate(participation, hooks).create(owner, this.settings);
        return [hexOfBytes(players.subarray(owner * 180, owner * 180 + 180)), token];
    }
}

test('미션 시작: 원본 10.78 204개 고유 입력(start 48·init 20·apply 64·prepare 8·ability 32·create 32)의 전체 상태·설정 600 BYTE·Player 180 BYTE·효과 순서가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    let count = 0;
    const unique = new Set();
    for (const row of observations) {
        assert.equal(row.length, 5);
        assert.ok(!unique.has(`${row[0]}\t${row[1]}`), '고유 입력');
        unique.add(`${row[0]}\t${row[1]}`);
        const scene = new StartScene(row[1]);
        let raw = '';
        let result = '-';
        if (row[0] === 'start') raw = scene.start();
        else if (row[0] === 'create') { const created = scene.create(); raw = created[0]; result = String(created[1]); }
        else {
            const runtime = scene.settingsRuntime();
            if (row[0] === 'init') runtime.initialize(scene.settings);
            else if (row[0] === 'apply') runtime.apply(scene.settings, scene.numbers[1]);
            else if (row[0] === 'prepare') runtime.prepare(scene.settings, scene.numbers[1]);
            else if (row[0] === 'ability') runtime.setAbility(scene.settings, scene.config.get('name') ?? '', scene.numbers[5] !== 0);
            else assert.fail(`알 수 없는 종류 ${row[0]}`);
            raw = hexOfBytes(scene.settings);
        }
        count += 1;
        const label = `행 ${count} ${row[0]} ${row[1]}`;
        assert.equal(raw, row[2], `${label} raw`);
        assert.equal(result, row[3], `${label} 결과`);
        assert.equal(scene.traceText(), row[4], `${label} trace`);
    }
    assert.equal(count, 204);
});

test('미션 시작: start 48행을 실제 현재 getter 로 반대 fallback 과 함께 재생해도 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    let compared = 0;
    for (const row of observations) {
        if (row[0] !== 'start') continue;
        const scene = new StartScene(row[1]);
        assert.equal(scene.start(true), row[2]);
        assert.equal(scene.traceText(), row[4]);
        compared += 1;
    }
    assert.equal(compared, 48);
});

test('미션 시작: 같은 start 48행의 allowAnyCapture·ordinaryGeysers 출력은 실제 장기 GameAuxState 에만 쓴다', { skip: !rows && '원본 fixture 없음' }, () => {
    let compared = 0;
    for (const row of observations) {
        if (row[0] !== 'start') continue;
        const aux = new GameAuxState();
        aux.allowAnyCapture = -71;
        aux.ordinaryGeysers = 73;
        const scene = new StartScene(row[1]);
        assert.equal(scene.start(false, aux), row[2]);
        assert.equal(scene.traceText(), row[4]);
        compared += 1;
    }
    assert.equal(compared, 48);
});

test('미션 시작 계약: SP/표시/기술/AI 효과 뒤 현재 owner 를 다시 읽고 myTech 앞의 owner 는 보관한다', () => {
    const gate = createMissionFrameGateState();
    const state = createMissionStartState();
    const globals = createMissionStartGlobals();
    globals.localOwner = 8; // getter 와 다른 값형 fallback 이다.
    let currentOwner = 1;
    const currentKnown = 1;
    const currentEditor = 1;
    const trace = [];
    const add = (value) => trace.push(value); // 실제 읽기/효과 위치에서만 관찰한다.
    const hooks = {
        get: (key) => {
            if (key === 'myStartMoney') { add('M'); return '12'; }
            if (key === 'myTech') { add('T'); currentOwner = 5; }
            return '';
        },
        sectionNames: () => '', set: () => {},
        setSp: (owner, money) => { assert.ok(owner === 1 && money === 194162); add('S1'); currentOwner = 2; },
        refreshSp: () => { add('R'); currentOwner = 3; },
        clearTechnology: (owner) => { assert.equal(owner, 3); add('C3'); currentOwner = 4; },
        addTechnology: (owner) => { assert.equal(owner, 4); add('A4'); currentOwner = 6; },
        hasPriest: (owner) => { add(`P${owner}`); return owner === 2; },
        createAi: (owner) => { assert.equal(owner, 2); add('I2'); currentOwner = 7; },
        refreshCommandGump: () => { add('Q'); }, resetCamera: () => { add('Z'); }, gameSeconds: () => { add('V'); return 17.25; },
    };
    const start = new RawMissionStart(gate, state, globals, hooks);
    assert.throws(() => start.bindCurrentInputs({ localOwner: () => currentOwner, editor: () => currentEditor }), TypeError);
    start.bindCurrentInputs({ localOwner: () => { add(`O${currentOwner}`); return currentOwner; }, playerKnown: () => { add('K'); return currentKnown; }, editor: () => { add('E'); return currentEditor; } });
    start.start();
    assert.equal(trace.join(';'), 'M;E;K;O1;S1;R;O3;C3;O4;T;A4;O6;P1;O6;P2;I2;O7;P3;O7;P4;O7;P5;O7;P6;O7;O7;P8;Q;Z;V');
    assert.ok(globals.localOwner === 8 && globals.playerKnown === 0 && globals.editor === 0 && gate.started === 1 && gate.startSeconds === 17.25);
});

test('미션 시작 계약: 미복원 생성자/초기 갱신을 성공으로 간주하지 않고 기존 brain·NoTemple 은 생성자 없이 반환하며 선택 효과 누락은 started 를 쓰기 전에 거부한다', () => {
    const players = new Uint8Array(1620);
    const participation = new RawPlayerParticipation(players, new Array(81).fill(0), new Uint8Array(9), new PlayerParticipationState(), {});
    const settings = new Uint8Array(600);
    const missing = new RawAiCreate(participation, {});
    assert.throws(() => missing.create(1, settings), Error);
    assert.ok(participation.brain(1) === 0 && players[180 + 0x50] === 0);
    put(players, 180 + 0xb0, 73);
    assert.equal(missing.create(1, settings), 73);
    put(players, 180 + 0xb0, 0);
    const gate = createMissionFrameGateState();
    const state = createMissionStartState();
    const globals = createMissionStartGlobals();
    const hooks = {
        get: () => '', sectionNames: () => '', set: () => {}, refreshSp: () => {},
        clearTechnology: (owner) => { assert.equal(owner, 1); globals.localOwner = 8; },
        addTechnology: (owner) => { assert.equal(owner, 8); },
        hasPriest: (owner) => { assert.notEqual(owner, globals.localOwner); return false; },
        refreshCommandGump: () => {}, resetCamera: () => {}, gameSeconds: () => 16777217.25,
    };
    new RawMissionStart(gate, state, globals, hooks).start();
    assert.ok(gate.started === 1 && gate.startSeconds === 16777217.25);
    gate.started = 0;
    const unavailable = new RawMissionStart(gate, state, globals, { ...hooks, refreshSp: undefined });
    assert.throws(() => unavailable.start(), TypeError);
    assert.equal(gate.started, 0);
    assert.throws(() => new RawMissionStart(gate, state, globals, {}), TypeError);
});

test('미션 시작 계약: 시작 금액은 SP 지정 전에 단정도 float 로 줄고(2^24+1 → 2^24) 빈 tellAt 항목은 기한 0 이다', () => {
    const gate = createMissionFrameGateState();
    gate.tellAtSeconds = 5;
    const state = createMissionStartState();
    const globals = createMissionStartGlobals();
    globals.playerKnown = 1;
    const received = [];
    const hooks = {
        get: (key) => (key === 'myStartMoney' ? '16777217' : ''), sectionNames: () => '', set: () => {},
        setSp: (owner, money) => { received.push([owner, money]); }, refreshSp: () => {}, clearTechnology: () => {}, addTechnology: () => {}, hasPriest: () => false,
        refreshCommandGump: () => {}, resetCamera: () => {}, gameSeconds: () => 1,
    };
    new RawMissionStart(gate, state, globals, hooks).start();
    assert.deepEqual(received, [[1, 16777216]]);
    assert.equal(gate.tellAtSeconds, 0);
});

test('미션 AI 설정 계약(미션 시작 관측으로 처음 대조한 apply/setAbility 의 fixture 밖 경계): AllyList 는 양수만 비트로 더하고 능력 이름은 첫 NUL 에서 끊긴다', () => {
    const settings = new Uint8Array(600);
    const values = { aiAllyList: '0;2;9;33;-1;x' }; // owner −1(공통)의 키는 접두 `ai` 에 목록 이름을 붙인다.
    const runtime = new RawMissionAiSettings({ stuffRefreshRate: 3, startingSp: 2500, checkingAsserts: false }, { get: (key) => values[key] ?? '' });
    runtime.apply(settings, -1);
    const dword = (bytes, offset) => (bytes[offset] | (bytes[offset + 1] << 8) | (bytes[offset + 2] << 16) | (bytes[offset + 3] << 24)) >>> 0;
    // 0 과 음수와 숫자가 아닌 항목은 무시하고 2 → 비트 2, 9 → 비트 9, 33 → 비트 1(하위 5비트)이 켜진다.
    assert.equal(dword(settings, 0x23c), (1 << 2) | (1 << 9) | (1 << 1));
    // 능력 이름은 첫 NUL 앞까지만 읽는다: 능력 표 첫 항목(비트 0)이 켜진다.
    const ability = new Uint8Array(600);
    runtime.setAbility(ability, 'FOCUS_ON_THINGS_THAT_ATTACK_BUILDINGS ignored', true);
    assert.equal(dword(ability, 0x248), 1);
});

