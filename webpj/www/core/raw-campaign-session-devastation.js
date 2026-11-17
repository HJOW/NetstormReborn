// ConnectDevastation 앞단의 현재 TYPE 표와 상태 주문 결선이다(225553df, WB04-E678).
import { typeNumber, typeString } from '../assets/type-table.js';
import { RawBombStatus } from './raw-bomb-status.js';
import { dispatchCampaignFence } from './raw-campaign-fence-dispatch.js';
import { FENCE_TYPE_OPERATION } from './raw-fence-type-parent.js';
import { RawWindSpell } from './raw-wind-spell.js';
import { RawManoSpell } from './raw-mano-spell.js';
import { WindAviaryProcessFields } from './raw-wind-aviary-state.js';
import { WindAviaryProcess, findAttachedProcess } from './raw-wind-aviary-process.js';
import { currentAllocationBytes } from './current-allocation-bytes.js';
import { RawBombTreason, BOMB_TREASON_ROLE } from './raw-bomb-treason.js';
import { RawBombHeal } from './raw-bomb-heal.js';
import { RawBombHardener } from './raw-bomb-hardener.js';
import { RawBombStrike } from './raw-bomb-strike.js';
import { projectCameraCoordinates } from '../render/camera-view.js';
import { RawBombTargets } from './raw-bomb-targets.js';
import { RawBombBlast } from './raw-bomb-blast.js';
import { makeCampaignBombBlastHooks } from './raw-bomb-blast-bindings.js';
import { connectedCampaignBombTable } from './raw-campaign-session-particles.js';
import { RawBombActivation } from './raw-bomb-activation.js';
import { RegularProcess } from './squid-process.js';
import { RawBombRegular } from './raw-bomb-regular.js';
import { RawFrameAdvance } from './raw-frame-advance.js';
import { RawBombEffect, finalizeBombEffectPacket } from './raw-bomb-effect.js';
import { bombTimerParent, receiveBombTimerPacket } from './raw-bomb-timer-packet.js';

// 같은 현재 가상 표를 기준으로 주문 자격 하위를 선택한다.
const MANO_TABLES = new Set([0x5026a0, 0x502778, 0x502840]);
const STRIKE_TABLES = new Set([0x502908, 0x502b70, 0x5029d0, 0x502aa8]);
const WIND_TABLES = new Set([0x5025d8, 0x502e68, 0x502f30]);

// 실제 Session의 TYPE·World·상태 주문을 한 번 구성한다.
export class RawCampaignSessionDevastation {
    // 같은 owner와 설치 시작·정상 복귀·반납 관문이다.
    #owner; #started = false; #ready = false; #summonsStarted = false; #summonsReady = false; #effectsStarted = false; #effectsReady = false; #strikeStarted = false; #strikeReady = false; #targetsStarted = false; #targetsReady = false; #activationStarted = false; #activationReady = false; #regularStarted = false; #regularReady = false; #effectStarted = false; #effectReady = false; #disposed = false;
    // 현재 상태 주문 효과를 후속 시전 분배기에 빌린다.
    statusHooks = null;
    // 기존 실제 하위에 연결한 트리즌과 힐 효과를 현재 수명에서 보관한다.
    treasonHooks = null; healHooks = null;
    // 직접 피해 주문이 빌리는 현재 상태와 값형 효과 연결이다.
    strikeState = null; strikeHooks = null;
    // 검색기가 값으로 빌리는 현재 반경·자격 분배기다.
    targetHooks = null;
    // 활성화 수신이 소유하는 비용·예약 효과 표다.
    activationHooks = null;
    // 정기 주문의 확보·검색·송수신과 해제 연결이다.
    regularHooks = null;
    // 효과 수신이 값으로 빌리는 실제 시전 종료·타이머·분배 연결이다.
    effectHooks = null;
    // 구성 시 World나 난수는 변경하지 않는다.
    constructor(owner) { this.#owner = owner; }
    // 완료 전 또는 반납된 함수 값의 실행을 거부한다.
    #live() { if (this.#disposed || !this.#ready) throw new Error('반납/미연결 Session 파괴 주문'); }

    // 같은 타입 번호의 시전 프레임·비용·음향과 상태 주문을 설치한다.
    bindStatus() {
        const o = this.#owner, c = o.core;
        if (this.#disposed || this.#started || !o.bridgeCarriers || o.bombStatus) throw new Error('Session 파괴 주문 설치 순서 오류');
        this.#started = true;
        const native = o.world.storage.usesNativeTransmissionDevices?.() ?? false;
        if (!native) o.devastationRegularTypes = new Array(c.types.length);
        o.devastationEffectTypes = new Array(c.types.length);
        // 같은 TYPE 번호의 실제 A 프레임·시전 시간과 비용을 사용한다.
        for (let type = 0; type < c.types.length; type += 1) {
            if (!native) o.devastationRegularTypes[type] = { firstFrame: c.frames[type].run('A').first, castTime: o.carrierRegularModules.types[type].castTime };
            o.devastationEffectTypes[type] = { cost: c.types[type].cost, sound: 0, delay: 0 };
        }
        // 자산 block 번호로 현재 음향 토큰과 지연을 덧붙인다.
        for (const asset of o.assets.game.types()) {
            const type = 70 + asset.block, row = o.devastationEffectTypes[type];
            if (!row) throw new RangeError('파괴 주문 실제 자산 TYPE 범위');
            const sound = typeString(asset.definition, 'fireSound'), delay = typeNumber(asset.definition, 'effecttime') ?? 0;
            if (!Number.isFinite(delay) || delay < -2147483648 || delay > 2147483647) throw new RangeError('파괴 주문 effecttime 범위');
            row.sound = sound ? o.soundNames.lookup(sound) : 0; row.delay = Math.trunc(delay);
        }
        const hooks = {
            refresh: (sid) => { this.#live(); c.display.update(c.pool.slot(sid), 0); },
            unpop: (sid, flags) => { this.#live(); o.world.unpop(sid, flags); },
            repop: (sid, flags) => { this.#live(); o.world.repop(sid, flags); },
            fence: (sid) => { this.#live(); dispatchCampaignFence(o, sid, FENCE_TYPE_OPERATION.refresh); },
            flare: (sid, count, event, type, direction) => { this.#live(); o.flare.spawnDirection(sid, count, event, type, direction); },
        };
        o.bombStatusState.alliances = o.alliances;
        o.bombStatus = new RawBombStatus(c.pool, c.types, o.bombStatusState, hooks);
        this.statusHooks = hooks; this.#ready = true;
    }

    // 같은 현재 spawns와 장기 TYPE를 Wind·Mano 주문에 연결한다.
    bindSummons() {
        const o = this.#owner, c = o.core;
        if (this.#disposed || !this.#ready || this.#summonsStarted || !o.windAviary || o.windSpell || o.manoSpell) throw new Error('Session 소환 주문 설치 순서 오류');
        this.#summonsStarted = true;
        if (o.world.storage.usesNativeTransmissionDevices?.()) throw new Error('Native 장기 주문 spawns는 4D-8에서 연결');
        o.windSpellCounts = new Int32Array(c.types.length);
        // 실제 자산 block의 현재 spawns를 같은 TYPE 저장소에 채운다.
        for (const asset of o.assets.game.types()) {
            const type = 70 + asset.block, count = typeNumber(asset.definition, 'spawns') ?? 0;
            if (type < 0 || type >= o.windSpellCounts.length || !Number.isFinite(count) || count < -2147483648 || count > 2147483647) throw new RangeError('소환 주문 spawns TYPE/값 범위');
            o.windSpellCounts[type] = Math.trunc(count);
        }
        o.windSpellState.counts = o.windSpellCounts;
        const summons = {
            exists: (parent, type) => { this.#live(); return findAttachedProcess(c.pool, parent, type) !== 0; },
            allocate: () => { this.#live(); const fields = new WindAviaryProcessFields(); currentAllocationBytes(fields.bytes); return fields; },
            attach: (fields, parent, type, flags) => { this.#live(); c.processes.attach(new WindAviaryProcess(o.windAviary, fields), type, parent, flags); },
        };
        const env = { get server() { return c.processState.server; }, get now() { return c.processState.now; } };
        o.windSpell = new RawWindSpell(c.pool, env, o.windSpellState, summons);
        const mano = {
            wallSeconds: o.services.sound.wallSeconds,
            spellType: (level) => { this.#live(); const types = o.gameplayGlobals.gameAux().manoSpellTypes; if (level >= types.length) throw new RangeError('맨 오 워 주문 레벨 범위'); return types[level]; },
            flyerType: () => { this.#live(); return o.gameplayGlobals.gameAux().rainFlyerType; },
            create: (type, flags) => { this.#live(); return c.factory.create(type, flags); },
            owner: (sid, unused, owner) => { this.#live(); o.ownership.setOwner(sid, owner); },
            header: (sid, abstract, inlined) => {
                this.#live(); const type = c.pool.slot(sid)[10], record = c.types[type], shape = c.shapes[type];
                if (!record || !shape) throw new RangeError('맨 오 워 주문 현재 헤더 TYPE 물리 범위');
                const limit = Math.imul(shape.frameCount, (record.flags1 & 0x440000) ? 2 : 1);
                if (limit < 1) o.world.storage.recordDisplayFrameError(type, 0, abstract);
                if (limit < 1 || !shape.loaded || record.maxHitPoints === -0x22222223 || record.maxHitPoints === -0x32323233) o.world.storage.assertDisplay('frameCheck( abstract, frame, false )', inlined ? 'c:\\ns\\o\\rifttype.h' : '..\\o\\rifttype.h', 0xe1);
                const header = c.shapes[type].frames[0]; if (!header) throw new RangeError('맨 오 워 주문 현재 SHP 프레임 범위');
                return [header.height, header.hotspotY];
            },
            center: (sid) => { this.#live(); return c.neighbors.center(sid); },
            pop: (sid, unused, x, y, flags) => { this.#live(); o.world.pop(sid, x, y, flags); },
        };
        o.manoSpell = new RawManoSpell(c.pool, () => c.processState.server, o.windSpellCounts, mano);
        this.#summonsReady = true;
    }

    // 같은 풀·소유권·받침과 현재 펜스에 트리즌·힐·하드너를 연결한다.
    bindEffects() {
        const o = this.#owner, c = o.core;
        if (this.#disposed || !this.#summonsReady || this.#effectsStarted || o.bombTreason || o.bombHeal || o.bombHardener) throw new Error('Session 파괴 효과 설치 순서 오류');
        this.#effectsStarted = true;
        const treason = {
            role: (role) => {
                this.#live(); const aux = o.gameplayGlobals.gameAux();
                switch (role) {
                    case BOMB_TREASON_ROLE.NoIsland: return c.bridgeState.noIslandType;
                    case BOMB_TREASON_ROLE.EmptyGeyser: return aux.emptyGeyserType;
                    case BOMB_TREASON_ROLE.Fence: return aux.sunFenceType;
                    case BOMB_TREASON_ROLE.Bridge: return c.bridgeState.bridgeType;
                    case BOMB_TREASON_ROLE.Connector: return c.bridgeState.connectorType;
                    case BOMB_TREASON_ROLE.Flare: return aux.bombFlareType;
                    default: throw new RangeError('트리즌 현재 역할 범위');
                }
            },
            owner: (sid, unused, owner) => { this.#live(); o.ownership.setOwner(sid, owner); },
            refresh: (sid) => { this.#live(); c.display.update(c.pool.slot(sid), 0); },
            fence: (sid, rebuild) => { this.#live(); dispatchCampaignFence(o, sid, rebuild ? FENCE_TYPE_OPERATION.rebuild : FENCE_TYPE_OPERATION.refresh); },
            find: (x, y, type) => { this.#live(); return o.world.find(x, y, type); },
            support: (x, y, owner) => { this.#live(); o.world.setSupportOwner(x, y, owner); },
            assertion: (expression, file, line) => { this.#live(); o.world.storage.assertDisplay(expression, file, line); },
            trace: (level, message) => { this.#live(); o.world.storage.traceGraph(level, message); },
            flare: (sid, count, event, type, direction) => { this.#live(); o.flare.spawnDirection(sid, count, event, type, direction); },
        };
        o.bombTreason = new RawBombTreason(c.pool, c.hash, c.types, c.frames, treason);
        const heal = {
            hasHitPoints: (sid) => { this.#live(); return o.hp.hasHitPoints(sid); },
            maximumHitPoints: (sid) => { this.#live(); return o.hp.maxHitPoints(sid); },
            fenceType: () => { this.#live(); return o.state.gunTargetState.sunFenceType; },
            burstType: () => { this.#live(); return o.healBurstType; },
            fence: (sid) => { this.#live(); dispatchCampaignFence(o, sid, FENCE_TYPE_OPERATION.rebuild); },
            unpop: (sid, unused, flags) => { this.#live(); o.world.unpop(sid, flags); },
            repop: (sid, unused, flags) => { this.#live(); o.world.repop(sid, flags); },
            refresh: (sid) => { this.#live(); c.display.update(c.pool.slot(sid), 0); },
            create: (type, flags) => { this.#live(); return c.factory.create(type, flags); },
            pop: (sid, unused, x, y, flags) => { this.#live(); o.world.pop(sid, x, y, flags); },
        };
        o.bombHeal = new RawBombHeal(c.pool, c.types, heal);
        o.bombHardener = new RawBombHardener(c.pool, c.types, c.frames, o.random, c.frame, (sid, count, event) => { this.#live(); o.flare.spawn(sid, count, event); });
        this.treasonHooks = treason; this.healHooks = heal; this.#effectsReady = true;
    }

    // 현재 장기 역할·공유 난수와 실제 피해·입자·카메라를 직접 피해 주문에 연결한다.
    bindStrike() {
        const o = this.#owner, c = o.core, self = this;
        if (this.#disposed || !this.#effectsReady || this.#strikeStarted || o.bombStrike) throw new Error('Session 직접 피해 주문 설치 순서 오류');
        this.#strikeStarted = true;
        const hit = o.hits.damageHooks({}).damage;
        const state = {
            get boss() { self.#live(); return o.state.assetState.readBoss(); },
            get specialType() { self.#live(); return o.gameplayGlobals.gameAux().windFlyerType; },
            get firstEffectType() { self.#live(); return o.gameplayGlobals.gameAux().gravityEffectType; },
            get hailEffectType() { self.#live(); return o.gameplayGlobals.gameAux().meteorEffectType; },
        };
        const hooks = {
            create: (type, flags) => { this.#live(); return c.factory.create(type, flags); },
            pop: (sid, unused, x, y, flags) => { this.#live(); o.world.pop(sid, x, y, flags); },
            destroy: (sid, unused, flags) => { this.#live(); o.world.destroy(sid, flags); },
            damage: (sid, unused, amount, owner, parent) => { this.#live(); if (typeof hit !== 'function') throw new Error('라이트닝 주문 실제 피해 미연결'); hit(sid, amount, owner, parent); },
            sound: (x, y) => { this.#live(); if (typeof o.services.sound.playOnceAt !== 'function') throw new Error('라이트닝 주문 실제 위치 음향 미연결'); o.services.sound.playOnceAt(x, y, o.soundNames.lookup('thunderCrack.wav'), 0, 0); },
            beam: (source, target) => { this.#live(); o.particles.addBeam(source, target, 'shortZap.wav'); },
            project: (x, y) => { this.#live(); const p = o.gameplayGlobals.camera().snapshot().pixels; return projectCameraCoordinates(p.x, p.y, x, y); },
            column: (sid, from, to, life) => { this.#live(); o.particles.addScreenBolt(sid, from, to, life); },
        };
        o.bombStrike = new RawBombStrike(c.pool, c.types, o.random, state, hooks);
        this.strikeState = state; this.strikeHooks = hooks; this.#strikeReady = true;
    }

    // 현재 TYPE 사거리와 같은 실제 폭발·각 주문 자격을 검색기에 연결한다.
    bindTargets() {
        const o = this.#owner, c = o.core;
        if (this.#disposed || !this.#strikeReady || this.#targetsStarted || o.devastationTargets || o.devastationBlast) throw new Error('Session 주문 표적 설치 순서 오류');
        this.#targetsStarted = true;
        const hooks = {
            radius: (parent, table) => { this.#live(); if (!connectedCampaignBombTable(table)) throw new TypeError('주문 반경 표 오류'); return o.devastationTargets.defaultRadius(parent); },
            eligible: (parent, table, target) => {
                this.#live(); table >>>= 0; if (!connectedCampaignBombTable(table)) throw new TypeError('주문 자격 표 오류');
                o.runtimeModules.prepareDevastation();
                if (table === 0x503188) return o.bombTreason.mayTarget(parent, target);
                if (MANO_TABLES.has(table)) return o.manoSpell.mayTarget(parent, target);
                if (STRIKE_TABLES.has(table)) return o.bombStrike.mayTarget(parent, target) !== 0;
                if (table === 0x5030c0) return o.bombHardener.mayTarget(target) !== 0;
                if (table === 0x502448) return o.bombHeal.mayTarget(target) !== 0;
                if (WIND_TABLES.has(table)) return o.windSpell.mayTarget(parent);
                return table === 0x502370 ? o.devastationBlast.mayTarget(target) : o.bombStatus.mayTarget(parent, target);
            },
            assertion: (line) => { throw new Error(`원본 Devastation 권한 진단 ${line}`); },
        };
        o.devastationTargets = new RawBombTargets(c.pool, c.hash, c.types, o.state.devastationTargetState, hooks);
        o.devastationBlast = new RawBombBlast(c.pool, c.types, c.frames, o.devastationTargets, o.state.devastationBlastState, makeCampaignBombBlastHooks(o.world, o.hits, o.devastationTargets));
        this.targetHooks = hooks; this.#targetsReady = true;
    }

    // 현재 비용·소유자와 같은 프로세스 Host에 주문 활성화 수신을 연결한다.
    bindActivation() {
        const o = this.#owner, c = o.core;
        if (this.#disposed || !this.#targetsReady || this.#activationStarted || o.devastationActivation) throw new Error('Session 주문 활성화 설치 순서 오류');
        this.#activationStarted = true;
        const hooks = {
            cost: (type) => { this.#live(); return o.hp.typeCostUnits(type); },
            subtract: (owner, amount) => { this.#live(); o.hp.addSp(owner, amount); },
            assertion: (line) => { throw new Error(`원본 Devastation genus 진단 ${line}`); },
            allocate: (bytes) => {
                this.#live(); if (bytes !== 40 || o.pendingDevastationRegular) throw new Error('Devastation 주기 확보 순서 오류');
                o.pendingDevastationRegular = new RegularProcess(0, 0.1); return true;
            },
            construct: (event, parent, payload) => {
                this.#live(); if (!o.pendingDevastationRegular || event !== 0 || payload !== Math.fround(0.1)) throw new Error('Devastation SharedRegular 확보 누락');
                const process = o.pendingDevastationRegular; o.pendingDevastationRegular = null;
                const attached = c.processes.attach(process, 61, parent, 0x50); if (attached) attached.resetTask(event, c.processState.now);
            },
        };
        o.devastationActivation = new RawBombActivation(c.pool, c.types, () => o.playerState.battleOwner, hooks);
        this.activationHooks = hooks; this.#activationReady = true;
    }

    // 현재 검색과 방송을 사용해 지역 주문 정기의 실제 효과 경계를 연결한다.
    bindRegular() {
        const o = this.#owner, c = o.core;
        if (this.#disposed || !this.#activationReady || this.#regularStarted || o.devastationRegular) throw new Error('Session 주문 정기 설치 순서 오류');
        this.#regularStarted = true;
        if (o.world.storage.usesNativeTransmissionDevices?.()) throw new Error('Native 장기 주문 정기는 4D-8에서 연결');
        const hooks = {
            notice: (carrier, count, event) => { this.#live(); o.flare.spawn(carrier, count | 0, event); },
            frame: (sid, frame, flags) => { this.#live(); c.frame.set(sid, frame | 0, flags); },
            animate: (sid, delta, flags) => { this.#live(); new RawFrameAdvance(c.pool, c.frames, c.frame).advance(sid, delta | 0, flags); },
            allocate: (bytes) => { this.#live(); if (bytes !== 400 || o.devastationSearchMemory) throw new Error('Devastation 검색 배열 확보 순서 오류'); o.devastationSearchMemory = new Uint8Array(bytes); return true; },
            targets: (parent, storage) => { this.#live(); const result = { storage, capacity: storage.length | 0, count: 0 }; o.devastationTargets.find(parent, result); return result.count >>> 0; },
            broadcast: (packet) => {
                this.#live();
                // 실제 연결된 비로컬 참가자가 있을 때만 현재 패킷 길이를 확정한다.
                for (let owner = 1; owner < 9; owner += 1) {
                    if ((o.players[owner * 180 + 0x50] & 1) && o.services.battlePeers[owner] && owner !== o.playerState.battleOwner) { finalizeBombEffectPacket(packet, true); break; }
                }
                const length = packet[2] | (packet[3] << 8); if (length > packet.length) throw new RangeError('Devastation 실제 송신 범위');
                o.tables.broadcast.send(packet.subarray(0, length));
            },
            receive: (sender, connection, packet) => { this.#live(); o.runtimeModules.receiveDevastationEffect(packet); },
            release: (bytes) => { this.#live(); if (bytes !== 400 || !o.devastationSearchMemory) throw new Error('Devastation 검색 배열 해제 순서 오류'); o.devastationSearchMemory = null; },
        };
        o.devastationRegular = new RawBombRegular(c.pool, o.devastationRegularTypes, o.state.devastationRegularState, hooks);
        this.regularHooks = hooks; this.#regularReady = true;
    }

    // 같은 시전 프로세스·대상 효과·상태 타이머에 효과 패킷 수신을 연결한다.
    bindEffect() {
        const o = this.#owner, c = o.core;
        if (this.#disposed || !this.#regularReady || this.#effectStarted || o.devastationEffect) throw new Error('Session 주문 효과 설치 순서 오류');
        this.#effectStarted = true;
        const hooks = {
            stopCast: (carrier) => {
                this.#live(); let cast;
                // 현재 첫 시전 사건을 반복 재조회하고 실제 Kernel 반납을 확인한다.
                while ((cast = c.processes.findEvent(carrier, 0x493))) {
                    const pid = c.kernel.find(cast); c.processes.kill(cast, 0);
                    if (c.processes.isRegistered(pid, cast)) throw new Error('Devastation 시전 종료의 실제 권한/삭제 실패');
                }
            },
            allocate: (bytes) => {
                this.#live();
                if (!o.devastationEffectMemory) o.devastationEffectMemory = new Uint8Array(bytes);
                else { if (bytes !== 40 || o.pendingBombTimerStorage || o.pendingBombTimer) throw new Error('상태 타이머 확보 순서 오류'); o.pendingBombTimerStorage = new Uint8Array(bytes); }
                return true;
            },
            apply: (parent, table, target) => {
                this.#live(); table >>>= 0; if (!connectedCampaignBombTable(table)) throw new TypeError('주문 효과 표 오류');
                o.runtimeModules.prepareDevastation();
                if (MANO_TABLES.has(table)) return;
                if (table === 0x503188) o.bombTreason.apply(parent, target);
                else if (STRIKE_TABLES.has(table)) o.bombStrike.apply(parent, target);
                else if (table === 0x5030c0) o.bombHardener.apply(parent, target);
                else if (table === 0x502448) o.bombHeal.apply(parent, target);
                else if (table === 0x502370) o.devastationBlast.apply(parent, target);
                else if (table !== 0x5025d8) o.bombStatus.apply(parent, target);
            },
            construct: (delay, unused, members, flags) => {
                this.#live(); if (!o.pendingBombTimerStorage || o.pendingBombTimer) throw new Error('상태 타이머 생성 메모리 누락');
                o.pendingBombTimer = o.teleportModules.makeBombTimer(delay, members, flags); o.pendingBombTimerStorage = null; return 1;
            },
            argument: (x, y) => { this.#live(); if (x || y) throw new Error('상태 타이머 예약 인자 오류'); return bombTimerParent(c.pool, c.types, o.state.assetState.currentResidences().items()); },
            attach: (token, type, parent, form, flags) => {
                this.#live(); if (token !== 1 || !o.pendingBombTimer || parent < 0 || parent > 65535 || form < 0 || form > 65535) throw new Error('상태 타이머 예약 객체/인자 오류');
                const process = o.pendingBombTimer; o.pendingBombTimer = null; c.processes.attach(process, type, parent, flags, form);
            },
            sound: (values) => { this.#live(); if (typeof o.services.sound.play !== 'function') throw new Error('Devastation 실제 효과음 미연결'); o.services.sound.play(...values); },
            destroy: (sid, table, flags) => { this.#live(); if (!connectedCampaignBombTable(table)) throw new TypeError('주문 삭제 표 오류'); o.world.destroy(sid, flags); },
            release: () => { this.#live(); if (!o.devastationEffectMemory) throw new Error('Devastation 효과 배열 해제 순서 오류'); o.devastationEffectMemory = null; },
        };
        o.devastationEffect = new RawBombEffect(c.pool, o.devastationEffectTypes, o.state.devastationEffectState, hooks);
        o.runtimeModules.prepareDevastation(); this.effectHooks = hooks; this.#effectReady = true;
    }

    // 현재 free Form에 타이머 패킷을 읽고 같은 Host의 지정 SID로 부착한다.
    receiveTimer(packet) {
        this.#live(); const o = this.#owner, c = o.core;
        const state = { boss: o.state.assetState.readBoss(), get processType() { return o.bombTimerType; } };
        const hooks = {
            assertion: (line) => { throw new Error(`원본 상태 타이머 수신 진단 ${line}`); },
            create: (type, x, y) => {
                this.#live(); if (type !== 57 || x || y || o.pendingBombTimer) throw new Error('상태 타이머 수신 factory 계약 오류');
                o.pendingBombTimer = o.teleportModules.makeBombTimer(0, [], 0); return 1;
            },
            read: (token, bytes) => { this.#live(); if (token !== 1 || !o.pendingBombTimer) throw new Error('상태 타이머 수신 객체 누락'); o.pendingBombTimer.readPacket(bytes); },
            attach: (token, type, parent, form, flags) => {
                this.#live(); if (token !== 1 || !o.pendingBombTimer) throw new Error('상태 타이머 수신 부착 객체 누락');
                const process = o.pendingBombTimer; o.pendingBombTimer = null; c.processes.attach(process, type, parent, flags, form);
            },
        };
        receiveBombTimerPacket(c.pool, state, packet, hooks);
    }

    // World가 프로세스를 비운 뒤 같은 함수 값의 재진입을 차단한다.
    dispose() { this.#disposed = true; this.#owner.pendingDevastationRegular = null; this.#owner.devastationSearchMemory = null; this.#owner.devastationEffectMemory = null; this.#owner.pendingBombTimerStorage = null; this.#owner.pendingBombTimer = null; }
    // 이 앞단만의 정상 설치 여부이며 전체 주문 설치는 아니다.
    get ready() { return this.#ready; }
    // 주문 결선 완료는 전체 파괴 주문 설치 완료와 구별한다.
    get summonsReady() { return this.#summonsReady; }
    // 이 효과 세 종류만 설치했으며 전체 주문 설치와 구별한다.
    get effectsReady() { return this.#effectsReady; }
    // 직접 피해 주문 설치만 완료했으며 표적 검색은 아직 별도다.
    get strikeReady() { return this.#strikeReady; }
    // 검색·폭발 설치만 완료했으며 활성화와 전체 주문은 별도다.
    get targetsReady() { return this.#targetsReady; }
    // 활성화 수신만 설치했으며 정기와 효과 분배는 별도다.
    get activationReady() { return this.#activationReady; }
    // 정기 주문 설치만 완료했으며 효과 수신은 별도다.
    get regularReady() { return this.#regularReady; }
    // 효과 수신 설치만 완료했으며 Meltdown과 전체 Load는 별도다.
    get effectReady() { return this.#effectReady; }
}
