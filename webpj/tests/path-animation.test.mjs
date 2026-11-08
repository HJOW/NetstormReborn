// 경로 도착 애니메이션 raw-path-animation.js(pathSide·pathDirection·pathRestFrame·RawPathAnimation)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: pathanimation-x86.tsv 10.78(originals) 구간 — Read 608행(프레임 코드의 side/direction 반환)과 Stop 1408행(Unpop/Repop 호출 기록과 슬롯 50바이트). CD 10.72·10.37 행은 웹 범위(10.78) 밖이다.
//  - 계약: path_animation_rejects_invalid_inputs_before_effects(배열 밖 입력은 가상 효과/슬롯 변경 전 거부)·path_animation_arrival_integrates_raw_unpop_pop_and_repeated_stop(실제 Unpop/Pop 으로 새 크기의 해시 단계 재등록과 반복 정지).
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E62: 경로 도착 애니메이션 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { put, get, floatToBits, bitsToFloat } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidPop, RAW_POP_RESULT } from '../www/core/squid-pop.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { SquidFactory } from '../www/core/squid-factory.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { RawPathAnimation, pathSide, pathDirection, pathRestFrame } from '../www/core/raw-path-animation.js';

// 원본 관측 파일이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E62)
const file = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'pathanimation-x86.tsv');

// 객체 위치(소수 좌표), 관찰 SID, 다리 연결 객체 TYPE, 판본당 Read/Stop 행 수이다. (2026-11-08 WB04-E62)
const X = 20.75;
const Y = 21.9;
const SOURCE = 50;
const BRIDGE_TYPE = 82;
const READ_ROWS = 608;
const STOP_ROWS = 1408;
const COMMON_VTABLE = 0x11010000;

// 예외가 나는지 본다. (2026-11-08 WB04-E62)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E62)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 저장한 프레임 코드 표(번호 → 4바이트 코드 목록)와 독립 관찰 행을 읽는다. 코드 네 바이트는 side·variant·number·flags 이다. (2026-11-08 WB04-E62)
function loadFixture() {
    if (!fs.existsSync(file)) return null;
    const codes = new Map();
    const rows = [];
    // 주석이 아닌 행을 종류별로 모은다.
    for (const line of fs.readFileSync(file, 'utf8').split(/\r?\n/)) {
        if (line.length === 0 || line[0] === '#') continue;
        const row = line.split('\t');
        if (row[0] === 'Codes') {
            assert.equal(row[2].length % 8, 0);
            const list = [];
            for (let index = 0; index < row[2].length; index += 8) {
                const byte = (n) => Number.parseInt(row[2].slice(index + n * 2, index + n * 2 + 2), 16);
                list.push({ side: byte(0), variant: byte(1), flags: byte(3) });
            }
            codes.set(Number(row[1]), new FrameCodeTable(list));
        } else rows.push(row);
    }
    return { codes, rows };
}
const fixture = loadFixture();

// 실행기의 객체 번호 50 까지 client 슬롯을 확보한 장면이다. (2026-11-08 WB04-E62)
function animationScene() {
    const pool = new SidPool(24000, { server: false });
    // 원본 관찰의 SID 와 맞춰 가상 사건의 번호도 그대로 비교한다.
    for (let id = 5; id <= SOURCE; id += 1) assert.equal(pool.allocate(2), id);
    const frames = Array.from({ length: 96 }, () => new FrameCodeTable([]));
    const scene = { pool, frames, events: [] };
    // 프레임 필드의 값을 십진수로 기록한다.
    scene.frame = () => String(get(pool.slot(SOURCE), 36, 4));
    // 입력 프레임·extra·소유자와 전체 슬롯을 원본 실행기와 같게 만든다.
    scene.prepare = (codes, frame, extra, owner) => {
        frames[BRIDGE_TYPE] = codes;
        scene.events = [];
        const raw = pool.allocated(SOURCE);
        raw.fill(0);
        put(raw, 0, COMMON_VTABLE);
        raw[10] = BRIDGE_TYPE;
        put(raw, 14, floatToBits(X));
        put(raw, 18, floatToBits(Y));
        put(raw, 36, frame >>> 0);
        raw[40] = extra;
        raw[34] = owner;
    };
    // 가상 Unpop/Pop 은 원본 실행기와 같은 호출 기록 경계이다.
    scene.hooks = () => ({
        unpop: (sid, flags) => scene.events.push(`U:${sid}:${flags}:${scene.frame()}`),
        repop: (sid, flags) => {
            const raw = pool.slot(sid);
            scene.events.push(`P:${sid}:${get(raw, 14)}:${get(raw, 18)}:${flags}:${scene.frame()}`);
        }
    });
    return scene;
}

test('PathAnimation: 원본 10.78 Read 608행의 side/direction 과 Stop 1408행의 도착 접두 사건·슬롯이 같다', { skip: !fixture && '원본 fixture 없음' }, () => {
    const scene = animationScene();
    let reads = 0;
    let stops = 0;
    // 각 기대값은 해당 PE 의 실행 결과이며 다른 판본 결과를 복사하지 않는다.
    for (const row of fixture.rows) {
        if (row[1] !== 'originals') continue;
        const codes = fixture.codes.get(Number(row[2]));
        const frame = Number(row[3]) | 0;
        if (row[0] === 'Read') {
            assert.equal(row.length, 6);
            assert.equal(pathSide(codes, frame), Number(row[4]));
            assert.equal(pathDirection(codes, frame), Number(row[5]));
            reads += 1;
            continue;
        }
        assert.equal(row.length, 8);
        scene.prepare(codes, frame, Number(row[4]), Number(row[5]));
        new RawPathAnimation(scene.pool, scene.frames, scene.hooks()).stop(SOURCE);
        assert.equal(scene.events.join(';'), row[6], `Stop 행 ${stops}`);
        assert.equal(hex(scene.pool.slot(SOURCE)), row[7], `Stop 행 ${stops}`);
        stops += 1;
    }
    assert.equal(reads, READ_ROWS);
    assert.equal(stops, STOP_ROWS);
});

test('PathAnimation: 배열 밖 입력은 가상 효과/슬롯 변경 전에 거부한다', { skip: !fixture && '원본 fixture 없음' }, () => {
    const scene = animationScene();
    const codes = fixture.codes.get(0);
    assert.ok(throws(() => new RawPathAnimation(scene.pool, scene.frames, {})));
    assert.ok(throws(() => new RawPathAnimation(scene.pool, scene.frames, { ...scene.hooks(), repop: undefined })));
    scene.prepare(codes, 0, 0, 3);
    const animation = new RawPathAnimation(scene.pool, scene.frames, scene.hooks());
    const reject = (sid) => {
        const before = hex(scene.pool.slot(SOURCE));
        assert.ok(throws(() => animation.stop(sid)));
        assert.ok(scene.events.length === 0 && hex(scene.pool.slot(SOURCE)) === before);
    };
    // 타입/음수 프레임/배열 끝/예약/free SID 각각을 부작용 없이 거부한다.
    for (const type of [69, 96, 255]) {
        scene.pool.allocated(SOURCE)[10] = type;
        reject(SOURCE);
    }
    scene.pool.allocated(SOURCE)[10] = BRIDGE_TYPE;
    put(scene.pool.allocated(SOURCE), 36, 0xffffffff);
    reject(SOURCE);
    put(scene.pool.allocated(SOURCE), 36, 64);
    reject(SOURCE);
    put(scene.pool.allocated(SOURCE), 36, 0);
    reject(0);
    reject(51);
    assert.ok(throws(() => pathSide(codes, -1)));
    assert.ok(throws(() => pathDirection(codes, 64)));
    assert.ok(throws(() => pathRestFrame(fixture.codes.get(3), 0)));
});

test('PathAnimation: 실제 일반 Unpop/Pop 은 프레임 변경 전 해제를 마치고 새 크기의 해시 단계로 재등록하며 반복 정지에도 중복 체인이 없다', () => {
    // 일반 생성/등록을 지원하는 실제 bridgeConnector 타입 번호이다. 사제 전용 훅은 이 검사의 범위 밖이다.
    const type = 155;
    const offset = 36;
    const level = 33;
    const pool = new SidPool(24000, { server: true });
    const hash = new SquidHash();
    const spots = new Uint8Array(65536);
    const types = Array.from({ length: 188 }, () => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0 }));
    const frames = types.map(() => new FrameCodeTable([]));
    types[type].constructorAddress = typeConstructorAddress(type);
    types[type].flags1 = 0x28000002;
    types[type].flags2 = 0x02000000;
    types[type].footX = 1;
    types[type].footY = 1;
    frames[type] = new FrameCodeTable([{ side: 0x41, variant: 0x50, flags: 0 }, { side: 0x41, variant: 0x50, flags: 0 }, { side: 0x41, variant: 0x50, flags: 0 }]);
    const unpop = new SquidUnpop(pool, hash, spots);
    const pop = new SquidPop(pool, hash, spots);
    pop.bindTransmission(() => {});
    const factory = new SquidFactory(pool, types, { unpop });
    const sid = factory.create(type);
    assert.equal(pop.pop(sid, types[type], 1, 1, X, Y), RAW_POP_RESULT.registered);
    const order = [];
    const animation = new RawPathAnimation(pool, frames, {
        unpop: (object, flags) => {
            order.push('unpop');
            assert.equal(flags, 0);
            unpop.unpop(object, types[type], flags);
            assert.equal(hash.bucket(pool.slot(object)[level], X, Y), 0);
        },
        repop: (object, flags) => {
            order.push('repop');
            assert.ok(flags === 0x40 && get(pool.slot(object), offset, 4) === 1);
            assert.equal(pop.pop(object, types[type], 3, 4, X, Y, flags), RAW_POP_RESULT.registered);
        }
    });
    animation.stop(sid);
    assert.deepEqual(order, ['unpop', 'repop']);
    assert.ok(pool.slot(sid)[level] === 2 && hash.bucket(1, X, Y) === 0 && hash.bucket(2, X, Y) === sid);
    assert.ok((pool.slot(sid)[11] & 4) === 0 && bitsToFloat(get(pool.slot(sid), 14)) === Math.fround(X) && bitsToFloat(get(pool.slot(sid), 18)) === Math.fround(Y));
    // 원본은 이미 정지 프레임이어도 가상 호출 두 번을 반복한다. 중복 체인은 생기면 안 된다.
    order.length = 0;
    animation.stop(sid);
    assert.deepEqual(order, ['unpop', 'repop']);
    assert.ok(get(pool.slot(sid), 4, 2) === 0 && hash.bucket(2, X, Y) === sid);
});
