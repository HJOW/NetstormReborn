// 객체 프레임 지정 squid-frame.js(SquidFrame)와 프레임 연결 squid-frame-binding.js(makeSquidFrameHooks)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: setframe-x86.tsv 의 10.78(originals) Set 1920행·Thunk 4행(표시 갱신 D=+0x88/E=+0x8c·Unpop U·Pop P 호출 기록과 슬롯 50바이트). CD 10.72·10.37 행은 웹 범위(10.78) 밖이다.
//  - 계약: set_frame_rejects_missing_hooks_and_invalid_flags·set_frame_real_unpop_and_pop_move_hash_level_on_next_change(패치) 와
//    FrameDisplayTests 의 frame_display_binding_* 네 가지(old/new 변경 영역·선택 표식·그림자·전체 갱신 억제, 현재 프레임 범위 오류, 연결 불일치 거부, SHP 크기 두 배 범위)이다.
//    픽셀을 그리는 Renderer 확인은 월드 표시 단위의 몫이라 변경 영역(DirtyRegions 통지 순서)까지만 대조한다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E59: 객체 프레임 지정 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, bitsToFloat, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidPop, RAW_POP_RESULT } from '../www/core/squid-pop.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { SquidFactory } from '../www/core/squid-factory.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { SquidDisplay } from '../www/core/squid-display.js';
import { SquidFrame, FRAME_UPDATE_ALTERNATE } from '../www/core/squid-frame.js';
import { makeSquidFrameHooks } from '../www/core/squid-frame-binding.js';
import { DirtyRegions } from '../www/render/dirty-regions.js';

// 원본 관측 파일이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E59)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'setframe-x86.tsv'));

// 기계어 실행기의 입력 번호·타입 번호·가상 표 기록값과 객체 위치(소수 좌표), 이중 프레임 비트, 관찰 행 수이다. (2026-11-08 WB04-E59)
const OBJECT = 50;
const TYPE = 82;
const VTABLE = 0x11010000;
const CAPACITY = 24000;
const X = 20.75;
const Y = 21.9;
const DOUBLE_FRAMES = 0x440000;
const SET_ROWS = 1920;
const THUNK_ROWS = 4;

// 예외가 나는지 본다. (2026-11-08 WB04-E59)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E59)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 슬롯의 프레임 DWORD 를 부호 없는 십진수로 만든다. (2026-11-08 WB04-E59)
function frameNow(pool, sid) {
    const raw = pool.slot(sid);
    return String(((raw[0x24] | (raw[0x25] << 8) | (raw[0x26] << 16) | (raw[0x27] << 24)) >>> 0));
}

// 관찰 파일을 Sizes / Kind / 나머지 행으로 나눈다. (2026-11-08 WB04-E59)
function loadFixture() {
    const sizes = [];
    const kinds = [];
    const body = [];
    // 주석이 아닌 행을 종류별로 모은다.
    for (const row of rows) {
        if (row[0] === 'Sizes') {
            for (const entry of row[1].split(';')) {
                const pair = entry.split(',');
                sizes.push([bitsToFloat(Number(pair[0])), bitsToFloat(Number(pair[1]))]);
            }
        } else if (row[0] === 'Kind') {
            kinds.push({ flags1: Number(row[2]) >>> 0, flags2: Number(row[3]) >>> 0, count: Number(row[4]) });
        } else body.push(row);
    }
    return { sizes, kinds, rows: body };
}

// 실행기와 같은 풀 번호 5~74 를 확보한 장면과 호출 기록 훅이다. (2026-11-08 WB04-E59)
function frameScene(sizes) {
    const pool = new SidPool(CAPACITY, { server: false });
    // 예측/서버 목록과 무관한 client 번호를 입력 슬롯으로 확보한다.
    for (let id = 5; id <= 74; id += 1) assert.equal(pool.allocate(2), id);
    const types = Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0 }));
    const scene = { pool, types, events: [], kind: null };
    // 실행기와 같은 객체 슬롯과 타입을 준비한다: 가상 표, 타입, 위치, 프레임, 단계 바이트.
    scene.prepare = (kind, level, frame) => {
        scene.kind = kind;
        scene.events = [];
        types[TYPE].flags1 = kind.flags1;
        types[TYPE].flags2 = kind.flags2;
        const raw = pool.allocated(OBJECT);
        raw.fill(0);
        put(raw, 0, VTABLE);
        raw[10] = TYPE;
        put(raw, 14, floatToBits(X));
        put(raw, 18, floatToBits(Y));
        put(raw, 0x24, frame);
        raw[0x21] = level;
    };
    // 호출 사실과 인자만 기록한다. 프레임 크기는 합성 SHP 표에서 읽고 패치판 00419850 의 범위 검사(assert)는 예외로 옮긴다.
    scene.hooks = () => ({
        frameSize: (sid, frame) => {
            const limit = scene.kind.count * ((scene.kind.flags1 & DOUBLE_FRAMES) !== 0 ? 2 : 1);
            if (frame < 0 || frame >= limit) throw new RangeError('프레임이 표 밖이다(00419850 assert)');
            return frame < sizes.length ? sizes[frame] : [0, 0];
        },
        update: (sid, flags) => scene.events.push(`${(flags & FRAME_UPDATE_ALTERNATE) !== 0 ? 'E' : 'D'}:${sid}:${frameNow(pool, OBJECT)}`),
        unpop: (sid, flags) => scene.events.push(`U:${sid}:${flags}:${frameNow(pool, OBJECT)}`),
        pop: (sid, x, y, flags) => scene.events.push(`P:${sid}:${floatToBits(x)}:${floatToBits(y)}:${flags}:${frameNow(pool, OBJECT)}`)
    });
    scene.eventText = () => (scene.events.length === 0 ? '-' : scene.events.join(';'));
    return scene;
}

test('SquidFrame: 원본 10.78 프레임 지정 1920행의 사건 순서와 슬롯 50바이트가 같다(Thunk 4행 포함)', { skip: !rows && '원본 fixture 없음' }, () => {
    const fixture = loadFixture();
    assert.equal(fixture.sizes.length, 8);
    assert.equal(fixture.kinds.length, 4);
    const scene = frameScene(fixture.sizes);
    let sets = 0;
    let thunks = 0;
    // 10.78(originals) 관찰 행만 재생한다.
    for (const row of fixture.rows) {
        if (row[1] !== 'originals') continue;
        if (row[0] === 'Thunk') {
            // base 의 +0x8c 는 +0x88 로 넘어간다. 표시 갱신 훅이 두 경로를 하나로 받는 근거이다.
            assert.ok(row.length === 4 && row[3] === 'D:50:0');
            thunks += 1;
            continue;
        }
        assert.equal(row.length, 9);
        let actual;
        try {
            scene.prepare(fixture.kinds[Number(row[2])], Number(row[3]), Number(row[4]));
            const frames = new SquidFrame(scene.pool, scene.types, scene.hooks());
            frames.set(OBJECT, Number(row[5]) | 0, Number(row[6]));
            actual = `${scene.eventText()}|${hex(scene.pool.slot(OBJECT))}`;
        } catch (error) {
            actual = `예외: ${error.message}`;
        }
        assert.equal(actual, `${row[7]}|${row[8]}`, `행 ${sets}`);
        sets += 1;
    }
    assert.equal(sets, SET_ROWS);
    assert.equal(thunks, THUNK_ROWS);
});

test('SquidFrame: 훅 누락과 원본 assert 에 해당하는 flags 는 상태를 바꾸기 전에 거부한다', { skip: !rows && '원본 fixture 없음' }, () => {
    const fixture = loadFixture();
    const scene = frameScene(fixture.sizes);
    scene.prepare(fixture.kinds[2], 2, 3);
    const hooks = scene.hooks();
    assert.ok(throws(() => new SquidFrame(scene.pool, scene.types, { ...hooks, pop: undefined })));
    assert.ok(throws(() => new SquidFrame(scene.pool, scene.types, { ...hooks, frameSize: undefined })));
    const frames = new SquidFrame(scene.pool, scene.types, hooks);
    const before = scene.pool.bytes();
    assert.ok(throws(() => frames.set(OBJECT, 1, 1)));
    assert.ok(throws(() => frames.set(OBJECT, 1, 0x2010)));
    assert.deepEqual(scene.events, []);
    assert.deepEqual(scene.pool.bytes(), before);
    // 현재 프레임이 표 밖이면 패치판 00419850 의 assert 이다. 훅이 거부하고 단계 바이트·프레임은 그대로이다.
    scene.prepare(fixture.kinds[2], 2, 9);
    const invalid = scene.pool.bytes();
    assert.ok(throws(() => frames.set(OBJECT, 1, 0)));
    assert.deepEqual(scene.events, []);
    assert.deepEqual(scene.pool.bytes(), invalid);
});

// 실제 Pop/Unpop/표시를 조립한 장면이다: 공통 가상 표시/Pop 을 쓰는 연결 객체 타입 155, 판본 크기의 TYPE 표, 합성 SHP 메타데이터, 변경 표(DirtyRegions) 출력 장치이다. (2026-11-08 WB04-E59)
const REAL_TYPE = 155;
function realScene({ shadow = false, serverPool = true } = {}) {
    const pool = new SidPool(32768, { server: serverPool });
    const types = Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0, defaultFrame: 0, name: '' }));
    const record = types[REAL_TYPE];
    record.constructorAddress = typeConstructorAddress(REAL_TYPE);
    record.flags1 = (0x28000002 | (shadow ? 0x40000 : 0)) >>> 0;
    record.flags2 = 0x02000000;
    record.footX = 1;
    record.footY = 1;
    // 다른 픽셀 경계와 칸 크기를 가진 합성 SHP 메타데이터이다.
    const shapes = types.map(() => ({ frameCount: 0, loaded: false, frames: [] }));
    const frame = (width, height, hotspotX, hotspotY, cellWidth, cellHeight) => ({ width, height, hotspotX, hotspotY, cellWidth, cellHeight });
    shapes[REAL_TYPE] = { frameCount: shadow ? 2 : 4, loaded: true, frames: [frame(11, 17, 12, 16, 1, 1), frame(26, 10, 5, 2, 2, 1.5), frame(8, 4, 2, 1, 5, 5), frame(6, 3, 1, 0, 6, 6)] };
    if (shadow) {
        shapes[REAL_TYPE].frames[2].cellWidth = 1;
        shapes[REAL_TYPE].frames[2].cellHeight = 1;
    }
    // 변경 표를 두고 첫 화면은 전체 갱신 상태(원본 displayOff)로 시작한다. 객체의 프레임은 통지 시점의 raw 값을 기록한다.
    const dirty = new DirtyRegions(640, 480);
    dirty.invalidateAll();
    const scene = { pool, types, shapes, dirty, notices: [], object: 0 };
    const sink = {
        suppressed: () => dirty.fullRedraw,
        invalidate: (rect, flags) => {
            scene.notices.push({ rect: { ...rect }, flags, frame: Number(frameNow(pool, scene.object)) });
            dirty.add(rect, flags);
        }
    };
    scene.display = new SquidDisplay(types, shapes, sink, { cameraX: 0, cameraY: 0, zoom: 65536, viewport: { left: 0, top: 0, right: 640, bottom: 480 } });
    scene.hash = new SquidHash();
    scene.spots = new Uint8Array(65536);
    scene.unpop = new SquidUnpop(pool, scene.hash, scene.spots, scene.display);
    scene.pop = new SquidPop(pool, scene.hash, scene.spots, { display: scene.display });
    // 서버 Pop 의 전송 writer 는 이 검사에서 연결 객체가 없는 명시 입력이라 호출만 받는다(패킷 바이트를 검사하지 않는다).
    scene.pop.bindTransmission(() => {});
    scene.factory = new SquidFactory(pool, types, { unpop: scene.unpop });
    // 이전 표시 요청을 끝내 다음 부분 갱신을 관찰한다(Draw/Present 후 변경 표 초기화).
    scene.flush = () => {
        dirty.clear();
        scene.notices = [];
    };
    return scene;
}

// 픽셀 경계와 칸 크기가 다른 입력으로 표시/해시 연결을 확인한다. 변경 영역 사각형은 원본 정수 모서리별로 비교한다. (2026-11-08 WB04-E59)
function runRealScene(shadow) {
    const scene = realScene({ shadow });
    const { pool, types, display, hash } = scene;
    const object = scene.factory.create(REAL_TYPE);
    scene.object = object;
    if (shadow) pool.allocated(object)[40] |= 4;
    scene.flush();
    assert.equal(scene.pop.pop(object, types[REAL_TYPE], 1, 1, 20, 20), RAW_POP_RESULT.registered);
    scene.flush();
    const frames = new SquidFrame(pool, types, makeSquidFrameHooks(pool, types, display, scene.unpop, scene.pop));
    frames.set(object, 1, shadow ? FRAME_UPDATE_ALTERNATE : 0);
    const same = (a, b) => a.left === b[0] && a.top === b[1] && a.right === b[2] && a.bottom === b[3];
    if (shadow) {
        assert.equal(scene.notices.length, 4);
        assert.ok(same(scene.notices[0].rect, [296, 189, 332, 222]));
        assert.ok(same(scene.notices[1].rect, [318, 219, 327, 224]));
        assert.ok(same(scene.notices[2].rect, [312, 203, 345, 229]));
        assert.ok(same(scene.notices[3].rect, [319, 220, 326, 224]));
        // old main/shadow 뒤 new main/shadow 이다. 0x2000 자체는 변경 표 플래그에 섞이지 않는다.
        scene.notices.forEach((notice, index) => {
            assert.equal(notice.flags, 16);
            assert.equal(notice.frame, index < 2 ? 0 : 1);
        });
        scene.flush();
        frames.set(object, 1, FRAME_UPDATE_ALTERNATE);
        assert.equal(scene.notices.length, 4);
        scene.flush();
        scene.dirty.invalidateAll();
        frames.set(object, 0, 0);
        assert.equal(scene.notices.length, 0);
        assert.equal(Number(frameNow(pool, object)), 0);
    } else {
        assert.equal(scene.notices.length, 2);
        assert.ok(scene.notices[0].frame === 0 && scene.notices[1].frame === 1);
        assert.ok(same(scene.notices[0].rect, [308, 204, 320, 222]) && same(scene.notices[1].rect, [315, 218, 342, 229]));
        scene.notices = [];
        frames.set(object, 2, 0);
        assert.equal(scene.notices.length, 2);
        assert.equal(pool.slot(object)[0x21], 1);
        assert.equal(hash.bucket(1, 20, 20), object);
        scene.flush();
        frames.set(object, 3, 0);
        assert.ok(scene.notices.length === 2 && scene.notices[0].frame === 2 && scene.notices[1].frame === 3);
        assert.equal(pool.slot(object)[0x21], 3);
        assert.ok(hash.bucket(1, 20, 20) === 0 && hash.bucket(3, 20, 20) === object && (pool.slot(object)[11] & 4) === 0);
        assert.ok(same(scene.notices[0].rect, [318, 219, 327, 224]) && same(scene.notices[1].rect, [319, 220, 326, 224]));
        assert.deepEqual(display.frameSize(pool.slot(object), 3), [6, 6]);
    }
}

test('SquidFrame 연결: old/new 변경 영역과 단계 이동 재등록이 같은 raw 풀에서 같다', () => runRealScene(false));

test('SquidFrame 연결: 선택 표식·그림자·alternate 표시·같은 프레임 반복과 전체 갱신 억제를 보존한다', () => runRealScene(true));

test('SquidFrame 연결: 현재 프레임 범위 오류는 단계 바이트·해시·표시 영역 쓰기 전에 거부한다', () => {
    const scene = realScene();
    const { pool, types, shapes, display } = scene;
    scene.dirty.clear();
    const sid = scene.factory.create(REAL_TYPE);
    scene.object = sid;
    put(pool.allocated(sid), 36, 4);
    const frame = new SquidFrame(pool, types, makeSquidFrameHooks(pool, types, display, scene.unpop, scene.pop));
    const before = hex(pool.slot(sid));
    // 현재 프레임 4 는 TYPE 프레임 수(4) 밖이라 00419850 assert 가 상세 writer 앞에서 거부된다(출력 장치에 오류 상세 writer 가 없다).
    assert.ok(throws(() => frame.set(sid, 0, 0)));
    assert.equal(hex(pool.slot(sid)), before);
    assert.equal(scene.notices.length, 0);
    put(pool.allocated(sid), 36, 0);
    shapes[REAL_TYPE].loaded = false;
    const missing = hex(pool.slot(sid));
    assert.ok(throws(() => frame.storeLevel(sid)));
    assert.equal(hex(pool.slot(sid)), missing);
    shapes[REAL_TYPE].loaded = true;
    types[REAL_TYPE].flags1 = (types[REAL_TYPE].flags1 | 0x40000) >>> 0;
    // 그림자 두 배 범위 안의 마지막 물리 프레임 3 은 읽고 4 는 표 밖이다.
    assert.deepEqual(display.frameSize(pool.slot(sid), 3), [6, 6]);
    assert.ok(throws(() => display.frameSize(pool.slot(sid), 4)));
});

test('SquidFrame 연결: 표시 미연결 공간 모듈·다른 풀/타입 표 조합은 구성 시점에 거부한다', () => {
    const scene = realScene();
    const { pool, types, display, hash, spots, unpop, pop } = scene;
    const other = new SidPool(32768, { server: true });
    const silentUnpop = new SquidUnpop(pool, hash, spots);
    const silentPop = new SquidPop(pool, hash, spots);
    assert.ok(throws(() => makeSquidFrameHooks(pool, types, display, silentUnpop, pop)));
    assert.ok(throws(() => makeSquidFrameHooks(pool, types, display, unpop, silentPop)));
    assert.ok(throws(() => makeSquidFrameHooks(other, types, display, unpop, pop)));
    const wrongHash = new SquidUnpop(pool, new SquidHash(), spots, display);
    const wrongSpots = new SquidUnpop(pool, hash, new Uint8Array(65536), display);
    assert.ok(throws(() => makeSquidFrameHooks(pool, types, display, wrongHash, pop)));
    assert.ok(throws(() => makeSquidFrameHooks(pool, types, display, wrongSpots, pop)));
    const copied = types.map((record) => ({ ...record }));
    copied[REAL_TYPE].footX = 2;
    assert.ok(throws(() => makeSquidFrameHooks(pool, copied, display, unpop, pop)));
    assert.ok(throws(() => display.validateFrameBinding(types.slice(0, 187))));
});

test('SquidDisplay.frameSize: 패치의 logical frameCheck 와 그림자 두 배 범위, 해제 표식을 보존한다', () => {
    const scene = realScene();
    const { types, shapes } = scene;
    shapes[REAL_TYPE].frameCount = 1;
    const raw = new Uint8Array(64);
    raw[10] = REAL_TYPE;
    assert.ok(throws(() => scene.display.frameSize(raw, 1)), '프레임 수 밖은 assert');
    types[REAL_TYPE].flags1 = (types[REAL_TYPE].flags1 | 0x40000) >>> 0;
    assert.deepEqual(scene.display.frameSize(raw, 1), [2, 1.5]);
    assert.ok(throws(() => scene.display.frameSize(raw, 2)));
    // 원본 frameCheck 의 해제 흔적 값(TYPE 첫 DWORD)은 assert 이다.
    types[REAL_TYPE].maxHitPoints = -0x22222223;
    assert.ok(throws(() => scene.display.frameSize(raw, 1)));
    types[REAL_TYPE].maxHitPoints = -0x32323233;
    assert.ok(throws(() => scene.display.frameSize(raw, 1)));
    types[REAL_TYPE].maxHitPoints = 0;
    assert.deepEqual(scene.display.frameSize(raw, 1), [2, 1.5]);
});
