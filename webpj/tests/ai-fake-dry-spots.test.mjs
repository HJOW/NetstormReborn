// AI 가짜 부족 좌표 raw-ai-fake-dry-spots.js(RawAiFakeDrySpots)를 cpppj 계약 시험(손으로 쓴 입력과 기대값)으로 검사한다.
//  - 이 몸체에는 독립 원본 관측 fixture 가 없다(cpppj 시험이 원본 분기 순서를 직접 단언한다). 같은 입력과 단언을 옮겼다: fake TYPE 0 이면 다른 관문/목록/효과를 요청하지 않음, full 제거가 신전 부재 반환보다 먼저(cursor/가짜 좌표 불변),
//    신전은 중심 대신 raw 좌표이며 중복 실패에서도 Added 로그, 공급 목록 포인터/count 는 진입 cache 이고 첫 최고점수 규칙, 중심 효과 뒤 현재 Dry Spot 포인터와 점수에서 고른 index 보존, 공급 반경 0.7 배 strict 와 NaN 폐기와 중심 X/Y 원래 쌍,
//    유효성 경계/NaN 후보의 무로그, signed 음수 공급 count 는 탐색 없이 Seed 로 보충, Add 가 false 여도 선택 좌표/전략 4 갱신과 디버그 표식/로그 순서, 표식이 현재 가짜 좌표를 무효화하면 Seed 가 뒤따르지만 로그는 원래 후보,
//    후보 계산 뒤 현재 TYPE/전략/b00 재조회, 선택된 경로/표식 누락의 앞선 쓰기 보존.
//  - 같은 실제 본체/12바이트 목록과 E170 `RawAiDrySpots` 의 Add/Remove/Seed 를 같은 저장소에서 사용한다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E172: AI 가짜 부족 좌표 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import { put, get, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { GameRandom } from '../www/core/random.js';
import { AI_BRAIN_BYTES, RawAiBrainMemory } from '../www/core/raw-ai-brain.js';
import { RawAiDrySpots } from '../www/core/raw-ai-dry-spots.js';
import { AI_FAKE_DRY_LOG, RawAiFakeDrySpots } from '../www/core/raw-ai-fake-dry-spots.js';

// 예외가 나는지 본다. (2026-11-09 WB04-E172)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 비트 비교용 단정도 변환이다. (2026-11-09 WB04-E172)
const bits = (value) => floatToBits(Math.fround(value));

// 실제 본체/12바이트 목록과 기존 실제 Add/Remove/Seed 를 같은 저장소에서 사용한다. (2026-11-09 WB04-E172)
class FakeDryScene {
    // 기본은 유효한 현재 가짜 좌표와 한 부족 기록이며 공급 시설/신전은 caller 가 선택한다.
    constructor() {
        this.memory = new RawAiBrainMemory(0x54000000, (raw) => raw.fill(0));
        this.token = this.memory.allocate(AI_BRAIN_BYTES);
        this.records = this.memory.allocate(120);
        this.suppliers = this.memory.allocate(32);
        this.pool = new SidPool(32768, { server: false });
        this.random = new GameRandom(8);
        this.radius = 22;
        this.limit = 256;
        this.debug = 99;
        this.center = { x: 40, y: 60 };
        this.candidate = { x: 100, y: 100 };
        this.scores = [100];
        this.indices = [0];
        this.trace = [];
        this.scorePoints = [];
        this.sources = [];
        this.targets = [];
        this.centers = [];
        this.logs = [];
        this.logPoints = [];
        this.duringScore = null;
        this.duringCenter = null;
        this.duringCandidate = null;
        this.duringAdd = null;
        this.duringMark = null;
        this.duringLog = null;
        this.field(0x280, 73);
        this.field(0xadc, 10);
        this.field(0xae0, 0);
        this.field(0xae4, this.records);
        this.field(0xaec, 2);
        this.field(0x2f4, this.suppliers);
        this.field(0x2f8, 8);
        this.field(0x2fc, 0);
        this.field(0xaf0, floatToBits(90));
        this.field(0xaf4, floatToBits(90));
        this.field(0x260, 2);
        this.field(0xb00, 1);
        assert.ok(this.dry().add(this.token, 150, 160, 71));
        this.hooks = {
            score: (brain, point, box) => {
                assert.equal(brain, this.token);
                const call = this.scorePoints.length;
                this.scorePoints.push(point);
                this.trace.push('score');
                box.value = this.indices.at(Math.min(call, this.indices.length - 1));
                const value = this.scores.at(Math.min(call, this.scores.length - 1));
                if (this.duringScore) this.duringScore(call);
                return value;
            },
            center: (sid) => { this.centers.push(sid); this.trace.push('center'); if (this.duringCenter) this.duringCenter(); return this.center; },
            candidate: (source, target) => { this.sources.push(source); this.targets.push(target); this.trace.push('candidate'); if (this.duringCandidate) this.duringCandidate(); return this.candidate; },
            add: (brain, point, type) => { assert.equal(brain, this.token); this.trace.push('add'); const result = this.dry().add(brain, point.x, point.y, type); if (this.duringAdd) this.duringAdd(); return result; },
            remove: (brain, index) => { assert.ok(brain === this.token && index === 0); this.trace.push('remove'); this.dry().remove(brain, index); },
            seed: (brain) => { assert.equal(brain, this.token); this.trace.push('seed'); this.dry().seed(brain, this.random); },
            mark: (point) => { assert.ok(point.x === this.candidate.x && point.y === this.candidate.y); this.trace.push('mark'); if (this.duringMark) this.duringMark(); },
            log: (brain, action, x, y) => { assert.equal(brain, this.token); this.trace.push(action === AI_FAKE_DRY_LOG.added ? 'added' : 'discarded'); this.logs.push(action); this.logPoints.push([x, y]); if (this.duringLog) this.duringLog(); }
        };
    }

    // 현재 본체 DWORD 를 읽거나(값 없음) 지정 DWORD 를 쓴다. (2026-11-09 WB04-E172)
    field(at, value) {
        if (value === undefined) return get(this.memory.bytes(this.token), at);
        put(this.memory.bytes(this.token), at, value >>> 0);
        return undefined;
    }

    // 기존 실제 Dry Spot 모듈을 같은 저장소에 연결한다. (2026-11-09 WB04-E172)
    dry() { return new RawAiDrySpots(this.memory, this.pool, {}); }

    // 같은 실제 풀/본체와 명시 하위로 전체 루틴을 조립한다. (2026-11-09 WB04-E172)
    make() { return new RawAiFakeDrySpots(this.memory, this.pool, { manaRadius: () => this.radius, mapLimit: () => this.limit, debug: () => this.debug }, this.hooks); }

    // 원래 공급 시설 목록에 사용할 실제 raw 객체를 만든다. (2026-11-09 WB04-E172)
    object(x, y) {
        const sid = this.pool.allocate(2);
        const raw = this.pool.allocated(sid);
        raw.fill(0);
        raw[10] = 136;
        put(raw, 14, floatToBits(x));
        put(raw, 18, floatToBits(y));
        return sid;
    }

    // 현재 공급 시설의 SID DWORD 목록을 설정한다. (2026-11-09 WB04-E172)
    supply(values) {
        this.field(0x2fc, values.length);
        // 지정한 SID 순서만 기록한다.
        values.forEach((sid, index) => put(this.memory.bytes(this.suppliers), index * 4, sid));
    }
}

test('AI 가짜 부족좌표: fake TYPE 0 이면 다른 관문/목록/실제 효과를 전혀 요청하지 않는다', () => {
    const input = new FakeDryScene();
    input.field(0x280, 0);
    input.field(0xadc, 0);
    input.field(0xae4, 0);
    input.hooks = {};
    input.make().extend(input.token);
    assert.ok(input.field(0xae0) === 1 && input.trace.length === 0 && input.random.state === 8);
});

test('AI 가짜 부족좌표: full 제거는 신전이 없는 반환보다 먼저이며 cursor/가짜 좌표를 보정하지 않는다', () => {
    const input = new FakeDryScene();
    input.field(0xadc, 1);
    input.field(0xae8, 7);
    input.make().extend(input.token);
    assert.ok(JSON.stringify(input.trace) === '["remove"]' && !input.field(0xae0) && input.field(0xae8) === 7 && input.field(0xaf0) === bits(90));
});

test('AI 가짜 부족좌표: 공급 시설 0 은 중심 대신 신전 raw 좌표를 사용하며 중복 실패에서도 원래 Added 로그를 남긴다', () => {
    const input = new FakeDryScene();
    const temple = input.object(150, 160);
    input.field(0x294, temple);
    input.make().extend(input.token);
    assert.ok(JSON.stringify(input.trace) === '["add","added"]' && input.field(0xae0) === 1 && input.centers.length === 0 && input.sources.length === 0 && input.field(0xaf0) === bits(90));
    assert.ok(JSON.stringify(input.logs) === JSON.stringify([AI_FAKE_DRY_LOG.added]) && JSON.stringify(input.logPoints[0]) === '[150,160]');
});

test('AI 가짜 부족좌표: 공급 목록 포인터/count 는 진입 cache 이며 각 객체의 raw 좌표와 첫 최고점수 규칙은 유지한다', () => {
    const input = new FakeDryScene();
    const first = input.object(20, 30);
    const second = input.object(50, 60);
    input.supply([first, second]);
    input.scores = [10, 10];
    const other = input.memory.allocate(4);
    put(input.memory.bytes(other), 0, 65535);
    input.duringScore = (call) => {
        if (!call) {
            input.field(0x2f4, other);
            input.field(0x2fc, 0);
            put(input.pool.allocated(second), 14, floatToBits(55));
        }
    };
    input.make().extend(input.token);
    assert.ok(input.scorePoints.length === 2 && input.scorePoints[1].x === 55 && JSON.stringify(input.centers) === JSON.stringify([first]) && input.sources[0].x === 60 && input.sources[0].y === 60 && input.targets[0].x === 149 && input.targets[0].y === 159);
});

test('AI 가짜 부족좌표: 중심 효과 뒤 현재 Dry Spot pointer 를 사용하되 점수에서 선택한 index 는 보존한다', () => {
    const input = new FakeDryScene();
    assert.ok(input.dry().add(input.token, 180, 190, 71));
    input.supply([input.object(179, 190)]);
    input.scores = [999];
    input.indices = [1];
    const replacement = input.memory.allocate(24);
    put(input.memory.bytes(replacement), 12, floatToBits(181));
    put(input.memory.bytes(replacement), 16, floatToBits(191));
    input.duringCenter = () => { input.field(0xae4, replacement); input.field(0xae0, 1); input.field(0xadc, 2); };
    input.make().extend(input.token);
    assert.ok(input.targets[0].x === 180 && input.targets[0].y === 190 && input.field(0xae0) === 2);
});

test('AI 가짜 부족좌표: 현재 공급 반경의 0.7 배보다 커야 채택하며 NaN 반경도 폐기한다(거리 중심은 X/Y 원래 쌍)', () => {
    // 원본 float 0.7 을 넓힌 임계값과 거리의 양쪽을 서로 다른 실제 반경으로 지정한다.
    for (const radius of [20, 30, Number.NaN]) {
        const input = new FakeDryScene();
        input.supply([input.object(20, 30)]);
        input.center = { x: 40, y: 60 };
        input.candidate = { x: 60, y: 60 };
        input.radius = radius;
        input.make().extend(input.token);
        const accepted = radius === 20;
        assert.ok((input.field(0xae0) === 2) === accepted && input.logs.at(-1) === (accepted ? AI_FAKE_DRY_LOG.added : AI_FAKE_DRY_LOG.discarded));
    }
});

test('AI 가짜 부족좌표: 유효성 경계/NaN 후보에서는 거리/등록/폐기 로그 없이 현재 가짜 좌표만 확인한다', () => {
    // 축 0/상한/NaN 은 원본 strict 좌표 검사의 서로 다른 거부 입력이다.
    for (const coordinate of [0, 256, Number.NaN]) {
        const input = new FakeDryScene();
        input.supply([input.object(20, 30)]);
        input.candidate = { x: coordinate, y: 100 };
        input.make().extend(input.token);
        assert.ok(input.field(0xae0) === 1 && input.logs.length === 0 && JSON.stringify(input.trace) === '["score","center","candidate"]');
    }
});

test('AI 가짜 부족좌표: signed 음수 공급 count 는 탐색을 건너뛰고 유효하지 않은 현재 가짜 좌표만 실제 Seed 로 보충한다', () => {
    const input = new FakeDryScene();
    input.field(0x2fc, 0xffffffff);
    input.field(0x2f4, 0);
    input.field(0xaf0, 0);
    input.field(0x248, 0);
    input.make().extend(input.token);
    assert.ok(JSON.stringify(input.trace) === '["seed"]' && input.random.state === 8 && input.logs.length === 0);
});

test('AI 가짜 부족좌표: 실제 Add 가 false 여도 선택 좌표/전략 4 를 갱신하고 현재 디버그 표식/로그를 원래 순서로 실행한다', () => {
    const input = new FakeDryScene();
    input.supply([input.object(20, 30)]);
    assert.ok(input.dry().add(input.token, 100, 100, 99));
    input.field(0x2d4, 4);
    input.field(0x3cc, 7);
    input.field(0x3d0, 9);
    input.field(0x3d4, 11);
    input.duringAdd = () => { input.field(0x280, 77); input.debug = 29; };
    input.make().extend(input.token);
    assert.ok(input.field(0xae0) === 2 && input.field(0xaf0) === bits(100) && input.field(0xaf4) === bits(100) && !input.field(0x3cc) && input.field(0x3d0) === 0xc61c4000 && input.field(0x3d4) === 0xc61c4000 && input.field(0x3d8) === 1);
    assert.ok(JSON.stringify(input.trace) === '["score","center","candidate","add","mark","added"]' && get(input.memory.bytes(input.records), 20) === 99);
});

test('AI 가짜 부족좌표: 채택 뒤 표식이 현재 가짜 좌표를 무효화하면 실제 Seed 가 뒤따르지만 최종 로그는 원래 후보다', () => {
    const input = new FakeDryScene();
    input.supply([input.object(20, 30)]);
    input.debug = 29;
    input.field(0x248, 0x20);
    input.field(0x270, floatToBits(10));
    input.field(0x274, floatToBits(20));
    input.field(0x278, floatToBits(30));
    input.field(0x27c, floatToBits(40));
    input.duringMark = () => { input.field(0xaf0, 0); };
    input.make().extend(input.token);
    assert.equal(JSON.stringify(input.trace), '["score","center","candidate","add","mark","seed","added"]');
    assert.equal(JSON.stringify(input.logPoints[0]), '[100,100]');
    assert.ok(input.field(0xaf0) === bits(10) && input.field(0xaf4) === bits(20) && input.field(0xae0) === 3 && input.random.state !== 8);
});

test('AI 가짜 부족좌표: 후보 계산 뒤 현재 TYPE/전략과 b00 을 읽고 출력이 꺼진 경우 실제 로그를 요구하지 않는다', () => {
    const input = new FakeDryScene();
    input.supply([input.object(20, 30)]);
    input.duringCandidate = () => { input.field(0x280, 0xdeadbeef); input.field(0xb00, 0); input.field(0x2d4, 3); };
    input.hooks.log = undefined;
    input.make().extend(input.token);
    assert.ok(input.field(0xae0) === 2 && get(input.memory.bytes(input.records), 20) === 0xdeadbeef && !input.field(0x3d8) && input.logs.length === 0);
});

test('AI 가짜 부족좌표: 선택된 실제 경로/표식 누락에서도 이미 완료한 full 제거·좌표/전략 쓰기는 유지한다', () => {
    const input = new FakeDryScene();
    assert.ok(input.dry().add(input.token, 180, 190, 71));
    input.field(0xadc, 2);
    input.supply([input.object(20, 30)]);
    input.hooks.candidate = undefined;
    assert.ok(throws(() => input.make().extend(input.token)) && input.field(0xae0) === 1);
    const marked = new FakeDryScene();
    marked.supply([marked.object(20, 30)]);
    marked.debug = 29;
    marked.field(0x2d4, 4);
    marked.hooks.mark = undefined;
    assert.ok(throws(() => marked.make().extend(marked.token)) && marked.field(0xae0) === 2 && marked.field(0xaf0) === bits(100) && marked.field(0x3d8) === 1 && marked.logs.length === 0);
});

test('AI 가짜 부족좌표 생성자: 본체 소유자/같은 PATCH 풀/현재 전역 조회기가 없으면 거부하고 가짜 본체를 거부한다', () => {
    const input = new FakeDryScene();
    const globals = { manaRadius: () => 22, mapLimit: () => 256, debug: () => 0 };
    assert.ok(throws(() => new RawAiFakeDrySpots(undefined, input.pool, globals, input.hooks)));
    assert.ok(throws(() => new RawAiFakeDrySpots(input.memory, {}, globals, input.hooks)));
    for (const name of ['manaRadius', 'mapLimit', 'debug']) assert.ok(throws(() => new RawAiFakeDrySpots(input.memory, input.pool, { ...globals, [name]: undefined }, input.hooks)));
    const made = new RawAiFakeDrySpots(input.memory, input.pool, globals, undefined);
    assert.equal(made.pool, input.pool);
    const small = input.memory.allocate(16);
    assert.ok(throws(() => made.extend(small)));
});
