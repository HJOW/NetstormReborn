// 신전 의식 raw-temple-ritual.js(TempleRitualState·RawTempleRitual·makeTempleRitualHooks)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: templeritual-x86.tsv 의 originals 행 — 모드·현재 프레임·방향 글자·사제/룬 프로필·권한·예약 결과·지면 값·위치·효과 뒤 변이(권한 반전·신전 프레임 변경)를 입력으로, 가상 count·단계·Anchor/Point 좌표 비트·
//    룬 조회·강한 사제 조회·추상 룬 조회, 의식 전체의 효과 순서(F 프레임/A 위치 음향/N 확보/S 공유 예약/C 생성/O 소유자/P Pop), 네 슬롯의 raw 전체(가상 표 DWORD 제외), 최종 권한이 같다.
//    cpppj TempleRitualTests.cpp 의 장면(합성 SHP·방향 코드·첫 프레임 표)을 같은 순서로 옮겼다.
//  - cpppj 시험 이식: 실제 Factory/Owner → 같은 Frame → 실제 0x307 Host 예약의 mode 0 사슬(rawTempleRitual_real_frame_and_307_shared_consumer).
//  - 웹 계약: 생성자 검사, stageFromCount 경계 표, 훅 누락 거부, makeTempleRitualHooks 의 새 묶음/풀 불일치.
// 10.72(CD)는 범위 밖이다. 실제 게임·브라우저 실행이 아니다. (2026-11-13 WB04-E365: 7-P3 RawTempleRitual 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, floatToBits, bitsToFloat } from './raw-support.mjs';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { SidPool } from '../www/core/sid-pool.js';
import { Kernel } from '../www/core/kernel.js';
import { GameRandom } from '../www/core/random.js';
import { ScrambledSpStore } from '../www/core/sp-store.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { SquidFrame } from '../www/core/squid-frame.js';
import { SquidFactory } from '../www/core/squid-factory.js';
import { SquidOwner, SquidOwnerMode } from '../www/core/squid-owner.js';
import { RawSquidDestroy } from '../www/core/squid-destroy.js';
import { SquidPostPopState } from '../www/core/squid-postpop-state.js';
import { SquidReward, SquidRewardState } from '../www/core/squid-reward.js';
import { SquidProcessHost, SquidProcessState } from '../www/core/squid-process.js';
import { RawSquidNeighbors } from '../www/core/raw-squid-neighbors.js';
import { RawPriestRitualActions, makePriestRitualActionHooks } from '../www/core/raw-priest-ritual-actions.js';
import { setRawSquidHitPoints } from '../www/core/raw-squid-word.js';
import { RawTempleRitual, TempleRitualState, makeTempleRitualHooks } from '../www/core/raw-temple-ritual.js';

// 원본 관측 10.78 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-13 WB04-E365)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'templeritual-x86.tsv'))?.filter((row) => row[0] === 'originals') ?? null;

// 합성 가상 표 주입값(이미지 비교는 앞 DWORD 를 제외한다)과 월드 크기이다. (2026-11-13 WB04-E365)
const VTABLE = 0x1240f000;
const CAPACITY = 24000;

// 바이트 배열의 소문자 16진 문자열이다. (2026-11-13 WB04-E365)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 188칸 TYPE 입력이다(cpppj MakeFormTypeInputs: 모든 칸 0 + 내장 Form 네 생성자 주소). (2026-11-13 WB04-E365)
function formTypeInputs() {
    return Array.from({ length: 188 }, (_, number) => ({
        cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0, group: 0,
        ...([1, 2, 3, 6].includes(number) ? { constructorAddress: typeConstructorAddress(number) } : {}),
    }));
}

// 합성 SHP 와 방향 코드/첫 프레임 표 입력을 만든다(cpppj TempleAssets). 출력 선택은 계산하지 않는다. (2026-11-13 WB04-E365)
function templeAssets(types, frames, shapes, side) {
    // native 와 같은 세 타입의 기본 HP/핫스폿과 방향 코드이다.
    for (const type of [158, 165, 166]) {
        const count = type === 165 ? 180 : type === 166 ? 6 : 32;
        Object.assign(types[type], { maxHitPoints: 200, flags1: 16, footX: type === 165 ? 3 : 1, footY: type === 165 ? 3 : 1 });
        const codes = [];
        shapes[type] = { loaded: true, frameCount: count, frames: [] };
        // SHP 정수 핫스폿은 입력이며 목표점의 계산은 복원 몸체가 한다.
        for (let index = 0; index < count; index += 1) {
            shapes[type].frames.push({ width: 16, height: 11, hotspotX: (index % 5) * 16, hotspotY: (index % 5) * 11, muzzleX: (index % 5) * 16, muzzleY: (index % 5) * 11 });
            codes.push({ side: type === 165 ? side.charCodeAt(0) : 0x41, variant: 0x50, number: index + 1, flags: 0 });
        }
        // 사제 first-table 은 A 둘/K..P 각 하나다. 이후 P 는 해당 첫 프레임을 바꾸지 않는다.
        if (type === 158) for (let index = 2; index < count; index += 1) codes[index].side = index < 8 ? 0x4b + index - 2 : 0x50;
        frames[type] = new FrameCodeTable(codes);
    }
    types[158].flags2 = 0x210000;
}

// 효과 순서를 원본과 같은 구분자로 기록한다. (2026-11-13 WB04-E365)
const templeEffects = (effects) => (effects.length ? effects.join(';') : '-');

// 원본 목표점 출력의 두 단정도 비트를 묶는다. (2026-11-13 WB04-E365)
const pointBits = (point) => `${floatToBits(point.x)},${floatToBits(point.y)}`;

test('TempleRitual: 원본 10.78 관측 전 행의 단계·목표점·조회·의식 효과 순서·슬롯 raw·최종 권한이 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    let checked = 0;
    // 입력마다 같은 상태 저장소를 새로 만들어 앞선 행의 예측 결과/예약을 남기지 않는다.
    for (const row of rows) {
        assert.equal(row.length, 7);
        const input = row[1].split(',');
        const xy = row[2].split(',');
        assert.deepEqual([input.length, xy.length], [9, 4]);
        const mode = Number(input[0]);
        const currentFrame = Number(input[1]);
        const profile = Number(input[3]);
        const boss = Number(input[4]);
        const reserve = Number(input[5]);
        const spot = Number(input[6]);
        const mutation = Number(input[8]);
        const pool = new SidPool(CAPACITY, { server: true });
        const hash = new SquidHash();
        const spots = new Uint8Array(65536).fill(spot);
        const types = formTypeInputs();
        const frames = types.map(() => new FrameCodeTable([]));
        const shapes = types.map(() => ({ loaded: false, frameCount: 0, frames: [] }));
        templeAssets(types, frames, shapes, input[2][0]);
        const slots = [];
        // 네 raw 슬롯의 초기 바이트는 native 와 같고 누락 룬도 0 이미지로 유지한다.
        for (let sid = 5; sid <= 8; sid += 1) {
            assert.equal(pool.allocate(2), sid);
            slots[sid] = pool.allocated(sid);
            slots[sid].fill(0);
        }
        // 슬롯 하나를 입력 좌표 비트로 채우고 일반 해시 첫 단계 버킷에 건다(좌표 비트는 float 를 거치지 않고 그대로 쓴다).
        const add = (sid, type, xBits, yBits, extra, rawState) => {
            const raw = slots[sid];
            put(raw, 0, VTABLE);
            raw[10] = type;
            raw[11] = rawState;
            raw[40] = extra;
            put(raw, 14, xBits);
            put(raw, 18, yBits);
            const x = bitsToFloat(xBits);
            const y = bitsToFloat(yBits);
            put(raw, 4, hash.bucket(1, x, y), 2);
            hash.setBucket(1, x, y, sid);
        };
        const xBits = Number(xy[0]);
        const yBits = Number(xy[1]);
        const x = bitsToFloat(xBits);
        const y = bitsToFloat(yBits);
        add(5, 165, floatToBits(40.75), floatToBits(41.25), 0, 0);
        add(6, 158, xBits, yBits, 0, 0);
        put(slots[5], 36, currentFrame >>> 0, 4);
        slots[5][34] = 1;
        slots[6][34] = profile === 2 ? 2 : 1;
        setRawSquidHitPoints(pool, 6, profile === 1 ? 99 : 100);
        if (profile >= 3) add(7, 166, Number(xy[2]), Number(xy[3]), profile === 6 ? 8 : profile === 5 ? 1 : 0, profile === 4 ? 2 : 0);
        const footprint = new RawSquidNeighbors(pool, hash, spots, types, frames);
        const reward = new SquidReward(pool, types, new SquidPostPopState(), new SquidRewardState(), new ScrambledSpStore(new GameRandom(0), () => 1));
        const state = new TempleRitualState({ boss: boss !== 0 });
        const effects = [];
        const allocate = () => { effects.push('N:40'); if (mutation === 3) state.boss = !state.boss; return reserve !== 0; };
        const shared = (sid, event, payload) => { effects.push(`S:${event},${sid},${floatToBits(payload)}`); };
        const actionHooks = {
            setFrame: (sid, frame, flags) => {
                assert.deepEqual([sid, flags], [6, 0]);
                effects.push(`F:6,${frame},0`);
                put(slots[6], 36, frame >>> 0, 4);
                if (mutation === 1) state.boss = !state.boss;
                if (mutation === 2) put(slots[5], 36, 14, 4);
            },
            reserveShared: allocate,
            addShared: shared,
        };
        const actions = new RawPriestRitualActions(pool, frames, actionHooks);
        let fixtureCount = null;
        const hooks = {
            reserveShared: allocate,
            addShared: shared,
            create: (type, flags) => {
                assert.deepEqual([type, flags], [166, 0]);
                effects.push('C:166,0');
                slots[8].fill(0xa5);
                put(slots[8], 0, VTABLE);
                slots[8][10] = 166;
                slots[8][11] = 4;
                return 8;
            },
            setHitPoints: (sid, hp) => { assert.deepEqual([sid, hp], [8, 0]); setRawSquidHitPoints(pool, sid, hp); },
            setOwner: (sid, player) => { assert.equal(sid, 8); effects.push(`O:8,${player}`); slots[8][34] = player; },
            pop: (sid, px, py, flags) => {
                assert.equal(sid, 8);
                effects.push(`P:8,${pointBits({ x: px, y: py })},${flags}`);
                put(slots[8], 14, floatToBits(px));
                put(slots[8], 18, floatToBits(py));
                slots[8][11] &= ~4;
            },
            playAt: (px, py, sound, flags) => { effects.push(`A:${pointBits({ x: px, y: py })},${sound},${flags}`); },
            // 기존 합성 가상 표는 실제 기존 count 몸체를 명시 연결하며 원문 기대값은 유지한다.
            count: (sid) => { if (!fixtureCount) throw new Error('기존 fixture의 실제 Count 장치 설치 전 호출'); return fixtureCount.count(sid); },
        };
        const ritual = new RawTempleRitual(pool, hash, types, frames, shapes, spots, footprint, reward, actions, state, hooks);
        fixtureCount = ritual;
        const stage = ritual.stage(5);
        const actual = [ritual.count(5), stage, pointBits(ritual.stageAnchor(5, Math.min(stage, 4))), pointBits(ritual.stagePoint(5, Math.min(stage, 4))),
            ritual.runeAt(5, { x, y }), ritual.findStrongOwnedPriest(5), ritual.hasAbstractRune(5) ? 1 : 0].join(',');
        assert.equal(actual, row[3], `행 ${checked} 조회`);
        ritual.ritual(5, 6, mode);
        assert.equal(templeEffects(effects), row[4], `행 ${checked} 효과`);
        assert.equal(state.boss, Number(row[6]) !== 0, `행 ${checked} 권한`);
        const images = row[5].split(';');
        assert.equal(images.length, 4);
        // 가상 표 주입 DWORD 외 raw 전체를 비교해 HP 폭/extra/프레임/옆 stale 바이트도 확인한다.
        for (let sid = 5; sid <= 8; sid += 1) assert.equal(hex(pool.slot(sid)).slice(8), images[sid - 5].slice(8), `행 ${checked} 슬롯 ${sid}`);
        checked += 1;
    }
    assert.ok(checked > 1000);
});

test('TempleRitual: 실제 Factory/Owner → 같은 Frame → 실제 0x307 Host 예약의 mode 0 사슬을 현재 신전/사제 슬롯에서 실행한다(rawTempleRitual_real_frame_and_307_shared_consumer)', () => {
    const pool = new SidPool(CAPACITY, { server: true });
    const hash = new SquidHash();
    const spots = new Uint8Array(65536);
    const types = formTypeInputs();
    const frames = types.map(() => new FrameCodeTable([]));
    const shapes = types.map(() => ({ loaded: false, frameCount: 0, frames: [] }));
    templeAssets(types, frames, shapes, 'A');
    // 165/158/166 의 실제 생성자 주소를 입력으로 공급하고 base vtable 로 대체하지 않는다.
    for (const type of [158, 165, 166]) types[type].constructorAddress = typeConstructorAddress(type);
    const book = new SquidPostPopState();
    const owner = new SquidOwner(pool, types, book, new SquidOwnerMode());
    const factory = new SquidFactory(pool, types);
    const temple = factory.create(165, 2);
    const priest = factory.create(158, 2);
    owner.set(temple, 1);
    owner.set(priest, 1);
    setRawSquidHitPoints(pool, priest, 100);
    const templeRaw = pool.allocated(temple);
    const priestRaw = pool.allocated(priest);
    put(templeRaw, 14, floatToBits(40.75));
    put(templeRaw, 18, floatToBits(41.25));
    put(templeRaw, 36, 25, 4);
    templeRaw[11] = 0;
    priestRaw[11] = 0;
    priestRaw[33] = 1;
    const footprint = new RawSquidNeighbors(pool, hash, spots, types, frames);
    const reward = new SquidReward(pool, types, book, new SquidRewardState(), new ScrambledSpStore(new GameRandom(0), () => 1));
    let updates = 0;
    let sounds = 0;
    let reservations = 0;
    const frame = new SquidFrame(pool, types, {
        frameSize: (sid, index) => { assert.ok(sid === priest && index >= 0 && index < frames[158].codes.length); return [1, 1]; },
        update: (sid, flags) => { assert.deepEqual([sid, flags], [priest, 0]); updates += 1; },
        unpop: () => { throw new Error('같은 단계의 사제 의식 프레임은 Unpop 을 요청하지 않는다'); },
        pop: () => { throw new Error('같은 단계의 사제 의식 프레임은 Pop 을 요청하지 않는다'); },
    });
    const destroy = new RawSquidDestroy(pool, new SquidUnpop(pool, hash, spots), types);
    const kernel = new Kernel(32);
    const clock = new SquidProcessState();
    clock.now = 177;
    const host = new SquidProcessHost(pool, types, kernel, destroy, clock, { asset: { selected: () => 0, emit: () => { throw new Error('신전 의식 mode0 은 자산 삭제를 요청하지 않는다'); } } });
    try {
        const actions = new RawPriestRitualActions(pool, frames, makePriestRitualActionHooks(pool, frame, host, { reserveShared: () => { throw new Error('신전 의식 mode0 은 사제 604 확보를 요청하지 않는다'); } }));
        const state = new TempleRitualState();
        const ritual = new RawTempleRitual(pool, hash, types, frames, shapes, spots, footprint, reward, actions, state, makeTempleRitualHooks(pool, factory, owner, host, {
            reserveShared: () => { reservations += 1; return true; },
            pop: () => { throw new Error('신전 의식 mode0 은 Rune 생성/Pop 을 요청하지 않는다'); },
            playAt: (x, y, sound, flags) => { assert.deepEqual([x, y, sound, flags, updates], [42.75, 43.25, 'forRain2.wav', 0, 2]); sounds += 1; },
        }));
        const point = ritual.stagePoint(temple, 2);
        put(priestRaw, 14, floatToBits(point.x));
        put(priestRaw, 18, floatToBits(point.y));
        hash.setBucket(1, point.x, point.y, priest);
        ritual.ritual(temple, priest, 0);
        assert.deepEqual([updates, sounds, reservations, kernel.size], [2, 1, 1, 1]);
        const pending = host.findEvent(temple, 0x307);
        assert.ok(pending);
        assert.deepEqual([pending.payload, pending.time, pending.parent], [1.8799999952316284, 177, temple]);
        assert.deepEqual([ritual.pool === host.pool, get(priestRaw, 36, 4)], [true, 4]);
    } finally {
        kernel.clear();
    }
});

// 계약 시험용 작은 월드이다: 신전 5(165)·사제 6(158)·선택 룬 7(166)을 만들고 합성 SHP/프레임 표를 쓴다. 효과는 모두 기록만 한다. (2026-11-13 WB04-E365)
function contractWorld(overrides = {}) {
    const pool = new SidPool(CAPACITY, { server: true });
    const hash = new SquidHash();
    const spots = new Uint8Array(65536).fill(overrides.spot ?? 0);
    const types = formTypeInputs();
    const frames = types.map(() => new FrameCodeTable([]));
    const shapes = types.map(() => ({ loaded: false, frameCount: 0, frames: [] }));
    templeAssets(types, frames, shapes, overrides.side ?? 'A');
    for (let sid = 5; sid <= 8; sid += 1) { assert.equal(pool.allocate(2), sid); pool.allocated(sid).fill(0); }
    const world = { pool, hash, spots, types, frames, shapes, effects: [], state: new TempleRitualState(overrides.state ?? {}) };
    // 슬롯 하나를 float 좌표로 채우고 해시 첫 단계 버킷에 건다.
    world.add = (sid, type, x, y, extra = 0, rawState = 0) => {
        const raw = pool.allocated(sid);
        put(raw, 0, VTABLE);
        raw[10] = type;
        raw[11] = rawState;
        raw[40] = extra;
        put(raw, 14, floatToBits(x));
        put(raw, 18, floatToBits(y));
        put(raw, 4, hash.bucket(1, x, y), 2);
        hash.setBucket(1, x, y, sid);
        return raw;
    };
    world.add(5, 165, 40.75, 41.25);
    pool.allocated(5)[34] = 1;
    world.add(6, 158, 42.75, 43.25);
    pool.allocated(6)[34] = 1;
    setRawSquidHitPoints(pool, 6, 100);
    world.footprint = new RawSquidNeighbors(pool, hash, spots, types, frames);
    world.reward = new SquidReward(pool, types, new SquidPostPopState(), new SquidRewardState(), new ScrambledSpStore(new GameRandom(0), () => 1));
    world.actions = new RawPriestRitualActions(pool, frames, {
        setFrame: (sid, frame) => { world.effects.push(`F:${sid},${frame}`); put(pool.allocated(sid), 36, frame >>> 0, 4); },
        reserveShared: () => true,
        addShared: (sid, event) => { world.effects.push(`S:${event},${sid}`); },
    });
    world.hooks = {
        count: (sid) => world.make({}).count(sid),
        reserveShared: () => true,
        addShared: (sid, event, payload) => { world.effects.push(`S:${event},${sid},${payload}`); },
        create: (type, flags) => { world.effects.push(`C:${type},${flags}`); const raw = pool.allocated(8); raw.fill(0xa5); raw[10] = type; raw[11] = overrides.runeState ?? 4; return 8; },
        setHitPoints: (sid, hp) => { world.effects.push(`H:${sid},${hp}`); setRawSquidHitPoints(pool, sid, hp); },
        setOwner: (sid, player) => { world.effects.push(`O:${sid},${player}`); pool.allocated(sid)[34] = player; },
        pop: (sid, x, y, flags) => { world.effects.push(`P:${sid},${pointBits({ x, y })},${flags}`); },
        playAt: (x, y, sound, flags) => { world.effects.push(`A:${pointBits({ x, y })},${sound},${flags}`); },
    };
    world.make = (hooks = world.hooks) => new RawTempleRitual(pool, hash, types, frames, shapes, spots, world.footprint, world.reward, world.actions, world.state, hooks);
    return world;
}

test('TempleRitual 생성자: PATCH 풀·같은 풀/해시의 하위 연결·188칸 표·지도 크기만 받는다', () => {
    const w = contractWorld();
    assert.equal(w.make().pool, w.pool);
    assert.throws(() => new RawTempleRitual({ edition: 'cd1072' }, w.hash, w.types, w.frames, w.shapes, w.spots, w.footprint, w.reward, w.actions, w.state, {}), TypeError);
    const otherPool = new SidPool(CAPACITY, { server: true });
    const otherFootprint = new RawSquidNeighbors(otherPool, w.hash, w.spots, w.types, w.frames);
    const otherHash = new RawSquidNeighbors(w.pool, new SquidHash(), w.spots, w.types, w.frames);
    const otherReward = new SquidReward(otherPool, w.types, new SquidPostPopState(), new SquidRewardState(), new ScrambledSpStore(new GameRandom(0), () => 1));
    const otherActions = new RawPriestRitualActions(otherPool, w.frames, { setFrame: () => {}, reserveShared: () => true, addShared: () => {} });
    const mismatch = (error) => error instanceof TypeError && /풀\/해시 불일치/.test(error.message);
    for (const [footprint, reward, actions] of [[otherFootprint, w.reward, w.actions], [otherHash, w.reward, w.actions], [w.footprint, otherReward, w.actions], [w.footprint, w.reward, otherActions]]) {
        assert.throws(() => new RawTempleRitual(w.pool, w.hash, w.types, w.frames, w.shapes, w.spots, footprint, reward, actions, w.state, {}), mismatch);
    }
    const size = (error) => error instanceof RangeError && /표 크기 오류/.test(error.message);
    assert.throws(() => new RawTempleRitual(w.pool, w.hash, w.types.slice(1), w.frames, w.shapes, w.spots, w.footprint, w.reward, w.actions, w.state, {}), size);
    assert.throws(() => new RawTempleRitual(w.pool, w.hash, w.types, w.frames.slice(1), w.shapes, w.spots, w.footprint, w.reward, w.actions, w.state, {}), size);
    assert.throws(() => new RawTempleRitual(w.pool, w.hash, w.types, w.frames, w.shapes.slice(1), w.spots, w.footprint, w.reward, w.actions, w.state, {}), size);
    assert.throws(() => new RawTempleRitual(w.pool, w.hash, w.types, w.frames, w.shapes, new Uint8Array(65535), w.footprint, w.reward, w.actions, w.state, {}), size);
});

test('TempleRitual stageFromCount: count 별 첫 프레임 [1, 3, 64, 120] 을 빼 11/22/33/44/54 경계로 0..5 를 고르고 감김·범위 밖 count 를 처리한다', () => {
    for (const [count, first] of [[0, 1], [1, 3], [2, 64], [3, 120]]) {
        const expected = [[10, 0], [11, 1], [21, 1], [22, 2], [32, 2], [33, 3], [43, 3], [44, 4], [54, 4], [55, 5], [-5, 0], [300, 5]];
        for (const [relative, stage] of expected) assert.equal(RawTempleRitual.stageFromCount(first + relative, count), stage, `count ${count} 상대 ${relative}`);
    }
    // DWORD 감김: 상대값이 int32 로 감겨 양수가 되는 입력은 5, 음수가 되는 입력은 0 이다.
    assert.equal(RawTempleRitual.stageFromCount(-2147483648, 1), 5);
    assert.equal(RawTempleRitual.stageFromCount(2147483647, 0), 5);
    assert.equal(RawTempleRitual.stageFromCount(-2147483647, 3), 5);
    assert.throws(() => RawTempleRitual.stageFromCount(0, 4), (error) => error instanceof RangeError && /count 첫 프레임 표 범위 오류/.test(error.message));
});

test('TempleRitual count/stage: 방향 글자 N/M/L 이 프레임 구간보다 먼저이고, 현재 프레임은 가상 조회 전에 저장되며, 조회 장치가 없으면 실제 Dais 가상 표만 직접 count 를 쓴다', () => {
    const w = contractWorld();
    const side = (letter) => { w.frames[165] = new FrameCodeTable(Array.from({ length: 180 }, (_, index) => ({ side: letter.charCodeAt(0), variant: 0x50, number: index + 1, flags: 0 }))); };
    const frameAt = (frame) => put(w.pool.allocated(5), 36, frame >>> 0, 4);
    const ritual = w.make();
    // 방향 글자: N→1, M→2, L→3 (프레임과 무관), 그 밖(A)은 프레임 구간 63/64/119/120 경계.
    for (const [letter, frame, expected] of [['N', 150, 1], ['M', 0, 2], ['L', 0, 3], ['A', 0, 1], ['A', 63, 1], ['A', 64, 2], ['A', 119, 2], ['A', 120, 3], ['A', 179, 3], ['O', 179, 3], ['K', 10, 1]]) {
        side(letter);
        frameAt(frame);
        assert.equal(w.make().count(5), expected, `${letter}/${frame}`);
    }
    // 프레임이 표 밖이거나 TYPE 이 표 밖이면 RangeError 이다.
    side('A');
    frameAt(180);
    assert.throws(() => ritual.count(5), RangeError);
    frameAt(0xffffffff);
    assert.throws(() => ritual.count(5), RangeError);
    frameAt(0);
    w.pool.allocated(5)[10] = 255;
    assert.throws(() => ritual.count(5), (error) => error instanceof RangeError && /Count 현재 TYPE/.test(error.message));
    w.pool.allocated(5)[10] = 165;
    // stage: 가상 조회(hooks.count)는 호출마다 한 번이며 frame −1 은 조회 전에 읽은 현재 프레임을 쓴다(조회가 프레임을 바꿔도 저장된 값).
    frameAt(25);
    const calls = [];
    const hooks = { ...w.hooks, count: (sid) => { calls.push(sid); frameAt(200); return 1; } };
    const probe = w.make(hooks);
    assert.equal(probe.stage(5), 2);
    assert.deepEqual(calls, [5]);
    // 명시 frame 은 현재 프레임을 읽지 않는다.
    frameAt(25);
    assert.equal(probe.stage(5, 14), 1);
    assert.equal(probe.stage(5, 25), 2);
    // 조회 장치가 없으면: 실제 Dais 가상 표(0x507950)는 직접 count, 아니면 거부.
    frameAt(25);
    const bare = w.make({ ...w.hooks, count: undefined });
    assert.throws(() => bare.stage(5), (error) => error instanceof Error && /가상80 분배 장치 미연결/.test(error.message));
    put(w.pool.allocated(5), 0, 0x507950);
    assert.equal(bare.stage(5), 2);
    put(w.pool.allocated(5), 0, 0x507951);
    assert.throws(() => bare.stage(5), /가상80 분배/);
});

test('TempleRitual 목표점: stageAnchor/stagePoint 는 SHP 포구 쌍과 현재 픽셀 배율로 계산하고 잘못된 단계·배율·SHP·지면 범위를 거부하며 단계 4 의 지면 보정 조건을 따른다', () => {
    const w = contractWorld();
    const ritual = w.make();
    // 단계 s 의 Anchor: 신전 좌표 + (포구 쌍 ÷ 배율), 포구 인덱스 = 3 + 오프셋[s], 합성 SHP 의 포구는 (index%5)*(16,11).
    for (const [stage, index] of [[0, 3], [1, 14], [2, 25], [3, 36], [4, 47]]) {
        const hotX = (index % 5) * 16;
        const hotY = (index % 5) * 11;
        const anchor = ritual.stageAnchor(5, stage);
        assert.equal(anchor.x, Math.fround(hotX / 16 + 40.75), `x ${stage}`);
        assert.equal(anchor.y, Math.fround(hotY / 11 + 41.25), `y ${stage}`);
    }
    assert.throws(() => ritual.stageAnchor(5, 5), (error) => error instanceof RangeError && /numBridgesMissing/.test(error.message));
    // 픽셀 배율이 비유한/0 이면 거부하고, 배율을 바꾸면 좌표가 바뀐다.
    for (const scale of [0, NaN, Infinity]) {
        w.state.pixelScaleX = scale;
        assert.throws(() => ritual.stageAnchor(5, 1), (error) => error instanceof RangeError && /픽셀 배율 오류/.test(error.message));
    }
    w.state.pixelScaleX = 16;
    w.state.pixelScaleY = 0;
    assert.throws(() => ritual.stageAnchor(5, 1), RangeError);
    w.state.pixelScaleY = 11;
    w.state.pixelScaleX = 8;
    assert.equal(ritual.stageAnchor(5, 1).x, Math.fround(((14 % 5) * 16) / 8 + 40.75));
    w.state.pixelScaleX = 16;
    // 단계 4 의 지면 보정: 절삭 칸의 지도 값이 (6 비트가 있고 8 비트가 없을 때)만 (−9, −6) 픽셀을 더한다.
    const base = ritual.stagePoint(5, 4);
    const cellOf = (point) => Math.trunc(point.y) * 256 + Math.trunc(point.x);
    for (const [spot, corrected] of [[0, false], [1, false], [2, true], [4, true], [6, true], [8, false], [10, false], [14, false], [16, false], [18, true]]) {
        w.spots[cellOf(base)] = spot;
        const point = ritual.stagePoint(5, 4);
        const expected = corrected ? { x: Math.fround(-9 / 16 + base.x), y: Math.fround(-6 / 11 + base.y) } : base;
        assert.deepEqual([point.x, point.y], [expected.x, expected.y], `spot ${spot}`);
    }
    w.spots.fill(0);
    // 룬 SHP 가 없거나 로드되지 않으면 거부한다.
    w.shapes[166].loaded = false;
    assert.throws(() => ritual.stagePoint(5, 0), (error) => error instanceof RangeError && /frameCheck/.test(error.message));
    w.shapes[166].loaded = true;
    w.state.runeType = 188;
    assert.throws(() => ritual.stagePoint(5, 0), RangeError);
    w.state.runeType = 166;
    // 프레임 두 배 비트(flags1 0x440000)이면 SHP 프레임 한계가 두 배이다(룬 frameCount 3: 단계 3 의 프레임 3 은 한계 밖, 두 배 비트가 있으면 안).
    w.shapes[166].frameCount = 3;
    assert.deepEqual([ritual.stagePoint(5, 2).x === ritual.stagePoint(5, 2).x], [true]);
    assert.throws(() => ritual.stagePoint(5, 3), (error) => error instanceof RangeError && /frameCheck/.test(error.message));
    w.types[166].flags1 = 0x440000;
    assert.equal(Number.isFinite(ritual.stagePoint(5, 3).x), true);
    assert.equal(Number.isFinite(ritual.stagePoint(5, 4).x), true);
    w.types[166].flags1 = 0x40000;
    assert.equal(Number.isFinite(ritual.stagePoint(5, 4).x), true);
    w.types[166].flags1 = 16;
    w.shapes[166].frameCount = 6;
    // 신전 TYPE 이 표 밖이거나 SHP 칸이 없으면 거부한다.
    w.pool.allocated(5)[10] = 187;
    assert.throws(() => ritual.stageAnchor(5, 0), RangeError);
});

test('TempleRitual 조회: runeAt 은 현재 룬 TYPE 의 첫 슬롯만(표면 건너뛰기는 룬 flags2 비트 1·2 가 없을 때), findStrongOwnedPriest 는 같은 소유자·HP ≥ 최대 HP÷2(정수 나눗셈·signed), hasAbstractRune 은 extra 비트 0', () => {
    const w = contractWorld();
    const ritual = w.make();
    // 룬 두 개가 같은 칸에 있을 때 해시 체인의 앞(나중에 건 것)이 먼저 나오며 첫 일치에서 끝난다.
    const rune = w.add(7, 166, 42.25, 43.5, 0, 0);
    w.add(8, 166, 42.25, 43.5, 0, 0);
    const point = { x: Math.fround(42.25 + (0 * 16) / 16), y: 43.5 };
    const found = ritual.runeAt(5, point);
    assert.ok(found === 7 || found === 8);
    // 룬 TYPE 이 아닌 슬롯만 있으면 0 이다.
    rune[10] = 71;
    w.pool.allocated(8)[10] = 71;
    assert.equal(ritual.runeAt(5, point), 0);
    rune[10] = 166;
    w.pool.allocated(8)[10] = 166;
    // 강한 사제: 같은 소유자·HP 100(최대 200)이면 통과, 99 는 탈락, 소유자가 다르면 탈락, 최대 HP 201 이면 100 이 통과(정수 절삭).
    assert.equal(ritual.findStrongOwnedPriest(5), 6);
    setRawSquidHitPoints(w.pool, 6, 99);
    assert.equal(ritual.findStrongOwnedPriest(5), 0);
    // 최대 HP 는 SquidReward 가 생성 때 복사한 표를 읽으므로 값을 바꾼 뒤 새 보상기를 만든다.
    w.types[158].maxHitPoints = 201;
    w.reward = new SquidReward(w.pool, w.types, new SquidPostPopState(), new SquidRewardState(), new ScrambledSpStore(new GameRandom(0), () => 1));
    const odd = w.make();
    setRawSquidHitPoints(w.pool, 6, 100);
    assert.equal(odd.findStrongOwnedPriest(5), 6);
    setRawSquidHitPoints(w.pool, 6, 99);
    assert.equal(odd.findStrongOwnedPriest(5), 0);
    setRawSquidHitPoints(w.pool, 6, -1);
    assert.equal(ritual.findStrongOwnedPriest(5), 0);
    w.types[158].maxHitPoints = 200;
    setRawSquidHitPoints(w.pool, 6, 100);
    w.pool.allocated(6)[34] = 2;
    assert.equal(ritual.findStrongOwnedPriest(5), 0);
    w.pool.allocated(6)[34] = 1;
    // 현재 사제 TYPE 번호는 소비 위치마다 읽는다(상자로 바꾸면 그 TYPE 이 사제).
    w.state.sharedPriestType = { value: 71 };
    assert.equal(ritual.findStrongOwnedPriest(5), 0);
    w.state.sharedPriestType = null;
    // 추상 룬: extra 비트 0 이 선 현재 룬 TYPE 슬롯이 신전 발자국(왼쪽 위 (38.75, 39.25) ~ 오른쪽 아래 (40.75, 41.25)) 안에 있을 때만 참이다. 발자국 밖의 룬(슬롯 7·8)은 보지 않는다.
    w.pool.allocated(7)[40] = 1;
    w.pool.allocated(8)[40] = 1;
    assert.equal(ritual.hasAbstractRune(5), false);
    const inside = w.pool.allocate(2);
    assert.equal(inside, 9);
    const insideRaw = w.add(9, 166, 39.5, 40.5, 0, 0);
    assert.equal(ritual.hasAbstractRune(5), false);
    insideRaw[40] = 1;
    assert.equal(ritual.hasAbstractRune(5), true);
    insideRaw[40] = 3;
    assert.equal(ritual.hasAbstractRune(5), true);
    insideRaw[40] = 2;
    assert.equal(ritual.hasAbstractRune(5), false);
    insideRaw[40] = 1;
    insideRaw[10] = 71;
    assert.equal(ritual.hasAbstractRune(5), false);
    insideRaw[10] = 166;
    // 현재 룬 TYPE 번호는 소비 위치마다 읽는다(상자로 바꾸면 그 TYPE 이 룬).
    w.state.sharedRuneType = { value: 71 };
    assert.equal(ritual.hasAbstractRune(5), false);
    w.state.sharedRuneType = null;
    assert.equal(ritual.hasAbstractRune(5), true);
});

test('TempleRitual spawnRune/ritual: 룬 생성 순서와 단계 재조회, void 진단 복귀, 필수 장치 누락 거부, 단계 5 는 아무것도 하지 않는다', () => {
    // 정상: 생성 → HP 0 → 현재 단계 프레임(DWORD) → abstract 1 → 현재 소유자 → 현재 단계 Anchor 에 Pop(flags 0).
    const w = contractWorld({ runeState: 4 });
    const ritual = w.make();
    put(w.pool.allocated(5), 36, 25, 4);
    ritual.spawnRune(5);
    const anchor = ritual.stageAnchor(5, 2);
    assert.deepEqual(w.effects, ['C:166,0', 'H:8,0', 'O:8,1', `P:8,${pointBits(anchor)},0`]);
    const rune = w.pool.slot(8);
    assert.deepEqual([get(rune, 36, 4), rune[40] & 1, rune[34]], [2, 1, 1]);
    // 생성된 슬롯이 void(비트 4)가 아니면 진단(식·파일·줄)을 부르고 복귀 뒤 계속하며, 진단 장치가 없으면 쓰기 직후 거부한다.
    const open = contractWorld({ runeState: 0 });
    put(open.pool.allocated(5), 36, 25, 4);
    const diagnostics = [];
    open.make({ ...open.hooks, assertion: (...args) => { diagnostics.push(args); } }).spawnRune(5);
    assert.deepEqual(diagnostics, [['isVoid()', 'c:\\ns\\o\\squid.h', 0x282]]);
    assert.equal(open.effects.at(-1).startsWith('P:8,'), true);
    const strict = contractWorld({ runeState: 0 });
    put(strict.pool.allocated(5), 36, 25, 4);
    assert.throws(() => strict.make().spawnRune(5), /선택된 실제 장치 미연결/);
    assert.deepEqual(strict.effects, ['C:166,0', 'H:8,0']);
    assert.equal(get(strict.pool.slot(8), 36, 4), 2);
    // 필수 장치가 없으면 해당 지점에서 거부하고 이전 효과는 남는다.
    for (const [name, effects] of [['create', []], ['setHitPoints', ['C:166,0']], ['setOwner', ['C:166,0', 'H:8,0']], ['pop', ['C:166,0', 'H:8,0', 'O:8,1']]]) {
        const missing = contractWorld();
        put(missing.pool.allocated(5), 36, 25, 4);
        const hooks = { ...missing.hooks };
        delete hooks[name];
        assert.throws(() => missing.make(hooks).spawnRune(5), /선택된 실제 장치 미연결/, name);
        assert.deepEqual(missing.effects, effects, name);
    }
    // 생성 SID 가 0 이거나 풀 밖이면 RangeError 이고 HP 효과 전에 멈춘다.
    for (const sid of [0, CAPACITY, CAPACITY + 1]) {
        const bad = contractWorld();
        const hooks = { ...bad.hooks, create: () => { bad.effects.push('C'); return sid; } };
        assert.throws(() => bad.make(hooks).spawnRune(5), (error) => error instanceof RangeError && /Rune 생성SID 오류/.test(error.message));
        assert.deepEqual(bad.effects, ['C']);
    }
    // 단계 5(count 1 구간에서 상대 프레임 55 이상: 프레임 60)는 의식 전체가 아무것도 하지 않는다.
    const late = contractWorld();
    put(late.pool.allocated(5), 36, 60, 4);
    assert.equal(late.make().stage(5), 5);
    late.make().ritual(5, 6, 0);
    late.make().ritual(5, 6, 1);
    assert.deepEqual(late.effects, []);
});

test('TempleRitual makeTempleRitualHooks: 실제 Factory/HP setter/Owner/Host 를 연결한 새 묶음을 돌려주고 나머지 훅과 원래 묶음은 보존하며 풀 불일치를 거부한다', () => {
    const pool = new SidPool(CAPACITY, { server: true });
    const types = formTypeInputs();
    for (const type of [165, 166]) types[type].constructorAddress = typeConstructorAddress(type);
    const book = new SquidPostPopState();
    const owner = new SquidOwner(pool, types, book, new SquidOwnerMode());
    const factory = new SquidFactory(pool, types);
    const kernel = new Kernel(32);
    const host = new SquidProcessHost(pool, types, kernel, new RawSquidDestroy(pool, new SquidUnpop(pool, new SquidHash(), new Uint8Array(65536)), types), new SquidProcessState(), { asset: { selected: () => 0, emit: () => {} } });
    try {
        const original = { pop: () => 'pop', playAt: () => 'sound', reserveShared: () => 'reserve', create: () => 99 };
        const hooks = makeTempleRitualHooks(pool, factory, owner, host, original);
        assert.notEqual(hooks, original);
        assert.equal(original.create(), 99);
        assert.deepEqual([hooks.pop(), hooks.playAt(), hooks.reserveShared()], ['pop', 'sound', 'reserve']);
        // 생성은 실제 Factory 로, HP 는 공통 setter 로, 소유자는 Owner 로, 예약은 Host 의 공유 Regular 로 간다.
        const temple = hooks.create(165, 2);
        assert.equal(pool.slot(temple)[10], 165);
        hooks.setHitPoints(temple, 77);
        assert.equal(get(pool.slot(temple), 26, 4), 77);
        hooks.setOwner(temple, 3);
        assert.equal(pool.slot(temple)[34], 3);
        pool.allocated(temple)[11] = 0;
        hooks.addShared(temple, 0x307, 1.5);
        assert.equal(host.findEvent(temple, 0x307).payload, 1.5);
        const other = new SidPool(CAPACITY, { server: true });
        const message = (error) => error instanceof TypeError && /풀 불일치/.test(error.message);
        assert.throws(() => makeTempleRitualHooks(other, factory, owner, host, {}), message);
        assert.throws(() => makeTempleRitualHooks(pool, new SquidFactory(other, types), owner, host, {}), message);
        assert.throws(() => makeTempleRitualHooks(pool, factory, new SquidOwner(other, types, book, new SquidOwnerMode()), host, {}), message);
        assert.throws(() => makeTempleRitualHooks(pool, factory, owner, { pool: other }, {}), message);
    } finally {
        kernel.clear();
    }
});

test('TempleRitualState: 지역 값이 기본이고 장기 상자가 있으면 소비 위치마다 상자의 현재 값을 읽는다', () => {
    const state = new TempleRitualState();
    assert.deepEqual([state.boss, state.priestType, state.runeType, state.pixelScaleX, state.pixelScaleY], [true, 158, 166, 16, 11]);
    assert.deepEqual([state.currentBoss, state.currentPriestType, state.currentRuneType], [true, 158, 166]);
    const boss = { value: false };
    const priest = { value: 71 };
    const rune = { value: 72 };
    state.sharedBoss = boss;
    state.sharedPriestType = priest;
    state.sharedRuneType = rune;
    assert.deepEqual([state.currentBoss, state.currentPriestType, state.currentRuneType], [false, 71, 72]);
    boss.value = true;
    priest.value = 5;
    rune.value = 6;
    assert.deepEqual([state.currentBoss, state.currentPriestType, state.currentRuneType], [true, 5, 6]);
    assert.deepEqual([state.boss, state.priestType, state.runeType], [true, 158, 166]);
    assert.equal(new TempleRitualState({ boss: false, runeType: 9 }).currentRuneType, 9);
});

// 사제 위치와 룬 생성 슬롯 채움 값, 프레임 동작 뒤 변이를 바꿀 수 있게 한 계약 월드이다. (2026-11-13 WB04-E365)
function mutableWorld({ priestAt = [42.75, 43.25], runeFill = 0, runeState = 4, count = null } = {}) {
    const w = contractWorld({ runeState });
    if (priestAt) {
        const raw = w.pool.allocated(6);
        const [x, y] = priestAt;
        // 처음 해시 버킷(42.75, 43.25)에 걸린 사제를 새 위치의 버킷으로 옮긴다.
        w.hash.setBucket(1, 42.75, 43.25, 0);
        put(raw, 14, floatToBits(x));
        put(raw, 18, floatToBits(y));
        put(raw, 4, w.hash.bucket(1, x, y), 2);
        w.hash.setBucket(1, x, y, 6);
    }
    w.countCalls = 0;
    w.hooks.count = (sid) => { w.countCalls += 1; return w.make({}).count(sid); };
    w.hooks.create = (type, flags) => { w.effects.push(`C:${type},${flags}`); const raw = w.pool.allocated(8); raw.fill(runeFill); raw[10] = type; raw[11] = runeState; return 8; };
    w.onFrame = null;
    w.actions = new RawPriestRitualActions(w.pool, w.frames, {
        setFrame: (sid, frame) => { w.effects.push(`F:${sid},${frame}`); put(w.pool.allocated(sid), 36, frame >>> 0, 4); if (w.onFrame) w.onFrame(sid); },
        reserveShared: () => true,
        addShared: (sid, event) => { w.effects.push(`S4:${event},${sid}`); },
    });
    return w;
}

// 사제를 정확히 지정한 점(float)에 둔다. (2026-11-13 WB04-E365)
function placePriest(w, point) {
    const raw = w.pool.allocated(6);
    put(raw, 14, floatToBits(point.x));
    put(raw, 18, floatToBits(point.y));
}

test('TempleRitual 좌표 절삭: 무한·NaN·±2^31 이상·−2^31 미만은 좌표 절삭 범위 오류이고 경계 안쪽(−2^31, 2^31−128)은 허용한다', () => {
    const w = contractWorld();
    const ritual = w.make({ ...w.hooks, count: (sid) => w.make({}).count(sid) });
    const range = (error) => error instanceof RangeError && /좌표 절삭 범위/.test(error.message);
    for (const x of [2147483648, 4294967296, Infinity, -Infinity, NaN, -2147483904]) assert.throws(() => ritual.runeAt(5, { x, y: 1 }), range, `x ${x}`);
    for (const y of [2147483648, -2147483904, Infinity, NaN]) assert.throws(() => ritual.runeAt(5, { x: 1, y }), range, `y ${y}`);
    assert.equal(ritual.runeAt(5, { x: -2147483648, y: 1 }), 0);
    assert.equal(ritual.runeAt(5, { x: 1, y: -2147483648 }), 0);
    assert.equal(ritual.runeAt(5, { x: 2147483520, y: 1 }), 0);
});

test('TempleRitual SHP 한계: 프레임 수·두 배 비트(0x40000/0x400000/0x440000 이지만 비트 0 은 아님)·프레임 배열 길이·로드 여부로 의식 목표점 프레임을 거부한다', () => {
    const w = contractWorld();
    const ritual = w.make();
    const frameCheck = (error) => error instanceof RangeError && /frameCheck/.test(error.message);
    // 신전 SHP: 단계 s 의 프레임은 3 + 오프셋(0, 11, 22, 33, 44) = 3, 14, 25, 36, 47.
    w.shapes[165].frameCount = 47;
    assert.throws(() => ritual.stageAnchor(5, 4), frameCheck);
    assert.ok(Number.isFinite(ritual.stageAnchor(5, 3).x));
    w.shapes[165].frameCount = 48;
    assert.ok(Number.isFinite(ritual.stageAnchor(5, 4).x));
    // 두 배 비트가 있으면 한계가 두 배(세 배가 아님), 비트 0 만 있으면 두 배가 아니다.
    for (const [flags1, count, doubled] of [[0x440000, 20, true], [0x40000, 24, true], [0x400000, 24, true], [1, 24, false], [16, 24, false]]) {
        w.types[165].flags1 = flags1;
        w.shapes[165].frameCount = count;
        if (doubled) {
            assert.ok(Number.isFinite(ritual.stageAnchor(5, 3).x), `두 배 ${flags1}`);
            if (count === 20) assert.throws(() => ritual.stageAnchor(5, 4), frameCheck);
        } else {
            assert.throws(() => ritual.stageAnchor(5, 4), frameCheck, `한 배 ${flags1}`);
        }
    }
    // 두 배 한계(16 → 32)와 세 배(48)는 갈린다: 프레임 36 은 두 배 한계 밖이다.
    w.types[165].flags1 = 0x440000;
    w.shapes[165].frameCount = 16;
    assert.ok(Number.isFinite(ritual.stageAnchor(5, 2).x));
    assert.throws(() => ritual.stageAnchor(5, 3), frameCheck);
    w.types[165].flags1 = 16;
    w.shapes[165].frameCount = 180;
    // 프레임 배열이 frameCount 보다 짧으면 배열 길이로도 거부한다(정의되지 않은 칸을 읽지 않음).
    const longFrames = w.shapes[165].frames;
    w.shapes[165].frames = longFrames.slice(0, 40);
    assert.throws(() => ritual.stageAnchor(5, 4), frameCheck);
    assert.ok(Number.isFinite(ritual.stageAnchor(5, 2).x));
    // 배열 길이와 같은 프레임(47)도 한계 밖이고 한 칸 더 있으면(48) 안이다.
    w.shapes[165].frames = longFrames.slice(0, 47);
    assert.throws(() => ritual.stageAnchor(5, 4), frameCheck);
    w.shapes[165].frames = longFrames.slice(0, 48);
    assert.ok(Number.isFinite(ritual.stageAnchor(5, 4).x));
    w.shapes[165].frames = longFrames;
    w.shapes[165].loaded = false;
    assert.throws(() => ritual.stageAnchor(5, 0), frameCheck);
});

test('TempleRitual 목표점 계산: 포구 쌍(표시 기준점이 아님)을 축별 배율로 나눠 좌표에 더하며 단정도 저장은 한 번만 하고 배율 하나라도 비유한이면 거부한다', () => {
    const w = contractWorld();
    const ritual = w.make();
    // 포구 (33, −22) 와 표시 기준점 (7, 9) 이 서로 다른 프레임: 단계 0 의 신전 프레임 3.
    w.shapes[165].frames[3] = { width: 16, height: 11, hotspotX: 7, hotspotY: 9, muzzleX: 33, muzzleY: -22 };
    const anchor = ritual.stageAnchor(5, 0);
    assert.deepEqual([anchor.x, anchor.y], [Math.fround(33 / 16 + 40.75), Math.fround(-22 / 11 + 41.25)]);
    // 단정도 저장은 합산 뒤 한 번이다: 142/3 + 75.86727905273438 은 먼저 float 로 줄이면 다른 값이 된다.
    w.state.pixelScaleX = 3;
    w.state.pixelScaleY = 3;
    w.shapes[165].frames[3].muzzleX = 142;
    w.shapes[165].frames[3].muzzleY = 142;
    put(w.pool.allocated(5), 14, floatToBits(75.86727905273438));
    put(w.pool.allocated(5), 18, floatToBits(75.86727905273438));
    const tight = ritual.stageAnchor(5, 0);
    assert.deepEqual([tight.x, tight.y], [123.20061492919922, 123.20061492919922]);
    // 배율 한쪽만 비유한/0 이어도 거부한다.
    for (const [x, y] of [[NaN, 11], [Infinity, 11], [16, NaN], [16, Infinity], [16, -Infinity], [-Infinity, 11], [0, 11], [16, 0]]) {
        w.state.pixelScaleX = x;
        w.state.pixelScaleY = y;
        assert.throws(() => ritual.stageAnchor(5, 0), (error) => error instanceof RangeError && /픽셀 배율 오류/.test(error.message), `${x},${y}`);
    }
    // 음수 배율은 허용된다(부호만 반대로 더한다).
    w.state.pixelScaleX = -16;
    w.state.pixelScaleY = -11;
    w.shapes[165].frames[3].muzzleX = 32;
    w.shapes[165].frames[3].muzzleY = 22;
    put(w.pool.allocated(5), 14, floatToBits(40.75));
    put(w.pool.allocated(5), 18, floatToBits(41.25));
    assert.deepEqual([ritual.stageAnchor(5, 0).x, ritual.stageAnchor(5, 0).y], [38.75, 39.25]);
});

test('TempleRitual 단계 4 지면 칸: 칸 번호가 0(−0 포함)이면 허용하고 음수나 배열 길이 이상이면 지면 범위 오류이며 절삭 칸에서 읽는다', () => {
    const w = contractWorld();
    const ritual = w.make();
    const place = (x, y) => { const raw = w.pool.allocated(5); put(raw, 14, floatToBits(x)); put(raw, 18, floatToBits(y)); };
    const bad = (error) => error instanceof RangeError && /마지막 목표 지면 범위/.test(error.message);
    // 단계 4 점 = 신전 좌표 + (anchor 포구 (2, 2) + 룬 포구 (4, 4)).
    place(-6.5, -6.5);
    const origin = ritual.stagePoint(5, 4);
    assert.deepEqual([origin.x, origin.y], [-0.5, -0.5]);
    place(249.5, 249.5);
    assert.deepEqual([ritual.stagePoint(5, 4).x, ritual.stagePoint(5, 4).y], [255.5, 255.5]);
    place(250, 249.5);
    assert.throws(() => ritual.stagePoint(5, 4), bad);
    place(249.5, 250);
    assert.throws(() => ritual.stagePoint(5, 4), bad);
    place(5, -8);
    assert.throws(() => ritual.stagePoint(5, 4), bad);
    // 지도 값은 소수 칸이 아니라 절삭한 칸(행 우선)에서 읽는다: 칸 (10, 20) 의 값만 보정을 일으킨다.
    place(4.25, 14.25);
    const base = ritual.stagePoint(5, 4);
    assert.deepEqual([Math.trunc(base.x), Math.trunc(base.y)], [10, 20]);
    w.spots[20 * 256 + 10] = 2;
    const corrected = ritual.stagePoint(5, 4);
    assert.deepEqual([corrected.x, corrected.y], [Math.fround(-9 / 16 + base.x), Math.fround(-6 / 11 + base.y)]);
    w.spots[20 * 256 + 10] = 0;
    w.spots[10 * 256 + 20] = 2;
    assert.deepEqual([ritual.stagePoint(5, 4).x, ritual.stagePoint(5, 4).y], [base.x, base.y]);
});

test('TempleRitual runeAt: 룬 flags2 의 비트 1·2 가 없을 때만 표면 층을 건너뛰고(비트 4·8·16 등은 무관) 층별 해시에서 첫 룬 TYPE 슬롯을 찾으며 표 밖 룬 TYPE 은 거부한다', () => {
    const w = contractWorld();
    // 룬 슬롯 하나를 표면 층(0)에만 건다.
    const raw = w.pool.allocated(7);
    raw.fill(0);
    put(raw, 0, VTABLE);
    raw[10] = 166;
    put(raw, 14, floatToBits(42.25));
    put(raw, 18, floatToBits(43.5));
    put(raw, 4, w.hash.bucket(0, 42.25, 43.5), 2);
    w.hash.setBucket(0, 42.25, 43.5, 7);
    const ritual = w.make();
    const point = { x: 42.25, y: 43.5 };
    // flags2 & 6 === 0 이면 표면 층을 건너뛰어 찾지 못한다. 비트 1·2 가 하나라도 있으면 표면 층부터 훑어 찾는다.
    for (const [flags2, found] of [[0, 0], [1, 0], [8, 0], [16, 0], [0x200000, 0], [2, 7], [4, 7], [6, 7], [3, 7], [7, 7], [0x200006, 7]]) {
        w.types[166].flags2 = flags2;
        assert.equal(ritual.runeAt(5, point), found, `flags2 ${flags2}`);
    }
    // 현재 룬 TYPE 번호가 표 길이 이상이면 SHP 조회가 아니라 룬 타입 범위 오류이다.
    for (const runeType of [188, 255]) {
        w.state.runeType = runeType;
        assert.throws(() => ritual.runeAt(5, point), (error) => error instanceof RangeError && /현재Rune 타입 범위/.test(error.message), `룬 TYPE ${runeType}`);
    }
});

test('TempleRitual 사제 조회 경계: 발자국을 위·아래·왼쪽·오른쪽으로 정확히 한 칸씩 넓히고(신전 칸 (38.75,39.25)~(40.75,41.25)) 홀수 임계 HP 는 정수 절삭이다', () => {
    // 칸 범위는 x 37..42, y 38..43 이다: 안쪽 끝 칸은 찾고 바깥 한 칸은 찾지 못한다.
    const cases = [
        [[37.5, 40.5], 6], [[36.5, 40.5], 0],
        [[40.5, 38.5], 6], [[40.5, 37.5], 0],
        [[42.5, 40.5], 6], [[43.5, 40.5], 0],
        [[40.5, 43.5], 6], [[40.5, 44.5], 0],
    ];
    for (const [priestAt, expected] of cases) {
        const w = mutableWorld({ priestAt });
        assert.equal(w.make().findStrongOwnedPriest(5), expected, `사제 ${priestAt}`);
    }
    // 임계 HP: 최대 202 → 101: 100 은 탈락, 101·102 는 통과(| 1 같은 짝수화로 갈리지 않는다).
    const w = mutableWorld();
    w.types[158].maxHitPoints = 202;
    w.reward = new SquidReward(w.pool, w.types, new SquidPostPopState(), new SquidRewardState(), new ScrambledSpStore(new GameRandom(0), () => 1));
    const ritual = w.make();
    for (const [hp, expected] of [[100, 0], [101, 6], [102, 6], [98, 0]]) {
        setRawSquidHitPoints(w.pool, 6, hp);
        assert.equal(ritual.findStrongOwnedPriest(5), expected, `HP ${hp}`);
    }
    // 최대 HP 가 음수이면 정수 나눗셈은 0 쪽으로 절삭한다: −3 ÷ 2 = −1 이므로 HP −2 는 탈락, −1 은 통과한다.
    w.types[158].maxHitPoints = -3;
    w.reward = new SquidReward(w.pool, w.types, new SquidPostPopState(), new SquidRewardState(), new ScrambledSpStore(new GameRandom(0), () => 1));
    const negative = w.make();
    setRawSquidHitPoints(w.pool, 6, -2);
    assert.equal(negative.findStrongOwnedPriest(5), 0);
    setRawSquidHitPoints(w.pool, 6, -1);
    assert.equal(negative.findStrongOwnedPriest(5), 6);
    w.types[158].maxHitPoints = 202;
    w.reward = new SquidReward(w.pool, w.types, new SquidPostPopState(), new SquidRewardState(), new ScrambledSpStore(new GameRandom(0), () => 1));
    // 임계가 후보의 최대 HP 에서 나온다: 후보 사제 TYPE 의 최대 HP 가 신전 TYPE 의 최대 HP 와 다르면 후보 쪽을 쓴다.
    w.types[165].maxHitPoints = 100000;
    const other = new SquidReward(w.pool, w.types, new SquidPostPopState(), new SquidRewardState(), new ScrambledSpStore(new GameRandom(0), () => 1));
    setRawSquidHitPoints(w.pool, 6, 101);
    w.reward = other;
    assert.equal(w.make().findStrongOwnedPriest(5), 6);
});

test('TempleRitual 추상 룬 조회: 발자국 우하단은 거의 올림(0.99999)이며 정수 경계 값에서는 올리지 않고 아주 작은 소수부부터 다음 칸을 포함한다', () => {
    // 신전 위치가 곧 발자국의 오른쪽/아래이다. x 가 정수면 칸 40 까지, 40.001 이면 칸 41 까지 본다.
    for (const [templeX, runeX, expected] of [[40, 41.5, false], [40, 40.5, true], [40.001, 41.5, true], [40.001, 42.5, false]]) {
        const w = contractWorld();
        put(w.pool.allocated(5), 14, floatToBits(templeX));
        w.pool.allocate(2);
        w.add(9, 166, runeX, 40.5, 1, 0);
        assert.equal(w.make().hasAbstractRune(5), expected, `x ${templeX}/${runeX}`);
    }
    for (const [templeY, runeY, expected] of [[41, 42.5, false], [41, 41.5, true], [41.001, 42.5, true], [41.001, 43.5, false]]) {
        const w = contractWorld();
        put(w.pool.allocated(5), 18, floatToBits(templeY));
        w.pool.allocate(2);
        w.add(9, 166, 39.5, runeY, 1, 0);
        assert.equal(w.make().hasAbstractRune(5), expected, `y ${templeY}/${runeY}`);
    }
});

test('TempleRitual ritual: 목표점은 두 축이 모두 같아야 하며(한 축만 같으면 다음 단계 점을 본다) 단계별 음향 이름·지연 float·가상 count 조회 횟수를 지킨다', () => {
    const sounds = ['forWind2.wav', 'forSun2.wav', 'forRain2.wav', 'forThunder2.wav', 'forStorm2.wav'];
    const delays = [1.9600000381469727, 2.25, 1.8799999952316284, 2.2200000286102295, 2.4100000858306885];
    for (let stage = 0; stage <= 4; stage += 1) {
        const w = mutableWorld();
        put(w.pool.allocated(5), 36, 3 + 11 * stage, 4);
        const ritual = w.make();
        const point = ritual.stagePoint(5, stage);
        placePriest(w, point);
        w.countCalls = 0;
        ritual.ritual(5, 6, 0);
        // 현재 단계 점이 맞으면 mode 0 은 사제 동작(F) → 위치 음향(A) → 공유 예약(S)이며 count 조회는 단계 계산 한 번과 룬 조회 한 번이다.
        assert.equal(w.countCalls, 2);
        assert.equal(w.effects.length, 3, `단계 ${stage}`);
        assert.match(w.effects[0], /^F:6,/);
        assert.equal(w.effects[1], `A:${pointBits(point)},${sounds[stage]},0`);
        assert.equal(w.effects[2], `S:775,5,${delays[stage]}`);
        // 한 축만 어긋나면(다음 단계 점도 맞지 않으므로) 아무것도 하지 않는다.
        for (const [dx, dy] of [[1, 0], [0, 1], [0.5, 0], [0, 0.5]]) {
            const m = mutableWorld();
            put(m.pool.allocated(5), 36, 3 + 11 * stage, 4);
            placePriest(m, { x: point.x + dx, y: point.y + dy });
            m.make().ritual(5, 6, 0);
            assert.deepEqual(m.effects, [], `단계 ${stage} 어긋남 ${dx},${dy}`);
        }
    }
    // 현재 단계 점이 아니라 다음 단계 점에 있어도 같은 단계의 동작을 한다(한 축만 같으면 거부).
    const next = mutableWorld();
    const nextRitual = next.make();
    const nextPoint = nextRitual.stagePoint(5, 1);
    placePriest(next, nextPoint);
    nextRitual.ritual(5, 6, 0);
    assert.equal(next.effects[1], `A:${pointBits(nextPoint)},forWind2.wav,0`);
    const half = mutableWorld();
    placePriest(half, { x: nextPoint.x, y: nextPoint.y + 1 });
    half.make().ritual(5, 6, 0);
    assert.deepEqual(half.effects, []);
    const half2 = mutableWorld();
    placePriest(half2, { x: nextPoint.x + 1, y: nextPoint.y });
    half2.make().ritual(5, 6, 0);
    assert.deepEqual(half2.effects, []);
    // 마지막 단계(4)에서는 다음 단계 점을 보지 않는다.
    const last = mutableWorld();
    put(last.pool.allocated(5), 36, 47, 4);
    const lastRitual = last.make();
    placePriest(last, lastRitual.stagePoint(5, 4));
    lastRitual.ritual(5, 6, 0);
    assert.equal(last.effects.length, 3);
    // 같은 위치에 이미 룬이 있으면 아무것도 하지 않는다.
    const runed = mutableWorld();
    const runedRitual = runed.make();
    const runedPoint = runedRitual.stagePoint(5, 0);
    placePriest(runed, runedPoint);
    runed.add(7, 166, runedPoint.x, runedPoint.y, 0, 0);
    runedRitual.ritual(5, 6, 0);
    assert.deepEqual(runed.effects, []);
});

test('TempleRitual ritual 사제 동작 뒤 재조회: 프레임 효과가 사제 위치를 옮기면 위치 음향은 옮겨진 현재 좌표를 쓴다', () => {
    const w = mutableWorld();
    const ritual = w.make();
    const point = ritual.stagePoint(5, 0);
    placePriest(w, point);
    w.onFrame = () => placePriest(w, { x: point.x + 1.5, y: point.y + 2.5 });
    ritual.ritual(5, 6, 0);
    assert.equal(w.effects[1], `A:${pointBits({ x: point.x + 1.5, y: point.y + 2.5 })},forWind2.wav,0`);
});

// 신전 발자국 한 칸 이웃 안(39.5, 40.5)에 강한 사제(슬롯 9, 같은 소유자, HP 100)를 더한다. 의식 대상 사제(슬롯 6)는 목표점에 있어 이웃 밖이다. (2026-11-13 WB04-E365)
function addStrongPriest(w, hp = 100) {
    assert.equal(w.pool.allocate(2), 9);
    w.add(9, 158, 39.5, 40.5);
    w.pool.allocated(9)[34] = 1;
    setRawSquidHitPoints(w.pool, 9, hp);
}

test('TempleRitual ritual 후속 모드: 강한 사제의 후속·권한(상자 포함)·추상 룬 유무에 따른 룬 생성과 가상 count 조회 횟수를 지킨다', () => {
    // 후속 모드는 사제 후속(F 와 604 예약)을 먼저 하고, 권한이 있고 추상 룬이 없으면 룬을 만든다(count 조회: 단계·룬 조회·후속 단계 + 생성 두 번).
    const spawn = mutableWorld();
    addStrongPriest(spawn);
    spawn.state.boss = true;
    const ritual = spawn.make();
    placePriest(spawn, ritual.stagePoint(5, 0));
    spawn.countCalls = 0;
    ritual.ritual(5, 6, 1);
    assert.deepEqual(spawn.effects.map((effect) => effect.split(':')[0]), ['F', 'S4', 'C', 'H', 'O', 'P']);
    assert.equal(spawn.countCalls, 5);
    // 권한이 없으면 생성하지 않고 count 조회는 세 번이다. 지역 권한이 거짓이어도 상자가 참이면 생성한다.
    const idle = mutableWorld();
    idle.state.boss = false;
    addStrongPriest(idle);
    const idleRitual = idle.make();
    placePriest(idle, idleRitual.stagePoint(5, 0));
    idle.countCalls = 0;
    idleRitual.ritual(5, 6, 1);
    assert.deepEqual(idle.effects.map((effect) => effect.split(':')[0]), ['F', 'S4']);
    assert.equal(idle.countCalls, 3);
    const boxed = mutableWorld();
    boxed.state.boss = false;
    boxed.state.sharedBoss = { value: true };
    addStrongPriest(boxed);
    const boxedRitual = boxed.make();
    placePriest(boxed, boxedRitual.stagePoint(5, 0));
    boxedRitual.ritual(5, 6, 1);
    assert.equal(boxed.effects.some((effect) => effect.startsWith('C:')), true);
    // 신전 발자국 안에 추상 룬이 이미 있으면 생성하지 않는다(권한이 있어도).
    const taken = mutableWorld();
    addStrongPriest(taken);
    const takenRitual = taken.make();
    placePriest(taken, takenRitual.stagePoint(5, 0));
    assert.equal(taken.pool.allocate(2), 10);
    taken.add(10, 166, 39.5, 40.5, 1, 0);
    takenRitual.ritual(5, 6, 1);
    assert.equal(taken.effects.some((effect) => effect.startsWith('C:')), false);
    // 강한 사제가 없으면(HP 부족) 사제 후속 없이 룬 생성만 이어진다.
    const weak = mutableWorld();
    addStrongPriest(weak, 1);
    const weakRitual = weak.make();
    placePriest(weak, weakRitual.stagePoint(5, 0));
    weakRitual.ritual(5, 6, 1);
    assert.deepEqual(weak.effects.map((effect) => effect.split(':')[0]), ['C', 'H', 'O', 'P']);
});

test('TempleRitual spawnRune 세부: 생성 슬롯의 추상 비트를 새로 세우고 void 아닌 슬롯은 비트 4 만 보며, 각 효과 뒤 현재 단계를 다시 읽는다', () => {
    // 생성 슬롯을 0 으로 채워 추상 비트(extra bit 0)가 실제로 세워지는지 본다.
    const clean = mutableWorld({ runeFill: 0 });
    put(clean.pool.allocated(5), 36, 25, 4);
    clean.make().spawnRune(5);
    assert.equal(clean.pool.slot(8)[40], 1);
    // void 비트(4)만이 진단을 막는다: 비트 1 만 있는 슬롯은 진단을 부르고 비트 5 는 부르지 않는다.
    for (const [rawState, called] of [[1, true], [0, true], [4, false], [5, false], [2, true], [8, true], [12, false]]) {
        const w = mutableWorld({ runeState: rawState });
        put(w.pool.allocated(5), 36, 25, 4);
        let calls = 0;
        w.make({ ...w.hooks, setHitPoints: () => {}, setOwner: () => {}, assertion: () => { calls += 1; } }).spawnRune(5);
        assert.equal(calls > 0, called, `상태 ${rawState}`);
    }
    // 소유자 효과가 신전 프레임을 바꾸면 Pop 좌표는 바뀐 현재 단계의 Anchor 이고 룬 프레임에 쓴 단계는 바꾸기 전 값이다.
    const moved = mutableWorld();
    put(moved.pool.allocated(5), 36, 25, 4);
    const ritual = moved.make({ ...moved.hooks, setOwner: (sid, player) => { moved.effects.push(`O:${sid},${player}`); put(moved.pool.allocated(5), 36, 40, 4); } });
    ritual.spawnRune(5);
    const expected = moved.make().stageAnchor(5, 3);
    assert.equal(moved.effects.at(-1), `P:8,${pointBits(expected)},0`);
    assert.equal(get(moved.pool.slot(8), 36, 4), 2);
    // 룬 프레임은 DWORD 하나만 쓰고(옆 바이트 보존) 소유자는 신전의 현재 소유자(효과 뒤)를 쓴다.
    const stale = mutableWorld({ runeFill: 0xa5 });
    put(stale.pool.allocated(5), 36, 25, 4);
    stale.pool.allocated(5)[34] = 7;
    stale.make().spawnRune(5);
    const rune = stale.pool.slot(8);
    assert.deepEqual([rune[40], rune[35], rune[41], get(rune, 36, 4)], [0xa5, 0xa5, 0xa5, 2]);
    assert.ok(stale.effects.includes('O:8,7'));
});

test('TempleRitual 생성 슬롯 채움이 없는 stage: 명시 frame 은 현재 프레임을 읽지 않고 음수도 그대로 쓰며, 실제 Dais 직접 count 는 바로 뒤 바이트에 영향받지 않는다', () => {
    const w = mutableWorld();
    put(w.pool.allocated(5), 36, 25, 4);
    const ritual = w.make();
    assert.equal(ritual.stage(5, -2), 0);
    assert.equal(ritual.stage(5, -1), 2);
    assert.equal(ritual.stage(5, 25), 2);
    // 실제 Dais 가상 표: 바로 뒤 4~5 바이트(해시 사슬)가 비영이어도 직접 count 를 쓴다.
    const bare = w.make({ ...w.hooks, count: undefined });
    put(w.pool.allocated(5), 0, 0x507950);
    w.pool.allocated(5)[4] = 0x7f;
    w.pool.allocated(5)[5] = 0x01;
    assert.equal(bare.stage(5), 2);
});

test('TempleRitual makeTempleRitualHooks 생성 영역: 지정한 flags 로 실제 Factory 를 부른다(2 는 클라이언트 영역, 0 은 서버 영역)', () => {
    const pool = new SidPool(CAPACITY, { server: true });
    const types = formTypeInputs();
    types[166].constructorAddress = typeConstructorAddress(166);
    const book = new SquidPostPopState();
    const owner = new SquidOwner(pool, types, book, new SquidOwnerMode());
    const factory = new SquidFactory(pool, types);
    const kernel = new Kernel(32);
    const host = new SquidProcessHost(pool, types, kernel, new RawSquidDestroy(pool, new SquidUnpop(pool, new SquidHash(), new Uint8Array(65536)), types), new SquidProcessState(), { asset: { selected: () => 0, emit: () => {} } });
    try {
        const hooks = makeTempleRitualHooks(pool, factory, owner, host, {});
        assert.ok(hooks.create(166, 2) < pool.layout.serverFirst);
        assert.ok(hooks.create(166, 0) >= pool.layout.serverFirst);
        const sid = hooks.create(166, 2);
        pool.allocated(sid)[11] = 0;
        hooks.setOwner(sid, 5);
        assert.equal(pool.slot(sid)[34], 5);
        hooks.addShared(sid, 0x307, 2.5);
        assert.equal(host.findEvent(sid, 0x307).payload, 2.5);
        assert.equal(Object.keys(hooks).sort().join(), 'addShared,create,setHitPoints,setOwner');
    } finally {
        kernel.clear();
    }
});
