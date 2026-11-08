// AI 배치 TYPE 선택 raw-ai-drop-selection.js(RawAiDropSelection)를 cpppj 계약 시험(손으로 쓴 입력과 기대값)으로 검사한다.
//  - 이 몸체에는 독립 원본 관측 fixture 가 없다(cpppj 시험이 원본 분기 순서를 직접 단언한다). 같은 입력과 단언을 옮겼다: 기술 0개는 진행/재선택 표식과 TYPE 만 쓰고 직전 상태 보존, 현재 owner 조회는 한 번이고 기술 부모/개수는 최초 값,
//    현재 네 방향 전역만 전체 DWORD 로 비교하고 6 미만 회전은 2씩 증가(signed 비교의 홀수/음수 방향 포함), ordinal 은 DWORD 증가 뒤 signed 나머지, 능력 BYTE 0x80 은 직전 배치가 없어도 난수 100 을 쓰고 상위 BYTE 는 쓰지 않음,
//    실제 목록 압축 뒤 같은 index 재검사와 직전 점수/회전 유지, 80/30 확률 경계, 재사용 승인 쿼리(현재 TYPE/owner BYTE/flags 0), 블로커 우선(반복보다 우선·현재 블로커·부재·회전 예산), 최고 점수는 count 0 에서만 0 으로,
//    운반체와 공급 TYPE 의 less/unordered 쿨다운, 선택된 nth 누락의 부분 쓰기 보존.
//  - E173 `RawAiDropScan` 의 실제 30후보 압축(remove)을 같은 저장소에서 사용한다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E174: AI 배치 TYPE 선택 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import { put, get, floatToBits, bitsToFloat } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { GameRandom } from '../www/core/random.js';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { AI_BRAIN_BYTES, RawAiBrainMemory } from '../www/core/raw-ai-brain.js';
import { RawAiDropScan } from '../www/core/raw-ai-drop-scan.js';
import { AI_DROP_SELECTION_LOG, RawAiDropSelection } from '../www/core/raw-ai-drop-selection.js';

// 예외가 나는지 본다. (2026-11-09 WB04-E174)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 같은 실제 본체/후보 압축·공용 난수와 명시 기술 getter 를 준비한다. (2026-11-09 WB04-E174)
class DropSelectionScene {
    // 기본은 새 순환 선택/빈 후보/직전 점수 123 이며 최초 ordinal 0 에서 1 로 진행한다.
    constructor() {
        this.memory = new RawAiBrainMemory(0x53000000, (raw) => raw.fill(0));
        this.token = this.memory.allocate(AI_BRAIN_BYTES);
        this.list = this.memory.allocate(32);
        this.pool = new SidPool(32768, { server: false });
        this.types = Array.from({ length: 188 }, () => ({ flags2: 0 }));
        this.frames = Array.from({ length: 188 }, () => new FrameCodeTable([]));
        this.spots = new Uint8Array(65536);
        this.surfaces = new Uint16Array(65536);
        this.allies = new Uint32Array(81);
        this.bounds = { left: 10, top: 10, right: 246, bottom: 246 };
        this.bridge = 82;
        this.random = new GameRandom(1);
        this.now = 100;
        this.directional = [131, 129, 140, 142];
        this.parent = 5;
        this.blocker = 0;
        this.count = 2;
        this.products = [128, 129];
        this.trace = [];
        this.queries = [];
        this.legal = [];
        this.duringTechnology = null;
        this.duringCount = null;
        this.duringBlocker = null;
        this.duringNth = null;
        this.duringLog = null;
        this.duringPlace = null;
        assert.equal(this.pool.allocate(2), 5);
        this.field(0x260, 1);
        this.field(0x36c, this.list);
        this.field(0x3e8, 123);
        this.field(0x3f0, 9);
        this.field(0x3ec, 7);
        this.field(0xaf8, 8);
        this.field(0xafc, 9);
        this.scan = new RawAiDropScan(this.memory, this.pool, this.types, this.frames, this.spots, this.surfaces, this.allies, this.random, { bridgeType: () => this.bridge, bounds: () => this.bounds }, {
            assertion: () => { throw new Error('명시 후보 압축 진단'); }
        });
        this.hooks = {
            technology: (owner) => {
                assert.ok(this.field(0xaf8) === 10 && this.field(0xafc) === 10 && !this.field(0x3ec));
                this.trace.push(`tech:${owner}`);
                const saved = this.parent;
                if (this.duringTechnology) this.duringTechnology();
                return saved;
            },
            count: (technology) => { this.trace.push(`count:${technology}`); const saved = this.count; if (this.duringCount) this.duringCount(); return saved; },
            blocker: (technology) => { this.trace.push(`blocker:${technology}`); if (this.duringBlocker) this.duringBlocker(); return this.blocker; },
            nth: (technology, index) => { this.trace.push(`nth:${technology}:${index}`); if (this.duringNth) this.duringNth(); return index < this.products.length ? this.products[index] : 0; },
            mayPlace: (query) => {
                const index = this.queries.length;
                this.queries.push(query);
                this.trace.push(`place:${Math.trunc(query.x)}`);
                if (this.duringPlace) this.duringPlace(index);
                return this.legal.length === 0 || this.legal[Math.min(index, this.legal.length - 1)];
            },
            remove: (brain, index) => { assert.equal(brain, this.token); this.trace.push(`remove:${index}`); this.scan.remove(brain, index); },
            log: (brain, action, type, direction) => {
                assert.equal(brain, this.token);
                this.trace.push(action === AI_DROP_SELECTION_LOG.hated ? 'hate' : action === AI_DROP_SELECTION_LOG.retried ? 'retry' : `scan:${type}:${direction}`);
                if (this.duringLog) this.duringLog(action);
            }
        };
    }

    // 현재 본체 DWORD 를 읽거나(값 없음) 지정 DWORD 만 입력한다. (2026-11-09 WB04-E174)
    field(at, value) {
        if (value === undefined) return get(this.memory.bytes(this.token), at);
        put(this.memory.bytes(this.token), at, value >>> 0);
        return undefined;
    }

    // 원본 QWORD 기한을 축소 없이 입력한다. (2026-11-09 WB04-E174)
    time(at, value) {
        const view = new DataView(new ArrayBuffer(8));
        view.setFloat64(0, value, true);
        this.field(at, view.getUint32(0, true));
        this.field(at + 4, view.getUint32(4, true));
    }

    // 직전 표적 목록의 순서/중복을 그대로 기록한다. (2026-11-09 WB04-E174)
    listOf(values) {
        this.field(0x374, values.length);
        // 입력 목록 순서를 보존한다.
        values.forEach((value, index) => put(this.memory.bytes(this.list), index * 4, value));
    }

    // 실제 같은 후보 XY/표적 배열의 명시 입력이다. (2026-11-09 WB04-E174)
    choice(index, x, y, target) {
        this.field(0x408 + index * 8, floatToBits(x));
        this.field(0x40c + index * 8, floatToBits(y));
        this.field(0x4f8 + index * 4, target);
    }

    // 원본 반복 관문을 통과할 명시 직전 상태를 준비한다. (2026-11-09 WB04-E174)
    repeat(type = 128) {
        this.field(0x248, 0x80);
        this.field(0x3f4, 1);
        this.field(0x3e0, type);
        this.field(0x3fc, 77);
        this.field(0xb00, 1);
        this.listOf([77]);
    }

    // 같은 실제 본체/풀/난수/현재 방향 전역으로 선택기를 만든다. (2026-11-09 WB04-E174)
    body() { return new RawAiDropSelection(this.memory, this.pool, this.types, this.directional, this.random, () => this.now, this.hooks); }

    // 현재 실제 저장소와 하위로 한 번 선택한다. (2026-11-09 WB04-E174)
    select() {
        const body = this.body();
        assert.equal(body.pool, this.pool);
        body.select(this.token);
    }
}

test('AI 배치 선택: 기술 0개는 진행/완료 표식·TYPE/재선택만 쓰고 후보·점수·직전 상태는 보존한다', () => {
    const scene = new DropSelectionScene();
    scene.count = 0;
    scene.repeat();
    scene.field(0x3e4, 6);
    scene.field(0x570, 2);
    scene.field(0x404, 1);
    scene.select();
    assert.ok(JSON.stringify(scene.trace) === '["tech:1","count:5"]' && !scene.field(0x3e0) && scene.field(0x3f0) === 1 && scene.field(0x3e4) === 6 && scene.field(0x570) === 2 && scene.field(0x3e8) === 123
        && scene.field(0x3f4) === 1 && scene.field(0x3fc) === 77 && scene.field(0x404) === 1 && scene.random.state === 1);
});

test('AI 배치 선택: 현재 owner 조회는 한 번이고 나머지는 최초 기술 부모/개수를 계속 사용한다', () => {
    const scene = new DropSelectionScene();
    scene.field(0xb00, 1);
    scene.duringTechnology = () => { scene.field(0x260, 2); scene.parent = 6; };
    scene.duringCount = () => { scene.count = 0; };
    scene.select();
    assert.ok(JSON.stringify(scene.trace) === '["tech:1","count:5","nth:5:1","scan:129:0"]' && scene.field(0x3e0) === 129 && scene.field(0x3dc) === 1 && !scene.field(0x3e8) && !scene.field(0x3f0) && scene.field(0x400) === 1);
});

test('AI 배치 선택: 네 현재 전역만 전체 DWORD 로 비교하고 6 미만 회전은 2씩 증가한다', () => {
    const scene = new DropSelectionScene();
    const body = scene.body();
    assert.ok(body.directional(131) && !body.directional(0x10083));
    scene.directional[0] = 130;
    assert.ok(body.directional(130) && !body.directional(131));
    scene.field(0x3e0, 130);
    scene.field(0x3e4, 4);
    scene.select();
    assert.ok(scene.field(0x3e0) === 130 && scene.field(0x3e4) === 6 && JSON.stringify(scene.trace) === '["tech:1","count:5"]');
});

test('AI 배치 선택: 방향은 signed 비교라 홀수/음수도 원본 그대로 2를 더하고 6 이상은 기술을 순환한다', () => {
    const scene = new DropSelectionScene();
    scene.field(0x3e0, 131);
    scene.field(0x3e4, 5);
    scene.select();
    assert.ok(scene.field(0x3e4) === 7 && scene.field(0x3e0) === 131);
    scene.trace.length = 0;
    scene.field(0x3e4, 0xffffffff);
    scene.select();
    assert.equal(scene.field(0x3e4), 1);
    scene.field(0x3e4, 6);
    scene.select();
    assert.ok(scene.field(0x3e0) === 129 && !scene.field(0x3e4) && scene.field(0x3dc) === 1);
});

test('AI 배치 선택: ordinal 은 DWORD 증가 뒤 signed 나머지이며 음수는 다른 양수 인덱스로 고치지 않는다', () => {
    const scene = new DropSelectionScene();
    scene.field(0x3dc, 0xffffffff);
    scene.select();
    assert.ok(scene.field(0x3dc) === 0 && scene.field(0x3e0) === 128);
    scene.field(0x3e0, 0);
    scene.field(0x3dc, 0xfffffffe);
    scene.select();
    assert.ok(scene.field(0x3dc) === 0xffffffff && !scene.field(0x3e0) && scene.trace.at(-1) === 'nth:5:4294967295');
});

test('AI 배치 선택: 하위 BYTE 80h 는 직전 배치가 없어도 난수 100 을 쓰며 상위 BYTE 80h 는 쓰지 않는다', () => {
    const scene = new DropSelectionScene();
    scene.field(0x248, 0x80);
    const expected = new GameRandom(1);
    expected.next(100);
    scene.select();
    assert.equal(scene.random.state, expected.state);
    const higher = new DropSelectionScene();
    higher.field(0x248, 0x8000);
    higher.select();
    assert.equal(higher.random.state, 1);
});

test('AI 배치 선택: 실제 목록 압축 뒤 같은 index 에서 다음 후보를 재검사하고 직전 점수·회전을 유지한다', () => {
    const scene = new DropSelectionScene();
    scene.repeat();
    scene.field(0x3e4, 4);
    scene.field(0x570, 3);
    scene.choice(0, 11, 12, 7);
    scene.choice(1, 21, 22, 8);
    scene.choice(2, 31, 32, 9);
    scene.legal = [false, false, true];
    scene.select();
    assert.deepEqual(scene.trace, ['tech:1', 'count:5', 'hate', 'place:11', 'remove:0', 'place:21', 'remove:0', 'place:31', 'retry', 'scan:128:4']);
    assert.ok(scene.field(0x570) === 1 && scene.field(0x3e8) === 123 && !scene.field(0x400) && !scene.field(0x3f4) && !scene.field(0x3fc) && bitsToFloat(scene.field(0x408)) === 31 && scene.field(0x4f8) === 9);
});

test('AI 배치 선택: 직전 표적 목록의 80/30 확률 경계와 비어 있지 않은 비일치 목록을 구분한다', () => {
    // 작은 원본 시드는 첫 next(100) 이 시드와 같으므로 실제 경계 입력으로 쓴다.
    for (const seed of [79, 80, 29, 30]) {
        const scene = new DropSelectionScene();
        scene.repeat();
        scene.random.setState(seed);
        if (seed < 40) scene.listOf([]);
        scene.select();
        const repeated = seed === 79 || seed === 29;
        assert.equal(scene.trace.includes('retry'), repeated);
    }
    const nonempty = new DropSelectionScene();
    nonempty.repeat();
    nonempty.listOf([88]);
    nonempty.select();
    assert.ok(!nonempty.trace.includes('retry'));
});

test('AI 배치 선택: 재사용 승인 쿼리는 현재 TYPE/owner BYTE 이며 회전값은 flags 에 넣지 않는다', () => {
    const scene = new DropSelectionScene();
    scene.repeat();
    scene.field(0x570, 2);
    scene.choice(0, 11, 12, 7);
    scene.choice(1, 21, 22, 8);
    scene.duringLog = (action) => { if (action === AI_DROP_SELECTION_LOG.hated) { scene.field(0x3e0, 129); scene.field(0x260, 0x102); } };
    scene.select();
    assert.equal(scene.queries.length, 2);
    // 각 현재 승인 입력을 원본 계약과 비교한다.
    for (const query of scene.queries) assert.ok(query.type === 129 && query.owner === 2 && !query.argument && !query.flags && query.mode === 1);
});

test('AI 배치 선택: 블로커가 있으면 반복보다 우선하고 원본 순환 예산 안에서 찾은 TYPE 만 선택한다', () => {
    const scene = new DropSelectionScene();
    scene.repeat();
    scene.field(0x404, 1);
    scene.count = 3;
    scene.products = [128, 136, 142];
    scene.types[142].flags2 = 0x4000000;
    scene.blocker = 6;
    scene.select();
    assert.ok(scene.field(0x3e0) === 142 && !scene.field(0x404) && !scene.field(0x570) && !scene.field(0x3e8));
    assert.deepEqual(scene.trace, ['tech:1', 'count:5', 'hate', 'blocker:5', 'nth:5:1', 'nth:5:2', 'scan:142:0']);
});

test('AI 배치 선택: 현재 TYPE 가 블로커면 조회를 생략하고 재사용 경로의 404 를 강제로 지우지 않는다', () => {
    const scene = new DropSelectionScene();
    scene.repeat(142);
    scene.types[142].flags2 = 0x4000000;
    scene.field(0x404, 1);
    scene.field(0x570, 1);
    scene.choice(0, 11, 12, 7);
    scene.select();
    assert.ok(scene.field(0x404) === 1 && scene.field(0x570) === 1 && scene.field(0x3e8) === 123 && !scene.trace.includes('blocker:5'));
});

test('AI 배치 선택: 블로커 부재는 404 만 지우고 원본 반복을 허용하며 기술 개수+2 회전 예산도 그대로다', () => {
    const absent = new DropSelectionScene();
    absent.repeat();
    absent.field(0x404, 1);
    absent.select();
    assert.ok(!absent.field(0x404) && absent.trace.includes('retry'));
    const bounded = new DropSelectionScene();
    bounded.field(0x404, 1);
    bounded.field(0x3e0, 131);
    bounded.count = 1;
    bounded.products = [128];
    bounded.blocker = 6;
    bounded.select();
    assert.ok(bounded.field(0x3e0) === 131 && bounded.field(0x3e4) === 6 && !bounded.field(0x404) && JSON.stringify(bounded.trace) === '["tech:1","count:5","blocker:5"]');
});

test('AI 배치 선택: 최고 점수는 count==0 에서만 0 으로 만들고 signed 음수 count 는 점수를 보존한다', () => {
    const scene = new DropSelectionScene();
    scene.repeat();
    scene.field(0x570, 0xffffffff);
    scene.select();
    assert.ok(scene.field(0x570) === 0xffffffff && scene.field(0x3e8) === 123 && scene.field(0x400) === 1);
    const empty = new DropSelectionScene();
    empty.repeat();
    empty.select();
    assert.ok(!empty.field(0x570) && !empty.field(0x3e8) && empty.field(0x400) === 1);
});

test('AI 배치 선택: 운반체는 선택을 지우고 공급 TYPE 의 less/unordered 기한도 재선택 상태로 만든다', () => {
    const carrier = new DropSelectionScene();
    carrier.products[1] = 133;
    carrier.types[133].flags2 = 0x10000;
    carrier.select();
    assert.ok(!carrier.field(0x3e0) && carrier.field(0x3f0) === 1);
    // NaN 도 미도달이며 equality 는 도달이다. 저장 QWORD 를 float 로 줄이지 않는다.
    for (const due of [100, 100.00000001, Number.NaN]) {
        const scene = new DropSelectionScene();
        scene.products[1] = 136;
        scene.types[136].flags2 = 0x800;
        scene.time(0x5b8, due);
        scene.select();
        assert.equal(scene.field(0x3e0) === 136, due === 100);
        assert.equal(scene.field(0x3f0), due === 100 ? 0 : 1);
    }
    const nan = new DropSelectionScene();
    nan.products[1] = 136;
    nan.types[136].flags2 = 0x800;
    nan.now = Number.NaN;
    nan.select();
    assert.ok(!nan.field(0x3e0) && nan.field(0x3f0) === 1);
});

test('AI 배치 선택: 선택된 nth 누락은 이미 기록한 순번/진행을 보존하고 뒤 정리를 수행하지 않는다', () => {
    const scene = new DropSelectionScene();
    scene.field(0x3e0, 128);
    scene.field(0x3e4, 4);
    scene.hooks.nth = undefined;
    assert.ok(throws(() => scene.select()) && scene.field(0x3dc) === 1 && scene.field(0x3e0) === 128 && scene.field(0x3e4) === 4 && scene.field(0x3e8) === 123 && scene.field(0x3f0) === 9 && scene.field(0xaf8) === 10);
});

test('AI 배치 선택 생성자: 본체 소유자/PATCH 풀/188 TYPE/네 방향 TYPE/난수/시각 조회기가 없으면 거부하고 가짜 본체를 거부한다', () => {
    const scene = new DropSelectionScene();
    const make = (overrides) => () => new RawAiDropSelection(...Object.values({ memory: scene.memory, pool: scene.pool, types: scene.types, directional: scene.directional, random: scene.random, now: () => 0, hooks: scene.hooks, ...overrides }));
    assert.ok(throws(make({ memory: undefined })) && throws(make({ pool: {} })) && throws(make({ types: scene.types.slice(0, 10) })) && throws(make({ directional: [1, 2, 3] })) && throws(make({ random: undefined })) && throws(make({ now: undefined })));
    assert.ok(!throws(make({ hooks: undefined })));
    assert.ok(throws(() => scene.body().select(scene.memory.allocate(16))));
});
