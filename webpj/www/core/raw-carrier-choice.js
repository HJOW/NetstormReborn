// 골렘(Carrier)의 귀환·간헐천 후보 목록 수집(428ea0/429420)과 실제 경로 시각에 따른 후보 평가(4280a0)를 옮긴 모듈이다(cpppj `o/RawCarrierChoice.cpp/.h`, 10.78 전용).
// 경로를 찾지 못한 후보도 거친 거리 점수(임시 WORD)로 비교하는 원본 분기를 유지한다. 발자국·BFS·유효성은 훅이고 정렬은 E90 의 원본 비교기/CRT qsort 를 쓴다.
//  - evaluate(candidates, anchor, mover): mover 가 유효하면 이동 속도(movement)를 한 번 읽고 AI 가 있거나 비행(flags2 0x20000)이면 경로 탐색을 생략한다(처음에 고정). 첫 순회는 후보 목록을 원래 ring 으로 한 번씩 돌며(현재 count 를 매번 다시 읽음)
//    surface 후보는 진단(0xace)을 부른 뒤 다시 검사해 여전히 surface 면 건너뛰고, 아니면 기준점에서 후보 발자국으로의 두 접근점 사이 거리/속도(넓은 거리, _ftol 하위 WORD)를 후보 슬롯 +8 의 임시 WORD 로 쓴다.
//    이어 목록을 원본 CRT qsort 와 비교기로 정렬한다. 두 번째 순회는 임시 WORD 가 지금까지의 최소(float, 초기 10000)보다 작고 경로 비교가 켜졌으며 탐색 생략이 아닐 때만 새 임시 경로를 만들어 BFS(cardinal TYPE 이면 flags 1)를 부르고,
//    경로가 있으면 현재 시각으로 Prepare 해 `마지막 시각 − 현재 시각`을 새 점수로 하며 currentMetric 에 `속도×점수`(float 비트)를 기록한다. 엄격한 최소값만 채택하며 마지막에는 검사 전역이 켜지고 본 수가 현재 count 와 다르면 진단(0xb03)을 부르고
//    currentMetric 을 baseline DWORD 로 복사한다(float 변환 없음).
//  - chooseReturn(parent): 현재 Player+0x7c 의 집 신전(유효하면)을 선두에 넣고 별도 outposts 목록(nonzero 수만큼 ring 순회, 용량 500 상한·중복 보존)을 수집해 evaluate(후보, parent, parent)이다.
//  - chooseGeyser(parent): 먼저 귀환점을 고르고(물리 범위 안일 때만) Graph 조회(462bc0; 반환값은 쓰지 않지만 생략하지 않음) 뒤 현재 geysers 목록(여기서는 nonzero 수가 아니라 index 와 count 를 비교)을 그 점에 대해 evaluate 한다.
// state = CarrierChoiceState({ comparePaths, cardinalTypes[4], baselineBits, currentMetricBits }), players 는 9명 Player 바이트 표(0xb4 간격, +0x7c 집·+0xb0 AI), returns/geysers 는 SquidPostPopList, now 는 현재 시각 getter 또는 `{ value }`,
// checking 은 5e4794 진단 DWORD getter, board 는 RawBoardTarget, hooks = { movement(sid)→[speed, turning], footprint(sid)→사각형, search(path, sid, start, destination, flags), validReturn(parent, target), validGeyser(parent, target),
// graphFor(sid), assertion(line), sortAddresses?()→{ poolBase, arrayBase }(독립 관측의 주소 재배치; 게임 경로에서는 비움) }. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E102: 운반 후보 선택 이식으로 추가)
import { RawPathRuntime } from './raw-path-runtime.js';
import { RawCarrierCandidateSort } from './raw-carrier-candidate-sort.js';
import { SquidPostPopList } from './squid-postpop-state.js';

// 후보 평가가 읽는 슬롯 위치(TYPE·소유자·좌표·임시 WORD)와 Player 표 폭·위치, 목록 용량 상한, 판본의 TYPE 수이다. (2026-11-08 WB04-E102)
const TYPE_AT = 10;
const OWNER_AT = 34;
const X_AT = 14;
const Y_AT = 18;
const METRIC_AT = 8;
const PLAYER_COUNT = 9;
const PLAYER_STRIDE = 0xb4;
const PLAYER_HOME = 0x7c;
const PLAYER_AI = 0xb0;
const LIST_LIMIT = 500;
const TYPE_COUNT = 188;
// surface TYPE 플래그, 비행 genus, 초기 최소 점수와 ftol 의 signed64 불확정값 경계, 진단 번호이다. (2026-11-08 WB04-E102)
const SURFACE = 0x800;
const FLYER = 0x20000;
const INITIAL_BEST = 10000;
const INT64_BOUNDARY = 2 ** 63;
const ASSERT_SURFACE = 0xace;
const ASSERT_SEEN = 0xb03;
const fround = Math.fround;

// 선택된 실제 하위가 없으면 빈 효과나 성공값으로 바꾸지 않는다. (2026-11-08 WB04-E102)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('운반 후보의 실제 하위 미연결');
    return hook;
}

// 단정도 float 를 비트(uint32)로 바꾼다. (2026-11-08 WB04-E102)
function floatBits(value) {
    const view = new DataView(new ArrayBuffer(4));
    view.setFloat32(0, value, true);
    return view.getUint32(0, true);
}

// 41cfc0 의 넓은 거리/speed 와 _ftol 하위 WORD 이다(디컴파일 C 에 누락된 428210 의 FDIV 를 보존). (2026-11-08 WB04-E102)
function travelWord(first, second, speed) {
    const dx = first.x - second.x;
    const dy = first.y - second.y;
    const time = Math.sqrt(dy * dy + dx * dx) / speed;
    if (!Number.isFinite(time) || time >= INT64_BOUNDARY || time < -INT64_BOUNDARY) return 0;
    return Number(BigInt.asUintN(16, BigInt(Math.trunc(time))));
}

// 후보 평가가 읽는 현재 전역과 원본 임시 metric DWORD 이다. (2026-11-08 WB04-E102)
export class CarrierChoiceState {
    // 5316ac: AI/비행체가 아니면 거친 점수보다 나은 후보의 경로를 평가한다.
    comparePaths = true;
    // 488fb0 이 현재 타입 getter 네 개와 비교한다.
    cardinalTypes = [137, 156, 135, 187];
    // 540bf8/540bfc: 마지막에는 float 변환 없이 DWORD 를 복사한다.
    baselineBits = 0x461c4000;
    currentMetricBits = 0x461c4000;
}

// 운반 후보 수집/평가기이다. (2026-11-08 WB04-E102)
export class RawCarrierChoice {
    // 부모·후보·임시 거리의 같은 물리 저장소이다. (2026-11-08 WB04-E102)
    #pool;
    // 현재 genus/surface 플래그를 읽는 188 TYPE 표이다. (2026-11-08 WB04-E102)
    #types;
    // 현재 9개 Player 의 귀환점/AI 포인터 바이트 표이다. (2026-11-08 WB04-E102)
    #players;
    // 55a70c/710/714 의 현재 별도 신전·아웃포스트 목록이다. (2026-11-08 WB04-E102)
    #returns;
    // 5950dc/e0/e4 의 현재 간헐천 목록이다. (2026-11-08 WB04-E102)
    #geysers;
    // 실제 55b4d0 현재 시각의 소유자(getter 또는 { value })이다. (2026-11-08 WB04-E102)
    #now;
    // 실제 5e4794 진단 전역 getter 이다. (2026-11-08 WB04-E102)
    #checking;
    // 복원한 일반 접근점 41d530 계산기이다. (2026-11-08 WB04-E102)
    #board;
    // 현재 탐색 설정/타입/임시 metric 의 공유 전역이다. (2026-11-08 WB04-E102)
    #state;
    // 실제 큰 하위와 관측용 주소 재배치이다. (2026-11-08 WB04-E102)
    #hooks;

    // PATCH 의 물리 폭과 참조 계약을 확인한다. Player 기록과 슬롯 폭이 다른 CD 의 후보 몸체를 대신 실행하지 않는다. (2026-11-08 WB04-E102)
    constructor(pool, types, players, returns, geysers, now, checking, board, state, hooks = {}) {
        if (pool?.edition !== 'patch1078' || !types || types.length !== TYPE_COUNT || !players || players.length !== PLAYER_COUNT * PLAYER_STRIDE) {
            throw new Error('운반 후보의 판본/TYPE/Player9 오류');
        }
        this.#pool = pool;
        this.#types = types;
        this.#players = players;
        this.#returns = returns;
        this.#geysers = geysers;
        this.#now = now;
        this.#checking = checking;
        this.#board = board;
        this.#state = state;
        this.#hooks = hooks;
    }

    // 다른 월드의 후보·경로 연결을 확인할 때 쓰는 같은 풀이다. (2026-11-08 WB04-E102)
    get pool() { return this.#pool; }

    // 현재 시각이다. (2026-11-08 WB04-E102)
    #time() { return typeof this.#now === 'function' ? this.#now() : this.#now.value; }

    // DWORD 후보를 물리 범위(0 번 슬롯 포함) 검사 뒤 SID 로 바꾼다. free/dead 필터를 넣지 않는다. (2026-11-08 WB04-E102)
    #number(number) {
        if (number >= this.#pool.capacity || number > 0xffff) throw new RangeError('운반 후보의 물리 SID 범위 오류');
        return number;
    }

    // 두 좌표 float 를 원본 읽기 시점에 보관한다. (2026-11-08 WB04-E102)
    #position(sid) {
        const raw = this.#pool.slot(sid);
        const view = new DataView(raw.buffer, raw.byteOffset, raw.byteLength);
        return { x: view.getFloat32(X_AT, true), y: view.getFloat32(Y_AT, true) };
    }

    // 효과 뒤의 현재 raw 타입을 다시 읽는다. (2026-11-08 WB04-E102)
    #type(sid) {
        const kind = this.#pool.slot(sid)[TYPE_AT];
        if (kind >= this.#types.length) throw new RangeError('운반 후보의 현재 TYPE 범위 오류');
        return this.#types[kind];
    }

    // 40f100: owner 0 은 거짓이며 그 외는 현재 Player+b0 의 AI 포인터가 0 이 아니면 참이다. (2026-11-08 WB04-E102)
    #controlled(sid) {
        const owner = this.#pool.slot(sid)[OWNER_AT];
        if (!owner) return false;
        if (owner >= PLAYER_COUNT) throw new RangeError('운반 후보의 현재 Player 범위 오류');
        return this.#playerWord(owner * PLAYER_STRIDE + PLAYER_AI) !== 0;
    }

    // Player 표의 비정렬 DWORD 이다. (2026-11-08 WB04-E102)
    #playerWord(offset) {
        const players = this.#players;
        return (players[offset] | (players[offset + 1] << 8) | (players[offset + 2] << 16) | (players[offset + 3] << 24)) >>> 0;
    }

    // 임시 WORD(+8)만 쓴다. 객체의 실제 scratch 이므로 할당 여부나 타입으로 쓰기를 막지 않는다. (2026-11-08 WB04-E102)
    #setMetric(sid, value) {
        const raw = this.#pool.physical(sid);
        raw[METRIC_AT] = value & 0xff;
        raw[METRIC_AT + 1] = value >> 8;
    }

    // 원본 signed count 를 보존한다(음수를 큰 양수 목록으로 순회하지 않는다). (2026-11-08 WB04-E102)
    static #count(list) { return list.readCount() | 0; }

    // 4280a0: WORD 거리 → 원본 정렬 → 필요한 임시 경로 → 엄격한 최소값이다. 두 순회는 nonzero 카운터/표면 후보 처리/현재 count 재조회 시점이 서로 다르다. (2026-11-08 WB04-E102)
    evaluate(candidates, anchor, mover) {
        const hooks = this.#hooks;
        const state = this.#state;
        let movement = [1, 90];
        let skipProbe = false;
        if (mover && mover < this.#pool.capacity) {
            movement = need(hooks.movement)(mover);
            skipProbe = this.#controlled(mover) || (this.#type(mover).flags2 & FLYER) !== 0;
        }
        let index = 0;
        let seen = 0;
        let total = RawCarrierChoice.#count(candidates);
        if (total > 0) {
            // 첫 순회에서 surface 는 진단 후 재검사하고 여전히 surface 면 seen 을 늘리지 않는다.
            do {
                const entries = candidates.storage();
                if (index >= entries.length) break;
                if (total) {
                    const at = index % total;
                    if (at >= entries.length) throw new RangeError('운반 후보 목록 범위 오류');
                    const candidate = this.#number(entries[at]);
                    if (candidate) {
                        if (this.#type(candidate).flags1 & SURFACE) need(hooks.assertion)(ASSERT_SURFACE);
                        if (!(this.#type(candidate).flags1 & SURFACE)) {
                            const point = this.#position(candidate);
                            const anchorRect = need(hooks.footprint)(anchor);
                            const first = this.#board.nearest(anchorRect, point, 1);
                            const rectangle = need(hooks.footprint)(candidate);
                            const second = this.#board.nearest(rectangle, first, 1);
                            this.#setMetric(candidate, travelWord(second, first, movement[0]));
                            seen += 1;
                        }
                    }
                }
                total = RawCarrierChoice.#count(candidates);
                index += 1;
            } while (seen < total);
        }
        const storage = candidates.storage();
        if (candidates.readCount() > storage.length) throw new RangeError('운반 후보 정렬의 count/capacity 오류');
        // 실제 원본 비교기와 구형 CRT 를 사용한다. 관측의 주소 재배치는 배열 문맥만 추가한다.
        const addresses = hooks.sortAddresses ? hooks.sortAddresses() : undefined;
        const sorter = new RawCarrierCandidateSort(this.#pool, addresses);
        const sorted = Uint32Array.from(storage.slice(0, candidates.readCount()));
        sorter.sort(sorted);
        // 정렬 결과를 같은 목록의 앞부분에 되돌려 쓴다.
        for (let at = 0; at < sorted.length; at += 1) storage[at] = sorted[at];
        let best = INITIAL_BEST;
        let chosen = 0;
        index = 0;
        seen = 0;
        total = RawCarrierChoice.#count(candidates);
        if (total > 0) {
            // 경로 탐색은 거친 WORD 점수가 현재 최소값보다 작을 때만 수행한다.
            do {
                const entries = candidates.storage();
                if (index >= entries.length) break;
                if (total) {
                    const at = index % total;
                    if (at >= entries.length) throw new RangeError('운반 후보 목록 범위 오류');
                    const candidate = this.#number(entries[at]);
                    if (candidate) {
                        const raw = this.#pool.slot(candidate);
                        let metric = raw[METRIC_AT] | (raw[METRIC_AT + 1] << 8);
                        if (state.comparePaths && !skipProbe && metric < best) {
                            // 원본처럼 매 탐색 전에 count 와 5000개 시각을 0 으로 만든다.
                            const temporary = new RawPathRuntime();
                            const startPoint = this.#position(anchor);
                            const candidatePoint = this.#position(candidate);
                            const candidateRect = need(hooks.footprint)(candidate);
                            const destination = this.#board.nearest(candidateRect, startPoint, 1);
                            const anchorRect = need(hooks.footprint)(anchor);
                            const start = this.#board.nearest(anchorRect, candidatePoint, 1);
                            const kind = this.#pool.slot(mover)[TYPE_AT];
                            const cardinal = state.cardinalTypes.includes(kind);
                            need(hooks.search)(temporary, mover, start, destination, cardinal ? 1 : 0);
                            if (temporary.count) {
                                const now = this.#time();
                                temporary.prepare(now, now, movement[0], movement[1], 0);
                                metric = temporary.time(temporary.count - 1) - this.#time();
                                state.currentMetricBits = floatBits(fround(movement[0] * metric));
                            }
                        }
                        if (metric < best) {
                            best = fround(metric);
                            chosen = candidate;
                        }
                        seen += 1;
                    }
                }
                total = RawCarrierChoice.#count(candidates);
                index += 1;
            } while (seen < total);
        }
        if (Number(this.#checking()) !== 0 && seen !== RawCarrierChoice.#count(candidates)) need(hooks.assertion)(ASSERT_SEEN);
        state.currentMetricBits = state.baselineBits;
        return chosen;
    }

    // 428ea0: 현재 home 을 선두에 넣고 outposts 의 nonzero 수를 세며 원래 500개 상한과 중복을 유지한다. (2026-11-08 WB04-E102)
    chooseReturn(parent) {
        const hooks = this.#hooks;
        const candidates = new SquidPostPopList();
        candidates.entries = new Array(LIST_LIMIT).fill(0);
        const owner = this.#pool.slot(parent)[OWNER_AT];
        if (owner >= PLAYER_COUNT) throw new RangeError('귀환 후보의 현재 Player 범위 오류');
        const home = this.#playerWord(owner * PLAYER_STRIDE + PLAYER_HOME);
        if (home && need(hooks.validReturn)(parent, this.#number(home))) candidates.entries[candidates.count++] = home;
        let index = 0;
        let seen = 0;
        let total = RawCarrierChoice.#count(this.#returns);
        if (total > 0) {
            // 빈 항목은 seen 을 늘리지 않고 capacity 에 도달하면 멈춘다.
            do {
                const entries = this.#returns.storage();
                if (index >= entries.length) break;
                if (total) {
                    const at = index % total;
                    if (at >= entries.length) throw new RangeError('귀환 후보 목록 범위 오류');
                    const target = entries[at];
                    if (target) {
                        if (need(hooks.validReturn)(parent, this.#number(target)) && candidates.count < LIST_LIMIT) candidates.entries[candidates.count++] = target;
                        seen += 1;
                    }
                }
                total = RawCarrierChoice.#count(this.#returns);
                index += 1;
            } while (seen < total);
        }
        return this.evaluate(candidates, parent, parent);
    }

    // 429420: 귀환점이 물리 범위 안일 때만 Graph 조회/간헐천 수집을 수행한다. (2026-11-08 WB04-E102)
    chooseGeyser(parent) {
        const hooks = this.#hooks;
        const home = this.chooseReturn(parent);
        if (!home || home >= this.#pool.capacity) return 0;
        need(hooks.graphFor)(parent);
        const candidates = new SquidPostPopList();
        candidates.entries = new Array(LIST_LIMIT).fill(0);
        let index = 0;
        let total = RawCarrierChoice.#count(this.#geysers);
        if (total > 0) {
            // 원본에는 capacity 조기 종료가 없다. 확보 범위 밖 접근만 오류로 거부한다.
            do {
                if (total) {
                    const entries = this.#geysers.storage();
                    const at = index % total;
                    if (at >= entries.length) throw new RangeError('간헐천 후보 목록 범위 오류');
                    const target = entries[at];
                    if (target && need(hooks.validGeyser)(parent, this.#number(target)) && candidates.count < LIST_LIMIT) candidates.entries[candidates.count++] = target;
                }
                total = RawCarrierChoice.#count(this.#geysers);
                index += 1;
            } while (index < total);
        }
        return this.evaluate(candidates, home, parent);
    }
}
