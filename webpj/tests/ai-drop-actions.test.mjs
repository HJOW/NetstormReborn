// AI 배치 행동 raw-ai-drop-actions.js(RawAiDropActions)를 cpppj 계약 시험(손으로 쓴 입력과 기대값)으로 검사한다.
//  - 이 몸체에는 독립 원본 관측 fixture 가 없다(cpppj 시험이 원본 분기 순서를 직접 단언한다). 같은 입력과 단언을 옮겼다: 초기 TYPE 0 은 생산 조회를 하지 않고 선택 뒤 TYPE 0 이면 뒤 스캔/관문 생략, 생산능력 상실 알림은 TYPE 를 지우지 않음,
//    스캔 입력은 선택 직후 snapshot 이고 완료 로그는 스캔 뒤 현재 TYPE/방향과 이미 쓴 완료 flag, 두 QWORD 기한/NaN 현재 시각의 ordered 미도달, signed 지갑 1200, 최초 시도 400 은 완료를 기다리고 재사용은 Partial 알림 뒤 진행,
//    현재 생산 조각의 pending bit 만 거름, 사제 탐색의 signed 진행과 strict 목표 좌표, 빈 후보 진단 뒤 현재 TYPE flags 로 fake/seed, 비용 거부는 직전 상태 보존, 공용 난수로 후보를 현재 count 로 소비하고 실패 후보도 실제 압축,
//    모든 후보 거부는 집중 표적 보존, 승인/로그/배치 뒤 현재 후보·TYPE/방향·표적과 owner/focus 읽기, 집중은 하위 BYTE 40h 만, 중단은 압축 효과 뒤 현재 +0x3f4 로만, 비용 비교가 count 를 0 으로 바꾸는 경우, 누락 효과의 부분 쓰기 보존, 로그가 꺼져도 물리 표적 조회.
//  - E173 `RawAiDropScan` 의 실제 30후보 압축(remove)을 같은 저장소에서 사용한다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E174: AI 배치 행동 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import { put, get, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { GameRandom } from '../www/core/random.js';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { AI_BRAIN_BYTES, RawAiBrainMemory } from '../www/core/raw-ai-brain.js';
import { RawAiDropScan } from '../www/core/raw-ai-drop-scan.js';
import { AI_DROP_ACTION_LOG, RawAiDropActions } from '../www/core/raw-ai-drop-actions.js';

// 예외가 나는지 본다. (2026-11-09 WB04-E174)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

const F = Math.fround;

// 원래 알림의 검사 trace 이름이다. (2026-11-09 WB04-E174)
const NAMES = Object.freeze({
    [AI_DROP_ACTION_LOG.lostAbility]: 'lost', [AI_DROP_ACTION_LOG.scanComplete]: 'complete', [AI_DROP_ACTION_LOG.partialScan]: 'partial', [AI_DROP_ACTION_LOG.cannotAfford]: 'cannot',
    [AI_DROP_ACTION_LOG.badChoice]: 'bad', [AI_DROP_ACTION_LOG.laying]: 'lay', [AI_DROP_ACTION_LOG.target]: 'target'
});

// 같은 실제 본체·풀과 기존 실제 후보 압축기를 명시 원문 효과에 연결한다. (2026-11-09 WB04-E174)
class DropActionScene {
    // 기본은 TYPE 128/스캔 완료/지갑 2000/빈 후보이며 이전 성공·표적·집중값은 남겨 둔다.
    constructor() {
        this.memory = new RawAiBrainMemory(0x55000000, (raw) => raw.fill(0));
        this.token = this.memory.allocate(AI_BRAIN_BYTES);
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
        this.limit = 256;
        this.checking = false;
        this.product = true;
        this.pending = false;
        this.afford = true;
        this.selectedType = 128;
        this.trace = [];
        this.products = [];
        this.scanTypes = [];
        this.scanDirections = [];
        this.queries = [];
        this.legal = [];
        this.deployments = [];
        this.notices = [];
        this.duringProduct = null;
        this.duringPlace = null;
        this.duringRemove = null;
        this.duringSelect = null;
        this.duringScan = null;
        this.duringAfford = null;
        this.duringDeploy = null;
        this.duringAssertion = null;
        this.duringLog = null;
        assert.ok(this.pool.allocate(2) === 5 && this.pool.allocate(2) === 6 && this.pool.allocate(2) === 7);
        const productRaw = this.pool.allocated(5);
        productRaw.fill(0);
        productRaw[10] = 6;
        // 두 표적은 후보 좌표와 서로 다른 raw 위치/TYPE 를 사용한다.
        for (const sid of [6, 7]) {
            const raw = this.pool.allocated(sid);
            raw.fill(0);
            raw[10] = sid === 6 ? 150 : 151;
            put(raw, 14, floatToBits(sid === 6 ? 71.8 : 93.9));
            put(raw, 18, floatToBits(sid === 6 ? 82.7 : 104.1));
        }
        this.field(0x260, 1);
        this.field(0x3e0, 128);
        this.field(0x3e4, 2);
        this.field(0x3ec, 1);
        this.field(0x2d8, 2000);
        this.field(0x3f4, 77);
        this.field(0x3fc, 88);
        this.field(0x3f8, 99);
        this.field(0xb00, 1);
        this.scan = new RawAiDropScan(this.memory, this.pool, this.types, this.frames, this.spots, this.surfaces, this.allies, this.random, { bridgeType: () => this.bridge, bounds: () => this.bounds }, {
            assertion: () => { throw new Error('예상하지 않은 빈 후보 압축'); }
        });
        this.hooks = {
            product: (brain, type) => {
                assert.equal(brain, this.token);
                const call = this.products.length;
                this.products.push(type);
                this.trace.push('product');
                const result = this.product ? 5 : 0;
                if (this.duringProduct) this.duringProduct(call);
                return result;
            },
            select: (brain) => { assert.equal(brain, this.token); this.trace.push('select'); this.field(0x3e0, this.selectedType); if (this.duringSelect) this.duringSelect(); },
            scan: (brain, type, direction) => { assert.equal(brain, this.token); this.trace.push('scan'); this.scanTypes.push(type); this.scanDirections.push(direction); if (this.duringScan) this.duringScan(); return this.pending; },
            canAfford: (brain, type) => { assert.ok(brain === this.token && type === this.field(0x3e0)); this.trace.push('afford'); if (this.duringAfford) this.duringAfford(); return this.afford; },
            mayPlace: (query) => {
                const call = this.queries.length;
                this.queries.push(query);
                this.trace.push('place');
                const result = this.legal.length === 0 ? true : this.legal[Math.min(call, this.legal.length - 1)];
                if (this.duringPlace) this.duringPlace(call);
                return result;
            },
            deploy: (brain, type, point, argument, direction) => { assert.equal(brain, this.token); this.trace.push('deploy'); this.deployments.push({ type, point, argument, direction }); if (this.duringDeploy) this.duringDeploy(); },
            fake: (brain) => { assert.equal(brain, this.token); this.trace.push('fake'); },
            seed: (brain) => { assert.equal(brain, this.token); this.trace.push('seed'); },
            remove: (brain, index) => { assert.equal(brain, this.token); this.trace.push('remove'); this.scan.remove(brain, index); if (this.duringRemove) this.duringRemove(index); },
            assertion: (text, line) => { assert.ok(text === 'drop.value == 0' && line === 0xf8b); this.trace.push('assert'); if (this.duringAssertion) this.duringAssertion(); },
            log: (brain, notice) => { assert.equal(brain, this.token); this.trace.push(NAMES[notice.action]); this.notices.push(notice); if (this.duringLog) this.duringLog(notice.action); }
        };
    }

    // 현재 본체 DWORD 를 읽거나(값 없음) 지정 DWORD 를 쓴다. (2026-11-09 WB04-E174)
    field(at, value) {
        if (value === undefined) return get(this.memory.bytes(this.token), at);
        put(this.memory.bytes(this.token), at, value >>> 0);
        return undefined;
    }

    // QWORD 기한을 float 축소 없이 저장한다. (2026-11-09 WB04-E174)
    time(at, value) {
        const view = new DataView(new ArrayBuffer(8));
        view.setFloat64(0, value, true);
        this.field(at, view.getUint32(0, true));
        this.field(at + 4, view.getUint32(4, true));
    }

    // 실제 선택 배열에 후보와 표적 DWORD 를 기록한다. (2026-11-09 WB04-E174)
    choice(index, x, y, target = 6) {
        this.field(0x408 + index * 8, floatToBits(x));
        this.field(0x40c + index * 8, floatToBits(y));
        this.field(0x4f8 + index * 4, target);
    }

    // 같은 실제 본체/풀/공용 난수와 명시 원문 하위로 행동한다. (2026-11-09 WB04-E174)
    actions() {
        return new RawAiDropActions(this.memory, this.pool, this.types, this.random, { now: () => this.now, mapLimit: () => this.limit, checking: () => Number(this.checking) }, this.hooks);
    }

    // 한 번 실행한다. (2026-11-09 WB04-E174)
    run() { this.actions().run(this.token); }
}

test('AI 배치 행동: 초기 TYPE 0 에서는 생산 조회를 하지 않으며 선택 뒤 TYPE 0 이면 뒤 스캔/관문도 생략한다', () => {
    const input = new DropActionScene();
    input.field(0x3e0, 0);
    input.selectedType = 0;
    input.run();
    assert.ok(JSON.stringify(input.trace) === '["select"]' && input.field(0x3f4) === 77 && input.field(0x3fc) === 88 && !input.field(0x3f0) && input.random.state === 1);
});

test('AI 배치 행동: 생산능력 상실 알림 자체는 TYPE 를 지우거나 재선택을 강제하지 않는다', () => {
    const input = new DropActionScene();
    input.product = false;
    input.pending = true;
    input.field(0x3ec, 0);
    input.run();
    assert.ok(JSON.stringify(input.trace) === '["product","lost","scan","product"]' && input.field(0x3e0) === 128 && !input.field(0x3f0) && !input.field(0x3ec) && input.field(0x3f4) === 77);
});

test('AI 배치 행동: 스캔 입력은 선택 직후 snapshot 이고 완료 로그는 스캔 뒤 현재 TYPE/방향과 이미 쓴 완료 flag 다', () => {
    const input = new DropActionScene();
    input.field(0x3e0, 0);
    input.field(0x3ec, 0);
    input.duringScan = () => { input.field(0x3e0, 129); input.field(0x3e4, 4); };
    input.duringLog = (action) => { if (action === AI_DROP_ACTION_LOG.scanComplete) { assert.equal(input.field(0x3ec), 1); input.time(0x5a8, 101); } };
    input.run();
    assert.ok(JSON.stringify(input.trace) === '["select","scan","complete"]' && JSON.stringify(input.scanTypes) === '[128]' && JSON.stringify(input.scanDirections) === '[2]' && input.notices[0].type === 129 && input.notices[0].direction === 4);
});

test('AI 배치 행동: 미래/NaN 의 두 기한과 NaN 현재 시각은 스캔 뒤 관문에서 ordered 미도달이다', () => {
    // 두 기한을 각각 검증하고 equality 는 통과시키며 float 로 차이를 지우지 않는다.
    for (const at of [0x5a8, 0x5e8]) {
        for (const due of [100.00000001, Number.NaN]) {
            const input = new DropActionScene();
            input.time(at, due);
            input.run();
            assert.equal(JSON.stringify(input.trace), '["product"]');
        }
    }
    const now = new DropActionScene();
    now.now = Number.NaN;
    now.run();
    assert.equal(JSON.stringify(now.trace), '["product"]');
    const equal = new DropActionScene();
    equal.time(0x5a8, 100);
    equal.time(0x5e8, 100);
    equal.run();
    assert.equal(JSON.stringify(equal.trace), '["product","product","seed"]');
});

test('AI 배치 행동: 지갑은 signed 1200 이상일 때만 두 번째 생산 조회/후보 행동을 진행한다', () => {
    // 1199/음수 DWORD/정확한 1200 을 구분한다.
    for (const wallet of [1199, 0xffffffff, 0x80000000, 1200]) {
        const input = new DropActionScene();
        input.field(0x2d8, wallet);
        input.run();
        assert.equal(JSON.stringify(input.trace), wallet === 1200 ? '["product","product","seed"]' : '["product"]');
    }
});

test('AI 배치 행동: 최초 시도 400 은 완료를 기다리고 재사용 시도는 스캔 미완료에서도 원래 Partial 알림 뒤 진행한다', () => {
    const first = new DropActionScene();
    first.field(0x400, 1);
    first.field(0x3ec, 0);
    first.pending = true;
    first.run();
    assert.ok(JSON.stringify(first.trace) === '["product","scan"]' && !first.field(0x3f0));
    const reuse = new DropActionScene();
    reuse.field(0x3ec, 0);
    reuse.pending = true;
    reuse.run();
    assert.ok(JSON.stringify(reuse.trace) === '["product","scan","product","partial","seed"]' && reuse.field(0x3f0) === 1);
});

test('AI 배치 행동: 현재 생산 조각의 WORD 100 만 거르고 free/dead 나 다른 pending 비트를 추가 필터하지 않는다', () => {
    const input = new DropActionScene();
    input.pool.allocated(5)[21] = 1;
    input.run();
    assert.equal(JSON.stringify(input.trace), '["product","product"]');
    const other = new DropActionScene();
    other.pool.allocated(5)[21] = 2;
    other.pool.allocated(5)[11] |= 2;
    other.run();
    assert.equal(JSON.stringify(other.trace), '["product","product","seed"]');
});

test('AI 배치 행동: 사제 탐색은 signed 진행 비교이고 유효한 목표 좌표만 배치를 막는다', () => {
    const searching = new DropActionScene();
    searching.field(0x5a0, 2);
    searching.field(0x5a4, 1);
    searching.run();
    assert.equal(JSON.stringify(searching.trace), '["product","product"]');
    const signedProgress = new DropActionScene();
    signedProgress.field(0x5a0, 0xffffffff);
    signedProgress.run();
    assert.equal(signedProgress.field(0x3f0), 1);
    // strict 유효 좌표/0/NaN 의 목표 관문을 서로 구분한다.
    for (const x of [10, 0, Number.NaN]) {
        const input = new DropActionScene();
        input.field(0xad0, floatToBits(x));
        input.field(0xad4, floatToBits(10));
        input.run();
        assert.equal(input.field(0x3f0), x === 10 ? 0 : 1);
    }
});

test('AI 배치 행동: 빈 후보 진단에서 복귀하면 현재 TYPE flags 를 다시 읽어 fake/seed 를 선택하며 직전 성공은 보존한다', () => {
    const input = new DropActionScene();
    input.checking = true;
    input.field(0x3e8, 9);
    input.types[136].flags2 = 0x800;
    input.duringAssertion = () => { input.field(0x3e0, 136); };
    input.run();
    assert.ok(JSON.stringify(input.trace) === '["product","product","assert","fake"]' && input.field(0x3f0) === 1 && input.field(0x3f4) === 77 && input.field(0x3fc) === 88 && input.random.state === 1);
});

test('AI 배치 행동: 비용 거부는 기존 성공/표적·집중/후보를 지우기 전이며 현재 TYPE 이름으로 알린다', () => {
    const input = new DropActionScene();
    input.field(0x570, 1);
    input.choice(0, 11, 12);
    input.afford = false;
    input.duringAfford = () => { input.field(0x3e0, 129); };
    input.run();
    assert.ok(JSON.stringify(input.trace) === '["product","product","afford","cannot"]' && input.notices[0].type === 129 && input.field(0x3f4) === 77 && input.field(0x3fc) === 88 && input.field(0x3f8) === 99 && input.field(0x570) === 1 && input.field(0x3f0) === 1 && input.random.state === 1);
});

test('AI 배치 행동: 공용 난수를 현재 count 로 소비하고 실패 후보도 기존 실제 압축기로 제거한다', () => {
    const input = new DropActionScene();
    input.field(0x570, 3);
    input.choice(0, 11, 12);
    input.choice(1, 21, 22);
    input.choice(2, 31, 32);
    input.legal = [false, false, true];
    const expected = new GameRandom(1);
    const remaining = [11, 21, 31];
    const chosen = [];
    // 실제 원본 난수로 매 압축 전 index 를 계산하며 기대 후보의 상대 순서를 유지한다.
    while (remaining.length > 0) {
        const index = expected.next(remaining.length);
        chosen.push(remaining[index]);
        remaining.splice(index, 1);
    }
    input.run();
    assert.ok(input.queries.length === 3 && input.deployments.length === 1 && input.random.state === expected.state && !input.field(0x570) && input.field(0x3f4) === 1 && input.field(0x3fc) === 6 && input.field(0x3f8) === 99 && input.field(0x3f0) === 1);
    // 압축 전 각각의 현재 선택 좌표를 비교한다.
    for (let index = 0; index < 3; index += 1) assert.equal(input.queries[index].x, chosen[index]);
    assert.ok(input.deployments[0].point.x === chosen.at(-1) && input.notices.at(-1).action === AI_DROP_ACTION_LOG.target && input.notices.at(-1).type === 150 && input.notices.at(-1).x === 71 && input.notices.at(-1).y === 82);
});

test('AI 배치 행동: 모든 후보 거부는 이전 성공/표적만 지우고 집중 표적을 건드리지 않는다', () => {
    const input = new DropActionScene();
    input.field(0x570, 2);
    input.choice(0, 11, 12);
    input.choice(1, 21, 22);
    input.legal = [false, false];
    input.run();
    assert.ok(input.queries.length === 2 && input.deployments.length === 0 && !input.field(0x570) && !input.field(0x3f4) && !input.field(0x3fc) && input.field(0x3f8) === 99 && input.field(0x3f0) === 1);
});

test('AI 배치 행동: 승인/로그/실제 배치 뒤의 현재 후보·TYPE/방향·표적과 하위 BYTE owner/focus 를 각각 읽는다', () => {
    const input = new DropActionScene();
    input.field(0x570, 1);
    input.field(0x260, 0x101);
    input.choice(0, F(11.8), F(12.8));
    input.duringPlace = () => { input.field(0x3e0, 129); input.field(0x3e4, 6); input.choice(0, F(21.9), F(22.9)); };
    input.duringLog = (action) => {
        if (action === AI_DROP_ACTION_LOG.laying) input.field(0x4f8, 7);
        if (action === AI_DROP_ACTION_LOG.target) { input.field(0x3e0, 130); input.field(0x3e4, 8); input.choice(0, F(31.2), F(32.2), 7); }
    };
    input.duringDeploy = () => { input.field(0x4f8, 6); input.field(0x248, 0x40); };
    input.run();
    const query = input.queries[0];
    const deployed = input.deployments[0];
    assert.ok(query.type === 128 && query.owner === 1 && !query.argument && !query.flags && query.mode === 1 && query.x === F(11.8) && query.y === F(12.8));
    assert.ok(input.notices[0].type === 129 && input.notices[0].x === 21 && input.notices[0].y === 22 && input.notices[1].type === 151 && input.notices[1].x === 93 && input.notices[1].y === 104);
    assert.ok(deployed.type === 130 && deployed.point.x === F(31.2) && deployed.point.y === F(32.2) && !deployed.argument && deployed.direction === 8 && input.field(0x3fc) === 6 && input.field(0x3f8) === 6);
});

test('AI 배치 행동: 집중은 BYTE 40h 에서만 선택하며 상위 BYTE 40h 는 이전 집중 표적을 유지한다', () => {
    // 실제 현재 능력값 두 가지로 원문 BYTE 검사를 구분한다.
    for (const capability of [0x40, 0x4000]) {
        const input = new DropActionScene();
        input.field(0x248, capability);
        input.field(0x570, 1);
        input.choice(0, 11, 12);
        input.run();
        assert.equal(input.field(0x3f8), capability === 0x40 ? 6 : 99);
    }
});

test('AI 배치 행동: 중단은 승인 결과의 local 이 아니라 실제 압축 효과 뒤 현재 3f4 로만 판단한다', () => {
    const invalid = new DropActionScene();
    invalid.field(0x570, 1);
    invalid.choice(0, 11, 12);
    invalid.legal = [false];
    invalid.duringRemove = () => { invalid.field(0x3f4, 1); };
    invalid.run();
    assert.ok(invalid.deployments.length === 0 && invalid.field(0x3f4) === 1 && invalid.field(0x3f0) === 1);
    const twice = new DropActionScene();
    twice.field(0x570, 2);
    twice.choice(0, 11, 12);
    twice.choice(1, 21, 22);
    twice.duringRemove = () => { if (twice.field(0x570)) twice.field(0x3f4, 0); };
    twice.run();
    assert.ok(twice.deployments.length === 2 && !twice.field(0x570) && twice.field(0x3f4) === 1);
});

test('AI 배치 행동: 비용 비교가 count 를 0 으로 바꿔도 이미 들어온 후보 분기는 이전 성공/표적을 지운다', () => {
    const input = new DropActionScene();
    input.field(0x570, 1);
    input.choice(0, 11, 12);
    input.duringAfford = () => { input.field(0x570, 0); };
    input.run();
    assert.ok(JSON.stringify(input.trace) === '["product","product","afford"]' && !input.field(0x3f4) && !input.field(0x3fc) && input.field(0x3f0) === 1 && input.random.state === 1);
});

test('AI 배치 행동: 실제 배치/압축 경계 누락은 이미 수행한 난수/로그/성공 쓰기를 취소하지 않는다', () => {
    const input = new DropActionScene();
    input.field(0x570, 1);
    input.choice(0, 11, 12);
    input.hooks.deploy = undefined;
    assert.ok(throws(() => input.run()) && input.field(0x570) === 1 && !input.field(0x3f4) && !input.field(0x3fc) && !input.field(0x3f0) && input.random.state !== 1 && input.notices.length === 2);
    const removed = new DropActionScene();
    removed.field(0x570, 1);
    removed.choice(0, 11, 12);
    removed.hooks.remove = undefined;
    assert.ok(throws(() => removed.run()) && removed.field(0x3f4) === 1 && removed.field(0x3fc) === 6 && removed.field(0x570) === 1 && !removed.field(0x3f0));
});

test('AI 배치 행동: 로그가 꺼져도 승인 뒤 현재 표적/TYPE 조회는 생략하지 않는다', () => {
    const input = new DropActionScene();
    input.field(0x570, 1);
    input.choice(0, 11, 12, 65535);
    input.field(0xb00, 0);
    input.hooks.log = undefined;
    assert.ok(throws(() => input.run()) && input.deployments.length === 0 && input.field(0x570) === 1 && !input.field(0x3f4));
});

test('AI 배치 행동 생성자: 본체 소유자/PATCH 풀/188 TYPE/난수/현재 전역 조회기가 없으면 거부하고 가짜 본체를 거부한다', () => {
    const scene = new DropActionScene();
    const globals = { now: () => 0, mapLimit: () => 256, checking: () => 0 };
    assert.ok(throws(() => new RawAiDropActions(undefined, scene.pool, scene.types, scene.random, globals, scene.hooks)));
    assert.ok(throws(() => new RawAiDropActions(scene.memory, {}, scene.types, scene.random, globals, scene.hooks)));
    assert.ok(throws(() => new RawAiDropActions(scene.memory, scene.pool, scene.types.slice(0, 10), scene.random, globals, scene.hooks)));
    assert.ok(throws(() => new RawAiDropActions(scene.memory, scene.pool, scene.types, undefined, globals, scene.hooks)));
    for (const name of ['now', 'mapLimit', 'checking']) assert.ok(throws(() => new RawAiDropActions(scene.memory, scene.pool, scene.types, scene.random, { ...globals, [name]: undefined }, scene.hooks)));
    assert.equal(scene.actions().pool, scene.pool);
    assert.ok(throws(() => scene.actions().run(scene.memory.allocate(16))));
});
