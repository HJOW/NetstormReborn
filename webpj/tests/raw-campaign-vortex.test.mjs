// Vortex.inc의 typed Session 계약을 같은 실제 World/Template/목록/Host로 대조한다(WB04-E589).
// 전용 원본 관측/직접 cpppj 시험은 없다. 전체 Load·자연 완주·포획/희생·공중3종 전용 검사가 아니다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { readData } from './helpers.mjs';
import { realCaptureAssets } from './capture-assets-support.mjs';
import { get, put, floatToBits } from './raw-support.mjs';
import { GameAssets } from '../www/assets/game-assets.js';
import { GamePalette } from '../www/assets/palette.js';
import { SECTION_NAMES } from '../www/assets/fort.js';
import { SoundList } from '../www/audio/sound-list.js';
import { RawCampaignSessionOwners } from '../www/core/raw-campaign-session-owners.js';
import { RawVortexAscend } from '../www/core/raw-vortex-ascend.js';
import { RawVortexDeparture } from '../www/core/raw-vortex-departure.js';
import { ClientSubscriberGlobals } from '../www/core/client-subscriber-globals.js';

// 실제 보호 자산과 현재188TYPE를 같은 owner에 빌린다.
const data = realCaptureAssets(); const game = new GameAssets(data.table, data.database, GamePalette.parse(readData('d/GIFCLOUD.COL')));
// 선택하지 않은 외부 효과를 빈 성공으로 바꾸지 않는다.
function forbidden() { throw new Error('선택되지 않은 외부 효과'); }
// 필수 실제 시각/음향 경계만 외부 입력으로 공급한다.
function owner(extra = {}) { return new RawCampaignSessionOwners(game, { read: forbidden }, 'thewarbegins', { unixSeconds: () => 1700000000, soundNames: new SoundList(), sound: { wallSeconds: () => 0, play: forbidden, playOnceAt: forbidden, playLoopAt: forbidden, stop: forbidden, isPlaying: forbidden }, ...extra }); }
// 앞 생성자 구간을 실제 원문 순서로 연결한다. Session을 Runtime에 공개하지 않는다.
function before(o) { o.initializePlayersAndSp(); o.connectPlayerTablesAndPackets(); o.connectIslandsAndPriests(); o.connectPlayersAndOwnership(); o.connectBodiesAndTypeTables(); o.connectGunAndCursorViews(); o.connectHitBodies(); o.connectGunBodies(); o.connectParticles(); o.connectTeleport(); o.connectCombat(); }
// 이번 생성자1537까지의 실제 모듈을 조립한다.
function scene(extra = {}) { const o = owner(extra); before(o); o.connectVortex(); return o; }
// 순수 명령 입력의 실제 생성 슬롯이다. 미션 객체 Pop/플레이를 실행하지 않는다.
function actor(o, type = 74, owner = 1, x = 20, y = 20) { const sid = o.world.create(type, 2); const raw = o.core.pool.allocated(sid); raw[11] &= 0xfc; raw[34] = owner; put(raw, 14, floatToBits(x)); put(raw, 18, floatToBits(y)); return sid; }
// 같은 실제 섬0과 hash0의 표면 WORD를 연결한다. Fort 로드 완료의 대체물이 아니다.
function surface(o, territory = 7) { o.islands.reset(); o.islands.add(1, territory); const sid = actor(o, 94); put(o.core.pool.allocated(sid), 8, 0, 2); o.core.hash.setCell(0, 20, 20, sid); return sid; }
// 게임 틱을 돌리지 않고 같은 Kernel에 부착된 정기 처리기만 읽는다.
function regulars(o, parent) { const found = []; for (let pid = 1; pid <= o.core.kernel.capacity; ++pid) { const p = o.core.kernel.get(pid); if (p?.parent === parent) found.push(p); } return found; }

test('Session Vortex: 선행/한 번 결선·같은 Pool/현재 noIsland·난수와 반납을 유지한다', () => {
    const o = owner();
    try { assert.throws(() => o.connectVortex(), /초기화 전/); before(o); const seed = o.random.state; o.connectVortex(); assert.ok(o.vortexAscend instanceof RawVortexAscend); assert.ok(o.vortexDeparture instanceof RawVortexDeparture); assert.equal(o.vortexAscend.pool, o.core.pool); assert.equal(o.vortexDeparture.pool, o.core.pool); assert.equal(o.random.state, seed); o.core.bridgeState.noIslandType = 156; assert.equal(o.vortexModules.ascendState.sharedNoIslandType.value, 156); assert.equal(o.vortexModules.ready, true); assert.throws(() => o.connectVortex(), /반복/); assert.throws(() => o.gameplayGlobals.requireSession(), /미연결/); } finally { o.dispose(); }
    assert.throws(() => o.connectVortex(), /반납/); assert.throws(() => o.vortexModules.command(1, 0, 0), /반납/);
});

test('Session Vortex: 다른 Pool의 섬은 후보 상태 연결 뒤 실패하고 재시작을 거부한다', () => {
    const o = owner(); const other = owner();
    try { before(o); before(other); o.islands = other.islands; assert.throws(() => o.connectVortex(), /풀 불일치/); assert.ok(o.vortexModules.ascendState.sharedNoIslandType); assert.equal(o.vortexAscend, null); assert.equal(o.vortexModules.ready, false); assert.throws(() => o.connectVortex(), /반복/); } finally { o.dispose(); other.dispose(); }
});

test('Session Vortex: 현재 handle64를 빌리고 이름 진단/설정 함수 사본과 NUL 포함 바이트를 보존한다', () => {
    const globals = new ClientSubscriberGlobals(); globals.handle().set([0xe9, 65, 0]); const calls = []; const install = { setConfig: (...args) => calls.push(['A', ...args]) };
    const o = scene({ subscriberGlobals: globals, priestTakenName: (owner, handle) => { calls.push(['name', owner, handle]); return '\xe9X'; }, install, factoryTell: (name) => calls.push(['tell', name]) });
    try { install.setConfig = forbidden; assert.equal(o.handle, globals.handle()); o.vortexModules.departureHooks.warnAscend(2); assert.deepEqual(calls, [['name', 2, '\xe9A'], ['A', 'global.subject', Uint8Array.of(0xe9, 88, 0)], ['tell', 'WarnAscend']]); globals.handle().fill(65); assert.throws(() => o.vortexModules.playerName(2), /NUL 누락/); o.services.priestTakenName = null; assert.throws(() => o.vortexModules.playerName(2), /이름 장치 미연결/); } finally { o.dispose(); }
});

test('Session Vortex: WORD를 먼저 쓰고 현재 GameAux TYPE일 때만 실제 표면 통지를 실행한다', () => {
    const o = scene();
    try { const sid = actor(o); const aux = o.gameplayGlobals.gameAux(); const calls = []; const notify = o.core.building.notifySurface.bind(o.core.building); o.core.building.notifySurface = (value) => { calls.push([value, get(o.core.pool.slot(value), 12, 2)]); notify(value); }; o.vortexModules.setState(sid, 0x12345); assert.deepEqual(calls, []); assert.equal(get(o.core.pool.slot(sid), 12, 2), 0x2345); aux.stateNotifyType = 74; o.vortexModules.setState(sid, 0x9876); assert.deepEqual(calls, [[sid, 0x9876]]); aux.stateNotifyType = 82; o.vortexModules.setState(sid, 1); assert.equal(calls.length, 1); } finally { o.dispose(); }
});

test('Session Vortex: 같은 Template의 공유 writer와 현재 소유 섬 조회·선택 누락을 사용한다', () => {
    const o = scene();
    try { const h = o.vortexModules.departureHooks; o.templateStorage.install(1, { sections: SECTION_NAMES.map(() => new Uint8Array(0)) }, 'mission.fort'); const records = Uint8Array.from({ length: 120 }, (_, n) => n); h.writeTerritory(1, records); assert.deepEqual(o.templateStorage.selectTerritory(1).bytes, records); assert.throws(() => h.readTerritory(1, records), /reader 미설치/); assert.throws(() => h.snapshot(), /snapshot 부모 미설치/); assert.throws(() => h.refreshProduction(), /생산 갱신 미연결/); assert.throws(() => h.requestEdit(), /웹 제외/); o.islands.reset(); o.islands.add(2, 0); o.islands.add(2, 1); assert.equal(h.ownedIslands(2), 2); o.islands.delete(0); assert.equal(h.ownedIslands(2), 1); } finally { o.dispose(); }
});

test('Session Vortex: 후보80BYTE는 중첩 수명별로 보관하고 초과 반납을 거부한다', () => {
    const o = scene();
    try { const h = o.vortexModules.ascendHooks; assert.equal(h.reserveCandidateBuffer(), true); const first = o.vortexModules.candidateBuffers[0]; assert.equal(first.length, 80); assert.equal(h.reserveCandidateBuffer(), true); assert.notEqual(first, o.vortexModules.candidateBuffers[1]); h.releaseCandidateBuffer(); assert.equal(o.vortexModules.candidateBuffers[0], first); h.releaseCandidateBuffer(); assert.throws(() => h.releaseCandidateBuffer(), /수명 불일치/); } finally { o.dispose(); }
});

test('Session Vortex Descend: Fort/지역 진단 뒤의 현재 서버·메타 명령을 두 동일 DWORD로 큐에 보낸다', () => {
    const calls = []; const ns = { serverSelection: 3 }; const o = scene({ nsInfoGlobals: ns, developerQueue: (...args) => calls.push(args) });
    try { surface(o, -123); o.core.ownerMode.fort = false; o.world.storage.bindDisplayDiagnostics({ value: '' }, (expression) => { assert.equal(expression, 'inFortMode'); ns.serverSelection = 0x8001; o.localStateGlobals.viewMetaCommand().value = 0x23456789; }); o.vortexModules.queueDescend(20.5, 20.9); assert.deepEqual(calls, [[0x23456789, 0xffffff85, 0xffffff85]]); o.services.developerQueue = null; assert.throws(() => o.vortexModules.queueDescend(20, 20), /일반 입력 큐 미연결/); } finally { o.dispose(); }
});

test('Session Vortex Priest: 실제 공유 목록의 첫 살아 있는 사제만 선택하고 조회 뒤 free를 다시 읽는다', () => {
    const o = scene();
    try { surface(o); const temple = actor(o, 74, 2); assert.equal(o.vortexModules.priestOnIsland(temple, 2), false); const first = actor(o, 74, 2); const second = actor(o, 74, 2); const list = o.priestState.priests.currentPriests(); list.append(first); list.append(second); assert.equal(o.vortexModules.priestOnIsland(temple, 2), true); const island = o.ownership.islandNumber.bind(o.ownership); let calls = 0; o.ownership.islandNumber = (...args) => { ++calls; o.core.pool.physical(first)[11] |= 1; return island(...args); }; assert.equal(o.vortexModules.priestOnIsland(temple, 2), false); assert.equal(calls, 1); o.ownership.islandNumber = island; o.core.pool.physical(first)[11] |= 2; assert.equal(o.vortexModules.priestOnIsland(temple, 2), true); } finally { o.dispose(); }
});

test('Session Vortex request: 실제 방송 후 같은 변경 패킷을 직접 receiver9에 넘긴다', () => {
    const calls = []; const peers = new Array(9).fill(null); const o = scene({ battlePeers: peers, player: { log: (...args) => calls.push(['log', ...args]) } });
    try { surface(o); const temple = actor(o); o.playerState.battleOwner = 1; o.players[2 * 180 + 0x50] |= 1; o.services.battlePeers[2] = (bytes) => { calls.push(['send', [...bytes]]); bytes[4] = 9; }; o.vortexModules.requestDeparture(temple, 256); assert.deepEqual(calls, [['send', [1, 0x51, 8, 0, 0, temple & 255, temple >>> 8, 1]], ['log', 29, "Can't ascend island 9\n"]]); } finally { o.dispose(); }
});

test('Session Vortex 51h: 실제 현재 owner·Regular3·이름/설정·두 소리 순서로 이어진다', () => {
    const calls = []; const o = scene({ priestTakenName: (owner) => { calls.push(['name', owner]); return 'enemy'; }, install: { setConfig: (key, bytes) => calls.push(['config', key, [...bytes]]) }, factoryTell: (section) => calls.push(['tell', section]), sound: { wallSeconds: () => 0, play: (...args) => calls.push(['sound', ...args]), playOnceAt: forbidden, playLoopAt: forbidden, stop: forbidden, isPlaying: forbidden } });
    try { surface(o); const temple = actor(o, 74, 2); o.playerState.battleOwner = 1; o.core.processState.server = false; o.vortexDeparture.receive(9, Uint8Array.of(1, 0x51, 8, 0, 0, temple, temple >>> 8, 0)); assert.deepEqual(regulars(o, temple).map((p) => [p.event, p.payload]), [[3, 0]]); assert.deepEqual(calls, [['name', 2], ['config', 'global.subject', [101, 110, 101, 109, 121, 0]], ['tell', 'WarnAscend'], ['sound', o.soundNames.lookup('enemyIslandPreparing.wav'), 0, 0, 0, 0, 0], ['sound', o.soundNames.lookup('islandRise.wav'), 0, 0, 0, 0, 0]]); } finally { o.dispose(); }
});

test('Session Vortex command: 진단/확인 관문·누락 Tell과 소리/설치 전 송신의 선행 WORD를 보존한다', () => {
    const calls = []; const o = scene({ sound: { wallSeconds: () => 0, play: (...args) => calls.push(['sound', ...args]), playOnceAt: forbidden, playLoopAt: forbidden, stop: forbidden, isPlaying: forbidden } });
    try { const temple = actor(o); o.world.storage.bindDisplayDiagnostics({ value: '' }, (...args) => calls.push(['assert', ...args])); assert.equal(o.vortexModules.command(temple, 0xff00006f, 0), 1); assert.deepEqual(calls.shift(), ['assert', '0', '\\Ns\\O\\Vortex.cpp', 0x2ea]); assert.equal(o.vortexModules.command(temple, 0x0300007c, 2), 1); assert.equal(get(o.core.pool.slot(temple), 12, 2), 0); assert.throws(() => o.vortexModules.command(temple, 0x0300007c, 0), /설정\/Tell 장치 미연결/); surface(o); assert.throws(() => o.vortexModules.command(temple, 0x0300007c, 1), /실제 설치 전/); assert.equal(get(o.core.pool.slot(temple), 12, 2), 1); assert.deepEqual(calls, [['sound', o.soundNames.lookup('preparingAscend.wav'), 0, 0, 0, 0, 0]]); assert.throws(() => o.vortexModules.command(temple, 0x030000a7, 0), /ObjectCommand 미설치/); put(o.core.pool.allocated(temple), 12, 0, 2); assert.equal(o.vortexModules.command(temple, 0x030000a7, 0), 1); assert.deepEqual(regulars(o, temple).map((p) => [p.event, p.payload]), [[4, 0]]); } finally { o.dispose(); }
});

test('Session Vortex command: 그림 문자열/공통 위임·현재 Login의 mutable 패킷 계약을 유지한다', () => {
    const calls = []; const o = scene({ install: { setConfig: (key, bytes) => calls.push([key, [...bytes]]) }, factoryTell: (section) => calls.push([section]) });
    try { const temple = actor(o); assert.equal(o.vortexModules.command(temple, 0x0300007c, 0), 1); assert.deepEqual(calls, [['global.param1', [...Buffer.from(String(temple | 0x10000)), 0]], ['PriestOffIsland']]); o.objectCommand = { command: (...args) => { calls.push(args); return 0x12345678; } }; assert.equal(o.vortexModules.command(temple, 0xfe000001, 0), 0x12345678); assert.deepEqual(calls.at(-1), [temple, 0xfe000001]); o.loaded = true; const bytes = Uint8Array.of(1, 2, 3); assert.throws(() => o.vortexModules.sendChatMutable(9, bytes), /실제 로컬 전송 없는/); o.login = { sendMutable: (target, packet) => { assert.equal(packet, bytes); calls.push([target, [...packet]]); } }; o.vortexModules.sendChatMutable(0xffffffff, bytes); assert.deepEqual(calls.at(-1), [-1, [1, 2, 3]]); } finally { o.login = null; o.dispose(); }
});
