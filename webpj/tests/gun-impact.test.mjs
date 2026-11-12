// 투사체 충돌 효과 raw-gun-impact.js(GunImpactState·RawGunImpact·makeGunImpactRuntimeHooks·makeMissileGunImpactHooks·makeAssetGunImpactHooks)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: gunimpact-x86.tsv 의 originals 행 — 생산 Visual 0·anim Handle 1·좌표 제한 2·anim 생성자 3·Kernel 개수 4·지면 Ground 5 가 반환 비트·호출 순서(Q 개수/C 생성/P Pop/D 갱신/U 해제/A·B Regular 부착/
//    Qd Dude/D 삭제)·anim 슬롯 50바이트·Regular 의미 상태(별도 Host)·공유 난수 상태·좌표 8바이트까지 같다. cpppj GunImpactTests.cpp 의 ImpactScene 을 같은 순서로 옮겼다.
//  - cpppj 시험 이식: 누락된 생성/삭제/Dude 경계 거부(GunImpact_MissingCreationDeleteAndDudesRejected), 장기 억제 double 상자의 현재값 읽기·쓰기(SharedSuppression…), 독립 상태 유지(LegacyAggregate…),
//    실제 자산(anim 88프레임)과 같은 Factory/Pop/Regular/장부/종속 삭제 사슬(ActualAssetsFactoryPopRegularAndDeletion, 웹 준비 자료의 실제 TYPE·SHP).
//  - 웹 계약: handleIn 단독·handler 분배/fallback·훅 공장 셋(새 묶음·풀/Kernel 용량 불일치 거부)·생성자 검사.
// 10.72(CD)는 범위 밖이다. 실제 게임·브라우저 실행이 아니다. (2026-11-13 WB04-E363: 7-P2 RawGunImpact 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, floatToBits, bitsToFloat } from './raw-support.mjs';
import { realCaptureAssets } from './capture-assets-support.mjs';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { SidPool } from '../www/core/sid-pool.js';
import { Kernel, PROCESS_CAPACITY } from '../www/core/kernel.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { SquidPostPop } from '../www/core/squid-postpop.js';
import { SquidPostPopState } from '../www/core/squid-postpop-state.js';
import { SquidPop, RAW_POP_RESULT } from '../www/core/squid-pop.js';
import { SquidDisplay } from '../www/core/squid-display.js';
import { SquidFrame } from '../www/core/squid-frame.js';
import { makeSquidFrameHooks } from '../www/core/squid-frame-binding.js';
import { SquidFactory } from '../www/core/squid-factory.js';
import { RawSquidDestroy } from '../www/core/squid-destroy.js';
import { SquidProcessHost, SquidProcessState } from '../www/core/squid-process.js';
import { SquidDeletionState, SquidDestroyLifecycle } from '../www/core/squid-destroy-lifecycle.js';
import { GameRandom } from '../www/core/random.js';
import { GunImpactState, RawGunImpact, makeAssetGunImpactHooks, makeGunImpactRuntimeHooks, makeMissileGunImpactHooks } from '../www/core/raw-gun-impact.js';

// 원본 관측 10.78 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-13 WB04-E363)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'gunimpact-x86.tsv'))?.filter((row) => row[0] === 'originals') ?? null;

// 풀 크기와 native 의 anim 슬롯 번호이다. (2026-11-13 WB04-E363)
const CAPACITY = 24000;
const SOURCE = 50;

// 바이트 배열의 소문자 16진 문자열이다. (2026-11-13 WB04-E363)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 예외가 나는지 본다. (2026-11-13 WB04-E363)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 188칸 TYPE 입력이다(cpppj MakeFormTypeInputs: 모든 칸 0 + 내장 Form 네 생성자 주소). (2026-11-13 WB04-E363)
function formTypeInputs() {
    return Array.from({ length: 188 }, (_, number) => ({
        cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0, group: 0, hotspotX: 0, hotspotY: 0,
        ...([1, 2, 3, 6].includes(number) ? { constructorAddress: typeConstructorAddress(number) } : {}),
    }));
}

// A..P 구간의 존재/빈 구간 및 비구간 padding 을 원본 입력과 같은 80프레임에 만든다(글자 event 는 event%4 개의 프레임). (2026-11-13 WB04-E363)
function impactFrames() {
    const result = Array.from({ length: 188 }, () => new FrameCodeTable([]));
    const codes = Array.from({ length: 80 }, () => ({ side: 255, variant: 0, number: 0, flags: 0 }));
    // 각 실제 입력 글자는 고유 시작 위치와 0..3개 길이를 갖는다.
    for (let event = 0; event < 16; event += 1) for (let frame = 0; frame < event % 4; frame += 1) codes[event * 5 + frame] = { side: 0x41 + event, variant: 0, number: frame, flags: 0 };
    result[160] = new FrameCodeTable(codes);
    return result;
}

// 64비트 정수 문자열의 little endian 비트를 double 로 본다. (2026-11-13 WB04-E363)
function wideDouble(text) {
    const view = new DataView(new ArrayBuffer(8));
    view.setBigUint64(0, BigInt(text), true);
    return view.getFloat64(0, true);
}

// double 의 64비트 정수 문자열이다. (2026-11-13 WB04-E363)
function wideBits(value) {
    const view = new DataView(new ArrayBuffer(8));
    view.setFloat64(0, value, true);
    return view.getBigUint64(0, true).toString();
}

// 실제 Regular 생성/부착의 공개 상태를 원본의 관찰된 의미 필드와 대조한다(별도 Host). (2026-11-13 WB04-E363)
function regularState(event, payload) {
    const pool = new SidPool(CAPACITY, { server: false });
    const types = formTypeInputs();
    // Native attach 관찰 경계가 제공한 Form 71/부모 50 을 같은 실제 풀에서 확보한다.
    for (let id = 5; id <= 70; id += 1) assert.equal(pool.allocate(2), id);
    const raw = pool.allocated(SOURCE);
    raw.fill(0);
    raw[10] = 160;
    const destroy = new RawSquidDestroy(pool, new SquidUnpop(pool, new SquidHash(), new Uint8Array(65536)), types);
    const state = new SquidProcessState();
    state.now = 10;
    const host = new SquidProcessHost(pool, types, new Kernel(PROCESS_CAPACITY), destroy, state);
    const regular = host.addRegular(SOURCE, event, payload);
    assert.ok(regular);
    return [regular.form, regular.parent, wideBits(regular.time), regular.event, floatToBits(regular.payload), regular.count].join(',');
}

// 독립 입력 raw/프레임 표와 원본의 동일한 공간/생성 관찰 경계를 소유한다(cpppj ImpactScene). (2026-11-13 WB04-E363)
class ImpactScene {
    constructor() {
        this.pool = new SidPool(CAPACITY, { server: false });
        this.types = formTypeInputs();
        this.frames = impactFrames();
        this.random = new GameRandom(0);
        this.state = new GunImpactState();
        this.events = [];
        this.row = null;
        this.process = '-';
        this.point = new Uint8Array(8);
        this.setter = new SquidFrame(this.pool, this.types, this.frameHooks());
        // 원본 SID 50 을 실제 할당해 쓰기 계약을 유지한다.
        for (let id = 5; id <= 50; id += 1) assert.equal(this.pool.allocate(2), id);
    }

    // 현재 raw frame 을 원본 폭(DWORD)으로 읽는다. (2026-11-13 WB04-E363)
    frame() { return get(this.pool.slot(SOURCE), 36, 4); }

    // 실제 지정기의 표시/공간 관찰 시점과 인자를 원본과 동일하게 기록한다. (2026-11-13 WB04-E363)
    frameHooks() {
        return {
            frameSize: () => [1, 1],
            update: () => { this.events.push(`D:50:${this.frame()}`); },
            unpop: (sid, flags) => { this.events.push(`U:50:${flags}:${this.frame()}`); },
            pop: (sid, x, y, flags) => { this.pop(x, y, flags); },
        };
    }

    // 현재 Pop 경계에서는 원본처럼 virtual 몸체를 대체하며 frame 을 호출 시점에 읽는다. (2026-11-13 WB04-E363)
    pop(x, y, flags) { this.events.push(`P:50:${floatToBits(x)}:${floatToBits(y)}:${flags}:${this.frame()}`); }

    // 원본 Kernel 한도와 배열 끝 입력을 그대로 공급한다. (2026-11-13 WB04-E363)
    processCount() { return [0, 2, 30000, 30001, 39999][Number(this.row[12])]; }

    // 현재 float 입력의 실제 비트를 읽는다. (2026-11-13 WB04-E363)
    f(index) { return bitsToFloat(Number(this.row[index])); }

    // 매 행은 동일 원본 padding/raw/전역 입력을 만든다. (2026-11-13 WB04-E363)
    prepare(value) {
        assert.equal(value.length, 19);
        this.row = value;
        this.events = [];
        this.process = '-';
        this.random.setState(Number(value[11]));
        this.state.suppressedTime = wideDouble(value[9]);
        this.state.suppressed = value[10] === 'True' ? 1 : (value[10] === 'False' ? 0 : Number(value[10]));
        put(this.point, 0, Number(value[2]));
        put(this.point, 4, Number(value[3]));
        const raw = this.pool.allocated(SOURCE);
        // 관찰되지 않은 padding 은 원본과 동일한 패턴으로 남긴다.
        for (let index = 0; index < raw.length; index += 1) raw[index] = (index * 7 + 3) & 0xff;
        put(raw, 0, 0x509758);
        raw[10] = 160;
        raw[11] = 0;
        put(raw, 14, floatToBits(20.75));
        put(raw, 18, floatToBits(21.9));
        raw[33] = Number(value[8]);
        raw[40] = 0;
        put(raw, 36, Number(value[7]), 4);
    }

    // 원본과 같은 실제 생성 인자/Pop/Regular 의미 필드 및 엄격한 삭제/Dude 경계이다. (2026-11-13 WB04-E363)
    hooks() {
        return {
            processCount: () => { const count = this.processCount(); this.events.push(`Q:${count}`); return count; },
            create: (type, flags) => { this.events.push(`C:${type}:${flags}`); return SOURCE; },
            pop: (sid, x, y, flags) => { this.pop(x, y, flags); },
            regular: (sid, event, payload) => {
                this.events.push('A:40');
                this.events.push(`B:46:${sid}:0:80`);
                this.process = regularState(event, payload);
            },
            destroy: (sid, flags) => { this.events.push(`D:${sid}:${flags}`); },
            notifyDudes: (x, y) => { this.events.push(`Qd:${floatToBits(x)}:${floatToBits(y)}`); },
        };
    }

    // 실제 생산 코드만 실행하고 원본 결과 칸을 기대값으로 넘기지 않는다. (2026-11-13 WB04-E363)
    run() {
        const logic = new RawGunImpact(this.pool, this.frames, this.setter, this.random, this.state, this.hooks());
        let result = '-';
        switch (Number(this.row[1])) {
        case 0: logic.visual(this.f(2), this.f(3), Number(this.row[4]), this.f(6)); break;
        case 1: result = String(floatToBits(logic.handle(SOURCE, Number(this.row[4]), Number(this.row[5]), this.f(6)))); break;
        case 2: {
            const clamped = RawGunImpact.clamp(this.f(2), this.f(3));
            put(this.point, 0, floatToBits(clamped[0]));
            put(this.point, 4, floatToBits(clamped[1]));
            break;
        }
        case 3: {
            this.types[160].constructorAddress = typeConstructorAddress(160);
            new SquidFactory(this.pool, this.types).construct(160, SOURCE);
            break;
        }
        case 4: {
            const kernel = new Kernel(PROCESS_CAPACITY);
            const count = this.processCount();
            // 실제 같은 개수의 객체를 등록한 Kernel.size 를 원본 배열 순회와 대조한다.
            for (let index = 0; index < count; index += 1) assert.notEqual(kernel.add({ runFrame() {} }), 0);
            result = String(kernel.size);
            break;
        }
        case 5: logic.ground(this.f(2), this.f(3)); break;
        default: throw new Error('충돌 fixture 몸체 오류');
        }
        return [result, this.events.join(';'), hex(this.pool.slot(SOURCE)), this.process, String(this.random.state), hex(this.point)];
    }
}

test('GunImpact: 원본 10.78 관측 전 행(생산·anim Handle·좌표 제한·anim 생성자·Kernel 개수·지면)이 반환 비트·호출 순서·슬롯·Regular 상태·난수·좌표까지 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = new ImpactScene();
    let count = 0;
    // 원본 고유 입력을 각 한 번만 읽는다.
    for (const row of rows) {
        scene.prepare(row);
        assert.deepEqual(scene.run(), row.slice(13), `Impact 행 ${count} body ${row[1]}`);
        count += 1;
    }
    assert.equal(count, rows.length);
    assert.ok(count > 1000);
});

test('GunImpact: 실제 하위 생성/프레임 종료/Dude 반응을 누락된 성공으로 바꾸지 않는다', () => {
    const scene = new ImpactScene();
    scene.prepare(rows[0]);
    const missing = new RawGunImpact(scene.pool, scene.frames, scene.setter, scene.random, scene.state, {});
    assert.ok(throws(() => missing.visual(50, 50, 3, 0.02)));
    assert.ok(throws(() => missing.handle(SOURCE, 0, 0, 0.02)));
    assert.ok(throws(() => missing.ground(50, 50)));
    assert.deepEqual([scene.events, scene.random.state], [[], 0]);
});

test('GunImpact: 장기 억제 double 상자의 현재값을 생성/Pop/Regular 앞에서 읽고 쓰기도 같은 소유자로 가며 지역 사본은 보존한다', () => {
    const scene = new ImpactScene();
    scene.prepare(rows[0]);
    const owner = { value: 17.25 };
    scene.state = new GunImpactState({ suppressedTime: 0, suppressed: 0, animationType: 160, sharedSuppressedTime: owner });
    const impact = new RawGunImpact(scene.pool, scene.frames, scene.setter, scene.random, scene.state, scene.hooks());
    const before = hex(scene.pool.slot(SOURCE));
    const randomBefore = scene.random.state;
    // 기존 값 0 과 반대되는 장기 비영값이 관문을 선택해야 한다.
    impact.visual(50, 50, 3, 0.02);
    assert.deepEqual([scene.events, hex(scene.pool.slot(SOURCE)), scene.random.state], [[], before, randomBefore]);
    // 지역 값을 91.5, 소유자를 0 으로 바꾸면 같은 객체의 다음 호출은 소유자 0 을 즉시 읽어 생성한다.
    scene.state.suppressedTime = 91.5;
    owner.value = 0;
    impact.visual(50, 50, 3, 0.02);
    assert.equal(scene.events.filter((event) => event === 'C:160:2').length, 1);
    assert.deepEqual([scene.frame(), scene.process !== '-', scene.state.suppressedTime], [15, true, 91.5]);
    // 쓰기도 같은 소유자로 향하며 지역 fallback 을 덮지 않는다.
    scene.events.length = 0;
    scene.state.currentSuppressedTime = -4.5;
    assert.deepEqual([owner.value, scene.state.suppressedTime], [-4.5, 91.5]);
    impact.visual(50, 50, 3, 0.02);
    assert.deepEqual(scene.events, []);
    // 음수 0 은 원문 double 비교에서 0 과 같아 생성하며 원시 비트를 보존한다.
    owner.value = -0;
    impact.visual(50, 50, 3, 0.02);
    assert.equal(scene.events.filter((event) => event === 'C:160:2').length, 1);
    assert.ok(Object.is(scene.state.currentSuppressedTime, -0));
});

test('GunImpact: 상자 없는 독립 상태는 지역 억제 double 의 읽기/쓰기/생성 의미를 그대로 유지한다', () => {
    const scene = new ImpactScene();
    scene.prepare(rows[0]);
    scene.state = new GunImpactState({ suppressedTime: 12.5, suppressed: 0, animationType: 160 });
    const impact = new RawGunImpact(scene.pool, scene.frames, scene.setter, scene.random, scene.state, scene.hooks());
    assert.equal(scene.state.sharedSuppressedTime, null);
    impact.visual(50, 50, 3, 0.02);
    assert.deepEqual(scene.events, []);
    scene.state.currentSuppressedTime = 0;
    impact.visual(50, 50, 3, 0.02);
    assert.equal(scene.state.suppressedTime, 0);
    assert.equal(scene.events.filter((event) => event === 'C:160:2').length, 1);
    assert.deepEqual([scene.frame(), scene.process !== '-'], [15, true]);
});

test('GunImpact: 실제 자산(anim 88프레임)과 같은 Factory/Pop/Regular/장부/종속 삭제 사슬을 소비해 22프레임을 진행하고 지운다(ActualAssetsFactoryPopRegularAndDeletion)', () => {
    const assets = realCaptureAssets();
    const types = assets.types;
    const frames = types.map(() => new FrameCodeTable([]));
    frames[160] = FrameCodeTable.fromDefinition(assets.table.definitions[160 - 70]);
    const run = frames[160].run('D');
    assert.deepEqual([frames[160].codes.length, run.first, run.count], [88, 66, 22]);
    const pool = new SidPool(CAPACITY, { server: true });
    const hash = new SquidHash();
    const spots = new Uint8Array(65536);
    const rectangles = [];
    const sink = { suppressed: () => false, invalidate: (rect) => { rectangles.push(rect); } };
    const display = new SquidDisplay(types, assets.shapes, sink, { cameraX: 0, cameraY: 0, zoom: 65536, viewport: { left: 0, top: 0, right: 1024, bottom: 768 } });
    const books = new SquidPostPopState();
    books.localOwner = 1;
    const post = new SquidPostPop(pool, types, books);
    const unpop = new SquidUnpop(pool, hash, spots, display);
    const pop = new SquidPop(pool, hash, spots, { display, postPop: post });
    const setter = new SquidFrame(pool, types, makeSquidFrameHooks(pool, types, display, unpop, pop));
    const factory = new SquidFactory(pool, types, { unpop });
    const destroy = new RawSquidDestroy(pool, unpop, types);
    const lifecycle = new SquidDestroyLifecycle(pool, types, books, new SquidDeletionState(), destroy, { emit: () => {}, forward: () => assert.fail('종속 가상 효과가 올라오면 안 된다') });
    const kernel = new Kernel();
    const clock = new SquidProcessState();
    clock.now = 10;
    const host = new SquidProcessHost(pool, types, kernel, destroy, clock, { asset: lifecycle.hooks() });
    let created = 0;
    const hooks = {
        pop: (sid, x, y, flags) => {
            created = sid;
            const frame = get(pool.slot(sid), 36, 4) | 0;
            const size = display.frameSize(pool.slot(sid), frame);
            assert.equal(pop.pop(sid, types[pool.slot(sid)[10]], size[0], size[1], x, y, flags), RAW_POP_RESULT.registered);
        },
        destroy: (sid, flags) => { destroy.destroy(sid, flags, host.hooks()); },
    };
    try {
        const impact = new RawGunImpact(pool, frames, setter, new GameRandom(0), new GunImpactState(), makeGunImpactRuntimeHooks(pool, factory, kernel, host, hooks));
        host.setRegularHandler(impact.handler());
        impact.visual(50.5, 50.5, 3, 0.02);
        assert.ok(created);
        assert.deepEqual([kernel.size, books.globalCounts()[160], books.depth], [1, 1, 0]);
        const regular = host.findEvent(created, 3);
        assert.ok(regular);
        const form = regular.form;
        assert.deepEqual([regular.parent, regular.time, regular.count], [created, 10, 0]);
        // 실제 현재 구간의 22프레임을 같은 Kernel/표시/공간에서 진행한다.
        for (let index = 0; index < run.count; index += 1) {
            clock.now = regular.time;
            kernel.runFrame();
            assert.equal(regular.count, index + 1);
            assert.equal(get(pool.slot(created), 36, 4), run.first + index);
        }
        clock.now = regular.time;
        kernel.runFrame();
        assert.deepEqual([kernel.size, host.fromForm(form)], [0, null]);
        assert.ok(pool.slot(created)[11] & 3);
        assert.ok(pool.slot(form)[11] & 3);
        assert.deepEqual([books.globalCounts()[160], books.totalCost, books.depth, destroy.preDepth, destroy.postDepth], [0, 0, 0, 0, 0]);
        assert.ok(rectangles.length > 0);
        // 마지막 삭제 뒤 실제 해시에 부모나 종속 Form 이 남지 않는다.
        for (let level = 0; level < 4; level += 1) for (const sid of hash.entries(level)) assert.ok(sid !== created && sid !== form);
    } finally {
        kernel.clear();
    }
});

// 계약 시험용 장면이다: 원본 행 하나로 raw 슬롯을 준비하고 억제를 풀며, 실제 Kernel 개수는 지정한 값을 공급한다. (2026-11-13 WB04-E363)
function contractScene(count = 0) {
    const scene = new ImpactScene();
    scene.prepare(rows[0]);
    scene.state.suppressedTime = 0;
    scene.state.suppressed = 0;
    scene.row = rows[0].slice();
    scene.processCount = () => count;
    return scene;
}

test('GunImpact 생성자: PATCH 풀·같은 풀의 지정기·188칸 프레임 표·anim 160 만 받는다', () => {
    const scene = contractScene();
    const make = (pool = scene.pool, frames = scene.frames, setter = scene.setter, state = scene.state) => new RawGunImpact(pool, frames, setter, scene.random, state, {});
    assert.equal(make().pool, scene.pool);
    assert.throws(() => make({ edition: 'cd1072' }), TypeError);
    assert.throws(() => make(scene.pool, scene.frames, new SquidFrame(new SidPool(CAPACITY, { server: false }), scene.types, scene.frameHooks())), (error) => error instanceof TypeError && /풀 불일치/.test(error.message));
    assert.throws(() => make(scene.pool, scene.frames.slice(1)), RangeError);
    assert.throws(() => make(scene.pool, scene.frames, scene.setter, new GunImpactState({ animationType: 161 })), RangeError);
});

test('GunImpact clamp: 하한 1·상한 255 로 제한하고 NaN 은 보존하며 경계값은 그대로다', () => {
    const cases = [[0.999, 1, 1, 1], [1, 255, 1, 255], [255.5, -3, 255, 1], [-0, Infinity, 1, 255], [-Infinity, 300, 1, 255], [100.25, 7.5, 100.25, 7.5],
        // 1 과 2 사이는 그대로 두고, 255 와 256 사이는 255 로 내린다(두 축 독립).
        [1.5, 1.25, 1.5, 1.25], [1.75, 1.999, 1.75, 1.999], [3, 255.5, 3, 255], [255.75, 255.25, 255, 255], [256, 256, 255, 255]];
    for (const [x, y, expectedX, expectedY] of cases) assert.deepEqual(RawGunImpact.clamp(x, y), [expectedX, expectedY], `${x},${y}`);
    const nan = RawGunImpact.clamp(NaN, NaN);
    assert.ok(Number.isNaN(nan[0]) && Number.isNaN(nan[1]));
    assert.ok(Number.isNaN(RawGunImpact.clamp(NaN, 5)[0]) && RawGunImpact.clamp(NaN, 5)[1] === 5);
});

test('GunImpact visual: 억제(NaN·비영 double·비영 DWORD)와 Kernel 한도(30001)는 아무것도 쓰지 않고 반환하며, 경계 누락은 생성 전에 거부한다', () => {
    // NaN 억제는 0 과 같지 않으므로 반환한다(음수 0 은 같아 생성한다).
    for (const [time, suppressed, count, creates] of [[NaN, 0, 0, false], [1e-300, 0, 0, false], [0, 1, 0, false], [0, 0, 30000, true], [0, 0, 30001, false], [-0, 0, 0, true]]) {
        const scene = contractScene(count);
        scene.state.suppressedTime = time;
        scene.state.suppressed = suppressed;
        const before = hex(scene.pool.slot(SOURCE));
        new RawGunImpact(scene.pool, scene.frames, scene.setter, scene.random, scene.state, scene.hooks()).visual(50, 50, 3, 0.02);
        assert.equal(scene.events.some((event) => event.startsWith('C:')), creates, `${time}/${suppressed}/${count}`);
        if (!creates) assert.equal(hex(scene.pool.slot(SOURCE)), before);
    }
    // Kernel 개수 조회가 없으면 억제가 아닌 한 거부한다. 억제 중에는 조회조차 요구하지 않는다.
    {
        const scene = contractScene();
        const hooks = scene.hooks();
        delete hooks.processCount;
        const logic = new RawGunImpact(scene.pool, scene.frames, scene.setter, scene.random, scene.state, hooks);
        assert.throws(() => logic.visual(50, 50, 3, 0.02), /Kernel 개수 조회 누락/);
        scene.state.suppressed = 5;
        logic.visual(50, 50, 3, 0.02);
    }
    // 생성/Pop/Regular 중 하나라도 없으면 아무것도 쓰기 전에 거부한다(슬롯 불변, 생성 훅 미호출).
    for (const name of ['create', 'pop', 'regular']) {
        const scene = contractScene();
        const hooks = scene.hooks();
        delete hooks[name];
        const before = hex(scene.pool.slot(SOURCE));
        assert.throws(() => new RawGunImpact(scene.pool, scene.frames, scene.setter, scene.random, scene.state, hooks).visual(50, 50, 3, 0.02), /생성\/Pop\/Regular 누락/, name);
        assert.deepEqual([hex(scene.pool.slot(SOURCE)), scene.events.some((event) => event.startsWith('C:'))], [before, false], name);
    }
    // 글자 번호가 16 이상이면 생성 전에 RangeError 이다.
    {
        const scene = contractScene();
        const range = (error) => error instanceof RangeError && /실제 글자 구간 오류/.test(error.message);
        assert.throws(() => new RawGunImpact(scene.pool, scene.frames, scene.setter, scene.random, scene.state, scene.hooks()).visual(50, 50, 16, 0.02), range);
        assert.throws(() => new RawGunImpact(scene.pool, scene.frames, scene.setter, scene.random, scene.state, scene.hooks()).visual(50, 50, 17, 0.02), RangeError);
        assert.equal(scene.events.some((event) => event.startsWith('C:')), false);
    }
});

test('GunImpact visual: 제한한 좌표와 인자로 anim 을 만들고 첫 프레임(빈 구간은 −1)을 DWORD 로 쓴 뒤 Pop(flags 0)·Regular(변형·간격)를 부르며 다른 바이트는 보존한다', () => {
    for (const [variant, first] of [[0, -1], [1, 5], [2, 10], [3, 15], [4, -1], [7, 35]]) {
        const scene = contractScene();
        const raw = Uint8Array.from(scene.pool.slot(SOURCE));
        const regularCalls = [];
        const hooks = scene.hooks();
        const regular = hooks.regular;
        hooks.regular = (...args) => { regularCalls.push(args); regular(...args); };
        new RawGunImpact(scene.pool, scene.frames, scene.setter, scene.random, scene.state, hooks).visual(0.25, 300, variant, 0.375);
        const after = scene.pool.slot(SOURCE);
        // 프레임 DWORD(36..39)만 바뀐다.
        for (let index = 0; index < raw.length; index += 1) if (index < 36 || index > 39) assert.equal(after[index], raw[index], `바이트 ${index}`);
        assert.equal(get(after, 36, 4) | 0, first, `변형 ${variant}`);
        assert.deepEqual(scene.events.slice(0, 3), ['Q:0', 'C:160:2', `P:50:${floatToBits(1)}:${floatToBits(255)}:0:${(first >>> 0)}`]);
        assert.deepEqual(regularCalls, [[SOURCE, variant, 0.375]]);
    }
    // 생성된 슬롯이 anim 160 이 아니거나 가상 표가 anim 이 아니면 쓰기 전에 거부한다.
    for (const damage of [(raw) => { raw[10] = 159; }, (raw) => { put(raw, 0, 0x509759); }]) {
        const scene = contractScene();
        const hooks = scene.hooks();
        hooks.create = (type, flags) => { scene.events.push(`C:${type}:${flags}`); damage(scene.pool.allocated(SOURCE)); return SOURCE; };
        const logic = new RawGunImpact(scene.pool, scene.frames, scene.setter, scene.random, scene.state, hooks);
        assert.throws(() => logic.visual(50, 50, 3, 0.02), /Factory 실제 anim 생성 오류/);
        assert.equal(scene.events.some((event) => event.startsWith('P:')), false);
    }
});

test('GunImpact ground: Dude 반응(제한한 좌표)→공유 난수 100 한 번→변형 3·간격 0.02 의 visual 순서이며 억제 중에도 난수는 소비하고 Dude 반응 경계가 없으면 아무것도 하지 않는다', () => {
    const scene = contractScene();
    const regularCalls = [];
    const hooks = scene.hooks();
    const regular = hooks.regular;
    hooks.regular = (...args) => { regularCalls.push(args); regular(...args); };
    const reference = new GameRandom(0);
    const startState = scene.random.state;
    reference.setState(startState);
    reference.next(100);
    // Dude 반응은 공유 난수 소비보다 먼저이므로, 반응 시점의 난수 상태는 시작 상태 그대로다.
    const randomAtDude = [];
    const dude = hooks.notifyDudes;
    hooks.notifyDudes = (...args) => { randomAtDude.push(scene.random.state); dude(...args); };
    new RawGunImpact(scene.pool, scene.frames, scene.setter, scene.random, scene.state, hooks).ground(0.5, 999);
    assert.equal(scene.random.state, reference.state);
    assert.deepEqual(randomAtDude, [startState]);
    assert.equal(scene.events[0], `Qd:${floatToBits(1)}:${floatToBits(255)}`);
    assert.deepEqual(scene.events.slice(1, 3), ['Q:0', 'C:160:2']);
    assert.deepEqual(regularCalls, [[SOURCE, 3, Math.fround(0.02)]]);
    // 억제 중이어도 Dude 반응과 난수 소비는 일어나고 생성만 없다.
    const suppressed = contractScene();
    suppressed.state.suppressed = 1;
    const stateBefore = suppressed.random.state;
    new RawGunImpact(suppressed.pool, suppressed.frames, suppressed.setter, suppressed.random, suppressed.state, suppressed.hooks()).ground(50, 50);
    assert.deepEqual(suppressed.events, [`Qd:${floatToBits(50)}:${floatToBits(50)}`]);
    assert.notEqual(suppressed.random.state, stateBefore);
    // Dude 반응이 없으면 난수도 건드리지 않는다.
    const none = contractScene();
    const noneHooks = none.hooks();
    delete noneHooks.notifyDudes;
    const noneBefore = none.random.state;
    assert.throws(() => new RawGunImpact(none.pool, none.frames, none.setter, none.random, none.state, noneHooks).ground(50, 50), /Dude 반응 미복원/);
    assert.deepEqual([none.random.state, none.events], [noneBefore, []]);
});

test('GunImpact handle: 현재 TYPE 의 글자 구간 안의 signed count 만 프레임을 지정하고 payload 를 돌려주며, 구간 밖·음수는 Destroy(0)과 −1 이고 표 밖·삭제 경계 누락은 거부한다', () => {
    const scene = contractScene();
    const logic = new RawGunImpact(scene.pool, scene.frames, scene.setter, scene.random, scene.state, scene.hooks());
    // 글자 D(event 3)는 첫 번호 15 개수 3: count 0..2 는 지정, 3 과 음수는 삭제.
    for (const count of [0, 1, 2]) {
        scene.events.length = 0;
        assert.equal(logic.handle(SOURCE, 3, count, 0.5), 0.5);
        assert.equal(get(scene.pool.slot(SOURCE), 36, 4), 15 + count);
    }
    for (const count of [3, 4, 0xffffffff, 0x80000000]) {
        scene.events.length = 0;
        const frame = get(scene.pool.slot(SOURCE), 36, 4);
        assert.equal(logic.handle(SOURCE, 3, count, 0.5), -1);
        assert.deepEqual(scene.events, ['D:50:0']);
        assert.equal(get(scene.pool.slot(SOURCE), 36, 4), frame);
    }
    // 개수 0 인 글자(A)는 첫 번호가 −1 이고 항상 삭제이다.
    scene.events.length = 0;
    assert.equal(logic.handle(SOURCE, 0, 0, 0.5), -1);
    // 글자 번호가 16 이상이거나 슬롯 TYPE 이 표 밖이면 RangeError 이다.
    const range = (error) => error instanceof RangeError && /실제 글자 구간 오류/.test(error.message);
    assert.throws(() => logic.handle(SOURCE, 16, 0, 0), range);
    // 글자 번호 15('P')는 범위 안이다: 개수 3·첫 번호 75 이다.
    assert.equal(logic.handle(SOURCE, 15, 2, 0.75), 0.75);
    assert.equal(get(scene.pool.slot(SOURCE), 36, 4), 77);
    // 가상 표가 anim 이 아니면 TypeError 이다.
    scene.pool.allocated(SOURCE)[0] ^= 1;
    assert.throws(() => logic.handle(SOURCE, 3, 0, 0), TypeError);
    scene.pool.allocated(SOURCE)[0] ^= 1;
    // 삭제 경계가 없으면 구간 밖에서만 거부한다(구간 안은 필요하지 않다).
    const hooks = scene.hooks();
    delete hooks.destroy;
    const bare = new RawGunImpact(scene.pool, scene.frames, scene.setter, scene.random, scene.state, hooks);
    assert.equal(bare.handle(SOURCE, 3, 1, 0.25), 0.25);
    assert.throws(() => bare.handle(SOURCE, 3, 3, 0.25), /anim 종료 삭제 누락/);
    // 정적 본문은 같은 규칙을 생성자 없이 쓴다.
    scene.events.length = 0;
    assert.equal(RawGunImpact.handleIn(scene.pool, scene.frames, scene.setter, (sid, flags) => { scene.events.push(`X${sid}:${flags}`); }, SOURCE, 3, 9, 0.5), -1);
    assert.deepEqual(scene.events, ['X50:0']);
    // 슬롯 TYPE 이 표 길이(188)와 같거나 크면 글자 구간 오류이고, 표 안의 마지막 TYPE(187)은 빈 구간으로 삭제된다.
    for (const type of [188, 200, 255]) {
        scene.pool.allocated(SOURCE)[10] = type;
        assert.throws(() => RawGunImpact.handleIn(scene.pool, scene.frames, scene.setter, () => {}, SOURCE, 3, 0, 0), range, `TYPE ${type}`);
    }
    scene.pool.allocated(SOURCE)[10] = 187;
    scene.events.length = 0;
    assert.equal(RawGunImpact.handleIn(scene.pool, scene.frames, scene.setter, (sid, flags) => { scene.events.push(`Y${sid}:${flags}`); }, SOURCE, 3, 0, 0), -1);
    assert.deepEqual(scene.events, ['Y50:0']);
});

test('GunImpact handles/handler: raw 가상 표로만 anim 을 판정하고 anim 이 아닌 부모는 기존 처리기로 넘기며 없으면 거부한다', () => {
    const scene = contractScene();
    const logic = new RawGunImpact(scene.pool, scene.frames, scene.setter, scene.random, scene.state, scene.hooks());
    assert.equal(logic.handles(SOURCE), true);
    assert.equal(logic.handles(0), false);
    // 슬롯 0 의 가상 표가 anim 이어도 SID 0 은 빈 값이라 anim 이 아니다.
    put(scene.pool.physical(0), 0, 0x509758);
    assert.equal(logic.handles(0), false);
    put(scene.pool.physical(0), 0, 0);
    assert.equal(logic.handles(CAPACITY), false);
    assert.equal(logic.handles(CAPACITY + 5), false);
    scene.pool.allocated(SOURCE).set([0, 0, 0, 0]);
    assert.equal(logic.handles(SOURCE), false);
    put(scene.pool.allocated(SOURCE), 0, 0x509758);
    assert.equal(logic.handles(SOURCE), true);
    // 타입 번호만으로 추측하지 않는다: TYPE 이 160 이 아니어도 가상 표가 anim 이면 anim 이다.
    scene.pool.allocated(SOURCE)[10] = 0;
    assert.equal(logic.handles(SOURCE), true);
    scene.pool.allocated(SOURCE)[10] = 160;
    // handler: anim 은 직접 처리, 그 밖은 fallback 의 결과와 인자를 그대로 쓴다.
    const calls = [];
    const handler = logic.handler((sid, event, count, payload) => { calls.push([sid, event, count, payload]); return 7.5; });
    assert.equal(handler(SOURCE, 3, 1, 0.5), 0.5);
    assert.deepEqual(calls, []);
    scene.pool.allocated(SOURCE).set([0, 0, 0, 0]);
    assert.equal(handler(SOURCE, 3, 1, 0.5), 7.5);
    assert.deepEqual(calls, [[SOURCE, 3, 1, 0.5]]);
    assert.throws(() => logic.handler()(SOURCE, 3, 1, 0.5), /충돌 효과 외 실제 Handle 미연결/);
});

test('GunImpact 훅 공장: 실제 Factory/Kernel/Host 를 생성·개수·Regular(0x50) 경계에 연결하고 나머지 훅과 원래 묶음은 보존하며 판본/풀 불일치를 거부한다', () => {
    const pool = new SidPool(CAPACITY, { server: false });
    const types = formTypeInputs();
    const factory = new SquidFactory(pool, types);
    const kernel = new Kernel(PROCESS_CAPACITY);
    const destroy = new RawSquidDestroy(pool, new SquidUnpop(pool, new SquidHash(), new Uint8Array(65536)), types);
    const host = new SquidProcessHost(pool, types, kernel, destroy, new SquidProcessState());
    const original = { pop: () => 'pop', destroy: () => 'destroy', notifyDudes: () => 'dudes', processCount: () => 99, create: () => 98, regular: () => 97 };
    const hooks = makeGunImpactRuntimeHooks(pool, factory, kernel, host, original);
    assert.notEqual(hooks, original);
    assert.equal(original.processCount(), 99);
    assert.deepEqual([hooks.pop(), hooks.destroy(), hooks.notifyDudes()], ['pop', 'destroy', 'dudes']);
    assert.equal(hooks.processCount(), 0);
    const sid = hooks.create(71, 2);
    assert.equal(pool.slot(sid)[10], 71);
    // Regular 는 flags 0x50 으로 부착되어 Kernel 개수가 늘고, 부착 실패는 거부한다.
    hooks.regular(sid, 3, 0.5);
    assert.equal(hooks.processCount(), 1);
    assert.equal(host.findEvent(sid, 3).payload, 0.5);
    const failing = { pool, addRegular: () => null };
    assert.throws(() => makeGunImpactRuntimeHooks(pool, factory, kernel, failing, {}).regular(sid, 3, 0.5), /Regular 부착 실패/);
    // Regular 는 변형·간격 그대로 flags 0x50 으로 부착한다.
    const attachCalls = [];
    const recording = { pool, addRegular: (...args) => { attachCalls.push(args); return {}; } };
    makeGunImpactRuntimeHooks(pool, factory, kernel, recording, {}).regular(sid, 5, 0.25);
    assert.deepEqual(attachCalls, [[sid, 5, 0.25, 0x50]]);
    const other = new SidPool(CAPACITY, { server: false });
    const message = (error) => error instanceof TypeError && /Factory\/Host\/Kernel 판본 오류/.test(error.message);
    assert.throws(() => makeGunImpactRuntimeHooks(other, factory, kernel, host, {}), message);
    assert.throws(() => makeGunImpactRuntimeHooks(pool, new SquidFactory(other, types), kernel, host, {}), message);
    assert.throws(() => makeGunImpactRuntimeHooks(pool, factory, new Kernel(100), host, {}), message);
    assert.throws(() => makeGunImpactRuntimeHooks(pool, factory, kernel, { pool: other }, {}), message);
});

test('GunImpact 훅 공장: Missile 의 impact/hitEffect 는 visual/ground 로, 자산의 damageGround 는 ground 로 가고 나머지 훅과 원래 묶음은 보존한다', () => {
    const calls = [];
    const impact = { visual: (...args) => { calls.push(['visual', ...args]); return 'v'; }, ground: (...args) => { calls.push(['ground', ...args]); return 'g'; } };
    const missileOriginal = { destroy: () => 'kept', impact: () => 'old', hitEffect: () => 'old' };
    const missile = makeMissileGunImpactHooks(impact, missileOriginal);
    assert.notEqual(missile, missileOriginal);
    assert.equal(missile.destroy(), 'kept');
    assert.equal(missileOriginal.impact(), 'old');
    missile.impact(1.5, 2.5, 5, 0.375);
    missile.hitEffect(3.5, 4.5);
    assert.deepEqual(calls, [['visual', 1.5, 2.5, 5, 0.375], ['ground', 3.5, 4.5]]);
    assert.deepEqual(Object.keys(makeMissileGunImpactHooks(impact, {})).sort(), ['hitEffect', 'impact']);
    const assetOriginal = { hitPoints: () => 'kept', damageGround: () => 'old' };
    const asset = makeAssetGunImpactHooks(impact, assetOriginal);
    assert.notEqual(asset, assetOriginal);
    assert.equal(asset.hitPoints(), 'kept');
    assert.equal(assetOriginal.damageGround(), 'old');
    asset.damageGround(5.5, 6.5);
    assert.deepEqual(calls.at(-1), ['ground', 5.5, 6.5]);
    assert.deepEqual(Object.keys(makeAssetGunImpactHooks(impact, {})), ['damageGround']);
});
