// Session Login 생성자의 옵션/내부 전송 앞단과 InstallZone을 실제 Session 저장소에 연결한다.
// 전체 패킷·상태 조립과 Begin/Pump는 후속 단계이며 이 객체 생성만으로 접속하지 않는다.
import { bindInternalCampaignTransport } from './campaign-transport-services.js';
import { createCampaignLoginServices, requireLoginService } from './campaign-login-services.js';
import { installLocalMissionZone, LocalMissionTransport, makeLocalMissionLogin } from './local-mission-transport.js';
import { RawMissionBattleOptions, resetMissionBattleOptions } from './raw-mission-battle-options.js';
import { RawMissionClientPackets, registerMissionClientPackets, sanitizeMissionPlayerName } from './raw-mission-client-packets.js';
import { RawMissionLoginPackets } from './raw-mission-login-packets.js';
import { RawMissionServerPackets, registerMissionMetadata } from './raw-mission-server-packets.js';
import { RawMissionSettingsPacket, registerMissionSettingsPacket } from './raw-mission-settings-packet.js';
import { RawClockSynchronization } from './raw-clock-synchronization.js';
import { LocalMissionProgress, RawLocalMissionState } from './raw-local-mission-state.js';
import { ClientHelpWord } from './client-help-word.js';
import { RawFortImage } from './raw-fort-image.js';
import { MissionLoginPlayerCounts, RawMissionLoginTimer } from './raw-mission-login-timer.js';
import { RawMissionLogin, registerMissionLogin } from './raw-mission-login.js';
import { setPlayerSpAmount } from './raw-fort-money-load.js';
import { registerDeveloperCommandPacket } from './raw-developer-command-packet.js';
import { registerStateMetaBattleDescription } from './raw-state-meta-battle-description.js';
import { registerConnectionActivity } from './raw-connection-activity.js';

// 현재 Session을 빌려 원본 Login의 전송 앞단과 Zone 설치만 소유한다.
export class RawCampaignSessionLoginFront {
    // 실제 부모·장치와 내부 전송 수명, 반납 관문이다.
    #owner; #effects; #internal = null; #disposed = false; #registered = false; #wall = 0; #localTitle = 0; #timeout = 0; #localRequestPhase = 0; #beginSeed = 0; #battleSeed = 0;
    // 원문의 독립 초기 지역 입력과 현재 수신 종류 기록이다.
    pendingStamp = 0; overrideFlags = 0; wireTypes = [];
    // 현재 연결표와 패킷 분배기 및 같은 옵션/패킷/시계 처리기를 소유자와 함께 공개한다.
    transport; options; metadata; forts; settings; clockSynchronization; state; timer;
    // 실제 참여 소유자 준비 뒤 생성하는 로그인 서버와 후속 클라이언트다.
    server = null; client = null;
    // 원문 독립 fixture의 현재 UI 및 입력 모드 값이다.
    uiMode = 0; inputMode = 0;

    // Player 초기화 뒤 옵션 기본값과 현재 전송을 순서대로 준비한다.
    constructor(owner, effects = {}) {
        if (!owner?.playersReady) throw new TypeError('Login의 실제 Player 초기화 누락');
        this.#owner = owner; this.#effects = createCampaignLoginServices(effects);
        if (!owner.services.missionOptions) resetMissionBattleOptions(owner.battleOptions);
        if (this.#effects.transport) {
            const transport = { ...this.#effects.transport };
            if (!transport.lifetime || !transport.registry || !transport.dispatch || typeof transport.detachSession !== 'function' || typeof transport.callbacksActive !== 'function') throw new Error('외부 Login 전송의 실제 수명/등록표/반납 관문 누락');
            if (owner.services.connectionRegistry && owner.services.connectionRegistry !== transport.registry) throw new Error('Login과 Client State의 현재 연결표 불일치');
            owner.services.connectionRegistry = transport.registry; this.transport = transport;
        } else {
            this.#internal = new LocalMissionTransport(owner.players, () => owner.core.processState.server, this.#device(), owner.services.connectionRegistry ?? null);
            this.transport = bindInternalCampaignTransport(this.#internal);
        }
        try {
            const aux = owner.gameplayGlobals.gameAux(), ranges = owner.gameplayGlobals.manaRanges(), energy = owner.energyState;
            this.options = new RawMissionBattleOptions({
                options: owner.battleOptions, types: owner.core.types, typeUse: aux.typeUse(),
                rangeTable: new Uint32Array(ranges.buffer, ranges.byteOffset, ranges.length), selectedType: () => owner.geyserType,
                range: owner.gameplayGlobals.manaRadius(),
                rangeSquared: { get bits() { return energy.radiusSquaredBits; }, set bits(value) { energy.radiusSquaredBits = value; } },
                value: { get value() { return aux.refundPercent; }, set value(value) { aux.refundPercent = value; } },
            });
            const local = owner.localStateGlobals;
            this.metadata = new RawMissionServerPackets(owner.players, {
                get server() { return owner.core.processState.server; }, get metamap() { return owner.core.ownerMode.challenge; },
                get localOwner() { return owner.playerState.battleOwner; }, get centralCreated() { return local.centralCreated().value; },
                get archipelago() { return aux.archipelagoPercent; }, zoneAddress: local.zone(),
            }, {
                send: (target, packet) => this.#send(target, packet, true),
                peerAddress: (ownerId) => this.#transport('peerAddress')(ownerId | 0), log: this.#effects.log,
                assertion: (expression) => this.#assertion(expression), creatingIslands: this.#effects.creatingIslands,
                setArchipelago: (value) => { aux.archipelagoPercent = value; }, setCentralCreated: (value) => { local.centralCreated().value = value; },
                layout: () => owner.battleOptions[5], predictableCount: () => owner.core.pool.predictableCursor,
                randomCentral: () => { throw new Error('스커미시 중앙섬 생성은 후속 단계'); },
                surroundedCentral: () => { throw new Error('스커미시 특수 중앙섬 생성은 후속 단계'); },
            });
            this.forts = new RawMissionLoginPackets(owner.players, {
                current: (slot) => {
                    this.#live(); if (slot >= 9) return null;
                    const headers = owner.headers, pointer = new DataView(headers.buffer, headers.byteOffset, headers.byteLength).getUint32(slot * 20, true);
                    return pointer ? owner.templateStorage.template(slot) : null;
                }, initialLength: () => 4,
            }, { typeCount: () => owner.core.types.length }, {
                log: () => this.#need('log', 'Fort 송신')('로컬 Fort 전송\n'),
                send: (target, packet) => this.#send(target, packet, true),
            });
            const values = aux.missionSettings();
            this.settings = new RawMissionSettingsPacket({ current: (field) => values[field], setCurrent: (field, value) => { values[field] = value; }, send: (target, packet) => this.#send(target, packet, false) });
            const registry = this.transport.registry;
            this.clockSynchronization = new RawClockSynchronization(local.clockSynchronization(), local.clockSamples(), {
                gameSeconds: this.#effects.gameSeconds, wallSeconds: this.#effects.wallSeconds,
                send: (token, packet) => registry.sendDirect(token, packet),
                reply: (connection, packet, address) => {
                    if (address && (address.length !== connection.address.length || address.some((byte, index) => byte !== connection.address[index]))) throw new Error('현재 내부 연결과 다른 시간 응답 주소는 웹 범위 밖');
                    registry.sendDirect(registry.tokenFor(connection), packet);
                },
            });
            this.state = new RawLocalMissionState(local.state(), this.#stateGlobals(), this.#stateHooks());
            this.timer = new RawMissionLoginTimer(this.#timerState(), new MissionLoginPlayerCounts(owner.players), {
                savedGame: () => local.savedGame().value, server: () => owner.core.processState.server,
                mission: () => owner.objectState.mission, missionPost: () => owner.globals.missionPost,
                initialGeysers: () => owner.state.assetState.readBoss(), checkingAssertions: () => Boolean(owner.state.gunState.checking),
            }, this.#timerHooks());
        } catch (error) {
            if (this.#effects.transport) this.transport.detachSession(); else this.#internal.dispose();
            this.#disposed = true; throw error;
        }
    }

    // 선택 위치에 도달한 실제 장치만 요구한다.
    #need(name, where) { return requireLoginService(this.#effects, name, where); }
    // 반납 뒤 낡은 전송과 현재 Session을 다시 쓰지 않는다.
    #live() { if (this.#disposed) throw new Error('반납한 Session Login 앞단'); return this.#owner; }
    // 호출 시 선택된 현재 전송 장치만 요구한다.
    #transport(name) { this.#live(); const device = this.transport[name]; if (typeof device !== 'function') throw new Error(`Login 전송 ${name} 미연결`); return device; }
    // 현재 실제 Session 진단을 호출한다.
    #assertion(expression) { const o = this.#live(); if (typeof o.services.ai?.assertion !== 'function') throw new Error('Login 진단 장치 누락'); o.services.ai.assertion(expression); }
    // 같은 장기 State/권한/Player 및 현재 게임·벽시각의 실제 속성을 빌린다.
    #stateGlobals() {
        const o = this.#live(), self = this, local = o.localStateGlobals;
        const title = o.services.htmlWindowGlobals ? o.services.htmlWindowGlobals.text.background : null;
        return {
            get server() { return o.core.processState.server; }, set server(value) { o.core.processState.server = value; },
            get boss() { return o.state.assetState.readBoss(); }, set boss(value) { o.state.assetState.writeBoss(value); },
            get inBattle() { return o.state.assetState.inBattle; }, set inBattle(value) { o.state.assetState.inBattle = Boolean(value); },
            get missionPost() { return o.globals.missionPost; }, get missionWorld() { return o.objectState.mission; },
            get savedGame() { return local.savedGame().value; }, set savedGame(value) { local.savedGame().value = value; },
            get simulconnect() { return local.simulconnect().value; }, get metamap() { return o.core.ownerMode.challenge; }, player: o.playerState,
            get zonePending() { return local.zonePending().value; }, set zonePending(value) { local.zonePending().value = value; },
            get gameTicks() { return local.gameTicks().value; }, set gameTicks(value) { local.gameTicks().value = value; },
            get sourceType() { return local.sourceType().value; }, set sourceType(value) { local.sourceType().value = value; },
            get cdPresent() { return local.cdPresent().value; }, set cdPresent(value) { local.cdPresent().value = value; },
            get metaRefresh() { return local.metaRefresh().value; }, set metaRefresh(value) { local.metaRefresh().value = value; },
            get metaRebuild() { return local.metaRebuild().value; }, set metaRebuild(value) { local.metaRebuild().value = value; },
            get titleInitial() { return title ? title[0] : self.#localTitle; }, set titleInitial(value) { if (title) title[0] = value; else self.#localTitle = value & 0xff; },
            get battleByte() { return local.battleByte().value; },
            get connectEnd() { return local.connectEnd().value; }, set connectEnd(value) { local.connectEnd().value = value; },
            get activityEnd() { return local.activityEnd().value; }, set activityEnd(value) { local.activityEnd().value = value; },
            get gameSeconds() { return o.core.processState.now; }, get wallCached() { return self.#wall; },
        };
    }
    // 현재 UI 장치가 선택된 위치에서만 실제 전환을 요청한다.
    #setUiMode(mode) {
        const o = this.#live(); this.uiMode = mode;
        if (o.services.adoptInitializedPlayers || this.#effects.uiMode || this.#effects.battlePalette || this.#effects.startingProgress) this.#need('uiMode', 'Login UI 모드')(mode);
        else this.#need('refresh', 'Login UI 갱신')();
    }
    // 실제 선택기는 경로와 지연 읽기를 함께 반환하며 독립 입력만 기존 파일 이름을 사용한다.
    #selectFortFile() {
        const o = this.#live();
        if (o.services.missionFortFileSelect) return o.services.missionFortFileSelect(o.missionFortName);
        const name = o.services.missionFortFileName || o.missionFortName;
        return { fileName: name, read: () => o.services.missionFortRead ? o.services.missionFortRead(name) : o.assets.files.read(`d/${name}.fort`) };
    }
    // 선택된 경로의 읽기와 현재 TYPE/owner/제목을 같은 Template 부모에 빌린다.
    #fortFileHooks(selected) {
        const o = this.#live(), hooks = { ...o.services.missionFortLoad };
        hooks.read = () => { if (typeof selected?.read !== 'function') throw new Error('선택된 미션 파일 읽기 미연결'); return selected.read(); };
        hooks.sameName = (first, second) => ClientHelpWord.equalsIgnoreCase(String(first).split('\0', 1)[0], String(second).split('\0', 1)[0]);
        if (!hooks.wrongName) hooks.wrongName = () => this.#need('log', 'Template 이름 진단')('Template.cpp 진단 386: 기존 manager 파일 이름 불일치\n');
        hooks.typeCount = () => o.core.types.length; hooks.currentSlot = () => o.playerState.templateOwner;
        hooks.refreshTitle = o.services.install.refreshWindowTitle; return hooks;
    }
    // 기존 manager의 이름 비교와 읽기 순서를 실제 Template 파일 부모에 위임한다.
    #reloadFort(owner) {
        const o = this.#live(); if ((owner >>> 0) >= 9) throw new RangeError('현재 미션 Template의 실제9슬롯 범위');
        const selected = this.#selectFortFile(); if (!selected?.fileName) throw new Error('미션 파일 선택기의 실제 파일 이름 누락');
        o.templateStorage.loadFile(owner, {}, selected.fileName, this.#fortFileHooks(selected));
    }
    // 실제 선택 파일은 지역 Template를 읽어 다섯 섹션만 송신하며 독립 입력은 현재 manager 이미지를 사용한다.
    #sendLogin() {
        const o = this.#live(); let image;
        if (o.services.missionFortFileSelect) {
            const selected = this.#selectFortFile();
            image = o.templateStorage.readLoginImage(selected.fileName, this.#fortFileHooks(selected), o.localStateGlobals.battleTerritory().value);
        } else image = new RawFortImage('patch1078', o.templateStorage.template(o.playerState.templateOwner), { typeCount: () => o.core.types.length }).save();
        const packet = makeLocalMissionLogin(o.globals.mySubscriber, o.handle, image.bytes()); this.#send(9, packet.bytes(), false);
    }
    // 실제 Client의 진행 창은 별도 수명이며 독립 부분 장치에서만 갱신으로 닫는다.
    #closeProgress() {
        const o = this.#live();
        if (o.services.adoptInitializedPlayers || this.#effects.uiMode || this.#effects.battlePalette || this.#effects.closeProgress || this.#effects.startingProgress) this.#need('closeProgress', '로그인 진행 창 닫기')();
        else this.#need('refresh', '로그인 진행 갱신')();
    }
    // 로그인 타이머가 같은 장기 기한·Begin·스크립트·Client 시계를 직접 빌린다.
    #timerState() {
        const o = this.#live(), self = this, local = o.localStateGlobals, request = this.#effects.requestPhase;
        return {
            get end() { return local.loginEnd().value; }, set end(value) { local.loginEnd().value = value; },
            get timeout() { return self.#timeout; }, set timeout(value) { self.#timeout = value | 0; },
            get begin() { return local.begin().value; }, set begin(value) { local.begin().value = value; },
            get scriptBreak() { return local.scriptBreak().value; }, set scriptBreak(value) { local.scriptBreak().value = value; },
            get requestPhase() { return request ? request.value : self.#localRequestPhase; },
            set requestPhase(value) { if (request) request.value = value >>> 0; else self.#localRequestPhase = value >>> 0; },
            get clock() { return o.gameplayGlobals.battleClockOrigin().value; }, set clock(value) { o.gameplayGlobals.battleClockOrigin().value = value; },
        };
    }
    // 미션 또는 현재 Player 미확정이면 원문 재접속 저장은 파일 접근 전에 자연 반환한다.
    #saveReconnect() {
        const o = this.#live(); if (o.globals.missionPost || !o.playerState.playerKnown) return;
        throw new Error('비미션 재접속 저장은 웹 후속 단계');
    }
    // 현재 Player/연결·사제/시계와 선택된 UI 효과를 실제 타이머에 연결한다.
    #timerHooks() {
        const o = this.#live(), effects = this.#effects, local = o.localStateGlobals;
        return {
            configInt: (key) => this.#need('configInt', '로그인 타이머 설정')(key, 30), wallSeconds: effects.wallSeconds, clock: effects.clock,
            broadcast: (packet) => { if (effects.observePacket) this.#need('log', '타이머 방송')(`Timer broadcast local${o.state.assetState.localOwner}\n`); o.tables.broadcast.send(packet); },
            priestRefresh: (owner) => o.priestParticipation.refresh(owner), connection: (id) => id === 9 && this.#transport('exists')(9),
            remoteStatus: (id) => o.state.connectionActivity.receive(id), saveReconnect: () => this.#saveReconnect(),
            closeProgress: () => { if (effects.closeTimerProgress) effects.closeTimerProgress(); else this.#closeProgress(); },
            log: (_kind, text) => this.#need('log', '로그인 타이머 기록')(text), abortBattle: effects.abortBattle,
            progress: () => { const body = this.timer.progressText(this.#need('frameWallSeconds', '로그인 진행 시각')(), this.#need('translate', '로그인 진행 번역')); local.loginProgressSerial().value = this.#need('showTimerProgress', '로그인 진행 창')(body); },
            initializeGeneral: () => { throw new Error('스커미시 일반 전투 초기화는 후속 단계'); },
            createInitialGeysers: o.createWorldGeysers ?? undefined, resetCamera: o.services.install.resetCamera,
            serverAssertion: () => this.#assertion('server'),
        };
    }
    // 서버 로그인은 같은 Player·Core·권한·시계와 장기 타입/디버그 전역을 읽고 쓴다.
    #serverGlobals() {
        const o = this.#live(), self = this, local = o.localStateGlobals, aux = o.gameplayGlobals.gameAux(), core = o.coreDataGlobals;
        return {
            get server() { return o.core.processState.server; }, get boss() { return o.state.assetState.readBoss(); },
            get simulconnect() { return local.simulconnect().value; }, get missionWorld() { return o.objectState.mission; },
            get missionPost() { return o.globals.missionPost; }, get metamap() { return o.core.ownerMode.challenge; }, player: o.playerState,
            get mySubscriber() { return o.globals.mySubscriber; },
            get fortCount() { return o.playerRecords.fortCount().value; }, set fortCount(value) { o.playerRecords.fortCount().value = value; },
            get lastLoginPlayer() { return o.playerRecords.lastLoginPlayer().value; }, set lastLoginPlayer(value) { o.playerRecords.lastLoginPlayer().value = value; },
            get predictableCount() { return o.core.pool.predictableCursor; },
            get begin() { return local.begin().value; }, set begin(value) { local.begin().value = value; },
            get beginSeed() { return self.#beginSeed; }, set beginSeed(value) { self.#beginSeed = value >>> 0; },
            get battleSeed() { return self.#battleSeed; }, set battleSeed(value) { self.#battleSeed = value >>> 0; },
            sharedBattleSeed: { get value() { return aux.resourcePoolBits; }, set value(value) { aux.storeResourcePoolBits(value >>> 0); } },
            get debugBurst() { return local.debugBurst().value; }, get debugPacketHeader() { return aux.packet0cHeader(); },
            get debugPacketInitialLength() { return aux.packet0cInitialLength; },
            get clockStamp() { return o.gameplayGlobals.battleClockStamp().value; }, set clockStamp(value) { o.gameplayGlobals.battleClockStamp().value = value; },
            get baseClock() { return o.gameplayGlobals.battleClockOrigin().value; }, set baseClock(value) { o.gameplayGlobals.battleClockOrigin().value = value; },
            get captureEnd() { return local.captureEnd().value; }, set captureEnd(value) { local.captureEnd().value = value; },
            get currentCoreData() { return core.current; }, set currentCoreData(value) { core.current = value; },
            cachedCoreData: core.cached(), zoneAddress: local.zone(), battleOptions: o.battleOptions,
            get savedGame() { return local.savedGame().value; }, get inBattle() { return o.state.assetState.inBattle; },
        };
    }
    // 실제 장기 Core 복원과 선택된 Chat 재초기화를 같은 Player에 적용한다.
    #restoreCoreData(owner) {
        const o = this.#live(), local = o.localStateGlobals;
        o.coreDataGlobals.restore(o.playerState, () => Number(o.core.ownerMode.challenge), {
            missionPost: () => Number(o.globals.missionPost), savedGame: () => Number(local.savedGame().value), checking: () => Number(o.state.gunState.checking),
        }, owner, { assertion: (expression) => this.#assertion(expression), initializeChat: this.#effects.initializeChat });
    }
    // 실제 참여 부모가 준비된 다음 서버 패킷 몸체를 한 번 생성한다. 수신기 등록과 월드 설치는 후속이다.
    bindServer(install = null) {
        const o = this.#live(); if (this.server || !o.participation || (install !== null && typeof install !== 'function')) throw new Error('서버 Login의 실제 참여/설치 또는 중복 결선 오류');
        const effects = this.#effects;
        this.server = new RawMissionLogin(o.players, o.participation, this.#serverGlobals(), {
            disconnect: (id) => this.#transport('close')(id),
            pendingReply: (connection, packet) => {
                const id = connection.sender | 0; if (this.#transport('connection')(id) !== connection) throw new Error('거부 응답의 현재 연결 정체성 불일치');
                this.#send(id, packet, false);
            },
            peerAddress: (owner) => this.#transport('peerAddress')(owner | 0),
            send: (target, packet) => this.#send(target, packet, true), broadcast: (packet) => o.tables.broadcast.send(packet),
            flush: () => this.#transport('flush')(), wallSeconds: effects.wallSeconds, clock: effects.clock, spBits: (owner) => o.sp.get(owner),
            applyOptions: (packet) => { if (!this.client) throw new Error('현재 Login 클라이언트 옵션 수신기 미연결'); this.client.options(0, packet); },
            clearTypes: () => this.options.clearTypes(), sendPlayer: (owner, target) => this.metadata.sendPlayer(owner, target),
            sendFortMetadata: (target) => this.metadata.sendMetadata(target), loadCurrentFort: (owner) => this.#reloadFort(owner),
            sendFort: (owner, target, count) => this.forts.sendFort(owner, target, count),
            loadWorld: () => { if (!install) throw new Error('현재 Session Load 설치 미연결'); install(); },
            sendSettings: (target) => this.settings.send(target), restoreCoreData: (owner) => this.#restoreCoreData(owner),
            loadFortImage: () => { throw new Error('원격 Fort는 웹 일반 멀티플레이 범위 밖'); },
            synchronizePeer: () => { throw new Error('원격 월드 동기화는 후속 단계'); },
            makeBattleIsland: () => { throw new Error('스커미시 전투섬 생성은 후속 단계'); },
            assertion: (expression) => this.#assertion(expression),
            setSp: (owner, amount) => setPlayerSpAmount(owner | 0, amount | 0, o.sp, { clientActive: () => o.playerState.playerKnown, localOwner: () => o.playerState.battleOwner, editor: () => o.globals.editor }, () => this.#need('refresh', '현재 SP 갱신')()),
            cleanupReconnectClients: () => { throw new Error('비미션 재접속 정리는 후속 단계'); }, checkingAssertions: () => o.state.gunState.checking, log: effects.log,
        });
        return this.server;
    }
    // 클라이언트 수신기와 서버가 같은 Player·Core·seed·상태 블록을 직접 공유한다.
    #clientGlobals() {
        const o = this.#live(), self = this, local = o.localStateGlobals, blocks = local.state(), aux = o.gameplayGlobals.gameAux(), core = o.coreDataGlobals;
        return {
            get server() { return o.core.processState.server; }, get boss() { return o.state.assetState.readBoss(); },
            get missionPost() { return o.globals.missionPost; }, get savedGame() { return local.savedGame().value; },
            get metamap() { return o.core.ownerMode.challenge; }, get inBattle() { return o.state.assetState.inBattle; }, get editor() { return o.globals.editor; },
            get outpost() { if (o.services.challengeGlobals) throw new Error('메타 CS 상태는 웹 범위 밖'); return 0; }, player: o.playerState,
            get beginSeed() { return self.#beginSeed; }, set beginSeed(value) { self.#beginSeed = value >>> 0; },
            get battleSeed() { return self.#battleSeed; }, set battleSeed(value) { self.#battleSeed = value >>> 0; },
            sharedBattleSeed: { get value() { return aux.resourcePoolBits; }, set value(value) { aux.storeResourcePoolBits(value >>> 0); } },
            get revision() { return o.revision; }, set revision(value) { o.revision = value; },
            get optionValue() { return aux.refundPercent; }, set optionValue(value) { aux.refundPercent = value; },
            get begin() { return local.begin().value; }, set begin(value) { local.begin().value = value; },
            get clockStamp() { return o.gameplayGlobals.battleClockStamp().value; }, set clockStamp(value) { o.gameplayGlobals.battleClockStamp().value = value; },
            get baseClock() { return o.gameplayGlobals.battleClockOrigin().value; }, set baseClock(value) { o.gameplayGlobals.battleClockOrigin().value = value; },
            get phase() { return blocks.phase[0]; }, get currentState() { return blocks.current[0]; }, set currentState(value) { blocks.current[0] = value; },
            get currentAux() { return blocks.current[14]; }, set currentAux(value) { blocks.current[14] = value; },
            get pendingState() { return blocks.next[0]; }, set pendingState(value) { blocks.next[0] = value; },
            get sourceTicks() { return local.sourceType().value; }, set sourceTicks(value) { local.sourceType().value = value; },
            get coreDataActive() { return core.active; }, set coreDataActive(value) { core.active = value; },
            get captureEnd() { return local.captureEnd().value; }, set captureEnd(value) { local.captureEnd().value = value; },
            get loginEnd() { return local.loginEnd().value; },
            get currentCoreData() { return core.current; }, set currentCoreData(value) { core.current = value; },
            cachedCoreData: core.cached(), zoneAddress: local.zone(), battleOptions: o.battleOptions, currentHandle: o.handle, hidden: o.hiddenPlayers,
        };
    }
    // 서버 준비 뒤 클라이언트의 실제 패킷 몸체를 같은 현재 owner에 연결한다. 수신기 등록은 후속이다.
    bindClient() {
        const o = this.#live(), effects = this.#effects;
        if (!this.server || this.client) throw new Error('클라이언트 Login의 실제 서버 전제 또는 중복 결선 오류');
        this.client = new RawMissionClientPackets(o.players, o.participation, o.sp, this.#clientGlobals(), {
            clock: effects.clock, wallSeconds: effects.wallSeconds, runState: () => this.state.run(), resetCamera: o.services.install.resetCamera, refreshSp: effects.refresh,
            observerAssertion: (expression, file, line) => o.world.storage.assertDisplay(expression, file, line),
            outpostOptions: effects.metaBattleOptions, reconnectUi: effects.reconnectUi,
            localFort: () => { throw new Error('일반 전투 Fort는 웹 후속 스커미시 단계'); },
            openFortImage: () => { throw new Error('원격 Fort는 웹 일반 멀티플레이 범위 밖'); },
            readFortImage: () => { throw new Error('원격 Fort는 웹 일반 멀티플레이 범위 밖'); },
            buildFort: () => { throw new Error('스커미시 전투섬 생성은 후속 단계'); },
            updateRange: () => this.options.updateRange(), requiredEnergy: () => this.options.typeCost(),
            setEnergyType: (value) => this.options.setSelectedTypeCost(value), readOptionValue: () => this.options.currentValue(),
            addChat: o.services.ai.participation.addChat, playerUiBusy: effects.playerUiBusy, playerNotice: effects.playerNotice,
            send: (target, packet) => this.#send(target, packet, false), sendReady: (target) => this.server.sendReady(target),
            restoreCoreData: (owner) => this.#restoreCoreData(owner),
        });
        return this.client;
    }
    // 서버·클라이언트 준비 뒤 원본 순서의 로그인 수신기와 관찰기를 같은 전송에 등록한다.
    registerReceivers() {
        const o = this.#live(); if (!this.server || !this.client || this.#registered) throw new Error('Login 수신기의 서버/클라이언트 전제 또는 중복 등록 오류');
        this.#registered = true; const dispatch = this.transport.dispatch, storage = o.world.storage;
        try {
            registerDeveloperCommandPacket(dispatch, (text) => {
                this.#live(); if (typeof o.executeDeveloperCommand !== 'function') throw new Error('실제 개발자 명령 해석기 미연결'); o.executeDeveloperCommand(text);
            }, (expression, file, line) => storage.assertDisplay(expression, file, line));
            registerMissionLogin(dispatch, this.server); registerMissionClientPackets(dispatch, this.client);
            registerMissionMetadata(dispatch, this.metadata); registerMissionSettingsPacket(dispatch, this.settings);
            registerStateMetaBattleDescription(dispatch, () => this.client.state.outpost, this.#effects.metaBattleDescription,
                (expression) => storage.assertDisplay(expression, '\\Ns\\O\\_p_packets.cpp', 0xae3));
            registerConnectionActivity(dispatch, o.state.connectionActivity);
        } catch (error) { this.dispose(); throw error; }
    }
    // 원문 상태8은 현재 블록과 내부 주소를 확인하고 실제 전송에 접속을 맡긴다.
    #connect(address, port) {
        const o = this.#live(), current = o.localStateGlobals.state().current;
        if (!(address instanceof Uint8Array) || address.length !== 20 || current[0] !== 8) throw new Error('현재 State8/주소20BYTE 접속 계약 오류');
        if (o.services.generalBattleStart?.textAddress || current[33]) throw new Error('일반 또는 저장 재접속 주소 처리는 후속 단계');
        this.#transport('connect')(address, port);
    }
    // 원문 StateHooks 중 실제 장기 owner와 연결된 앞단이다. 시작 공통 부모는 후속이다.
    #stateHooks() {
        const o = this.#live(), local = o.localStateGlobals;
        return {
            logState: (phase) => this.#need('log', 'State 로그')(`로컬 미션 상태 ${phase >>> 0}\n`), wallSeconds: this.#effects.wallSeconds,
            initializeRenderer: this.#effects.initializeRenderer, progress: this.#effects.progress,
            connectServer: (address, port) => this.#connect(address, port),
            connectionStatus: (id) => this.#transport('status')(id), connectionOpen: (id) => this.#transport('isOpen')(id),
            closeConnection: (id) => this.#transport('close')(id), renameConnection: (pending, owner) => this.#transport('assign')(pending, owner),
            logConnection: (line, value) => this.#need('log', '연결 진단')(`연결 진단 ${line}:${value}\n`),
            serverConnection: () => this.transport.registry.cleanup().transceivers[9],
            attachCommander: this.#effects.attachCommander ?? ((token) => this.clockSynchronization.attach(this.transport.dispatch, token)),
            installMapHandlers: () => this.clockSynchronization.initializeServer(this.transport.dispatch),
            sendLogin: () => this.#sendLogin(), closeProgress: () => this.#closeProgress(),
            connectedProgress: () => {
                if (this.#effects.progress) this.#effects.progress(o.globals.missionPost ? LocalMissionProgress.MissionTransferring : local.savedGame().value ? LocalMissionProgress.ReconnectTransferring : LocalMissionProgress.Transferring, 0);
                else this.#need('refresh', '로그인 전송 안내')();
            },
            fortAvailable: (owner) => (owner >>> 0) < 9 && new DataView(o.headers.buffer, o.headers.byteOffset, o.headers.byteLength).getUint32((owner >>> 0) * 20, true) !== 0,
            loadCurrentFort: () => this.#reloadFort(o.playerState.templateOwner),
            fortCore: (owner) => o.templateState.get(owner), touchFort: (owner) => o.templateState.changed(owner),
            localAddress: () => this.#transport('address')(9), uiMode: (mode) => this.#setUiMode(mode), loginTimer: (start) => this.timer.run(start),
            refreshFort: o.services.install.refreshWindowTitle,
            inputMode: (mode) => { this.inputMode = Number(mode); this.#need('refresh', '입력 모드')(); },
        };
    }
    // 현재 6eh 관찰 로그 뒤 원문의 mutable 또는 읽기 전용 송신을 선택한다.
    #send(target, packet, mutable) {
        this.#live();
        if (this.#effects.observePacket && packet[1] === 0x6e) {
            if (packet.length < 8) throw new RangeError('Ready 송신 마스크 물리 부족');
            const mask = new DataView(packet.buffer, packet.byteOffset, packet.byteLength).getUint32(4, true);
            this.#need('log', 'Ready 송신')(`Ready 송신 target${target | 0} mask${mask}\n`);
        }
        this.#transport(mutable ? 'sendMutable' : 'send')(target | 0, packet);
    }
    // 내부 패킷 분배기가 실제 시계·수신 기록·연결 반납을 사용한다.
    #device() {
        return {
            now: () => this.#need('wallSeconds', '패킷 분배')() >>> 0,
            record: (connection, packet) => {
                const o = this.#live(); if (packet.length < 4) throw new RangeError('현재 수신 기록의 실제 헤더 부족');
                this.wireTypes.push(packet[1]);
                if (this.#effects.observePacket) this.#effects.observePacket(connection.sender, o.state.assetState.readBoss(), o.state.assetState.localOwner, packet);
            },
            terminate: (connection) => { connection.flags |= 0x40; this.#need('log', '연결 종료')('로컬 미션 연결 종료\n'); },
            close: () => { this.#need('log', '연결 닫기')('로컬 연결 큐/소켓 종료\n'); },
            release: () => { this.#need('log', '연결 반납')('로컬 연결 반납\n'); },
        };
    }

    // 실제 공유 Player·Core·권한·Zone에 원본 가변 Zone을 설치한다.
    installZone(zone) {
        const o = this.#live(), self = this, local = o.localStateGlobals, active = o.connectionActivityGlobals, aux = o.gameplayGlobals.gameAux();
        const globals = {
            get server() { return o.core.processState.server; }, set server(value) { o.core.processState.server = value; },
            get boss() { return o.state.assetState.readBoss(); }, set boss(value) { o.state.assetState.writeBoss(value); },
            get mySubscriber() { return o.globals.mySubscriber; }, get pendingStamp() { return self.pendingStamp; },
            get activeStamp() { return active.activeStamp().value; }, set activeStamp(value) { active.activeStamp().value = value; },
            get activeZoneWord() { return active.activeZoneWord().value; }, set activeZoneWord(value) { active.activeZoneWord().value = value; },
            get zoneFlags() { return local.zoneWords()[0]; }, set zoneFlags(value) { local.zoneWords()[0] = value; },
            get earlyCaptureEnd() { return local.captureEnd().value; }, set earlyCaptureEnd(value) { local.captureEnd().value = value; },
            get adjustZoneFlags() { return aux.terrainEnabled; }, get missionWorld() { return o.objectState.mission; },
            get zoneFlagOverride() { return self.overrideFlags; }, get metamap() { return o.core.ownerMode.challenge; },
            get currentCoreData() { return o.coreDataGlobals.current; }, set currentCoreData(value) { o.coreDataGlobals.current = value; },
            get cachedCoreData() { return o.coreDataGlobals.cached(); },
        };
        const hooks = {
            loginTimeout: () => this.#need('configInt', 'Zone 로그인 제한')('loginTimeout', 30),
            seconds: () => this.#need('wallSeconds', 'Zone 벽시각')(),
            storeName: (subscriber, name, storage) => sanitizeMissionPlayerName(subscriber, storage.subarray(0, name.length + 1)), // C++ string_view의 길이와 NUL 한 칸만 빌린다.
            serve: (port) => { if (typeof this.transport.serve !== 'function') throw new Error('Zone 서버 전송 장치 누락'); this.transport.serve(port); },
            assertion: (expression) => { if (typeof o.services.ai?.assertion !== 'function') throw new Error('Zone 진단 장치 누락'); o.services.ai.assertion(expression); },
        };
        installLocalMissionZone(zone, o.players, o.playerState, globals, hooks, this.#need('configInt', 'Zone 서버 포트')('gameServerPort', 6799) >>> 0);
    }

    // 외부 등록 대여 또는 내부 두 끝을 실제 소유 방식에 따라 반납한다.
    dispose() {
        if (this.#disposed) return;
        if (this.#effects.transport) this.transport.detachSession(); else this.#internal.dispose();
        this.#disposed = true;
    }
}
