// 건설 표시 프레임 갱신 raw-construction-frame.js(RawConstructionFrame: run·draw·expand·append·color·clock)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: constructionframe-x86.tsv 의 `originals` 435행(Run 202·Expand 64·Append 8·Color 90·Clock 49·Draw 18·Sequence 4) — 부모 raw(좌표/HP/상태)·표시 전역·시각(64비트 double 패턴)·입자 3개·난수 상태·색 표를 주었을 때
//    외부 요청 순서(H 최대 HP/I dirty 사각형/D 장치 제출/K 종료), 필드 14개(사각형·delta·scaledDelta·선택·불투명·firstDraw·활성·용량·총수·시각·누적), 원본 픽셀·출력 버퍼·입자 12칸 전체·난수 상태·반환값, 5단계 연속 기록이 같다.
//    `originalCD`/`original1037` 행은 다른 판본이라 사용하지 않는다.
//  - 계약: 잘못된 저장소/팔레트를 쓰기 전에 거부, 경계 누락 거부와 용량 0 Draw 생략, 실제 표시 생성(RawConstructionVisual)→연속 Run/Draw→원래 픽셀 복원→abstract 종료 요청.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E130: 건설 표시 프레임 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, bitsToFloat, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { GameRandom } from '../www/core/random.js';
import { RawConstructionVisual, createConstructionVisualState } from '../www/core/raw-construction-visual.js';
import { RawConstructionFrame, createConstructionFrameFields, createConstructionFrameState } from '../www/core/raw-construction-frame.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E130)
const allRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'constructionframe-x86.tsv')) ?? [];
const rows = allRows.filter((row) => row[0] === 'originals');

// 실행기의 합성 부모/팔레트와 패치 판본 행 수이다. (2026-11-08 WB04-E130)
const PARENT = 50;
const VTABLE = 0x501dd0;
const PALETTE = 0x12000200;
const PATCH_ROWS = 435;

// 입력은 DWORD 와 시각 double 의 64비트 패턴을 함께 포함한다. (2026-11-08 WB04-E130)
const [INSTANT, PATTERN, RANDOM, CAPACITY, TOTAL, ACTIVE, SELECTED, HP, MAXIMUM, FIRST, NOW, LAST, ELAPSED, RATE, X, Y, SCALE_X, SCALE_Y, CAMERA_X, CAMERA_Y, PIXEL, LIFE, COLORS, MUTATE, FREE, PARENT_COLUMN, SOURCE, MAX_LIFE, INPUT_COUNT] = Array.from({ length: 29 }, (_, index) => index);

// BigInt 하위 32비트를 부호 없는 수로 읽는다. float 입력은 하위 DWORD 만 그대로 쓴다. (2026-11-08 WB04-E130)
const u32 = (value) => Number(BigInt.asUintN(32, value));
const f = (value) => bitsToFloat(u32(value));

// 64비트 패턴과 double 의 변환이다. (2026-11-08 WB04-E130)
function bitsToDouble(bits) {
    const view = new DataView(new ArrayBuffer(8));
    view.setBigUint64(0, BigInt.asUintN(64, BigInt(bits)), true);
    return view.getFloat64(0, true);
}
function doubleToBits(value) {
    const view = new DataView(new ArrayBuffer(8));
    view.setFloat64(0, value, true);
    return view.getBigUint64(0, true).toString();
}

// 바이트 배열을 소문자 16진수로 만든다. (2026-11-08 WB04-E130)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 입력 순서와 빈 열을 보존해 잇는다. 비면 실행기와 같은 '-' 이다. (2026-11-08 WB04-E130)
const join = (values, separator) => (values.length ? values.join(separator) : '-');

// 예외가 나는지 본다. (2026-11-08 WB04-E130)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 실제 풀/시각/입자/팔레트와 원본과 같은 입력 경계를 소유하는 장면이다. (2026-11-08 WB04-E130)
class FrameScene {
    constructor() {
        this.pool = new SidPool(32768, { server: false });
        this.random = new GameRandom(0);
        this.visualState = createConstructionVisualState();
        this.state = createConstructionFrameState();
        this.colors = [];
        this.palette = { token: PALETTE, count: 0, entries: this.colors };
        this.input = [];
        this.events = [];
        this.dirtyCount = 0;
        this.frame = new RawConstructionFrame(this.pool, this.visualState, this.state, this.palette, this.random, this.hooks());
    }

    // 최대 HP·dirty 큐·장치·Kill 의 명시 입력/관찰 경계이다.
    hooks() {
        return {
            // 원본과 같은 경계 중 HP/좌표/시각 변경이다.
            maxHitPoints: (parent) => {
                assert.equal(parent, PARENT);
                this.events.push('H:50');
                const raw = this.pool.physical(PARENT);
                if (this.input[MUTATE] === 1n) put(raw, 26, 1);
                if (this.input[MUTATE] === 2n) put(raw, 26, 100);
                if (this.input[MUTATE] === 3n) { put(raw, 14, floatToBits(-7.75)); this.state.now = 30; }
                return Number(BigInt.asIntN(32, this.input[MAXIMUM]));
            },
            // 이전/새 사각형 사이에 카메라/부모 abstract 가 바뀔 수 있다.
            invalidate: (rectangle) => {
                this.events.push(`I:${rectangle.left}:${rectangle.top}:${rectangle.right}:${rectangle.bottom}`);
                if (!this.dirtyCount && this.input[MUTATE] === 4n) this.state.cameraX = 100.25;
                if (!this.dirtyCount && this.input[MUTATE] === 5n) this.pool.physical(PARENT)[40] = 0;
                this.dirtyCount += 1;
            },
            // 현재 출력 버퍼는 호출 중에만 검사하고 보관하지 않는다.
            draw: (request) => {
                assert.ok(request.pixels.length === 64 && request.pitch === 8 && request.clip.left === 0 && request.clip.top === 0 && request.clip.right === 8 && request.clip.bottom === 8);
                this.events.push(`D:${request.x}:${request.y}:${request.pitch}:0:0:8:8`);
            },
            // Kill 의 실제 삭제/해제는 외부 소유자에게 연결할 다음 경계이다.
            kill: (flags) => { assert.ok(!flags); this.events.push('K:0'); },
        };
    }

    // 기대 프레임 결과 대신 raw/전역/팔레트 입력만 준비한다.
    prepare(text) {
        this.input = text.split(',').map(BigInt);
        this.events = [];
        this.dirtyCount = 0;
        assert.equal(this.input.length, INPUT_COUNT);
        const raw = this.pool.physical(PARENT);
        raw.fill(0);
        put(raw, 0, VTABLE);
        raw[10] = 100;
        raw[11] = Number(this.input[FREE]);
        raw[40] = 1;
        put(raw, 14, u32(this.input[X]));
        put(raw, 18, u32(this.input[Y]));
        put(raw, 26, u32(this.input[HP]));
        this.random.setState(u32(this.input[RANDOM]));
        this.visualState.scaleX = f(this.input[SCALE_X]);
        this.visualState.scaleY = f(this.input[SCALE_Y]);
        Object.assign(this.state, { now: bitsToDouble(this.input[NOW]), rate: f(this.input[RATE]), cameraX: f(this.input[CAMERA_X]), cameraY: f(this.input[CAMERA_Y]) });
        // 색 수와 인접 DWORD 를 구분하기 위한 고유 DWORD 입력이다.
        this.colors.length = 0;
        for (let index = 0; index <= Number(this.input[COLORS]); index += 1) this.colors.push((0x81000000 + index * 257 + 7) >>> 0);
        this.palette.token = PALETTE;
        this.palette.count = Number(BigInt.asIntN(32, this.input[COLORS]));
        this.palette.entries = this.colors;
    }

    // 실행기의 8×8 소스 입력과 이미 선택된 세 입자를 공급한다.
    initial() {
        const fields = createConstructionFrameFields();
        const v = fields.visual;
        v.parent = Number(this.input[PARENT_COLUMN] & 0xffffn);
        v.instant = this.input[INSTANT] !== 0n;
        v.left = Math.fround(1.1);
        v.top = Math.fround(-2.1);
        v.right = Math.fround(3.1);
        v.bottom = Math.fround(4.1);
        v.lastTime = bitsToDouble(this.input[LAST]);
        v.elapsed = bitsToDouble(this.input[ELAPSED]);
        fields.delta = 13;
        fields.scaledDelta = 17;
        v.palette = PALETTE;
        const pixels = new Uint8Array(64);
        // 아직 선택하지 않은 픽셀의 입력 패턴만 만든다.
        for (let index = 0; index < 64; index += 1) pixels[index] = this.input[PATTERN] === 2n && index % 3 === 0 ? 255 : (index * 13 + 17) % 255;
        v.image = { width: 8, height: 8, hotspotX: -3, hotspotY: 2, pixels };
        v.output = new Uint8Array(64).fill(255);
        v.lastRow = 56;
        v.minimum = v.instant ? 2 : bitsToFloat(0x3fa66666);
        v.maximum = f(this.input[MAX_LIFE]);
        v.selected = u32(this.input[SELECTED]);
        v.opaque = 64;
        v.firstDraw = this.input[FIRST] !== 0n;
        v.active = u32(this.input[ACTIVE]);
        v.capacity = u32(this.input[CAPACITY]);
        v.total = u32(this.input[TOTAL]);
        v.particles = Array.from({ length: v.capacity }, () => ({ active: 0, color: 0, life: 0, pixel: 0 }));
        // 이미 선택된 입자 위치/수명/색은 입력이며 알고리즘으로 계산하지 않는다.
        for (let index = 0; index < v.total; index += 1) {
            const pixel = 27 + index * 8;
            v.particles[index] = { active: index < v.active ? 1 : 0, color: pixels[pixel], life: Math.fround(f(this.input[LIFE]) * (index + 1)), pixel };
            pixels[pixel] = 255;
        }
        if (!this.input[SOURCE]) v.image.pixels = new Uint8Array(0);
        return fields;
    }
}

// 실행기의 전체 필드/입자/두 버퍼/난수 직렬화 순서이다. 기대 갱신 결과는 재계산하지 않는다. (2026-11-08 WB04-E130)
function snapshot(fields, random) {
    const v = fields.visual;
    const values = [floatToBits(v.left), floatToBits(v.top), floatToBits(v.right), floatToBits(v.bottom), floatToBits(fields.delta), floatToBits(fields.scaledDelta),
        v.selected, v.opaque, v.firstDraw ? 1 : 0, v.active, v.capacity, v.total].map(String);
    values.push(doubleToBits(v.lastTime), doubleToBits(v.elapsed));
    // 미사용 칸의 잔여 값도 원본의 두 번 복사한 순서와 비교한다.
    const particles = v.particles.map((particle) => `${particle.active}:${particle.color}:${floatToBits(particle.life)}:${particle.pixel}`);
    return [join(values, ':'), hex(v.image.pixels), hex(v.output), join(particles, ';'), String(random.state)];
}

test('ConstructionFrame: 원본 10.78 Run/Expand/Append/Color/Clock/Draw/Sequence 435행의 외부 요청·필드·픽셀·출력·입자·난수·반환값·연속 기록이 같다', { skip: rows.length === 0 && '원본 fixture 없음' }, () => {
    const scene = new FrameScene();
    let count = 0;
    // 입력만 구성하며 기대 필드/입자/출력은 저장 PE 관찰에서 읽는다.
    for (const row of rows) {
        assert.equal(row.length, 11);
        scene.prepare(row[2]);
        const fields = scene.initial();
        let result = 0;
        const history = [];
        if (row[1] === 'Run') scene.frame.run(fields);
        else if (row[1] === 'Expand') scene.frame.expand(fields, u32(scene.input[PIXEL]));
        else if (row[1] === 'Append') { if (scene.frame.append(fields, Number(BigInt.asIntN(32, scene.input[PIXEL])))) result = fields.visual.total; }
        else if (row[1] === 'Color') result = scene.frame.color(f(scene.input[LIFE]), f(scene.input[MAX_LIFE]));
        else if (row[1] === 'Clock') scene.frame.clock(fields);
        else if (row[1] === 'Draw') scene.frame.draw(fields);
        else {
            // 원본과 같은 외부 HP/시각 입력으로 모든 중간 상태를 검사한다.
            for (let step = 0; step < 5; step += 1) {
                scene.state.now = step * 0.0625;
                put(scene.pool.physical(PARENT), 26, step * 25);
                if (step === 4) scene.pool.physical(PARENT)[40] = 0;
                scene.frame.run(fields);
                scene.frame.draw(fields);
                history.push(join(snapshot(fields, scene.random), '|'));
            }
        }
        const observed = snapshot(fields, scene.random);
        // source=null 의 조기 반환에서는 입력 메모리의 원래 이미지가 따로 남는다.
        if (!scene.input[SOURCE]) {
            const saved = scene.input;
            scene.input = [...saved];
            scene.input[SOURCE] = 1n;
            observed[1] = hex(scene.initial().visual.image.pixels);
            scene.input = saved;
        }
        assert.deepEqual([join(scene.events, ';'), ...observed, result, join(history, '~')], [row[3], ...row.slice(4, 9), Number(row[9]), row[10]], `건설 프레임 행 ${count} ${row[1]}`);
        count += 1;
    }
    assert.equal(count, PATCH_ROWS);
});

// 계약 검사에서 쓰는 유한 입력이다. 결과 계산은 포함하지 않는다. (2026-11-08 WB04-E130)
const defaultInput = () => [0, 0, 17, 12, 3, 0, 3, 50, 101, 1, doubleToBits(0.025), 0, doubleToBits(1000000.125), floatToBits(0.7), floatToBits(20.1), floatToBits(-3.2),
    floatToBits(16), floatToBits(11), floatToBits(0.75), floatToBits(-0.25), 27, floatToBits(0.25), 11, 0, 0, 50, 1, floatToBits(2)].join(',');

test('ConstructionFrame: 안전하지 않은 저장소/입자/색 표 입력을 쓰기 전에 거부한다', () => {
    const scene = new FrameScene();
    scene.prepare(defaultInput());
    let fields = scene.initial();
    const source = hex(fields.visual.image.pixels);
    fields.visual.lastRow = 63;
    assert.ok(throws(() => scene.frame.run(fields)) && hex(fields.visual.image.pixels) === source && scene.events.length === 0);
    fields = scene.initial();
    fields.visual.output = fields.visual.output.slice(0, -1);
    assert.ok(throws(() => scene.frame.run(fields)));
    fields = scene.initial();
    fields.visual.particles[0].pixel = 64;
    assert.ok(throws(() => scene.frame.expand(fields, 27)));
    fields = scene.initial();
    assert.ok(throws(() => scene.frame.append(fields, 64)) && fields.visual.total === 3 && scene.random.state === 17);
    fields.visual.parent = 0;
    assert.ok(throws(() => scene.frame.run(fields)));
    scene.palette.entries = scene.colors.slice(0, 11);
    assert.ok(throws(() => scene.frame.color(2, 2)));
    assert.ok(throws(() => scene.frame.color(1, 0)));
});

test('ConstructionFrame: 종료/장치 경계는 누락할 수 없고 용량 0 Draw 는 부모/장치에 접근하지 않는다', () => {
    const scene = new FrameScene();
    scene.prepare(defaultInput());
    const hooks = scene.hooks();
    delete hooks.kill;
    assert.ok(throws(() => new RawConstructionFrame(scene.pool, scene.visualState, scene.state, scene.palette, scene.random, hooks)));
    const fields = createConstructionFrameFields();
    scene.frame.draw(fields);
    assert.equal(scene.events.length, 0);
    fields.visual.capacity = 1;
    fields.visual.total = 2;
    assert.ok(throws(() => scene.frame.draw(fields)));
});

test('ConstructionFrame: 실제 표시 생성의 소유 상태를 연속 Run/Draw 에 넘기고 원래 픽셀 복구와 abstract 종료 요청을 확인한다', () => {
    const scene = new FrameScene();
    scene.prepare(defaultInput());
    const raw = scene.pool.physical(PARENT);
    raw[34] = 1;
    scene.visualState.palette = PALETTE;
    const image = scene.initial().visual.image;
    // 실제 SHP 처럼 투명 테두리가 있는 선택 전 캡처 입력을 공급한다.
    for (let index = 0; index < image.pixels.length; index += 1) {
        const row = Math.floor(index / 8);
        const column = index % 8;
        image.pixels[index] = row === 0 || row === 7 || column === 0 || column === 7 ? 255 : (index * 13 + 17) % 255;
    }
    const hooks = {
        reserve: () => true,
        attach: () => {},
        createPalette: () => PALETTE,
        gradient: () => {},
        buildPalette: () => {},
        capture: () => image,
        releaseParticles: () => {},
    };
    const visual = new RawConstructionVisual(scene.pool, scene.visualState, scene.random, hooks);
    // opaque=36, 최대 HP=21 에서는 원본 목표의 정수 절단 후에도 모든 선택 입자가 활성화된다.
    scene.input[MAXIMUM] = 21n;
    // 첫 프레임 초기화 뒤 HP 100% 에서 모든 선택 입자를 충분한 시간에 걸쳐 만료시킨다.
    for (const instant of [false, true]) {
        put(raw, 26, 0);
        const fields = createConstructionFrameFields();
        fields.visual = visual.create(PARENT, instant);
        scene.events = [];
        scene.dirtyCount = 0;
        scene.state.now = 0;
        const opaque = fields.visual.opaque;
        assert.ok(fields.visual.total > 0 && fields.visual.capacity === 500);
        scene.frame.run(fields);
        scene.frame.draw(fields);
        assert.ok(fields.visual.firstDraw && fields.delta === 0 && scene.events.length > 0);
        put(raw, 26, 21);
        // 앞 프레임의 확장이 다음 프레임에도 계속될 수 있으므로 실제 상태로 여러 번 갱신한다.
        for (let step = 1; step <= 20; step += 1) {
            scene.state.now = step;
            scene.frame.run(fields);
        }
        assert.ok(fields.visual.total === 0 && fields.visual.active === 0 && fields.visual.selected === opaque);
        assert.equal(hex(fields.visual.output), hex(image.pixels));
        const count = scene.events.length;
        scene.frame.draw(fields);
        assert.equal(scene.events.length, count + 1);
        raw[40] = 0;
        scene.frame.run(fields);
        assert.equal(scene.events.at(-1), 'K:0');
        raw[40] = 1;
    }
});
