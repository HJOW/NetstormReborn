// Factory 후처리 raw-factory-post-pop.js(RawFactoryPostPop·FactoryPostPopTypes·makeFactoryPostPopHooks)와 자산 후처리 공유 상태 raw-asset-post-pop-state.js 를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: factorypostpop-x86.tsv 의 10.78(originals) 498행(입력 11개: 종류·TYPE·flags·extra·boss·개수·프레임·예약 마스크·목록 개수·중복·변이 → 효과 기록·슬롯 50바이트·아웃포스트/거주 목록 전체·후보 두 개의 현재 TYPE/상태/extra).
//    일반 Factory 만의 부분 재생(FactoryParentCurrent_OrdinaryNativeRowsKeepCompleteExpectedOutput)은 가상 +0x20 분배(postPopVirtual)로, 전체 498행은 Activate 로 실행한다. CD 10.72 행은 웹 범위(10.78) 밖이다.
//  - 계약: FactoryParentCurrent_*·FactoryGlobalsCurrent_*(선택하지 않은 장치 비요구, 예약 실패 시 미연결 생성자 비요구, 늦은 프레임 실패의 예약/공통 보존, 현재 TYPE/boss 재조회, 경고→삭제 순서, 두 가상 개수의 현재 TYPE,
//    개수 실패, 물리 TYPE 읽기 시점, 타이머/제외 번호의 전체 DWORD·현재 값·독립 사본).
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E61: Factory 후처리 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidPostPop } from '../www/core/squid-postpop.js';
import { SquidPostPopState } from '../www/core/squid-postpop-state.js';
import { RawCarrierPostPop, CarrierPostPopState } from '../www/core/raw-carrier-post-pop.js';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { AssetPostPopState } from '../www/core/raw-asset-post-pop-state.js';
import { RawFactoryPostPop, FactoryPostPopTypes, makeFactoryPostPopHooks } from '../www/core/raw-factory-post-pop.js';

// 원본 관측 10.78 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E61)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'factorypostpop-x86.tsv'))?.filter((row) => row[0] === 'originals') ?? null;

// 관찰 SID 이다. (2026-11-08 WB04-E61)
const SOURCE = 50;

// 예외가 나는지 본다. (2026-11-08 WB04-E61)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E61)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 입력 글자 구간만 공급하며 프레임 보정 기대값은 native 가 계산한다. (2026-11-08 WB04-E61)
const factoryCodes = () => [['A', 0], ['A', 1], ['B', 0], ['B', 1], ['B', 2], ['H', 0], ['H', 1]]
    .map(([side, flags]) => ({ side: side.charCodeAt(0), variant: 'A'.charCodeAt(0), flags }));

// 같은 풀·해시·프레임과 callback 입력을 묶은 검사 장면이다. native 의 client 번호 50/60/61 을 확보하며 미션 초기화 자체를 검증하지는 않는다. (2026-11-08 WB04-E61)
function factoryScene() {
    const pool = new SidPool(24000, { server: false });
    for (let sid = 5; sid <= 61; sid += 1) assert.equal(pool.allocate(2), sid);
    const types = Array.from({ length: 188 }, () => ({ cost: 0, flags1: 0, flags2: 0, footX: 1, footY: 1, maxHitPoints: 0, zOrder: 0 }));
    const frames = types.map(() => new FrameCodeTable(factoryCodes()));
    const scene = { pool, hash: new SquidHash(), types, frames, baseState: null, carrierState: null, state: null, events: [], reserveMask: 0, alloc: 0, mutation: 0 };
    // 입력 슬롯/체인/목록을 초기화하며 기대 결과는 만들지 않는다.
    scene.prepare = (input) => {
        assert.equal(input.length, 11);
        scene.events = [];
        scene.alloc = 0;
        scene.reserveMask = input[7];
        scene.mutation = input[10];
        scene.state = new AssetPostPopState();
        scene.baseState = new SquidPostPopState();
        scene.carrierState = new CarrierPostPopState();
        scene.baseState.suppressed = true;
        scene.carrierState.boss = true;
        scene.state.boss = input[4] !== 0;
        scene.hash.reset();
        scene.hash.setCell(1, 25, 26, 50);
        // 실제 finder 의 next 저장 순서가 source→60→61 이 되게 한다.
        for (const sid of [50, 60, 61]) {
            const raw = pool.allocated(sid);
            raw.fill(0);
            put(raw, 0, input[0] ? 0x509210 : 0x509148);
            put(raw, 4, sid === 50 ? 60 : sid === 60 ? 61 : 0, 2);
            raw[10] = sid === 61 ? (input[1] !== 86 ? 86 : 87) : input[1];
            raw[11] = sid === 50 ? (input[5] << 5) & 0xff : 0;
            raw[40] = sid === 50 ? input[3] : sid === 61 ? 8 : 0;
            put(raw, 14, floatToBits(sid === 50 ? 50.75 : sid === 60 ? 50.25 : 50.5));
            put(raw, 18, floatToBits(sid === 50 ? 52.5 : sid === 60 ? 52.75 : 52.25));
        }
        put(pool.allocated(SOURCE), 36, input[6]);
        // 중복 번호나 꽉 찬 capacity 를 그대로 입력한다.
        for (const list of [scene.state.outposts, scene.state.residences]) {
            list.entries = [input[9] ? 50 : 7, 8, 9, 10];
            list.count = input[8];
        }
    };
    // 직접 Damageable 이 실제 공통 장부를 호출하고 native 경계와 같은 사건을 남긴다.
    scene.damageableHooks = (base) => ({
        carrierCheck: () => { throw new Error('Factory는 Carrier 가상을 호출하지 않습니다'); },
        claimGround: () => { throw new Error('이 입력은 로딩 소유권을 호출하지 않습니다'); },
        validateBase: (sid, flags) => base.validateBase(sid, flags),
        base: (sid, flags) => {
            scene.events.push(`D:${flags}`);
            if (scene.mutation === 1) pool.allocated(sid)[11] = 7 << 5;
            base.postPopBase(sid, flags);
        }
    });
    // native 와 같은 프로세스·프레임/소유권·로그/삭제 관찰 경계를 제공한다.
    scene.hooks = () => ({
        reserveRegular: () => { scene.events.push('A:40'); const bit = 1 << scene.alloc; scene.alloc += 1; return (scene.reserveMask & bit) !== 0; },
        addRegular: (sid, event, delay) => {
            scene.events.push(`R:${sid}:${event}:${floatToBits(delay)}`);
            if (scene.mutation === 2 && event === 2) pool.allocated(sid)[10] = 87;
        },
        setFrame: (sid, frame, flags) => {
            scene.events.push(`F:${frame >>> 0}:${flags}`);
            put(pool.allocated(sid), 36, frame >>> 0);
        },
        regionOwnership: (x, y) => {
            scene.events.push(`C:${floatToBits(x)}:${floatToBits(y)}`);
            if (scene.mutation === 3) pool.allocated(SOURCE)[10] = 87;
        },
        warnDuplicate: (sid, x, y) => scene.events.push(`W:${x >>> 0}:${y >>> 0}`),
        destroy: (sid, flags) => {
            scene.events.push(`X:${sid}:${flags}`);
            pool.allocated(sid)[11] |= 2;
            if (scene.mutation === 4) {
                const next = pool.allocated(61);
                next[40] = 0;
                next[10] = pool.slot(SOURCE)[10];
            }
        }
    });
    // 공통 장부·직접 Damageable·Factory 를 조립한다(공통 분배기에는 호출자가 등록한다).
    scene.build = (hooks = scene.hooks(), numbers = new FactoryPostPopTypes(), damageableHooks = null) => {
        const base = new SquidPostPop(pool, types, scene.baseState);
        const damageable = new RawCarrierPostPop(pool, types, scene.carrierState, damageableHooks ?? scene.damageableHooks(base));
        const factory = new RawFactoryPostPop(pool, scene.hash, types, frames, base, damageable, scene.state, hooks, numbers);
        return { base, damageable, factory };
    };
    // 이전 목록 메모리와 callback 뒤 후보 상태까지 원본 형식으로 기록한다.
    scene.output = () => {
        const lists = [scene.state.outposts, scene.state.residences].map((list) => `${list.count}:${list.entries.join(',')}`).join('|');
        const other = [60, 61].map((sid) => { const raw = pool.slot(sid); return `${raw[10]},${raw[11]},${raw[40]}`; }).join('|');
        return [scene.events.length === 0 ? '-' : scene.events.join(';'), hex(pool.slot(SOURCE)), lists, other];
    };
    return scene;
}

// 행의 입력 목록(11개 DWORD)이다. (2026-11-08 WB04-E61)
const inputOf = (row) => row[1].split(',').map((part) => Number(part) >>> 0);

test('FactoryParentCurrent: 일반 Factory 행(126 제외, 아웃포스트 제외)의 전체 출력이 가상 +0x20 분배로 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = factoryScene();
    let checked = 0;
    // 기존 입력·기대값과 행 순서를 변경하지 않는다.
    for (const row of rows) {
        const input = inputOf(row);
        if (input[0] !== 0 || input[1] === 126) continue;
        scene.prepare(input);
        const { base, factory } = scene.build();
        base.setBody('factory', factory);
        scene.baseState.depth += 1;
        base.postPopVirtual(SOURCE, input[2]);
        assert.deepEqual(scene.output(), row.slice(2), `행 ${checked}`);
        assert.equal(scene.baseState.depth, 0);
        checked += 1;
    }
    assert.ok(checked > 100, `일반 Factory 행 수 ${checked}`);
});

test('FactoryPostPop: 원본 10.78 498행(Factory·아웃포스트·동적 finder)의 전체 출력이 Activate 로 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = factoryScene();
    let checked = 0;
    // 두 입력 격자를 모두 재생한다.
    for (const row of rows) {
        assert.equal(row.length, 6);
        const input = inputOf(row);
        scene.prepare(input);
        const { base, factory } = scene.build();
        assert.ok(!base.handlesDerived(SOURCE));
        base.setBody('factory', factory);
        assert.ok(base.handlesDerived(SOURCE));
        base.activate(SOURCE, input[2]);
        assert.equal(scene.baseState.depth, 0);
        assert.deepEqual(scene.output(), row.slice(2), `행 ${checked}`);
        checked += 1;
    }
    assert.equal(checked, 498);
});

// 준비 후 조립해 한 번 실행하는 보조이다. depth 는 1 로 두고 factory.postPop 을 부른다. (2026-11-08 WB04-E61)
function runPost(input, { flags, mutate = () => {}, numbers, hooksEdit, damageableEdit } = {}) {
    const scene = factoryScene();
    scene.prepare(input);
    mutate(scene);
    const hooks = scene.hooks();
    if (hooksEdit) hooksEdit(hooks, scene);
    const built = scene.build(hooks, numbers, damageableEdit ? damageableEdit(scene) : null);
    scene.baseState.depth += 1;
    return { scene, ...built, run: () => built.factory.postPop(SOURCE, flags) };
}

test('FactoryParentCurrent: 최초가 아니고 프레임이 유효하면 예약·프레임·중복·지역 장치를 요구하지 않는다', () => {
    const t = runPost([0, 85, 0, 0, 0, 0, 0, 0, 0, 0, 0], { flags: 0, hooksEdit: (hooks) => { for (const key of Object.keys(hooks)) hooks[key] = undefined; } });
    t.base.setBody('factory', t.factory);
    t.base.postPopVirtual(SOURCE, 0);
    assert.deepEqual(t.scene.events, ['D:0']);
    assert.equal(t.scene.baseState.depth, 0);
});

test('FactoryParentCurrent: 두 new 실패는 미연결 생성자를 호출하지 않고 공통 부모까지 진행하며, 첫 확보 성공 뒤 생성자 누락은 앞선 효과를 보존한다', () => {
    const failed = runPost([0, 85, 1, 0, 0, 0, 0, 0, 0, 0, 0], { flags: 1, hooksEdit: (hooks) => { hooks.addRegular = undefined; hooks.setFrame = undefined; } });
    failed.run();
    assert.deepEqual(failed.scene.events, ['A:40', 'A:40', 'D:1']);
    assert.ok(failed.scene.alloc === 2 && failed.scene.baseState.depth === 0);
    const missing = runPost([0, 85, 1, 0, 0, 0, 0, 1, 0, 0, 0], { flags: 1, hooksEdit: (hooks) => { hooks.addRegular = undefined; } });
    assert.ok(throws(() => missing.run()));
    assert.deepEqual(missing.scene.events, ['A:40']);
    assert.ok(missing.scene.alloc === 1 && missing.scene.baseState.depth === 1);
});

test('FactoryParentCurrent: 프레임 장치 누락 전에 실행한 두 예약과 공통 깊이 감소를 되돌리지 않는다', () => {
    const t = runPost([0, 85, 1, 0, 0, 0, 7, 7, 0, 0, 0], { flags: 1, hooksEdit: (hooks) => { hooks.setFrame = undefined; } });
    t.base.setBody('factory', t.factory);
    assert.ok(throws(() => t.base.postPopVirtual(SOURCE, 1)));
    assert.deepEqual(t.scene.events, ['A:40', 'R:50:2:1090519040', 'A:40', 'R:50:5:1056964608', 'D:1']);
    assert.ok(t.scene.baseState.depth === 0 && get(t.scene.pool.slot(SOURCE), 36) === 7);
});

test('FactoryParentCurrent: 공통 부모가 바꾼 TYPE·레벨·boss 를 뒤 프레임/중복 분기가 다시 읽는다', () => {
    const t = runPost([0, 86, 0, 0, 0, 0, 7, 0, 0, 0, 0], {
        flags: 0,
        damageableEdit: (scene) => {
            const base = new SquidPostPop(scene.pool, scene.types, scene.baseState);
            scene.altBase = base;
            const hooks = scene.damageableHooks(base);
            hooks.base = (sid, flags) => {
                scene.events.push('D');
                scene.pool.allocated(sid)[10] = 87;
                scene.pool.allocated(sid)[11] = 1 << 5;
                scene.pool.allocated(60)[10] = 87;
                scene.state.boss = true;
                base.postPopBase(sid, flags);
            };
            return hooks;
        }
    });
    t.run();
    assert.deepEqual(t.scene.events, ['D', 'F:2:0', 'W:50:52', 'X:60:0']);
    assert.equal(t.scene.baseState.depth, 0);
});

test('FactoryParentCurrent: 중복 경고가 후보를 바꿔도 삭제 장치 누락 전의 효과를 보존하고, 일치 후보가 없으면 경고·삭제 장치를 선택하지 않는다', () => {
    const t = runPost([0, 85, 0, 0, 1, 0, 0, 0, 0, 0, 0], {
        flags: 0,
        hooksEdit: (hooks, scene) => {
            hooks.destroy = undefined;
            hooks.warnDuplicate = (sid) => { scene.events.push('W'); scene.pool.allocated(sid)[11] = 0x10; };
        }
    });
    assert.ok(throws(() => t.run()));
    assert.deepEqual(t.scene.events, ['D:0', 'W']);
    assert.ok(t.scene.pool.slot(60)[11] === 0x10 && t.scene.baseState.depth === 0);
    const none = runPost([0, 85, 0, 0, 1, 0, 0, 0, 0, 0, 0], {
        flags: 0,
        mutate: (scene) => { scene.pool.allocated(60)[10] = 86; },
        hooksEdit: (hooks) => { hooks.warnDuplicate = undefined; hooks.destroy = undefined; hooks.setFrame = undefined; hooks.regionOwnership = undefined; }
    });
    none.run();
    assert.deepEqual(none.scene.events, ['D:0']);
    assert.equal(none.scene.baseState.depth, 0);
});

test('FactoryParentCurrent: 두 가상 개수 호출이 각각 현재 TYPE 을 읽고 저장 프레임/첫 구간을 유지한다', () => {
    let calls = 0;
    const first = runPost([0, 85, 0, 0, 0, 0, 6, 0, 0, 0, 0], {
        flags: 0,
        hooksEdit: (hooks, scene) => {
            hooks.virtualCount = (sid) => {
                calls += 1;
                scene.events.push(`C${calls}`);
                const raw = scene.pool.allocated(sid);
                raw[10] = calls === 1 ? 86 : 87;
                put(raw, 36, 100);
                return calls === 1 ? 1 : 7;
            };
        }
    });
    first.run();
    assert.deepEqual(first.scene.events, ['D:0', 'C1', 'C2', 'F:2:0']);
    assert.ok(calls === 2 && get(first.scene.pool.slot(SOURCE), 36) === 2);
    // 두 번째 표의 개수와 첫 표의 시작을 더하며 콜백이 쓴 현재 프레임을 비교에 섞지 않는다.
    calls = 0;
    const second = runPost([0, 85, 0, 0, 0, 0, 6, 0, 0, 0, 0], {
        flags: 0,
        mutate: (scene) => {
            const codes = [0, 1, 2].map(() => ({ side: 0x41, variant: 0x41, flags: 0 }));
            for (let index = 0; index < 10; index += 1) codes.push({ side: 0x48, variant: 0x41, flags: index });
            scene.frames[87] = new FrameCodeTable(codes);
        },
        hooksEdit: (hooks, scene) => {
            hooks.setFrame = undefined;
            hooks.virtualCount = (sid) => {
                calls += 1;
                scene.pool.allocated(sid)[10] = calls === 1 ? 86 : 87;
                put(scene.pool.allocated(sid), 36, 100);
                return calls === 1 ? 1 : 7;
            };
        }
    });
    second.run();
    assert.ok(calls === 2 && get(second.scene.pool.slot(SOURCE), 36) === 100 && second.scene.events.length === 1 && second.scene.events[0] === 'D:0');
});

test('FactoryParentCurrent: 가상 개수 실패는 끝난 공통 효과를 지우지 않고 물리 TYPE 오류는 공통 억제 뒤 Count 에서만 나타난다', () => {
    const count = runPost([0, 85, 0, 0, 0, 0, 7, 0, 0, 0, 0], {
        flags: 0,
        hooksEdit: (hooks, scene) => { hooks.virtualCount = () => { scene.events.push('C'); throw new Error('명시 가상80 중단'); }; }
    });
    assert.ok(throws(() => count.run()));
    assert.deepEqual(count.scene.events, ['D:0', 'C']);
    assert.ok(count.scene.baseState.depth === 0 && get(count.scene.pool.slot(SOURCE), 36) === 7);
    const physical = runPost([0, 255, 0, 0, 0, 0, 7, 0, 0, 0, 0], { flags: 0 });
    assert.ok(throws(() => physical.run()));
    assert.deepEqual(physical.scene.events, ['D:0']);
    assert.equal(physical.scene.baseState.depth, 0);
    const numbers = new FactoryPostPopTypes();
    numbers.fixedFrame = 255;
    const fixed = new RawFactoryPostPop(physical.scene.pool, physical.scene.hash, physical.scene.types, physical.scene.frames, physical.base, physical.damageable, physical.scene.state, {}, numbers);
    physical.scene.baseState.depth += 1;
    fixed.postPop(SOURCE, 0);
    assert.ok(physical.scene.baseState.depth === 0 && get(physical.scene.pool.slot(SOURCE), 36) === 7);
});

test('FactoryGlobalsCurrent: 타이머 번호는 전체 DWORD 를 비교하고 생성 콜백이 바꾼 현재 값을 읽으며 독립 사본은 유지된다', () => {
    // BYTE TYPE 와 전체 DWORD 비교를 유지하며 생성 후 변경한 현재 값을 읽는다.
    let first = 341;
    let second = 342;
    const fixed = 127;
    const numbers = new FactoryPostPopTypes();
    numbers.currentFirstTimer = () => first;
    numbers.currentSecondTimer = () => second;
    numbers.currentFixedFrame = () => fixed;
    const t = runPost([0, 85, 1, 0, 0, 0, 0, 7, 0, 0, 0], { flags: 1, numbers });
    t.run();
    assert.deepEqual(t.scene.events, ['A:40', 'R:50:5:1056964608', 'D:1']);
    first = 85;
    second = 87;
    t.scene.events = [];
    t.scene.alloc = 0;
    t.scene.baseState.depth += 1;
    t.run();
    assert.deepEqual(t.scene.events, ['A:40', 'R:50:2:1090519040', 'A:40', 'R:50:5:1056964608', 'D:1']);
    // 첫 Regular 생성 콜백이 바꾼 두 번째 TYPE DWORD 를 다음 비교에서 읽는다.
    let changed = 86;
    const next = new FactoryPostPopTypes();
    next.currentFirstTimer = () => 85;
    next.currentSecondTimer = () => changed;
    next.currentFixedFrame = () => 127;
    const u = runPost([0, 85, 1, 0, 0, 0, 0, 7, 0, 0, 0], {
        flags: 1,
        numbers: next,
        hooksEdit: (hooks) => { const add = hooks.addRegular; hooks.addRegular = (sid, event, delay) => { add(sid, event, delay); if (event === 2) changed = 85; }; }
    });
    u.run();
    assert.ok(changed === 85 && u.scene.alloc === 3 && u.scene.events.length === 7 && u.scene.events[1] === 'R:50:2:1090519040' && u.scene.events[3] === 'R:50:1:1056964608' && u.scene.events.at(-1) === 'D:1');
    // 첫 할당 실패의 콜백 변경도 다음 TYPE 비교에서 보존한다.
    let secondFailed = 86;
    let calls = 0;
    const third = new FactoryPostPopTypes();
    third.currentFirstTimer = () => 85;
    third.currentSecondTimer = () => secondFailed;
    third.currentFixedFrame = () => 127;
    const v = runPost([0, 85, 1, 0, 0, 0, 0, 7, 0, 0, 0], {
        flags: 1,
        numbers: third,
        hooksEdit: (hooks, scene) => { hooks.reserveRegular = () => { scene.events.push('A'); calls += 1; if (calls === 1) { secondFailed = 85; return false; } return true; }; }
    });
    v.run();
    assert.deepEqual(v.scene.events, ['A', 'A', 'R:50:1:1056964608', 'A', 'R:50:5:1056964608', 'D:1']);
    assert.equal(v.scene.baseState.depth, 0);
    // 장기 getter 를 지정하지 않은 기존 독립 호출자는 생성 때 값 사본을 유지한다.
    const owned = new FactoryPostPopTypes();
    const w = runPost([0, 85, 1, 0, 0, 0, 0, 7, 0, 0, 0], { flags: 1, numbers: owned });
    owned.firstTimerFactory = 86;
    owned.secondTimer = 85;
    w.run();
    assert.ok(w.scene.alloc === 2 && w.scene.events[1] === 'R:50:2:1090519040' && w.scene.events[3] === 'R:50:5:1056964608' && w.scene.baseState.depth === 0);
});

test('FactoryGlobalsCurrent: 공통 콜백 뒤 현재 제외 TYPE 가 같으면 가상 개수/프레임 장치를 선택하지 않고, 제외 번호도 전체 DWORD 이다', () => {
    let fixed = 127;
    const numbers = new FactoryPostPopTypes();
    numbers.currentFirstTimer = () => 85;
    numbers.currentSecondTimer = () => 87;
    numbers.currentFixedFrame = () => fixed;
    const t = runPost([0, 85, 0, 0, 0, 0, 7, 0, 0, 0, 0], {
        flags: 0,
        numbers,
        hooksEdit: (hooks) => { hooks.setFrame = undefined; hooks.virtualCount = () => { throw new Error('현재 제외된 일반 Factory는 Count하지 않는다'); }; },
        damageableEdit: (scene) => {
            const base = new SquidPostPop(scene.pool, scene.types, scene.baseState);
            const hooks = scene.damageableHooks(base);
            const common = hooks.base;
            hooks.base = (sid, flags) => { common(sid, flags); fixed = 85; };
            return hooks;
        }
    });
    t.run();
    assert.ok(t.scene.events.length === 1 && t.scene.events[0] === 'D:0' && get(t.scene.pool.slot(SOURCE), 36) === 7 && t.scene.baseState.depth === 0);
    // 제외 번호도 BYTE 로 좁히지 않으며 다음 helper 호출에서 현재 DWORD 를 다시 읽는다.
    let reads = 0;
    let wide = 341;
    const keep = new FactoryPostPopTypes();
    keep.currentFirstTimer = () => 85;
    keep.currentSecondTimer = () => 87;
    keep.currentFixedFrame = () => wide;
    const u = runPost([0, 85, 0, 0, 0, 0, 7, 0, 0, 0, 0], { flags: 0, numbers: keep, hooksEdit: (hooks) => { hooks.virtualCount = () => { reads += 1; return 0; }; } });
    u.factory.adjustFrame(SOURCE);
    assert.ok(reads === 2 && get(u.scene.pool.slot(SOURCE), 36) === 0);
    wide = 85;
    put(u.scene.pool.allocated(SOURCE), 36, 7);
    u.factory.adjustFrame(SOURCE);
    assert.ok(reads === 2 && get(u.scene.pool.slot(SOURCE), 36) === 7);
});

test('RawFactoryPostPop: 상태 설정은 같은 풀의 Level 저장 뒤 표시 갱신을 호출하고, 프레임 훅은 같은 풀의 SquidFrame 만 받는다', () => {
    const t = runPost([0, 85, 0, 0, 0, 0, 7, 0, 0, 0, 0], { flags: 0 });
    const updates = [];
    const hooks = { ...t.scene.hooks(), updateState: (sid) => updates.push(sid) };
    const built = t.scene.build(hooks);
    const level = { pool: t.scene.pool, set: (sid, value) => { t.scene.pool.allocated(sid)[11] = (value << 5) & 0xff; } };
    built.factory.setState(level, SOURCE, 3);
    assert.ok(updates.length === 1 && t.scene.pool.slot(SOURCE)[11] === 3 << 5);
    assert.ok(throws(() => built.factory.setState({ pool: new SidPool(24000, { server: false }), set: () => {} }, SOURCE, 3)));
    const calls = [];
    const wired = makeFactoryPostPopHooks(t.scene.pool, { pool: t.scene.pool, set: (sid, value, flags) => calls.push([sid, value, flags]) }, {});
    wired.setFrame(SOURCE, 4, 0);
    assert.deepEqual(calls, [[SOURCE, 4, 0]]);
    assert.ok(throws(() => makeFactoryPostPopHooks(t.scene.pool, { pool: new SidPool(24000, { server: false }) }, {})));
});
