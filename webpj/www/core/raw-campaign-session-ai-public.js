// Session 공개 AI 생성·직접 명령·준비/초기화의 같은 본체와 설치 관문이다(225553df, WB04-E672).
import { RawAiCommandState } from './raw-ai-command-state.js';

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
