// 이동 시작 raw-path-start.js(RawPathStart)와 이동 프로세스 선택 raw-movement-process-types.js(RawMovementProcessTypes·selectMovementProcessType)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: pathprocesstype-x86.tsv 의 `originals` 376행(모든 타입 번호와 벌룬 genus 교차의 선택 결과), pathstart-x86.tsv 의 `originals` 256행(부모 134/158 × 장면 profile 0~7 × 권한 × 기존 경로 × 시작점 교체 × 대기 사건)
//    — 실제 BFS·Host·RawPathData 직렬화·SquidFrame 에 연결한 결과(성공 여부, 생성 C·정리 K·Init Pop P·겹침 정리 B 의 공유 효과 순서, 명령 필드 7개, 꼭짓점 문자열, 부모 프레임, 남은 사건 0x25c/0x25d 개수와 재귀 guard).
//  - cpppj 시험 이식: 두 시험의 장면(StartScene)을 같은 입력·같은 순서로 웹 모듈에 옮겼다. 사제는 서버 번호 Form 으로 한 번 송신하고(25바이트 이상) 기본 프레임을 두 번 갱신하며 0x25c/0x25d 를 모두 지운다.
//  - 웹 계약: 입력 검사, 파생 종류의 createType 요구·종류/풀 불일치 거부, 지원하지 않는 부모 통지 거부, Walker/웜/Bulf/벌룬의 간헐천 기록, 선택 상태의 소유자 대여와 값 사본.
// 10.72(CD) 행은 범위 밖이다. 실제 게임·브라우저 실행이 아니다. (2026-11-13 WB04-E351: 6-P RawPathStart 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { SquidPop, RAW_POP_RESULT } from '../www/core/squid-pop.js';
import { SquidFrame } from '../www/core/squid-frame.js';
import { RawSquidDestroy, SQUID_DESTROY_EFFECT } from '../www/core/squid-destroy.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { RawContainedLookup } from '../www/core/raw-contained-lookup.js';
import { Kernel } from '../www/core/kernel.js';
import { RegularProcess, SquidProcessHost, SquidProcessState } from '../www/core/squid-process.js';
import { PATH_LAYOUT } from '../www/core/raw-path-runtime.js';
import { RawPathSearch, RawPathSearchState, makePathSearchHooks } from '../www/core/raw-path-search.js';
import { RawPathProcess, RawPathProcessState } from '../www/core/raw-path-process.js';
import { createPathDataHeader, makePathFormTransmission } from '../www/core/raw-path-data.js';
import { RawMovementProcessTypes, selectMovementProcessType } from '../www/core/raw-movement-process-types.js';
import { RawPathStart, startFootprint } from '../www/core/raw-path-start.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-13 WB04-E351)
const fixtures = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures');
const typeRows = readRows(path.join(fixtures, 'pathprocesstype-x86.tsv'))?.filter((row) => row[0] === 'originals') ?? null;
const startRows = readRows(path.join(fixtures, 'pathstart-x86.tsv'))?.filter((row) => row[0] === 'originals') ?? null;

// 실제 상위 호출의 명령 DWORD 세 개이다. 마지막 인자도 BYTE 로 줄이지 않는다. (2026-11-13 WB04-E351)
const HELPER = 0x12345678;
const TAIL_WORD = 0x87654321;
const TAIL_BYTE = 0xa5fedcba;

// double 비트(십진 문자열)를 수로 바꾼다. (2026-11-13 WB04-E351)
function doubleOf(text) {
    const view = new DataView(new ArrayBuffer(8));
    view.setBigUint64(0, BigInt(text), true);
    return view.getFloat64(0, true);
}

// double 의 비트를 십진 문자열로 바꾼다. (2026-11-13 WB04-E351)
function doubleBits(value) {
    const view = new DataView(new ArrayBuffer(8));
    view.setFloat64(0, value, true);
    return view.getBigUint64(0, true).toString();
}

// 내장 Form 네 생성자 주소만 넣은 188칸 TYPE 입력이다(cpppj MakeFormTypeInputs). (2026-11-13 WB04-E351)
function formTypeInputs() {
    return Array.from({ length: 188 }, (_, number) => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, ...([1, 2, 3, 6].includes(number) ? { constructorAddress: typeConstructorAddress(number) } : {}) }));
}

test('selectMovementProcessType: 원본 10.78 pathprocesstype 376행 — 모든 타입 번호와 벌룬 genus 교차의 선택 결과가 같다', { skip: !typeRows && '원본 fixture 없음' }, () => {
    let checked = 0;
    // 각 실제 PE 의 전체 타입 번호와 balloon genus 교차를 단일 선택 몸체로 대조한다.
    for (const row of typeRows) {
        assert.equal(row.length, 5);
        const types = formTypeInputs();
        const type = Number(row[1]);
        types[type].flags2 = Number(row[2]);
        const state = new RawMovementProcessTypes();
        assert.equal(Number(row[4]), 0);
        assert.equal(selectMovementProcessType(types, state, type), Number(row[3]), `행 ${checked}`);
        checked += 1;
    }
    assert.equal(checked, 376);
});

test('selectMovementProcessType: 우선순위·전역 대여·값 사본·입력 검사', () => {
    const types = formTypeInputs();
    const state = new RawMovementProcessTypes();
    // 기본 번호와 선택: 썬 워커 133·empty 187 → 웜 40, 윈드 137·레인 156 → Walker 38, 텔레포트 134 → 23, 벌프 135 → 41, Dude 70 → 24, 사제 158 → 54, 마나 볼트 105 → 39, 그 밖 → 40.
    const expected = { 133: 40, 187: 40, 137: 38, 156: 38, 134: 23, 135: 41, 70: 24, 158: 54, 105: 39, 0: 40, 99: 40 };
    for (const [type, process] of Object.entries(expected)) assert.equal(selectMovementProcessType(types, state, Number(type)), process, `타입 ${type}`);
    // 벌룬 genus 는 다른 비교(텔레포트·사제 등)보다 앞서고 웜/Walker 보다 뒤이다.
    types[134].flags2 = 0x20000;
    types[133].flags2 = 0x20000;
    types[137].flags2 = 0x20000;
    assert.equal(selectMovementProcessType(types, state, 134), 42);
    assert.equal(selectMovementProcessType(types, state, 133), 40);
    assert.equal(selectMovementProcessType(types, state, 137), 38);
    // 전역을 바꾸면 다음 조회가 그 값을 읽는다.
    state.teleport = 99;
    state.priest = 134;
    assert.equal(selectMovementProcessType(types, state, 99), 23);
    assert.equal(selectMovementProcessType(types, state, 134), 42);
    types[134].flags2 = 0;
    assert.equal(selectMovementProcessType(types, state, 134), 54);
    // 입력 검사: 타입 표 크기와 범위.
    assert.throws(() => selectMovementProcessType(types, state, 188), RangeError);
    assert.throws(() => selectMovementProcessType(types, state, -1), RangeError);
    assert.throws(() => selectMovementProcessType(types.slice(1), state, 0), RangeError);
    assert.throws(() => selectMovementProcessType([...types, { flags2: 0 }], state, 0), RangeError);
    // 소유자 대여: 소유자 값을 읽고 쓰며 값 사본은 독립 저장소를 가진다.
    const priest = { priestType: 200 };
    const roles = { templeProductType: 11, emptyCarrierType: 12, rainWalkerType: 13, bulfType: 14, dudePathType: 15 };
    const shared = new RawMovementProcessTypes({ priest, roles });
    assert.deepEqual([shared.priest, shared.sunWalker, shared.empty, shared.rainWalker, shared.bulf, shared.dudeProcess], [200, 11, 12, 13, 14, 15]);
    priest.priestType = 201;
    roles.bulfType = 99;
    assert.equal(shared.priest, 201);
    assert.equal(selectMovementProcessType(types, shared, 99), 41);
    shared.sunWalker = 77;
    shared.priest = 202;
    assert.deepEqual([roles.templeProductType, priest.priestType], [77, 202]);
    const copy = shared.copy();
    shared.bulf = 5;
    assert.equal(copy.bulf, 99);
    assert.deepEqual([copy.dudeProcess, copy.priest, copy.sunWalker, copy.manaBolt, copy.flyProcess], [15, 202, 77, 105, 39]);
    // 부호 있는 입력도 DWORD 로 저장한다.
    shared.priest = -1;
    assert.equal(priest.priestType, 0xffffffff);
    assert.equal(roles.bulfType, 5);
    assert.throws(() => new RawMovementProcessTypes({ priest: {} }), TypeError);
    assert.throws(() => new RawMovementProcessTypes({ roles: { templeProductType: 1 } }), TypeError);
});

// PE 에 공급한 고유 장면과 같은 SID/hash/spot/프레임/전역을 만든다. 서버 풀이며 client 5..53 번호를 입력으로 확보한다(cpppj StartScene). (2026-11-13 WB04-E351)
class StartScene {
    constructor() {
        this.pool = new SidPool(32768, { server: true });
        this.hash = new SquidHash();
        this.types = formTypeInputs();
        // 검색이 읽는 평면 프레임 코드 배열이다(타입별 { side, variant, number, flags } 목록).
        this.frames = this.types.map(() => []);
        this.shapes = this.types.map(() => ({ frames: [] }));
        this.spots = new Uint8Array(65536);
        this.searchState = new RawPathSearchState();
        this.clock = new SquidProcessState();
        this.pathState = new RawPathProcessState(this.clock);
        this.effects = [];
        // 원본 mutable 부모 및 간헐천 타입 번호와 목적지 스냅/재귀 guard 이다(Start 가 retrying 에 쓴다).
        this.state = { clock: this.clock, dudeType: 70, teleportType: 134, manaBoltType: 105, priestType: 158, geyserType: 122, snapPriestDestination: false, retrying: false };
        // 원본과 같은 client 5..53 번호를 장면 입력으로 확보하며 결과를 계산하지 않는다.
        for (let id = 5; id <= 53; id += 1) assert.equal(this.pool.allocate(2), id);
    }

    // 현재 raw 객체를 해당 단계의 실제 머리/next 로 등록한다. (2026-11-13 WB04-E351)
    insert(sid, type, x, y, level, buried = false) {
        const raw = this.pool.allocated(sid);
        raw.fill(0);
        raw[10] = type;
        raw[40] = buried ? 8 : 0;
        put(raw, 14, floatToBits(x));
        put(raw, 18, floatToBits(y));
        this.types[type].footX = this.types[type].footY = 1;
        put(raw, 4, this.hash.bucket(level, x, y), 2);
        this.hash.setBucket(level, x, y, sid);
    }

    // profile 별 실제 장면 입력만 공급한다. 경로 생성은 RawPathSearch 가 한다. (2026-11-13 WB04-E351)
    setup(parentType, profile, boss) {
        this.clock.now = 123.125;
        this.clock.boss = boss;
        this.state.snapPriestDestination = profile === 1;
        this.state.retrying = profile === 7;
        let next = 6;
        this.types[83].flags1 = 0x800;
        this.types[83].flags2 = 2;
        this.frames[83] = [{ side: 0x41, variant: 0x41, number: 1, flags: 0 }];
        // 네모 표면의 모든 셀을 실제 Graph3/spot6 상태로 넣는다.
        for (let y = 9; y < 15; y += 1) {
            // 같은 줄의 여섯 셀도 각각 실제 독립 SID 를 갖는다.
            for (let x = 9; x < 15; x += 1) {
                const sid = next;
                next += 1;
                this.insert(sid, 83, x, y, 0);
                this.pool.allocated(sid)[30] = 3;
                this.spots[y * 256 + x] = 6;
            }
        }
        this.insert(50, 122, 13.75, 13.25, 1);
        this.insert(51, 122, 13.75, 13.25, 1, true);
        this.insert(52, 122, 13.5, 13.5, 1);
        if (profile === 6) {
            this.insert(53, 150, 10.75, 10.25, 1);
            this.types[150].flags2 = 0x40000;
        }
        const raw = this.pool.allocated(5);
        raw.fill(0);
        raw[10] = parentType;
        raw[11] = profile === 4 ? 4 : 0;
        put(raw, 0, 0x501dd0);
        put(raw, 14, floatToBits(9.2));
        put(raw, 18, floatToBits(9.3));
        raw[34] = 2;
        raw[33] = 1;
        if (profile !== 4) {
            put(raw, 4, this.hash.bucket(1, 9.2, 9.3), 2);
            this.hash.setBucket(1, 9.2, 9.3, 5);
        }
        this.types[parentType].footX = this.types[parentType].footY = 1;
        if (profile === 2 || profile === 6 || profile === 7) {
            this.hash.setCell(0, 10, 10, 0);
            this.hash.setCell(0, 9, 9, 0);
        }
    }

    // 목적지 입력을 PE 도구와 동일한 float32 로 반환한다. (2026-11-13 WB04-E351)
    destination(profile) {
        const point = profile === 3 ? [-1.25, 400.5] : profile === 5 ? [9.2, 9.3] : [13.2, 13.3];
        return { x: Math.fround(point[0]), y: Math.fround(point[1]) };
    }
}

// 시작 장면에 Pop/Unpop·Host·직렬화·Frame·시작기를 조립한 한 벌이다. old 는 기존 경로(legacy)의 효과 기록 여부를 가르는 훅 묶음 선택이다. (2026-11-13 WB04-E351)
function assemble(scene, { createType = null } = {}) {
    const { pool, hash, types, spots } = scene;
    const pop = new SquidPop(pool, hash, spots);
    const unpop = new SquidUnpop(pool, hash, spots);
    const kernel = new Kernel(32);
    // 화면/네트워크 삭제 송신은 호출 관찰 경계다. 실제 Form 제거/부모체인은 Host 가 처리한다.
    const host = new SquidProcessHost(pool, types, kernel, new RawSquidDestroy(pool, unpop, types), scene.clock, {
        asset: { selected: () => 0, emit: (event) => { assert.equal(event.effect, SQUID_DESTROY_EFFECT.transmit); } },
    });
    const search = new RawPathSearch(pool, hash, spots, types, scene.frames, scene.searchState);
    const world = { scene, pool, hash, types, pop, unpop, kernel, host, search, sent: 0, updates: 0, metadataReads: 0, sentForm: 0 };
    const header = createPathDataHeader();
    const buffer = new Uint8Array(2048).fill(0xa5);
    host.setFormTransmit(makePathFormTransmission(pool, types, header, buffer, (length) => {
        assert.ok(length > 0);
        assert.equal(get(buffer, 6, 2), 5);
        world.sentForm = get(buffer, 4, 2);
        assert.ok(world.sentForm >= pool.layout.serverFirst);
        world.sent += 1;
    }));
    // 실제 pop/unpop 을 공급하고 실행하지 않은 걷기/도착 효과는 예외로 경계를 지킨다.
    const processHooks = (old) => makePathSearchHooks(pool, search, {
        unpop: (sid, flags) => { unpop.unpop(sid, types[pool.slot(sid)[10]], flags); },
        pop: (sid, point, flags) => {
            if (!old) scene.effects.push(`P:${floatToBits(point.x)},${floatToBits(point.y)},${flags}`);
            assert.equal(pop.pop(sid, types[pool.slot(sid)[10]], 1, 1, point.x, point.y, flags), RAW_POP_RESULT.registered);
        },
        repop: (sid, flags) => {
            const raw = pool.slot(sid);
            const view = new DataView(raw.buffer, raw.byteOffset, raw.byteLength);
            assert.equal(pop.pop(sid, types[raw[10]], 1, 1, view.getFloat32(14, true), view.getFloat32(18, true), flags), RAW_POP_RESULT.registered);
        },
        halt: () => { throw new Error('시작 대조의 Halt 미실행 경계'); },
        arrival: () => { throw new Error('시작 대조의 Arrival 미실행 경계'); },
        soundId: () => { throw new Error('시작 대조의 발소리 미실행 경계'); },
        stepSound: () => { throw new Error('시작 대조의 발소리 미실행 경계'); },
        clearBlocked: () => { throw new Error('시작 대조의 도착 겹침 미실행 경계'); },
        // native Kill 인자는 0 이지만 실제 서버의 client Form 삭제는 공통 destroy 가 0x10 을 더해 cleanup 에 전달한다.
        cleanup: (runtime, parent, flags) => {
            assert.ok(parent === 5 && flags === (old ? 0x10 : 0));
            if (old) scene.effects.push('K');
        },
    });
    world.processHooks = processHooks;
    const frame = new SquidFrame(pool, types, {
        frameSize: () => [1, 1],
        update: (sid, flags) => { assert.ok(sid === 5 && flags === 0); world.updates += 1; },
        unpop: (sid, flags) => { unpop.unpop(sid, types[pool.slot(sid)[10]], flags); },
        pop: (sid, x, y, flags) => { assert.equal(pop.pop(sid, types[pool.slot(sid)[10]], 1, 1, x, y, flags), RAW_POP_RESULT.registered); },
    });
    const lookup = new RawContainedLookup(pool, (form, flags) => {
        const process = host.fromForm(form);
        assert.ok(process);
        host.kill(process, flags);
    });
    const typeState = new RawMovementProcessTypes();
    const hooks = {
        processType: (parentType) => selectMovementProcessType(types, typeState, parentType),
        create: (priest) => {
            scene.effects.push('C');
            return new RawPathProcess(pool, hash, types, scene.frames, scene.shapes, spots, scene.pathState, processHooks(false), { priest });
        },
        metadata: (parent) => {
            assert.equal(parent, 5);
            world.metadataReads += 1;
            if (world.metadataReads === 2) assert.ok(!host.findEvent(parent, 0x25c) && !host.findEvent(parent, 0x25d));
            return { speed: Math.fround(0.7), turningSpeed: 90, defaultFrame: 7 };
        },
        clearBlocked: (point, parentType, owner) => { scene.effects.push(`B:${floatToBits(point.x)},${floatToBits(point.y)},${parentType},${owner}`); },
    };
    if (createType) hooks.createType = createType(world);
    world.hooks = hooks;
    world.starter = new RawPathStart(pool, hash, types, host, lookup, frame, scene.state, hooks);
    return world;
}

// 사제 시작이 다른 시험에서도 쓰는 요청 한 건이다. (2026-11-13 WB04-E351)
function requestOf(scene, profile, replace) {
    return { parent: 5, destination: scene.destination(profile), replaceStart: replace ? 1 : 0, helper: HELPER, tailWord: TAIL_WORD, tailByte: TAIL_BYTE, reserved: 0x13579bdf };
}

// 경로 꼭짓점/방향/시각 비트를 원본 관찰 문자열로 읽는다. (2026-11-13 WB04-E351)
function startPoints(runtime) {
    const parts = [];
    // 원본 배열 순서로 읽으며 기대 경로를 생성하지 않는다.
    for (let index = 0; index < runtime.count; index += 1) {
        const point = runtime.point(index);
        parts.push(`${floatToBits(point.x)},${floatToBits(point.y)},${runtime.direction(index)},${doubleBits(runtime.time(index))}`);
    }
    return parts.length === 0 ? '-' : parts.join(';');
}

// 관찰 이벤트에서 생성·이전 정리·겹침 요청·Init Pop 만 정확한 순서로 고른다. (2026-11-13 WB04-E351)
function sharedEvents(text) {
    const selected = text.split(';').filter((event) => event === 'C' || event === 'K' || event.startsWith('P:') || event.startsWith('B:'));
    return selected.length === 0 ? '-' : selected.join(';');
}

test('RawPathStart: 원본 10.78 pathstart 256행 — 실제 BFS/Host/RawPathData/SquidFrame 에 연결한 시작 결과와 공유 효과가 같다', { skip: !startRows && '원본 fixture 없음' }, () => {
    let checked = 0;
    // 모든 고유 장면은 서로의 free list/예약/경로를 공유하지 않는 새 월드로 만든다.
    for (const row of startRows) {
        assert.equal(row.length, 8);
        const input = row[1].split(',');
        assert.equal(input.length, 6);
        const type = Number(input[0]);
        const profile = Number(input[1]);
        const pending = Number(input[5]);
        const boss = Number(input[2]) !== 0;
        const legacy = Number(input[3]) !== 0;
        const replace = Number(input[4]) !== 0;
        const scene = new StartScene();
        scene.setup(type, profile, boss);
        const world = assemble(scene);
        const { pool, host, kernel } = world;
        const where = `행 ${checked} (${row[1]})`;
        try {
            if (legacy) {
                const old = new RawPathProcess(pool, scene.hash, scene.types, scene.frames, scene.shapes, scene.spots, scene.pathState, world.processHooks(true), { priest: type === 158 });
                old.runtime.setTargets(0x87654321, 0xfedcba98);
                assert.ok(RawPathProcess.attach(host, old, 5, 0x10));
            }
            // 서로 다른 사건을 반복 등록해 사제 시작의 전체 삭제/다시 검색을 검사한다.
            for (const event of [0x25c, 0x25d]) for (let index = 0; index < pending; index += 1) assert.ok(host.addRegular(5, event, 1));
            const result = world.starter.start(requestOf(scene, profile, replace));
            assert.equal(result !== null, Number(row[2]) !== 0, `${where} 성공`);
            assert.equal(scene.effects.length === 0 ? '-' : scene.effects.join(';'), sharedEvents(row[3]), `${where} 공유 효과`);
            if (result) {
                assert.equal(startPoints(result.runtime), row[5], `${where} 꼭짓점`);
                const raw = result.runtime.bytes;
                const fields = [PATH_LAYOUT.targetA, PATH_LAYOUT.targetB, PATH_LAYOUT.helper, 0x186e4, 0x186e8, 0x186dc, 0x186e0].map((offset) => get(raw, offset));
                assert.equal(fields.join(','), row[4], `${where} 명령 필드`);
                assert.equal(get(pool.slot(5), 6, 2), result.form);
                assert.ok(get(pool.slot(result.form), 16, 2) === 5 && get(raw, 4) === result.form && get(raw, 8) === 5);
                if (type === 158) {
                    assert.ok(world.sent === 1 && result.form === world.sentForm && world.updates === 2);
                    assert.ok(!host.findEvent(5, 0x25c) && !host.findEvent(5, 0x25d));
                } else {
                    assert.ok(world.sent === 0 && result.form < pool.layout.serverFirst && world.updates === 0);
                }
            } else {
                assert.ok(world.sent === 0 && world.updates === 0 && world.metadataReads === 0, `${where} 실패 부수효과`);
            }
            assert.equal(get(pool.slot(5), 36, 4), Number(row[6]), `${where} 부모 프레임`);
            const remaining = [0, 0];
            // 같은 Host 의 현재 종속 체인에서 실제 남은 각 사건의 개수를 읽는다.
            for (let current = get(pool.slot(5), 6, 2); current; current = get(pool.slot(current), 4, 2)) {
                const regular = host.fromForm(current);
                if (regular instanceof RegularProcess) {
                    if (regular.event === 0x25c) remaining[0] += 1;
                    if (regular.event === 0x25d) remaining[1] += 1;
                }
            }
            assert.equal(`${remaining[0]},${remaining[1]},${scene.state.retrying ? 1 : 0}`, row[7], `${where} 남은 사건`);
        } finally {
            kernel.clear();
        }
        checked += 1;
    }
    assert.equal(checked, 256);
});

test('RawPathStart: 입력 검사 — 부모 SID·유한 목적지는 부착 전에 거부하고, 생성 연결이 맞지 않으면 시작하지 않는다', () => {
    const scene = new StartScene();
    scene.setup(134, 0, true);
    const world = assemble(scene);
    try {
        const base = requestOf(scene, 0, false);
        assert.throws(() => world.starter.start({ ...base, parent: 0 }), /입력 범위/);
        assert.throws(() => world.starter.start({ ...base, parent: 40000 }), /입력 범위/);
        assert.throws(() => world.starter.start({ ...base, destination: { x: NaN, y: 1 } }), /입력 범위/);
        assert.throws(() => world.starter.start({ ...base, destination: { x: 1, y: Infinity } }), /입력 범위/);
        assert.deepEqual(scene.effects, []);
        // 생성자 연결: 다른 풀의 호스트/조회/프레임, 판본 표, 필수 훅 누락을 거부한다.
        const { pool, hash, types } = scene;
        const { host } = world;
        const other = new SidPool(32768, { server: true });
        const make = (overrides = {}) => new RawPathStart(overrides.pool ?? pool, hash, overrides.types ?? types, overrides.host ?? host, overrides.lookup ?? new RawContainedLookup(pool, () => {}),
            overrides.frame ?? new SquidFrame(pool, types, { frameSize: () => [1, 1], update: () => {}, unpop: () => {}, pop: () => {} }), scene.state, overrides.hooks ?? world.hooks);
        assert.ok(make());
        assert.throws(() => make({ pool: other }), TypeError);
        assert.throws(() => make({ lookup: new RawContainedLookup(other, () => {}) }), TypeError);
        assert.throws(() => make({ frame: new SquidFrame(other, types, { frameSize: () => [1, 1], update: () => {}, unpop: () => {}, pop: () => {} }) }), TypeError);
        assert.throws(() => make({ types: types.slice(1) }), TypeError);
        for (const name of ['processType', 'create', 'metadata', 'clearBlocked']) {
            const hooks = { ...world.hooks };
            delete hooks[name];
            assert.throws(() => make({ hooks }), TypeError, name);
        }
    } finally {
        world.kernel.clear();
    }
});

test('RawPathStart: 파생 종류 선택은 createType 을 요구하고 종류/풀이 맞지 않는 생성은 거부하며, 미지원 종류는 미복원 경계이다', () => {
    // processType 이 파생 번호(42)를 고르면 createType 이 없을 때 생성 전에 거부한다.
    const scene = new StartScene();
    scene.setup(134, 0, true);
    const world = assemble(scene);
    try {
        world.hooks.processType = () => 42;
        const starter = new RawPathStart(world.host.pool, scene.hash, scene.types, world.host, new RawContainedLookup(world.host.pool, () => {}),
            new SquidFrame(world.host.pool, scene.types, { frameSize: () => [1, 1], update: () => {}, unpop: () => {}, pop: () => {} }), scene.state, world.hooks);
        assert.throws(() => starter.start(requestOf(scene, 0, false)), /파생 이동의 실제 생성자 미연결/);
        assert.deepEqual(scene.effects, []);
        world.hooks.processType = () => 99;
        const unsupported = new RawPathStart(world.host.pool, scene.hash, scene.types, world.host, new RawContainedLookup(world.host.pool, () => {}),
            new SquidFrame(world.host.pool, scene.types, { frameSize: () => [1, 1], update: () => {}, unpop: () => {}, pop: () => {} }), scene.state, world.hooks);
        assert.throws(() => unsupported.start(requestOf(scene, 0, false)), /다른 파생 이동 생성\/Init 미복원 경계/);
    } finally {
        world.kernel.clear();
    }
    // createType 이 있으면 create 대신 그것으로 만들고, 생성이 null·다른 풀·다른 종류이면 거부한다.
    for (const [name, build, message] of [
        ['null', () => null, /생성 실패/],
        ['다른 종류', (w) => new RawPathProcess(w.host.pool, w.scene.hash, w.scene.types, w.scene.frames, w.scene.shapes, w.scene.spots, w.scene.pathState, w.processHooks(false), {}), /생성 종류 불일치/],
        ['다른 풀', (w) => {
            const foreign = new SidPool(32768, { server: true });
            return new RawPathProcess(foreign, w.scene.hash, w.scene.types, w.scene.frames, w.scene.shapes, w.scene.spots, w.scene.pathState, w.processHooks(false), { balloon: true });
        }, /생성 풀 불일치/],
    ]) {
        const fresh = new StartScene();
        fresh.setup(134, 0, true);
        const created = [];
        const built = assemble(fresh, { createType: (w) => (type) => { created.push(type); return build(w); } });
        try {
            built.hooks.processType = () => 42;
            const starter = new RawPathStart(built.host.pool, fresh.hash, fresh.types, built.host, new RawContainedLookup(built.host.pool, () => {}),
                new SquidFrame(built.host.pool, fresh.types, { frameSize: () => [1, 1], update: () => {}, unpop: () => {}, pop: () => {} }), fresh.state, built.hooks);
            assert.throws(() => starter.start(requestOf(fresh, 0, false)), message, name);
            assert.deepEqual(created, [42], name);
            assert.ok(!fresh.effects.includes('C'), name);
        } finally {
            built.kernel.clear();
        }
    }
});

test('RawPathStart: Walker/웜/Bulf/벌룬 시작은 사제 부모에서 간헐천만 기록하고 사건·프레임은 건드리지 않으며, 지원하지 않는 부모는 통지에서 거부한다', () => {
    // 부모 TYPE 158(사제)에 파생 종류를 붙인다: createType 으로 해당 종류를 만든다.
    for (const [processNumber, kinds] of [[38, { walker: true }], [40, { worm: true }], [41, { bulf: true }]]) {
        const scene = new StartScene();
        scene.setup(158, 0, true);
        scene.state.sunWalkerType = 158;
        const world = assemble(scene, { createType: (w) => () => new RawPathProcess(w.host.pool, scene.hash, scene.types, scene.frames, scene.shapes, scene.spots, scene.pathState, w.processHooks(false), kinds) });
        try {
            world.hooks.processType = () => processNumber;
            // 사제 이외의 부착 검사(웜은 sunWalker/empty 부모)를 통과하도록 현재 전역 타입을 맞춘다.
            const typeState = new RawMovementProcessTypes();
            assert.ok(typeState);
            const starter = new RawPathStart(world.host.pool, scene.hash, scene.types, world.host, new RawContainedLookup(world.host.pool, () => {}),
                new SquidFrame(world.host.pool, scene.types, { frameSize: () => [1, 1], update: () => { world.updates += 1; }, unpop: () => {}, pop: () => {} }), scene.state, world.hooks);
            scene.pathState.sunWalkerType = 158;
            const result = starter.start(requestOf(scene, 0, false));
            assert.ok(result, `종류 ${processNumber}`);
            assert.equal(result.movementType(), processNumber);
            // 목적지 (13.2,13.3) 칸의 체인은 머리 52 → 매몰된 51(건너뜀) → 50 이고 마지막으로 만난 간헐천 50 이 targetA 이다.
            assert.equal(result.runtime.targetA, 50, `종류 ${processNumber}`);
            assert.equal(world.updates, 0);
        } finally {
            world.kernel.clear();
        }
    }
    // 텔레포트 프로세스지만 부모 TYPE 이 텔레포트/마나 볼트/Dude/사제 어느 것도 아니면 통지에서 거부한다.
    const scene = new StartScene();
    scene.setup(100, 0, true);
    const world = assemble(scene);
    try {
        world.hooks.processType = () => 23;
        const starter = new RawPathStart(world.host.pool, scene.hash, scene.types, world.host, new RawContainedLookup(world.host.pool, () => {}),
            new SquidFrame(world.host.pool, scene.types, { frameSize: () => [1, 1], update: () => {}, unpop: () => {}, pop: () => {} }), scene.state, world.hooks);
        assert.throws(() => starter.start(requestOf(scene, 0, false)), /다른 부모 이동 통지 미복원 경계/);
    } finally {
        world.kernel.clear();
    }
});

// 시작기와 호스트를 새 장면에 조립하고 Init 호출 인자를 기록하는 도우미이다: processType 훅을 지정한 종류로 고정하고(기본 텔레포트 23), 만든 프로세스의 initializePath 인자를 world.initArgs 에 쌓는다. (2026-11-13 WB04-E351)
function spyWorld(parentType, profile, boss, { processNumber = 23, state = {} } = {}) {
    const scene = new StartScene();
    scene.setup(parentType, profile, boss);
    Object.assign(scene.state, state);
    const world = assemble(scene);
    world.initArgs = [];
    const create = world.hooks.create;
    world.hooks.create = (priest) => {
        const process = create(priest);
        const original = process.initializePath.bind(process);
        process.initializePath = (...args) => { world.initArgs.push(args); return original(...args); };
        return process;
    };
    world.hooks.processType = () => processNumber;
    world.starter = new RawPathStart(scene.pool, scene.hash, scene.types, world.host, new RawContainedLookup(scene.pool, (form, flags) => { world.host.kill(world.host.fromForm(form), flags); }),
        new SquidFrame(scene.pool, scene.types, { frameSize: () => [1, 1], update: () => { world.updates += 1; }, unpop: () => {}, pop: () => {} }), scene.state, world.hooks);
    return world;
}

test('RawPathStart: 목적지는 [0, 254] 로 자르고(0.5 와 254.5 는 그대로/254), 부모 SID 가 풀 용량과 같아도 거부한다', () => {
    // [입력, 기대 Init 목적지]
    const cases = [[{ x: -5, y: 300 }, { x: 0, y: 254 }], [{ x: 0.5, y: 254.5 }, { x: 0.5, y: 254 }], [{ x: 254, y: 0 }, { x: 254, y: 0 }], [{ x: 300, y: -0.25 }, { x: 254, y: 0 }]];
    for (const [input, expected] of cases) {
        const world = spyWorld(134, 0, true);
        try {
            world.starter.start({ ...requestOf(world.scene, 0, false), destination: input });
        } catch {
            // 목적지가 지도 밖이면 검색이 실패해 null 일 수 있다. Init 인자는 기록된다.
        } finally {
            world.kernel.clear();
        }
        assert.equal(world.initArgs.length >= 1, true);
        assert.deepEqual(world.initArgs[0][1], expected, JSON.stringify(input));
    }
    const world = spyWorld(134, 0, true);
    try {
        assert.throws(() => world.starter.start({ ...requestOf(world.scene, 0, false), parent: 32768 }), /입력 범위/);
    } finally {
        world.kernel.clear();
    }
});

test('RawPathStart: Dude/마나 볼트/텔레포트 부모는 client 번호 Form(0x10)으로 부착하고 사제는 서버 번호 Form 으로 부착한다', () => {
    // [부모 TYPE, 프로세스 번호, client 번호 기대]. Dude(70)·마나 볼트(105)·텔레포트(134)는 flags 0x10, 사제(158)는 0.
    for (const [parentType, processNumber, client] of [[70, 23, true], [105, 23, true], [134, 23, true], [158, 54, false]]) {
        const world = spyWorld(parentType, 0, true, { processNumber });
        try {
            const result = world.starter.start(requestOf(world.scene, 0, false));
            assert.ok(result, `부모 ${parentType}`);
            assert.equal(result.form < world.scene.pool.layout.serverFirst, client, `부모 ${parentType}`);
            assert.equal(world.sent, client ? 0 : 1, `부모 ${parentType}`);
        } finally {
            world.kernel.clear();
        }
    }
});

test('RawPathStart: 사제 목적지 스냅은 사제 부모·스냅 전역·flags 0 일 때만 올림 칸 − 0.5 로 바꾼다', () => {
    // [부모 TYPE, 스냅 전역, 사제 TYPE 전역, 프로세스 번호, 기대 스냅]. 사제 TYPE 전역을 134 로 두면 flags 0x10 이 스냅을 막는지 볼 수 있다.
    const cases = [
        [158, true, 158, 54, true],
        [158, false, 158, 54, false],
        [134, true, 158, 23, false],
        [134, true, 134, 23, false],
        [100, true, 100, 23, true],
        [100, true, 158, 23, false],
    ];
    for (const [parentType, snap, priestType, processNumber, snapped] of cases) {
        const world = spyWorld(parentType, 0, true, { processNumber, state: { snapPriestDestination: snap, priestType } });
        try {
            try { world.starter.start(requestOf(world.scene, 0, false)); } catch { /* 지원하지 않는 부모의 통지는 거부된다 */ }
        } finally {
            world.kernel.clear();
        }
        // 목적지 (13.2, 13.3) → 올림 (14, 14) − 0.5 = (13.5, 13.5).
        assert.deepEqual(world.initArgs[0][1], snapped ? { x: 13.5, y: 13.5 } : world.scene.destination(0), `부모 ${parentType} 스냅 ${snap} 사제 ${priestType}`);
    }
});

test('RawPathStart: 시작 뒤 완료 표시는 꺼지고, 대상 둘은 이어받으며, 시작점 교체는 첫 꼭짓점 좌표가 둘 다 같을 때만 연속 꼭짓점에 적용한다', () => {
    const world = spyWorld(134, 0, true);
    try {
        // 생성 직후 완료 표시가 켜져 있어도 Start 가 끄고 대상 둘을 새로 쓴다.
        const create = world.hooks.create;
        world.hooks.create = (priest) => {
            const process = create(priest);
            process.runtime.setFinished(true);
            process.runtime.setTargets(7, 8);
            return process;
        };
        world.starter = new RawPathStart(world.scene.pool, world.scene.hash, world.scene.types, world.host, new RawContainedLookup(world.scene.pool, () => {}),
            new SquidFrame(world.scene.pool, world.scene.types, { frameSize: () => [1, 1], update: () => {}, unpop: () => {}, pop: () => {} }), world.scene.state, world.hooks);
        const result = world.starter.start(requestOf(world.scene, 0, false));
        assert.ok(result);
        assert.equal(result.runtime.finished, false);
        assert.deepEqual([result.runtime.targetA, result.runtime.targetB], [0, 0]);
    } finally {
        world.kernel.clear();
    }
    // 시작점 교체: 부모 (9.2, 9.3) 의 올림 (10, 10). 첫 꼭짓점이 (10, 5)처럼 한 좌표만 같으면 교체하지 않고(바로 중단), 둘 다 같으면 연속으로 교체한다.
    for (const [first, replaced] of [[{ x: 10, y: 5 }, false], [{ x: 10, y: 10 }, true], [{ x: 5, y: 10 }, false]]) {
        const spy = spyWorld(134, 0, true);
        try {
            const create = spy.hooks.create;
            spy.hooks.create = (priest) => {
                const process = create(priest);
                process.initializePath = () => {
                    process.runtime.setPath([first, first, { x: 20, y: 20 }], [0, 0, 3]);
                };
                return process;
            };
            spy.starter = new RawPathStart(spy.scene.pool, spy.scene.hash, spy.scene.types, spy.host, new RawContainedLookup(spy.scene.pool, () => {}),
                new SquidFrame(spy.scene.pool, spy.scene.types, { frameSize: () => [1, 1], update: () => {}, unpop: () => {}, pop: () => {} }), spy.scene.state, spy.hooks);
            const result = spy.starter.start(requestOf(spy.scene, 0, true));
            assert.ok(result);
            // 교체되면 처음 두 꼭짓점이 시작점(부모 현재 좌표 9.2, 9.3)이 된다.
            const start = { x: Math.fround(9.2), y: Math.fround(9.3) };
            assert.deepEqual([result.runtime.point(0), result.runtime.point(1)], replaced ? [start, start] : [first, first], JSON.stringify(first));
        } finally {
            spy.kernel.clear();
        }
    }
});

test('RawPathStart: 실패 정리는 부모 발자국 주변의 막는 부류(0x8c044200 의 어느 비트든)만 정리 훅에 알리고 최상위 비트만 있는 부류도 포함한다', () => {
    // 표면 단계(level 0)의 같은 부류 후보는 건너뛴다(skipSurface). 프로필 2: 시작 표면이 비어 Init 이 꼭짓점을 만들지 못해 실패 정리로 간다. 부모 (9.2, 9.3) 주변 칸에 후보를 둔다.
    // [후보 genus, 정리 훅 호출 기대]
    for (const [genus, called] of [[0x80000000, true], [0x00004000, true], [0x00040000, true], [0x08000000, true], [0x00000200, true], [0x00000001, false], [0x00000100, false], [0x00001000, false]]) {
        const world = spyWorld(134, 2, true);
        try {
            const candidate = world.scene.pool.allocate(2);
            world.scene.insert(candidate, 77, 9.5, 9.5, 1);
            world.scene.types[77].flags2 = genus;
            const result = world.starter.start(requestOf(world.scene, 2, false));
            assert.equal(result, null);
            const cleared = world.scene.effects.filter((entry) => entry.startsWith('B:'));
            assert.equal(cleared.length > 0, called, `genus ${genus.toString(16)}`);
        } finally {
            world.kernel.clear();
        }
    }
});

test('RawPathStart: 파생 이동의 간헐천 기록은 표면 단계를 건너뛰고 마지막 후보를 쓰며, 부착이 거부되면 시작은 오류이다', () => {
    // 사제 부모(158)의 파생 Walker 시작: 목적지 (11.5, 11.5) 칸의 표면 SID(20)를 간헐천 TYPE 으로 바꿔도 표면 단계는 건너뛴다.
    const scene = new StartScene();
    scene.setup(158, 0, true);
    const world = assemble(scene, { createType: (w) => () => new RawPathProcess(w.pool, w.scene.hash, w.scene.types, w.scene.frames, w.scene.shapes, w.scene.spots, w.scene.pathState, w.processHooks(false), { walker: true }) });
    try {
        world.hooks.processType = () => 38;
        const metadata = world.hooks.metadata;
        world.hooks.metadata = (parent) => {
            // 첫 호출(Prepare 직전)에서 표면 SID 의 TYPE 을 간헐천 TYPE 으로 바꾼다.
            if (world.metadataReads === 0) world.pool.allocated(20)[10] = 122;
            return metadata(parent);
        };
        const starter = new RawPathStart(world.pool, scene.hash, scene.types, world.host, new RawContainedLookup(world.pool, () => {}),
            new SquidFrame(world.pool, scene.types, { frameSize: () => [1, 1], update: () => {}, unpop: () => {}, pop: () => {} }), scene.state, world.hooks);
        const result = starter.start({ ...requestOf(scene, 0, false), destination: { x: 11.5, y: 11.5 } });
        assert.ok(result);
        assert.equal(result.runtime.targetA, 0);
    } finally {
        world.kernel.clear();
    }
    // 부모가 죽은 채로 부착하면 패치판은 등록하지 않고 시작은 오류이다.
    const second = spyWorld(134, 0, true);
    try {
        const metadata = second.hooks.metadata;
        second.hooks.metadata = (parent) => { second.scene.pool.allocated(5)[11] |= 2; return metadata(parent); };
        second.starter = new RawPathStart(second.scene.pool, second.scene.hash, second.scene.types, second.host, new RawContainedLookup(second.scene.pool, () => {}),
            new SquidFrame(second.scene.pool, second.scene.types, { frameSize: () => [1, 1], update: () => {}, unpop: () => {}, pop: () => {} }), second.scene.state, second.hooks);
        assert.throws(() => second.starter.start(requestOf(second.scene, 0, false)), /부모 소멸 계약 미복원 경계/);
    } finally {
        second.kernel.clear();
    }
});

test('startFootprint: 확장 입력만 1..255 로 자르고, 한 축이 0 이거나 256 이상이면 원래 점 전체를 무효로 보아 확장 입력만 영역으로 쓴다', () => {
    // [점, 발자국, 기대 영역]
    const cases = [
        [{ x: 10.5, y: 20.5 }, [1, 1], { left: 10, top: 20, right: 11, bottom: 21 }],
        [{ x: 10.5, y: 20.5 }, [3, 3], { left: 8, top: 18, right: 11, bottom: 21 }],
        [{ x: 10.5, y: 20.5 }, [1, 3], { left: 10, top: 18, right: 11, bottom: 21 }],
        [{ x: 10.5, y: 20.5 }, [3, 1], { left: 8, top: 20, right: 11, bottom: 21 }],
        [{ x: 1.5, y: 1.5 }, [5, 5], { left: 1, top: 1, right: 2, bottom: 2 }],
        [{ x: 300, y: 20.5 }, [1, 1], { left: 255, top: 20, right: 255, bottom: 21 }],
        [{ x: 256, y: 5.5 }, [1, 1], { left: 255, top: 5, right: 255, bottom: 6 }],
        [{ x: 0, y: 5.5 }, [1, 1], { left: 1, top: 5, right: 1, bottom: 6 }],
        [{ x: 5.5, y: 0 }, [1, 1], { left: 5, top: 1, right: 6, bottom: 1 }],
        [{ x: 5.5, y: 256 }, [1, 1], { left: 5, top: 255, right: 6, bottom: 255 }],
        [{ x: 255.5, y: 255.5 }, [1, 1], { left: 255, top: 255, right: 256, bottom: 256 }],
    ];
    for (const [point, foot, expected] of cases) assert.deepEqual(startFootprint(point, foot[0], foot[1]), expected, JSON.stringify([point, foot]));
    assert.throws(() => startFootprint({ x: NaN, y: 1 }, 1, 1), RangeError);
    assert.throws(() => startFootprint({ x: 1, y: Infinity }, 1, 1), RangeError);
    assert.throws(() => startFootprint({ x: 1, y: 1 }, -1, 1), RangeError);
    assert.throws(() => startFootprint({ x: 1, y: 1 }, 1, -1), RangeError);
});

test('RawPathStart: 시작기는 훅 사본을 쓰고, 부모 좌표가 비유한이면 시작점 교체의 올림 계산이 거부한다', () => {
    const world = spyWorld(134, 0, true);
    try {
        const hooks = world.hooks;
        const original = hooks.processType;
        const starter = world.starter;
        // 만든 뒤 원본 훅 객체를 바꿔도 시작기는 만든 시점의 효과를 쓴다.
        hooks.processType = () => { throw new Error('훅 사본이 아님'); };
        const result = starter.start(requestOf(world.scene, 0, false));
        assert.ok(result);
        assert.ok(original);
    } finally {
        world.kernel.clear();
    }
    // 부모 좌표가 비유한이면 시작점 교체의 올림 계산이 거부한다(Init 이 부모 위치를 읽지 않도록 경로를 직접 둔다).
    const nan = spyWorld(134, 0, true);
    try {
        const create = nan.hooks.create;
        nan.hooks.create = (priest) => {
            const process = create(priest);
            process.initializePath = () => { process.runtime.setPath([{ x: 10, y: 10 }, { x: 20, y: 20 }], [0, 3]); };
            return process;
        };
        nan.starter = new RawPathStart(nan.scene.pool, nan.scene.hash, nan.scene.types, nan.host, new RawContainedLookup(nan.scene.pool, () => {}),
            new SquidFrame(nan.scene.pool, nan.scene.types, { frameSize: () => [1, 1], update: () => {}, unpop: () => {}, pop: () => {} }), nan.scene.state, nan.hooks);
        put(nan.scene.pool.allocated(5), 14, 0x7fc00000);
        assert.throws(() => nan.starter.start({ ...requestOf(nan.scene, 0, true), destination: { x: 13, y: 13 } }), /비정상 좌표/);
    } finally {
        nan.kernel.clear();
    }
});

test('RawPathStart: 실패 정리는 표면 단계(level 0)의 후보를 건너뛴다', () => {
    const world = spyWorld(134, 2, true);
    try {
        const candidate = world.scene.pool.allocate(2);
        world.scene.insert(candidate, 77, 9.5, 9.5, 0);
        world.scene.types[77].flags2 = 0x80000000;
        const result = world.starter.start(requestOf(world.scene, 2, false));
        assert.equal(result, null);
        assert.equal(world.scene.effects.filter((entry) => entry.startsWith('B:')).length, 0);
    } finally {
        world.kernel.clear();
    }
});
