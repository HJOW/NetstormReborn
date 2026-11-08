// 썬 바리케이드 수명 raw-fence-lifetime.js(RawFenceLifetime)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: fence-lifetime-x86.tsv(10.78 전용; 두 x87 정밀도에서 일치한 입력) 207행 — 모드(0 Clear / 1 Refresh / 2 PreDestroy)·부모/이웃 상태·extra·이웃 타입·이웃 차단 마스크·예약 마스크·변이 단계·부모 소유자·flags →
//    호출 순서(S 이웃 / M 표식 / L 연결선 / D Unpop / U Repop / B 재생성 / N 예약 / A 지연 Regular / P Damageable)와 다섯 슬롯(50~54) raw 전체.
//  - 계약: FenceLifetime_GuardsAndSamePool(잘못된 판본/범위/선택한 누락 효과 거부, 같은 풀).
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E74: 썬 바리케이드 수명 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { RawFenceLifetime } from '../www/core/raw-fence-lifetime.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E74)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'fence-lifetime-x86.tsv'));

// 독립 입력 수이다. (2026-11-08 WB04-E74)
const ROWS = 207;

// 예외가 나는지 본다. (2026-11-08 WB04-E74)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E74)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 실제 계산 대신 원문에 공급한 외부 이웃/효과 입력만 준비하는 장면이다. (2026-11-08 WB04-E74)
class FenceScene {
    constructor() {
        this.pool = new SidPool(24000, { server: false });
        this.input = [];
        this.trace = [];
        this.mutated = false;
        this.reserved = 0;
        // 실제 Pool 에서 검사 슬롯을 확보한다.
        for (let sid = 5; sid <= 54; sid += 1) assert.equal(this.pool.allocate(2), sid);
    }

    // dead 입력도 준비하는 검사 전용 접근이다. 제품의 할당 규칙은 바꾸지 않는다.
    raw(sid) {
        return this.pool.physical(sid);
    }

    // 원문의 모든 인수 비트를 같은 순서로 기록한다.
    event(kind, values = []) {
        this.trace.push([kind, ...values].join(':'));
    }

    // 최초 선택된 외부 효과만 다음 현재 상태를 바꾸는 명시 입력이다.
    mutate(kind) {
        if (this.mutated) return;
        const mode = this.input[7];
        if (!((mode === 1 && kind === 'M') || (mode === 2 && kind === 'D') || (mode === 3 && kind === 'B') || (mode === 4 && kind === 'L') || (mode === 5 && kind === 'N'))) return;
        this.mutated = true;
        if (mode === 1) { put(this.raw(50), 14, floatToBits(125.5)); this.raw(50)[34] = 3; }
        if (mode === 2) this.raw(52)[11] = 2;
        if (mode === 3) this.raw(50)[10] = 128;
        if (mode === 4) this.raw(50)[11] = 2;
        if (mode === 5) this.raw(53)[11] = 2;
    }

    // 독립 입력만 새로 준비하며 이전 결과를 다음 행으로 넘기지 않는다.
    prepare(values) {
        assert.equal(values.length, 10);
        this.input = values;
        this.trace = [];
        this.mutated = false;
        this.reserved = 0;
        // parent/네 이웃 raw 의 무관한 바이트도 표식으로 보존한다.
        for (let sid = 50; sid <= 54; sid += 1) {
            const bytes = this.raw(sid);
            bytes.fill(0xa5);
            put(bytes, 0, 1);
            bytes[10] = sid === 50 ? 136 : values[4];
            bytes[11] = values[sid === 50 ? 1 : 3];
            bytes[34] = sid === 50 ? values[8] : 2;
            bytes[40] = sid === 50 ? values[2] : 0;
            put(bytes, 14, floatToBits(100 + sid));
            put(bytes, 18, floatToBits(120 + sid));
        }
    }

    // 실제 몸체의 계산/순서 대신 외부 입력과 가상 효과만 제공한다.
    hooks() {
        return {
            neighbor: (sid, direction, allied) => {
                const index = direction / 2;
                let result = sid === 50 ? 51 + (allied ? index : 3 - index) : (direction === 0 ? 50 : 0);
                if (this.input[5] & (1 << index)) result = 0;
                this.event('S', [sid, direction, allied ? 1 : 0, result]);
                return result;
            },
            clearMarkers: (sid, other) => { this.event('M', [sid, other]); this.mutate('M'); },
            clearLine: (points, first, second, a, b) => {
                this.event('L', [floatToBits(points[0]), floatToBits(points[1]), floatToBits(points[2]), floatToBits(points[3]), 0, first, second, a, b]);
                this.mutate('L');
            },
            unpop: (sid, flags) => { this.event('D', [sid, flags]); this.raw(sid)[11] |= 4; this.mutate('D'); },
            repop: (sid, flags) => { this.event('U', [sid, flags]); this.raw(sid)[11] &= ~4 & 0xff; this.mutate('U'); },
            rebuild: (sid) => { this.event('B', [sid]); this.mutate('B'); },
            reserveRegular: () => {
                this.event('N', [0x28]);
                const success = (this.input[6] & (1 << this.reserved)) !== 0;
                this.reserved += 1;
                this.mutate('N');
                return success;
            },
            addRegular: (parent, event, value) => { this.event('A', [event, parent, floatToBits(value)]); },
            damageable: (sid, flags) => { this.event('P', [sid, flags]); }
        };
    }

    // 전체 다섯 슬롯을 원문 순서 그대로 반환한다.
    snapshot() {
        let text = '';
        for (let sid = 50; sid <= 54; sid += 1) text += hex(this.pool.slot(sid));
        return text;
    }
}

test('FenceLifetime: 원본 10.78 207입력의 연결 정리/이웃 갱신/preDestroy 효과 순서와 다섯 슬롯 raw 전체가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = new FenceScene();
    let count = 0;
    // 원문 결과는 독립 TSV 에서 읽는다.
    for (const row of rows) {
        assert.equal(row.length, 3);
        scene.prepare(row[0].split(',').map(Number));
        const logic = new RawFenceLifetime(scene.pool, scene.hooks());
        if (scene.input[0] === 0) logic.clear(50);
        else if (scene.input[0] === 1) logic.refresh(50);
        else logic.preDestroy(50, scene.input[9]);
        assert.equal(scene.trace.length === 0 ? '-' : scene.trace.join(';'), row[1], `행 ${count}: 입력 ${row[0]}`);
        assert.equal(scene.snapshot(), row[2], `행 ${count}: 입력 ${row[0]}`);
        count += 1;
    }
    assert.equal(count, ROWS);
});

test('FenceLifetime: 잘못된 판본/범위/선택한 누락 효과를 허용하지 않는다', () => {
    const scene = new FenceScene();
    scene.prepare('0,0,0,0,136,0,3,0,1,64'.split(',').map(Number));
    const logic = new RawFenceLifetime(scene.pool, scene.hooks());
    assert.equal(logic.pool, scene.pool);
    assert.ok(throws(() => new RawFenceLifetime({ edition: 'cd1072' }, {})));
    const missing = new RawFenceLifetime(scene.pool, {});
    assert.ok(throws(() => missing.clear(50)));
    assert.ok(throws(() => logic.refresh(0)));
    assert.ok(throws(() => logic.refresh(24000)));
});
