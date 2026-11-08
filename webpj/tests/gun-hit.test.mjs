// 총 피해 시각/전달 raw-gun-hit.js(RawGunHit)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: gunhit-x86.tsv 의 `originals` 1029행 — 본체(0 Time / 1·4 SetTime / 2 Hit / 3 Apply)·저장 WORD·현재 시각 double 비트·DPS float 비트·표적 SID/상태·특수 flyer DPS·표적 genus →
//    외부 효과 순서(N 공간 통지 / D 가상 피해[표적·피해·소유자·발사 SID]), 발사 슬롯 50바이트 raw, helper 결과 double 버퍼. `originalCD` 행은 10.72 라 사용하지 않는다.
//  - 계약: 실제 피해 처리기 누락을 성공/시각 갱신으로 우회하지 않는다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E112: 총 피해 시각/전달 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, bitsToFloat } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { RawGunHit, makeMissileGunHitHooks } from '../www/core/raw-gun-hit.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E112)
const allRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'gunhit-x86.tsv'));
const rows = allRows ? allRows.filter((row) => row[0] === 'originals') : [];
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 예외가 나는지 본다. (2026-11-08 WB04-E112)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 원본 입력의 현재 풀/타입/시각과 외부 가상 피해 관찰을 보관하는 장면이다. (2026-11-08 WB04-E112)
function makeScene() {
    const scene = {
        pool: new SidPool(24000, { server: false }),
        types: Array.from({ length: 188 }, () => ({ flags2: 0, group: 0 })),
        hitTypes: Array.from({ length: 188 }, () => ({ damagePerSecond: 0, specialFlyerDamage: false, flyerDamagePerSecond: 0 })),
        output: new Uint8Array(8),
        events: [],
        now: 0
    };
    // 실제 입력 SID 를 미리 확보한다.
    for (let id = 5; id <= 60; id += 1) assert.equal(scene.pool.allocate(2), id);
    // 같은 실제 가상 경계의 인자와 순서를 기록한다.
    scene.hooks = {
        notifySurface: (firing) => scene.events.push(`N:${firing}`),
        damage: (target, damage, owner, firing) => scene.events.push(`D:${target}:${damage}:${owner}:${firing}`)
    };
    // independent 행의 숫자 입력으로 실제 필드를 준비한다.
    scene.prepare = (row) => {
        assert.equal(row.length, 12);
        scene.events = [];
        const view = new DataView(new ArrayBuffer(8));
        view.setBigUint64(0, BigInt(row[3]), true);
        scene.now = view.getFloat64(0, true);
        // helper 결과의 이전 바이트도 native 와 같은 패턴으로 유지한다.
        for (let index = 0; index < scene.output.length; index += 1) scene.output[index] = index * 7 + 3;
        // 원본의 raw 발사/표적 padding 전체를 보존한다.
        for (const [sid, type] of [[50, 128], [60, 71]]) {
            const raw = scene.pool.physical(sid);
            for (let index = 0; index < raw.length; index += 1) raw[index] = (index * 11 + 5) & 0xff;
            put(raw, 0, sid === 50 ? 0x11318000 : 0x11318100);
            raw[10] = type;
            raw[11] = sid === 60 ? Number(row[6]) : 0;
            raw[34] = 1;
            if (sid === 50) put(raw, 12, Number(row[2]), 2);
        }
        scene.types[128].group = 1;
        scene.types[128].flags2 = 0;
        scene.types[71].flags2 = Number(row[8]);
        scene.hitTypes[128].damagePerSecond = bitsToFloat(Number(row[4]));
        scene.hitTypes[128].specialFlyerDamage = Number(row[7]) !== 0;
        scene.hitTypes[128].flyerDamagePerSecond = 16;
    };
    // 각 실제 body 와 동일한 가상 경계를 실행한다.
    scene.run = (row) => {
        const logic = new RawGunHit(scene.pool, scene.types, scene.hitTypes, scene.hooks);
        switch (Number(row[1])) {
        case 0: {
            const view = new DataView(scene.output.buffer);
            view.setFloat64(0, logic.time(50), true);
            break;
        }
        case 1: case 4: logic.setTime(50, scene.now); break;
        case 2: logic.hit(50, Number(row[5]), scene.now); break;
        case 3: logic.apply(50, Number(row[5]), 17); break;
        default: throw new Error('GunHit fixture body 오류');
        }
        return [scene.events.join(';'), hex(scene.pool.slot(50)), hex(scene.output)];
    };
    return scene;
}

test('GunHit: 원본 10.78 1029입력의 Time/SetTime/Hit/Apply 효과 순서·발사 슬롯 raw·helper double 결과가 같다', { skip: rows.length === 0 && '원본 fixture 없음' }, () => {
    const scene = makeScene();
    let count = 0;
    // 저장된 실제 원본 관찰을 직접 소비한다.
    for (const row of rows) {
        scene.prepare(row);
        const actual = scene.run(row);
        assert.deepEqual(actual, row.slice(9), `행 ${count}: body ${row[1]} ${row.slice(2, 9).join(',')}`);
        count += 1;
    }
    assert.equal(count, 1029);
});

test('GunHit: 실제 피해 처리기 누락을 성공/시각 갱신으로 우회하지 않으며 Missile 연결과 범위 오류를 확인한다', { skip: rows.length === 0 && '원본 fixture 없음' }, () => {
    const scene = makeScene();
    scene.prepare(rows[0]);
    const logic = new RawGunHit(scene.pool, scene.types, scene.hitTypes, {});
    const before = get(scene.pool.slot(50), 12, 2);
    assert.ok(throws(() => logic.apply(50, 60, 17)));
    assert.equal(get(scene.pool.slot(50), 12, 2), before);
    assert.deepEqual(scene.events, []);
    assert.ok(throws(() => logic.setTime(50, 3.5)));
    assert.ok(throws(() => new RawGunHit({ edition: 'cd1072' }, scene.types, scene.hitTypes, {})));
    assert.ok(throws(() => new RawGunHit(scene.pool, scene.types, scene.hitTypes.slice(0, 10), {})));
    const full = new RawGunHit(scene.pool, scene.types, scene.hitTypes, scene.hooks);
    assert.equal(full.pool, scene.pool);
    assert.ok(throws(() => full.time(0)));
    assert.ok(throws(() => full.time(24000)));
    assert.ok(throws(() => full.setTime(50, Number.NaN)));
    // Missile 연결: hit/refresh 가 현재 state.now 로 같은 몸체를 부른다.
    const state = { now: 7.5 };
    const hooks = makeMissileGunHitHooks(full, state, { keep: 1 });
    assert.equal(hooks.keep, 1);
    hooks.refresh(50);
    assert.equal(full.time(50), 7.5);
    state.now = 8.25;
    hooks.refresh(50);
    assert.equal(full.time(50), 8.25);
    scene.events.length = 0;
    hooks.hit(50, 0);
    assert.deepEqual(scene.events, ['N:50']);
});
