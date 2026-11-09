// 일반 자산 피격 raw-damageable-hit.js(RawDamageableHit·DamageableHitState·makeDamageableGunHitHooks·makeDamageableHitBurningHooks)를 원본 10.78 관측과 cpppj 계약으로 대조한다(cpppj `DamageableHitTests`).
//  - 원본 관측: 일반 피격 독립 PE 관측의 originals 2923행 — 입력(Hit/BaseHit·HP·피해·두 레벨·공격자/피해자 소유자·보정 차단·무적·서버·상태·extra·가상 방어·HAS_HP·다이스 genus·공격자 TYPE flags2·damageEffect·
//    burning 조회/생성 결과·checking·local 소유자·지면 genus·지면 번호·주 섬·실제 시각·경고 마감) → 반환, 효과 순서(I 지면 조회/W 실제 시각/V 경고 사건/S 소리/F·L 알림/U unrealize/R 방어/D 삭제/N 소유자 통지/B burning 조회/A 생성/X 삭제),
//    피해자 50바이트 raw, 사망 전역 8바이트(피해자 SID·공격자 소유자), 경고 마감 double 8바이트. HP 최대값과 burning 판정은 실제 SquidReward·RawDamageableBurning 몸체를 쓴다. originalCD 행(10.72)은 범위 밖이다.
//  - 계약: DamageableHit_MissingDefenseAndDeleteRejected(가상 방어·삭제 누락은 오류이며 사망 전역을 쓰지 않음), DamageableHit_CurrentVtableDispatchAndFallback(10.78 판: 현재 raw vtable 의 BaseHit 진입 분배와 다른 vtable 의 fallback).
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E210: 일반 자산 피격 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { GameRandom } from '../www/core/random.js';
import { ScrambledSpStore } from '../www/core/sp-store.js';
import { SquidPostPopState } from '../www/core/squid-postpop-state.js';
import { SquidReward, SquidRewardState } from '../www/core/squid-reward.js';
import { PriestDamageState } from '../www/core/raw-priest-damage.js';
import { RawDamageableBurning, DamageableBurningState } from '../www/core/raw-damageable-burning.js';
import { RawDamageableHit, DamageableHitState, makeDamageableGunHitHooks, makeDamageableHitBurningHooks } from '../www/core/raw-damageable-hit.js';

// 원본 관측 10.78 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-09 WB04-E210)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'damageablehit-x86.tsv'))?.filter((row) => row[0] === 'originals') ?? null;
const ROWS = 2923;
// 원본 관찰 피해자·공격자·지면 SID 와 풀 크기이다. (2026-11-09 WB04-E210)
const SOURCE = 50;
const ATTACKER = 60;
const ISLAND = 70;
const CAPACITY = 24000;

// 예외가 나는지 본다. (2026-11-09 WB04-E210)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-09 WB04-E210)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// uint64 십진 문자열의 비트 패턴을 double 로 읽는다. (2026-11-09 WB04-E210)
function doubleOf(text) {
    const view = new DataView(new ArrayBuffer(8));
    view.setBigUint64(0, BigInt(text), true);
    return view.getFloat64(0, true);
}

// double 의 비트 8바이트(리틀엔디언)이다. (2026-11-09 WB04-E210)
function doubleBytes(value) {
    const bytes = new Uint8Array(8);
    new DataView(bytes.buffer).setFloat64(0, value, true);
    return bytes;
}

// 같은 raw/레벨/장부/HP/경고와 명시 하위 경계를 갖는 독립 장면이다(cpppj HitScene). (2026-11-09 WB04-E210)
class HitScene {
    constructor() {
        this.pool = new SidPool(CAPACITY, { server: false });
        this.types = [];
        this.book = new SquidPostPopState();
        this.hpMode = new SquidRewardState();
        this.store = new ScrambledSpStore(new GameRandom(), () => 1);
        this.levels = new PriestDamageState();
        this.state = new DamageableHitState(this.levels);
        this.burningState = new DamageableBurningState();
        this.row = null;
        this.events = [];
        // 같은 원본 SID 50/60/70 까지 실제 풀에 확보한다.
        for (let id = 5; id <= 70; id += 1) assert.equal(this.pool.allocate(2), id);
    }

    // 현재 독립 raw/타입/상태를 원본과 같은 저장 폭으로 초기화한다. (2026-11-09 WB04-E210)
    prepare(value) {
        assert.equal(value.length, 32);
        const n = (index) => Number(value[index]);
        this.row = value;
        this.events = [];
        this.types = Array.from({ length: 188 }, () => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0, damageEffect: 0 }));
        this.types[71].maxHitPoints = 300;
        this.types[71].flags1 = n(14) ? 0x10 : 0;
        this.types[71].flags2 = n(15) ? 0x400000 : 0;
        this.types[71].damageEffect = n(17) | 0;
        this.types[128].flags2 = n(16);
        this.types[90].flags2 = n(22);
        this.levels.invulnerable = n(9) ? 1 : 0;
        this.levels.levelCorrectionDisabled = n(8) !== 0;
        this.levels.levels.fill(0);
        this.levels.levels[n(6)] = n(4) | 0;
        this.levels.levels[n(7)] = n(5) | 0;
        this.state.server = n(10) !== 0;
        this.state.checking = n(20);
        this.state.localOwner = n(21) & 0xff;
        this.state.primaryIslands[n(7)] = n(24) | 0;
        this.state.warningTime = doubleOf(value[26]);
        this.state.killedVictim = 0;
        this.state.killerOwner = 0;
        this.burningState.checking = this.state.checking;
        const raw = this.pool.physical(SOURCE);
        // 주변 padding 과 HP 옆 필드도 같은 독립 패턴을 유지한다.
        for (let index = 0; index < raw.length; index += 1) raw[index] = (index * 17 + 3) & 0xff;
        put(raw, 0, 0x11319800);
        put(raw, 4, 0, 2);
        put(raw, 6, 0, 2);
        raw[10] = 71;
        raw[11] = n(11) & 0xff;
        raw[40] = n(12) & 0xff;
        raw[34] = n(7) & 0xff;
        put(raw, 14, floatToBits(20.75));
        put(raw, 18, floatToBits(21.9));
        put(raw, 26, n(2) >>> 0);
        put(raw, 36, 0);
        const attacker = this.pool.physical(ATTACKER);
        attacker.fill(0);
        attacker[10] = 128;
        const island = this.pool.physical(ISLAND);
        island.fill(0);
        island[10] = 90;
        put(island, 8, n(23), 2);
    }

    // burning 숫자 몸체에 원본과 같은 관찰 find/create/destroy 를 준다. (2026-11-09 WB04-E210)
    burningHooks() {
        const row = this.row;
        return {
            find: (sid, type, index) => {
                const found = Number(row[18]) && !index ? 90 : 0;
                this.events.push(`B:${sid}:${type}:${index}:${found}`);
                return found;
            },
            create: (sid) => { this.events.push(`A:${sid}`); return Number(row[19]) ? 90 : 0; },
            destroy: (sid, flags) => { this.events.push(`X:${sid}:${flags >>> 0}`); }
        };
    }

    // 명시 하위 경계의 인자/순서를 기록하며 일반 피해 몸체는 실제 구현을 실행한다. (2026-11-09 WB04-E210)
    hitHooks(burning) {
        const row = this.row;
        const hooks = {
            rejectDamage: (sid, attacker) => { this.events.push(`R:${sid}:${attacker}`); return Number(row[13]) !== 0; },
            destroy: (sid, flags) => { this.events.push(`D:${sid}:${flags >>> 0}`); },
            ownerDamageNotice: (owner, victim, attacker) => { this.events.push(`N:${owner}:${victim}:${attacker}`); },
            unrealize: (sid) => { this.events.push(`U:${sid}`); },
            islandAt: (x, y) => { this.events.push(`I:${floatToBits(x)}:${floatToBits(y)}`); return ISLAND; },
            realTime: () => { this.events.push(`W:${row[25]}`); return doubleOf(row[25]); },
            warningEvent: () => { this.events.push('V:12'); },
            warningSound: () => { this.events.push('S:islandUnderAttack.wav'); },
            warningText: () => { this.events.push('F:islandUnderAttack'); this.events.push('L:islandUnderAttack'); }
        };
        return makeDamageableHitBurningHooks(this.pool, burning, hooks);
    }

    // 실제 HP 최대값·burning·일반 피해 몸체를 같은 풀로 조립한다. (2026-11-09 WB04-E210)
    assemble() {
        const hp = new SquidReward(this.pool, this.types, this.book, this.hpMode, this.store);
        const burning = new RawDamageableBurning(this.pool, this.types, hp, this.burningState, this.burningHooks());
        return new RawDamageableHit(this.pool, this.types, this.state, this.hitHooks(burning));
    }

    // 실제 일반 피해를 실행한 뒤 원본과 같은 최종 필드를 직렬화한다. (2026-11-09 WB04-E210)
    run() {
        const hit = this.assemble();
        const row = this.row;
        const damage = parseInt(row[3], 10) | 0;
        const result = Number(row[1]) ? hit.hit(SOURCE, damage, Number(row[6]), ATTACKER) : hit.baseHit(SOURCE, damage, Number(row[6]), ATTACKER);
        const death = new Uint8Array(8);
        put(death, 0, this.state.killedVictim);
        put(death, 4, this.state.killerOwner);
        return [result ? '1' : '0', this.events.join(';'), hex(this.pool.slot(SOURCE)), hex(death), hex(doubleBytes(this.state.warningTime))];
    }
}

test('DamageableHit: 원본 10.78 2923입력의 반환·효과 순서·피해자 50바이트·사망 전역·경고 마감이 같다(PatchNativeBaseAndDerived)', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = new HitScene();
    let count = 0;
    // 저장된 고유 입력을 한 번씩만 대조한다.
    for (const row of rows) {
        scene.prepare(row);
        const actual = scene.run();
        assert.deepEqual(actual, row.slice(27), `행 ${count} body ${row[1]} 입력 ${row.slice(1, 27).join(',')}`);
        count += 1;
    }
    assert.equal(count, ROWS);
});

test('DamageableHit: 계약 — 가상 방어·삭제 누락은 정상 완료로 바꾸지 않고 사망 전역도 쓰지 않는다(MissingDefenseAndDeleteRejected)', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = new HitScene();
    const row = [...rows[0]];
    row[1] = '0'; row[2] = '1'; row[3] = '200'; row[10] = '1'; row[11] = '0'; row[12] = '0'; row[13] = '0';
    scene.prepare(row);
    const noDefense = new RawDamageableHit(scene.pool, scene.types, scene.state, {});
    assert.ok(throws(() => noDefense.baseHit(SOURCE, 200, 2, ATTACKER)));
    const noDelete = new RawDamageableHit(scene.pool, scene.types, scene.state, { rejectDamage: () => false });
    assert.ok(throws(() => noDelete.baseHit(SOURCE, 200, 2, ATTACKER)));
    assert.equal(scene.state.killedVictim, 0);
});

test('DamageableHit: 계약 — 현재 raw vtable 의 실제 BaseHit 진입으로 분배하고 다른 vtable 은 기존 fallback 을 보존한다(CurrentVtableDispatchAndFallback, 10.78)', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = new HitScene();
    const row = [...rows[0]];
    row[1] = '0'; row[2] = '150'; row[3] = '24'; row[4] = '0'; row[5] = '0'; row[10] = '1'; row[11] = '0'; row[12] = '0'; row[13] = '0';
    scene.prepare(row);
    const damage = scene.assemble();
    let fallbackCalls = 0;
    const hooks = makeDamageableGunHitHooks(damage, [{ vtable: 0x11319800, address: 0x4add10 }], {
        damage: (victim, amount, owner, attacker) => {
            assert.ok(victim === SOURCE && amount === 24 && owner === 2 && attacker === ATTACKER);
            fallbackCalls += 1;
        }
    });
    hooks.damage(SOURCE, 24, 2, ATTACKER);
    assert.ok(get(scene.pool.slot(SOURCE), 26) === 126 && fallbackCalls === 0);
    put(scene.pool.allocated(SOURCE), 0, 0xdead1234);
    hooks.damage(SOURCE, 24, 2, ATTACKER);
    assert.ok(fallbackCalls === 1 && get(scene.pool.slot(SOURCE), 26) === 126);
});

test('DamageableHit: 계약 — 섬+다리 genus(6) 지면의 섬 번호는 +8 WORD 대신 −1 이다(원본 관측에 없는 분기, cpppj 원문 계약)', { skip: !rows && '원본 fixture 없음' }, () => {
    // 경고 마감 0 과 실제 시각 5 초에서 주 섬 번호가 −1 이면 경고하고 0 이면 하지 않는다(지면 WORD 는 0).
    const fiveBits = new DataView(new ArrayBuffer(8));
    fiveBits.setFloat64(0, 5, true);
    for (const [primary, warned] of [['-1', true], ['0', false]]) {
        const scene = new HitScene();
        const row = [...rows[0]];
        row[1] = '1'; row[14] = '1'; row[15] = '0'; row[11] = '0'; row[12] = '0'; row[21] = row[7]; row[22] = '6'; row[23] = '0'; row[24] = primary;
        row[25] = fiveBits.getBigUint64(0, true).toString(); row[26] = '0';
        scene.prepare(row);
        scene.run();
        assert.equal(scene.events.includes('V:12'), warned, `주 섬 ${primary}`);
        assert.equal(scene.state.warningTime, warned ? 35 : 0, `주 섬 ${primary} 마감`);
    }
});

test('DamageableHit: 계약 — 10.72 풀·TYPE 표 크기·빈 분배 표·다른 풀의 burning 연결을 거부한다', () => {
    const levels = new PriestDamageState();
    const state = new DamageableHitState(levels);
    const pool = new SidPool(CAPACITY, { server: false });
    const types = Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0 }));
    assert.throws(() => new RawDamageableHit({ edition: 'cd1072' }, types, state, {}), TypeError);
    assert.throws(() => new RawDamageableHit(pool, types.slice(0, 171), state, {}), TypeError);
    assert.throws(() => new DamageableHitState({}), TypeError);
    const hit = new RawDamageableHit(pool, types, state, {});
    assert.throws(() => makeDamageableGunHitHooks(hit, [], {}), TypeError);
    assert.throws(() => makeDamageableHitBurningHooks(pool, { pool: new SidPool(CAPACITY) }, {}), TypeError);
    assert.throws(() => hit.baseHit(0, 1, 1, 0), RangeError);
});
