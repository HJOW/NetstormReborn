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
import { RawAiMovement, aiDistanceToFootprint } from './raw-ai-movement.js';
import { RawSquidName } from './raw-squid-name.js';
import { RawAiResourceChoice } from './raw-ai-resource-choice.js';
import { countContained } from './raw-contained-finder.js';
import { nuggetTypeNow } from './raw-carrier-arrival.js';
import { RawAiCarrierActions, findContainedGenus, daisApproachPoint } from './raw-ai-carrier-actions.js';
import { RawAiBuildSearch } from './raw-ai-build-search.js';
import { GunGroundTargetGlobals, RawGunGroundTarget } from './raw-gun-ground-target.js';
import { RawAiDropTargets } from './raw-ai-drop-targets.js';
import { RawAiDropEvaluation } from './raw-ai-drop-evaluation.js';
import { RawAiDropScan } from './raw-ai-drop-scan.js';
import { RawAiDropSelection, AI_DROP_SELECTION_LOG } from './raw-ai-drop-selection.js';
import { RawAiDryGeometry } from './raw-ai-dry-geometry.js';
import { RawAiFakeDrySpots, AI_FAKE_DRY_LOG } from './raw-ai-fake-dry-spots.js';
import { RawAiDropActions, AI_DROP_ACTION_LOG } from './raw-ai-drop-actions.js';
import { RawAiPriestActions, aiPriestInRitual, aiAltarContainsEnemy } from './raw-ai-priest-actions.js';
import { RawAiBridgePath } from './raw-ai-bridge-path.js';

// 현재 TYPE 표의 500바이트 간격을 재현하는 불투명 탐색 핸들 기준이다.
const AI_SEARCH_TYPE_BASE = 0x6b000000;

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
    // AI와 사용자 이동이 공유할 현재 명령 헤더 및 AI 효과다.
    aiMoveHooks = null;
    // AI 자원 선택이 사용하는 현재 월드 조회와 로그 효과다.
    aiResourceHooks = null;
    // AI 운반 행동이 사용하는 현재 월드 조회·명령 효과다.
    aiCarrierHooks = null;
    // AI 배치 탐색이 빌리는 현재 시계·배치 검사다.
    aiSearchHooks = null;
    // 지상 표적이 읽는 현재 소유자와 진단 전역이다.
    aiGroundGlobals = null;
    // AI 배치 표적 평가가 사용하는 실제 총기·객체 효과다.
    aiDropTargetHooks = null;
    // AI 배치 평가가 사용하는 공급·배치·부족 좌표 효과다.
    aiDropEvaluationHooks = null;
    // AI 배치 스캔의 실제 발자국·후보 평가 장치다.
    aiDropScanHooks = null;
    // AI 배치 선택이 사용하는 현재 기술과 후보 처리 장치다.
    aiDropSelectionHooks = null;
    // 부족 좌표 탐색의 현재 지도와 표식 생성·삭제 효과다.
    aiDryGeometryHooks = null;
    // 가짜 부족 좌표 갱신이 빌리는 평가·목록·표식 효과다.
    aiFakeDryHooks = null;
    // 실제 AI 배치 행동이 빌리는 선택·생산·검사 효과다.
    aiDropActionHooks = null;
    // 사제 AI가 같은 월드의 탐색·이동·회수를 호출하는 장치다.
    aiPriestActionHooks = null;
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
    // 운반 이동 뒤 AI의 좌표·대상 이동을 같은 명령 헤더와 경로에 연결한다.
    bindAiMovement() {
        this.#live(); const o = this.#owner, c = o.core;
        if (!this.moveState || this.aiMoveHooks || o.aiMovement) throw new Error('Session AI 이동 설치 순서 오류');
        const names = { ...o.services.ai.names, level: (sid) => { this.#live(); return o.assetModules.virtualCount(sid); } };
        const objectNames = new RawSquidName(c.pool, c.types, names);
        // 현재 AI 본체의 소유자와 로그 종류를 읽어 원문 접두를 만든다.
        const log = (token, message) => {
            this.#live(); if (!o.services.player.log) throw new Error('AI 이동 현재 로그 장치 미연결');
            const raw = o.aiMemory.bytes(token), view = new DataView(raw.buffer, raw.byteOffset, raw.byteLength);
            o.services.player.log(o.gameplayGlobals.aiDebug().read(0x52f3d8), `AI${view.getInt32(0x260, true)}: ${message}\n`);
        };
        const hooks = {
            movement: (kind) => { this.#live(); return o.priests.movementProcessType(kind); },
            lookup: (parent, kind, index, remove) => { this.#live(); return new RawContainedLookup(c.pool, (sid, flags) => o.world.destroy(sid, flags)).find(parent, kind, index, remove); },
            footprint: (sid) => { this.#live(); return c.neighbors.footprint(sid); },
            send: (target, packet) => { this.#live(); if (!o.services.carrierRequest) throw new Error('AI 실제 이동 요청 송신 미연결'); o.services.carrierRequest(target, packet); },
            sacrifice: (actor, target) => { this.#live(); o.sacrificeRequest.begin(actor, target); },
            target: (actor, target, replace, helper) => { this.#live(); return o.carrierTarget.startTarget(actor, target, replace, helper); },
            start: (request) => { this.#live(); return o.carrierRoutingModules.startMotion(request); },
            name: (sid) => { this.#live(); return objectNames.name(sid); },
            distance: (point, sid) => { this.#live(); return aiDistanceToFootprint(point, c.neighbors.footprint(sid)); },
            logTarget: (token, source, target) => log(token, `Move ${source} to ${target}`),
            logPoint: (token, name, x, y) => log(token, `Move ${name} to (${x},${y})`),
        };
        o.objectNames = objectNames;
        o.aiMovement = new RawAiMovement(o.aiMemory, c.pool, () => o.state.assetState.readBoss(), o.boardTarget, this.moveState, hooks);
        this.aiMoveHooks = hooks;
    }
    // AI 이동 뒤 현재 간헐천 목록·신전·운반 내용물의 선택을 연결한다.
    bindAiResourceChoice() {
        this.#live(); const o = this.#owner, c = o.core, s = o.state;
        if (!this.aiMoveHooks || this.aiResourceHooks || o.aiResourceChoice) throw new Error('Session AI 자원 선택 설치 순서 오류');
        // 현재 AI 소유자와 선택한 로그 번호를 원본 문자열에 전달한다.
        const log = (token, kind, text) => {
            this.#live(); if (!o.services.player.log) throw new Error('AI 자원 선택 실제 로그 미연결');
            const raw = o.aiMemory.bytes(token), view = new DataView(raw.buffer, raw.byteOffset, raw.byteLength);
            o.services.player.log(kind, `AI${view.getInt32(0x260, true)}: ${text}\n\n`);
        };
        const hooks = {
            center: (sid) => { this.#live(); const point = c.neighbors.center(sid); return { x: point[0], y: point[1] }; },
            graph: (sid) => { this.#live(); return o.anchor.graphFor(sid); },
            crystals: (sid) => { this.#live(); return countContained(c.pool, o.priestState.contained, sid, 0x20, nuggetTypeNow(s.motionState.carrier)); },
            target: (token, actor, target) => { this.#live(); o.aiMovement.toTarget(token, actor, target, 0); },
            home: (token, actor) => { this.#live(); o.aiMovement.home(token, actor); },
            captureLog: (token, actor) => log(token, 29, `PriestGetter==${actor}`),
            disconnectedLog: (token, geyser) => log(token, o.gameplayGlobals.aiDebug().read(0x52f3e4), `Put disconnectd geyser ${geyser} back into yetToConnect list.`),
            assertion: (text) => { this.#live(); if (!o.services.ai.assertion) throw new Error('AI 간헐천 원본 진단 미연결'); o.services.ai.assertion(text); },
        };
        o.aiResourceChoice = new RawAiResourceChoice(o.aiMemory, c.pool, c.types, () => c.bookkeeping.invalidGraph >>> 0, hooks);
        this.aiResourceHooks = hooks;
    }
    // AI 자원 선택 뒤 운반 행동을 같은 경로·내용물·Player에 연결한다.
    bindAiCarrierActions() {
        this.#live(); const o = this.#owner, c = o.core, s = o.state;
        if (!this.aiResourceHooks || this.aiCarrierHooks || o.aiCarrierActions) throw new Error('Session AI 운반 행동 설치 순서 오류');
        // 원본 참여 번호 진단 뒤 현재 Player와 사제 상태를 읽는다.
        const enemyAvailable = (owner) => {
            this.#live();
            if (owner < 1 || owner > 8) { if (!o.services.ai.assertion) throw new Error('AI 적 사제 참여 진단 미연결'); o.services.ai.assertion('playerId > INVALID_PLAYER_ID && playerId < MAX_PLAYERS'); return false; }
            return Boolean(o.players[owner * 180 + 0x50] & 0x22) && o.priestState.priests.currentOwnerState()[owner] === 0;
        };
        const hooks = {
            maxHitPoints: (sid) => { this.#live(); return o.hp.maxHitPoints(sid); },
            prisoner: (sid) => { this.#live(); return findContainedGenus(c.pool, o.priestState.contained, c.types, sid, 0x20, 0x200000, 0); },
            path: (sid) => { this.#live(); return o.aiMovement.movementForm(sid); },
            daisPoint: (sid) => { this.#live(); return daisApproachPoint(c.pool, c.types, () => s.gunState.checking, o.world.storage.boardTargetState.invalid, (text) => { if (!o.services.ai.assertion) throw new Error('AI 다이스 접근 진단 미연결'); o.services.ai.assertion(text); }, sid); },
            point: (token, sid, point) => { this.#live(); o.aiMovement.toPoint(token, sid, point, 0); },
            home: (token, sid) => { this.#live(); o.aiMovement.home(token, sid); },
            enemyAvailable,
            vulnerable: (sid) => { this.#live(); return o.aiCollectors.vulnerable(sid); },
            flyingGetter: (token) => { this.#live(); return o.aiResourceChoice.hasFlyingCaptureCarrier(token); },
            graph: (sid) => { this.#live(); return o.anchor.graphFor(sid); },
            crystals: (sid) => this.aiResourceHooks.crystals(sid),
            firstCrystal: (sid) => { this.#live(); return o.priests.carrier.findContained(sid, 0x20, nuggetTypeNow(s.motionState.carrier), 0); },
            dropRequest: (sid) => { this.#live(); if (!o.carrierDrop || !o.prepareCarrierDrop) throw new Error('AI 운반 투하 실제 설치 전 호출'); o.prepareCarrierDrop(); o.carrierDrop.request(sid, 0); },
            destroy: (sid) => { this.#live(); o.world.destroy(sid, 0); },
            target: (token, actor, target) => { this.#live(); o.aiMovement.toTarget(token, actor, target, 0); },
            remember: (token, sid) => { this.#live(); o.aiResourceChoice.rememberCaptureCarrier(token, sid); },
            collect: (token, sid) => { this.#live(); o.aiResourceChoice.collectOrHome(token, sid); },
            targets: (sid) => { this.#live(); const kind = o.priests.movementProcessType(c.pool.slot(sid)[10]); const form = new RawContainedLookup(c.pool, (target, flags) => o.world.destroy(target, flags)).find(sid, kind); if (!form) return [0, 0]; const path = c.processes.fromForm(form); if (!(path instanceof RawPathProcess)) throw new Error('AI 현재 이동 프로세스 미연결'); return [path.runtime.targetA, path.runtime.targetB]; },
            reclassify: (token) => { this.#live(); o.aiResourceChoice.reclassifyDisconnected(token); },
            stop: (sid) => { this.#live(); o.aiMovement.stop(sid); },
            assertion: (text, line) => { this.#live(); if (!o.services.ai.assertion) throw new Error(`AI 운반 행동 원본 진단 미연결 ${line}`); o.services.ai.assertion(text); },
        };
        const globals = { now: () => c.processState.now, boss: () => s.assetState.readBoss(), checking: () => s.gunState.checking, invalidGraph: () => c.bookkeeping.invalidGraph >>> 0 };
        o.aiCarrierActions = new RawAiCarrierActions(o.aiMemory, c.pool, c.types, globals, hooks);
        this.aiCarrierHooks = hooks;
    }
    // AI 운반 행동 뒤 같은 spot·TYPE과 게임 배치 파이프라인을 탐색에 연결한다.
    bindAiBuildSearch() {
        this.#live(); const o = this.#owner, c = o.core;
        if (!this.aiCarrierHooks || this.aiSearchHooks || o.aiBuildSearch) throw new Error('Session AI 배치 탐색 설치 순서 오류');
        const hooks = {
            wall: o.services.construction.wallSeconds,
            mayPlace: (query) => { this.#live(); o.assetModules.prepareConstruction(); return o.constructionModules.mayPlace(query); },
        };
        o.aiBuildSearch = new RawAiBuildSearch(o.aiMemory, c.types, c.spots, AI_SEARCH_TYPE_BASE, hooks);
        this.aiSearchHooks = hooks;
    }
    // AI 배치 탐색 뒤 같은 총기 표와 현재 월드로 지상 표적 조회를 연결한다.
    bindAiGroundTarget() {
        this.#live(); const o = this.#owner, c = o.core, s = o.state;
        if (!this.aiSearchHooks || this.aiGroundGlobals || o.aiGroundTarget) throw new Error('Session AI 지상 표적 설치 순서 오류');
        const globals = new GunGroundTargetGlobals();
        Object.defineProperties(globals, {
            excludedType: { get: () => o.aiGroundExcludedType },
            dependType: { get: () => o.flyerDependType },
            metamap: { get: () => c.ownerMode.challenge },
            boss: { get: () => s.assetState.readBoss() },
        });
        globals.sharedChecking = () => s.gunState.checking;
        globals.assertion = (text, line) => { this.#live(); if (!o.services.ai.assertion) throw new Error(`AI 지상 검색 원본 진단 미연결 ${line}`); o.services.ai.assertion(text); };
        o.aiGroundTarget = new RawGunGroundTarget(c.pool, c.hash, c.types, o.gunTypes, o.gunTarget, s.gunTargetState, globals);
        this.aiGroundGlobals = globals;
    }
    // AI 지상 검색 뒤 같은 총기·장애물과 현재 객체 정보로 배치 표적을 평가한다.
    bindAiDropTargets() {
        this.#live(); const o = this.#owner, c = o.core, s = o.state;
        if (!this.aiGroundGlobals || this.aiDropTargetHooks || o.aiDropTargets) throw new Error('Session AI 배치 표적 설치 순서 오류');
        const globals = { altarType: () => s.runeState.readAltarType(), directionalType: () => s.constructionState.confirm.patternTypes[7] };
        const hooks = {
            captive: (sid) => { this.#live(); return findContainedGenus(c.pool, o.priestState.contained, c.types, sid, 0x20, 0x200000, 0); },
            rate: (sid) => { this.#live(); const type = c.pool.slot(sid)[10]; if (!o.hitTypes[type]) throw new RangeError('AI 배치 표적 현재 TYPE 범위'); return o.hitTypes[type].damagePerSecond; },
            maxHitPoints: (sid) => { this.#live(); return o.hp.maxHitPoints(sid); },
            ground: (x, y, type, owner) => { this.#live(); return o.aiGroundTarget.find(x, y, type, owner); },
            gun: (x, y, type, direction, owner) => { this.#live(); return o.gunTarget.findTarget(x, y, type, direction, owner); },
            blockers: (owner, point, capacity) => { this.#live(); return o.gunBlocker.collect(owner, point, capacity); },
            center: (sid) => { this.#live(); const point = c.neighbors.center(sid); return { x: point[0], y: point[1] }; },
            assertion: (text, line) => { this.#live(); if (!o.services.ai.assertion) throw new Error(`AI 배치 표적 원본 진단 미연결 ${line}`); o.services.ai.assertion(text); },
        };
        o.aiDropTargets = new RawAiDropTargets(o.aiMemory, c.pool, c.types, globals, hooks);
        this.aiDropTargetHooks = hooks;
    }
    // 표적 평가 뒤 에너지 공급과 부족 좌표를 같은 AI 본체에 연결한다.
    bindAiDropEvaluation() {
        this.#live(); const o = this.#owner, c = o.core;
        if (!this.aiDropTargetHooks || this.aiDropEvaluationHooks || o.aiDropEvaluation) throw new Error('Session AI 배치 평가 설치 순서 오류');
        // 현재 AI owner와 선택한 종류로 원본 진단 로그를 남긴다.
        const log = (token, kind, text) => {
            this.#live(); if (!o.services.player.log) throw new Error('AI 배치 평가 실제 로그 미연결');
            const raw = o.aiMemory.bytes(token), view = new DataView(raw.buffer, raw.byteOffset, raw.byteLength);
            o.services.player.log(kind, `AI${view.getInt32(0x260, true)}: ${text}\n\n`);
        };
        const hooks = {
            collect: (box, x, y, type, owner, theme) => { this.#live(); box.text = o.energySupply.collect(x, y, type, owner, theme); },
            mayPlace: (query) => { this.#live(); o.assetModules.prepareConstruction(); return o.constructionModules.mayPlace(query); },
            energy: (box, x, y, type, owner) => { this.#live(); const result = o.energySupply.evaluate(box.text, x, y, type, owner); box.text = result.output; return result.value; },
            dry: (token, x, y, type) => { this.#live(); return o.aiDrySpots.add(token, x, y, type); },
            unsupported: (token, type) => { this.#live(); if (!c.types[type]) throw new RangeError('AI 배치 평가 TYPE 범위'); log(token, 29, `I don't know how to make evaluations for ${c.types[type].name}`); },
            dryLog: (token, x, y) => log(token, o.gameplayGlobals.aiDebug().read(0x52f3dc), `Dry Spot at (${x},${y})`),
        };
        const globals = { manaRadius: () => o.cursorManaGlobals.radiusLimit, probeType: () => o.aiManaProbeType };
        o.aiDropEvaluation = new RawAiDropEvaluation(o.aiMemory, c.pool, c.types, o.aiDropTargets, globals, hooks);
        this.aiDropEvaluationHooks = hooks;
    }
    // 배치 평가 뒤 같은 표면·spot·난수에서 AI 후보를 순회한다.
    bindAiDropScan() {
        this.#live(); const o = this.#owner, c = o.core;
        if (!this.aiDropEvaluationHooks || this.aiDropScanHooks || o.aiDropScan) throw new Error('Session AI 배치 스캔 설치 순서 오류');
        const hooks = {
            free: (type, x, y) => { this.#live(); return o.aiBuildSearch.freeFootprint(type, x, y); },
            consider: (token, type, point, direction, best, target) => { this.#live(); return o.aiDropEvaluation.considerStored(token, type, point, direction, best, target); },
            assertion: (text, line) => { this.#live(); if (!o.services.ai.assertion) throw new Error(`AI 배치 스캔 원본 진단 미연결 ${line}`); o.services.ai.assertion(text); },
        };
        const globals = { bridgeType: () => c.bridgeState.bridgeType, bounds: () => o.aiDropScanBounds };
        o.aiDropScan = new RawAiDropScan(o.aiMemory, c.pool, c.types, c.frames, c.spots, c.hash.view(0), o.alliances, o.random, globals, hooks);
        this.aiDropScanHooks = hooks;
    }
    // 후보 스캔 뒤 현재 기술 내용물과 네 방향 TYPE으로 배치 대상을 선택한다.
    bindAiDropSelection() {
        this.#live(); const o = this.#owner, c = o.core, s = o.state;
        if (!this.aiDropScanHooks || this.aiDropSelectionHooks || o.aiDropSelection) throw new Error('Session AI 배치 선택 설치 순서 오류');
        const directional = Array(4);
        // 원본의 네 전역은 값 복사 대신 매 조회마다 현재 TYPE을 읽는다.
        for (let index = 0; index < 4; index += 1) Object.defineProperty(directional, index, { get: () => s.constructionState.confirm.patternTypesRead()[index + 4] });
        const hooks = {
            technology: (owner) => { this.#live(); return o.aiProductionModules.technology(owner); },
            count: (parent) => { this.#live(); return countContained(c.pool, o.priestState.contained, parent, 8, 0); },
            blocker: (parent) => { this.#live(); return findContainedGenus(c.pool, o.priestState.contained, c.types, parent, 8, 0x4000000, 0); },
            nth: (parent, index) => { this.#live(); const sid = o.priests.carrier.findContained(parent, 8, 0, index); if (!sid) return 0; const raw = c.pool.slot(sid); return raw[18] | (raw[19] << 8); },
            mayPlace: (query) => { this.#live(); o.assetModules.prepareConstruction(); return o.constructionModules.mayPlace(query); },
            remove: (token, index) => { this.#live(); o.aiDropScan.remove(token, index); },
            log: (token, action, type, direction) => {
                this.#live(); if (!o.services.player.log) throw new Error('AI 배치 선택 실제 로그 미연결');
                const raw = o.aiMemory.bytes(token), view = new DataView(raw.buffer, raw.byteOffset, raw.byteLength);
                const kind = action === AI_DROP_SELECTION_LOG.hated ? 29 : o.gameplayGlobals.aiDebug().read(0x52f3d4);
                const message = action === AI_DROP_SELECTION_LOG.hated ? 'I hated the last one...' : action === AI_DROP_SELECTION_LOG.retried ? 'That worked. Lets try it again!' : `Scanning ${c.types[type].name} at rotation ${direction}.\n`;
                o.services.player.log(kind, `AI${view.getInt32(0x260, true)}: ${message}\n`);
            },
        };
        o.aiDropSelection = new RawAiDropSelection(o.aiMemory, c.pool, c.types, directional, o.random, () => c.processState.now, hooks);
        this.aiDropSelectionHooks = hooks;
    }
    // 배치 선택 뒤 같은 지면과 월드 객체를 부족 좌표 기하에 연결한다.
    bindAiDryGeometry() {
        this.#live(); const o = this.#owner, c = o.core;
        if (!this.aiDropSelectionHooks || this.aiDryGeometryHooks || o.aiDryGeometry) throw new Error('Session AI 부족 좌표 기하 설치 순서 오류');
        const globals = {
            get bounds() { return o.aiDropScanBounds; },
            get debug() { return o.gameplayGlobals.aiDebug().read(0x52f3dc); },
            get markerType() { return o.aiDryMarkerType; },
        };
        const hooks = {
            surface: (x, y) => { this.#live(); return c.hash.cell(0, x, y); },
            create: (type, flags) => { this.#live(); return o.world.create(type, flags); },
            word: (sid, value) => { this.#live(); const raw = c.pool.allocated(sid); raw[12] = value & 255; raw[13] = (value >>> 8) & 255; if (raw[10] === c.bridgeState.bridgeType) c.building.notifySurface(sid); },
            pop: (sid, point, flags) => { this.#live(); o.world.pop(sid, point.x, point.y, flags); },
            destroy: (sid, flags) => { this.#live(); o.world.destroy(sid, flags); },
        };
        o.aiDryGeometry = new RawAiDryGeometry(c.pool, globals, hooks);
        this.aiDryGeometryHooks = hooks;
    }
    // 현재 공급 거리와 기하를 같은 AI 부족 좌표 목록에 연결한다.
    bindAiFakeDrySpots() {
        this.#live(); const o = this.#owner, c = o.core;
        if (!this.aiDryGeometryHooks || this.aiFakeDryHooks || o.aiFakeDrySpots) throw new Error('Session AI 부족 좌표 갱신 설치 순서 오류');
        const globals = { manaRadius: () => o.cursorManaGlobals.radiusLimit, mapLimit: () => o.cursorManaGlobals.mapLimit, debug: () => o.gameplayGlobals.aiDebug().read(0x52f3dc) };
        const hooks = {
            score: (token, point, index) => { this.#live(); return o.aiDropEvaluation.dryDistance(token, point, index); },
            center: (sid) => { this.#live(); const point = c.neighbors.center(sid); return { x: point[0], y: point[1] }; },
            candidate: (source, target) => { this.#live(); return o.aiDryGeometry.candidate(source, target); },
            add: (token, point, type) => { this.#live(); return o.aiDrySpots.add(token, point.x, point.y, type); },
            remove: (token, index) => { this.#live(); o.aiDrySpots.remove(token, index); },
            seed: (token) => { this.#live(); o.aiDrySpots.seed(token, o.random); },
            mark: (point) => { this.#live(); o.aiDryGeometry.mark(point); },
            log: (token, action, x, y) => {
                this.#live(); if (!o.services.player.log) throw new Error('AI 부족 좌표 갱신 실제 로그 미연결');
                const raw = o.aiMemory.bytes(token), view = new DataView(raw.buffer, raw.byteOffset, raw.byteLength);
                const text = action === AI_FAKE_DRY_LOG.added ? 'Added fake dry spot at (' : 'Fake Discarded at (';
                o.services.player.log(o.gameplayGlobals.aiDebug().read(0x52f3dc), `AI${view.getInt32(0x260, true)}: ${text}${x},${y})\n\n`);
            },
        };
        o.aiFakeDrySpots = new RawAiFakeDrySpots(o.aiMemory, c.pool, globals, hooks);
        this.aiFakeDryHooks = hooks;
    }
    // 선택과 부족 좌표 준비 뒤 같은 생산·후보 목록으로 배치 행동을 구성한다.
    bindAiDropActions() {
        this.#live(); const o = this.#owner, c = o.core, s = o.state;
        if (!this.aiFakeDryHooks || this.aiDropActionHooks || o.aiDropActions) throw new Error('Session AI 배치 행동 설치 순서 오류');
        const globals = { now: () => c.processState.now, mapLimit: () => o.cursorManaGlobals.mapLimit, checking: () => s.gunState.checking };
        const hooks = {
            product: (token, type) => { this.#live(); return o.aiStrategySetup.findProduct(token, type); },
            select: (token) => { this.#live(); o.aiDropSelection.select(token); },
            scan: (token, type, direction) => { this.#live(); return o.aiDropScan.scan(token, type, direction); },
            canAfford: (token, type) => { this.#live(); return o.aiDeployment.canAfford(token, type); },
            mayPlace: (query) => { this.#live(); o.assetModules.prepareConstruction(); return o.constructionModules.mayPlace(query); },
            deploy: (token, type, point, argument, direction) => { this.#live(); o.aiCollectors.deployWithNotice(token, type, point.x, point.y, argument, direction); },
            fake: (token) => { this.#live(); o.aiFakeDrySpots.extend(token); },
            seed: (token) => { this.#live(); o.aiDrySpots.seed(token, o.random); },
            remove: (token, index) => { this.#live(); o.aiDropScan.remove(token, index); },
            assertion: (text, line) => { this.#live(); if (!o.services.ai.assertion) throw new Error(`AI 배치 행동 원본 진단 미연결 ${line}`); o.services.ai.assertion(text); },
            log: (token, notice) => {
                this.#live(); if (!o.services.player.log) throw new Error('AI 배치 행동 실제 로그 미연결');
                const raw = o.aiMemory.bytes(token), view = new DataView(raw.buffer, raw.byteOffset, raw.byteLength);
                const name = c.types[notice.type]?.name, point = `(${notice.x},${notice.y})`;
                let message;
                switch (notice.action) {
                case AI_DROP_ACTION_LOG.lostAbility: message = `Lost ability to build ${name}\n`; break;
                case AI_DROP_ACTION_LOG.scanComplete: message = `Drop Scan Complete ${name} Rot=${notice.direction}.\n`; break;
                case AI_DROP_ACTION_LOG.partialScan: message = 'Building with scan only partly done.\n'; break;
                case AI_DROP_ACTION_LOG.cannotAfford: message = `Can't afford ${name}\n`; break;
                case AI_DROP_ACTION_LOG.badChoice: message = `Bad choice ${name} at ${point}\n`; break;
                case AI_DROP_ACTION_LOG.laying: message = `Laying ${name} at ${point}\n`; break;
                case AI_DROP_ACTION_LOG.target: message = `My target is the ${notice.type}/${name} at ${point}\n`; break;
                default: throw new RangeError('AI 배치 행동 로그 종류');
                }
                o.services.player.log(o.gameplayGlobals.aiDebug().read(0x52f3d4), `AI${view.getInt32(0x260, true)}: ${message}\n`);
            },
        };
        o.aiDropActions = new RawAiDropActions(o.aiMemory, c.pool, c.types, o.random, globals, hooks);
        this.aiDropActionHooks = hooks;
    }
    // 배치 행동 뒤 현재 사제의 탐색·이동·회수 경계를 연결한다.
    bindAiPriestActions() {
        this.#live(); const o = this.#owner, c = o.core, s = o.state;
        if (!this.aiDropActionHooks || this.aiPriestActionHooks || o.aiPriestActions) throw new Error('Session AI 사제 행동 설치 순서 오류');
        const globals = { now: () => c.processState.now, mapLimit: () => o.cursorManaGlobals.mapLimit, daisType: () => s.runeState.readDaisType() };
        const hooks = {
            scan: (token, budget) => { this.#live(); o.aiBuildSearch.scan(token, budget); },
            vulnerable: (sid) => { this.#live(); return o.aiCollectors.vulnerable(sid); },
            busy: (sid) => { this.#live(); return Boolean(c.processes.findEvent(sid, 0x493)); },
            path: (sid) => { this.#live(); return o.aiMovement.movementForm(sid); },
            maxHitPoints: (sid) => { this.#live(); return o.hp.maxHitPoints(sid); },
            ritual: (sid) => { this.#live(); return aiPriestInRitual(c.pool, c.frames, sid); },
            altarEnemy: (enemy, dais) => { this.#live(); return aiAltarContainsEnemy(c.pool, dais, enemy, s.runeState.readAltarType(), (x, y, type) => o.world.find(x, y, type)); },
            target: (token, actor, target, sacrifice) => { this.#live(); o.aiMovement.toTarget(token, actor, target, sacrifice); },
            distance: (point, sid) => { this.#live(); return aiDistanceToFootprint(point, c.neighbors.footprint(sid)); },
            select: (token) => { this.#live(); return o.aiBuildChoice.select(token); },
            home: (token, sid) => { this.#live(); o.aiMovement.home(token, sid); },
            collect: (token, sid) => { this.#live(); o.aiResourceChoice.collectOrHome(token, sid); },
            deploy: (token, type, point) => { this.#live(); o.aiCollectors.deployWithNotice(token, type, point.x, point.y, 0, 0); },
            begin: (token, request) => { this.#live(); o.aiBuildSearch.initialize(token, request.type, request.bounds, request.avoid, request.owner); },
            point: (token, sid, point) => { this.#live(); o.aiMovement.toPoint(token, sid, point, 0); },
            enemyLowFunds: (token) => { this.#live(); return o.aiBuildChoice.enemyLowFunds(token); },
            salvage: (sid) => { this.#live(); o.salvageCommand.request(sid); },
        };
        o.aiPriestActions = new RawAiPriestActions(o.aiMemory, c.pool, globals, hooks);
        this.aiPriestActionHooks = hooks;
    }
    // 현재 전역과 선택적 호출자 scratch를 빌리는 AI 다리 경로를 만든다.
    aiBridgePath(endpointScratch = null) {
        this.#live(); const o = this.#owner, c = o.core;
        if (!this.aiPriestActionHooks) throw new Error('Session AI 다리 경로 설치 순서 오류');
        const globals = { now: () => c.processState.now, mapLimit: () => o.cursorManaGlobals.mapLimit, bridgeType: () => o.world.storage.links.bridgeType,
            logKind: () => o.gameplayGlobals.aiDebug().read(0x52f3cc), startMarker: () => o.aiBridgeStartType, endMarker: () => o.aiBridgeEndType, targetMarker: () => o.aiBridgeTargetType };
        const hooks = {
            initialize: (token, source, target) => { this.#live(); new RawAiBridgePath(o.aiMemory, c.pool, o.contentFinder, globals, {}).initialize(token, source, target); },
            prepare: (token) => { this.#live(); return o.aiBridgeCandidates.prepare(token); },
            search: (token) => {
                this.#live(); let scratch = endpointScratch;
                if (!scratch) { if (!o.services.ai.bridgeScratchBytes) throw new Error('실제 AI 검색 caller8BYTE 입력 미연결'); const raw = new Uint8Array(8); o.services.ai.bridgeScratchBytes(raw); const view = new DataView(raw.buffer); scratch = { x: view.getFloat32(0, true), y: view.getFloat32(4, true), xBits: view.getUint32(0, true), yBits: view.getUint32(4, true) }; }
                return o.aiBridgeSearch.search(token, scratch);
            },
            start: (token, target) => { this.#live(); return o.aiBridgeCandidates.start(token, target, o.aiDropScanBounds); },
            deploy: (token, type, point, pattern, direction) => { this.#live(); o.aiCollectors.deployWithNotice(token, type, point.x, point.y, pattern, direction); },
            marker: (token, type, point) => { this.#live(); o.aiDeployment.place(token, type, point.x, point.y, 0, 0); },
            technology: (owner) => { this.#live(); return o.aiProductionModules.technology(owner); },
            destroy: (sid, flags) => { this.#live(); o.world.destroy(sid, flags); },
            refresh: (token, parent) => { this.#live(); o.aiStrategySetup.refreshBridges(token, parent); },
            log: (token, kind, message) => {
                this.#live(); if (!o.services.player.log) throw new Error('AI 다리 현재 로그 장치 미연결');
                const raw = o.aiMemory.bytes(token), view = new DataView(raw.buffer, raw.byteOffset, raw.byteLength);
                o.services.player.log(kind, `AI${view.getInt32(0x260, true)}: ${message}\n`);
            },
        };
        return new RawAiBridgePath(o.aiMemory, c.pool, o.contentFinder, globals, hooks);
    }
    // 실제 Host가 먼저 비워진 뒤 지역 함수의 대여를 반납한다. 장기 목록/시계를 재설정하지 않는다.
    dispose() { this.#disposed = true; }
    // 앞부분의 실제 본체 설치 완료이며 전체 ConnectMotion/Load가 아니다.
    get ready() { return this.#ready; }
}
