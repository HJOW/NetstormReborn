// ConnectAi2835..2879의 실제 객체 통지/World 등록·준비/전략/경제 부모다(225553df, WB04-E624).
// 사용: connectAiBrains 뒤 connectAiWorld. 전체 행동/외곽 Session·Load는 후속 구간이다.
import { RawAiObjectNotice, AI_OBJECT_LOG } from './raw-ai-object-notice.js';
import { RawAiWorldRegistration } from './raw-ai-world-registration.js';
import { RawAiPreparation } from './raw-ai-preparation.js';
import { RawAiStrategySetup } from './raw-ai-strategy-setup.js';
import { RawAiEconomy } from './raw-ai-economy.js';
import { RawPriestLookup } from './raw-priest-lookup.js';
import { RawContainedLookup } from './raw-contained-lookup.js';
import { RawPathProcess } from './raw-path-process.js';

// 현재 비정렬 DWORD를 보존한다.
function dword(bytes, offset) { return new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength).getUint32(offset, true); }
// 실제 분산 SP/기억 좌표 DWORD의 float 비트다.
function float(bits) { const bytes = new DataView(new ArrayBuffer(4)); bytes.setUint32(0, bits, true); return bytes.getFloat32(0, true); }
// 선택된 외부 장치만 요구한다.
function need(effect, name) { if (typeof effect !== 'function') throw new Error(`Session AI 월드 실제 ${name} 미연결`); return effect; }

// 같은 실제 World/본체와 현재 전역을 빌리고 Host 정리 뒤 통지를 분리하는 생성 구간이다.
export class RawCampaignSessionAiWorld {
    // 같은 owner와 한 번 구성/정상 복귀/반납 관문이다.
    #owner; #started = false; #ready = false; #disposed = false;
    // 값형 hook 표를 실제 모듈과 공유한다. 독립 근사 계산에 쓰지 않는다.
    objectsHooks = null; registrationHooks = null; preparationHooks = null; setupHooks = null;
    // 생성 자체로 World/AI 목록을 변경하지 않는다.
    constructor(owner) { this.#owner = owner; }
    // 현재 SID를 실제 Pool에서 접근하기 전만 검증한다.
    #sid(value) { if (value > 65535 || value >= this.#owner.core.pool.capacity) throw new RangeError('Session AI 월드 현재 SID 범위'); return value; }
    // 같은 현재 이동 Form을 찾아 실제 Path의 두 목표를 읽는다(2952, 뒤 운반 행동도 공유).
    carrierTargets(sid) {
        this.#live(); const o = this.#owner; const c = o.core; const kind = o.priests.movementProcessType(c.pool.slot(sid)[10]);
        const form = new RawContainedLookup(c.pool, (target, flags) => o.world.destroy(target, flags)).find(sid, kind);
        if (!form) return [0, 0]; const path = c.processes.fromForm(form);
        if (!(path instanceof RawPathProcess)) throw new Error('AI 현재 이동 프로세스 미연결');
        return [path.runtime.targetA, path.runtime.targetB];
    }
    // source2948의 진단 뒤 같은 현재 승패 부모를 조회한다. 실제 포획/전투를 실행하지 않는다.
    priestCaptured(owner) {
        this.#live(); const o = this.#owner;
        if (owner < 1 || owner > 8) need(o.services.ai.assertion, '사제 포획 진단')('playerId > INVALID_PLAYER_ID && playerId < MAX_PLAYERS');
        return o.outcome.priestCaptured(owner);
    }
    // 원문은 동일 청크 signed 경계를16배 후 float로 저장한다.
    regionBounds(region) {
        this.#live(); const rect = this.#owner.chunks.bounds(region);
        return [rect.left, rect.top, rect.right, rect.bottom].map((value) => Math.fround(value << 4));
    }
    // 공통 후처리의 실제 선택 수명과 현재 Client 상태를 매번 읽는다.
    recheckSelection() {
        this.#live(); const o = this.#owner; const selection = o.activeSelection?.deref()?.selection;
        if (selection) { selection.recheckMouse((x, y, type) => o.world.find(float(x), float(y), type)); return; }
        const current = o.gameplayGlobals.selection; if (!current.mouseType) return;
        const found = o.world.find(float(current.remembered[0]), float(current.remembered[1]), current.mouseType);
        if (!found || (current.selected && current.selected !== found)) return;
        throw new Error('공통 후처리 현재 선택 몸체 수명 미연결');
    }
    // 실제 객체/간헐천 통지를 같은 현재 본체에 연결하고, 준비/전략·경제는 구성만 한다.
    bind() {
        const o = this.#owner; const c = o.core;
        if (this.#disposed || !o.aiModules?.brainsReady || this.#started) throw new Error('Session AI 월드의 초기화 전/반복 결선'); this.#started = true;
        const objects = {
            carrierTargets: (sid) => this.carrierTargets(sid),
            distanceSquared: (first, second) => { this.#live(); const target = o.gunTarget.center(this.#sid(second)); const source = o.gunTarget.center(first); const dx = source[0] - target[0]; const dy = source[1] - target[1]; return dx * dx + dy * dy; },
            log: (token, value) => {
                this.#live(); const raw = o.aiMemory.bytes(token); if (!dword(raw, 0xb00)) return;
                const log = need(o.services.player.log, '생성/삭제 통지 로그'); let text;
                if (value.kind === AI_OBJECT_LOG.built) { if (value.type >= c.types.length) throw new RangeError('AI 건설 로그 현재 TYPE 범위'); text = `Built ${c.types[value.type].name} at (${value.x},${value.y})`; }
                else text = (value.kind === AI_OBJECT_LOG.madeGeyserIneligible ? 'Made Geyser ' : 'Cannot make Geyser ') + value.object + ' Ineligible.\n';
                log(29, `AI${dword(raw, 0x260) | 0}: ${text}\n`);
            },
        }; this.objectsHooks = objects;
        o.aiObjects = new RawAiObjectNotice(o.aiMemory, c.pool, c.types, o.players, () => c.processState.now, () => o.gameplayGlobals.gameAux().outpostType, () => c.ownerMode.challenge, objects);
        c.postPop.bindAiNotice((owner, sid, added) => { this.#live(); o.aiObjects.notice(owner, sid, added); });
        o.world.commonDeletion.bindPatchAiNotice((owner, sid, added) => { this.#live(); o.aiObjects.notice(owner, sid, added); });
        c.postPop.bindLocalOwner(() => o.gameplayGlobals.player.battleOwner); c.postPop.bindSelectionRecheck(() => this.recheckSelection());
        const registration = {
            center: (sid) => { this.#live(); return o.gunTarget.center(sid); },
            fartherGeyser: (token, sid) => { this.#live(); const raw = o.aiMemory.bytes(token); if (!dword(raw, 0xb00)) return; need(o.services.player.log, '간헐천 거리 로그')(29, `AI${dword(raw, 0x260) | 0}: Not closer to geyser ${sid | 0}.\n\n`); },
            missingSurface: (sid) => { this.#live(); need(o.services.ai.assertion, '간헐천 표면 진단')(`간헐천 표면 누락 SID=${sid}`); },
        }; this.registrationHooks = registration;
        o.aiWorld = new RawAiWorldRegistration(o.aiMemory, c.pool, c.types, o.players, o.alliances, c.hash.view(0), o.aiObjects, registration);
        o.assetBindings.bindPatchGeyserAi((token, sid, state) => { this.#live(); o.aiWorld.geyser(token, sid, state); });
        const preparation = {
            missingEnemyAssertion: () => need(o.services.ai.assertion, '적 목표 진단')('enemyPlayerId != INVALID_PLAYER_ID'),
            priest: (owner) => new RawPriestLookup(c.pool, o.priestState.priests).find(owner), temple: (region) => o.islands.temple(region), maxHitPoints: (sid) => o.hp.maxHitPoints(sid),
            pointDistance: (source, target) => { const dx = source[0] - target[0]; const dy = source[1] - target[1]; return Math.sqrt(dx * dx + dy * dy); },
            graph: (sid) => o.anchor.graphFor(sid), regionBounds: (region) => this.regionBounds(region),
            storedSp: (owner) => float(o.sp.get(owner)), addLocalSp: (delta) => o.hp.addSp(o.playerState.battleOwner, delta), demo: () => o.services.missionOutcome.demo(), knowledgeOpen: () => o.cursorDrop.lockDepth === 0,
            blockingGumpOpen: () => need(o.services.ai.blockingGumpOpen, '현재 차단 Gump 조회')(), initialScan: (token) => { this.#live(); o.aiWorld.fullScan(token); },
        }; this.preparationHooks = preparation;
        o.aiPreparation = new RawAiPreparation(o.aiMemory, c.pool, o.players, o.alliances, { localOwner: () => o.playerState.battleOwner, playerKnown: () => o.playerState.playerKnown, noRegion: () => o.noRegion, invalidGraph: () => c.bookkeeping.invalidGraph >>> 0, now: () => c.processState.now }, preparation);
        const setup = {
            regionBounds: (region) => this.regionBounds(region), temple: (region) => o.islands.temple(region), destroy: (sid, flags) => { this.#live(); o.world.destroy(sid, flags); },
            assertion: (text) => need(o.services.ai.assertion, '전략 준비 진단')(text), demo: () => o.services.missionOutcome.demo() !== 0,
        }; this.setupHooks = setup;
        o.aiStrategySetup = new RawAiStrategySetup(o.aiMemory, o.player, o.aiPreparation, o.priests.carrier, o.contained, o.contentFinder, c.types, o.random, o.alliances,
            { localOwner: () => o.playerState.battleOwner, noRegion: () => o.noRegion, templeProduct: () => o.templeProduct, bridgeType: () => o.gameplayGlobals.gameAux().bridgeType, mapLimit: () => o.cursorManaGlobals.mapLimit, now: () => c.processState.now }, setup);
        o.aiEconomy = new RawAiEconomy(o.aiMemory, o.sp, () => c.processState.now, { demo: () => o.services.missionOutcome.demo() !== 0, captured: (owner) => this.priestCaptured(owner) });
        this.#ready = true;
    }
    // 반납 뒤 이전 외부 통지를 거부한다.
    #live() { if (this.#disposed) throw new Error('반납한 Session AI 월드 부모'); }
    // 같은 Host 정리 뒤 Session 통지만 분리한다. 장기 기본 장치와 자료는 보존한다.
    dispose() { if (this.#disposed) return; this.#disposed = true; if (this.#started) { const o = this.#owner; o.core.postPop.bindAiNotice(null); o.world.commonDeletion.bindPatchAiNotice(null); o.core.postPop.bindSelectionRecheck(null); o.core.postPop.bindLocalOwner(null); } }
    // 이 생성 구간만 완료한 것이며 전체 AI/Session·Load 완료는 아니다.
    get ready() { return this.#ready; }
}
