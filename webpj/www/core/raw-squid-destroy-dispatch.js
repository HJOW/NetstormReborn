// 다리 가상 삭제(vtable +0x10 재정의, 004220f0)를 공통 destroy 앞에 연결하는 모듈이다(cpppj `o/RawSquidDestroyDispatch.cpp/.h`, 10.78 전용).
// 타입 번호 대신 실제 raw 가상 표(다리 0x005034c8)로 재정의를 고른다. 다리이고 편집기가 아니며 권한이 있고 abstract/buried(extra 9)가 아닐 때만 Graph 번호와 현재 프레임을 읽는다:
//   Graph 표면 수가 5 이상이면 프레임 코드의 단단한 플래그(0x40)나 디버그 유지가 삭제를 막는다(거부 시 dead·장부·표시·프로세스를 바꾸지 않고 false). 그 밖은 공통 `RawSquidDestroy.destroy` 로 넘기고 true 를 돌려준다.
// 단락 조건(편집기·권한·abstract/buried)이 통과하지 못하면 무효 Graph 번호(254)나 프레임 범위를 읽지 않는다. 통과했는데 Graph 번호 254/표 밖이거나 프레임 범위 밖이면 원본의 NULL/범위 밖 접근을 쓰기 전 오류로 남긴다.
// 현재 서버/편집기 전역은 `bindCurrentModes` 로 호출마다 읽게 할 수 있다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-10 WB04-E214: 다리 가상 삭제 분배 이식으로 추가)

import { GRAPH_INVALID } from './graph.js';
import { RawSquidDestroy } from './squid-destroy.js';
import { BRIDGE_MINIMUM_GRAPH_SURFACES, bridgeDestroyProceeds } from './bridge-decay.js';
import { BRIDGE_VTABLE } from './raw-bridge-events.js';

// 슬롯의 가상 표/그래프 번호/프레임/extra 위치이다(10.78). (2026-11-10 WB04-E214)
const GRAPH_OFFSET = 30;
const FRAME_OFFSET = 36;
const EXTRA_OFFSET = 40;
const TYPE_OFFSET = 10;

// 슬롯의 리틀엔디언 DWORD 이다. 주변 필드를 섞지 않고 네 바이트만 조합한다. (2026-11-10 WB04-E214)
function dword(raw, offset) {
    return (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0;
}

// 다리 가상 삭제 분배기이다. base 는 공통 삭제기(RawSquidDestroy), neighbors 는 같은 풀의 RawSquidNeighbors(프레임 코드 조회), mode 는 { authority, editor, debugKeep, … } 다리 모드 객체(변경은 다음 호출부터 반영),
// graph 는 같은 풀의 RawGraph(없으면 비활성 표의 표면 수 0)이다. 참조 대상은 분배기보다 오래 살아야 한다.
// 사용: const dispatch = new RawSquidDestroyDispatch(destroy, neighbors, mode, graph); if (!dispatch.destroy(sid, flags, hooks)) { /* 삭제 거부 */ }. (2026-11-10 WB04-E214)
export class RawSquidDestroyDispatch {
    // 공통 삭제기·프레임 조회기·모드·Graph 이다.
    #base;
    #neighbors;
    #mode;
    #graph;
    // 호출마다 현재 boss 를 읽는 권한 소유자({ boss })와 현재 편집기 DWORD getter 이다(없으면 mode 의 값을 쓴다).
    #authority = null;
    #editor = null;

    // 삭제 몸체와 프레임 탐색기·Graph 가 같은 풀/공간을 참조하는지 구성 때 확인한다. (2026-11-10 WB04-E214)
    constructor(base, neighbors, mode, graph = null) {
        if (!(base instanceof RawSquidDestroy) || base.pool !== neighbors?.pool) throw new TypeError('가상 삭제/프레임 SID 풀이 다릅니다');
        if (graph) {
            if (graph.pool !== base.pool) throw new TypeError('가상 삭제/Graph SID 풀이 다릅니다');
            base.validateGraph(graph);
            graph.validateTypes(neighbors.types());
        }
        this.#base = base;
        this.#neighbors = neighbors;
        this.#mode = mode;
        this.#graph = graph;
    }

    // 현재 장기 전역만 빌리고 풀/Graph/모드를 초기화하지 않는다. authority 는 { boss } 소유자, editor 는 현재 DWORD 를 돌려주는 함수(null 이면 mode 의 값 유지)이다. (2026-11-10 WB04-E214)
    bindCurrentModes(authority, editor = null) {
        if (typeof authority?.boss !== 'boolean' || (editor !== null && typeof editor !== 'function')) throw new TypeError('가상 삭제의 현재 권한/편집기 전역 형식 오류');
        this.#authority = authority;
        this.#editor = editor;
    }

    // 004220f0: false 는 다리 재정의의 삭제 거부이다. true 는 공통 몸체로 전달했음을 뜻하며 이미 dead 이면 몸체가 반환한다. 원본이 공통 destroy 를 직접 부르는 지점은 RawSquidDestroy.destroy 를 사용한다.
    // 거부 시 풀·표·스택·콜백·깊이를 바꾸지 않는다. (2026-11-10 WB04-E214)
    destroy(sid, flags, hooks) {
        const raw = this.#base.pool.slot(sid);
        if (dword(raw, 0) === BRIDGE_VTABLE) {
            const extra = raw[EXTRA_OFFSET];
            // 외부 효과 뒤 이 진입에 도달한 현재 두 전역을 읽는다.
            const current = { ...this.#mode };
            if (this.#authority) current.authority = this.#authority.boss;
            if (this.#editor) current.editor = this.#editor() !== 0;
            // editor/권한/abstract/buried 조건이 통과할 때만 그래프 번호와 현재 프레임을 읽는다.
            if (!current.editor && current.authority && (extra & 9) === 0) {
                let surfaces = 0;
                if (this.#graph) {
                    const number = raw[GRAPH_OFFSET];
                    // 원본은 254 조회의 NULL 을 역참조한다. 호스트에서는 쓰기 전 예외로 표시한다.
                    if (number === GRAPH_INVALID) throw new Error('가상 다리 삭제의 무효 Graph 번호');
                    const records = this.#graph.records();
                    if (number >= records.length) throw new RangeError('가상 다리 삭제의 Graph 표 범위 오류');
                    surfaces = records[number].surfaces;
                }
                if (surfaces >= BRIDGE_MINIMUM_GRAPH_SURFACES) {
                    const frame = dword(raw, FRAME_OFFSET) | 0;
                    const table = this.#neighbors.frames(raw[TYPE_OFFSET]);
                    const codes = table.codes ?? table;
                    if (frame < 0 || frame >= codes.length) throw new RangeError('가상 다리 삭제 프레임 범위 오류');
                    if (!bridgeDestroyProceeds(current, extra, surfaces, codes[frame].flags)) return false;
                }
            }
        }
        this.#base.destroy(sid, flags, hooks);
        return true;
    }
}
