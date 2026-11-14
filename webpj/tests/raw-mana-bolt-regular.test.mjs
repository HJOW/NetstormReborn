// 마나 볼트 주기(45a8e0)의 프레임/좌표/소리/가상 공간 요청과 반환을 원본 10.78 독립 관측 196행(두 정밀도)에 대조한다. 원본게임을 실행하지 않는다. (2026-11-15 WB04-E468)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { ManaBoltArrivalState, RawManaBoltArrival } from '../www/core/raw-mana-bolt-arrival.js';
import { ManaBoltRegularState, RawManaBoltRegular } from '../www/core/raw-mana-bolt-regular.js';
import { CameraView, projectCameraCoordinates } from '../www/render/camera-view.js';

// 단정도 비트와 비정렬 raw 쓰기이다.
const bits = (value) => { const view = new DataView(new ArrayBuffer(4)); view.setFloat32(0, value, true); return view.getUint32(0, true); };
function store(raw, offset, value, width = 4) { for (let index = 0; index < width; index += 1) raw[offset + index] = Math.floor(value / 2 ** (8 * index)) & 0xff; }
const get = (raw, offset, width = 4) => { let value = 0; for (let index = 0; index < width; index += 1) value += raw[offset + index] * 2 ** (8 * index); return value; };

// 원본 독립 입력을 해시/TYPE/SHP/현재 카메라에 배치하는 장면이다. 같은 풀/해시/도착 검색과 현재 카메라 원점·월드 비트를 가진다.
class BoltRegularScene {
    constructor() {
        this.pool = new SidPool(32768, { server: true }); this.hash = new SquidHash(); this.types = Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0 }));
        this.frames = Array.from({ length: 188 }, () => new FrameCodeTable([])); this.shapes = Array.from({ length: 188 }, () => ({ loaded: false, frameCount: 0, frames: [] }));
        this.arrival = new RawManaBoltArrival(this.pool, this.hash, this.types, new ManaBoltArrivalState(), {}); this.state = new ManaBoltRegularState(); this.camera = [0, 0]; this.origin = [0, 0]; this.events = [];
        for (let sid = 5; sid <= 60; sid += 1) assert.equal(this.pool.allocate(2), sid); // 마지막 후보 60 까지 같은 실제 client free list 에서 확보한다.
        this.frames[105] = new FrameCodeTable([{ side: 65, variant: 80, number: 1, flags: 0 }, { side: 65, variant: 80, number: 2, flags: 0 }, { side: 66, variant: 80, number: 1, flags: 0 }, { side: 66, variant: 80, number: 2, flags: 0 }]);
        this.types[105].maxHitPoints = 100; this.types[100].footX = 1; this.types[100].footY = 1; this.shapes[105] = { loaded: true, frameCount: 4, frames: Array.from({ length: 4 }, () => ({ width: 0, hotspotX: 0 })) };
    }
    // 원본 순서를 구분자 하나로 연결한다.
    event(value) { this.events.push(value); }
    // 한 행의 raw 를 초기화한다. 카메라 결과는 이 구현에서 재계산하지 않고 실제 497490 관측을 사용한다.
    setup(input, observedCamera) {
        this.hash.reset(); this.events = []; const bolt = this.pool.allocated(50); const target = this.pool.allocated(60); bolt.fill(0); target.fill(0);
        store(bolt, 0, 0x509810); store(bolt, 10, 105, 1); store(bolt, 12, input[3], 2); store(bolt, 14, bits(20.75)); store(bolt, 18, bits(Math.fround(21.9))); store(bolt, 34, 1, 1); store(bolt, 36, input[2]);
        if (input[4]) { store(target, 10, 100, 1); store(target, 40, input[5], 1); store(target, 34, input[6], 1); store(target, 14, bits(20.5)); store(target, 18, bits(21.5)); this.hash.setBucket(1, 20.5, 21.5, 60); }
        const profile = input[7]; this.origin = [[0, 0], [319, 127], [1920, 1080], [2147483600, -2147483600]][profile]; const header = [[32, 5], [1, -7], [32767, 32767], [-16, -8]][profile];
        this.shapes[105].frames[0].width = header[0]; this.shapes[105].frames[0].hotspotX = header[1];
        this.camera = observedCamera.split(',').map(Number); const view = new CameraView(); view.setPixels(this.origin[0], this.origin[1]); assert.deepEqual([view.data.getUint32(0, true), view.data.getUint32(4, true)], this.camera);
        this.view = view;
    }
    // 실제 Frame/공간은 독립 원본과 같은 외부 경계다. 표 계산과 현재 검색은 모듈 자체가 실행한다.
    hooks() {
        return {
            setFrame: (sid, frame, flags) => { this.event(`F:${frame >>> 0}:${flags}`); store(this.pool.allocated(sid), 36, frame >>> 0); },
            sound: (x, y, name) => { assert.equal(name, 'influenceIcon.wav'); this.event(`S:${bits(x)}:${bits(y)}`); },
            project: (x, y) => projectCameraCoordinates(this.origin[0], this.origin[1], x, y),
            inverse: (x, y) => { const point = this.view.screenToWorld(x, y); return [point.x, point.y]; },
            unpop: (_sid, flags) => { this.event(`U:${flags}`); },
            pop: (sid, x, y, flags) => { this.event(`P:${bits(x)}:${bits(y)}:${flags}`); const raw = this.pool.allocated(sid); store(raw, 14, bits(x)); store(raw, 18, bits(y)); },
            destroy: (_sid, flags) => { this.event(`K:${flags}`); },
        };
    }
}

test('ManaBoltRegular 원본 196행: 반환 간격/삭제·프레임·위치 float·효과 순서가 두 정밀도에서 모두 일치한다', () => {
    const table = fs.readFileSync(new URL('../../cpppj/tests/fixtures/mana-bolt-regular-x86.tsv', import.meta.url), 'utf8').split(/\r?\n/).filter((line) => line && !line.startsWith('#')).map((line) => line.split('\t'));
    assert.equal(table.length, 196); const scene = new BoltRegularScene(); const regular = new RawManaBoltRegular(scene.pool, scene.types, scene.frames, scene.shapes, scene.arrival, scene.state, scene.hooks());
    for (const [index, row] of table.entries()) {
        assert.equal(row.length, 7, `행 ${index}`); const input = row[0].split(',').map(Number); scene.setup(input, row[6]); assert.ok(regular.handles(50));
        assert.equal(bits(regular.run(50, input[0], input[1])), Number(row[1]), `행 ${index} 반환`); const raw = scene.pool.slot(50);
        assert.deepEqual([get(raw, 36), get(raw, 14), get(raw, 18), scene.events.join(';')], [Number(row[2]), Number(row[3]), Number(row[4]), row[5]], `행 ${index} ${row[0]}`);
    }
});

test('ManaBoltRegular 생성/분배 계약: 판본·TYPE 표 크기·같은 월드 검사, 가상 표만 분배, 선택 장치 미연결 거부, frameZero 장치는 shapes 검사를 대신한다', () => {
    const s = new BoltRegularScene(); const make = (overrides = {}) => new RawManaBoltRegular(overrides.pool ?? s.pool, overrides.types ?? s.types, overrides.frames ?? s.frames, overrides.shapes ?? s.shapes, overrides.arrival ?? s.arrival, s.state, s.hooks());
    assert.doesNotThrow(() => make()); assert.throws(() => make({ types: s.types.slice(1) }), TypeError); assert.throws(() => make({ frames: s.frames.slice(1) }), TypeError); assert.throws(() => make({ shapes: s.shapes.slice(1) }), TypeError);
    const other = new SidPool(32768, { server: true }); assert.throws(() => make({ pool: other }), TypeError); assert.equal(make().pool, s.pool);
    assert.deepEqual({ ...new ManaBoltRegularState() }, { interval: Math.fround(0.05), spacing: 16 });
    // 가상 표만 보고 TYPE 은 선검사하지 않으며 다른 표는 거부한다.
    s.setup([0, 0, 0, 0, 1, 1, 1, 0], '0,0'); const regular = make(); s.pool.allocated(50)[10] = 7; assert.equal(regular.handles(50), true); store(s.pool.allocated(50), 0, 0x509811); assert.equal(regular.handles(50), false); assert.throws(() => regular.run(50, 0, 0), /다른 마나 볼트/);
    // 선택된 장치 미연결은 거부하고 도달하지 않은 장치는 요구하지 않는다.
    s.setup([2, 0, 0, 0, 1, 1, 1, 0], '0,0'); const bare = new RawManaBoltRegular(s.pool, s.types, s.frames, s.shapes, s.arrival, s.state, { setFrame: () => {} }); assert.equal(bare.run(50, 2, 5), Math.fround(0.05)); assert.throws(() => new RawManaBoltRegular(s.pool, s.types, s.frames, s.shapes, s.arrival, s.state, {}).run(50, 2, 5), /효과 미연결/);
    // frameZero 가 있으면 shapes 를 읽지 않는다. 없으면 미적재 SHP 를 거부한다.
    s.setup([1, 3, 0, 0, 1, 1, 1, 0], '0,0'); const calls = []; const hooks = { ...s.hooks(), frameZero: (type) => { calls.push(type); return { width: 4, hotspotX: 2 }; } }; s.shapes[105].loaded = false;
    const direct = new RawManaBoltRegular(s.pool, s.types, s.frames, s.shapes, s.arrival, s.state, hooks); assert.equal(direct.run(50, 1, 3), Math.fround(0.05)); assert.deepEqual(calls, [105]); assert.throws(() => make().run(50, 1, 3), /SHP 오류/);
    s.shapes[105].loaded = true; s.types[105].maxHitPoints = -0x22222223; assert.throws(() => make().run(50, 1, 3), /SHP 오류/); s.types[105].maxHitPoints = -0x32323233; assert.throws(() => make().run(50, 1, 3), /SHP 오류/);
});

// 관측 fixture 가 닿지 않는 경계(raw 필드 폭·글자 구간 경계·프레임 번호 홀수·대상 extra 0·SHP 검사 항목별·생성 검사 항목별)를 독립 입력으로 고정한다. (2026-11-15 WB04-E468)
test('ManaBoltRegular raw 읽기 폭: 가상 표 DWORD 와 WORD 필드는 인접 바이트를 섞지 않는다', () => {
    const s = new BoltRegularScene(); s.setup([1, 0, 0, 3, 1, 1, 1, 0], '0,0'); const regular = new RawManaBoltRegular(s.pool, s.types, s.frames, s.shapes, s.arrival, s.state, s.hooks());
    s.pool.allocated(50)[4] = 7; assert.equal(regular.handles(50), true); // 가상 표 뒤 바이트(+4)는 표 비교에 들어가지 않는다.
    // WORD +12(값 3)가 위치 Y 에 (WORD+1)×간격 으로 쓰이고 +14 의 float 낮은 바이트가 섞이지 않는다.
    store(s.pool.allocated(50), 14, bits(Math.fround(20.123456))); let inverse; const hooks = { ...s.hooks(), inverse: (x, y) => { inverse = [x, y]; return [0, 0]; }, project: () => [100, 1000] };
    new RawManaBoltRegular(s.pool, s.types, s.frames, s.shapes, s.arrival, s.state, hooks).run(50, 1, 0); assert.deepEqual(inverse, [100 - (32 + 5) - 16, 1000 - (3 + 1) * 16]);
});

test('ManaBoltRegular 프레임 구간: 프레임 번호의 낮은 비트와 글자 구간 경계 A..P 를 그대로 쓰고 범위 밖은 거부한다', () => {
    const s = new BoltRegularScene(); const code = (side) => ({ side, variant: 80, number: 1, flags: 0 });
    s.frames[105] = new FrameCodeTable([code(65), code(66), code(66), code(80)]); s.setup([0, 5, 0, 0, 1, 1, 1, 0], '0,0'); const regular = new RawManaBoltRegular(s.pool, s.types, s.frames, s.shapes, s.arrival, s.state, s.hooks());
    // 프레임 0 은 'A' 구간(첫 0, 1칸): setFrame 은 5 % 1 + 0 = 0. 프레임 1 은 'B' 구간(첫 1, 2칸): 5 % 2 + 1 = 2.
    regular.run(50, 2, 5); assert.equal(s.events.at(-1), 'F:0:0'); store(s.pool.allocated(50), 36, 1); regular.run(50, 2, 5); assert.equal(s.events.at(-1), 'F:2:0');
    // 마지막 글자 'P'(direction 15)는 허용하고 'Q'(16)·'@'(-1)는 구간 범위 오류다.
    store(s.pool.allocated(50), 36, 3); assert.doesNotThrow(() => regular.run(50, 2, 1));
    for (const side of [0x51, 0x40]) { s.frames[105] = new FrameCodeTable([code(side)]); store(s.pool.allocated(50), 36, 0); assert.throws(() => new RawManaBoltRegular(s.pool, s.types, s.frames, s.shapes, s.arrival, s.state, s.hooks()).run(50, 2, 0), RangeError, String(side)); }
    // count 는 signed 나머지다(음수 count 는 음수 나머지에 구간 첫 번호를 더한다).
    s.frames[105] = new FrameCodeTable([code(65), code(65), code(65)]); store(s.pool.allocated(50), 36, 0); const negative = new RawManaBoltRegular(s.pool, s.types, s.frames, s.shapes, s.arrival, s.state, s.hooks()); negative.run(50, 2, 0xfffffffd); assert.equal(s.events.at(-1), `F:${(-3 % 3) >>> 0}:0`); negative.run(50, 2, 0xffffffff); assert.equal(s.events.at(-1), `F:${(-1 % 3) >>> 0}:0`);
});

test('ManaBoltRegular 대상 이동: 대상이 있고 SHP 헤더가 유효해야 이동하며 SHP 검사는 항목별로 거부한다', () => {
    const s = new BoltRegularScene(); const base = s.shapes[105]; const run = () => new RawManaBoltRegular(s.pool, s.types, s.frames, s.shapes, s.arrival, s.state, s.hooks()).run(50, 1, 0);
    s.setup([1, 0, 0, 0, 1, 1, 1, 0], '0,0'); assert.equal(run(), Math.fround(0.05)); assert.ok(!s.events.includes('K:0'));
    s.setup([1, 0, 0, 0, 1, 1, 1, 0], '0,0'); s.shapes[105] = { ...base, frameCount: 1, frames: [base.frames[0]] }; assert.equal(run(), Math.fround(0.05)); s.shapes[105] = base; // frame0 하나만 있는 SHP 도 유효하다.
    const cases = { 미적재: { ...base, loaded: false }, 'frameCount 0': { ...base, frameCount: 0 }, 'frames 빔': { ...base, frames: [] }, 'frameCount 음수': { ...base, frameCount: -1 } };
    for (const [label, shape] of Object.entries(cases)) { s.setup([1, 0, 0, 0, 1, 1, 1, 0], '0,0'); s.shapes[105] = shape; assert.throws(run, /SHP 오류/, label); s.shapes[105] = base; }
    s.setup([1, 0, 0, 0, 0, 0, 0, 0], '0,0'); assert.equal(run(), -1); assert.ok(s.events.includes('K:0')); // 대상이 없으면 자기 삭제 뒤 -1 이다.
});

test('ManaBoltRegular 생성 검사: 판본/TYPE 표/프레임 표/SHP 표/같은 월드 조건을 각각 독립으로 거부한다', () => {
    const s = new BoltRegularScene(); const make = (pool, types, frames, shapes, arrival) => new RawManaBoltRegular(pool, types, frames, shapes, arrival, s.state, {}); const fake = { edition: 'cd1072' };
    // 각 조건 하나만 어긋난 입력이다.
    assert.throws(() => make(fake, s.types, s.frames, s.shapes, { pool: fake }), TypeError, '판본'); assert.throws(() => make(s.pool, s.types.slice(1), s.frames.slice(1), s.shapes.slice(1), s.arrival), TypeError, 'types 만 187');
    assert.throws(() => make(s.pool, s.types, s.frames.slice(1), s.shapes, s.arrival), TypeError, 'frames 만'); assert.throws(() => make(s.pool, s.types, s.frames, s.shapes.slice(1), s.arrival), TypeError, 'shapes 만');
    assert.throws(() => make(s.pool, s.types, s.frames, s.shapes, { pool: {} }), TypeError, '같은 월드');
});
