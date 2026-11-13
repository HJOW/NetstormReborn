// RawCursorDrop을 cpppj CursorDropTests와 원본 cursordrop-x86.tsv originals 686행·cursor-preview-width-x86.tsv originals 140행으로 대조한다.
// CD/10.37·실제 게임 입력/음향은 범위 밖이며 GUI 생산 칸의 실제 반환 연결은 RawCursorReturnPiece 단위에서 한다. (2026-11-14 WB04-E398)
import './nan-exact.mjs';
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, bitsToFloat, floatToBits, put, get } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { CursorDropState, RawCursorDrop, makeCursorDropHooks } from '../www/core/raw-cursor-drop.js';

// 두 독립 관측의 10.78 행이다. 다른 판본은 웹 지원 범위 밖이다.
const rows = readRows(path.join(REPO_ROOT, 'cpppj/tests/fixtures/cursordrop-x86.tsv'))?.filter((row) => row[0] === 'originals');
const widthRows = readRows(path.join(REPO_ROOT, 'cpppj/tests/fixtures/cursor-preview-width-x86.tsv'))?.filter((row) => row[0] === 'originals');
// 원본 실행기 입력 열의 순서다. 좌표/비용도 DWORD 비트 입력으로 보존한다.
const [LOCK, LEGAL, X, Y, INPUT, COST_FLAGS, GUI, PROCESS, RECORD, ARG, DIRECTION, FLAGS, QUALITY, ALLOWED, FIRST, SECOND,
    COST, FOOT, SCREEN_X, SCREEN_Y, WIDTH, BATTLE, BLOCKED, QUERY_CHANGE, PREPARE_CHANGE, SOURCE_CHANGE, DETACH_CHANGE,
    RECEIVE_CHANGE, LOCAL_CHANGE, ADD_CHANGE] = Array.from({ length: 30 }, (_, index) => index);
// 미리보기와 원본 TYPE 번호다. 새 제품 객체를 관측용 번호로 생성하지 않는다.
const PREVIEW = 50; const TYPE = 100;

// 각 관측의 입력·명시 하위 효과만 공급하는 장면이다. 기대 사건과 최종 상태는 fixture에서 읽는다.
class DropScene {
    // 같은 풀·TYPE 표·현재 전역·입력·사건과 콜백 호출 횟수다.
    pool = new SidPool(24000, { server: false });
    types = Array.from({ length: 188 }, () => ({ cost: 0, footX: 0, name: '' }));
    state = new CursorDropState(); input = []; events = []; prepared = 0; sources = 0;

    // 호스트가 쓰는 미리보기 50까지 한 번만 확보한다.
    constructor() {
        // 행을 바꿀 때 할당/게임 난수를 다시 소비하지 않고 같은 물리 슬롯을 재사용한다.
        for (let sid = 5; sid <= PREVIEW; sid += 1) assert.equal(this.pool.allocate(2), sid);
    }

    // 원본과 같은 입력 필드만 지정하고 raw의 나머지는 0으로 둔다.
    prepare(values = rows[0][1].split(',').map(Number)) {
        this.input = values; this.events = []; this.prepared = 0; this.sources = 0;
        const v = values; const s = new CursorDropState(); this.state = s;
        Object.assign(s, { lockDepth: v[LOCK], legal: v[LEGAL], preview: PREVIEW, x: bitsToFloat(v[X]), y: bitsToFloat(v[Y]),
            inputFlags: v[INPUT], costFlags: v[COST_FLAGS], legalGump: v[GUI] ? 1 : 0, cursorGump: v[PROCESS] ? 2 : 0,
            recording: v[RECORD], recorder: 27, type: TYPE, argument: v[ARG], direction: v[DIRECTION],
            screenX: v[SCREEN_X] | 0, screenY: v[SCREEN_Y] | 0, cursorWidth: v[WIDTH] | 0, battle: v[BATTLE], blocked: v[BLOCKED] });
        Object.assign(this.types[TYPE], { cost: bitsToFloat(v[COST]), footX: v[FOOT] | 0, name: 'cursorType' });
        const raw = this.pool.allocated(PREVIEW); raw.fill(0); put(raw, 20, v[FLAGS], 2); raw[30] = v[QUALITY];
        return this;
    }

    // 원본 실행기와 같은 사건 인수의 부호 없는 십진 표기다.
    append(letter, ...values) { this.events.push([letter, ...values.map((value) => value >>> 0)].join(':')); }

    // 원본 관측이 지정한 하위 효과만 제공한다. 기대 분기/상태는 재계산하지 않는다.
    hooks() {
        const s = this.state; const v = this.input; const append = (letter, ...values) => this.append(letter, ...values);
        return {
            queryCursor: () => { append('Q', 0); if (v[QUERY_CHANGE] === 1) s.legal ^= 1; if (v[QUERY_CHANGE] === 2) s.lockDepth = 7; },
            prepareCursor: () => {
                append('P', ++this.prepared, floatToBits(s.x), floatToBits(s.y), s.preview);
                if (v[PREPARE_CHANGE] === 1 && this.prepared === 1) { s.x = Math.fround(-0.500001); s.y = Math.fround(0.499999); }
                if (v[PREPARE_CHANGE] === 2 && this.prepared === 2) { s.x = 20.25; s.y = 21.75; s.preview = 51; }
            },
            highlight: (value) => append('H', value),
            playDrop: () => this.events.push('S:droppiece.wav:0:0:0:0:0'),
            previewSource: (sid) => {
                const result = v[++this.sources === 1 ? FIRST : SECOND]; append('U', sid, result);
                if (v[SOURCE_CHANGE] && this.sources === 2) { const raw = this.pool.allocated(sid); put(raw, 20, v[FLAGS] ^ 0x108, 2); raw[30] = v[QUALITY] ^ 3; }
                return result;
            },
            destroy: (sid, flags) => append('D', sid, flags),
            pop: (sid, x, y, flags) => append('O', sid, floatToBits(x), floatToBits(y), flags),
            record: (recorder, name, x, y, argument, direction) => this.events.push(`R:${recorder}:${name}:${floatToBits(x)}:${floatToBits(y)}:${argument}:${direction}`),
            placeLocal: (request) => {
                append('L', request.preview, floatToBits(request.x), floatToBits(request.y), request.argument, request.direction);
                if (v[LOCAL_CHANGE] === 1) s.lockDepth = 0; if (v[LOCAL_CHANGE] === 2) s.costFlags ^= 0x40;
            },
            addLocalSp: (delta) => { append('A', floatToBits(delta)); if (v[ADD_CHANGE]) { this.types[TYPE].cost = 9.75; s.screenX = -17; s.cursorWidth = -3; } },
            showSp: (amount, x, y, flags) => append('V', amount, x, y, flags),
            battleRefresh: () => append('B', 0), invalidGui: () => append('I', 0),
            guiFlags: (gump) => { append('G', gump, 901, 0, 0); return v[ALLOWED]; }, guiParent: () => 123,
            detach: (sid, flags) => { append('N', sid, flags); if (v[DETACH_CHANGE]) { const raw = this.pool.allocated(sid); put(raw, 20, 0xffff, 2); raw[30] = 0xfe; } },
            attach: (sid, parent, flags) => append('J', sid, parent, flags),
            guiReceive: (gump, sid) => { append('G', gump, 902, sid, 0); if (v[RECEIVE_CHANGE]) s.inputFlags ^= 0x80; },
            returnPiece: (flags, source, quality) => append('T', flags, source, quality),
            activatePreview: (value) => append('E', value), selectCursor: (value) => append('C', value),
            refresh: (a, b, c) => append('F', a, b, c), closeGump: (gump, a, b, c) => append('X', gump, a, b, c),
        };
    }

    // 최종 전역/raw 14개를 원본 관측 순서 그대로 직렬화한다.
    snapshot() {
        const s = this.state; const raw = this.pool.slot(PREVIEW);
        return [s.lockDepth, s.legal, s.preview, floatToBits(s.x), floatToBits(s.y), s.inputFlags, s.costFlags, s.legalGump ? 1 : 0,
            s.cursorGump ? 1 : 0, s.argument, s.direction, floatToBits(this.types[TYPE].cost), get(raw, 20, 2), raw[30]].join(',');
    }

    // 현재 장면의 실제 제품 처리기다.
    body(hooks = this.hooks()) { return new RawCursorDrop(this.pool, this.types, this.state, hooks); }
}

test('CursorDrop_ReplaysOriginals: 원본 10.78 686행의 호출 순서·DWORD 인수·최종 전역/raw 14개가 일치한다', { skip: rows === undefined }, () => {
    assert.equal(rows.length, 686);
    const scene = new DropScene();
    // 각 행의 입력만 공급하고 결과는 저장 관측과 직접 비교한다.
    for (const row of rows) {
        assert.equal(row.length, 4);
        const values = row[1].split(',').map(Number); assert.equal(values.length, 30);
        scene.prepare(values); scene.body().drop();
        assert.equal(scene.events.join(';') || '-', row[2], row[1]);
        assert.equal(scene.snapshot(), row[3], row[1]);
    }
});

// 명시한 준비 단계에서 중단하는 관측 장치다. 제품이 삼키지 않아야 한다.
class PrepareStop extends Error {}

test('CursorPreviewDword_Native280CaptureAndPreparePrefix: 10.78 140행의 전체 DWORD·준비 중단·현재값 변경 부분 효과가 일치한다', { skip: widthRows === undefined }, () => {
    assert.equal(widthRows.length, 140);
    const scene = new DropScene(); let stops = 0;
    // CD를 제외한 저장 관측만 공급한다. 중단 이후 효과를 성공으로 채우지 않는다.
    for (const row of widthRows) {
        assert.equal(row.length, 10); scene.prepare(row[6].split(',').map(Number)); scene.state.preview = Number(row[2]);
        const hooks = scene.hooks(); const prepare = hooks.prepareCursor;
        hooks.prepareCursor = () => { prepare(); if (scene.prepared === Number(row[4])) scene.state.preview = Number(row[5]); if (scene.prepared === Number(row[3])) throw new PrepareStop(); };
        let completed = true;
        try { scene.body(hooks).drop(); } catch (error) { if (!(error instanceof PrepareStop)) throw error; completed = false; stops += 1; }
        assert.equal(completed, row[7] === '1', row.slice(0, 6).join(','));
        assert.equal(scene.events.join(';') || '-', row[8]); assert.equal(scene.snapshot(), row[9]);
    }
    assert.equal(stops, 20);
});

test('CursorDrop_RejectsInvalidInputs: 필수 경계·풀 판본·TYPE·NaN·부모/연관 SID를 소비 위치에서 거부하고 부분 효과를 보존한다', () => {
    const scene = new DropScene().prepare(); const hooks = scene.hooks();
    assert.throws(() => new RawCursorDrop(scene.pool, [], scene.state, hooks), /자료\/효과/);
    assert.throws(() => new RawCursorDrop({ edition: 'cd1072' }, scene.types, scene.state, hooks), /자료\/효과/);
    // 모든 필수 장치의 누락을 개별로 검사한다.
    for (const name of Object.keys(hooks)) assert.throws(() => scene.body({ ...hooks, [name]: undefined }), /자료\/효과/, name);
    const body = scene.body(); assert.equal(body.pool, scene.pool);
    scene.state.lockDepth = 1; scene.state.preview = 0; body.drop(); assert.deepEqual(scene.events, []);
    scene.state.lockDepth = 0; assert.throws(() => body.drop(), /미리보기 DWORD/); assert.equal(scene.state.lockDepth, 1);
    scene.prepare(); scene.state.x = NaN; assert.throws(() => scene.body().drop(), /유한/); assert.equal(scene.prepared, 1);
    scene.prepare(); scene.state.recording = 1; scene.state.type = 300; assert.throws(() => scene.body().drop(), /타입 번호/);
    scene.prepare(); scene.state.legalGump = 1;
    assert.throws(() => scene.body({ ...scene.hooks(), guiParent: () => 0 }).drop(), /parentSid/);
    assert.deepEqual([get(scene.pool.slot(PREVIEW), 20, 2), scene.state.preview, scene.state.lockDepth], [0x100, PREVIEW, 0]);
    scene.prepare(); scene.state.inputFlags = 0x80;
    assert.throws(() => scene.body({ ...scene.hooks(), previewSource: () => 0xffffffff }).drop(), /연관 SID/);
});

test('CursorDrop_PreservesRejectedGuiAndLocksConstruction: GUI 거부와 두 잠금 뒤 재호출은 현재 미리보기/Process를 보존한다', () => {
    const values = rows[0][1].split(',').map(Number); values[GUI] = 1; values[ALLOWED] = 8;
    const scene = new DropScene().prepare(values); scene.body().drop();
    assert.equal(scene.events.at(-1), 'I:0'); assert.deepEqual([scene.state.preview, scene.state.cursorGump, scene.state.lockDepth], [50, 2, 0]);
    assert.equal(get(scene.pool.slot(PREVIEW), 20, 2), 8);
    values[GUI] = 0; scene.prepare(values); const body = scene.body(); body.drop();
    assert.deepEqual([scene.state.lockDepth, scene.state.preview, scene.state.cursorGump], [2, 0, 0]);
    const before = scene.events.slice(); body.drop(); assert.deepEqual(scene.events, before);
});

test('CursorPreviewDword_DropHostGuardDoesNotAliasLowWord: 큰 DWORD는 로컬/이동/GUI 소비 전 효과만 남기고 하위 WORD 슬롯을 건드리지 않는다', () => {
    const scene = new DropScene();
    // 서로 다른 높은 WORD와 세 실제 소비 경로를 각각 검사한다.
    for (const number of [0x10032, 0x80000032, 0xffff0032]) {
        // 로컬 배치는 첫 잠금을 수행한 뒤 번호를 검사하고 나머지는 잠그기 전에 검사한다.
        for (const mode of [0, 1, 2]) {
            const values = rows[0][1].split(',').map(Number); values[INPUT] = mode === 1 ? 0x80 : 0; values[GUI] = mode === 2 ? 1 : 0; values[ALLOWED] = 0; values[PROCESS] = 0;
            scene.prepare(values); scene.state.preview = number; const before = scene.pool.slot(PREVIEW); let placed = 0;
            assert.throws(() => scene.body({ ...scene.hooks(), placeLocal: () => { placed += 1; } }).drop(), /미리보기 DWORD/);
            assert.deepEqual([scene.state.preview, scene.prepared, scene.sources, placed, scene.state.lockDepth], [number, 2, 0, 0, mode === 0 ? 1 : 0]);
            assert.deepEqual(scene.pool.slot(PREVIEW), before);
        }
    }
});

test('CursorDrop 어댑터: 같은 풀의 실제 배치/SP/소리 경계를 새 묶음으로 연결하고 로컬 플레이어는 매번 읽는다', () => {
    const scene = new DropScene().prepare(); const calls = []; const original = scene.hooks();
    const local = { pool: scene.pool, place: (request) => calls.push(['local', request]) };
    const reward = { pool: scene.pool, addSp: (owner, delta) => calls.push(['sp', owner, delta]) };
    const sound = { playByName: (...args) => calls.push(['sound', ...args]) };
    const hooks = makeCursorDropHooks(scene.pool, local, reward, sound, scene.state, original);
    assert.notEqual(hooks, original); assert.equal(hooks.prepareCursor, original.prepareCursor); assert.notEqual(hooks.placeLocal, original.placeLocal);
    const request = { preview: PREVIEW, x: 1, y: 2, argument: 3, direction: 4 }; hooks.placeLocal(request);
    scene.state.localPlayer = 7; hooks.addLocalSp(-3.5); scene.state.localPlayer = 2; hooks.addLocalSp(9); hooks.playDrop();
    assert.deepEqual(calls, [['local', request], ['sp', 7, -3.5], ['sp', 2, 9], ['sound', 'droppiece.wav', 0, 0, 0, 0, 0]]);
    assert.throws(() => makeCursorDropHooks({}, local, reward, sound, scene.state, original), /SID 풀/);
    assert.throws(() => makeCursorDropHooks(scene.pool, local, { ...reward, pool: {} }, sound, scene.state, original), /SID 풀/);
});

test('CursorDrop GUI: detach 뒤 WORD 두 바이트만 바꾸고 캡처한 기존 플래그/품질과 두 번째 연관 번호를 반환한다', () => {
    const scene = new DropScene().prepare(); scene.state.legalGump = 1;
    const hooks = scene.hooks(); let returned;
    hooks.detach = (sid) => { const raw = scene.pool.allocated(sid); raw.fill(0xa5); raw[11] = 0; };
    hooks.returnPiece = (...args) => { returned = args; };
    scene.body(hooks).drop();
    const expected = new Uint8Array(50).fill(0xa5); expected[11] = 0; expected[20] = 0; expected[21] = 1;
    assert.deepEqual(scene.pool.slot(PREVIEW), expected);
    assert.deepEqual(returned, [8, 88, 3]);
});

test('CursorDrop 비용: float32 비용의 CRT 하위 DWORD·signed 나눗셈·화면 감김과 비유한 비용을 정확히 보존한다', () => {
    const scene = new DropScene();
    // 기대 절삭값은 명시 상수이고 화면 정수 계산은 독립 BigInt로 한다.
    for (const [cost, amount] of [[9.75, 9], [-9.75, -9], [4294967808, 512], [2 ** 63, 0], [NaN, 0], [Infinity, 0]]) {
        scene.prepare(); Object.assign(scene.state, { inputFlags: 0x80, costFlags: 0x40, screenX: -2147483648, cursorWidth: -3, screenY: -9 });
        Object.assign(scene.types[TYPE], { cost: Math.fround(cost), footX: 2147483647 });
        const hooks = scene.hooks(); let shown; let charged;
        hooks.showSp = (...args) => { shown = args; }; hooks.addLocalSp = (delta) => { charged = delta; };
        scene.body(hooks).drop();
        const halfFoot = BigInt.asIntN(32, 2147483647n * 16n) / 2n;
        const x = Number(BigInt.asIntN(32, -2147483648n - halfFoot - (-3n / 2n)));
        assert.deepEqual(shown, [amount, x, -9, 0]);
        assert.equal(charged, Math.fround(-Math.fround(cost)));
    }
});
