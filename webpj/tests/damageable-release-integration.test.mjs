// 종속 해방 raw-damageable-release.js(WB04-E252)를 앞 단위가 미뤄 둔 두 합성 장면에 그대로 조립해 대조한다.
//  1) cpppj `priest_spawn_release_uses_real_factory_and_priest_owner`(E68 이 "해방 파일 이식 때"로 미룸): 종속 해방 → 사제 나선 생성(raw-priest-spawn.js) → 실제 서버 SID/사제 생성자(SquidFactory) → 실제 사제 소유자
//     (RawPriestOwner·SquidOwner)까지 한 풀에서 잇는다. 생성 SID 15000·WORD 0x8181(로컬 표식 비트 7)·소유자 1·후보 WORD 불변·Pop/Carrier 검사/알림 각 1회를 본다.
//  2) cpppj `priest_predestroy_cleans_real_regen_form_kernel_and_common_bookkeeping`(E63 이 "Damageable/Kernel 정리 통합은 해당 단위에서"로 미룸): 사제 삭제에 실제 Damageable 접두(E251)·종속 해방(E252)·Carrier·
//     공통 삭제 장부(SquidDestroyLifecycle)·회복 Form·Kernel·SID 반납을 연결한다. client/server SID 두 경우의 깊이·전파 분기를 본다. 해방 순회에서 회복 Form(TYPE 46)은 contained TYPE(6)이 아니라 제외되므로
//     해방의 생성 경계가 불리면 실패한다.
// 장면 입력(TYPE 표의 사제·보호막 필드)만 합성이고 기대값은 cpppj 시험의 단언을 1:1 로 옮긴 것이다. CD 10.72 반복은 웹 범위 밖이다. 실제 게임·브라우저 실행이 아니다. (2026-11-10 WB04-E252)
import assert from 'node:assert/strict';
import test from 'node:test';
import { Kernel } from '../www/core/kernel.js';
import { GameRandom } from '../www/core/random.js';
import { RawCarrierPreDestroy, makeCarrierPreDestroyHooks } from '../www/core/raw-carrier-pre-destroy.js';
import { ContainedFinderState } from '../www/core/raw-contained-finder.js';
import { DAMAGEABLE_SOUND, DamageablePreDestroyState, RawDamageablePreDestroy, makeDamageablePreDestroyHooks } from '../www/core/raw-damageable-pre-destroy.js';
import { DamageableReleaseState, RawDamageableRelease } from '../www/core/raw-damageable-release.js';
import { PriestForcefieldState, RawPriestForcefield, makePriestForcefieldHooks } from '../www/core/raw-priest-forcefield.js';
import { PRIEST_TYPE, PriestOwnerState, RawPriestOwner, makePriestOwnerDispatch } from '../www/core/raw-priest-owner.js';
import { PRIEST_REGEN_EVENT, PriestPostPopState, RawPriestPostPop, makePriestPostPopProcessHooks } from '../www/core/raw-priest-post-pop.js';
import { RawPriestPreDestroy, makePriestPreDestroyHooks } from '../www/core/raw-priest-pre-destroy.js';
import { PriestSpawnState, RawPriestSpawn, makePriestSpawnHooks } from '../www/core/raw-priest-spawn.js';
import { SidPool } from '../www/core/sid-pool.js';
import { ScrambledSpStore } from '../www/core/sp-store.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { SquidDeletionState, SquidDestroyLifecycle } from '../www/core/squid-destroy-lifecycle.js';
import { RawSquidDestroy, SQUID_DESTROY_EFFECT } from '../www/core/squid-destroy.js';
import { SquidFactory } from '../www/core/squid-factory.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidOwner, SquidOwnerMode } from '../www/core/squid-owner.js';
import { SquidPostPopState } from '../www/core/squid-postpop-state.js';
import { SquidPostPop } from '../www/core/squid-postpop.js';
import { SquidProcessHost, SquidProcessState } from '../www/core/squid-process.js';
import { SquidReward, SquidRewardState } from '../www/core/squid-reward.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { floatToBits, get, put } from './raw-support.mjs';

// 공간/파생 보호막 효과를 대체하는 합성 자산의 원본 TYPE 번호와 분석 도구의 합성 풀 크기이다. (2026-11-10 WB04-E252)
const FORCEFIELD_TYPE = 167;
const CAPACITY = 24000;

// 내장 Form 네 생성자 주소만 넣은 188칸 TYPE 입력이다(cpppj MakeFormTypeInputs; 나머지 필드는 0). (2026-11-10 WB04-E252)
const formTypeInputs = () => Array.from({ length: 188 }, (_, number) => ({
    cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0, group: 0, ...([1, 2, 3, 6].includes(number) ? { constructorAddress: typeConstructorAddress(number) } : {}),
}));

// 사제 TYPE 의 생성자·분류·HP·발자국을 원본 값으로 채운다. (2026-11-10 WB04-E252)
function priestType(types) {
    Object.assign(types[PRIEST_TYPE], { constructorAddress: typeConstructorAddress(PRIEST_TYPE), flags1: 0x69012, flags2: 0x210000, maxHitPoints: 200, footX: 1, footY: 1 });
    return types[PRIEST_TYPE];
}

// 종속 해방→나선→실제 서버 SID/사제 생성자→실제 사제 소유자까지 연결한다.
test('damageable-release 합성: 해방이 사제 나선 생성·실제 Factory·사제 소유자까지 이어져 WORD 0x8181 과 소유자 1 을 쓴다', () => {
    const pool = new SidPool(CAPACITY, { server: true });
    const freeBefore = pool.freeCount;
    const types = Array.from({ length: 188 }, () => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0 }));
    priestType(types);
    const factory = new SquidFactory(pool, types);
    const parent = factory.create(PRIEST_TYPE, 2);
    const contained = pool.allocate(2);
    const parentRaw = pool.allocated(parent);
    const childRaw = pool.allocated(contained);
    put(parentRaw, 14, floatToBits(20.75));
    put(parentRaw, 18, floatToBits(21.9));
    put(parentRaw, 6, contained, 2);
    childRaw[10] = 6;
    childRaw[11] = 8;
    put(childRaw, 4, 0, 2);
    put(childRaw, 12, 0x8101, 2);
    put(childRaw, 18, PRIEST_TYPE, 2);
    put(childRaw, 20, 0x20, 2);
    const book = new SquidPostPopState();
    const mode = new SquidOwnerMode();
    mode.battle = true;
    const owner = new SquidOwner(pool, types, book, mode);
    const ownerState = new PriestOwnerState({ battlePlayer: 1 });
    const counts = { notifications: 0, pops: 0, carrierChecks: 0 };
    let born = 0;
    const priestOwner = new RawPriestOwner(pool, mode, ownerState, { base: (sid, player) => owner.set(sid, player), notify: () => { counts.notifications += 1; } });
    const spots = new Uint8Array(65536);
    // 일반 Pop/가상 Carrier 검사는 기록 경계이며 실제 생성·소유자 쓰기를 검사한다.
    const spawn = new RawPriestSpawn(pool, types, spots, new PriestSpawnState(), {
        mayPlace: () => assert.fail('spot 이 0 이면 배치 검사를 부르지 않는다'),
        create: (type, flags) => { assert.ok(type === PRIEST_TYPE && flags === 0); born = factory.create(type, flags); return born; },
        setOwner: makePriestOwnerDispatch(priestOwner, (sid, player) => owner.set(sid, player)),
        pop: (sid, x, y, flags) => { assert.ok(sid === born && x === 21 && y === 22 && flags === 0 && (pool.slot(sid)[11] & 4)); counts.pops += 1; },
        notifySurface: () => assert.fail('사제는 다리 TYPE 이 아니다'),
        checkCarrier: (sid) => { assert.equal(sid, born); counts.carrierChecks += 1; },
    });
    const releaseState = new DamageableReleaseState();
    releaseState.battle = true;
    const release = new RawDamageableRelease(pool, types, spots, new ContainedFinderState(), releaseState, makePriestSpawnHooks(pool, spawn, {
        spawnPriest: () => assert.fail('연결한 나선 생성으로 바뀌어야 한다'),
        create: () => assert.fail('사제 후보는 일반 생성 경계를 쓰지 않는다'),
        setOwner: () => assert.fail('사제 후보는 일반 소유자 경계를 쓰지 않는다'),
        pop: () => assert.fail('사제 후보는 일반 Pop 경계를 쓰지 않는다'),
        notifySurface: () => assert.fail('사제 후보는 표면 알림을 쓰지 않는다'),
    }));
    release.release(parent);
    assert.equal(born, 15000);
    assert.equal(pool.freeCount, freeBefore - 3);
    assert.ok(priestOwner.handles(born));
    assert.equal(get(pool.slot(born), 12, 2), 0x8181);
    assert.equal(pool.slot(born)[34], 1);
    assert.equal(get(childRaw, 12, 2), 0x8101);
    assert.deepEqual(counts, { notifications: 1, pops: 1, carrierChecks: 1 });
});

// void 자산의 삭제에 실제 Damageable 접두·종속 해방·Carrier·장부·종속 form·Kernel·SID 반납을 연결한다.
// server/client SID 의 깊이/전파 분기를 각각 검사한다.
for (const server of [false, true]) {
    test(`damageable-release 합성: 사제 삭제가 실제 Damageable 접두·해방·Carrier·장부·회복 Form·Kernel 을 정리한다(${server ? 'server' : 'client'} SID)`, () => {
        const pool = new SidPool(32768, { server });
        const initialFree = pool.freeCount;
        const types = formTypeInputs();
        Object.assign(priestType(types), { cost: 123 });
        Object.assign(types[FORCEFIELD_TYPE], { cost: 20, footX: 1, footY: 1 });
        const factory = new SquidFactory(pool, types);
        const root = factory.create(PRIEST_TYPE, server ? 0 : 2);
        const child = pool.allocate(server ? 0 : 2);
        pool.allocated(child)[10] = FORCEFIELD_TYPE;
        pool.allocated(root)[34] = 1;
        pool.allocated(child)[34] = 1;
        // 일반 Pop 미복원 범위는 유지하며 합성 void 자산의 실제 공간 조회 입력을 등록한다.
        for (const sid of [root, child]) {
            put(pool.allocated(sid), 14, floatToBits(20.75));
            put(pool.allocated(sid), 18, floatToBits(21.9));
        }
        const book = new SquidPostPopState();
        book.localOwner = 1;
        book.totalCost = 10;
        book.depth = 2;
        const post = new SquidPostPop(pool, types, book);
        post.postPopBase(root, 1);
        post.postPopBase(child, 1);
        assert.ok(book.totalCost === 153 && book.globalCounts()[PRIEST_TYPE] === 1 && book.globalCounts()[FORCEFIELD_TYPE] === 1);
        const hash = new SquidHash();
        const spots = new Uint8Array(65536);
        const unpop = new SquidUnpop(pool, hash, spots);
        const destroy = new RawSquidDestroy(pool, unpop, types);
        const deletion = new SquidDeletionState();
        const counts = { transmitted: 0, forceRequests: 0, carrierCalls: 0, dependentEffects: 0, collapsed: 0, sounds: 0, releases: 0 };
        const lifecycle = new SquidDestroyLifecycle(pool, types, book, deletion, destroy, {
            emit: () => {},
            forward: (event) => { assert.equal(event.effect, SQUID_DESTROY_EFFECT.transmit); counts.transmitted += 1; },
        });
        hash.setBucket(1, 20.75, 21.9, child);
        const lookup = new RawPriestForcefield(pool, hash, types, new PriestForcefieldState());
        const priests = new PriestPostPopState();
        priests.priests.entries = [0, 0, 0];
        let host = null;
        const carrierState = new ContainedFinderState();
        // 실제 해방 순회에서 타입 46 회복 form 은 제외된다. 생성 경계가 불리면 실패한다.
        const release = new RawDamageableRelease(pool, types, spots, carrierState, new DamageableReleaseState(), {
            spawnPriest: () => assert.fail('해방할 사제 후보가 없다'),
            create: () => assert.fail('해방할 일반 후보가 없다'),
            setOwner: () => assert.fail('해방할 일반 후보가 없다'),
            pop: () => assert.fail('해방할 일반 후보가 없다'),
            notifySurface: () => assert.fail('해방할 일반 후보가 없다'),
        });
        const damageable = new RawDamageablePreDestroy(pool, spots, carrierState, new DamageablePreDestroyState(), {
            collapse: (sid) => { assert.equal(sid, root); counts.collapsed += 1; },
            explosion: () => assert.fail('폭발 flags 가 없다'),
            sound: (event) => {
                assert.ok(event.sound === DAMAGEABLE_SOUND.collapse && event.x === Math.fround(20.75) && event.y === Math.fround(21.9));
                counts.sounds += 1;
            },
            releaseContained: (sid) => { assert.equal(sid, root); counts.releases += 1; release.release(sid); },
            validateBase: (sid, flags) => { lifecycle.validatePre(sid, flags); lifecycle.validatePost(sid, flags); },
            base: (sid, flags) => {
                assert.ok(sid === root && (pool.slot(child)[11] & 1) && destroy.preDepth === 1);
                counts.carrierCalls += 1;
                // 해방 순회와 공통 장부/깊이는 실제 몸체이며 파편/소리 출력은 기록 경계다.
                lifecycle.preDestroy(sid, flags);
            },
        });
        const carrier = new RawCarrierPreDestroy(pool, carrierState, makeDamageablePreDestroyHooks(pool, damageable, { dependentFound: () => { counts.dependentEffects += 1; } }));
        const pre = new RawPriestPreDestroy(pool, priests, makeCarrierPreDestroyHooks(pool, carrier, makePriestForcefieldHooks(pool, lookup, {
            destroyForcefield: (sid, flags) => {
                assert.ok(sid === child && flags === 0 && priests.priests.count === 0 && (pool.slot(root)[11] & 2));
                // 같은 dead 사제를 중첩 삭제해도 새 pre/종속/장부 효과가 생기지 않는다.
                destroy.destroy(root, 0, host.hooks());
                counts.forceRequests += 1;
                destroy.destroy(sid, flags, host.hooks());
            },
        })));
        const kernel = new Kernel();
        const processState = new SquidProcessState();
        host = new SquidProcessHost(pool, types, kernel, destroy, processState, { asset: makePriestPreDestroyHooks(pool, pre, lifecycle.hooks()) });
        const store = new ScrambledSpStore(new GameRandom(), () => 1);
        const hp = new SquidReward(pool, types, book, new SquidRewardState(), store);
        const prefix = new RawPriestPostPop(pool, hp, priests, makePriestPostPopProcessHooks(host));
        prefix.prefix(root, 1);
        const regular = host.findEvent(root, PRIEST_REGEN_EVENT);
        assert.ok(regular && kernel.size === 1 && priests.priests.count === 1);
        const form = regular.form;
        assert.equal(pool.freeCount, initialFree - 3);
        destroy.destroy(root, 0x200000, host.hooks());
        assert.deepEqual(counts, { transmitted: server ? 2 : 0, forceRequests: 1, carrierCalls: 1, dependentEffects: 0, collapsed: 1, sounds: 1, releases: 1 });
        assert.ok(priests.priests.count === 0 && kernel.size === 0);
        assert.ok((pool.slot(root)[11] & 1) && (pool.slot(child)[11] & 1) && (pool.slot(form)[11] & 1));
        assert.ok(pool.freeCount === initialFree && destroy.preDepth === 0 && destroy.postDepth === 0);
        assert.ok(book.totalCost === 10 && book.localCounts()[PRIEST_TYPE] === 0 && book.globalCounts()[PRIEST_TYPE] === 0 && book.globalCounts()[FORCEFIELD_TYPE] === 0);
        assert.ok(book.localSecondaryCounts()[PRIEST_TYPE] === 1 && !store.initialized);
    });
}
