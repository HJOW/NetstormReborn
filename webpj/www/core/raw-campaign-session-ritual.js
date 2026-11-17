// 같은 Session의 의식 프레임과 예약 객체를 현재 Pool·Frame·Host에 연결한다(225553df, WB04-E645).
import { RegularProcess } from './squid-process.js';
import { RawPriestRitualActions } from './raw-priest-ritual-actions.js';
import { RawPriestRitualFrame } from './raw-priest-ritual-frame.js';
import { RawContainedLookup } from './raw-contained-lookup.js';
import { RawTempleRitual } from './raw-temple-ritual.js';
import { RawRunePostPop } from './raw-rune-post-pop.js';
import { setRawSquidHitPoints } from './raw-squid-word.js';
import { DaisRegularState, RawDaisRegular } from './raw-dais-regular.js';
import { RawPriestSacrificeRequest } from './raw-priest-sacrifice-request.js';
import { RawContainedFinder } from './raw-contained-finder.js';
import { SoundProcess } from './sound-process.js';
import { grantForeignRitualSp, grantForeignRitualType } from './raw-foreign-ritual-reward.js';
import { readPlayerSpAmount, setPlayerSpAmount } from './raw-fort-money-load.js';
import { typeNumber } from '../assets/type-table.js';
import { currentNuggetType } from './raw-path-cleanup.js';

// 원문 ConnectRitual의 사제 앞단과 후속 의식이 공유할 Regular 예약 수명이다.
export class RawCampaignSessionRitual {
    // 동일 owner와 설치·반납 관문이다.
    #owner; #started = false; #ready = false; #templeStarted = false; #templeReady = false; #daisStarted = false; #daisReady = false; #runeRegistered = false; #disposed = false;
    // 아직 부착하지 않은 실제 예약과 현재 함수 표다.
    pendingRegular = null; frameHooks = null; actionHooks = null;
    // 신전·룬 효과는 같은 현재 World의 함수 표를 빌린다.
    templeHooks = null; runeHooks = null;
    // 희생에서 사용할 지연 소리의 미부착 객체와 실제 효과 표다.
    pendingSound = null; daisHooks = null; sacrificeHooks = null;
    // 구성만으로 현재 객체나 난수를 변경하지 않는다.
    constructor(owner) { this.#owner = owner; }
    // 반납한 의식 함수가 이전 World에 접근하지 못하도록 한다.
    #live() { if (this.#disposed || !this.#ready) throw new Error('반납/미연결 Session 의식 앞단'); }
    // 동일 객체 하나를 확보하며 중첩 예약은 원문처럼 거부한다.
    reserveRegular() {
        this.#live(); if (this.pendingRegular) throw new Error('의식 Regular 중첩 예약');
        this.pendingRegular = new RegularProcess(0, 0); return true;
    }
    // 확보한 동일 객체를 재초기화해 타입46 또는61로 부착하고 현재 시각을 사용한다.
    addRegular(sid, event, payload, type) {
        this.#live(); if (!this.pendingRegular) throw new Error('의식 Regular 실제 예약 누락');
        if (type !== 46 && type !== 61) throw new RangeError('의식 Regular 타입 오류');
        const reserved = this.pendingRegular; this.pendingRegular = null;
        reserved.initializeDetached(event, payload);
        const attached = this.#owner.core.processes.attachRegular(reserved, sid, type === 61);
        if (attached) attached.setNextTime(this.#owner.core.processState.now);
    }
    // 공황 뒤 실제 사제604 처리기와 K~P 동작을 같은 프레임/음향/Host에 한 번 설치한다.
    bind() {
        const o = this.#owner, c = o.core;
        if (this.#disposed || this.#started || !o.panicModules?.ready || o.priestRitualFrame || o.ritualActions) throw new Error('Session 의식 앞단 설치 순서 오류');
        this.#started = true;
        this.frameHooks = {
            setFrame: (sid, value, flags) => { this.#live(); c.frame.set(sid, value, flags); },
            eventExists: (sid, event) => { this.#live(); return !!c.processes.findEvent(sid, event); },
            playAt: (x, y, name, flags) => { this.#live(); if (!o.services.sound.playOnceAt) throw new Error('사제604 위치 음향 미연결'); o.services.sound.playOnceAt(x, y, o.soundNames.lookup(name), flags, 0); },
        };
        o.priestRitualFrame = new RawPriestRitualFrame(c.pool, c.frames, this.frameHooks);
        this.actionHooks = { setFrame: this.frameHooks.setFrame, reserveShared: () => this.reserveRegular(), addShared: (sid, event, payload) => this.addRegular(sid, event, payload, 61) };
        o.ritualActions = new RawPriestRitualActions(c.pool, c.frames, this.actionHooks);
        this.#ready = true;
    }
    // 사제 앞단 뒤 신전 의식과 룬의 실제 후처리를 같은 현재 자료에 연결한다.
    bindTemple() {
        this.#live(); const o = this.#owner, c = o.core, s = o.state;
        if (this.#templeStarted || o.templeRitual || o.runePostPop) throw new Error('Session 신전 의식 반복 결선');
        this.#templeStarted = true;
        s.ritualState.sharedPriestType = { get value() { return o.priestState.movement.priest; } };
        s.ritualState.sharedBoss = { get value() { return s.assetState.readBoss(); } };
        o.ritualLookup = new RawContainedLookup(c.pool, (sid, flags) => { this.#live(); o.world.destroy(sid, flags); });
        this.templeHooks = {
            assertion: (expression, _file, line) => { this.#live(); if (!o.services.ai.assertion) throw new Error(`의식 helper의 현재 진단 장치 미연결 ${line}`); o.services.ai.assertion(expression); },
            count: (sid) => { this.#live(); return o.rewardCount(sid); },
            pop: (sid, x, y, flags) => { this.#live(); o.world.pop(sid, x, y, flags); },
            playAt: this.frameHooks.playAt,
            reserveShared: () => this.reserveRegular(), addShared: (sid, event, payload) => this.addRegular(sid, event, payload, 61),
            create: (type, flags) => { this.#live(); return c.factory.create(type, flags); },
            setHitPoints: (sid, value) => { this.#live(); setRawSquidHitPoints(c.pool, sid, value); },
            setOwner: (sid, owner) => { this.#live(); c.owner.set(sid, owner); },
        };
        o.templeRitual = new RawTempleRitual(c.pool, c.hash, c.types, c.frames, c.shapes, c.spots, c.neighbors, o.hp, o.ritualActions, s.ritualState, this.templeHooks);
        this.runeHooks = {
            reserveRegular: () => this.reserveRegular(), addRegular: (sid, event, payload) => this.addRegular(sid, event, payload, 46),
            startConstruction: (sid, duration) => { this.#live(); o.assetModules.prepareBuilding(c.processState.now); o.building.start(sid, duration); },
            suppressDuration: () => { this.#live(); return o.globals.missionPost; },
            playerFlags: (owner) => { this.#live(); if (owner < 0 || owner > 8 || owner * 180 + 0x50 >= o.players.length) throw new RangeError('Rune 현재 Player 기록 물리 범위'); return o.players[owner * 180 + 0x50]; },
        };
        o.runePostPop = new RawRunePostPop(c.pool, c.hash, c.types, s.runeState, c.postPop, this.runeHooks);
        c.postPop.setBody('rune', o.runePostPop); this.#runeRegistered = true; this.#templeReady = true;
    }
    // 동일 신전·룬·경로/Player 장부에서 다이스 의식과 희생 요청을 구성한다.
    bindDais() {
        this.#live(); const o = this.#owner, c = o.core, s = o.state;
        if (!this.#templeReady || this.#daisStarted || o.daisState || o.daisRegular) throw new Error('Session 다이스 의식 설치 순서 오류');
        this.#daisStarted = true;
        const state = o.daisState = new DaisRegularState(s.ritualState, s.runeState, c.processState, o.playerState.battleOwner);
        state.bindLocalOwner({ get value() { return o.playerState.battleOwner; }, set value(value) { o.playerState.battleOwner = value; } });
        state.sharedBridgeType = () => c.bridgeState.bridgeType; state.sharedNuggetType = () => o.gameplayGlobals.gameAux().nuggetType;
        state.debug = () => o.gameplayGlobals.aiDebug().checking().value; state.bindPlayerRecords(o.players); state.voiceCue = 0x020000df;
        const assertion = o.services.ai.assertion;
        o.priestState.contained.bindChecking(() => o.gameplayGlobals.aiDebug().checking().value);
        o.priestState.contained.assertion = (expression, _file, line) => { if (!assertion) throw new Error(`내용물 finder 현재 진단 장치 미연결 ${line}`); assertion(expression); };
        // 값형 서비스 표의 원문 진단 함수는 설치 당시 함수를 보유한다.
        this.daisHooks = {
            notifySurface: (sid) => { this.#live(); c.building.notifySurface(sid); },
            assertion: (expression, _file, line) => { this.#live(); if (!assertion) throw new Error(`다이스 현재 진단 장치 미연결 ${line}`); assertion(expression); },
            boss: () => { this.#live(); return s.assetState.readBoss(); },
            alliance: (row, column) => { this.#live(); if (row > 8 || column > 8) throw new RangeError('다이스 현재 동맹 행/열 범위'); return o.alliances[row * 9 + column] | 0; },
            priestType: () => { this.#live(); return o.priestState.movement.priest; }, count: (sid) => { this.#live(); return o.rewardCount(sid); },
            setFrame: this.frameHooks.setFrame, destroy: (sid, flags) => { this.#live(); o.world.destroy(sid, flags); },
            reserveShared: () => this.reserveRegular(), addShared: (sid, event, payload) => this.addRegular(sid, event, payload, 61), playAt: this.frameHooks.playAt,
            processType: (type) => { this.#live(); return o.priests.movementProcessType(type); },
            pathMetadata: (sid) => { this.#live(); const d = o.assets.game.typeTable.definitions[c.pool.slot(sid)[10] - 70]; if (!d) throw new RangeError('다이스 경로 현재 TYPE 범위'); const speed = typeNumber(d, 'speed'), turning = typeNumber(d, 'turningSpeed'); if (speed == null || turning == null) throw new Error('다이스 경로 TYPE 속도 누락'); return { speed: Math.fround(speed), turningSpeed: Math.fround(turning), defaultFrame: d.defaultFrame }; },
            damageable: (sid, event, count, payload) => { this.#live(); return o.assetBindings.regular.damageableHandle(sid, event, count, payload); },
            reserveSound: () => { this.#live(); if (this.pendingSound) throw new Error('다이스 지연 소리 중첩 예약'); this.pendingSound = new SoundProcess(o.sounds, 0, 0, 0); return true; },
            loadSound: (name, alternate, flags) => { this.#live(); if (alternate || flags !== 0xc) throw new Error('다이스 지연 소리 인자 오류'); return o.soundNames.lookup(name); },
            attachSound: (sid, sound, alternate, flags) => { this.#live(); if (!this.pendingSound) throw new Error('다이스 지연 소리 실제 예약 누락'); const process = this.pendingSound; this.pendingSound = null; process.initializeUnattached(sound, alternate, flags); const attached = c.processes.attach(process, s.soundState.currentType(), sid, 0x10); return attached ? attached.form : 0; },
            wallSeconds: o.services.sound.wallSeconds,
            setSoundStart: (form, time) => { this.#live(); if (form > 65535) throw new RangeError('다이스 소리 Form 번호 범위'); const process = c.processes.fromForm(form); if (!(process instanceof SoundProcess)) throw new Error('다이스 현재 Sound47 누락'); process.setStartTime(time); },
            stopVoice: (command, first, second) => { this.#live(); if (!o.services.assets.queueCommand) throw new Error('다이스 실제 입력 명령 큐 미연결'); o.services.assets.queueCommand(command, first | 0, second | 0); },
            addLocalSp: (delta) => { this.#live(); o.hp.addSp(o.playerState.battleOwner, delta); },
            refreshOwner: (owner) => { this.#live(); o.priestParticipation.refresh(owner); },
            priestExists: (owner) => { this.#live(); if (!o.outcome) throw new Error('희생 완료 현재 참여 판정 설치 전 호출'); return o.outcome.priestDead(owner); },
            chat: (notice) => { this.#live(); if (notice.kind !== 'Stormpower' && notice.kind !== 'Upgrade') throw new Error('멀티플레이 희생 사망 안내 미연결'); if (!o.services.priest.shield.tell) throw new Error('희생 완료 실제 Tell 소비자 미연결'); o.services.priest.shield.tell(notice.kind === 'Stormpower' ? 'SacrificeForStormpower' : 'AltarUpgrade'); },
            localType: (type) => { this.#live(); if (!o.learnNewTechnology) throw new Error('로컬 기술 획득 현재 Session 미연결'); o.learnNewTechnology(type); },
            refreshLocal: () => { this.#live(); if (!o.refreshLocalRitual) throw new Error('의식 뒤 현재 Session 갱신 미연결'); o.refreshLocalRitual(); },
            normalizeForeignSp: (owner) => { this.#live(); const reads = { clientActive: () => o.playerState.playerKnown, localOwner: () => o.playerState.battleOwner, editor: () => o.globals.editor }; grantForeignRitualSp(owner, (player) => readPlayerSpAmount(player, o.sp, reads), (player, amount) => setPlayerSpAmount(player, amount, o.sp, reads, o.services.refreshSp)); },
            grantForeignType: (owner, type) => { this.#live(); grantForeignRitualType({ bytes: o.coreDataGlobals.records(), origin: 0 }, owner, type, (selected) => { if (selected >= c.types.length) throw new RangeError('비로컬 의식 TYPE 범위'); return c.types[selected].techBitIndex; }, () => { throw new Error('비로컬 의식 실제 진단 미연결'); }); },
            localSpecial: () => { throw new Error('다이스 로컬 특수 효과 미연결'); }, multiplayerStatus: () => { throw new Error('다이스 멀티플레이 상태 미연결'); },
        };
        o.daisRegular = new RawDaisRegular(c.pool, c.hash, c.types, c.frames, o.templeRitual, o.runePostPop, o.priests.carrier, c.processes, o.ritualLookup, o.motion.paths.starter, state, this.daisHooks);
        this.sacrificeHooks = {
            allowed: (dais, priest) => { this.#live(); o.runtimeModules.prepareRitual(); return o.daisRegular.canBridge(dais, priest); },
            remove: (parent, type, index, remove) => { this.#live(); const finder = new RawContainedFinder(c.pool, o.contentFinder); for (let sid = finder.begin(parent); sid; sid = finder.next()) { const raw = c.pool.slot(sid); if ((raw[18] | (raw[19] << 8)) !== type) continue; if (!index) { if (!remove) return; if (c.pool.isServer || (sid > 4 && sid < c.pool.layout.serverFirst)) o.world.destroy(sid, 0); } index = (index - 1) >>> 0; } },
            command: (dais, priest, current) => { this.#live(); o.runtimeModules.prepareRitual(); o.daisRegular.bridgeCommand(dais, priest, current); },
            music: (name) => { this.#live(); if (!o.services.requestMusic) throw new Error('희생 의식 실제 음악 요청 미연결'); o.services.requestMusic(name); },
        };
        o.sacrificeRequest = new RawPriestSacrificeRequest(c.pool, () => currentNuggetType(s.motionState.cleanup), () => o.playerState.battleOwner, this.sacrificeHooks);
        o.runtimeModules.prepareRitual(); this.#daisReady = true;
    }
    // 실제 Host를 비운 뒤 미부착 예약과 함수 대여를 반납한다.
    dispose() { if (this.#disposed) return; this.#disposed = true; this.pendingRegular = null; this.pendingSound = null; if (this.#runeRegistered) { this.#owner.core.postPop.setBody('rune', null); this.#runeRegistered = false; } }
    // 이 앞단의 완료이며 나머지 의식 및 Load 완료는 아니다.
    get ready() { return this.#ready; }
    // 신전과 룬만의 설치 상태이며 다이스와 전체 Load는 포함하지 않는다.
    get templeReady() { return this.#templeReady; }
    // 다이스 및 희생 요청 설치 상태이며 제단 수명과 전체 Load는 포함하지 않는다.
    get daisReady() { return this.#daisReady; }
}
