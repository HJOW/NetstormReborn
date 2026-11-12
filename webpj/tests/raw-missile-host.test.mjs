// 투사체 Host 공장 raw-missile.js(makeMissileFactoryHooks·createMissile·makeGunMissileHooks·MissileProcess.destroy)를 cpppj 계약과 합성 입력으로 검사한다.
//  - cpppj MissileTests 의 Missile_ActualFactorySpaceKernelAndDelete(PATCH): 실제 SquidFactory 자산·소유자·공간 Pop/Unpop·Kernel 부착(flags 0x10)·공통 삭제 수명을 같은 풀에서 검증한다.
//    범위 종료 한 프레임 뒤 Kernel 크기 0·표시 자산과 Form 이 모두 삭제 상태·총 사거리 갱신 1회·Pop 2회·삭제기 깊이 0.
//  - 웹 계약: 풀이 다르면 거부(생성 전에 아무 훅도 부르지 않음)·create 만 교체하고 나머지 훅은 보존·원래 묶음 불변·부착 인자(TYPE 28·표시 자산·flags 0x10)·부착 실패는 null/거부·
//    makeGunMissileHooks 가 createProjectile 만 교체·MissileProcess.destroy 가 진행 중 프레임에 자기 삭제를 알림(C++ ~Missile).
// 호스트 진단 중 자기 삭제(MissileChecking_ActualHostDeletion…)는 missile-checking.test.mjs 에 있다. 10.72(CD)는 범위 밖이다. 실제 게임·브라우저 실행이 아니다. (2026-11-13 WB04-E361: Host 공장 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import { put, get, floatToBits, bitsToFloat } from './raw-support.mjs';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { SidPool } from '../www/core/sid-pool.js';
import { Kernel } from '../www/core/kernel.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { SquidPostPop } from '../www/core/squid-postpop.js';
import { SquidPostPopState } from '../www/core/squid-postpop-state.js';
import { SquidPop, RAW_POP_RESULT } from '../www/core/squid-pop.js';
import { SquidFactory } from '../www/core/squid-factory.js';
import { SquidOwner, SquidOwnerMode } from '../www/core/squid-owner.js';
import { RawSquidDestroy } from '../www/core/squid-destroy.js';
import { SquidProcessHost, SquidProcessState } from '../www/core/squid-process.js';
import { SquidDeletionState, SquidDestroyLifecycle } from '../www/core/squid-destroy-lifecycle.js';
import { RawGunHit, makeMissileGunHitHooks } from '../www/core/raw-gun-hit.js';
import { MissileFields } from '../www/core/raw-missile-fields.js';
import { MissileProcess, MissileState, RawMissile, createMissile, makeGunMissileHooks, makeMissileFactoryHooks } from '../www/core/raw-missile.js';

// 풀 용량(진단이 읽는 현재 상한)이다. (2026-11-13 WB04-E361)
const CAPACITY = 24000;

// 예외가 나는지 본다. (2026-11-13 WB04-E361)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 188칸 TYPE 입력이다(cpppj MakeFormTypeInputs: 모든 칸 0 + 내장 Form 네 생성자 주소). (2026-11-13 WB04-E361)
function formTypeInputs() {
    return Array.from({ length: 188 }, (_, number) => ({
        cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0, group: 0, hotspotX: 0, hotspotY: 0,
        ...([1, 2, 3, 6].includes(number) ? { constructorAddress: typeConstructorAddress(number) } : {}),
    }));
}

// 요청 구조이다(cpppj GunProjectileRequest 기본값: 속도 20·표시 80·반사 0·포구 기본 선택을 뜻하는 무효 좌표 (0,0)): 발사/표적/표적 위치/표적 중심(무효)/효과 종류 0. (2026-11-13 WB04-E361)
const request = (firing, target) => ({ firing, target, speed: 20, displayType: 80, effectType: 0, targetPosition: [50, 60], targetCenter: [-10000, -10000], ricochet: 0, firingPosition: [0, 0] });

// 실제 공장·소유자·Pop/Unpop·Kernel·삭제 수명·총 피해 몸체를 같은 풀에 묶은 세계이다(cpppj Missile_ActualFactorySpaceKernelAndDelete 의 장면). (2026-11-13 WB04-E361)
function actualWorld() {
    const pool = new SidPool(CAPACITY, { server: false });
    const types = formTypeInputs();
    types[128].group = 1;
    for (const number of [80, 128, 71]) { types[number].footX = 1; types[number].footY = 1; }
    const hash = new SquidHash();
    const spots = new Uint8Array(65536);
    const book = new SquidPostPopState();
    book.suppressed = true;
    const owner = new SquidOwner(pool, types, book, new SquidOwnerMode());
    const postPop = new SquidPostPop(pool, types, book);
    const pop = new SquidPop(pool, hash, spots, { postPop });
    const unpop = new SquidUnpop(pool, hash, spots);
    const factory = new SquidFactory(pool, types, { unpop });
    const destroy = new RawSquidDestroy(pool, unpop, types);
    const kernel = new Kernel();
    const clock = new SquidProcessState();
    clock.now = 10;
    const firing = factory.create(128, 2);
    const target = factory.create(71, 2);
    owner.set(firing, 1);
    owner.set(target, 2);
    put(pool.allocated(firing), 14, 0x42480000);
    put(pool.allocated(firing), 18, 0x42480000);
    put(pool.allocated(target), 14, 0x42480000);
    put(pool.allocated(target), 18, 0x42700000);
    const deletion = new SquidDeletionState();
    deletion.unitLostSuppressed1 = true;
    let host = null;
    const lifecycle = new SquidDestroyLifecycle(pool, types, book, deletion, destroy, {
        emit: () => {},
        forward: (event) => { if (host) host.hooks().emit(event); else throw new Error('Missile 수명 검사 Host 미설정'); },
    });
    host = new SquidProcessHost(pool, types, kernel, destroy, clock, { asset: lifecycle.hooks() });
    return { pool, types, hash, spots, book, owner, pop, unpop, factory, destroy, kernel, clock, host, firing, target };
}

test('Missile 실제 공장: 자산 생성·소유자·공간 Pop·flags 0x10 Kernel 부착·범위 종료 한 프레임 뒤 공통 삭제 수명을 같은 풀에서 지킨다(Missile_ActualFactorySpaceKernelAndDelete)', () => {
    const world = actualWorld();
    const { pool, types, pop, unpop, owner, factory, destroy, kernel, clock, host, firing, target } = world;
    const state = new MissileState({ now: 10 });
    const missileTypes = types.map(() => ({ range: 0, frameCount: 0, firingSound: 0, impactSound: 0, damagePerSecond: 0, specialFlyerDamage: false, flyerDamagePerSecond: 0 }));
    missileTypes[128].range = 10;
    const hitTypes = types.map(() => ({ damagePerSecond: 0 }));
    let refreshed = 0;
    let moves = 0;
    const gunHit = new RawGunHit(pool, types, hitTypes, {
        notifySurface: (sid) => { assert.equal(sid, firing); refreshed += 1; },
        damage: () => { throw new Error('범위 종료 검사에 직접 피해가 발생함'); },
    });
    const hooks = {
        muzzle: (sid) => { const raw = pool.slot(sid); return [bitsToFloat(get(raw, 14)), bitsToFloat(get(raw, 18))]; },
        hotspot: () => [0, 0],
        targetDimensions: () => [1, 1],
        rayEnd: (from, to) => [from[0], to[1]],
        owner: (sid, player) => { owner.set(sid, player); },
        unpop: (sid, flags) => { unpop.unpop(sid, types[pool.slot(sid)[10]], flags); },
        pop: (sid, x, y, flags) => {
            assert.equal(pop.pop(sid, types[pool.slot(sid)[10]], 1, 1, x, y, flags), RAW_POP_RESULT.registered);
            moves += 1;
        },
        center: () => [49.5, 49.5],
        targetBounds: (sid, point) => [point[0] - 1, point[1] - 1, point[0], point[1]],
        hitEffect: () => { throw new Error('범위 종료 검사에 충돌 효과가 발생함'); },
        destroy: (sid, flags) => { destroy.destroy(sid, flags, host.hooks()); },
    };
    const logic = new RawMissile(pool, types, missileTypes, state, makeMissileFactoryHooks(pool, factory, makeMissileGunHitHooks(gunHit, state, hooks)));
    try {
        const missile = createMissile(host, logic, request(firing, target));
        assert.ok(missile instanceof MissileProcess);
        const asset = missile.parent;
        const form = missile.form;
        assert.deepEqual([kernel.size, host.fromForm(form) === missile, missile.fields.firing, missile.fields.target], [1, true, firing, target]);
        // 표시 자산은 발사자 소유자(1)로 만들어지고 free/dead/void 가 아니며, Form 이 그 종속 head 에 붙는다. Pop 은 한 번 실행됐다.
        assert.deepEqual([pool.slot(asset)[34], pool.slot(asset)[11] & 7, get(pool.slot(asset), 6, 2), moves], [1, 0, form, 1]);
        state.now = 20;
        clock.now = 20;
        kernel.runFrame();
        // 사거리 종료: Kernel 에서 빠지고 표시 자산과 Form 이 삭제되며 총 사거리 갱신 1회·Pop 한 번 더, 삭제기 깊이는 0 이다.
        assert.equal(kernel.size, 0);
        assert.ok(pool.slot(asset)[11] & 3);
        assert.ok(pool.slot(form)[11] & 3);
        assert.deepEqual([refreshed, moves, destroy.preDepth, destroy.postDepth], [1, 2, 0, 0]);
    } finally {
        kernel.clear();
    }
});

test('Missile 공장 계약: makeMissileFactoryHooks 는 create 만 같은 풀의 실제 공장으로 바꾸고 나머지 훅과 원래 묶음은 보존하며 풀이 다르면 거부한다', () => {
    const pool = new SidPool(CAPACITY, { server: false });
    const types = formTypeInputs();
    const factory = new SquidFactory(pool, types);
    const created = [];
    const original = { create: () => 999, destroy: () => 'kept', pop: () => 'pop' };
    const hooks = makeMissileFactoryHooks(pool, factory, original);
    assert.notEqual(hooks, original);
    assert.equal(original.create(), 999);
    assert.notEqual(hooks.create, original.create);
    assert.deepEqual([hooks.destroy(), hooks.pop()], ['kept', 'pop']);
    assert.deepEqual(Object.keys(hooks).sort(), ['create', 'destroy', 'pop']);
    // 훅이 없어도 create 하나만 생긴다.
    assert.deepEqual(Object.keys(makeMissileFactoryHooks(pool, factory, {})), ['create']);
    // create(type, flags)는 실제 공장의 create 로 전달되어 새 SID 를 돌려준다.
    const first = pool.freeCount;
    const sid = hooks.create(71, 2);
    created.push(sid);
    assert.equal(pool.slot(sid)[10], 71);
    assert.equal(pool.freeCount, first - 1);
    // 풀이 다르면 TypeError(풀 불일치)이다.
    assert.throws(() => makeMissileFactoryHooks(new SidPool(CAPACITY, { server: false }), factory, {}), (error) => error instanceof TypeError && /풀 불일치/.test(error.message));
});

// 부착 인자를 기록하는 가짜 Host 와 구성 순서를 기록하는 로직이다(생성 전/후 구분). (2026-11-13 WB04-E361)
function fakeParts(pool, attached = true) {
    const log = [];
    // 생성자에 넘어온 요청 객체를 그대로(같은 객체로) 보관한다.
    const requests = [];
    const logic = {
        pool,
        processType: 28,
        construct: (fields, req) => { requests.push(req); log.push(['construct', req.firing, req.target]); return 77; },
    };
    const host = {
        pool,
        attach: (process, type, parent, flags) => {
            log.push(['attach', process instanceof MissileProcess, type, parent, flags]);
            return attached ? process : null;
        },
    };
    return { log, logic, host, requests };
}

test('Missile 공장 계약: createMissile 은 풀이 다르면 생성 전에 거부하고 TYPE(processType)·표시 자산·flags 0x10 으로 부착하며 부착 실패는 null 이다', () => {
    const pool = new SidPool(CAPACITY, { server: false });
    const { log, logic, host, requests } = fakeParts(pool);
    const sent = { firing: 5, target: 6 };
    const process = createMissile(host, logic, sent);
    assert.ok(process instanceof MissileProcess);
    assert.equal(requests[0], sent);
    assert.deepEqual(log, [['construct', 5, 6], ['attach', true, 28, 77, 0x10]]);
    // 로직의 processType 이 바뀌면 그 값을 쓴다.
    logic.processType = 41;
    createMissile(host, logic, { firing: 1, target: 2 });
    assert.deepEqual(log.at(-1), ['attach', true, 41, 77, 0x10]);
    // 부착이 없거나 MissileProcess 가 아니면 null 이다.
    const none = fakeParts(pool, false);
    assert.equal(createMissile(none.host, none.logic, { firing: 1, target: 2 }), null);
    const other = fakeParts(pool);
    other.host.attach = () => ({});
    assert.equal(createMissile(other.host, other.logic, { firing: 1, target: 2 }), null);
    // 풀이 다르면 생성(construct)도 부르기 전에 TypeError 이다.
    const mismatch = fakeParts(pool);
    mismatch.host.pool = new SidPool(CAPACITY, { server: false });
    assert.throws(() => createMissile(mismatch.host, mismatch.logic, { firing: 1, target: 2 }), (error) => error instanceof TypeError && /풀 불일치/.test(error.message));
    assert.deepEqual(mismatch.log, []);
});

test('Missile 공장 계약: makeGunMissileHooks 는 createProjectile 만 교체해 같은 Host 의 Missile 을 만들고 부착 실패는 거부하며 풀이 다르면 거부한다', () => {
    const pool = new SidPool(CAPACITY, { server: false });
    const { log, logic, host, requests } = fakeParts(pool);
    const original = { createProjectile: () => 'old', sound: () => 'kept' };
    const hooks = makeGunMissileHooks(host, logic, original);
    assert.notEqual(hooks, original);
    assert.equal(original.createProjectile(), 'old');
    assert.equal(hooks.sound(), 'kept');
    assert.deepEqual(Object.keys(hooks).sort(), ['createProjectile', 'sound']);
    assert.deepEqual(Object.keys(makeGunMissileHooks(host, logic, {})), ['createProjectile']);
    const sent = { firing: 5, target: 6 };
    hooks.createProjectile(sent);
    assert.deepEqual(log, [['construct', 5, 6], ['attach', true, 28, 77, 0x10]]);
    // 요청은 가공하지 않고 같은 객체로 생성자에 전달된다.
    assert.equal(requests[0], sent);
    // 부착에 실패하면(생성 자산 삭제 중) 거부한다.
    const none = fakeParts(pool, false);
    assert.throws(() => makeGunMissileHooks(none.host, none.logic, {}).createProjectile({ firing: 1, target: 2 }), (error) => error instanceof Error && /부착 실패/.test(error.message));
    // 풀이 다르면 훅을 만들 때 거부한다.
    const other = new SidPool(CAPACITY, { server: false });
    assert.throws(() => makeGunMissileHooks({ ...host, pool: other }, logic, {}), (error) => error instanceof TypeError && /풀 불일치/.test(error.message));
});

test('MissileProcess: Kernel 이 지울 때(destroy)에만 진행 중인 프레임의 자기 삭제 관찰을 끈다', () => {
    const pool = new SidPool(CAPACITY, { server: false });
    const seen = [];
    const logic = { frame: (fields, alive) => { seen.push(alive()); } };
    const process = new MissileProcess(logic, new MissileFields());
    process.runFrame();
    assert.deepEqual(seen, [true]);
    // dispose 와 destroy 는 같은 효과이고, 한 번 꺼지면 이후 프레임의 관찰은 거짓이다.
    process.destroy();
    process.runFrame();
    assert.deepEqual(seen, [true, false]);
    const second = new MissileProcess(logic, new MissileFields());
    second.dispose();
    second.runFrame();
    assert.deepEqual(seen, [true, false, false]);
    // 실제 Kernel 의 remove 가 destroy 를 불러 같은 효과를 낸다.
    const kernel = new Kernel();
    const third = new MissileProcess(logic, new MissileFields());
    const id = kernel.add(third);
    kernel.remove(id);
    third.runFrame();
    assert.equal(seen.at(-1), false);
    assert.equal(pool.freeCount > 0, true);
});
