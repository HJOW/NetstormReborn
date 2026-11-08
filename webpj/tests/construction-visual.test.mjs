// 건설 표시 생성·좌표 동기화·초기 입자 선택 raw-construction-visual.js(RawConstructionVisual)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: constructionvisual-x86.tsv 의 `originals` 592행(Create 520·Seed 60·Sync 12) — 부모 raw 입력·캡처 SHP 패턴 7종·공유 난수 상태·색 표 캐시 상태를 주었을 때
//    외부 요청 순서(A 확보/B 부착/P 팔레트 생성/G 색 띠/J 팔레트 합성/C 캡처/F 입자 반납), 가상 표·필드 21개(부모·zOrder·사각형·팔레트·크기·hotspot·마지막 행·수명 범위·누적 선택·불투명·활성·용량·초기 입자 수),
//    원본 SHP 픽셀(입자로 가져간 곳은 투명)·출력 버퍼·초기 입자 목록 전체·난수 상태·색 표 캐시가 같다. `originalCD`/`original1037` 행은 10.72/다른 판본이라 사용하지 않는다.
//  - 계약: 확보 실패·잘못된 크기/출력/상태를 쓰기 전에 거부, 경계 누락·다른 풀의 어댑터 거부, 건설 시작 어댑터가 초기 상태를 저장소로 넘김.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E128: 건설 표시 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, bitsToFloat, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { GameRandom } from '../www/core/random.js';
import { RawConstructionVisual, createConstructionVisualFields, createConstructionVisualState, makeConstructionVisualHooks } from '../www/core/raw-construction-visual.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E128)
const allRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'constructionvisual-x86.tsv')) ?? [];
const rows = allRows.filter((row) => row[0] === 'originals');

// 실행기와 같은 부모 SID·합성 가상 표·풀 크기·공유 색 표 항목이다. (2026-11-08 WB04-E128)
const PARENT = 50;
const VTABLE = 0x11010000;
const CAPACITY = 24000;
const PALETTE = 0x12000200;

// fixture 입력의 DWORD 열 순서이다. 기대 입자를 계산하지 않는다. (2026-11-08 WB04-E128)
const INSTANT = 0, WIDTH = 1, HEIGHT = 2, PATTERN = 3, RANDOM = 4, COLD = 5, MUTATE = 6, Z = 7, X = 8, Y = 9, SCALE_X = 10, SCALE_Y = 11, SELECTED = 12, OLD = 13, INPUT_COUNT = 14;

// 바이트 배열을 소문자 16진수로 만든다. (2026-11-08 WB04-E128)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 예외가 나는지 본다. (2026-11-08 WB04-E128)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 실제 풀/표시/난수와 명시 경계의 입력을 공급하는 장면이다. (2026-11-08 WB04-E128)
class VisualScene {
    constructor() {
        this.pool = new SidPool(CAPACITY, { server: false });
        this.random = new GameRandom(0);
        this.state = createConstructionVisualState();
        this.input = [];
        this.events = [];
        this.failBytes = 0;
        this.visual = new RawConstructionVisual(this.pool, this.state, this.random, this.hooks());
    }

    // 부모 자료와 공유 상태만 설정한다. 이미지와 입자 결과는 생성하지 않는다.
    prepare(values) {
        assert.equal(values.length, INPUT_COUNT);
        this.input = values;
        this.events = [];
        this.failBytes = 0;
        this.random.setState(values[RANDOM]);
        Object.assign(this.state, createConstructionVisualState(), { palette: values[COLD] ? 0 : PALETTE, scaleX: bitsToFloat(values[SCALE_X]), scaleY: bitsToFloat(values[SCALE_Y]) });
        const raw = this.pool.physical(PARENT);
        raw.fill(0);
        put(raw, 0, VTABLE);
        raw[10] = 100;
        raw[34] = 1;
        raw[35] = values[Z] & 0xff;
        put(raw, 14, values[X]);
        put(raw, 18, values[Y]);
    }

    // 각 패턴은 PE 실행에 공급한 캡처 입력이다. 입자 선택 기대값은 아니다.
    image() {
        const width = this.input[WIDTH];
        const height = this.input[HEIGHT];
        const pixels = new Uint8Array(width * height);
        // 하단/구멍/중앙 열/마지막 픽셀/색 0 을 구별하는 일곱 입력을 만든다.
        for (let index = 0; index < pixels.length; index += 1) {
            let color = (index * 13 + 17) % 255;
            const pattern = this.input[PATTERN];
            if (pattern === 1) color = 255;
            if (pattern === 2 && index % 3 === 0) color = 255;
            if (pattern === 3 && Math.floor(index / width) > Math.floor(height / 3)) color = 255;
            if (pattern === 4 && index % width !== Math.floor(width / 2)) color = 255;
            if (pattern === 5 && index + 1 !== pixels.length) color = 255;
            if (pattern === 6) color = 0;
            pixels[index] = color;
        }
        return { width, height, hotspotX: -3, hotspotY: 2, pixels };
    }

    // 실제 부착/캡처/팔레트/확보 요청을 기록하고 실행기와 같은 경계 변경만 공급한다.
    hooks() {
        return {
            reserve: (size) => { this.events.push(`A:${size}`); return size !== this.failBytes; },
            attach: (fields, type) => {
                this.events.push(`B:${fields.parent}:${type}`);
                if (this.input[MUTATE] === 1) this.state.palette = PALETTE;
                if (this.input[MUTATE] === 2) put(this.pool.physical(PARENT), 14, floatToBits(7.25));
            },
            createPalette: () => { this.events.push('P'); return PALETTE; },
            gradient: (count, from, to, noise) => {
                // 실제 경계에서 관찰한 시작/끝 RGB 이다.
                const blue = [0, 0, 1];
                const white = [1, 1, 1];
                const gray = [128 / 255, 128 / 255, 128 / 255];
                assert.equal(noise, 2.4);
                assert.ok((count === 8 && String(from) === String(blue) && String(to) === String(white)) || (count === 3 && String(from) === String(white) && String(to) === String(gray)));
                this.events.push(`G:${count}`);
            },
            buildPalette: (palette) => { assert.equal(palette, PALETTE); this.events.push('J'); },
            capture: (parent) => {
                this.events.push(`C:${parent}`);
                if (this.input[MUTATE] === 3) {
                    this.state.scaleX = -4.5;
                    put(this.pool.physical(PARENT), 14, floatToBits(-1.25));
                }
                return this.image();
            },
            releaseParticles: () => { this.events.push('F'); },
        };
    }

    // 직접 seed/synchronize 검사에 필요한 기존 필드만 입력으로 공급한다.
    initial() {
        const fields = createConstructionVisualFields();
        fields.parent = PARENT;
        fields.instant = this.input[INSTANT] !== 0;
        fields.zOrder = 123;
        fields.left = 1; fields.top = 2; fields.right = 3; fields.bottom = 4;
        fields.palette = PALETTE;
        fields.image = this.image();
        fields.output = new Uint8Array(fields.image.pixels.length).fill(255);
        fields.lastRow = (fields.image.height - 1) * fields.image.width;
        fields.minimum = fields.instant ? 2 : bitsToFloat(0x3fa66666);
        fields.maximum = fields.instant ? 9 : 2;
        fields.selected = this.input[SELECTED];
        fields.firstDraw = true;
        fields.active = 3;
        if (this.input[OLD]) { fields.capacity = 9; fields.particles = Array.from({ length: 9 }, () => ({ active: 0, color: 0, life: 0, pixel: 0 })); }
        return fields;
    }
}

// 실제 필드를 실행기의 DWORD 열 순서로 직렬화한다. 음수는 하위 DWORD 비트로 보존한다. (2026-11-08 WB04-E128)
function fieldsText(fields) {
    const table = fields.instant ? 0x503680 : 0x5036ac;
    const image = fields.image;
    return [table, fields.parent, fields.zOrder >>> 0,
        floatToBits(fields.left), floatToBits(fields.top), floatToBits(fields.right), floatToBits(fields.bottom), fields.palette,
        image.width >>> 0, image.height >>> 0, image.hotspotX >>> 0, image.hotspotY >>> 0, fields.lastRow >>> 0,
        floatToBits(fields.minimum), floatToBits(fields.maximum),
        fields.selected, fields.opaque, fields.firstDraw ? 1 : 0, fields.active, fields.capacity, fields.total].join(':');
}

// 입자 목적지는 출력 버퍼의 픽셀 번호로 정규화한다. 미사용 칸은 0 임을 별도로 확인한다. (2026-11-08 WB04-E128)
function particlesText(fields) {
    assert.equal(fields.particles.length, fields.capacity);
    const text = [];
    // 사용한 항목의 active·원래 색·수명 비트·목적지 순서를 보존한다.
    for (let index = 0; index < fields.particles.length; index += 1) {
        const particle = fields.particles[index];
        if (index >= fields.total) {
            assert.ok(!particle.active && !particle.color && !floatToBits(particle.life) && !particle.pixel);
            continue;
        }
        text.push(`${particle.active}:${particle.color}:${floatToBits(particle.life)}:${particle.pixel}`);
    }
    return text.length ? text.join(';') : '-';
}

// 계약/통합 검사에서 사용할 기본 입력이다. 결과 계산은 포함하지 않는다. (2026-11-08 WB04-E128)
const defaultInput = () => [0, 7, 9, 0, 0, 0, 0, 128, floatToBits(20.5), floatToBits(-3.25), floatToBits(16), floatToBits(11), 0, 0];

test('ConstructionVisual: 원본 10.78 Create/Seed/Sync 592행의 외부 요청·필드·픽셀·출력·입자·난수·색 표가 같다', { skip: rows.length === 0 && '원본 fixture 없음' }, () => {
    const scene = new VisualScene();
    let count = 0;
    // 입력만 구성하고 기대 선택/수명/좌표는 실제 PE 관찰에서 읽는다.
    for (const row of rows) {
        assert.equal(row.length, 10);
        scene.prepare(row[2].split(',').map(Number));
        let fields;
        if (row[1] === 'Create') fields = scene.visual.create(PARENT, scene.input[INSTANT] !== 0);
        else {
            fields = scene.initial();
            if (row[1] === 'Seed') scene.visual.seed(fields);
            else scene.visual.synchronize(fields);
        }
        const observed = [scene.events.join(';'), fieldsText(fields), hex(fields.image.pixels), hex(fields.output), particlesText(fields), scene.random.state, scene.state.palette];
        const expected = [row[3], row[4], row[5], row[6], row[7], Number(row[8]), Number(row[9])];
        assert.deepEqual(observed, expected, `건설 표시 행 ${count} ${row[1]}`);
        count += 1;
    }
    assert.equal(count, 592);
});

test('ConstructionVisual: 확보 실패와 잘못된 이미지/출력/부모 상태를 쓰기 전에 거부한다', () => {
    const scene = new VisualScene();
    scene.prepare(defaultInput());
    scene.failBytes = 136;
    assert.ok(throws(() => scene.visual.create(PARENT, false)) && scene.events.join(';') === 'A:136');
    const cold = defaultInput();
    cold[COLD] = 1;
    scene.prepare(cold);
    scene.failBytes = 8;
    assert.ok(throws(() => scene.visual.create(PARENT, false)) && scene.events.join(';') === 'A:136;B:50:17;A:8' && !scene.state.palette);
    scene.prepare(defaultInput());
    scene.failBytes = 63;
    assert.ok(throws(() => scene.visual.create(PARENT, false)) && scene.random.state === 0);
    scene.prepare(defaultInput());
    scene.failBytes = 6000;
    const unallocated = scene.initial();
    assert.ok(throws(() => scene.visual.seed(unallocated)) && hex(unallocated.image.pixels) === hex(scene.image().pixels) && scene.random.state === 0);
    let fields = scene.initial();
    fields.image.height = 0;
    assert.ok(throws(() => scene.visual.seed(fields)));
    fields = scene.initial();
    fields.output = fields.output.slice(0, -1);
    assert.ok(throws(() => scene.visual.seed(fields)) && hex(fields.image.pixels) === hex(scene.image().pixels));
    scene.prepare(defaultInput());
    scene.pool.physical(PARENT)[11] = 4;
    assert.ok(throws(() => scene.visual.create(PARENT, true)) && scene.events.length === 0);
});

test('ConstructionVisual: 필수 경계 누락과 다른 풀/저장소 없는 어댑터를 거부하고 건설 시작 어댑터가 초기 상태를 넘긴다', () => {
    const scene = new VisualScene();
    scene.prepare(defaultInput());
    const hooks = scene.hooks();
    delete hooks.capture;
    assert.ok(throws(() => new RawConstructionVisual(scene.pool, scene.state, scene.random, hooks)));
    const other = new SidPool(CAPACITY, { server: false });
    assert.ok(throws(() => makeConstructionVisualHooks(other, scene.visual, () => {}, {})));
    assert.ok(throws(() => makeConstructionVisualHooks(scene.pool, scene.visual, null, {})));
    // 어댑터가 건설 시작의 표시 효과로 create 결과를 저장소에 넘긴다.
    const stored = [];
    const connected = makeConstructionVisualHooks(scene.pool, scene.visual, (fields) => stored.push(fields), { marker: 7 });
    assert.equal(connected.marker, 7);
    connected.effect(PARENT, true);
    assert.equal(stored.length, 1);
    assert.ok(stored[0].instant && stored[0].minimum === 2 && stored[0].maximum === 9 && stored[0].total > 0);
    // 두 번째 생성은 공유 색 표를 재사용한다(팔레트 생성 요청이 없다).
    scene.state.palette = 0;
    const first = scene.visual.create(PARENT, false);
    const size = scene.events.length;
    const second = scene.visual.create(PARENT, false);
    assert.equal(first.palette, second.palette);
    assert.equal(scene.events.length, size + 5);
});
