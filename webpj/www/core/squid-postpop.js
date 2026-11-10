// 공통 후처리(postPop) 처리기 `SquidPostPop` 의 가상 분배·사전 검사·공통 장부 연결을 옮긴 모듈이다(cpppj `o/SquidPostPop.cpp/.h` + `o/SquidPostPopPatch.cpp`, 10.78 전용).
// 역할:
//  - 분배(dispatchPostPop): 공간 Pop 이 성공한 뒤 Activate 부모(raw-squid-activate.js)가 깊이를 올리고 현재 가상 +20 으로 부르는 `postPopVirtual(sid, flags)` 가 실제 가상 표(vtable)로 파생 몸체를 고른다.
//    파생 몸체(벌룬·비행·Player·buried·Dude·다이스·Rune·사제·크랩·Carrier/Damageable·자산·Factory)는 자기 몸체 안에서 공통 장부를 한 번 부르고, 접두만 가진 종류(주문 효과·다리·섬·표면·보호막·Teleport)는
//    접두를 실행한 뒤 이 처리기의 공통 몸체로 내려간다. 파생 몸체가 아직 연결되지 않은 종류는 지원한다고 가장하지 않고 거부한다.
//  - 공통 몸체(postPop/postPopBase): `runSquidPostPopParent`(004b0d30)에 현재 TYPE·Graph 영역/Add·진단·비용·공급원·공장·AI 통지·통계·선택 재검사 장치를 연결한다. 효과 사이에서 모든 값을 다시 읽는다.
//  - 사전 검사(validate/validateBase): 독립 명시 호출과 파생 몸체의 직접 호출용 무효과 검사이다. 실제 10.78 가상 부모는 가상 몸체 앞에 전체 사전 검사를 끼우지 않는다.
//  - 비용(totalCost): 10.78 은 로딩 때의 정수 비용표 `trunc(cost + type*23) - type*23` 에 현재 전역 비용을 더해 32비트로 감는다.
// 파생 몸체 인터페이스(아직 옮기지 않은 종류가 후속 단위에서 연결한다): { pool, handles(sid) → boolean, validate(sid, flags, pop), postPop(sid, flags) } 이고
//  접두형은 prefix(sid, flags), Carrier 는 damageablePostPop(sid, flags) 도 가진다. 접두 함수: setIslandPrefix/setSurfacePrefix/setTeleportPrefix((sid, flags) => void), 다리 연결 장치 bridgeConnector((sid) => void).
// 이 모듈은 DOM 을 쓰지 않는다. CD 10.72 판본 분기(검증 호출 순서·28/32/35 오프셋)는 옮기지 않았다. (2026-11-08 WB04-E41: SquidPostPop 처리기 본체 이식으로 추가)
import { RawAssetPostPop } from './raw-asset-post-pop.js';
import { registerManaProvider } from './raw-mana-registration.js';
import { SquidPop } from './squid-pop.js';
import { runSquidPostPopParent } from './squid-postpop-parent.js';

// 기본 장치가 연결된 장기 Client 에서 현재 파생 몸체가 없을 때 장기 자원 후처리(defaultResourcePostPop)로 보내는 가상 표이다: 벌룬 그림자·Factory 둘·0x5040c0·0x504198·buried·0x50e888·0x509470
// (그리고 자산 전용 표와 Carrier/Damageable 직접 표). 이 표의 몸체가 미설치면 공통 성공으로 바꾸지 않고 실패한다. (2026-11-10 WB04-E258: cpppj 기준선 뒤 DispatchPostPop 선택 분기 재반영)
const DEFAULT_RESOURCE_TABLES = new Set([0x508cc8, 0x509148, 0x509210, 0x5040c0, 0x504198, 0x50a748, 0x50e888, 0x509470]);

// 슬롯 필드 위치(타입·상태·소유자·extra·그래프 번호)와 상태/extra 비트, 첫 자산 타입 번호, 타입 수이다. (2026-11-08 WB04-E41)
const TYPE_OFFSET = 10;
const STATE_OFFSET = 11;
const GRAPH_OFFSET = 30;
const OWNER_OFFSET = 34;
const EXTRA_OFFSET = 40;
const FREE = 1;
const CONTAINED = 8;
const ABSTRACT_BURIED = 9;
const FIRST_ASSET_TYPE = 70;
const TYPE_COUNT = 188;
const PLAYER_COUNT = 8;

// 공통 후처리 분기 마스크다: 영역 군 genus, 공장 군 genus, 공급원 flags1, 그래프 억제/변경/파괴 flags. (2026-11-08 WB04-E41)
const REGION_MASK = 0x50444208;
const FACTORY_MASK = 0x4200;
const PROVIDER = 0x10000000;
const NO_GRAPH = 0x2000000;
const GRAPH_CHANGE = 0x203;
const DESTROY_GRAPH = 0x800;
const SURFACE = 0x800;

// 패치 정수 비용표가 타입 번호마다 더해 저장하는 정수이다. (2026-11-08 WB04-E41)
const COST_BIAS = 23;

// 10.78 실제 가상 표 주소이다: 다리·섬 받침·noIsland 표면·보호막·사제·Teleport, 그리고 직접 Carrier/Damageable 몸체를 쓰는 표. (2026-11-08 WB04-E41)
export const POSTPOP_VTABLE = Object.freeze({
    bridge: 0x005034c8,
    island: 0x00506d98,
    surface: 0x00506f08,
    forcefield: 0x00507770,
    priest: 0x0050f210,
    teleport: 0x005070c0,
    carrier: Object.freeze([0x00503d80, 0x00503e50, 0x0050e568]),
    damageable: Object.freeze([0x0050a820, 0x0050a8e8]),
});

// 파생 몸체를 등록하는 종류 이름이다. 등록 이름은 setBody(kind, body) 에 쓴다. (2026-11-08 WB04-E41)
const BODY_KINDS = Object.freeze(['balloon', 'flyer', 'player', 'buried', 'dude', 'dais', 'rune', 'priest', 'carrierSteal', 'carrier', 'asset', 'factory', 'bomb', 'forcefield']);

// 슬롯 첫 DWORD(가상 표 주소)를 읽는다. (2026-11-08 WB04-E41)
function vtableOf(raw) {
    return (raw[0] | (raw[1] << 8) | (raw[2] << 16) | (raw[3] << 24)) >>> 0;
}

// 신호 없는 선택 장치를 꺼낸다. 없으면 도달한 위치에서 오류이다. (2026-11-08 WB04-E41)
function need(device, message) {
    if (typeof device !== 'function') throw new Error(message);
    return device;
}

// 다리 postPop 의 접두(00422150): flags 비트 1 은 extra 비트 2 를 켜고, 비트 1 이거나 (비트 4 이고 abstract 가 아님)이면 연결 효과(004213b0)를 요청한다.
// 원문의 첫 저장은 연결 장치 요구보다 앞이며 free/contained 를 추가 검사하지 않는다. 사용: bridgePrefix(pool, sid, flags, connector). (2026-11-08 WB04-E41)
export function bridgePrefix(pool, sid, flags, connector) {
    if (flags & 1) pool.physical(sid)[EXTRA_OFFSET] |= 2;
    else if (!(flags & 4) || (pool.slot(sid)[EXTRA_OFFSET] & 1)) return;
    // 콜백이 자기 연결을 해제해도 실행 중 callable 의 수명을 보존한다.
    const selected = connector;
    if (!selected) throw new Error('다리 postPop 선택 연결 효과 미연결');
    selected(sid);
}

// 공통 후처리 처리기이다. 사용:
//   const post = new SquidPostPop(pool, types, state, graph);   // state = SquidPostPopState, graph = RawGraph(선택)
//   const pop = new SquidPop(pool, hash, spots, { postPop: post });   // Pop 이 postPopVirtual/activationDepth/validateSpace 를 쓴다
// types 는 타입 188개 { flags1, flags2, footX, footY, cost } 이며 값으로 복사한다. 상태/Graph/풀의 수명은 이 객체보다 길어야 한다. (2026-11-08 WB04-E41)
export class SquidPostPop {
    #pool;
    #types;
    #state;
    #graph;
    #bridgeConnector;
    #currentTypes = null;
    #loadedCostTypes = null;
    #bodies = {};
    #islandPrefix = null;
    #surfacePrefix = null;
    #teleportPrefix = null;
    #aiNotice = null;
    #selectionRecheck = null;
    #localOwner = null;
    #manaDiagnostics = {};
    #defaultDevices = null;
    #defaultResourcePostPop = null;

    // 판본·크기와 Graph 의 풀/타입 일치를 확인한다.
    constructor(pool, types, state, graph = null, bridgeConnector = null) {
        if (pool?.edition !== 'patch1078') throw new TypeError('postPop 은 10.78 SID 풀만 받습니다.');
        if (types.length !== TYPE_COUNT) throw new RangeError('postPop 타입 판본/크기 오류');
        if (graph && graph.pool !== pool) throw new TypeError('postPop/Graph SID 풀이 다릅니다');
        this.#pool = pool;
        this.#types = Array.from(types, (type) => ({ ...type }));
        this.#state = state;
        this.#graph = graph;
        this.#bridgeConnector = bridgeConnector;
        if (graph) graph.validateTypes(types);
    }

    // Pop 과 다른 풀을 연결하지 않도록 소유자를 제공한다.
    get pool() { return this.#pool; }

    // 실제 Activate 부모와 파생 몸체가 공유하는 현재 깊이(005c8488)를 읽고 쓰는 소유자이다. 사본 깊이를 만들지 않는다.
    get activationDepth() {
        const state = this.#state;
        return { get value() { return state.depth; }, set value(next) { state.depth = next >>> 0; } };
    }

    // 현재 월드의 188 TYPE 배열을 빌린다(null 이면 독립 소유 사본). 실제 필드는 복사하거나 덮어쓰지 않는다.
    bindCurrentTypes(types) {
        if (types && types.length !== 0 && types.length !== TYPE_COUNT) throw new RangeError('postPop 현재 TYPE 대여의 길이 오류');
        this.#currentTypes = types && types.length !== 0 ? types : null;
    }

    // 로딩 때의 정수 비용표를 빌린다(현재 TYPE 의 플래그/발자국 대여와 분리). null 은 생성 시 사본의 비용으로 되돌린다.
    bindLoadedCosts(types) {
        if (types && types.length !== 0 && types.length !== TYPE_COUNT) throw new RangeError('postPop 로딩 비용표 대여의 길이 오류');
        this.#loadedCostTypes = types && types.length !== 0 ? types : null;
    }

    // 004166e0 의 실제 현재 Player/brain 통지다: (owner, sid, 생성 여부) => void. 빈 값은 분리한다.
    bindAiNotice(notice) { this.#aiNotice = notice; }

    // 장기 기본 장치(실제 Client 자료/원문 큰 하위)를 결합한다: { aiNotice(owner, sid, created), localOwner(), recheckSelection() } 셋 다 필요하며 하나라도 없으면 거부한다.
    // 현재 Session 이 개별로 연결한 통지/로컬 owner/선택 재검사가 있으면 그것이 먼저이고, 없을 때 이 기본 장치를 쓴다. 선택된 누락을 빈 성공으로 대체하지 않는다.
    // (2026-11-10 WB04-E258: cpppj 기준선 뒤 BindDefaultPatchDevices 재반영)
    bindDefaultPatchDevices(devices) {
        if (typeof devices?.aiNotice !== 'function' || typeof devices.localOwner !== 'function' || typeof devices.recheckSelection !== 'function') throw new TypeError('기본 공통 후처리의 실제 Client 장치 누락');
        this.#defaultDevices = { aiNotice: devices.aiNotice, localOwner: devices.localOwner, recheckSelection: devices.recheckSelection };
    }

    // 현재 Session 파생이 없을 때만 같은 장기 Storage 의 실제 자원 가상 몸체를 선택하는 분배를 결합한다: (sid, flags) => void. 함수가 아니면 거부한다. (2026-11-10 WB04-E258)
    bindDefaultResourcePostPop(postPop) {
        if (typeof postPop !== 'function') throw new TypeError('장기 자원 가상 PostPop 의 실제 PATCH 장치 필요');
        this.#defaultResourcePostPop = postPop;
    }

    // 현재 World/Session 의 파생 몸체 열네 종과 AI 통지/로컬 owner/선택 재검사/Teleport 접두 연결만 반납한다. 장기 장부·섬/표면 접두·기본 장치·기본 자원 후처리·Graph 는 보존한다(Native 전역 reset 이 아니다). (2026-11-10 WB04-E258)
    releaseRuntimeBindings() {
        this.#aiNotice = null;
        this.#selectionRecheck = null;
        this.#localOwner = null;
        this.#teleportPrefix = null;
        this.#bodies = {};
    }

    // 같은 Client 의 checking/마나 중복/공통 0x80b 정상 복귀 진단 장치를 빌린다: { checking(), assertion(식, 파일, 줄) }.
    bindManaDiagnostics(diagnostics) { this.#manaDiagnostics = diagnostics ?? {}; }

    // 장부 뒤 현재 마우스 선택 재검사이며 Session 종료 때 분리한다.
    bindSelectionRecheck(recheck) { this.#selectionRecheck = recheck; }

    // AI 통지 뒤 현재 Client 로컬 owner 를 읽는 getter 이다. 독립 구성은 state.localOwner 를 쓴다.
    bindLocalOwner(owner) { this.#localOwner = owner; }

    // 섬 받침(004421c0) 접두를 연결한다. 연결 계층이 Pop 을 거쳐 이 객체를 다시 부르므로 생성 뒤에 공급한다.
    setIslandPrefix(prefix) { this.#islandPrefix = prefix; }

    // noIsland(004423b0) 접두를 연결한다.
    setSurfacePrefix(prefix) { this.#surfacePrefix = prefix; }

    // 건설 마나 Teleport(0x5070c0)의 최초 Pop 접두(4429f0)를 연결한다. flags 비트 1 에서만 호출한다.
    setTeleportPrefix(prefix) { this.#teleportPrefix = prefix; }

    // 파생 몸체를 같은 풀에 등록한다. body 가 null 이면 해제한다. 종류는 BODY_KINDS 중 하나이다.
    setBody(kind, body) {
        if (!BODY_KINDS.includes(kind)) throw new RangeError(`postPop 파생 몸체 종류 오류: ${kind}`);
        if (body && body.pool !== this.#pool) throw new TypeError(`${kind} Pop/후처리 풀 불일치`);
        this.#bodies[kind] = body ?? null;
    }

    // 현재 타입 목록(월드가 빌려준 것 또는 독립 소유 사본)이다.
    #currentTypeList() { return this.#currentTypes ?? this.#types; }

    // 등록 몸체가 해당 SID 를 처리하는지 묻는다.
    #handlesBody(kind, sid) { return Boolean(this.#bodies[kind]?.handles(sid)); }

    // 다리 VT 이다. 10.78 은 연결 장치 설치 여부와 무관하게 실제 VT 로 분배한다.
    handlesBridge(sid) { return vtableOf(this.#pool.slot(sid)) === POSTPOP_VTABLE.bridge; }

    // 섬 받침 VT 이다.
    handlesIsland(sid) { return vtableOf(this.#pool.slot(sid)) === POSTPOP_VTABLE.island; }

    // noIsland 표면 VT 이다.
    handlesSurface(sid) { return vtableOf(this.#pool.slot(sid)) === POSTPOP_VTABLE.surface; }

    // 사제는 실제 VT 와 등록 몸체가 있을 때만 분배하며 현재 TYPE 을 추가 관문으로 만들지 않는다.
    handlesPriest(sid) { return Boolean(this.#bodies.priest) && vtableOf(this.#pool.slot(sid)) === POSTPOP_VTABLE.priest; }

    // 보호막도 실제 VT 와 등록 접두가 있을 때만 분배한다.
    handlesForcefield(sid) { return Boolean(this.#bodies.forcefield) && vtableOf(this.#pool.slot(sid)) === POSTPOP_VTABLE.forcefield; }

    // 직접 Carrier 몸체의 VT 이며 전체 몸체가 연결됐는가.
    handlesCarrier(sid) { return Boolean(this.#bodies.carrier) && POSTPOP_VTABLE.carrier.includes(vtableOf(this.#pool.slot(sid))); }

    // 직접 Damageable 몸체의 VT 이며 전체 몸체가 연결됐는가.
    handlesDamageable(sid) { return Boolean(this.#bodies.carrier) && POSTPOP_VTABLE.damageable.includes(vtableOf(this.#pool.slot(sid))); }

    // 한 SID 가 어떤 파생 몸체로든 분배되는지 확인한다(CD 사전 검사와 Validate 가 쓴다).
    handlesDerived(sid) {
        return this.#handlesBody('bomb', sid) || this.#handlesBody('carrierSteal', sid) || this.handlesBridge(sid) || this.handlesIsland(sid) || this.handlesSurface(sid) ||
            this.handlesPriest(sid) || this.handlesForcefield(sid) || this.handlesCarrier(sid) || this.handlesDamageable(sid) || this.#handlesBody('asset', sid) ||
            this.#handlesBody('factory', sid) || this.#handlesBody('rune', sid) || this.#handlesBody('dais', sid) || this.#handlesBody('dude', sid) ||
            this.#handlesBody('player', sid) || this.#handlesBody('buried', sid) || this.#handlesBody('flyer', sid) || this.#handlesBody('balloon', sid);
    }

    // 현재 TYPE 번호가 범위 안인 레코드이다.
    #type(number) {
        const types = this.#currentTypeList();
        if (number >= types.length) throw new RangeError('공통후처리 현재 TYPE 범위 오류');
        return types[number];
    }

    // 로딩 정수 비용표 기준의 누적 비용(signed 32비트 감김)이다: trunc(cost + type*23) - type*23 에 현재 전역 비용을 더한다.
    totalCost(number) {
        const costs = this.#loadedCostTypes ?? this.#types;
        if (number >= costs.length) throw new RangeError('postPop 로딩 비용표 번호 범위 오류');
        const cost = Math.fround(costs[number].cost ?? 0);
        if (!Number.isFinite(cost)) throw new TypeError('postPop 유한 비용이 필요합니다');
        const bias = number * COST_BIAS;
        const encoded = cost + bias;
        if (encoded < -2147483648 || encoded > 2147483647) throw new RangeError('postPop 패치 비용 표 범위 오류');
        const value = ((Math.trunc(encoded) - bias) | 0) + this.#state.totalCost;
        // 정상 비용 누적의 signed DWORD 경계 통과는 원본 ftol 반환의 low DWORD 로 보존한다.
        if (value < -4294967296 || value > 4294967295) throw new RangeError('postPop 비용 누적 범위 오류');
        return Math.trunc(value) | 0;
    }

    // 명시 무효과 사전 검사(독립 호출/파생 몸체용): 슬롯 상태·연결 장치·지원 VT·등록 몸체의 검사를 순서대로 하고 마지막에 공통 검사를 한다.
    validate(sid, flags, pop = null) {
        const bytes = this.#pool.slot(sid);
        if (sid < 5 || (bytes[STATE_OFFSET] & (FREE | CONTAINED)) || bytes[TYPE_OFFSET] < FIRST_ASSET_TYPE || bytes[TYPE_OFFSET] >= this.#currentTypeList().length) throw new Error('postPop raw 자산 상태 오류');
        if (this.handlesBridge(sid) && !this.#bridgeConnector && ((flags & 1) || ((flags & 4) && !(bytes[EXTRA_OFFSET] & 1)))) throw new Error('명시 다리 postPop 사전 검사의 연결 효과 미연결');
        if ((flags & 1) && ((this.handlesIsland(sid) && !this.#islandPrefix) || (this.handlesSurface(sid) && !this.#surfacePrefix))) throw new Error('명시 섬/표면 postPop 사전 검사의 접두 미연결');
        if (!SquidPop.supports(vtableOf(bytes), flags) && !this.handlesDerived(sid)) throw new Error('파생 postPop 미복원');
        // 등록된 몸체의 자기 사전 검사를 C++ 와 같은 순서로 호출한다.
        for (const [kind, matches] of [
            ['priest', this.handlesPriest(sid)], ['rune', this.#handlesBody('rune', sid)], ['dais', this.#handlesBody('dais', sid)], ['dude', this.#handlesBody('dude', sid)],
            ['player', this.#handlesBody('player', sid)], ['buried', this.#handlesBody('buried', sid)], ['balloon', this.#handlesBody('balloon', sid)],
            ['forcefield', this.handlesForcefield(sid)], ['asset', this.#handlesBody('asset', sid)], ['factory', this.#handlesBody('factory', sid)],
        ]) {
            if (matches) this.#bodies[kind].validate(sid, flags, pop);
        }
        this.validateBase(sid, flags, pop);
    }

    // 파생 몸체가 공통 몸체를 직접 부르기 전의 검사다(가상 표의 postPop 지원 여부는 제외). 억제면 아무것도 읽지 않는다.
    validateBase(sid, flags, pop = null) {
        const bytes = this.#pool.slot(sid);
        const state = this.#state;
        if (sid < 5 || (bytes[STATE_OFFSET] & (FREE | CONTAINED)) || bytes[TYPE_OFFSET] < FIRST_ASSET_TYPE || bytes[TYPE_OFFSET] >= this.#currentTypeList().length) throw new Error('postPop raw 자산 상태 오류');
        if (state.suppressed) return;
        const type = this.#currentTypeList()[bytes[TYPE_OFFSET]];
        if (state.graphsEnabled) {
            const region = Boolean((type.flags2 & REGION_MASK) && (flags & 3) && !(flags & NO_GRAPH));
            const add = Boolean((type.flags1 & SURFACE) && (flags & GRAPH_CHANGE) && !(flags & NO_GRAPH));
            if ((flags & GRAPH_CHANGE) && !(flags & NO_GRAPH) && (flags & DESTROY_GRAPH)) throw new Error('postPop destroyGraph 원본 assert 경로');
            if ((region || add) && !this.#graph) throw new Error('postPop 표면/영역 그래프가 연결되지 않았습니다');
            if (this.#graph) this.#graph.validatePostPop(sid, region, add, pop);
        }
        if (!(flags & 1) || (bytes[EXTRA_OFFSET] & ABSTRACT_BURIED)) return;
        const owner = bytes[OWNER_OFFSET];
        if (owner > PLAYER_COUNT) throw new RangeError('postPop 소유자 범위 오류');
        if (!this.#aiNotice && owner && state.aiAttached[owner]) throw new Error('postPop AI 생성 통지 미복원');
        if (state.pendingPlacement) throw new Error('postPop 배치 선택 해제 미복원');
        this.totalCost(bytes[TYPE_OFFSET]);
        if (type.flags1 & PROVIDER) state.currentProviders().items();
        if (type.flags2 & FACTORY_MASK) {
            state.currentFactories().items();
            if (owner) state.ownerFactories[owner].items();
        }
    }

    // 독립 기존 fixture 의 후처리 호환 진입이다: 사전 검사 → 깊이 증가 → 분배 → 깊이 복귀 확인. 실제 PATCH Pop 은 별도 Activate 부모가 깊이와 진단을 맡는다.
    activate(sid, flags) {
        this.validate(sid, flags);
        const before = this.#state.depth;
        this.#state.depth = (this.#state.depth + 1) >>> 0;
        this.#dispatchPostPop(sid, flags);
        if (this.#state.depth !== before) throw new Error('독립 후처리의 공통 깊이 불일치');
    }

    // 실제 Activate 부모가 깊이를 올린 뒤 현재 가상 +20 으로 부르는 진입이다. 10.78 부모는 가상 몸체 앞에 전체 공통 사전 검사를 삽입하지 않는다.
    postPopVirtual(sid, flags) { this.#dispatchPostPop(sid, flags); }

    // 실제 Pop 이 같은 풀과 해시/spot 을 읽는지 확인한다(다른 지도에 Graph 를 갱신하는 연결을 막는다).
    validateSpace(hash, spots) {
        if (this.#graph) this.#graph.validateSpace(hash, spots);
    }

    // 선택된 실제 파생 또는 공통 몸체를 호출한다. 깊이는 호출자가 소유하며 이 분배는 올리지 않는다.
    #dispatchPostPop(sid, flags) {
        const bodies = this.#bodies;
        // 기본 장치가 연결된 장기 Client: 현재 파생 몸체가 없는 자원 가상 표는 장기 자원 후처리로 보낸다(미설치면 이 위치에서 실패).
        const table = vtableOf(this.#pool.slot(sid));
        if (this.#defaultDevices?.aiNotice && !this.handlesDerived(sid) &&
            (DEFAULT_RESOURCE_TABLES.has(table) || RawAssetPostPop.hasDedicatedVtable(table) || POSTPOP_VTABLE.carrier.includes(table) || POSTPOP_VTABLE.damageable.includes(table))) {
            const resource = this.#defaultResourcePostPop;
            if (!resource) throw new Error('Native 전용 자산 후처리의 실제 장기 모듈 미설치');
            resource(sid, flags);
            return;
        }
        // 몸체 안에서 공통 장부를 한 번 호출하는 종류를 원문 순서대로 고른다.
        for (const kind of ['balloon', 'flyer', 'player', 'buried', 'dude', 'dais', 'rune']) {
            if (this.#handlesBody(kind, sid)) { bodies[kind].postPop(sid, flags); return; }
        }
        if (this.handlesPriest(sid)) { bodies.priest.postPop(sid, flags); return; }
        if (this.#handlesBody('carrierSteal', sid)) { bodies.carrierSteal.postPop(sid, flags); return; }
        // 두 직접 몸체도 같은 flags 의 공통 장부를 내부에서 한 번 호출한다.
        if (this.handlesCarrier(sid) || this.handlesDamageable(sid)) {
            if (this.handlesCarrier(sid)) bodies.carrier.postPop(sid, flags);
            else bodies.carrier.damageablePostPop(sid, flags);
            return;
        }
        if (this.#handlesBody('asset', sid)) { bodies.asset.postPop(sid, flags); return; }
        if (this.#handlesBody('factory', sid)) { bodies.factory.postPop(sid, flags); return; }
        const raw = this.#pool.slot(sid);
        // 접두만 가진 종류는 접두를 실행한 뒤 아래 공통 몸체로 내려간다.
        if (this.#handlesBody('bomb', sid)) bodies.bomb.prefix(sid, flags);
        else if (this.handlesBridge(sid)) bridgePrefix(this.#pool, sid, flags, this.#bridgeConnector);
        else if (this.handlesIsland(sid)) {
            if (flags & 1) need(this.#islandPrefix, '섬 선택 접두 미연결')(sid, flags);
        } else if (this.handlesSurface(sid)) {
            if (flags & 1) need(this.#surfacePrefix, '표면 선택 접두 미연결')(sid, flags);
        } else if (this.handlesForcefield(sid)) bodies.forcefield.prefix(sid, flags);
        else if (vtableOf(raw) === POSTPOP_VTABLE.teleport) {
            if (flags & 1) need(this.#teleportPrefix, 'Teleport 선택 생성 접두 미연결')(sid, flags);
        } else if (!SquidPop.supports(vtableOf(raw), flags)) throw new Error('현재 가상 후처리 몸체 미복원');
        this.postPop(sid, flags);
    }

    // 공통 후처리(004b0d30)의 순서: 영역 무효화 → 그래프 생성/리셋 → 비용 → 공급/작업장 목록 → 소유자 조건 통계 → 선택 재검사 → 깊이 감소. 하위 실패 전의 효과는 되돌리지 않는다.
    postPop(sid, flags) {
        const state = this.#state;
        const pool = this.#pool;
        const diagnostics = this.#manaDiagnostics;
        const graph = this.#graph;
        const hooks = {
            suppressed: () => state.readSuppressed(),
            graphsEnabled: () => state.graphsEnabled,
            type: (number) => this.#type(number),
            region: (object) => { if (!graph) throw new Error('실제 후처리 영역 그래프 미연결'); graph.invalidateRegion(object); },
            add: (object) => { if (!graph) throw new Error('실제 후처리 Add 그래프 미연결'); graph.add(object); },
            assertion: (expression, file, line) => need(diagnostics.assertion, '공통 후처리 실제 진단 미연결')(expression, file, line),
            // 독립 미연결 Graph 의 기존 유효 표면 번호 쓰기 계약을 유지한다.
            resetGraph: (object) => { if (graph) graph.setNumber(object, state.invalidGraph); else pool.physical(object)[GRAPH_OFFSET] = state.invalidGraph; },
            cost: (object) => { state.totalCost = this.totalCost(pool.slot(object)[TYPE_OFFSET]); },
            provider: (object) => registerManaProvider(state.currentProviders(), state, object, diagnostics),
            ownerFactory: (owner, object) => {
                if (!owner) return;
                if (owner >= state.ownerFactories.length) throw new RangeError('공통 후처리 현재 owner 공장 범위 오류');
                state.ownerFactories[owner].append(object);
            },
            factory: (object) => { state.currentFactories().append(object); state.productionDirty = (state.productionDirty + 1) >>> 0; },
            aiNotice: (owner, object) => {
                const notice = this.#aiNotice || this.#defaultDevices?.aiNotice;
                if (notice) { notice(owner, object, true); return; }
                if (owner >= state.aiAttached.length) throw new RangeError('공통 후처리 현재 AI owner 범위 오류');
                if (owner && state.aiAttached[owner]) throw new Error('공통 후처리 실제 AI 통지 미연결');
            },
            localOwner: () => {
                const owner = this.#localOwner || this.#defaultDevices?.localOwner;
                return owner ? owner() : state.localOwner;
            },
            localCount: (number) => { state.localCounts()[number] += 1; state.localSecondaryCounts()[number] += 1; },
            globalCount: (number) => { state.globalCounts()[number] += 1; },
            // 독립 명시 입력의 mouseType 0 은 장치 없이 자연 반환한다.
            recheckSelection: () => {
                const recheck = this.#selectionRecheck || this.#defaultDevices?.recheckSelection;
                if (recheck) recheck();
                else if (state.pendingPlacement) throw new Error('공통 후처리 실제 선택 재검사 미연결');
            },
        };
        runSquidPostPopParent(pool, sid, flags, state, hooks);
    }

    // 파생 몸체가 공통 몸체를 직접 부르는 비가상 호출이다(10.78 은 postPop 과 같다).
    postPopBase(sid, flags) { this.postPop(sid, flags); }
}
