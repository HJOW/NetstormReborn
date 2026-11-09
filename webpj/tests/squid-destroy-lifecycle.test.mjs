// 공통 삭제 장부 squid-destroy-lifecycle.js(SquidDestroyLifecycle·SquidDeletionState)를 원본 10.78 관측과 cpppj 계약(DestroyLifecycleTests·DestroyGraphTests)으로 대조한다.
//  - 원본 관측 destroylifecycle-x86.tsv 의 10.78(originals) 576행(두 x87 정밀도): 직접 Pre/Post 한 쌍(Pair 384행)과 공통 destroy→장부 pre→Unpop→장부 post→SID 반납 전체(Destroy 192행)의
//    외부 효과 순서·대상/주변 일곱 슬롯 바이트·풀 카운터와 두 free 머리·꼬리·풀 전체/네 단계 해시/spot/삭제 기록의 Adler-32·선택·pre/post 깊이·장부(비용·더티·깊이·세 통계 표·공급/작업장/소유자별 목록 전체)·배치/손실 좌표.
//  - 원본 관측 destroygraph-x86.tsv 의 10.78 576행(Pre 192·Post 192·Destroy 192)과 island-graph-frame-x86.tsv 의 10.78 행: 위에 더해 Graph 분할/Free·특수 H 치환·9→1 프레임 치환·주변 표면 순차 Add 와 Graph 기록 255행·flood 스택 Adler.
//  - 계약: 생성/삭제가 현재 수·비용을 되돌리고 누적 생산 수만 늘림·미지원 효과(Graph/AI/손상 목록/누락 콜백)의 장부 쓰기 전 거부·억제와 비용 경계·다른 풀/Form/contained 거부·장기 손실 좌표/비용의 소리 뒤 재조회(GameAux)·
//    Free 안전·H 치환 타입 검증·후보 프레임 오류의 쓰기 전 거부·다른 공간/타입 Graph 연결 거부.
//  - CD 10.72·10.37 행과 기준선 뒤 cpppj 가 더한 실제 PATCH(Native) 경로는 범위 밖이다(LEFT_JOBS.webpj.md 3.6절 ③).
// 실제 게임·브라우저 실행이 아니다. (2026-11-10 WB04-E213: 공통 삭제 장부 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, adler32, put, get, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { RawSquidDestroy, SQUID_DESTROY_EFFECT } from '../www/core/squid-destroy.js';
import { SquidPostPop } from '../www/core/squid-postpop.js';
import { SquidPostPopState } from '../www/core/squid-postpop-state.js';
import { RawGraph } from '../www/core/raw-graph.js';
import { TypeFlag1 } from '../www/assets/type-flags.js';
import { SQUID_DELETION_EFFECT, SquidDeletionState, SquidDestroyLifecycle } from '../www/core/squid-destroy-lifecycle.js';

// 원본 관측 파일에서 10.78(originals) 행만 읽는다. 없으면 해당 검사를 건너뛴다. (2026-11-10 WB04-E213)
const fixtureDir = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures');
const readOriginals = (name) => readRows(path.join(fixtureDir, name))?.filter((row) => row[1] === 'originals') ?? null;
const lifecycleRows = readOriginals('destroylifecycle-x86.tsv');
const graphRows = readOriginals('destroygraph-x86.tsv');
const islandRows = readOriginals('island-graph-frame-x86.tsv');

// 실제 Reset/Create 와 같은 풀 크기와 처음 일곱 client 번호, 장면 좌표(20.75f, 21.9f)이다. (2026-11-10 WB04-E213)
const CAPACITY = 32768;
const IDS = [5, 6, 7, 8, 9, 10, 11];
const X = Math.fround(20.75);
const Y = Math.fround(21.9);

// 예외가 나는지 본다. (2026-11-10 WB04-E213)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-10 WB04-E213)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// DWORD 값 목록(또는 WORD 폭)을 리틀엔디언 바이트로 만든다. (2026-11-10 WB04-E213)
function appendDwords(bytes, values, width = 4) {
    for (const value of values) for (let index = 0; index < width; index += 1) bytes.push(Math.floor(value / 2 ** (8 * index)) & 0xff);
}

// 원본 결과를 계산하지 않는 중복/미사용 꼬리 포함 목록 입력이다(여섯 패턴의 활성 항목 수 0·1·5·6·4·3). (2026-11-10 WB04-E213)
function listInput(list, pattern, base) {
    list.count = [0, 1, 5, 6, 4, 3][pattern];
    list.entries = Array.from({ length: 6 }, (_, index) => base + index);
    if (pattern === 1) list.entries[0] = 5;
    if (pattern === 3) list.entries[0] = list.entries[2] = list.entries[5] = 5;
    if (pattern === 4) list.entries.fill(5, 0, 4);
    if (pattern === 5) list.entries[1] = 5;
}

// 입력 19/25 칸의 공통 부분(장부·상태·통계 표)을 장부와 삭제 상태에 채운다. 삭제 관련 입력 번호: 5 소유자·6 로컬 소유자·8 flags·9 선택·10 억제·11/12 소리 억제·13 소리 제외 TYPE·14 목록 패턴·15 비용·16 더티·18 통계 시드.
// (2026-11-10 WB04-E213)
function fillBookkeeping(book, state, input) {
    state.selected = input[9] === '1' ? 5 : 0;
    book.localOwner = Number(input[6]);
    book.suppressed = input[10] === '1';
    state.unitLostSuppressed1 = input[11] === '1';
    state.unitLostSuppressed2 = input[12] === '1';
    state.silentType = Number(input[13]);
    const pattern = Number(input[14]);
    book.totalCost = Number(input[15]) | 0;
    book.productionDirty = Number(input[16]) >>> 0;
    book.depth = 3;
    listInput(book.providers, pattern, 0x60000000);
    listInput(book.factories, pattern, 0x61000000);
    // 원본 Player 0 의 미사용 목록까지 같은 입력을 넣는다.
    for (let index = 0; index < 9; index += 1) listInput(book.ownerFactories[index], pattern, 0x70000000 + index * 100);
    const seed = Number(input[18]) >>> 0;
    // 통계 표 전체의 비대상 타입 보존과 DWORD 감김도 비교한다.
    for (let index = 0; index < 256; index += 1) {
        book.localCounts()[index] = (seed + Math.imul(index, 0x9e3779b1)) >>> 0;
        book.localSecondaryCounts()[index] = (book.localCounts()[index] + 17) >>> 0;
        book.globalCounts()[index] = (book.localCounts()[index] + 34) >>> 0;
    }
    state.placementX = -7.25;
    state.placementY = 8.5;
    state.lostX = 90.25;
    state.lostY = -91.5;
}

// 같은 풀/상태/처리기를 조립하고 원본 가상 효과를 대체 기록으로 받는 장면이다(cpppj DestroyLifecycleTests/DestroyGraphTests 의 Scene). Graph 입력(nodes 등)이 있으면 Graph 를 함께 연결한다.
// 기대값은 계산하지 않으며 fixture 만 기대값이다. (2026-11-10 WB04-E213)
class DestroyScene {
    // input 은 입력 19/25 칸, graphInput = { nodes, spots, records, islandFrame } 이면 Graph 장면이다.
    constructor(input, graphInput = null) {
        this.pool = new SidPool(CAPACITY, { server: true });
        this.hash = new SquidHash();
        this.types = Array.from({ length: 188 }, () => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, group: 0, maxHitPoints: 0, zOrder: 0 }));
        this.spots = new Uint8Array(65536);
        this.unpop = new SquidUnpop(this.pool, this.hash, this.spots);
        this.destroy = new RawSquidDestroy(this.pool, this.unpop, this.types);
        this.book = new SquidPostPopState();
        this.state = new SquidDeletionState();
        this.events = [];
        this.flags = 0;
        this.graph = null;
        assert.equal(input.length, graphInput ? 25 : 19);
        // 부모 실제 Create 와 동일한 client 할당만 준비한다.
        for (const sid of IDS) assert.equal(this.pool.allocate(2), sid);
        if (graphInput) this.#graphNodes(input, graphInput);
        else this.#plainNodes(input);
        this.flags = Number(input[8]);
        fillBookkeeping(this.book, this.state, input);
        if (graphInput) {
            this.book.graphsEnabled = input[19] === '1';
            this.state.rebuildGraph = input[20] === '1';
            this.state.regionMask = Number(input[21]) >>> 0;
            const stack = Uint32Array.from({ length: 4096 }, (_, index) => (Number(input[22]) + index) >>> 0);
            let frames = graphInput.frames;
            // 실제 157 의 물리 프레임 H=8 과 162 의 단일 프레임을 기존 합성 입력과 구별하여 재현한다.
            if (graphInput.islandFrame) {
                frames = frames.map((codes) => codes.slice());
                frames[157] = Array.from({ length: 9 }, () => ({ side: 65, variant: 80, number: 1, flags: 0 }));
                frames[157][8] = { side: 72, variant: 80, number: 9, flags: 0 };
                frames[162] = [{ side: 65, variant: 80, number: 1, flags: 0 }];
            }
            this.frames = frames;
            this.graph = new RawGraph(this.pool, this.hash, this.spots, this.types, frames, { records: graphInput.records, stack });
        }
        this.lifecycle = new SquidDestroyLifecycle(this.pool, this.types, this.book, this.state, this.destroy, {
            emit: (event) => this.emit(event),
            forward: (event) => {
                if (event.effect !== SQUID_DESTROY_EFFECT.transmit) throw new Error('장면에 없는 종속 가상 효과');
                this.events.push(`T:${event.sid}:${event.flags}`);
            },
        }, this.graph);
    }

    // 장부 전용 장면: 같은 위치의 root 0단계·주변 1단계 버킷 체인을 넣는다. (2026-11-10 WB04-E213)
    #plainNodes(input) {
        for (let index = 0; index < IDS.length; index += 1) {
            const raw = this.pool.allocated(IDS[index]);
            raw.fill(0);
            put(raw, 0, 0x501dd0);
            raw[10] = 74 + index;
            put(raw, 14, floatToBits(X));
            put(raw, 18, floatToBits(Y));
            this.types[74 + index].footX = 1;
            this.types[74 + index].footY = 1;
            put(raw, 4, this.hash.bucket(index === 0 ? 0 : 1, X, Y), 2);
            this.hash.setBucket(index === 0 ? 0 : 1, X, Y, IDS[index]);
        }
        const number = Number(input[0]);
        const type = this.types[number];
        type.flags1 = Number(input[1]) >>> 0;
        type.flags2 = Number(input[2]) >>> 0;
        floatBitsInto(type, Number(input[3]) >>> 0);
        type.group = Number(input[4]);
        type.footX = 1;
        type.footY = 1;
        const root = this.pool.allocated(5);
        root[10] = number;
        root[11] = input[17] === '1' ? 4 : 0;
        root[34] = Number(input[5]);
        root[40] = Number(input[7]);
        if (root[11] & 4) this.hash.setBucket(0, X, Y, 0);
    }

    // Graph 장면: 일곱 객체의 타입·프레임·상태·좌표·그래프 번호·레벨을 입력으로 만든다(cpppj DestroyGraphTests Scene). (2026-11-10 WB04-E213)
    #graphNodes(input, graphInput) {
        const frames = Array.from({ length: 188 }, () => []);
        graphInput.frames = frames;
        for (let index = 0; index < graphInput.nodes.length; index += 1) {
            const n = graphInput.nodes[index];
            assert.equal(n.length, 13);
            const number = index ? 74 + index : Number(input[0]);
            const type = this.types[number];
            type.flags1 = n[4] >>> 0;
            type.flags2 = n[5] >>> 0;
            type.footX = n[2];
            type.footY = n[3];
            type.cost = 1.25;
            frames[number] = [{ side: n[6], variant: n[7], number: 1, flags: 0 }, { side: 65, variant: 80, number: 2, flags: 0 }];
            const raw = this.pool.allocated(IDS[index]);
            raw.fill(0);
            put(raw, 0, 0x501dd0);
            put(raw, 8, 23, 2);
            raw[10] = number;
            raw[11] = n[9];
            put(raw, 14, floatToBits(n[0]));
            put(raw, 18, floatToBits(n[1]));
            raw[30] = n[11];
            raw[33] = n[12];
            raw[34] = 1;
            put(raw, 36, n[8] >>> 0, 4);
            raw[40] = n[10];
            if (!(n[9] & 4)) {
                put(raw, 4, this.hash.bucket(n[12], n[0], n[1]), 2);
                this.hash.setBucket(n[12], n[0], n[1], IDS[index]);
            }
        }
        const number = Number(input[0]);
        this.types[number].group = Number(input[4]);
        if (number !== 162) {
            const replacement = this.types[162];
            replacement.flags1 = TypeFlag1.surface;
            replacement.flags2 = Number(input[24]) >>> 0;
            replacement.footX = replacement.footY = Number(input[23]);
            replacement.cost = 1.25;
            frames[162] = [{ side: 65, variant: 80, number: 1, flags: 0 }, { side: 65, variant: 80, number: 2, flags: 0 }];
        }
        // interior 바이트는 해시/타입 입력과 별개다.
        for (const spot of graphInput.spots) this.spots[spot[1] * 256 + spot[0]] = spot[2];
    }

    // 실제 장부 함수가 호출한 외부 UI/보상/소리만 기계어 대체 기록과 같은 형식으로 둔다. (2026-11-10 WB04-E213)
    emit(event) {
        switch (event.effect) {
        case SQUID_DELETION_EFFECT.clearSelection: this.events.push(`S:${this.state.selected}`); break;
        case SQUID_DELETION_EFFECT.refund: this.events.push(`Q:${event.sid}:${event.recipient}`); break;
        case SQUID_DELETION_EFFECT.unitLostSound: this.events.push(`L:${event.sid}`); break;
        default: throw new Error(`모르는 삭제 장부 효과 ${event.effect}`);
        }
    }

    // P/O 함수 진입 때 raw state 를 별도로 관찰한다. (2026-11-10 WB04-E213)
    entry(tag, sid, value) { this.events.push(`${tag}:${sid}:${value}:${this.pool.slot(sid)[11]}`); }

    // 직접 훅 쌍/전·후 훅의 깊이 underflow 와 통합 삭제의 균형 깊이를 서로 구별한다. kind: Pair(Pre+Post)·Pre·Post·Destroy. (2026-11-10 WB04-E213)
    run(kind) {
        if (kind === 'Pair') {
            this.entry('P', 5, this.flags);
            this.lifecycle.preDestroy(5, this.flags);
            this.entry('O', 5, this.flags);
            this.lifecycle.postDestroy(5, this.flags);
        } else if (kind === 'Pre') {
            this.entry('P', 5, this.flags);
            this.lifecycle.preDestroy(5, this.flags);
        } else if (kind === 'Post') {
            this.entry('O', 5, this.flags);
            this.lifecycle.postDestroy(5, this.flags);
        } else {
            const hooks = this.lifecycle.hooks();
            const emit = hooks.emit;
            // 전체 삭제 안에서도 공통 훅 몸체 자체를 호출한다.
            hooks.emit = (event) => {
                if (event.effect === SQUID_DESTROY_EFFECT.preDestroy) this.entry('P', event.sid, event.flags);
                if (event.effect === SQUID_DESTROY_EFFECT.postDestroy) this.entry('O', event.sid, event.flags);
                emit(event);
            };
            this.destroy.destroy(5, this.flags, hooks);
        }
    }

    // 네 단계 해시 전체 WORD 의 Adler-32 이다. (2026-11-10 WB04-E213)
    hashAdler() {
        const bytes = [];
        for (let level = 0; level < 4; level += 1) appendDwords(bytes, this.hash.entries(level), 2);
        return adler32(Uint8Array.from(bytes));
    }

    // client/서버 최근 삭제 기록의 모든 DWORD 의 Adler-32 이다. (2026-11-10 WB04-E213)
    logAdler() {
        const bytes = [];
        for (const client of [true, false]) for (const record of this.pool.deletions(client)) appendDwords(bytes, [record.sid, record.type]);
        return adler32(Uint8Array.from(bytes));
    }

    // 모든 목록의 inactive DWORD 와 누적 생산 표를 포함한 장부 결과다. (2026-11-10 WB04-E213)
    bookkeeping() {
        const book = this.book;
        const values = [book.totalCost, book.productionDirty, book.depth];
        const data = [];
        // 서로 다른 세 통계 표를 정해진 순서로 해시한다.
        for (const table of [book.localCounts(), book.localSecondaryCounts(), book.globalCounts()]) {
            const bytes = [];
            appendDwords(bytes, table);
            values.push(adler32(Uint8Array.from(bytes)));
        }
        // 공급원/작업장 다음에 Player 0..8 전체 목록을 관찰한다.
        for (const list of [book.providers, book.factories, ...book.ownerFactories]) {
            values.push(list.count);
            appendDwords(data, list.entries);
        }
        values.push(adler32(Uint8Array.from(data)));
        return values;
    }

    // 외부 효과·raw 슬롯 바이트·전체 풀/공간·장부·선택/배치/손실 좌표를 모두 대조한다. (2026-11-10 WB04-E213)
    check(row, label) {
        assert.equal(this.events.length === 0 ? '-' : this.events.join(';'), row[4], `${label} 사건`);
        // root 와 주변 일곱 슬롯은 checksum 대신 직접 바이트도 확인한다.
        for (const entry of row[5].split(';')) {
            const [sid, bytes] = entry.split(':');
            assert.equal(hex(this.pool.slot(Number(sid))), bytes, `${label} 슬롯 ${sid}`);
        }
        const pool = this.pool;
        const actual = [pool.freeCount, pool.predictableCursor, pool.firstFree(true), pool.firstFree(false), pool.tail(true), pool.tail(false),
            adler32(pool.bytes()), this.hashAdler(), adler32(this.spots), this.logAdler(), this.state.selected, this.destroy.preDepth, this.destroy.postDepth];
        assert.deepEqual(actual, row[6].split(',').map(Number), `${label} 풀/공간/깊이`);
        assert.deepEqual(this.bookkeeping(), row[7].split(',').map(Number), `${label} 장부`);
        const coordinates = [this.state.placementType, floatToBits(this.state.placementX), floatToBits(this.state.placementY), floatToBits(this.state.lostX), floatToBits(this.state.lostY)];
        assert.deepEqual(coordinates, row[8].split(',').map(Number), `${label} 배치/손실 좌표`);
        assert.equal(this.book.pendingPlacement, this.state.placementType !== 0);
    }

    // Graph 장면의 기록 255행과 flood 스택 Adler 를 대조한다. (2026-11-10 WB04-E213)
    checkGraph(row, label) {
        const expected = recordsFromHex(row[12]);
        const records = this.graph.records();
        assert.equal(records.length, expected.length);
        for (let index = 0; index < expected.length; index += 1) {
            assert.deepEqual([records[index].surfaces, records[index].inUse, records[index].reserved], [expected[index].surfaces, expected[index].inUse, expected[index].reserved], `${label} Graph 행 ${index}`);
        }
        assert.equal(graphHashes(this.graph)[1], Number(row[13]), `${label} flood 스택`);
    }
}

// 탐색 가능한 float 입력 비트를 타입 비용에 넣는다(원본 입력은 float 비트 값이다). (2026-11-10 WB04-E213)
function floatBitsInto(type, bits) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, bits, true);
    type.cost = view.getFloat32(0, true);
}

// 발자국/프레임/상태/해시 단계의 원본 입력('-' 이면 빈 목록, 항목은 ';' 필드는 ',')이다. (2026-11-10 WB04-E213)
function nodes(text) {
    return text === '-' ? [] : text.split(';').map((entry) => entry.split(',').map(Number));
}

// little endian hex 에서 세 signed WORD 를 패딩 없이 해독한다(255행 × 12 hex). (2026-11-10 WB04-E213)
function recordsFromHex(text) {
    assert.equal(text.length, 255 * 12);
    const signed = (low, high) => (((low | (high << 8)) << 16) >> 16);
    return Array.from({ length: 255 }, (_, index) => {
        const words = [0, 1, 2].map((word) => {
            const at = index * 12 + word * 4;
            return signed(parseInt(text.slice(at, at + 2), 16), parseInt(text.slice(at + 2, at + 4), 16));
        });
        return { surfaces: words[0], inUse: words[1], reserved: words[2] };
    });
}

// Graph 기록 255행(부호 있는 WORD 비트 그대로)과 flood 스택의 Adler 쌍이다. (2026-11-10 WB04-E213)
function graphHashes(graph) {
    const records = [];
    for (const record of graph.records()) appendDwords(records, [record.surfaces & 0xffff, record.inUse & 0xffff, record.reserved & 0xffff], 2);
    const stack = [];
    appendDwords(stack, graph.floodStack());
    return [adler32(Uint8Array.from(records)), adler32(Uint8Array.from(stack))];
}

// 삭제 Graph fixture 한 행으로 장면을 만든다. (2026-11-10 WB04-E213)
function graphScene(row, islandFrame = false) {
    assert.equal(row.length, 14);
    return new DestroyScene(row[3].split(','), { nodes: nodes(row[9]), spots: nodes(row[10]), records: recordsFromHex(row[11]), islandFrame });
}

// 같은 종류의 행을 모두 재생한다. (2026-11-10 WB04-E213)
function replayLifecycle(kind, expected) {
    const rows = lifecycleRows.filter((row) => row[0] === kind);
    for (const row of rows) {
        assert.equal(row.length, 9);
        const scene = new DestroyScene(row[3].split(','));
        scene.run(kind);
        scene.check(row, `${row[0]} ${row[2]} ${row[3]}`);
    }
    assert.equal(rows.length, expected);
}

test('삭제 장부: 원본 10.78 공통 Pre/Post 한 쌍 384행(선택 복구·중복 제거·통계·억제·비용)', { skip: !lifecycleRows && '원본 fixture 없음' }, () => {
    replayLifecycle('Pair', 384);
});

test('삭제 장부: 원본 10.78 공통 destroy→Unpop→SID 반납 통합 192행', { skip: !lifecycleRows && '원본 fixture 없음' }, () => {
    replayLifecycle('Destroy', 192);
});

// 삭제 Graph fixture 를 종류별로 재생한다. (2026-11-10 WB04-E213)
function replayGraph(kind) {
    const rows = graphRows.filter((row) => row[0] === kind);
    for (const row of rows) {
        const scene = graphScene(row);
        scene.run(kind);
        scene.check(row, `${row[0]} ${row[2]} ${row[3]}`);
        scene.checkGraph(row, `${row[0]} ${row[2]} ${row[3]}`);
    }
    assert.equal(rows.length, 192);
}

test('삭제 Graph: 원본 10.78 pre 의 표면 분할/Free·특수 H 치환·선택/장부 순서 192행', { skip: !graphRows && '원본 fixture 없음' }, () => {
    replayGraph('Pre');
});

test('삭제 Graph: 원본 10.78 post 의 발자국 경계·네 해시 단계·중복 머리·inactive 표면과 순차 Add 192행', { skip: !graphRows && '원본 fixture 없음' }, () => {
    replayGraph('Post');
});

test('삭제 Graph: 원본 10.78 destroy→pre Graph/장부→Unpop→post Graph/비용→Release 전체 192행', { skip: !graphRows && '원본 fixture 없음' }, () => {
    replayGraph('Destroy');
});

test('삭제 Graph: 실제 157 H=8 프레임과 162 단일 프레임 치환의 pre/Destroy 전체 상태(10.78)', { skip: !islandRows && '원본 fixture 없음' }, () => {
    assert.ok(islandRows.length > 0);
    for (const row of islandRows) {
        const scene = graphScene(row, true);
        scene.run(row[0]);
        scene.check(row, `${row[0]} ${row[2]}`);
        scene.checkGraph(row, `${row[0]} ${row[2]}`);
    }
});

// 입력 19 칸의 정상 기본값이다. 외부 보상/소리를 억제하고 이미 void 인 자산이다. (2026-11-10 WB04-E213)
const basic = () => '74,0,0,1142292480,0,1,1,0,2097152,0,0,0,0,148,3,1000,0,1,123'.split(',');

// 장부 사본(통계 표·합계)이다. (2026-11-10 WB04-E213)
function snapshotBook(book) {
    return { total: book.totalCost, local: Array.from(book.localCounts()), global: Array.from(book.globalCounts()), made: Array.from(book.localSecondaryCounts()) };
}

test('삭제 장부 계약: 생성/삭제는 현재 수와 비용을 되돌리고 누적 생산 수만 늘린 채 유지한다', () => {
    const input = basic();
    input[1] = '268435456';
    input[2] = '512';
    input[14] = '0';
    const scene = new DestroyScene(input);
    const before = snapshotBook(scene.book);
    const post = new SquidPostPop(scene.pool, scene.types, scene.book);
    post.activate(5, 1);
    const made = scene.book.localSecondaryCounts()[74];
    scene.destroy.destroy(5, scene.flags, scene.lifecycle.hooks());
    const after = snapshotBook(scene.book);
    assert.equal(after.total, before.total);
    assert.deepEqual(after.local, before.local);
    assert.deepEqual(after.global, before.global);
    assert.equal(scene.book.localSecondaryCounts()[74], made);
    assert.equal(made, before.made[74] + 1);
    assert.equal(scene.book.providers.count, 0);
    assert.equal(scene.book.factories.count, 0);
    assert.equal(scene.book.ownerFactories[1].count, 0);
    assert.equal(scene.book.productionDirty, 4);
    assert.equal(scene.destroy.preDepth, 0);
    assert.equal(scene.destroy.postDepth, 0);
});

test('삭제 장부 계약: Graph/AI/손상 목록/누락 효과는 장부 쓰기와 깊이 감소 전에 거부한다', () => {
    const input = basic();
    input[1] = '268437504';
    input[2] = '512';
    const scene = new DestroyScene(input);
    const before = scene.bookkeeping();
    const raw = adler32(scene.pool.bytes());
    scene.book.graphsEnabled = true;
    assert.ok(throws(() => scene.lifecycle.preDestroy(5, 0)));
    assert.deepEqual(scene.bookkeeping(), before);
    assert.ok(throws(() => scene.lifecycle.postDestroy(5, 0)));
    assert.deepEqual(scene.bookkeeping(), before);
    scene.book.graphsEnabled = false;
    scene.book.aiAttached[1] = true;
    assert.ok(throws(() => scene.lifecycle.preDestroy(5, scene.flags)));
    assert.deepEqual(scene.bookkeeping(), before);
    scene.book.aiAttached[1] = false;
    scene.book.providers.count = 7;
    const corrupt = scene.bookkeeping();
    assert.ok(throws(() => scene.lifecycle.preDestroy(5, scene.flags)));
    assert.deepEqual(scene.bookkeeping(), corrupt);
    scene.book.providers.count = 6;
    const missing = new SquidDestroyLifecycle(scene.pool, scene.types, scene.book, scene.state, scene.destroy, {});
    assert.ok(throws(() => missing.preDestroy(5, 0)));
    scene.state.selected = 5;
    assert.ok(throws(() => missing.preDestroy(5, scene.flags)));
    assert.equal(adler32(scene.pool.bytes()), raw);
    assert.equal(scene.destroy.preDepth, 0);
    assert.equal(scene.destroy.postDepth, 0);
});

test('삭제 장부 계약: 억제는 Pre 의 통계만 생략하고 ordinary Post 비용은 남기며 비유한 비용은 쓰기 전에 거부한다', () => {
    const input = basic();
    input[10] = '1';
    const scene = new DestroyScene(input);
    const before = snapshotBook(scene.book);
    scene.run('Pair');
    assert.deepEqual(Array.from(scene.book.localCounts()), before.local);
    assert.deepEqual(Array.from(scene.book.globalCounts()), before.global);
    assert.equal(scene.book.totalCost, before.total - 600);
    assert.equal(scene.destroy.preDepth, 0xffffffff);
    assert.equal(scene.destroy.postDepth, 0xffffffff);
    scene.types[74].cost = Infinity;
    const invalid = new SquidDestroyLifecycle(scene.pool, scene.types, scene.book, scene.state, scene.destroy, {});
    const cost = scene.book.totalCost;
    assert.ok(throws(() => invalid.postDestroy(5, 0)));
    assert.equal(scene.book.totalCost, cost);
    assert.equal(scene.destroy.postDepth, 0xffffffff);
});

test('삭제 장부 계약: 다른 풀 연결·Form 루트·contained 는 명시적으로 거부한다', () => {
    const scene = new DestroyScene(basic());
    const other = new SidPool(CAPACITY, { server: true });
    assert.ok(throws(() => new SquidDestroyLifecycle(other, scene.types, scene.book, scene.state, scene.destroy, {})));
    assert.ok(throws(() => new SquidDestroyLifecycle(scene.pool, scene.types.slice(0, 171), scene.book, scene.state, scene.destroy, {})));
    const raw = scene.pool.allocated(5);
    raw[10] = 20;
    assert.ok(throws(() => scene.lifecycle.preDestroy(5, scene.flags)));
    raw[10] = 74;
    raw[11] |= 8;
    assert.ok(throws(() => scene.lifecycle.postDestroy(5, 0)));
    assert.equal(scene.destroy.preDepth, 0);
    assert.equal(scene.destroy.postDepth, 0);
});

test('삭제 장부 계약(GameAux): 소리 효과 뒤 현재 손실 좌표와 장기 비용을 읽고 지역 사본은 쓰지 않는다', () => {
    const input = basic();
    input[8] = '0';
    const scene = new DestroyScene(input);
    const owner = { cost: 1000, lostX: 0, lostY: 0 };
    scene.book.bindTotalCost({ get value() { return owner.cost; }, set value(next) { owner.cost = next; } });
    scene.state.sharedLostX = { get value() { return owner.lostX; }, set value(next) { owner.lostX = next; } };
    scene.state.sharedLostY = { get value() { return owner.lostY; }, set value(next) { owner.lostY = next; } };
    let sounds = 0;
    // 음향장치의 선언된 효과만 주입하며 삭제 판정을 대신하지 않는다.
    const lifecycle = new SquidDestroyLifecycle(scene.pool, scene.types, scene.book, scene.state, scene.destroy, {
        emit: (event) => {
            if (event.effect !== SQUID_DELETION_EFFECT.unitLostSound) return;
            sounds += 1;
            owner.cost = 2000;
            owner.lostX = -1;
            owner.lostY = -2;
            const raw = scene.pool.allocated(5);
            put(raw, 14, floatToBits(22.25));
            put(raw, 18, floatToBits(23.75));
        },
    });
    lifecycle.preDestroy(5, 0);
    lifecycle.postDestroy(5, 0);
    assert.equal(sounds, 1);
    assert.equal(owner.lostX, 22.25);
    assert.equal(owner.lostY, 23.75);
    assert.equal(owner.cost, 1400);
    assert.equal(scene.state.lostX, 90.25);
    assert.equal(scene.state.lostY, -91.5);
});

test('삭제 Graph 계약: Free 는 raw root 번호만 해제하고 254 는 자연 반환이며 비정상 번호는 거부한다', { skip: !graphRows && '원본 fixture 없음' }, () => {
    const row = graphCase('Pre', 13);
    const scene = graphScene(row);
    const pool = adler32(scene.pool.bytes());
    const hash = scene.hashAdler();
    const spots = adler32(scene.spots);
    const stack = graphHashes(scene.graph)[1];
    const graphByte = scene.pool.slot(5)[30];
    const before = scene.graph.records().map((record) => ({ ...record }));
    scene.graph.free(graphByte);
    const after = scene.graph.records();
    assert.deepEqual([after[graphByte].surfaces, after[graphByte].inUse, after[graphByte].reserved], [0, 0, before[graphByte].reserved]);
    assert.deepEqual({ ...after[1] }, before[1]);
    assert.equal(adler32(scene.pool.bytes()), pool);
    assert.equal(scene.hashAdler(), hash);
    assert.equal(adler32(scene.spots), spots);
    assert.equal(graphHashes(scene.graph)[1], stack);
    const unchanged = graphHashes(scene.graph);
    scene.graph.free(254);
    assert.deepEqual(graphHashes(scene.graph), unchanged);
    for (const invalid of [251, 252, 253, 255]) assert.ok(throws(() => scene.graph.free(invalid)));
    assert.deepEqual(graphHashes(scene.graph), unchanged);
});

// 첫 patch/x87(639) 묶음에서 지정된 사례를 꺼낸다(초기 스택 seed 가 index*17+10 인 행). (2026-11-10 WB04-E213)
function graphCase(kind, index) {
    const seed = String(index * 17 + 10);
    const row = graphRows.find((candidate) => candidate[0] === kind && candidate[2] === '639' && candidate[3].split(',')[22] === seed);
    assert.ok(row, `삭제 Graph 입력 없음 ${kind} ${index}`);
    return row;
}

test('삭제 Graph 계약: H 치환 타입 유효성은 필요한 분기에만 검사하며 실패하면 선택/장부/Graph 를 보존한다', { skip: !graphRows && '원본 fixture 없음' }, () => {
    const scene = graphScene(graphCase('Pre', 8));
    scene.state.specialSurfaceReplacementType = 255;
    const book = scene.bookkeeping();
    const graph = graphHashes(scene.graph);
    const raw = adler32(scene.pool.bytes());
    assert.ok(throws(() => scene.lifecycle.preDestroy(5, scene.flags)));
    assert.deepEqual(scene.bookkeeping(), book);
    assert.deepEqual(graphHashes(scene.graph), graph);
    assert.equal(adler32(scene.pool.bytes()), raw);
    assert.equal(scene.state.selected, 5);
    assert.equal(scene.destroy.preDepth, 0);
    const skippedRow = graphCase('Pre', 9);
    const skipped = graphScene(skippedRow);
    skipped.state.specialSurfaceReplacementType = 255;
    skipped.run('Pre');
    skipped.check(skippedRow, 'H 치환 분기 밖');
});

test('삭제 Graph 계약: 주변 후보 프레임 오류는 실제 풀/표/스택·비용·깊이를 반영하기 전에 거부한다', { skip: !graphRows && '원본 fixture 없음' }, () => {
    const scene = graphScene(graphCase('Post', 14));
    put(scene.pool.allocated(7), 36, 999);
    const book = scene.bookkeeping();
    const graph = graphHashes(scene.graph);
    const raw = adler32(scene.pool.bytes());
    assert.ok(throws(() => scene.lifecycle.postDestroy(5, scene.flags)));
    assert.deepEqual(scene.bookkeeping(), book);
    assert.deepEqual(graphHashes(scene.graph), graph);
    assert.equal(adler32(scene.pool.bytes()), raw);
    assert.equal(scene.destroy.postDepth, 0);
});

test('삭제 Graph 계약: 풀·해시·spot·타입이 다른 Graph 는 공통 삭제에 연결하지 않는다', { skip: !graphRows && '원본 fixture 없음' }, () => {
    const scene = graphScene(graphCase('Pre', 0));
    const otherHash = new SquidHash();
    const otherSpots = new Uint8Array(65536);
    const wrongHash = new RawGraph(scene.pool, otherHash, scene.spots, scene.types, scene.frames);
    const wrongSpots = new RawGraph(scene.pool, scene.hash, otherSpots, scene.types, scene.frames);
    const otherTypes = scene.types.map((type) => ({ ...type }));
    otherTypes[74].footX = 2;
    const wrongTypes = new RawGraph(scene.pool, scene.hash, scene.spots, otherTypes, scene.frames);
    // 생성 시 연결 오류를 거부하며 기존 장면은 변경하지 않는다.
    for (const wrong of [wrongHash, wrongSpots, wrongTypes]) {
        assert.ok(throws(() => new SquidDestroyLifecycle(scene.pool, scene.types, scene.book, scene.state, scene.destroy, {}, wrong)));
    }
    assert.equal(scene.destroy.preDepth, 0);
    assert.equal(scene.destroy.postDepth, 0);
});

test('삭제 장부 계약: 현재 AI 삭제 통지는 공급원 제거 뒤·작업장 제거 앞에서 현재 소유자/SID 로 불리고 해제할 수 있다', () => {
    const input = basic();
    input[1] = '268435456';
    input[2] = '512';
    input[14] = '0';
    const scene = new DestroyScene(input);
    scene.book.aiAttached[1] = true;
    assert.ok(throws(() => scene.lifecycle.preDestroy(5, scene.flags)), 'AI 통지 미연결 거부');
    const calls = [];
    scene.lifecycle.bindPatchAiNotice((owner, sid, flag) => calls.push([owner, sid, flag, scene.book.providers.count, scene.book.factories.count]));
    scene.lifecycle.preDestroy(5, scene.flags);
    assert.deepEqual(calls, [[1, 5, false, 0, 0]]);
    scene.lifecycle.bindPatchAiNotice(null);
    assert.ok(throws(() => scene.lifecycle.preDestroy(5, scene.flags)));
});

test('삭제 장부 계약: 상태의 손실 좌표는 단정도로 기록하고 장기 저장소가 있으면 그것을 읽고 쓴다', () => {
    const state = new SquidDeletionState();
    state.writeLostX(0.1);
    state.writeLostY(-0.3);
    assert.equal(state.readLostX(), Math.fround(0.1));
    assert.equal(state.readLostY(), Math.fround(-0.3));
    const box = { value: 0 };
    state.sharedLostX = box;
    state.writeLostX(22.25);
    assert.equal(box.value, 22.25);
    assert.equal(state.readLostX(), 22.25);
    assert.equal(state.lostX, Math.fround(0.1));
    assert.equal(get(new Uint8Array([1, 0, 0, 0]), 0), 1);
});
