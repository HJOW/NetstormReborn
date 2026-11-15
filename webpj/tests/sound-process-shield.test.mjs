// cpppj SoundProcess_FollowsShieldCreationAndDeletion의10.78 경로를 E213의 실제 보호막/공간/표시/삭제 장면으로 대조한다.
// 실제 게임/브라우저/소리 장치는 실행하지 않는다. (2026-11-16 WB04-E571)
import './nan-exact.mjs';
import { SoundList, SOUND_LIST_BASE } from '../www/audio/sound-list.js';
import { SoundProcessState, SoundProcessSystem, makePriestShieldSoundHooks } from '../www/core/sound-process.js';
import test from 'node:test';
import assert from 'node:assert/strict';
import { put, floatToBits } from './raw-support.mjs';
import { realPlayerAssets } from './player-scene-support.mjs';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { SquidPostPop } from '../www/core/squid-postpop.js';
import { SquidPostPopState } from '../www/core/squid-postpop-state.js';
import { SquidPop, RAW_POP_RESULT } from '../www/core/squid-pop.js';
import { SquidDisplay } from '../www/core/squid-display.js';
import { SquidFrame } from '../www/core/squid-frame.js';
import { makeSquidFrameHooks } from '../www/core/squid-frame-binding.js';
import { SquidFactory } from '../www/core/squid-factory.js';
import { SquidOwner, SquidOwnerMode } from '../www/core/squid-owner.js';
import { RawSquidDestroy } from '../www/core/squid-destroy.js';
import { Kernel } from '../www/core/kernel.js';
import { SquidProcessHost, SquidProcessState } from '../www/core/squid-process.js';
import { RawFrameAdvance } from '../www/core/raw-frame-advance.js';
import { PRIEST_TYPE, PRIEST_VTABLE } from '../www/core/raw-priest-owner.js';
import { RawPriestForcefield, PriestForcefieldState } from '../www/core/raw-priest-forcefield.js';
import { RawPriestShield, PriestShieldState, makePriestShieldCreationHooks } from '../www/core/raw-priest-shield.js';
import { SquidDeletionState, SquidDestroyLifecycle } from '../www/core/squid-destroy-lifecycle.js';
import {
    FORCEFIELD_TYPE, RawForcefieldPostPop, RawForcefieldRegular, ForcefieldRegularState, makeForcefieldFrameHooks, makeForcefieldPostPopProcessHooks,
} from '../www/core/raw-forcefield-lifecycle.js';

// 읽기 전용 관측: 프레임 DWORD(+36)와 상태 BYTE(+11)이다. (2026-11-10 WB04-E213)
const frameOf = (pool, sid) => {
    const raw = pool.slot(sid);
    return (raw[36] | (raw[37] << 8) | (raw[38] << 16) | (raw[39] << 24)) | 0;
};

test('SoundProcess: 실제 보호막 생성→이름 조회→Sound47 부착→Kernel 위치 반복→자기 삭제 정지와 확보 실패가 같은 장부에서 균형을 이룬다', () => {
    const assets = realPlayerAssets();
    const types = assets.types.map((record) => ({ ...record }));
    // 보호막/사제의 필요한 필드만 합성한다(분류 비트가 장부 경로를 바꾸지 않도록 flags 를 명시).
    Object.assign(types[FORCEFIELD_TYPE], { maxHitPoints: 100, cost: 37, footX: 1, footY: 1, flags1: 0, flags2: 0 });
    Object.assign(types[PRIEST_TYPE], { footX: 1, footY: 1, flags1: 0, flags2: 0x210000 });
    const pool = new SidPool(24000, { server: false });
    const hash = new SquidHash();
    const spots = new Uint8Array(65536);
    // 사제는 실제 client 할당으로 만들고 해시 1단계 체인의 머리에 둔다.
    const priest = pool.allocate(2);
    const raw = pool.allocated(priest);
    raw.fill(0);
    put(raw, 0, PRIEST_VTABLE);
    raw[10] = PRIEST_TYPE;
    raw[34] = 1;
    put(raw, 14, floatToBits(20.75));
    put(raw, 18, floatToBits(21.9));
    raw[33] = 1;
    hash.setBucket(1, 20.75, 21.9, priest);
    // 프레임 표와 SHP 두 장(프레임 두 개짜리 보호막, 한 개짜리 사제)이다. 두 번째 필드는 그림자 여부가 아닌 loaded 이다.
    const frames = types.map(() => new FrameCodeTable([]));
    frames[FORCEFIELD_TYPE] = new FrameCodeTable([{ side: 65, variant: 80, flags: 0 }, { side: 65, variant: 80, flags: 0 }]);
    const shapes = types.map(() => ({ frameCount: 0, loaded: false, frames: [] }));
    shapes[FORCEFIELD_TYPE] = { frameCount: 2, loaded: true, frames: [
        { width: 12, height: 20, hotspotX: 3, hotspotY: 18, cellWidth: 1, cellHeight: 1 }, { width: 16, height: 21, hotspotX: 4, hotspotY: 19, cellWidth: 1, cellHeight: 1 }] };
    shapes[PRIEST_TYPE] = { frameCount: 1, loaded: true, frames: [{ width: 12, height: 20, hotspotX: 3, hotspotY: 18, cellWidth: 1, cellHeight: 1 }] };
    let changes = 0;
    const sink = { suppressed: () => false, invalidate: () => { changes += 1; } };
    const display = new SquidDisplay(types, shapes, sink, { cameraX: 0, cameraY: 0, zoom: 65536, viewport: { left: 0, top: 0, right: 640, bottom: 480 } });
    const unpop = new SquidUnpop(pool, hash, spots, display);
    const book = new SquidPostPopState();
    book.localOwner = 1;
    const base = new SquidPostPop(pool, types, book);
    const pop = new SquidPop(pool, hash, spots, { display, postPop: base });
    const destroy = new RawSquidDestroy(pool, unpop, types);
    const deletion = new SquidDeletionState();
    deletion.unitLostSuppressed1 = true;
    const lifecycle = new SquidDestroyLifecycle(pool, types, book, deletion, destroy, { emit: () => {}, forward: () => assert.fail('종속 가상 효과가 올라오면 안 된다') });
    const kernel = new Kernel();
    const clock = new SquidProcessState();
    clock.now = 12.5;
    let regular = null;
    const host = new SquidProcessHost(pool, types, kernel, destroy, clock, {
        regular: (sid, event, count, payload) => regular.handle(sid, event, count, payload),
        asset: lifecycle.hooks(),
    });
    const prefix = new RawForcefieldPostPop(pool, makeForcefieldPostPopProcessHooks(pool, host, () => true));
    base.setBody('forcefield', prefix);
    const setter = new SquidFrame(pool, types, makeSquidFrameHooks(pool, types, display, unpop, pop));
    const advance = new RawFrameAdvance(pool, frames, setter);
    regular = new RawForcefieldRegular(pool, hash, types, new ForcefieldRegularState(), makeForcefieldFrameHooks(pool, advance, {
        destroy: (sid, flags) => destroy.destroy(sid, flags, host.hooks()),
    }));
    const factory = new SquidFactory(pool, types);
    const owner = new SquidOwner(pool, types, book, new SquidOwnerMode());
    const lookup = new RawPriestForcefield(pool, hash, types, new PriestForcefieldState());
    const notice = new PriestShieldState({ localPlayer: 0, loadingDepth: 1, noticeClock: 1 });
    let shield = 0;
    // 이름 표/프로세스는 실제 모듈이고 오디오 장치 효과만 기록한다.
    const sounds = new SoundList(); sounds.initialize();
    const soundState = new SoundProcessState(); soundState.frame = 100;
    const played = [], playing = 0x15007000; let reserve = true;
    const soundSystem = new SoundProcessSystem(host, soundState, {
        wallSeconds: () => 3,
        playLoopAt: (x, y, current, sound) => { played.push(['L', floatToBits(x), floatToBits(y), current, sounds.name(sound)]); return playing; },
        playOnceAt: () => assert.fail('보호막은 위치 반복만 요청한다'), play: () => assert.fail('보호막은 전역 재생하지 않는다'),
        stop: (sound) => { played.push(['S', sound]); }, isPlaying: (sound) => { played.push(['P', sound]); return 0; },
    });
    const hooks = makePriestShieldSoundHooks(pool, sounds, soundSystem, makePriestShieldCreationHooks(pool, factory, owner, {
        pop: (sid, x, y, flags) => {
            shield = sid;
            assert.equal(pop.pop(sid, types[FORCEFIELD_TYPE], 1, 1, x, y, flags), RAW_POP_RESULT.registered);
        },
        reserveSound: () => reserve,
        noticeSound: () => assert.fail('로딩 중에는 안내 소리가 없다'),
        translate: () => assert.fail('로딩 중에는 번역하지 않는다'),
        tell: () => assert.fail('로딩 중에는 안내하지 않는다'),
    }));
    const create = new RawPriestShield(pool, lookup, notice, hooks);
    const before = pool.freeCount;
    create.ensure(priest);
    assert.equal(lookup.find(priest), shield);
    assert.equal(kernel.size, 3);
    assert.equal(book.depth, 0);
    assert.equal(book.globalCounts()[FORCEFIELD_TYPE], 1);
    assert.equal(book.totalCost, 37);
    assert.ok(host.findEvent(shield, 1));
    assert.ok(host.findEvent(shield, 2));
    // 이미 있으면 다시 만들지 않는다.
    const allocated = pool.freeCount;
    create.ensure(priest);
    assert.equal(pool.freeCount, allocated);
    assert.equal(kernel.size, 3);
    const entry = sounds.lookup('PRIESTFORCEFIELD.WAV');
    assert.equal(entry, SOUND_LIST_BASE + 44); assert.equal(sounds.name(entry), 'priestForceField.wav'); assert.deepEqual(played, []);
    // Kernel 한 프레임: 사건1의 표시 진행·사건2의 대기와 소리 프로세스까지 세 개가 계속 산다.
    kernel.runFrame();
    assert.equal(kernel.size, 3);
    assert.ok(changes > 0);
    assert.equal(frameOf(pool, shield), 1);
    const where = [floatToBits(20.75), floatToBits(21.9)];
    assert.deepEqual(played, [['L', ...where, 0, 'priestForceField.wav']]); assert.equal(soundState.typeFrames[FORCEFIELD_TYPE], 100);
    kernel.runFrame(); soundState.frame += 1; kernel.runFrame();
    assert.deepEqual(played, [['L', ...where, 0, 'priestForceField.wav'], ['L', ...where, playing, 'priestForceField.wav'], ['L', ...where, playing, 'priestForceField.wav']]);
    assert.equal(soundState.typeFrames[FORCEFIELD_TYPE], 101);
    // 사제가 사라진 뒤 사건 2 의 시각이 되면 보호막이 자기 삭제하고 종속 Form 이 반납되며 장부가 복구된다.
    const due = host.findEvent(shield, 2).time;
    unpop.unpop(priest, types[PRIEST_TYPE], 0);
    clock.now = due;
    played.length = 0; kernel.runFrame();
    assert.deepEqual(played.slice(-2), [['S', playing], ['P', playing]]);
    assert.equal(kernel.size, 0);
    assert.ok(pool.slot(shield)[11] & 1);
    assert.equal(hash.bucket(0, 20.75, 21.9), 0);
    assert.equal(pool.freeCount, before);
    assert.equal(book.globalCounts()[FORCEFIELD_TYPE], 0);
    assert.equal(book.totalCost, 0);
    assert.equal(destroy.preDepth, 0);
    assert.equal(destroy.postDepth, 0);
    assert.equal(lookup.find(priest), 0);
    reserve = false; played.length = 0; create.ensure(priest);
    assert.equal(kernel.size, 2);
    assert.equal(lookup.find(priest), shield);
    // 외부 삭제도 같은 실제 종속 Form/Kernel 정리를 사용한다.
    destroy.destroy(shield, 0, host.hooks());
    assert.equal(kernel.size, 0);
    assert.equal(pool.freeCount, before);
    assert.equal(book.totalCost, 0); assert.deepEqual(played, []);
});
