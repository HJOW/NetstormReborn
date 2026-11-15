// 힐 주문 대상 본문과 버스트 정기 raw-bomb-heal.js(RawBombHeal·handleHealBurstRegular·handleGravityBurstRegular)를 원본 10.78 관측 122행과 cpppj 시험 계약으로 대조한다(WB04-E553).
//  - bomb-gravity-regular-x86.tsv 122행: 그라비테이션 정기의 signed count·프레임 감김/역방향·종료(일반 Regular 2 선행 뒤 삭제)·NaN payload 와 표시 갱신 순서(S=표시 갱신 두 번째, R=refresh, ND=Regular 부착 뒤 삭제).
//  - 계약(cpppj ClientBombHealOwner_*): 자격 마스크·QA 선반환·signed DWORD 감김 평균·최초 HP 저장·상태 두 비트·펜스→보행 순서·현재 가상 표 재조회·버스트 중심·누락 효과·버스트 진행/종료.
// 실제 게임·브라우저 실행이 아니다. (2026-11-15 WB04-E553)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, floatToBits, bitsToFloat } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { Kernel } from '../www/core/kernel.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { RawSquidDestroy } from '../www/core/squid-destroy.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { SquidProcessHost, SquidProcessState } from '../www/core/squid-process.js';
import { SquidFrame } from '../www/core/squid-frame.js';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { RawBombHeal, handleHealBurstRegular, handleGravityBurstRegular } from '../www/core/raw-bomb-heal.js';

// 원본 관측 fixture 이다. 없으면 해당 검사를 건너뛴다. (2026-11-15 WB04-E553)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'bomb-gravity-regular-x86.tsv'));
// 프레임 지정기의 선택 안 한 가상 효과는 호출되면 실패한다(검사 전용 금지 경계). (2026-11-15 WB04-E553)
const forbidden = () => { throw new Error('힐 시험에서 호출되면 안 되는 효과'); };

// 실제 Host/Kernel 과 188칸 TYPE/프레임 표를 가진 장면이다: 부모(대상) 슬롯은 필요할 때 새로 확보한다. (2026-11-15 WB04-E553)
function hostScene() {
    const pool = new SidPool(32768, { server: true });
    const types = Array.from({ length: 188 }, (_, number) => ({
        cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, ...([1, 2, 3, 6].includes(number) ? { constructorAddress: typeConstructorAddress(number) } : {})
    }));
    const destroy = new RawSquidDestroy(pool, new SquidUnpop(pool, new SquidHash(), new Uint8Array(65536)), types);
    const kernel = new Kernel();
    const state = new SquidProcessState();
    state.now = 10;
    const host = new SquidProcessHost(pool, types, kernel, destroy, state);
    const frames = Array.from({ length: 188 }, () => new FrameCodeTable([]));
    return { pool, types, host, frames, state };
}

// 지정 TYPE 의 객체 슬롯을 새로 확보한다. (2026-11-15 WB04-E553)
function asset(scene, type, frame = 0) {
    const sid = scene.pool.allocate();
    const raw = scene.pool.allocated(sid);
    raw[10] = type;
    raw[11] = 0;
    raw[33] = 0;
    put(raw, 36, frame);
    return sid;
}

test('handleGravityBurstRegular: 원본 10.78 그라비테이션 정기 122행의 프레임·반환·효과 순서가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = hostScene();
    scene.types[174].flags2 = 4;
    let count = 0;
    // 동일 실제 풀/Host 의 서로 다른 부모에 각 독립 입력을 공급한다.
    for (const row of rows.filter((fields) => fields[0] !== 'total')) {
        const sid = asset(scene, 174, Number(row[2]));
        scene.frames[174] = new FrameCodeTable(Array.from({ length: Number(row[0]) }, () => ({ side: 65, variant: 65, number: 0, flags: 0 })));
        const state = { trace: '', updates: 0 };
        const hooks = {
            frameSize: forbidden,
            unpop: forbidden,
            pop: forbidden,
            update: (parent, flags) => { assert.ok(parent === sid && flags === 0); state.updates += 1; if (state.updates === 2) state.trace += 'S'; }
        };
        const setter = new SquidFrame(scene.pool, scene.types, hooks);
        const result = handleGravityBurstRegular(scene.pool, scene.frames, setter, scene.host,
            (parent) => { assert.ok(parent === sid && state.updates === 2); state.trace += 'R'; },
            (parent, flags) => {
                const next = scene.host.findEvent(sid, 2);
                assert.ok(parent === sid && flags === 0x100048 && next && next.payload === Math.fround(0.05) && next.count === 0);
                state.trace += 'ND';
            },
            sid, Number(row[1]), bitsToFloat(Number(row[3])));
        assert.equal(get(scene.pool.slot(sid), 36), Number(row[4]), `행 ${count} 프레임`);
        assert.equal(floatToBits(result), Number(row[5]), `행 ${count} 반환`);
        assert.equal(state.trace, row[6], `행 ${count} 효과`);
        count += 1;
    }
    assert.equal(count, 122);
});

// ---- cpppj ClientBombHealOwner_* 계약 시험(WB04-E553) ----

// 힐 대상 본문의 작은 장면이다: 같은 풀의 부모와 대상(표면 대상, HP 없음)·기본 효과(펜스 타입 없음·표시 갱신 무동작)이다. (2026-11-15 WB04-E553)
function healScene() {
    const pool = new SidPool(24000, { server: false });
    const parent = pool.allocate(2);
    const target = pool.allocate(2);
    const types = Array.from({ length: 188 }, () => ({ flags2: 0, footX: 0, footY: 0 }));
    pool.allocated(parent).fill(0);
    const raw = pool.allocated(target);
    raw.fill(0);
    raw[10] = 94;
    types[94].flags2 = 4;
    const hooks = { hasHitPoints: () => false, fenceType: () => 0xffffffff, refresh: () => {} };
    return { pool, parent, target, types, raw, hooks, body: () => new RawBombHeal(pool, types, hooks) };
}

test('RawBombHeal: 자격은 현재 genus 의 전체 마스크이고 QA 대상은 TYPE/장치를 읽기 전에 끝난다', () => {
    const f = healScene();
    const body = f.body();
    f.raw[11] = 3;
    f.raw[34] = 255;
    put(f.raw, 26, 0);
    f.types[94].flags2 = 0xffffffff;
    assert.equal(body.mayTarget(f.target), 0x81574204);
    f.types[94].flags2 = 0x200000;
    assert.equal(body.mayTarget(f.target), 0);
    f.types[94].flags2 = 0x80000000;
    assert.equal(body.mayTarget(f.target), 0x80000000);
    // 대상이 부모의 운반체 WORD 이면 잘못된 현재 TYPE 과 누락 장치도 읽지 않고 상태/HP 를 그대로 둔다.
    const g = healScene();
    put(g.pool.allocated(g.parent), 12, g.target, 2);
    g.raw[10] = 255;
    g.raw[40] = 0x30;
    put(g.raw, 26, 123);
    new RawBombHeal(g.pool, g.types, {}).apply(g.parent, g.target);
    assert.deepEqual([g.raw[40], get(g.raw, 26)], [0x30, 123]);
    // 판본/표 크기 계약이다.
    assert.throws(() => new RawBombHeal({ edition: 'cd1072' }, f.types, f.hooks), TypeError);
    assert.throws(() => new RawBombHeal(f.pool, f.types.slice(0, 187), f.hooks), TypeError);
    assert.throws(() => new RawBombHeal(f.pool, [...f.types, f.types[0]], f.hooks), TypeError);
});

test('RawBombHeal: HP 평균은 DWORD 합의 signed 0 쪽 절삭이고 최초 HP 를 최대 HP 조회 전에 저장한다', () => {
    const f = healScene();
    let maximum = 0;
    f.hooks.hasHitPoints = () => true;
    f.hooks.maximumHitPoints = () => maximum;
    const body = f.body();
    // [현재, 최대, 기대] 7가지: ADD/CDQ/SUB/SAR 의 32비트 감김과 음수 0 방향 평균이다.
    for (const [health, max, expected] of [[100, 301, 200], [-4, 1, -1], [-3, 0, -1], [0, 1, 0], [2147483647, 1, -1073741824], [-2147483648, -1, 1073741823], [2147483647, 2147483647, -1]]) {
        put(f.raw, 26, health >>> 0);
        maximum = max;
        body.apply(f.parent, f.target);
        assert.equal(get(f.raw, 26) | 0, expected, `${health},${max}`);
    }
    // 최대 HP 조회가 현재 HP 를 바꿔도 최초 HP(101)로 평균을 쓴다.
    const g = healScene();
    put(g.raw, 26, 101);
    g.hooks.hasHitPoints = () => true;
    g.hooks.maximumHitPoints = () => { put(g.raw, 26, 9999); return 300; };
    g.body().apply(g.parent, g.target);
    assert.equal(get(g.raw, 26), 200);
});

test('RawBombHeal: 상태 두 비트만 해제하고 펜스→보행 순서와 현재 가상 표 재조회, 갱신 뒤 현재 버스트 중심을 쓴다', () => {
    // HP 가 없어도 은신/마비 두 비트(0x30)만 해제하고 현재 가상 88 을 한 번 호출한다.
    let f = healScene();
    f.raw[40] = 0xff;
    put(f.raw, 0, 0x33333333);
    put(f.raw, 26, 0xabcdef12);
    let calls = 0;
    f.hooks.refresh = (target, table) => { assert.ok(target === f.target && table === get(f.raw, 0) && f.raw[40] === 0xcf); calls += 1; };
    f.body().apply(f.parent, f.target);
    assert.deepEqual([calls, f.raw[40], get(f.raw, 26)], [1, 0xcf, 0xabcdef12]);
    // 펜스 효과가 바꾼 현재 genus 와 해제가 바꾼 현재 가상 표를 순서대로 쓴다(F→U→P).
    f = healScene();
    let order = '';
    f.hooks.fenceType = () => f.raw[10];
    f.hooks.fence = (target) => { assert.equal(target, f.target); order += 'F'; f.types[94].flags2 = 0x200004; put(f.raw, 0, 0x11111111); };
    f.hooks.unpop = (target, table, flags) => { assert.ok(target === f.target && table === 0x11111111 && flags === 0); order += 'U'; put(f.raw, 0, 0x22222222); };
    f.hooks.repop = (target, table, flags) => { assert.ok(target === f.target && table === 0x22222222 && flags === 0x40); order += 'P'; };
    f.body().apply(f.parent, f.target);
    assert.equal(order, 'FUP');
    // 갱신이 genus/전역을 바꾸면 버스트가 만들어지고, 생성 뒤의 대상 TYPE/좌표(발자국 중심)와 새 객체의 현재 가상 표로 놓는다.
    f = healScene();
    const created = f.pool.allocate(2);
    let burst = 176;
    order = '';
    f.hooks.refresh = () => { order += 'R'; f.types[94].flags2 = 0; burst = 177; };
    f.hooks.burstType = () => burst;
    f.hooks.create = (kind, flags) => {
        assert.ok(kind === 177 && flags === 2);
        order += 'C';
        f.raw[10] = 117;
        f.types[117].footX = 6;
        f.types[117].footY = 10;
        put(f.raw, 14, floatToBits(30));
        put(f.raw, 18, floatToBits(50));
        put(f.pool.allocated(created), 0, 0x12345678);
        return created;
    };
    f.hooks.pop = (target, table, x, y, flags) => { assert.ok(target === created && table === 0x12345678 && x === 27 && y === 45 && flags === 0); order += 'P'; };
    f.body().apply(f.parent, f.target);
    assert.equal(order, 'RCP');
    // 선택한 표시 갱신이 없으면 앞선 HP/상태 변경을 되돌리지 않고 명시 오류이다.
    f = healScene();
    f.raw[40] = 0x30;
    put(f.raw, 26, 100);
    f.hooks.hasHitPoints = () => true;
    f.hooks.maximumHitPoints = () => 300;
    f.hooks.refresh = undefined;
    assert.throws(() => f.body().apply(f.parent, f.target), /미연결/);
    assert.deepEqual([f.raw[40], get(f.raw, 26)], [0, 200]);
});

test('handleHealBurstRegular: 현재 프레임+1 진행과 NaN payload 보존, 종료는 일반 Regular 2 를 먼저 붙인 뒤 삭제한다', () => {
    const scene = hostScene();
    const frames = scene.frames;
    frames[94] = new FrameCodeTable(Array.from({ length: 5 }, () => ({ side: 65, variant: 65, number: 0, flags: 0 })));
    const sid = asset(scene, 94, 1);
    let updates = 0;
    const hooks = { frameSize: forbidden, unpop: forbidden, pop: forbidden, update: () => { updates += 1; } };
    const setter = new SquidFrame(scene.pool, (scene.types[94].flags2 = 4, scene.types), hooks);
    let refreshes = 0;
    // count 0 에서도 원래 현재 프레임+1(1→2)이며 첫 A 프레임으로 되돌리지 않는다. NaN payload 는 그대로 돌려준다.
    const result = handleHealBurstRegular(scene.pool, frames, setter, scene.host, (target) => { assert.ok(target === sid && get(scene.pool.slot(sid), 36) === 2); refreshes += 1; }, undefined, sid, 0, Number.NaN);
    assert.ok(Number.isNaN(result));
    assert.deepEqual([refreshes, updates], [1, 2]);
    // 종료 경계(count = 길이−1): 이벤트 2/0.05/횟수 0 의 일반 Regular 가 삭제 전에 붙어 있고 삭제 flags 가 0x100048 이다.
    let destroyed = false;
    const ended = handleHealBurstRegular(scene.pool, frames, setter, scene.host, undefined, (target, flags) => {
        const next = scene.host.findEvent(sid, 2);
        assert.ok(target === sid && flags === 0x100048 && next && next.payload === Math.fround(0.05) && next.count === 0);
        destroyed = true;
    }, sid, 4, 0.25);
    assert.equal(ended, -1);
    assert.equal(destroyed, true);
    // 풀/프레임/Host 계약이다(각 검사를 따로 걸리게 다른 쪽을 맞춘다).
    const fake = { edition: 'cd1072', capacity: 24000 };
    assert.throws(() => handleHealBurstRegular(fake, frames, setter, { pool: fake }, undefined, undefined, sid, 0, 0), { name: 'TypeError', message: /힐 버스트 실제 풀\/프레임\/Host 계약 오류/ });
    assert.throws(() => handleHealBurstRegular(scene.pool, frames.slice(0, 187), setter, scene.host, undefined, undefined, sid, 0, 0), { name: 'TypeError', message: /힐 버스트 실제 풀\/프레임\/Host 계약 오류/ });
    assert.throws(() => handleHealBurstRegular(scene.pool, [...frames, frames[0]], setter, scene.host, undefined, undefined, sid, 0, 0), { name: 'TypeError', message: /힐 버스트 실제 풀\/프레임\/Host 계약 오류/ });
    assert.throws(() => handleHealBurstRegular(scene.pool, frames, setter, { pool: fake }, undefined, undefined, sid, 0, 0), { name: 'TypeError', message: /힐 버스트 실제 풀\/프레임\/Host 계약 오류/ });
    assert.throws(() => handleGravityBurstRegular(fake, frames, { pool: fake }, { pool: fake }, undefined, undefined, sid, 0, 0), { name: 'TypeError', message: /그라비테이션 실제 풀\/프레임\/Host 계약 오류/ });
    assert.throws(() => handleGravityBurstRegular(scene.pool, frames, { pool: {} }, scene.host, undefined, undefined, sid, 0, 0), { name: 'TypeError', message: /그라비테이션 실제 풀\/프레임\/Host 계약 오류/ });
    assert.throws(() => handleGravityBurstRegular(scene.pool, frames, setter, { pool: fake }, undefined, undefined, sid, 0, 0), { name: 'TypeError', message: /그라비테이션 실제 풀\/프레임\/Host 계약 오류/ });
    assert.throws(() => handleGravityBurstRegular(scene.pool, [...frames, frames[0]], setter, scene.host, undefined, undefined, sid, 0, 0), { name: 'TypeError', message: /그라비테이션 실제 풀\/프레임\/Host 계약 오류/ });
    // 현재 TYPE 이 표 크기와 같으면 두 정기 모두 범위 오류이다.
    const bad = asset(scene, 188, 0);
    assert.throws(() => handleHealBurstRegular(scene.pool, frames, setter, scene.host, undefined, undefined, bad, 0, 0), RangeError);
    assert.throws(() => handleGravityBurstRegular(scene.pool, frames, setter, scene.host, undefined, undefined, bad, 0, 0), RangeError);
});

test('handleHealBurstRegular: 진행/종료 경계(길이−1)와 signed count·종료 Regular 의 부착 인자', () => {
    const scene = hostScene();
    scene.types[94].flags2 = 4;
    scene.frames[94] = new FrameCodeTable(Array.from({ length: 5 }, () => ({ side: 65, variant: 65, number: 0, flags: 0 })));
    const hooks = { frameSize: forbidden, unpop: forbidden, pop: forbidden, update: () => {} };
    const setter = new SquidFrame(scene.pool, scene.types, hooks);
    const run = (count) => {
        const sid = asset(scene, 94, 0);
        let ended = false;
        const result = handleHealBurstRegular(scene.pool, scene.frames, setter, scene.host, () => {}, () => { ended = true; }, sid, count, 0.5);
        return { ended, result };
    };
    // 길이 5 이면 한계는 4: count 3 은 진행, 4 는 종료이며 count 0xffffffff(signed −1)도 진행이다.
    assert.deepEqual([run(3), run(4), run(0xffffffff)], [{ ended: false, result: 0.5 }, { ended: true, result: -1 }, { ended: false, result: 0.5 }]);
    // 종료 때 붙이는 일반 Regular 는 이벤트 2/0.05, 공유 아님, flags 0x50 이다.
    const calls = [];
    const original = scene.host.attachRegular.bind(scene.host);
    scene.host.attachRegular = (process, parent, shared, flags) => { calls.push([process.event, process.payload, shared, flags]); return original(process, parent, shared, flags); };
    run(4);
    assert.deepEqual(calls, [[2, Math.fround(0.05), false, 0x50]]);
    // 그라비테이션도 같은 인자로 붙인다(프레임 0 에서 종료).
    calls.length = 0;
    const sid = asset(scene, 94, 0);
    handleGravityBurstRegular(scene.pool, scene.frames, setter, scene.host, () => {}, () => {}, sid, 4, 0.5);
    assert.deepEqual(calls, [[2, Math.fround(0.05), false, 0x50]]);
});

test('RawBombHeal: 현재 TYPE 이 표 크기이면 범위 오류이고 버스트 중심은 단정도로 저장한다', () => {
    const f = healScene();
    const body = f.body();
    f.raw[10] = 188;
    assert.throws(() => body.mayTarget(f.target), RangeError);
    // 대상 x 0.1(단정도)·발자국 3 의 중심은 (0.1f − 1.5)를 단정도로 좁힌 값이다(좁히지 않으면 마지막 비트가 달라진다).
    const g = healScene();
    const created = g.pool.allocate(2);
    g.types[94].flags2 = 0;
    g.types[94].footX = 3;
    g.types[94].footY = 5;
    put(g.raw, 14, floatToBits(0.1));
    put(g.raw, 18, floatToBits(0.2));
    g.hooks.burstType = () => 177;
    g.hooks.create = () => created;
    let seen = null;
    g.hooks.pop = (target, table, x, y) => { seen = [x, y]; };
    g.body().apply(g.parent, g.target);
    assert.deepEqual(seen, [Math.fround(Math.fround(0.1) - 1.5), Math.fround(Math.fround(0.2) - 2.5)]);
});
