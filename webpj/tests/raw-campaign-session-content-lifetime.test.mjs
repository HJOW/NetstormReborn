// InstallContents3050..3134의 같은 Form6/Carrier/Player·Unpop/생성 부모를 대조한다(WB04-E597).
// 실제 보호 자산/기존 몸체와 명시 외부 경계의 계약이며 게임/실제 포획·희생 관측이 아니다.
import './nan-exact.mjs';
import test from 'node:test';
import assert from 'node:assert/strict';
import { GameAssets } from '../www/assets/game-assets.js';
import { GamePalette } from '../www/assets/palette.js';
import { typeString } from '../www/assets/type-table.js';
import { SoundList } from '../www/audio/sound-list.js';
import { RawCampaignSessionOwners } from '../www/core/raw-campaign-session-owners.js';
import { RawCampaignLifetimeBindings } from '../www/core/raw-campaign-lifetime-bindings.js';
import { ContentFormHost } from '../www/core/content-form-host.js';
import { RawContainedCreate } from '../www/core/contained-create.js';
import { SoundProcess } from '../www/core/sound-process.js';
import { RegularProcess } from '../www/core/squid-process.js';
import { readData } from './helpers.mjs';
import { realCaptureAssets } from './capture-assets-support.mjs';
import { put, get } from './raw-support.mjs';

// 선택하지 않은 실제 외부 효과는 빈 성공으로 대신하지 않는다.
function forbidden() { throw new Error('선택하지 않은 외부 효과'); }
// 같은 보호 자산·팔레트와 실제 서비스 기반 owner다.
const data = realCaptureAssets(); const game = new GameAssets(data.table, data.database, GamePalette.parse(readData('d/GIFCLOUD.COL')));
// 서비스 함수는 명시 소비자로만 공급한다. World/하위 객체는 실제 구현이다.
function owner(extra = {}) { return new RawCampaignSessionOwners(game, { read: forbidden }, 'thewarbegins', { unixSeconds: () => 1700000000, soundNames: new SoundList(), sound: { wallSeconds: () => 0, play: forbidden, playOnceAt: forbidden, playLoopAt: forbidden, stop: forbidden, isPlaying: forbidden }, ...extra }); }
// 실제 생성자 앞 구간을 통지까지 원문 순서로 연결한다.
function before(o) { o.initializePlayersAndSp(); o.connectPlayerTablesAndPackets(); o.connectIslandsAndPriests(); o.connectPlayersAndOwnership(); o.connectBodiesAndTypeTables(); o.connectGunAndCursorViews(); o.connectHitBodies(); o.connectGunBodies(); o.connectParticles(); o.connectTeleport(); o.connectCombat(); o.connectVortex(); o.connectAssets(); o.connectWorldRuntimeHooks(); o.connectContentNotices(); }
// 실제 수명까지 연결한다. 전체 Session/Load는 게시하지 않는다.
function scene(extra = {}) { const o = owner(extra); before(o); o.connectContentLifetime(); return o; }
// 현재 Player 통지 체인 입력의 실제 SID와 물리 VT다. 세계에 Pop하지 않는다.
function parent(o) { const sid = o.world.create(75, 0); put(o.core.pool.allocated(sid), 0, 0x50ee08); return sid; }

test('Session ContentLifetime: 선행/반복/반납 관문과 실제 동일 Pool·Form6 등록·World 미공개를 보존한다', () => {
    const o = owner(); try { assert.throws(() => o.connectContentLifetime(), /초기화 전/); before(o); const bytes = o.core.pool.bytes(); const seed = o.random.state; o.connectContentLifetime();
        assert.equal(o.contentModules.lifetimeReady, true); assert.ok(o.contentForms instanceof ContentFormHost); assert.ok(o.lifetime instanceof RawCampaignLifetimeBindings); assert.ok(o.contained instanceof RawContainedCreate); assert.equal(o.contentForms.pool, o.core.pool); assert.equal(o.lifetime.pool, o.core.pool); assert.equal(o.contained.pool, o.core.pool); assert.equal(o.containedUnpop.pool, o.core.pool);
        assert.deepEqual(o.core.pool.bytes(), bytes); assert.equal(o.random.state, seed); assert.throws(() => o.core.forms.bind(o.lifetime.contents()), /중복/); assert.throws(() => o.gameplayGlobals.requireSession(), /미연결/); assert.throws(() => o.connectContentLifetime(), /반복/);
    } finally { o.dispose(); }
    assert.throws(() => o.connectContentLifetime(), /반납/); assert.throws(() => o.contentModules.carrierEffects.allocate(40), /반납/);
});

test('Session ContentLifetime: 실제 생성·정렬 부착·공통 Pop/Unpop이 같은 Parent/이웃 WORD를 갱신한다', () => {
    const o = scene(); try { const root = parent(o); const calls = []; const pop = o.core.factory.popForm.bind(o.core.factory); const unpop = o.core.factory.unpopForm.bind(o.core.factory);
        o.core.factory.popForm = (...args) => { calls.push(['pop', ...args]); return pop(...args); }; o.core.factory.unpopForm = (...args) => { calls.push(['unpop', ...args]); return unpop(...args); };
        const first = o.contained.create(root, 94, 7, 0x20, 0x10, 8); const second = o.contained.create(root, 94, 9, 0x20, 0x10, 10);
        assert.equal(get(o.core.pool.slot(root), 6, 2), first); assert.equal(get(o.core.pool.slot(first), 4, 2), second); assert.equal(get(o.core.pool.slot(second), 14, 2), first); assert.equal(get(o.core.pool.slot(second), 16, 2), root);
        assert.deepEqual([get(o.core.pool.slot(first), 12, 2), get(o.core.pool.slot(first), 18, 2), o.core.pool.slot(first)[30]], [7, 94, 8]);
        o.contentForms.unpop(first); assert.equal(get(o.core.pool.slot(root), 6, 2), second); assert.equal(get(o.core.pool.slot(second), 14, 2), 0); assert.equal(o.core.pool.slot(first)[11] & 8, 0); assert.deepEqual(calls, [['pop', first, 0x110], ['pop', second, 0x110], ['unpop', first]]);
    } finally { o.dispose(); }
});

test('Session ContentLifetime: 현재 커서 DWORD/로컬 BYTE/수입 사건과 후설치 커서 취소 부모를 사용한다', () => {
    const o = owner(); before(o); o.playerState.battleOwner = 0x107; o.connectContentLifetime();
    try { const s = o.state.contentLifetimeState; assert.equal(s.localOwner, 7); assert.equal(s.notice, 0x020000b4); o.playerState.battleOwner = 3; assert.equal(s.localOwner, 7); o.cursorDrop.preview = 0xdead1234; assert.equal(s.readCursor(), 0xdead1234);
        assert.throws(() => o.contentModules.lifetimeEffects.cancelCursor(), /커서 수명 미설치/); const calls = []; o.cursor = { cancel: () => calls.push('first') }; o.contentModules.lifetimeEffects.cancelCursor(); o.cursor = { cancel: () => calls.push('next') }; o.contentModules.lifetimeEffects.cancelCursor(); assert.deepEqual(calls, ['first', 'next']);
    } finally { o.dispose(); }
});

test('Session ContentLifetime: 원격 수입은 현재 접속/NULL 관문을 보존하고 연결된5a는 미연결 오류다', () => {
    const o = scene(); try { const h = o.contentModules.lifetimeEffects; assert.throws(() => h.localNotice(1), /cpp:3060/); assert.throws(() => h.sound(1, 2), /cpp:3066/); h.remoteNotice(0); h.remoteNotice(1);
        o.services.battlePeers[1] = forbidden; o.players[180 + 0x50] = 0; h.remoteNotice(1); o.players[180 + 0x50] = 1; assert.throws(() => h.remoteNotice(1), /연결된 원격5a/); o.services.battlePeers[1] = null; h.remoteNotice(1);
        o.services.battlePeers[0] = forbidden; assert.throws(() => h.remoteNotice(0), /연결된 원격5a/); assert.throws(() => h.remoteNotice(9), /Player 범위/);
    } finally { o.dispose(); }
});

test('Session ContentLifetime: 서비스 값 복사와 기본값을 쓰지 않는 사용자 효과를 보존한다', () => {
    const calls = []; const life = { localNotice: (event) => calls.push(['local', event]), remoteNotice: (owner) => calls.push(['remote', owner]) }; const carrier = { sound: (token) => calls.push(['sound', token]), allocate: () => 77, create: () => 88, duration: (token, time) => calls.push(['duration', token, time]) }; const o = owner({ contentLifetime: life, carrierContent: carrier }); life.localNotice = forbidden; carrier.sound = forbidden;
    try { before(o); o.connectContentLifetime(); o.services.contentLifetime.localNotice = forbidden; o.services.carrierContent.sound = forbidden; const m = o.contentModules; m.lifetimeEffects.localNotice(4); m.lifetimeEffects.remoteNotice(6); m.carrierEffects.sound(123); assert.equal(m.carrierEffects.allocate(40), 77); assert.equal(m.carrierEffects.create(77, 1, 2, 3, 4), 88); m.carrierEffects.duration(88, 1.5); assert.deepEqual(calls, [['local', 4], ['remote', 6], ['sound', 123], ['duration', 88, 1.5]]); assert.equal(o.pendingPickupSound, null);
    } finally { o.dispose(); }
});

test('Session ContentLifetime: 부모는 현재 다이스→연구→Carrier→값 복사한 외부 통지 순서로 선택한다', () => {
    const calls = []; const o = scene({ contentNotice: (...args) => calls.push(['other', ...args]) }); try { const root = parent(o); const m = o.contentModules; o.services.contentNotice = forbidden;
        o.daisNotice = { handles: () => true, notice: (...args) => calls.push(['dais', ...args]) }; o.factoryResearch = { handlesNotice: forbidden }; m.parentNotice(root, 2, 3);
        o.daisNotice.handles = () => false; o.factoryResearch = { handlesNotice: () => true, notice: (...args) => calls.push(['research', ...args]) }; m.parentNotice(root, 4, 5);
        o.factoryResearch.handlesNotice = () => false; const original = o.carrierContentNotice; o.carrierContentNotice = { handles: () => true, notice: (...args) => calls.push(['carrier', ...args]) }; m.parentNotice(root, 6, 7); o.carrierContentNotice = original; m.parentNotice(root, 8, 9);
        assert.deepEqual(calls, [['dais', root, 2, 3], ['research', root, 4, 5], ['carrier', root, 6, 7], ['other', root, 8, 9]]);
    } finally { o.dispose(); }
});

test('Session ContentLifetime: 같은 실제 Player 통지로 이어지고 다른 부모 누락을 성공으로 바꾸지 않는다', () => {
    const o = scene(); try { const root = parent(o); const calls = []; const notify = o.playerContent.notify.bind(o.playerContent); o.playerContent.notify = (...args) => { calls.push(args); notify(...args); }; o.contentNotice.notice(root, 9, 2); assert.deepEqual(calls, [[root, 9, 2]]);
        put(o.core.pool.physical(root), 0, 0x501dd0); assert.throws(() => o.contentModules.parentNotice(root, 10, 3), /다른 캠페인 내용물 실제 부모 통지 미연결/);
    } finally { o.dispose(); }
});

test('Session ContentLifetime: 운반 상태는 현재 저장/로딩/폭탄·소유자/사제·전역 snapshot 픽셀을 조회한다', () => {
    const o = scene(); try { const s = o.contentModules.carrierNoticeState; o.contentSavedGame = true; o.objectState.loadingDepth = 2; o.contentBombBlocked = 3; o.playerState.battleOwner = 4; o.priestState.movement.priest = 159; o.priestState.movement.rainWalker = 145; o.gameplayGlobals.camera().snapshot().setPixels(41, 42); o.gameplayGlobals.camera().rendererView().setPixels(51, 52);
        assert.deepEqual([s.savedGame, s.loadingDepth, s.bombBlocked, s.localOwner, s.priestType, s.cameraX, s.cameraY, s.freeSoundBlockerType], [true, 2, 3, 4, 159, 41, 42, 145]);
    } finally { o.dispose(); }
});

test('Session ContentLifetime: 운반 중심/TYPE 탐색은 실제 현재 부모이고 genus는 뒤 설치할 피해 부모다', () => {
    const o = scene(); try { const root = parent(o); const h = o.contentModules.carrierEffects; assert.deepEqual(h.center(root), o.gunTarget.center(root)); assert.equal(h.typeAt(10, 10, 94), o.world.find(10, 10, 94)); assert.throws(() => h.genusAt(1, 2, 3), /DamageablePost 미설치/);
        const calls = []; o.damageablePost = { genusAt: (...args) => { calls.push(args); return 19; } }; assert.equal(h.genusAt(11, 12, 13), 19); assert.deepEqual(calls, [[11, 12, 13]]);
    } finally { o.dispose(); }
});

test('Session ContentLifetime: TYPE 소리는 보호 정의의 현재6c/7c이며 다른 오프셋만 기존 장치에 남긴다', () => {
    const calls = []; const o = scene({ carrierContent: { typeWord: (type, offset) => { calls.push([type, offset]); return 0x98765432; } } });
    try { const h = o.contentModules.carrierEffects; o.services.carrierContent.typeWord = forbidden; const records = game.types();
        // 두 속성이 다른 보호 정의를 찾아 오프셋 교환을 구별한다. 객체 생성/행동 관측은 하지 않는다.
        const index = records.findIndex(({ definition }) => typeString(definition, 'activeSound') !== typeString(definition, 'pickupSound')); assert.ok(index >= 0);
        const entry = records[index]; const active = typeString(entry.definition, 'activeSound'); const pickup = typeString(entry.definition, 'pickupSound'); assert.equal(h.typeWord(index + 70, 0x6c), active ? o.soundNames.lookup(active) : 0); assert.equal(h.typeWord(index + 70, 0x7c), pickup ? o.soundNames.lookup(pickup) : 0);
        assert.equal(h.typeWord(12, 0x99), 0x98765432); assert.deepEqual(calls, [[12, 0x99]]); assert.throws(() => h.typeWord(69, 0x6c), /TYPE 범위/); assert.throws(() => h.typeWord(188, 0x7c), /TYPE 범위/);
    } finally { o.dispose(); }
});

test('Session ContentLifetime: 선택 내용물 표시는 로딩 관문 뒤 같은 실제 Regular를 부착한다', () => {
    const o = scene(); try { const root = parent(o); const h = o.contentModules.carrierEffects; o.objectState.loadingDepth = 1; h.checkCarrier(root); assert.equal(get(o.core.pool.slot(root), 6, 2), 0); o.objectState.loadingDepth = 0; h.checkCarrier(root); const form = get(o.core.pool.slot(root), 6, 2); assert.ok(form > 0); assert.ok(o.core.processes.fromForm(form) instanceof RegularProcess);
    } finally { o.dispose(); }
});

test('Session ContentLifetime: 운반 소리·위치 소리·번개는 현재 장치/이름표/원문 인자를 유지한다', () => {
    const calls = []; const o = scene(); try { const h = o.contentModules.carrierEffects; o.services.sound.play = (...args) => calls.push(['play', ...args]); o.services.sound.playOnceAt = (...args) => calls.push(['at', ...args]); const free = o.soundNames.lookup('priestFree.wav'); const thunder = o.soundNames.lookup('thunderCrack.wav');
        h.sound(0x76543210); h.freeSound('priestFree.wav'); h.soundAt(1.25, 2.5, 'thunderCrack.wav'); const particles = o.particles; o.particles = null; assert.throws(() => h.bolt(1, [2, 3], [4, 5], 0.4), /표시시스템 미설치/); const stop = new Error('현재 실제 입자 경계'); o.particles = { addScreenBolt: (...args) => { calls.push(['bolt', ...args]); throw stop; } }; assert.throws(() => h.bolt(1, [2, 3], [4, 5], 0.4), (error) => error === stop); o.particles = particles;
        assert.deepEqual(calls, [['play', 0x76543210, 0, 0, 0, 1, 0], ['play', free, 0, 0, 0, 1, 0], ['at', 1.25, 2.5, thunder, 0, 0], ['bolt', 1, [2, 3], [4, 5], 0.4]]);
    } finally { o.dispose(); }
});

test('Session ContentLifetime: Sound47은 같은 실제 예약 객체를 한 번 부착하고 현재 Form/지연을 조회한다', () => {
    const o = scene(); try { const root = parent(o); const h = o.contentModules.carrierEffects; assert.throws(() => h.allocate(39), /예약크기/); assert.equal(h.allocate(40), 1); const reserved = o.pendingPickupSound; assert.ok(reserved instanceof SoundProcess); assert.equal(reserved.form, 0); assert.throws(() => h.allocate(40), /중첩/); assert.throws(() => h.create(2, root, 1, 2, 12), /실제예약/); assert.equal(o.pendingPickupSound, reserved);
        const form = h.create(1, root, 123, 456, 12); assert.ok(form > 0); assert.equal(o.pendingPickupSound, null); assert.equal(o.core.processes.fromForm(form), reserved); assert.deepEqual([reserved.sound, reserved.alternate, reserved.flags, o.core.pool.slot(form)[10]], [123, 456, 12, o.state.soundState.currentType()]); assert.throws(() => h.create(1, root, 1, 2, 3), /실제예약/); assert.throws(() => reserved.initializeUnattached(1, 2, 3), /미부착 예약/);
        o.sounds.hooks.wallSeconds = () => 100.25; h.duration(form, 0.1); assert.equal(reserved.startTime, 100.25 + Math.fround(0.1)); assert.throws(() => h.duration(65536, 0), /Form번호범위/); assert.throws(() => h.duration(root, 0), /현재객체 누락/);
    } finally { o.dispose(); }
});

test('Session ContentLifetime: 종속 UI 사건은 현재 전역이며 큐/진단 함수 사본과 signed32 인자를 보존한다', () => {
    const calls = []; const o = scene({ assets: { queueCommand: (...args) => calls.push(['queue', ...args]) }, ai: { assertion: (expression) => calls.push(['assert', expression]) } });
    try { const root = parent(o); const form = o.gumpForms.create(2); put(o.core.pool.physical(root), 6, form, 2); o.services.assets.queueCommand = forbidden; o.services.ai.assertion = forbidden; o.containedUnpop.notifyUi(root); o.contentUnpopEvent = 0xfedcba98; o.containedUnpop.notifyUi(root); o.contentModules.unpopHooks.command(0xabcdef01, 0x80000000, 0xffffffff); o.contentModules.unpopHooks.assertion('same', 'ignored', 1);
        assert.deepEqual(calls, [['queue', 0x02000039, root, 0], ['queue', 0xfedcba98, root, 0], ['queue', 0xabcdef01, -2147483648, -1], ['assert', 'same']]);
    } finally { o.dispose(); }
});

test('Session ContentLifetime: 내용물 생성은 현재 VT3을 공통 Gump 부착으로 선택하고 TYPE6으로 고정하지 않는다', () => {
    const assertions = []; const o = scene({ ai: { assertion: (expression) => assertions.push(expression) } }); try { const root = parent(o); o.contentFinder.containedType = 3; const form = o.contained.create(root, 94, 7, 0x20, 0x10, 8); assert.equal(o.core.pool.slot(form)[10], 3); assert.equal(o.gumpForms.parent(form), root); assert.equal(get(o.core.pool.slot(root), 6, 2), form); assert.deepEqual(assertions, ['getType()!=contentFormType']); } finally { o.dispose(); }
});

test('Session ContentLifetime: 미부착 Sound 예약은 부모 반납에서 사라지고 이후 호출은 수명 밖 오류다', () => {
    const o = scene(); o.contentModules.carrierEffects.allocate(40); const reserved = o.pendingPickupSound; o.dispose(); assert.equal(o.pendingPickupSound, null); assert.equal(reserved.form, 0); assert.equal(reserved.pid, 0); assert.throws(() => o.contentModules.carrierEffects.create(1, 5, 1, 2, 3), /반납/);
});
