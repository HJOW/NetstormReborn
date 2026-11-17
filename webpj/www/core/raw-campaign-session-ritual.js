// 같은 Session의 의식 프레임과 예약 객체를 현재 Pool·Frame·Host에 연결한다(225553df, WB04-E645).
import { RegularProcess } from './squid-process.js';
import { RawPriestRitualActions } from './raw-priest-ritual-actions.js';
import { RawPriestRitualFrame } from './raw-priest-ritual-frame.js';
import { RawContainedLookup } from './raw-contained-lookup.js';
import { RawTempleRitual } from './raw-temple-ritual.js';
import { RawRunePostPop } from './raw-rune-post-pop.js';
import { setRawSquidHitPoints } from './raw-squid-word.js';

// 원문 ConnectRitual의 사제 앞단과 후속 의식이 공유할 Regular 예약 수명이다.
export class RawCampaignSessionRitual {
    // 동일 owner와 설치·반납 관문이다.
    #owner; #started = false; #ready = false; #templeStarted = false; #templeReady = false; #runeRegistered = false; #disposed = false;
    // 아직 부착하지 않은 실제 예약과 현재 함수 표다.
    pendingRegular = null; frameHooks = null; actionHooks = null;
    // 신전·룬 효과는 같은 현재 World의 함수 표를 빌린다.
    templeHooks = null; runeHooks = null;
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
    // 실제 Host를 비운 뒤 미부착 예약과 함수 대여를 반납한다.
    dispose() { if (this.#disposed) return; this.#disposed = true; this.pendingRegular = null; if (this.#runeRegistered) { this.#owner.core.postPop.setBody('rune', null); this.#runeRegistered = false; } }
    // 이 앞단의 완료이며 나머지 의식 및 Load 완료는 아니다.
    get ready() { return this.#ready; }
    // 신전과 룬만의 설치 상태이며 다이스와 전체 Load는 포함하지 않는다.
    get templeReady() { return this.#templeReady; }
}
