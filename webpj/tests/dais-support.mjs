// 다이스·제단 시험이 함께 쓰는 합성 장면 보조 모음이다(cpppj DaisRegularTests/DaisProgressTests/AltarSacrificeTests 의 장면을 같은 순서로 옮김).
// makeWorld 는 같은 풀에 TYPE/프레임/해시/Host/협력 부품과 다이스 몸체를 만들고, progressScene 은 진행 관측 한 행의 입력을 채운다. 실제 게임·브라우저 실행이 아니다.
// (2026-11-13 WB04-E367: dais-regular 시험에서 분리, 같은 날 WB04-E368 제단 시험이 함께 사용)
import assert from 'node:assert/strict';
import { put, get, floatToBits } from './raw-support.mjs';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { SidPool } from '../www/core/sid-pool.js';
import { Kernel } from '../www/core/kernel.js';
import { GameRandom } from '../www/core/random.js';
import { ScrambledSpStore } from '../www/core/sp-store.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { SquidFrame } from '../www/core/squid-frame.js';
import { RawSquidDestroy } from '../www/core/squid-destroy.js';
import { SquidPostPop } from '../www/core/squid-postpop.js';
import { SquidPostPopState } from '../www/core/squid-postpop-state.js';
import { SquidReward, SquidRewardState } from '../www/core/squid-reward.js';
import { RegularProcess, SquidProcessHost, SquidProcessState } from '../www/core/squid-process.js';
import { RawSquidNeighbors } from '../www/core/raw-squid-neighbors.js';
import { RawPriestRitualActions } from '../www/core/raw-priest-ritual-actions.js';
import { setRawSquidHitPoints } from '../www/core/raw-squid-word.js';
import { RawTempleRitual, TempleRitualState } from '../www/core/raw-temple-ritual.js';
import { RawRunePostPop, RunePostPopState, RUNE_VTABLE } from '../www/core/raw-rune-post-pop.js';
import { RawCarrierPreDestroy } from '../www/core/raw-carrier-pre-destroy.js';
import { ContainedFinderState } from '../www/core/raw-contained-finder.js';
import { RawContainedLookup } from '../www/core/raw-contained-lookup.js';
import { RawPathStart } from '../www/core/raw-path-start.js';
import { PATCH_DAIS_VTABLE } from '../www/core/raw-dais-content-notice.js';
import { DaisRegularState, RawDaisRegular } from '../www/core/raw-dais-regular.js';

// 합성 가상 표 주입값(이미지 비교는 앞 DWORD 를 제외한다), 풀 크기, 희생 코드 타입이다. (2026-11-13 WB04-E367)
export const VTABLE = 0x11010000;
export const CAPACITY = 24000;
export const ALTAR_VTABLE = 0x005076b8;

// 바이트 배열의 소문자 16진 문자열이다. (2026-11-13 WB04-E367)
export const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 효과 순서를 원본 구분자로 묶는다(빈 결과도 원본 표식 '-' 를 유지한다). (2026-11-13 WB04-E367)
export const joinEffects = (effects) => (effects.length ? effects.join(';') : '-');

// 선택하지 않은 효과를 빈 함수로 바꾸지 않고 실행 시 실패하게 만드는 훅이다. (2026-11-13 WB04-E367)
export const forbid = (name) => () => { throw new Error(`미선택 하위 효과: ${name}`); };

// 188칸 TYPE 입력이다(cpppj MakeFormTypeInputs). (2026-11-13 WB04-E367)
export function formTypeInputs() {
    return Array.from({ length: 188 }, (_, number) => ({
        cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0, group: 0,
        ...([1, 2, 3, 6].includes(number) ? { constructorAddress: typeConstructorAddress(number) } : {}),
    }));
}

// 방향 코드와 SHP 핫스폿을 합성 입력으로 채운다(cpppj DaisAssets/ProgressAssets). patterns 는 [타입, 코드 문자열] 배열이다. (2026-11-13 WB04-E367)
export function applyAssets(types, frames, shapes, patterns) {
    // 출력 구간/프레임은 계산하지 않고 입력 코드만 넣는다.
    for (const [type, pattern] of patterns) {
        types[type].footX = types[type].footY = type === 165 ? 3 : 1;
        const codes = [];
        shapes[type] = { loaded: true, frameCount: pattern.length, frames: [] };
        // 원본 입력의 번호/변형과 signed short 핫스폿을 준비한다.
        for (let index = 0; index < pattern.length; index += 1) {
            codes.push({ side: pattern.charCodeAt(index), variant: 0x50, number: index + 1, flags: 0 });
            shapes[type].frames.push({ width: 16, height: 11, hotspotX: (index % 5) * 16, hotspotY: (index % 5) * 11, muzzleX: (index % 5) * 16, muzzleY: (index % 5) * 11 });
        }
        frames[type] = new FrameCodeTable(codes);
    }
    // Temple helper 의 세 자산 HP 입력이다.
    for (const type of [158, 165, 166]) Object.assign(types[type], { maxHitPoints: 200, flags1: 16 });
}

// 코어 관측의 다섯 자산 코드(158 사제·164 제단·165 다이스·166 룬·169 희생 코드표)이다. (2026-11-13 WB04-E367)
export const corePatterns = (side) => [
    [158, `AEKLMNOP${'P'.repeat(24)}`], [164, `AEMLP${'P'.repeat(11)}`], [165, side.repeat(180)], [166, 'A'.repeat(6)],
    [169, `${'A'.repeat(3)}${'B'.repeat(42)}${'C'.repeat(3)}${'D'.repeat(132)}`],
];

// 한 장면의 모든 협력 부품을 같은 풀에 만든다. 미선택 하위 효과는 모두 실패하는 훅이며 overrides 로 필요한 것만 교체한다.
//  - options: { patterns, frameHooks, templeCount(true 면 기존 count 연결) }, overrides: { dais: 훅 덮어쓰기 }.
// 슬롯 5..11 은 client free list 에서 순서대로 확보해 0 으로 채운다. (2026-11-13 WB04-E367)
export function makeWorld(options = {}, daisOverrides = {}) {
    const pool = new SidPool(CAPACITY, { server: true });
    const hash = new SquidHash();
    const spots = new Uint8Array(65536);
    const types = formTypeInputs();
    const frames = types.map(() => new FrameCodeTable([]));
    const shapes = types.map(() => ({ loaded: false, frameCount: 0, frames: [] }));
    applyAssets(types, frames, shapes, options.patterns ?? corePatterns('A'));
    const slots = [];
    // 비교 슬롯 외의 unused 슬롯도 원본의 zero 입력으로 둔다.
    for (let sid = 5; sid <= 11; sid += 1) {
        assert.equal(pool.allocate(2), sid);
        slots[sid] = pool.allocated(sid);
        slots[sid].fill(0);
    }
    const book = new SquidPostPopState();
    book.localOwner = 1;
    const base = new SquidPostPop(pool, types, book);
    const destroy = new RawSquidDestroy(pool, new SquidUnpop(pool, hash, spots), types);
    const kernel = new Kernel(32);
    const clock = new SquidProcessState();
    const host = new SquidProcessHost(pool, types, kernel, destroy, clock);
    const frame = new SquidFrame(pool, types, options.frameHooks ?? {
        frameSize: forbid('프레임 크기'), update: forbid('프레임 표시'), unpop: forbid('Frame Unpop'), pop: forbid('Frame Pop'),
    });
    const containedState = new ContainedFinderState();
    const carrier = new RawCarrierPreDestroy(pool, containedState, { validateDamageable: forbid('Carrier 검사'), damageablePre: forbid('Carrier 삭제'), dependentFound: forbid('Carrier 후처리') });
    const lookup = new RawContainedLookup(pool, forbid('경로 종속 삭제'));
    const pathState = { clock, dudeType: 70, teleportType: 134, manaBoltType: 105, priestType: 158, geyserType: 122, snapPriestDestination: false, retrying: false };
    const path = new RawPathStart(pool, hash, types, host, lookup, frame, pathState, {
        processType: forbid('Path mapper'), create: forbid('Path 생성'), metadata: forbid('Path 메타'), clearBlocked: forbid('경로 막힘 정리'),
    });
    const footprint = new RawSquidNeighbors(pool, hash, spots, types, frames);
    const rewardState = new SquidRewardState();
    const reward = new SquidReward(pool, types, book, rewardState, new ScrambledSpStore(new GameRandom(0), () => 1));
    const actions = new RawPriestRitualActions(pool, frames, { setFrame: forbid('사제 의식 Begin'), reserveShared: forbid('사제 604 예약'), addShared: forbid('사제 604 부착') });
    const templeState = new TempleRitualState();
    let fixtureCount = null;
    const ritual = new RawTempleRitual(pool, hash, types, frames, shapes, spots, footprint, reward, actions, templeState, {
        create: forbid('Rune 생성'), setHitPoints: forbid('Rune HP'), setOwner: forbid('Rune owner'), pop: forbid('Rune Pop'), playAt: forbid('의식 위치 음향'),
        reserveShared: forbid('Temple 307 예약'), addShared: forbid('Temple 307 부착'),
        count: (sid) => { if (!fixtureCount) throw new Error('기존 fixture의 실제 Count 장치 설치 전 호출'); return fixtureCount.count(sid); },
    });
    fixtureCount = ritual;
    const runeState = new RunePostPopState();
    const rune = new RawRunePostPop(pool, hash, types, runeState, base, { reserveRegular: forbid('Rune304 확보'), addRegular: forbid('Rune304 부착'), startConstruction: forbid('Rune 건설') });
    const state = new DaisRegularState(templeState, runeState, clock, book.localOwner);
    // 모든 상위 효과는 기본적으로 실패한다. 선택한 효과만 덮어쓴다.
    const hooks = {
        setFrame: forbid('setFrame'), destroy: forbid('삭제'), reserveShared: forbid('reserveShared'), addShared: forbid('addShared'), playAt: forbid('위치 음향'),
        reserveSound: forbid('희생 소리 확보'), loadSound: forbid('희생 소리 조회'), attachSound: forbid('희생 소리 부착'), wallSeconds: forbid('벽시계'),
        setSoundStart: forbid('지연 시각'), stopVoice: forbid('희생 종료'), processType: forbid('Dais Path 조회'), pathMetadata: forbid('Dais Path 메타'),
        damageable: forbid('나머지 Damageable'), addLocalSp: forbid('SP 획득'), normalizeForeignSp: forbid('다른 SP 변환'), grantForeignType: forbid('다른 타입 획득'),
        localSpecial: forbid('로컬 특수 획득'), localType: forbid('로컬 타입 획득'), refreshOwner: forbid('owner 갱신'), refreshLocal: forbid('메뉴 갱신'),
        priestExists: forbid('player 사제 조회'), multiplayerStatus: forbid('멀티 상태 조회'), chat: forbid('희생 채팅'),
        ...daisOverrides,
    };
    const dais = new RawDaisRegular(pool, hash, types, frames, ritual, rune, carrier, host, lookup, path, state, hooks);
    // 슬롯 하나를 해시 첫 단계 버킷에 건다(좌표 40.75/41.25 float 비트).
    const insert = (sid, type) => {
        const raw = slots[sid];
        put(raw, 0, VTABLE);
        raw[10] = type;
        put(raw, 14, floatToBits(40.75));
        put(raw, 18, floatToBits(41.25));
        put(raw, 4, hash.bucket(1, 40.75, 41.25), 2);
        hash.setBucket(1, 40.75, 41.25, sid);
    };
    return { pool, hash, spots, types, frames, shapes, slots, book, base, destroy, kernel, clock, host, frame, containedState, carrier, lookup, path, footprint, rewardState, reward,
        actions, templeState, ritual, runeState, rune, state, hooks, dais, insert };
}

// 제단(164) 코드 표를 진행 관측의 구간표(AAA OOOO MMM LLL PPP)로 교체한다. 입력 코드만 바꾸고 실제 구간표는 FrameCodeTable 이 계산한다. (2026-11-13 WB04-E367)
export function replaceAltarFrames(world) {
    const pattern = `${'A'.repeat(3)}${'O'.repeat(4)}${'M'.repeat(3)}${'L'.repeat(3)}${'P'.repeat(3)}`;
    world.frames[164] = new FrameCodeTable([...pattern].map((letter, index) => ({ side: letter.charCodeAt(0), variant: 0x50, number: index + 1, flags: 0 })));
}

// 지연 소리 시작 시각 double 을 원본 little endian 이미지로 만든다. (2026-11-13 WB04-E367)
export function doubleImage(value) {
    const buffer = Buffer.alloc(8);
    buffer.writeDoubleLE(value, 0);
    return buffer.toString('hex');
}

// 채팅 요청 종류를 원본 문자열 key 로 바꾼다(10.78 인자 구성). (2026-11-13 WB04-E367)
export const CHAT_KEYS = { Upgrade: 'AltarUpgrade', Stormpower: 'SacrificeForStormpower', LocalPriestDead: 'MultiplayerPriestDead(%s,%s,%d)', OtherPriestDead: 'MultiplayerOtherPriestDead(%s,%s,%s,%s)' };

// 진행 관측 한 행의 장면을 만들어 입력대로 채우고 효과 기록 훅을 돌려준다. (2026-11-13 WB04-E367)
// options.altarFrames: 제단 관측처럼 실제 Frame 의 표시 갱신(짝수 번째)을 효과 'F:' 로 기록한다. options.altarAssets165: 다이스(165) 코드표를 제단 관측의 K·L·M·N·O·P 8칸씩 + A 132칸으로 바꾼다.
export function progressScene(input, overrides = {}, options = {}) {
    const effects = [];
    let soundAttempt = 0;
    const soundStarts = [0, 0];
    let world;
    const hooks = {
        setFrame: (sid, value, flags) => {
            assert.equal(flags, 0);
            effects.push(`F:${sid},${value >>> 0},0`);
            put(world.pool.allocated(sid), 36, value >>> 0, 4);
        },
        reserveShared: () => { effects.push('N:40'); return input.reserve !== 0; },
        addShared: (sid, event, payload) => { effects.push(`S:${event},${sid},${floatToBits(payload)}`); },
        playAt: (x, y, name, flags) => { effects.push(`A:${floatToBits(x)},${floatToBits(y)},${name},${flags}`); },
        destroy: (sid, flags) => { effects.push(`destroy:${sid},${flags}`); },
        reserveSound: () => { const index = soundAttempt; soundAttempt += 1; effects.push(`NS:${index},40`); return (input.soundMask & (1 << index)) !== 0; },
        loadSound: (name, zero, flags) => { assert.deepEqual([zero, flags], [0, 12]); effects.push(`LOAD:${name},0,12`); return 0x9000 + (name === 'thunderCrack.wav' ? 1 : 0); },
        attachSound: (sid, sound, zero, flags) => {
            const index = sound - 0x9000;
            assert.ok(index < 2 && sid === 5 && zero === 0 && flags === 12);
            effects.push(`ATTACH:${index},${sid},${sound},0,12`);
            return (input.attachMask & (1 << index)) ? 0x11017000 + index * 0x100 : 0;
        },
        wallSeconds: () => { effects.push('WALL:123.25'); return 123.25; },
        setSoundStart: (process, start) => { const index = (process - 0x11017000) / 0x100; assert.ok(index < 2); soundStarts[index] = start; },
        stopVoice: (cue, a, b) => { effects.push(`V:${cue},${a},${b}`); },
        addLocalSp: (delta) => { effects.push(`SP:${floatToBits(delta)}`); },
        grantForeignType: (owner, type) => { effects.push(`H:${owner},${type}`); },
        localSpecial: (value) => { effects.push(`L:${value}`); },
        localType: (type) => { effects.push(`T:${type}`); },
        refreshOwner: (owner) => { effects.push(`R:${owner}`); },
        refreshLocal: () => { effects.push('UI:0x44fc00', 'UI:0x4beb30', 'UI:0x4bfac0'); },
        priestExists: () => true,
        multiplayerStatus: () => 3,
        chat: (message) => { effects.push(`CHAT:${CHAT_KEYS[message.kind]}`); },
        ...overrides,
    };
    let updates = 0;
    const frameHooks = options.altarFrames
        ? {
            frameSize: () => [1, 1],
            update: (sid, flags) => {
                assert.equal(flags, 0);
                updates += 1;
                if (updates % 2 === 0) effects.push(`F:${sid},${get(world.pool.slot(sid), 36, 4)},0`);
            },
            unpop: forbid('Altar fixture unexpected Frame Unpop'),
            pop: forbid('Altar fixture unexpected Frame Pop'),
        }
        : { frameSize: () => [1, 1], update: forbid('프레임 표시'), unpop: forbid('Frame Unpop'), pop: forbid('Frame Pop') };
    world = makeWorld({ patterns: corePatterns('A'), frameHooks }, hooks);
    replaceAltarFrames(world);
    if (options.altarAssets165) {
        const pattern = `${'K'.repeat(8)}${'L'.repeat(8)}${'M'.repeat(8)}${'N'.repeat(8)}${'O'.repeat(8)}${'P'.repeat(8)}${'A'.repeat(132)}`;
        world.frames[165] = new FrameCodeTable([...pattern].map((letter, index) => ({ side: letter.charCodeAt(0), variant: 0x50, number: index + 1, flags: 0 })));
    }
    const { pool, slots, insert, state } = world;
    insert(5, 165);
    insert(6, 158);
    insert(7, 164);
    put(slots[5], 0, PATCH_DAIS_VTABLE);
    put(slots[5], 36, input.daisFrame >>> 0, 4);
    put(slots[6], 36, input.priestFrame >>> 0, 4);
    put(slots[7], 36, input.altarFrame >>> 0, 4);
    slots[5][34] = 1;
    slots[6][34] = 1;
    slots[7][34] = 1;
    setRawSquidHitPoints(pool, 6, 100);
    world.types[158].flags2 = 0x210000;
    put(slots[5], 6, input.captured ? 10 : 0, 2);
    put(slots[10], 0, VTABLE);
    slots[10][10] = 6;
    put(slots[10], 12, input.oldOwner, 2);
    put(slots[10], 18, input.captured, 2);
    put(slots[10], 20, 36, 2);
    state.alliances[input.oldOwner + input.owner * 9] = input.alliance;
    // 원본과 같은 현재 해시 단계/owner/제단 WORD/초기 시각 입력이다.
    for (const sid of [5, 6, 7]) slots[sid][33] = 1;
    slots[5][34] = input.owner;
    slots[5][40] = input.extra;
    put(slots[7], 0, ALTAR_VTABLE);
    put(slots[7], 12, input.oldOwner, 2);
    state.localOwner = input.local;
    state.grantFlags.fill(1);
    state.voiceCue = 99;
    world.runeState.suppressDuration = input.suppress !== 0;
    world.templeState.boss = input.boss !== 0;
    // 룬 304 는 실제 룬 생성자의 가상표와 같은 좌표/owner 입력을 쓰며 해시에 새 후보를 추가하지 않는다.
    put(slots[9], 0, RUNE_VTABLE);
    slots[9][10] = 166;
    put(slots[9], 14, floatToBits(40.75));
    put(slots[9], 18, floatToBits(41.25));
    slots[9][34] = input.owned ? input.owner : 3;
    // 명시 Host 로 실제 24 Regular 를 붙이고 두 개일 때 Native 입력 순서의 체인을 만든다.
    for (let index = 0; index < input.forms; index += 1) {
        const attached = world.host.attach(new RegularProcess(24, 0.12), 61, 7, 0x10);
        assert.ok(attached);
        assert.equal(attached.form, 12 + index);
    }
    if (input.forms === 2) {
        put(pool.allocated(7), 6, 12, 2);
        put(pool.allocated(12), 4, 13, 2);
        put(pool.allocated(12), 14, 0, 2);
        put(pool.allocated(13), 4, 0, 2);
        put(pool.allocated(13), 14, 12, 2);
    }
    return { world, effects, soundStarts };
}
