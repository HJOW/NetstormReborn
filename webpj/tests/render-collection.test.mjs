// 표시 수집기 raw-render-collection.js(RawRenderCollection: bounds·intersects·collect)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: rendercollection-x86.tsv 의 `originals` 1435행(Bounds 226·Intersect 225·Collect 948·Order 36) — 배율·등록 영역·가시 영역·슬롯 구성(빈 칸 포함)·제한 수·부모 raw(head·좌표·상태)를 주었을 때
//    환산된 월드 사각형 16바이트, 교차 판정(독립 helper 와 collector 내부 판정 모두), 반환 개수와 128바이트 출력(미사용 칸의 0xa5 와 항목 tail 보존 포함), 그리기 정렬 비교(CompareDrawOrder) 결과가 같다.
//    `originalCD`/`original1037` 행은 다른 판본이라 사용하지 않는다.
//  - 계약: 호스트 연결 오류(다른 풀·조회 함수 없음)와 잘못된 배율/부모/출력 용량을 변경 전에 거부하고, 출력을 다 채운 뒤 잘못된 후속 부모에 접근하지 않으며, 조회가 null 인 슬롯은 건너뛴다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E131: 표시 수집기 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, bitsToFloat } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { RawRenderRegistry, createRenderProcessFields } from '../www/core/raw-construction-lifetime.js';
import { RawRenderCollection } from '../www/core/raw-render-collection.js';
import { compareDrawOrder } from '../www/render/draw-sort.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E131)
const allRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'rendercollection-x86.tsv')) ?? [];
const rows = allRows.filter((row) => row[0] === 'originals');

// 독립 관찰의 공통 입력 열 순서이다. 부동소수 값도 DWORD 비트로 읽는다. (2026-11-08 WB04-E131)
const [SX, SY, LEFT, TOP, RIGHT, BOTTOM, VLEFT, VTOP, VRIGHT, VBOTTOM, SLOTS, PATTERN, LIMIT, Z, X, Y, FLAGS] = Array.from({ length: 17 }, (_, index) => index);
const CAPACITY = 24000;

// float 비트를 수로 읽는다. (2026-11-08 WB04-E131)
const float = (bits) => bitsToFloat(bits);

// 예외가 나는지 본다. (2026-11-08 WB04-E131)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 16진수로 만든다. (2026-11-08 WB04-E131)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 월드 사각형을 float 4개 16바이트로 직렬화한다. (2026-11-08 WB04-E131)
function rectBytes(rect) {
    const view = new DataView(new ArrayBuffer(16));
    [rect.left, rect.top, rect.right, rect.bottom].forEach((value, index) => view.setFloat32(index * 4, value, true));
    return hex(new Uint8Array(view.buffer));
}

// 실제 raw 풀/등록 목록에 완성된 객체 입력을 공급하는 장면이다. 기대 환산이나 가시 여부는 계산하지 않는다. (2026-11-08 WB04-E131)
class CollectionScene {
    constructor() {
        this.pool = new SidPool(CAPACITY, { server: false });
        this.state = { processType: 17, palette: 0, scaleX: 16, scaleY: 11 };
        this.fields = [];
        this.registry = null;
        this.input = [];
    }

    // 등록/해제는 이 관찰에서 실행하지 않으므로 외부 효과를 요청하면 실패한다.
    hooks() {
        return {
            reserve: () => { assert.fail('reserve'); },
            releaseArray: () => { assert.fail('releaseArray'); },
            attach: () => { assert.fail('attach'); },
        };
    }

    // 완성된 각 입력 기록을 읽어 슬롯·영역·부모를 연결한다. 실행기의 입력 생성 공식을 복제하지 않는다.
    prepare(parameters, objects) {
        this.input = parameters.split(',').map(Number);
        assert.equal(this.input.length, 17);
        this.state.scaleX = float(this.input[SX]);
        this.state.scaleY = float(this.input[SY]);
        this.fields = Array.from({ length: this.input[SLOTS] }, () => createRenderProcessFields());
        const slots = new Array(this.input[SLOTS]).fill(0);
        if (objects !== '-') {
            // 객체 기록은 slot,parent,z,left,top,right,bottom,x,y,flags 순서이다.
            for (const record of objects.split(';')) {
                const values = record.split(',').map(Number);
                assert.equal(values.length, 10);
                const slot = values[0];
                const field = this.fields[slot];
                slots[slot] = slot + 1;
                field.token = slot + 1;
                field.slot = slot;
                field.parent = values[1];
                field.zOrder = values[2] | 0;
                field.left = float(values[3]);
                field.top = float(values[4]);
                field.right = float(values[5]);
                field.bottom = float(values[6]);
                const raw = this.pool.physical(values[1]);
                put(raw, 6, 7, 2);
                raw[11] = values[9] & 0xff;
                put(raw, 14, values[7]);
                put(raw, 18, values[8]);
            }
        }
        this.registry = new RawRenderRegistry(this.pool, this.hooks(), slots);
    }

    // 현재 필드와 목록의 수명은 반환한 collector 보다 길다.
    collector() {
        return new RawRenderCollection(this.pool, this.state, this.registry, (token) => this.fields[token - 1]);
    }
}

test('RenderCollection: 원본 10.78 Bounds/Intersect/Collect/Order 1435행의 환산 사각형·교차·반환 개수·출력 128바이트·정렬 비교가 같다', { skip: rows.length === 0 && '원본 fixture 없음' }, () => {
    const scene = new CollectionScene();
    let count = 0;
    // 각 행의 기대 출력은 독립 PE 실행으로 생성한 저장 관찰에서만 읽는다.
    for (const row of rows) {
        assert.equal(row.length, 8);
        scene.prepare(row[2], row[3]);
        const collector = scene.collector();
        const input = scene.input;
        const view = { left: float(input[VLEFT]), top: float(input[VTOP]), right: float(input[VRIGHT]), bottom: float(input[VBOTTOM]) };
        const output = new Uint8Array(128).fill(0xa5);
        const label = `표시 수집 행 ${count} ${row[1]}`;
        if (row[1] === 'Bounds') {
            // 환산 결과는 NaN 비트까지 그대로 비교한다.
            assert.equal(rectBytes(collector.bounds(scene.fields[0])), row[6], label);
            assert.equal(hex(output), row[5], label);
        } else if (row[1] === 'Intersect') {
            const bounds = { left: float(input[LEFT]), top: float(input[TOP]), right: float(input[RIGHT]), bottom: float(input[BOTTOM]) };
            assert.equal(RawRenderCollection.intersects(view, bounds), Number(row[4]) !== 0, label);
            assert.equal(collector.collect(view, output, 1), Number(row[4]), label);
        } else if (row[1] === 'Collect') {
            const result = collector.collect(view, output, input[LIMIT] | 0);
            assert.equal(result >>> 0, Number(row[4]), label);
            assert.equal(hex(output), row[5], label);
        } else {
            const result = compareDrawOrder({ x: float(input[X]), y: float(input[Y]), depth: (input[Z] << 16) >> 16 }, { x: 0, y: 0, depth: 0 });
            assert.equal(result >>> 0, Number(row[4]), label);
        }
        count += 1;
    }
    assert.equal(count, 1435);
});

test('RenderCollection: 호스트 연결 오류와 잘못된 배율/부모/출력 용량을 거부하고 출력을 채운 뒤 후속 부모에 접근하지 않는다', () => {
    const scene = new CollectionScene();
    scene.prepare('1065353216,1065353216,0,0,1092616192,1092616192,0,0,1101004800,1101004800,2,0,1,0,0,0,0',
        '0,50,0,0,0,1092616192,1092616192,0,0,255;1,51,0,0,0,1092616192,1092616192,0,0,0');
    const collector = scene.collector();
    const output = new Uint8Array(16);
    const view = { left: 0, top: 0, right: 20, bottom: 20 };
    const token = () => new DataView(output.buffer).getUint32(8, true);
    put(scene.pool.physical(51), 6, 0, 2);
    assert.ok(collector.collect(view, output, 1) === 1 && token() === 1);
    assert.ok(throws(() => collector.collect(view, output, 2)));
    assert.equal(token(), 1);
    scene.state.scaleX = 0;
    assert.ok(collector.collect(view, new Uint8Array(0), 0) === 0 && collector.collect(view, new Uint8Array(0), -1) === 0);
    assert.ok(throws(() => collector.bounds(scene.fields[0])));
    scene.state.scaleX = Infinity;
    assert.ok(throws(() => collector.bounds(scene.fields[0])));
    scene.state.scaleX = NaN;
    assert.ok(throws(() => collector.bounds(scene.fields[0])));
    scene.state.scaleX = 1;
    scene.fields[0].parent = -1;
    assert.ok(throws(() => collector.bounds(scene.fields[0])));
    scene.fields[0].parent = scene.pool.capacity;
    assert.ok(throws(() => collector.bounds(scene.fields[0])));
    scene.fields[0].parent = 51;
    assert.ok(throws(() => collector.bounds(scene.fields[0])));
    const other = new SidPool(CAPACITY, { server: false });
    assert.ok(throws(() => new RawRenderCollection(other, scene.state, scene.registry, () => null)));
    assert.ok(throws(() => new RawRenderCollection(scene.pool, scene.state, scene.registry, null)));
    const incomplete = new RawRenderCollection(scene.pool, scene.state, scene.registry, () => null);
    assert.ok(incomplete.collect(view, output, 1) === 0 && token() === 1);
});
