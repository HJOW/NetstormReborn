// ConnectMotion3845..3888의 현재 Board·운반 후보/대상/회피·재탐색 결선이다(225553df, WB04-E642).
// 사용: 같은 owner의 connectMotionInputs 뒤 bind. 실제 경로 생성은 뒤 Motion 본체 설치를 기다린다.
import { RawBoardTarget } from './raw-board-target.js';
import { CarrierChoiceState, RawCarrierChoice } from './raw-carrier-choice.js';
import { CarrierEscapeState, RawCarrierEscape } from './raw-carrier-escape.js';
import { RawCarrierTarget } from './raw-carrier-target.js';
import { RawPathRetarget } from './raw-path-retarget.js';
import { nuggetTypeNow } from './raw-carrier-arrival.js';
import { captureCampaignFlag } from './raw-campaign-session-state.js';
import { typeNumber } from '../assets/type-table.js';

// 같은 현재 경로/Graph/Player/목록을 빌리는 실제 지역 운반 모듈의 소유자다.
export class RawCampaignSessionCarrierRouting {
    // 반복/부분 설치/반납 관문과 기존 Impl 참조다.
    #owner; #started = false; #ready = false; #disposed = false;
    // 원문 지역값의 수명과 후속 경로가 사용하는 같은 함수 표다.
    choiceState = new CarrierChoiceState(); escapeState = new CarrierEscapeState();
    choiceHooks = null; targetHooks = null; escapeHooks = null; retargetHooks = null;

    // 상태 구성 자체로 장기 Board 실패점/난수/Pool/현재 시각을 재설정하지 않는다.
    constructor(owner) { this.#owner = owner; }
    // 반납한 owner에 남은 함수 값의 재진입을 거부한다.
    #live() { if (this.#disposed || !this.#ready) throw new Error('반납/미연결 Session 운반 경로'); }
    // 순환 참조인 실제 Motion은 호출 때 같은 owner에서 조회한다.
    #motion() { this.#live(); const motion = this.#owner.motion; if (!motion) throw new Error('현재 이동 설치 전 운반 경로 조회'); return motion; }

    // Board 실패점은 Storage 물리를 빌리고 나머지는 원문3845..3888 순서로 한 번 설치한다.
    bind() {
        const o = this.#owner, c = o.core, s = o.state;
        if (this.#disposed || this.#started || !o.motionModules?.inputsReady || o.motion || o.boardTarget || o.carrierChoice) throw new Error('Session 운반 경로의 초기화 전/반복 결선');
        this.#started = true; const e = o.motionModules.effects;
        o.boardTarget = new RawBoardTarget(o.world.storage.boardTargetState, (line) => { this.#live(); throw new Error(`원본 Boardcoord.cpp 대상 접근 진단 ${line}`); });
        const choice = this.choiceHooks = {
            movement: (sid) => {
                this.#live(); const kind = c.pool.slot(sid)[10], definitions = o.assets.game.typeTable.definitions;
                if (kind < 70 || kind - 70 >= definitions.length) return [0, 0];
                const definition = definitions[kind - 70]; return [Math.fround(typeNumber(definition, 'speed') ?? 0), Math.fround(typeNumber(definition, 'turningSpeed') ?? 0)];
            },
            footprint: (sid) => { this.#live(); return c.neighbors.footprint(sid); },
            search: (temporary, parent, source, destination, flags) => { this.#motion().paths.searchCandidate(temporary, parent, source, destination, flags); },
            validReturn: (parent, candidate) => this.#motion().carrierArrival.validReturn(parent, candidate),
            validGeyser: (parent, candidate) => this.#motion().carrierArrival.validGeyser(parent, candidate),
            graphFor: (sid) => { this.#live(); return o.anchor.graphFor(sid); },
            assertion: (line) => { this.#live(); throw new Error(`원본 Carrier.cpp 후보 선택 진단 ${line}`); },
        };
        // RawCheckingFlag의 값형 복사도 현재 포인터/지역 fallback 선택을 보존한다.
        const checking = captureCampaignFlag(s.gunState, 'checking');
        o.carrierChoice = new RawCarrierChoice(c.pool, c.types, o.players, s.assetState.currentOutposts(), s.assetState.currentGeysers(), () => c.processState.now, checking, o.boardTarget, this.choiceState, choice);
        e.carrier.chooseReturn = (sid) => { this.#live(); return o.carrierChoice.chooseReturn(sid); };
        e.carrier.chooseGeyser = (sid) => { this.#live(); return o.carrierChoice.chooseGeyser(sid); };
        e.carrier.currentControlled = (owner) => { this.#live(); return o.participation.brain(owner) !== 0; };
        const target = this.targetHooks = {
            footprint: choice.footprint, start: (request) => this.startMotion(request), chooseReturn: e.carrier.chooseReturn,
        };
        o.carrierTarget = new RawCarrierTarget(c.pool, c.types, o.players, () => s.assetState.readBoss(), o.boardTarget, target);
        e.carrier.goReturn = (sid, place) => { this.#live(); o.carrierTarget.goReturn(sid, place); };
        e.carrier.goGeyser = (sid, geyser) => { this.#live(); o.carrierTarget.goGeyser(sid, geyser); };
        e.carrierGoHome = (sid) => { this.#live(); o.carrierTarget.goHome(sid); };
        const escape = this.escapeHooks = {
            path: (sid) => this.#motion().paths.currentRuntime(sid),
            random: (low, high) => { this.#live(); if (high <= low) throw new RangeError('회피 난수 구간 오류'); return low + (o.random.next(high - low) | 0); },
            start: (sid, destination) => this.startMotion({ parent: sid, destination, replaceStart: 0, helper: 0, tailWord: 0, tailByte: 0, reserved: 0 })?.runtime ?? null,
            blocker: (point) => e.carrier.genusAt(point.x, point.y, 0x10000),
            search: choice.search, movement: choice.movement,
            valid: (point) => { this.#live(); return o.carrierFalling.validPoint(point.x, point.y); },
            assertion: (line) => { this.#live(); throw new Error(`원본 Carrier.cpp 회피 깊이 진단 ${line}`); },
        };
        o.carrierEscape = new RawCarrierEscape(c.pool, c.hash, c.types, c.spots, () => c.processState.now, checking, this.choiceState.cardinalTypes, this.escapeState, escape);
        o.services.motion.carrier.escape = (sid, distance, event) => { this.#live(); return o.carrierEscape.run(sid, distance, event); };
        e.carrier.escape = o.services.motion.carrier.escape;
        const retarget = this.retargetHooks = {
            find: (sid, mask, type, index) => { this.#live(); return o.priests.carrier.findContained(sid, mask, type, index); },
            chooseReturn: e.carrier.chooseReturn, chooseGeyser: e.carrier.chooseGeyser, goReturn: e.carrier.goReturn, goGeyser: e.carrier.goGeyser,
        };
        o.pathRetarget = new RawPathRetarget(c.pool, () => nuggetTypeNow(s.motionState.carrier), retarget);
        e.path.retarget = (runtime, initialParent) => { this.#live(); o.pathRetarget.update(runtime, initialParent); };
        this.#ready = true;
    }

    // 3012..3019의 실제 종류별489080 시작을 같은 경로에 연결한다. AI 본체와는 독립인 선행이다.
    startMotion(request) {
        const motion = this.#motion(), o = this.#owner, kind = o.core.pool.slot(request.parent)[10], movement = o.priestState.movement;
        if (kind === movement.priest) return motion.paths.startPriest(request);
        if ([133, 111, 141, 146, 135, 137, 156].includes(kind)) return motion.paths.startCarrier(request);
        if (kind === movement.teleport || kind === movement.manaBolt) return motion.paths.startConstruction(request);
        throw new Error('다른 실제 운반 경로 미복원 경계');
    }
    // 부모가 실제 Host를 비운 뒤 함수 대여만 반납하고 장기 Board 실패점/Player는 보존한다.
    dispose() { this.#disposed = true; }
    // 이 구간의 설치 완료이며 전체 이동/Load 성공은 아니다.
    get ready() { return this.#ready; }
}
