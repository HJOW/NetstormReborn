// 같은 실제 World의 사제/Carrier/Graph/공간/현재 Path로 Halt·Arrival·cleanup을 조립한다(cpppj client/RawCampaignMotionBindings, 225553df).
// 사용: 같은 clock의 Start/Process 상태와 실제 외부 효과를 공급하고 paths로 명령/현재 경로를 조회한다. (2026-11-16 WB04-E568)
import { RawCampaignPathBindings } from './raw-campaign-path-bindings.js';
import { RawCanonPlacementTerrain, createCanonPlacementTerrainState } from './raw-canon-placement-terrain.js';
import { RawPathCleanup, createPathCleanupState } from './raw-path-cleanup.js';
import { RawPriestHalt, PriestHaltState } from './raw-priest-halt.js';
import { RawCarrierArrival, createCarrierArrivalState } from './raw-carrier-arrival.js';
import { RawPriestArrival, createPriestArrivalState } from './raw-priest-arrival.js';
import { RawContainedLookup } from './raw-contained-lookup.js';
import { RawPathSearchState } from './raw-path-search.js';

// 상위 캠페인이 소유하는 현재 모드/타입/동맹/BFS 전역이다. 하위는 이 상태를 복사하지 않고 읽는다.
export class RawCampaignMotionState {
    // 중립 도착의 실제 지면/지역 상태와 부모 Halt의 현재 권한이다.
    terrain = createCanonPlacementTerrainState();
    halt = new PriestHaltState();
    // 실제 Carrier/사제 도착과 공간 이후 정리 상태이다.
    carrier = createCarrierArrivalState();
    priest = createPriestArrivalState();
    cleanup = createPathCleanupState();
    // 같은 그래프/정책/거리표 검색이 소비하는 현재 상태이다.
    search = new RawPathSearchState();
}

// 큰 운반/포획/의식/경제 효과는 입력 장치로 남기고 같은 완료 몸체와 현재 Path의 순환 참조만 닫는다.
export class RawCampaignMotionBindings {
    // 같은 실제 Core와 서로를 참조하는 도착/정리/Path 처리기이다.
    #core; #terrain; #cleanup; #halt; #carrier; #arrival; #paths = null; #lookup;

    // 첫 명령 전에 Pool/장부/같은 시계를 검증하고 실제 경로 생성기는 마지막에 구성한다.
    constructor(world, priests, islands, contained, anchor, hp, frames, state, start, process, effects = {}) {
        // 같은 Pool/배열/Host/시계를 빌리는 현재 Core이다.
        const core = world.core(); this.#core = core;
        if ([priests.priestState, islands, contained, anchor, hp].some((body) => body.pool !== core.pool) || hp.bookkeeping !== core.bookkeeping || frames.length !== core.frames.length || start.clock !== core.processState || process.clock !== core.processState) throw new TypeError('캠페인 이동의 동일 Pool/장부/현재시계/프레임 불일치');
        // C++ 값 인수의 각 함수 묶음을 복사하되 함수가 읽는 실제 상태/장치는 공유한다.
        const hooks = { ...effects, cleanup: { ...effects.cleanup }, halt: { ...effects.halt }, carrier: { ...effects.carrier }, priest: { ...effects.priest }, path: { ...effects.path } };
        // 선택 시 명시 거부할 배치 전용 효과이다.
        const unavailable = (name) => () => { throw new Error(`캠페인 도착 getter의 배치 ${name} 미연결`); };
        this.#terrain = new RawCanonPlacementTerrain(core.pool, core.types, frames, core.hash.view(0), state.terrain, { candidatePermission: unavailable('권한'), shapePermission: unavailable('Shape'), finishRelations: unavailable('Finish') });
        // 의식/환불/할당은 외부 경계이며 내용물/Graph/예약/삭제는 같은 실제 모듈이다.
        this.#cleanup = new RawPathCleanup(core.pool, core.hash, core.types, state.cleanup, { ...hooks.cleanup,
            contained: (sid, mask, type, index) => priests.carrier.findContained(sid, mask, type, index), graphFor: (sid) => anchor.graphFor(sid),
            addShared: (sid, event, payload) => { core.processes.addSharedRegular(sid, event, Math.fround(payload)); }, destroy: (sid, flags) => world.destroy(sid, flags),
        });
        this.#halt = new RawPriestHalt(core.pool, core.hash, core.types, core.spots, priests.carrierCheck, priests.priestState, hp, state.halt, { ...hooks.halt,
            unpop: (sid, flags) => world.unpop(sid, flags), pop: (sid, x, y, flags) => world.pop(sid, Math.fround(x), Math.fround(y), flags),
        });
        // Carrier의 현재 Path 조회는 생성기 구성 뒤 실제 종속 체인을 매번 다시 읽는다.
        this.#carrier = new RawCarrierArrival(core.pool, core.hash, core.types, priests.carrier, this.#cleanup, priests.priestState, anchor, contained, core.neighbors, state.carrier, { ...hooks.carrier,
            path: (sid) => { if (!this.#paths) throw new Error('캠페인 Path 구성 전 도착 요청'); return this.#paths.currentRuntime(sid); }, destroy: (sid, flags) => world.destroy(sid, flags),
        });
        this.#arrival = new RawPriestArrival(core.pool, priests.priestState, hp, this.#terrain, islands, core.neighbors, contained, this.#carrier, state.priest, { ...hooks.priest, destroy: (sid, flags) => world.destroy(sid, flags) });
        this.#lookup = new RawContainedLookup(core.pool, (sid, flags) => world.destroy(sid, flags));
        hooks.path.carrierArrival = (sid, first, second) => this.#carrier.arrival(sid, first, second);
        this.#paths = new RawCampaignPathBindings(world, core, priests, this.#halt, this.#arrival, this.#cleanup, this.#lookup, start, process, state.search, hooks.path);
    }

    // 후속 모듈도 같은 실제 이동/도착/정리/지면 몸체를 빌린다.
    get paths() { return this.#paths; }
    get carrierArrival() { return this.#carrier; }
    get cleanup() { return this.#cleanup; }
    get priestArrival() { return this.#arrival; }
    get terrain() { return this.#terrain; }
    get pool() { return this.#core.pool; }
    // empty 주기 wrapper도 원문의 같은 공통 부모 Halt를 재사용한다.
    commonHalt(parent) { return this.#halt.commonHalt(parent); }
}
