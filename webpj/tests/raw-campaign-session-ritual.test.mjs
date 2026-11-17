// 실제 Session의 사제 의식과 공유 Regular 예약 수명을 검사한다(WB04-E645).
import test from 'node:test';
import assert from 'node:assert/strict';
import { campaignOwners } from './campaign-session-support.mjs';
import { RawCampaignSessionRitual } from '../www/core/raw-campaign-session-ritual.js';

// 실제 설치 순서를 사용하며 저장 미션 Load는 실행하지 않는다.
function scene(temple = false, extra = {}) {
    const o = campaignOwners({ ...extra, sound: { playOnceAt: () => 0, ...extra.sound } });
    try {
        o.connectWorldInstall({ sections: Array.from({ length: 35 }, () => new Uint8Array(0)) });
        o.connectOutcome(); o.connectAiBrains(); o.connectAiWorld(); o.connectAiProduction(); o.connectBuilding();
        o.connectMotionArrivals(); o.connectMotionInputs(); o.connectCarrierRouting(); o.connectMotionRuntime(); o.connectPriestPanic(); o.connectRitualPriest();
        if (temple) o.connectRitualTemple(); return o;
    } catch (error) { o.dispose(); throw error; }
}

test('사제 의식: 설치 순서·실제 Pool·반복 및 반납', () => {
    const early = campaignOwners(), partial = new RawCampaignSessionRitual(early);
    try { assert.throws(() => partial.bind(), /순서/); } finally { early.dispose(); partial.dispose(); }
    const o = scene(), module = o.ritualModules;
    try {
        assert.equal(o.ritualActions.pool, o.core.pool); assert.equal(o.loaded, false);
        assert.throws(() => o.connectRitualPriest(), /순서/);
    } finally { o.dispose(); }
    assert.throws(() => module.reserveRegular(), /반납/);
});

test('신전 의식: 같은 Pool·현재 사제 TYPE/권한과 반복 설치 관문', () => {
    const o = scene(true);
    try {
        const state = o.state.ritualState;
        assert.equal(o.templeRitual.pool, o.core.pool); assert.equal(o.runePostPop.pool, o.core.pool);
        o.priestState.movement.priest = 157; assert.equal(state.currentPriestType, 157);
        assert.equal(state.currentBoss, o.state.assetState.readBoss());
        assert.equal(o.ritualModules.templeReady, true); assert.equal(o.loaded, false);
        assert.throws(() => o.connectRitualTemple(), /반복/);
    } finally { o.dispose(); }
});

test('룬 후처리: 실제 등록 몸체의 최초 Pop은 같은 Host에 304를 예약한다', () => {
    const o = scene(true);
    try {
        const rune = o.world.create(166); o.world.pop(rune, 32, 32, 1);
        const task = o.core.processes.findEvent(rune, 0x304);
        assert.ok(task); assert.equal(task.shared, false); assert.equal(task.payload, Math.fround(0.01));
        assert.equal(o.ritualModules.pendingRegular, null);
    } finally { o.dispose(); }
});

test('다이스 의식: 같은 현재 장부·동맹·사제 TYPE과 설치 순서', () => {
    const early = scene();
    try { assert.throws(() => early.connectRitualDais(), /순서/); } finally { early.dispose(); }
    const o = scene(true);
    try {
        o.players[0x50] = 3; o.alliances[2] = 0xffffffff; o.connectRitualDais();
        assert.equal(o.daisRegular.pool, o.core.pool); assert.equal(o.daisState.playerRecords, o.players);
        assert.equal(o.state.runeState.playerFlags[0], 3); assert.equal(o.daisState.alliances[2], -1);
        o.priestState.movement.priest = 157; assert.equal(o.ritualModules.daisHooks.priestType(), 157);
        assert.equal(o.ritualModules.daisReady, true); assert.equal(o.loaded, false);
        assert.throws(() => o.connectRitualDais(), /순서/);
    } finally { o.dispose(); }
});

// 제단은 같은 Pool과 예약 객체를 공유하고 선행 결선 및 반납을 확인한다.
test('제단 의식: 실제 몸체와 공유 Regular 예약 및 설치 순서', () => {
    const early = scene(true);
    try { assert.throws(() => early.connectRitualAltar(), /순서/); } finally { early.dispose(); }
    const o = scene(true);
    const module = o.ritualModules;
    try {
        o.connectRitualDais(); o.connectRitualAltar();
        assert.equal(o.altarSacrifice.pool, o.core.pool);
        assert.equal(module.altarState.escapePercent, 40);
        assert.equal(module.altarHooks.boss(), o.state.assetState.readBoss());
        assert.equal(module.altarHooks.reserveShared(), true);
        const reserved = module.pendingRegular;
        assert.throws(() => module.altarHooks.reserveShared(), /중첩/);
        const altar = o.world.create(164);
        module.altarHooks.addShared(altar, 20, Math.fround(0.08));
        assert.equal(o.core.processes.findEvent(altar, 20), reserved);
        assert.equal(reserved.shared, true);
        assert.equal(module.pendingRegular, null);
        assert.throws(() => o.connectRitualAltar(), /순서/);
        assert.equal(o.loaded, false);
    } finally { o.dispose(); }
    assert.throws(() => module.altarHooks.boss(), /반납/);
});

// 실제 월드 Pop의 제단 정기와 로그인 종료 시각 대여를 확인한다.
test('다이스 수명: 제단 Pop의 같은 Host 예약과 현재 로그인 시각', () => {
    const o = scene(true);
    try {
        o.connectRitualDais();
        assert.throws(() => o.connectDaisLifecycle(), /순서/);
        o.connectRitualAltar(); o.connectDaisLifecycle();
        assert.equal(o.daisLifecycle.pool, o.core.pool);
        o.loginEnd = 19.5;
        assert.equal(o.ritualModules.lifecycleState.gameTime, 19.5);
        const altar = o.world.create(164);
        o.world.pop(altar, 32, 32, 1);
        const task = o.core.processes.findEvent(altar, 21);
        assert.ok(task); assert.equal(task.shared, false);
        assert.equal(o.ritualModules.pendingRegular, null);
        const dais = o.world.create(165);
        o.core.owner.set(dais, 1);
        o.world.pop(dais, 40, 40, 2);
        const record = new DataView(o.players.buffer, o.players.byteOffset, o.players.byteLength);
        assert.equal(record.getUint32(0xb4 + 0x94, true), dais);
        assert.equal(record.getUint32(0xb4 + 0x98, true), dais);
        assert.throws(() => o.connectDaisLifecycle(), /순서/);
        assert.equal(o.loaded, false);
    } finally { o.dispose(); }
});

// 내용물 통지의 실제 전역 조회와 명령 큐를 확인하며 사제 포획을 재현하지 않는다.
test('다이스 통지: 현재 전역·입력 큐와 제거 이유 경계', () => {
    const commands = [];
    const o = scene(true, { assets: { queueCommand: (...args) => commands.push(args) } });
    const module = o.ritualModules;
    try {
        o.connectRitualDais(); o.connectRitualAltar();
        assert.throws(() => o.connectDaisNotice(), /순서/);
        o.connectDaisLifecycle(); o.connectDaisNotice();
        assert.equal(o.daisNotice.pool, o.core.pool);
        o.priestState.movement.priest = 157;
        assert.equal(module.noticeState.priestType(), 157);
        assert.equal(module.noticeState.localOwner(), o.playerState.battleOwner);
        assert.equal(module.noticeState.bridgeType(), o.core.bridgeState.bridgeType);
        module.noticeHooks.stopVoice(module.noticeState.cue(), 0, 0);
        assert.deepEqual(commands, [[0x020000d7, 0, 0]]);
        const dais = o.world.create(165);
        o.daisNotice.notice(dais, 0, 2);
        assert.equal(commands.length, 1);
        assert.throws(() => o.connectDaisNotice(), /순서/);
        assert.equal(o.loaded, false);
    } finally { o.dispose(); }
    assert.throws(() => module.noticeState.priestType(), /반납/);
});

// 회수 요청은 현재 소유자/금지를 읽고 삭제 대신 내부 전송만 한다.
test('회수 명령: 현재 금지와 실제 송신 패킷', () => {
    const sent = [], tells = [];
    const o = scene(true, { carrierRequest: (target, packet) => sent.push([target, [...packet]]), priest: { shield: { tell: (key) => tells.push(key) } } });
    try {
        o.connectRitualDais(); o.connectRitualAltar(); o.connectDaisLifecycle();
        assert.throws(() => o.connectSalvage(), /순서/);
        o.connectDaisNotice(); o.connectSalvage();
        // 제단은 다이스를 찾으므로 다른 건물로 요청한다.
        const building = o.world.create(70);
        o.core.owner.set(building, o.playerState.battleOwner);
        o.gameplayGlobals.denySalvage().value = 1;
        o.salvageCommand.request(building);
        assert.deepEqual(tells, ['DenySalvage']); assert.equal(sent.length, 0);
        o.gameplayGlobals.denySalvage().value = 0;
        o.salvageCommand.request(building);
        assert.deepEqual(sent, [[9, [1, 0x54, 8, 0, building & 255, building >>> 8, 0x20, 0]]]);
        assert.equal(o.core.pool.slot(building)[11] & 1, 0);
        assert.throws(() => o.connectSalvage(), /순서/);
        assert.equal(o.loaded, false);
    } finally { o.dispose(); }
});

// 운반 이동은 같은 Graph·현재 경로를 조회해 실제 내부 전송으로 보낸다.
test('운반 이동: 설치 순서와 경로 요청 패킷', () => {
    const sent = [];
    const o = scene(true, { carrierRequest: (target, packet) => sent.push([target, [...packet]]), deletion: { selected: () => 0 } });
    try {
        o.connectRitualDais(); o.connectRitualAltar(); o.connectDaisLifecycle(); o.connectDaisNotice();
        assert.throws(() => o.connectCarrierMove(), /순서/);
        o.connectSalvage(); o.connectCarrierMove();
        assert.equal(o.carrierMove.pool, o.core.pool);
        const priest = o.world.create(158);
        o.carrierMove.request(priest, { x: 4, y: 5 }, 0);
        assert.equal(sent.length, 1); assert.equal(sent[0][0], 9);
        assert.deepEqual(sent[0][1].slice(0, 6), [1, 0x40, 18, 0, priest & 255, priest >>> 8]);
        assert.throws(() => o.connectCarrierMove(), /순서/);
        assert.equal(o.loaded, false);
    } finally { o.dispose(); }
});

// AI의 비서버 이동은 사용자 이동과 같은 현재 헤더를 사용한다.
test('AI 이동: 같은 명령 헤더와 현재 전송', () => {
    const sent = [];
    const o = scene(true, { carrierRequest: (target, packet) => sent.push([target, [...packet]]), deletion: { selected: () => 0 }, ai: { allocationBytes: (bytes) => bytes.fill(0), names: { language: () => 0, substitute: (text) => text } } });
    let token = 0;
    try {
        o.connectRitualDais(); o.connectRitualAltar(); o.connectDaisLifecycle(); o.connectDaisNotice(); o.connectSalvage();
        assert.throws(() => o.connectAiMovement(), /순서/);
        o.connectCarrierMove(); o.connectAiMovement();
        assert.equal(o.aiMovement.pool, o.core.pool);
        token = o.aiMemory.allocate(0xb08);
        const priest = o.world.create(158);
        o.gameplayGlobals.authority().boss = false;
        o.motionRuntimeModules.moveState.request.message = 0x42;
        o.aiMovement.toPoint(token, priest, { x: 4, y: 5 }, 0);
        assert.equal(sent.length, 1);
        assert.deepEqual(sent[0][1].slice(0, 6), [1, 0x42, 18, 0, priest & 255, priest >>> 8]);
        assert.throws(() => o.connectAiMovement(), /순서/);
        assert.equal(o.loaded, false);
    } finally { if (token) o.aiMemory.free(token); o.dispose(); }
});

// AI 자원 선택은 현재 본체·그래프·내용물 저장소를 그대로 사용한다.
test('AI 자원 선택: 선행 관문과 신전 없는 현재 좌표', () => {
    const o = scene(true, { carrierRequest: () => {}, deletion: { selected: () => 0 }, ai: { allocationBytes: (bytes) => bytes.fill(0) } });
    let token = 0;
    try {
        o.connectRitualDais(); o.connectRitualAltar(); o.connectDaisLifecycle(); o.connectDaisNotice(); o.connectSalvage(); o.connectCarrierMove();
        assert.throws(() => o.connectAiResourceChoice(), /순서/);
        o.connectAiMovement(); o.connectAiResourceChoice();
        assert.equal(o.aiResourceChoice.pool, o.core.pool);
        token = o.aiMemory.allocate(0xb08);
        const raw = o.aiMemory.bytes(token), view = new DataView(raw.buffer, raw.byteOffset, raw.byteLength);
        view.setFloat32(0x268, 12.5, true); view.setFloat32(0x26c, 7.25, true);
        const point = o.aiResourceChoice.homeCenter(token);
        assert.deepEqual([point.x, point.y], [12.5, 7.25]);
        const priest = o.world.create(158);
        assert.equal(o.motionRuntimeModules.aiResourceHooks.crystals(priest), 0);
        assert.equal(o.motionRuntimeModules.aiResourceHooks.graph(priest), o.anchor.graphFor(priest));
        assert.throws(() => o.connectAiResourceChoice(), /순서/);
        assert.equal(o.loaded, false);
    } finally { if (token) o.aiMemory.free(token); o.dispose(); }
});

// 수집자와 목표가 없는 AI 본체는 월드나 난수 변경 없이 자연 종료한다.
test('AI 운반 행동: 선행 관문과 빈 본체', () => {
    const o = scene(true, { carrierRequest: () => {}, deletion: { selected: () => 0 }, ai: { allocationBytes: (bytes) => bytes.fill(0) } });
    let token = 0;
    try {
        o.connectRitualDais(); o.connectRitualAltar(); o.connectDaisLifecycle(); o.connectDaisNotice(); o.connectSalvage(); o.connectCarrierMove(); o.connectAiMovement();
        assert.throws(() => o.connectAiCarrierActions(), /순서/);
        o.connectAiResourceChoice(); o.connectAiCarrierActions();
        assert.equal(o.aiCarrierActions.pool, o.core.pool);
        token = o.aiMemory.allocate(0xb08);
        const before = o.random.state;
        o.aiCarrierActions.act(token);
        assert.equal(o.random.state, before);
        o.players[180 + 0x50] = 0x22;
        assert.equal(o.motionRuntimeModules.aiCarrierHooks.enemyAvailable(1), true);
        assert.throws(() => o.motionRuntimeModules.aiCarrierHooks.dropRequest(0), /설치 전/);
        assert.throws(() => o.connectAiCarrierActions(), /순서/);
        assert.equal(o.loaded, false);
    } finally { if (token) o.aiMemory.free(token); o.dispose(); }
});

// 배치 탐색은 동일 AI 본체에 현재 TYPE 핸들과 영역을 작성한다.
test('AI 배치 탐색: 선행 관문과 실제 본체 쓰기', () => {
    const o = scene(true, { carrierRequest: () => {}, deletion: { selected: () => 0 }, ai: { allocationBytes: (bytes) => bytes.fill(0) } });
    let token = 0;
    try {
        o.connectRitualDais(); o.connectRitualAltar(); o.connectDaisLifecycle(); o.connectDaisNotice(); o.connectSalvage(); o.connectCarrierMove(); o.connectAiMovement(); o.connectAiResourceChoice();
        assert.throws(() => o.connectAiBuildSearch(), /순서/);
        o.connectAiCarrierActions(); o.connectAiBuildSearch();
        token = o.aiMemory.allocate(0xb08);
        o.aiBuildSearch.initialize(token, 80, { left: 1, top: 1, right: 5, bottom: 4 }, { x: 0, y: 0 }, 1);
        const raw = o.aiMemory.bytes(token), view = new DataView(raw.buffer, raw.byteOffset, raw.byteLength);
        assert.equal(view.getUint32(0x574, true), 80);
        assert.equal(view.getUint32(0x574 + 0x18, true), 0x6b000000 + 80 * 500);
        assert.throws(() => o.connectAiBuildSearch(), /순서/);
        assert.equal(o.loaded, false);
    } finally { if (token) o.aiMemory.free(token); o.dispose(); }
});

// 지상 표적 장치는 현재 권한과 진단 전역을 빌리며 공격을 실행하지 않는다.
test('AI 지상 표적: 선행 관문과 현재 전역', () => {
    const o = scene(true, { carrierRequest: () => {}, deletion: { selected: () => 0 }, ai: { allocationBytes: (bytes) => bytes.fill(0) } });
    try {
        o.connectRitualDais(); o.connectRitualAltar(); o.connectDaisLifecycle(); o.connectDaisNotice(); o.connectSalvage(); o.connectCarrierMove(); o.connectAiMovement(); o.connectAiResourceChoice(); o.connectAiCarrierActions();
        assert.throws(() => o.connectAiGroundTarget(), /순서/);
        o.connectAiBuildSearch(); o.connectAiGroundTarget();
        assert.equal(o.aiGroundTarget.pool, o.core.pool);
        const globals = o.motionRuntimeModules.aiGroundGlobals;
        o.aiGroundExcludedType = 73; o.flyerDependType = 2;
        o.gameplayGlobals.authority().boss = false;
        assert.equal(globals.excludedType, 73); assert.equal(globals.dependType, 2); assert.equal(globals.boss, false);
        assert.throws(() => o.connectAiGroundTarget(), /순서/);
        assert.equal(o.loaded, false);
    } finally { o.dispose(); }
});

// 배치 표적은 같은 객체 TYPE의 현재 공격 속성만 조회한다.
test('AI 배치 표적: 선행 관문과 현재 TYPE 속성', () => {
    const o = scene(true, { carrierRequest: () => {}, deletion: { selected: () => 0 }, ai: { allocationBytes: (bytes) => bytes.fill(0) } });
    try {
        o.connectRitualDais(); o.connectRitualAltar(); o.connectDaisLifecycle(); o.connectDaisNotice(); o.connectSalvage(); o.connectCarrierMove(); o.connectAiMovement(); o.connectAiResourceChoice(); o.connectAiCarrierActions(); o.connectAiBuildSearch();
        assert.throws(() => o.connectAiDropTargets(), /순서/);
        o.connectAiGroundTarget(); o.connectAiDropTargets();
        const target = o.world.create(80);
        o.hitTypes[80].damagePerSecond = 12.5;
        assert.equal(o.motionRuntimeModules.aiDropTargetHooks.rate(target), 12.5);
        assert.throws(() => o.connectAiDropTargets(), /순서/);
        assert.equal(o.loaded, false);
    } finally { o.dispose(); }
});

// 부족 좌표가 없는 AI 본체는 실제 공급 장치 호출 없이 점수 0이다.
test('AI 배치 평가: 선행 관문과 빈 부족 좌표', () => {
    const o = scene(true, { carrierRequest: () => {}, deletion: { selected: () => 0 }, ai: { allocationBytes: (bytes) => bytes.fill(0) } });
    let token = 0;
    try {
        o.connectRitualDais(); o.connectRitualAltar(); o.connectDaisLifecycle(); o.connectDaisNotice(); o.connectSalvage(); o.connectCarrierMove(); o.connectAiMovement(); o.connectAiResourceChoice(); o.connectAiCarrierActions(); o.connectAiBuildSearch(); o.connectAiGroundTarget();
        assert.throws(() => o.connectAiDropEvaluation(), /순서/);
        o.connectAiDropTargets(); o.connectAiDropEvaluation();
        assert.equal(o.aiDropEvaluation.pool, o.core.pool);
        token = o.aiMemory.allocate(0xb08);
        const index = { value: 9 };
        assert.equal(o.aiDropEvaluation.dryDistance(token, { x: 4, y: 5 }, index), 0);
        assert.equal(index.value, 9);
        assert.throws(() => o.connectAiDropEvaluation(), /순서/);
        assert.equal(o.loaded, false);
    } finally { if (token) o.aiMemory.free(token); o.dispose(); }
});

// 후보 스캔은 현재 표면 Pool과 원본 초기 영역을 공유한다.
test('AI 배치 스캔: 선행 관문과 일반 표면', () => {
    const o = scene(true, { carrierRequest: () => {}, deletion: { selected: () => 0 }, ai: { allocationBytes: (bytes) => bytes.fill(0) } });
    try {
        o.connectRitualDais(); o.connectRitualAltar(); o.connectDaisLifecycle(); o.connectDaisNotice(); o.connectSalvage(); o.connectCarrierMove(); o.connectAiMovement(); o.connectAiResourceChoice(); o.connectAiCarrierActions(); o.connectAiBuildSearch(); o.connectAiGroundTarget(); o.connectAiDropTargets();
        assert.throws(() => o.connectAiDropScan(), /순서/);
        o.connectAiDropEvaluation(); o.connectAiDropScan();
        assert.equal(o.aiDropScan.pool, o.core.pool);
        assert.deepEqual(o.aiDropScanBounds, { left: 10, top: 10, right: 246, bottom: 246 });
        const surface = o.world.create(80), cell = { x: 12, y: 13 }, before = o.random.state;
        assert.equal(o.aiDropScan.adjustBridge(surface, cell), false);
        assert.deepEqual(cell, { x: 12, y: 13 }); assert.equal(o.random.state, before);
        assert.throws(() => o.connectAiDropScan(), /순서/);
        assert.equal(o.loaded, false);
    } finally { o.dispose(); }
});

// 배치 선택은 현재 방향 TYPE과 공용 본체를 사용하고 설치 순서를 지킨다.
test('AI 배치 선택: 선행 관문과 현재 방향 종류', () => {
    const o = scene(true, { carrierRequest: () => {}, deletion: { selected: () => 0 }, ai: { allocationBytes: (bytes) => bytes.fill(0) } });
    try {
        o.connectRitualDais(); o.connectRitualAltar(); o.connectDaisLifecycle(); o.connectDaisNotice(); o.connectSalvage(); o.connectCarrierMove(); o.connectAiMovement(); o.connectAiResourceChoice(); o.connectAiCarrierActions(); o.connectAiBuildSearch(); o.connectAiGroundTarget(); o.connectAiDropTargets(); o.connectAiDropEvaluation();
        assert.throws(() => o.connectAiDropSelection(), /순서/);
        o.connectAiDropScan(); o.connectAiDropSelection();
        assert.equal(o.aiDropSelection.pool, o.core.pool);
        const current = o.state.constructionState.confirm.patternTypesRead()[4];
        assert.equal(o.aiDropSelection.directional(current), true);
        o.state.gunState.thunderCannonType = 132;
        assert.equal(o.aiDropSelection.directional(132), true);
        assert.equal(o.aiDropSelection.directional(0xffffffff), false);
        assert.throws(() => o.connectAiDropSelection(), /순서/);
        assert.equal(o.loaded, false);
    } finally { o.dispose(); }
});

// 부족 좌표 기하는 현재 지면과 같은 Pool의 상태 WORD를 사용한다.
test('AI 부족 좌표 기하: 선행 관문과 실제 월드', () => {
    const o = scene(true, { carrierRequest: () => {}, deletion: { selected: () => 0 }, ai: { allocationBytes: (bytes) => bytes.fill(0) } });
    const module = o.motionRuntimeModules;
    try {
        o.connectRitualDais(); o.connectRitualAltar(); o.connectDaisLifecycle(); o.connectDaisNotice(); o.connectSalvage(); o.connectCarrierMove(); o.connectAiMovement(); o.connectAiResourceChoice(); o.connectAiCarrierActions(); o.connectAiBuildSearch(); o.connectAiGroundTarget(); o.connectAiDropTargets(); o.connectAiDropEvaluation(); o.connectAiDropScan();
        assert.throws(() => o.connectAiDryGeometry(), /순서/);
        o.connectAiDropSelection(); o.connectAiDryGeometry();
        assert.equal(o.aiDryGeometry.pool, o.core.pool); assert.equal(o.aiDryMarkerType, 104);
        assert.equal(module.aiDryGeometryHooks.surface(12, 13), o.core.hash.cell(0, 12, 13));
        const sid = o.world.create(80); module.aiDryGeometryHooks.word(sid, 0x1234);
        assert.deepEqual(Array.from(o.core.pool.slot(sid).subarray(12, 14)), [0x34, 0x12]);
        assert.throws(() => o.connectAiDryGeometry(), /순서/); assert.equal(o.loaded, false);
    } finally { o.dispose(); }
    assert.throws(() => module.aiDryGeometryHooks.surface(12, 13), /반납/);
});

// 부족 좌표 갱신기는 빈 본체에서 월드와 난수를 변경하지 않는다.
test('AI 부족 좌표 갱신: 선행 관문과 빈 본체', () => {
    const o = scene(true, { carrierRequest: () => {}, deletion: { selected: () => 0 }, ai: { allocationBytes: (bytes) => bytes.fill(0) } });
    let token = 0;
    try {
        o.connectRitualDais(); o.connectRitualAltar(); o.connectDaisLifecycle(); o.connectDaisNotice(); o.connectSalvage(); o.connectCarrierMove(); o.connectAiMovement(); o.connectAiResourceChoice(); o.connectAiCarrierActions(); o.connectAiBuildSearch(); o.connectAiGroundTarget(); o.connectAiDropTargets(); o.connectAiDropEvaluation(); o.connectAiDropScan(); o.connectAiDropSelection();
        assert.throws(() => o.connectAiFakeDrySpots(), /순서/);
        o.connectAiDryGeometry(); o.connectAiFakeDrySpots();
        assert.equal(o.aiFakeDrySpots.pool, o.core.pool);
        token = o.aiMemory.allocate(0xb08); const before = o.random.state;
        o.aiFakeDrySpots.extend(token); assert.equal(o.random.state, before);
        assert.throws(() => o.connectAiFakeDrySpots(), /순서/); assert.equal(o.loaded, false);
    } finally { if (token) o.aiMemory.free(token); o.dispose(); }
});

// 배치 행동은 선행 갱신기와 같은 Pool·생산 가능 판정을 사용한다.
test('AI 배치 행동: 선행 관문과 현재 생산 비용', () => {
    const o = scene(true, { carrierRequest: () => {}, deletion: { selected: () => 0 }, ai: { allocationBytes: (bytes) => bytes.fill(0) } });
    let token = 0;
    try {
        o.connectRitualDais(); o.connectRitualAltar(); o.connectDaisLifecycle(); o.connectDaisNotice(); o.connectSalvage(); o.connectCarrierMove(); o.connectAiMovement(); o.connectAiResourceChoice(); o.connectAiCarrierActions(); o.connectAiBuildSearch(); o.connectAiGroundTarget(); o.connectAiDropTargets(); o.connectAiDropEvaluation(); o.connectAiDropScan(); o.connectAiDropSelection(); o.connectAiDryGeometry();
        assert.throws(() => o.connectAiDropActions(), /순서/);
        o.connectAiFakeDrySpots(); o.connectAiDropActions();
        assert.equal(o.aiDropActions.pool, o.core.pool);
        token = o.aiMemory.allocate(0xb08);
        assert.equal(o.motionRuntimeModules.aiDropActionHooks.canAfford(token, 80), o.aiDeployment.canAfford(token, 80));
        assert.throws(() => o.connectAiDropActions(), /순서/); assert.equal(o.loaded, false);
    } finally { if (token) o.aiMemory.free(token); o.dispose(); }
});

// 사제 행동은 설치 순서와 같은 Pool을 지키며 사제 0에서 동작을 생략한다.
test('AI 사제 행동: 선행 관문과 사제 없는 본체', () => {
    const o = scene(true, { carrierRequest: () => {}, deletion: { selected: () => 0 }, ai: { allocationBytes: (bytes) => bytes.fill(0) } });
    let token = 0;
    try {
        o.connectRitualDais(); o.connectRitualAltar(); o.connectDaisLifecycle(); o.connectDaisNotice(); o.connectSalvage(); o.connectCarrierMove(); o.connectAiMovement(); o.connectAiResourceChoice(); o.connectAiCarrierActions(); o.connectAiBuildSearch(); o.connectAiGroundTarget(); o.connectAiDropTargets(); o.connectAiDropEvaluation(); o.connectAiDropScan(); o.connectAiDropSelection(); o.connectAiDryGeometry(); o.connectAiFakeDrySpots();
        assert.throws(() => o.connectAiPriestActions(), /순서/);
        o.connectAiDropActions(); o.connectAiPriestActions();
        assert.equal(o.aiPriestActions.pool, o.core.pool);
        token = o.aiMemory.allocate(0xb08); const before = o.random.state;
        o.aiPriestActions.act(token, 0); assert.equal(o.random.state, before);
        assert.throws(() => o.connectAiPriestActions(), /순서/); assert.equal(o.loaded, false);
    } finally { if (token) o.aiMemory.free(token); o.dispose(); }
});

// AI 다리 경로는 선행 행동 뒤 동일한 Pool과 본체를 빌린다.
test('AI 다리 경로: 선행 관문과 초기화', () => {
    const o = scene(true, { carrierRequest: () => {}, deletion: { selected: () => 0 }, ai: { allocationBytes: (bytes) => bytes.fill(0) } });
    let token = 0;
    try {
        o.connectRitualDais(); o.connectRitualAltar(); o.connectDaisLifecycle(); o.connectDaisNotice(); o.connectSalvage(); o.connectCarrierMove(); o.connectAiMovement(); o.connectAiResourceChoice(); o.connectAiCarrierActions(); o.connectAiBuildSearch(); o.connectAiGroundTarget(); o.connectAiDropTargets(); o.connectAiDropEvaluation(); o.connectAiDropScan(); o.connectAiDropSelection(); o.connectAiDryGeometry(); o.connectAiFakeDrySpots(); o.connectAiDropActions();
        assert.throws(() => o.aiBridgePath(), /순서/);
        o.connectAiPriestActions(); const path = o.aiBridgePath({ x: 0, y: 0 });
        assert.equal(path.pool, o.core.pool); assert.deepEqual([o.aiBridgeStartType, o.aiBridgeEndType, o.aiBridgeTargetType], [113, 119, 117]);
        token = o.aiMemory.allocate(0xb08); const before = o.random.state;
        path.initialize(token, { x: 12, y: 13 }, { x: 14, y: 15 }); assert.equal(o.random.state, before);
        assert.equal(o.loaded, false);
    } finally { if (token) o.aiMemory.free(token); o.dispose(); }
    assert.throws(() => o.aiBridgePath(), /반납/);
});

// 다리 전략은 선행 행동과 같은 후보·Pool을 사용한다.
test('AI 다리 전략: 선행 관문과 빈 사각형', () => {
    const o = scene(true, { carrierRequest: () => {}, deletion: { selected: () => 0 }, ai: { allocationBytes: (bytes) => bytes.fill(0) } });
    try {
        o.connectRitualDais(); o.connectRitualAltar(); o.connectDaisLifecycle(); o.connectDaisNotice(); o.connectSalvage(); o.connectCarrierMove(); o.connectAiMovement(); o.connectAiResourceChoice(); o.connectAiCarrierActions(); o.connectAiBuildSearch(); o.connectAiGroundTarget(); o.connectAiDropTargets(); o.connectAiDropEvaluation(); o.connectAiDropScan(); o.connectAiDropSelection(); o.connectAiDryGeometry(); o.connectAiFakeDrySpots(); o.connectAiDropActions();
        assert.throws(() => o.connectAiBridgeStrategies(), /순서/);
        o.connectAiPriestActions(); o.connectAiBridgeStrategies();
        assert.equal(o.aiBridgeStrategies.pool, o.core.pool); assert.equal(o.aiBridgeDisabled, false);
        assert.equal(o.aiBridgePlanning.rectangleFree(12, 13, 11, 13), true);
        assert.throws(() => o.connectAiBridgeStrategies(), /순서/); assert.equal(o.loaded, false);
    } finally { o.dispose(); }
});

// 전체 프레임은 같은 Player와 현재 로그·행동 장치를 사용한다.
test('AI 프레임: 설치 순서와 현재 저장소 결선', () => {
    const logs = [], o = scene(true, { carrierRequest: () => {}, deletion: { selected: () => 0 }, player: { log: (...args) => logs.push(args) }, ai: { allocationBytes: (bytes) => bytes.fill(0) } });
    let token = 0;
    try {
        o.connectRitualDais(); o.connectRitualAltar(); o.connectDaisLifecycle(); o.connectDaisNotice(); o.connectSalvage(); o.connectCarrierMove(); o.connectAiMovement(); o.connectAiResourceChoice(); o.connectAiCarrierActions(); o.connectAiBuildSearch(); o.connectAiGroundTarget(); o.connectAiDropTargets(); o.connectAiDropEvaluation(); o.connectAiDropScan(); o.connectAiDropSelection(); o.connectAiDryGeometry(); o.connectAiFakeDrySpots(); o.connectAiDropActions(); o.connectAiPriestActions();
        assert.throws(() => o.connectAiRuntime(), /순서/);
        o.connectAiBridgeStrategies(); o.connectAiRuntime();
        assert.throws(() => o.connectAiRuntime(), /순서/);
        const hooks = o.motionRuntimeModules.aiRuntimeHooks;
        token = o.aiMemory.allocate(0xb08); const raw = o.aiMemory.bytes(token), view = new DataView(raw.buffer, raw.byteOffset, raw.byteLength);
        view.setInt32(0x260, 2, true); hooks.dryNotice(token, 17, true); assert.equal(logs.length, 0);
        view.setUint32(0xb00, 1, true); hooks.dryNotice(token, 17, true); hooks.took(token, 1.25);
        assert.deepEqual(logs, [[17, 'AI2: Killed Fake Dry Spot\n\n'], [29, 'AI2: Took 1.3 seconds.\n\n']]);
        assert.throws(() => hooks.priest(token, o.core.pool.capacity), /SID 범위/);
        view.setUint32(0x258, 1, true);
        // 전체 프레임의 현재 타이밍 조회만 독립적으로 실행하도록 행동 효과를 대체한다.
        for (const key of ['prepare', 'selectGoal', 'collectors', 'factories', 'priest', 'drops', 'bridges', 'carriers', 'economy']) hooks[key] = () => {};
        hooks.dryScan = () => 0; hooks.money = () => 0; hooks.wall = () => 1;
        assert.equal(o.aiRuntime.tick(token), true);
        o.core.ownerMode.battle = false; o.aiRuntime.frame();
        assert.equal(o.loaded, false);
    } finally { if (token) o.aiMemory.free(token); o.dispose(); }
    assert.throws(() => o.connectAiRuntime(), /반납/);
});

test('다이스 효과: 동일 Sound47 예약·지연 시각과 Tell 명령 경계', () => {
    const o = scene(true);
    try {
        o.connectRitualDais(); const hooks = o.ritualModules.daisHooks, dais = o.world.create(165);
        assert.equal(hooks.reserveSound(), true); const pending = o.ritualModules.pendingSound;
        assert.throws(() => hooks.reserveSound(), /중첩/);
        const sound = hooks.loadSound('priestSacrifice2.wav', 0, 12), form = hooks.attachSound(dais, sound, 0, 12);
        assert.equal(o.core.processes.fromForm(form), pending); hooks.setSoundStart(form, 12.5); assert.equal(pending.startTime, 12.5);
        assert.throws(() => hooks.chat({ kind: 'LocalPriestDead' }), /멀티플레이/);
        assert.throws(() => hooks.stopVoice(1, 0, 0), /명령 큐 미연결/);
    } finally { o.dispose(); }
});

test('다이스 보상·안내: 실제 Core/SP 장부와 입력 큐·Tell을 공유한다', () => {
    const queued = [], tells = [], o = scene(true, { assets: { queueCommand: (...args) => queued.push(args) }, priest: { shield: { tell: (key) => tells.push(key) } } });
    try {
        o.connectRitualDais(); const h = o.ritualModules.daisHooks;
        h.stopVoice(0x020000df, 0xffffffff, 2); h.chat({ kind: 'Stormpower' }); h.chat({ kind: 'Upgrade' });
        assert.deepEqual(queued, [[0x020000df, -1, 2]]); assert.deepEqual(tells, ['SacrificeForStormpower', 'AltarUpgrade']);
        const bytes = new DataView(new ArrayBuffer(4)); bytes.setFloat32(0, 123, true); o.sp.set(2, bytes.getUint32(0, true));
        h.normalizeForeignSp(2); bytes.setUint32(0, o.sp.get(2), true); assert.equal(bytes.getFloat32(0, true), 5123);
        const type = o.core.types.findIndex((item) => item.techBitIndex >= 0 && item.techBitIndex < 32);
        assert.ok(type >= 0); const bit = o.core.types[type].techBitIndex, records = o.coreDataGlobals.records();
        h.grantForeignType(2, type); const view = new DataView(records.buffer, records.byteOffset, records.byteLength);
        assert.notEqual(view.getUint32(42, true) & (1 << bit), 0);
    } finally { o.dispose(); }
});

test('희생 요청: 실제 내용물 탐색은 너겟만 제거하고 같은 다이스 자격을 조회한다', () => {
    const sounds = [], notices = [], o = scene(true, { deletion: { selected: () => 0 }, contentLifetime: { sound: (...args) => sounds.push(args), localNotice: (value) => notices.push(value) }, assets: { queueCommand: () => {} } });
    try {
        o.connectRitualDais(); const priest = o.world.create(158), dais = o.world.create(165);
        const nugget = o.contained.create(priest, 154, 9, 0x20, 0, 0);
        assert.ok(nugget); o.ritualModules.sacrificeHooks.remove(priest, 154, 0, 1);
        assert.equal(o.priests.carrier.findContained(priest, 0x20, 154, 0), 0);
        assert.equal(sounds.length, 1); assert.equal(notices.length, 0);
        assert.equal(typeof o.sacrificeRequest.allowed(dais, priest), 'boolean');
    } finally { o.dispose(); }
});

test('의식 예약: 실제 동일 객체와 중첩·미예약 관문, 공유 Host 시각', () => {
    const o = scene();
    try {
        const m = o.ritualModules, priest = o.world.create(158);
        assert.throws(() => m.addRegular(priest, 604, 0.01, 61), /예약 누락/);
        assert.equal(m.reserveRegular(), true); const reserved = m.pendingRegular;
        assert.throws(() => m.reserveRegular(), /중첩/);
        m.addRegular(priest, 604, Math.fround(0.01), 61);
        assert.equal(m.pendingRegular, null); assert.equal(o.core.processes.findEvent(priest, 604), reserved);
        assert.equal(reserved.time, o.core.processState.now); assert.equal(reserved.shared, true);
    } finally { o.dispose(); }
});

test('사제 의식 동작: 실제 K 프레임과 같은 Host에 604 공유 예약', () => {
    const o = scene();
    try {
        const priest = o.world.create(158);
        o.ritualActions.continue(priest, 0);
        const raw = o.core.pool.slot(priest), view = new DataView(raw.buffer, raw.byteOffset, raw.byteLength);
        assert.equal(view.getInt32(36, true), o.core.frames[158].run(0x4b).first);
        assert.ok(o.core.processes.findEvent(priest, 604));
        assert.equal(o.ritualModules.pendingRegular, null);
    } finally { o.dispose(); }
});
