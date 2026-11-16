// Session 생성자1871..1880의 지역/승패와 ConnectAi2777..2834의 참여·본체·초기/지갑 부모다(225553df, WB04-E623).
// 사용: connectWorldInstall 뒤 connectOutcome → connectAiBrains. 후속 통지/전략·행동 조립과 외곽 Session/Load는 아직 남는다.
import { RegionOwner } from './raw-region-owner.js';
import { createMissionOutcomeState, RawMissionOutcome, makeMissionOutcomeHooks } from './raw-mission-outcome.js';
import { RawPlayerParticipation } from './raw-player-participation.js';
import { RawAiBrainConstructor, RawAiCreate } from './raw-ai-brain.js';
import { RawAiBrainInitialUpdate } from './raw-ai-initial-update.js';
import { RawAiWallet } from './raw-ai-wallet.js';
import { RawPriestLookup } from './raw-priest-lookup.js';
import { bindAiTypeRequiredEnergy } from './raw-type-required-energy.js';
import { changePlayerColor } from './raw-player-color.js';
import { isAiEnemyOfLocal } from './ai-alliance-side.js';

// 선언부의 독립 typed 상태다. 함수형 전역은 실제 호출 시 이 owner의 현재 칸을 읽는다.
export function createCampaignAiState() { return { outcomeState: createMissionOutcomeState(), aiWorkshops: new Uint32Array([86, 87, 88, 85]), aiExcludedType: 133, aiMissionFallback: 0 }; }
// 선택한 실제 외부 장치만 요구하며 미연결을 기본 성공으로 바꾸지 않는다.
function need(device, name) { if (typeof device !== 'function') throw new Error(`Session AI 실제 ${name} 미연결`); return device; }
// 같은 Player 기록의 현재 비정렬 DWORD다.
function dword(bytes, offset) { return new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength).getUint32(offset, true); }

// 같은 실제 owner의 생성 구간별 한 번 결선과 마지막 대여 수명이다.
export class RawCampaignSessionAi {
    // 같은 기반 owner와 실패 접두/반복/반납 관문이다.
    #owner; #outcomeStarted = false; #outcomeReady = false; #brainsStarted = false; #brainsReady = false; #disposed = false;
    // 기반 owner만 빌리며 생성 자체로 사건이나 AI를 실행하지 않는다.
    constructor(owner) { this.#owner = owner; }

    // 같은 hash0/현재 TYPE와 같은 사제/Player/동맹을 연결한다. Check/Event를 실행하지 않는다.
    bindOutcome() {
        const o = this.#owner; const c = o.core;
        if (this.#disposed || !o.installModules?.ready || this.#outcomeStarted) throw new Error('Session 승패의 초기화 전/반복 결선');
        this.#outcomeStarted = true;
        o.regionOwner = new RegionOwner(c.pool, c.hash.view(0), () => o.terrainType);
        const ui = o.services.missionOutcome;
        // C++ services 사본의 함수만 채운다. 원래 호출자가 전달한 표는 바꾸지 않는다.
        for (const name of ['demo', 'tellBlocked', 'section', 'closeTransient', 'onSection']) if (!ui[name]) ui[name] = () => { throw new Error(`Session 승패 실제 ${name} 미연결`); };
        const hooks = { regionOwner: (x, y) => { this.#live(); return o.regionOwner.atPoint(x, y); }, section: ui.section, closeTransientGumps: ui.closeTransient, onSection: ui.onSection,
            invalidPlayerAssert: () => { throw new Error('Session 승패 실제 player 진단 미연결'); } };
        o.outcome = new RawMissionOutcome(c.pool, o.priestState.priests, o.outcomeState, makeMissionOutcomeHooks(c.pool, o.hp, hooks));
        o.outcome.bindCurrentInputs({ players: o.players, allies: o.alliances, localPlayer: () => o.playerState.battleOwner, demo: ui.demo, tellBlocked: ui.tellBlocked });
        this.#outcomeReady = true;
    }

    // 참여·실제 본체/초기 갱신·AiCreate·HP 지갑을 원문 순서로 만든다. AI 객체 생성은 호출자가 나중에 선택한다.
    bindBrains() {
        const o = this.#owner; const c = o.core;
        if (this.#disposed || !this.#outcomeReady || this.#brainsStarted) throw new Error('Session AI 본체의 초기화 전/반복 결선');
        this.#brainsStarted = true;
        const hooks = { ...o.services.ai.participation }; const otherAssign = hooks.assignConnection; const otherAlliance = hooks.sendAlliance;
        hooks.assignConnection = (pending, owner) => { this.#live(); if (o.connectionAssign) o.connectionAssign(pending, owner); else need(otherAssign, '로그인 연결 재배치')(pending, owner); };
        hooks.sendAlliance = (target, owner, mask) => { this.#live(); if (o.connectionAlliance) o.connectionAlliance(target, owner, mask); else need(otherAlliance, '동맹 요청 전송')(target, owner, mask); };
        hooks.destructBrain = (token) => { this.#live(); o.aiConstructor.destruct(token); };
        hooks.freeBrain = (token) => { this.#live(); o.aiConstructor.freeBody(token); };
        o.participation = new RawPlayerParticipation(o.players, o.alliances, o.hiddenPlayers, o.participationState, hooks);
        o.participation.bindCurrentInputs({ localOwner: () => o.playerState.battleOwner, playerKnown: () => o.playerState.playerKnown, battle: () => c.ownerMode.battle,
            metamap: () => c.ownerMode.challenge, localTransport: () => need(o.services.ai.localTransport, '로그인 로컬 전송 조회')() });
        const energy = bindAiTypeRequiredEnergy(o.assets.game.typeTable.definitions, c.types);
        o.aiConstructor = new RawAiBrainConstructor(o.aiMemory, o.player, o.participation, o.islands, o.alliances, c.types,
            { localOwner: () => o.playerState.battleOwner, noRegion: () => o.noRegion, invalidGraph: () => c.bookkeeping.invalidGraph >>> 0 },
            { priest: (owner) => new RawPriestLookup(c.pool, o.priestState.priests).find(owner) & 0xffff, regionAt: (x, y) => o.ownership.islandNumber(x, y), requiredEnergy: energy });
        o.aiInitial = new RawAiBrainInitialUpdate(o.aiMemory, o.player, o.technology, o.contentFinder, c.types,
            { missionMode: () => o.globals.missionPost ? 1 : 0, excludedType: () => o.aiExcludedType, workshops: o.aiWorkshops },
            { defaultName: () => need(o.services.ai.defaultName, 'defaultAiName 현재 언어 조회')(), requiredEnergy: energy });
        const create = {
            changeColor: (owner, color) => { this.#live(); changePlayerColor(c.pool, c.bridgeState, owner, color, (sid, frame, flags) => c.frame.set(sid, frame, flags)); o.services.ai.changeColor?.(owner, color); },
            firstRegion: (owner) => { if (owner >= 9) throw new RangeError('Session AI 첫 지역 owner 범위'); return dword(o.players, owner * 180 + 0x78); },
            islandTemple: (region) => o.islands.temple(region | 0),
            destroyTemple: (sid, flags) => { this.#live(); if (sid > 65535 || sid >= c.pool.capacity) throw new RangeError('Session AI 신전 SID 범위'); o.world.destroy(sid, flags); },
            difficulty: (owner, settings) => {
                if (!o.services.ai.difficulty || owner >= 9 || o.services.missionOutcome.demo() !== 0) return 0;
                const local = o.playerState.battleOwner;
                return o.services.ai.difficulty(owner, local >= 9 ? owner !== local : isAiEnemyOfLocal(owner, local, dword(o.players, local * 180 + 0x54), settings));
            },
            construct: (owner, settings, bytes) => { this.#live(); return o.aiConstructor.construct(owner, settings, bytes); },
            initialUpdate: (token, settings) => { this.#live(); o.aiInitial.update(token, settings); },
        };
        o.aiCreate = new RawAiCreate(o.participation, create);
        o.aiWallet = new RawAiWallet(o.aiMemory, o.players, { disabled: () => c.ownerMode.challenge, demo: () => o.services.missionOutcome.demo() !== 0 });
        o.hp.bindPatchWallet((owner, amount) => { this.#live(); o.aiWallet.credit(owner, amount); });
        this.#brainsReady = true;
    }

    // 소멸자4027의 owner1..8만 정리한다. 하나의 실패도 다른 owner 정리를 막지 않으며 Host보다 먼저 실행한다.
    releaseBrains() {
        const o = this.#owner;
        // 현재 Player 포인터와 정상 State 반납 증빙을 owner 방문마다 다시 읽는다.
        for (let owner = 1; owner < 9; owner += 1) if (o.participation) {
            try { if (!o.participation.brainReturnedForState(owner)) o.participation.removeBrain(owner); }
            catch (error) { console.error(`실제 Session AI 정리 실패 owner${owner}:`, error); }
        }
    }
    // 구성 도중 콜백도 사용할 수 있지만 부모가 반납한 뒤의 외부 대여는 거부한다.
    #live() { if (this.#disposed) throw new Error('반납한 Session AI 부모'); }
    // Host 정리가 끝난 뒤 대여를 닫는다. 실제 brain 반납은 그 전에 releaseBrains로 수행한다.
    dispose() { this.#disposed = true; }
    // 이 생성 구간만 완료한 상태다. 전체 ConnectAi/Session 완료와 구별한다.
    get outcomeReady() { return this.#outcomeReady; }
    // 실제 본체/초기/지갑의 조립 완료이며 통지·전략·행동은 후속 구간이다.
    get brainsReady() { return this.#brainsReady; }
}
