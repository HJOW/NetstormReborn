// 공통 pre/postDestroy 의 비전투 장부 처리를 옮긴 모듈이다(cpppj `o/SquidDestroyLifecycle.cpp/.h`, 10.78 전용: preDestroy 004b0950·postDestroy 004b0840).
// preDestroy 순서: 선택 복구(배치 기억·선택 해제) → Graph 분할/Free → (장부 억제가 아니면) 소유자 작업장 제거 → 보상 → 현재 타입 수·전역 수 감소 → 소리/손실 좌표 → 공급원 제거 → 현재 AI 통지 → 전체 작업장 제거 → 깊이 감소.
// postDestroy 순서: 주변 표면 Add(Graph) → ordinary 이면 전역 비용 차감 → 깊이 감소. 모든 효과 사이에서 raw 슬롯의 현재 TYPE/상태를 다시 읽고 캐시하지 않는다.
// 외부 효과(UI 선택 해제·보상·유닛 손실 소리)와 종속 가상 효과는 호출자 장치(emit/forward)이며, 선택된 장치가 없으면 해당 위치에서 오류다(빈 성공으로 채우지 않는다).
// 이 모듈이 옮긴 판은 기준선(`1f00a25a`)의 독립 typed 경로다. 기준선 뒤 cpppj 3커밋이 더한 실제 PATCH(Native) 전용 분기 — 현재 전역 getter(selectedRaw·localOwner·loading/fort/silent/surface 현재 DWORD)·
// 실제 PATCH 진단(`!isForm()` 0x771)·DWORD 감김 압축 제거·현재 TYPE 표 빌림·최초 로딩 비용 표·선행 검사 생략 — 은 옮기지 않았다(LEFT_JOBS.webpj.md 3.6절 ③ 표). CD 10.72 는 웹 범위 밖이다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-10 WB04-E213: 공통 삭제 장부 이식으로 추가)

import { TypeFlag1 } from '../assets/type-flags.js';
import { SQUID_DESTROY_EFFECT } from './squid-destroy.js';

// 삭제 장부가 호출자에게 요청하는 외부 효과 이름이다: 선택 해제 UI·삭제 보상(SP/지갑)·유닛 손실 소리. (2026-11-10 WB04-E213)
export const SQUID_DELETION_EFFECT = Object.freeze({
    clearSelection: 'clearSelection',
    refund: 'refund',
    unitLostSound: 'unitLostSound',
});

// 슬롯의 TYPE/상태/좌표 위치와 첫 자산 TYPE, 상태 비트(free+contained)이다. (2026-11-10 WB04-E213)
const TYPE_OFFSET = 10;
const STATE_OFFSET = 11;
const X_OFFSET = 14;
const Y_OFFSET = 18;
const OWNER_OFFSET = 34;
const EXTRA_OFFSET = 40;
const GRAPH_OFFSET = 30;
const FIRST_ASSET_TYPE = 70;
const INVALID_STATE = 9;
const TYPE_COUNT = 188;
const PLAYER_COUNT = 8;

// 타입 분류 비트와 삭제 flags 이다: 작업장 마스크(0x4200)·공급원(0x10000000)·배치 기억(0x30000), 보상 없음(0x200000)·Graph 없음(0x2000000)·Graph 해제(0x800)·소리 제외 분류(0x200000). (2026-11-10 WB04-E213)
const FACTORY_MASK = 0x4200;
const PROVIDER = 0x10000000;
const PLACEMENT = 0x30000;
const NO_REFUND = 0x200000;
const NO_GRAPH = 0x2000000;
const DESTROY_GRAPH = 0x800;
const SILENT_GENUS = 0x200000;

// 원본 타입 비용 표에 더하는 인코딩 계수이다. (2026-11-10 WB04-E213)
const COST_BIAS = 23;

// 프레임 방향 문자 'H' 의 코드이다(특수 표면 삭제 분기). (2026-11-10 WB04-E213)
const SIDE_H = 72;

// float32 비트 변환 도구이다. (2026-11-10 WB04-E213)
const floatView = new DataView(new ArrayBuffer(4));

// 비정렬 raw 좌표 DWORD 를 단정도 float 로 읽는다. 호스트 엔디언/정렬을 결과에 섞지 않는다. (2026-11-10 WB04-E213)
function coordinate(raw, offset) {
    floatView.setUint32(0, (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0, true);
    return floatView.getFloat32(0, true);
}

// 선택된 외부 효과가 없으면 그 위치에서 거부한다. (2026-11-10 WB04-E213)
function need(device, message) {
    if (typeof device !== 'function') throw new TypeError(message);
    return device;
}

// 삭제 처리 중 조회·갱신하는 상태 묶음이다. selected 는 현재 선택 SID(0 이면 없음), placementType/X/Y 는 abstract/buried 선택 객체가 남기는 배치 기억(0 이면 없음),
// lostX/lostY 는 최근 손실 좌표(sharedLostX/Y 가 { value } 로 있으면 장기 저장소를 직접 읽고 쓴다), unitLostSuppressed1/2 는 두 소리/좌표 기록 억제 전역, silentType 은 소리 제외 TYPE 이다.
// 사용: const state = new SquidDeletionState(); state.selected = 5; state.unitLostSuppressed1 = true. (2026-11-10 WB04-E213)
export class SquidDeletionState {
    // 현재 선택 SID 이다(0 이면 선택 없음).
    selected = 0;
    // abstract/buried 선택 객체의 배치 기억 TYPE 과 좌표이다. 0 이면 없다.
    placementType = 0;
    placementX = 0;
    placementY = 0;
    // 최근 손실 좌표(독립 저장). 원본 float 비트 폭을 유지하려고 쓰기는 단정도로 반올림한다.
    lostX = 0;
    lostY = 0;
    // 소리/좌표 기록의 두 억제 전역이다.
    unitLostSuppressed1 = false;
    unitLostSuppressed2 = false;
    // 소리만 제외하는 TYPE 번호이다(PATCH 레인 바리케이드).
    silentType = 0;
    // postDestroy 가 Graph 영역 갱신을 판단하는 대상 전역 마스크(두 PE 초기값 0x50444200, 실제 분기는 8 을 OR)이다.
    regionMask = 0x50444200;
    // 원본 삭제 분할 정책 전역이다(Graph 소진 복구와 구별).
    rebuildGraph = false;
    // 특수 H 치환·9 감소 타입 전역이다.
    specialSurfaceType = 157;
    specialSurfaceReplacementType = 162;
    largeSurfaceType = 162;
    // 장기 손실 좌표 저장소({ value } 상자)이다. null 이면 lostX/lostY 를 쓴다.
    sharedLostX = null;
    sharedLostY = null;

    // 손실 X 의 현재 값이다. 소리 효과 뒤 현재 저장소를 읽는다. (2026-11-10 WB04-E213)
    readLostX() { return this.sharedLostX ? this.sharedLostX.value : this.lostX; }

    // 손실 X 를 단정도로 기록한다. (2026-11-10 WB04-E213)
    writeLostX(value) { if (this.sharedLostX) this.sharedLostX.value = Math.fround(value); else this.lostX = Math.fround(value); }

    // 손실 Y 의 현재 값이다. X 와 Y 는 별개의 실제 주소에 보존한다. (2026-11-10 WB04-E213)
    readLostY() { return this.sharedLostY ? this.sharedLostY.value : this.lostY; }

    // 손실 Y 를 단정도로 기록한다. (2026-11-10 WB04-E213)
    writeLostY(value) { if (this.sharedLostY) this.sharedLostY.value = Math.fround(value); else this.lostY = Math.fround(value); }
}

// 공통 삭제 장부이다. pool 은 10.78 SidPool, types 는 188칸 TYPE 표(group·cost·flags1·flags2; 생성 때 사본을 만들어 입력을 고정), bookkeeping 은 Pop 과 같은 SquidPostPopState, state 는 SquidDeletionState,
// destroy 는 같은 풀의 RawSquidDestroy, hooks = { emit(event: { effect, sid, recipient }), forward(event: 종속 가상 효과) }, graph 는 같은 풀의 RawGraph(없으면 Graph 효과가 필요한 삭제를 거부),
// reward 는 같은 풀·장부의 SquidReward(있으면 보상 이벤트를 내보내지 않고 실제 SP/지갑 계산을 한다)이다. 풀·삭제기·상태의 수명은 이 객체보다 길어야 한다.
// 사용: const lifecycle = new SquidDestroyLifecycle(pool, types, book, state, destroy, { emit, forward }); destroy.destroy(sid, flags, lifecycle.hooks()). (2026-11-10 WB04-E213)
export class SquidDestroyLifecycle {
    // 풀·타입 사본·장부·상태·삭제기·외부 효과·Graph·보상 연결이다.
    #pool;
    #types;
    #book;
    #state;
    #destroy;
    #hooks;
    #graph;
    #reward;
    // 공급 목록 제거 뒤·전체 작업장 제거 앞의 현재 Player/brain 삭제 통지(소유자, SID, false)이다. 없으면 AI 부착 소유자의 삭제를 거부한다.
    #aiNotice = null;

    // 판본별 타입 수와 풀/Graph/보상 연결을 확인한다. (2026-11-10 WB04-E213)
    constructor(pool, types, bookkeeping, state, destroy, hooks = {}, graph = null, reward = null) {
        if (pool?.edition !== 'patch1078' || !Array.isArray(types) || types.length !== TYPE_COUNT) throw new RangeError('삭제 훅 타입 판본/크기 오류');
        if (destroy?.pool !== pool) throw new TypeError('삭제 훅 SID 풀이 다릅니다');
        this.#pool = pool;
        this.#types = types.map((record) => ({ ...record }));
        this.#book = bookkeeping;
        this.#state = state;
        this.#destroy = destroy;
        this.#hooks = hooks ?? {};
        this.#graph = graph;
        this.#reward = reward;
        if (graph) {
            if (graph.pool !== pool) throw new TypeError('삭제 훅/Graph SID 풀이 다릅니다');
            graph.validateTypes(types);
            destroy.validateGraph(graph);
        }
        // 보상은 같은 풀과 같은 장부(로컬 소유자/AI 부착)를 읽어야 지급 대상이 어긋나지 않는다.
        if (reward && (reward.pool !== pool || reward.bookkeeping !== bookkeeping)) throw new TypeError('삭제 훅/보상 풀 또는 장부가 다릅니다');
    }

    // 루트 자산에만 공통 장부 처리를 제공한다. 현재 슬롯을 매번 읽으며 free/contained/예약 SID/Form TYPE/TYPE 표 밖은 거부한다(form/contained 의 파생 메서드는 별도). (2026-11-10 WB04-E213)
    #object(sid) {
        const raw = this.#pool.slot(sid);
        if (sid < 5 || sid === this.#pool.layout.predictableFirst || (raw[STATE_OFFSET] & INVALID_STATE) || raw[TYPE_OFFSET] < FIRST_ASSET_TYPE || raw[TYPE_OFFSET] >= this.#types.length) {
            throw new Error('삭제 훅 raw 자산 상태 오류');
        }
        return raw;
    }

    // 앞선 외부 콜백이 raw 타입을 바꿨을 때 후속 타입 조회를 캐시하지 않는다. (2026-11-10 WB04-E213)
    #type(sid) { return this.#types[this.#object(sid)[TYPE_OFFSET]]; }

    // 소유자 BYTE(+34)이다. (2026-11-10 WB04-E213)
    #owner(sid) { return this.#object(sid)[OWNER_OFFSET]; }

    // abstract 또는 buried(extra 9)이면 글로벌 장부/비용 경로를 생략한다. 소유자 목록 제거와 구별한다. (2026-11-10 WB04-E213)
    #ordinary(sid) { return !(this.#object(sid)[EXTRA_OFFSET] & INVALID_STATE); }

    // 원본 unitLost 조건이다. 소리 제외 TYPE 은 좌표 기록과 별도로 소리에만 적용한다. (2026-11-10 WB04-E213)
    #lostNotice(sid, flags) {
        return this.#type(sid).group !== 10 && !(flags & (NO_REFUND | DESTROY_GRAPH)) && this.#owner(sid) === this.#book.localOwner &&
            !(this.#type(sid).flags2 & SILENT_GENUS) && !this.#state.unitLostSuppressed1 && !this.#state.unitLostSuppressed2;
    }

    // postDestroy 의 x87 뺄셈/0 방향 절삭을 low DWORD 로 보존한 다음 전역 비용이다. 비유한 비용·패치 비용 표 범위·누적 범위 밖은 거부한다. (2026-11-10 WB04-E213)
    #remainingCost(sid) {
        const number = this.#object(sid)[TYPE_OFFSET];
        const cost = Math.fround(this.#types[number].cost ?? 0);
        let decoded = cost;
        if (!Number.isFinite(cost)) throw new TypeError('삭제 훅 유한 비용이 필요합니다');
        const bias = number * COST_BIAS;
        const encoded = cost + bias;
        if (encoded < -2147483648 || encoded > 2147483647) throw new RangeError('삭제 훅 패치 비용 표 범위 오류');
        decoded = (Math.trunc(encoded) - bias) | 0;
        const result = this.#book.totalCost - decoded;
        if (result < -4294967296 || result > 4294967295) throw new RangeError('삭제 훅 비용 누적 범위 오류');
        return Math.trunc(result) | 0;
    }

    // 선택 해제의 외부 UI 경계와 논리 선택 번호를 함께 처리한다. (2026-11-10 WB04-E213)
    #clearSelection(sid) {
        need(this.#hooks.emit, '삭제 훅 선택 해제 콜백 누락')({ effect: SQUID_DELETION_EFFECT.clearSelection, sid, recipient: 0 });
        this.#state.selected = 0;
    }

    // 선택 처리 뒤·장부 억제 앞의 실제 분할/Free 이다. 0x800 Free 를 noGraph 보다 먼저 판단하며 buried(extra 8)만 Graph 효과를 생략한다. validate 이면 변경하지 않는다. (2026-11-10 WB04-E213)
    #preGraph(sid, flags, validate) {
        const raw = this.#object(sid);
        if (!this.#book.graphsEnabled || (raw[EXTRA_OFFSET] & 8) || !(this.#type(sid).flags1 & TypeFlag1.surface) || (!(flags & DESTROY_GRAPH) && (flags & NO_GRAPH))) return;
        if (!this.#graph) throw new Error('공통 preDestroy Graph 효과 미연결');
        const graph = this.#graph;
        if (flags & DESTROY_GRAPH) {
            const number = raw[GRAPH_OFFSET];
            if (validate) graph.validateFree(number); else graph.free(number);
            return;
        }
        let sourceType = null;
        if (raw[TYPE_OFFSET] === this.#state.specialSurfaceType) {
            if (graph.frame(sid).side !== SIDE_H) return;
            sourceType = this.#state.specialSurfaceReplacementType;
        }
        const removed = (sourceType ?? raw[TYPE_OFFSET]) === this.#state.largeSurfaceType ? 9 : 1;
        if (validate) graph.validateDetach(sid, this.#state.rebuildGraph, removed, sourceType);
        else graph.detach(sid, this.#state.rebuildGraph, removed, sourceType);
    }

    // postDestroy 의 주변 표면 Add 가 필요한가. noGraph 는 pre 분할만 억제하며 post Add 는 임의로 억제하지 않는다. (2026-11-10 WB04-E213)
    #needsPostGraph(sid, flags) {
        return Boolean(this.#book.graphsEnabled && !(this.#object(sid)[EXTRA_OFFSET] & 8) && !(flags & DESTROY_GRAPH) && (this.#type(sid).flags2 & (this.#state.regionMask | 8)));
    }

    // 손상 목록·미연결 Graph/AI/보상/소리 효과·비유한 비용을 장부 쓰기 전에 무효과로 거부한다(선택/Graph 는 통계 억제보다 먼저 실행되므로 그 분기대로 확인). (2026-11-10 WB04-E213)
    validatePre(sid, flags) {
        const raw = this.#object(sid);
        const type = this.#type(sid);
        const book = this.#book;
        if (this.#state.selected === sid && !this.#hooks.emit) throw new TypeError('삭제 훅 선택 해제 콜백 누락');
        this.#preGraph(sid, flags, true);
        if (book.suppressed) return;
        const owner = this.#owner(sid);
        if (owner > PLAYER_COUNT) throw new RangeError('삭제 훅 소유자 범위 오류');
        if (type.flags2 & FACTORY_MASK) { if (owner) book.ownerFactories[owner].items(); }
        if (!this.#ordinary(sid)) return;
        if (!this.#aiNotice && owner && book.aiAttached[owner]) throw new Error('삭제 훅 AI 통지 미복원');
        // 보상 객체가 있으면 계산 입력 오류를 쓰기 전에 거부하고, 없으면 외부 보상 콜백이 필요하다.
        if (!(flags & NO_REFUND)) {
            if (this.#reward) this.#reward.plan(sid, (flags >>> 16) & 15, false);
            else if (!this.#hooks.emit) throw new TypeError('삭제 훅 보상 콜백 누락');
        }
        if (this.#lostNotice(sid, flags) && raw[TYPE_OFFSET] !== this.#state.silentType && !this.#hooks.emit) throw new TypeError('삭제 훅 소리 콜백 누락');
        if (type.flags1 & PROVIDER) book.currentProviders().items();
        if (type.flags2 & FACTORY_MASK) book.currentFactories().items();
    }

    // postDestroy 의 비용 효과는 억제와 무관하며 Graph 분기를 앞서 거부한다. (2026-11-10 WB04-E213)
    validatePost(sid, flags) {
        this.#object(sid);
        if (this.#needsPostGraph(sid, flags)) {
            if (!this.#graph) throw new Error('공통 postDestroy Graph 효과 미연결');
            this.#graph.validatePostDestroy(sid);
        }
        if (this.#ordinary(sid)) this.#remainingCost(sid);
    }

    // 004b0950: 끝에서 기존 공통 삭제기의 pre 깊이를 감소시킨다. (2026-11-10 WB04-E213)
    preDestroy(sid, flags) {
        this.validatePre(sid, flags);
        const book = this.#book;
        const state = this.#state;
        if (state.selected === sid) {
            if (!this.#ordinary(sid) && (this.#type(sid).flags2 & PLACEMENT)) {
                const raw = this.#object(sid);
                state.placementX = coordinate(raw, X_OFFSET);
                state.placementY = coordinate(raw, Y_OFFSET);
                state.placementType = raw[TYPE_OFFSET];
                book.pendingPlacement = true;
            }
            this.#clearSelection(sid);
        }
        this.#preGraph(sid, flags, false);
        if (!book.suppressed) {
            if (this.#type(sid).flags2 & FACTORY_MASK) {
                const owner = this.#owner(sid);
                if (owner) book.ownerFactories[owner].removeAll(sid);
            }
            if (this.#ordinary(sid)) {
                if (!(flags & NO_REFUND)) {
                    // 원본 인자: sid, 플래그 16..19비트의 수신자, 마지막 0(삭제 보상 비율).
                    if (this.#reward) this.#reward.refund(sid, (flags >>> 16) & 15, false);
                    else this.#hooks.emit({ effect: SQUID_DELETION_EFFECT.refund, sid, recipient: (flags >>> 16) & 15 });
                }
                const number = this.#object(sid)[TYPE_OFFSET];
                if (this.#owner(sid) === book.localOwner) book.localCounts()[number] -= 1;
                // localSecondaryCounts 는 누적 생산 수이므로 줄이지 않는다.
                book.globalCounts()[number] -= 1;
                if (this.#lostNotice(sid, flags)) {
                    if (this.#object(sid)[TYPE_OFFSET] !== state.silentType) this.#hooks.emit({ effect: SQUID_DELETION_EFFECT.unitLostSound, sid, recipient: 0 });
                    const raw = this.#object(sid);
                    state.writeLostX(coordinate(raw, X_OFFSET));
                    state.writeLostY(coordinate(raw, Y_OFFSET));
                }
                if (this.#type(sid).flags1 & PROVIDER) {
                    book.currentProviders().removeAll(sid);
                    book.productionDirty = (book.productionDirty + 1) >>> 0;
                }
                // 현재 owner 의 실제 AI 통지는 공급 목록 뒤·전체 작업장 목록 앞이다.
                if (this.#aiNotice) this.#aiNotice(this.#owner(sid), sid, false);
                if (this.#type(sid).flags2 & FACTORY_MASK) {
                    book.currentFactories().removeAll(sid);
                    book.productionDirty = (book.productionDirty + 1) >>> 0;
                }
            }
        }
        this.#destroy.completePreDestroy();
    }

    // 004b0840: 주변 표면을 원본 순서로 다시 Add 하고 ordinary 비용을 마지막에 차감한 뒤 post 깊이를 감소시킨다. (2026-11-10 WB04-E213)
    postDestroy(sid, flags) {
        this.validatePost(sid, flags);
        if (this.#needsPostGraph(sid, flags)) this.#graph.postDestroy(sid);
        if (this.#ordinary(sid)) this.#book.totalCost = this.#remainingCost(sid);
        this.#destroy.completePostDestroy();
    }

    // 공통 destroy 의 실제 가상 훅이다. 공통 Pre/Post/선택 해제는 이 몸체로, 종속 release/destroy·전파는 forward 로 보낸다(미연결이면 그 경계에서 오류).
    // 파생 다리 훅은 BasePre/BasePost 에서 이 몸체를 호출한다. (2026-11-10 WB04-E213)
    hooks() {
        return {
            emit: (event) => {
                switch (event.effect) {
                case SQUID_DESTROY_EFFECT.preDestroy: this.preDestroy(event.sid, event.flags); break;
                case SQUID_DESTROY_EFFECT.postDestroy: this.postDestroy(event.sid, event.flags); break;
                case SQUID_DESTROY_EFFECT.clearSelection: this.#clearSelection(event.sid); break;
                default:
                    need(this.#hooks.forward, '삭제 훅 종속/전파 콜백 미연결')(event);
                }
            },
            selected: () => this.#state.selected,
        };
    }

    // 공급 목록 제거 뒤·전체 작업장 제거 앞의 현재 AI 삭제 통지를 연결한다. 함수가 아니면(null 포함) 연결을 해제한다. (2026-11-10 WB04-E213)
    bindPatchAiNotice(notice) {
        this.#aiNotice = typeof notice === 'function' ? notice : null;
    }
}
