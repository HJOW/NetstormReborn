// CampaignObjectMenuTests의 TeleportCurrent/선택 생성자 계약을 같은 실제 owner/Host/보호 자산으로 대조한다(WB04-E585).
// 전체 Session Load·브라우저·시전/전투·공중3종 전용 생성/행동 검사가 아니다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { readData } from './helpers.mjs';
import { realCaptureAssets } from './capture-assets-support.mjs';
import { get, put } from './raw-support.mjs';
import { GameAssets } from '../www/assets/game-assets.js';
import { GamePalette } from '../www/assets/palette.js';
import { SoundList } from '../www/audio/sound-list.js';
import { RawCampaignSessionOwners } from '../www/core/raw-campaign-session-owners.js';
import { RawCampaignMotionBindings } from '../www/core/raw-campaign-motion-bindings.js';
import { RawPlayerPlacementAnchor } from '../www/core/raw-player-placement-anchor.js';
import { RawContainedCreate, makeContainedCreateHooks } from '../www/core/contained-create.js';
import { SquidPostPopList } from '../www/core/squid-postpop-state.js';
import { RawContainedLookup } from '../www/core/raw-contained-lookup.js';
import { BombEffectTimerProcess } from '../www/core/raw-bomb-effect-timer.js';
import { CursorManaProcess } from '../www/core/raw-cursor-mana.js';
import { CursorRangeProcess } from '../www/core/raw-cursor-range.js';

// 같은 실제 COL/116TYPE/SHP를 각 owner의 현재188TYPE/월드에 빌린다.
const data = realCaptureAssets(); const game = new GameAssets(data.table, data.database, GamePalette.parse(readData('d/GIFCLOUD.COL')));
// 선택하지 않은 외부 효과를 성공으로 채우지 않는다.
function forbidden() { throw new Error('선택되지 않은 외부 효과'); }
// 앞 생성 구간을 실제 원문 순서로 조립한다. 전체 Session을 Runtime에 공개하지 않는다.
function scene() {
    const o = new RawCampaignSessionOwners(game, { read: forbidden }, 'thewarbegins', { unixSeconds: () => 1700000000, soundNames: new SoundList(), sound: { wallSeconds: () => 0, play: forbidden, playOnceAt: forbidden, playLoopAt: forbidden, stop: forbidden, isPlaying: forbidden } });
    o.initializePlayersAndSp(); o.connectPlayerTablesAndPackets(); o.connectIslandsAndPriests(); o.connectPlayersAndOwnership(); o.connectBodiesAndTypeTables(); o.connectGunAndCursorViews(); o.connectHitBodies(); o.connectGunBodies(); o.connectParticles();
    return o;
}
// cpp 검사처럼 물리 VT만 Teleport로 바꾸고 TYPE74의 공통 부모가 별도 효과를 요구하지 않게 한다.
function source(o) {
    const sid = o.world.create(74, 2); const raw = o.core.pool.allocated(sid);
    put(raw, 0, 0x5070c0); raw[11] = raw[40] = 0; raw[34] = 1;
    o.core.types[74].flags1 = o.core.types[74].flags2 = 0; return sid;
}
// 실제 가상 접두에서 공통 장부 복귀까지 실행한다. 실패는 올린 깊이를 보존한다.
function post(o, sid, flags = 0x2000001) { ++o.core.bookkeeping.depth; o.core.postPop.postPopVirtual(sid, flags); }
// 현재 Kernel 등록 순서로 같은 부모의 실제 프로세스를 찾는다. 게임 프레임은 진행하지 않는다.
function processes(o, parent) {
    const found = [];
    // 실제 Kernel의 고정 슬롯을 읽으며 별도 효과 목록을 만들지 않는다.
    for (let pid = 1; pid <= o.core.kernel.capacity; ++pid) { const p = o.core.kernel.get(pid); if (p?.parent === parent) found.push(p); }
    return found;
}

test('Session Teleport: 입자 선행·한 번 결선·미공개·반납과 장기 바람/약화 HP getter를 유지한다', () => {
    const o = scene();
    try {
        const ready = o.particleModules; o.particleModules = null; assert.throws(() => o.connectTeleport(), /초기화 전/); o.particleModules = ready;
        o.connectTeleport(); assert.equal(o.teleportModules.ready, true); assert.throws(() => o.connectTeleport(), /반복/); assert.throws(() => o.gameplayGlobals.requireSession(), /미연결/);
        o.world.storage.wind.x = Math.fround(0.25); assert.equal(o.teleportModules.wind(), 0.25); o.world.storage.wind.x = -0.5; assert.equal(o.teleportModules.wind(), -0.5);
        const aux = o.gameplayGlobals.gameAux(); assert.equal(o.teleportModules.weakenedMana(), false); aux.priestQuarterHp = true; assert.equal(o.teleportModules.weakenedMana(), true);
        assert.throws(() => o.particles.bindWind(() => 0), /다른 owner/); assert.throws(() => o.particles.bindWeakenedMana(() => false), /다른 주소/);
    } finally { o.dispose(); }
    o.dispose(); assert.throws(() => o.connectTeleport(), /반납/);
});

test('TeleportCurrent: 비최초/free 입력은 잘못된 현재 효과 TYPE을 읽지 않고 공통 깊이로 복귀한다', () => {
    const o = scene();
    try { o.connectTeleport(); const sid = source(o); const raw = o.core.pool.allocated(sid); raw[11] = 1; raw[40] = 8; o.gameplayGlobals.gameAux().streakProcessType = 188; const before = o.core.kernel.size; post(o, sid, 0x2000000); assert.equal(o.core.bookkeeping.depth, 0); assert.equal(o.core.kernel.size, before); assert.equal(raw[11], 1); raw[11] = raw[40] = 0; } finally { o.dispose(); }
});

test('TeleportCurrent: 현재 TYPE의 생성자 선택과 생성자 자체 부착 TYPE을 구분한다', () => {
    const o = scene();
    try {
        o.connectTeleport(); const c = o.core; const aux = o.gameplayGlobals.gameAux();
        c.types[57] = { ...c.types[12] }; c.types[58] = { ...c.types[11] }; c.types[56] = { ...c.types[11] }; aux.fieryTailProcessType = 56;
        // 현재 Streak 함수는57에, Fiery 함수는58에 별칭으로 놓아 고정12 우회를 검출한다.
        for (const [selected, attached, life, offset] of [[57, 57, 2, -8], [58, 56, 4, 0]]) {
            aux.streakProcessType = selected; const sid = source(o); const before = c.kernel.size; post(o, sid);
            const list = processes(o, sid); assert.equal(list.length, 1); assert.equal(c.kernel.size, before + 1); assert.equal(c.pool.slot(list[0].form)[10], attached); assert.equal(list[0].fields.particles.length, 120); assert.equal(list[0].fields.maximumLife, life); assert.equal(list[0].fields.offset, offset); assert.equal(c.bookkeeping.depth, 0);
        }
    } finally { o.dispose(); }
});

test('TeleportCurrent: 물리 TYPE 범위와 미지원 주소는 공통 부모 앞에서 실패하고 깊이를 보존한다', () => {
    const o = scene();
    try {
        o.connectTeleport(); o.core.types[12].constructorAddress = 0xdeadbeef;
        // 선택된 오류 둘은 공통 깊이/장부/Kernel을 성공으로 보정하지 않는다.
        for (const type of [188, 0xffffffff, 12]) {
            const sid = source(o); o.gameplayGlobals.gameAux().streakProcessType = type; const before = o.core.kernel.size;
            assert.throws(() => post(o, sid), /TYPE 표 물리 범위|다른 입자 생성 함수/); assert.equal(o.core.bookkeeping.depth, 1); assert.equal(o.core.kernel.size, before); o.core.bookkeeping.depth = 0;
        }
    } finally { o.dispose(); }
});

test('TeleportStubCurrent: NULL 생성자 진단이 바꾼 현재 슬롯으로 공통 부모를 계속하고 선택 함수를 다시 읽지 않는다', () => {
    const o = scene();
    try {
        o.connectTeleport(); const c = o.core; const aux = o.gameplayGlobals.gameAux(); aux.streakProcessType = 61; const sid = source(o); const raw = c.pool.allocated(sid); const calls = [];
        c.types[61].constructorAddress = 0x470b00;
        o.services.ai.assertion = (expression) => { calls.push(expression); c.types[61].constructorAddress = 0xdeadbeef; raw[10] = 75; c.types[75].flags1 = c.types[75].flags2 = 0; };
        const before = c.bookkeeping.globalCounts()[75]; post(o, sid); assert.deepEqual(calls, ['0']); assert.equal(c.bookkeeping.globalCounts()[75], before + 1); assert.equal(c.bookkeeping.depth, 0); assert.equal(processes(o, sid).length, 0);
    } finally { o.dispose(); }
});

test('TeleportStubCurrent: 진단 누락/중단은 앞선 슬롯 쓰기와 공통 깊이를 보존한다', () => {
    const o = scene();
    try {
        o.connectTeleport(); const c = o.core; o.gameplayGlobals.gameAux().streakProcessType = 61; c.types[61].constructorAddress = 0x470b00; const sid = source(o);
        assert.throws(() => post(o, sid), /실제 진단 장치 미연결/); assert.equal(c.bookkeeping.depth, 1); c.bookkeeping.depth = 0;
        o.services.ai.assertion = () => { c.pool.allocated(sid)[34] = 3; throw new Error('진단 중단'); };
        assert.throws(() => post(o, sid), /진단 중단/); assert.equal(c.bookkeeping.depth, 1); assert.equal(c.pool.slot(sid)[34], 3); c.bookkeeping.depth = 0;
    } finally { o.dispose(); }
});

test('Form/AssetConstructorCurrent: 주소별 물리 쓰기와 NULL을 같은 SID에 적용하고 TYPE/할당을 바꾸지 않는다', () => {
    const o = scene();
    try {
        o.connectTeleport(); const c = o.core; const aux = o.gameplayGlobals.gameAux(); aux.streakProcessType = 61;
        // 원문 네 Form과 두 자산/NULL은 실제 검증된 생성자 recipe를 사용한다.
        for (const [kind, expected] of [[1, 0x512110], [2, 0x512098], [3, 0x512188], [6, 0x512208], [89, 0x50f540], [103, 0x50c3b0], [153, 0x5070c0]]) {
            c.types[61].constructorAddress = c.types[kind].constructorAddress; const sid = source(o); const free = c.pool.freeCount; const kernel = c.kernel.size; const count = c.bookkeeping.globalCounts()[74]; post(o, sid);
            assert.equal(get(c.pool.slot(sid), 0), expected, `TYPE${kind}`); assert.equal(c.pool.slot(sid)[10], 74); assert.equal(c.pool.freeCount, free); assert.equal(c.kernel.size, kernel); assert.equal(c.bookkeeping.globalCounts()[74], count + 1); assert.equal(c.bookkeeping.depth, 0); put(c.pool.allocated(sid), 0, 0x5070c0);
        }
    } finally { o.dispose(); }
});

test('BombTimerConstructorCurrent: 같은 실제 장치의 빈100칸/기한0을 Init/부착 없이 보관하고 반납한다', () => {
    const o = scene();
    try {
        o.connectTeleport(); const c = o.core; o.gameplayGlobals.gameAux().streakProcessType = 61; c.types[61].constructorAddress = c.types[57].constructorAddress; assert.equal(c.types[61].constructorAddress, 0x420ad0);
        c.processState.now = 100.25; const sid = source(o); const free = c.pool.freeCount; const kernel = c.kernel.size; post(o, sid);
        assert.equal(o.unstartedBombTimers.length, 1); const timer = o.unstartedBombTimers[0]; assert.ok(timer instanceof BombEffectTimerProcess); assert.deepEqual([timer.parent, timer.form, timer.fields.endTime, timer.fields.capacity, timer.fields.flags], [0, 0, 0, 100, 0]); assert.deepEqual(timer.fields.members, []); c.processState.now = 9999.5; assert.equal(timer.fields.endTime, 0); assert.equal(c.pool.freeCount, free); assert.equal(c.kernel.size, kernel); assert.equal(c.bookkeeping.depth, 0);
    } finally { o.dispose(); }
    assert.equal(o.unstartedBombTimers.length, 0);
});

test('CursorConstructorCurrent: 현재 함수와 별도 부착 TYPE의 실제 Form/8표시를 같은 Host에 만든다', () => {
    const o = scene();
    try {
        o.connectTeleport(); const c = o.core; o.gameplayGlobals.gameAux().streakProcessType = 61; Object.assign(c.types[74], { theme: 0, group: 0, footX: 1, footY: 1 }); o.cursorManaGlobals.processType = 58; o.cursorRangeGlobals.processType = 57;
        const lookup = new RawContainedLookup(c.pool, forbidden);
        // 마나/사정거리 두 고정 몸체가 각자의 현재 프로세스 번호를 사용한다.
        for (const [address, type, marker, Kind, list, countAt] of [[0x4741d0, 58, 103, CursorManaProcess, 12, 0x84], [0x474900, 57, 104, CursorRangeProcess, 20, 0x1a4]]) {
            c.types[61].constructorAddress = address; const sid = source(o); const kernel = c.kernel.size; const free = c.pool.freeCount; post(o, sid); const form = lookup.find(sid, type); const p = c.processes.fromForm(form);
            assert.ok(p instanceof Kind); assert.equal(p.parent, sid); assert.equal(p.form, form); assert.equal(c.pool.slot(form)[10], type); assert.equal(c.kernel.size, kernel + 1); assert.equal(c.pool.freeCount, free - 9); assert.equal(get(p.fields.bytes, countAt), 8); assert.equal(c.bookkeeping.depth, 0);
            // 실제 표시 자식8개의 현재 TYPE/숨김 비트를 읽는다. 표시 프레임은 진행하지 않는다.
            for (let index = 0; index < 8; ++index) { const child = get(p.fields.bytes, list + index * 4); assert.ok(child); assert.equal(c.pool.slot(child)[10], marker); assert.ok(c.pool.slot(child)[40] & 8); }
        }
    } finally { o.dispose(); }
});

test('PathConstructorCurrent: 같은 실제 Motion의8주소를 Init/부모/Kernel 시작 없이 생성·보관한다', () => {
    const o = scene();
    try {
        o.connectTeleport(); const c = o.core; const s = o.state;
        // 아직 Session 생성 순서에서 설치 전인 실제 Motion 몸체를 같은 owner 자료에 연결한다. 외부 행동은 명시 거부한다.
        const anchor = new RawPlayerPlacementAnchor(c.pool, c.types, c.hash.view(0), c.bookkeeping, new SquidPostPopList(), o.priestState.contained, s.anchorState);
        const contained = new RawContainedCreate(c.pool, { containedType: () => o.gameplayGlobals.gameAux().contentFormType, bridgeType: () => c.bridgeState.bridgeType }, makeContainedCreateHooks(c.pool, c.factory, { notifySurface: (sid) => c.building.notifySurface(sid), attach: (sid, parent, flags) => c.forms.attach(sid, parent, flags) }));
        o.motion = new RawCampaignMotionBindings(o.world, o.priests, o.islands, contained, anchor, o.hp, o.metadata.frames, s.motionState, s.start, s.process, {
            cleanup: { reserveShared: forbidden, ritual: forbidden, refund: forbidden }, halt: { carrierPickup: forbidden, nuggetTransfer: forbidden },
            carrier: { genusAt: forbidden, pickup: forbidden, escape: forbidden, chooseReturn: forbidden, chooseGeyser: forbidden, goReturn: forbidden, goGeyser: forbidden, minedPacket: forbidden, robust: forbidden }, priest: { neutralReturn: forbidden },
            path: { metadata: forbidden, soundId: forbidden, stepSound: forbidden, startBlocked: forbidden, arrivalBlocked: forbidden, retarget: forbidden },
        });
        o.gameplayGlobals.gameAux().streakProcessType = 61; const free = c.pool.freeCount; const kernel = c.kernel.size;
        // 각 함수 주소가 고정한 클래스만 고르며 다른 클래스의 행동은 실행하지 않는다.
        for (const address of [0x48be40, 0x48bf30, 0x48bed0, 0x48bea0, 0x48bf90, 0x48bf00, 0x48be70, 0x48bf60]) { c.types[61].constructorAddress = address; const sid = source(o); const before = c.pool.freeCount; post(o, sid); const p = o.unstartedPaths.at(-1); assert.equal(p.pool, c.pool); assert.equal(p.parent, 0); assert.equal(p.form, 0); assert.equal(c.pool.freeCount, before); assert.equal(c.kernel.size, kernel); assert.equal(c.bookkeeping.depth, 0); }
        assert.equal(o.unstartedPaths.length, 8); assert.equal(c.pool.freeCount, free - 8);
    } finally { o.dispose(); }
    assert.equal(o.unstartedPaths.length, 0);
});
