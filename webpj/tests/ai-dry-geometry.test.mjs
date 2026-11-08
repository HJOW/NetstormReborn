// AI 가짜 부족 좌표 탐색 raw-ai-dry-geometry.js(RawAiDryGeometry·aiPointDistance)를 cpppj 계약 시험(손으로 쓴 입력과 기대값)으로 검사한다.
//  - 이 몸체에는 독립 원본 관측 fixture 가 없다(cpppj 시험이 원본 분기 순서를 직접 단언한다). 같은 입력과 단언을 옮겼다: 두 float 차이/제곱합의 중간 float 저장 없음, 동률은 왼쪽 우선이며 두 방향을 매 반복 모두 방문,
//    0.9999f 편향 합 뒤 ftol, 점유 칸의 연속 횟수 초기화와 오른쪽만 먼저 4 에 도달, 경계 끝값 포함·이동 뒤 경계 밖 한 칸 조회, 물리 256 밖은 빈칸이며 지도 효과를 요청하지 않음, 디버그 표식 순서/현재 종류 재조회,
//    표식 정리(TYPE BYTE 와 WORD 93 일치만, dead 방문/free 건너뜀, 삭제 전 다음 SID 저장, 현재 전체 DWORD TYPE), 누락 효과의 부분 쓰기 보존.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E163: AI 가짜 부족 좌표 탐색 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import { put, get, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { RawAiDryGeometry, aiPointDistance } from '../www/core/raw-ai-dry-geometry.js';

// 예외가 나는지 본다. (2026-11-09 WB04-E163)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 단정도 점을 만든다. (2026-11-09 WB04-E163)
const point = (x, y) => ({ x: Math.fround(x), y: Math.fround(y) });

// 같은 실제 풀과 명시 지도/표식 효과 입력을 준비하는 시험 장면이다. (2026-11-09 WB04-E163)
class DryGeometryScene {
    // 외부 효과의 결과를 주입하지 않고 각 실제 슬롯/현재 지도를 읽고 쓴다.
    constructor() {
        this.pool = new SidPool(32768, { server: false });
        this.globals = { bounds: { left: 10, top: 10, right: 246, bottom: 246 }, debug: 99, markerType: 104 };
        this.surface = new Uint16Array(65536);
        this.cells = [];
        this.points = [];
        this.created = [];
        this.destroyed = [];
        this.trace = [];
        this.duringPop = null;
        this.duringDestroy = null;
        this.hooks = {
            surface: (x, y) => { this.cells.push([x, y]); return this.surface[y * 256 + x]; },
            create: (type, flags) => {
                assert.equal(flags, 2);
                const sid = this.pool.allocate(flags);
                const raw = this.pool.allocated(sid);
                raw.fill(0);
                raw[10] = type;
                this.created.push(sid);
                this.trace.push('create');
                return sid;
            },
            word: (sid, word) => { assert.equal(word, 93); put(this.pool.allocated(sid), 12, word, 2); this.trace.push('word'); },
            pop: (sid, at, flags) => {
                assert.ok(!flags && get(this.pool.slot(sid), 12, 2) === 93);
                put(this.pool.allocated(sid), 14, floatToBits(at.x));
                put(this.pool.allocated(sid), 18, floatToBits(at.y));
                this.points.push(at);
                this.trace.push('pop');
                if (this.duringPop) this.duringPop(this.points.length);
            },
            destroy: (sid, flags) => {
                assert.ok(!flags);
                this.destroyed.push(sid);
                this.trace.push('destroy');
                if (this.duringDestroy) this.duringDestroy(sid);
            }
        };
    }

    // 같은 현재 풀/전역/선택 효과를 빌린다. (2026-11-09 WB04-E163)
    make() { return new RawAiDryGeometry(this.pool, this.globals, this.hooks); }

    // 현재 풀에 TYPE/WORD 가 다른 실제 표식 후보를 준비한다. (2026-11-09 WB04-E163)
    object(type, word) {
        const sid = this.pool.allocate(2);
        const raw = this.pool.allocated(sid);
        raw.fill(0);
        raw[10] = type;
        put(raw, 12, word, 2);
        return sid;
    }
}

test('AI 가짜좌표: 두 float 의 차이/제곱합은 중간 float 저장 없이 계산한다', () => {
    assert.ok(aiPointDistance({ x: 0, y: 0 }, { x: 3, y: 4 }) === 5 && aiPointDistance({ x: -1, y: -1 }, { x: -1, y: 4 }) === 5);
    const huge = 3.4028234663852886e38;
    assert.ok(Number.isFinite(aiPointDistance({ x: huge, y: 0 }, { x: -huge, y: 0 })));
    assert.ok(Number.isNaN(aiPointDistance({ x: Number.NaN, y: 0 }, { x: 0, y: 0 })));
});

test('AI 가짜좌표: 동률은 왼쪽 우선이며 두 방향을 각 반복에서 모두 방문한다', () => {
    const input = new DryGeometryScene();
    const found = input.make().candidate(point(30, 40), point(34, 40));
    assert.ok(found.x === 32 && found.y === 44 && input.cells.length === 8 && input.created.length === 0 && input.destroyed.length === 0);
    assert.deepEqual([input.cells[0], input.cells[1], input.cells[6], input.cells[7]], [[32, 41], [32, 39], [32, 44], [32, 36]]);
    const vertical = new DryGeometryScene();
    const other = vertical.make().candidate(point(40, 40), point(40, 44));
    assert.ok(other.x === 36 && other.y === 42);
});

test('AI 가짜좌표: ceil 이나 사제용 0.99999 가 아닌 float 0.9999 편향 합 뒤 ftol 을 사용한다', () => {
    const input = new DryGeometryScene();
    input.make().candidate(point(30.00005, 40.00005), point(34.00005, 40.00005));
    assert.deepEqual(input.cells[0], [32, 41]);
    const next = new DryGeometryScene();
    next.make().candidate(point(30.0002, 40.0002), point(34.0002, 40.0002));
    assert.deepEqual(next.cells[0], [33, 42]);
});

test('AI 가짜좌표: 점유 칸은 각 방향 연속 횟수를 0 으로 만들며 오른쪽만 먼저 4 에 도달할 수 있다', () => {
    const input = new DryGeometryScene();
    input.surface[42 * 256 + 32] = 5;
    const found = input.make().candidate(point(30, 40), point(34, 40));
    assert.ok(found.x === 32 && found.y === 36 && input.cells.length === 8);
    const both = new DryGeometryScene();
    both.surface[42 * 256 + 32] = 5;
    both.surface[39 * 256 + 32] = 6;
    const other = both.make().candidate(point(30, 40), point(34, 40));
    assert.ok(other.x === 32 && other.y === 35 && both.cells.length === 10);
});

test('AI 가짜좌표: 현재 점이 경계 끝값에 있으면 이동하고 이동 뒤 경계 밖 한 칸도 원본처럼 조회한다', () => {
    const input = new DryGeometryScene();
    const found = input.make().candidate(point(30, 10), point(34, 10));
    assert.ok(found.x === 32 && found.y === 14 && input.cells.length === 5);
    assert.deepEqual(input.cells[1], [32, 9]);
});

test('AI 가짜좌표: 물리 256 범위 밖은 빈칸으로 계산하고 현재 지도 효과를 요청하지 않는다', () => {
    const input = new DryGeometryScene();
    input.globals.bounds = { left: -500, top: -500, right: 500, bottom: 500 };
    input.hooks.surface = undefined;
    const found = input.make().candidate(point(256, 40), point(260, 40));
    assert.ok(found.x === 258 && found.y === 44 && input.cells.length === 0);
});

test('AI 가짜좌표: 디버그 표식은 초기 두 점 → 선분 네 점 → 수직 양쪽 여덟 점 순서이며 현재 종류도 재조회한다', () => {
    const input = new DryGeometryScene();
    const old = input.object(104, 93);
    input.globals.debug = 29;
    input.make().candidate(point(30, 40), point(34, 40));
    assert.deepEqual(input.destroyed, [old]);
    assert.ok(input.points.length === 14 && input.created.length === 14 && input.trace[0] === 'destroy');
    assert.ok(input.points[0].x === 30 && input.points[1].x === 34 && input.points[2].x === 31 && input.points[5].x === 34
        && input.points[6].y === 41 && input.points[7].y === 39 && input.points[12].y === 44 && input.points[13].y === 36);
    const changed = new DryGeometryScene();
    changed.globals.debug = 29;
    changed.duringPop = (count) => { if (count === 1) { changed.globals.debug = 99; changed.globals.markerType = 105; } };
    changed.make().candidate(point(30, 40), point(34, 40));
    assert.ok(changed.points.length === 2 && changed.pool.slot(changed.created[0])[10] === 104 && changed.pool.slot(changed.created[1])[10] === 105);
});

test('AI 가짜좌표: 원본 선택 위치에서 누락 효과를 거부하고 생성/kind 쓰기 등 이미 수행한 상태를 보존한다', () => {
    const input = new DryGeometryScene();
    input.hooks.word = undefined;
    assert.ok(throws(() => input.make().mark(point(1, 2))) && input.created.length === 1 && get(input.pool.slot(input.created[0]), 12, 2) === 0 && input.points.length === 0);
    const after = new DryGeometryScene();
    after.hooks.pop = undefined;
    assert.ok(throws(() => after.make().mark(point(1, 2))) && after.created.length === 1 && get(after.pool.slot(after.created[0]), 12, 2) === 93 && after.points.length === 0);
});

test('AI 가짜좌표: 표식 정리는 TYPE BYTE 와 WORD 93 이 모두 일치하는 것만 지우며 dead 는 방문하고 free 는 건너뛴다', () => {
    const input = new DryGeometryScene();
    const first = input.object(104, 93);
    input.object(104, 92);
    input.object(105, 93);
    const dead = input.object(104, 93);
    input.pool.allocated(dead)[11] |= 2;
    const free = input.object(104, 93);
    input.pool.allocated(free)[11] |= 1;
    input.make().clearMarkers();
    assert.deepEqual(input.destroyed, [first, dead]);
});

test('AI 가짜좌표: 표식 정리는 삭제 전에 다음 SID 를 저장하므로 삭제 효과에서 다음이 free 가 되어도 저장 SID 를 방문한다', () => {
    const input = new DryGeometryScene();
    const first = input.object(104, 93);
    const second = input.object(104, 93);
    input.duringDestroy = (sid) => { if (sid === first) input.pool.allocated(second)[11] |= 1; };
    input.make().clearMarkers();
    assert.deepEqual(input.destroyed, [first, second]);
});

test('AI 가짜좌표: 표식 정리는 각 삭제 뒤 현재 전체 DWORD TYPE 을 읽고 상위 비트가 있으면 BYTE 를 잘라 일치시키지 않는다', () => {
    const input = new DryGeometryScene();
    const first = input.object(104, 93);
    const second = input.object(105, 93);
    input.object(104, 93);
    input.duringDestroy = (sid) => { if (sid === first) input.globals.markerType = 105; };
    input.make().clearMarkers();
    assert.deepEqual(input.destroyed, [first, second]);
    const upper = new DryGeometryScene();
    upper.object(104, 93);
    upper.globals.markerType = 0x168;
    upper.make().clearMarkers();
    assert.equal(upper.destroyed.length, 0);
});

test('AI 가짜좌표: 처음 도달하는 지도 경계만 요구하며 미연결이면 후보를 위조하지 않고 거부한다', () => {
    const input = new DryGeometryScene();
    input.hooks.surface = undefined;
    assert.ok(throws(() => input.make().candidate(point(30, 40), point(34, 40))) && input.created.length === 0);
    // PATCH WORD 풀이 아니면 생성 전에 거부한다.
    assert.ok(throws(() => new RawAiDryGeometry(new SidPool(70000, { server: false }), input.globals, input.hooks)));
});
