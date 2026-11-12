// 다이스 의식 상위 몸체 raw-dais-regular.js(RawDaisRegular)와 공용 분기 raw-ritual-regular.js 를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측 daisregular-core-x86.tsv 의 originals 행(334): 첫 하위 범위(사제 604 중지·붕괴·희생 시작·희생 진행·305 단계·다리 자격)의 반환 float 비트·효과 순서·네 슬롯 raw 전체.
//  - 원본 관측 daisprogress-x86.tsv 의 originals 행 중 다이스만 쓰는 모드(진행·룬 304·희생 진행): 효과 순서·난수 상태 없음·Player 결과 DWORD·다섯 슬롯 raw·지연 소리 시각 이미지.
//    (제단 정기를 쓰는 handle 모드는 RawAltarSacrifice 이식 때 이어서 대조한다.)
//  - 웹 계약: 생성자 거부, 가상표 거부, 훅 누락 거부, 공용 분기 순서, 상태 객체, 훅 공장.
// 합성 장면(방향 코드·첫 프레임·핫스폿)은 cpppj DaisRegularTests/DaisProgressTests 의 입력을 같은 순서로 옮겼다. 실제 게임·브라우저 실행이 아니다. (2026-11-13 WB04-E367: 7-P3 RawDaisRegular 이식과 함께 추가)
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
import { RawPathProcess, RawPathProcessState } from '../www/core/raw-path-process.js';
import { PATCH_DAIS_VTABLE } from '../www/core/raw-dais-content-notice.js';
import { DaisRegularState, RawDaisRegular, makeDaisRegularHooks } from '../www/core/raw-dais-regular.js';
import { runRuneRegular, runDaisHandle } from '../www/core/raw-ritual-regular.js';
import { VTABLE, CAPACITY, ALTAR_VTABLE, hex, joinEffects, forbid, formTypeInputs, applyAssets, corePatterns, makeWorld, replaceAltarFrames, doubleImage, CHAT_KEYS, progressScene } from './dais-support.mjs';

// 원본 관측 10.78(originals) 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-13 WB04-E367)
const readOriginals = (name) => readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', name))?.filter((row) => row[0] === 'originals') ?? null;
const coreRows = readOriginals('daisregular-core-x86.tsv');
const progressRows = readOriginals('daisprogress-x86.tsv');


test('DaisRegular: 원본 10.78 코어 관측 전 행의 반환 비트·효과 순서·네 슬롯 raw 가 같다', { skip: !coreRows && '원본 fixture 없음' }, () => {
    let checked = 0;
    // 입력마다 새 풀/해시/Host 를 만들어 이전 행의 결과를 쓰지 않는다.
    for (const row of coreRows) {
        assert.equal(row.length, 5);
        const input = row[1].split(',');
        assert.equal(input.length, 11);
        const [mode, , side] = input;
        const [currentFrame, priestFrame, eventExists, reserve, altarExists, captured, genus, priestOwner, alliance] = [1, 3, 4, 5, 6, 7, 8, 9, 10].map((index) => Number(input[index]));
        const effects = [];
        let pending = false;
        let world;
        const setFrame = (sid, value, flags) => {
            // 604 삭제는 첫 프레임 지정 직전에 한 번 K 로 기록한다.
            if (pending && !world.host.findEvent(6, 604)) { effects.push('K:604,6,0'); pending = false; }
            assert.equal(flags, 0);
            effects.push(`F:${sid},${value >>> 0},0`);
            put(world.pool.allocated(sid), 36, value >>> 0, 4);
        };
        world = makeWorld({ patterns: corePatterns(side[0]) }, {
            setFrame,
            reserveShared: () => { effects.push('N:40'); return reserve !== 0; },
            addShared: (sid, event, payload) => { effects.push(`S:${event},${sid},${floatToBits(payload)}`); },
            playAt: (x, y, name, flags) => { effects.push(`A:${floatToBits(x)},${floatToBits(y)},${name},${flags}`); },
        });
        const { pool, slots, insert } = world;
        insert(5, 165);
        insert(6, 158);
        if (altarExists) insert(7, 164);
        put(slots[5], 0, PATCH_DAIS_VTABLE);
        put(slots[5], 36, currentFrame >>> 0, 4);
        put(slots[6], 36, (mode === 'stop' ? priestFrame : 0) >>> 0, 4);
        put(slots[7], 36, (mode === 'can' ? priestFrame : 1) >>> 0, 4);
        slots[5][34] = 1;
        slots[6][34] = priestOwner;
        slots[7][34] = 1;
        setRawSquidHitPoints(pool, 6, 100);
        world.types[158].flags2 = genus;
        put(slots[5], 6, captured ? 10 : 0, 2);
        put(slots[10], 0, VTABLE);
        slots[10][10] = 6;
        put(slots[10], 12, 2, 2);
        put(slots[10], 18, 158, 2);
        put(slots[10], 20, 32, 2);
        world.state.alliances[2 + 9] = alliance;
        // 사제 604 를 실제 Host 에 붙여 중지 본문이 첫 604 를 지우는지 본다.
        if (mode === 'stop' && eventExists) {
            assert.ok(world.host.attach(new RegularProcess(604, 0.01), 61, 6, 0x10));
            assert.ok(world.host.findEvent(6, 604));
            assert.equal(get(pool.slot(6), 6, 2), 12);
            pending = true;
        }
        let returned = '-';
        if (mode === 'stop') world.dais.stopPriestRitual(6);
        else if (mode === 'collapse') world.dais.collapse(5);
        else if (mode === 'start') world.dais.startSacrifice(5);
        else if (mode === 'advance') returned = String(floatToBits(world.dais.sacrificeAdvance(5)));
        else if (mode === 'handle305') returned = String(floatToBits(world.dais.handle(5, 0x305, 1, 9.25)));
        else if (mode === 'can') returned = world.dais.canBridge(5, 6) ? '1' : '0';
        else assert.fail(`Dais 첫 fixture 모드 누락: ${mode}`);
        assert.equal(returned, row[2], `행 ${checked} 반환`);
        assert.equal(joinEffects(effects), row[3], `행 ${checked} 효과`);
        const images = row[4].split(';');
        assert.equal(images.length, 4);
        // 가상 표 주입 DWORD 외 초기 범위가 읽고 쓴 raw 전체를 비교한다.
        [5, 6, 7, 10].forEach((sid, index) => assert.equal(hex(pool.slot(sid)).slice(8), images[index].slice(8), `행 ${checked} 슬롯 ${sid}`));
        world.kernel.clear();
        checked += 1;
    }
    assert.equal(checked, 334);
});


test('DaisRegular: 원본 10.78 진행 관측(진행·룬 304·희생 진행)의 효과 순서·Player 결과·다섯 슬롯 raw·지연 소리 시각이 같다', { skip: !progressRows && '원본 fixture 없음' }, () => {
    let checked = 0;
    // 제단 정기를 쓰는 handle 모드는 RawAltarSacrifice 이식 때 대조하므로 건너뛴다.
    for (const row of progressRows.filter((candidate) => !candidate[1].includes('"mode":"handle"'))) {
        assert.equal(row.length, 8);
        const input = JSON.parse(row[1]);
        const { world, effects, soundStarts } = progressScene(input);
        const { pool, state } = world;
        const payload = Math.fround(Number(input.payload));
        let returned = '-';
        if (input.mode === 'progress') world.dais.progress(5);
        else if (input.mode === 'rune') returned = String(floatToBits(world.dais.handle(9, 0x304, 0, payload)));
        else if (input.mode === 'advance') returned = String(floatToBits(world.dais.sacrificeAdvance(5)));
        else assert.fail(`DaisProgress 모드 누락: ${input.mode}`);
        assert.equal(returned, row[2], `행 ${checked} 반환`);
        assert.equal(joinEffects(effects), row[3], `행 ${checked} 효과`);
        assert.equal(state.playerResults.join(','), row[7], `행 ${checked} Player 결과`);
        // 난수는 다이스 모드에서 소비하지 않으므로 관측의 최종 상태가 입력 seed 와 같다.
        assert.equal(Number(row[6]), input.seed, `행 ${checked} 난수`);
        const images = row[4].split(';');
        assert.equal(images.length, 5);
        [5, 6, 7, 9, 10].forEach((sid, index) => assert.equal(hex(pool.slot(sid)).slice(8), images[index].slice(8), `행 ${checked} 슬롯 ${sid}`));
        assert.equal(soundStarts.map(doubleImage).join(';'), row[5], `행 ${checked} 소리 시각`);
        world.kernel.clear();
        checked += 1;
    }
    assert.equal(checked, 426);
});

// ---------------------------------------------------------------------------------------------------------------------
// cpppj DaisCheckingParentsTests 의 자격·명령·깊이 계약 이식이다. 같은 실제 Pool/Host/PathStart 를 공유하되 행동 효과는 모두 명시 실패 장치로 연결하고,
// 진단 복귀 중 외부 입력(onAssertion/onCount)이 현재 raw·권한·동맹을 바꿔도 몸체가 현재 값을 다시 읽는지 확인한다. (2026-11-13 WB04-E367)
// ---------------------------------------------------------------------------------------------------------------------

// 진단 복귀 계약 검사용 장면이다. effects 는 금지 행동이 불린 횟수, diagnostics 는 진단 줄 번호 기록, allianceArguments 는 동맹 조회 인자 기록이다.
function parentScene(options = {}) {
    const scene = {
        effects: 0, pathCreates: 0, countReads: 0, allianceReads: 0, countValue: 1, checking: 1, currentBoss: true, currentAlliance: 0, canonicalPriestType: 158,
        diagnostics: [], allianceArguments: [], onAssertion: null, onCount: null,
    };
    // 호출된 행동을 빈 성공으로 대체하지 않고 횟수를 세어 실패시킨다.
    const forbidden = () => { scene.effects += 1; throw new Error('다이스 checking 검사에서 실제 행동 효과 금지'); };
    const pool = new SidPool(CAPACITY, { server: true });
    const hash = new SquidHash();
    const spots = new Uint8Array(65536);
    const types = formTypeInputs();
    const frames = types.map(() => new FrameCodeTable([]));
    const shapes = types.map(() => ({ loaded: false, frameCount: 0, frames: [] }));
    // 자격·StagePoint 조회가 읽는 합성 코드/핫스폿만 준비한다(기본 제단 방향 I 와 거부 방향 M 을 구분).
    for (const type of [74, 158, 164, 165, 166]) {
        types[type].footX = types[type].footY = type === 165 ? 3 : 1;
        types[type].maxHitPoints = 200;
        const codes = Array.from({ length: 180 }, () => ({ side: 0x49, variant: 0x50, number: 1, flags: 0 }));
        if (type === 164) codes[1].side = 0x4d;
        frames[type] = new FrameCodeTable(codes);
        shapes[type] = { loaded: true, frameCount: 180, frames: Array.from({ length: 180 }, () => ({ width: 0, height: 0, hotspotX: 0, hotspotY: 0, muzzleX: 0, muzzleY: 0 })) };
    }
    types[158].flags2 = 0x200000;
    const raw = (sid) => pool.physical(sid);
    // 사용 슬롯을 실제 할당해 Host 부착 입력과 공유한다.
    for (let sid = 5; sid <= 12; sid += 1) {
        assert.equal(pool.allocate(2), sid);
        raw(sid).fill(0);
    }
    const object = (sid, type, x, y) => { raw(sid)[10] = type; put(raw(sid), 0, 0x501dd0); put(raw(sid), 14, floatToBits(x)); put(raw(sid), 18, floatToBits(y)); };
    object(5, 165, 40.75, 41.25);
    object(6, 158, 43.75, 44.25);
    object(7, 164, 40.75, 41.25);
    put(raw(5), 0, PATCH_DAIS_VTABLE);
    put(raw(5), 36, 3, 4);
    for (const sid of [5, 6, 7]) raw(sid)[34] = 1;
    hash.setBucket(1, 40.75, 41.25, 7);
    // 포획 행동 없이 내용물 finder 의 명시 자격 입력만 둔다.
    put(raw(5), 6, 10, 2);
    raw(10)[10] = 6;
    put(raw(10), 18, 158, 2);
    put(raw(10), 20, 0x20, 2);
    raw(10)[12] = 2;
    const book = new SquidPostPopState();
    const clock = new SquidProcessState();
    clock.boss = false;
    const pathState = new RawPathProcessState(clock);
    const finderState = new ContainedFinderState();
    finderState.bindChecking(() => scene.checking);
    finderState.bindBoss({ get value() { return scene.currentBoss; } });
    const assertion = (expression, file, line) => {
        scene.diagnostics.push(line);
        if (scene.onAssertion) scene.onAssertion(expression, file, line);
        else throw new Error('검사에서 선택하지 않은 원문 진단');
    };
    finderState.assertion = assertion;
    const templeState = new TempleRitualState();
    const runeState = new RunePostPopState();
    const state = new DaisRegularState(templeState, runeState, clock, 1);
    state.debug = () => scene.checking;
    const post = new SquidPostPop(pool, types, book);
    const destroy = new RawSquidDestroy(pool, new SquidUnpop(pool, hash, spots), types);
    const kernel = new Kernel(32);
    const host = new SquidProcessHost(pool, types, kernel, destroy, clock);
    const frame = new SquidFrame(pool, types, { frameSize: forbidden, update: forbidden, unpop: forbidden, pop: forbidden });
    const carrier = new RawCarrierPreDestroy(pool, finderState, { validateDamageable: forbidden, damageablePre: forbidden, dependentFound: forbidden });
    const lookup = new RawContainedLookup(pool, forbidden);
    // 성공 경로나 검색/이동 효과 없이 초기 Count 0 인 실제 객체만 만든다.
    // 삭제 통지(공간 접두 Unpop/Repop 과 정리)는 삭제를 시험하는 곳에서만 quiet 로 허용한다.
    const emptyPath = (quiet = false) => new RawPathProcess(pool, hash, types, frames, shapes, spots, pathState, {
        unpop: quiet ? () => {} : forbidden, repop: quiet ? () => {} : forbidden, pop: forbidden, halt: forbidden, arrival: forbidden, soundId: forbidden, stepSound: forbidden,
        search: forbidden, clearBlocked: forbidden, cleanup: quiet ? () => {} : forbidden,
    }, { priest: true });
    const pathStartState = { clock, dudeType: 70, teleportType: 134, manaBoltType: 105, priestType: 158, geyserType: 122, snapPriestDestination: false, retrying: false };
    const realPath = new RawPathStart(pool, hash, types, host, lookup, frame, pathStartState, {
        processType: () => 54, create: (priest) => { assert.equal(priest, true); scene.pathCreates += 1; return emptyPath(); }, metadata: forbidden, clearBlocked: forbidden,
    });
    // 시작 요청을 관찰해야 하는 시험은 같은 풀의 대역(pool·start)을 주입한다.
    const path = options.path ? options.path(scene, pool, realPath) : realPath;
    const footprint = new RawSquidNeighbors(pool, hash, spots, types, frames);
    const reward = new SquidReward(pool, types, book, new SquidRewardState(), new ScrambledSpStore(new GameRandom(0x12345678), forbidden));
    const actions = new RawPriestRitualActions(pool, frames, { setFrame: forbidden, reserveShared: forbidden, addShared: forbidden });
    const ritual = new RawTempleRitual(pool, hash, types, frames, shapes, spots, footprint, reward, actions, templeState, {
        create: forbidden, setHitPoints: forbidden, setOwner: forbidden, pop: forbidden, playAt: forbidden, reserveShared: forbidden, addShared: forbidden,
        count: () => { scene.countReads += 1; if (scene.onCount) scene.onCount(); return scene.countValue; },
    });
    const rune = new RawRunePostPop(pool, hash, types, runeState, post, { reserveRegular: forbidden, addRegular: forbidden, startConstruction: forbidden });
    // 의식 대역을 쓰면 다이스는 대역을, 장면은 실제 의식을 노출한다.
    const daisRitual = options.ritual ? options.ritual(ritual) : ritual;
    // 룬 대역을 쓰면 다이스는 대역(제단·다이스 조회)을 본다.
    const daisRune = options.rune ? options.rune(rune) : rune;
    const daisHooks = {
        setFrame: forbidden, destroy: forbidden, reserveShared: forbidden, addShared: forbidden, playAt: forbidden, reserveSound: forbidden, loadSound: forbidden,
        attachSound: forbidden, wallSeconds: forbidden, setSoundStart: forbidden, stopVoice: forbidden, processType: () => 54, pathMetadata: forbidden, damageable: forbidden,
        addLocalSp: forbidden, normalizeForeignSp: forbidden, grantForeignType: forbidden, localSpecial: forbidden, localType: forbidden, refreshOwner: forbidden,
        refreshLocal: forbidden, priestExists: forbidden, multiplayerStatus: forbidden, chat: forbidden,
        assertion,
        boss: () => scene.currentBoss,
        alliance: (row, column) => { scene.allianceReads += 1; scene.allianceArguments.push([row, column]); return scene.currentAlliance; },
        count: (sid) => ritual.count(sid),
        priestType: () => scene.canonicalPriestType,
        ...(options.hooks ?? {}),
    };
    const dais = new RawDaisRegular(pool, hash, types, frames, daisRitual, daisRune, carrier, host, lookup, path, state, daisHooks);
    return Object.assign(scene, { pool, hash, types, frames, kernel, host, clock, templeState, ritual, dais, raw, emptyPath, pathStartState, state, finderState, rune, runeState, lookup, carrier, path, daisHooks });
}

test('DaisCheckingParents: CanBridge 는 genus 진단 전에 저장한 first/next 와 진단 뒤 현재 TYPE/owner/kind/동맹을 구별한다', () => {
    const scene = parentScene();
    const { raw } = scene;
    scene.types[158].flags2 = 0;
    put(raw(10), 4, 11, 2);
    put(raw(10), 20, 0, 2);
    raw(11)[10] = 6;
    put(raw(11), 18, 0, 2);
    raw(12)[10] = 6;
    scene.onAssertion = (expression, file, line) => {
        assert.deepEqual([expression, file, line], ['priest.getGenus() & gPRIEST', '.\\Dais.cpp', 0x140]);
        raw(6)[10] = 74;
        scene.types[74].flags2 = 0x200000;
        scene.canonicalPriestType = 74;
        raw(5)[34] = 2;
        raw(6)[34] = 2;
        raw(7)[34] = 3;
        // 새 head 12 와 바뀐 next 0 으로 검색을 재시작하면 결과가 달라진다(저장한 first/next 를 계속 써야 한다).
        put(raw(5), 6, 12, 2);
        put(raw(10), 4, 0, 2);
        put(raw(11), 18, 74, 2);
        put(raw(11), 20, 0x20, 2);
        raw(11)[12] = 0x84;
        scene.currentAlliance = 0;
    };
    assert.equal(scene.dais.canBridge(5, 6), true);
    assert.deepEqual(scene.diagnostics, [0x140]);
    assert.deepEqual(scene.allianceArguments, [[3, 4]]);
    assert.equal(scene.effects, 0);
});

test('DaisCheckingParents: dead 부모의 finder 진단이 복귀한 뒤에는 현재 head 를 처음 읽는다', () => {
    const scene = parentScene();
    const { raw } = scene;
    scene.currentBoss = false;
    raw(5)[11] = 2;
    put(raw(10), 20, 0, 2);
    raw(11)[10] = 6;
    put(raw(11), 18, 158, 2);
    put(raw(11), 20, 0x20, 2);
    raw(11)[12] = 3;
    scene.onAssertion = (expression, file, line) => {
        assert.deepEqual([expression, file, line], ['!p.isDead() || boss || yesIKnowThatIShouldntLookInsideMyselfDuringADestroy', '\\Ns\\O\\Squidfinder.cpp', 0x13c]);
        raw(5)[11] = 0;
        put(raw(5), 6, 11, 2);
        scene.currentBoss = true;
    };
    assert.equal(scene.dais.canBridge(5, 6), true);
    assert.deepEqual(scene.diagnostics, [0x13c]);
    assert.deepEqual(scene.allianceArguments, [[1, 3]]);
    assert.equal(scene.effects, 0);
});

test('DaisCheckingParents: CanBridge 는 같은 조회에서 현재 동맹을 다시 읽고 풀을 바꾸지 않는다', () => {
    const scene = parentScene();
    scene.currentAlliance = 1;
    const before = scene.pool.bytes();
    assert.equal(scene.dais.canBridge(5, 6), false);
    scene.currentAlliance = 0;
    assert.equal(scene.dais.canBridge(5, 6), true);
    assert.deepEqual(scene.pool.bytes(), before);
    assert.equal(scene.effects, 0);
});

test('DaisCheckingParents: 명령은 현재 권한을 읽고 실제 빈 PathStart 뒤 p 진단에서 정상 반환한다', () => {
    const scene = parentScene();
    // 신전 의식 권한이 참이어도 다이스의 현재 Client 권한(훅)이 거짓이면 경로를 만들지 않는다.
    scene.currentBoss = false;
    scene.templeState.boss = true;
    scene.dais.bridgeCommand(5, 6, 1);
    assert.deepEqual([scene.pathCreates, scene.effects], [0, 0]);
    scene.currentBoss = true;
    scene.templeState.boss = false;
    scene.onAssertion = (expression, file, line) => {
        assert.deepEqual([expression, file, line], ['p', '.\\Dais.cpp', 0x18e]);
        scene.currentBoss = false;
        scene.checking = 0;
    };
    // NULL 뒤 진단 복귀는 재시도나 성공 경로 효과를 실행하지 않는다.
    scene.dais.bridgeCommand(5, 6, 1);
    assert.equal(scene.pathCreates, 1);
    assert.deepEqual(scene.diagnostics, [0x18e]);
    assert.equal(scene.kernel.size, 0);
    assert.equal(scene.effects, 0);
    assert.equal(scene.pathStartState.retrying, false);
});

test('DaisCheckingParents: 이동 깊이 진단의 비교는 저장한 깊이이고 최종 반환은 현재 깊이다', () => {
    const scene = parentScene();
    const process = RawPathProcess.attach(scene.host, scene.emptyPath(), 6, 0x10);
    assert.ok(process);
    process.runtime.setMoveAwayDepth(-1);
    scene.onAssertion = (expression, file, line) => {
        assert.deepEqual([expression, file, line], ['0 <= p->moveAwayDepth', '\\Ns\\O\\Carrier.cpp', 0x87b]);
        process.runtime.setMoveAwayDepth(5);
    };
    // 새 깊이 5 로 두 번째 비교를 다시 만들지 않는다.
    assert.equal(scene.dais.moveAwayDepth(6), 5);
    assert.deepEqual(scene.diagnostics, [0x87b]);
    scene.diagnostics.length = 0;
    process.runtime.setMoveAwayDepth(4);
    scene.onAssertion = (expression, file, line) => {
        assert.deepEqual([expression, file, line], ['p->moveAwayDepth <= MAX_MOVE_AWAY_DEPTH', '\\Ns\\O\\Carrier.cpp', 0x87c]);
        scene.checking = 0;
        process.runtime.setMoveAwayDepth(2);
    };
    assert.equal(scene.dais.moveAwayDepth(6), 2);
    assert.deepEqual(scene.diagnostics, [0x87c]);
    assert.equal(scene.effects, 0);
});

// ---------------------------------------------------------------------------------------------------------------------
// 웹 계약 시험: 원본 관측이 닿지 않는 경계(생성자 거부·상태 객체·공용 분기 순서·방향 경계·훅 누락·실제 기록 대여)를 확인한다. (2026-11-13 WB04-E367)
// ---------------------------------------------------------------------------------------------------------------------

// 기록하는 전체 훅 묶음이다. cfg 로 확보 결과와 지연 소리 마스크를 정하고 log 에 원본 표기로 쌓는다. (2026-11-13 WB04-E367)
function recorder(cfg = {}) {
    const log = [];
    const config = { reserve: true, soundMask: 3, attachMask: 3, ...cfg };
    let soundAttempt = 0;
    const hooks = {
        setFrame: (sid, value, flags) => { log.push(`F:${sid},${value},${flags}`); },
        destroy: (sid, flags) => { log.push(`D:${sid},${flags}`); },
        reserveShared: () => { log.push('N'); return config.reserve; },
        addShared: (sid, event, payload) => { log.push(`S:${sid},${event},${floatToBits(payload)}`); },
        playAt: (x, y, name, flags) => { log.push(`A:${name},${flags}`); },
        reserveSound: () => { const index = soundAttempt; soundAttempt += 1; log.push(`NS:${index}`); return (config.soundMask & (1 << index)) !== 0; },
        loadSound: (name, zero, flags) => { log.push(`LOAD:${name},${zero},${flags}`); return name === 'thunderCrack.wav' ? 0x9001 : 0x9000; },
        attachSound: (sid, sound, zero, flags) => { log.push(`ATTACH:${sid},${sound},${zero},${flags}`); return (config.attachMask & (1 << (sound - 0x9000))) ? 0x11017000 + (sound - 0x9000) * 0x100 : 0; },
        wallSeconds: () => { log.push('WALL'); return 100.5; },
        setSoundStart: (process, start) => { log.push(`START:${process},${start}`); },
        stopVoice: (cue, a, b) => { log.push(`V:${cue},${a},${b}`); },
        processType: forbid('processType'),
        pathMetadata: forbid('pathMetadata'),
        damageable: (sid, event, count, payload) => { log.push(`DMG:${sid},${event},${count},${payload}`); return 3.5; },
        addLocalSp: (delta) => { log.push(`SP:${delta}`); },
        normalizeForeignSp: (owner) => { log.push(`NF:${owner}`); },
        grantForeignType: (owner, type) => { log.push(`GF:${owner},${type}`); },
        localSpecial: (value) => { log.push(`LS:${value}`); },
        localType: (type) => { log.push(`LT:${type}`); },
        refreshOwner: (owner) => { log.push(`R:${owner}`); },
        refreshLocal: () => { log.push('RL'); },
        priestExists: (owner) => { log.push(`PE:${owner}`); return config.priestExists ? config.priestExists(owner) : true; },
        multiplayerStatus: () => { log.push('MS'); return 7; },
        chat: (message) => { log.push(`CHAT:${message.kind},${message.originalOwner},${message.currentOwner},${message.status}`); },
        notifySurface: (sid) => { log.push(`NOTE:${sid}`); },
        assertion: (expression, file, line) => { log.push(`ASSERT:${line.toString(16)}`); },
        ...(cfg.hooks ?? {}),
    };
    return { log, hooks, config };
}

// 코어 장면과 같은 배치(5 다이스·6 사제·7 제단·10 포획 Form)를 만든 세계이다. frames 는 patterns 로 바꿀 수 있다. (2026-11-13 WB04-E367)
function standardWorld(cfg = {}, patterns = null) {
    const rec = recorder(cfg);
    const world = makeWorld(patterns ? { patterns } : {}, rec.hooks);
    const { pool, slots, insert } = world;
    insert(5, 165);
    insert(6, 158);
    insert(7, 164);
    put(slots[5], 0, PATCH_DAIS_VTABLE);
    put(slots[5], 36, cfg.daisFrame ?? 3, 4);
    slots[5][34] = 1;
    slots[6][34] = 1;
    slots[7][34] = 1;
    setRawSquidHitPoints(pool, 6, 100);
    world.types[158].flags2 = 0x210000;
    put(slots[7], 0, ALTAR_VTABLE);
    return { ...world, ...rec };
}

test('DaisRegular: 생성자는 10.78 풀·같은 풀의 여섯 협력 부품·188칸 표를 모두 확인한다', () => {
    const w = makeWorld();
    const other = makeWorld();
    const make = (overrides = {}) => {
        const a = { pool: w.pool, hash: w.hash, types: w.types, frames: w.frames, ritual: w.ritual, rune: w.rune, carrier: w.carrier, host: w.host, lookup: w.lookup, path: w.path, ...overrides };
        return new RawDaisRegular(a.pool, a.hash, a.types, a.frames, a.ritual, a.rune, a.carrier, a.host, a.lookup, a.path, w.state, w.hooks);
    };
    assert.ok(make().pool === w.pool);
    // 다른 월드의 부품 하나라도 섞이면 거부한다.
    for (const name of ['ritual', 'rune', 'carrier', 'host', 'lookup', 'path']) assert.throws(() => make({ [name]: other[name] }), TypeError, name);
    assert.throws(() => make({ pool: { ...w.pool, edition: 'cd1072' } }), TypeError);
    assert.throws(() => make({ types: w.types.slice(0, 187) }), TypeError);
    assert.throws(() => make({ frames: w.frames.slice(0, 187) }), TypeError);
    assert.throws(() => make({ types: { length: 188 } }), TypeError);
    assert.throws(() => make({ frames: { length: 188 } }), TypeError);
    assert.throws(() => make({ ritual: null }), TypeError);
    assert.throws(() => make({ pool: null }), TypeError);
});

test('DaisRegularState: 기본값·로컬 owner 대여·획득 플래그·Player 기록 대여가 원본 필드 폭과 위치를 따른다', () => {
    const w = makeWorld();
    const state = new DaisRegularState(w.templeState, w.runeState, w.clock);
    // 원본 초기 전역 기본값과 표 크기이다.
    assert.deepEqual([state.sacrificeType, state.nuggetType, state.noIslandType, state.bridgeType, state.selectedType, state.voiceCue, state.localOwner], [169, 154, 93, 82, 0, 0, 0]);
    assert.deepEqual([state.alliances.length, state.grantFlags.length, state.playerResults.length], [81, 9, 9]);
    assert.ok(state.ritual === w.templeState && state.rune === w.runeState && state.clock === w.clock);
    assert.deepEqual([state.currentNuggetType, state.currentBridgeType], [154, 82]);
    // 생성자 로컬 번호와 부호 없는 변환이다.
    assert.equal(new DaisRegularState(w.templeState, w.runeState, w.clock, 3).localOwner, 3);
    state.localOwner = -1;
    assert.equal(state.localOwner, 0xffffffff);
    // 실제 장기 상자를 빌리면 읽기/쓰기 모두 그 상자를 쓰고, null 이면 독립 값으로 돌아간다.
    const box = { value: 6 };
    state.localOwner = 2;
    state.bindLocalOwner(box);
    assert.equal(state.localOwner, 6);
    state.localOwner = 7;
    assert.equal(box.value, 7);
    box.value = -2;
    assert.equal(state.localOwner, 0xfffffffe);
    state.bindLocalOwner(null);
    assert.equal(state.localOwner, 2);
    for (const bad of [5, 'x', {}, undefined]) assert.throws(() => state.bindLocalOwner(bad), TypeError);
    // 현재 획득 종류/다리 TYPE 은 장기 getter 가 있으면 그 값이다(부호 없음).
    state.sharedNuggetType = () => 200;
    state.sharedBridgeType = () => -1;
    assert.deepEqual([state.currentNuggetType, state.currentBridgeType], [200, 0xffffffff]);
    // 진단(checking) 값은 비영일 때만 참이다.
    for (const [value, expected] of [[0, false], [1, true], [true, true], [false, false], [null, false], [undefined, false], [() => 0, false], [() => 2, true], [() => 0x80000000, true]]) {
        state.debug = value;
        assert.equal(state.currentDebug, expected, String(value));
    }
    // 독립 획득 플래그와 Player 번호 범위(0..8)이다.
    state.setGrantFlag(8, 0x1ff);
    assert.equal(state.grantFlag(8), 0xff);
    state.setGrantFlag(0, 5);
    assert.equal(state.grantFlag(0), 5);
    for (const bad of [9, -1, 1.5, '1']) {
        assert.throws(() => state.grantFlag(bad), RangeError);
        assert.throws(() => state.setGrantFlag(bad, 1), RangeError);
        assert.throws(() => state.playerResultSlot(bad), RangeError);
    }
    // 독립 희생 결과 DWORD 는 owner 별 네 바이트이고 little endian 으로 쓴다.
    state.setPlayerResult(3, 0x01020304);
    assert.deepEqual([...state.playerResultSlot(3)], [4, 3, 2, 1]);
    state.setPlayerResult(8, 0xfffffffe);
    state.setPlayerResult(0, 7);
    assert.deepEqual(state.playerResults, [7, 0, 0, 0x01020304, 0, 0, 0, 0, 0xfffffffe]);
    assert.equal(state.playerResultSlot(2).length, 4);
    // 실제 Player 기록(9칸 × 0xb4)을 묶으면 +0xa0 결과와 +0x71 획득 플래그를 그 기록에서 읽고 쓴다.
    const records = new Uint8Array(9 * 0xb4);
    state.bindPlayerRecords(records);
    state.setGrantFlag(2, 9);
    assert.equal(records[2 * 0xb4 + 0x71], 9);
    assert.equal(state.grantFlag(2), 9);
    records[5 * 0xb4 + 0x71] = 0x42;
    assert.equal(state.grantFlag(5), 0x42);
    state.setGrantFlag(5, 0x1ff);
    assert.equal(records[5 * 0xb4 + 0x71], 0xff);
    const slot = state.playerResultSlot(4);
    assert.equal(slot.length, 4);
    slot[0] = 0xaa;
    assert.equal(records[4 * 0xb4 + 0xa0], 0xaa);
    state.setPlayerResult(1, 0x11223344);
    assert.deepEqual([...records.subarray(0xb4 + 0xa0, 0xb4 + 0xa4)], [0x44, 0x33, 0x22, 0x11]);
    assert.deepEqual(state.playerResults.slice(0, 2), [0, 0x11223344]);
    // 크기가 맞지 않는 기록은 거부하고 null 이면 독립 저장소로 돌아간다.
    for (const size of [0, 9 * 0xb4 - 1, 9 * 0xb4 + 1, 9 * 0xac]) assert.throws(() => state.bindPlayerRecords(new Uint8Array(size)), RangeError);
    state.bindPlayerRecords(null);
    assert.deepEqual(state.playerResults, [7, 0, 0, 0x01020304, 0, 0, 0, 0, 0xfffffffe]);
});

test('runRuneRegular: 304 만 검색 → 진행 → 룬 삭제(0x48) 순서로 실행하고 나머지는 payload 를 그대로 돌려준다', () => {
    const log = [];
    const pool = { capacity: 100 };
    const make = (dais) => ({ findDais: (rune) => { log.push(`find:${rune}`); return dais; }, progress: (found) => { log.push(`progress:${found}`); }, destroy: (rune, flags) => { log.push(`destroy:${rune},${flags}`); } });
    // 다른 사건은 훅을 하나도 부르지 않는다(훅이 없어도 된다).
    assert.equal(runRuneRegular(pool, 9, 0x305, 2.5, {}), 2.5);
    assert.equal(runRuneRegular(pool, 9, 0x303, -7, {}), -7);
    assert.equal(runRuneRegular(pool, 9, 0x304, 2.5, make(5)), -1);
    assert.deepEqual(log.splice(0), ['find:9', 'progress:5', 'destroy:9,72']);
    // 다이스를 못 찾았거나 풀 범위 밖이면 진행하지 않고 룬만 지운다.
    assert.equal(runRuneRegular(pool, 9, 0x304, 0, make(0)), -1);
    assert.deepEqual(log.splice(0), ['find:9', 'destroy:9,72']);
    assert.equal(runRuneRegular(pool, 9, 0x304, 0, make(100)), -1);
    assert.deepEqual(log.splice(0), ['find:9', 'destroy:9,72']);
    assert.equal(runRuneRegular(pool, 9, 0x304, 0, make(99)), -1);
    assert.deepEqual(log.splice(0), ['find:9', 'progress:99', 'destroy:9,72']);
    // 선택된 훅이 없으면 실행 가능한 빈 효과로 바꾸지 않고 거부한다.
    assert.throws(() => runRuneRegular(pool, 9, 0x304, 0, {}), /의식 정기의 선택된 실제 효과 미연결/);
    assert.throws(() => runRuneRegular(pool, 9, 0x304, 0, { findDais: () => 5, destroy: () => {} }), /미연결/);
    assert.throws(() => runRuneRegular(pool, 9, 0x304, 0, { findDais: () => 0, progress: () => {} }), /미연결/);
});

test('runDaisHandle: 305 는 다음 프레임 단계 → 현재 단계 → 프레임 지정 순서이고 같은 단계면 0.1 아니면 0 이다', () => {
    const pool = { slot: (sid) => { const raw = new Uint8Array(128); put(raw, 36, sid === 1 ? 0x7fffffff : sid === 2 ? 0xffffffff : 10, 4); return raw; } };
    const log = [];
    const hooks = (stages) => ({
        stage: (dais, frame) => { log.push(`stage:${frame}`); return stages[frame]; },
        frame: (dais, frame, flags) => { log.push(`frame:${frame},${flags}`); },
        sacrificeAdvance: (dais) => { log.push(`advance:${dais}`); return 0.5; },
        ritual: (dais) => { log.push(`ritual:${dais}`); },
        damageable: (dais, event, count, payload) => { log.push(`dmg:${event},${count},${payload}`); return 9; },
    });
    assert.equal(runDaisHandle(pool, 3, 0x305, 0, 1, hooks({ 10: 1, 11: 1 })), Math.fround(0.1));
    assert.deepEqual(log.splice(0), ['stage:11', 'stage:10', 'frame:11,0']);
    assert.equal(runDaisHandle(pool, 3, 0x305, 0, 1, hooks({ 10: 1, 11: 2 })), 0);
    log.length = 0;
    // 프레임 DWORD 는 부호 있고 +1 은 int32 로 감긴다.
    assert.equal(runDaisHandle(pool, 1, 0x305, 0, 1, hooks({ 2147483647: 0, [-2147483648]: 0 })), Math.fround(0.1));
    assert.deepEqual(log.splice(0), ['stage:-2147483648', 'stage:2147483647', 'frame:-2147483648,0']);
    assert.equal(runDaisHandle(pool, 2, 0x305, 0, 1, hooks({ [-1]: 0, 0: 0 })), Math.fround(0.1));
    assert.deepEqual(log.splice(0), ['stage:0', 'stage:-1', 'frame:0,0']);
    assert.equal(runDaisHandle(pool, 3, 0x306, 0, 1, hooks({})), 0.5);
    assert.deepEqual(log.splice(0), ['advance:3']);
    // 307 은 count 가 0 이 아닐 때만 의식을 실행하고 0 을 돌려준다.
    assert.equal(runDaisHandle(pool, 3, 0x307, 2, 1.5, hooks({})), 0);
    assert.deepEqual(log.splice(0), ['ritual:3']);
    assert.equal(runDaisHandle(pool, 3, 0x307, 0, 1.5, hooks({})), 9);
    assert.deepEqual(log.splice(0), ['dmg:775,0,1.5']);
    assert.equal(runDaisHandle(pool, 3, 0x308, 1, 1.25, hooks({})), 9);
    assert.equal(runDaisHandle(pool, 3, 0x304, 1, 1.25, hooks({})), 9);
    assert.deepEqual(log.splice(0), ['dmg:776,1,1.25', 'dmg:772,1,1.25']);
    assert.throws(() => runDaisHandle(pool, 3, 0x308, 1, 1, {}), /미연결/);
    assert.throws(() => runDaisHandle(pool, 3, 0x305, 1, 1, { stage: () => 0 }), /미연결/);
});

test('RawDaisRegular.handle: 실제 룬/다이스 가상표만 받고 0x305~0x307 을 같은 몸체로 분배한다', () => {
    const w = standardWorld({ daisFrame: 20 });
    // 룬/다이스 가상표가 아니면 거부한다.
    assert.equal(w.dais.handles(5), true);
    assert.equal(w.dais.handles(6), false);
    assert.throws(() => w.dais.handle(6, 0x305, 0, 1), TypeError);
    put(w.slots[9], 0, RUNE_VTABLE);
    assert.equal(w.dais.handles(9), true);
    put(w.slots[9], 0, RUNE_VTABLE + 1);
    assert.equal(w.dais.handles(9), false);
    put(w.slots[9], 0, PATCH_DAIS_VTABLE + 1);
    assert.equal(w.dais.handles(9), false);
    // 룬 가상표의 304 이외 사건은 payload 를 그대로 돌려주고 아무 훅도 부르지 않는다.
    put(w.slots[9], 0, RUNE_VTABLE);
    assert.equal(w.dais.handle(9, 0x305, 0, 4.5), 4.5);
    assert.deepEqual(w.log, []);
    // 다이스 307: count 0 이면 나머지 Damageable 로, 0 이 아니면 의식(후속)으로 가서 Damageable 은 부르지 않는다.
    assert.equal(w.dais.handle(5, 0x307, 0, 6.5), 3.5);
    assert.deepEqual(w.log.splice(0), ['DMG:5,775,0,6.5']);
    assert.equal(w.dais.handle(5, 0x307, 1, 6.5), 0);
    assert.ok(!w.log.some((entry) => entry.startsWith('DMG')));
    w.log.length = 0;
    assert.equal(w.dais.handle(5, 0x400, 2, 1.5), 3.5);
    assert.deepEqual(w.log.splice(0), ['DMG:5,1024,2,1.5']);
    // 306 은 희생 진행(코드표 169)의 반환 지연이다(프레임 20 은 B 구간 안이라 다음 프레임 21 과 0.12).
    assert.equal(w.dais.handle(5, 0x306, 0, 1), Math.fround(0.12));
    assert.deepEqual(w.log.splice(0), ['F:5,21,0']);
    // 훅 누락은 도달한 위치에서만 거부한다.
    const bare = makeWorld({}, { destroy: undefined, damageable: undefined });
    bare.insert(5, 165);
    put(bare.slots[5], 0, PATCH_DAIS_VTABLE);
    assert.throws(() => bare.dais.handle(5, 0x400, 1, 1), /다이스의 선택된 실제 장치 미연결/);
    put(bare.slots[9], 0, RUNE_VTABLE);
    assert.equal(bare.dais.handle(9, 0x1, 0, 1), 1);
});

// 코어 자산에서 사제(158) 방향 코드만 바꾼 패턴 목록이다. (2026-11-13 WB04-E367)
const withPriestCodes = (codes) => corePatterns('A').map(([type, pattern]) => (type === 158 ? [158, codes] : [type, pattern]));

// 제단/포획 Form 이 달린 다이스에 내용물 Form 하나를 만든다(head 연결은 호출자가 한다). (2026-11-13 WB04-E367)
function giveForm(w, sid, kind, flags, next = 0, original = 0) {
    const raw = w.slots[sid];
    put(raw, 0, VTABLE);
    raw[10] = 6;
    put(raw, 4, next, 2);
    put(raw, 12, original, 2);
    put(raw, 18, kind, 2);
    put(raw, 20, flags, 2);
}

test('RawDaisRegular.stopPriestRitual: 방향 10..14 에서만 첫 604 한 개를 지우고 현재 타입 E 첫 프레임을 지정한다', () => {
    // 프레임 0..5 의 사제 방향 글자: I(8) J(9) K(10) O(14) P(15) E(4).
    const patterns = withPriestCodes(`IJKOPE${'A'.repeat(26)}`);
    for (const [frame, acts] of [[0, false], [1, false], [2, true], [3, true], [4, false], [5, false]]) {
        const w = standardWorld({}, patterns);
        put(w.slots[6], 36, frame, 4);
        assert.ok(w.host.attach(new RegularProcess(604, 0.01), 61, 6, 0x10));
        assert.ok(w.host.attach(new RegularProcess(604, 0.02), 61, 6, 0x10));
        w.dais.stopPriestRitual(6);
        assert.deepEqual(w.log, acts ? ['F:6,5,0'] : [], `프레임 ${frame}`);
        // 두 번 붙인 604 중 첫 번째만 지워지고 두 번째는 남는다.
        assert.ok(w.host.findEvent(6, 604), `프레임 ${frame} 남은 604`);
    }
    // 604 가 없어도 프레임은 지정한다.
    const none = standardWorld({}, patterns);
    put(none.slots[6], 36, 2, 4);
    none.dais.stopPriestRitual(6);
    assert.deepEqual(none.log, ['F:6,5,0']);
    // 정적 본문은 같은 풀의 Host 와 프레임 지정만 쓴다. 지정 훅이 없으면 거부한다.
    assert.throws(() => RawDaisRegular.stopPriestRitualIn(none.pool, none.frames, none.host, 6, undefined), /미연결/);
    RawDaisRegular.stopPriestRitualIn(none.pool, none.frames, none.host, 6, (sid, value, flags) => none.log.push(`S:${sid},${value},${flags}`));
    assert.equal(none.log.at(-1), 'S:6,5,0');
    // TYPE 이 표 밖(188)이면 방향 조회 전에 거부하고, 604 삭제 중 TYPE 이 바뀌어도 E 조회 전에 다시 확인한다.
    const bad = standardWorld({}, patterns);
    put(bad.slots[6], 36, 2, 4);
    bad.slots[6][10] = 188;
    assert.throws(() => bad.dais.stopPriestRitual(6), (error) => error instanceof RangeError && /방향 TYPE/.test(error.message));
    const swap = standardWorld({}, patterns);
    put(swap.slots[6], 36, 2, 4);
    class Corrupting extends RegularProcess {
        // 삭제될 때 사제 TYPE 을 표 밖으로 바꾼다.
        destroy() { swap.pool.physical(6)[10] = 188; }
    }
    assert.ok(swap.host.attach(new Corrupting(604, 0.01), 61, 6, 0x10));
    assert.throws(() => swap.dais.stopPriestRitual(6), (error) => error instanceof RangeError && /frame TYPE/.test(error.message));
});

test('RawDaisRegular.collapse·startSacrifice: 예약 성공 여부와 무관하게 위치 음향/프레임을 내고 count 별로 다이스 프레임을 정한다', () => {
    const delay = floatToBits(Math.fround(0.01));
    // collapse: 새 공유 305 예약 뒤 현재 단계 목표점에서 음향을 낸다(예약이 실패해도 음향은 낸다).
    for (const reserve of [true, false]) {
        let played = null;
        const w = standardWorld({ reserve, hooks: { playAt: (x, y, name, flags) => { played = [x, y, name, flags]; w.log.push('A'); } } });
        w.dais.collapse(5);
        assert.deepEqual(w.log, reserve ? ['N', `S:5,773,${delay}`, 'A'] : ['N', 'A']);
        const point = w.ritual.stagePoint(5, w.ritual.stage(5));
        assert.deepEqual(played, [point.x, point.y, 'altarBurnCollapse.wav', 0]);
    }
    // startSacrifice: extra 비트 2 → 제단 M 프레임 → count 1/2 만 다이스 프레임 → 공유 306 예약.
    for (const [count, frames] of [[0, []], [1, ['F:5,0,0']], [2, ['F:5,45,0']], [3, []], [4, []]]) {
        const w = standardWorld({ hooks: { count: () => count } });
        w.slots[5][40] = 0x41;
        w.dais.startSacrifice(5);
        assert.equal(w.slots[5][40], 0x43);
        assert.deepEqual(w.log, ['F:7,2,0', ...frames, 'N', `S:5,774,${delay}`], `count ${count}`);
    }
    // 예약 실패는 306 부착만 건너뛴다. 제단이 없으면 M 프레임도 건너뛴다.
    const failed = standardWorld({ reserve: false, hooks: { count: () => 1 } });
    failed.dais.startSacrifice(5);
    assert.deepEqual(failed.log, ['F:7,2,0', 'F:5,0,0', 'N']);
    const noAltar = standardWorld({ hooks: { count: () => 2 } });
    noAltar.hash.setBucket(1, 40.75, 41.25, 6);
    noAltar.slots[6][4] = 0;
    noAltar.slots[6][5] = 0;
    noAltar.dais.startSacrifice(5);
    assert.deepEqual(noAltar.log, ['F:5,45,0', 'N', `S:5,774,${delay}`]);
});

test('RawDaisRegular.sacrificeAdvance: 전역 169 코드 구간에서 다음 프레임/0.12 또는 종료 0 이고 범위 밖 코드는 거부한다', () => {
    // 169 코드표: A(0) B(1) C(2,3) D(4..6) E(7..179). 구간이 한 칸인 글자는 같은 프레임에 머문다.
    const patterns = corePatterns('A').map(([type, pattern]) => (type === 169 ? [169, `ABCCDDD${'E'.repeat(173)}`] : [type, pattern]));
    const delay = Math.fround(0.12);
    for (const [frame, expected] of [[0, 0], [1, 1], [2, 3], [4, 5], [5, 6], [7, 8], [178, 179]]) {
        const w = standardWorld({}, patterns);
        put(w.slots[5], 36, frame, 4);
        w.slots[5][40] = 0xff;
        assert.equal(w.dais.sacrificeAdvance(5), delay, `프레임 ${frame}`);
        assert.deepEqual(w.log, [`F:5,${expected},0`], `프레임 ${frame}`);
        assert.equal(w.slots[5][40], 0xff);
    }
    // 구간의 끝에서 처음으로 돌아가면 종료 0 이고 희생 중 비트만 지우며 종료 본문(cue 정지 → 제단 P)을 부른다.
    for (const frame of [3, 6, 179]) {
        const w = standardWorld({}, patterns);
        put(w.slots[5], 36, frame, 4);
        w.slots[5][40] = 0xff;
        w.slots[5][33] = 1;
        w.state.voiceCue = 5;
        assert.equal(w.dais.sacrificeAdvance(5), 0, `프레임 ${frame}`);
        assert.equal(w.slots[5][40], 0xfd);
        assert.deepEqual(w.log.slice(0, 2), ['V:5,0,0', 'F:7,4,0']);
    }
    // 코드표 밖/음수 프레임과 범위 밖 희생 TYPE 은 효과 없이 거부한다.
    for (const frame of [-1, 180, 4000]) {
        const w = standardWorld({}, patterns);
        put(w.slots[5], 36, frame >>> 0, 4);
        assert.throws(() => w.dais.sacrificeAdvance(5), (error) => error instanceof RangeError && /희생 코드 범위/.test(error.message), `프레임 ${frame}`);
        assert.deepEqual(w.log, []);
    }
    const typed = standardWorld({}, patterns);
    typed.state.sacrificeType = 188;
    assert.throws(() => typed.dais.sacrificeAdvance(5), RangeError);
    typed.state.sacrificeType = 187;
    assert.throws(() => typed.dais.sacrificeAdvance(5), RangeError);
});

test('RawDaisRegular.altarEscape: 제단 위치의 다이스 조회는 현재 TYPE 의 genus 로 표면 단계를 정하고 제단 L 프레임과 24 예약을 낸다', () => {
    const delay = floatToBits(Math.fround(0.12));
    for (const reserve of [true, false]) {
        const w = standardWorld({ reserve });
        w.dais.altarEscape(7);
        assert.deepEqual(w.log, reserve ? ['F:7,3,0', 'N', `S:7,24,${delay}`] : ['F:7,3,0', 'N']);
    }
    // 조회는 결과를 버리지만 먼저 읽는다: 다이스 TYPE 이 표 밖이면 거부한다.
    const range = standardWorld();
    range.runeState.daisType = 188;
    assert.throws(() => range.dais.altarEscape(7), (error) => error instanceof RangeError && /타입 범위/.test(error.message));
    assert.deepEqual(range.log, []);
    // 표면 단계(level 0)를 훑을지는 다이스 TYPE flags2 의 genus 비트 6 이 정한다. level 0 에 놓은 순환 사슬을 만나면 finder 가 거부하므로 훑었는지 알 수 있다.
    for (const [flags2, scansSurface] of [[0, false], [1, false], [2, true], [4, true], [6, true], [8, false]]) {
        const w = standardWorld();
        w.types[165].flags2 = flags2;
        w.slots[11][10] = 1;
        put(w.slots[11], 4, 11, 2);
        w.hash.setCell(0, 40, 41, 11);
        if (scansSurface) assert.throws(() => w.dais.altarEscape(7), /종속 체인 손상|체인/, `flags2 ${flags2}`);
        else w.dais.altarEscape(7);
    }
    // 첫 다이스 TYPE 후보를 만나면 거기서 멈추고 뒤의 순환 사슬은 보지 않는다. 다른 TYPE 이면 계속 가서 순환을 만난다.
    const hit = standardWorld();
    put(hit.slots[11], 4, 11, 2);
    hit.slots[11][10] = 1;
    put(hit.slots[5], 4, 11, 2);
    hit.hash.setBucket(1, 40.75, 41.25, 5);
    hit.dais.altarEscape(7);
    const miss = standardWorld();
    put(miss.slots[11], 4, 11, 2);
    miss.slots[11][10] = 1;
    put(miss.slots[6], 4, 11, 2);
    miss.hash.setBucket(1, 40.75, 41.25, 6);
    assert.throws(() => miss.dais.altarEscape(7), /체인/);
});

// 의식 대역이다: 단계·추상 룬·강한 사제 조회만 장면 입력으로 바꾸고 나머지는 실제 의식에 맡긴다. calls 는 (단계 조회 횟수)를 센다. (2026-11-13 WB04-E367)
function stubRitual(real, control) {
    return {
        pool: real.pool,
        count: (sid) => real.count(sid),
        stage: (dais, frame) => {
            control.stageCalls = (control.stageCalls ?? 0) + 1;
            // stageSeq 가 있으면 호출 순서대로 단계를 돌려준다(마지막 값을 반복).
            if (control.stageSeq) return control.stageSeq[Math.min(control.stageCalls - 1, control.stageSeq.length - 1)];
            return control.stage !== undefined ? control.stage : real.stage(dais, frame);
        },
        hasAbstractRune: (dais) => { control.abstractCalls = (control.abstractCalls ?? 0) + 1; return control.abstract !== undefined ? control.abstract : real.hasAbstractRune(dais); },
        stagePoint: (dais, stage) => real.stagePoint(dais, stage),
        findStrongOwnedPriest: (dais) => (control.priest !== undefined ? control.priest : real.findStrongOwnedPriest(dais)),
        ritual: (dais, priest, mode) => { control.rituals?.push([dais, priest, mode]); return real.ritual(dais, priest, mode); },
    };
}

test('RawDaisRegular.canBridge: 4단계 추상 룬·제단 방향·사제 genus/owner·포획 Form 조건을 원문 순서로 판정한다', () => {
    // 단계 4 에 추상 룬이 있으면 제단 조회 전에 거부하고, 다른 단계/추상 룬 없음은 계속한다.
    for (const [stage, abstract, expected] of [[4, true, false], [4, false, true], [3, true, true], [5, true, true]]) {
        const control = { stage, abstract };
        const scene = parentScene({ ritual: (real) => stubRitual(real, control) });
        assert.equal(scene.dais.canBridge(5, 6), expected, `단계 ${stage} 추상 ${abstract}`);
        assert.equal(control.abstractCalls ?? 0, stage === 4 ? 1 : 0);
    }
    // 제단이 없으면 거부한다(해시 머리에서 제단을 뺀다).
    const noAltar = parentScene();
    noAltar.hash.setBucket(1, 40.75, 41.25, 0);
    assert.equal(noAltar.dais.canBridge(5, 6), false);
    assert.equal(noAltar.allianceReads, 0);
    // 제단 방향 글자: I(8) M(12) L(11) K(10) N(13) 중 M·L 만 거부한다. 제단 TYPE 이 표 밖이면 거부한다.
    const dirs = parentScene();
    dirs.frames[164] = new FrameCodeTable([0x49, 0x4d, 0x4c, 0x4b, 0x4e].map((side) => ({ side, variant: 0x50, number: 1, flags: 0 })));
    for (const [frame, expected] of [[0, true], [1, false], [2, false], [3, true], [4, true]]) {
        put(dirs.raw(7), 36, frame, 4);
        assert.equal(dirs.dais.canBridge(5, 6), expected, `제단 프레임 ${frame}`);
    }
    // 현재 제단 TYPE 번호가 표 밖이면 제단 조회에서 거부한다.
    dirs.runeState.altarType = 188;
    assert.throws(() => dirs.dais.canBridge(5, 6), RangeError);
    // 사제 genus 비트: 없으면 진단(checking) 없이 거부하고 checking 이 켜졌으면 진단 후 거부한다. 장치가 없으면 거부 대신 오류이다.
    for (const flags2 of [0, 1, 0x100000, 0x400000]) {
        const quiet = parentScene();
        quiet.types[158].flags2 = flags2;
        quiet.checking = 0;
        assert.equal(quiet.dais.canBridge(5, 6), false, `genus ${flags2}`);
        assert.deepEqual(quiet.diagnostics, []);
        const loud = parentScene();
        loud.types[158].flags2 = flags2;
        loud.onAssertion = () => {};
        assert.equal(loud.dais.canBridge(5, 6), false);
        assert.deepEqual(loud.diagnostics, [0x140]);
    }
    const noDevice = parentScene({ hooks: { assertion: undefined } });
    noDevice.types[158].flags2 = 0;
    assert.throws(() => noDevice.dais.canBridge(5, 6), /원문 진단 장치 미연결/);
    // 사제 TYPE 이 표 밖이면 genus 조회에서 거부한다.
    const badPriest = parentScene();
    badPriest.raw(6)[10] = 188;
    assert.throws(() => badPriest.dais.canBridge(5, 6), (error) => error instanceof RangeError && /사제 TYPE/.test(error.message));
    // 사제/다이스 owner 가 다르면 거부한다.
    const owners = parentScene();
    owners.raw(6)[34] = 2;
    assert.equal(owners.dais.canBridge(5, 6), false);
    assert.equal(owners.allianceReads, 0);
    // 포획 Form: 0x20 비트와 현재 사제 종류가 모두 맞아야 하고, 틀린 후보는 건너뛰어 다음 후보를 본다. 원래 owner 는 하위 7비트이다.
    const chain = parentScene();
    put(chain.raw(10), 4, 11, 2);
    put(chain.raw(10), 18, 99, 2);
    chain.raw(11)[10] = 6;
    put(chain.raw(11), 18, 158, 2);
    put(chain.raw(11), 20, 0x20, 2);
    chain.raw(11)[12] = 0x83;
    assert.equal(chain.dais.canBridge(5, 6), true);
    assert.deepEqual(chain.allianceArguments, [[1, 3]]);
    // 0x20 이 없는 후보(다른 비트만)는 사제 종류가 같아도 후보가 아니다.
    for (const flags of [0, 1, 0x10, 0x21 & ~0x20, 0x40]) {
        const w = parentScene();
        put(w.raw(10), 20, flags, 2);
        assert.equal(w.dais.canBridge(5, 6), false, `flags ${flags}`);
        assert.equal(w.allianceReads, 0);
    }
    // 현재 사제 종류가 후보 종류와 다르면 후보가 아니다.
    const kind = parentScene();
    kind.canonicalPriestType = 157;
    assert.equal(kind.dais.canBridge(5, 6), false);
    // 원래 owner 가 8 이면 행/열 범위 안이고 9 이상이면 동맹 조회에서 거부한다.
    const edge = parentScene();
    edge.raw(10)[12] = 0x88;
    assert.equal(edge.dais.canBridge(5, 6), true);
    assert.deepEqual(edge.allianceArguments, [[1, 8]]);
    const over = parentScene();
    over.raw(10)[12] = 0x89;
    assert.throws(() => over.dais.canBridge(5, 6), /플레이어 범위/);
    // 동맹이 0 이 아니면 거부하고 포획 Form 이 없으면 거부한다.
    const allied = parentScene();
    allied.currentAlliance = -1;
    assert.equal(allied.dais.canBridge(5, 6), false);
    const empty = parentScene();
    put(empty.raw(5), 6, 0, 2);
    assert.equal(empty.dais.canBridge(5, 6), false);
});

test('RawDaisRegular.moveAwayDepth: 현재 첫 경로 Form 의 깊이를 읽고 진단 비교는 저장 깊이, 경로 수명은 진단 뒤에도 확인한다', () => {
    const seen = [];
    const scene = parentScene({ hooks: { processType: (type) => { seen.push(type); return 54; } } });
    // 경로 Form 이 없으면 0 이고 사제 TYPE 으로 프로세스 종류를 정한다.
    assert.equal(scene.dais.moveAwayDepth(6), 0);
    assert.deepEqual(seen, [158]);
    const process = RawPathProcess.attach(scene.host, scene.emptyPath(true), 6, 0x10);
    assert.ok(process);
    // 진단이 꺼져 있으면 음수/큰 깊이도 그대로 돌려주고 진단하지 않는다.
    scene.checking = 0;
    for (const depth of [-1, 0, 3, 4, 9]) {
        process.runtime.setMoveAwayDepth(depth);
        assert.equal(scene.dais.moveAwayDepth(6), depth);
    }
    assert.deepEqual(scene.diagnostics, []);
    // 진단이 켜져 있으면 깊이 3 까지는 조용하고 4 이상은 0x87c, 음수는 0x87b 만 낸다.
    scene.checking = 1;
    scene.onAssertion = () => {};
    for (const [depth, lines] of [[0, []], [3, []], [4, [0x87c]], [9, [0x87c]], [-1, [0x87b]], [-9, [0x87b]]]) {
        scene.diagnostics.length = 0;
        process.runtime.setMoveAwayDepth(depth);
        assert.equal(scene.dais.moveAwayDepth(6), depth);
        assert.deepEqual(scene.diagnostics, lines, `깊이 ${depth}`);
    }
    // 진단 중 경로가 삭제되면 진단 뒤 현재 깊이를 읽기 전에 거부한다.
    process.runtime.setMoveAwayDepth(-1);
    scene.onAssertion = () => { scene.host.kill(process, 0); };
    assert.throws(() => scene.dais.moveAwayDepth(6), /경로 수명 종료/);
    // 경로 Form 이 있는데 실제 경로 프로세스가 아니면 거부한다.
    const other = parentScene();
    assert.ok(other.host.attach(new RegularProcess(1, 1), 54, 6, 0x10));
    assert.throws(() => other.dais.moveAwayDepth(6), /Path form 실제 프로세스 누락/);
});

// 경로 시작 요청을 기록하는 대역을 돌려준다. result 로 반환할 가짜 경로를 정한다. (2026-11-13 WB04-E367)
function spyPath() {
    const spy = { requests: [], result: null };
    spy.make = (scene, pool) => ({ pool, start: (request) => { spy.requests.push(request); return spy.result; } });
    // 가짜 경로 프로세스: 마지막 점 교체·Prepare·깊이 기록을 남긴다.
    spy.fake = (count = 3) => {
        const log = [];
        return {
            log,
            runtime: { count, replacePoint: (index, point) => { log.push(`replace:${index},${floatToBits(point.x)},${floatToBits(point.y)}`); }, setMoveAwayDepth: (depth) => { log.push(`depth:${depth}`); } },
            prepare: (override, speed, turning) => { log.push(`prepare:${override},${speed},${turning}`); },
        };
    };
    return spy;
}

test('RawDaisRegular.bridgeCommand: 강한 사제·자격·단계·권한·깊이를 순서대로 확인하고 PathStart 요청과 마지막 점/Prepare/깊이를 같은 순서로 지정한다', () => {
    const metadata = () => ({ speed: Math.fround(0.7), turningSpeed: 90 });
    const make = (control = {}, spy = spyPath()) => {
        const scene = parentScene({ path: spy.make, ritual: (real) => stubRitual(real, control), hooks: { pathMetadata: metadata } });
        scene.checking = 0;
        return { scene, spy };
    };
    // 단계 0 이고 currentStage 0 이면 다음 단계(1) 목표로, currentStage 가 0 이 아니면 현재 단계 목표로 시작한다.
    for (const [current, tailByte] of [[0, 1], [1, 0], [7, 0]]) {
        const control = { stage: 0 };
        const { scene, spy } = make(control);
        const process = spy.fake();
        spy.result = process;
        scene.dais.bridgeCommand(5, 6, current);
        const target = scene.ritual.stagePoint(5, tailByte);
        assert.deepEqual(spy.requests, [{ parent: 6, destination: { x: target.x, y: target.y }, replaceStart: 1, helper: 0, tailWord: 5, tailByte, reserved: 0 }], `현재 ${current}`);
        assert.deepEqual(process.log, [`replace:2,${floatToBits(target.x)},${floatToBits(target.y)}`, `prepare:0,${Math.fround(0.7)},90`, 'depth:1']);
    }
    // 시작 점 개수가 달라도 마지막 꼭짓점만 교체한다.
    const one = make({ stage: 0 });
    const single = one.spy.fake(1);
    one.spy.result = single;
    one.scene.dais.bridgeCommand(5, 6, 1);
    assert.ok(single.log[0].startsWith('replace:0,'));
    // 단계 4 는 currentStage 0 이면 목표 5(미초기화 좌표)라 거부하고, currentStage 가 있으면 단계 4 목표로 시작한다. 단계 5 이상은 아무 것도 하지 않는다.
    const four = make({ stage: 4, abstract: false });
    assert.throws(() => four.scene.dais.bridgeCommand(5, 6, 0), /손상 목표5/);
    four.spy.result = four.spy.fake();
    four.scene.dais.bridgeCommand(5, 6, 1);
    assert.equal(four.spy.requests.length, 1);
    assert.equal(four.spy.requests[0].tailByte, 4);
    const five = make({ stage: 5 });
    five.scene.dais.bridgeCommand(5, 6, 0);
    five.scene.dais.bridgeCommand(5, 6, 1);
    assert.deepEqual(five.spy.requests, []);
    // 명시 사제가 0 이면 현재 강한 사제를 찾고, 없으면 아무 것도 하지 않는다. 명시 사제가 있으면 찾지 않는다.
    for (const [priest, found, expectedRequests] of [[0, 0, 0], [0, 6, 1], [6, 0, 1]]) {
        const control = { stage: 0, priest: found };
        const { scene, spy } = make(control);
        spy.result = spy.fake();
        scene.dais.bridgeCommand(5, priest, 1);
        assert.equal(spy.requests.length, expectedRequests, `명시 ${priest} 강한 ${found}`);
    }
    // 자격(owner 불일치)이 안 되면 단계를 읽기 전에 돌아간다.
    const owners = make({ stage: 0 });
    owners.scene.raw(6)[34] = 2;
    owners.scene.dais.bridgeCommand(5, 6, 1);
    assert.deepEqual(owners.spy.requests, []);
    // 사제가 이미 목표 칸에 있으면 시작하지 않고 의식 mode 0 을 호출한다. 권한이 없으면 목표가 달라도 시작하지 않는다.
    const rituals = [];
    const same = make({ stage: 0, rituals });
    const target = same.scene.ritual.stagePoint(5, 0);
    // 목표와 같은 칸(절삭 정수)이지만 좌표는 정확히 같지 않게 둔다(정확히 같으면 실제 의식 본문이 사제 동작 훅까지 간다).
    const sameCell = (value) => (value === Math.trunc(value) + 0.5 ? Math.trunc(value) + 0.25 : Math.trunc(value) + 0.5);
    put(same.scene.raw(6), 14, floatToBits(sameCell(target.x)), 4);
    put(same.scene.raw(6), 18, floatToBits(sameCell(target.y)), 4);
    same.scene.dais.bridgeCommand(5, 6, 1);
    assert.deepEqual(rituals, [[5, 6, 0]]);
    assert.deepEqual(same.spy.requests, []);
    const noBoss = make({ stage: 0 });
    noBoss.scene.currentBoss = false;
    noBoss.scene.dais.bridgeCommand(5, 6, 1);
    assert.deepEqual(noBoss.spy.requests, []);
    // 이동 깊이: 현재 깊이 + 1 이 4 이상이면 시작하지 않고, int32 감김도 원문처럼 부호 있는 값으로 비교한다.
    for (const [depth, started, written] of [[0, true, 1], [2, true, 3], [3, false, null], [10, false, null], [-1, true, 0], [-2147483648, true, -2147483647], [2147483647, true, -2147483648]]) {
        const { scene, spy } = make({ stage: 0 });
        const attached = RawPathProcess.attach(scene.host, scene.emptyPath(), 6, 0x10);
        attached.runtime.setMoveAwayDepth(depth);
        const fake = spy.fake();
        spy.result = fake;
        scene.dais.bridgeCommand(5, 6, 1);
        assert.equal(spy.requests.length, started ? 1 : 0, `깊이 ${depth}`);
        if (started) assert.equal(fake.log.at(-1), `depth:${written}`);
    }
    // 시작이 실패(null)하면 진단이 켜졌을 때만 p 진단을 내고 아무 효과 없이 돌아간다.
    for (const checking of [0, 1]) {
        const { scene, spy } = make({ stage: 0 });
        scene.checking = checking;
        scene.onAssertion = () => {};
        spy.result = null;
        scene.dais.bridgeCommand(5, 6, 1);
        assert.equal(spy.requests.length, 1);
        assert.deepEqual(scene.diagnostics, checking ? [0x18e] : []);
    }
});

test('RawDaisRegular.progress: 약한 사제는 604 중지를 건너뛰고 단계 4 미만에서는 권한이 있을 때만 다리 명령 뒤 붕괴를 낸다', () => {
    const delay = floatToBits(Math.fround(0.01));
    for (const boss of [true, false]) {
        const log = [];
        const spy = spyPath();
        const control = { priest: 0 };
        const scene = parentScene({
            path: spy.make,
            ritual: (real) => stubRitual(real, control),
            hooks: {
                reserveShared: () => { log.push('N'); return true; }, addShared: (sid, event, payload) => { log.push(`S:${sid},${event},${floatToBits(payload)}`); },
                playAt: (x, y, name) => { log.push(`A:${name}`); }, pathMetadata: () => ({ speed: 1, turningSpeed: 1 }),
            },
        });
        scene.checking = 0;
        scene.currentBoss = boss;
        spy.result = null;
        // 강한 사제를 찾지 못하면(조회 결과 0) 604 중지와 다리 명령을 모두 건너뛴다(0 번 슬롯을 사제로 다루지 않는다).
        scene.dais.progress(5);
        assert.deepEqual(log, ['N', `S:5,773,${delay}`, 'A:altarBurnCollapse.wav']);
        assert.deepEqual(spy.requests, []);
        // 강한 사제를 찾으면 권한이 있을 때만 단계 0 의 다음 목표(1)로 시작하며 요청 뒤에 붕괴 예약이 온다.
        log.length = 0;
        control.priest = 6;
        scene.dais.progress(5);
        assert.deepEqual(log, ['N', `S:5,773,${delay}`, 'A:altarBurnCollapse.wav']);
        assert.equal(spy.requests.length, boss ? 1 : 0);
        if (boss) assert.deepEqual([spy.requests[0].parent, spy.requests[0].tailWord, spy.requests[0].tailByte], [6, 5, 1]);
    }
});

test('RawDaisRegular.grantCapturedType: 획득 플래그를 먼저 지우고 너겟/noIsland/기타 종류를 로컬·비로컬로 나눈다', () => {
    const nugget = 154;
    for (const [type, local, expected, selected] of [
        [nugget, true, ['SP:5000', 'CHAT:Stormpower,0,0,0'], 0], [nugget, false, ['NF:2'], 0],
        [93, true, ['LS:0'], 0], [93, false, [], 0],
        [7, true, ['LT:7'], 7], [7, false, ['GF:2,7'], 0], [0, true, ['LT:0'], 0], [155, false, ['GF:2,155'], 0],
    ]) {
        const w = standardWorld();
        w.state.grantFlags.fill(1);
        w.state.localOwner = local ? 2 : 1;
        w.dais.grantCapturedType(type, 2);
        assert.deepEqual(w.log, expected, `종류 ${type} 로컬 ${local}`);
        assert.equal(w.state.grantFlag(2), 0);
        assert.equal(w.state.grantFlag(1), 1);
        assert.equal(w.state.selectedType, selected);
    }
    // 현재 획득 종류는 장기 getter 가 있으면 그 값이고, 독립 값(154)은 더 이상 너겟이 아니다. noIsland 번호도 독립 값을 따른다.
    const shared = standardWorld();
    shared.state.sharedNuggetType = () => 200;
    shared.state.localOwner = 2;
    shared.dais.grantCapturedType(200, 2);
    shared.dais.grantCapturedType(154, 2);
    assert.deepEqual(shared.log, ['SP:5000', 'CHAT:Stormpower,0,0,0', 'LT:154']);
    const island = standardWorld();
    island.state.noIslandType = 40;
    island.state.localOwner = 2;
    island.dais.grantCapturedType(40, 2);
    island.dais.grantCapturedType(93, 2);
    assert.deepEqual(island.log, ['LS:0', 'LT:93']);
    // 플레이어 범위(0..8)를 벗어나면 효과 전에 거부한다.
    const range = standardWorld();
    range.state.grantFlags.fill(1);
    assert.throws(() => range.dais.grantCapturedType(7, 9), RangeError);
    assert.deepEqual(range.log, []);
    range.dais.grantCapturedType(7, 8);
    range.dais.grantCapturedType(7, 0);
    assert.deepEqual(range.log, ['GF:8,7', 'GF:0,7']);
});

test('RawDaisRegular.finishSacrifice: 포획 획득 분기의 UI·알림·SP 안내와 업그레이드 분기의 프레임·삭제·표면 WORD 순서가 원문과 같다', () => {
    const owners = Array.from({ length: 8 }, (_, index) => `R:${index + 1}`);
    // 기본: 제단 P 프레임 → 포획 종류 획득(로컬 너겟) → 모든 owner 갱신 → 로컬 메뉴 갱신. 원래 owner 가 로컬이 아니고 다이스 owner 가 로컬이면 채팅은 없다.
    const base = standardWorld();
    giveForm(base, 10, 154, 4);
    put(base.slots[5], 6, 10, 2);
    put(base.slots[7], 12, 2, 2);
    base.state.voiceCue = 7;
    base.state.grantFlags.fill(1);
    base.dais.finishSacrifice(5);
    assert.deepEqual(base.log, ['V:7,0,0', 'F:7,4,0', 'SP:5000', 'CHAT:Stormpower,0,0,0', ...owners, 'RL']);
    assert.equal(base.state.playerResults[1], 154);
    assert.equal(base.state.grantFlag(1), 0);
    // 비로컬 다이스 owner: 원래 owner 가 로컬이면 로컬 사제 사망 안내(상태 helper 값 포함), 둘 다 로컬이 아니면 타인 사망 안내이다.
    for (const [original, owner, expected] of [[1, 3, ['MS', 'CHAT:LocalPriestDead,1,3,7']], [2, 3, ['CHAT:OtherPriestDead,2,3,0']], [3, 3, ['CHAT:OtherPriestDead,3,3,0']]]) {
        const w = standardWorld();
        giveForm(w, 10, 154, 4);
        put(w.slots[5], 6, 10, 2);
        put(w.slots[7], 12, original, 2);
        w.slots[5][34] = owner;
        w.dais.finishSacrifice(5);
        assert.deepEqual(w.log, ['V:0,0,0', 'F:7,4,0', 'NF:3', ...owners, ...expected]);
        assert.equal(w.state.playerResults[3], 154);
    }
    // 시간 억제: 획득을 건너뛰고(결과만 기록) 로컬이면 SP 5000 과 사제 존재 확인 뒤 동맹이 아닌 죽은 상대가 있을 때만 안내한다.
    for (const [dead, alliance, expected] of [[3, 0, ['CHAT:Stormpower,0,0,0']], [3, 1, []], [3, -1, []], [0, 0, []]]) {
        const w = standardWorld({ priestExists: (owner) => owner !== dead });
        w.state.grantFlags.fill(1);
        w.runeState.suppressDuration = true;
        giveForm(w, 10, 154, 4);
        put(w.slots[5], 6, 10, 2);
        w.state.alliances[1 * 9 + 3] = alliance;
        w.dais.finishSacrifice(5);
        assert.deepEqual(w.log, ['V:0,0,0', 'F:7,4,0', ...owners, 'SP:5000', ...Array.from({ length: 8 }, (_, index) => `PE:${index + 1}`), ...expected], `죽은 ${dead} 동맹 ${alliance}`);
        assert.equal(w.state.playerResults[1], 154);
        // 시간 억제 중에는 획득 본문을 건너뛰어 획득 플래그도 그대로이다.
        assert.equal(w.state.grantFlag(1), 1);
    }
    // 시간 억제에 다이스 owner 가 로컬이 아니면 SP 안내도 없다.
    const quiet = standardWorld();
    quiet.runeState.suppressDuration = true;
    quiet.slots[5][34] = 3;
    quiet.dais.finishSacrifice(5);
    assert.deepEqual(quiet.log, ['V:0,0,0', 'F:7,4,0', ...owners]);
    // 제단이 없으면 진단 뒤 같은 순서로 계속하고(제단 0 의 P 첫 프레임 -1), 진단 장치가 없으면 cue 정지 뒤 거부한다.
    const lost = standardWorld();
    lost.hash.setBucket(1, 40.75, 41.25, 0);
    lost.dais.finishSacrifice(5);
    assert.deepEqual(lost.log.slice(0, 3), ['V:0,0,0', 'ASSERT:2ba', 'F:0,-1,0']);
    const bare = standardWorld({ hooks: { assertion: undefined } });
    bare.hash.setBucket(1, 40.75, 41.25, 0);
    assert.throws(() => bare.dais.finishSacrifice(5), /원문 진단 장치 미연결/);
    assert.deepEqual(bare.log, ['V:0,0,0']);
});

test('RawDaisRegular.finishSacrifice: 업그레이드 분기는 count 별 다음 첫 프레임·Player 결과 주소 보관·포획 삭제·제단 표면 WORD 순서를 지킨다', () => {
    // 다이스에 달린 포획 Form: 10(종류 165, mask 4)·11(종류 158, mask 0x20). 제단 원래 owner 는 2 이다.
    const prepare = (cfg = {}) => {
        const w = standardWorld(cfg);
        giveForm(w, 10, 165, 4, 11);
        giveForm(w, 11, 158, 0x20);
        put(w.slots[5], 6, 10, 2);
        put(w.slots[7], 12, 2, 2);
        w.state.bridgeType = 164;
        return w;
    };
    const owners = Array.from({ length: 8 }, (_, index) => `R:${index + 1}`);
    for (const [count, first] of [[0, 3], [1, 64], [2, 120], [3, 0]]) {
        const w = prepare({ hooks: { count: () => count } });
        w.dais.finishSacrifice(5);
        assert.deepEqual(w.log, ['V:0,0,0', 'F:7,4,0', `F:5,${first},0`, 'D:10,4194376', 'D:11,4194376', 'NOTE:7', 'CHAT:Upgrade,0,0,0', ...owners, 'RL'], `count ${count}`);
        assert.equal(read2(w.slots[7], 12), 0);
        assert.equal(w.state.playerResults[1], count);
    }
    // count 가 표 밖이면 효과 전에 거부한다.
    const big = prepare({ hooks: { count: () => 4 } });
    assert.throws(() => big.dais.finishSacrifice(5), (error) => error instanceof RangeError && /다음 첫 프레임/.test(error.message));
    assert.deepEqual(big.log, ['V:0,0,0', 'F:7,4,0']);
    // 현재 다리 TYPE 이 제단 TYPE 과 다르면 표면 알림을 부르지 않는다(WORD 는 지운다). 알림 장치가 없으면 같을 때 거부한다.
    const other = prepare({ hooks: { count: () => 1 } });
    other.state.bridgeType = 82;
    other.dais.finishSacrifice(5);
    assert.ok(!other.log.includes('NOTE:7'));
    assert.equal(read2(other.slots[7], 12), 0);
    const lazy = prepare({ hooks: { count: () => 1, notifySurface: undefined } });
    assert.throws(() => lazy.dais.finishSacrifice(5), /표면 통지 미연결/);
    // 두 번째 count 에서 Player 기록을 바꿔 끼워도 결과는 첫 count 앞에서 보관한 독립 주소에 쓴다. 사제 포획 Form 이 없으면 표면 WORD 를 지키고 알림도 없다.
    let calls = 0;
    const records = new Uint8Array(9 * 0xb4);
    const swapped = prepare({ hooks: { count: () => { calls += 1; if (calls === 2) swapped.state.bindPlayerRecords(records); return 2; } } });
    swapped.dais.finishSacrifice(5);
    swapped.state.bindPlayerRecords(null);
    assert.equal(swapped.state.playerResults[1], 2);
    assert.equal(records[0xb4 + 0xa0], 0);
    const noPriest = prepare({ hooks: { count: () => 1 } });
    put(noPriest.slots[10], 4, 0, 2);
    noPriest.dais.finishSacrifice(5);
    assert.deepEqual(noPriest.log, ['V:0,0,0', 'F:7,4,0', 'F:5,64,0', 'D:10,4194376', 'CHAT:Upgrade,0,0,0', ...owners, 'RL']);
    assert.equal(read2(noPriest.slots[7], 12), 2);
    // 다이스 owner 가 로컬이 아니면 업그레이드 안내가 없고, 시간 억제 중에는 업그레이드 분기를 건너뛴다.
    const foreign = prepare({ hooks: { count: () => 1 } });
    foreign.slots[5][34] = 3;
    foreign.dais.finishSacrifice(5);
    assert.ok(!foreign.log.includes('CHAT:Upgrade,0,0,0'));
    const suppressed = prepare({ hooks: { count: () => 1 } });
    suppressed.runeState.suppressDuration = true;
    suppressed.dais.finishSacrifice(5);
    assert.ok(!suppressed.log.some((entry) => entry.startsWith('D:')));
});

// 슬롯의 WORD 를 읽는다. (2026-11-13 WB04-E367)
function read2(raw, offset) {
    return get(raw, offset, 2);
}

test('makeDaisRegularHooks: 실제 Frame/Host/Damageable/로컬 SP 를 연결한 새 묶음을 만들고 같은 풀이 아니면 거부한다', () => {
    const w = makeWorld();
    const calls = [];
    const frame = { pool: w.pool, set: (...args) => calls.push(['frame', ...args]) };
    const host = { pool: w.pool, addSharedRegular: (...args) => calls.push(['host', ...args]) };
    const assets = { pool: w.pool, damageableHandle: (...args) => { calls.push(['assets', ...args]); return 4.5; } };
    const reward = { pool: w.pool, addSp: (...args) => calls.push(['reward', ...args]) };
    const original = { chat: () => {}, setFrame: forbid('이전 setFrame') };
    const bundle = makeDaisRegularHooks(w.pool, frame, host, assets, reward, w.state, original);
    assert.notEqual(bundle, original);
    assert.equal(bundle.chat, original.chat);
    bundle.setFrame(5, 12, 0);
    bundle.addShared(5, 773, 0.5);
    assert.equal(bundle.damageable(5, 775, 1, 2.5), 4.5);
    w.state.localOwner = 4;
    bundle.addLocalSp(5000);
    w.state.localOwner = 6;
    bundle.addLocalSp(-1);
    assert.deepEqual(calls, [['frame', 5, 12, 0], ['host', 5, 773, 0.5], ['assets', 5, 775, 1, 2.5], ['reward', 4, 5000], ['reward', 6, -1]]);
    // 훅을 주지 않아도 만들 수 있다.
    assert.equal(typeof makeDaisRegularHooks(w.pool, frame, host, assets, reward, w.state).setFrame, 'function');
    // 다른 풀의 연결이 하나라도 있으면 거부한다.
    const alien = { pool: makeWorld().pool };
    for (const [index, parts] of [[0, [alien, host, assets, reward]], [1, [frame, alien, assets, reward]], [2, [frame, host, alien, reward]], [3, [frame, host, assets, alien]]]) {
        assert.throws(() => makeDaisRegularHooks(w.pool, ...parts, w.state, {}), TypeError, `연결 ${index}`);
    }
    assert.throws(() => makeDaisRegularHooks(w.pool, null, host, assets, reward, w.state), TypeError);
});

// 룬 대역이다: 제단 조회 결과를 장면 입력으로 정하고 다이스 검색은 실제 룬에 맡긴다. (2026-11-13 WB04-E367)
function stubRune(real, control) {
    return { pool: real.pool, altarAt: () => control.altar, findOwnedDais: (rune) => real.findOwnedDais(rune) };
}

test('RawDaisRegular: 좌표 절삭은 정상 signed32 범위만 받고 비유한/범위 밖은 효과 전에 거부한다', () => {
    // 제단 이탈의 다이스 조회가 제단 좌표를 절삭한다. 허용 경계는 −2^31 포함, 2^31 제외이다.
    for (const [axis, value] of [['x', -2147483648], ['x', 2147483520], ['y', -2147483648], ['y', 2147483520], ['x', 255.5]]) {
        const w = standardWorld();
        put(w.slots[7], axis === 'x' ? 14 : 18, floatToBits(value), 4);
        w.dais.altarEscape(7);
        assert.equal(w.log[0], 'F:7,3,0', `${axis}=${value}`);
    }
    for (const [axis, value] of [['x', NaN], ['x', Infinity], ['x', -Infinity], ['x', 2147483648], ['x', -2147483904], ['y', NaN], ['y', 2147483648], ['y', -2147483904]]) {
        const w = standardWorld();
        put(w.slots[7], axis === 'x' ? 14 : 18, floatToBits(value), 4);
        assert.throws(() => w.dais.altarEscape(7), (error) => error instanceof RangeError && /정상 절삭 범위/.test(error.message), `${axis}=${value}`);
        assert.deepEqual(w.log, []);
    }
});

test('RawDaisRegular.altarEscape: 제단 현재 TYPE 이 프레임 표 밖이면 거부하고 표 안 마지막 칸은 빈 표로 −1 프레임을 지정한다', () => {
    // 제단을 해시 사슬에서 빼 두어(finder 가 표 밖 TYPE 후보를 먼저 거부하지 않게) 프레임 첫 번호 조회에서 거부하는지 본다.
    const detach = (w) => { w.hash.setBucket(1, 40.75, 41.25, 6); w.slots[6][4] = 0; w.slots[6][5] = 0; };
    for (const type of [188, 255]) {
        const w = standardWorld();
        detach(w);
        w.slots[7][10] = type;
        assert.throws(() => w.dais.altarEscape(7), (error) => error instanceof RangeError && /frame TYPE 물리 범위/.test(error.message), `TYPE ${type}`);
        assert.deepEqual(w.log, []);
    }
    const last = standardWorld();
    detach(last);
    last.slots[7][10] = 187;
    last.dais.altarEscape(7);
    assert.equal(last.log[0], 'F:7,-1,0');
});

test('RawDaisRegular.handle: 다이스 307 은 현재 강한 사제를 찾아 의식 mode 1 로 호출하고 Damageable 은 부르지 않는다', () => {
    const rituals = [];
    const control = { priest: 6, rituals };
    const scene = parentScene({ ritual: (real) => stubRitual(real, control), hooks: { damageable: (...args) => { scene.effects += 100; return 9; } } });
    assert.equal(scene.dais.handle(5, 0x307, 3, 1.5), 0);
    assert.deepEqual(rituals, [[5, 6, 1]]);
    assert.equal(scene.effects, 0);
    // count 0 이면 의식 없이 Damageable 의 반환을 그대로 돌려준다.
    assert.equal(scene.dais.handle(5, 0x307, 0, 1.5), 9);
    assert.deepEqual(rituals, [[5, 6, 1]]);
    assert.equal(scene.effects, 100);
});

test('RawDaisRegular.stopPriestRitual: 604 삭제는 클라이언트 삭제 인자 0 으로 요청한다(직접 Kill(…, 0) 과 같은 통지 인자)', () => {
    const patterns = withPriestCodes(`IJKOPE${'A'.repeat(26)}`);
    const seen = [];
    class Recording extends RegularProcess {
        // 폼 삭제 통지에서 받은 인자를 기록한다.
        onFormDestroy(flags) { seen.push(flags); super.onFormDestroy?.(flags); }
    }
    const base = standardWorld({}, patterns);
    const direct = base.host.attach(new Recording(604, 0.01), 61, 6, 0x10);
    assert.ok(direct);
    base.host.kill(direct, 0);
    assert.equal(seen.length, 1);
    const baseline = seen[0];
    seen.length = 0;
    const w = standardWorld({}, patterns);
    put(w.slots[6], 36, 2, 4);
    assert.ok(w.host.attach(new Recording(604, 0.01), 61, 6, 0x10));
    w.dais.stopPriestRitual(6);
    assert.deepEqual(seen, [baseline]);
});

test('RawDaisRegular: 룬 조회가 풀 밖 SID(풀 크기와 같은 값)를 돌려주면 제단·사제로 다루지 않는다', () => {
    const capacity = 24000;
    // startSacrifice: 풀 밖 제단은 M 프레임을 건너뛰고(조회로 읽지 않고) 나머지 순서는 그대로이다.
    const start = parentScene({ rune: (real) => stubRune(real, { altar: capacity }), hooks: { setFrame: (sid, value) => { start.log.push(`F:${sid},${value}`); }, reserveShared: () => false } });
    start.log = [];
    start.dais.startSacrifice(5);
    assert.deepEqual(start.log, ['F:5,0']);
    // canBridge: 풀 밖 제단은 거부(슬롯 접근 없이), 제단 TYPE 이 프레임 표 밖이면 방향 조회에서 거부한다.
    const out = parentScene({ rune: (real) => stubRune(real, { altar: capacity }) });
    assert.equal(out.dais.canBridge(5, 6), false);
    const bad = parentScene({ rune: (real) => stubRune(real, { altar: 7 }) });
    bad.raw(7)[10] = 188;
    assert.throws(() => bad.dais.canBridge(5, 6), (error) => error instanceof RangeError && /제단 TYPE 범위/.test(error.message));
    // progress: 풀 밖 강한 사제와 풀 밖 제단(시간 억제 중 이탈 분기)은 604 중지/제단 이탈을 건너뛴다. 제단이 없으면(0) 이탈하지 않는다.
    for (const altar of [capacity, 0]) {
        const log = [];
        const control = { priest: capacity };
        const scene = parentScene({
            ritual: (real) => stubRitual(real, control),
            rune: (real) => stubRune(real, { altar }),
            hooks: {
                playAt: (x, y, name) => { log.push(`A:${name}`); }, reserveSound: () => { log.push('NS'); return false; },
                setFrame: (sid, value) => { log.push(`F:${sid},${value}`); }, reserveShared: () => { log.push('N'); return false; },
            },
        });
        put(scene.raw(5), 36, 47, 4);
        scene.runeState.suppressDuration = true;
        scene.dais.progress(5);
        assert.deepEqual(log, ['A:itIsDone2.wav', 'NS', 'NS'], `제단 ${altar}`);
    }
});

test('RawDaisRegular.bridgeCommand: 단계 5 거부는 두 번 읽고(자격 뒤·목표 계산 뒤), 목표 X/Y 가 모두 같을 때만 즉시 의식을 한다', () => {
    const metadata = () => ({ speed: 1, turningSpeed: 1 });
    // 자격 뒤의 첫 읽기에서 단계 5 이면 목표 계산 전에 돌아간다(읽기 횟수 2: 자격 1 + 거부 1).
    const early = { stageSeq: [5] };
    const earlySpy = spyPath();
    const earlyScene = parentScene({ path: earlySpy.make, ritual: (real) => stubRitual(real, early), hooks: { pathMetadata: metadata } });
    earlyScene.checking = 0;
    earlySpy.result = earlySpy.fake();
    earlyScene.dais.bridgeCommand(5, 6, 1);
    assert.equal(early.stageCalls, 2);
    assert.deepEqual(earlySpy.requests, []);
    // 목표 계산 직전에 단계가 5 로 바뀌면 시작하지 않는다(읽기 순서: 자격 4, 거부 4, 목표 4, 재확인 5).
    const late = { stageSeq: [4, 4, 4, 5], abstract: false };
    const lateSpy = spyPath();
    const lateScene = parentScene({ path: lateSpy.make, ritual: (real) => stubRitual(real, late), hooks: { pathMetadata: metadata } });
    lateScene.checking = 0;
    lateSpy.result = lateSpy.fake();
    lateScene.dais.bridgeCommand(5, 6, 1);
    assert.equal(late.stageCalls, 4);
    assert.deepEqual(lateSpy.requests, []);
    // 목표 칸의 X 만 같고 Y 가 다르면 의식이 아니라 경로 시작이다(둘 다 같아야 의식).
    const rituals = [];
    const spy = spyPath();
    const scene = parentScene({ path: spy.make, ritual: (real) => stubRitual(real, { stage: 0, rituals }), hooks: { pathMetadata: metadata } });
    scene.checking = 0;
    spy.result = spy.fake();
    const target = scene.ritual.stagePoint(5, 0);
    const sameCell = (value) => (value === Math.trunc(value) + 0.5 ? Math.trunc(value) + 0.25 : Math.trunc(value) + 0.5);
    put(scene.raw(6), 14, floatToBits(sameCell(target.x)), 4);
    put(scene.raw(6), 18, floatToBits(target.y + 5), 4);
    scene.dais.bridgeCommand(5, 6, 1);
    assert.deepEqual(rituals, []);
    assert.equal(spy.requests.length, 1);
    spy.requests.length = 0;
    put(scene.raw(6), 14, floatToBits(target.x + 5), 4);
    put(scene.raw(6), 18, floatToBits(sameCell(target.y)), 4);
    scene.dais.bridgeCommand(5, 6, 1);
    assert.deepEqual(rituals, []);
    assert.equal(spy.requests.length, 1);
});

test('RawDaisRegular.finishSacrifice: 제단 원래 owner 는 WORD 두 바이트만 읽고 Player 결과는 가상 count 의 DWORD 네 바이트 전체를 쓴다', () => {
    // 원래 owner 가 로컬(1)이고 제단 좌표의 낮은 바이트가 0 이 아니어도(옆 바이트를 읽지 않으므로) 로컬 사제 사망 안내이다.
    const w = standardWorld();
    giveForm(w, 10, 154, 4);
    put(w.slots[5], 6, 10, 2);
    put(w.slots[7], 12, 1, 2);
    w.slots[7][14] = 0x12;
    w.slots[5][34] = 3;
    w.dais.finishSacrifice(5);
    assert.ok(w.log.includes('CHAT:LocalPriestDead,1,3,7'));
    // 업그레이드 분기의 결과 DWORD: 첫 count 는 표 인덱스(1), 두 번째 count 는 결과 값으로 네 바이트 모두 쓴다.
    let calls = 0;
    const big = standardWorld({ hooks: { count: () => { calls += 1; return calls === 1 ? 1 : 0x01020304; } } });
    giveForm(big, 10, 165, 4, 11);
    giveForm(big, 11, 158, 0x20);
    put(big.slots[5], 6, 10, 2);
    big.dais.finishSacrifice(5);
    assert.equal(big.state.playerResults[1], 0x01020304);
    assert.deepEqual([...big.state.playerResultSlot(1)], [4, 3, 2, 1]);
});

test('RawDaisRegular.finishSacrifice: mask 4 가 아닌 포획 후보(다른 비트만 선 Form)는 포획으로 읽지 않는다', () => {
    const owners = Array.from({ length: 8 }, (_, index) => `R:${index + 1}`);
    // 종류 154 Form 의 mask 가 1 뿐이면 포획 종류가 없는 것과 같다: 종류 0 을 로컬 타입으로 받고 SP 안내는 없다.
    const w = standardWorld();
    giveForm(w, 10, 154, 1);
    put(w.slots[5], 6, 10, 2);
    w.dais.finishSacrifice(5);
    assert.deepEqual(w.log, ['V:0,0,0', 'F:7,4,0', 'LT:0', ...owners, 'RL']);
    assert.equal(w.state.playerResults[1], 0);
    // 같은 mask 1 Form 이 다이스 종류(165)여도 업그레이드가 아니다.
    const upgrade = standardWorld({ hooks: { count: () => { throw new Error('업그레이드 분기로 가면 안 됨'); } } });
    giveForm(upgrade, 10, 165, 1);
    put(upgrade.slots[5], 6, 10, 2);
    upgrade.dais.finishSacrifice(5);
    assert.ok(!upgrade.log.some((entry) => entry.startsWith('D:')));
});

test('RawDaisRegular.progress: 제단이 없으면(0) 포획 종류가 있어도 제단 이탈을 하지 않는다', () => {
    const w = standardWorld({ daisFrame: 47, reserveSoundMask: 0, soundMask: 0 });
    giveForm(w, 10, 154, 4);
    put(w.slots[5], 6, 10, 2);
    // 제단을 해시에서 뺀다.
    w.hash.setBucket(1, 40.75, 41.25, 6);
    w.slots[6][4] = 0;
    w.slots[6][5] = 0;
    w.dais.progress(5);
    assert.ok(!w.log.some((entry) => entry.startsWith('F:')));
    assert.ok(w.log.includes('A:itIsDone2.wav,0'));
});

test('runDaisHandle: 305 의 프레임 DWORD 는 네 바이트가 모두 제 위치에서 읽힌다', () => {
    const pool = { slot: () => { const raw = new Uint8Array(128); put(raw, 36, 0x01020304, 4); return raw; } };
    const stages = [];
    runDaisHandle(pool, 3, 0x305, 0, 1, {
        stage: (dais, frame) => { stages.push(frame); return 0; }, frame: (dais, frame) => { stages.push(`set:${frame}`); },
    });
    assert.deepEqual(stages, [0x01020305, 0x01020304, `set:${0x01020305}`]);
});
