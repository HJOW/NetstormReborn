// 썬 바리케이드 재생성 raw-fence-rebuild.js(RawFenceRebuild)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: fence-create-x86.tsv 의 재생성(R) 166행 — 입력 5열(부모 extra·이웃 존재 비트·사거리 비트·이웃 extra 네 BYTE·외부 효과 변이) → 부모+네 이웃(SID 50..54) 전체 50바이트 raw, 호출 순서
//    (Neighbor/Range/Markers/Shields: 좌표 float 비트·소유자·SID). 같은 fixture 의 표식(M)/포스필드(S) 행은 RawFenceCreate 단위(미이식)에서 대조한다.
//  - 계약: FenceCreate_RequiredBoundariesAndPartialWrites 의 재생성 부분(진입 extra 차단은 하위 효과를 요구하지 않고, 선택한 경계의 누락은 거부).
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E81: 썬 바리케이드 재생성 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { RawFenceRebuild } from '../www/core/raw-fence-rebuild.js';

// 원본 관측 중 재생성(R) 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E81)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'fence-create-x86.tsv'))?.filter((row) => row[0] === 'R') ?? null;

// 관찰 부모 SID, 이웃 SID 범위, 독립 입력 수, 격리 원본 실행기의 가상 표 기록값, 클라이언트 확보 상한이다. (2026-11-08 WB04-E81)
const SOURCE = 50;
const ROWS = 166;
const VTABLE = 0x124f0000;
const LAST_CLIENT = 159;

// 예외가 나는지 본다. (2026-11-08 WB04-E81)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E81)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 네 float 인수의 원본 비트를 순서대로 기록한다. (2026-11-08 WB04-E81)
const pointsTrace = (prefix, points) => points.reduce((text, value) => `${text},${floatToBits(value)}`, prefix);

// 같은 풀과 명시 하위 효과 입력만 조립하는 재생성 장면이다. (2026-11-08 WB04-E81)
class RebuildScene {
    constructor(values) {
        this.input = values.split(',').map(Number);
        this.mutation = this.input[4];
        this.direction = 0;
        this.trace = [];
        this.pool = new SidPool(24000);
        // 명시 생성이 반환하는 60~159 와 같은 클라이언트 슬롯을 입력으로 확보한다.
        for (let sid = 5; sid <= LAST_CLIENT; sid += 1) assert.equal(this.pool.allocate(2), sid);
        this.parents = [50, 51, 52, 53, 54];
        // 부모와 네 이웃의 모든 보존 바이트를 원본 입력과 동일하게 준비한다.
        for (const sid of this.parents) {
            const raw = this.pool.allocated(sid);
            raw.fill(0xa5);
            put(raw, 0, VTABLE);
            raw[10] = 136;
            raw[11] = 0;
            raw[40] = 0;
            raw[34] = 1;
            put(raw, 14, floatToBits(sid + 0.25));
            put(raw, 18, floatToBits(sid + 0.5));
            raw[40] = sid === SOURCE ? this.input[0] : (this.input[3] >>> ((sid - 51) * 8)) & 0xff;
        }
        this.rebuild = new RawFenceRebuild(this.pool, this.hooks());
    }

    // 재생성 관찰의 이웃/거리/하위 생성은 선언한 외부 입력이며 실제 Session 은 제품 모듈을 연결한다.
    hooks() {
        return {
            neighbor: (sid, value) => {
                assert.equal(sid, SOURCE);
                this.direction = value;
                this.trace.push(`Neighbor,${value}`);
                if (this.mutation === 1 && value === 0) put(this.pool.allocated(SOURCE), 14, floatToBits(120.0));
                return (this.input[1] & (1 << (value / 2))) ? 51 + value / 2 : 0;
            },
            withinRange: (sid, points) => {
                assert.equal(sid, SOURCE);
                this.trace.push(pointsTrace('Range', points));
                if (this.mutation === 2) this.pool.allocated(51 + this.direction / 2)[40] = 0x21;
                return (this.input[2] & (1 << (this.direction / 2))) !== 0;
            },
            markers: (sid, other) => {
                assert.equal(sid, SOURCE);
                this.trace.push(`Markers,${other}`);
                if (this.mutation === 3) {
                    this.pool.allocated(SOURCE)[34] = 5;
                    put(this.pool.allocated(other), 18, floatToBits(125.0));
                }
            },
            shields: (points, a, b, first, second) => {
                this.trace.push(`${pointsTrace('Shields', points)},1,${a},${b},${first},${second}`);
                if (this.mutation === 4) this.pool.allocated(SOURCE)[40] = 9;
            }
        };
    }

    // 원본이 기록한 전체 부모 바이트이다.
    parentsHex() { return this.parents.map((sid) => hex(this.pool.slot(sid))).join(''); }
}

test('FenceRebuild: 원본 10.78 166입력의 진입 관문·거리 뒤 대상 extra·생성 뒤 현재 인수와 호출 순서·부모/이웃 raw 가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    let count = 0;
    // 기대값은 실제 원본에서만 읽는다.
    for (const row of rows) {
        assert.equal(row.length, 5);
        const scene = new RebuildScene(row[1]);
        scene.rebuild.rebuild(SOURCE);
        const label = `행 ${count}: 입력 ${row[1]}`;
        assert.equal(scene.parentsHex(), row[2], label);
        assert.equal('-', row[3], label);
        assert.equal(scene.trace.length === 0 ? '-' : scene.trace.join(';'), row[4], label);
        count += 1;
    }
    assert.equal(count, ROWS);
});

test('FenceRebuild: 부모 extra 로 막히면 하위 효과를 요구하지 않고 선택한 경계의 누락만 거부하며 판본을 구별한다', () => {
    const scene = new RebuildScene('1,15,15,0,0');
    const noEffects = new RawFenceRebuild(scene.pool, {});
    noEffects.rebuild(SOURCE);
    assert.equal(scene.trace.length, 0);
    scene.pool.allocated(SOURCE)[40] = 0;
    assert.ok(throws(() => noEffects.rebuild(SOURCE)));
    // 이웃이 없으면 사거리/표식/포스필드를 요구하지 않는다.
    const alone = new RawFenceRebuild(scene.pool, { neighbor: () => 0 });
    alone.rebuild(SOURCE);
    assert.ok(throws(() => new RawFenceRebuild({ edition: 'cd1072' }, {})));
});
