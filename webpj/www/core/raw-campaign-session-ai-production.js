// ConnectAi2880..2902의 실제 배치·콜렉터/시설·다리 탐색/건설 선택 부모다(225553df, WB04-E625).
// 사용: connectAiWorld 뒤 connectAiProduction. 실제 새 공중3종 전용 실행과 외곽 Session/Load는 이 단계에서 하지 않는다.
import { RawAiDeployment } from './raw-ai-deployment.js';
import { RawAiCollectors } from './raw-ai-collectors.js';
import { RawAiBridgeCandidates } from './raw-ai-bridge-candidates.js';
import { RawAiBridgeSearch } from './raw-ai-bridge-search.js';
import { RawAiFactoryProduction, AI_FACTORY_ACTION } from './raw-ai-factory-production.js';
import { RawAiDrySpots } from './raw-ai-dry-spots.js';
import { RawAiBuildChoice } from './raw-ai-build-choice.js';
import { RegularProcess } from './squid-process.js';

// 실제 비정렬 DWORD와 단정도 SP 비트를 보존한다.
function dword(bytes, offset) { return new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength).getUint32(offset, true); }
function float(bits) { const view = new DataView(new ArrayBuffer(4)); view.setUint32(0, bits, true); return view.getFloat32(0, true); }
// 선택한 실제 장치가 없으면 도달 위치에서만 오류를 낸다.
function need(effect, name) { if (typeof effect !== 'function') throw new Error(`Session AI 생산 실제 ${name} 미연결`); return effect; }

// 같은 실제 owner의 생산/배치 준비 생성 구간이다. 현재 본체/TYPE/난수와 이미 옮긴 큰 하위를 빌린다.
export class RawCampaignSessionAiProduction {
    // 같은 owner와 실패 접두/반복/반납 상태다.
    #owner; #started = false; #ready = false; #disposed = false;
    // C++ 값형 함수 표다. 구성 자체로 생산/배치를 실행하지 않는다.
    deploymentHooks = null; collectorHooks = null; factoryHooks = null; dryHooks = null; choiceHooks = null;
    // 기반 owner만 빌린다.
    constructor(owner) { this.#owner = owner; }
    // 현재 기술 부모의 전체 DWORD를 실제 SID 접근 위치에서만 검증한다.
    technology(owner) { this.#live(); const o = this.#owner; const sid = o.player.technology(owner | 0); if (sid > 65535 || sid >= o.core.pool.capacity) throw new RangeError('AI 생산 현재 기술 부모 SID 범위'); return sid; }
    // 원문2946: 범위 밖은 진단 후 false이며 현재 Player 플래그/사제 표를 다시 읽는다.
    enemyAvailable(owner) {
        this.#live(); const o = this.#owner;
        if (owner < 1 || owner > 8) { need(o.services.ai.assertion, '적 사제 참여 진단')('playerId > INVALID_PLAYER_ID && playerId < MAX_PLAYERS'); return false; }
        return Boolean(o.players[owner * 180 + 0x50] & 0x22) && o.priestState.priests.currentOwnerState()[owner] === 0;
    }
    // 원문2950의 진단 뒤 현재 승패 부모를 쓴다.
    priestDead(owner) { this.#live(); const o = this.#owner; if (owner < 1 || owner > 8) need(o.services.ai.assertion, '사제 생존 진단')('playerId > INVALID_PLAYER_ID && playerId < MAX_PLAYERS'); return o.outcome.priestDead(owner); }
    // 현재 장기 AI 진단 DWORD를 매 로그 시점에 읽는다.
    #log(address, token, text) { this.#live(); const o = this.#owner; need(o.services.player.log, '현재 로그')(o.gameplayGlobals.aiDebug().read(address), `AI${dword(o.aiMemory.bytes(token), 0x260) | 0}: ${text}`); }
    // 실제 자원 수집자 생산의 남은 칸이다. 완성 통지 전인 건설 중 유닛도 세어 기한 사이의 한 개 초과 생산을 막는다.
    // 포획용 비행 유닛의 별도 판단/기존 준비 완료 상태는 바꾸지 않으며 실제 Pool은 읽기만 한다.
    collectorCapacity(token) {
        this.#live(); const o = this.#owner; const c = o.core; const body = o.aiMemory.bytes(token);
        const desired = dword(body, 0x234) | 0, owner = dword(body, 0x260);
        const limit = o.aiPreparation.countAttachedGeysers(token) > 0 ? Math.max(0, desired) : desired !== 0 ? 1 : 0;
        if (!limit) return false;
        if (owner < 1 || owner > 8) throw new RangeError('AI 수집자 생산의 현재 소유자 범위');
        // 매 호출의 한 사본만 읽는다. free/dead/void/contained와 묻힌 후보는 실제 생산 유닛으로 세지 않는다.
        const bytes = c.pool.bytes(); let count = 0;
        for (let offset = 0; offset < bytes.length; offset += 50) {
            if ((bytes[offset + 11] & 15) || (bytes[offset + 40] & 8) || bytes[offset + 34] !== owner) continue;
            const type = bytes[offset + 10]; if (type >= c.types.length) continue;
            const genus = c.types[type].flags2 >>> 0;
            if ((genus & 0x30000) && !(genus & 0x200000) && ++count >= limit) return false;
        }
        return true;
    }
    // 실제 일곱 부모를 원문 순서로 구성한다.
    bind() {
        const o = this.#owner; const c = o.core; const s = o.state;
        if (this.#disposed || !o.aiWorldModules?.ready || this.#started) throw new Error('Session AI 생산의 초기화 전/반복 결선'); this.#started = true;
        const deployment = {
            product: (token, type) => o.aiStrategySetup.findProduct(token, type), technology: (owner) => this.technology(owner),
            destroy: (sid, flags) => { this.#live(); o.world.destroy(sid, flags); },
            reserveRegular: () => { this.#live(); o.aiPendingRegular = new RegularProcess(0, 0); return true; },
            addRegular: (parent, duration) => {
                this.#live(); if (!o.aiPendingRegular) throw new Error('AI 배치 실제 Regular 예약 누락');
                const process = o.aiPendingRegular; o.aiPendingRegular = null; process.initializeDetached(0, duration);
                const attached = c.processes.attachRegular(process, parent); if (!attached) throw new Error('AI 배치 실제 Regular 부착 실패'); attached.resetTask(0, c.processState.now);
            },
            request: (request) => { this.#live(); o.assetModules.prepareConstruction(); o.construction.request(request); },
            localPlace: (request) => { this.#live(); if (!o.cursor) throw new Error('AI 배치 실제 로컬4433b0 미설치'); o.cursor.placeLocal(request); },
            demo: () => o.services.missionOutcome.demo() !== 0, assertion: (text) => need(o.services.ai.assertion, '배치 진단')(text),
        }; this.deploymentHooks = deployment;
        const move = { get value() { return s.constructionState.notice.moveLocalBuilder; }, set value(value) { s.constructionState.notice.moveLocalBuilder = value; } };
        o.aiDeployment = new RawAiDeployment(o.aiMemory, c.pool, c.types, o.contained, { authority: () => o.gameplayGlobals.authority().boss, moveLocalBuilder: move, now: () => c.processState.now }, deployment);
        const collectors = {
            // 실제 생성 전의 완성/건설 중 합계를 같은 세션에서 조회한다.
            collectorCapacity: (token) => this.collectorCapacity(token),
            product: (token, type) => o.aiStrategySetup.findProduct(token, type), canAfford: (token, type) => o.aiDeployment.canAfford(token, type), maxHitPoints: (sid) => o.hp.maxHitPoints(sid),
            enemyAvailable: (owner) => this.enemyAvailable(owner),
            mayPlace: (query) => { this.#live(); o.assetModules.prepareConstruction(); return o.constructionModules.mayPlace(query); },
            deploy: (token, type, x, y, argument, direction) => { this.#live(); o.aiDeployment.place(token, type, x, y, argument, direction); },
            log: (token, type, x, y) => { need(o.services.player.log, '콜렉터 Lay 로그'); if (type >= c.types.length) throw new RangeError('AI 콜렉터 Lay TYPE 범위'); this.#log(0x52f3d0, token, `Lay ${c.types[type].name} at (${x},${y})\n`); },
            dropSound: () => { this.#live(); need(o.services.sound.play, '로컬 드롭 음향')(o.soundNames.lookup('droppiece.wav'), 0, 0, 0, 0, 0); },
        }; this.collectorHooks = collectors;
        o.aiCollectors = new RawAiCollectors(o.aiMemory, c.pool, c.types, o.random, { now: () => c.processState.now, localOwner: () => o.playerState.battleOwner, invalidGraph: () => c.bookkeeping.invalidGraph >>> 0 }, collectors);
        const bridge = () => o.contentBridgeType;
        o.aiBridgeCandidates = new RawAiBridgeCandidates(o.aiMemory, c.pool, c.types, c.frames, c.hash.view(0), c.spots, { bridge, fencemark: () => o.fenceNodeType, windArcher: () => s.gunState.readWindArcherType() });
        o.aiBridgeSearch = new RawAiBridgeSearch(o.aiMemory, c.frames, c.spots, bridge, o.random);
        const factories = {
            technology: (owner) => this.technology(owner), product: (token, type) => o.aiStrategySetup.findProduct(token, type), addProduct: (token, parent, type) => { this.#live(); o.aiStrategySetup.addProduct(token, parent, type); },
            level: (sid) => { this.#live(); const vt = dword(c.pool.slot(sid), 0); if (vt !== 0x509148 && vt !== 0x509210) throw new Error('AI 시설 가상level80 미지원 실제 표'); return o.rewardCount(sid); },
            upgrade: (sid) => { this.#live(); o.factoryUpgradeCommand.request(sid); }, demo: () => o.services.missionOutcome.demo() !== 0,
            log: (token, action, type) => { need(o.services.player.log, '시설 생산 로그'); let text = 'Upgrade factory!\n'; if (action === AI_FACTORY_ACTION.addedProduct) { if (type >= c.types.length) throw new RangeError('AI 시설 생산 로그 TYPE 범위'); text = `Added ${c.types[type].name} to factory.\n`; } this.#log(0x52f3e0, token, text + '\n'); },
        }; this.factoryHooks = factories;
        o.aiFactoryProduction = new RawAiFactoryProduction(o.aiMemory, c.pool, c.types, o.contentFinder, { now: () => c.processState.now, invalidGraph: () => c.bookkeeping.invalidGraph >>> 0 }, factories);
        const dry = {
            energy: (box, x, y, type, owner) => { this.#live(); const result = o.energySupply.evaluate(box.text, x, y, type, owner); box.text = result.output; return result.value; },
            log: (token, fake) => this.#log(0x52f3dc, token, (fake ? 'Killed Fake Dry Spot\n' : 'Killed Dry Spot\n') + '\n'),
        }; this.dryHooks = dry; o.aiDrySpots = new RawAiDrySpots(o.aiMemory, c.pool, dry);
        const choice = {
            captured: (owner) => o.aiWorldModules.priestCaptured(owner), dead: (owner) => this.priestDead(owner), canAfford: (token, type) => o.aiDeployment.canAfford(token, type),
            wallet: (owner) => o.aiWallet.balance(owner), storedSp: (owner) => float(o.sp.get(owner)),
            assertion: (text, line) => need(o.services.ai.assertion, `건설 선택 진단${line}`)(text),
        }; this.choiceHooks = choice;
        o.aiBuildChoice = new RawAiBuildChoice(o.aiMemory, c.types, { localOwner: () => o.playerState.battleOwner, playerKnown: () => o.playerState.playerKnown, collectorType: () => o.templeProduct, daisType: () => s.runeState.readDaisType(), workshops: o.aiWorkshops,
            temple: (theme) => { const address = 0x541244 + theme * 4; if (!o.tutorialTypes.has(address)) throw new RangeError('AI 건설 선택 현재 신전 TYPE 누락'); return o.tutorialTypes.get(address); } }, choice);
        this.#ready = true;
    }
    // 반납 뒤의 오래된 대여를 거부한다.
    #live() { if (this.#disposed) throw new Error('반납한 Session AI 생산 부모'); }
    // Host 정리 뒤 남은 미부착 예약만 반납한다.
    dispose() { if (this.#disposed) return; this.#disposed = true; this.#owner.aiPendingRegular = null; }
    // 이 생성 구간만 완료한 것이며 실제 행동/Session·Load 완료는 아니다.
    get ready() { return this.#ready; }
}
