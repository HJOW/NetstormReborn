// 제단 몸체 raw-altar-sacrifice.js(RawAltarSacrifice)와 제단 정기 분기 raw-ritual-regular.js(runAltarRegular)·고유 삭제 raw-dais-deletion.js(RawDaisDeletion)를 원본 10.78 관측으로 대조한다.
//  - altarsacrifice-x86.tsv 의 originals 1,435행(모드 handle/count/escape/free/release/pre): 반환 비트·효과 순서(F 프레임·destroy·N/S 예약·A/G 음향·X 애니메이션·unpop/repop·C/owner/P 해방·E 효과·R 갱신)·
//    난수 상태·Player 결과·다섯 슬롯 raw(pre 모드는 마지막 공통 Damageable 진입 직전 경계의 raw)·제단 24 Form 조회 횟수가 같다.
//  - daisprogress-x86.tsv 의 originals 중 제단 정기 handle 1,056행(저항/탈출/희생 진행/최종 붕괴의 반환·효과·난수·Player 결과·다섯 슬롯 raw).
// cpppj AltarSacrificeTests/DaisProgressTests 의 장면(합성 자산·실제 FrameAdvance/Host)을 같은 순서로 옮겼다. 실제 게임·브라우저 실행이 아니다. (2026-11-13 WB04-E368: 7-P3 RawAltarSacrifice 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, floatToBits } from './raw-support.mjs';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { GameRandom } from '../www/core/random.js';
import { RegularProcess } from '../www/core/squid-process.js';
import { RawFrameAdvance } from '../www/core/raw-frame-advance.js';
import { DamageablePreDestroyState, RawDamageablePreDestroy } from '../www/core/raw-damageable-pre-destroy.js';
import { AltarSacrificeState, runAltarRegular } from '../www/core/raw-ritual-regular.js';
import { setRawSquidHitPoints } from '../www/core/raw-squid-word.js';
import { PATCH_DAIS_VTABLE } from '../www/core/raw-dais-content-notice.js';
import { RawAltarSacrifice, PATCH_ALTAR_VTABLE, makeAltarSacrificeHooks } from '../www/core/raw-altar-sacrifice.js';
import { VTABLE, hex, joinEffects, forbid, progressScene } from './dais-support.mjs';

// 원본 관측 10.78(originals) 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-13 WB04-E368)
const readOriginals = (name) => readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', name))?.filter((row) => row[0] === 'originals') ?? null;
const altarRows = readOriginals('altarsacrifice-x86.tsv');
const progressRows = readOriginals('daisprogress-x86.tsv');

// 마지막 Damageable 진입 경계에서 검사만 중단시키는 표식이다. 그 전체 삭제를 완료로 표시하지 않는다. (2026-11-13 WB04-E368)
class AltarBodyEnd extends Error {}

// 한 행의 장면을 만들고 실제 FrameAdvance/Damageable 전처리/제단 몸체를 같은 풀에 붙인다. kind 는 'altar'(altarsacrifice) 또는 'progress'(daisprogress handle)이다.
// 반환: { world, effects, soundStarts, altar, random, boundaryImages }. (2026-11-13 WB04-E368)
function altarScene(input, kind) {
    const isAltar = kind === 'altar';
    // 제단 관측은 다이스의 희생 소리 장치를 쓰지 않는다(실행되면 경계가 드러난다).
    const overrides = isAltar ? { reserveSound: forbid('희생소리확보'), loadSound: forbid('희생소리조회'), attachSound: forbid('희생소리부착'), wallSeconds: forbid('벽시계'), setSoundStart: forbid('지연시각') } : {};
    const { world, effects, soundStarts } = progressScene(input, overrides, { altarFrames: true, altarAssets165: isAltar });
    const { pool, slots } = world;
    const random = new GameRandom(0);
    random.setState(input.seed);
    const boundaryImages = [];
    const damageHooks = {
        collapse: forbid('상위Native전처리경계뒤붕괴'), explosion: forbid('상위Native전처리경계뒤폭발'), sound: forbid('상위Native전처리경계뒤소리'),
        releaseContained: forbid('상위Native전처리경계뒤일반해방'), base: forbid('상위Native전처리경계뒤공통body'),
        validateBase: isAltar
            ? (sid, flags) => {
                effects.push(`B:${sid},${flags}`);
                for (const slot of [5, 6, 7, 8, 10]) boundaryImages.push(hex(pool.slot(slot)));
                throw new AltarBodyEnd('경계');
            }
            : forbid('DaisProgress unexpected DamageablePre'),
    };
    const damageable = new RawDamageablePreDestroy(pool, world.spots, world.containedState, new DamageablePreDestroyState(), damageHooks);
    const advance = new RawFrameAdvance(pool, world.frames, world.frame);
    const altarState = new AltarSacrificeState();
    const bits = (value) => floatToBits(value);
    const hooks = {
        setFrame: world.hooks.setFrame,
        destroy: (sid, flags) => { effects.push(`destroy:${sid},${flags}`); },
        reserveShared: () => { effects.push('N:40'); return input.reserve !== 0; },
        addShared: (sid, event, payload) => { effects.push(`S:${event},${sid},${bits(payload)}`); },
        randomInt: (low, high) => ((low + random.next((high - low) >>> 0)) | 0),
        randomFloat: (low, high) => Math.fround(random.next(65536) * (1 / 65536) * Math.abs(high - low) + low),
        playAt: (x, y, name, flags) => { effects.push(`A:${bits(x)},${bits(y)},${name},${flags}`); },
        playGlobal: (name, a, b, c, d, e) => { effects.push(`G:${name},${a},${b},${c},${d},${e}`); },
        removeAnimation: (sid) => { effects.push(`X:${sid}`); },
        unpop: (sid, flags) => { effects.push(`unpop:${sid},${flags}`); },
        repop: (sid, flags) => { effects.push(`repop:${sid},${flags}`); },
        create: (type, flags) => {
            assert.deepEqual([type, flags], [158, 0]);
            effects.push('C:158,0,8');
            slots[8].fill(0);
            put(slots[8], 0, VTABLE);
            slots[8][10] = 158;
            slots[8][11] = 4;
            return 8;
        },
        setOwner: (sid, owner) => { effects.push(`owner:${sid},${owner}`); slots[sid][34] = owner; },
        pop: (sid, x, y, flags) => {
            effects.push(`P:${sid},${bits(x)},${bits(y)},${flags}`);
            put(slots[sid], 14, bits(x));
            put(slots[sid], 18, bits(y));
        },
        escapeEffect: (sid, seconds, flags) => { effects.push(`E:${sid},${bits(seconds)},${flags}`); },
        refreshOwner: (owner) => { effects.push(`R:${owner}`); },
        damageable: forbid('제단fixture의나머지Damageable사건'),
        ...(isAltar ? { count: (sid) => world.ritual.count(sid) } : {}),
    };
    const altar = new RawAltarSacrifice(world.pool, world.hash, world.types, world.frames, world.footprint, world.rune, world.ritual, world.carrier, world.dais, advance, world.host, damageable, world.state, altarState, hooks);
    return { world, effects, soundStarts, altar, random, boundaryImages, advance, damageable, altarState, hooks };
}

test('AltarSacrifice: 원본 10.78 altarsacrifice 1,435행의 반환 비트·효과 순서·난수·Player 결과·다섯 슬롯 raw·제단 24 Form 조회가 같다', { skip: !altarRows && '원본 fixture 없음' }, () => {
    let checked = 0;
    // 입력마다 새 풀/해시/Host 를 만들어 이전 행의 결과를 쓰지 않는다.
    for (const row of altarRows) {
        assert.equal(row.length, 7);
        const input = JSON.parse(row[1]);
        const { world, effects, altar, random, boundaryImages } = altarScene(input, 'altar');
        let returned = '-';
        if (input.mode === 'handle') returned = String(floatToBits(altar.handle(7, input.event, input.count, 9.25)));
        else if (input.mode === 'count') returned = String(altar.count(7));
        else if (input.mode === 'escape') altar.escapeFrame(5, input.count);
        else if (input.mode === 'free') altar.releasePriest(5);
        else if (input.mode === 'release') altar.releaseFromAltar(7);
        else if (input.mode === 'pre') assert.throws(() => altar.daisPreDestroy(5, 0x400000), AltarBodyEnd);
        else assert.fail(`제단 fixture 모드 누락: ${input.mode}`);
        // 관측의 Q 항목은 제단 24 Form 조회이며 실제 Host 조회와 완료 횟수로 확인하고 효과열에서는 분리한다.
        const expected = [];
        let formQueries = 0;
        for (const effect of row[3].split(';')) {
            if (effect.startsWith('Q:')) {
                formQueries += 1;
                const process = world.host.fromForm(Number(effect.slice(2)));
                assert.ok(process instanceof RegularProcess && process.event === 24, `행 ${checked} Q`);
            } else if (effect !== '-') expected.push(effect);
        }
        const where = `행 ${checked} (${row[1]})`;
        assert.equal(formQueries, input.mode === 'pre' && !(input.extra & 9) ? input.forms : 0, `${where} 조회 횟수`);
        assert.equal(returned, row[2], `${where} 반환`);
        assert.equal(joinEffects(effects), joinEffects(expected), `${where} 효과`);
        assert.equal(random.state, Number(row[5]), `${where} 난수`);
        assert.equal(world.state.playerResults.join(','), row[6], `${where} Player 결과`);
        const images = row[4].split(';');
        assert.equal(images.length, 5);
        // 마지막 공통 Damageable 진입 직전 경계(pre) 또는 최종 raw 의 가상 표 주입 DWORD 외 전체를 비교한다.
        [5, 6, 7, 8, 10].forEach((sid, index) => assert.equal((input.mode === 'pre' ? boundaryImages[index] : hex(world.pool.slot(sid))).slice(8), images[index].slice(8), `${where} 슬롯 ${sid}`));
        world.kernel.clear();
        checked += 1;
    }
    assert.equal(checked, 1435);
});

test('AltarSacrifice: 원본 10.78 daisprogress 제단 정기 handle 1,056행의 저항·탈출·희생 진행·최종 붕괴가 반환·효과·난수·Player 결과·슬롯 raw 까지 같다', { skip: !progressRows && '원본 fixture 없음' }, () => {
    let checked = 0;
    // 다이스만 쓰는 모드는 dais-regular 시험이 대조하고 여기서는 제단 정기 handle 만 쓴다.
    for (const row of progressRows.filter((candidate) => candidate[1].includes('"mode":"handle"'))) {
        assert.equal(row.length, 8);
        const input = JSON.parse(row[1]);
        const { world, effects, soundStarts, altar, random } = altarScene(input, 'progress');
        const payload = Math.fround(Number(input.payload));
        const returned = String(floatToBits(altar.handle(7, input.event, input.count, payload)));
        const where = `행 ${checked} (${row[1]})`;
        assert.equal(returned, row[2], `${where} 반환`);
        assert.equal(joinEffects(effects), row[3], `${where} 효과`);
        assert.equal(random.state, Number(row[6]), `${where} 난수`);
        assert.equal(world.state.playerResults.join(','), row[7], `${where} Player 결과`);
        const images = row[4].split(';');
        assert.equal(images.length, 5);
        [5, 6, 7, 9, 10].forEach((sid, index) => assert.equal(hex(world.pool.slot(sid)).slice(8), images[index].slice(8), `${where} 슬롯 ${sid}`));
        // 제단 정기는 다이스의 지연 소리를 쓰지 않아 두 시각 모두 0 이다.
        assert.deepEqual(soundStarts, [0, 0]);
        assert.equal(row[5], '0000000000000000;0000000000000000');
        world.kernel.clear();
        checked += 1;
    }
    assert.equal(checked, 1056);
});

// ---------------------------------------------------------------------------------------------------------------------
// 제단 정기 분기(runAltarRegular)의 순수 계약 시험: 훅 인자·호출 순서·경계값을 합성 풀/코드표/기록 훅으로 직접 확인한다. (2026-11-13 WB04-E368)
// ---------------------------------------------------------------------------------------------------------------------

// 제단(타입 1)과 다이스(타입 2)의 합성 코드표이다. 제단: 프레임 0 'A'(0), 1 'K'(10), 2 'P'(15), 3 'M'(12), 0x100 'M'(12), 0x102 'O'(14), 나머지 'A'. 다이스: 0 'A', 1 'K'(10), 2 'M', 3 'J'(9). (2026-11-13 WB04-E368)
function pureTables() {
    const altar = Array.from({ length: 300 }, () => 0x41);
    altar[1] = 0x4b;
    altar[2] = 0x50;
    altar[3] = 0x4d;
    altar[0x100] = 0x4d;
    altar[0x102] = 0x4f;
    const dais = [0x41, 0x4b, 0x4d, 0x4a];
    const table = (letters) => new FrameCodeTable(letters.map((side, index) => ({ side, variant: 0x50, number: index + 1, flags: 0 })));
    const frames = Array.from({ length: 188 }, () => new FrameCodeTable([]));
    frames[1] = table(altar);
    frames[2] = table(dais);
    return frames;
}

// 순수 장면: slots 의 { type, frame, x, y, owner, orig } 로 가짜 풀(capacity 100)을 만들고 모든 훅 호출을 log 에 기록한다. ctl 로 조회·난수·진행 결과를 정한다. (2026-11-13 WB04-E368)
function pureAltar(slots, ctl = {}) {
    const log = [];
    const pool = {
        capacity: 100,
        slot: (sid) => {
            const spec = slots[sid] ?? {};
            const raw = new Uint8Array(128);
            raw[10] = spec.type ?? 0;
            put(raw, 36, (spec.frame ?? 0) >>> 0, 4);
            put(raw, 14, floatToBits(spec.x ?? 0), 4);
            put(raw, 18, floatToBits(spec.y ?? 0), 4);
            raw[34] = spec.owner ?? 0;
            raw[12] = spec.orig ?? 0;
            return raw;
        },
    };
    // 큐는 앞에서부터 소비하고 비면 마지막 값을 반복한다.
    const pull = (queue, fallback) => (queue && queue.length ? (queue.length > 1 ? queue.shift() : queue[0]) : fallback);
    const hooks = {
        dais: () => { log.push('dais'); return pull(ctl.daisQ, ctl.dais ?? 0); },
        priest: () => { log.push('priest'); return ctl.priest ?? 0; },
        boss: () => { log.push('boss'); return ctl.boss ?? false; },
        alliance: (a, b) => { log.push(`al:${a},${b}`); return ctl.alliance ?? 0; },
        release: (sid) => { log.push(`rel:${sid}`); },
        escapeFrame: (sid, count) => { log.push(`esc:${sid},${count}`); },
        destroy: (sid, flags) => { log.push(`des:${sid},${flags}`); },
        advance: (sid, delta, flags) => { log.push(`adv:${sid},${delta},${flags}`); return pull(ctl.advanceQ, false); },
        frame: (sid, frame, flags) => { log.push(`fr:${sid},${frame},${flags}`); },
        reserveShared: () => { log.push('N'); return ctl.reserve ?? true; },
        addShared: (sid, event, payload) => { log.push(`S:${sid},${event},${payload}`); },
        randomInt: (low, high) => { log.push(`ri:${low},${high}`); return pull(ctl.riQ, 0); },
        randomFloat: (low, high) => { log.push(`rf:${low},${high}`); return pull(ctl.rfQ, 0); },
        sound: (x, y, name, flags) => { log.push(`snd:${x},${y},${name},${flags}`); },
        removeAnimation: (sid) => { log.push(`rm:${sid}`); },
        damageable: (sid, event, count, payload) => { log.push(`dmg:${sid},${event},${count},${payload}`); return 7; },
    };
    const state = new AltarSacrificeState();
    if (ctl.escapePercent !== undefined) state.escapePercent = ctl.escapePercent;
    if (ctl.struggleSeconds !== undefined) state.struggleSeconds = ctl.struggleSeconds;
    const tables = ctl.tables ?? pureTables();
    const run = (altar, event, count, payload) => runAltarRegular(pool, tables, altar, event, count, payload, state, hooks);
    return { log, run, state, pool };
}

test('runAltarRegular: 20·22 는 방향 14 에서만 ±1 진행하고 22 의 구간 끝은 +1 로 되돌리며 23 은 payload, 그 밖은 Damageable 이다', () => {
    // 방향이 14(프레임 0x102 'O')가 아니면 진행하지 않고 0 이다.
    for (const event of [20, 22]) {
        for (const frame of [0, 1, 2, 3, 0x100, 0x101]) {
            const w = pureAltar({ 10: { type: 1, frame } });
            assert.equal(w.run(10, event, 0, 1.5), 0, `이벤트 ${event} 프레임 ${frame}`);
            assert.ok(!w.log.some((entry) => entry.startsWith('adv:')));
        }
    }
    const frame = 0x102;
    const up = pureAltar({ 10: { type: 1, frame } }, { advanceQ: [true] });
    assert.equal(up.run(10, 20, 0, 1.5), 0);
    assert.deepEqual(up.log.filter((entry) => entry.startsWith('adv:')), ['adv:10,1,0']);
    const upWait = pureAltar({ 10: { type: 1, frame } }, { advanceQ: [false] });
    assert.equal(upWait.run(10, 20, 0, 1.5), 1.5);
    const down = pureAltar({ 10: { type: 1, frame } }, { advanceQ: [true, false] });
    assert.equal(down.run(10, 22, 0, 1.5), 0);
    assert.deepEqual(down.log.filter((entry) => entry.startsWith('adv:')), ['adv:10,-1,0', 'adv:10,1,0']);
    const downWait = pureAltar({ 10: { type: 1, frame } }, { advanceQ: [false] });
    assert.equal(downWait.run(10, 22, 0, 2.25), 2.25);
    assert.deepEqual(downWait.log.filter((entry) => entry.startsWith('adv:')), ['adv:10,-1,0']);
    // 23 은 payload, 그 밖의 사건은 Damageable 의 반환을 그대로 돌려준다. 제단 조회는 항상 먼저 한 번 한다.
    const same = pureAltar({ 10: { type: 1, frame } });
    assert.equal(same.run(10, 23, 3, 0.75), 0.75);
    assert.deepEqual(same.log, ['dais']);
    for (const event of [19, 25, 0x400]) {
        const w = pureAltar({ 10: { type: 1, frame } });
        assert.equal(w.run(10, event, 5, 0.5), 7);
        assert.deepEqual(w.log, ['dais', `dmg:10,${event},5,0.5`]);
    }
});

test('runAltarRegular: 프레임 DWORD 는 부호 있는 네 바이트 전체로 읽고 TYPE 이 표 밖이면 거부한다', () => {
    // 프레임 0x0102 만 방향 14 이다: 낮은 바이트(0x02)·둘째 바이트(0x01)를 바꿔 읽으면 다른 코드가 된다.
    const exact = pureAltar({ 10: { type: 1, frame: 0x102 } }, { advanceQ: [true] });
    assert.equal(exact.run(10, 20, 0, 1), 0);
    // 상위 바이트가 있는 프레임은 코드표 밖이라 거부한다(음수 포함).
    for (const frame of [0x01000102, 0x00010102, 0xffffffff, 300]) {
        const w = pureAltar({ 10: { type: 1, frame } });
        assert.throws(() => w.run(10, 20, 0, 1), RangeError, `프레임 ${frame}`);
    }
    for (const type of [188, 255]) {
        const w = pureAltar({ 10: { type, frame: 0 } });
        assert.throws(() => w.run(10, 20, 0, 1), (error) => error instanceof RangeError && /방향 TYPE 물리 범위/.test(error.message), `TYPE ${type}`);
    }
    // 187 은 표 안(빈 코드표)이라 코드표 조회에서 거부한다.
    const last = pureAltar({ 10: { type: 187, frame: 0 } });
    assert.throws(() => last.run(10, 20, 0, 1), (error) => error instanceof RangeError && !/TYPE 물리 범위/.test(error.message));
});

test('runAltarRegular: 21 은 자동 해방(권한·포획 사제·동맹)을 먼저 확인하고 방향 15 면 payload, 아니면 난수로 20/22 를 다시 예약한다', () => {
    const base = { 10: { type: 1, frame: 1 + 1 }, 20: { type: 2, frame: 0, x: 45.5, y: 46.25, owner: 2 }, 30: { orig: 0x83 } };
    // 자동 해방: 권한 + 유효한 포획 사제 + 동맹이면 해방 후 -1 이고 더 하지 않는다. 동맹 인자는 (원래 owner 하위 7비트, 다이스 owner) 이다.
    const free = pureAltar(base, { dais: 20, priest: 30, boss: true, alliance: 5 });
    assert.equal(free.run(10, 21, 0, 1), -1);
    assert.deepEqual(free.log, ['dais', 'priest', 'boss', 'al:3,2', 'rel:10']);
    // 권한이 없거나 포획 사제가 풀 밖(0, 100)이면 동맹을 묻지 않는다. 동맹이 아니면 계속한다(방향 15 → payload).
    for (const [boss, priest] of [[false, 30], [true, 0], [true, 100]]) {
        const w = pureAltar(base, { dais: 20, priest, boss, alliance: 5 });
        assert.equal(w.run(10, 21, 0, 1.25), 1.25);
        assert.ok(!w.log.some((entry) => entry.startsWith('al:') || entry.startsWith('rel:')), `권한 ${boss} 사제 ${priest}`);
    }
    const noAlly = pureAltar(base, { dais: 20, priest: 30, boss: true, alliance: 0 });
    assert.equal(noAlly.run(10, 21, 0, 1.25), 1.25);
    assert.deepEqual(noAlly.log, ['dais', 'priest', 'boss', 'al:3,2']);
    // 99 는 유효한 포획 사제이다. 원래 owner 8·다이스 owner 8 은 허용하고 9 는 동맹 인덱스 범위 밖이다.
    for (const [orig, owner, ok] of [[8, 8, true], [9, 8, false], [8, 9, false], [0x88, 8, true], [0x89, 1, false]]) {
        const w = pureAltar({ ...base, 20: { ...base[20], owner }, 99: { orig } }, { dais: 20, priest: 99, boss: true, alliance: 0 });
        if (ok) assert.equal(w.run(10, 21, 0, 1), 1);
        else assert.throws(() => w.run(10, 21, 0, 1), (error) => error instanceof RangeError && /동맹 인덱스/.test(error.message));
    }
    // 방향 15(프레임 2 'P')는 payload 를 돌려주고 난수를 쓰지 않는다. 포획 사제 조회는 방향 확인보다 앞선다.
    const gone = pureAltar({ ...base, 10: { type: 1, frame: 2 } }, { dais: 20, priest: 0, boss: false });
    assert.equal(gone.run(10, 21, 0, 0.5), 0.5);
    assert.ok(!gone.log.some((entry) => entry.startsWith('ri:')));
});

test('runAltarRegular: 21 의 탈출 확률·20/22 선택·소리 선택표·다음 지연을 난수 인자와 함께 확인한다', () => {
    const base = (dais) => ({ 10: { type: 1, frame: 3 }, 20: { type: 2, frame: 0, x: 45.5, y: 46.25, owner: 2 }, ...(dais ? {} : {}) });
    const run = (ctl, slots = base(true)) => {
        const w = pureAltar(slots, { dais: 20, boss: false, ...ctl });
        const result = w.run(10, 21, 0, 9);
        return { ...w, result };
    };
    // 탈출 확률: 난수 몫 < escapePercent(40) 이면 22 + 역진행, 아니면 20. 인자는 (0, 100000) 이다.
    for (const [roll, percent, event, advances] of [[39999, 40, 22, 1], [40000, 40, 20, 0], [0, 40, 22, 1], [9999, 10, 22, 1], [10000, 10, 20, 0], [99999, 100, 22, 1], [100000, 100, 20, 0]]) {
        const w = run({ riQ: [roll, 0, 0], rfQ: [0], escapePercent: percent });
        assert.equal(w.log.filter((entry) => entry.startsWith('adv:')).length, advances, `${roll}/${percent}`);
        assert.deepEqual(w.log.find((entry) => entry.startsWith('S:')), `S:10,${event},${Math.fround(0.08)}`, `${roll}/${percent}`);
        assert.equal(w.log.find((entry) => entry.startsWith('ri:')), 'ri:0,100000');
    }
    // 예약이 실패하면 부착하지 않는다.
    const failed = run({ riQ: [0, 0, 0], rfQ: [0], reserve: false });
    assert.ok(failed.log.includes('N') && !failed.log.some((entry) => entry.startsWith('S:')));
    // 다이스가 있고 방향이 10 이 아니면 소리 두 개: 사슬(0..4)과 저항(0..2)이며 위치는 다이스 좌표 − 3.0(float32) 이다.
    const centerX = Math.fround(45.5 - 3);
    const centerY = Math.fround(46.25 - 3);
    const chains = ['chainRattle23.wav', 'chainRattle24.wav', 'chainRattle25.wav', 'chainRattle26.wav', 'chainRattle27.wav'];
    const struggles = ['priestStruggleFade01.wav', 'priestStruggleFade02.wav', 'priestStruggleFade03.wav'];
    for (let chain = 0; chain < 5; chain += 1) {
        for (const [roll, struggle] of [[0, 0], [999, 0], [1000, 1], [1999, 1], [2000, 2], [2999, 2]]) {
            const w = run({ riQ: [99999, chain, roll], rfQ: [0] });
            const sounds = w.log.filter((entry) => entry.startsWith('snd:'));
            assert.deepEqual(sounds, [`snd:${centerX},${centerY},${chains[chain]},0`, `snd:${centerX},${centerY},${struggles[struggle]},0`], `사슬 ${chain} 난수 ${roll}`);
            assert.deepEqual(w.log.filter((entry) => entry.startsWith('ri:')), ['ri:0,100000', 'ri:0,5', 'ri:0,3000']);
        }
    }
    // 난수가 선택표 밖이면 소리 전에 거부한다.
    for (const [chain, roll] of [[5, 0], [-1, 0], [100, 0], [0, 3000], [0, -1000], [0, 99999]]) {
        const w = pureAltar(base(true), { dais: 20, riQ: [99999, chain, roll], rfQ: [0] });
        assert.throws(() => w.run(10, 21, 0, 9), RangeError, `사슬 ${chain} 저항 ${roll}`);
    }
    // 다이스가 없거나(0)·풀 밖(100)·방향 10(프레임 1 'K')이면 소리와 사슬 난수를 쓰지 않고, 다이스 99(풀 안)는 소리를 낸다.
    for (const [daisSid, frame, sounds] of [[0, 0, 0], [100, 0, 0], [20, 1, 0], [99, 0, 2]]) {
        const w = pureAltar({ ...base(true), 99: { type: 2, frame: 0, x: 45.5, y: 46.25 }, 20: { type: 2, frame, x: 45.5, y: 46.25 } }, { dais: daisSid, riQ: [99999, 0, 0], rfQ: [0] });
        w.run(10, 21, 0, 9);
        assert.equal(w.log.filter((entry) => entry.startsWith('snd:')).length, sounds, `다이스 ${daisSid} 프레임 ${frame}`);
    }
    // 다음 지연은 난수 실수(0..2) + 저항 초(float32)이며 float32 로 저장한다.
    for (const [roll, seconds] of [[Math.fround(0.1), 3.0], [Math.fround(1.7), 2.5], [0, 3.0]]) {
        const w = run({ riQ: [99999, 0, 0], rfQ: [roll], struggleSeconds: seconds });
        assert.equal(w.result, Math.fround(roll + seconds));
        assert.ok(w.log.includes('rf:0,2'));
    }
    const dflt = run({ riQ: [99999, 0, 0], rfQ: [Math.fround(0.1)] });
    assert.equal(dflt.result, Math.fround(Math.fround(0.1) + 3));
    // 기본 상태값이다.
    const state = new AltarSacrificeState();
    assert.deepEqual([state.struggleSeconds, state.escapePercent], [3, 40]);
});

test('runAltarRegular: 24 는 빈 프레임 다이스를 삭제하거나 진행/이탈/붕괴 소리를 내고 구간 끝에서 포획 삭제·폭발·프레임을 정한다', () => {
    const slots = { 10: { type: 1, frame: 0, x: 12.5, y: 13.75 }, 20: { type: 2, frame: 0, x: 45.5, y: 46.25 }, 30: {} };
    const run = (ctl, extra = {}) => {
        const w = pureAltar({ ...slots, ...extra }, { dais: 20, boss: true, ...ctl });
        const result = w.run(10, 24, ctl.count ?? 0, ctl.payload ?? 0.5);
        return { ...w, result };
    };
    // 다이스가 풀 밖/없으면 거부한다.
    for (const dais of [0, 100, 150]) {
        const w = pureAltar(slots, { dais });
        assert.throws(() => w.run(10, 24, 0, 1), /다이스0 프레임읽기 경계/, `다이스 ${dais}`);
    }
    // 다이스 방향이 10(프레임 1 'K')이면 권한이 있을 때 삭제(0x400000)하고 -1, 없으면 payload 이다.
    const empty = run({ boss: true }, { 20: { ...slots[20], frame: 1 } });
    assert.equal(empty.result, -1);
    assert.deepEqual(empty.log.filter((entry) => entry.startsWith('des:')), ['des:20,4194304']);
    const noBoss = run({ boss: false }, { 20: { ...slots[20], frame: 1 } });
    assert.equal(noBoss.result, 0.5);
    assert.ok(!noBoss.log.some((entry) => entry.startsWith('des:') || entry.startsWith('adv:')));
    // 방향 9(프레임 3 'J')는 10 이 아니므로 진행한다.
    const nine = run({ advanceQ: [false], rfQ: [1], payload: 0.5, count: 0 }, { 20: { ...slots[20], frame: 3 } });
    assert.ok(nine.log.includes('adv:10,1,0'));
    // 진행 중(advance false): count < 3 이면 난수 없이 이탈 프레임, 3 이상이면 (0,100000) 난수 몫 < 50 일 때만 이탈한다. 붕괴 소리는 실수 < payload 일 때만 낸다.
    for (const [count, roll, escapes] of [[0, 99999, true], [2, 99999, true], [3, 49999, true], [3, 50000, false], [7, 0, true], [7, 99999, false]]) {
        const w = run({ advanceQ: [false], riQ: [roll], rfQ: [1], count });
        assert.equal(w.log.includes(`esc:20,${count}`), escapes, `count ${count} 난수 ${roll}`);
        assert.equal(w.log.filter((entry) => entry.startsWith('ri:')).length, count < 3 ? 0 : 1);
        if (count >= 3) assert.equal(w.log.find((entry) => entry.startsWith('ri:')), 'ri:0,100000');
        assert.equal(w.result, 0.5);
    }
    for (const [roll, sounds] of [[0.25, 1], [0.5, 0], [0.75, 0]]) {
        const w = run({ advanceQ: [false], rfQ: [roll], payload: 0.5, count: 0 });
        assert.equal(w.log.filter((entry) => entry.startsWith('snd:')).length, sounds, `실수 ${roll}`);
        assert.ok(w.log.includes('rf:0,1'));
        if (sounds) assert.ok(w.log.includes(`snd:${Math.fround(12.5)},${Math.fround(13.75)},altarBurnCollapse.wav,0`));
        assert.equal(w.result, 0.5);
    }
    // 구간 끝(advance true): 다이스를 다시 조회해 풀 밖이면 payload, 아니면 권한이 있을 때 포획 사제를 지우고(flags 8) 애니메이션 제거·폭발·다이스 K·제단 M 프레임 뒤 3.0 이다.
    const finish = run({ advanceQ: [true], priest: 30, daisQ: [20, 20] });
    assert.equal(finish.result, 3);
    assert.deepEqual(finish.log.slice(finish.log.indexOf('adv:10,1,0') + 1), ['dais', 'boss', 'priest', 'des:30,8', 'rm:10', `snd:${Math.fround(12.5)},${Math.fround(13.75)},explosion.wav,0`, 'fr:20,1,0', 'fr:10,3,0']);
    const bossless = run({ advanceQ: [true], priest: 30, boss: false, daisQ: [20, 20] });
    assert.ok(!bossless.log.includes('priest') && !bossless.log.some((entry) => entry.startsWith('des:')));
    assert.equal(bossless.result, 3);
    const nobody = run({ advanceQ: [true], priest: 0, daisQ: [20, 20] });
    assert.ok(!nobody.log.some((entry) => entry.startsWith('des:')));
    // 구간 끝에서 다시 조회한 다이스가 풀 밖이면 payload 를 돌려주고 아무 것도 하지 않는다(조회는 맨 앞·24 분기·구간 끝 세 번이다).
    for (const second of [0, 100]) {
        const lost = run({ advanceQ: [true], priest: 30, daisQ: [20, 20, second] });
        assert.equal(lost.result, 0.5);
        assert.ok(!lost.log.some((entry) => entry.startsWith('rm:') || entry.startsWith('fr:')));
    }
    // 현재 TYPE 의 글자 첫 번호를 읽고 TYPE 이 표 밖(188)이면 다이스 프레임 지정 전에 거부한다.
    const badDais = pureAltar({ ...slots, 40: { type: 188 } }, { dais: 20, boss: true, advanceQ: [true], priest: 30, daisQ: [20, 20, 40] });
    assert.throws(() => badDais.run(10, 24, 0, 0.5), (error) => error instanceof RangeError && /frame TYPE 물리 범위/.test(error.message));
    // 제단 TYPE 이 표 밖이면 다이스 K 프레임 뒤 제단 M 프레임 조회에서 거부한다(제단 방향은 24 에서 읽지 않는다).
    const badAltar = pureAltar({ ...slots, 10: { type: 188, frame: 0 } }, { dais: 20, boss: true, advanceQ: [true], priest: 30, daisQ: [20, 20, 20] });
    assert.throws(() => badAltar.run(10, 24, 0, 0.5), (error) => error instanceof RangeError && /frame TYPE 물리 범위/.test(error.message));
    assert.ok(badAltar.log.includes('fr:20,1,0') && !badAltar.log.some((entry) => entry.startsWith('fr:10,')));
});

// ---------------------------------------------------------------------------------------------------------------------
// RawAltarSacrifice 웹 계약 시험: 생성자·조회·이탈 프레임·해방·전처리·훅 공장의 경계를 합성 장면에서 직접 확인한다. (2026-11-13 WB04-E368)
// ---------------------------------------------------------------------------------------------------------------------

// 제단 관측과 같은 기본 입력(다이스 프레임 3, 제단 프레임 0, 권한 있음)이다. (2026-11-13 WB04-E368)
const plain = (overrides = {}) => ({
    alliance: 0, altarFrame: 0, boss: 1, captured: 0, count: 0, daisFrame: 3, event: 20, extra: 0, forms: 0, local: 1, mode: 'handle', oldOwner: 2, owner: 1, reserve: 1, seed: 0, suppress: 0, payload: 9.25, ...overrides,
});

// 같은 장면의 부품으로 훅만 바꾼 새 제단 몸체를 만든다. (2026-11-13 WB04-E368)
const rebuild = (scene, hooks) => new RawAltarSacrifice(scene.world.pool, scene.world.hash, scene.world.types, scene.world.frames, scene.world.footprint, scene.world.rune, scene.world.ritual,
    scene.world.carrier, scene.world.dais, scene.advance, scene.world.host, scene.damageable, scene.world.state, scene.altarState, hooks);

// 제단(7)과 다이스(5)를 해시 사슬에서 빼 조회 결과가 비게 만든다(사슬 7 → 6 → 5 에서 5 를 뗀다). (2026-11-13 WB04-E368)
const detachDais = (scene) => { put(scene.world.slots[6], 4, 0, 2); };
const detachAltar = (scene) => { scene.world.hash.setBucket(1, 40.75, 41.25, 6); put(scene.world.slots[7], 4, 0, 2); };

test('RawAltarSacrifice: 생성자는 10.78 풀과 여덟 협력 부품의 같은 풀·188칸 표를 확인한다', () => {
    const a = altarScene(plain(), 'altar');
    const b = altarScene(plain(), 'altar');
    const parts = (swap = {}) => {
        const w = a.world;
        const p = { pool: w.pool, hash: w.hash, types: w.types, frames: w.frames, footprint: w.footprint, rune: w.rune, ritual: w.ritual, carrier: w.carrier, dais: w.dais, advance: a.advance, host: w.host, damageable: a.damageable, ...swap };
        return new RawAltarSacrifice(p.pool, p.hash, p.types, p.frames, p.footprint, p.rune, p.ritual, p.carrier, p.dais, p.advance, p.host, p.damageable, w.state, a.altarState, a.hooks);
    };
    assert.ok(parts().pool === a.world.pool);
    const alien = { footprint: b.world.footprint, rune: b.world.rune, ritual: b.world.ritual, carrier: b.world.carrier, dais: b.world.dais, advance: b.advance, host: b.world.host, damageable: b.damageable };
    for (const name of Object.keys(alien)) assert.throws(() => parts({ [name]: alien[name] }), TypeError, name);
    assert.throws(() => parts({ pool: { ...a.world.pool, edition: 'cd1072' } }), TypeError);
    assert.throws(() => parts({ types: a.world.types.slice(0, 187) }), TypeError);
    assert.throws(() => parts({ frames: a.world.frames.slice(0, 187) }), TypeError);
    assert.throws(() => parts({ types: { length: 188 } }), TypeError);
    assert.throws(() => parts({ frames: { length: 188 } }), TypeError);
    assert.throws(() => parts({ pool: null }), TypeError);
});

test('RawAltarSacrifice.handles/handle: 실제 제단 가상표만 받고 사건은 제단 정기 분기로 분배한다', () => {
    const s = altarScene(plain({ event: 23 }), 'altar');
    assert.equal(s.altar.handles(7), true);
    for (const sid of [5, 6, 9]) assert.equal(s.altar.handles(sid), false);
    put(s.world.slots[9], 0, PATCH_ALTAR_VTABLE + 1);
    assert.equal(s.altar.handles(9), false);
    assert.throws(() => s.altar.handle(6, 23, 0, 1), TypeError);
    assert.equal(s.altar.handle(7, 23, 0, 4.5), 4.5);
    // 제단이 아닌 사건은 Damageable 훅으로 간다(훅 누락은 거부).
    const damage = [];
    const custom = rebuild(s, { ...s.hooks, damageable: (...args) => { damage.push(args); return 2.5; } });
    assert.equal(custom.handle(7, 31, 6, 0.25), 2.5);
    assert.deepEqual(damage, [[7, 31, 6, 0.25]]);
    assert.throws(() => rebuild(s, { ...s.hooks, damageable: undefined }).handle(7, 31, 6, 0.25), /제단의 선택된 실제 장치 미연결/);
    // 제단 24 최종 단계의 방향 동맹 표는 (원래 owner + owner × 9) 칸이다: 자동 해방 21 에서 동맹이 있으면 해방한다.
    const ally = altarScene(plain({ event: 21, captured: 158, altarFrame: 7, oldOwner: 2, owner: 1 }), 'altar');
    ally.world.state.alliances[2 + 1 * 9] = 1;
    assert.equal(ally.altar.handle(7, 21, 0, 1), -1);
    assert.ok(ally.effects.includes('G:priestFree.wav,0,0,0,1,0'));
    const none = altarScene(plain({ event: 21, captured: 158, altarFrame: 7, oldOwner: 2, owner: 1 }), 'altar');
    none.world.state.alliances[1 + 2 * 9] = 1;
    assert.notEqual(none.altar.handle(7, 21, 0, 1), -1);
});

test('RawAltarSacrifice.daisAt/count: 기준점의 첫 다이스 TYPE 후보를 찾고 좌표·TYPE 범위를 거부하며 개수는 대상의 가상 +0x80 에 맡긴다', () => {
    const s = altarScene(plain(), 'altar');
    assert.equal(s.altar.daisAt(7), 5);
    // 다이스 TYPE 이 사슬에 없으면 0 이고 개수도 0(가상 조회 없음)이다.
    const calls = [];
    const counted = rebuild(s, { ...s.hooks, count: (sid) => { calls.push(sid); return 9; } });
    assert.equal(counted.count(7), 9);
    assert.deepEqual(calls, [5]);
    detachDais(s);
    assert.equal(s.altar.daisAt(7), 0);
    assert.equal(counted.count(7), 0);
    assert.deepEqual(calls, [5]);
    // 좌표 절삭 경계: −2^31 포함·2^31 제외, 비유한 거부(x·y 모두).
    for (const [offset, value, ok] of [[14, -2147483648, true], [14, 2147483520, true], [18, -2147483648, true], [18, 2147483520, true], [14, NaN, false], [14, Infinity, false], [14, -Infinity, false],
        [14, 2147483648, false], [14, -2147483904, false], [18, NaN, false], [18, 2147483648, false], [18, -2147483904, false]]) {
        const w = altarScene(plain(), 'altar');
        put(w.world.slots[7], offset, floatToBits(value), 4);
        if (ok) w.altar.daisAt(7);
        else assert.throws(() => w.altar.daisAt(7), (error) => error instanceof RangeError && /좌표 절삭 범위/.test(error.message), `${offset}=${value}`);
    }
    // 현재 다이스 TYPE 번호가 표 밖(188)이면 거부하고 표 안 마지막(187)은 후보가 없어 0 이다.
    const range = altarScene(plain(), 'altar');
    range.world.runeState.daisType = 188;
    assert.throws(() => range.altar.daisAt(7), (error) => error instanceof RangeError && /다이스 타입 범위/.test(error.message));
    range.world.runeState.daisType = 187;
    assert.equal(range.altar.daisAt(7), 0);
    // genus 의 표면 비트(6)가 있으면 표면 단계(level 0)까지 훑는다: level 0 의 순환 사슬을 만나면 finder 가 거부한다.
    for (const [flags2, scans] of [[0, false], [1, false], [2, true], [4, true], [6, true], [8, false]]) {
        const w = altarScene(plain(), 'altar');
        w.world.types[165].flags2 = flags2;
        w.world.slots[11][10] = 1;
        put(w.world.slots[11], 4, 11, 2);
        w.world.hash.setCell(0, 40, 41, 11);
        if (scans) assert.throws(() => w.altar.daisAt(7), /체인/, `flags2 ${flags2}`);
        else assert.equal(w.altar.daisAt(7), 5);
    }
    // 첫 다이스 후보에서 멈추고 뒤의 순환 사슬은 보지 않는다. 다른 TYPE 이 앞서면 계속 진행해 순환을 만난다.
    const first = altarScene(plain(), 'altar');
    first.world.slots[11][10] = 1;
    put(first.world.slots[11], 4, 11, 2);
    put(first.world.slots[5], 4, 11, 2);
    assert.equal(first.altar.daisAt(7), 5);
    const miss = altarScene(plain(), 'altar');
    miss.world.slots[11][10] = 1;
    put(miss.world.slots[11], 4, 11, 2);
    miss.world.slots[5][10] = 1;
    put(miss.world.slots[5], 4, 11, 2);
    assert.throws(() => miss.altar.daisAt(7), /체인/);
    // 가상 개수 장치가 없고 대상 가상표가 실제 다이스가 아니면 거부, 다이스이면 신전 의식의 count 를 쓴다.
    const bare = altarScene(plain(), 'altar');
    const noHook = rebuild(bare, { ...bare.hooks, count: undefined });
    assert.equal(noHook.count(7), bare.world.ritual.count(5));
    put(bare.world.slots[5], 0, VTABLE);
    assert.throws(() => noHook.count(7), /가상80 분배 장치 미연결/);
});

test('RawAltarSacrifice.escapeFrame: count 0~2 는 방향 글자 첫 번호 + count 로 프레임을 지정하고 3 이상은 구간 길이 × 1000 까지 난수만 소비한다', () => {
    // 다이스(165) 코드표: K(0..7) L(8..15) M(16..23) N(24..31) O(32..39) P(40..47) A(48..179).
    const sceneWith = (frame, count) => {
        const s = altarScene(plain({ daisFrame: frame }), 'altar');
        const frames = [];
        const randoms = [];
        const custom = rebuild(s, { ...s.hooks, count: () => count ?? 0, setFrame: (sid, value, flags) => frames.push([sid, value, flags]), randomInt: (low, high) => { randoms.push([low, high]); return 0; } });
        return { s, custom, frames, randoms };
    };
    // count 1·2: 현재 프레임의 방향 글자 구간 첫 번호 + count 이다(가상 count 는 읽지 않는다).
    for (const [frame, first] of [[0, 0], [9, 8], [20, 16], [30, 24], [33, 32], [45, 40], [100, 48]]) {
        for (const count of [1, 2]) {
            const w = sceneWith(frame, 99);
            w.custom.escapeFrame(5, count);
            assert.deepEqual(w.frames, [[5, first + count, 0]], `프레임 ${frame} count ${count}`);
            assert.deepEqual(w.randoms, []);
        }
    }
    // count 0: 방향을 14 − 가상 count 로 다시 정한다(DWORD 감김). 첫 프레임 조회는 그 글자 구간이다.
    for (const [virtual, first] of [[0, 32], [1, 24], [3, 8], [4, 0], [5, -1], [8, -1], [14, 48], [0xffffffff, 40]]) {
        const w = sceneWith(100, virtual);
        w.custom.escapeFrame(5, 0);
        assert.deepEqual(w.frames, [[5, first, 0]], `가상 count ${virtual}`);
    }
    // 방향 표(0..15) 밖이면 거부한다: 15 → −1, 16 → −2, 0xfffffffe → 16.
    for (const virtual of [15, 16, 100, 0xfffffffe, 0x7fffffff]) {
        const w = sceneWith(100, virtual);
        assert.throws(() => w.custom.escapeFrame(5, 0), (error) => error instanceof RangeError && /첫 프레임 표 범위/.test(error.message), `가상 count ${virtual}`);
        assert.deepEqual(w.frames, []);
    }
    // count 3 이상: 프레임을 지정하지 않고 현재 방향 글자 구간 길이 × 1000 을 상한으로 난수를 소비한다.
    for (const [frame, length] of [[0, 8], [9, 8], [20, 8], [100, 132]]) {
        for (const count of [3, 4, 50]) {
            const w = sceneWith(frame, 0);
            w.custom.escapeFrame(5, count);
            assert.deepEqual(w.frames, []);
            assert.deepEqual(w.randoms, [[3000, length * 1000]], `프레임 ${frame} count ${count}`);
        }
    }
    // 장치 누락은 도달한 위치에서 거부한다.
    const lazy = altarScene(plain(), 'altar');
    assert.throws(() => rebuild(lazy, { ...lazy.hooks, setFrame: undefined }).escapeFrame(5, 1), /제단의 선택된 실제 장치 미연결/);
    assert.throws(() => rebuild(lazy, { ...lazy.hooks, randomInt: undefined }).escapeFrame(5, 3), /제단의 선택된 실제 장치 미연결/);
});

test('RawAltarSacrifice.releasePriest: 진단 세 곳(제단·투명 프레임·권한)을 정상 복귀 뒤 이어가고 포획 사제 해방의 생성·WORD·owner·Pop·효과·삭제·소리·604 취소 순서를 지킨다', () => {
    const asserts = (scene) => { scene.hooks.assertion = (expression, file, line) => { scene.effects.push(`ASSERT:${expression}|${file}|${line}`); }; return rebuild(scene, scene.hooks); };
    const full = altarScene(plain({ captured: 158 }), 'altar');
    // 제단 중심 x + 0.5(float32) 와 중심 y 에 해방 사제를 Pop 한다.
    const center = full.world.footprint.center(5);
    full.world.state.sharedBridgeType = null;
    full.altar.releasePriest(5);
    const popX = floatToBits(Math.fround(center[0] + 0.5));
    assert.deepEqual(full.effects, ['C:158,0,8', 'owner:8,0', `P:8,${popX},${floatToBits(center[1])},0`, `E:8,${floatToBits(5)},0`, 'destroy:10,0', 'G:priestFree.wav,0,0,0,1,0']);
    assert.equal(get(full.world.pool.slot(8), 12, 2), 2);
    // 강한 사제의 의식 중(프레임 K)이면 604 를 취소하고 E 프레임을 지정한다. 약한 사제(HP 50)나 사제 없음이면 건너뛴다.
    const stop = altarScene(plain({ captured: 158 }), 'altar');
    put(stop.world.slots[6], 36, 2, 4);
    stop.altar.releasePriest(5);
    assert.equal(stop.effects.at(-1), 'F:6,1,0');
    const weak = altarScene(plain({ captured: 158 }), 'altar');
    put(weak.world.slots[6], 36, 2, 4);
    setRawSquidHitPoints(weak.world.pool, 6, 50);
    weak.altar.releasePriest(5);
    assert.ok(!weak.effects.some((entry) => entry.startsWith('F:')));
    // 포획 사제가 없으면 생성하지 않고 소리와 604 취소만 한다.
    const empty = altarScene(plain(), 'altar');
    empty.altar.releasePriest(5);
    assert.deepEqual(empty.effects, ['G:priestFree.wav,0,0,0,1,0']);
    // 제단이 없으면 진단 'altar.isValid()' 를 낸다(정상 복귀하면 계속한다). 진단 장치가 없으면 거부한다.
    const lost = altarScene(plain(), 'altar');
    detachAltar(lost);
    asserts(lost).releasePriest(5);
    assert.equal(lost.effects[0], 'ASSERT:altar.isValid()|.\\Dais.cpp|971');
    assert.equal(lost.effects.length, 2);
    const bare = altarScene(plain(), 'altar');
    detachAltar(bare);
    assert.throws(() => bare.altar.releasePriest(5), /해방 원문 진단 장치 미연결/);
    // 투명/희생 프레임(방향 12·11) 진단은 checking 이 켜졌을 때만 낸다. 제단 프레임: A(0..2) O(3..6) M(7..9) L(10..12) P(13..15).
    for (const [frame, diagnosed] of [[0, false], [3, false], [7, true], [9, true], [10, true], [12, true], [13, false]]) {
        for (const debug of [0, 1]) {
            const w = altarScene(plain({ altarFrame: frame }), 'altar');
            w.world.state.debug = debug;
            asserts(w).releasePriest(5);
            const expected = debug && diagnosed ? ['ASSERT:!altar.isInvisibleFrame() && !altar.isSacrifice()|.\\Dais.cpp|972', 'G:priestFree.wav,0,0,0,1,0'] : ['G:priestFree.wav,0,0,0,1,0'];
            assert.deepEqual(w.effects, expected, `프레임 ${frame} 진단 ${debug}`);
        }
    }
    // 권한이 없으면 진단 'boss' 뒤에도 포획 해방을 계속한다.
    const noBoss = altarScene(plain({ boss: 0, captured: 158 }), 'altar');
    asserts(noBoss).releasePriest(5);
    assert.equal(noBoss.effects[0], 'ASSERT:boss|.\\Dais.cpp|973');
    assert.ok(noBoss.effects.includes('destroy:10,0'));
    // 권한 훅이 있으면 신전 의식의 권한보다 우선한다.
    const hooked = altarScene(plain({ boss: 0, captured: 158 }), 'altar');
    hooked.altar = rebuild(hooked, { ...hooked.hooks, boss: () => true, assertion: undefined });
    hooked.altar.releasePriest(5);
    assert.ok(hooked.effects.includes('destroy:10,0'));
    // 사제 종류 0 이면 일반 wildcard 와 달라 조회를 거부한다.
    const zero = altarScene(plain({ captured: 158 }), 'altar');
    zero.world.state.ritual.priestType = 0;
    assert.throws(() => zero.altar.releasePriest(5), /specialized kind0/);
    // 해방 사제 WORD 지정이 현재 다리 TYPE 이면 표면 통지를 부르고, 장치가 없으면 거부한다.
    const notified = [];
    const bridge = altarScene(plain({ captured: 158 }), 'altar');
    bridge.world.state.bridgeType = 158;
    rebuild(bridge, { ...bridge.hooks, notifySurface: (sid) => notified.push(sid) }).releasePriest(5);
    assert.deepEqual(notified, [8]);
    const lazy = altarScene(plain({ captured: 158 }), 'altar');
    lazy.world.state.bridgeType = 158;
    assert.throws(() => lazy.altar.releasePriest(5), /표면 통지 미연결/);
    // 선택하지 않은 장치는 도달한 위치에서만 요구한다.
    const missing = altarScene(plain({ captured: 158 }), 'altar');
    for (const name of ['create', 'setOwner', 'pop', 'escapeEffect', 'destroy', 'playGlobal']) {
        assert.throws(() => rebuild(missing, { ...missing.hooks, [name]: undefined }).releasePriest(5), /제단의 선택된 실제 장치 미연결/, name);
    }
});

test('RawAltarSacrifice.releaseFromAltar: 다이스가 있으면 해방 → 제단 Unpop → P 첫 번호 직접 쓰기(표시 지정 없음) → Repop 순서이다', () => {
    const s = altarScene(plain({ captured: 158 }), 'altar');
    put(s.world.slots[7], 40, 0x52, 1);
    s.altar.releaseFromAltar(7);
    assert.deepEqual(s.effects.slice(-3), ['G:priestFree.wav,0,0,0,1,0', 'unpop:7,0', 'repop:7,0']);
    // P 구간의 첫 번호(13)를 프레임 DWORD 네 바이트에만 쓰고 이웃 바이트(extra 40)는 보존한다.
    assert.equal(get(s.world.pool.slot(7), 36, 4), 13);
    assert.equal(s.world.pool.slot(7)[40], 0x52);
    assert.ok(!s.effects.some((entry) => /^F:7,/.test(entry)));
    // 다이스가 없으면 해방 없이 Unpop/Repop 만 한다.
    const none = altarScene(plain(), 'altar');
    detachDais(none);
    none.altar.releaseFromAltar(7);
    assert.deepEqual(none.effects, ['unpop:7,0', 'repop:7,0']);
    // checking 이 켜졌고 제단 방향이 12 이면 진단 0x578 을 낸 뒤 계속한다.
    const diag = altarScene(plain({ altarFrame: 7 }), 'altar');
    diag.world.state.debug = 1;
    const lines = [];
    rebuild(diag, { ...diag.hooks, assertion: (expression, file, line) => lines.push([expression, file, line]) }).releaseFromAltar(7);
    // 부모 진단(0x578) 뒤 정상 복귀하면 다이스 해방 본문의 같은 조건 진단(0x3cc)도 이어서 낸다.
    assert.deepEqual(lines, [['!isInvisibleFrame() && !isSacrifice()', '.\\Dais.cpp', 0x578], ['!altar.isInvisibleFrame() && !altar.isSacrifice()', '.\\Dais.cpp', 0x3cc]]);
    // 선택한 Unpop/Repop 장치가 없으면 거부한다.
    assert.throws(() => rebuild(none, { ...none.hooks, unpop: undefined }).releaseFromAltar(7), /미연결/);
    assert.throws(() => rebuild(none, { ...none.hooks, repop: undefined }).releaseFromAltar(7), /미연결/);
});

test('RawAltarSacrifice.daisPreDestroy: 실제 다이스 가상표만 받고 제단 24 완료는 삭제 flags 0x400000 일 때만, extra 9 관문 안에서는 아무 것도 하지 않는다', () => {
    const run = (input, flags) => {
        const s = altarScene(plain(input), 'altar');
        assert.throws(() => s.altar.daisPreDestroy(5, flags), AltarBodyEnd);
        return s;
    };
    // 가상표가 다이스가 아니면 거부한다.
    const bad = altarScene(plain(), 'altar');
    assert.throws(() => bad.altar.daisPreDestroy(6, 0), TypeError);
    // forms 1(제단 24 Form): 0x400000 이면 희생 완료(V:), 아니면 건너뛰며 제단 삭제(0x48)·owner 갱신은 항상 한다. extra 에 1 또는 8 이 있으면 전부 건너뛰고 공통 전처리만 부른다.
    for (const [flags, extra, finish, rest] of [[0x400000, 0, true, true], [0, 0, false, true], [0x400000, 1, false, false], [0x400000, 8, false, false], [0x400000, 9, false, false], [0, 2, false, true], [0x400000, 4, true, true], [0x400000, 0x10, true, true], [0x400000, 0x80, true, true]]) {
        const s = run({ forms: 1, extra }, flags);
        const joined = s.effects.join(';');
        assert.equal(joined.includes('V:99,0,0'), finish, `flags ${flags} extra ${extra}`);
        assert.equal(joined.includes('destroy:7,72'), rest, `flags ${flags} extra ${extra}`);
        assert.equal(joined.includes('R:8'), rest, `flags ${flags} extra ${extra}`);
        assert.equal(s.effects.at(-1), `B:5,${flags}`);
    }
    // 완료를 부르는데 제단이 없으면 제단 프로세스 부모 누락으로 거부한다.
    const lost = altarScene(plain({ forms: 1 }), 'altar');
    detachAltar(lost);
    assert.throws(() => lost.altar.daisPreDestroy(5, 0x400000), /제단 프로세스 부모 누락/);
});

test('makeAltarSacrificeHooks: 실제 Frame/Host/Factory/Damageable/난수를 연결한 새 묶음을 만들고 같은 풀이 아니면 거부한다', () => {
    const s = altarScene(plain(), 'altar');
    const { pool } = s.world;
    const calls = [];
    const frame = { pool, set: (...args) => calls.push(['frame', ...args]) };
    const host = { pool, addSharedRegular: (...args) => calls.push(['host', ...args]) };
    const factory = { pool, create: (...args) => { calls.push(['factory', ...args]); return 8; } };
    const assets = { pool, damageableHandle: (...args) => { calls.push(['assets', ...args]); return 4.5; } };
    const limits = [];
    const random = { next: (limit) => { limits.push(limit); return 7; } };
    const original = { chat: () => {}, setFrame: forbid('이전') };
    const bundle = makeAltarSacrificeHooks(pool, frame, host, factory, assets, random, original);
    assert.notEqual(bundle, original);
    assert.equal(bundle.chat, original.chat);
    bundle.setFrame(7, 12, 0);
    bundle.addShared(7, 20, 0.5);
    assert.equal(bundle.create(158, 0), 8);
    assert.equal(bundle.damageable(7, 31, 1, 2.5), 4.5);
    assert.deepEqual(calls, [['frame', 7, 12, 0], ['host', 7, 20, 0.5], ['factory', 158, 0], ['assets', 7, 31, 1, 2.5]]);
    // 정수 난수는 low + next(high − low) 이고 상한은 DWORD 로 감기며 결과는 int32 이다.
    assert.equal(bundle.randomInt(100, 110), 107);
    assert.equal(bundle.randomInt(-5, 5), 2);
    assert.deepEqual(limits, [10, 10]);
    assert.equal(bundle.randomInt(5, 0), (5 + 7) | 0);
    assert.deepEqual(limits.at(-1), (0 - 5) >>> 0);
    const wrap = makeAltarSacrificeHooks(pool, frame, host, factory, assets, { next: () => 0xffffffff }, {});
    assert.equal(wrap.randomInt(10, 20), (10 + 0xffffffff) | 0);
    // 실수 난수는 next(65536)/65536 × |high − low| + low 를 float32 로 저장한다.
    const real = makeAltarSacrificeHooks(pool, frame, host, factory, assets, { next: (limit) => { limits.push(limit); return 16384; } }, {});
    assert.equal(real.randomFloat(0, 2), 0.5);
    assert.equal(real.randomFloat(1, 0), Math.fround(0.25 * 1 + 1));
    assert.equal(real.randomFloat(0, -4), 1);
    assert.equal(limits.at(-1), 65536);
    // 다른 풀의 연결이 하나라도 있으면 거부한다.
    const alien = { pool: altarScene(plain(), 'altar').world.pool };
    for (const [index, parts] of [[0, [alien, host, factory, assets]], [1, [frame, alien, factory, assets]], [2, [frame, host, alien, assets]], [3, [frame, host, factory, alien]]]) {
        assert.throws(() => makeAltarSacrificeHooks(pool, ...parts, random, {}), TypeError, `연결 ${index}`);
    }
    assert.throws(() => makeAltarSacrificeHooks(pool, null, host, factory, assets, random), TypeError);
});

test('runAltarRegular: 좌표 float 은 네 바이트 모두 제 위치에서 읽고, SID 1 도 유효한 다이스이며, 프레임 DWORD 의 셋째 바이트도 제 위치에서 읽는다', () => {
    // 낮은 바이트가 0 이 아닌 불규칙 좌표: 소리 위치는 다이스 좌표 − 3.0(float32)·제단 좌표 그대로이다.
    const ax = Math.fround(12.3456789);
    const ay = Math.fround(13.7891234);
    const dx = Math.fround(45.6789012);
    const dy = Math.fround(46.1234567);
    const slots = { 10: { type: 1, frame: 3, x: ax, y: ay }, 20: { type: 2, frame: 0, x: dx, y: dy, owner: 2 } };
    const chain = pureAltar(slots, { dais: 20, riQ: [99999, 0, 0], rfQ: [0] });
    chain.run(10, 21, 0, 9);
    const centerX = Math.fround(dx - 3);
    const centerY = Math.fround(dy - 3);
    assert.equal(chain.log.find((entry) => entry.startsWith('snd:')), `snd:${centerX},${centerY},chainRattle23.wav,0`);
    const finish = pureAltar(slots, { dais: 20, boss: true, advanceQ: [true], priest: 0, daisQ: [20, 20, 20] });
    finish.run(10, 24, 0, 0.5);
    assert.ok(finish.log.includes(`snd:${ax},${ay},explosion.wav,0`));
    const collapse = pureAltar(slots, { dais: 20, advanceQ: [false], rfQ: [0] });
    collapse.run(10, 24, 0, 0.5);
    assert.ok(collapse.log.includes(`snd:${ax},${ay},altarBurnCollapse.wav,0`));
    // SID 1 은 유효한 주소(1..capacity-1)이다: 다이스가 1 이어도 24 를 진행하고 이탈 프레임을 지정한다.
    const one = pureAltar({ 10: slots[10], 1: { type: 2, frame: 0, x: dx, y: dy } }, { dais: 1, advanceQ: [false], rfQ: [1] });
    assert.equal(one.run(10, 24, 0, 0.5), 0.5);
    assert.ok(one.log.includes('esc:1,0'));
    // 프레임 DWORD 의 셋째 바이트: 코드표가 충분히 크면 0x10002 가 방향 14 이다.
    const big = Array.from({ length: 0x10100 }, () => ({ side: 0x41, variant: 0x50, number: 1, flags: 0 }));
    big[0x10002] = { side: 0x4f, variant: 0x50, number: 1, flags: 0 };
    const tables = pureTables();
    tables[1] = new FrameCodeTable(big);
    const wide = pureAltar({ 10: { type: 1, frame: 0x10002 } }, { tables, advanceQ: [true] });
    assert.equal(wide.run(10, 20, 0, 1), 0);
    assert.ok(wide.log.includes('adv:10,1,0'));
});

test('RawAltarSacrifice: 하위 SID 1 의 다이스도 유효하고, 표 밖 TYPE 은 방향·첫 번호 조회에서 거부하며, P 첫 번호는 프레임 DWORD 네 바이트에 쓴다', () => {
    // 물리 슬롯 1(예약 번호)을 다이스로 사슬 머리에 놓아도 SID 1 은 유효한 주소라서 개수를 조회한다.
    const s = altarScene(plain(), 'altar');
    const calls = [];
    const counted = rebuild(s, { ...s.hooks, count: (sid) => { calls.push(sid); return 9; } });
    const raw = s.world.pool.physical(1);
    raw.fill(0);
    put(raw, 0, PATCH_DAIS_VTABLE);
    raw[10] = 165;
    put(raw, 14, floatToBits(40.75), 4);
    put(raw, 18, floatToBits(41.25), 4);
    put(raw, 4, s.world.hash.bucket(1, 40.75, 41.25), 2);
    s.world.hash.setBucket(1, 40.75, 41.25, 1);
    assert.equal(counted.daisAt(7), 1);
    assert.equal(counted.count(7), 9);
    assert.deepEqual(calls, [1]);
    // 이탈 프레임의 방향 조회는 대상 TYPE 이 표 밖(188)이면 거부한다(187 은 표 안의 빈 코드표라 다른 오류).
    for (const type of [188, 255]) {
        const w = altarScene(plain(), 'altar');
        w.world.slots[11][10] = type;
        assert.throws(() => w.altar.escapeFrame(11, 1), (error) => error instanceof RangeError && /방향 TYPE 물리 범위/.test(error.message), `TYPE ${type}`);
    }
    // 해방의 P 첫 번호 조회도 제단 TYPE 이 표 밖이면 직접 쓰기 전에 거부한다.
    const first = altarScene(plain(), 'altar');
    first.world.slots[11][10] = 188;
    assert.throws(() => first.altar.releaseFromAltar(11), (error) => error instanceof RangeError && /frame TYPE 물리 범위/.test(error.message));
    assert.deepEqual(first.effects, ['unpop:11,0']);
    // P 첫 번호가 한 바이트를 넘으면(258) 프레임 DWORD 의 낮은 두 바이트에 쓰고 나머지 두 바이트는 0 이 된다.
    const wide = altarScene(plain(), 'altar');
    wide.world.frames[164] = new FrameCodeTable(Array.from({ length: 300 }, (_, index) => ({ side: index === 258 ? 0x50 : 0x41, variant: 0x50, number: 1, flags: 0 })));
    detachDais(wide);
    wide.world.slots[7].set([0xaa, 0xaa, 0xaa, 0xaa], 36);
    wide.altar.releaseFromAltar(7);
    assert.deepEqual([...wide.world.slots[7].subarray(36, 40)], [2, 1, 0, 0]);
});
