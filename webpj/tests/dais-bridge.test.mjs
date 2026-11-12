// 다이스 다리 명령 raw-dais-regular.js(RawDaisRegular.bridgeCommand)를 원본 10.78 daisbridge 관측과 대조한다.
//  - 원본 관측 daisbridge-x86.tsv 의 originals 320행: 입력 JSON(blocked·boss·current·depth·explicit·legacy·stage)에서 실제 RawPathStart/RawPathSearch(BFS)/Host/RawPathData 직렬화/
//    RawPathCleanup 에 연결한 같은 풀의 새/기존 경로가 만든 생성·이전 정리 사건(C/K)·꼭짓점 수·명령 필드 여섯 개·꼭짓점/방향/시각 비트·Form 부모를 전수 대조한다.
// cpppj DaisBridgeTests 의 장면(합성 자산·표면 11×11·포획 사제)을 같은 순서로 옮겼다. 실제 게임·브라우저 실행이 아니다. (2026-11-13 WB04-E367: 7-P3 RawDaisRegular 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, floatToBits } from './raw-support.mjs';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { SidPool } from '../www/core/sid-pool.js';
import { Kernel } from '../www/core/kernel.js';
import { GameRandom } from '../www/core/random.js';
import { ScrambledSpStore } from '../www/core/sp-store.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidPop, RAW_POP_RESULT } from '../www/core/squid-pop.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { SquidFrame } from '../www/core/squid-frame.js';
import { RawSquidDestroy, SQUID_DESTROY_EFFECT } from '../www/core/squid-destroy.js';
import { SquidPostPop } from '../www/core/squid-postpop.js';
import { SquidPostPopState } from '../www/core/squid-postpop-state.js';
import { SquidReward, SquidRewardState } from '../www/core/squid-reward.js';
import { SquidProcessHost, SquidProcessState } from '../www/core/squid-process.js';
import { RawSquidNeighbors } from '../www/core/raw-squid-neighbors.js';
import { RawPriestRitualActions } from '../www/core/raw-priest-ritual-actions.js';
import { RawTempleRitual, TempleRitualState } from '../www/core/raw-temple-ritual.js';
import { RawRunePostPop, RunePostPopState } from '../www/core/raw-rune-post-pop.js';
import { RawCarrierPreDestroy } from '../www/core/raw-carrier-pre-destroy.js';
import { ContainedFinderState } from '../www/core/raw-contained-finder.js';
import { RawContainedLookup } from '../www/core/raw-contained-lookup.js';
import { RawPathStart } from '../www/core/raw-path-start.js';
import { RawPathProcess, RawPathProcessState } from '../www/core/raw-path-process.js';
import { RawPathSearch, RawPathSearchState, makePathSearchHooks } from '../www/core/raw-path-search.js';
import { RawPathCleanup, createPathCleanupState } from '../www/core/raw-path-cleanup.js';
import { PATH_LAYOUT } from '../www/core/raw-path-runtime.js';
import { createPathDataHeader, makePathFormTransmission } from '../www/core/raw-path-data.js';
import { RawMovementProcessTypes, selectMovementProcessType } from '../www/core/raw-movement-process-types.js';
import { PATCH_DAIS_VTABLE } from '../www/core/raw-dais-content-notice.js';
import { DaisRegularState, RawDaisRegular } from '../www/core/raw-dais-regular.js';

// 원본 관측 10.78(originals) 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-13 WB04-E367)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'daisbridge-x86.tsv'))?.filter((row) => row[0] === 'originals') ?? null;

// 선택하지 않은 의식/장치/도착 효과를 실행 가능한 빈 효과로 바꾸지 않는다. (2026-11-13 WB04-E367)
const forbidden = () => { throw new Error('Bridge 명령의 미선택 하위 효과'); };

// 188칸 TYPE 입력이다(cpppj MakeFormTypeInputs). (2026-11-13 WB04-E367)
function formTypeInputs() {
    return Array.from({ length: 188 }, (_, number) => ({
        cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0, group: 0,
        ...([1, 2, 3, 6].includes(number) ? { constructorAddress: typeConstructorAddress(number) } : {}),
    }));
}

// double 의 비트를 십진 문자열로 바꾼다. (2026-11-13 WB04-E367)
function doubleBits(value) {
    const view = new DataView(new ArrayBuffer(8));
    view.setFloat64(0, value, true);
    return view.getBigUint64(0, true).toString();
}

// 현재 입력 프레임/첫 표/핫스폿은 기존 원본과 같은 합성 자산이다(158 사제·164 제단·165 다이스·166 룬, 표면 83). 출력 목표/경로는 계산하지 않는다. (2026-11-13 WB04-E367)
function bridgeAssets(types, frames, shapes) {
    const patterns = [[158, `AEKLMNOP${'P'.repeat(24)}`], [164, `${'A'.repeat(3)}${'O'.repeat(4)}${'M'.repeat(3)}${'L'.repeat(3)}${'P'.repeat(3)}`], [165, 'A'.repeat(180)], [166, 'A'.repeat(6)]];
    for (const [type, pattern] of patterns) {
        types[type].footX = types[type].footY = type === 165 ? 3 : 1;
        types[type].maxHitPoints = 200;
        types[type].flags1 = 16;
        shapes[type] = { loaded: true, frameCount: pattern.length, frames: [] };
        const codes = [];
        // 원본의 signed short 핫스폿은 Temple.stagePoint 가 실제로 읽는다.
        for (let index = 0; index < pattern.length; index += 1) {
            codes.push({ side: pattern.charCodeAt(index), variant: 0x50, number: index + 1, flags: 0 });
            shapes[type].frames.push({ width: 1, height: 1, hotspotX: (index % 5) * 16, hotspotY: (index % 5) * 11, muzzleX: (index % 5) * 16, muzzleY: (index % 5) * 11 });
        }
        frames[type] = new FrameCodeTable(codes);
    }
    // 원본 입력의 표면 83 과 사제 158 코드 포인터가 공유되어 최종 표면 코드도 AP 가 된다.
    types[158].flags2 = 0x210000;
    Object.assign(types[83], { footX: 1, footY: 1, flags1: 0x800, flags2: 2 });
    frames[83] = new FrameCodeTable([{ side: 0x41, variant: 0x50, number: 1, flags: 0 }]);
}

// 실제 꼭짓점/방향/시각 비트를 원본과 같은 구분자로 읽는다. (2026-11-13 WB04-E367)
function bridgePoints(runtime) {
    const parts = [];
    // 꼭짓점 순서는 실제 BFS 와 Prepare 가 정한다.
    for (let index = 0; index < runtime.count; index += 1) {
        const point = runtime.point(index);
        parts.push(`${floatToBits(point.x)},${floatToBits(point.y)},${runtime.direction(index)},${doubleBits(runtime.time(index))}`);
    }
    return parts.length === 0 ? '-' : parts.join(';');
}

// 원본 allocator/이전 경로 Kill 만 공통 사건으로 비교한다. Host/Frame/반납은 실제 저장소를 검사한다. (2026-11-13 WB04-E367)
const bridgeEffects = (events) => {
    const selected = events.filter((event) => event === 'C' || event === 'K');
    return selected.length ? selected.join(';') : '-';
};

test('DaisRegular.bridgeCommand: 원본 10.78 daisbridge 320행 — 같은 풀의 새/기존 실제 Path 와 명령 꼬리·마지막 점·두 번째 Prepare·깊이·Host 가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    let checked = 0;
    // 각 입력은 자체 풀/해시/Host/프레임/전역을 사용한다.
    for (const row of rows) {
        assert.equal(row.length, 7);
        const input = JSON.parse(row[1]);
        const boss = input.boss !== 0;
        const legacy = input.legacy !== 0;
        const pool = new SidPool(24000, { server: true });
        const hash = new SquidHash();
        const spots = new Uint8Array(65536);
        const types = formTypeInputs();
        const tables = types.map(() => new FrameCodeTable([]));
        const shapes = types.map(() => ({ loaded: false, frameCount: 0, frames: [] }));
        bridgeAssets(types, tables, shapes);
        // 경로 검색/프로세스는 평면 코드 배열을, 의식/다이스는 코드표를 읽는다(같은 코드).
        const flat = tables.map((table) => table.codes);
        // Native 의 모든 입력 SID 를 실제 client free list 에서 확보한다.
        for (let sid = 5; sid < 221; sid += 1) {
            assert.equal(pool.allocate(2), sid);
            pool.allocated(sid).fill(0);
        }
        // 합성 가상 표는 일반 가상 표로 준비하고 실제 Dais 가상 표는 아래에서 유지한다.
        const insert = (sid, type, x, y, level) => {
            const raw = pool.allocated(sid);
            put(raw, 0, 0x501dd0);
            raw[10] = type;
            put(raw, 14, floatToBits(x));
            put(raw, 18, floatToBits(y));
            raw[33] = level;
            put(raw, 4, hash.bucket(level, x, y), 2);
            hash.setBucket(level, x, y, sid);
        };
        insert(5, 158, 9.2, 9.3, 1);
        insert(60, 165, 11.75, 11.25, 1);
        insert(61, 164, 11.75, 11.25, 1);
        put(pool.allocated(60), 0, PATCH_DAIS_VTABLE);
        put(pool.allocated(60), 36, 3 + 11 * input.stage, 4);
        let next = 100;
        // 입력 표면을 실제 hash0/Graph3/spot6 에 넣는다. 경로는 기존 실제 BFS 가 만든다.
        for (let y = 9; y < 20; y += 1) {
            for (let x = 9; x < 20; x += 1) {
                const sid = next;
                next += 1;
                insert(sid, 83, x, y, 0);
                pool.allocated(sid)[30] = 3;
                spots[y * 256 + x] = 6;
            }
        }
        // blocked 입력은 표면 머리만 지우며 사제/제단/포획 자격 조건을 유지한다.
        if (input.blocked) for (let y = 0; y < 256; y += 1) for (let x = 0; x < 256; x += 1) hash.setCell(0, x, y, 0);
        for (const sid of [5, 60, 61]) pool.allocated(sid)[34] = 1;
        put(pool.allocated(5), 26, 200, 4);
        put(pool.allocated(60), 6, 63, 2);
        const captured = pool.allocated(63);
        put(captured, 0, 0x11010000);
        captured[10] = 6;
        put(captured, 12, 2, 2);
        put(captured, 18, 158, 2);
        put(captured, 20, 32, 2);
        const pop = new SquidPop(pool, hash, spots);
        const unpop = new SquidUnpop(pool, hash, spots);
        const destroy = new RawSquidDestroy(pool, unpop, types);
        const kernel = new Kernel(32);
        const clock = new SquidProcessState();
        clock.now = 123.125;
        clock.boss = boss;
        const pathState = new RawPathProcessState(clock);
        const book = new SquidPostPopState();
        const post = new SquidPostPop(pool, types, book);
        const host = new SquidProcessHost(pool, types, kernel, destroy, clock, { asset: { selected: () => 0, emit: (event) => { assert.equal(event.effect, SQUID_DESTROY_EFFECT.transmit); } } });
        const search = new RawPathSearch(pool, hash, spots, types, flat, new RawPathSearchState());
        const effects = [];
        let transmissions = 0;
        let frameUpdates = 0;
        let ritual = null;
        const header = createPathDataHeader();
        const packet = new Uint8Array(2048).fill(0xa5);
        host.setFormTransmit(makePathFormTransmission(pool, types, header, packet, (length) => { assert.ok(length > 0 && get(packet, 6, 2) === 5); transmissions += 1; }));
        const frame = new SquidFrame(pool, types, {
            frameSize: () => [1, 1],
            update: (sid, flags) => { assert.ok(sid === 5 && flags === 0); frameUpdates += 1; },
            unpop: (sid, flags) => { unpop.unpop(sid, types[pool.slot(sid)[10]], flags); },
            pop: (sid, x, y, flags) => { assert.equal(pop.pop(sid, types[pool.slot(sid)[10]], 1, 1, x, y, flags), RAW_POP_RESULT.registered); },
        });
        const lookup = new RawContainedLookup(pool, forbidden);
        const containedState = new ContainedFinderState();
        const carrier = new RawCarrierPreDestroy(pool, containedState, { validateDamageable: forbidden, damageablePre: forbidden, dependentFound: forbidden });
        const cleanupState = createPathCleanupState();
        cleanupState.boss = boss;
        const cleanup = new RawPathCleanup(pool, hash, types, cleanupState, {
            reserveShared: forbidden, addShared: forbidden, contained: (sid, mask, kind, index) => carrier.findContained(sid, mask, kind, index), graphFor: forbidden,
            ritual: (dais, priest, mode) => { assert.ok(ritual); ritual.ritual(dais, priest, mode); }, refund: forbidden, destroy: forbidden,
        });
        const processHooks = (old) => makePathSearchHooks(pool, search, {
            unpop: (sid, flags) => { unpop.unpop(sid, types[pool.slot(sid)[10]], flags); },
            pop: (sid, point, flags) => { assert.equal(pop.pop(sid, types[pool.slot(sid)[10]], 1, 1, point.x, point.y, flags), RAW_POP_RESULT.registered); },
            repop: (sid, flags) => {
                const raw = pool.slot(sid);
                const view = new DataView(raw.buffer, raw.byteOffset, raw.byteLength);
                assert.equal(pop.pop(sid, types[raw[10]], 1, 1, view.getFloat32(14, true), view.getFloat32(18, true), flags), RAW_POP_RESULT.registered);
            },
            halt: forbidden, arrival: forbidden, stepSound: forbidden, soundId: forbidden, clearBlocked: forbidden,
            cleanup: (runtime, parent) => { assert.equal(parent, 5); if (old) effects.push('K'); cleanup.afterSpatial(runtime, parent); },
        });
        let oldForm = 0;
        // 객체 메모리 재사용과 관계없이 실제 이전 Form SID 를 구분한다.
        if (legacy) {
            const old = new RawPathProcess(pool, hash, types, flat, shapes, spots, pathState, processHooks(true), { priest: true });
            old.runtime.setMoveAwayDepth(input.depth);
            const attached = RawPathProcess.attach(host, old, 5, 0x10);
            assert.ok(attached);
            oldForm = attached.form;
        }
        const mapper = new RawMovementProcessTypes();
        const processType = (type) => selectMovementProcessType(types, mapper, type);
        const metadata = () => ({ speed: Math.fround(0.7), turningSpeed: 90, defaultFrame: 7 });
        const path = new RawPathStart(pool, hash, types, host, lookup, frame, { clock, dudeType: 70, teleportType: 134, manaBoltType: 105, priestType: 158, geyserType: 122, snapPriestDestination: false, retrying: false }, {
            processType, metadata, clearBlocked: forbidden,
            create: (priest) => { assert.equal(priest, true); effects.push('C'); return new RawPathProcess(pool, hash, types, flat, shapes, spots, pathState, processHooks(false), { priest }); },
        });
        const footprint = new RawSquidNeighbors(pool, hash, spots, types, tables);
        const reward = new SquidReward(pool, types, book, new SquidRewardState(), new ScrambledSpStore(new GameRandom(0), () => 1));
        const actions = new RawPriestRitualActions(pool, tables, { setFrame: forbidden, reserveShared: forbidden, addShared: forbidden });
        const templeState = new TempleRitualState({ boss });
        ritual = new RawTempleRitual(pool, hash, types, tables, shapes, spots, footprint, reward, actions, templeState, {
            create: forbidden, setHitPoints: forbidden, setOwner: forbidden, pop: forbidden, playAt: forbidden, reserveShared: forbidden, addShared: forbidden,
            count: (sid) => ritual.count(sid),
        });
        const runeState = new RunePostPopState();
        const rune = new RawRunePostPop(pool, hash, types, runeState, post, { reserveRegular: forbidden, addRegular: forbidden, startConstruction: forbidden });
        const state = new DaisRegularState(templeState, runeState, clock, 1);
        const dais = new RawDaisRegular(pool, hash, types, tables, ritual, rune, carrier, host, lookup, path, state, {
            setFrame: forbidden, destroy: forbidden, reserveShared: forbidden, addShared: forbidden, playAt: forbidden, reserveSound: forbidden, loadSound: forbidden,
            attachSound: forbidden, wallSeconds: forbidden, setSoundStart: forbidden, stopVoice: forbidden, processType, pathMetadata: metadata, damageable: forbidden,
            addLocalSp: forbidden, normalizeForeignSp: forbidden, grantForeignType: forbidden, localSpecial: forbidden, localType: forbidden, refreshOwner: forbidden,
            refreshLocal: forbidden, priestExists: forbidden, multiplayerStatus: forbidden, chat: forbidden,
        });
        dais.bridgeCommand(60, input.explicit ? 5 : 0, input.current);
        const form = lookup.find(5, 54);
        const process = form ? host.fromForm(form) : null;
        const current = process instanceof RawPathProcess ? process : null;
        const created = Number(row[3]) !== 0;
        const where = `행 ${checked} (${row[1]})`;
        assert.equal(bridgeEffects(effects), bridgeEffects(row[2].split(';')), `${where} 사건`);
        if (created) {
            assert.ok(current && form !== oldForm, `${where} 새 경로`);
            assert.equal(current.runtime.count, Number(row[3]), `${where} 꼭짓점 수`);
            assert.equal(bridgePoints(current.runtime), row[5], `${where} 꼭짓점`);
            const raw = current.runtime.bytes;
            const fields = [PATH_LAYOUT.targetA, PATH_LAYOUT.targetB, PATH_LAYOUT.helper, 0x186e4, 0x186e8, 0x186d8].map((offset) => get(raw, offset));
            assert.equal(fields.join(','), row[4], `${where} 명령 필드`);
            assert.ok(form >= pool.layout.serverFirst && get(pool.slot(form), 16, 2) === 5 && current.parent === 5, `${where} Form 부모`);
            assert.ok(transmissions === 1 && frameUpdates === 2 && kernel.size === 1, `${where} 전송/갱신/커널`);
        } else {
            assert.ok(transmissions === 0 && frameUpdates === 0, `${where} 무전송`);
            if (bridgeEffects(effects).includes('K')) assert.ok(!current && kernel.size === 0, `${where} 이전 경로 제거`);
            else assert.ok((current !== null) === legacy && kernel.size === (legacy ? 1 : 0), `${where} 기존 경로 유지`);
        }
        // Kernel 종료는 destructor 경계이며 아직 경로 걷기/가상 도착의 완료를 주장하지 않는다.
        kernel.clear();
        checked += 1;
    }
    assert.equal(checked, 320);
});
