// 생산 칸 반환을 cpppj ReturnPieceTests·CursorDropTests의 실제 생성/Regular/GUI 장면과 원본 returnpiece-x86.tsv originals 633행으로 검사한다.
// 기대 raw와 사건/난수는 저장 관측에서 읽는다. 실제 게임·브라우저 실행이 아니다. (2026-11-14 WB04-E399)
import './nan-exact.mjs';
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { GameRandom } from '../www/core/random.js';
import { RawContainedCreate, makeContainedCreateHooks } from '../www/core/contained-create.js';
import { SquidFactory } from '../www/core/squid-factory.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { RawSquidDestroy } from '../www/core/squid-destroy.js';
import { Kernel } from '../www/core/kernel.js';
import { SquidProcessHost, SquidProcessState } from '../www/core/squid-process.js';
import { CursorDropState, RawCursorDrop } from '../www/core/raw-cursor-drop.js';
import { CursorReturnPieceState, RawCursorReturnPiece, makeCursorReturnPieceHooks, makeReturnPieceHooks } from '../www/core/raw-cursor-return-piece.js';

// 관측 파일의 10.78 행과 입력 열 순서다. CD/10.37은 웹 범위 밖이다.
const rows = readRows(path.join(REPO_ROOT, 'cpppj/tests/fixtures/returnpiece-x86.tsv'))?.filter((row) => row[0] === 'originals');
const [TYPE, FLAGS, QUALITY, KEEP, FORT, INDEX, SEED, ARGUMENT, ATTACH_FLAGS, RAW_TYPE, ALLOCATION, FACTORY_CHANGE,
    NOTIFY_CHANGE, ATTACH_CHANGE, ALLOCATE_CHANGE] = Array.from({ length: 15 }, (_, index) => index);
// 원본의 부모·생성 번호와 raw 16진 직렬화다.
const PARENT = 77; const BORN = 100;
const hex = (raw) => Buffer.from(raw).toString('hex');

// cpppj MakeFormTypeInputs와 같은 네 내장 Form 생성자 주소만 공급한다. 자산의 근사 메타데이터를 만들지 않는다.
const formTypes = () => Array.from({ length: 188 }, (_, number) => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0,
    ...([1, 2, 3, 6].includes(number) ? { constructorAddress: typeConstructorAddress(number) } : {}) }));

// 관측 입력과 지정된 생성/알림/부착/할당 효과만 제공하는 독립 장면이다.
class ReturnScene {
    // 같은 풀·현재 contained/반환 전역·게임 난수·입력·사건이다.
    pool = new SidPool(24000); containedState = {}; state = new CursorReturnPieceState(); random = new GameRandom(); input = []; events = [];

    // 소스의 InputRaw가 쓰는 100번까지 한 번 확보한다. 원문 물리 입력은 각 행에서 다시 쓴다.
    constructor() { for (let sid = 5; sid <= BORN; sid += 1) assert.equal(this.pool.allocate(2), sid); }

    // 기대 계산 없이 원본 실행기의 주변 바이트/전역 입력만 공급한다.
    prepare(v = rows[0][2].split(',').map(Number)) {
        this.input = v; this.events = []; this.containedState = { contained: 6, bridge: 82 };
        this.state = Object.assign(new CursorReturnPieceState(), { type: v[TYPE], keepPieces: v[KEEP], fort: v[FORT], lifetimeIndex: v[INDEX] & 255 });
        this.random.setState(v[SEED]); const raw = this.pool.physical(BORN);
        // 좁은 쓰기 폭을 관측하도록 각 이웃 바이트에 다른 값을 둔다.
        for (let i = 0; i < raw.length; i += 1) raw[i] = (i * 37 + 11) & 255;
        put(raw, 0, 0x11010000); raw[10] = v[RAW_TYPE]; raw[11] = 4;
        return this;
    }

    // 원본 순서의 DWORD 사건 표기다.
    append(letter, ...values) { this.events.push([letter, ...values.map((value) => value >>> 0)].join(':')); }

    // 실제 RawContainedCreate를 쓰되 원본 관측이 지정한 가상 효과만 공급한다.
    contained() {
        const v = this.input;
        return new RawContainedCreate(this.pool, { containedType: () => this.containedState.contained, bridgeType: () => this.containedState.bridge }, {
            create: (type, flags) => {
                this.append('C', type, flags);
                if (v[FACTORY_CHANGE]) { this.containedState.bridge = 6; this.containedState.contained = 7; this.state.bridgeType = 6; this.state.type = 101; }
                return BORN;
            },
            notifySurface: (sid) => {
                this.events.push(`N:${sid}:${hex(this.pool.slot(sid))}`);
                if (v[NOTIFY_CHANGE]) { const raw = this.pool.allocated(sid); put(raw, 12, 0x9876, 2); put(raw, 20, 0xffff, 2); raw[30] = 0xfe; }
            },
            attach: (sid, parent, flags) => {
                this.events.push(`J:${sid}:${parent}:${flags}:${hex(this.pool.slot(sid))}`);
                if (v[ATTACH_CHANGE]) { this.state.keepPieces ^= 1; this.state.type = 102; }
            },
        });
    }

    // 할당 성공/실패와 할당 뒤 실제 설정 변경을 입력으로 공급한다.
    timerHooks() {
        return {
            allocateTimer: (size) => { this.append('A', size); if (this.input[ALLOCATE_CHANGE]) { this.state.fort ^= 1; this.state.lifetimeIndex = 2; } return this.input[ALLOCATION] !== 0; },
            createTimer: (sid, event, duration) => this.append('T', event, sid, floatToBits(duration)),
        };
    }

    // 최종 전역/난수/반환 SID 8개를 저장 관측 순서로 꺼낸다.
    snapshot(result) { return [this.state.type, this.containedState.bridge, this.containedState.contained, this.state.keepPieces, this.state.fort,
        this.random.state, this.state.lifetimeIndex, result].join(','); }

    // 현재 실제 생성기에 반환 몸체를 붙인다.
    body(hooks = this.timerHooks()) { return new RawCursorReturnPiece(this.contained(), this.state, this.random, hooks); }
}

test('ReturnPiece_ReplaysOriginals: 원본 10.78 633행의 반환/contained/Factory 사건·전체 raw·전역·난수가 일치한다', { skip: rows === undefined }, () => {
    assert.equal(rows.length, 633); const scene = new ReturnScene();
    // Return 528·Create 101·Factory 4행을 같은 제품 생성 경로로 재생한다.
    for (const row of rows) {
        assert.equal(row.length, 6); const v = row[2].split(',').map(Number); assert.equal(v.length, 15); scene.prepare(v);
        let result = 0;
        if (row[1] === 'Return') scene.body().returnPiece(v[FLAGS], PARENT, v[QUALITY]);
        else if (row[1] === 'Create') result = scene.contained().create(PARENT, v[TYPE], v[ARGUMENT], v[FLAGS], v[ATTACH_FLAGS], v[QUALITY]);
        else {
            assert.equal(row[1], 'Factory'); scene.pool.reset(); scene.append('S', v[FLAGS]); result = new SquidFactory(scene.pool, formTypes()).createContained(v[FLAGS]);
        }
        assert.equal(scene.events.join(';') || '-', row[3], row.slice(0, 3).join(' '));
        assert.equal(hex(scene.pool.slot(row[1] === 'Factory' ? result : BORN)), row[4], row[2]);
        assert.equal(scene.snapshot(result), row[5], row[2]);
    }
});

test('ReturnPiece_RejectsInvalidContracts: 타이머·부모·시간 번호·같은 풀 계약과 실패 뒤 생성 효과를 보존한다', () => {
    const scene = new ReturnScene().prepare();
    assert.throws(() => new RawCursorReturnPiece(scene.contained(), scene.state, scene.random, {}), /타이머 경계/);
    const body = scene.body(); assert.equal(body.pool, scene.pool);
    assert.throws(() => body.returnPiece(8, 0, 3), /부모 SID/); assert.throws(() => body.returnPiece(8, 0xffffffff, 3), /부모 SID/);
    assert.deepEqual(scene.events, []); scene.state.lifetimeIndex = 3;
    assert.throws(() => body.returnPiece(8, PARENT, 3), /시간 번호/); assert.equal(scene.events.at(-1), 'A:40');
    scene.input[ALLOCATION] = 0; body.returnPiece(8, PARENT, 3); scene.state.type = 82; body.returnPiece(8, PARENT, 3);
    assert.throws(() => makeReturnPieceHooks({}, body, {}), /SID 풀/);
    assert.throws(() => makeCursorReturnPieceHooks({}, { pool: scene.pool }, {}), /SID 풀/);
});

test('ReturnPiece 현재 getter: 생성 전후 유지 옵션을 다시 읽고 할당 뒤 모드·속도를 읽으며 요새는 속도를 읽지 않는다', () => {
    const scene = new ReturnScene().prepare(); const calls = [];
    let reads = 0; const hooks = { ...scene.timerHooks(), productTimers: () => { calls.push('keep'); return reads++ === 0 ? 0 : 1; },
        fortMode: () => { calls.push('fort'); return 0; }, unitRate: () => { calls.push('rate'); return 2; } };
    scene.body(hooks).returnPiece(0x108, PARENT, 3); assert.deepEqual(calls, ['keep', 'keep']); assert.ok(!scene.events.some((event) => event.startsWith('A:')));
    assert.equal(get(scene.pool.slot(BORN), 20, 2), 0x108);
    calls.length = 0; hooks.productTimers = () => { calls.push('keep'); return 0; }; hooks.fortMode = () => { calls.push('fort'); return 1; };
    scene.body(hooks).returnPiece(8, PARENT, 3); assert.deepEqual(calls, ['keep', 'keep', 'fort']); assert.equal(scene.events.at(-1), `T:0:100:${0x38d1b717}`);
    calls.length = 0; hooks.fortMode = () => { calls.push('fort'); return 0; };
    scene.body(hooks).returnPiece(8, PARENT, 3); assert.deepEqual(calls, ['keep', 'keep', 'fort', 'rate']); assert.equal(scene.events.at(-1), `T:0:100:${floatToBits(1)}`);
});

test('ReturnPiece_ConnectsFactoryAndRegular: 실제 생성·Form·Kernel에 네 소멸 시간을 저장하고 1초 예약을 진행한다', () => {
    const pool = new SidPool(24000, { server: false }); const types = formTypes(); const factory = new SquidFactory(pool, types);
    const destroy = new RawSquidDestroy(pool, new SquidUnpop(pool, new SquidHash(), new Uint8Array(65536)), types);
    const kernel = new Kernel(); const clock = new SquidProcessState(); clock.now = 12.5; const born = []; let handled = 0;
    const host = new SquidProcessHost(pool, types, kernel, destroy, clock, { regular: (parent, event, count, duration) => {
        assert.deepEqual([parent, event, count, duration], [born.at(-1), 0, 0, 1]); handled += 1; return duration;
    } });
    const contained = new RawContainedCreate(pool, undefined, makeContainedCreateHooks(pool, factory, {
        notifySurface: () => assert.fail('일반 조각의 표면 통지 없음'),
        attach: (sid, parent, flags) => { assert.deepEqual([parent, flags], [PARENT, 0x10]); born.push(sid); const raw = pool.allocated(sid); raw[11] = (raw[11] & ~4) | 8; },
    }));
    const state = new CursorReturnPieceState(); state.type = 100; const random = new GameRandom(17);
    const allocation = { allocateTimer: (size) => { assert.equal(size, 40); return true; } };
    const timerHooks = makeCursorReturnPieceHooks(pool, host, allocation); assert.notEqual(timerHooks, allocation); assert.equal(allocation.createTimer, undefined);
    const body = new RawCursorReturnPiece(contained, state, random, timerHooks);
    // 세 일반 옵션과 요새 모드의 실제 payload/시계/부모/개수를 대조한다.
    for (let index = 0; index < 4; index += 1) {
        state.lifetimeIndex = index; state.fort = index === 3 ? 1 : 0; body.returnPiece(0x108, PARENT, 3);
        const sid = born.at(-1); const raw = pool.slot(sid); const regular = host.findEvent(sid, 0);
        assert.deepEqual([raw[10], get(raw, 18, 2), get(raw, 12, 2), get(raw, 20, 2), raw[30]], [6, 100, 0, 0x108, 2]);
        assert.ok(regular); assert.deepEqual([regular.parent, regular.time, regular.count], [sid, 12.5, 0]);
        assert.equal(floatToBits(regular.payload), [floatToBits(10), floatToBits(5), floatToBits(1), 0x38d1b717][index]);
    }
    assert.equal(kernel.size, 4); assert.equal(random.state, 17);
    state.fort = 0; state.lifetimeIndex = 2; body.returnPiece(8, PARENT, 1); const regular = host.findEvent(born.at(-1), 0);
    assert.ok(regular); regular.runFrame(); assert.equal(handled, 1); assert.deepEqual([regular.time, regular.count], [13.5, 1]);
    clock.now = 13; regular.runFrame(); assert.equal(handled, 1);
});

test('ReturnPiece_ConnectsGuiDrop: GUI 내려놓기가 같은 풀의 실제 생산 칸을 생성하고 기존 품질/플래그를 반환한다', () => {
    const pool = new SidPool(24000, { server: false }); const types = formTypes(); const factory = new SquidFactory(pool, types);
    const parent = factory.create(100, 2); const preview = factory.create(6, 2); const raw = pool.allocated(preview); put(raw, 20, 8, 2); raw[30] = 3;
    let returned = 0; const contained = new RawContainedCreate(pool, undefined, makeContainedCreateHooks(pool, factory, {
        notifySurface: () => assert.fail('일반 조각의 표면 통지 없음'), attach: (sid, actualParent, flags) => { assert.deepEqual([actualParent, flags], [parent, 0x10]); returned = sid; },
    }));
    const returnState = new CursorReturnPieceState(); returnState.type = 100; returnState.keepPieces = 1; const random = new GameRandom();
    const body = new RawCursorReturnPiece(contained, returnState, random, { allocateTimer: () => assert.fail('유지 조각 타이머 없음'), createTimer: () => assert.fail('유지 조각 타이머 없음') });
    const state = Object.assign(new CursorDropState(), { legal: 1, preview, legalGump: 1 }); let sources = 0;
    const hooks = {
        queryCursor() {}, prepareCursor() {}, highlight: (value) => assert.equal(value, 0), playDrop() {}, previewSource: () => ++sources === 1 ? 77 : parent,
        destroy: () => assert.fail('기존 조각 삭제 없음'), pop: () => assert.fail('월드 배치 없음'), record: () => assert.fail('기록 없음'), placeLocal: () => assert.fail('로컬 배치 없음'),
        addLocalSp: () => assert.fail('GUI 비용 없음'), showSp: () => assert.fail('GUI 비용 없음'), battleRefresh: () => assert.fail('GUI 전투 갱신 없음'), invalidGui: () => assert.fail('허용된 GUI'),
        guiFlags: () => 0x100, guiParent: () => 123,
        detach: (sid) => { const current = pool.allocated(sid); put(current, 20, 0xffff, 2); current[30] = 0xfe; }, attach() {}, guiReceive() {},
        activatePreview() {}, selectCursor() {}, refresh: (...args) => assert.deepEqual(args, [0, 0, 1]), closeGump: () => assert.fail('커서 Process 없음'),
    };
    const connected = makeReturnPieceHooks(pool, body, hooks); assert.equal(hooks.returnPiece, undefined); assert.equal(connected.guiFlags, hooks.guiFlags);
    new RawCursorDrop(pool, types, state, connected).drop();
    assert.ok(returned); assert.deepEqual([state.preview, state.lockDepth, sources], [0, 1, 2]);
    const result = pool.slot(returned); assert.deepEqual([result[10], get(result, 18, 2), get(result, 20, 2), result[30]], [6, 100, 8, 2]);
    assert.deepEqual([get(pool.slot(preview), 20, 2), pool.slot(preview)[30], random.state], [0x100, 0xfe, 0]);
});
