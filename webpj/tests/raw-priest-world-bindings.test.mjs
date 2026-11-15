// cpppj RawPriestWorldBindingsTests의 PATCH 조립 계약을 같은 원문 기대값으로 대조한다.
// 이동 선택 원본 376행은 path-start.test.mjs가 이미 연결했으며 여기서는 실제 월드 소비자를 검사한다. (2026-11-16 WB04-E563)
import test from 'node:test';
import assert from 'node:assert/strict';
import { PriestWorld, PRIEST, FIELD, unreachable } from './priest-world-support.mjs';
import { get, put, bitsToFloat, floatToBits } from './raw-support.mjs';
import { RawPriestWorldBindings } from '../www/core/raw-priest-world-bindings.js';
import { RawAuthorityState } from '../www/core/raw-authority-state.js';
import { SquidFormDispatch } from '../www/core/squid-form-dispatch.js';
import { PRIEST_REGEN_EVENT } from '../www/core/raw-priest-post-pop.js';
import { PRIEST_FALL_EVENT } from '../www/core/raw-priest-fall.js';

// cpppj _LoadsFullHealthAndRunsActualRegular의 PATCH 분기다.
test('RawPriestWorldBindings: fullHP 로딩·재Pop·실제 Regular 예약과 SP 회전', () => {
    const world = new PriestWorld(); const sid = world.add(); const body = world.bindings;
    assert.equal(body.priestState.currentHitPoints(sid), 200);
    assert.deepEqual(body.priests.priests.items(), [sid]);
    assert.equal(body.priests, world.state.priests); assert.equal(world.state.priests.ownerState[1], 0);
    const regen = world.host.findEvent(sid, PRIEST_REGEN_EVENT);
    assert.equal(regen.parent, sid); assert.equal(regen.payload, 8.25); assert.equal(world.kernel.size, 1);
    assert.equal(body.forcefield.find(sid), 0); assert.equal(body.checkCarrier(sid), false);
    world.effects.unpop(sid, 0); world.pop(sid, 30, 31);
    assert.equal(world.host.findEvent(sid, PRIEST_REGEN_EVENT), regen); assert.equal(world.kernel.size, 1); assert.equal(body.priests.priests.count, 1);
    body.setLoadingDepth(0); world.clock.now = 9; world.kernel.runFrame();
    assert.equal(regen.count, 1); assert.equal(regen.time, 17.25); assert.equal(body.priestState.currentHitPoints(sid), 200);
    assert.equal(world.audits, 1); assert.equal(world.fallbackEvents, 0); assert.equal(world.book.depth, 0);
    assert.equal(world.state.regen.patch.sequence, world.state.regen.patch.observedSequence);
    assert.equal(world.store.get(1), floatToBits(1000.5));
});

// cpppj _ShieldFallAndDestructionShareWorld의 PATCH 분기다.
test('RawPriestWorldBindings: 보호막·낙하가 현재 hash0를 읽고 같은 Form/장부로 삭제된다', () => {
    const world = new PriestWorld(); const body = world.bindings; const sid = world.add(); body.setLoadingDepth(0);
    world.pool.allocated(sid)[40] |= 0x20;
    world.effects.unpop(sid, 0); world.pop(sid, 30, 31);
    const field = body.forcefield.find(sid);
    assert.ok(field); assert.equal(world.kernel.size, 3); assert.deepEqual([world.soundLoads, world.soundAttachments, world.notices], [1, 1, 1]);
    assert.equal(body.fallWalker(sid), true); assert.equal(world.falls, 1); assert.equal(world.kernel.size, 4);
    assert.equal(body.forcefield.find(sid), field); assert.equal(world.soundLoads, 1);
    const support = world.hash.cell(0, 30, 31); assert.ok(support); world.hash.setCell(0, 30, 31, 0);
    world.clock.now = 0.02; world.kernel.runFrame();
    assert.equal(get(world.pool.slot(sid), 36), 2); assert.ok(world.host.findEvent(sid, PRIEST_FALL_EVENT)); assert.equal(world.kernel.size, 4);
    world.hash.setCell(0, 30, 31, support); world.clock.now = 0.13; world.kernel.runFrame();
    assert.equal(get(world.pool.slot(sid), 36), 1); assert.equal(world.host.findEvent(sid, PRIEST_FALL_EVENT), null); assert.equal(world.kernel.size, 3);
    assert.equal(world.dispatch.destroy(sid, 0), true);
    assert.ok(world.pool.slot(sid)[11] & 1); assert.ok(world.pool.slot(field)[11] & 1);
    assert.equal(world.kernel.size, 0); assert.equal(body.priests.priests.count, 0); assert.equal(world.book.depth, 0);
    assert.equal(world.destroy.preDepth, 0); assert.equal(world.destroy.postDepth, 0); assert.equal(world.fallbackEvents, 0);
});

// cpppj _RejectsWrongBindingAndUnknownPriestEvent의 PATCH 분기다.
test('RawPriestWorldBindings: 표면 사본·다른 풀·중복 연결·미복원 고유 사건 거부', () => {
    const world = new PriestWorld(); const other = new PriestWorld(); const body = world.bindings;
    assert.throws(() => new RawPriestWorldBindings({ ...world.core, surfaces: world.hash.entries(0) }, world.state, world.effects));
    // 지연 연결 세 종류의 다른 풀과 이미 연결한 같은 풀을 각각 거부한다.
    for (const [method, value, foreign] of [['bindProcesses', world.host, other.host], ['bindDamageable', world.damageable, other.damageable], ['bindPlacement', world.placement, other.placement]]) {
        assert.throws(() => body[method](foreign)); assert.throws(() => body[method](value));
    }
    const sid = world.add(); assert.throws(() => body.regularHandler()(sid, 0x25c, 0, 1));
    assert.equal(body.regularHandler((parent, event, count, payload) => {
        assert.deepEqual([parent, event, count], [sid, 0x25c, 3]); return payload;
    })(sid, 0x25c, 3, 2.5), 2.5);
    body.validateReady(); assert.equal(body.priests.priests.count, 1); assert.equal(world.kernel.size, 1);
});

// cpppj _SpawnUsesOriginalOwnerWordAndActualFactory의 PATCH 분기다. 포획/희생 관측을 추가하지 않는다.
test('RawPriestWorldBindings: 나선 배치·실제 생성자가 원소유자 WORD와 회복을 보존한다', () => {
    const world = new PriestWorld(); const body = world.bindings; body.setLoadingDepth(0);
    body.spawn.spawn(40, 41, PRIEST, 0x2082);
    assert.equal(body.priests.priests.count, 1);
    const sid = body.priests.priests.items()[0]; const raw = world.pool.slot(sid);
    assert.equal(raw[10], PRIEST); assert.equal(raw[34], 2); assert.equal(get(raw, 12, 2), 0x2002);
    assert.equal(bitsToFloat(get(raw, 14)), 40); assert.equal(bitsToFloat(get(raw, 18)), 41);
    assert.equal(body.priestState.currentHitPoints(sid), 200); assert.equal(body.priests.ownerState[2], 0);
    assert.ok(world.host.findEvent(sid, PRIEST_REGEN_EVENT)); assert.equal(world.kernel.size, 1); assert.equal(world.book.depth, 0);
});

// cpppj _HealingCrossesHalfHealthAndZeroDamageEffectReturns의 PATCH 분기다.
test('RawPriestWorldBindings: 절반 HP 회복·보호막 정리·현재 damageEffect 재조회', () => {
    const world = new PriestWorld(); const sid = world.add(); const body = world.bindings; body.setLoadingDepth(0);
    world.state.hitPoints.recoveryAllowed[1] = true; body.setHitPoints(sid, 90);
    const field = body.forcefield.find(sid); assert.ok(field); assert.equal(body.priestState.immobile(sid), true); assert.equal(world.kernel.size, 3);
    world.clock.now = 9; world.kernel.runFrame();
    assert.equal(body.priestState.currentHitPoints(sid), 123); assert.equal(body.priestState.immobile(sid), false);
    assert.equal(body.forcefield.find(sid), 0); assert.ok(world.pool.slot(field)[11] & 1);
    assert.equal(world.kernel.size, 1); assert.equal(world.healthRequests, 0); assert.equal(world.book.depth, 0);
    world.types[PRIEST].damageEffect = 1; body.damageableRefresh(sid); assert.equal(world.healthRequests, 1);
    world.types[PRIEST].damageEffect = 0; body.damageableRefresh(sid); assert.equal(world.healthRequests, 1);
});

// cpppj _SharedAuthorityAliasesAllLeavesAndRejectsForeignOwner의 PATCH 분기다.
test('RawPriestWorldBindings: 네 소비자가 같은 현재 boss를 빌리고 외부 권한 교체를 거부한다', () => {
    const world = new PriestWorld(); const body = world.bindings; const authority = new RawAuthorityState(); authority.server = false;
    body.bindAuthority(authority); body.bindAuthority(authority);
    const state = world.state;
    assert.equal(state.hitPoints.sharedAuthority, state.fall.sharedAuthority); assert.equal(state.fall.sharedAuthority, state.carrier.sharedBoss);
    assert.throws(() => body.bindAuthority(new RawAuthorityState())); assert.throws(() => body.setAuthority(false)); assert.equal(authority.boss, true); assert.equal(authority.server, false);
    const sid = world.add(); authority.boss = false;
    assert.equal(state.contained.currentBoss, false); assert.equal(state.carrier.isBoss(), false); assert.equal(state.fall.isAuthority(), false);
    body.setHitPoints(sid, 1); assert.equal(body.priestState.currentHitPoints(sid), 100);
    authority.boss = true; body.setHitPoints(sid, 1); assert.equal(body.priestState.currentHitPoints(sid), 1);
});

// cpppj _PlayerRecoveryRejectsForeignRecords의 PATCH 분기다.
test('RawPriestWorldBindings: Player9 대여의 폭·물리 주소와 실패 후 기존 대여 보존', () => {
    const world = new PriestWorld(); const body = world.bindings; const records = new Uint8Array(9 * 0xb4);
    assert.throws(() => body.bindPlayerRecords(new Uint8Array(0))); assert.equal(world.state.hitPoints.playerRecords.length, 0);
    body.bindPlayerRecords(records); body.bindPlayerRecords(records.subarray());
    assert.throws(() => body.bindPlayerRecords(records.slice())); assert.throws(() => body.bindPlayerRecords(records.subarray(1)));
    assert.equal(world.state.hitPoints.playerRecords.buffer, records.buffer); assert.equal(world.state.hitPoints.playerRecords.length, records.length);
});

// cpppj _PlayerRecoveryRunsActualRegenAndShieldCleanup의 PATCH 분기다.
test('RawPriestWorldBindings: 현재 신전 DWORD를 실제 회복과 보호막 삭제가 공유한다', () => {
    const world = new PriestWorld(); const body = world.bindings; const players = new Uint8Array(9 * 0xb4); body.bindPlayerRecords(players);
    const sid = world.add(); body.setLoadingDepth(0); world.state.hitPoints.recoveryAllowed[1] = true; body.setHitPoints(sid, 90);
    const field = body.forcefield.find(sid); assert.ok(field); assert.equal(world.kernel.size, 3);
    body.setHitPoints(sid, 200); assert.equal(body.priestState.currentHitPoints(sid), 90); assert.equal(body.forcefield.find(sid), field);
    put(players, 0xb4 + 0x7c, 0x10000); world.state.hitPoints.recoveryAllowed[1] = false; world.clock.now = 9; world.kernel.runFrame();
    assert.equal(body.priestState.currentHitPoints(sid), 123); assert.equal(body.priestState.immobile(sid), false); assert.equal(body.forcefield.find(sid), 0);
    assert.ok(world.pool.slot(field)[11] & 1); assert.equal(world.kernel.size, 1); assert.equal(world.healthRequests, 0); assert.equal(world.book.depth, 0);
    assert.equal(world.destroy.preDepth, 0); assert.equal(world.destroy.postDepth, 0);
});

// 웹 추가 계약: 구성 실패는 아직 외부 postPop에 가상 지원을 등록하지 않아야 한다.
test('RawPriestWorldBindings: 필수 효과·같은 풀·장기 몸체 검증과 지연 장치의 명시 거부', () => {
    const world = new PriestWorld({ bind: false }); const other = new PriestWorld({ bind: false });
    // 각 필수 효과와 실제 풀 참조 하나씩을 빠뜨려 부분 등록을 막는다.
    for (const name of ['pop', 'unpop', 'repop', 'destroy', 'claimGround', 'ownerNotice', 'notifySurface', 'dependentFound', 'reserveRegular', 'fallSound']) {
        assert.throws(() => new RawPriestWorldBindings(world.core, world.state, { ...world.effects, [name]: null }));
    }
    for (const name of ['reward', 'factory', 'owner', 'postPop', 'frame']) assert.throws(() => new RawPriestWorldBindings({ ...world.core, [name]: other.core[name] }, world.state, world.effects));
    for (const name of ['sharedForcefield', 'sharedCarrier', 'sharedPriestPre', 'sharedForcefieldRegular']) assert.throws(() => new RawPriestWorldBindings(world.core, world.state, { ...world.effects, [name]: { pool: other.pool } }));
    const bare = new RawPriestWorldBindings(world.core, world.state, { ...world.effects, shield: {}, regen: {} });
    assert.throws(() => bare.validateReady(), /ProcessHost/);
    bare.bindProcesses({ pool: world.pool }); assert.throws(() => bare.validateReady(), /Damageable/);
    bare.bindDamageable({ pool: world.pool }); assert.throws(() => bare.validateReady(), /배치/);
    bare.bindPlacement({ pool: world.pool }); bare.validateReady();
    assert.throws(() => bare.uniqueRegular(0, 0x25c, 0, 0));
    assert.throws(() => bare.setLocalPlayer(9)); assert.throws(() => bare.setLocalPlayer(-1));
});

// 웹 추가 계약: Native 저장소 설치 전에도 명시 장기 몸체 대여와 destructor 등록 반납 계약을 검사한다.
test('RawPriestWorldBindings: 같은 장기 몸체 대여·소유자 우회·TYPE getter·정리 수명', () => {
    const world = new PriestWorld(); const original = world.bindings;
    const effects = { ...world.effects, sharedForcefield: original.forcefield, sharedCarrier: original.carrier,
        sharedPriestPre: { pool: world.pool, handles: () => false, preDestroy: unreachable },
        sharedForcefieldRegular: { pool: world.pool, handle: (sid, event, count, value) => value + 1 },
        nonPriestOwner: (sid, player) => { world.owner.set(sid, player); world.ownerNotices += 1; } };
    const body = new RawPriestWorldBindings(world.core, world.state, effects);
    assert.equal(body.forcefield, original.forcefield); assert.equal(body.carrier, original.carrier);
    const field = world.factory.create(FIELD); body.setOwner(field, 3); assert.equal(world.pool.slot(field)[34], 3); assert.equal(world.ownerNotices, 1);
    assert.equal(body.regularHandler()(field, 1, 2, 3), 4);
    world.state.movement.priest = 99; assert.equal(world.state.forcefieldRegular.currentPriestType(), 99); assert.equal(body.movementProcessType(99), 54);
    body.bindPlayerRecords(new Uint8Array(9 * 0xb4)); body.dispose();
    assert.equal(world.state.hitPoints.playerRecords.length, 0);
    // 새 분배기에서 사제 preDestroy 지원 반납을 실제 다음 Pop의 오류로 확인한다.
    const priest = world.factory.create(PRIEST); world.owner.set(priest, 1);
    assert.throws(() => world.pop(priest, 30, 31), /후처리|미복원/);
    assert.ok(new SquidFormDispatch(world.pool, world.destroy, world.common.hooks()));
});

// 웹 추가 계약: 긴 Client 상태를 다음 Session이 빌려도 같은 원본 권한 주소를 유지한다.
test('RawPriestWorldBindings: 다음 Session이 같은 장기 권한/상태를 다시 대여한다', () => {
    const world = new PriestWorld(); const authority = new RawAuthorityState(); world.bindings.bindAuthority(authority);
    const firstRead = world.state.hitPoints.sharedAuthority; const firstView = world.state.contained.bossOwner;
    world.bindings.dispose();
    const next = new RawPriestWorldBindings(world.core, world.state, world.effects); next.bindAuthority(authority);
    assert.equal(world.state.hitPoints.sharedAuthority, firstRead); assert.equal(world.state.contained.bossOwner, firstView);
    authority.boss = false; assert.equal(world.state.hitPoints.isAuthority(), false); assert.equal(world.state.contained.currentBoss, false);
    // 외부가 한 소비자만 다른 권한으로 바꾼 상태를 임의로 덮지 않는다.
    world.state.contained.bindBoss({ value: true }); assert.throws(() => next.bindAuthority(authority));
    assert.throws(() => next.setAuthority(true)); next.dispose();
});
