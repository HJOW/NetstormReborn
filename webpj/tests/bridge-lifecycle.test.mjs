// 다리 삭제 전후 훅(RawBridgeLifecycle)·낙하 좌표 포장을 원본 10.78 관측 bridgeeffects-x86.tsv 로 대조한다(cpppj `BridgeLifecycleTests.cpp` 이식, WB04-E247).
//  - Link 882행: 링크 객체의 두 참조·state·extra 조합에서 linkNeedsDestroy(00422290)의 판정(D:60:0 이면 삭제 필요).
//  - Pre 840행: preDestroy(004221b0)의 탐색 범위(Q)·연결 삭제(D)·공통 preDestroy(A) 순서와 풀 전체 바이트. 같은 840행을 현재 연결 TYPE getter 로 읽는 Native 경로(preDestroyNative)로 한 번 더 재생한다.
//  - Post 600행: postDestroy(00422300)의 파편 요청(N)·낙하 소리 좌표 비트(S)·좌표를 다시 읽은 탐색(Q)·walker 낙하(F)·공통 postDestroy(B) 순서와 풀 전체 바이트.
//  - Delay 242·Pack 242행: delayedFallPayload(00421530/00421f90)의 두 _ftol 하위 비트 합과 수치 float 비트(비유한·음수·바이트 경계·float 정밀도 경계 포함).
//  - 계약: 훅 누락·SID 0 거부(효과 시작 전), Native 가 첫 연결 삭제 뒤 바뀐 전역을 다음 후보에서 읽음, BYTE 슬롯 TYPE 과 전체 DWORD 비교.
// 입력 slot 은 각 행의 합성 객체(SID 50 다리·60 링크 등)이며 실제 월드 번호 배치가 아니다. CD 10.72(originalCD)·10.37 행은 범위 밖이다. (2026-11-10 WB04-E247)
import assert from 'node:assert/strict';
import path from 'node:path';
import test from 'node:test';
import { BRIDGE_FALL_EVENT, BRIDGE_LIFECYCLE_EFFECT, RawBridgeLifecycle } from '../www/core/bridge-lifecycle.js';
import { SidPool } from '../www/core/sid-pool.js';
import { REPO_ROOT } from './helpers.mjs';
import { floatToBits, put, readRows } from './raw-support.mjs';

// 분석 도구와 같은 합성 풀 경계와 입력 번호(다리 50, 링크 60, 첫/둘째 참조 61/62, 후속 63, 기타 64)이다. 실제 게임 월드 번호가 아니다. (2026-11-10 WB04-E247)
const CAPACITY = 24000;
const BRIDGE = 50;
const LINK = 60;
const FIRST = 61;
const SECOND = 62;
const FOLLOW = 63;
const OTHER = 64;

// extra 위치(10.78)와 stride 이다. (2026-11-10 WB04-E247)
const EXTRA = 40;
const STRIDE = 50;

// 10.78(originals) 행만 쓴다. 파일이 없으면 검사를 건너뛴다. (2026-11-10 WB04-E247)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'bridgeeffects-x86.tsv'))?.filter((row) => row[1] === 'originals') ?? null;
const kind = (name) => rows.filter((row) => row[0] === name);

// 10진 문자열 DWORD 를 부호 없는 수로 읽는다. (2026-11-10 WB04-E247)
const number = (text) => Number(BigInt(text) & 0xffffffffn);

// cpppj `Scene`: 같은 풀의 할당 슬롯을 보존하고 고정 탐색 목록과 사건 기록 효과 장치로 입력을 재현한다. 풀 번호 5~64 를 확보한다. (2026-11-10 WB04-E247)
class Scene {
    constructor() {
        this.pool = new SidPool(CAPACITY, { server: false });
        this.slots = new Array(65);
        // 클라이언트 영역의 앞 슬롯들을 할당해 합성 SID 50~64 를 준비한다.
        for (let id = 5; id <= 64; id += 1) {
            assert.equal(this.pool.allocate(2), id);
            this.slots[id] = this.pool.allocated(id);
        }
        this.types = Array.from({ length: 188 }, () => ({ flags2: 0 }));
        this.types[74].flags2 = 4;
        this.types[75].flags2 = 0;
        this.types[76].flags2 = 0x10004;
        this.types[77].flags2 = 0x10000;
        this.selected = [];
        this.cursor = 0;
        this.events = [];
        this.dynamic = false;
    }

    // 실제 원본 입력과 같은 슬롯/링크/탐색 목록으로 되돌린다. post 이면 후행 목록과 TYPE 77/75/76 입력이다.
    prepare(extra, x, y, scenario, post) {
        this.events = [];
        this.cursor = 0;
        this.dynamic = false;
        // 각 객체의 raw 슬롯을 초기화한다. 이전 행의 dead/free 상태를 넘기지 않는다.
        for (const sid of [BRIDGE, LINK, FIRST, SECOND, FOLLOW, OTHER]) {
            const raw = this.slots[sid];
            raw.fill(0);
            raw[10] = sid === LINK || sid === FOLLOW ? 75 : 74;
            raw[11] = sid === FIRST ? 2 : 0;
            raw[EXTRA] = sid === BRIDGE ? extra : 0;
            put(raw, 14, x);
            put(raw, 18, y);
        }
        put(this.slots[LINK], 12, FIRST, 2);
        put(this.slots[LINK], 8, SECOND, 2);
        put(this.slots[FOLLOW], 12, LINK, 2);
        put(this.slots[FOLLOW], 8, SECOND, 2);
        const plans = [[], [LINK, FOLLOW, OTHER], [FOLLOW, LINK, OTHER], [LINK, LINK, FOLLOW], [OTHER], [LINK, FOLLOW], [FOLLOW, LINK]];
        this.selected = plans[scenario];
        if (scenario === 5) this.slots[FIRST][EXTRA] = 9;
        if (scenario === 6) this.slots[SECOND][11] = 2;
        if (post) {
            const postPlans = [[], [LINK, FOLLOW, OTHER], [LINK, FOLLOW, OTHER], [LINK, LINK, OTHER], [FOLLOW]];
            this.selected = postPlans[scenario];
            this.slots[LINK][10] = 77;
            this.slots[FOLLOW][10] = 75;
            this.slots[OTHER][10] = 76;
            this.dynamic = scenario === 2;
        }
    }

    // 실제 외부 효과 경계를 기록하고 같은 풀의 dead/type 변경을 적용한다.
    hooks() {
        const E = BRIDGE_LIFECYCLE_EFFECT;
        return {
            // 두 정수 사각형 범위를 기록하고 고정 목록의 첫 항목을 준다.
            begin: (range) => {
                this.events.push(`Q:${range.left}:${range.top}:${range.right}:${range.bottom}`);
                this.cursor = 0;
                return this.selected.length ? this.selected[0] : 0;
            },
            // 고정 탐색 목록의 다음 항목이다. 목록은 원본 일반 탐색기를 대체한 입력 계약이다.
            next: () => {
                this.cursor += 1;
                return this.cursor < this.selected.length ? this.selected[this.cursor] : 0;
            },
            // 가상 destroy/fall 은 이 독립 검사의 동일한 대체 효과(dead/type)만 수행한다.
            emit: (event) => {
                const id = event.sid;
                const flags = event.flags;
                switch (event.effect) {
                case E.destroyLink: this.events.push(`D:${id}:${flags}`); this.slots[id][11] |= 2; break;
                case E.notifyRemoval: this.events.push(`N:${id}`); break;
                case E.fallSound: this.events.push(`S:${floatToBits(event.x)}:${floatToBits(event.y)}`); break;
                case E.fallWalker:
                    this.events.push(`F:${id}`);
                    this.slots[id][11] |= 2;
                    if (this.dynamic && id === LINK) this.slots[FOLLOW][10] = 76;
                    break;
                case E.basePreDestroy: this.events.push(`A:${id}:${flags}`); break;
                case E.basePostDestroy: this.events.push(`B:${id}:${flags}`); break;
                default: assert.fail(`모르는 효과 ${event.effect}`);
                }
            },
        };
    }

    // 효과 순서의 fixture 표현이다(빈 목록은 '-').
    eventsText() { return this.events.length ? this.events.join(';') : '-'; }

    // 전체 풀에서 예상한 타입/state 두 필드 외에는 쓰기가 없는지도 검사한다.
    checkState(expected, before) {
        const next = Uint8Array.from(before);
        // 각 객체의 독립 기대값을 읽어 풀 사본의 해당 두 바이트만 갱신한다.
        for (const tuple of expected.split(';')) {
            const [sid, type, state] = tuple.split(',').map(number);
            next[sid * STRIDE + 10] = type;
            next[sid * STRIDE + 11] = state;
        }
        assert.deepEqual(this.pool.bytes(), next);
    }
}

test('bridge-lifecycle: linkNeedsDestroy 가 원본 10.78 Link 882행(참조 경계·state·비대칭 extra)과 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = new Scene();
    const lifecycle = new RawBridgeLifecycle(scene.pool, scene.types);
    let count = 0;
    // 패치 helper 의 모든 경계/state/extra 조합이다.
    for (const row of kind('Link')) {
        scene.prepare(0, 0, 0, 0, false);
        put(scene.slots[LINK], 12, number(row[3]), 2);
        put(scene.slots[LINK], 8, number(row[4]), 2);
        scene.slots[FIRST][11] = number(row[5]);
        scene.slots[SECOND][11] = number(row[6]);
        scene.slots[FIRST][EXTRA] = number(row[7]);
        scene.slots[SECOND][EXTRA] = number(row[8]);
        assert.equal(lifecycle.linkNeedsDestroy(LINK), row[9] === 'D:60:0', `Link ${count + 1}`);
        count += 1;
    }
    assert.equal(count, 882);
});

// 실제 pre/postDestroy 상위 호출의 모든 효과·탐색 범위·슬롯 바이트를 검사한다. native 이면 연결 TYPE 을 현재 전역 getter 로 읽는다.
function replay(name, expectedCount, native = false) {
    const scene = new Scene();
    const lifecycle = new RawBridgeLifecycle(scene.pool, scene.types);
    let count = 0;
    for (const row of kind(name)) {
        scene.prepare(number(row[3]), number(row[5]), number(row[6]), number(row[7]), name === 'Post');
        const before = scene.pool.bytes();
        const hooks = scene.hooks();
        if (name === 'Pre') {
            if (native) lifecycle.preDestroyNative(BRIDGE, number(row[4]), () => 75, hooks);
            else lifecycle.preDestroy(BRIDGE, number(row[4]), 75, hooks);
        } else lifecycle.postDestroy(BRIDGE, number(row[4]), hooks);
        assert.equal(scene.eventsText(), row[8], `${name} ${count + 1} 효과`);
        scene.checkState(row[9], before);
        count += 1;
    }
    assert.equal(count, expectedCount);
}

test('bridge-lifecycle: preDestroy 가 원본 10.78 Pre 840행의 탐색 범위·연결 삭제 순서·슬롯과 같다', { skip: !rows && '원본 fixture 없음' }, () => replay('Pre', 840));
test('bridge-lifecycle: Native preDestroyNative(현재 연결 TYPE getter)도 같은 Pre 840행을 재생한다', { skip: !rows && '원본 fixture 없음' }, () => replay('Pre', 840, true));
test('bridge-lifecycle: postDestroy 가 원본 10.78 Post 600행의 파편·소리 좌표·walker 낙하·공통 후처리와 같다', { skip: !rows && '원본 fixture 없음' }, () => replay('Post', 600));

test('bridge-lifecycle: delayedFallPayload 가 원본 10.78 Delay·Pack 484행의 수치 float 비트와 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    let delay = 0;
    let pack = 0;
    // 비유한 값·음수·바이트 경계·float 정밀도 경계도 실제 _ftol 결과와 비교한다.
    for (const row of rows) {
        if (row[0] !== 'Delay' && row[0] !== 'Pack') continue;
        const view = new DataView(new ArrayBuffer(8));
        view.setUint32(0, number(row[3]), true);
        view.setUint32(4, number(row[4]), true);
        const actual = RawBridgeLifecycle.delayedFallPayload(view.getFloat32(0, true), view.getFloat32(4, true));
        assert.equal(floatToBits(actual), number(row[5]), `${row[0]} ${row[3]},${row[4]}`);
        if (row[0] === 'Delay') delay += 1;
        else pack += 1;
    }
    assert.deepEqual([delay, pack], [242, 242]);
    assert.equal(BRIDGE_FALL_EVENT, 0x2692);
});

test('bridge-lifecycle: 훅 누락과 SID 0 은 효과를 시작하기 전에 거부한다', () => {
    const scene = new Scene();
    scene.prepare(0, 0, 0, 0, false);
    const lifecycle = new RawBridgeLifecycle(scene.pool, scene.types);
    const hooks = scene.hooks();
    hooks.next = null;
    assert.throws(() => lifecycle.postDestroy(BRIDGE, 0, hooks), TypeError);
    assert.deepEqual(scene.events, []);
    assert.throws(() => lifecycle.preDestroy(0, 0, 75, scene.hooks()), RangeError);
    assert.deepEqual(scene.events, []);
    // Native 는 getter 가 아닌 값을 받지 않는다.
    assert.throws(() => lifecycle.preDestroyNative(BRIDGE, 0, 75, scene.hooks()), TypeError);
    assert.deepEqual(scene.events, []);
});

// 첫 연결 삭제가 TYPE 전역을 바꾸면 다음 후보는 현재 새 DWORD 로 비교한다(값으로 받는 typed 경로는 이를 보지 못한다).
test('bridge-lifecycle: Native 는 첫 연결 삭제 뒤 바뀐 전역을 다음 후보에서 읽는다', () => {
    const scene = new Scene();
    const half = floatToBits(50.5);
    scene.prepare(0, half, half, 1, false);
    const lifecycle = new RawBridgeLifecycle(scene.pool, scene.types);
    let current = 75;
    const hooks = scene.hooks();
    const effect = hooks.emit;
    hooks.emit = (event) => {
        effect(event);
        if (event.effect === BRIDGE_LIFECYCLE_EFFECT.destroyLink && event.sid === LINK) {
            current = 76;
            scene.slots[FOLLOW][10] = 76;
        }
    };
    lifecycle.preDestroyNative(BRIDGE, 0x1234, () => current, hooks);
    assert.ok(scene.events.includes('D:60:0'));
    assert.ok(scene.events.includes('D:63:0'));
    assert.equal(scene.events.at(-1), 'A:50:4660');
    // 값으로 받는 typed 경로는 같은 장면에서 전역 변경을 볼 수 없어도 후속 슬롯 TYPE 만 76 이 되어 63 은 삭제하지 않는다.
    const typed = new Scene();
    typed.prepare(0, half, half, 1, false);
    const typedHooks = typed.hooks();
    const typedEffect = typedHooks.emit;
    typedHooks.emit = (event) => {
        typedEffect(event);
        if (event.effect === BRIDGE_LIFECYCLE_EFFECT.destroyLink && event.sid === LINK) typed.slots[FOLLOW][10] = 76;
    };
    new RawBridgeLifecycle(typed.pool, typed.types).preDestroy(BRIDGE, 0x1234, 75, typedHooks);
    assert.ok(typed.events.includes('D:60:0'));
    assert.ok(!typed.events.includes('D:63:0'));
});

// fixture 가 구별하지 못하는 경계(변형 시험에서 살아남은 입력): preDestroy 범위의 거의 올림 보정값은 float 비트 0x3f7fff58 이며 1 ulp 큰 값과 칸이 갈리는 좌표가 있고,
// 낙하 소리 효과가 좌표를 바꾸면 postDestroy 는 그 뒤 좌표를 다시 읽어 탐색 칸을 정한다. 낙하 좌표의 _ftol 상한(2^63 정확히)은 하위 DWORD 가 0 이라 구별되지 않는 동등 변형이다. (2026-11-10 WB04-E247)
test('bridge-lifecycle: preDestroy 보정값 경계와 낙하 소리 뒤 좌표 재조회', () => {
    // 0x3727ffff(약 1.00135794e-5)는 보정값을 더해도 1 에 못 미쳐 0 칸, 0x37280000(정확히 1-보정값)은 정확히 1 이 되어 1 칸이다.
    for (const [bits, cell] of [[0x3727ffff, 0], [0x37280000, 1]]) {
        const scene = new Scene();
        scene.prepare(0, bits, bits, 0, false);
        new RawBridgeLifecycle(scene.pool, scene.types).preDestroy(BRIDGE, 0, 75, scene.hooks());
        assert.equal(scene.events[0], `Q:${cell - 1}:${cell - 1}:${cell + 1}:${cell + 1}`, `좌표 비트 ${bits.toString(16)}`);
    }
    // 소리 효과가 좌표를 바꾸면 이후 탐색은 바뀐 좌표의 칸이고 소리는 바뀌기 전 좌표이다.
    const scene = new Scene();
    scene.prepare(0, floatToBits(20.5), floatToBits(21.5), 0, true);
    const hooks = scene.hooks();
    const effect = hooks.emit;
    hooks.emit = (event) => {
        effect(event);
        if (event.effect === BRIDGE_LIFECYCLE_EFFECT.fallSound) {
            put(scene.slots[BRIDGE], 14, floatToBits(70.5));
            put(scene.slots[BRIDGE], 18, floatToBits(80.5));
        }
    };
    new RawBridgeLifecycle(scene.pool, scene.types).postDestroy(BRIDGE, 0, hooks);
    assert.deepEqual(scene.events, ['N:50', `S:${floatToBits(20.5)}:${floatToBits(21.5)}`, 'Q:70:80:70:80', 'B:50:0']);
});

// BYTE 원시 TYPE 와 전체 DWORD 전역을 비교하므로 낮은 8비트만 같아도 연결 삭제가 되면 안 된다.
test('bridge-lifecycle: Native 연결 TYPE 비교는 전체 DWORD 이다', () => {
    const scene = new Scene();
    const half = floatToBits(50.5);
    scene.prepare(0, half, half, 1, false);
    new RawBridgeLifecycle(scene.pool, scene.types).preDestroyNative(BRIDGE, 0, () => 0x1004b, scene.hooks());
    assert.ok(scene.events.every((text) => !text.startsWith('D:')));
});
