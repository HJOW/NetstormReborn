// AI 부족 좌표 점수/시설별 분배/후보 승인 raw-ai-drop-evaluation.js(RawAiDropEvaluation)를 cpppj 계약 시험(손으로 쓴 입력과 기대값)으로 검사한다.
//  - 이 몸체에는 독립 원본 관측 fixture 가 없다(cpppj 시험이 원본 분기 순서를 직접 단언한다). 같은 입력과 단언을 옮겼다: 빈/음수 count 는 포인터를 읽지 않고 출력 index 보존, 정확한 0 거리 제외와 NaN/거리 1000 건너뜀과 같은 최고는 첫 index,
//    축소 전 값 비교 뒤 float 저장, 반경 22 의 거리 44 경계 제외와 float 저장 점수의 절삭/길이+1, 공급 C 문자열의 첫 NUL 과 현재 signed 길이 한도, flags800 이 블로커/group3 보다 우선과 출력 보존, group3 지상/0·1 Gun/6 무진단,
//    미지원 group 진단(현재 b00)과 TYPE 경계, 지상 결과를 점수 하위 실패보다 먼저 기록, 전체 배치 인자(owner BYTE/mode 1)와 거부 시 이후 없음, 평가/에너지는 하위 효과 뒤 현재 full owner 재조회, 양수 동률 승인과 음수 개선,
//    채택 안 해도 대상 평가 효과 보존, 에너지 거부 뒤 실제 부족 좌표 추가와 중복 로그 반복 없음, 선택된 하위 누락의 prefix 보존, 저장 점수(+0x3e8)의 현재 읽기/쓰기 위치.
//  - E170 `RawAiDrySpots`(실제 Add)와 E172 `RawAiDropTargets`(실제 표적 평가)를 같은 저장소에서 사용한다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E173: AI 배치 평가 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import { put, get, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { AI_BRAIN_BYTES, RawAiBrainMemory } from '../www/core/raw-ai-brain.js';
import { RawAiDrySpots } from '../www/core/raw-ai-dry-spots.js';
import { RawAiDropTargets } from '../www/core/raw-ai-drop-targets.js';
import { RawAiDropEvaluation } from '../www/core/raw-ai-drop-evaluation.js';

// 예외가 나는지 본다. (2026-11-09 WB04-E173)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

const F = Math.fround;
const P = { x: F(20.25), y: F(30.75) };

// 같은 본체/실제 부족 좌표 backing/실제 표적 평가를 연결한 명시 입력 장면이다. (2026-11-09 WB04-E173)
class EvaluationScene {
    // 기본은 같은 적 소유자 2 의 10점 표적, TYPE 90 Gun 과 near dry spot 한 개다.
    constructor() {
        this.memory = new RawAiBrainMemory(0x51000000, (raw) => raw.fill(0));
        this.token = this.memory.allocate(AI_BRAIN_BYTES);
        this.records = this.memory.allocate(120);
        this.pool = new SidPool(32768, { server: false });
        this.types = Array.from({ length: 188 }, () => ({ flags2: 0, group: 0 }));
        this.altar = 164;
        this.directional = 142;
        this.probe = 71;
        this.radius = 22;
        this.rate = 0;
        this.found = 5;
        this.placement = true;
        this.energy = 1;
        this.supply = 's';
        this.trace = [];
        this.duringPlace = null;
        this.duringRate = null;
        this.duringCollect = null;
        this.duringEnergy = null;
        this.lastPlacement = null;
        this.lastCollectOwner = 0;
        this.lastCollectType = 0;
        this.lastEnergyOwner = 0;
        assert.equal(this.pool.allocate(2), 5);
        const raw = this.pool.allocated(5);
        raw.fill(0);
        raw[10] = 80;
        raw[34] = 2;
        put(raw, 26, 1000);
        this.field(0x260, 1);
        this.field(0x264, 2);
        this.field(0xad8, 2);
        this.field(0xadc, 10);
        this.field(0xae4, this.records);
        this.field(0xaec, 1);
        this.types[90].group = 0;
        this.types[91].group = 3;
        this.types[92].flags2 = 0x4000000;
        this.types[93].flags2 = 0x800;
        this.types[93].group = 3;
        this.types[94].group = 6;
        this.types[95].group = 7;
        this.targets = new RawAiDropTargets(this.memory, this.pool, this.types, { altarType: () => this.altar, directionalType: () => this.directional }, {
            rate: (actor) => { assert.equal(actor, 5); this.trace.push('rate'); if (this.duringRate) this.duringRate(); return this.rate; },
            gun: (x, y, type, direction, owner) => { assert.equal(direction, 2); this.trace.push(`gun:${owner}`); return this.found; },
            ground: (x, y, type, owner) => { this.trace.push(`ground:${owner}`); return this.found; },
            blockers: (owner, point, capacity) => { assert.equal(capacity, 30); this.trace.push('blockers'); return [5]; }
        });
        this.hooks = {
            collect: (box, x, y, type, owner, theme) => {
                assert.ok(box.text === '' && theme === -1);
                this.trace.push('collect');
                this.lastCollectOwner = owner;
                this.lastCollectType = type;
                if (this.duringCollect) this.duringCollect();
                box.text = this.supply;
            },
            mayPlace: (query) => { this.trace.push('place'); this.lastPlacement = query; if (this.duringPlace) this.duringPlace(); return this.placement; },
            energy: (box, x, y, type, owner) => { assert.equal(box.text, ''); this.trace.push('energy'); this.lastEnergyOwner = owner; if (this.duringEnergy) this.duringEnergy(); return this.energy; },
            dry: (brain, x, y, type) => { assert.equal(brain, this.token); this.trace.push('dry'); return new RawAiDrySpots(this.memory, this.pool, {}).add(brain, x, y, type); },
            unsupported: (brain, type) => { assert.equal(brain, this.token); this.trace.push(`unknown:${type}`); },
            dryLog: (brain, x, y) => { assert.equal(brain, this.token); this.trace.push(`drylog:${x}:${y}`); }
        };
        this.dry([{ x: F(21.25), y: F(30.75) }]);
    }

    // 현재 본체 DWORD 를 읽거나(값 없음) 지정 DWORD 만 입력한다. (2026-11-09 WB04-E173)
    field(at, value) {
        if (value === undefined) return get(this.memory.bytes(this.token), at);
        put(this.memory.bytes(this.token), at, value >>> 0);
        return undefined;
    }

    // 기록 TYPE 와 무관한 명시 XY 입력을 현재 backing 에 쓴다. (2026-11-09 WB04-E173)
    dry(values) {
        this.field(0xae0, values.length);
        // 원본 기록 순서를 유지한다.
        values.forEach((point, index) => {
            put(this.memory.bytes(this.records), index * 12, floatToBits(point.x));
            put(this.memory.bytes(this.records), index * 12 + 4, floatToBits(point.y));
            put(this.memory.bytes(this.records), index * 12 + 8, 0xdeadbeef);
        });
    }

    // 같은 현재 실제 표적 계산기를 평가/승인 하위와 연결한다. (2026-11-09 WB04-E173)
    actions() { return new RawAiDropEvaluation(this.memory, this.pool, this.types, this.targets, { manaRadius: () => this.radius, probeType: () => this.probe }, this.hooks); }
}

test('AI 배치 평가: 빈/음수 count 는 포인터를 읽지 않고 출력 index 를 보존한다', () => {
    const scene = new EvaluationScene();
    scene.field(0xae0, 0);
    scene.field(0xae4, 0);
    const box = { value: 99 };
    assert.ok(scene.actions().dryDistance(scene.token, P, box) === 0 && box.value === 99);
    scene.field(0xae0, 0xffffffff);
    assert.ok(scene.actions().dryDistance(scene.token, P, box) === 0 && box.value === 99);
});

test('AI 배치 평가: 정확한 0 거리는 제외하고 NaN/거리 1000 은 건너뛰며 같은 최고는 첫 index 다', () => {
    const scene = new EvaluationScene();
    scene.dry([{ x: F(20.25), y: F(30.75) }, { x: F(21.25), y: F(30.75) }, { x: F(19.25), y: F(30.75) }, { x: F(1020.25), y: F(30.75) }, { x: Number.NaN, y: F(30.75) }]);
    const box = { value: 99 };
    assert.ok(scene.actions().dryDistance(scene.token, P, box) === 999 && box.value === 1 && scene.trace.length === 0);
});

test('AI 배치 평가: 최고값 float 저장 때문에 더 가까운 둘째 기록도 축소 전 값이 저장 최고보다 작으면 유지한다', () => {
    const scene = new EvaluationScene();
    scene.dry([{ x: F(0.000002), y: 0 }, { x: F(0.000001), y: 0 }]);
    const box = { value: 99 };
    assert.ok(scene.actions().dryDistance(scene.token, { x: 0, y: 0 }, box) === 1000 && box.value === 0);
});

test('AI 배치 평가: 반경 22 의 거리 44 경계는 제외하며 점수는 float 저장 후 절삭/길이+1 로 나눈다', () => {
    const scene = new EvaluationScene();
    scene.dry([{ x: F(64.25), y: F(30.75) }]);
    assert.ok(scene.actions().supplyScore(scene.token, P) === 0 && scene.trace.length === 0);
    scene.dry([{ x: F(63.75), y: F(30.75) }]);
    assert.ok(scene.actions().supplyScore(scene.token, P) === 478 && JSON.stringify(scene.trace) === '["collect"]');
    scene.trace.length = 0;
    scene.radius = Number.NaN;
    assert.ok(scene.actions().supplyScore(scene.token, P) === 0 && scene.trace.length === 0);
});

test('AI 배치 평가: 공급 문자열은 첫 NUL 까지 세며 빈 문자열과 signed 길이 한도를 원본대로 처리한다', () => {
    const scene = new EvaluationScene();
    scene.supply = 's\0sss';
    scene.probe = 75;
    scene.field(0x260, 0x101);
    scene.field(0xad8, 0);
    scene.duringCollect = () => { scene.field(0xad8, 2); scene.field(0x260, 2); scene.probe = 76; };
    assert.ok(scene.actions().supplyScore(scene.token, P) === 499 && scene.lastCollectOwner === 0x101 && scene.lastCollectType === 75);
    scene.duringCollect = null;
    scene.field(0xad8, 1);
    assert.ok(!scene.actions().supplyScore(scene.token, P));
    scene.field(0xad8, 0xffffffff);
    assert.ok(!scene.actions().supplyScore(scene.token, P));
    scene.supply = '';
    scene.field(0xad8, 10);
    assert.ok(!scene.actions().supplyScore(scene.token, P));
});

test('AI 배치 평가: flags800 은 블로커/group3 보다 우선하며 공급 평가는 표적 출력을 바꾸지 않는다', () => {
    const scene = new EvaluationScene();
    scene.types[93].flags2 |= 0x4000000;
    const target = { value: 9 };
    assert.ok(scene.actions().evaluate(scene.token, 93, P, 2, target) === 499 && target.value === 9 && JSON.stringify(scene.trace) === '["collect"]');
    scene.types[93].flags2 = 0x4000000;
    scene.trace.length = 0;
    assert.ok(scene.actions().evaluate(scene.token, 93, P, 2, target) === 10 && target.value === 5 && JSON.stringify(scene.trace) === '["blockers","rate"]');
});

test('AI 배치 평가: group3 은 별도 지상 검색이고 0/1 은 실제 Gun 경로이며 6 은 진단 없이 0 이다', () => {
    const scene = new EvaluationScene();
    const target = { value: 9 };
    assert.ok(scene.actions().evaluate(scene.token, 91, P, 2, target) === 10 && JSON.stringify(scene.trace) === '["ground:1","rate"]');
    scene.trace.length = 0;
    assert.ok(scene.actions().evaluate(scene.token, 90, P, 2, target) === 10 && JSON.stringify(scene.trace) === '["gun:1","rate"]');
    scene.types[90].group = 1;
    scene.trace.length = 0;
    assert.ok(scene.actions().evaluate(scene.token, 90, P, 2, target) === 10 && JSON.stringify(scene.trace) === '["gun:1","rate"]');
    scene.trace.length = 0;
    scene.field(0xb00, 1);
    target.value = 9;
    assert.ok(!scene.actions().evaluate(scene.token, 94, P, 2, target) && target.value === 9 && scene.trace.length === 0);
});

test('AI 배치 평가: 미지원 group 은 출력 보존/현재 b00 진단만 수행하며 잘못된 TYPE 는 명시 거부한다', () => {
    const scene = new EvaluationScene();
    const target = { value: 9 };
    assert.ok(!scene.actions().evaluate(scene.token, 95, P, 2, target) && target.value === 9 && scene.trace.length === 0);
    scene.field(0xb00, 1);
    assert.ok(!scene.actions().evaluate(scene.token, 95, P, 2, target) && JSON.stringify(scene.trace) === '["unknown:95"]');
    assert.ok(throws(() => scene.actions().evaluate(scene.token, 188, P, 2, target)));
});

test('AI 배치 평가: 별도 검색은 0 도 출력하고 점수 하위 실패보다 먼저 실제 SID 를 쓴다', () => {
    const scene = new EvaluationScene();
    const target = { value: 9 };
    scene.found = 0;
    assert.ok(!scene.actions().evaluate(scene.token, 91, P, 2, target) && !target.value);
    scene.found = 5;
    scene.duringRate = () => { assert.equal(target.value, 5); throw new Error('명시 점수 getter 실패'); };
    assert.ok(throws(() => scene.actions().evaluate(scene.token, 91, P, 2, target)) && target.value === 5);
});

test('AI 배치 평가: 전체 배치 검사는 방향 0/소유자 BYTE/mode 1 이며 거부 시 이후 평가·출력도 없다', () => {
    const scene = new EvaluationScene();
    scene.field(0x260, 0x101);
    scene.placement = false;
    const best = { x: 7, y: 8 };
    const score = { value: 3 };
    const target = { value: 9 };
    assert.ok(!scene.actions().consider(scene.token, 90, P, 2, best, score, target));
    const query = scene.lastPlacement;
    assert.ok(query.type === 90 && !query.argument && !query.flags && query.owner === 1 && query.mode === 1 && query.x === P.x && query.y === P.y);
    assert.ok(best.x === 7 && best.y === 8 && score.value === 3 && target.value === 9 && JSON.stringify(scene.trace) === '["place"]');
});

test('AI 배치 평가: 평가/에너지는 앞선 하위 효과 뒤 현재 full owner 를 다시 읽는다', () => {
    const scene = new EvaluationScene();
    scene.field(0x260, 0x101);
    scene.duringPlace = () => { scene.field(0x260, 0x102); };
    scene.duringRate = () => { scene.field(0x260, 0x103); };
    const best = { x: 7, y: 8 };
    const score = { value: 0 };
    const target = { value: 9 };
    scene.energy = 0x80000000;
    assert.ok(scene.actions().consider(scene.token, 90, P, 2, best, score, target));
    assert.ok(scene.lastPlacement.owner === 1 && scene.lastEnergyOwner === 0x103 && best.x === P.x && best.y === P.y && score.value === 10 && target.value === 5);
    assert.deepEqual(scene.trace, ['place', 'gun:258', 'rate', 'energy']);
});

test('AI 배치 평가: 양수 동률은 승인하고 0/음수 동률은 승인하지 않는다(음수도 개선이면 승인할 수 있다)', () => {
    const scene = new EvaluationScene();
    let best = { x: 7, y: 8 };
    const score = { value: 10 };
    const target = { value: 9 };
    assert.ok(scene.actions().consider(scene.token, 90, P, 2, best, score, target) && score.value === 10 && best.x === P.x);
    scene.trace.length = 0;
    score.value = 0;
    best = { x: 7, y: 8 };
    assert.ok(!scene.actions().consider(scene.token, 94, P, 2, best, score, target) && JSON.stringify(scene.trace) === '["place"]');
    scene.rate = -11;
    scene.trace.length = 0;
    score.value = -1;
    assert.ok(!scene.actions().consider(scene.token, 90, P, 2, best, score, target) && JSON.stringify(scene.trace) === '["place","gun:1","rate"]');
    score.value = -2;
    assert.ok(scene.actions().consider(scene.token, 90, P, 2, best, score, target) && score.value === -1);
});

test('AI 배치 평가: 후보를 채택하지 않아도 실제 표적 SID 의 평가 효과를 보존하고 최고 좌표/점수는 유지한다', () => {
    const scene = new EvaluationScene();
    const best = { x: 7, y: 8 };
    const score = { value: 20 };
    const target = { value: 9 };
    assert.ok(!scene.actions().consider(scene.token, 90, P, 2, best, score, target));
    assert.ok(target.value === 5 && best.x === 7 && best.y === 8 && score.value === 20 && JSON.stringify(scene.trace) === '["place","gun:1","rate"]');
});

test('AI 배치 평가: 실제 부족 좌표 추가는 에너지 거부 뒤이며 중복 실패는 로그도 반복하지 않는다', () => {
    const scene = new EvaluationScene();
    scene.field(0xae0, 0);
    scene.energy = 0;
    scene.duringEnergy = () => { scene.field(0xb00, 1); };
    const best = { x: 7, y: 8 };
    const score = { value: 0 };
    const target = { value: 9 };
    assert.ok(!scene.actions().consider(scene.token, 90, P, 2, best, score, target));
    assert.ok(scene.field(0xae0) === 1 && get(scene.memory.bytes(scene.records), 0) === floatToBits(20.25) && get(scene.memory.bytes(scene.records), 4) === floatToBits(30.75) && get(scene.memory.bytes(scene.records), 8) === 90);
    assert.ok(best.x === 7 && score.value === 0 && target.value === 5 && JSON.stringify(scene.trace) === '["place","gun:1","rate","energy","dry","drylog:20:30"]');
    scene.trace.length = 0;
    assert.ok(!scene.actions().consider(scene.token, 90, P, 2, best, score, target) && scene.field(0xae0) === 1 && JSON.stringify(scene.trace) === '["place","gun:1","rate","energy","dry"]');
});

test('AI 배치 평가: 선택된 하위 미연결은 평가가 이미 쓴 표적을 보존하고 최고 좌표/점수는 바꾸지 않는다', () => {
    const scene = new EvaluationScene();
    scene.hooks.energy = undefined;
    const best = { x: 7, y: 8 };
    const score = { value: 0 };
    const target = { value: 9 };
    assert.ok(throws(() => scene.actions().consider(scene.token, 90, P, 2, best, score, target)) && target.value === 5 && best.x === 7 && score.value === 0);
    scene.energy = 0;
    scene.hooks.energy = () => 0;
    scene.hooks.dry = undefined;
    assert.ok(throws(() => scene.actions().consider(scene.token, 90, P, 2, best, score, target)) && !scene.field(0xb00));
});

test('AI 배치 평가: 스캔의 실제 +0x3e8 은 평가 뒤 읽고 승인 뒤 그 자리에서 쓰며 임시 사본을 되돌리지 않는다', () => {
    const scene = new EvaluationScene();
    scene.field(0x3e8, 0);
    scene.duringRate = () => { scene.field(0x3e8, 100); };
    const best = { x: 7, y: 8 };
    const target = { value: 9 };
    assert.ok(!scene.actions().considerStored(scene.token, 90, P, 2, best, target) && scene.field(0x3e8) === 100 && target.value === 5 && best.x === 7);
    scene.duringRate = () => { scene.field(0x3e8, 5); };
    scene.duringEnergy = () => { assert.equal(scene.field(0x3e8), 5); };
    assert.ok(scene.actions().considerStored(scene.token, 90, P, 2, best, target) && scene.field(0x3e8) === 10 && best.x === P.x && best.y === P.y);
});

test('AI 배치 평가 생성자: 본체 소유자/같은 PATCH 풀·표적 평가기/TYPE 표/현재 전역 조회기가 없으면 거부하고 가짜 본체를 거부한다', () => {
    const scene = new EvaluationScene();
    const globals = { manaRadius: () => 22, probeType: () => 71 };
    assert.ok(throws(() => new RawAiDropEvaluation(undefined, scene.pool, scene.types, scene.targets, globals, scene.hooks)));
    assert.ok(throws(() => new RawAiDropEvaluation(scene.memory, scene.pool, scene.types, undefined, globals, scene.hooks)));
    assert.ok(throws(() => new RawAiDropEvaluation(scene.memory, new SidPool(32768, { server: false }), scene.types, scene.targets, globals, scene.hooks)));
    assert.ok(throws(() => new RawAiDropEvaluation(scene.memory, scene.pool, undefined, scene.targets, globals, scene.hooks)));
    assert.ok(throws(() => new RawAiDropEvaluation(scene.memory, scene.pool, scene.types, scene.targets, { manaRadius: () => 22 }, scene.hooks)));
    const small = scene.memory.allocate(16);
    assert.ok(throws(() => scene.actions().dryDistance(small, P, { value: 0 })) && scene.actions().pool === scene.pool);
});
