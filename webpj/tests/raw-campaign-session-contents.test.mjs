// InstallContents3020..3049의 실제 통지 접두/현재 공유 값/선택된 실패를 대조한다(WB04-E596).
// 보호 자산과 실제 기존 몸체를 사용한다. 전체 Session/수명/게임 또는 실제 포획 관측은 아니다.
import './nan-exact.mjs';
import test from 'node:test';
import assert from 'node:assert/strict';
import { GameAssets } from '../www/assets/game-assets.js';
import { GamePalette } from '../www/assets/palette.js';
import { SoundList } from '../www/audio/sound-list.js';
import { RawCampaignSessionOwners } from '../www/core/raw-campaign-session-owners.js';
import { GumpFormHost } from '../www/core/gump-form-host.js';
import { RawPlayerContentNotice } from '../www/core/raw-player-content-notice.js';
import { RawContentParentNotice } from '../www/core/content-parent-notice.js';
import { RawPriestTakenNotice } from '../www/core/raw-priest-taken-notice.js';
import { SQUID_DESTROY_EFFECT } from '../www/core/squid-destroy.js';
import { realCaptureAssets } from './capture-assets-support.mjs';
import { readData } from './helpers.mjs';
import { put, floatToBits } from './raw-support.mjs';

// 선택하지 않은 실제 외부 장치는 빈 성공으로 대신하지 않는다.
function forbidden() { throw new Error('선택하지 않은 외부 효과'); }
// 보호 자산과 같은 팔레트다. 현재 TYPE를 시험용 기대값으로 덮어쓰지 않는다.
const data = realCaptureAssets(); const game = new GameAssets(data.table, data.database, GamePalette.parse(readData('d/GIFCLOUD.COL')));
// 같은 기반 owner만 만든다. 런타임/내용물 설치는 각 검사가 선택한다.
function owner(extra = {}) { return new RawCampaignSessionOwners(game, { read: forbidden }, 'thewarbegins', { unixSeconds: () => 1700000000, soundNames: new SoundList(), sound: { wallSeconds: () => 0, play: forbidden, playOnceAt: forbidden, playLoopAt: forbidden, stop: forbidden, isPlaying: forbidden }, ...extra }); }
// 원문 생성 순서의 앞 구간을 실제 모듈들로 연결한다.
function before(o) { o.initializePlayersAndSp(); o.connectPlayerTablesAndPackets(); o.connectIslandsAndPriests(); o.connectPlayersAndOwnership(); o.connectBodiesAndTypeTables(); o.connectGunAndCursorViews(); o.connectHitBodies(); o.connectGunBodies(); o.connectParticles(); o.connectTeleport(); o.connectCombat(); o.connectVortex(); o.connectAssets(); o.connectWorldRuntimeHooks(); }
// 통지까지 연결한 실제 owner다. 전체 Session을 게시하지 않는다.
function scene(extra = {}) { const o = owner(extra); before(o); o.connectContentNotices(); return o; }
// Player 통지 입력용 실제 SID와 같은 물리 바이트다. World에는 Pop하지 않는다.
function parent(o) { const sid = o.world.create(75, 0); put(o.core.pool.allocated(sid), 0, 0x50ee08); return sid; }
// 실제 Gump 생성/소유 ID를 사용하고, UI 체인의 지정 입력은 같은 물리 head/next에 쓴다.
function ui(o, root, id) { const sid = o.gumpForms.create(2); o.gumpForms.setOwner(sid, id); put(o.core.pool.physical(root), 6, sid, 2); return sid; }

test('Session Contents: Runtime 앞/반복/반납 관문과 같은 실제 모듈·World 미공개를 보존한다', () => {
    const o = owner();
    try { assert.throws(() => o.connectContentNotices(), /초기화 전/); before(o); const bytes = o.core.pool.bytes(); const seed = o.random.state; o.connectContentNotices();
        assert.equal(o.contentModules.noticesReady, true); assert.ok(o.gumpForms instanceof GumpFormHost); assert.ok(o.playerContent instanceof RawPlayerContentNotice); assert.ok(o.contentNotice instanceof RawContentParentNotice); assert.ok(o.priestTakenNotice instanceof RawPriestTakenNotice);
        assert.equal(o.gumpForms.pool, o.core.pool); assert.equal(o.playerContent.pool, o.core.pool); assert.equal(o.priestTakenNotice.pool, o.core.pool); assert.deepEqual(o.core.pool.bytes(), bytes); assert.equal(o.random.state, seed); assert.equal(o.world.deletionState.silentType, 147); assert.equal(o.lifetime, null);
        assert.throws(() => o.core.forms.bind(o.gumpForms), /중복/); assert.throws(() => o.gameplayGlobals.requireSession(), /미연결/); assert.throws(() => o.connectContentNotices(), /반복/);
    } finally { o.dispose(); }
    assert.throws(() => o.connectContentNotices(), /반납/); assert.throws(() => o.contentModules.takenHooks.wallSeconds(), /반납/);
});

test('Session Contents: Gump 소유 통지는 현재 서비스로 가고 ID0은 조회를 생략한다', () => {
    const calls = []; const o = scene({ gumpNotice: (...args) => calls.push(['first', ...args]) });
    try { const sid = o.gumpForms.create(2); put(o.core.pool.physical(sid), 16, 0x1234, 2); o.gumpForms.notify(sid); assert.deepEqual(calls, []); o.gumpForms.setOwner(sid, 0xfedcba98); o.gumpForms.notify(sid);
        o.services.gumpNotice = (...args) => calls.push(['next', ...args]); o.gumpForms.notify(sid); assert.deepEqual(calls, [['first', 0xfedcba98, 0x1234], ['next', 0xfedcba98, 0x1234]]);
        o.services.gumpNotice = null; assert.throws(() => o.gumpForms.notify(sid), /GumpForm UI통지미연결/);
    } finally { o.dispose(); }
});

test('Session Contents: 공통 삭제는 같은 현재 선택/파생 효과로 연결하고 초기 emit 참조를 보존한다', () => {
    const calls = []; const deletion = { selected: () => 7, emit: (event) => calls.push(event) }; const o = owner({ deletion }); deletion.emit = forbidden;
    try { before(o); const previous = o.world.commonDeletion; const state = o.world.deletionState; o.connectContentNotices(); assert.notEqual(o.world.commonDeletion, previous); assert.equal(o.world.deletionState, state);
        o.services.deletion.emit = forbidden; o.core.surfaces.hooks().emit({ effect: SQUID_DESTROY_EFFECT.clearSelection, sid: 23 });
        assert.deepEqual(calls, [{ effect: SQUID_DESTROY_EFFECT.clearSelection, sid: 23, parent: 0, flags: 0 }]); assert.throws(() => o.world.bindDeletion(o.hp, () => 0, {}), /한 번/);
    } finally { o.dispose(); }
});

test('Session Contents: 삭제 설치 전 실제 객체가 있으면 Gump 등록 뒤에서 실패하고 재시도 성공으로 숨기지 않는다', () => {
    const o = owner(); before(o); const sid = o.world.create(75, 0); const bytes = o.core.pool.slot(sid); const previous = o.world.commonDeletion;
    try { assert.throws(() => o.connectContentNotices(), /첫 객체 생성 전/); assert.ok(o.gumpForms instanceof GumpFormHost); assert.throws(() => o.core.forms.bind(o.gumpForms), /중복/); assert.equal(o.contentModules.noticesReady, false); assert.equal(o.world.commonDeletion, previous); assert.equal(o.playerContent, null); assert.deepEqual(o.core.pool.slot(sid), bytes); assert.throws(() => o.connectContentNotices(), /반복/); } finally { o.dispose(); }
});

test('Session Contents: Player UI 함수는 값 복사하며 현재 TYPE/사건·다음 링크의 조회 순서를 유지한다', () => {
    const calls = []; const hooks = { money: forbidden, gump: (id) => { calls.push(['lookup', id]); return 0x7788; }, handle: (...args) => calls.push(['handle', ...args]) }; const o = owner({ playerContent: hooks }); hooks.gump = forbidden; hooks.handle = forbidden;
    try { before(o); o.connectContentNotices(); const root = parent(o); const first = ui(o, root, 0x12345678); const second = o.gumpForms.create(2); o.gumpForms.setOwner(second, 0xfedcba98); put(o.core.pool.physical(first), 4, second, 2); o.services.playerContent.gump = forbidden; o.services.playerContent.handle = forbidden;
        o.contentEvent = 0xabcdef01; o.playerContent.notify(root, 31, 2); assert.deepEqual(calls, [['lookup', 0x12345678], ['handle', 0x7788, 0xabcdef01, 31, 2], ['lookup', 0xfedcba98], ['handle', 0x7788, 0xabcdef01, 31, 2]]);
        calls.length = 0; o.contentGumpType = 9; o.playerContent.notify(root, 3, 4); assert.deepEqual(calls, []);
    } finally { o.dispose(); }
});

test('Session Contents: Player SP는 bit_cast float이고 PlayerKnown·현재 소유자·현재 표본/횟수를 공유한다', () => {
    const o = scene(); try { const root = parent(o); const patch = o.priestState.regen.patch; const diagnostic = o.gameplayGlobals.patchDiagnostic();
        patch.remaining = 2; patch.writeSample(0x11223344); diagnostic.value = 5; o.playerState.playerKnown = 0; o.sp.set(7, floatToBits(8001)); o.playerState.battleOwner = 7;
        o.playerContent.notify(root, 0, 0); assert.equal(patch.remaining, 1); assert.equal(diagnostic.value, 5);
        o.playerState.playerKnown = 1; o.playerContent.notify(root, 0, 0); assert.equal(patch.remaining, 0); assert.equal(diagnostic.value, 0x11223344);
        patch.remaining = 1; diagnostic.value = 9; o.sp.set(7, floatToBits(6999)); o.playerContent.notify(root, 0, 0); assert.equal(diagnostic.value, 9);
    } finally { o.dispose(); }
});

test('Session Contents: Player는 SP Get 효과 뒤 현재 표본을 재조회하고 cursor 차이를 같은 진단에 기록한다', () => {
    const o = scene(); try { const root = parent(o); const patch = o.priestState.regen.patch; const diagnostic = o.gameplayGlobals.patchDiagnostic(); const get = o.sp.get.bind(o.sp); const calls = [];
        o.playerState.playerKnown = 1; o.playerState.battleOwner = 6; patch.remaining = 1; patch.writeSample(0x11); o.sp.set(6, floatToBits(9000));
        o.sp.get = (id) => { calls.push(id); patch.writeSample(0x22); return get(id); }; o.playerContent.notify(root, 0, 0); assert.deepEqual(calls, [6]); assert.equal(diagnostic.value, 0x22);
        o.sp.get = get; patch.writeSample(0x33); diagnostic.value = 0; patch.sequence = (o.sp.offset + 1) | 0; o.playerContent.notify(root, 0, 0); assert.equal(diagnostic.value, 0x33);
    } finally { o.dispose(); }
});

test('Session Contents: 빈 Player UI 장치는 생성 때 성공 대역을 만들지 않고 선택 지점에서 실패한다', () => {
    const o = scene(); try { const root = parent(o); o.playerContent.notify(root, 0, 0); ui(o, root, 0x1234); assert.throws(() => o.playerContent.notify(root, 1, 2), /cpp:3030/); } finally { o.dispose(); }
    const second = scene({ playerContent: { gump: () => 3 } }); try { const root = parent(second); ui(second, root, 1); assert.throws(() => second.playerContent.notify(root, 1, 2), /cpp:3030/); } finally { second.dispose(); }
});

test('Session Contents: 내용물 부모는 실제 현재VT와 호출 때 설치된 같은 수명 포인터를 선택한다', () => {
    const o = scene(); try { const root = parent(o); assert.throws(() => o.worldHooks.contentNotice(root, 11, 12), /수명 부모 미설치/); const calls = [];
        o.lifetime = { parentNotice: (...args) => calls.push(['first', ...args]) }; o.worldHooks.contentNotice(root, 13, 14); o.lifetime = { parentNotice: (...args) => calls.push(['next', ...args]) }; o.contentNotice.notice(root, 15, 16);
        assert.deepEqual(calls, [['first', root, 13, 14], ['next', root, 15, 16]]); put(o.core.pool.physical(root), 0, 0x501dd0); o.lifetime = null; o.contentNotice.notice(root, 17, 18);
    } finally { o.dispose(); }
});

test('Session Contents: Form 상태는 현재 finder/battle/checking을 조회하며 진단은 현재 서비스에 표현식만 넘긴다', () => {
    const calls = []; const o = scene({ ai: { assertion: (text) => calls.push(['first', text]) } });
    try { const s = o.contentFormState; o.contentFinder.containedType = 19; o.core.ownerMode.battle = false; o.gameplayGlobals.aiDebug().checking().value = -3; assert.deepEqual([s.contentType(), s.inBattle(), s.checkingLinks()], [19, false, -3]);
        s.assertion('first', 'ignored', 71); o.services.ai.assertion = (text) => calls.push(['next', text]); s.assertion('second', 'ignored', 72); assert.deepEqual(calls, [['first', 'first'], ['next', 'second']]); o.services.ai.assertion = null; assert.throws(() => s.assertion('x', 'y', 73), /진단 장치 미연결 73/);
    } finally { o.dispose(); }
});

test('Session Contents: 포획 안내 하위 경계는 현재 참여/벽시각/Tell에 원문 인자를 전달한다', () => {
    const calls = []; const o = scene({ factoryTell: (text) => calls.push(['tell', text]) });
    try { const h = o.contentModules.takenHooks; const original = o.priestParticipation.refresh; o.priestParticipation.refresh = (owner) => calls.push(['refresh', owner]); h.refresh(4); o.priestParticipation.refresh = original;
        o.services.sound.wallSeconds = () => 123.5; assert.equal(h.wallSeconds(), 123.5); h.tell('WarnPriestTaken'); assert.deepEqual(calls, [['refresh', 4], ['tell', 'WarnPriestTaken']]);
        o.services.sound.wallSeconds = null; assert.throws(() => h.wallSeconds(), /벽시각 미연결/); o.services.factoryTell = null; assert.throws(() => h.tell('selected'), /Tell 미연결/);
    } finally { o.dispose(); }
});

test('Session Contents: 포획 이름은 현재64BYTE의 NUL 앞 원문 바이트이며 누락/비종결을 구별한다', () => {
    const calls = []; const o = scene(); try { const h = o.contentModules.takenHooks; assert.throws(() => h.name(3), /Player 이름 미연결/);
        o.services.priestTakenName = (owner, handle) => { calls.push([owner, handle]); return 'actual-name'; }; o.handle.set([0x41, 0xe9, 0, 0x42]); assert.equal(h.name(3), 'actual-name'); assert.deepEqual(calls, [[3, 'Aé']]);
        o.handle[0] = 0x43; h.name(4); assert.deepEqual(calls[1], [4, 'Cé']); o.handle.fill(0x41); assert.throws(() => h.name(5), /NUL 누락/); assert.equal(calls.length, 2);
    } finally { o.dispose(); }
});

test('Session Contents: 포획 소리는 현재 이름표와 전역 재생의6인자를 사용하며 미선택 장치는 호출하지 않는다', () => {
    const calls = []; const o = scene(); try { const h = o.contentModules.takenHooks; o.services.sound.play = (...args) => calls.push(args); const sound = o.soundNames.lookup('ourPriestCaptured.wav'); h.sound('ourPriestCaptured.wav'); assert.deepEqual(calls, [[sound, 0, 0, 0, 1, 0]]);
        o.services.sound.play = null; assert.throws(() => h.sound('enemyPriestCaptured.wav'), /전역 음향 미연결/); assert.equal(calls.length, 1);
    } finally { o.dispose(); }
});
