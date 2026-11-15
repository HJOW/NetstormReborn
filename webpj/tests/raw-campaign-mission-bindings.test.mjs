// 실제 미션 Start/AI 설정 연결 raw-campaign-mission-start-bindings.js 와 Tutorial 형식 공통 프레임 연결 raw-campaign-tutorial-one-bindings.js 를 cpppj 계약으로 대조한다(4-D C 구획, WB04-E520).
// cpppj 대응(`CampaignMissionStartTests.cpp`·`CampaignTutorialOneTests.cpp`의 이 단위 소유 계약):
//  - MissionStart: 모든 외부 소비자 누락 거부, Start 순서(prepareWorld → tellAtList 쓰기/예약 → SP → 기술 → 사제 있는 owner 만 AI → 명령 Gump → 랜덤 간헐천 → 카메라 → started),
//    CreateAi 의 범위 1..8·caller 미정 바이트 보존·기본값→공통→owner 누적과 실제 Session AI 생성 호출, FindLivePriest 의 dead 제외.
//  - TutorialOne: 안내/UI 계약 누락 거부, 데모·번호 밖 미션은 전용 판정기를 만들지 않음, 번호별 판정기 생성(1·2·3~6)과 인자, 공통 프레임 순서, 특수 명령(First·다음 단계·다음 안내),
//    TellStage/TellAt 의 정지·인덱스 증가·다음 예약.
// 실제 `RawCampaignSession` 은 아직 없어 두 모듈의 session 덕타입 계약만 대조한다. 기대값은 cpppj 계약과 같은 관계이며 실제 게임·브라우저 실행이 아니다. (2026-11-15 WB04-E520)
import assert from 'node:assert/strict';
import test from 'node:test';
import { RawCampaignMissionStartBindings } from '../www/core/raw-campaign-mission-start-bindings.js';
import { RawCampaignTutorialOneBindings } from '../www/core/raw-campaign-tutorial-one-bindings.js';
import { MISSION_AI_SETTINGS_SIZE } from '../www/core/raw-mission-ai-settings.js';
import { createMissionFrameGateState } from '../www/core/raw-mission-frame-gate.js';
import { createMissionStartGlobals, createMissionStartState } from '../www/core/raw-mission-start.js';

// 사제 슬롯의 상태 BYTE(+11)·소유자 BYTE(+34)와 dead 비트이다. (2026-11-15 WB04-E520)
const STATE = 11;
const OWNER = 34;
const DEAD = 2;

// 미션 스크립트의 `mission.키` 조회·쓰기·섹션 이름을 흉내 내는 최소 Script 이다. (2026-11-15 WB04-E520)
function makeScript(values = {}, sections = '') {
    const store = new Map(Object.entries(values));
    return {
        loaded: true,
        store,
        get: (key) => (store.has(key) ? store.get(key) : null),
        set: (key, value) => { store.set(key, value); return true; },
        sectionNames: () => sections,
    };
}

// 같은 풀의 사제 슬롯 두 개(owner 2 생존·owner 3 dead)를 가진 풀과 사제 목록이다. (2026-11-15 WB04-E520)
function makePriests() {
    const slots = new Map();
    const make = (sid, owner, state) => { const raw = new Uint8Array(50); raw[OWNER] = owner; raw[STATE] = state; slots.set(sid, raw); };
    make(5, 2, 0);
    make(6, 3, DEAD);
    make(7, 3, 0);
    const pool = { edition: 'patch1078', capacity: 100, slot: (sid) => slots.get(sid) };
    const list = { readCount: () => 3, items: () => [5, 6, 7] };
    return { pool, list };
}

// MissionStart 용 가짜 Session 과 기록 배열이다. (2026-11-15 WB04-E520)
function makeStartScene(overrides = {}) {
    const events = [];
    const { pool, list } = makePriests();
    const aiCalls = [];
    const session = {
        core: () => ({ pool }),
        priests: () => ({ priests: () => ({ priests: list }) }),
        localOwner: () => 1,
        playerKnown: () => 1,
        editor: () => 0,
        setSp: (owner, money) => { events.push(`setSp:${owner}:${money}`); },
        clearTechnology: (owner) => { events.push(`clearTech:${owner}`); },
        addTechnology: (owner, list2) => { events.push(`addTech:${owner}:${list2}`); },
        createAi: (owner, bytes) => { aiCalls.push([owner, Uint8Array.from(bytes)]); events.push(`createAi:${owner}`); return 100 + owner; },
        ...overrides.session,
    };
    const script = makeScript({ myStartMoney: '123', myTech: 'sunWalker', randGeysers: '1', moreGeysers: '0', demoRestartTimer: '0', ...overrides.values }, overrides.sections ?? 'A;B');
    const gate = createMissionFrameGateState();
    const state = createMissionStartState();
    const globals = createMissionStartGlobals();
    const aiGlobals = { stuffRefreshRate: 3, startingSp: 2500, checkingAsserts: false };
    const effects = {
        settingsBytes: (owner, bytes) => { events.push(`settings:${owner}`); bytes.fill(0xa5); },
        gameSeconds: () => 42,
        refreshSp: () => { events.push('refreshSp'); },
        refreshCommandGump: () => { events.push('refreshCommand'); },
        randomGeysers: () => { events.push('randomGeysers'); },
        resetCamera: () => { events.push('resetCamera'); },
        prepareWorld: () => { events.push('prepareWorld'); },
        ...overrides.effects,
    };
    return { events, session, script, gate, state, globals, aiGlobals, effects, aiCalls, make: () => new RawCampaignMissionStartBindings(session, script, gate, state, globals, aiGlobals, effects) };
}

test('RawCampaignMissionStartBindings: 필수 소비자 누락·미로딩 Script·테스트 자동 플레이 입력은 거부한다', () => {
    for (const name of ['settingsBytes', 'gameSeconds', 'refreshSp', 'refreshCommandGump', 'randomGeysers', 'resetCamera']) {
        const scene = makeStartScene({ effects: { [name]: undefined } });
        assert.throws(() => scene.make(), TypeError, name);
    }
    const unloaded = makeStartScene();
    unloaded.script.loaded = false;
    assert.throws(() => unloaded.make(), TypeError);
    // 테스트 전용 자동 플레이는 웹 범위 밖이므로 조용히 무시하지 않는다.
    assert.throws(() => makeStartScene({ effects: { testPlay: { startSp: 5 } } }).make(), /웹 범위 밖/);
    assert.equal(makeStartScene().make().testPlay(), null);
});

test('RawCampaignMissionStartBindings: Start 는 prepareWorld → SP → 기술 → 사제 있는 owner 의 AI → 명령 Gump → 랜덤 간헐천 → 카메라 → started 순서이다', () => {
    const scene = makeStartScene();
    const bindings = scene.make();
    assert.equal(scene.gate.started, 0);
    bindings.start();
    // owner 1 은 로컬(비데모)이라 제외, owner 2 는 생존 사제, owner 3 은 dead 사제와 생존 사제가 함께 있어 생존 쪽을 찾는다.
    assert.deepEqual(scene.events, ['prepareWorld', 'setSp:1:123', 'refreshSp', 'clearTech:1', 'addTech:1:sunWalker',
        'settings:2', 'createAi:2', 'settings:3', 'createAi:3', 'refreshCommand', 'randomGeysers', 'resetCamera']);
    assert.equal(scene.gate.started, 1);
    assert.equal(scene.gate.startSeconds, 42);
    // tellAtList 는 섹션 이름 목록으로 쓰였다.
    assert.equal(scene.script.get('tellAtList'), 'A;B');
    // 같은 객체의 시작 상태를 한 번 올렸다.
    assert.equal(scene.state.sequence, 1);
});

test('RawCampaignMissionStartBindings: prepareWorld 가 없으면 원본 Start 와 같고 데모는 로컬 owner 의 AI 도 만든다', () => {
    const scene = makeStartScene({ effects: { prepareWorld: undefined } });
    scene.gate.demo = 1;
    scene.make().start();
    assert.equal(scene.events.includes('prepareWorld'), false);
    // 데모는 owner 1 도 사제 조회 대상이지만 사제가 없어 AI 는 2·3 뿐이다.
    assert.deepEqual(scene.events.filter((item) => item.startsWith('createAi')), ['createAi:2', 'createAi:3']);
});

test('RawCampaignMissionStartBindings: CreateAi 는 caller 미정 바이트를 보존하고 기본값→공통→owner 설정을 누적한 뒤 Session AI 를 만든다', () => {
    const scene = makeStartScene({ values: { ai2StartMoney: '777', ai2Name: 'Bob', aiGeyserAttachments: '5', ai2GeyserAttachments: '9' } });
    const bindings = scene.make();
    const token = bindings.createAi(2);
    assert.equal(token, 102);
    assert.equal(scene.aiCalls.length, 1);
    const [owner, bytes] = scene.aiCalls[0];
    assert.equal(owner, 2);
    assert.equal(bytes.length, MISSION_AI_SETTINGS_SIZE);
    const view = new DataView(bytes.buffer);
    // 기본값이 쓰는 칸은 바뀌고(+0x230 은 공통 5 뒤 owner 9) 이름은 NUL 로 끝난다.
    assert.equal(view.getUint32(0x230, true), 9);
    assert.equal(view.getUint32(0x22c, true), 777);
    assert.deepEqual(Array.from(bytes.subarray(0, 4)), [66, 111, 98, 0]);
    // 어떤 단계도 쓰지 않는 caller 바이트는 그대로 남는다.
    assert.equal(bytes[0x200], 0xa5);
    assert.equal(bytes[0x100], 0xa5);
    // 소유자 범위 밖은 효과 전에 거부한다.
    const before = scene.events.length;
    assert.throws(() => bindings.createAi(0), RangeError);
    assert.throws(() => bindings.createAi(9), RangeError);
    assert.equal(scene.events.length, before);
});

test('RawCampaignMissionStartBindings: FindLivePriest 는 dead 를 제외하고 같은 Start/게이트 주소를 돌려준다', () => {
    const scene = makeStartScene();
    const bindings = scene.make();
    assert.equal(bindings.findLivePriest(2), 5);
    // owner 3 는 dead 슬롯 6 을 건너뛰고 생존 슬롯 7 을 찾는다.
    assert.equal(bindings.findLivePriest(3), 7);
    assert.equal(bindings.findLivePriest(4), 0);
    assert.equal(bindings.gate(), scene.gate);
    assert.equal(typeof bindings.startBody().refreshTellAt, 'function');
});

// Tutorial 연결 장면: 같은 gate/start 상태와 실제 판정기 대역(번호별 생성 인자를 기록)이다. (2026-11-15 WB04-E520)
function makeTutorialScene({ number = 1, demo = 0, deadPriest = false, tellResult = true } = {}) {
    const events = [];
    const gateState = createMissionFrameGateState();
    gateState.started = 1;
    gateState.lockFrames = 0;
    gateState.tutorialNumber = number;
    gateState.demo = demo;
    const startState = createMissionStartState();
    const makers = [];
    const judge = (name) => ({ handle: (...args) => { events.push(`${name}:${JSON.stringify(args)}`); return 7; } });
    const session = {
        checkMissionOutcome: () => { events.push('outcome'); },
        localOwner: () => 1,
        missionPriestDead: () => deadPriest,
        makeTutorialOne: (gate) => { makers.push(['one', gate]); return judge('one'); },
        makeTutorialTwo: (gate, hooks) => { makers.push(['two', gate, hooks]); return judge('two'); },
        makeTutorialLater: (gate, state, hooks) => { makers.push(['later', gate, state, hooks]); return judge('later'); },
    };
    const script = makeScript({ tellAtList: 'x1;x2' });
    let refreshes = 0;
    const start = { startBody: () => ({ refreshTellAt: () => { refreshes += 1; events.push('refreshTellAt'); } }) };
    const effects = {
        frame: {
            gameSeconds: () => 10, paused: () => false, dialogsClear: () => true, knowledgeOpen: () => false, guideOpen: () => false, blockingGumpOpen: () => false,
            endDemo: () => { events.push('endDemo'); }, pause: () => { events.push('pause'); }, resume: () => { events.push('resume'); },
        },
        tell: (section) => { events.push(`tell:${section}`); return tellResult; },
        closeTransient: () => { events.push('closeTransient'); },
        selected: () => 0, resetSelection: () => {}, clearSelection: () => {},
    };
    return { events, gateState, startState, session, script, start, effects, makers, refreshes: () => refreshes, make: () => new RawCampaignTutorialOneBindings(session, script, start, gateState, startState, effects) };
}

test('RawCampaignTutorialOneBindings: 안내/UI 연결이 없으면 거부하고 데모·번호 밖 미션은 전용 판정기를 만들지 않는다', () => {
    const missing = makeTutorialScene();
    missing.effects.tell = undefined;
    assert.throws(() => missing.make(), TypeError);
    const closing = makeTutorialScene();
    closing.effects.closeTransient = undefined;
    assert.throws(() => closing.make(), TypeError);
    for (const options of [{ demo: 1, number: 1 }, { number: 0 }, { number: 7 }]) {
        const scene = makeTutorialScene(options);
        const bindings = scene.make();
        assert.equal(scene.makers.length, 0);
        // 판정기가 없어도 공통 프레임은 승패만 확인하고 전용 판정은 부르지 않는다(데모/번호 밖은 gate 가 호출하지 않음).
        bindings.frame();
        assert.deepEqual(scene.events, ['outcome']);
    }
});

test('RawCampaignTutorialOneBindings: 번호별 판정기를 같은 gate 에 연결하고 프레임은 사건 0 으로 한 번 실행한다', () => {
    const one = makeTutorialScene({ number: 1 });
    const bindingsOne = one.make();
    assert.equal(one.makers.length, 1);
    assert.equal(one.makers[0][0], 'one');
    assert.equal(one.makers[0][1], bindingsOne.gate());
    bindingsOne.frame();
    assert.deepEqual(one.events, ['outcome', 'one:[0]']);
    const two = makeTutorialScene({ number: 2 });
    two.make();
    assert.equal(two.makers[0][0], 'two');
    const hooksTwo = two.makers[0][2];
    assert.equal(hooksTwo.tell, two.effects.tell);
    assert.equal(hooksTwo.closeTransientGumps, two.effects.closeTransient);
    assert.equal(hooksTwo.gameSeconds, two.effects.frame.gameSeconds);
    assert.equal(hooksTwo.selected, two.effects.selected);
    // Early3~6 은 같은 Start 상태와 함께 만들고 현재 사건의 세 DWORD 를 번호와 함께 전달한다.
    for (const number of [3, 4, 5, 6]) {
        const later = makeTutorialScene({ number });
        const bindings = later.make();
        assert.equal(later.makers[0][0], 'later');
        assert.equal(later.makers[0][2], later.startState);
        bindings.event({ code: 0x0100aa, x: -2, y: 5 });
        assert.deepEqual(later.events, ['outcome', `later:[${number},[${0x0100aa},${(-2) >>> 0},5]]`]);
    }
});

test('RawCampaignTutorialOneBindings: First·다음 단계·다음 안내 명령은 공통 프레임을 추가 실행하지 않고 1 을 반환한다', () => {
    const scene = makeTutorialScene({ number: 1 });
    const bindings = scene.make();
    // First: 순번 0 은 플래그만 요청한다.
    assert.equal(bindings.event({ code: 0x02000091, x: 0, y: 0 }), 1);
    assert.equal(scene.startState.flags & 1, 1);
    assert.deepEqual(scene.events, []);
    // 순번이 올라간 뒤의 First 는 현재 단계 Tell 을 다시 실행한다(단계 65 = 'A').
    scene.startState.sequence = 1;
    assert.equal(bindings.event({ code: 0x02000091, x: 0, y: 0 }), 1);
    assert.deepEqual(scene.events, ['tell:A.', 'closeTransient', 'pause']);
    scene.events.length = 0;
    // 다음 단계 명령은 modifier 비트를 지우고 비교하며 gate.advance 만 실행한다.
    assert.equal(bindings.event({ code: 0x020000d3 | 0x08000000, x: 0, y: 0 }), 1);
    assert.equal(scene.gateState.stage, 66);
    assert.equal(scene.gateState.lockFrames, 10);
    assert.deepEqual(scene.events, []);
    // 다음 안내 명령은 TellAt 이다.
    assert.equal(bindings.event({ code: 0x020000d2, x: 0, y: 0 }), 1);
    assert.deepEqual(scene.events, ['tell:x1', 'closeTransient', 'pause', 'refreshTellAt']);
    // 그 밖의 사건은 공통 프레임으로 가고 판정기의 반환값을 돌려준다.
    scene.events.length = 0;
    scene.gateState.lockFrames = 0;
    assert.equal(bindings.event({ code: 0x1234, x: 1, y: 2 }), 7);
    assert.deepEqual(scene.events, ['outcome', `one:[${0x1234}]`]);
});

test('RawCampaignTutorialOneBindings: TellStage 는 단계 문자열·창 없음 결과 0·데모 섹션을 원본대로 쓴다', () => {
    const shown = makeTutorialScene({ number: 1, tellResult: true });
    const bindingsShown = shown.make();
    bindingsShown.tellStage();
    assert.deepEqual(shown.events, ['tell:A.', 'closeTransient', 'pause']);
    assert.equal(bindingsShown.stageShown(), 1);
    assert.equal(shown.gateState.scriptPaused, 1);
    // 창이 없으면 후속 정리 없이 결과 0 만 남긴다.
    const none = makeTutorialScene({ number: 1, tellResult: false });
    const bindingsNone = none.make();
    bindingsNone.tellStage();
    assert.deepEqual(none.events, ['tell:A.']);
    assert.equal(bindingsNone.stageShown(), 0);
    // 데모는 Demo 섹션이다.
    const demo = makeTutorialScene({ number: 1, demo: 1, tellResult: false });
    demo.make().tellStage();
    assert.deepEqual(demo.events, ['tell:Demo']);
});

test('RawCampaignTutorialOneBindings: TellAt 은 사제가 죽어도 인덱스 증가와 다음 예약을 실행한다', () => {
    const dead = makeTutorialScene({ number: 1, deadPriest: true });
    const bindingsDead = dead.make();
    bindingsDead.tellAt();
    assert.deepEqual(dead.events, ['refreshTellAt']);
    assert.equal(dead.startState.tellAtIndex, 1);
    const alive = makeTutorialScene({ number: 1 });
    const bindings = alive.make();
    bindings.tellAt();
    assert.deepEqual(alive.events, ['tell:x1', 'closeTransient', 'pause', 'refreshTellAt']);
    bindings.tellAt();
    // 두 번째 호출은 목록의 두 번째 항목이다.
    assert.equal(alive.events.filter((item) => item === 'tell:x2').length, 1);
    assert.equal(alive.startState.tellAtIndex, 2);
    assert.equal(alive.refreshes(), 2);
});

test('RawCampaignTutorialOneBindings: 판정기가 예외를 내도 현재 사건이 복원되어 다음 호출에 stale 인자가 남지 않는다', () => {
    const scene = makeTutorialScene({ number: 3 });
    let fail = true;
    scene.session.makeTutorialLater = (gate, state, hooks) => ({
        handle: (number, current) => {
            scene.events.push(`later:${JSON.stringify(current)}`);
            if (fail) throw new Error('판정 실패');
            return 5;
        },
    });
    const bindings = scene.make();
    assert.throws(() => bindings.event({ code: 0x10, x: 8, y: 9 }), /판정 실패/);
    fail = false;
    assert.equal(bindings.event({ code: 0x20, x: 1, y: 2 }), 5);
    assert.deepEqual(scene.events.filter((item) => item.startsWith('later:')), ['later:[16,8,9]', 'later:[32,1,2]']);
});
