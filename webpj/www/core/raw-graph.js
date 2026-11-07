// 실제 SID 풀·4단계 해시·spot·타입/프레임 표를 정수 Graph 연산(graph.js)에 연결하는 모듈이다(cpppj `o/RawGraph.cpp/.h` + `o/RawGraphLive.cpp`, 10.78 전용).
// 두 갈래가 있다.
//  - 사전 계산(calculate): 풀·해시·spot·기록·스택을 복사한 뒤 정수 Graph 로 같은 연산을 미리 돌려 보고, 손상/소진 입력이면 실제 공간을 쓰기 전에 거부한다(validate* 가 사용, 무효과).
//  - 실제 실행(execute*): 현재 풀·기록·스택을 직접 읽고 쓴다. 진단(assertion)과 로그(trace)가 돌려준 뒤의 현재 슬롯·번호·지도를 그때그때 다시 읽으므로 사본 Commit 으로 덮어쓰지 않는다.
// 실제 월드의 장기 물리는 RawGraphStorage(기록 255행 + flood 스택 4096 DWORD)가 소유하며, 독립 호출은 records/stack 입력으로 사본을 만들 수 있다.
// 진단은 { checking: () => 현재 DWORD, assertion: (식, 파일, 줄) => void }, 로그는 (수준, 문자열) => void, 시계는 () => 초(실수), 삭제 정책은 getter 두 개이다.
// 필요한 장치가 없는 진단 지점에 도달하면 Error 로 멈추고 그 앞에서 이미 쓴 효과는 되돌리지 않는다. 이 모듈은 DOM 을 쓰지 않는다.
// 범위: CD 10.72 판본, 사본 Commit(원본에서 더 쓰지 않는 사전 계획 반영)은 옮기지 않았다. (2026-11-08 WB04-E40: RawGraph/RawGraphLive 이식으로 추가)
import { WORLD_CELLS } from '../assets/terrain-mask.js';
import { GRAPH_COUNT, GRAPH_FLOOD_SIZE, GRAPH_INVALID, GRAPH_RECOVERY, GRAPH_TABLE_SIZE, Graph, checkGraphSentinel, removeGraphSurface } from './graph.js';
import { RawSquidFinder } from './raw-squid-finder.js';
import { NEIGHBOR_FLAG, RawSquidNeighborWalk, RawSquidNeighbors } from './raw-squid-neighbors.js';
import { rawPathSurfaceAt } from './raw-path-surface.js';
import { effectiveGenus } from './squid-space.js';
import { SQUID_HASH_LEVELS, SQUID_HASH_SCALES, SquidHash } from './squid-hash.js';
import { SurfaceFinder, TYPE_FLAG1_SURFACE } from './surface-finder.js';

// 슬롯 필드 위치(다음 링크·타입·상태·좌표·그래프 번호·프레임·extra)이다. 10.78 슬롯 배치이다. (2026-11-08 WB04-E40)
const NEXT_OFFSET = 4;
const TYPE_OFFSET = 10;
const STATE_OFFSET = 11;
const X_OFFSET = 14;
const Y_OFFSET = 18;
const GRAPH_OFFSET = 30;
const FRAME_OFFSET = 36;
const EXTRA_OFFSET = 40;

// 상태 비트(free·dead·void·contained)와 extra 의 매몰 비트, 표면 SID 한계(부호 있는 short), 첫 자산 타입 번호, 10.78 타입 수, 그래프 번호 예외 타입(울타리 표식·바람 궁수)이다. (2026-11-08 WB04-E40)
const FREE = 1;
const DEAD = 2;
const VOID = 4;
const CONTAINED = 8;
const BURIED = 8;
const MAXIMUM_ID = 32767;
const FIRST_ASSET_TYPE = 70;
const TYPE_COUNT = 188;
const FENCE_MARK_TYPE = 90;
const WIND_ARCHER_TYPE = 140;

// 연결 목록(지역 배열 64칸)·진단 파일 이름·Squid 진단 파일 이름·sentinel 두 WORD 값이다. (2026-11-08 WB04-E40)
const MAXIMUM_CONNECTIONS = 64;
const GRAPH_FILE = '\\Ns\\O\\Graph.cpp';
const SQUID_FILE = '\\Ns\\O\\Squid.cpp';
const SENTINEL = 0x7dfd;

// 사전 계산 연산 종류이다. Inspect 는 쓰기 계획 없이 기존 풀/지도/번호의 물리 계약만 확인한다. (2026-11-08 WB04-E40)
const OP = Object.freeze({ add: 'Add', flood: 'Flood', region: 'Region', regionAdd: 'RegionAdd', detach: 'Detach', rebuild: 'Rebuild', allocate: 'Allocate', inspect: 'Inspect' });

// 부호 있는 WORD 로 감기는 저장이다(원문 WORD 증감의 low16·signed 해석). (2026-11-08 WB04-E40)
const word = (value) => (((value & 0xffff) << 16) >> 16);

// 정렬되지 않은 리틀엔디언 정수 읽기이다(폭 1~4바이트, 결과는 부호 없는 32비트). (2026-11-08 WB04-E40)
function read(raw, offset, width = 4) {
    let value = 0;
    // 필드 폭 안의 바이트만 낮은 쪽부터 조합한다.
    for (let index = 0; index < width; index += 1) value += raw[offset + index] * 2 ** (8 * index);
    return value >>> 0;
}

// float 비트를 단정도로 읽는 보조 뷰이다. (2026-11-08 WB04-E40)
const floatView = new DataView(new ArrayBuffer(4));

// 슬롯 좌표 DWORD 를 단정도 float 로 읽는다. (2026-11-08 WB04-E40)
function floatOf(raw, offset) {
    floatView.setUint32(0, read(raw, offset), true);
    return floatView.getFloat32(0, true);
}

// std::clamp 와 같은 비교 순서의 정수 자르기이다. (2026-11-08 WB04-E40)
const clamp = (value, low, high) => (value < low ? low : high < value ? high : value);

// 비표면의 영역 통지 칸이다: 지도 안 유한 좌표를 0 쪽으로 절삭한다. (원본 004ade70→0049ae80→0041def0) (2026-11-08 WB04-E40)
function regionCoordinate(value) {
    if (!Number.isFinite(value) || value < 0 || value >= WORLD_CELLS) throw new RangeError('raw 그래프 영역은 지도 안 유한 좌표가 필요합니다');
    return Math.trunc(value) | 0;
}

// 표면 스냅샷은 정수 칸 연결을 유지하므로 소수 좌표를 거부한다. (2026-11-08 WB04-E40)
function coordinate(value) {
    if (Math.trunc(value) !== value) throw new RangeError('raw 그래프는 지도 안 정수 좌표가 필요합니다');
    return regionCoordinate(value);
}

// 타입이 표면인지(flags1 비트 0x800) 판정한다. (2026-11-08 WB04-E40)
const isSurface = (type) => ((type.flags1 >>> 0) & TYPE_FLAG1_SURFACE) !== 0;

// 6 BYTE 그래프 레코드 하나(surfaces·inUse·reserved)를 새로 만든다. (2026-11-08 WB04-E40)
const newRecord = () => ({ surfaces: 0, inUse: 0, reserved: 0 });

// 월드보다 오래 보관하는 두 실제 배열이다: 기록 255행(reserved 와 틈 3행 포함)과 이전 flood 의 흔적을 남기는 4096 DWORD 스택이다.
// 원문의 별도 heap 두 개를 한 연속 객체로 가장하지 않는다. (2026-11-08 WB04-E40)
export class RawGraphStorage {
    constructor() {
        this.records = Array.from({ length: GRAPH_TABLE_SIZE }, newRecord);
        this.stack = new Uint32Array(GRAPH_FLOOD_SIZE);
    }
}

// 원본 00462b20 의 월드 초기 표: 앞 251행의 두 WORD 를 지우고 0번 예약·reserved 하위 BYTE=1·sentinel 만 쓴다. 풀·스택·reserved 상위는 보존한다. (2026-11-08 WB04-E40)
export function resetRawGraphTable(storage) {
    // 앞 251개 정상 번호의 두 WORD 만 지운다(reserved 와 251..253 은 그대로).
    for (let index = 0; index < GRAPH_COUNT; index += 1) {
        storage.records[index].surfaces = 0;
        storage.records[index].inUse = 0;
    }
    storage.records[0].inUse = 1;
    storage.records[0].reserved = word((storage.records[0].reserved & 0xff00) | 1);
    storage.records[GRAPH_INVALID].surfaces = SENTINEL;
    storage.records[GRAPH_INVALID].inUse = SENTINEL;
}

// 사전 계산 중 `nextPool` 복사본에 쓰기 위해 풀 바이트를 복사해 두는 최소 풀이다(원문 `auto nextPool=pool_` 와 같은 값 사본). (2026-11-08 WB04-E40)
class GraphPoolSnapshot {
    #bytes;
    #stride;
    #capacity;

    // 슬롯 폭은 원본 풀과 같고, 복사본은 원본 풀을 건드리지 않는다.
    constructor(pool) {
        this.#stride = pool.layout.stride;
        this.#capacity = pool.capacity;
        this.#bytes = pool.bytes();
    }

    get capacity() { return this.#capacity; }

    // RawSquidFinder 가 요구하는 판본 식별자이다(원본 풀과 같은 10.78).
    get edition() { return 'patch1078'; }

    // 읽기 사본 슬롯이다.
    slot(sid) { return this.#bytes.slice(sid * this.#stride, (sid + 1) * this.#stride); }

    // 쓰기 대여 슬롯이다(free 슬롯 포함).
    physical(sid) { return this.#bytes.subarray(sid * this.#stride, (sid + 1) * this.#stride); }
}

// 실제 월드 Graph 연결 실행기이다. 풀·해시·spot 의 수명은 이 객체보다 길어야 한다.
//  - new RawGraph(pool, hash, spots, types, frames, { storage, records, stack, recovery })
//    types: 타입 188개 { flags1, flags2, footX, footY }, frames: 타입별 프레임 코드 배열 188개 [{ side, variant, number, flags }].
//    storage 를 주면 그 장기 물리를 빌려(생성 효과 없음) 쓰고, 아니면 records/stack 입력(없으면 sentinel 만 가진 빈 표)으로 독립 사본을 만든다.
//    recovery: GRAPH_RECOVERY.fullPool 이면 Add/Detach/Pop 이 전체 풀을 검사하고 번호 소진 때 전체 재구성을 허용한다.
// (2026-11-08 WB04-E40)
export class RawGraph {
    #pool;
    #hash;
    #spots;
    #types;
    #currentTypes = null;
    #frames;
    #storage;
    #recovery;
    #detachEnabled = () => true;
    #detachRebuild = null;
    #clock = null;
    #diagnostics = {};
    #trace = null;

    // 타입/프레임 표는 값으로 복사해 호출자의 수명과 분리한다. 크기와 입력 표/스택의 길이를 검사한다.
    constructor(pool, hash, spots, types, frames, { storage = null, records = [], stack = [], recovery = GRAPH_RECOVERY.reject } = {}) {
        if (types.length !== TYPE_COUNT || frames.length !== TYPE_COUNT || spots.length !== WORLD_CELLS * WORLD_CELLS) throw new RangeError('raw 그래프 타입/프레임/지도 크기 오류');
        if (records.length !== 0 && records.length !== GRAPH_TABLE_SIZE) throw new RangeError('raw 그래프 표 크기 오류');
        if (stack.length !== 0 && stack.length !== GRAPH_FLOOD_SIZE) throw new RangeError('raw 그래프 스택 크기 오류');
        this.#pool = pool;
        this.#hash = hash;
        this.#spots = spots;
        this.#types = Array.from(types, (type) => ({ flags1: type.flags1, flags2: type.flags2, footX: type.footX, footY: type.footY }));
        this.#frames = Array.from(frames, (codes) => Array.from(codes ?? [], (code) => ({ ...code })));
        this.#recovery = recovery;
        if (storage) {
            // 실제 공유 표의 sentinel/reserved/stack 은 생성자가 다시 초기화하지 않는다.
            this.#storage = storage;
            return;
        }
        this.#storage = new RawGraphStorage();
        if (records.length === 0) {
            this.#storage.records[GRAPH_INVALID] = { surfaces: SENTINEL, inUse: SENTINEL, reserved: 0 };
        } else {
            // 입력 255행을 값으로 복사한다(reserved 와 틈 3행 포함).
            for (let index = 0; index < GRAPH_TABLE_SIZE; index += 1) this.#storage.records[index] = { surfaces: records[index].surfaces, inUse: records[index].inUse, reserved: records[index].reserved };
        }
        if (stack.length !== 0) this.#storage.stack.set(stack);
    }

    // 현재 기록 255행(sentinel·reserved 포함)을 직접 보여준다. 장기 물리를 빌린 경우 같은 배열이다.
    records() { return this.#storage.records; }

    // 이전 flood 의 비활성 흔적까지 포함한 4096 DWORD 스택이다.
    floodStack() { return this.#storage.stack; }

    // postPop/Pop 이 공유하는 SID 풀이다.
    get pool() { return this.#pool; }

    // 장기 월드 물리의 저장소이다.
    get storage() { return this.#storage; }

    // 서로 다른 해시/spot 에 Pop 하고 Graph 를 갱신하는 연결은 거부한다.
    validateSpace(hash, spots) {
        if (hash !== this.#hash || spots !== this.#spots) throw new TypeError('Pop/raw 그래프 공간이 다릅니다');
    }

    // postPop 과 표면 필터/발자국이 다른 타입 표를 연결하지 않는다. 그래프가 쓰는 네 필드만 비교한다.
    validateTypes(types) {
        const current = this.#currentTypeList();
        if (types.length !== current.length) throw new TypeError('postPop/raw 그래프 타입 크기가 다릅니다');
        // 프레임 연결과 표면 발자국의 의미가 모든 타입에서 같아야 한다.
        for (let index = 0; index < types.length; index += 1) {
            if (types[index].flags1 !== current[index].flags1 || types[index].flags2 !== current[index].flags2 ||
                types[index].footX !== current[index].footX || types[index].footY !== current[index].footY) throw new TypeError('postPop/raw 그래프 타입이 다릅니다');
        }
    }

    // 현재 월드의 188 TYPE 배열을 빌린다. null 이나 빈 배열은 독립 소유 사본으로 되돌린다. 진단/프레임/기록은 다시 초기화하지 않는다.
    bindCurrentTypes(types) {
        if (types && types.length !== 0 && types.length !== TYPE_COUNT) throw new TypeError('Graph 현재 TYPE 대여의 길이 오류');
        this.#currentTypes = types && types.length !== 0 ? types : null;
    }

    // 같은 클라이언트의 현재 checking/진단을 연결한다. 표/스택/풀은 쓰지 않는다.
    bindDiagnostics(diagnostics) { this.#diagnostics = diagnostics ?? {}; }

    // 실제 월드의 graphsWork/rebuildGraph 현재 값을 빌린다. 연결하지 않으면 활성이며 분할 정책은 호출 인자이다.
    bindDetachState(enabled, rebuild) {
        this.#detachEnabled = enabled;
        this.#detachRebuild = rebuild;
    }

    // 삭제 부모가 부르는 두 시각 읽기를 현재 클라이언트 시계에 연결한다. 결과는 외부에 쓰지 않는다.
    bindClock(clock) { this.#clock = clock; }

    // 원문 수준 0x20 로그를 현재 클라이언트 장치에 연결한다. 독립 입력은 생략할 수 있다.
    bindTrace(trace) { this.#trace = trace; }

    // 현재 선택된 타입 목록(월드가 빌려준 것 또는 독립 사본)이다.
    #currentTypeList() { return this.#currentTypes ?? this.#types; }

    // 번호가 범위 안인 현재 타입 레코드이다. 기존 at 과 같이 범위 밖은 거부한다.
    #currentType(number) {
        const types = this.#currentTypeList();
        if (number >= types.length) throw new RangeError('Graph 현재 TYPE 범위 오류');
        return types[number];
    }

    // 슬롯의 현재 물리 읽기/쓰기 뷰이다(원문 span 처럼 진단 복귀 뒤의 변경이 보인다).
    #raw(sid) { return this.#pool.physical(sid); }

    // 현재 checking DWORD 가 0 이 아닌지 읽는다.
    #checking() { return Boolean(this.#diagnostics.checking?.()); }

    // 원문 판본의 파일/행을 실제 진단에 전달한다. 복귀 뒤 호출자가 계속한다. 장치가 없으면 멈춘다.
    #diagnose(expression, line, file = GRAPH_FILE) {
        if (typeof this.#diagnostics.assertion !== 'function') throw new Error('Graph 실제 진단 장치 미연결');
        this.#diagnostics.assertion(expression, file, line);
    }

    // 앞 251행의 현재 사용 수를 세어 로그 장치를 부른다. 장치가 없으면 출력하지 않는다.
    #traceInUse() {
        if (!this.#trace) return;
        let used = 0;
        // 정상 번호 251개 중 사용 중인 행의 개수를 센다.
        for (let index = 0; index < GRAPH_COUNT; index += 1) if (this.#storage.records[index].inUse !== 0) used += 1;
        this.#trace(0x20, `${used} graphs in use\n`);
    }

    // 표면 타입 판정 진단이 필요한 비표면 타입인지(울타리 표식·바람 궁수 예외 포함) 검사한다.
    #surfaceOrException(type) {
        return isSurface(this.#currentType(type)) || type === FENCE_MARK_TYPE || type === WIND_ARCHER_TYPE;
    }

    // 현재 그래프 BYTE 를 읽는다. 비표면 타입(예외 제외)이면 먼저 진단하고 진단 복귀 뒤 같은 물리 슬롯을 다시 읽는다.
    #readNumber(sid) {
        const type = this.#raw(sid)[TYPE_OFFSET];
        if (!this.#surfaceOrException(type)) this.#diagnose('getTypeFlags() & TYPE_FLAG_IS_SURFACE || getType() == fencemarkType || getType() == windArcherType', 0x739, SQUID_FILE);
        return this.#raw(sid)[GRAPH_OFFSET];
    }

    // 타입/번호를 진단한 뒤 현재 물리 슬롯의 그래프 BYTE 만 쓴다. 진단으로 free 가 바뀌어도 추가 수명 관문을 만들지 않는다.
    #writeNumber(sid, graph) {
        const type = this.#raw(sid)[TYPE_OFFSET];
        if (!this.#surfaceOrException(type)) this.#diagnose('getTypeFlags() & TYPE_FLAG_IS_SURFACE || type == fencemarkType || type == windArcherType', 0x74b, SQUID_FILE);
        if (graph >= GRAPH_COUNT && graph !== GRAPH_INVALID && type !== FENCE_MARK_TYPE && type !== WIND_ARCHER_TYPE) {
            this.#diagnose('( n >=0 && n < MaxGraphs) || ( n==invalidGraphNum) || type == fencemarkType || type == windArcherType', 0x750, SQUID_FILE);
        }
        this.#raw(sid)[GRAPH_OFFSET] = graph;
    }

    // 공통 postPop 의 noGraph 리셋도 같은 현재 TYPE/진단/쓰기 몸체를 쓴다(004ad3e0).
    setNumber(sid, graph) { this.#writeNumber(sid, graph); }

    // 직접 Free 의 번호 범위: 254 는 자연 반환이고 251..253/255 는 정상 번호가 아니다.
    validateFree(graph) {
        if (graph >= GRAPH_COUNT && graph !== GRAPH_INVALID) throw new RangeError('raw 그래프 Free 번호 오류');
    }

    // 254 는 sentinel 도 읽지 않는다. 유효 번호는 sentinel 진단 복귀 뒤 현재 행의 앞 두 WORD 만 지운다(reserved·풀·스택 보존).
    free(graph) {
        this.validateFree(graph);
        if (graph === GRAPH_INVALID) return;
        checkGraphSentinel(this.#storage.records[GRAPH_INVALID], this.#diagnostics);
        this.#storage.records[graph].inUse = 0;
        this.#storage.records[graph].surfaces = 0;
    }

    // 특수 삭제 분기가 읽는 원본 프레임 방향 문자 코드이다. 타입/프레임 표를 캐시하지 않는다.
    frame(sid) {
        const raw = this.#raw(sid);
        const number = raw[TYPE_OFFSET];
        const index = read(raw, FRAME_OFFSET, 4);
        if (number < FIRST_ASSET_TYPE || number >= this.#currentTypeList().length || index >= this.#frames[number].length) throw new RangeError('raw 그래프 프레임 조회 오류');
        return this.#frames[number][index];
    }

    // 월드 초기 표를 만든다(Graph0 예약·reserved 하위 BYTE·sentinel 만 초기화, 풀/스택 보존).
    resetTable() { resetRawGraphTable(this.#storage); }

    // 새 좌표·void 해제·spot/해시 등록까지 예측해 실패를 공간 쓰기 전에 검출한다(무효과).
    validateAdd(sid, pop = null) { this.#calculate(sid, OP.add, 0, pop); }

    // 영역 통지→표면 Add 순서를 한 복사본에서 시험해 Pop 전에 모든 실패를 검출한다(무효과).
    validatePostPop(sid, invalidate, add, pop = null) {
        if (invalidate || add) this.#calculate(sid, invalidate ? (add ? OP.regionAdd : OP.region) : OP.add, 0, pop);
    }

    // 삭제 준비를 무효과로 검사한다. rebuild 는 분할 정책 비트 1, removedSurfaces 9 는 비트 2 로 인코딩한다.
    validateDetach(sid, rebuild = false, removedSurfaces = 1, sourceType = null) {
        if (removedSurfaces !== 1 && removedSurfaces !== 9) throw new RangeError('raw 그래프 감소 수 오류');
        this.#calculate(sid, OP.detach, (rebuild ? 1 : 0) | (removedSurfaces === 9 ? 2 : 0), null, sourceType);
    }

    // 00462d40 ↔ CD 0045c210: 영역 통지를 무효과로 시험한 뒤 현재 물리에서 실제로 실행한다(내부 spot 표면의 번호 무효화).
    invalidateRegion(sid) {
        this.#calculate(sid, OP.region, 0, null);
        this.#region(sid, null, this.#spots, { records: [], stack: null, numbers: [], changed: 0 }, true);
    }

    // 공통 postPop 의 무효과 사전 검사 뒤 영역 통지와 표면 Add 를 원문 순서대로 실행한다.
    postPop(sid, invalidate, add) {
        this.validatePostPop(sid, invalidate, add);
        if (invalidate) this.invalidateRegion(sid);
        if (add) this.add(sid);
    }

    // 원문 진단 복귀 뒤 현재 슬롯/표/이웃을 직접 소비하는 Add 이다.
    add(sid) { this.#executeAdd(sid); }

    // 삭제 분할을 실행한다. 활성 전역이 꺼져 있으면 아무것도 하지 않는다. 원천의 state/좌표/next 는 Unpop 에 남긴다.
    detach(sid, rebuild = false, removedSurfaces = 1, sourceType = null) {
        if (removedSurfaces !== 1 && removedSurfaces !== 9) throw new RangeError('raw 그래프 감소 수 오류');
        this.#executeDetach(sid, rebuild, removedSurfaces, sourceType);
    }

    // postDestroy 의 일반 finder→표면별 Add 를 무효과 사본에서 미리 검사한다.
    validatePostDestroy(sid) { this.#calculatePostDestroy(sid); }

    // 현재 일반 finder 와 순차 Add 를 실행한다. 사전 검사는 별도 사본에서 하고 Unpop/반납은 하지 않는다.
    postDestroy(sid) { this.#executePostDestroy(sid); }

    // 기존 raw 표면을 실제 탐색 순서로 flood 하고 번호를 바꾼 객체 수를 돌려준다.
    flood(sid, graph) { return this.#executeFlood(sid, graph, true); }

    // 전체 raw 풀의 현재 번호를 두 번 순회해 재구성한다.
    rebuild(resetAll) { this.#executeRebuild(resetAll); }

    // 빈 번호가 있으면 표만 갱신하고, 소진되면 전체 raw 풀 재구성 뒤 번호를 확보한다.
    allocate() { return this.#executeAllocate(true); }

    // 4단계 해시를 y 바깥/x 안쪽으로 훑어 발자국과 겹치는 내부 spot 표면의 번호를 무효화한다. 단계별 끝 버킷을 한 칸 넓힌다(표면 이웃 탐색과 다른 경로).
    // live 이면 현재 슬롯·기록에 직접 쓰고, 아니면 plan 에 쌓는다.
    #region(sid, pop, spots, plan, live) {
        const root = this.#raw(sid);
        const source = this.#currentType(root[TYPE_OFFSET]);
        const cell = isSurface(source) ? coordinate : regionCoordinate;
        const x = cell(pop ? pop.x : floatOf(root, X_OFFSET));
        const y = cell(pop ? pop.y : floatOf(root, Y_OFFSET));
        if (source.footX < 1 || source.footY < 1 || source.footX > WORLD_CELLS || source.footY > WORLD_CELLS) throw new RangeError('그래프 영역 발자국 오류');
        const left = Math.max(0, x - source.footX + 1);
        const top = Math.max(0, y - source.footY + 1);
        const right = x;
        const bottom = y;
        // 네 해시 단계를 낮은 단계부터 방문한다.
        for (let level = 0; level < SQUID_HASH_LEVELS; level += 1) {
            const scale = SQUID_HASH_SCALES[level];
            const side = SquidHash.side(level);
            const startX = Math.trunc(left / scale);
            const startY = Math.trunc(top / scale);
            const endX = Math.min(side - 1, Math.trunc(right / scale) + 1);
            const endY = Math.min(side - 1, Math.trunc(bottom / scale) + 1);
            // 일반 탐색의 y 바깥 순서이다.
            for (let cy = startY; cy <= endY; cy += 1) {
                // 최대 발자국이 더 큰 객체의 기준점도 끝 버킷에서 잡는다.
                for (let cx = startX; cx <= endX; cx += 1) {
                    const projected = pop !== null && pop.level === level && cy * side + cx === SquidHash.bucketIndex(level, pop.x, pop.y);
                    let current = projected ? sid : this.#hash.cell(level, cx, cy);
                    const chain = [];
                    // 각 버킷의 실제 머리/next 순서로 조회하며 손상된 순환은 쓰기 전에 거부한다.
                    while (current) {
                        if (current < 5 || current > MAXIMUM_ID || current >= this.#pool.capacity || chain.includes(current)) throw new Error('그래프 영역 해시 범위/순환 오류');
                        chain.push(current);
                        const candidate = current;
                        const bytes = this.#raw(candidate);
                        const next = pop && candidate === sid ? this.#hash.bucket(pop.level, pop.x, pop.y) : read(bytes, NEXT_OFFSET, 2);
                        current = next & 0xffff;
                        if (bytes[STATE_OFFSET] & (FREE | CONTAINED)) throw new Error('그래프 영역 후보 상태 오류');
                        if (bytes[EXTRA_OFFSET] & BURIED) continue;
                        if (bytes[TYPE_OFFSET] < FIRST_ASSET_TYPE || bytes[TYPE_OFFSET] >= this.#currentTypeList().length) throw new Error('그래프 영역 후보 타입 오류');
                        // 실제 일반 탐색의 비표면은 그래프를 바꾸지 않는다. 이동 객체의 소수 좌표를 표면 정수로 검증하지 않는다.
                        if (!isSurface(this.#currentTypeList()[bytes[TYPE_OFFSET]])) continue;
                        const ax = coordinate(pop && candidate === sid ? pop.x : floatOf(bytes, X_OFFSET));
                        const ay = coordinate(pop && candidate === sid ? pop.y : floatOf(bytes, Y_OFFSET));
                        // 원본 패치 일반 탐색은 유효 좌표 밖을 건너뛰지만 이 경로는 정상 입력만 받는다.
                        if (ax <= 0 || ay <= 0) throw new RangeError('그래프 영역 후보의 유효 좌표가 필요합니다');
                        const type = this.#currentTypeList()[bytes[TYPE_OFFSET]];
                        if (type.footX < 1 || type.footY < 1 || type.footX > WORLD_CELLS || type.footY > WORLD_CELLS) throw new RangeError('그래프 영역 후보 발자국 오류');
                        const candidateLeft = ax - type.footX + 1;
                        const candidateTop = ay - type.footY + 1;
                        if (ax < left || candidateLeft > right || ay < top || candidateTop > bottom || !isSurface(type) || !(spots[ay * WORLD_CELLS + ax] & 8)) continue;
                        const staged = plan.numbers.find((item) => item[0] === candidate);
                        const graph = live || !staged ? bytes[GRAPH_OFFSET] : staged[1];
                        if (graph === GRAPH_INVALID) continue;
                        if (graph >= GRAPH_COUNT) throw new RangeError('그래프 영역 후보 번호 오류');
                        if (live) {
                            // 진단 복귀 뒤 현재 슬롯의 그래프 BYTE 만 무효화한다. 다음 SID 는 finder 와 같이 이미 저장했다.
                            removeGraphSurface(this.#storage.records[graph], this.#diagnostics);
                            this.#writeNumber(candidate, GRAPH_INVALID);
                            continue;
                        }
                        const record = plan.records[graph];
                        // Remove 의 활성/양수 조건을 유지하며 0 이 되면 사용 여부도 지운다. reserved 는 보존한다.
                        if (record.inUse && record.surfaces > 0) {
                            record.surfaces -= 1;
                            if (!record.surfaces) record.inUse = 0;
                        }
                        if (!staged) plan.numbers.push([candidate, GRAPH_INVALID]);
                        else staged[1] = GRAPH_INVALID;
                    }
                }
            }
        }
    }

    // 삭제 연결의 일반 탐색이다: 원천을 한 칸 넓히고 단계별 끝 버킷도 하나 늘린다. flag 8 표면 탐색의 순서/교차 필터로 대체하지 않는다.
    // sourceFrameValid 가 false 이면 원본 마스크가 방향 코드를 덮어쓰는 연결만 해석한다.
    #detachConnections(source, finder, sourceFrameValid) {
        const result = [];
        // 발자국 helper 는 두 번째 점을 1..255 로 보정한다. 초기 점이 무효면 양 끝을 그 점으로 재설정한다.
        const bounds = (object) => {
            const x = clamp(object.x - object.width + 1, 1, WORLD_CELLS - 1);
            const y = clamp(object.y - object.height + 1, 1, WORLD_CELLS - 1);
            return object.x > 0 && object.y > 0 ? [x, y, object.x, object.y] : [x, y, x, y];
        };
        const [left, top, right, bottom] = bounds(source);
        let interior = 0xff;
        // 원본은 원천 발자국 spot 전체의 AND 로 내부 상태를 확인한다(원천의 정수 발자국 각 칸).
        for (let y = top; y <= bottom; y += 1) for (let x = left; x <= right; x += 1) interior &= this.#spots[y * WORLD_CELLS + x];
        if (interior & 8) return result;
        // 경계 접촉을 센다(패치의 작음 != 같음도 정수 좌표에서는 <=).
        const intersects = (aLeft, aTop, aRight, aBottom) => {
            const x1 = Math.max(left, aLeft);
            const y1 = Math.max(top, aTop);
            const x2 = Math.min(right, aRight);
            const y2 = Math.min(bottom, aBottom);
            return x1 > 0 && y1 > 0 && x2 < WORLD_CELLS && y2 < WORLD_CELLS && x1 <= x2 && y1 <= y2;
        };
        // 네 해시 단계를 낮은 단계부터 방문한다.
        for (let level = 0; level < SQUID_HASH_LEVELS; level += 1) {
            const scale = SQUID_HASH_SCALES[level];
            const side = SquidHash.side(level);
            const startX = Math.trunc(Math.max(0, left - 1) / scale);
            const startY = Math.trunc(Math.max(0, top - 1) / scale);
            const endX = Math.min(side - 1, Math.trunc(Math.min(WORLD_CELLS - 1, right + 1) / scale) + 1);
            const endY = Math.min(side - 1, Math.trunc(Math.min(WORLD_CELLS - 1, bottom + 1) / scale) + 1);
            // 같은 단계 안에서는 y/x, 같은 버킷 안에서는 next 순서를 보존한다.
            for (let cy = startY; cy <= endY; cy += 1) {
                // 끝 버킷의 큰 발자국 후보도 원본과 같은 순서로 판정한다.
                for (let cx = startX; cx <= endX; cx += 1) {
                    let current = this.#hash.cell(level, cx, cy);
                    const chain = [];
                    // 손상된 SID/자기 next/순환은 계획 계산 중에 거부한다.
                    while (current) {
                        if (current < 5 || current > MAXIMUM_ID || current >= this.#pool.capacity || chain.includes(current)) throw new Error('그래프 삭제 해시 범위/순환 오류');
                        chain.push(current);
                        const bytes = this.#raw(current);
                        const candidate = current;
                        current = read(bytes, NEXT_OFFSET, 2);
                        if (bytes[STATE_OFFSET] & (FREE | CONTAINED | VOID)) throw new Error('그래프 삭제 후보 상태 오류');
                        if (bytes[TYPE_OFFSET] < FIRST_ASSET_TYPE || bytes[TYPE_OFFSET] >= this.#currentTypeList().length) throw new Error('그래프 삭제 후보 타입 오류');
                        if ((bytes[STATE_OFFSET] & DEAD) || (bytes[EXTRA_OFFSET] & BURIED) || !isSurface(this.#currentTypeList()[bytes[TYPE_OFFSET]])) continue;
                        const object = finder.object(candidate);
                        const area = bounds(object);
                        if ((this.#spots[object.y * WORLD_CELLS + object.x] & 8) ||
                            intersects(area[0] - 1, area[1], area[2] + 1, area[3]) === intersects(area[0], area[1] - 1, area[2], area[3] + 1)) continue;
                        // 00441e40 은 다리/폭탄→육지 연결의 두 번째 코드를 A 로 덮어쓴다. 실제 157 H=8 을 162 의 한 프레임 표로 바꿀 때 그 무효 코드 바이트를 해석하지 않는다.
                        let connected;
                        try {
                            connected = SurfaceFinder.connects(object, source);
                        } catch (error) {
                            if (!(error instanceof RangeError) || sourceFrameValid) throw error;
                            throw new RangeError(`raw 그래프 삭제 치환 방향 코드가 필요합니다 source=${source.id},${source.x},${source.y},${source.width},${source.height},${source.flags2} candidate=${candidate},${object.x},${object.y},${object.width},${object.height},${object.flags2},${object.frame.side},${object.frame.variant}`);
                        }
                        if (!connected) continue;
                        if (result.includes(candidate)) throw new Error('그래프 삭제 중복 해시 후보');
                        result.push(candidate);
                    }
                }
            }
        }
        return result;
    }

    // 전체 계산을 복사본에서 수행하며 미지원 입력·소진·연결 오류를 쓰기 전에 보고한다. 결과 계획은 기록/스택 사본과 번호 변경 목록이다.
    #calculate(sid, operation, target, pop, sourceType = null) {
        if (pop) pop = { ...pop, x: Math.fround(pop.x), y: Math.fround(pop.y) }; // 원문 인자는 float 이므로 단정도로 맞춘다.
        const global = operation === OP.rebuild || operation === OP.allocate;
        const plan = { records: this.#storage.records.map((record) => ({ ...record })), stack: Uint32Array.from(this.#storage.stack), numbers: [], changed: 0 };
        if (operation === OP.allocate) {
            // 원본의 여유 번호 경로는 풀/프레임/지도를 읽지 않는다.
            for (let index = 0; index < GRAPH_COUNT; index += 1) {
                if (!plan.records[index].inUse) {
                    plan.records[index].surfaces = 0;
                    plan.records[index].inUse = 1;
                    plan.changed = index;
                    return plan;
                }
            }
        }
        const region = operation === OP.region || operation === OP.regionAdd;
        const add = operation === OP.add || operation === OP.regionAdd;
        // 자동 재구성 가능 경로는 void/contained 표면까지 전체 풀에서 읽는다. 현재 실행의 물리 검사도 같은 전체 풀 계약을 쓴다.
        const fullPool = global || (this.#recovery === GRAPH_RECOVERY.fullPool && (add || operation === OP.detach || operation === OP.inspect));
        const root = this.#raw(sid);
        const types = this.#currentTypeList();
        if (!global && (sid < 5 || sid > MAXIMUM_ID || (root[STATE_OFFSET] & (FREE | CONTAINED)) || root[TYPE_OFFSET] < FIRST_ASSET_TYPE ||
            root[TYPE_OFFSET] >= types.length || (operation !== OP.region && !isSurface(types[root[TYPE_OFFSET]])))) throw new Error('raw 그래프 표면 SID 오류');
        if (operation === OP.detach && (!(root[STATE_OFFSET] & DEAD) || (target & ~3))) throw new TypeError('raw 그래프 삭제 준비의 dead/정책 오류');
        let detachGraph = root[GRAPH_OFFSET];
        if (operation === OP.detach) {
            // GetGridSid 는 해시 객체 +12 의 0단계 머리를 읽는다. 다른 단계의 원천은 자연 반환한다.
            const x = coordinate(floatOf(root, X_OFFSET));
            const y = coordinate(floatOf(root, Y_OFFSET));
            const found = this.#hash.cell(0, x, y);
            if (!found) return plan;
            // 원본 GetGraph(position)은 삭제 원천과 독립적으로 0단계 머리 SID 의 graph byte 를 읽는다.
            if (found < 5 || found > MAXIMUM_ID || found >= this.#pool.capacity) throw new RangeError('raw 그래프 삭제 조회 SID 오류');
            const lookup = this.#raw(found);
            if ((lookup[STATE_OFFSET] & (FREE | CONTAINED | VOID)) || lookup[TYPE_OFFSET] < FIRST_ASSET_TYPE || lookup[TYPE_OFFSET] >= types.length ||
                !isSurface(types[lookup[TYPE_OFFSET]]) || coordinate(floatOf(lookup, X_OFFSET)) !== x || coordinate(floatOf(lookup, Y_OFFSET)) !== y) throw new Error('raw 그래프 삭제 조회 표면/위치 오류');
            // 원본은 무효/미사용 그래프에서 finder 를 만들지 않는다. 손상 이웃도 아직 읽지 않는다.
            const number = lookup[GRAPH_OFFSET];
            if (number === GRAPH_INVALID) return plan;
            if (number >= GRAPH_COUNT) throw new RangeError('raw 그래프 삭제 번호 오류');
            if (!this.#storage.records[number].inUse) return plan;
            detachGraph = number;
        }
        if (pop) {
            const type = types[root[TYPE_OFFSET]];
            if (operation === OP.flood || !pop.type || pop.level < 0 || pop.level > 3 || type.flags1 !== pop.type.flags1 ||
                type.flags2 !== pop.type.flags2 || type.footX !== pop.type.footX || type.footY !== pop.type.footY) throw new TypeError('Pop/raw 그래프 타입/단계 오류');
        }
        // Add 의 비활성 입력은 원본처럼 그래프 표/스택을 읽지 않고 돌아간다.
        const state = pop ? root[STATE_OFFSET] & ~VOID & 0xff : root[STATE_OFFSET];
        if (add && (state & 7) && !region) return plan;
        const map = this.#hash.entries(0); // 0단계 머리 지도의 독립 사본이다(y 바깥/x 안쪽 배열 순서).
        const spots = Uint8Array.from(this.#spots);
        if (pop) {
            const type = types[root[TYPE_OFFSET]];
            const cell = isSurface(type) ? coordinate : regionCoordinate;
            const x = cell(pop.x);
            const y = cell(pop.y);
            if (pop.level === 0) map[y * WORLD_CELLS + x] = sid;
            if (!(root[EXTRA_OFFSET] & BURIED)) {
                if (type.footX < 1 || type.footY < 1 || x - type.footX + 1 < 1 || y - type.footY + 1 < 1) throw new RangeError('raw 그래프 Pop 발자국 오류');
                // 실제 Pop 의 y/x 순서와 genus low byte OR 를 예측한다. overlap 은 Pop 이 처리한다.
                for (let cy = y - type.footY + 1; cy <= y; cy += 1) {
                    // 각 발자국 행을 최종 spot 으로 만든다.
                    for (let cx = x - type.footX + 1; cx <= x; cx += 1) {
                        spots[cy * WORLD_CELLS + cx] = (spots[cy * WORLD_CELLS + cx] | effectiveGenus(type.flags2, pop.x, pop.y, type.footX, type.footY, cx, cy)) & 0xff;
                    }
                }
            }
        }
        if (region) this.#region(sid, pop, spots, plan, false);
        if (operation === OP.region || (add && (state & 7))) return plan;
        const required = new Uint8Array(this.#pool.capacity);
        if (!global) required[sid] = 1;
        // 0단계 머리만 읽는다. 전체 발자국을 가짜 객체 번호로 채우거나 다른 해시 단계를 섞지 않는다.
        for (let index = 0; index < map.length; index += 1) {
            const id = map[index];
            if (!id) continue;
            if (id < 5 || id > MAXIMUM_ID || id >= this.#pool.capacity) throw new RangeError('raw 그래프 해시 SID 오류');
            const candidate = this.#raw(id);
            // 로컬 표면 스냅샷에만 비표면 필터를 적용한다. 살아 있는 실제 해시/좌표/다음 링크는 변경하지 않는다.
            if (!(candidate[STATE_OFFSET] & (FREE | CONTAINED | VOID)) && candidate[TYPE_OFFSET] >= FIRST_ASSET_TYPE && candidate[TYPE_OFFSET] < types.length && !isSurface(types[candidate[TYPE_OFFSET]])) {
                map[index] = 0;
                continue;
            }
            required[id] = 1;
        }
        const objects = [];
        const members = [];
        // 현재 살아 있는 표면과 지도에 실제 등장하는 비표면 후보를 읽어 원본 필터에 전달한다.
        for (let id = 5; id < this.#pool.capacity; id += 1) {
            const bytes = this.#raw(id);
            const currentState = pop && id === sid ? state : bytes[STATE_OFFSET];
            if (currentState & (fullPool ? FREE : (FREE | VOID))) {
                if (required[id] && id !== sid) throw new Error('raw 그래프 해시의 free/void SID');
                if (id !== sid) continue;
            }
            // 내부 Form 은 원본 flags getter 의 비표면 타입이며 공간 머리에 없으면 전체순회에서 생략한다.
            if (bytes[TYPE_OFFSET] < FIRST_ASSET_TYPE) {
                if (required[id]) throw new Error('raw 그래프 후보 타입 오류');
                continue;
            }
            if (bytes[TYPE_OFFSET] >= types.length) {
                if (required[id] || fullPool) throw new Error('raw 그래프 후보 타입 오류');
                continue;
            }
            const type = types[bytes[TYPE_OFFSET]];
            const surface = isSurface(type);
            if (!surface && !required[id]) continue;
            if (id > MAXIMUM_ID || ((currentState & CONTAINED) && (!fullPool || required[id])) || (fullPool && (currentState & VOID) && required[id])) throw new RangeError('raw 그래프 후보 상태/SID 범위 오류');
            const x = pop && id === sid ? pop.x : floatOf(bytes, X_OFFSET);
            const y = pop && id === sid ? pop.y : floatOf(bytes, Y_OFFSET);
            let code = { side: 0, variant: 0, number: 0, flags: 0 };
            if (surface) {
                const frame = read(bytes, FRAME_OFFSET, 4);
                if (frame >= this.#frames[bytes[TYPE_OFFSET]].length) throw new RangeError('raw 그래프 프레임 번호 오류');
                code = this.#frames[bytes[TYPE_OFFSET]][frame];
                const staged = plan.numbers.find((item) => item[0] === id);
                members.push({ id, graph: staged ? staged[1] : bytes[GRAPH_OFFSET], state: currentState });
            }
            objects.push({
                id, x: coordinate(x), y: coordinate(y), width: type.footX, height: type.footY, flags1: type.flags1, flags2: type.flags2,
                frame: code, dead: (currentState & DEAD) !== 0, buried: (bytes[EXTRA_OFFSET] & BURIED) !== 0,
            });
        }
        const finder = new SurfaceFinder(objects, map, spots);
        const graph = new Graph(finder, members, plan.records, plan.stack);
        if (operation === OP.inspect) return plan;
        if (operation === OP.rebuild) graph.rebuild(target !== 0);
        else if (operation === OP.allocate) plan.changed = graph.allocateWithRecovery();
        else if (add) graph.add(sid, this.#recovery);
        else if (operation === OP.detach) {
            const source = { ...finder.object(sid), frame: { ...finder.object(sid).frame } };
            let sourceFrameValid = true;
            // 치환 정보에서 실제 방향 코드가 필요한지 후보마다 판정한다.
            if (sourceType !== null) {
                if (sourceType >= types.length) throw new RangeError('raw 그래프 삭제 치환 타입 오류');
                const replacement = types[sourceType];
                const frame = read(root, FRAME_OFFSET, 4);
                if (!isSurface(replacement)) throw new RangeError('raw 그래프 삭제 치환 표면 오류');
                if (replacement.footX < 1 || replacement.footY < 1 || replacement.footX > WORLD_CELLS || replacement.footY > WORLD_CELLS) throw new RangeError('raw 그래프 삭제 치환 발자국 오류');
                source.width = replacement.footX;
                source.height = replacement.footY;
                source.flags1 = replacement.flags1;
                source.flags2 = replacement.flags2;
                sourceFrameValid = frame < this.#frames[sourceType].length;
                source.frame = sourceFrameValid ? { ...this.#frames[sourceType][frame] } : { side: 0, variant: 0, number: 0, flags: 0 };
            }
            // 연결 판단은 삭제 정보의 타입을 쓰되 전체 풀 재구성은 원본 raw 타입을 읽는다.
            graph.detachAt(sid, detachGraph, this.#detachConnections(source, finder, sourceFrameValid), (target & 1) !== 0, (target & 2) !== 0 ? 9 : 1, this.#recovery, source.flags2);
        } else plan.changed = graph.flood(sid, target);
        const records = graph.records();
        // 정수 Graph 가 만든 최종 기록/스택을 계획에 반영한다.
        for (let index = 0; index < records.length; index += 1) plan.records[index] = records[index];
        plan.stack.set(graph.floodStack());
        // 영역에서 변경한 번호도 최종 Add 결과로 덮고 다른 raw 필드에는 쓰지 않는다.
        for (const member of members) {
            const number = graph.number(member.id);
            const staged = plan.numbers.find((item) => item[0] === member.id);
            if (staged) staged[1] = number;
            else if (number !== member.graph) plan.numbers.push([member.id, number]);
        }
        return plan;
    }

    // postDestroy 의 일반 탐색 순서에 따라 이전 Add 의 번호/표/스택을 다음 Add 가 읽도록 풀 복사본에서 순차 Add 를 시험한다(원본 풀·기록·스택에는 쓰지 않는다).
    #calculatePostDestroy(sid) {
        const root = this.#raw(sid);
        const types = this.#currentTypeList();
        if (sid < 5 || (root[STATE_OFFSET] & (FREE | CONTAINED)) || root[TYPE_OFFSET] < FIRST_ASSET_TYPE || root[TYPE_OFFSET] >= types.length) throw new Error('raw 그래프 postDestroy 자산 오류');
        const type = types[root[TYPE_OFFSET]];
        // 예측 그림 삭제 후의 표면 재연결도 같은 원본 발자국 칸을 사용한다.
        const cell = isSurface(type) ? coordinate : regionCoordinate;
        const x = cell(floatOf(root, X_OFFSET));
        const y = cell(floatOf(root, Y_OFFSET));
        if (type.footX < 1 || type.footY < 1 || type.footX > WORLD_CELLS || type.footY > WORLD_CELLS) throw new RangeError('raw 그래프 postDestroy 발자국 오류');
        const left = clamp(x - type.footX + 1, 1, WORLD_CELLS - 1);
        const top = clamp(y - type.footY + 1, 1, WORLD_CELLS - 1);
        const area = { left, top, right: x > 0 && y > 0 ? x : left, bottom: x > 0 && y > 0 ? y : top };
        const nextPool = new GraphPoolSnapshot(this.#pool);
        const next = new RawGraph(nextPool, this.#hash, this.#spots, types, this.#frames, { records: this.#storage.records, stack: this.#storage.stack, recovery: this.#recovery });
        const finder = new RawSquidFinder(nextPool, this.#hash, types);
        // Begin 은 첫 Next 까지 수행한다. 반환 직전 next 저장과 단계/y/x 순서를 그대로 사용한다.
        for (let candidate = finder.begin(area); candidate; candidate = finder.next()) {
            if (nextPool.slot(candidate)[TYPE_OFFSET] >= types.length) throw new RangeError('raw 그래프 postDestroy 후보 타입 오류');
            if (isSurface(types[nextPool.slot(candidate)[TYPE_OFFSET]])) next.add(candidate);
        }
        const plan = { records: next.records().map((record) => ({ ...record })), stack: Uint32Array.from(next.floodStack()), numbers: [], changed: 0 };
        // 실제 raw 필드 중 graph byte 의 변경분만 원본 풀에 반영할 계획에 넣는다.
        for (let id = 5; id < this.#pool.capacity; id += 1) {
            const number = nextPool.slot(id)[GRAPH_OFFSET];
            if (number !== this.#raw(id)[GRAPH_OFFSET]) plan.numbers.push([id, number]);
        }
        return plan;
    }

    // 현재 낮은 빈 행을 선택하며 소진한 경우에만 전체 재구성을 수행한다. recover 가 거짓이면 소진 때 멈춘다.
    #executeAllocate(recover) {
        const records = this.#storage.records;
        const findFree = () => {
            // 정상 번호 251개에서 처음 비어 있는 행을 찾는다.
            for (let index = 0; index < GRAPH_COUNT; index += 1) if (!records[index].inUse) return index;
            return -1;
        };
        let found = findFree();
        if (found < 0) {
            if (!recover) throw new Error('그래프가 소진되어 전체 풀 재구성이 필요합니다');
            this.#executeRebuild(true);
            found = findFree();
            if (found < 0) {
                this.#diagnose('false', 0x25d);
                throw new Error('원문 Graph 할당의 NULL 반환은 호스트 레코드로 해석할 수 없습니다');
            }
        }
        records[found].surfaces = 0;
        records[found].inUse = 1;
        return found;
    }

    // 이웃 탐색기를 현재 풀/해시/spot/타입/프레임에 연결한다. 후보 타입/프레임은 바꾸지 않는다.
    #neighbors() {
        const neighbors = new RawSquidNeighbors(this.#pool, this.#hash, this.#spots, this.#currentTypeList(), this.#frames);
        if (this.#currentTypes) neighbors.bindCurrentTypes(this.#currentTypes);
        return neighbors;
    }

    // 현재 표/슬롯/스택에서 flood 한다. 진단과 로그가 고친 값을 덮지 않는다. inspect 는 외부 직접 호출의 초기 물리 검사이며 내부 호출은 거치지 않는다.
    #executeFlood(sid, graph, inspect = false) {
        const records = this.#storage.records;
        const stack = this.#storage.stack;
        checkGraphSentinel(records[GRAPH_INVALID], this.#diagnostics);
        this.validateFree(graph);
        if (inspect) this.#calculate(sid, OP.inspect, 0, null);
        const neighbors = this.#neighbors();
        stack[0] = sid;
        let depth = 1;
        let changed = 0;
        // 별도 방문 집합 없이 현재 목적 번호를 읽고 원문 LIFO 중복 push 를 유지한다.
        while (depth) {
            depth -= 1;
            const value = stack[depth];
            if (!value || value > 0xffff || value >= this.#pool.capacity) throw new RangeError('Graph 현재 flood stack SID 범위 오류');
            const current = value;
            const previous = this.#readNumber(current);
            if (previous === graph) continue;
            this.validateFree(previous);
            changed += 1;
            if (previous !== GRAPH_INVALID) records[previous].surfaces = word(records[previous].surfaces - 1);
            if (graph !== GRAPH_INVALID) records[graph].surfaces = word(records[graph].surfaces + 1);
            this.#writeNumber(current, graph);
            const walk = new RawSquidNeighborWalk(neighbors, current, NEIGHBOR_FLAG.surfaceMap);
            // Next 마다 현재 지도/후보를 읽는다. 선택된 진단이 슬롯/스택을 고친 뒤에도 순서를 계속한다.
            for (let candidate = walk.current; candidate; candidate = walk.next()) {
                if (this.#readNumber(candidate) === graph) continue;
                if (this.#checking() && candidate === current) this.#diagnose('cFinder.s().getSid() != sid', 0x176);
                if (depth >= stack.length) {
                    this.#diagnose('d < FLOOD_STACK_SIZE', 0x17a);
                    throw new RangeError('Graph flood의 원문 물리 stack 범위를 넘습니다');
                }
                stack[depth] = candidate;
                depth += 1;
                if (depth >= stack.length) this.#diagnose('d < FLOOD_STACK_SIZE', 0x17a);
            }
        }
        this.#traceInUse();
        checkGraphSentinel(records[GRAPH_INVALID], this.#diagnostics);
        return changed;
    }

    // 재구성은 초기 sentinel 진단 뒤 필요한 Reset, SID 순서 두 순회, 임시 행 해제와 마지막 sentinel 순서이다.
    #executeRebuild(resetAll) {
        const records = this.#storage.records;
        checkGraphSentinel(records[GRAPH_INVALID], this.#diagnostics);
        let exhausted = true;
        // 정상 번호 251개가 모두 사용 중인지 본다.
        for (let index = 0; index < GRAPH_COUNT; index += 1) if (!records[index].inUse) { exhausted = false; break; }
        this.#calculate(0, !resetAll && exhausted ? OP.allocate : OP.rebuild, resetAll ? 1 : 0, null);
        if (resetAll) resetRawGraphTable(this.#storage);
        const temporary = this.#executeAllocate(true);
        const firstLimit = this.#pool.capacity;
        // 원문 iterator 는 초기 1 에서 Next 선증가로 2 부터 시작하고 free 만 건너뛴다. surface 의 void/dead/contained 는 제외하지 않는다.
        for (let id = 2; id < firstLimit; id += 1) {
            const raw = this.#raw(id);
            if (raw[STATE_OFFSET] & FREE) continue;
            if (!isSurface(this.#currentType(raw[TYPE_OFFSET]))) continue;
            if (!resetAll && this.#readNumber(id) !== GRAPH_INVALID) continue;
            const x = floatOf(raw, X_OFFSET);
            const y = floatOf(raw, Y_OFFSET);
            if (!Number.isFinite(x) || !Number.isFinite(y) || x < 0 || y < 0 || x >= WORLD_CELLS || y >= WORLD_CELLS) throw new RangeError('Graph 재구성 spot 좌표 범위 오류');
            if (!(this.#spots[Math.trunc(y) * WORLD_CELLS + Math.trunc(x)] & 8)) {
                this.#writeNumber(id, temporary);
                records[temporary].surfaces = word(records[temporary].surfaces + 1);
            }
        }
        const secondLimit = this.#pool.capacity;
        // 다시 초기 1 → Next 로 2 부터 읽고 앞 flood 가 바꾼 현재 번호를 확인해 다음 성분의 번호를 할당한다.
        for (let id = 2; id < secondLimit; id += 1) {
            const raw = this.#raw(id);
            if (!(raw[STATE_OFFSET] & FREE) && isSurface(this.#currentType(raw[TYPE_OFFSET])) && this.#readNumber(id) === temporary) {
                const number = this.#executeAllocate(true);
                this.#executeFlood(id, number);
            }
        }
        records[temporary].surfaces = 0;
        records[temporary].inUse = 0;
        checkGraphSentinel(records[GRAPH_INVALID], this.#diagnostics);
    }

    // 원문 진단 후 현재 이웃 번호를 수집하고 가장 큰 첫 연결에 붙인 뒤 나머지 현재 무리를 병합한다.
    #executeAdd(sid) {
        const records = this.#storage.records;
        checkGraphSentinel(records[GRAPH_INVALID], this.#diagnostics);
        const raw = this.#raw(sid);
        const x = floatOf(raw, X_OFFSET);
        const y = floatOf(raw, Y_OFFSET);
        const valid = Number.isFinite(x) && Number.isFinite(y) && x > 0 && y > 0 && x < WORLD_CELLS && y < WORLD_CELLS;
        if (this.#checking() && !valid) this.#diagnose('pos.isValid()', 0xbd);
        if (!valid) return;
        if (this.#checking() && (this.#raw(sid)[STATE_OFFSET] & 7)) this.#diagnose('!s.isVoid() && !s.isDead() && !s.isFree()', 0xc2);
        if (this.#raw(sid)[STATE_OFFSET] & 7) return;
        this.#calculate(sid, OP.inspect, 0, null);
        const walk = new RawSquidNeighborWalk(this.#neighbors(), sid, NEIGHBOR_FLAG.surfaceMap);
        const connections = new Array(MAXIMUM_CONNECTIONS).fill(0);
        let count = 0;
        let biggestSize = -2147483647;
        let biggest = 0;
        // 원문은 번호를 snapshot 하지 않고 이미 넣은 SID 의 현재 graph BYTE 와 중복을 비교한다. 크기 초기값은 0x80000001 이며 동률에서는 첫 연결을 유지한다.
        for (let candidate = walk.current; candidate; candidate = walk.next()) {
            const number = this.#readNumber(candidate);
            if (number === GRAPH_INVALID) continue;
            this.validateFree(number);
            let duplicate = false;
            // 이미 넣은 SID 들의 현재 번호와 비교한다.
            for (let prior = 0; prior < count; prior += 1) {
                if (this.#readNumber(connections[prior]) === this.#readNumber(candidate)) { duplicate = true; break; }
            }
            if (duplicate) continue;
            if (count === connections.length) throw new RangeError('Graph 연결 목록의 원문 물리 범위를 넘습니다');
            connections[count] = candidate;
            count += 1;
            const current = this.#readNumber(candidate);
            this.validateFree(current);
            if (records[current].surfaces > biggestSize) {
                biggest = current;
                biggestSize = records[current].surfaces;
            }
        }
        if (!count) {
            const number = this.#executeAllocate(this.#recovery === GRAPH_RECOVERY.fullPool);
            if (records[number].inUse !== 1) this.#diagnose('g->inUse == true', 0xf3);
            this.#executeFlood(sid, number);
        } else {
            if (this.#checking() && records[biggest].inUse !== 1) this.#diagnose('g->inUse == true', 0x100);
            if (!records[biggest].inUse) records[biggest].inUse = 1;
            this.#writeNumber(sid, biggest);
            records[biggest].surfaces = word(records[biggest].surfaces + 1);
            if (count > 1) {
                let flooded = 0;
                // 같은 지역 배열 64칸을 순회하며 앞 flood/진단/로그가 바꾼 뒤 후보 번호를 다시 읽는다.
                for (const candidate of connections) {
                    if (!candidate || this.#readNumber(candidate) === biggest) continue;
                    const loser = this.#readNumber(candidate);
                    if (loser === GRAPH_INVALID) throw new RangeError('Graph 병합의 원문 loser NULL 행입니다');
                    this.validateFree(loser);
                    if (this.#checking() && records[loser].surfaces > biggestSize) this.#diagnose('loser->numSurfaces <= biggestSize', 0x113);
                    this.#executeFlood(candidate, biggest);
                    flooded += 1;
                    records[loser].surfaces = 0;
                    records[loser].inUse = 0;
                }
                if (this.#checking() && flooded !== count - 1) this.#diagnose('numFlooded == numConnections - 1', 0x11f);
            }
        }
        this.#traceInUse();
    }

    // 일반 finder 는 다음 raw 체인을 먼저 저장한다. Add 의 진단/로그 뒤 다음 후보의 현재 필드를 다시 읽으며 표면만 Add 한다.
    #executePostDestroy(sid) {
        this.validatePostDestroy(sid);
        const raw = this.#raw(sid);
        const type = this.#currentType(raw[TYPE_OFFSET]);
        const x = floatOf(raw, X_OFFSET);
        const y = floatOf(raw, Y_OFFSET);
        if (!Number.isFinite(x) || !Number.isFinite(y) || x < 0 || y < 0 || x >= WORLD_CELLS || y >= WORLD_CELLS) throw new RangeError('Graph postDestroy 좌표 범위 오류');
        const left = clamp(Math.trunc(x) - type.footX + 1, 1, 255);
        const top = clamp(Math.trunc(y) - type.footY + 1, 1, 255);
        const area = { left, top, right: x > 0 && y > 0 ? Math.trunc(x) : left, bottom: x > 0 && y > 0 ? Math.trunc(y) : top };
        const finder = new RawSquidFinder(this.#pool, this.#hash, this.#currentTypeList());
        // 이전 Add 가 고친 후보 타입/상태와 현재 hash/next 를 읽으며 표면만 Add 한다.
        for (let candidate = finder.begin(area); candidate; candidate = finder.next()) {
            if (isSurface(this.#currentType(this.#raw(candidate)[TYPE_OFFSET]))) this.add(candidate);
        }
    }

    // 삭제 분할 실행이다. 원문 삭제 정보는 sentinel 전 인자 값이며, 사전 검사는 기존 안전 계약만 유지하고 효과를 commit 하지 않는다.
    #executeDetach(sid, rebuild, removedSurfaces, sourceType) {
        if (!this.#detachEnabled()) return;
        const records = this.#storage.records;
        this.validateDetach(sid, this.#detachRebuild ? Boolean(this.#detachRebuild()) : rebuild, removedSurfaces, sourceType);
        const neighbors = this.#neighbors();
        const source = neighbors.source(sid);
        if (sourceType !== null) source.type = sourceType;
        checkGraphSentinel(records[GRAPH_INVALID], this.#diagnostics);
        if (!this.#detachEnabled()) return; // GetGraph(position)도 현재 활성 전역을 다시 읽는다.
        const head = rawPathSurfaceAt(this.#hash, source.x, source.y);
        if (!head) return;
        const number = this.#readNumber(head);
        if (number === GRAPH_INVALID) return;
        this.validateFree(number);
        if (!records[number].inUse) return;
        const walk = new RawSquidNeighborWalk(neighbors, source, 0, true);
        const connections = new Array(MAXIMUM_CONNECTIONS).fill(0);
        let count = 0;
        // 원문은 그래프별 중복 제거 없이 일반 finder 의 현재 SID 를 모두 저장한다.
        for (let candidate = walk.current; candidate; candidate = walk.next()) {
            if (count === connections.length) throw new RangeError('Graph 삭제 연결의 원문 물리 범위를 넘습니다');
            connections[count] = candidate;
            count += 1;
            if (count >= connections.length) this.#diagnose('numConnections < MAX_CONNECTIONS', 0x1ef);
        }
        if (this.#clock) this.#clock();
        // keep 의 초기값은 현재 전역을 한 번 읽지만 각 연결의 분할 조건은 Flood 뒤 다시 읽는다.
        const rebuilding = () => (this.#detachRebuild ? Boolean(this.#detachRebuild()) : rebuild);
        let keep = !rebuilding();
        let remaining = count;
        // 후보 graph 는 앞 flood/로그/진단이 고친 현재 슬롯에서 읽으며 초기 목록의 순서는 보존한다.
        for (let index = 0; index < count; index += 1, remaining -= 1) {
            const candidate = connections[index];
            if (!candidate) this.#diagnose('floodSid != 0', 0x20b);
            let current = 0;
            if (!candidate) {
                if (this.#checking()) this.#diagnose('mySid', 0x93);
            } else if (isSurface(this.#currentType(this.#raw(candidate)[TYPE_OFFSET]))) {
                current = this.#readNumber(candidate);
            } else {
                const raw = this.#raw(candidate);
                const surface = rawPathSurfaceAt(this.#hash, floatOf(raw, X_OFFSET), floatOf(raw, Y_OFFSET));
                current = surface ? this.#readNumber(surface) : GRAPH_INVALID;
            }
            this.validateFree(current);
            if (current !== number) continue;
            if (!rebuilding() && remaining < 2) {
                if (!(this.#currentType(source.type).flags2 & 8)) keep = true;
            } else {
                const allocated = this.#executeAllocate(this.#recovery === GRAPH_RECOVERY.fullPool);
                this.#executeFlood(candidate, allocated);
            }
        }
        if (count) records[number].surfaces = word(records[number].surfaces - removedSurfaces);
        if (!keep) {
            records[number].surfaces = 0;
            records[number].inUse = 0;
        }
        if (this.#clock) this.#clock();
        checkGraphSentinel(records[GRAPH_INVALID], this.#diagnostics);
    }
}
