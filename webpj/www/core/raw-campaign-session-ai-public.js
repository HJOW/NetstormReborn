// Session 공개 AI 생성·직접 명령·준비/초기화의 같은 본체와 설치 관문이다(225553df, WB04-E672).
import { RawAiCommandState } from './raw-ai-command-state.js';
import { makeAi, transferMakeAiObjects, clearMakeAiObjects } from './raw-make-ai.js';
import { RawMissionAiSettings, MISSION_AI_SETTINGS_SIZE } from './raw-mission-ai-settings.js';
import { currentAllocationBytes } from './current-allocation-bytes.js';

// 실제 미션이 설치된 뒤 Player 1..8만 받는다.
function requireOwner(session, owner) {
    if (!session.loaded || !Number.isInteger(owner) || owner < 1 || owner >= 9) throw new Error('실제 Session 설치 후 owner1..8 AI 호출만 허용');
}

// 현재 Player와 생존 본체를 방문마다 재조회하는 직접 명령기다.
function commands(session) {
    return new RawAiCommandState({ players: () => session.players, body: (token) => session.aiMemory.bytes(token) });
}

// 실제 AI 생성 및 초기 갱신으로 같은 설정을 전달한다.
export function createCampaignAi(session, owner, settings) {
    requireOwner(session, owner); return session.aiCreate.create(owner, settings);
}

// 기존 SP 갱신을 우회하는 원본 직접 지갑 명령이다.
export function crankCampaignAiMoney(session) {
    if (!session.loaded) throw new Error('AI 직접 명령의 실제 Session 미설치');
    commands(session).crankAiMoney();
}

// owner 0은 설치 상태를 읽기 전에 반환하고 나머지는 같은 현재 본체에 위임한다.
export function setCampaignAiParameter(session, packed) {
    const word = packed >>> 0;
    if (!(word & 7)) return;
    if (!session.loaded) throw new Error('AI 직접 명령의 실제 Session 미설치');
    commands(session).setAiParameter(word);
}

// 현재 소유자의 실제 AI가 있으면 준비 행동을 한 번 호출한다.
export function prepareCampaignAi(session, owner) {
    requireOwner(session, owner); const token = session.participation.brain(owner);
    if (!token) return false; session.aiPreparation.update(token); return true;
}

// 초기 전략이 없을 때만 갱신하고 효과 뒤 현재 본체 플래그를 재조회한다.
export function initializeCampaignAi(session, owner) {
    requireOwner(session, owner); const token = session.participation.brain(owner);
    if (!token) return false;
    // 각 읽기에서 같은 살아 있는 AI 본체를 다시 빌린다.
    const initialized = () => { const raw = session.aiMemory.bytes(token); return new DataView(raw.buffer, raw.byteOffset, raw.byteLength).getUint32(0x258, true); };
    if (!initialized()) session.aiStrategySetup.initialize(token);
    return initialized() !== 0;
}

// 일반 MakeAi의 기본 설정·객체 이전/삭제를 같은 실제 Session에 전달한다.
export function makeCampaignAi(session, owner) {
    const c = session.core, settings = new Uint8Array(MISSION_AI_SETTINGS_SIZE);
    const reads = {
        battle: () => c.ownerMode.battle, missionPost: () => session.globals.missionPost,
        playerKnown: () => session.playerState.playerKnown, localOwner: () => session.playerState.battleOwner,
        brain: (player) => { if (player >= 9) throw new RangeError('MakeAi local+1의 실제 Player9 범위'); return new DataView(session.players.buffer, session.players.byteOffset, session.players.byteLength).getUint32(player * 180 + 0xb0, true); },
    };
    // 로딩 깊이는 같은 월드 장치에 전파하며 별도 사본으로 복구하지 않는다.
    const depth = { get value() { return session.objectState.loadingDepth; }, set value(value) { session.objectState.loadingDepth = value; } };
    const objects = {
        capacity: () => c.pool.capacity, localOwner: reads.localOwner, playerType: () => session.playerState.playerType,
        bridgeType: () => c.bridgeState.bridgeType, slot: (sid) => { if (sid > 65535) throw new RangeError('MakeAi 객체 SID 범위'); return c.pool.slot(sid); },
        flags: (sid) => { if (sid > 65535) throw new RangeError('MakeAi 객체 SID 범위'); const type = c.pool.slot(sid)[10]; if (type >= c.types.length) throw new RangeError('MakeAi 객체 TYPE 범위'); return c.types[type].flags2; },
        players: () => session.players,
        region: (x, y) => { if (!session.loaded) throw new Error('설치 전 MakeAi 실제 섬 조회'); return session.ownership.islandNumber(x, y) >>> 0; },
        setOwner: (sid, target) => { if (!session.loaded) throw new Error('설치 전 실제 가상 소유자 변경'); session.ownership.setOwner(sid, target); },
        destroy: (sid, flags) => { if (!session.loaded) throw new Error('설치 전 MakeAi 실제 전체삭제'); session.world.destroy(sid, flags); },
        decrementLocal: (type) => { const counts = c.bookkeeping.localCounts(); if (type >= counts.length) throw new RangeError('MakeAi 로컬 수량 범위'); counts[type] -= 1; },
        decrementBuilt: (type) => { const counts = c.bookkeeping.localSecondaryCounts(); if (type >= counts.length) throw new RangeError('MakeAi 건설 수량 범위'); counts[type] -= 1; },
        loadingDepth: depth, loadingDepthChanged: (value) => session.world.setLoadingDepth(value),
    };
    const hooks = {
        initialize: (bytes) => { currentAllocationBytes(bytes); new RawMissionAiSettings(session.state.missionAiSettings, { get: () => { throw new Error('일반 MakeAi는 미션 override를 읽지 않는다'); } }).initialize(bytes); },
        stormPower: (player) => { const raw = new DataView(new ArrayBuffer(4)); raw.setUint32(0, session.sp.get(player), true); return raw.getFloat32(0, true); },
        create: (player, bytes) => createCampaignAi(session, player, bytes),
        transferObjects: (player) => transferMakeAiObjects(player, objects), clearObjects: (mask) => clearMakeAiObjects(mask, objects),
    };
    makeAi(owner >>> 0, settings, reads, hooks);
}
