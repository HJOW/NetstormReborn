// 프레임 진행 raw-frame-advance.js(RawFrameAdvance)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: frameadvance-x86.tsv 10.78(originals) 7680행(코드표 종류·타입 종류·해시 단계·현재 프레임·signed 증분·flags → 보정 여부, 실제 SquidFrame 의 표시/Unpop/Pop 호출 기록, 슬롯 50바이트).
//    CD 10.72·10.37 행은 웹 범위(10.78) 밖이다.
//  - 계약: FrameAdvance_ComposesRealDisplayAndSpace(실제 SquidDisplay/Pop/Unpop 으로 한 호출 늦은 해시 이동과 반환), 풀/지정기 연결 오류 거부.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E65: 프레임 진행 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidPop, RAW_POP_RESULT } from '../www/core/squid-pop.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { SquidFactory } from '../www/core/squid-factory.js';
import { SquidDisplay } from '../www/core/squid-display.js';
import { SquidFrame } from '../www/core/squid-frame.js';
import { makeSquidFrameHooks } from '../www/core/squid-frame-binding.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { RawFrameAdvance } from '../www/core/raw-frame-advance.js';

// 원본 관측 10.78 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E65)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'frameadvance-x86.tsv'))?.filter((row) => row[0] === 'originals') ?? null;

// 원본 실행기가 입력한 타입/가상 표/SID, 판본당 독립 행 수, 여덟 SHP 추가 헤더의 칸 크기, flags1/flags2/프레임 수 종류이다. (2026-11-08 WB04-E65)
const TYPE = 82;
const VTABLE = 0x11010000;
const OBJECT = 50;
const ROWS = 7680;
const SIZES = [[1, 1], [2, 2], [2.5, 1], [3, 4], [4, 4.5], [0.5, 9], [2, 1.5], [4, 4]];
const KINDS = [[0x802, 4, 8], [0x28000803, 2, 8], [0x28000003, 0x1000000, 8], [0x69012, 0x210000, 4]];

// 예외가 나는지 본다. (2026-11-08 WB04-E65)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E65)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 프레임 코드의 동일한 입력을 만든다. 구간 첫 번호/개수/진행 결과는 생산 모듈이 계산한다(number/variant 가 달라도 진행은 side 의 구간을 쓴다). (2026-11-08 WB04-E65)
const codes = (letters) => new FrameCodeTable([...letters].map((letter, index) => ({ side: letter.charCodeAt(0), variant: 80 + (index % 2), flags: index & 1 })));

// 188칸 타입 표이다. (2026-11-08 WB04-E65)
const frameTypes = () => Array.from({ length: 188 }, () => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0 }));

test('FrameAdvance: 원본 10.78 7680행의 보정 여부·표시/Unpop/Pop 호출 기록·슬롯 50바이트가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    const pool = new SidPool(24000, { server: false });
    // 원본 SID 50 이 실제 풀의 활성 할당 범위에 들도록 확보한다.
    for (let sid = 5; sid <= OBJECT; sid += 1) assert.equal(pool.allocate(2), sid);
    const types = frameTypes();
    const frames = types.map(() => new FrameCodeTable([]));
    let count = 0;
    // 저장된 기대값을 다시 계산하지 않고 입력을 생산 코드에 넘긴다.
    for (const row of rows) {
        assert.equal(row.length, 10);
        const kind = KINDS[Number(row[2])];
        types[TYPE].flags1 = kind[0];
        types[TYPE].flags2 = kind[1];
        frames[TYPE] = codes(Number(row[1]) === 0 ? 'AAABBJJJ' : 'JAJBAPJP');
        const raw = pool.allocated(OBJECT);
        raw.fill(0);
        put(raw, 0, VTABLE);
        raw[10] = TYPE;
        put(raw, 14, floatToBits(20.75));
        put(raw, 18, floatToBits(21.9));
        put(raw, 36, Number(row[4]) >>> 0);
        raw[33] = Number(row[3]);
        const events = [];
        // 모든 외부 효과에 호출 당시의 프레임을 붙여 지정 시점과 순서를 보존한다.
        const current = () => String(get(pool.slot(OBJECT), 36, 4));
        const setter = new SquidFrame(pool, types, {
            // 패치 헤더 helper 의 현재 프레임 범위만 검사한다. 새 목표 프레임은 원본도 검사하지 않는다.
            frameSize: (sid, frame) => {
                assert.ok(frame >= 0 && frame < kind[2] * ((kind[0] & 0x440000) ? 2 : 1), `행 ${count}`);
                return SIZES[frame];
            },
            update: (sid, flags) => events.push(`${flags ? 'E' : 'D'}:${sid}:${current()}`),
            unpop: (sid, flags) => events.push(`U:${sid}:${flags}:${current()}`),
            pop: (sid, x, y, flags) => events.push(`P:${sid}:${floatToBits(x)}:${floatToBits(y)}:${flags}:${current()}`)
        });
        const result = new RawFrameAdvance(pool, frames, setter).advance(OBJECT, Number(row[5]) | 0, Number(row[6]));
        assert.equal(Number(result), Number(row[7]), `행 ${count}`);
        assert.equal(events.length === 0 ? '-' : events.join(';'), row[8], `행 ${count}`);
        assert.equal(hex(pool.slot(OBJECT)), row[9], `행 ${count}`);
        count += 1;
    }
    assert.equal(count, ROWS);
});

test('FrameAdvance: 실제 공통 Pop/Unpop/표시로 한 호출 늦은 해시 이동과 반환을 확인한다', () => {
    const pool = new SidPool(24000, { server: true });
    const types = frameTypes();
    // 원본에서 일반 Pop/postPop 경로가 확인된 연결 타입이다.
    const type = 155;
    types[type].constructorAddress = typeConstructorAddress(type);
    types[type].flags1 = 0x28000002;
    types[type].flags2 = 0x2000000;
    types[type].footX = 1;
    types[type].footY = 1;
    const frames = types.map(() => new FrameCodeTable([]));
    frames[type] = codes('AAAA');
    const shapes = types.map(() => ({ frameCount: 0, loaded: false, frames: [] }));
    const frame = (width, height, hotspotX, hotspotY, cellWidth, cellHeight) => ({ width, height, hotspotX, hotspotY, cellWidth, cellHeight });
    shapes[type] = { frameCount: 4, loaded: true, frames: [frame(11, 17, 12, 16, 1, 1), frame(26, 10, 5, 2, 2, 1.5), frame(8, 4, 2, 1, 5, 5), frame(6, 3, 1, 0, 6, 6)] };
    const seen = { frames: [], rects: [], object: 0 };
    const sink = {
        suppressed: () => false,
        invalidate: (rect) => { seen.frames.push(get(pool.slot(seen.object), 36, 4)); seen.rects.push({ ...rect }); }
    };
    const display = new SquidDisplay(types, shapes, sink, { cameraX: 0, cameraY: 0, zoom: 65536, viewport: { left: 0, top: 0, right: 640, bottom: 480 } });
    const hash = new SquidHash();
    const spots = new Uint8Array(65536);
    const unpop = new SquidUnpop(pool, hash, spots, display);
    const pop = new SquidPop(pool, hash, spots, { display });
    pop.bindTransmission(() => {});
    const factory = new SquidFactory(pool, types, { unpop });
    seen.object = factory.create(type);
    assert.equal(pop.pop(seen.object, types[type], 1, 1, 20, 20), RAW_POP_RESULT.registered);
    const setter = new SquidFrame(pool, types, makeSquidFrameHooks(pool, types, display, unpop, pop));
    const advance = new RawFrameAdvance(pool, frames, setter);
    const clear = () => { seen.frames = []; seen.rects = []; };
    clear();
    assert.ok(!advance.advance(seen.object, 1));
    assert.deepEqual(seen.frames, [0, 1]);
    assert.ok(seen.rects.length === 2 && seen.rects[0].left !== seen.rects[1].left);
    clear();
    assert.ok(!advance.advance(seen.object, 1));
    assert.deepEqual(seen.frames, [1, 2]);
    assert.ok(hash.bucket(1, 20, 20) === seen.object && pool.slot(seen.object)[33] === 1);
    clear();
    assert.ok(!advance.advance(seen.object, 1));
    assert.deepEqual(seen.frames, [2, 3]);
    assert.ok(hash.bucket(1, 20, 20) === 0 && hash.bucket(3, 20, 20) === seen.object && pool.slot(seen.object)[33] === 3);
    clear();
    assert.ok(advance.advance(seen.object, 1));
    assert.deepEqual(seen.frames, [3, 0]);
    assert.equal(hash.bucket(3, 20, 20), seen.object);
    clear();
    assert.ok(!advance.advance(seen.object, 1));
    assert.deepEqual(seen.frames, [0, 1]);
    assert.ok(hash.bucket(3, 20, 20) === 0 && hash.bucket(1, 20, 20) === seen.object && (pool.slot(seen.object)[11] & 4) === 0);
});

test('FrameAdvance: 풀/지정기 불일치와 빈 프레임 표는 구성 때 거부하고 타입 범위 오류는 변경 전에 거부한다', () => {
    const pool = new SidPool(24000, { server: false });
    const types = frameTypes();
    const hooks = { frameSize: () => [1, 1], update: () => {}, unpop: () => {}, pop: () => {} };
    const setter = new SquidFrame(pool, types, hooks);
    const frames = types.map(() => new FrameCodeTable([]));
    assert.ok(throws(() => new RawFrameAdvance(new SidPool(24000, { server: false }), frames, setter)));
    assert.ok(throws(() => new RawFrameAdvance(pool, [], setter)));
    const advance = new RawFrameAdvance(pool, frames.slice(0, 10), setter);
    for (let id = 5; id <= OBJECT; id += 1) pool.allocate(2);
    pool.allocated(OBJECT)[10] = 82;
    const before = hex(pool.slot(OBJECT));
    assert.ok(throws(() => advance.advance(OBJECT, 1)) && hex(pool.slot(OBJECT)) === before);
    assert.equal(advance.setter, setter);
});
