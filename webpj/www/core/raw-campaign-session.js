// RawCampaignSession 외곽 생성자4318..4330과 소멸자 앞단의 typed 조립이다(225553df, WB04-E638).
// 사용: new RawCampaignSession(game, files, fort, mission, services) → 후속 Load → dispose.
// 생성 자체는 미션을 시작하거나 loaded를 켜지 않는다. Login/State 반납 증빙은 후속이다.
import { RawCampaignSessionOwners } from './raw-campaign-session-owners.js';
import { learnCampaignNewTechnology } from './raw-campaign-callbacks.js';
import { refreshLocalRitualData } from './raw-ritual-refresh.js';
import { createCampaignAi, crankCampaignAiMoney, setCampaignAiParameter, prepareCampaignAi, initializeCampaignAi, makeCampaignAi } from './raw-campaign-session-ai-public.js';
import { RawCampaignSessionAiActions } from './raw-campaign-session-ai-actions.js';
import { RawCampaignSessionLoginFront } from './raw-campaign-session-login-front.js';
import { registerFactoryResearchCommand } from './raw-factory-research.js';
import { registerConstructionData } from './raw-packet-dispatch.js';
import { receiveConstructionReject } from './raw-construction-failure-packets.js';
import { battleIncomeAmount, battleIncomePeriod } from './raw-battle-option-slots.js';
import { decodeOriginalText } from '../assets/text-encoding.js';
import { registerDeveloperCommandPacket } from './raw-developer-command-packet.js';
import { registerMissionSettingsPacket } from './raw-mission-settings-packet.js';
import { observePathData } from './raw-path-data.js';
import { DaisProcess, readDaisProcessData } from './raw-dais-process-data.js';
import { receiveSessionSquid } from './raw-campaign-session-squid.js';
import { decodeCompressedSquid, observeCompressedSquid } from './raw-squid-compressed-receive.js';
import { registerGeyserMined } from './raw-geyser-mined.js';
import { registerRemoteTellPacket } from './raw-remote-tell-packet.js';
import { receiveRitualChoice } from './raw-ritual-choice.js';
import { executeDeveloperSettings } from './raw-campaign-developer-settings.js';
import { executeDeveloperPlayers } from './raw-campaign-developer-players.js';
import { executeDeveloperReports } from './raw-campaign-developer-reports.js';
import { executeDeveloperObjects } from './raw-campaign-developer-objects.js';
import { executeDeveloperBridge } from './raw-campaign-developer-bridge.js';
import { executeDeveloperSubscriber } from './raw-campaign-developer-subscriber.js';
import { GunProcess, GunProcessFields } from './raw-gun-process.js';
import { FlyerProcess, FlyerProcessFields, RawFlyerProcess } from './raw-flyer-process.js';
import { findAttachedProcess, WindAviaryProcess } from './raw-wind-aviary-process.js';
import { WindAviaryProcessFields, RawWindAviaryState } from './raw-wind-aviary-state.js';
import { SunAviaryProcess, SunAviaryProcessFields } from './raw-sun-aviary-process.js';
import { RainAviaryProcess, RainAviaryProcessFields } from './raw-rain-aviary-process.js';
import { readAviaryProcessData } from './raw-aviary-process-data.js';
import { receiveRemoteDeletedSquid, receiveRemoteDeletedSquids, observeRemoteDeletedSquids, receiveContentSquid, receiveDependencySquid, receiveDeletedSquid } from './raw-squid-remote-deletion.js';
import { setRawSquidWord } from './raw-squid-word.js';
import { SquidFactory } from './squid-factory.js';
import { isFormConstructorVtable } from './squid-forms.js';
import { SQUID_FORM_VTABLES } from './squid-vtables.js';
import { beginSquidVerification, receiveFreeSquidVerification } from './raw-squid-verification.js';

// 분산 SP DWORD를 원본 단정도로 읽고 같은 비트 표현으로 쓴다.
function spFloat(bits) { const data = new DataView(new ArrayBuffer(4)); data.setUint32(0, bits, true); return data.getFloat32(0, true); }
// 현재 단정도를 분산 SP 저장소의 DWORD로 바꾼다.
function spBits(value) { const data = new DataView(new ArrayBuffer(4)); data.setFloat32(0, value, true); return data.getUint32(0, true); }

// Impl990..1865의 이미 이식한 생성 순서다. 각 단계는 같은 owner의 실제 하위 모듈을 한 번 만든다.
const CONSTRUCTOR_STEPS = Object.freeze([
    'PlayerTablesAndPackets', 'IslandsAndPriests', 'PlayersAndOwnership', 'BodiesAndTypeTables', 'GunAndCursorViews',
    'HitBodies', 'GunBodies', 'Particles', 'Teleport', 'Combat', 'Vortex', 'Assets', 'WorldRuntimeHooks',
    'ContentNotices', 'ContentLifetime', 'Production', 'Factories', 'Flare', 'Construction', 'Damageable', 'IceTower',
    'CarrierSteal', 'BombPostPop', 'AcidZap', 'AcidFence', 'CarrierRegular', 'SunAviary', 'WindAviary', 'RainAviary',
    'ArcSpire', 'Flyer', 'Fence', 'GemReward', 'WorldDeletion', 'WorldRegular', 'WorldTransmission', 'Terrain',
]);

// 외곽 주소를 게시하고 같은 Impl만 소유한다. Client 앱 수명 연결은4D-6·4D-7에서 수행한다.
export class RawCampaignSession {
    // 실제 Impl 소유자와 외곽 반납 관문이다. 외부에는 원시 owner를 공개하지 않는다.
    #owner = null; #login = null; #disposed = false;

    // 모든 Impl 모듈이 구성된 뒤 외곽 callback을 연결하고 마지막에 현재 Session을 게시한다.
    constructor(game, files, fort, mission, services = {}) {
        try {
            const login = services.localLogin; // C++ optional 함수 표의 값형 사본이며 공유 transport owner 자체는 유지한다.
            const input = { ...services, localLogin: login && typeof login === 'object' ? { ...login, transport: login.transport ? { ...login.transport } : login.transport } : login };
            const o = new RawCampaignSessionOwners(game, files, mission, input); this.#owner = o;
            o.initializePlayersAndSp();
            // source 생성자의 실제 조립 순서를 유지하며 초기화 누락을 성공으로 넘기지 않는다.
            for (const name of CONSTRUCTOR_STEPS) o[`connect${name}`]();
            o.connectWorldInstall(fort); o.connectOutcome(); o.connectAiBrains(); o.connectAiWorld(); o.connectAiProduction();
            o.learnNewTechnology = (type) => this.learnNewTechnology(type);
            o.vortexSnapshot = () => this.#snapshotCurrent();
            o.writeCurrentFortCore = (slot) => this.#live().writeFortCoreDataBody(slot);
            o.createWorldGeysers = () => this.generateWorldGeysers();
            o.placeBattleNuggets = (rectangle, quantity) => this.placeBattleNuggets(rectangle, quantity);
            // 31h는 같은 장기 설정을 사용하며 아직 이식하지 않은 명령은 명시적으로 중단한다. (2026-11-18 WB04-E740)
            o.executeDeveloperCommand = (text) => {
                const current = this.#live();
                const trace = (level, message) => current.world.storage.traceGraph(level, message);
                if (!executeDeveloperBridge(text, current.world.storage, trace) && !executeDeveloperSubscriber(text, current, trace) && !executeDeveloperSettings(text, current.gameplayGlobals.gameAux(), trace, current.services) && !executeDeveloperPlayers(text, current.players, trace, (owner, target) => current.participation.addAlly(owner, target)) && !executeDeveloperReports(text, current.core, current.world.storage.graph, trace, current.world.storage) && !executeDeveloperObjects(text, current, trace)) throw new Error('개발자 명령의 나머지 해석기 미연결');
            };
            o.refreshLocalRitual = () => {
                const current = this.#live();
                refreshLocalRitualData({ // 현재 Core/owner/CoreData와 두 독립 snapshot을 같은 외곽에 연결한다.
                    core: () => current.refreshCurrentPlayerCore(), templateOwner: () => current.playerState.templateOwner,
                    coreData: (slot) => current.writeFortCoreDataBody(slot), snapshot: () => this.#snapshotCurrent(),
                });
            };
            o.gameplayUse.publish(this);
        } catch (error) {
            this.#disposed = true; this.#owner?.dispose(); throw error;
        }
    }

    // 반납한 외곽 주소가 낡은 Impl이나 다음 세대의 전역을 다시 사용하지 못하게 한다.
    #live() { if (this.#disposed || !this.#owner) throw new Error('반납한 외곽 Session'); return this.#owner; }
    // 호출 시 살아 있는 같은 owner로 AI 행동 전달기를 구성한다.
    #aiActions() { return new RawCampaignSessionAiActions(this.#live()); }
    // 두 독립 if마다 현재 모드/owner를 다시 읽는다. manager 부재의 선행 생략을 추가하지 않는다.
    #snapshotCurrent() {
        const o = this.#live();
        if (o.core.ownerMode.battle) this.snapshotBattle(o.playerState.templateOwner);
        if (o.core.ownerMode.fort) this.snapshotFort(o.playerState.templateOwner);
    }

    // 같은 실제 World/Core를 빌리는 최소 공개 조회다. 나머지 공개 API는4D-6c에서 옮긴다.
    world() { return this.#live().world; }
    // C++ Core의 값형 aggregate를 돌려주되 Pool/권한/장부의 실제 참조는 그대로 빌린다(6382).
    core() { return { ...this.#live().core }; }
    // 원문의 설치 순서로 같은 실제 월드와 모든 모듈을 연결한다. 오류 시 앞선 효과를 되돌리지 않는다.
    load() {
        const o = this.#live();
        o.connectBuilding(); o.connectMotion(); o.runtime.load();
        o.connectDevastation(); o.connectCarrierDrop(); o.connectPriestPrayer(); o.connectBuilder();
        o.connectGameplayPlacement(); o.connectCursor(); o.loaded = true;
    }
    // 생성과 실제 설치 완료를 구별한다.
    get loaded() { return this.#live().loaded; }
    // 실제 Login 소유자를 만들고 27h 수신 시 같은 Session의 Load·게임 수신 등록을 원본 순서로 실행한다.
    #beginLocalConnection(parentRunning) {
        const o = this.#live();
        if (o.loaded || this.#login || !o.services.localLogin) throw new Error('실제 로컬 Login의 새 수명/장치 계약 오류');
        const login = new RawCampaignSessionLoginFront(o, o.services.localLogin);
        try {
            login.bindServer(() => {
                this.load();
                this.registerCarrierCommandPrefix(login.transport.dispatch);
                this.registerConstructionCommands(login.transport.dispatch);
                this.registerFactoryResearchCommands(login.transport.dispatch);
            });
            login.bindClient(); login.registerReceivers();
        } catch (error) { login.dispose(); throw error; }
        this.#login = login; login.begin(parentRunning);
    }
    // 독립 미션의 같은 상태8 시작을 실행한다.
    startLocalConnection() { this.#beginLocalConnection(false); }
    // 외부 State7 부모의 공통 확정을 중첩하지 않고 실제 후반만 준비한다.
    prepareLocalConnectionForState() { this.#beginLocalConnection(true); }
    // 원본의 로컬 로그인 상태만 Poll하고 큰 부모 상태는 호출자에게 남긴다.
    pollLocalConnectionState() {
        const o = this.#live(); if (!this.#login) return false;
        if (![8, 9, 10, 13, 14].includes(o.localStateGlobals.state().next[0])) return false;
        this.#login.state.poll(); return true;
    }
    // 현재 Login의 실제 송신·시계·수신만 진행한다.
    pumpLocalConnection() { this.#live(); if (this.#login) this.#login.pump(); }
    // 원본처럼 설치·상태14·현재 Player와 연결 비트를 모두 확인한다.
    localConnectionReady() {
        const o = this.#live(); if (!this.#login || !o.loaded || o.localStateGlobals.state().phase[0] !== 14 || !o.playerState.playerKnown) return false;
        const owner = o.playerState.battleOwner; return owner < 9 && (o.players[owner * 180 + 0x50] & 1) !== 0;
    }
    // 현재 확정된 원본 상태와 실제 수신 기록을 조회한다.
    localConnectionPhase() { const o = this.#live(); return this.#login ? o.localStateGlobals.state().phase[0] : 0; }
    // 기록은 호출자에게 복사본으로 돌려 같은 실제 목록의 임의 변경을 막는다.
    localConnectionPackets() { this.#live(); return this.#login ? this.#login.wireTypes.slice() : []; }
    // 설치된 같은 Session의 연구 본체를 현재 분배기에 등록한다.
    registerFactoryResearchCommands(dispatch) {
        const o = this.#live(); if (!o.loaded || !o.factoryResearch) throw new Error('미설치 Session의 연구 수신 등록');
        registerFactoryResearchCommand(dispatch, o.factoryResearch);
    }
    // 현재 분산 SP의 실수값을 읽는다.
    sp(owner) { return spFloat(this.#live().sp.get(owner)); }
    // 현재 분산 SP에 같은 단정도 값을 기록한다.
    setSp(owner, value) { this.#live().sp.set(owner, spBits(value)); }
    // requestCarrierMove(parent SID, { x, y }, sacrifice=0): 설치된 Session의 현재 선택·연결·내부 전송에 실제 이동 요청을 전달한다.
    requestCarrierMove(parent, destination, sacrifice = 0) {
        const o = this.#live();
        if (!o.loaded || !o.carrierMove) throw new Error('미설치 Session의 이동 명령');
        const point = { x: Math.fround(destination.x), y: Math.fround(destination.y) };
        o.carrierMove.request(parent, point, sacrifice);
    }
    // 현재 시각을 같은 건설·에셋·전투·입자·음향·AI·월드에 원본 순서로 공급한다.
    prepareFrame(time, paused) {
        const o = this.#live(), current = { ...time };
        o.flyerPaused = paused;
        if (o.world.storage.processes.hasDevices()) current.game = o.core.processState.now;
        else o.core.processState.now = current.game;
        o.assetModules.prepareBuilding(current.game); o.assetBindings.prepareFrame(current.game);
        o.combat.prepareFrame(current); o.particles.prepareFrame(current.game, current.number >>> 0);
        o.state.soundState.frame = current.number >>> 0;
        if (!paused) { if (!o.aiRuntime) throw new Error('실제 Session 자동 AI 프레임 설치 전 호출'); o.aiRuntime.frame(); }
        o.world.prepareFrame(current);
    }
    // 다리 붕괴 검사와 준비를 수행한 뒤 정지하지 않은 Kernel만 한 번 실행한다.
    runFrame(time, paused) {
        const o = this.#live(); o.world.scanBridgeDecay(time, paused); this.prepareFrame(time, paused);
        if (!paused) o.core.kernel.runFrame();
    }
    // 현재 모드와 같은 보상 본체를 사용해 지정 Player에 SP를 가산한다.
    #addMoney(owner, amount) {
        const o = this.#live(); o.rewardState.fixedSpA = o.core.ownerMode.fort; o.rewardState.fixedSpB = Boolean(o.globals.editor);
        o.rewardState.aiMirrorDisabled = o.core.ownerMode.challenge; o.hp.addSp(owner, Math.fround(amount));
    }
    // 6ch 수신은 잔액 관문·가산·안내·서버 방송을 원본 순서대로 처리한다.
    #receiveConstructionMoney(packet) {
        const o = this.#live(); if (!(packet instanceof Uint8Array) || packet.length < 11) throw new RangeError('SP 통지의 실제 11BYTE 부족');
        const amount = () => new DataView(packet.buffer, packet.byteOffset, packet.byteLength).getInt32(7, true);
        const notice = (kind, player, value) => {
            const status = o.services.localLogin?.status; if (typeof status !== 'function') throw new Error('SP 통지의 실제 MessageBoard 장치 미연결');
            status(kind, player, value >>> 0);
        };
        if (!packet[6]) {
            if (o.core.processState.server && packet[5] !== o.playerState.battleOwner) {
                const source = packet[5], available = source > 0 && source < 9 ? this.sp(source) : 0;
                if (available < amount()) { o.assetModules.prepareConstruction(); o.construction.chargeShortfall(0, packet[5], -10000, -10000, 0, 0); return; }
            }
            if (packet[5] !== o.playerState.battleOwner) this.#addMoney(packet[5], (-amount()) | 0);
        }
        if (packet[6] === 2) {
            if (!battleIncomePeriod(o.battleOptions)) return; const current = amount();
            if (current !== battleIncomeAmount(o.battleOptions)) return; notice(18, packet[5], current);
        }
        this.#addMoney(packet[4], amount());
        if (packet[4] === o.playerState.battleOwner && !packet[6]) notice(4, packet[5], amount());
        if (o.core.processState.server) o.tables.broadcast.send(packet);
    }
    // 설치된 건설·회수·승천·취소·SP 수신을 같은 실제 Session 분배 표에 등록한다.
    registerConstructionCommands(dispatch) {
        const o = this.#live(); if (!o.loaded) throw new Error('미설치 Session의 건설 수신 등록');
        o.construction.register(dispatch, () => o.assetModules.prepareConstruction());
        if (!o.buildingData) throw new Error('실제 건설 상태 수신 본체 미설치');
        dispatch.requireLength(0x37, 0xffff); registerConstructionData(dispatch, o.buildingData);
        dispatch.observe(0x54, null); dispatch.version(0x54, 1, 0); dispatch.requireLength(0x54, 8);
        dispatch.register(0x54, (sender, _connection, packet) => { this.#live(); o.salvageCommand.receive(sender, packet); });
        dispatch.observe(0x51, null); dispatch.version(0x51, 1, 0); dispatch.requireLength(0x51, 8);
        dispatch.register(0x51, (sender, _connection, packet) => { this.#live(); o.vortexDeparture.receive(sender | 0, packet); });
        dispatch.observe(0x2f, null); dispatch.version(0x2f, 1, 0); dispatch.requireLength(0x2f, 15);
        dispatch.register(0x2f, (_sender, _connection, packet) => {
            this.#live(); o.assetModules.prepareConstruction(); const c = o.core;
            receiveConstructionReject(packet, { pool: c.pool, hash: c.hash, types: c.types, frames: o.metadata.frames,
                patternTypes: o.state.constructionState.clear.patternTypes, get noIslandType() { return c.bridgeState.noIslandType; }, diagnostics: o.metadata.geometry.decoderDiagnostics },
            (sid, flags) => o.world.destroy(sid, flags));
        });
        dispatch.observe(0x6c, null); dispatch.version(0x6c, 1, 0); dispatch.requireLength(0x6c, 11);
        dispatch.register(0x6c, (_sender, _connection, packet) => this.#receiveConstructionMoney(packet));
    }
    // 각 수신 시 현재 Session의 무시 목록·Tell·안내·방송과 게임 시계를 빌린다.
    #allianceDevices() {
        const o = this.#live(), tell = o.services.factoryTell;
        return {
            boss: () => o.state.assetState.readBoss(), ignoredSubscriber: (subscriber) => o.chatGlobals.ignored.includes(subscriber),
            tell: (text) => { if (typeof tell !== 'function') throw new Error('동맹 협상의 실제 Tell 미연결'); tell(decodeOriginalText(Uint8Array.from(text, (letter) => letter.charCodeAt(0)))); },
            status: (kind, owner, value) => {
                if (this.#login) this.#login.showStatus(kind, owner, value);
                else { const status = o.services.localLogin?.status; if (typeof status !== 'function') throw new Error('동맹 협상의 실제 MessageBoard 미연결'); status(kind, owner, value); }
            },
            broadcast: (packet) => o.tables.broadcast.send(packet), gameSeconds: () => o.core.processState.now,
        };
    }
    // Carrier 등록의 첫 77h·79h·6fh를 같은 Player와 현재 협상 장치에 연결한다.
    registerAllianceNegotiationCommands(dispatch) {
        const o = this.#live(); if (!o.loaded) throw new Error('미설치 Session의 동맹 수신 등록');
        // 원문 순서대로 동일한 6BYTE 직접 수신 metadata를 기록한다.
        for (const type of [0x77, 0x79, 0x6f]) {
            dispatch.observe(type, () => {}); dispatch.version(type, 1, 0); dispatch.requireLength(type, 6);
            dispatch.registerDirect(type, (_sender, _connection, packet) => {
                this.#live(); const hooks = this.#allianceDevices();
                if (type === 0x77) o.participation.receiveAllianceRequest(packet, hooks);
                else if (type === 0x79) o.participation.receiveBrokenAlliance(packet, hooks);
                else {
                    const aux = o.gameplayGlobals.gameAux();
                    o.participation.receiveDraw(packet, { get value() { return aux.scheduledDrawDeadline; }, set value(value) { aux.scheduledDrawDeadline = value; } }, hooks);
                }
            });
        }
    }
    // 33h의 서버 SID만 현재 World에서 Unpop·원문 frame 저장·Pop한다.
    #receivePosition(packet) {
        const o = this.#live(); if (packet.length < 6) throw new RangeError('위치 수신의 WORD SID 부족');
        const sid = packet[4] | (packet[5] << 8), c = o.core;
        if (sid >= 5 && sid < 15000) return;
        if (c.pool.slot(sid)[11] & 7) return;
        o.world.unpop(sid, 0); const raw = c.pool.allocated(sid), data = new DataView(packet.buffer, packet.byteOffset, packet.byteLength);
        new DataView(raw.buffer, raw.byteOffset, raw.byteLength).setUint32(36, data.getUint32(14, true), true);
        const x = Math.fround(data.getInt32(6, true) / 65536), y = Math.fround(data.getInt32(10, true) / 65536);
        o.world.pop(sid, x, y, 0);
    }
    // 34h는 현재 화면 크기와 같은 Client 카메라 목표·갱신 전역에만 요청한다.
    #receiveCamera(packet) {
        const o = this.#live(), data = new DataView(packet.buffer, packet.byteOffset, packet.byteLength);
        const y = data.getFloat32(8, true), x = data.getFloat32(4, true);
        if (typeof o.services.cameraScreenSize !== 'function') throw new Error('카메라 수신의 현재 논리 화면 크기 장치 미연결');
        const size = o.services.cameraScreenSize(); o.gameplayGlobals.camera().requestCenter(x, y, size.x, size.y, o.localStateGlobals.metaRebuild());
    }
    // 35h는 현재 게임 시각으로 건설 효과를 준비한 뒤 실제 섬 정리를 호출한다.
    #receiveDropIsland(packet) {
        const o = this.#live(), data = new DataView(packet.buffer, packet.byteOffset, packet.byteLength);
        const x = Math.fround(data.getInt32(4, true) / 65536), owner = data.getUint32(16, true), flags = data.getUint32(12, true);
        const y = Math.fround(data.getInt32(8, true) / 65536);
        if (!o.buildingEffects) throw new Error('섬 정리 수신의 실제 건설 효과 미설치');
        o.assetModules.prepareBuilding(o.core.processState.now); o.buildingEffects.dropIsland(x, y, flags, owner);
    }
    // 6ah는 신규 Form에만 같은 DaisProcess를 부착하고 기존 Form의 현재 PID 필드만 갱신한다.
    #receiveDaisPacket(packet) {
        const o = this.#live(), c = o.core; if (c.pool.edition !== 'patch1078') throw new TypeError('Dais 수신의 PATCH 판본 불일치');
        if (packet.length < 8) throw new RangeError('Dais 수신의 Form DWORD 물리 부족');
        const form = new DataView(packet.buffer, packet.byteOffset, packet.byteLength).getUint32(4, true);
        if (form > 65535 || form >= c.pool.capacity) throw new RangeError('Dais 수신의 Form DWORD 물리 범위');
        const free = (c.pool.slot(form)[11] & 1) !== 0;
        if (packet.length < 9) throw new RangeError('Dais 수신의 현재 mask BYTE 물리 부족');
        const mask = packet[8], assertion = (expression, line) => o.world.storage.assertDisplay(expression, '..\\o\\_p_daisprocessdata.cpp', line);
        if (free) {
            if (mask !== 0xff) assertion('*(( char *)((char *)zacket+8) ) == (char)0xFFFFFFFF', 0x36);
            const pending = new DaisProcess();
            try {
                const parent = readDaisProcessData(pending.fields, packet, true), type = o.gameplayGlobals.gameAux().daisProcessType;
                if (parent > 65535 || parent >= c.pool.capacity) throw new RangeError('Dais 수신의 parent DWORD 물리 범위');
                c.processes.attach(pending, type, parent, 0x10, form);
            } finally { if (!c.kernel.find(pending)) pending.destroy(); }
            return;
        }
        if (mask & 1) assertion('(*(char *)(( (char *)zacket+8) ) & 1) == 0', 0x3e);
        const current = c.processes.fromForm(form); readDaisProcessData(current instanceof DaisProcess ? current.fields : null, packet, false);
    }
    // 3ah는 새 총 Process만 명시 Form에 부착하고 기존 PID의 부모와 예약을 보존한다.
    #receiveGunPacket(packet) {
        const o = this.#live(), c = o.core;
        if (packet.length < 9) throw new RangeError('총 수신의 실제9BYTE 머리 부족');
        const number = new DataView(packet.buffer, packet.byteOffset, packet.byteLength).getUint32(4, true);
        if (!number || number > 65535 || number >= c.pool.capacity) throw new RangeError('총 수신의 명시 Form SID 물리 범위');
        const assertion = (expression, line) => o.world.storage.assertDisplay(expression, 'c:\\ns\\o\\_p_gunprocessdata.cpp', line);
        if (c.pool.slot(number)[11] & 1) {
            if (packet[8] !== 0xff) assertion('*(( char *)((char *)zacket+8) ) == (char)0xFFFFFFFF', 0x4e);
            o.assetBindings.prepareFrame(c.processState.now); const logic = o.assetBindings.gun, fields = new GunProcessFields(), pending = new GunProcess(logic, fields);
            try {
                logic.initializeReceived(fields); const parent = { value: 0 }; logic.readData(fields, packet, parent);
                if (parent.value > 65535 || parent.value >= c.pool.capacity) throw new RangeError('총 수신의 DWORD 부모 SID 물리 범위');
                c.processes.attach(pending, o.gameplayGlobals.gameAux().gunProcessType, parent.value, 0x10, number);
            } finally { if (!c.kernel.find(pending)) pending.destroy(); }
            return;
        }
        if (packet[8] & 1) assertion('(*(char *)(( (char *)zacket+8) ) & 1) == 0', 0x56);
        const current = c.processes.fromForm(number); if (!(current instanceof GunProcess)) throw new Error('총 수신의 현재 PID/outer 객체 미발견');
        o.assetBindings.prepareFrame(c.processState.now); o.assetBindings.gun.readData(current.fields, packet, { value: 0 });
    }

    // 39h는 임시 객체로 부모 테마를 선택한 뒤 같은 Host의 명시 Form에 실제 객체를 부착한다.
    #receiveFlyerPacket(packet) {
        const o = this.#live(), c = o.core;
        if (packet.length < 9) throw new RangeError('비행 수신의 실제9BYTE 머리 부족');
        const number = new DataView(packet.buffer, packet.byteOffset, packet.byteLength).getUint32(4, true);
        if (!number || number > 65535 || number >= c.pool.capacity) throw new RangeError('비행 수신의 명시 Form SID 물리 범위');
        const assertion = (expression, line) => o.world.storage.assertDisplay(expression, 'c:\\ns\\o\\_p_baseflyerprocessdata.cpp', line);
        const parentSid = (value) => { if (value > 65535 || value >= c.pool.capacity) throw new RangeError('비행 수신의 DWORD 부모 SID 물리 범위'); return value; };
        if (c.pool.slot(number)[11] & 1) {
            if (packet[8] !== 0xff) assertion('*(( char *)((char *)zacket+8) ) == (char)0xFFFFFFFF', 0x60);
            if (!o.flyer) throw new Error('비행 수신의 실제 비행 본체 미연결');
            const probeFields = new FlyerProcessFields(); let probe = new FlyerProcess(o.flyer, probeFields), pending = null;
            try {
                RawFlyerProcess.initializeReceived(probeFields, 2);
                const firstParent = parentSid(RawFlyerProcess.readData(probeFields, packet)), type = c.pool.slot(firstParent)[10];
                if (type >= c.types.length) throw new RangeError('비행 수신의 현재 부모 TYPE 물리 범위');
                const fields = new FlyerProcessFields(); pending = new FlyerProcess(o.flyer, fields); RawFlyerProcess.initializeReceived(fields, c.types[type].theme);
                RawFlyerProcess.destructFields(probeFields); probe.destroy(); probe = null;
                const parent = parentSid(RawFlyerProcess.readData(fields, packet));
                c.processes.attach(pending, o.gameplayGlobals.gameAux().flyerProcessType, parent, 0x10, number);
            } finally {
                if (probe) { RawFlyerProcess.destructFields(probeFields); probe.destroy(); }
                if (pending && !c.kernel.find(pending)) { RawFlyerProcess.destructFields(pending.fields); pending.destroy(); }
            }
            return;
        }
        if (packet[8] & 1) assertion('(*(char *)(( (char *)zacket+8) ) & 1) == 0', 0x76);
        const current = c.processes.fromForm(number); if (!(current instanceof FlyerProcess)) throw new Error('비행 수신의 현재 PID/outer 미발견');
        RawFlyerProcess.readData(current.fields, packet);
    }

    // 86h는 선택된 세 명령만 현재 부모의 첫 비행 Form/PID에 전달한다.
    #receiveFlyerCommand(packet) {
        const o = this.#live(), c = o.core, data = new DataView(packet.buffer, packet.byteOffset, packet.byteLength), command = data.getUint32(12, true);
        const line = command === 1 ? 0xda : command === 2 ? 0xe4 : command === 3 ? 0xee : 0;
        if (!line) return;
        const type = o.gameplayGlobals.gameAux().flyerProcessType, parent = data.getUint32(4, true);
        if (parent > 65535 || parent >= c.pool.capacity) throw new RangeError('비행명령의 실제 DWORD 부모 SID 물리 범위');
        const form = findAttachedProcess(c.pool, parent, type);
        if (!form) o.world.storage.assertDisplay('processSid', '\\Ns\\O\\Flyer.cpp', line);
        const current = c.processes.fromForm(form); if (!(current instanceof FlyerProcess)) throw new Error('비행명령의 현재 Form PID에 대한 실제 Flyer outer 미연결');
        if (!o.flyer) throw new Error('86h 명령의 현재 Flyer 기한 본체 미연결'); o.flyer.command(current.fields, command);
    }

    // 아비어리 데이터는 새 Form만 부착하고 기존 Form은 현재 PID의 필드만 갱신한다.
    #receiveAviaryPacket(message, packet) {
        const o = this.#live(), c = o.core;
        if (![0x38, 0x73, 0x74].includes(message)) throw new RangeError('미지원 아비어리 수신 종류');
        if (packet.length < 9) throw new RangeError('아비어리 수신의 실제9BYTE 머리 부족');
        const number = new DataView(packet.buffer, packet.byteOffset, packet.byteLength).getUint32(4, true);
        if (!number || number > 65535 || number >= c.pool.capacity) throw new RangeError('아비어리 수신의 명시 Form SID 물리 범위');
        const file = message === 0x38 ? 'c:\\ns\\o\\_p_sunaviaryprocessdata.cpp' : message === 0x73 ? 'c:\\ns\\o\\_p_rainaviaryprocessdata.cpp' : 'c:\\ns\\o\\_p_windaviaryprocessdata.cpp';
        const assertion = (expression, line) => o.world.storage.assertDisplay(expression, file, line);
        const parentSid = (value) => { if (value > 65535 || value >= c.pool.capacity) throw new RangeError('아비어리 수신의 DWORD 부모 SID 물리 범위'); return value; };
        if (c.pool.slot(number)[11] & 1) {
            if (packet[8] !== 0xff) assertion('*(( char *)((char *)zacket+8) ) == (char)0xFFFFFFFF', message === 0x74 ? 0x4e : 0x3c);
            const logic = message === 0x38 ? o.sunAviary : message === 0x73 ? o.rainAviary : o.windAviary;
            if (!logic) throw new Error('아비어리의 실제 생성 본체 미연결');
            const fields = message === 0x38 ? new SunAviaryProcessFields() : message === 0x73 ? new RainAviaryProcessFields() : new WindAviaryProcessFields();
            const pending = message === 0x38 ? new SunAviaryProcess(logic, fields) : message === 0x73 ? new RainAviaryProcess(logic, fields) : new WindAviaryProcess(logic, fields);
            try {
                if (message === 0x74) RawWindAviaryState.initialize(fields, c.processState.server, c.processState.now); else logic.initialize(fields);
                const parent = message === 0x74 ? RawWindAviaryState.readProcess(fields, packet) : readAviaryProcessData(fields.bytes, packet);
                const type = message === 0x38 ? o.sunAviaryType : message === 0x73 ? o.rainAviaryType : o.windAviaryType;
                c.processes.attach(pending, type, parentSid(parent), 0x10, number);
            } finally { if (!c.kernel.find(pending)) pending.destroy(); }
            return;
        }
        if (packet[8] & 1) assertion('(*(char *)(( (char *)zacket+8) ) & 1) == 0', message === 0x74 ? 0x56 : 0x44);
        const current = c.processes.fromForm(number);
        if (message === 0x74) RawWindAviaryState.readProcess(current instanceof WindAviaryProcess ? current.fields : new Uint8Array(0), packet);
        else readAviaryProcessData(current instanceof SunAviaryProcess || current instanceof RainAviaryProcess ? current.fields.bytes : new Uint8Array(0), packet);
    }

    // 4ah/4bh는 같은 Client의 검증 DWORD와 현재 TYPE·로그를 공유한다.
    #receiveVerification(packet, freePacket) {
        const o = this.#live(), c = o.core, aux = o.gameplayGlobals.gameAux();
        const state = {
            get sid() { return aux.verifySid; }, set sid(value) { aux.verifySid = value; },
            get previous() { return aux.previousVerifySid; }, set previous(value) { aux.previousVerifySid = value; },
        };
        const hooks = {
            assertion: (expression, file, line) => o.world.storage.assertDisplay(expression, file, line),
            typeName: (sid) => { const type = c.pool.slot(sid)[10]; if (type >= c.types.length) throw new RangeError('검증 간격의 현재 TYPE 물리 범위'); return c.types[type].name.split('\0', 1)[0]; },
            log: o.services.player.log,
        };
        if (freePacket) receiveFreeSquidVerification(c.pool, packet, state, hooks); else beginSquidVerification(c.pool, packet, state, hooks);
    }

    // 2dh는 현재 내용물 TYPE과 실제 Form 가상 표에 맞는 부착 본체를 고른다. (2026-11-18 WB04-E731)
    #receiveContent(packet) {
        const o = this.#live(), c = o.core;
        // 현재 슬롯의 가상 표를 선택할 때마다 다시 읽는다.
        const table = (sid) => {
            const raw = c.pool.slot(sid);
            return new DataView(raw.buffer, raw.byteOffset, raw.byteLength).getUint32(0, true);
        };
        receiveContentSquid(c.pool, packet, {
            // 현재 파인더의 내용물 타입으로 같은 공장의 슬롯을 가져온다.
            contentType: () => o.contentFinder.containedType,
            take: (type, sid) => c.factory.take(type, sid),
            // 공용 WORD 설정기가 선택한 경우에만 지면 변경을 알린다.
            word: (sid, value) => setRawSquidWord(c.pool, sid, value, c.bridgeState.bridgeType, (current) => c.building.notifySurface(current)),
            // Form은 기본 플래그 1을 반환하고 자산은 현재 슬롯의 추가 상태를 읽는다.
            firstPop: (sid) => {
                const current = table(sid);
                if (isFormConstructorVtable(current)) return 1;
                if (!SquidFactory.supportsConstructedVtable(current)) throw new Error('내용물 수신의 현재 가상30 미연결');
                return c.factory.firstPopFlags(sid);
            },
            // 첫 Pop 처리 뒤 현재 가상 표에 맞는 부착 함수를 고른다.
            attach: (sid, parent, flags) => {
                const current = table(sid);
                if (current === SQUID_FORM_VTABLES.content) {
                    o.contentForms.attach(sid, parent, flags);
                    return;
                }
                if (isFormConstructorVtable(current)) {
                    c.factory.attachForm(sid, parent, flags);
                    return;
                }
                if (!SquidFactory.supportsConstructedVtable(current)) throw new Error('내용물 수신의 현재 가상50 미연결');
                o.world.attachBaseAsset(sid, parent, flags);
            },
        });
    }

    // Carrier 등록의 전체 typed 수신기를 원본 순서대로 등록한다. 기존 공개 이름은 호출자 호환을 위해 유지한다.
    registerCarrierCommandPrefix(dispatch) {
        const o = this.#live(); if (!o.loaded) throw new Error('미설치 Session의 Carrier 수신 등록');
        this.registerAllianceNegotiationCommands(dispatch);
        registerDeveloperCommandPacket(dispatch, (text) => {
            this.#live(); if (typeof o.executeDeveloperCommand !== 'function') throw new Error('실제 개발자 명령 해석기 미연결'); o.executeDeveloperCommand(text);
        }, (expression, file, line) => o.world.storage.assertDisplay(expression, file, line));
        registerMissionSettingsPacket(dispatch, o.gameplayGlobals.gameAux().missionSettings());
        // 원본 세 고정 길이의 현재 등록 metadata를 순서대로 설정한다.
        for (const [type, length] of [[0x33, 18], [0x34, 12], [0x35, 20]]) { dispatch.observe(type, null); dispatch.version(type, 1, 0); dispatch.requireLength(type, length); }
        dispatch.register(0x33, (_sender, _connection, packet) => this.#receivePosition(packet));
        dispatch.register(0x34, (_sender, _connection, packet) => this.#receiveCamera(packet));
        dispatch.register(0x35, (_sender, _connection, packet) => this.#receiveDropIsland(packet));
        const storage = o.world.storage;
        dispatch.observePacket(0x30, (packet, sending) => observePathData(packet, sending, (expression, file, line) => storage.assertDisplay(expression, file, line)));
        dispatch.version(0x30, 1, 0); dispatch.requireLength(0x30, 0xffff);
        dispatch.register(0x30, (_sender, _connection, packet) => { this.#live(); if (!o.motion?.paths) throw new Error('Path 수신의 실제 이동 본체 미연결'); o.motion.paths.receivePacket(packet); });
        dispatch.observe(0x6a, null); dispatch.version(0x6a, 1, 0); dispatch.requireLength(0x6a, 0xffff);
        dispatch.register(0x6a, (_sender, _connection, packet) => this.#receiveDaisPacket(packet));
        // 공통 객체 수신은 현재 Session의 같은 Pool과 장기 전역을 사용한다.
        dispatch.observe(0x2c, null); dispatch.version(0x2c, 1, 0); dispatch.requireLength(0x2c, 35);
        dispatch.register(0x2c, (_sender, _connection, packet) => receiveSessionSquid(this.#live(), packet));
        // 총 Process 전체 수신은 같은 Host의 명시 Form과 현재 PID를 쓴다.
        dispatch.observe(0x3a, null); dispatch.version(0x3a, 1, 0); dispatch.requireLength(0x3a, 0xffff);
        dispatch.register(0x3a, (_sender, _connection, packet) => this.#receiveGunPacket(packet));
        // 비행 데이터와 직접 명령은 같은 현재 Process Host를 사용한다.
        dispatch.observe(0x39, null); dispatch.version(0x39, 1, 0); dispatch.requireLength(0x39, 0xffff);
        dispatch.register(0x39, (_sender, _connection, packet) => this.#receiveFlyerPacket(packet));
        dispatch.observe(0x86, () => {}); dispatch.version(0x86, 1, 0); dispatch.requireLength(0x86, 20);
        dispatch.registerDirect(0x86, (_sender, _connection, packet) => this.#receiveFlyerCommand(packet));
        // 발사·회전 데이터는 같은 실제 전투 바인딩의 현재 Host 수신기로 전달한다.
        for (const type of [0x3b, 0x3c]) {
            dispatch.observe(type, null); dispatch.version(type, 1, 0); dispatch.requireLength(type, 0xffff);
            dispatch.register(type, (_sender, _connection, packet) => { this.#live(); if (!o.combat) throw new Error('애니메이션 수신의 실제 전투 본체 미연결'); o.combat.receiveAnimationPacket(type, packet); });
        }
        // 세 아비어리 데이터도 각각 같은 현재 Session의 Host에 전달한다.
        for (const type of [0x38, 0x73, 0x74]) {
            dispatch.observe(type, null); dispatch.version(type, 1, 0); dispatch.requireLength(type, 0xffff);
            dispatch.register(type, (_sender, _connection, packet) => this.#receiveAviaryPacket(type, packet));
        }
        // 원격 삭제는 현재 Pool과 World의 실제 flags 0 삭제를 사용한다.
        dispatch.observe(0x53, () => {}); dispatch.version(0x53, 1, 0); dispatch.requireLength(0x53, 6);
        dispatch.register(0x53, (_sender, _connection, packet) => { this.#live(); receiveRemoteDeletedSquid(o.core.pool, packet, (sid, flags) => o.world.destroy(sid, flags)); });
        dispatch.observePacket(0x62, (packet, sending) => observeRemoteDeletedSquids(packet, sending, (expression, file, line) => o.world.storage.assertDisplay(expression, file, line)));
        dispatch.version(0x62, 1, 0); dispatch.requireLength(0x62, 0xffff);
        dispatch.register(0x62, (_sender, _connection, packet) => { this.#live(); receiveRemoteDeletedSquids(o.core.pool, packet, (sid, flags) => o.world.destroy(sid, flags)); });
        // free 검증과 검증 시작은 같은 장기 두 DWORD를 사용한다.
        for (const [type, length] of [[0x4a, 7], [0x4b, 6]]) {
            dispatch.observe(type, null); dispatch.version(type, 1, 0); dispatch.requireLength(type, length);
            dispatch.register(type, (_sender, _connection, packet) => this.#receiveVerification(packet, type === 0x4a));
        }
        // 내용물 데이터는 현재 Form 가상 표에 따라 실제 부착을 선택한다. (2026-11-18 WB04-E731)
        dispatch.observe(0x2d, null); dispatch.version(0x2d, 1, 0); dispatch.requireLength(0x2d, 18);
        dispatch.register(0x2d, (_sender, _connection, packet) => this.#receiveContent(packet));
        // 삭제 통지는 같은 월드와 공유 SP 추적 상태를 사용한다. (2026-11-18 WB04-E733)
        dispatch.observe(0x2e, null); dispatch.version(0x2e, 1, 0); dispatch.requireLength(0x2e, 8);
        dispatch.register(0x2e, (_sender, _connection, packet) => {
            const current = this.#live(), core = current.core, patch = current.priestState.regen.patch, sp = current.sp;
            receiveDeletedSquid(core.pool, packet, {
                boss: () => core.processState.boss,
                assertion: (expression, file, line) => current.world.storage.assertDisplay(expression, file, line),
                typeName: (sid) => { const type = core.pool.slot(sid)[10]; if (type >= core.types.length) throw new RangeError('삭제 수신의 현재 TYPE 물리 범위'); return core.types[type].name.split('\0', 1)[0]; },
                log: current.services.player.log,
                dependType: () => current.gameplayGlobals.gameAux().dependFormType,
                refund: (sid, owner) => current.hp.refund(sid, owner, true),
                destroy: (sid, flags) => current.world.destroy(sid, flags),
                auditSp: () => {
                    let sample = patch.readSample();
                    if ((sp.offset >>> 0) !== (sp.trackingOffset >>> 0)) patch.writeChangedSample(sample);
                    if (patch.remaining) {
                        if (current.playerState.playerKnown) {
                            const money = spFloat(sp.get(current.playerState.battleOwner));
                            sample = patch.readSample();
                            if (money > 7000) patch.writeChangedSample(sample);
                        }
                        patch.remaining = (patch.remaining - 1) >>> 0;
                    }
                    if (!sp.firstPoolReady) patch.writeChangedSample(sample);
                    if (sp.permutation()[0] === -1) patch.writeChangedSample(sample);
                },
            });
        });
        // 감시 Form은 현재 타입과 가상 표를 확인해 같은 공장에 부착한다. (2026-11-18 WB04-E732)
        dispatch.observe(0x65, null); dispatch.version(0x65, 1, 0); dispatch.requireLength(0x65, 13);
        dispatch.register(0x65, (_sender, _connection, packet) => {
            const current = this.#live(), core = current.core;
            receiveDependencySquid(core.pool, packet, {
                dependType: () => current.gameplayGlobals.gameAux().dependFormType,
                take: (type, sid) => core.factory.take(type, sid),
                dependencyAttach: (sid, parent, flags) => {
                    const raw = core.pool.slot(sid);
                    if (new DataView(raw.buffer, raw.byteOffset, raw.byteLength).getUint32(0, true) !== SQUID_FORM_VTABLES.depend) throw new Error('감시 수신의 현재 가상74 미연결');
                    core.factory.attachForm(sid, parent, flags);
                },
            });
        });
        // 공유 예약은 현재 Process Host에 패킷의 시각과 횟수를 그대로 전달한다. (2026-11-18 WB04-E734)
        dispatch.observe(0x7e, null); dispatch.version(0x7e, 1, 0); dispatch.requireLength(0x7e, 28);
        dispatch.register(0x7e, (_sender, _connection, packet) => this.#live().core.processes.receiveSharedRegular(packet));
        // 압축 객체는 같은 전송 캐시와 TYPE에서 풀어 공통 객체 수신기로 전달한다. (2026-11-18 WB04-E735)
        dispatch.observePacket(0x7a, (packet, sending) => observeCompressedSquid(packet, sending, (expression, file, line) => storage.assertDisplay(expression, file, line)));
        dispatch.version(0x7a, 1, 0); dispatch.requireLength(0x7a, 0xffff);
        dispatch.register(0x7a, (_sender, _connection, packet) => {
            const current = this.#live(), core = current.core;
            const image = decodeCompressedSquid(packet, current.gameplayGlobals.gameAux().squidTransmission, {
                assertion: (expression, file, line) => current.world.storage.assertDisplay(expression, file, line),
                defaultType: () => core.bridgeState.bridgeType,
                type: (type) => { if (type >= core.types.length) throw new RangeError('압축 객체의 현재 TYPE 물리 범위'); return core.types[type]; },
                maximumHp: (selected) => core.factory.readSelectedHitPoints(selected, core.types),
            });
            receiveSessionSquid(current, image);
        });
        // 기존 채집 등록의 metadata를 설정한 뒤 같은 Session의 현재 확인 상태를 사용하는 수신기로 교체한다. (2026-11-18 WB04-E736)
        registerGeyserMined(dispatch, o.geyserMined);
        // 원격 Tell의 원본 바이트는 실제 화면 소비자의 문자열 경계에서 해독한다.
        registerRemoteTellPacket(dispatch, (text) => {
            const current = this.#live();
            if (typeof current.services.factoryTell !== 'function') throw new Error('원격 Tell의 실제 현재 cf730 미연결');
            current.services.factoryTell(decodeOriginalText(Uint8Array.from(text, (letter) => letter.charCodeAt(0))));
        }, (expression, file, line) => storage.assertDisplay(expression, file, line));
        dispatch.register(0x5b, (_sender, _connection, packet) => this.#live().buildingModules.receiveGeyserMined(packet));
        // 운반 명령 네 종류는 현재 Session의 해당 본체와 원본 패킷 길이를 사용한다. (2026-11-18 WB04-E737)
        for (const [type, length] of [[0x40, 18], [0x4f, 9], [0x50, 9], [0x60, 10]]) {
            dispatch.observe(type, null); dispatch.version(type, 1, 0); dispatch.requireLength(type, length);
        }
        dispatch.register(0x40, (sender, _connection, packet) => this.#live().carrierMove.receive(sender, packet));
        dispatch.register(0x4f, (sender, _connection, packet) => this.#live().carrierPickup.receive(sender, 0, packet));
        dispatch.register(0x50, (sender, _connection, packet) => this.#live().receiveCarrierDrop(sender, packet));
        dispatch.register(0x60, (sender, _connection, packet) => this.#live().carrierSteal.receive(sender, 0, packet));
        // AcidZap과 주문 수신은 같은 Session의 현재 주문 상태와 Host를 사용한다. (2026-11-18 WB04-E738)
        for (const [type, length] of [[0x61, 11], [0x5c, 6], [0x5f, 0xffff], [0x7d, 0xffff]]) {
            dispatch.observe(type, null); dispatch.version(type, 1, 0); dispatch.requireLength(type, length);
        }
        dispatch.register(0x61, (sender, _connection, packet) => this.#live().acidZap.receive(sender, 0, packet));
        dispatch.register(0x5c, (_sender, _connection, packet) => this.#live().runtimeModules.receiveDevastationActivation(packet));
        dispatch.register(0x5f, (_sender, _connection, packet) => this.#live().runtimeModules.receiveDevastationEffect(packet));
        dispatch.register(0x7d, (_sender, _connection, packet) => this.#live().receiveBombTimer(packet));
        // 의식 선택·기도·Meltdown 수신은 현재 Session의 동일한 풀과 처리기를 사용한다. (2026-11-18 WB04-E739)
        for (const [type, length] of [[0x6b, 10], [0x6d, 8], [0x75, 0xffff], [0x76, 6]]) {
            dispatch.observe(type, null); dispatch.version(type, 1, 0); dispatch.requireLength(type, length);
        }
        dispatch.register(0x6b, (_sender, _connection, packet) => {
            const current = this.#live();
            receiveRitualChoice(current.core.pool, current.contentFinder, current.contained, packet, (form, flags) => current.world.destroy(form, flags));
        });
        dispatch.register(0x6d, (_sender, _connection, packet) => this.#live().receivePriestPrayer(packet));
        dispatch.register(0x75, (_sender, _connection, packet) => this.#live().meltdown.receiveEffect(packet));
        dispatch.register(0x76, (_sender, _connection, packet) => this.#live().meltdown.receiveRequest(packet));
    }
    // 현재 방향별 동맹 DWORD를 복사하지 않고 빌린다.
    alliances() { return this.#live().alliances; }
    // 현재 Player 참여와 AI 소유자 수명을 빌린다.
    participation() { return this.#live().participation; }
    // 같은 옵션 저장소의 숨김 Player 바이트를 빌린다.
    hiddenPlayers() { return this.#live().hiddenPlayers; }
    // 현재 87바이트 옵션의 유효한 위치를 조회한다.
    battleOption(offset) { const options = this.#live().battleOptions; if (!Number.isInteger(offset) || offset < 0 || offset >= options.length) throw new RangeError('Session 전투 옵션 범위'); return options[offset]; }
    // 설치된 미션의 Player에 실제 AI를 생성한다.
    createAi(owner, settings) { return createCampaignAi(this.#live(), owner, settings); }
    // 일반 AI 설정과 객체 이전을 현재 Session에서 실행한다.
    makeAi(owner) { makeCampaignAi(this.#live(), owner); }
    // 현재 생존 AI 지갑에 원본 직접 명령을 실행한다.
    crankAiMoney() { crankCampaignAiMoney(this.#live()); }
    // packed 입력의 선택된 AI 필드에 원본 직접 명령을 실행한다.
    setAiParameter(packed) { setCampaignAiParameter(this.#live(), packed); }
    // 현재 Player의 AI 준비 행동을 한 번 진행한다.
    prepareAi(owner) { return prepareCampaignAi(this.#live(), owner); }
    // 현재 Player의 초기 전략이 준비되었는지 확인한다.
    initializeAi(owner) { return initializeCampaignAi(this.#live(), owner); }
    // 현재 기술 부모의 AI 다리 재고를 보충한다.
    refreshAiBridges(owner) { return this.#aiActions().refreshBridges(owner); }
    // 호출자 scratch로 AI 다리 경로를 한 번 진행한다.
    advanceAiBridge(owner, target, scratch) { return this.#aiActions().advanceBridge(owner, target, scratch); }
    // 같은 본체의 AI 경제 정리를 진행한다.
    reconcileAiEconomy(owner) { return this.#aiActions().reconcileEconomy(owner); }
    // 선택된 생산물을 현재 월드에 배치한다.
    deployAi(owner, type, x, y, argument, direction) { return this.#aiActions().deploy(owner, type, x, y, argument, direction); }
    // 같은 본체의 수집자 확보 행동을 진행한다.
    ensureAiCollectors(owner) { return this.#aiActions().ensureCollectors(owner); }
    // 같은 본체의 시설 생산 행동을 진행한다.
    manageAiFactories(owner) { return this.#aiActions().manageFactories(owner); }
    // 현재 부족 좌표 목록에 배치 후보를 기록한다.
    recordAiDrySpot(owner, x, y, type) { return this.#aiActions().recordDrySpot(owner, x, y, type); }
    // 부족 좌표 목록 한 항목을 재검사한다.
    recheckAiDrySpots(owner) { return this.#aiActions().recheckDrySpots(owner); }
    // 같은 난수로 부족 좌표 후보를 선택한다.
    seedAiDrySpot(owner) { return this.#aiActions().seedDrySpot(owner); }
    // 실제 표면과 경계에서 가짜 부족 좌표를 계산한다.
    computeAiDryCandidate(source, target) { return this.#aiActions().computeDryCandidate(source, target); }
    // 같은 AI 본체의 가짜 부족 좌표를 갱신한다.
    extendAiFakeDrySpots(owner) { return this.#aiActions().extendFakeDrySpots(owner); }
    // 현재 이동체의 실제 경로 Form을 조회한다.
    aiMovementForm(actor) { return this.#aiActions().movementForm(actor); }
    // 현재 이동체의 실제 경로를 중단한다.
    stopAiMovement(actor) { this.#aiActions().stopMovement(actor); }
    // 현재 AI의 대상 이동 명령을 실행한다.
    moveAiToObject(owner, actor, target, sacrifice) { return this.#aiActions().moveToObject(owner, actor, target, sacrifice); }
    // 현재 AI의 좌표 이동 명령을 실행한다.
    moveAiToPoint(owner, actor, point, sacrifice) { return this.#aiActions().moveToPoint(owner, actor, point, sacrifice); }
    // 현재 AI 이동체의 신전 복귀를 실행한다.
    returnAiHome(owner, actor) { return this.#aiActions().returnHome(owner, actor); }
    // 현재 운반자의 수집 또는 귀환을 판단한다.
    collectAiOrReturnHome(owner, actor) { return this.#aiActions().collectOrHome(owner, actor); }
    // 끊어진 AI 간헐천 목록을 재분류한다.
    recheckAiGeysers(owner) { return this.#aiActions().recheckGeysers(owner); }
    // 현재 AI의 전체 운반 행동을 실행한다.
    manageAiCarriers(owner) { return this.#aiActions().manageCarriers(owner); }
    // 현재 AI 본체의 경계와 회피 좌표로 건설 탐색을 시작한다.
    startAiBuildSearch(owner, type) { return this.#aiActions().startBuildSearch(owner, type); }
    // 현재 AI 건설 탐색을 진행한다.
    advanceAiBuildSearch(owner, budgeted) { return this.#aiActions().advanceBuildSearch(owner, budgeted); }
    // 현재 AI의 다음 건설 TYPE을 조회한다.
    selectAiBuildType(owner) { return this.#aiActions().selectBuildType(owner); }
    // 현재 AI 적의 자금 부족 조건을 조회한다.
    aiEnemyLowFunds(owner) { return this.#aiActions().enemyLowFunds(owner); }
    // 현재 AI 사제의 전체 행동을 실행한다.
    manageAiPriest(owner, priest) { return this.#aiActions().managePriest(owner, priest); }
    // 현재 AI 표적 점수를 조회한다.
    scoreAiTarget(owner, target) { return this.#aiActions().scoreTarget(owner, target); }
    // 실제 총기 표적 점수와 대상 출력을 조회한다.
    scoreAiGun(owner, type, x, y, direction, result) { return this.#aiActions().scoreGun(owner, type, x, y, direction, result); }
    // 실제 장애물 점수와 대상 출력을 조회한다.
    scoreAiBlockers(owner, type, x, y, direction, result) { return this.#aiActions().scoreBlockers(owner, type, x, y, direction, result); }
    // 현재 배치 점수와 대상 출력을 조회한다.
    scoreAiDrop(owner, type, x, y, direction, target) { return this.#aiActions().scoreDrop(owner, type, x, y, direction, target); }
    // 최고 배치 후보를 호출자의 상자에 반영한다.
    considerAiDrop(owner, type, x, y, direction, bestPoint, bestScore, target) { return this.#aiActions().considerDrop(owner, type, x, y, direction, bestPoint, bestScore, target); }
    // 현재 배치 후보 스캔을 진행한다.
    advanceAiDropScan(owner, type, direction) { return this.#aiActions().advanceDropScan(owner, type, direction); }
    // 선택한 AI 배치 후보를 제거한다.
    removeAiDropChoice(owner, index) { return this.#aiActions().removeDropChoice(owner, index); }
    // 현재 AI 배치 TYPE을 선택한다.
    selectAiDropType(owner) { return this.#aiActions().selectDropType(owner); }
    // 현재 AI 배치 행동을 실행한다.
    manageAiDrops(owner) { return this.#aiActions().manageDrops(owner); }
    // 현재 실제 객체 이름을 조회한다.
    objectName(parent) { return this.#aiActions().objectName(parent); }
    // 설치된 이름 장치에서 기본 이름을 조회한다.
    objectBaseName(parent) { return this.#aiActions().objectBaseName(parent); }
    // 현재 Player의 AI 본체를 명시 해제한다.
    removeAi(owner) { this.#aiActions().remove(owner); }
    // 현재 Session의 AI 메모리 소유자를 빌린다.
    aiMemory() { return this.#aiActions().memory(); }
    // 외곽 callback이 같은 현재 기술 부모/안내 본문을 선택한다.
    learnNewTechnology(type) { learnCampaignNewTechnology(this.#live(), type); }
    // 전체 저장의 설치 관문도 기존 실제 owner가 유지한다.
    snapshotFort(slot) { this.#live().snapshotFort(slot); }
    // Battle은 Fort 전용 자식을 덧붙이지 않는 기존 본문에 연결한다.
    snapshotBattle(slot) { this.#live().snapshotBattle(slot); }
    // 현재 요청 상태를 먼저 준비하는 실제 로컬 간헐천이다.
    generateLocalGeysers() { this.#live().generateLocalGeysers(); }
    // 현재 장기 옵션/공간을 빌리는 실제 세계 간헐천이다.
    generateWorldGeysers() { this.#live().generateWorldGeysers(); }
    // Impl callback 슬롯과 실제 typed 배치 본문을 구별한다.
    placeBattleNuggets(rectangle, quantity) { this.#live().placeBattleNuggetsBody(rectangle, quantity); }

    // 4822: 같은 World/건설 장치의 외곽 pin을 먼저 연결하고 장기 칸에도 보관한다.
    bindRuntimePin(pin = null) {
        const o = this.#live(); o.world.bindRuntimePin(pin); o.constructionRuntimePin = pin;
        if (o.constructionRuntime) o.constructionRuntime.pin = o.constructionRuntimePin;
    }
    // 4158..4160: 아직 Login이 없어도 같은 services가 보관한 실제 전송 대여를 반납한다.
    detachConnectionTransport() {
        const o = this.#live(), transport = o.services.localLogin?.transport;
        if (transport) {
            if (typeof transport.detachSession !== 'function') throw new Error('외곽 Session의 실제 전송 반납 장치 미연결');
            transport.detachSession();
        }
    }
    // State 증빙/Withdraw의 앞단만 옮긴다. 일반 Impl 반납은 기존 owner가 끝까지 수행한다.
    dispose() {
        if (this.#disposed) return;
        const o = this.#live();
        if (o.gameplayGlobals.runtimeCallActive(this)) throw new Error('실제 Runtime 배송 종료 전 외곽 Session 반납');
        try { this.bindRuntimePin(null); this.detachConnectionTransport(); }
        finally { try { this.#login?.dispose(); } finally { this.#disposed = true; o.dispose(); } }
    }
}
