// ConnectMotion3889..3929의 채취·너겟 전달·TYPE 메타데이터와 실제 이동 본체 조립이다(225553df, WB04-E643).
// 사용: 같은 owner의 기준점/운반 경로 뒤 bind. 뒤 ConnectMotion의 의식/명령/송신 설치와 Load는 별도다.
import { RawGeyserMined } from './raw-geyser-mined.js';
import { RawCarrierMining } from './raw-carrier-mining.js';
import { RawCarrierTransfer } from './raw-carrier-transfer.js';
import { RawContainedLookup } from './raw-contained-lookup.js';
import { RawCampaignMotionBindings } from './raw-campaign-motion-bindings.js';
import { startSelectionContentPreview } from './raw-squid-selection.js';
import { typeNumber, typeString } from '../assets/type-table.js';
import { CarrierMoveState, RawCarrierMove } from './raw-carrier-move.js';
import { findAttachedProcess } from './raw-wind-aviary-process.js';
import { RawPathProcess } from './raw-path-process.js';
import { RawSquidFinder } from './raw-squid-finder.js';

// 선택한 자리에서만 원문의 미연결 함수 경계를 요구한다.
function fillMissing(table, key) { if (!table[key]) table[key] = () => { throw new Error(`Session 이동 본체의 실제 ${key} 미연결`); }; }
// 필수 TYPE 수치는 value()와 같이 없으면 실패하며 float로 저장한다.
function requiredNumber(definition, key) { const value = typeNumber(definition, key); if (value === null || value === undefined) throw new Error(`경로 현재 TYPE ${key} 누락`); return Math.fround(value); }

// 기존 단일 Pool/World/Host에서 후속 경로 순환 참조를 실제 모듈로 닫는다.
export class RawCampaignSessionMotionRuntime {
    // 기존 Impl과 한 번 설치/부분 실패/정상 복귀/반납 관문이다.
    #owner; #started = false; #ready = false; #disposed = false;
    // 현재6BYTE 헤더의 지역 수명과 같은 채취/전달 함수 표다.
    miningHeader = { message: 0x5b, protocol: 1, length: 6 };
    minedHooks = null; miningHooks = null; transferHooks = null;
    // 운반 이동 요청의 현재 패킷 헤더와 같은 월드의 효과다.
    moveState = null; moveHooks = null;
    // 구성 자체로 실제 객체/프로세스/난수를 만들지 않는다.
    constructor(owner) { this.#owner = owner; }
    // 반납한 owner가 다음 세대의 전역이나 Pool을 사용하는 것을 막는다.
    #live() { if (this.#disposed || !this.#ready) throw new Error('반납/미연결 Session 이동 본체'); }

    // 채취/전달 뒤 현재 TYPE 효과를 구성하고 실제 MotionBindings를 마지막에 만든다.
    bind() {
        const o = this.#owner, c = o.core, s = o.state;
        if (this.#disposed || this.#started || !o.carrierRoutingModules?.ready || o.motion) throw new Error('Session 이동 본체의 초기화 전/반복 결선');
        this.#started = true; const e = o.motionModules.effects;
        const mined = this.minedHooks = {
            sound: (x, y, name, flags) => { this.#live(); o.services.sound.playOnceAt(x, y, o.soundNames.lookup(name), flags, 0); },
            destroy: (sid, flags) => { this.#live(); o.world.destroy(sid, flags); },
            create: (type, flags) => { this.#live(); return o.world.create(type, flags); },
            pop: (sid, x, y, flags) => { this.#live(); o.world.pop(sid, x, y, flags); },
        };
        o.geyserMined = new RawGeyserMined(c.pool, c.hash, c.types, o.random, s.geyserMinedState, mined);
        const mining = this.miningHooks = {
            broadcast: (packet) => { this.#live(); o.tables.broadcast.send(packet); },
            receive: (packet) => { this.#live(); o.buildingModules.receiveGeyserMined(packet); },
        };
        o.carrierMining = new RawCarrierMining(c.pool, this.miningHeader, mining);
        e.carrier.minedPacket = (geyser) => { this.#live(); o.carrierMining.notify(geyser); };
        // CD 전용 경제/음향과 원문 Robust 하위는 선택 시 명시 오류 또는 기존 외부 효과를 유지한다.
        for (const key of ['cdAddSp', 'cdOwnerPacket', 'playAt', 'robust']) fillMissing(e.carrier, key);
        const transfer = this.transferHooks = {
            find: (sid, mask, kind, index) => { this.#live(); return o.priests.carrier.findContained(sid, mask, kind, index); },
            detach: (form, flags) => { this.#live(); if (flags) throw new Error('너겟 전달의 실제 Unpop flags 오류'); o.contentForms.unpop(form); },
            attach: (form, parent, flags) => { this.#live(); o.contentForms.attach(form, parent, flags); },
            preview: (parent) => { this.#live(); startSelectionContentPreview(c.pool, parent, o.objectState.loadingDepth, (sid, event, payload) => { this.#live(); c.processes.addRegular(sid, event, payload); }, () => { throw new Error('너겟 전달 선택 미리보기의 실제 진단 미연결'); }); },
            broadcast: mining.broadcast,
            movementType: (type) => { this.#live(); return o.priests.movementProcessType(type); },
            removeMovement: (sid, type) => { this.#live(); new RawContainedLookup(c.pool, (target, flags) => { o.world.destroy(target, flags); }).find(sid, type, 0, true); },
            chooseGeyser: e.carrier.chooseGeyser, goGeyser: e.carrier.goGeyser, goHome: e.carrierGoHome,
            commonHalt: (sid) => { this.#live(); if (!o.motion) throw new Error('현재 이동 연결 전 고갈된 캐리어 정지'); return o.motion.commonHalt(sid); },
        };
        fillMissing(transfer, 'goHome');
        e.halt.carrierPickup = (sid) => { this.#live(); return o.carrierSteal.try(sid); };
        o.carrierTransfer = new RawCarrierTransfer(c.pool, c.hash, c.types, () => s.assetState.readBoss(), () => o.globals.editor, () => o.playerState.battleOwner, s.carrierTransferState, transfer);
        e.halt.nuggetTransfer = (sid) => { this.#live(); return o.carrierTransfer.transfer(sid); };
        e.priest.neutralReturn = (sid) => { this.#live(); if (!o.priestPanic) throw new Error('사제 panic 실제 설치 전 호출'); o.priestPanic.run(sid); };
        e.path.metadata = (sid) => {
            this.#live(); const kind = c.pool.slot(sid)[10], definition = o.assets.game.typeTable.definitions[kind - 70];
            if (!definition) throw new RangeError('경로 현재 TYPE 정의 범위');
            return { speed: kind === 70 || kind === 158 ? o.dudeSpeeds[kind].speed : requiredNumber(definition, 'speed'), turningSpeed: requiredNumber(definition, 'turningSpeed'), defaultFrame: definition.defaultFrame };
        };
        e.path.crabHalt = (sid) => { this.#live(); return o.carrierSteal.halt(sid); };
        e.path.dudeHalt = (sid) => { this.#live(); return o.dudeBehavior.halt(sid); };
        e.path.dudeArrival = (sid) => { this.#live(); return o.dudeBehavior.arrival(sid); };
        e.path.soundId = (sid) => { this.#live(); const sound = typeString(o.assets.game.typeTable.definitions[c.pool.slot(sid)[10] - 70], 'stepSound'); return sound ? o.soundNames.lookup(sound) : 0; };
        fillMissing(e.path, 'stepSound');
        o.motion = new RawCampaignMotionBindings(o.world, o.priests, o.islands, o.contained, o.anchor, o.hp, o.metadata.frames, s.motionState, s.start, s.process, e);
        this.#ready = true;
    }
    // 의식과 회수 뒤 현재 선택·경로·운반 대상에 이동 요청과 수신을 연결한다.
    bindCarrierMove() {
        this.#live(); const o = this.#owner, c = o.core;
        if (!o.salvageCommand || this.moveState || o.carrierMove) throw new Error('Session 운반 이동 설치 순서 오류');
        const state = new CarrierMoveState(), send = o.services.carrierRequest;
        const hooks = {
            selected: () => { this.#live(); if (!o.services.deletion.selected) throw new Error('이동 요청의 현재 선택 조회 미연결'); return o.services.deletion.selected(); },
            select: (sid, force, activate) => { this.#live(); const selection = o.activeSelection?.deref()?.selection; if (!selection) throw new Error('이동 요청의 현재 선택 수명 미연결'); selection.select(sid, force, activate); },
            assertion: (line) => { this.#live(); throw new Error(`원본 Carrier.cpp 이동 진단 ${line}`); },
            connected: (a, b) => { this.#live(); return o.anchor.graphAt(a.x, a.y) === o.anchor.graphAt(b.x, b.y); },
            movement: (kind) => { this.#live(); return o.priests.movementProcessType(kind); },
            path: (parent, kind) => { this.#live(); const form = findAttachedProcess(c.pool, parent, kind); if (!form) return null; const process = c.processes.fromForm(form); if (!(process instanceof RawPathProcess)) throw new Error('이동 요청의 현재 종속/Host 경로 불일치'); return process.runtime; },
            send: (target, packet) => { this.#live(); if (!send) throw new Error('이동 요청 실제 송신 미연결'); send(target, packet); },
            regular: (parent, event) => { this.#live(); return Boolean(c.processes.findEvent(parent, event)); },
            candidate: (parent, point) => { this.#live(); return o.motion.carrierArrival.pickupCandidate(parent, point.x, point.y); },
            returnPlace: (point) => { this.#live(); return o.motion.cleanup.returnPlaceAt(point.x, point.y, 0); },
            target: (parent, place) => { this.#live(); o.carrierTarget.startTarget(parent, place, 0, 0); },
            genusAt: (point, genus) => {
                this.#live(); const finder = new RawSquidFinder(c.pool, c.hash, c.types);
                const area = { left: Math.trunc(point.x), top: Math.trunc(point.y), right: Math.trunc(point.x), bottom: Math.trunc(point.y) };
                // 원본 finder의 첫 현재 genus 일치 후보를 반환한다.
                for (let sid = finder.begin(area, (genus & 6) === 0 ? 1 : 0); sid; sid = finder.next()) if (c.types[c.pool.slot(sid)[10]].flags2 & genus) return sid;
                return 0;
            },
            geyser: (parent, geyser) => { this.#live(); return o.carrierTarget.goGeyser(parent, geyser)?.runtime ?? null; },
            altarAllowed: (dais, priest) => { this.#live(); return o.sacrificeRequest.allowed(dais, priest); },
            sacrifice: (priest, dais) => { this.#live(); o.sacrificeRequest.begin(priest, dais); },
            start: (request) => { this.#live(); return o.carrierRoutingModules.startMotion(request)?.runtime ?? null; },
        };
        o.carrierMove = new RawCarrierMove(c.pool, c.types, () => c.ownerMode.fort, state, hooks);
        this.moveState = state; this.moveHooks = hooks;
    }
    // 실제 Host가 먼저 비워진 뒤 지역 함수의 대여를 반납한다. 장기 목록/시계를 재설정하지 않는다.
    dispose() { this.#disposed = true; }
    // 앞부분의 실제 본체 설치 완료이며 전체 ConnectMotion/Load가 아니다.
    get ready() { return this.#ready; }
}
