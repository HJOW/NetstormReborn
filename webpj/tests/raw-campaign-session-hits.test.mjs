// Session1340..1397의 실제 피해/AI·현재 상태·효과 기본값을 같은 보호 자산/World에서 대조한다(WB04-E581).
// 고정 수치 입력의 소스 계약 검사이며 실제 게임 교전/특정 적 공격·공중3종 전용 행동·새 원격MP 관측이 아니다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { GameAssets } from '../www/assets/game-assets.js';
import { SoundList } from '../www/audio/sound-list.js';
import { RawCampaignSessionOwners } from '../www/core/raw-campaign-session-owners.js';
import { rawPathSurfaceAt } from '../www/core/raw-path-surface.js';
import { realCaptureAssets } from './capture-assets-support.mjs';
import { get, put, floatToBits } from './raw-support.mjs';

// 읽기 전용 보호 자료와 생성 구간에서 읽지 않는 실제 VFS 경계다.
const data = realCaptureAssets(); const game = new GameAssets(data.table, data.database); const files = { read() { throw new Error('생성 구간의 VFS 읽기 금지'); } };
// 선택하지 않은 효과가 조용히 성공하지 못하게 한다.
function forbidden() { throw new Error('선택되지 않은 외부 장치'); }
// 실제 외부 소리 요청만 기록한다. 게임 계산/저장소는 실제 owner가 소유한다.
function scene(extra = {}, events = []) {
    const sound = { wallSeconds: () => 100, playLoopAt: forbidden, playOnceAt: (...args) => events.push(['once', ...args]), play: (...args) => events.push(['play', ...args]), stop: forbidden, isPlaying: forbidden, ...extra.sound };
    return new RawCampaignSessionOwners(game, files, 'thewarbegins', { unixSeconds: () => 1700000000, soundNames: new SoundList(), ...extra, sound });
}
// 실제 앞 생성 구간까지만 연결한다. 부분 실패 검사에도 같은 순서를 사용한다.
function beforeHits(o) { o.initializePlayersAndSp(); o.connectPlayerTablesAndPackets(); o.connectIslandsAndPriests(); o.connectPlayersAndOwnership(); o.connectBodiesAndTypeTables(); o.connectGunAndCursorViews(); }
// 실제 이번 피해 생성 구간까지 연결한다.
function connect(o) { beforeHits(o); o.connectHitBodies(); }
// 실제 생성자의 같은 슬롯에 고정 HP/owner/좌표 입력만 설치한다. Pop/교전은 대신하지 않는다.
function target(o, type, owner = 2, hp = 10000) { const sid = o.world.create(type, 2); const raw = o.core.pool.allocated(sid); raw[34] = owner; put(raw, 26, hp); put(raw, 14, floatToBits(21)); put(raw, 18, floatToBits(21)); return sid; }
// 기존 AI 피해 본체 계약의 네 실제 목록을 같은 메모리 owner에 입력으로 확보한다.
function brain(o, victim, logging = 1) {
    const token = o.aiMemory.allocate(0xb08); const raw = o.aiMemory.bytes(token);
    // 실제 pointer/capacity/count 세 DWORD를 각 목록의 독립 backing에 연결한다.
    for (const offset of [0x360, 0x36c, 0x378, 0x384]) { const block = o.aiMemory.allocate(16); put(raw, offset, block); put(raw, offset + 4, 4); put(raw, offset + 8, 0); }
    put(raw, 0x384 + 8, 1); put(o.aiMemory.bytes(get(raw, 0x384)), 0, victim); put(raw, 0x260, -2); put(raw, 0xb00, logging); return token;
}

test('Session 피격 구성: 앞 단계·실제 Pool/하위 몸체·미공개/반복/반납 관문을 유지한다', () => {
    const o = scene();
    try {
        assert.throws(() => o.connectHitBodies(), /초기화 전/); assert.equal(o.aiDamage, null); connect(o); assert.equal(o.hitModules.ready, true); assert.equal(o.aiDamage.pool, o.core.pool); assert.equal(o.hits.pool, o.core.pool);
        // 실제 다섯 하위 몸체가 모두 같은 World를 읽는다.
        for (const body of [o.hits.priestDamage, o.hits.generalDamage, o.hits.burning, o.hits.dudes, o.hits.impact]) assert.equal(body.pool, o.core.pool);
        assert.throws(() => o.connectHitBodies(), /반복/); assert.throws(() => o.gameplayGlobals.requireSession(), /미연결/);
    } finally { o.dispose(); }
    assert.throws(() => o.connectHitBodies(), /반납/);
});

test('Session 피해 상태: 현재 미션/Server/로컬 owner BYTE/Player 주 섬과 같은 억제 double을 읽는다', () => {
    const o = scene();
    try {
        connect(o); const s = o.state; o.globals.missionPost = 1; assert.equal(s.priestDamageState.isCorrectionDisabled(), true); o.globals.missionPost = 0; assert.equal(s.priestDamageState.isCorrectionDisabled(), false);
        o.core.processState.server = false; assert.equal(s.generalDamageState.isServer(), false); o.core.processState.server = true; assert.equal(s.generalDamageState.isServer(), true);
        o.playerState.battleOwner = 0x102; assert.equal(s.generalDamageState.currentLocalOwner(), 2); put(o.players, 2 * 0xb4 + 0x78, -7); assert.equal(s.generalDamageState.primaryIsland(2), -7); put(o.players, 2 * 0xb4 + 0x78, 31); assert.equal(s.generalDamageState.primaryIsland(2), 31);
        assert.equal(s.impactState.sharedSuppressedTime, o.localStateGlobals.loginEnd()); o.loginEnd = 12.75; assert.equal(s.impactState.currentSuppressedTime, 12.75); o.hits.impact.visual(20, 30, 0, 0.1); assert.equal(o.core.kernel.size, 0);
        s.impactState.currentSuppressedTime = 0; assert.equal(o.loginEnd, 0); o.hits.impact.visual(20, 30, 0, 0.1); assert.equal(o.core.kernel.size, 1);
    } finally { o.dispose(); }
    assert.equal(o.core.kernel.size, 0);
});

test('Session 실제 일반 피해: 같은 HP DWORD와 Player AI0의 자연 반환을 사용한다', () => {
    const o = scene();
    try { connect(o); const victim = target(o, 75); const attacker = target(o, 71, 3); assert.equal(o.hits.damageResult(victim, 7, attacker), 1); assert.equal(get(o.core.pool.slot(victim), 26), 9993); o.hits.damageHooks().damage(victim, 9, 3, attacker); assert.equal(get(o.core.pool.slot(victim), 26), 9984); } finally { o.dispose(); }
});

test('Session 실제 사제 피해: 같은 HP·Regular 회복 취소와 TYPE 기본 프레임을 사용한다', () => {
    const o = scene();
    try {
        connect(o); const victim = target(o, 158); const before = get(o.core.pool.slot(victim), 26); o.globals.missionPost = 1; o.core.processes.addRegular(victim, 0x25c, 1); o.core.processes.addRegular(victim, 0x25d, 1);
        o.hits.priestDamage.hit(victim, 3, 3, 0); assert.equal(get(o.core.pool.slot(victim), 26), before - 3); assert.equal(o.core.processes.findEvent(victim, 0x25c), null); assert.equal(o.core.processes.findEvent(victim, 0x25d), null); assert.equal(get(o.core.pool.slot(victim), 36), o.projectileTypes[158].defaultFrame);
    } finally { o.dispose(); }
});

test('Session 실제 주 섬 경고: 현재 미션 억제와 기본 소리/Tell·현재 벽시각을 사용한다', () => {
    const events = []; let wall = 100; const o = scene({ sound: { wallSeconds: () => wall, play: (...args) => events.push(['play', ...args]) }, priest: { shield: { tell: (key) => events.push(['tell', key]) } }, hit: { general: { warningEvent: () => events.push(['remove']) } } });
    try {
        connect(o); o.world.load([{ type: 97, owner: 1, frame: 0, x: 21, y: 21 }]); const ground = rawPathSurfaceAt(o.core.hash, 21, 21); put(o.players, 0xb4 + 0x78, get(o.core.pool.slot(ground), 8, 2)); const victim = target(o, 75, 1);
        o.globals.missionPost = 1; o.hits.damageResult(victim, 1, 0); const name = o.soundNames.lookup('islandUnderAttack.wav'); assert.deepEqual(events, [['play', name, 0, 0, 0, 1, 0], ['tell', 'islandUnderAttack']]); assert.equal(o.state.generalDamageState.warningTime, 130);
        events.length = 0; wall = 200; o.globals.missionPost = 0; o.hits.damageResult(victim, 1, 0); assert.deepEqual(events, [['remove'], ['play', name, 0, 0, 0, 1, 0], ['tell', 'islandUnderAttack']]); assert.equal(o.state.generalDamageState.warningTime, 230);
    } finally { o.dispose(); }
});

test('Session Hit 서비스: 값 사본·제공 장치 보존과 무조건 현재 기본 프레임 덮어쓰기를 구별한다', () => {
    const events = []; const record = (...args) => events.push(args); const hit = { priest: { defaultFrame: forbidden, ownerDamageNotice: record }, general: { ownerDamageNotice: record, warningSound: record, warningText: record, realTime: () => 22.5 }, burning: { create: record, assertion: record, bulfSound: record }, vortex: { constructionType: () => 99, resetEnd: record, refresh: record }, dudes: { createBurning: record, pathEndpoint: record, startPath: record } }; const o = scene({ hit });
    try {
        hit.priest.ownerDamageNotice = forbidden; hit.general.warningSound = forbidden; hit.vortex.constructionType = forbidden; connect(o); const h = o.hitModules.effects; const victim = target(o, 75);
        assert.equal(h.priest.defaultFrame(victim), o.projectileTypes[75].defaultFrame); assert.equal(h.priest.ownerDamageNotice, record); assert.equal(h.general.warningSound, record); assert.equal(h.general.realTime(), 22.5); assert.equal(h.vortex.constructionType(), 99); assert.equal(h.dudes.startPath, record); h.priest.ownerDamageNotice(1, victim, 0); assert.deepEqual(events, [[1, victim, 0]]);
    } finally { o.dispose(); }
});

test('Session Player 피해 통지: AI0/중립/모드 반환과 현재 AI 토큰의 실제 메모리 검증을 사용한다', () => {
    const o = scene();
    try { connect(o); const h = o.hitModules.effects; h.general.ownerDamageNotice(0, 0, 123); h.priest.ownerDamageNotice(2, 0, 123); put(o.players, 2 * 0xb4 + 0xb0, 0x12345678); assert.throws(() => h.general.ownerDamageNotice(2, 0, 123), /AI.*메모리|할당|블록|토큰/); o.core.ownerMode.challenge = true; h.general.ownerDamageNotice(2, 0, 123); } finally { o.dispose(); }
});

test('Session 실제 AI 피해: 같은 본체/미움 목록과 현재 이름·signed owner·정수 좌표 로그를 연결한다', () => {
    const events = []; const o = scene({ ai: { allocationBytes: (bytes) => bytes.fill(0) }, player: { log: (...args) => events.push(args) } });
    try {
        connect(o); const victim = target(o, 75); const attacker = target(o, 71, 3); const a = o.core.pool.allocated(attacker); put(a, 14, floatToBits(-4.75)); put(a, 18, floatToBits(9.25)); const token = brain(o, victim); const raw = o.aiMemory.bytes(token); put(o.players, 2 * 0xb4 + 0xb0, token);
        o.hitModules.effects.general.ownerDamageNotice(2, victim, attacker); const name = o.core.types[71].name; assert.deepEqual(events, [[29, `AI-2: I hate ${name} at (-4,9)\n\n`], [29, `AI-2: I hate MORE ${name} at (-4,9)\n\n`]]);
        assert.equal(get(raw, 0x36c + 8), 1); assert.equal(get(o.aiMemory.bytes(get(raw, 0x36c)), 0), attacker); assert.equal(get(raw, 0x378 + 8), 1);
        events.length = 0; put(raw, 0xb00, 0); const next = target(o, 128, 3); o.playerDamage.notice(2, victim, next); assert.deepEqual(events, []); assert.equal(get(raw, 0x36c + 8), 2);
    } finally { o.dispose(); }
});

test('Session AI 로그 누락: 실제 앞선 미움 append를 남기고 선택한 출력에서 실패한다', () => {
    const o = scene({ ai: { allocationBytes: (bytes) => bytes.fill(0) } });
    try { connect(o); const victim = target(o, 75); const attacker = target(o, 71, 3); const token = brain(o, victim); assert.throws(() => o.aiDamage.notice(token, victim, attacker), /로그 출력 소비자 미연결/); assert.equal(get(o.aiMemory.bytes(token), 0x36c + 8), 1); assert.equal(get(o.aiMemory.bytes(token), 0x378 + 8), 0); } finally { o.dispose(); }
});

test('Session Burning 장치: 현재 AI 진단과 같은 SoundList의 실제 불프 위치 소리를 사용한다', () => {
    const events = []; const ai = { assertion: (text) => events.push(['A', text]) }; const o = scene({ ai }, events); ai.assertion = forbidden;
    try {
        connect(o); const h = o.hitModules.effects; h.burning.assertion('식', '무시한 파일', 123); o.services.ai.assertion = (text) => events.push(['B', text]); h.burning.assertion('다음', '', 124); h.burning.bulfSound(0, 12.5, 33.25);
        assert.deepEqual(events, [['A', '식'], ['B', '다음'], ['once', 12.5, 33.25, o.soundNames.lookup('bulfHurt.wav'), 0, 0]]);
        o.services.ai.assertion = null; assert.throws(() => h.burning.assertion('식', '', 125), /진단 장치 미연결 125/); o.services.sound.playOnceAt = null; assert.throws(() => h.burning.bulfSound(0, 0, 0), /위치 음향 미연결/);
    } finally { o.dispose(); }
});

test('Session 후속 피격 경계: 실제 미설치 모듈을 대체하지 않고 현재 건설 번호와 경고 선택을 유지한다', () => {
    const o = scene();
    try {
        beforeHits(o); o.services.sound.wallSeconds = null; o.services.sound.play = null; o.connectHitBodies(); const h = o.hitModules.effects; assert.throws(() => h.burning.create(5), /Burning.*설치 전/); assert.throws(() => h.dudes.createBurning(5), /Burning.*설치 전/); assert.throws(() => h.dudes.pathEndpoint(5), /이동.*설치 전/); assert.throws(() => h.dudes.startPath(5, 1, 2), /이동.*설치 전/); assert.throws(() => h.vortex.resetEnd(5, 1), /PrepareConstruction 설치 전/);
        o.gameplayGlobals.gameAux().constructionProcessType = 39; assert.equal(h.vortex.constructionType(), 39); assert.throws(() => h.general.realTime(), /벽시각 미연결/); assert.throws(() => h.general.warningSound(), /전역 음향/); assert.throws(() => h.general.warningText(), /Tell/);
        o.globals.missionPost = 1; h.general.warningEvent(); o.globals.missionPost = 0; assert.throws(() => h.general.warningEvent(), /경고 이벤트 미연결/);
    } finally { o.dispose(); }
});

test('Session 피격 부분 실패: 실제 장기 거주 목록 거부 앞의 AI/상태/효과를 남기고 대여를 반환한다', () => {
    const o = scene();
    try {
        beforeHits(o); const residences = o.world.storage.sidLists.residences; residences.count = residences.capacity() + 1; assert.throws(() => o.connectHitBodies(), /count 범위/); assert.ok(o.aiDamage); assert.ok(o.playerDamage); assert.equal(o.hits, null); assert.equal(o.hitModules.ready, false); assert.equal(typeof o.state.generalDamageState.primaryIslandRead, 'function'); assert.throws(() => o.connectHitBodies(), /반복/); residences.count = 0;
    } finally { o.dispose(); }
    const p = o.playerRecords.acquire(); p.release(); const g = o.gameplayGlobals.acquire(); g.release();
});
