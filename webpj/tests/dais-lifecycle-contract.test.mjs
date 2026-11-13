// 다이스·제단 수명 raw-dais-lifecycle.js 의 계약 시험이다: 생성 검사·handles/validate 경계·최초 Pop 후처리의 순서·표/폭·훅 인자·현재 값 재조회·
// 제단 21 예약·삭제 후처리의 두 경로(독립 배열/실제 Player 기록)·상태 상자·훅 공장·DaisRegularState 의 새 접근자를 합성 장면으로 직접 확인한다.
// 장면은 dais-lifecycle-support.mjs 의 lifecycleScene(실제 Frame/Host/Damageable/Carrier 부품)을 쓰고 효과는 문자열로 기록한다. 실제 게임·브라우저 실행이 아니다.
// (2026-11-13 WB04-E369: 7-P3 RawDaisLifecycle 이식으로 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import { put, get, floatToBits } from './raw-support.mjs';
import { PATCH_DAIS_VTABLE } from '../www/core/raw-dais-content-notice.js';
import { PATCH_ALTAR_VTABLE } from '../www/core/raw-altar-sacrifice.js';
import { DaisRegularState } from '../www/core/raw-dais-regular.js';
import { CarrierPostPopState } from '../www/core/raw-carrier-post-pop.js';
import { DaisLifecycleState, RawDaisLifecycle, makeDaisLifecycleHooks } from '../www/core/raw-dais-lifecycle.js';
import { lifecycleScene, LifecycleBodyEnd, STRIDE, GRANT_AT, PAIR_AT, HOME_AT } from './dais-lifecycle-support.mjs';

// 기본 입력은 관측의 가장 단순한 다이스 최초 Pop 행이다(owner 1, 로컬 1, 저장 개수 1, 권한/억제/로딩 없음). (2026-11-13 WB04-E369)
const BASE = { boss: 0, dead: 0, first: 5, flags: 0, frame: 3, initial: 0, loading: 0, local: 1, mode: 'dpp', owner: 1, profile: 0, reserve: 1, runeExtra: 0, saved: 1, second: 5, suppress: 0 };
const scene = (patch = {}, bound = false, options = {}) => lifecycleScene({ ...BASE, ...patch }, bound, options);
// 마지막 공통 경계(LifecycleBodyEnd)에서 멈추는 호출을 실행하고 효과 목록을 돌려준다. (2026-11-13 WB04-E369)
const ends = (s, fn) => { assert.throws(fn, LifecycleBodyEnd); return s.effects; };
const bits = floatToBits;

test('DaisLifecycle 생성: 같은 풀의 여섯 협력 몸체와 188칸 표만 받고 효과 훅은 생성 때 요구하지 않는다', () => {
    const a = scene();
    const b = scene();
    const pick = (o, name, fallback) => (Object.hasOwn(o, name) ? o[name] : fallback);
    const make = (o = {}) => new RawDaisLifecycle(pick(o, 'pool', a.world.pool), a.world.hash, pick(o, 'types', a.world.types), pick(o, 'footprint', a.world.footprint), pick(o, 'temple', a.world.ritual), pick(o, 'altar', a.altar),
        pick(o, 'postPop', a.carrierPost), pick(o, 'preDestroy', a.damageable), a.world.state, a.altarState, a.lifecycleState, {});
    assert.equal(make().pool, a.world.pool);
    // 협력 몸체 하나라도 다른 월드의 것이면 거부한다.
    assert.throws(() => make({ footprint: b.world.footprint }), TypeError);
    assert.throws(() => make({ temple: b.world.ritual }), TypeError);
    assert.throws(() => make({ altar: b.altar }), TypeError);
    assert.throws(() => make({ postPop: b.carrierPost }), TypeError);
    assert.throws(() => make({ preDestroy: b.damageable }), TypeError);
    assert.throws(() => make({ pool: b.world.pool }), TypeError);
    assert.throws(() => make({ pool: null }), TypeError);
    assert.throws(() => make({ types: a.world.types.slice(0, 187) }), TypeError);
    assert.throws(() => make({ types: [...a.world.types, a.world.types[0]] }), TypeError);
    assert.throws(() => make({ types: null }), TypeError);
    assert.throws(() => make({ types: { length: 188 } }), TypeError);
});

test('DaisLifecycle handles/validate: 다이스·제단 최종 가상표와 TYPE 짝만 최초 Pop 대상이고 초기 개수 첫 번호 범위를 공간 쓰기 전에 거부한다', () => {
    const s = scene();
    const { lifecycle, world } = s;
    const { slots } = world;
    assert.equal(lifecycle.handles(5), true);
    assert.equal(lifecycle.handles(7), true);
    assert.equal(lifecycle.handles(6), false);
    lifecycle.validate(5, 0);
    lifecycle.validate(7, 0);
    assert.throws(() => lifecycle.validate(6, 0), { name: 'TypeError', message: /타입\/가상표/u });
    // 다이스 TYPE 에 제단 가상표(또는 반대)를 붙이면 거부한다.
    put(slots[5], 0, PATCH_ALTAR_VTABLE);
    assert.throws(() => lifecycle.validate(5, 0), { name: 'TypeError', message: /타입\/가상표/u });
    put(slots[5], 0, PATCH_DAIS_VTABLE);
    put(slots[7], 0, PATCH_DAIS_VTABLE);
    assert.throws(() => lifecycle.validate(7, 0), { name: 'TypeError', message: /타입\/가상표/u });
    put(slots[7], 0, PATCH_ALTAR_VTABLE);
    // 다이스 가상표지만 다른 TYPE 이면 거부한다.
    slots[5][10] = 164;
    assert.throws(() => lifecycle.validate(5, 0), { name: 'TypeError', message: /타입\/가상표/u });
    slots[5][10] = 165;
    // 제단은 owner 범위/개수 검사를 하지 않고 바로 통과한다.
    slots[7][34] = 9;
    lifecycle.validate(7, 7);
    // owner 0..8 은 허용하고 9 는 거부한다.
    slots[5][34] = 8;
    lifecycle.validate(5, 0);
    slots[5][34] = 0;
    lifecycle.validate(5, 0);
    slots[5][34] = 9;
    assert.throws(() => lifecycle.validate(5, 0), { name: 'RangeError', message: /Player범위/u });
    slots[5][34] = 1;
    // 게임 시작 전(시각 0) 최초 생성의 저장 개수 3 까지는 허용, 4 부터는 거부한다.
    world.state.grantFlags[1] = 3;
    lifecycle.validate(5, 1);
    world.state.grantFlags[1] = 4;
    assert.throws(() => lifecycle.validate(5, 1), { name: 'RangeError', message: /초기개수첫번호/u });
    assert.throws(() => lifecycle.validate(5, 7), { name: 'RangeError', message: /초기개수첫번호/u });
    assert.throws(() => lifecycle.validate(5, 4), { name: 'RangeError', message: /초기개수첫번호/u });
    // flags 의 하위 3비트가 없거나, 시각이 0 이 아니거나, 시간 억제 중이면 개수를 보지 않는다.
    lifecycle.validate(5, 0);
    lifecycle.validate(5, 8);
    s.lifecycleState.gameTime = 1;
    lifecycle.validate(5, 7);
    s.lifecycleState.gameTime = 0;
    world.runeState.suppressDuration = true;
    lifecycle.validate(5, 7);
});

test('DaisLifecycleState: 독립 게임 시각·Player 아홉 칸과 { value } 상자 대여', () => {
    const carrier = new CarrierPostPopState();
    const state = new DaisLifecycleState(carrier, 12.5);
    assert.equal(state.loading, carrier);
    assert.equal(state.gameTime, 12.5);
    assert.equal(state.players.length, 9);
    state.players.forEach((player) => assert.deepEqual(player, { first: 0, firstMirror: 0, second: 0, secondMirror: 0, homeIsland: 0 }));
    // 칸은 서로 다른 객체이다.
    state.players[1].first = 7;
    assert.equal(state.players[2].first, 0);
    assert.equal(new DaisLifecycleState(carrier).gameTime, 0);
    state.gameTime = 3;
    assert.equal(state.gameTime, 3);
    const box = { value: 99 };
    state.bindGameTime(box);
    assert.equal(state.gameTime, 99);
    // 상자를 빌리면 읽기/쓰기 모두 상자를 거치고 독립 값은 건드리지 않는다.
    state.gameTime = 4;
    assert.equal(box.value, 4);
    box.value = 5;
    assert.equal(state.gameTime, 5);
    state.bindGameTime(null);
    assert.equal(state.gameTime, 3);
    assert.throws(() => state.bindGameTime({}), TypeError);
    assert.throws(() => state.bindGameTime(5), TypeError);
    assert.throws(() => state.bindGameTime('x'), TypeError);
});

test('DaisRegularState: playerRecords 접근자와 grantFlagWriter 가 묶은 기록/독립 배열의 칸을 지금 시점에 고정한다', () => {
    const state = new DaisRegularState({}, {}, null, 0);
    assert.equal(state.playerRecords, null);
    const write = state.grantFlagWriter(3);
    write(0x1ff);
    assert.equal(state.grantFlags[3], 0xff);
    assert.equal(state.grantFlags[2], 0);
    // 독립 배열을 바꿔도 이미 고정한 쓰기는 옛 배열에 쓴다.
    const old = state.grantFlags;
    state.grantFlags = new Uint8Array(9);
    write(4);
    assert.equal(old[3], 4);
    assert.equal(state.grantFlags[3], 0);
    const records = new Uint8Array(9 * STRIDE).fill(0xa5);
    state.bindPlayerRecords(records);
    assert.equal(state.playerRecords, records);
    const bound = state.grantFlagWriter(2);
    bound(0x102);
    assert.equal(records[2 * STRIDE + GRANT_AT], 2);
    assert.equal(records[2 * STRIDE + GRANT_AT - 1], 0xa5);
    assert.equal(records[2 * STRIDE + GRANT_AT + 1], 0xa5);
    // 기록을 다른 것으로 바꿔도 고정한 쓰기는 옛 기록에 쓴다.
    const other = new Uint8Array(9 * STRIDE);
    state.bindPlayerRecords(other);
    bound(9);
    assert.equal(records[2 * STRIDE + GRANT_AT], 9);
    assert.equal(other[2 * STRIDE + GRANT_AT], 0);
    state.grantFlagWriter(8)(1);
    assert.equal(other[8 * STRIDE + GRANT_AT], 1);
    assert.throws(() => state.grantFlagWriter(9), { name: 'RangeError' });
    state.bindPlayerRecords(null);
    assert.equal(state.playerRecords, null);
});

test('DaisLifecycle 다이스 Pop: 로딩 시작 전 최초 생성은 저장 개수의 다음 첫 프레임을 쓰고 표 밖은 효과 전에 거부한다', () => {
    const table = [3, 64, 120, 0];
    // 각 저장 개수의 첫 프레임이 표대로 setFrame 으로 나간다(flags 4: 쌍 등록/제단 생성 없음).
    table.forEach((first, saved) => {
        const s = scene({ saved, flags: 4, initial: 0 });
        assert.deepEqual(ends(s, () => s.lifecycle.postPop(5, 4)), [`F:5,${first},0`, 'postpop:5,4']);
    });
    // 저장 개수 4 는 첫 번호 표 밖이라 효과 없이 거부한다.
    const bad = scene({ saved: 4, flags: 4 });
    assert.throws(() => bad.lifecycle.postPop(5, 4), { name: 'RangeError', message: /로딩개수첫번호/u });
    assert.deepEqual(bad.effects, []);
    // 시각이 0 이 아니거나, 하위 3비트가 없거나, 시간 억제 중이면 첫 프레임을 쓰지 않는다.
    const later = scene({ initial: 1, flags: 4 });
    assert.deepEqual(ends(later, () => later.lifecycle.postPop(5, 4)), ['postpop:5,4']);
    const none = scene({ flags: 8 });
    assert.deepEqual(ends(none, () => none.lifecycle.postPop(5, 8)), ['postpop:5,8']);
    const suppressed = scene({ suppress: 1, flags: 4, saved: 4 });
    assert.deepEqual(ends(suppressed, () => suppressed.lifecycle.postPop(5, 4)), ['postpop:5,4']);
    assert.equal(suppressed.world.state.grantFlags[1], 4);
});

test('DaisLifecycle 다이스 Pop: 현재 개수 - 1 을 owner 의 획득 플래그 BYTE 에 쓰고 Count 앞에서 쓰기 칸을 고정한다', () => {
    // 가상 개수 1..3 은 0..2, 0 은 8비트 폭으로 255 가 된다.
    [[1, 0], [2, 1], [3, 2], [0, 255]].forEach(([count, expected]) => {
        const s = scene({ owner: 2, flags: 8, initial: 1 }, false, { lifecycleHooks: { count: () => count } });
        ends(s, () => s.lifecycle.postPop(5, 8));
        assert.equal(s.world.state.grantFlags[2], expected);
        assert.equal(s.world.state.grantFlags[1], 1);
        assert.equal(s.world.state.grantFlags[3], 1);
    });
    // 실제 Player 기록 묶음에서는 +0x71 한 바이트만 바뀐다.
    const bound = scene({ owner: 2, flags: 8, initial: 1 }, true, { lifecycleHooks: { count: () => 3 } });
    ends(bound, () => bound.lifecycle.postPop(5, 8));
    bound.records.forEach((value, index) => {
        const offset = index % STRIDE;
        const owner = Math.floor(index / STRIDE);
        assert.equal(value, owner === 2 && offset === GRANT_AT ? 2 : bound.beforeRecords[index], `바이트 ${index}`);
    });
    // 가상 Count 가 기록을 다른 것으로 바꿔도 쓰기는 Count 앞에서 고정한 옛 기록에 간다.
    const fresh = new Uint8Array(9 * STRIDE);
    const reentrant = scene({ owner: 3, flags: 8, initial: 1 }, true, {
        lifecycleHooks: { count: () => { reentrant.world.state.bindPlayerRecords(fresh); return 2; } },
    });
    ends(reentrant, () => reentrant.lifecycle.postPop(5, 8));
    assert.equal(reentrant.records[3 * STRIDE + GRANT_AT], 1);
    assert.equal(fresh[3 * STRIDE + GRANT_AT], 0);
    // 독립 배열도 같다: Count 가 배열을 바꿔도 옛 배열에 쓴다.
    const independent = scene({ owner: 3, flags: 8, initial: 1 }, false, {
        lifecycleHooks: { count: () => { independent.world.state.grantFlags = new Uint8Array(9); return 3; } },
    });
    const oldFlags = independent.world.state.grantFlags;
    ends(independent, () => independent.lifecycle.postPop(5, 8));
    assert.equal(oldFlags[3], 2);
    assert.equal(independent.world.state.grantFlags[3], 0);
    // 시간 억제 중이면 개수를 읽지도 쓰지도 않는다(Count 훅이 불리면 실패).
    const quiet = scene({ owner: 2, flags: 8, suppress: 1 }, false, { lifecycleHooks: { count: () => { throw new Error('억제 중 Count 호출'); } } });
    ends(quiet, () => quiet.lifecycle.postPop(5, 8));
    assert.equal(quiet.world.state.grantFlags[2], 1);
});

test('DaisLifecycle 다이스 Pop: 주입한 suppressed/boss/count 훅이 장기 상태보다 우선하고 호출 시점마다 다시 읽힌다', () => {
    // suppressed 훅은 룬 상태의 시간 억제보다 우선한다(훅 참, 상태 거짓 → 억제).
    const hookSuppressed = scene({ flags: 4, suppress: 0 }, false, { lifecycleHooks: { suppressed: () => true } });
    assert.deepEqual(ends(hookSuppressed, () => hookSuppressed.lifecycle.postPop(5, 4)), ['postpop:5,4']);
    // 훅 거짓, 상태 참 → 억제 아님.
    const hookActive = scene({ flags: 4, suppress: 1 }, false, { lifecycleHooks: { suppressed: () => false } });
    assert.deepEqual(ends(hookActive, () => hookActive.lifecycle.postPop(5, 4)), ['F:5,64,0', 'postpop:5,4']);
    // boss 훅은 신전 의식의 권한보다 우선한다.
    const hookBoss = scene({ flags: 1, boss: 0, initial: 1, local: 99 }, false, { lifecycleHooks: { boss: () => true } });
    assert.deepEqual(ends(hookBoss, () => hookBoss.lifecycle.postPop(5, 1)), ['C:164,0,8', 'owner:8,1', `P:8,${bits(40.75)},${bits(41.25)},0`, 'postpop:5,1']);
    const noBoss = scene({ flags: 1, boss: 1, initial: 1, local: 99 }, false, { lifecycleHooks: { boss: () => false } });
    assert.deepEqual(ends(noBoss, () => noBoss.lifecycle.postPop(5, 1)), ['postpop:5,1']);
    // 훅이 없으면 신전 의식의 권한을 읽는다.
    const stateBoss = scene({ flags: 1, boss: 1, initial: 1, local: 99 });
    assert.equal(ends(stateBoss, () => stateBoss.lifecycle.postPop(5, 1))[0], 'C:164,0,8');
    // count 훅의 반환은 부호 없는 32비트로 쓴다(-1 → 0xffffffff → 마지막 바이트 0xfe).
    const wide = scene({ owner: 4, flags: 8, initial: 1 }, false, { lifecycleHooks: { count: () => -1 } });
    ends(wide, () => wide.lifecycle.postPop(5, 8));
    assert.equal(wide.world.state.grantFlags[4], 0xfe);
});

test('DaisLifecycle 다이스 Pop: flags 2 는 두 번째 쌍, flags 1 은 첫 쌍을 현재 owner 칸에 등록하고 owner 0 이면 등록하지 않는다', () => {
    // 독립 배열: 다이스 SID 5 가 두 칸씩 들어간다.
    const run = (patch, bound) => {
        const s = scene({ initial: 1, local: 99, first: 77, second: 78, ...patch }, bound);
        ends(s, () => s.lifecycle.postPop(5, patch.flags));
        return s;
    };
    const slot = (s, owner) => (s.records && s.records !== null ? [0, 4, 8, 12].map((delta) => get(s.records, owner * STRIDE + PAIR_AT + delta)) : null);
    const independent = (s, owner) => { const p = s.lifecycleState.players[owner]; return [p.first, p.firstMirror, p.second, p.secondMirror]; };
    const second = run({ flags: 2, owner: 3 }, false);
    assert.deepEqual(independent(second, 3), [77, 123, 5, 5]);
    assert.deepEqual(independent(second, 2), [77, 123, 78, 124]);
    const first = run({ flags: 1, owner: 3 }, false);
    assert.deepEqual(independent(first, 3), [5, 5, 78, 124]);
    const both = run({ flags: 3, owner: 3 }, false);
    assert.deepEqual(independent(both, 3), [5, 5, 5, 5]);
    // owner 0 은 등록하지 않는다.
    const neutral = run({ flags: 3, owner: 0 }, false);
    assert.deepEqual(independent(neutral, 0), [77, 123, 78, 124]);
    // 하위 3비트에 2/1 이 없으면 등록하지 않는다.
    const none = run({ flags: 4, owner: 3 }, false);
    assert.deepEqual(independent(none, 3), [77, 123, 78, 124]);
    // 실제 Player 기록 묶음에서는 같은 네 DWORD 만 바뀐다.
    const bound = run({ flags: 3, owner: 3 }, true);
    assert.deepEqual(slot(bound, 3), [5, 5, 5, 5]);
    assert.deepEqual(slot(bound, 2), [77, 123, 78, 124]);
    assert.equal(bound.records[3 * STRIDE + PAIR_AT + 16], 0xa5);
    assert.equal(bound.records[3 * STRIDE + PAIR_AT - 1], 0xa5);
    // owner 9 (범위 밖)는 등록 전에 거부한다.
    const wild = scene({ initial: 1, flags: 2, owner: 9 });
    assert.throws(() => wild.lifecycle.postPop(5, 2), { name: 'RangeError', message: /Player범위/u });
});

test('DaisLifecycle 다이스 Pop: 한 바이트를 넘는 다이스 SID 도 SID 쌍 네 칸의 DWORD 폭으로 등록하고 독립 배열은 16비트로 저장한다', () => {
    const wide = 291;
    const run = (bound) => {
        const s = scene({ initial: 1, local: 99, owner: 3, first: 77, second: 78 }, bound);
        // 291 번 슬롯을 실제 free list 에서 확보해 다이스 5 의 이미지를 그대로 옮긴다.
        let sid = 0;
        while (sid !== wide) sid = s.world.pool.allocate(2);
        s.world.pool.allocated(wide).set(s.world.pool.slot(5));
        assert.throws(() => s.lifecycle.postPop(wide, 3), LifecycleBodyEnd);
        return s;
    };
    const independent = run(false).lifecycleState.players[3];
    assert.deepEqual([independent.first, independent.firstMirror, independent.second, independent.secondMirror], [wide, wide, wide, wide]);
    const bound = run(true);
    assert.deepEqual([0, 4, 8, 12].map((delta) => get(bound.records, 3 * STRIDE + PAIR_AT + delta)), [wide, wide, wide, wide]);
    assert.equal(bound.records[3 * STRIDE + PAIR_AT + 16], 0xa5);
});

test('DaisLifecycle 다이스 Pop: 권한이 있으면 로딩 중 단계 첫 프레임을 되돌린 뒤 현재 제단 TYPE 을 만들어 owner 와 좌표로 Pop 한다', () => {
    const table = [0, 3, 64, 120];
    // 가상 개수 0..3 이 단계 첫 프레임 표를 고른다. 직접 쓰기라 setFrame 효과가 없고, Unpop 이 프레임 DWORD 를 쓰레기로 채워도 네 바이트를 모두 덮어쓰며 다른 바이트는 건드리지 않는다.
    table.forEach((expected, count) => {
        const images = [];
        const s = scene({ flags: 1, boss: 1, loading: 1, initial: 1, local: 99, frame: 63 }, false, {
            lifecycleHooks: {
                count: () => count,
                unpop: (sid, flags) => { s.effects.push(`unpop:${sid},${flags}`); s.world.pool.physical(sid).fill(0xff, 36, 40); images.push(Buffer.from(s.world.pool.slot(sid)).toString('hex')); },
                repop: (sid, flags) => { s.effects.push(`repop:${sid},${flags}`); images.push(Buffer.from(s.world.pool.slot(sid)).toString('hex')); },
            },
        });
        const effects = ends(s, () => s.lifecycle.postPop(5, 1));
        assert.equal(effects.some((effect) => effect.startsWith('F:5,')), false);
        assert.deepEqual(effects.slice(0, 2), ['unpop:5,0', 'repop:5,0']);
        // Unpop 뒤 이미지에서 프레임 네 바이트만 표의 값으로 바뀐다(나머지 바이트는 그대로).
        const dirty = Buffer.from(images[0], 'hex');
        const clean = Buffer.from(images[1], 'hex');
        const want = Buffer.from(dirty);
        want.writeUInt32LE(expected, 36);
        assert.equal(clean.toString('hex'), want.toString('hex'), `개수 ${count}`);
    });
    // 개수 4 는 단계 표 밖이라 Unpop 직후 거부하고 Repop/생성은 하지 않는다.
    const bad = scene({ flags: 1, boss: 1, loading: 1, initial: 1, local: 99, frame: 63 }, false, { lifecycleHooks: { count: () => 4 } });
    assert.throws(() => bad.lifecycle.postPop(5, 1), { name: 'RangeError', message: /단계 첫번호/u });
    assert.deepEqual(bad.effects, ['unpop:5,0']);
    // 음수 개수는 부호 없는 32비트(0xffffffff)로 읽어 단계 표 밖이 된다.
    const negative = scene({ flags: 1, boss: 1, loading: 1, initial: 1, local: 99, frame: 63 }, false, { lifecycleHooks: { count: () => -1 } });
    assert.throws(() => negative.lifecycle.postPop(5, 1), { name: 'RangeError', message: /단계 첫번호/u });
    assert.deepEqual(negative.effects, ['unpop:5,0']);
    // 로딩 밖이면 단계 되돌리기를 하지 않고, 현재 단계가 0 이어도 하지 않는다.
    const live = scene({ flags: 1, boss: 1, loading: 0, initial: 1, local: 99, frame: 63 });
    assert.equal(ends(live, () => live.lifecycle.postPop(5, 1))[0], 'C:164,0,8');
    const stageZero = scene({ flags: 1, boss: 1, loading: 1, initial: 1, local: 99, frame: 14 });
    assert.equal(ends(stageZero, () => stageZero.lifecycle.postPop(5, 1))[0], 'C:164,0,8');
    // 제단 TYPE 은 후보마다 룬 상태의 현재 번호를 읽고, 생성한 SID 에 현재 owner 와 다이스의 현재 좌표를 준다.
    const calls = [];
    const made = scene({ flags: 1, boss: 1, initial: 1, local: 99, owner: 6 }, false, {
        lifecycleHooks: {
            create: (type, flags) => { calls.push(['create', type, flags]); return 9; },
            setOwner: (sid, owner) => calls.push(['owner', sid, owner]),
            pop: (sid, x, y, flags) => calls.push(['pop', sid, x, y, flags]),
        },
    });
    made.world.runeState.altarType = 170;
    put(made.world.slots[5], 14, bits(12.5));
    put(made.world.slots[5], 18, bits(-3.25));
    ends(made, () => made.lifecycle.postPop(5, 1));
    assert.deepEqual(calls, [['create', 170, 0], ['owner', 9, 6], ['pop', 9, 12.5, -3.25, 0]]);
});

test('DaisLifecycle 다이스 Pop: 로컬 owner 가 로딩 밖에서 홈 섬 밖 제단을 올리면 조건을 순서대로 확인하고 경고한다', () => {
    const warn = (patch, bound = false, hooks = {}) => {
        const s = scene({ flags: 1, initial: 1, boss: 0, ...patch }, bound, { lifecycleHooks: hooks });
        return ends(s, () => s.lifecycle.postPop(5, 1));
    };
    const x = bits(40.75);
    const y = bits(41.25);
    // 섬 번호가 홈 섬(17)과 다르고 옵션/억제가 없으면 경고한다.
    assert.deepEqual(warn({ profile: 2 }), [`SURFACE:${x},${y}`, 'HAS:9', 'ISLAND:9', 'OPTION:noShowWarnCantUpgrade', 'CHAT:WarnCantUpgradeAltarOffHomeIsland', 'postpop:5,1']);
    assert.deepEqual(warn({ profile: 2 }, true).slice(-2), ['CHAT:WarnCantUpgradeAltarOffHomeIsland', 'postpop:5,1']);
    // 같은 섬이면 옵션을 읽지 않는다.
    assert.deepEqual(warn({ profile: 3 }), [`SURFACE:${x},${y}`, 'HAS:9', 'ISLAND:9', 'postpop:5,1']);
    // 지면에 섬이 없으면 번호를 읽지 않고, 옵션이 켜졌으면 경고하지 않는다.
    assert.deepEqual(warn({ profile: 1 }), [`SURFACE:${x},${y}`, 'HAS:9', 'postpop:5,1']);
    assert.deepEqual(warn({ profile: 4 }), [`SURFACE:${x},${y}`, 'HAS:9', 'ISLAND:9', 'OPTION:noShowWarnCantUpgrade', 'postpop:5,1']);
    // 지면 조회가 없거나 풀 밖이면 섬 존재를 묻지 않는다.
    assert.deepEqual(warn({ profile: 0 }), [`SURFACE:${x},${y}`, 'postpop:5,1']);
    assert.deepEqual(warn({ profile: 2 }, false, { surface: () => 24000 }), ['postpop:5,1']);
    assert.deepEqual(warn({ profile: 2 }, false, { surface: () => 23999, hasIsland: () => false }), ['postpop:5,1']);
    assert.deepEqual(warn({ profile: 2 }, false, { surface: () => 1, hasIsland: () => true, islandNumber: () => 18, option: () => false, chat: (key) => { throw new Error(`경고 ${key}`); }, suppressed: () => true }), ['postpop:5,1']);
    // 다른 owner 이거나 로딩 중이면 지면을 읽지 않는다.
    assert.deepEqual(warn({ profile: 2, local: 2 }), ['postpop:5,1']);
    assert.deepEqual(warn({ profile: 2, loading: 1 }), ['postpop:5,1']);
    // 홈 섬 번호는 묶은 기록에서 지금 읽는다: 18 로 바꾸면 같은 섬이라 옵션을 읽지 않는다.
    const s = scene({ flags: 1, initial: 1, profile: 2 }, true);
    put(s.records, 1 * STRIDE + HOME_AT, 18);
    assert.deepEqual(ends(s, () => s.lifecycle.postPop(5, 1)), [`SURFACE:${x},${y}`, 'HAS:9', 'ISLAND:9', 'postpop:5,1']);
    // 경고 직전 억제를 다시 읽는다: 첫 읽기 거짓, 두 번째 읽기 참이면 옵션은 읽되 경고하지 않는다.
    const reads = [false, true];
    const late = warn({ profile: 2 }, false, { suppressed: () => reads.shift() ?? true });
    assert.deepEqual(late, [`SURFACE:${x},${y}`, 'HAS:9', 'ISLAND:9', 'OPTION:noShowWarnCantUpgrade', 'postpop:5,1']);
    assert.deepEqual(reads, []);
});

test('DaisLifecycle 제단 Pop: flags 1 일 때만 로컬 21 을 예약하고 성공하면 현재 저항 지연 초(float32)로 부착한 뒤 공통 후처리로 끝난다', () => {
    const altar = (patch, flags, options = {}) => {
        const s = scene({ mode: 'app', ...patch }, false, options);
        s.altarState.struggleSeconds = patch.seconds ?? 3.0;
        return { s, effects: ends(s, () => s.lifecycle.postPop(7, flags)) };
    };
    assert.deepEqual(altar({}, 1).effects, ['N:40', `REG:21,7,${bits(3.0)}`, 'postpop:7,1']);
    assert.deepEqual(altar({}, 3).effects, ['N:40', `REG:21,7,${bits(3.0)}`, 'postpop:7,3']);
    // 예약이 실패하면 부착하지 않는다.
    assert.deepEqual(altar({ reserve: 0 }, 1).effects, ['N:40', 'postpop:7,1']);
    // flags 1 이 없으면 예약 자체를 하지 않는다.
    assert.deepEqual(altar({}, 2).effects, ['postpop:7,2']);
    assert.deepEqual(altar({}, 0).effects, ['postpop:7,0']);
    // 지연 초는 float32 로 부착한다.
    assert.deepEqual(altar({ seconds: 0.1 }, 1).effects, ['N:40', `REG:21,7,${bits(0.1)}`, 'postpop:7,1']);
    // 훅이 받는 수 자체가 float32 이다(비트 비교만으로는 double 전달이 가려진다).
    const received = [];
    const exactly = altar({ seconds: 0.1 }, 1, { lifecycleHooks: { addRegular: (sid, event, payload) => received.push([sid, event, payload]) } });
    assert.deepEqual(exactly.effects, ['N:40', 'postpop:7,1']);
    assert.deepEqual(received, [[7, 21, Math.fround(0.1)]]);
    assert.notEqual(received[0][2], 0.1);
    // 제단 Pop 은 다이스 전용 상태(개수·쌍)를 건드리지 않는다.
    const s = scene({ mode: 'app', initial: 0, saved: 2 });
    ends(s, () => s.lifecycle.postPop(7, 1));
    assert.equal(s.world.state.grantFlags[1], 2);
    assert.deepEqual(s.lifecycleState.players[1], { first: 5, firstMirror: 123, second: 5, secondMirror: 124, homeIsland: 17 });
});

test('DaisLifecycle 제단 전처리: 다이스에는 거부하고 훅 destroy 를 고유 삭제 실행기에 그대로 넘긴다', () => {
    const s = scene({ mode: 'apre' });
    assert.throws(() => s.lifecycle.altarPreDestroy(5, 0), { name: 'TypeError', message: /제단 파생전처리/u });
    assert.throws(() => s.lifecycle.altarPreDestroy(6, 0), { name: 'TypeError', message: /제단 파생전처리/u });
    // destroy 훅이 없으면 살아 있는 다이스 삭제에서 실패하고 아무 raw 도 바꾸지 않는다.
    const noDestroy = scene({ mode: 'apre' }, false, { lifecycleHooks: { destroy: undefined } });
    const before = Buffer.from(noDestroy.world.pool.slot(5)).toString('hex');
    put(noDestroy.world.slots[5], 12, 0, 2);
    assert.throws(() => noDestroy.lifecycle.altarPreDestroy(7, 0), { name: 'Error', message: /다이스 삭제의 선택된 실제 하위 미연결/u });
    assert.notEqual(before, '');
    // 제단 삭제 flags 에 0x400000 이 있으면 다이스를 다시 지우지 않고 바로 마지막 공통 전처리로 간다.
    const final = scene({ mode: 'apre' });
    assert.deepEqual(ends(final, () => final.lifecycle.altarPreDestroy(7, 0x400000)).slice(-1), ['B:7,4194304']);
});

test('DaisLifecycle 삭제 후처리: 현재 owner 와 정확히 일치하는 SID 쌍만 두 번째 쌍 → 첫 쌍 순서로 지우고 전체 Damageable 후처리로 flags 를 그대로 넘긴다', () => {
    const independent = (s, owner) => { const p = s.lifecycleState.players[owner]; return [p.first, p.firstMirror, p.second, p.secondMirror]; };
    const runBoth = (patch, check) => {
        // 독립 배열과 묶은 기록 양쪽에서 같은 결과여야 한다.
        const a = scene({ mode: 'dpost', ...patch }, false);
        assert.deepEqual(ends(a, () => a.lifecycle.postDestroy(5, patch.flags ?? 0)).slice(-1), [`post:5,${patch.flags ?? 0}`]);
        const b = scene({ mode: 'dpost', ...patch }, true);
        ends(b, () => b.lifecycle.postDestroy(5, patch.flags ?? 0));
        check([independent(a, patch.owner ?? 1), [0, 4, 8, 12].map((delta) => get(b.records, (patch.owner ?? 1) * STRIDE + PAIR_AT + delta))], a, b);
    };
    // 두 쌍 모두 이 다이스(5)면 모두 지운다.
    runBoth({ first: 5, second: 5, flags: 33 }, ([i, r]) => { assert.deepEqual(i, [0, 0, 0, 0]); assert.deepEqual(r, [0, 0, 0, 0]); });
    // 하나만 일치하면 그 쌍만 지우고 다른 쌍과 미러는 보존한다.
    runBoth({ first: 5, second: 78 }, ([i, r]) => { assert.deepEqual(i, [0, 0, 78, 124]); assert.deepEqual(r, [0, 0, 78, 124]); });
    runBoth({ first: 77, second: 5 }, ([i, r]) => { assert.deepEqual(i, [77, 123, 0, 0]); assert.deepEqual(r, [77, 123, 0, 0]); });
    runBoth({ first: 77, second: 78 }, ([i, r]) => { assert.deepEqual(i, [77, 123, 78, 124]); assert.deepEqual(r, [77, 123, 78, 124]); });
    // 미러 칸이 같아도 주 칸이 다르면 지우지 않는다(미러만 SID 일 때).
    const mirror = scene({ mode: 'dpost', first: 77, second: 78 });
    mirror.lifecycleState.players[1].firstMirror = 5;
    ends(mirror, () => mirror.lifecycle.postDestroy(5, 0));
    assert.deepEqual(independent(mirror, 1), [77, 5, 78, 124]);
    // owner 0 은 기록을 건드리지 않는다.
    const neutral = scene({ mode: 'dpost', owner: 0, first: 5, second: 5 });
    ends(neutral, () => neutral.lifecycle.postDestroy(5, 0));
    assert.deepEqual(independent(neutral, 0), [5, 123, 5, 124]);
    // 묶은 기록에서도 owner 0 이면 기록을 읽지 않는다.
    const neutralBound = scene({ mode: 'dpost', owner: 0, first: 5, second: 5 }, true);
    ends(neutralBound, () => neutralBound.lifecycle.postDestroy(5, 0));
    assert.deepEqual(neutralBound.records, neutralBound.beforeRecords);
    // owner 9 는 범위 밖이라 거부한다(독립 입력).
    const wild = scene({ mode: 'dpost', owner: 9 });
    assert.throws(() => wild.lifecycle.postDestroy(5, 0), { name: 'RangeError', message: /Player범위/u });
    assert.deepEqual(wild.effects, []);
    // 제단 삭제 후처리는 쌍에 손대지 않고 바로 전체 Damageable 후처리로 간다.
    const altar = scene({ mode: 'apost', first: 7, second: 7 }, true);
    assert.deepEqual(ends(altar, () => altar.lifecycle.postDestroy(7, 9)), ['post:7,9']);
    assert.deepEqual(altar.records, altar.beforeRecords);
    // 다이스/제단이 아닌 가상표는 어느 경로에서도 거부한다.
    const other = scene({ mode: 'dpost' }, true);
    assert.throws(() => other.lifecycle.postDestroy(6, 0), { name: 'TypeError', message: /삭제후처리/u });
    const otherIndependent = scene({ mode: 'dpost' });
    assert.throws(() => otherIndependent.lifecycle.postDestroy(6, 0), { name: 'TypeError', message: /삭제후처리/u });
    assert.throws(() => otherIndependent.lifecycle.postPop(6, 0), { name: 'TypeError', message: /Pop 타입/u });
});

test('DaisLifecycle 훅 요구: 도달한 훅이 없으면 효과 전에 같은 문구로 실패하고 도달하지 않은 훅은 요구하지 않는다', () => {
    const missing = (patch, hookName, fn, effectsAfter) => {
        const s = scene(patch, false, { lifecycleHooks: { [hookName]: undefined } });
        assert.throws(() => fn(s), { name: 'Error', message: /다이스 수명의 선택된 실제 장치 미연결/u }, hookName);
        assert.deepEqual(s.effects, effectsAfter, hookName);
    };
    missing({ flags: 4 }, 'setFrame', (s) => s.lifecycle.postPop(5, 4), []);
    missing({ flags: 1, boss: 1, loading: 1, initial: 1, local: 99, frame: 63 }, 'unpop', (s) => s.lifecycle.postPop(5, 1), []);
    missing({ flags: 1, boss: 1, loading: 1, initial: 1, local: 99, frame: 63 }, 'repop', (s) => s.lifecycle.postPop(5, 1), ['unpop:5,0']);
    missing({ flags: 1, boss: 1, initial: 1, local: 99 }, 'create', (s) => s.lifecycle.postPop(5, 1), []);
    missing({ flags: 1, boss: 1, initial: 1, local: 99 }, 'setOwner', (s) => s.lifecycle.postPop(5, 1), ['C:164,0,8']);
    missing({ flags: 1, boss: 1, initial: 1, local: 99 }, 'pop', (s) => s.lifecycle.postPop(5, 1), ['C:164,0,8', 'owner:8,1']);
    missing({ flags: 1, initial: 1, profile: 2 }, 'surface', (s) => s.lifecycle.postPop(5, 1), []);
    missing({ flags: 1, initial: 1, profile: 2 }, 'hasIsland', (s) => s.lifecycle.postPop(5, 1), [`SURFACE:${bits(40.75)},${bits(41.25)}`]);
    missing({ flags: 1, initial: 1, profile: 2 }, 'islandNumber', (s) => s.lifecycle.postPop(5, 1), [`SURFACE:${bits(40.75)},${bits(41.25)}`, 'HAS:9']);
    missing({ flags: 1, initial: 1, profile: 2 }, 'option', (s) => s.lifecycle.postPop(5, 1), [`SURFACE:${bits(40.75)},${bits(41.25)}`, 'HAS:9', 'ISLAND:9']);
    missing({ flags: 1, initial: 1, profile: 2 }, 'chat', (s) => s.lifecycle.postPop(5, 1), [`SURFACE:${bits(40.75)},${bits(41.25)}`, 'HAS:9', 'ISLAND:9', 'OPTION:noShowWarnCantUpgrade']);
    missing({ mode: 'app' }, 'reserveRegular', (s) => s.lifecycle.postPop(7, 1), []);
    missing({ mode: 'app' }, 'addRegular', (s) => s.lifecycle.postPop(7, 1), ['N:40']);
    missing({ mode: 'dpost' }, 'damageablePostDestroy', (s) => s.lifecycle.postDestroy(5, 0), []);
    missing({ mode: 'apost' }, 'damageablePostDestroy', (s) => s.lifecycle.postDestroy(7, 0), []);
    // flags 0 의 단순 Pop 은 생성/표시/음향 장치를 하나도 요구하지 않는다.
    const bare = scene({ flags: 0, initial: 1 }, false, { lifecycleHooks: Object.fromEntries(['setFrame', 'unpop', 'repop', 'create', 'setOwner', 'pop', 'surface', 'hasIsland', 'islandNumber', 'option', 'chat', 'reserveRegular', 'addRegular', 'destroy'].map((name) => [name, undefined])) });
    assert.deepEqual(ends(bare, () => bare.lifecycle.postPop(5, 0)), ['postpop:5,0']);
});

test('makeDaisLifecycleHooks: Frame/Factory/Owner/Host 를 같은 풀 몸체에 연결하고 나머지 훅은 보존하며 원래 묶음은 바꾸지 않는다', () => {
    const s = scene();
    const pool = s.world.pool;
    const log = [];
    const frame = { pool, set: (...args) => log.push(['frame', ...args]) };
    const host = { pool, addRegular: (...args) => log.push(['host', ...args]) };
    const factory = { pool, create: (...args) => { log.push(['factory', ...args]); return 42; } };
    const owner = { pool, set: (...args) => log.push(['owner', ...args]) };
    const original = { chat: () => 'keep', setFrame: () => 'replaced', create: () => 'replaced' };
    const hooks = makeDaisLifecycleHooks(pool, frame, host, factory, owner, original);
    assert.equal(hooks.chat, original.chat);
    assert.notEqual(hooks, original);
    assert.equal(original.setFrame(), 'replaced');
    hooks.setFrame(5, 64, 0);
    assert.equal(hooks.create(164, 0), 42);
    hooks.setOwner(8, 3);
    hooks.addRegular(7, 21, 3);
    assert.deepEqual(log, [['frame', 5, 64, 0], ['factory', 164, 0], ['owner', 8, 3], ['host', 7, 21, 3]]);
    assert.deepEqual(Object.keys(makeDaisLifecycleHooks(pool, frame, host, factory, owner)).sort(), ['addRegular', 'create', 'setFrame', 'setOwner']);
    // 풀이 다르거나 몸체가 없으면 거부한다.
    const other = { pool: scene().world.pool };
    assert.throws(() => makeDaisLifecycleHooks(pool, { ...frame, ...other }, host, factory, owner), TypeError);
    assert.throws(() => makeDaisLifecycleHooks(pool, frame, { ...host, ...other }, factory, owner), TypeError);
    assert.throws(() => makeDaisLifecycleHooks(pool, frame, host, { ...factory, ...other }, owner), TypeError);
    assert.throws(() => makeDaisLifecycleHooks(pool, frame, host, factory, { ...owner, ...other }), TypeError);
    assert.throws(() => makeDaisLifecycleHooks(pool, null, host, factory, owner), TypeError);
    assert.throws(() => makeDaisLifecycleHooks(pool, frame, undefined, factory, owner), TypeError);
    assert.throws(() => makeDaisLifecycleHooks(pool, frame, host, null, owner), TypeError);
    assert.throws(() => makeDaisLifecycleHooks(pool, frame, host, factory, undefined), TypeError);
});

test('DaisLifecycle 실제 Frame/Host 연결: 훅 공장으로 만든 setFrame/addRegular 가 실제 몸체에서 같은 효과를 낸다', () => {
    const s = scene({ flags: 4, mode: 'dpp' });
    const hooks = makeDaisLifecycleHooks(s.world.pool, s.world.frame, s.world.host, { pool: s.world.pool, create: () => 0 }, { pool: s.world.pool, set: () => {} }, s.hooks);
    const lifecycle = new RawDaisLifecycle(s.world.pool, s.world.hash, s.world.types, s.world.footprint, s.world.ritual, s.altar, s.carrierPost, s.damageable, s.world.state, s.altarState, s.lifecycleState, hooks);
    // 실제 SquidFrame.set 이 프레임을 쓰고 표시 갱신 훅(짝수 번째에 기록)을 부른다. 첫 갱신은 기록되지 않는다.
    assert.throws(() => lifecycle.postPop(5, 4), LifecycleBodyEnd);
    assert.equal(get(s.world.pool.slot(5), 36, 4), 64);
    assert.deepEqual(s.effects, ['F:5,64,0', 'postpop:5,4']);
});
