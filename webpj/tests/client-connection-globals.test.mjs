// Client 수명 전역 세 묶음(ClientSubscriberGlobals·ClientConnectionActivityGlobals·ClientConnectionGlobals)의 계약을 cpppj 시험과 같은 장면으로 대조한다(WB04-E325, 4-C).
//  - 원본 독립 관측 subscriber-lifetime-edges-v2-x86.tsv 120행 중 Tutorial Init 의 현재 login 복사 지점(004c335c) 10행: 구독자 전역 상자를 실제 `initializeTutorialMission` 에 연결해 호출 순서와 값 변화를 재생한다.
//  - cpppj `SubscriberGlobalsTests`: PE 초기값·안정 참조·빌린 상자의 초기화 금지·독립 fallback·Chat 접두 getter 의 현재 값 조회. C++ 복사/이동/대입 의미 시험(CampaignWorldGlobals 값 복사)은 JS 에 없는 언어 규칙이라 옮기지 않는다.
//  - cpppj `ConnectionActivityTests`: 두 활동 표와 사이의 Zone DWORD 가 별개 필드이고 몸체가 바뀌어도 주소/값이 유지됨. 몸체 자체(서명64 변환·분배기 등록)는 E309 `raw-connection-activity.test.mjs` 가 맡는다.
//  - cpppj `ConnectionCleanup/Registration` 의 Client 연결표 시험: 같은 표 주소 유지·동기화 DWORD 대여(사본 없음). 실제 `Client`/`ClientLocalStateGlobals` 는 4-D 라서 `clockSynchronization()` 최소 계약 장치로 대체한다.
// (2026-11-12 WB04-E325: 4-C 세 Client 전역 이식과 함께 추가)
import assert from 'node:assert/strict';
import test from 'node:test';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';
import { CampaignWorldGlobals } from '../www/core/raw-campaign-world-load.js';
import { PacketConnection } from '../www/core/raw-packet-dispatch.js';
import { RawConnectionActivity, CONNECTION_ACTIVITY_ENTRIES } from '../www/core/raw-connection-activity.js';
import { RawConnectionRegistry } from '../www/core/raw-connection-registry.js';
import { initializeTutorialMission } from '../www/core/tutorial-init.js';
import { resetClockSynchronization } from '../www/core/raw-clock-sync-reset.js';
import {
    ClientSubscriberGlobals, DwordBox, LOGIN_SUBSCRIBER_INITIAL, MY_SUBSCRIBER_INITIAL, SUBSCRIBER_HANDLE_BYTES,
} from '../www/core/client-subscriber-globals.js';
import { AUTO_RESTART_INITIAL, ClientConnectionActivityGlobals, DoubleBox } from '../www/core/client-connection-activity-globals.js';
import { ClientConnectionGlobals } from '../www/core/client-connection-globals.js';

// 원본 관측 행이다. 파일이 없으면 해당 시험을 건너뛴다. (2026-11-12 WB04-E325)
const subscriberRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'subscriber-lifetime-edges-v2-x86.tsv'));

// 리틀엔디언 16진 8글자를 DWORD 로 읽는다(원본 칼럼은 낮은 BYTE 부터 이어진 4BYTE 이다). (2026-11-12 WB04-E325)
function dwordOf(hex) {
    assert.equal(hex.length, 8, 'Subscriber 원시 DWORD 폭');
    let value = 0;
    // 낮은 BYTE 부터 합친다.
    for (let byte = 0; byte < 4; byte += 1) value |= parseInt(hex.slice(byte * 2, byte * 2 + 2), 16) << (byte * 8);
    return value >>> 0;
}

// ---------------------------------------------------------------- ClientSubscriberGlobals

// PE 초기 두 DWORD 와 별도 현재 이름 버퍼: 같은 상자/배열을 계속 돌려주고 쓰기가 즉시 보이며 DWORD/BYTE 폭으로 감긴다.
test('Subscriber 전역: PE 초기값과 안정된 참조·별도 이름 버퍼', () => {
    const owner = new ClientSubscriberGlobals();
    // 원문 PE 초기값: my=-1(0xffffffff), login=1, 이름 64BYTE 는 전부 0 이다.
    assert.deepEqual([owner.mySubscriber().value, owner.loginSubscriber().value], [0xffffffff, 1]);
    assert.deepEqual([MY_SUBSCRIBER_INITIAL, LOGIN_SUBSCRIBER_INITIAL, SUBSCRIBER_HANDLE_BYTES], [0xffffffff, 1, 64]);
    assert.equal(owner.handle().length, 64);
    assert.ok(owner.handle().every((byte) => byte === 0));
    // 접근자는 호출마다 같은 객체를 돌려준다(C++ 의 같은 주소).
    assert.equal(owner.mySubscriber(), owner.mySubscriber());
    assert.equal(owner.loginSubscriber(), owner.loginSubscriber());
    assert.equal(owner.handle(), owner.handle());
    assert.notEqual(owner.mySubscriber(), owner.loginSubscriber());
    // 쓰기는 현재 값이며 DWORD 폭으로 감긴다(-1 → 0xffffffff, 2^32+5 → 5).
    owner.handle()[0] = 0x48;
    owner.handle()[1] = 0x4a;
    owner.handle()[2] = 0;
    owner.mySubscriber().value = 0x80000000;
    owner.loginSubscriber().value = 0xabcdef01;
    assert.deepEqual([owner.mySubscriber().value, owner.loginSubscriber().value], [0x80000000, 0xabcdef01]);
    assert.deepEqual([...owner.handle().subarray(0, 3)], [0x48, 0x4a, 0]);
    owner.mySubscriber().value = -1;
    owner.loginSubscriber().value = 2 ** 32 + 5;
    assert.deepEqual([owner.mySubscriber().value, owner.loginSubscriber().value], [0xffffffff, 5]);
    // 이름 버퍼는 두 DWORD 와 별개 저장소이다(DWORD 쓰기가 이름을 건드리지 않음).
    assert.deepEqual([...owner.handle().subarray(0, 3)], [0x48, 0x4a, 0]);
    // 두 소유자는 서로 독립이다(새 Client 의 PE 초기값을 다시 구성).
    const other = new ClientSubscriberGlobals();
    assert.deepEqual([other.mySubscriber().value, other.loginSubscriber().value], [0xffffffff, 1]);
    assert.notEqual(other.handle(), owner.handle());
});

// 상자 자체의 계약: 초기값 감김과 서로 다른 상자 사이의 독립성.
test('DwordBox/DoubleBox: 초기값 폭 감김과 독립 저장', () => {
    assert.equal(new DwordBox().value, 0);
    assert.equal(new DwordBox(-1).value, 0xffffffff);
    assert.equal(new DwordBox(2 ** 32 + 7).value, 7);
    const first = new DwordBox(3);
    const second = new DwordBox(3);
    first.value = 9;
    assert.deepEqual([first.value, second.value], [9, 3]);
    // double 상자는 DWORD 로 줄이지 않는다(소수와 큰 값, 음수 보존).
    const box = new DoubleBox();
    assert.equal(box.value, 0);
    box.value = 123456789.125;
    assert.equal(box.value, 123456789.125);
    box.value = -0.5;
    assert.equal(box.value, -0.5);
    assert.equal(new DoubleBox(2 ** 40).value, 2 ** 40);
});

// 명시 상자 대여는 외부 DWORD 에 쓰지 않으며 editor/missionPost 는 각 보기에 남는다.
test('Subscriber 전역: 빌린 CampaignWorldGlobals 는 초기화하지 않고 모드는 공유하지 않는다', () => {
    const owner = new ClientSubscriberGlobals();
    owner.mySubscriber().value = 0x12345678;
    owner.loginSubscriber().value = 0xfedcba98;
    const first = CampaignWorldGlobals.borrow({ my: owner.mySubscriber(), login: owner.loginSubscriber() });
    const second = CampaignWorldGlobals.borrow({ my: owner.mySubscriber(), login: owner.loginSubscriber() });
    // 빌리는 것만으로는 현재 값을 쓰지 않는다.
    assert.deepEqual([owner.mySubscriber().value, owner.loginSubscriber().value], [0x12345678, 0xfedcba98]);
    assert.deepEqual([first.mySubscriber, second.loginSubscriber], [0x12345678, 0xfedcba98]);
    first.editor = 1;
    first.missionPost = false;
    first.mySubscriber = 25;
    // Editor/post 는 각 Session 보기에만 남고 구독자 DWORD 는 같은 칸이다.
    assert.deepEqual([second.editor, second.missionPost, second.mySubscriber, owner.mySubscriber().value], [0, true, 25, 25]);
    // 소유자 쪽 쓰기도 모든 보기에서 바로 보인다.
    owner.loginSubscriber().value = 77;
    assert.deepEqual([first.loginSubscriber, second.loginSubscriber], [77, 77]);
});

// 한쪽 상자만 빌리면 생략한 쪽은 독립 0 저장소를 쓰고 외부 값을 건드리지 않는다.
test('Subscriber 전역: 일부 상자 생략은 독립 0 저장소', () => {
    const owner = new ClientSubscriberGlobals();
    owner.mySubscriber().value = 17;
    owner.loginSubscriber().value = 29;
    const empty = CampaignWorldGlobals.borrow({});
    const other = CampaignWorldGlobals.borrow({});
    const myOnly = CampaignWorldGlobals.borrow({ my: owner.mySubscriber() });
    const loginOnly = CampaignWorldGlobals.borrow({ login: owner.loginSubscriber() });
    assert.deepEqual([empty.mySubscriber, empty.loginSubscriber, empty.editor, empty.missionPost], [0, 0, 0, true]);
    empty.mySubscriber = 71;
    empty.loginSubscriber = 81;
    assert.deepEqual([other.mySubscriber, other.loginSubscriber, owner.mySubscriber().value, owner.loginSubscriber().value], [0, 0, 17, 29]);
    assert.deepEqual([myOnly.mySubscriber, myOnly.loginSubscriber, loginOnly.mySubscriber, loginOnly.loginSubscriber], [17, 0, 0, 29]);
    // 생략한 쪽 쓰기는 외부 상자에 닿지 않고, 빌린 쪽 쓰기는 외부 상자에 닿는다.
    myOnly.loginSubscriber = 91;
    loginOnly.mySubscriber = 101;
    assert.deepEqual([owner.mySubscriber().value, owner.loginSubscriber().value], [17, 29]);
    myOnly.mySubscriber = 37;
    loginOnly.loginSubscriber = 49;
    assert.deepEqual([owner.mySubscriber().value, owner.loginSubscriber().value], [37, 49]);
    // 독립 기본 값 생성자는 값 입력을 자기 저장소로 둔다.
    const one = new CampaignWorldGlobals({ my: 7 });
    const two = new CampaignWorldGlobals({ my: 8, login: 9 });
    assert.deepEqual([one.mySubscriber, one.loginSubscriber, two.mySubscriber, two.loginSubscriber], [7, 0, 8, 9]);
});

// Chat 접두가 빌리는 구독자 조회는 복제 값이 아니며 소유자를 유지하는 소비자가 나중 쓰기도 본다.
test('Subscriber 전역: Chat 접두 getter 는 현재 값을 다시 읽는다', () => {
    let owner = new ClientSubscriberGlobals();
    const retained = owner;
    // Chat manager 접두의 subscriber 장치와 같은 형태(`() => 현재 DWORD`)이다.
    const chat = { current: { value: 0 }, subscriber: () => retained.mySubscriber().value, coreDataActive: () => 0 };
    // 첫 소유 참조를 버려도 소비자가 쥔 참조로 같은 칸이 살아 있다.
    owner = null;
    assert.equal(chat.subscriber(), 0xffffffff);
    retained.mySubscriber().value = 71;
    assert.equal(chat.subscriber(), 71);
});

// 실제 Tutorial Init 이 fileName callback 뒤 현재 login 을 복사하는 10개 원문 checkpoint 와 대조한다(cpppj SubscriberGlobals_TutorialInitUsesCurrentLoginAfterConfiguration).
test('Subscriber 전역: Tutorial Init 이 설정 뒤 현재 login 을 쓴다 (원본 관측 10행)', { skip: !subscriberRows && 'subscriber-lifetime-edges-v2-x86.tsv 없음' }, () => {
    let selected = 0;
    // 모든 관측 행의 칼럼 수를 확인하고 004c335c 지점 행만 재생한다.
    for (const fields of subscriberRows) {
        assert.equal(fields.length, 10, 'Subscriber 원시 칼럼 수');
        if (fields[0] !== '004c335c') continue;
        assert.equal(fields[2], 'currentLoginLoad');
        const owner = new ClientSubscriberGlobals();
        // Native 진입 때의 my 만 명시 입력한다.
        const entryMy = dwordOf(fields[5].slice(0, 8));
        owner.mySubscriber().value = entryMy;
        const state = { name: 'Tutorial1', namePrefix: 0, scratch: new Uint8Array(80).fill(0xa5) };
        const flags = { value: 0 };
        const demo = { value: 0 };
        const globals = {
            tutorialNumber: { value: 0 },
            missionNumber: { value: 0 },
            tutorialSeen: { value: 0 },
            // 이름과 달리 Tutorial Init 의 `loginSubscriber` 쓰기 대상은 00540c74(my)이고, 읽는 값 `initialSubscriber` 는 0054265c(login)이다.
            // cpppj 시험도 `TutorialInitGlobals{…, owner.MySubscriber(), owner.LoginSubscriber()}` 순서로 연결한다(원본 관측 actualStores 가 00540c74 쓰기).
            loginSubscriber: owner.mySubscriber(),
            initialSubscriber: () => owner.loginSubscriber().value,
        };
        const trace = [];
        const hooks = {
            clearTemplates: () => { trace.push('clear'); },
            resetOptions: () => { trace.push('reset'); },
            get: (key, fallback) => (key === 'demo' ? '' : (fallback ?? '0')),
            set: (key, value) => {
                assert.deepEqual([key, value], ['fileName', 'Tutorial1']);
                assert.equal(owner.mySubscriber().value, entryMy);
                // fileName 설정 callback 이 로그인 구독자를 바꾼다(원본 관측 입력).
                owner.loginSubscriber().value = Number(BigInt(fields[3]) & 0xffffffffn);
                trace.push('set');
            },
            requestFort: (name) => {
                assert.equal(name, 'Tutorial1');
                assert.equal(owner.mySubscriber().value, dwordOf(fields[6].slice(0, 8)));
                trace.push('request');
            },
            header: () => {
                assert.equal(owner.loginSubscriber().value, dwordOf(fields[6].slice(8, 16)));
                trace.push('header');
            },
        };
        initializeTutorialMission(state, flags, demo, globals, hooks);
        assert.equal(flags.value, 1);
        assert.deepEqual(trace, ['clear', 'reset', 'set', 'request', 'header']);
        selected += 1;
    }
    assert.deepEqual([subscriberRows.length, selected], [120, 10]);
});

// ---------------------------------------------------------------- ClientConnectionActivityGlobals

// 활동 표 시험 장치: 실제 시계/프레임/서버 입력과 호출 순서만 관측한다(cpppj ActivityDevice).
function makeActivityDevice() {
    const device = {
        globals: new ClientConnectionActivityGlobals(),
        wall: 100.75,
        frame: 7,
        server: false,
        trace: [],
        erased: [],
        // 실제 네 장치의 기본값이며 각 검사는 필요한 효과만 바꾼다.
        hooks() {
            return {
                wallSeconds: () => { device.trace.push('wall'); return device.wall; },
                frameNumber: () => { device.trace.push('frame'); return device.frame; },
                server: () => { device.trace.push('server'); return device.server; },
                eraseStatus: (kind, player) => { device.trace.push('erase'); device.erased.push([kind, player]); },
            };
        },
    };
    return device;
}

// 원문 Wall 의 부수 효과 뒤 현재 frame/server 를 읽고 두 store 사이에 server 조회를 둔다. 마지막 UI 효과가 바꾼 같은 표를 사본으로 복구하지 않는다.
test('활동 전역: Wall→Frame→두 store 순서와 현재 server 조회·표 복구 금지', () => {
    const device = makeActivityDevice();
    device.globals.frames().fill(0xaaaaaaaa);
    device.globals.wallSeconds().fill(0xbbbbbbbb);
    const hooks = device.hooks();
    hooks.wallSeconds = () => {
        device.trace.push('wall');
        // 아직 어떤 store 도 일어나지 않았다.
        assert.deepEqual([device.globals.frames()[9], device.globals.wallSeconds()[9]], [0xaaaaaaaa, 0xbbbbbbbb]);
        device.frame = 0x12345678;
        device.server = true;
        return device.wall;
    };
    hooks.server = () => {
        device.trace.push('server');
        // 벽시각 store 뒤, 프레임 store 앞에서 server 를 읽는다.
        assert.deepEqual([device.globals.wallSeconds()[9], device.globals.frames()[9]], [100, 0xaaaaaaaa]);
        device.frame = 999;
        return device.server;
    };
    const erase = hooks.eraseStatus;
    hooks.eraseStatus = (kind, player) => {
        assert.deepEqual([device.globals.frames()[9], device.globals.wallSeconds()[9]], [0x12345678, 100]);
        erase(kind, player);
        // UI 효과가 같은 표를 바꾼다.
        device.globals.frames()[9] = 0x55555555;
        device.globals.wallSeconds()[9] = 0x66666666;
    };
    new RawConnectionActivity(device.globals.tables(), hooks).receive(9);
    assert.deepEqual(device.trace, ['wall', 'frame', 'server', 'erase']);
    assert.deepEqual(device.erased, [[2, 9]]);
    assert.deepEqual([device.globals.frames()[9], device.globals.wallSeconds()[9]], [0x55555555, 0x66666666]);
});

// 비서버 분기의 player 는 sender 와 무관하게 0 이며 frame 0 과 음수 벽시각의 하위 DWORD 를 보존한다.
test('활동 전역: 비서버 삭제 인자와 frame 0·음수 벽시각 비정규화', () => {
    const device = makeActivityDevice();
    device.wall = -1.5;
    device.frame = 0;
    new RawConnectionActivity(device.globals.tables(), device.hooks()).receive(21);
    assert.deepEqual([device.globals.frames()[21], device.globals.wallSeconds()[21]], [0, 0xffffffff]);
    assert.deepEqual(device.erased, [[1, 0]]);
});

// 최초 BSS 0·두 표의 경계·사이 Zone DWORD 와 다음 몸체에 대여한 동일 주소를 함께 검사한다.
test('활동 전역: 최초 0·독립 두 표·Zone 간극 DWORD 가 몸체를 바꿔도 유지된다', () => {
    const device = makeActivityDevice();
    const globals = device.globals;
    assert.deepEqual([globals.activeStamp().value, globals.activeZoneWord().value], [0, 0]);
    // 하트비트 기한은 최초 0(double), 자동 재시작은 PE 초기 6 이다.
    assert.deepEqual([globals.heartbeatEnd().value, globals.autoRestart().value, AUTO_RESTART_INITIAL], [0, 6, 6]);
    // 두 표는 22칸 Uint32Array 이며 서로 다른 메모리이고 접근자는 같은 배열을 돌려준다.
    const { frames, wallSeconds } = globals.tables();
    assert.ok(frames instanceof Uint32Array && wallSeconds instanceof Uint32Array);
    assert.deepEqual([frames.length, wallSeconds.length, CONNECTION_ACTIVITY_ENTRIES], [22, 22, 22]);
    assert.equal(frames, globals.frames());
    assert.equal(wallSeconds, globals.wallSeconds());
    assert.notEqual(frames.buffer, wallSeconds.buffer);
    // 정상 22칸만 초기화/관측하며 인접 Native 필드를 표 항목으로 합치지 않는다.
    for (let index = 0; index < CONNECTION_ACTIVITY_ENTRIES; index += 1) {
        assert.deepEqual([frames[index], wallSeconds[index]], [0, 0]);
        frames[index] = 0x1000 + index;
        wallSeconds[index] = 0x2000 + index;
    }
    globals.activeStamp().value = 0x12345678;
    globals.activeZoneWord().value = 0x87654321;
    const beforeFrames = Uint32Array.from(frames);
    const beforeWall = Uint32Array.from(wallSeconds);
    new RawConnectionActivity(globals.tables(), device.hooks()).receive(21);
    // 마지막 정상 인덱스만 바뀌고 나머지 21칸은 그대로이다.
    for (let index = 0; index < CONNECTION_ACTIVITY_ENTRIES - 1; index += 1) assert.deepEqual([frames[index], wallSeconds[index]], [beforeFrames[index], beforeWall[index]]);
    device.wall = 111.75;
    device.frame = 88;
    new RawConnectionActivity(globals.tables(), device.hooks()).receive(0);
    // 두 번째 몸체도 같은 배열을 빌렸고 첫 몸체가 쓴 마지막 칸은 그대로이며 Zone 간극은 건드리지 않는다.
    assert.equal(globals.tables().frames, frames);
    assert.equal(globals.tables().wallSeconds, wallSeconds);
    assert.deepEqual([frames[0], wallSeconds[0], frames[21], wallSeconds[21]], [88, 111, 7, 100]);
    assert.deepEqual([globals.activeStamp().value, globals.activeZoneWord().value], [0x12345678, 0x87654321]);
});

// 두 표 정리(프레임 22DWORD 만 지움)가 간극·하트비트·자동 재시작을 암묵 초기화하지 않는 계약(소유자 분리).
test('활동 전역: Zone·하트비트·자동 재시작 칸은 표와 별개로 보존된다', () => {
    const globals = new ClientConnectionActivityGlobals();
    globals.activeStamp().value = 0xdeadbeef;
    globals.activeZoneWord().value = 0xcafebabe;
    globals.heartbeatEnd().value = 1234.5;
    globals.autoRestart().value = 1;
    globals.frames().fill(0x11111111);
    globals.wallSeconds().fill(0x22222222);
    // 프레임 표만 지우는 정리(cleanup 의 22DWORD)는 다른 칸을 만지지 않는다.
    globals.frames().fill(0);
    assert.deepEqual([globals.activeStamp().value, globals.activeZoneWord().value, globals.heartbeatEnd().value, globals.autoRestart().value], [0xdeadbeef, 0xcafebabe, 1234.5, 1]);
    assert.ok(globals.wallSeconds().every((value) => value === 0x22222222));
    // 상자 접근자는 호출마다 같은 객체를 돌려준다.
    assert.equal(globals.activeStamp(), globals.activeStamp());
    assert.equal(globals.heartbeatEnd(), globals.heartbeatEnd());
    assert.equal(globals.autoRestart(), globals.autoRestart());
    // 새 소유자는 다시 PE 초기값이다.
    const fresh = new ClientConnectionActivityGlobals();
    assert.deepEqual([fresh.heartbeatEnd().value, fresh.autoRestart().value, fresh.activeStamp().value], [0, 6, 0]);
});

// 정상 연결 인덱스 밖은 첫 Wall 효과 전에 거부하고 표·간극을 건드리지 않는다.
test('활동 전역: 잘못된 sender 는 Wall 이나 store 전에 거부한다', () => {
    const device = makeActivityDevice();
    device.globals.frames().fill(0x11111111);
    device.globals.wallSeconds().fill(0x22222222);
    device.globals.activeStamp().value = 33;
    device.globals.activeZoneWord().value = 44;
    const frames = Uint32Array.from(device.globals.frames());
    const wall = Uint32Array.from(device.globals.wallSeconds());
    const activity = new RawConnectionActivity(device.globals.tables(), device.hooks());
    // 정상 22칸 앞/뒤와 signed 양끝을 따로 확인한다.
    for (const sender of [-1, -22, 22, -(2 ** 31), 2 ** 31 - 1]) {
        assert.throws(() => activity.receive(sender), RangeError);
        assert.deepEqual([device.trace, device.erased], [[], []]);
        assert.deepEqual([device.globals.frames(), device.globals.wallSeconds()], [frames, wall]);
        assert.deepEqual([device.globals.activeStamp().value, device.globals.activeZoneWord().value], [33, 44]);
    }
});

// ---------------------------------------------------------------- ClientConnectionGlobals

// 시간 동기화 owner 최소 계약 장치이다(실제 ClientLocalStateGlobals 의 clockSynchronization 형태). (2026-11-12 WB04-E325)
function makeLocal(initial = 0) {
    const sync = { connection: initial >>> 0, replyMode: 0, gameTicks: 0 };
    return { sync, local: { clockSynchronization: () => sync } };
}

// 인자 없는 생성은 독립 표(최초 BSS 0)이고 등록소 기능을 그대로 쓴다.
test('연결표 전역: 인자 없는 생성은 독립 고정 표와 발급기를 만든다', () => {
    const globals = new ClientConnectionGlobals();
    assert.ok(globals instanceof RawConnectionRegistry);
    const view = globals.cleanup();
    assert.deepEqual([view.transceivers.length, view.buffers.length, view.pending.length, view.pendingCount.value, view.synchronization.value], [22, 22, 32, 0, 0]);
    // 같은 객체의 표는 호출마다 같은 배열이다(주소 고정).
    assert.equal(globals.cleanup().transceivers, view.transceivers);
    assert.equal(globals.cleanup().buffers, view.buffers);
    assert.equal(globals.cleanup().pending, view.pending);
    // 두 전역은 서로 독립이다.
    assert.notEqual(new ClientConnectionGlobals().cleanup().transceivers, view.transceivers);
});

// 같은 Client 의 표/개수는 Session 획득/반납과 독립이며 실제 동기화 DWORD 만 빌린다(cpppj ConnectionCleanup_ClientLongTablesPreserveAddressAndCurrentValues).
test('연결표 전역: 동기화 DWORD 를 사본 없이 빌리고 표 주소·현재 값을 유지한다', () => {
    const { sync, local } = makeLocal(0x1234);
    const globals = new ClientConnectionGlobals(local);
    // 빌리는 것만으로는 현재 값을 쓰지 않는다.
    assert.equal(sync.connection, 0x1234);
    assert.equal(globals.cleanup().synchronization.value, 0x1234);
    const view = globals.cleanup();
    view.transceivers[21] = 71;
    view.buffers[21] = 72;
    view.pending[31] = 73;
    view.pendingCount.value = 0x89abcdef;
    // 동기화 쓰기는 owner 의 같은 칸에 DWORD 로 감겨 닿는다.
    view.synchronization.value = 74;
    assert.equal(sync.connection, 74);
    // owner 쪽 쓰기도 등록소가 바로 본다(사본이 아님).
    sync.connection = 0xfffffffe;
    assert.equal(globals.cleanup().synchronization.value, 0xfffffffe);
    view.synchronization.value = -1;
    assert.equal(sync.connection, 0xffffffff);
    // Session 이 값을 쓰고 반납하는 동안에도 표 주소와 현재 값은 그대로이다.
    const current = globals.cleanup();
    assert.equal(current.transceivers, view.transceivers);
    assert.deepEqual([current.transceivers[21], current.buffers[21], current.pending[31], current.pendingCount.value], [71, 72, 73, 0x89abcdef]);
    // 같은 owner 의 시간 동기화 초기화(004c1f20)는 등록소의 동기화 칸도 0 으로 보이게 한다.
    resetClockSynchronization(local.clockSynchronization());
    assert.equal(globals.cleanup().synchronization.value, 0);
    // 기존 직접 owner 검사는 같은 표에 명시 동기화 상자를 빌릴 수 있다.
    const explicit = { value: 5 };
    assert.equal(globals.cleanup(explicit).synchronization, explicit);
    assert.equal(globals.cleanup(explicit).transceivers, view.transceivers);
});

// owner 가 없거나 시간 동기화 계약이 없으면 거부한다(C++ invalid_argument).
test('연결표 전역: 동기화 owner 누락은 거부한다', () => {
    assert.throws(() => new ClientConnectionGlobals(null), /연결표의 실제 Client 동기화 owner 누락/);
    assert.throws(() => new ClientConnectionGlobals(undefined), /연결표의 실제 Client 동기화 owner 누락/);
    assert.throws(() => new ClientConnectionGlobals({}), TypeError);
    assert.throws(() => new ClientConnectionGlobals({ clockSynchronization: () => ({ replyMode: 0 }) }), TypeError);
});

// 앱 최종 종료: 전역 표에 남은 노드와 초기 패킷을 명시 정리하고(등록소 규칙) 새 노드 발급을 막는다. 진행 중 부모 안에서는 거부된다.
test('연결표 전역: shutdownDevices 는 잔류 노드를 정리하고 새 발급을 막는다', () => {
    const { local } = makeLocal();
    const globals = new ClientConnectionGlobals(local);
    const events = [];
    const node = new PacketConnection();
    const token = globals.track(node, () => { events.push('close'); }, () => { events.push('free'); });
    globals.cleanup().transceivers[3] = token;
    // 수신 중(깊이>0)이면 거부하고 발급 차단도 걸지 않는다.
    node.depth = 1;
    assert.throws(() => globals.shutdownDevices(), /진행중Node배송/);
    node.depth = 0;
    globals.shutdownDevices();
    assert.deepEqual(events, ['close', 'free']);
    assert.equal(globals.owns(token), false);
    assert.equal(globals.cleanup().transceivers[3], 0);
    assert.throws(() => globals.track(new PacketConnection(), () => {}, () => {}), /최종종료/);
});

// Client 보다 오래 빌린 연결표도 동기화 owner 를 유지한다(GC 가 C++ shared_ptr 수명 보유를 대신한다).
test('연결표 전역: 빌린 동기화 owner 를 원래 참조를 놓은 뒤에도 쓴다', () => {
    const { sync, local } = makeLocal();
    let holder = local;
    const globals = new ClientConnectionGlobals(holder);
    globals.cleanup().synchronization.value = 71;
    assert.equal(sync.connection, 71);
    holder = null;
    // 원래 owner 참조를 놓아도 등록소의 빌린 상자는 같은 칸을 가리킨다.
    assert.equal(globals.cleanup().synchronization.value, 71);
    globals.cleanup().synchronization.value = 72;
    assert.equal(sync.connection, 72);
});
