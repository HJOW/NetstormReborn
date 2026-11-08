// 사제 피격 raw-priest-damage.js(RawPriestDamage·PriestDamageState·makePriestGunHitHooks·makePriestDamageWorldHooks)와 burning 판정 raw-damageable-burning.js(RawDamageableBurning·DamageableBurningState)를
// 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: priestdamage-x86.tsv 10.78(originals) 1712행 — 본문 종류(0 레벨 보정 / 1 사제 Hit / 2 burning 갱신)별 입력(HP·피해·rank·소유자·무적·권한·회복 사건·방향·damageEffect·burning 조회/생성 결과·checking·타입·최대 HP)
//    → 반환, 경계 호출 순서(B/A/X/S burning, C/F/N/U/R 피격), 슬롯 50 전체 raw, 보정 결과 4바이트. CD 10.72 행은 웹 범위(10.78) 밖이다.
//  - 계약: PriestDamage_MissingNonzeroBurningRejected(자연 zero 는 성공, 누락한 nonzero 생성 몸체 거부)·ActualRegularKernelAndDependChain(실제 Regular 취소와 HP1 감시 쌍 통지/삭제)·SharedBossRecheckedAfterRefresh(표시 갱신 뒤 권한 재조회).
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E67: 사제 피격과 burning 판정 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { GameRandom } from '../www/core/random.js';
import { ScrambledSpStore } from '../www/core/sp-store.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { RawSquidDestroy } from '../www/core/squid-destroy.js';
import { SquidFactory } from '../www/core/squid-factory.js';
import { SquidPostPopState } from '../www/core/squid-postpop-state.js';
import { SquidReward, SquidRewardState } from '../www/core/squid-reward.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { Kernel } from '../www/core/kernel.js';
import { SquidProcessHost, SquidProcessState } from '../www/core/squid-process.js';
import { DependFormHost } from '../www/core/depend-form-host.js';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { PRIEST_TYPE, PRIEST_VTABLE } from '../www/core/raw-priest-owner.js';
import { RawPriestState, PriestHitPointMode } from '../www/core/raw-priest-state.js';
import { RawDamageableBurning, DamageableBurningState } from '../www/core/raw-damageable-burning.js';
import { RawPriestDamage, PriestDamageState, makePriestGunHitHooks, makePriestDamageWorldHooks } from '../www/core/raw-priest-damage.js';

// 원본 관측 10.78 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E67)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'priestdamage-x86.tsv'))?.filter((row) => row[0] === 'originals') ?? null;

// 원본 관찰 피격자 SID, 판본당 독립 행 수이다. (2026-11-08 WB04-E67)
const SOURCE = 50;
const ROWS = 1712;

// 예외가 나는지 본다. (2026-11-08 WB04-E67)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E67)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 부호 있는 정수 문자열을 DWORD 로 바꾼다. (2026-11-08 WB04-E67)
const dword = (text) => parseInt(text, 10) >>> 0;

// 188칸 TYPE 입력이다. 내장 Form 네 생성자는 선택 인자로 채운다. (2026-11-08 WB04-E67)
const damageTypes = (forms = false) => Array.from({ length: 188 }, (_, number) => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0, damageEffect: 0, ...(forms && [1, 2, 3, 6].includes(number) ? { constructorAddress: typeConstructorAddress(number) } : {}) }));

// 현재 풀/실제 HP setter 와 명시 관찰 경계의 순서를 공유하는 피격 장면이다. (2026-11-08 WB04-E67)
class DamageScene {
    constructor() {
        this.pool = new SidPool(24000, { server: false });
        this.types = damageTypes();
        this.frames = this.types.map(() => new FrameCodeTable([{ side: 65, variant: 0, flags: 0 }]));
        this.store = new ScrambledSpStore(new GameRandom(), () => 1);
        this.book = new SquidPostPopState();
        this.hpMode = new SquidRewardState();
        this.spots = new Uint8Array(65536);
        this.mode = new PriestHitPointMode();
        this.damageState = new PriestDamageState();
        this.burningState = new DamageableBurningState();
        this.output = new Uint8Array(4);
        this.events = [];
        this.row = null;
        // 이후 독립 입력들이 같은 원본 번호를 쓰도록 SID 50 까지 확보한다.
        for (let id = 5; id <= SOURCE; id += 1) assert.equal(this.pool.allocate(2), id);
    }

    // 입력 숫자를 그대로 현재 타입/HP/rank/프레임에 반영한다.
    prepare(value) {
        assert.equal(value.length, 23);
        this.row = value;
        this.events = [];
        this.types = damageTypes();
        const type = Number(value[17]);
        this.types[type].maxHitPoints = parseInt(value[18], 10) | 0;
        this.types[type].flags1 = 0x69012;
        this.types[type].flags2 = 0x210000;
        this.types[type].damageEffect = parseInt(value[13], 10) | 0;
        this.frames = this.types.map(() => new FrameCodeTable([{ side: 65, variant: 0, flags: 0 }]));
        this.frames[type] = new FrameCodeTable([{ side: 65 + Number(value[12]), variant: 0, flags: 0 }]);
        this.hp = new SquidReward(this.pool, this.types, this.book, this.hpMode, this.store);
        this.hpState = new RawPriestState(this.pool, this.hp, this.types, this.spots);
        this.mode.authority = Number(value[10]) !== 0;
        this.mode.recoveryAllowed.fill(true);
        this.damageState.invulnerable = Number(value[9]) !== 0 ? 1 : 0;
        this.damageState.boss = this.mode.authority;
        this.damageState.levelCorrectionDisabled = Number(value[8]) !== 0;
        this.damageState.levels.fill(0);
        this.damageState.levels[Number(value[6])] = parseInt(value[4], 10);
        this.damageState.levels[Number(value[7])] = parseInt(value[5], 10);
        this.burningState.checking = Number(value[16]) !== 0 ? 1 : 0;
        const raw = this.pool.allocated(SOURCE);
        // raw padding 도 native 의 독립 입력 패턴과 동일하게 준비한다.
        for (let index = 0; index < raw.length; index += 1) raw[index] = (index * 17 + 3) & 0xff;
        put(raw, 0, PRIEST_VTABLE);
        put(raw, 4, 0, 2);
        put(raw, 6, 0, 2);
        raw[10] = type;
        raw[11] = 0;
        raw[40] = 0;
        raw[34] = Number(value[7]);
        put(raw, 14, floatToBits(20.75));
        put(raw, 18, floatToBits(21.9));
        put(raw, 26, dword(value[2]));
        put(raw, 36, 0);
        put(this.output, 0, dword(value[3]));
    }

    // 원본 가상 Unpop/Repop 관찰 시점의 현재 HP/좌표를 기록한다.
    space(marker, sid, flags) {
        const raw = this.pool.slot(sid);
        this.events.push(`${marker}:${sid}:${flags}:${this.hpState.currentHitPoints(sid)}:${get(raw, 14)}:${get(raw, 18)}`);
    }

    // nonzero burning 의 명시 조회/생성/삭제/소리 경계이다. 실제 파생 생성 완료로 세지 않는다.
    burningHooks() {
        return {
            find: (sid, type, index) => {
                const result = Number(this.row[14]) && !index ? 90 : 0;
                this.events.push(`B:${sid}:${type}:${index}:${result}`);
                return result;
            },
            create: (sid) => { this.events.push(`A:${sid}`); return Number(this.row[15]) ? 90 : 0; },
            destroy: (sid, flags) => { this.events.push(`X:${sid}:${flags}`); },
            bulfSound: (sid, x, y) => { this.events.push(`S:${sid}:${floatToBits(x)}:${floatToBits(y)}`); }
        };
    }

    // native 경계와 동일한 회복/프레임/소유자 관찰 및 실제 HP setter/burning 을 연결한다.
    priestHooks(burning) {
        return {
            hasRegenEvent: () => Number(this.row[11]) !== 0,
            cancelEvent: (sid, event) => { this.events.push(`C:${sid}:${event}`); },
            defaultFrame: () => 0,
            setFrame: (sid, frame, flags) => {
                this.events.push(`F:${sid}:${frame}:${flags}`);
                put(this.pool.allocated(sid), 36, frame >>> 0);
            },
            setHitPoints: (sid, value) => {
                this.hpState.setHitPoints(sid, value, this.mode, { unpop: (target, flags) => this.space('U', target, flags), repop: (target, flags) => this.space('R', target, flags) });
            },
            ownerDamageNotice: (owner, sid, attacker) => { this.events.push(`N:${owner}:${sid}:${attacker}`); },
            damageableRefresh: (sid) => { burning.refresh(sid); }
        };
    }

    // 반환/raw 전체/관찰 순서/보정 buffer 를 원본과 같은 형식으로 출력한다.
    run() {
        let result = '-';
        const burning = new RawDamageableBurning(this.pool, this.types, this.hp, this.burningState, this.burningHooks());
        const priest = new RawPriestDamage(this.pool, this.types, this.frames, this.damageState, this.priestHooks(burning));
        switch (Number(this.row[1])) {
        case 0:
            put(this.output, 0, RawPriestDamage.adjust(parseInt(this.row[3], 10), Number(this.row[6]), Number(this.row[7]), this.damageState) >>> 0);
            break;
        case 1:
            result = priest.hit(SOURCE, parseInt(this.row[3], 10), Number(this.row[6]), 60) ? '1' : '0';
            break;
        case 2:
            result = String(burning.refresh(SOURCE));
            break;
        default:
            throw new Error('PriestDamage fixture body 오류');
        }
        return [result, this.events.join(';'), hex(this.pool.slot(SOURCE)), hex(this.output)];
    }
}

test('PriestDamage: 원본 10.78 1712행의 레벨 보정·사제 Hit·HP setter·burning 판정과 경계 호출 순서·슬롯 raw 가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = new DamageScene();
    let count = 0;
    // 기대값은 저장된 native 관찰만 사용한다.
    for (const row of rows) {
        scene.prepare(row);
        assert.deepEqual(scene.run(), row.slice(19), `행 ${count}: 본문 ${row[1]}`);
        count += 1;
    }
    assert.equal(count, ROWS);
    assert.equal(scene.store.initialized, false);
});

test('PriestDamage: 자연 zero burning 은 성공하고 누락한 nonzero burning 생성 몸체는 명시적으로 거부한다', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = new DamageScene();
    const row = [...rows[0]];
    row[1] = '2';
    row[2] = '99';
    row[13] = '0';
    scene.prepare(row);
    const zero = new RawDamageableBurning(scene.pool, scene.types, scene.hp, scene.burningState, {});
    assert.equal(zero.refresh(SOURCE), 0);
    row[13] = '1';
    scene.prepare(row);
    const missing = new RawDamageableBurning(scene.pool, scene.types, scene.hp, scene.burningState, {});
    assert.ok(throws(() => missing.refresh(SOURCE)));
    assert.ok(throws(() => new RawDamageableBurning(new SidPool(24000, { server: false }), scene.types, scene.hp, scene.burningState, {})));
    assert.ok(throws(() => new RawDamageableBurning(scene.pool, scene.types.slice(1), scene.hp, scene.burningState, {})));
});

test('PriestDamage: 실제 Regular/Kernel 취소와 HP1 사제의 감시 쌍 통지/삭제를 같은 풀에서 연결한다', () => {
    const pool = new SidPool(24000, { server: false });
    const types = damageTypes(true);
    types[PRIEST_TYPE].constructorAddress = typeConstructorAddress(PRIEST_TYPE);
    types[PRIEST_TYPE].maxHitPoints = 200;
    types[PRIEST_TYPE].flags1 = 0x69012;
    types[PRIEST_TYPE].flags2 = 0x210000;
    const frames = types.map(() => new FrameCodeTable([{ side: 65, variant: 0, flags: 0 }]));
    const factory = new SquidFactory(pool, types);
    const victim = factory.create(PRIEST_TYPE, 2);
    const firstWatcher = factory.create(90, 2);
    const secondWatcher = factory.create(91, 2);
    // 아직 공간 Pop 을 주장하지 않는 독립 live 입력으로 사제 피격 몸체를 준비한다.
    const raw = pool.allocated(victim);
    raw[11] = 0;
    raw[34] = 1;
    put(raw, 26, 150);
    put(raw, 14, floatToBits(20.75));
    put(raw, 18, floatToBits(21.9));
    put(raw, 36, 0);
    const spots = new Uint8Array(65536);
    const destroy = new RawSquidDestroy(pool, new SquidUnpop(pool, new SquidHash(), spots), types);
    const kernel = new Kernel(16);
    const host = new SquidProcessHost(pool, types, kernel, destroy, new SquidProcessState());
    const first = host.addRegular(victim, 0x25c, Math.fround(0.1));
    const second = host.addRegular(victim, 0x25d, Math.fround(0.2));
    assert.ok(first && second && kernel.size === 2);
    const firstForm = first.form;
    const secondForm = second.form;
    let notices = 0;
    const depends = new DependFormHost(pool, destroy, {
        notice: (watcher, watched) => {
            assert.ok((watcher === firstWatcher || watcher === secondWatcher) && watched === victim);
            notices += 1;
        }
    });
    const firstPair = depends.create(victim, firstWatcher);
    const secondPair = depends.create(victim, secondWatcher);
    assert.ok(depends.has(victim, firstWatcher) && depends.has(victim, secondWatcher));
    const hp = new SquidReward(pool, types, new SquidPostPopState(), new SquidRewardState(), new ScrambledSpStore(new GameRandom(), () => 1));
    const hpState = new RawPriestState(pool, hp, types, spots);
    const mode = new PriestHitPointMode();
    mode.recoveryAllowed.fill(true);
    const burning = new RawDamageableBurning(pool, types, hp, new DamageableBurningState(), {});
    let framesChanged = 0;
    let ownerNotices = 0;
    let spaces = 0;
    let hooks = {
        defaultFrame: () => 0,
        setFrame: (sid, frame, flags) => { assert.ok(sid === victim && frame === 0 && flags === 0); framesChanged += 1; },
        ownerDamageNotice: (owner, sid, attacker) => { assert.ok(owner === 1 && sid === victim && attacker === firstWatcher); ownerNotices += 1; }
    };
    hooks = makePriestDamageWorldHooks(pool, host, hpState, mode, {
        unpop: (sid, flags) => { assert.ok(sid === victim && flags === 0); spaces += 1; },
        repop: (sid, flags) => { assert.ok(sid === victim && flags === 0); spaces += 1; }
    }, burning, hooks);
    const damage = new RawPriestDamage(pool, types, frames, new PriestDamageState(), hooks, depends);
    const gun = makePriestGunHitHooks(pool, damage, {});
    gun.damage(victim, 200, 2, firstWatcher);
    assert.ok(hpState.currentHitPoints(victim) === 1 && kernel.size === 0 && framesChanged === 1 && ownerNotices === 1 && spaces === 2 && notices === 2);
    assert.ok(!depends.has(victim, firstWatcher) && !depends.has(victim, secondWatcher));
    assert.equal(get(pool.slot(victim), 6, 2), 0);
    // 실제 쌍 삭제와 Regular 삭제가 각자의 SID 수명을 끝냈는지 확인한다.
    for (const sid of [firstForm, secondForm, firstPair.watchedForm, firstPair.watcherForm, secondPair.watchedForm, secondPair.watcherForm]) assert.equal(pool.slot(sid)[11] & 3, 3);
    assert.ok(destroy.preDepth === 0 && destroy.postDepth === 0);
    // 비사제 표적은 fallback 이 없으면 거부하고 있으면 그 분배로 넘긴다.
    const other = factory.create(90, 2);
    assert.ok(throws(() => gun.damage(other, 1, 1, victim)));
    const seen = [];
    makePriestGunHitHooks(pool, damage, { keep: 1 }, (sid, amount, owner, attacker) => seen.push([sid, amount, owner, attacker])).damage(other, 5, 2, victim);
    assert.deepEqual(seen, [[other, 5, 2, victim]]);
    assert.ok(throws(() => makePriestGunHitHooks(new SidPool(24000, { server: false }), damage, {})));
});

test('PriestDamage: 표시 갱신 뒤 권한을 다시 읽어 HP1 사제의 의존자 해제 여부를 정한다', { skip: !rows && '원본 fixture 없음' }, () => {
    let boss = true;
    let nextBoss = false;
    const scene = new DamageScene();
    const row = [...rows[0]];
    row[2] = '1';
    row[3] = '1';
    row[6] = '2';
    row[7] = '1';
    row[8] = '1';
    row[9] = '0';
    row[10] = '1';
    row[11] = '0';
    row[12] = '0';
    row[13] = '0';
    row[17] = '158';
    row[18] = '200';
    scene.prepare(row);
    scene.damageState.sharedBoss = () => boss;
    scene.mode.sharedAuthority = () => boss;
    const burning = new RawDamageableBurning(scene.pool, scene.types, scene.hp, new DamageableBurningState(), {});
    const hooks = scene.priestHooks(burning);
    let refreshes = 0;
    hooks.damageableRefresh = (sid) => { assert.equal(sid, SOURCE); refreshes += 1; boss = nextBoss; };
    const hit = new RawPriestDamage(scene.pool, scene.types, scene.frames, scene.damageState, hooks);
    assert.ok(hit.hit(SOURCE, 1, 2, 60) && !boss && scene.hpState.currentHitPoints(SOURCE) === 1);
    scene.damageState.boss = false;
    boss = true;
    nextBoss = true;
    assert.ok(!hit.hit(SOURCE, 1, 2, 60) && refreshes === 2);
});

test('PriestDamage: 연결 오류와 손상 입력은 첫 변경 전에 거부한다', () => {
    const scene = new DamageScene();
    const row = ['originals', '1', '150', '5', '0', '0', '2', '1', '0', '0', '1', '0', '0', '0', '0', '0', '0', '158', '200', '', '', '', '', '', ''];
    row.length = 23;
    scene.prepare(row);
    const hooks = scene.priestHooks(new RawDamageableBurning(scene.pool, scene.types, scene.hp, scene.burningState, scene.burningHooks()));
    // 다른 풀의 감시 호스트는 구성 때 거부한다.
    assert.ok(throws(() => new RawPriestDamage(scene.pool, scene.types, scene.frames, scene.damageState, hooks, { pool: new SidPool(24000, { server: false }) })));
    assert.ok(throws(() => new RawPriestDamage(scene.pool, scene.types, scene.frames.slice(1), scene.damageState, hooks)));
    const damage = new RawPriestDamage(scene.pool, scene.types, scene.frames, scene.damageState, hooks);
    assert.ok(throws(() => damage.hit(0, 1, 2, 60)));
    assert.ok(throws(() => damage.hit(24000, 1, 2, 60)));
    const raw = scene.pool.allocated(SOURCE);
    const before = hex(raw);
    raw[11] = 2;
    assert.ok(throws(() => damage.hit(SOURCE, 1, 2, 60)));
    raw[11] = 1;
    assert.ok(throws(() => damage.hit(SOURCE, 1, 2, 60)));
    raw[11] = 0;
    assert.equal(hex(raw), before);
    // 필수 피격 하위 몸체가 없으면 어떤 효과도 내지 않는다.
    assert.ok(throws(() => new RawPriestDamage(scene.pool, scene.types, scene.frames, scene.damageState, {}).hit(SOURCE, 1, 2, 60)));
    assert.deepEqual(scene.events, []);
    // 범위 밖 rank 는 거부하고 소유자 0 은 보정을 생략한다.
    scene.damageState.levels[1] = 24;
    scene.damageState.levels[2] = 0;
    assert.ok(throws(() => RawPriestDamage.adjust(100, 2, 1, scene.damageState)));
    assert.equal(RawPriestDamage.adjust(100, 0, 1, scene.damageState), 100);
});
