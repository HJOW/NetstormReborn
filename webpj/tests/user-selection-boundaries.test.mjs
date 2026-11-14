// 사용자 선택(RawUserSelection)·후보 선택(RawUserPick)의 경계 독립 시험: 원본 관측 fixture(1,074·87·154·601·1,092행)가 가리지 못하는 상수 비트·WORD 링크의 이웃 바이트·TYPE 범위·선택 허용 비트·
// 관문 순서·float 저장 경계를 직접 확인한다. 변형 확인에서 살아남은 항목을 이 파일이 죽인다. 원본 게임을 실행하지 않는다. (2026-11-15 WB04-E495)
import test from 'node:test';
import assert from 'node:assert/strict';
import { SidPool } from '../www/core/sid-pool.js';
import { RawUserSelection, WORLD_DESTINATION_SELECTED_COMMAND } from '../www/core/raw-user-selection.js';
import { RawUserPick } from '../www/core/raw-user-pick.js';

// 10.78 풀 용량과 TYPE 수이다.
const CAPACITY = 24000; const TYPES = 188;
// float32 비트 보기이다.
const f32 = new Float32Array(1); const u32 = new Uint32Array(f32.buffer);
const floatBits = (value) => { f32[0] = value; return u32[0]; };
// 비정렬 little endian 쓰기이다(낮은 바이트부터 폭만큼, 주변 바이트 보존).
function put(raw, offset, value, width = 4) { for (let index = 0; index < width; index += 1) raw[offset + index] = Math.floor((value >>> 0) / 2 ** (8 * index)) & 0xff; }

// 호출 기록과 결과를 남기는 최소 장치 묶음이다. 모든 하위가 연결된 상태에서 선택한 훅만 덮어써 어떤 경계에 도달했는지를 `calls` 로 확인한다.
function quiet(calls, overrides = {}) {
    const record = (name, result = 0) => (...args) => { calls.push(`${name}:${args.join(',')}`); return result; };
    return {
        paused: record('paused'), enabled: record('enabled', 1), blocked: record('blocked'), localOwner: record('localOwner', 1), editor: record('editor'), dais: record('dais', 165), manaProcess: record('mana', 50), rangeProcess: record('range', 51),
        selectNotice: record('N'), refresh: record('R'), processNotice: record('F'), dump: record('D'), showMana: record('T'), rangeType: record('V'), showRange: record('C'), content: record('I'), altar: record('A'),
        moving: record('M'), movementPreview: record('E'), highlight: record('H'), assertion: record('!'), sound: record('S'), appendMultiple: record('+'), removeMultiple: record('-'), writeEnabled: record('W'),
        ...overrides,
    };
}
// 풀·188 TYPE·빈 선택 상태를 만든다.
function miniScene() {
    const pool = new SidPool(CAPACITY, { server: true }); const types = Array.from({ length: TYPES }, () => ({ flags1: 0, flags2: 0, footX: 0, footY: 0 }));
    return { pool, types, state: { selected: 0, mouseType: 0, rememberedPoint: [0, 0], multiple: [0, 0, 0, 0, 0], multipleCount: 0 } };
}
// 슬롯 하나를 0 으로 채우고 TYPE 번호를 쓴다.
function slot(scene, sid, type = 100) { const raw = scene.pool.physical(sid); raw.fill(0); raw[10] = type; return raw; }

test('RawUserSelection 경계: 사용자 이동점 통지 명령값과 현재 TYPE 범위 오류', () => {
    assert.equal(WORLD_DESTINATION_SELECTED_COMMAND, 0x02000044);
    const scene = miniScene(); slot(scene, 55, 188); const calls = [];
    // 범위 밖 TYPE 바이트는 undefined 레코드를 읽기 전에 RangeError 로 거부한다.
    assert.throws(() => new RawUserSelection(scene.pool, scene.types, scene.state, quiet(calls)).select(55, 0, 0), RangeError);
});

test('RawUserSelection 경계: 프로세스 체인은 TYPE 10..69 의 마나/사정거리 프로세스에만 통지하며 WORD 링크의 이웃 바이트를 읽지 않는다', () => {
    // [TYPE, 마나 훅 값, 사정거리 훅 값, 통지 기대]: 경계 9/10/69/70 과 보통 값이다.
    for (const [type, mana, range, expected] of [[9, 9, 9, false], [10, 10, 10, true], [69, 69, 69, true], [70, 70, 70, false], [50, 50, 51, true], [49, 50, 51, false], [51, 50, 51, true]]) {
        const scene = miniScene(); const calls = []; scene.state.selected = 50;
        const owner = slot(scene, 50); owner[6] = 60; owner[8] = 0xff; // 하위 링크 WORD 뒤의 이웃 바이트는 무시되어야 한다.
        const node = slot(scene, 60, type); node[6] = 0xff; // 다음 링크 WORD(+4/+5) 뒤의 이웃 바이트도 무시한다.
        new RawUserSelection(scene.pool, scene.types, scene.state, quiet(calls, { manaProcess: () => mana, rangeProcess: () => range })).select(0, 0, 0);
        assert.equal(calls.includes('F:60,3,0,0'), expected, `TYPE ${type}`); assert.equal(scene.state.selected, 0);
    }
});

test('RawUserSelection 경계: TYPE 플래그 마스크는 정확한 비트에만 반응한다(낮은 비트는 마나 표시·이동·선택 불허로 보지 않는다)', () => {
    const scene = miniScene(); const calls = []; scene.types[100].flags1 = 1; scene.types[100].flags2 = 1; slot(scene, 55);
    assert.equal(new RawUserSelection(scene.pool, scene.types, scene.state, quiet(calls)).select(55, 0, 0), 55); assert.equal(scene.state.selected, 55);
    assert.ok(!calls.some((call) => call.startsWith('T:') || call.startsWith('I:') || call.startsWith('M:')));
    // 반대로 정확한 비트가 있으면 해당 경계에 도달한다(마나 표시 0x10000000·이동 0x10000/0x20000).
    for (const [flags1, flags2, prefix] of [[0x10000000, 0, 'T:'], [0, 0x10000, 'I:'], [0, 0x20000, 'I:']]) {
        const other = miniScene(); const otherCalls = []; other.types[100].flags1 = flags1; other.types[100].flags2 = flags2; slot(other, 55);
        new RawUserSelection(other.pool, other.types, other.state, quiet(otherCalls)).select(55, 0, 0); assert.ok(otherCalls.some((call) => call.startsWith(prefix)), prefix);
    }
});

test('RawUserSelection 경계: 기억 좌표는 두 float 비트를 그대로 쓰며 이동 불가 TYPE 은 쓰지 않는다', () => {
    const scene = miniScene(); scene.types[100].flags2 = 0x10000; scene.types[101].flags2 = 0; const selection = new RawUserSelection(scene.pool, scene.types, scene.state, {});
    selection.remember(0x11111111, 0xeeeeeeee, 100); assert.deepEqual(scene.state.rememberedPoint, [0x11111111, 0xeeeeeeee]); assert.equal(scene.state.mouseType, 100);
    selection.remember(1, 2, 101); assert.deepEqual(scene.state.rememberedPoint, [0x11111111, 0xeeeeeeee]); assert.equal(scene.state.mouseType, 100);
});

test('RawUserSelection 경계: 마우스 재확인은 선택을 강제하지 않아 선택 불허 TYPE 은 선택하지 않고 마우스 TYPE 만 지운다', () => {
    const scene = miniScene(); const calls = []; scene.types[74].flags1 = 0x08000000; slot(scene, 55, 74); scene.state.mouseType = 74;
    new RawUserSelection(scene.pool, scene.types, scene.state, quiet(calls)).recheckMouse(() => 55);
    assert.equal(scene.state.selected, 0); assert.equal(scene.state.mouseType, 0); assert.ok(!calls.some((call) => call.startsWith('N:')));
});

test('RawUserSelection 경계: 다중 승격은 현재 첫 원소 하나만 있어도 동작하고 빈 배열은 거부한다', () => {
    const scene = miniScene(); const calls = []; slot(scene, 55)[40] = 4; slot(scene, 56); scene.state.selected = 55; scene.state.multiple = [56]; scene.state.multipleCount = 1;
    new RawUserSelection(scene.pool, scene.types, scene.state, quiet(calls)).toggleMultiple(55, 0);
    assert.equal(scene.state.selected, 56); assert.deepEqual(calls.filter((call) => call.startsWith('-:') || call.startsWith('R:')), ['-:56', 'R:55']);
    const empty = miniScene(); slot(empty, 55)[40] = 4; empty.state.selected = 55; empty.state.multiple = []; empty.state.multipleCount = 1;
    assert.throws(() => new RawUserSelection(empty.pool, empty.types, empty.state, quiet([])).toggleMultiple(55, 0), RangeError);
});

test('RawUserSelection 경계: Shift 확장의 관문(정지·0 번·이동 불가)과 기본 선택 상태(죽음 비트 1 만 단일 선택으로, 비트 2 는 토글)', () => {
    // 후보 번호와 장면 설정자를 받아 확장을 한 번 실행하고 호출 기록을 돌려준다.
    const run = (setup, number = 56) => {
        const scene = miniScene(); const calls = []; scene.types[100].flags2 = 0x10000; scene.types[101].flags2 = 0; for (const sid of [55, 56]) slot(scene, sid, 100); slot(scene, 57, 101);
        scene.state.selected = 55; const hooks = setup(scene); new RawUserSelection(scene.pool, scene.types, scene.state, quiet(calls, hooks)).extendSelection(number, 0); return calls;
    };
    // 일반 상태의 기본 선택이면 토글(추가 목록)이다.
    assert.ok(run(() => ({})).includes('+:56'));
    // 기본 선택의 상태 비트 1(죽음)은 단일 선택으로 가고 비트 2(반납)는 토글이다.
    assert.ok(run((scene) => { scene.pool.physical(55)[11] = 1; return {}; }).some((call) => call.startsWith('D:')));
    assert.ok(run((scene) => { scene.pool.physical(55)[11] = 2; return {}; }).includes('+:56'));
    // 기본 선택이 이동 불가 TYPE 이면 단일 선택이다.
    assert.ok(run((scene) => { scene.state.selected = 57; return {}; }).some((call) => call.startsWith('D:')));
    // 정지·0 번·이동 불가 대상은 아무 효과도 없다(정지 확인만 기록된다).
    assert.deepEqual(run(() => ({ paused: () => 1 })), []);
    assert.deepEqual(run(() => ({}), 0), ['paused:']);
    assert.deepEqual(run(() => ({}), 57), ['paused:']);
});

test('RawUserSelection 경계: 다중 이동은 현재 배열 길이를 넘는 개수를 거부하고 enabled 1 을 쓰기 전에 멈춘다', () => {
    const scene = miniScene(); const calls = []; slot(scene, 55); scene.state.multiple = [55]; scene.state.multipleCount = 2; const moved = [];
    assert.throws(() => new RawUserSelection(scene.pool, scene.types, scene.state, quiet(calls)).moveMultiple(7, 9, (sid, x, y) => moved.push([sid, x, y])), RangeError);
    assert.deepEqual(moved, [[55, 7, 9]]); assert.deepEqual(calls.filter((call) => call.startsWith('W:')), ['W:0']);
});

test('RawUserSelection 경계: Carrier/Archer 메뉴 입구는 선택 허용 비트를 우회하지 않는다', () => {
    for (const method of ['carrierMenu', 'archerMenu']) {
        const scene = miniScene(); const calls = []; scene.types[100].flags1 = 0x08000000; slot(scene, 55);
        const result = new RawUserSelection(scene.pool, scene.types, scene.state, quiet(calls))[method](55, { code: 1 }, () => 0x80000001);
        assert.equal(result, 0x80000001); assert.equal(scene.state.selected, 0); assert.ok(!calls.some((call) => call.startsWith('N:')), method);
    }
});

// ---- RawUserPick 경계 시험 ----
// 객체 목록으로 후보 장면을 만들고 한 번 선택한다. object = { sid, type, owner, z, x, y, extra, hit, flags1, flags2 }, options = { owner, hide, meta, force, point, center }.
function pickRun(objects, options = {}) {
    const pool = new SidPool(CAPACITY, { server: true }); const types = Array.from({ length: TYPES }, () => ({ flags1: 0, flags2: 0, footX: 0, footY: 0 })); const calls = [];
    for (const object of objects) {
        const type = object.type ?? 80; if (types[type]) { types[type].flags1 = object.flags1 ?? 0; types[type].flags2 = object.flags2 ?? 0; }
        const raw = pool.physical(object.sid); raw.fill(0); raw[10] = type; raw[34] = object.owner ?? 0; raw[35] = (object.z ?? 0) & 0xff; raw[40] = object.extra ?? 0; put(raw, 14, floatBits(object.x ?? 0)); put(raw, 18, floatBits(object.y ?? 0));
    }
    const ids = objects.map((object) => object.sid); let cursor = 0; const point = options.point ?? [0, 0];
    const hooks = {
        paused: () => 0, localOwner: () => options.owner ?? 1, metamap: () => options.meta ?? 0, hideBuildings: () => options.hide ?? 0, project: () => point,
        begin: () => { cursor = 0; return ids[0]; }, next: () => (++cursor < ids.length ? ids[cursor] : 0),
        center: (sid) => { calls.push(`center:${sid}`); return options.center ? options.center(sid) : [0, 0]; }, hit: (sid) => { calls.push(`hit:${sid}`); return objects.find((object) => object.sid === sid).hit ?? true; },
    };
    const pick = new RawUserPick(pool, types, hooks); const result = options.force === undefined ? pick.pick(1, 2) : pick.pick(1, 2, options.force); return { result, calls };
}

test('RawUserPick 경계: 필터 마스크는 정확한 비트에만 반응한다(낮은 비트·형제 TYPE 번호·extra 0x10)', () => {
    // flags2 낮은 비트/flags1 낮은 비트/extra 비트 0 은 추상·비선택·숨김·이동·표면 판정에 영향을 주지 않는다.
    assert.equal(pickRun([{ sid: 50, flags2: 1, flags1: 1, extra: 1, owner: 2 }], { hide: 1 }).result, 50);
    assert.ok(!pickRun([{ sid: 50, flags2: 1 }]).calls.some((call) => call.startsWith('center:')));
    // 숨김 TYPE: 164 와 84 는 숨기고 165 는 숨기지 않는다.
    assert.equal(pickRun([{ sid: 50, type: 164 }], { hide: 1 }).result, 0); assert.equal(pickRun([{ sid: 50, type: 84 }], { hide: 1 }).result, 0); assert.equal(pickRun([{ sid: 50, type: 165 }], { hide: 1 }).result, 50);
    assert.equal(pickRun([{ sid: 50, type: 164 }], { hide: 0 }).result, 50);
    // 표면 비트(0x80006)만 마지막 근접 교체를 허락하고 낮은 비트는 허락하지 않는다.
    const near = { sid: 51, type: 81, flags2: 0x10000, hit: false };
    assert.equal(pickRun([{ sid: 50, flags2: 1 }, near]).result, 50); assert.equal(pickRun([{ sid: 50, flags2: 2 }, near]).result, 51);
    assert.equal(pickRun([{ sid: 50, flags2: 0x80000 }, near]).result, 51); assert.equal(pickRun([{ sid: 50, flags2: 4 }, near]).result, 51);
    // 추상(0x2000000)·비선택(0x8000000) 필터는 force 로 우회하고 기본 force 는 0 이다.
    assert.equal(pickRun([{ sid: 50, flags2: 0x2000000 }]).result, 0); assert.equal(pickRun([{ sid: 50, flags2: 0x2000000 }], { force: 1 }).result, 50);
    assert.equal(pickRun([{ sid: 50, flags1: 0x8000000 }]).result, 0); assert.equal(pickRun([{ sid: 50, flags1: 0x8000000 }], { force: 1 }).result, 50);
    // 다른 소유자의 extra 0x10 은 거부하고 자기 소유자의 것은 허용한다.
    assert.equal(pickRun([{ sid: 50, extra: 0x10, owner: 2 }]).result, 0); assert.equal(pickRun([{ sid: 50, extra: 0x10, owner: 1 }]).result, 50);
});

test('RawUserPick 경계: 근접 거리는 두 축을 각자 쓰며 1 번 SID 도 후보이고 TYPE 범위 밖은 거부한다', () => {
    // 점 (10, 0) 과 중심 (10, 1) 의 거리는 1(< 1.6)이며 축이 섞이면 9 가 된다.
    assert.equal(pickRun([{ sid: 50, type: 81, flags2: 0x10000, hit: false }], { point: [10, 0], center: () => [10, 1] }).result, 50);
    assert.equal(pickRun([{ sid: 50, type: 81, flags2: 0x10000, hit: false }], { point: [0, 10], center: () => [1, 10] }).result, 50);
    assert.equal(pickRun([{ sid: 50, type: 81, flags2: 0x10000, hit: false }], { point: [10, 0], center: () => [10, 2] }).result, 0);
    // 1 번 SID 는 후보 사슬의 시작으로 허용된다(0 번과 용량 이상만 끝).
    assert.equal(pickRun([{ sid: 1 }]).result, 1);
    // 범위 밖 TYPE 바이트는 undefined 레코드를 읽기 전에 RangeError 로 거부한다.
    assert.throws(() => pickRun([{ sid: 50, type: 188 }]), RangeError);
});

test('RawUserPick 경계: 초기 좌표/깊이 비교는 float 저장 경계와 signed 전체 BYTE 를 그대로 쓴다', () => {
    // 같은 깊이·같은 좌표(1e-8)의 두 후보는 첫 후보가 남는다(초기 0 에서의 float 차이 합이 원래 값을 보존).
    assert.equal(pickRun([{ sid: 50, x: 1e-8, y: 1e-8 }, { sid: 51, x: 1e-8, y: 1e-8 }]).result, 50);
    // 깊이가 1 만 큰 후보는 좌표가 앞서도 밀린다(signed BYTE 를 반으로 줄이면 같아져 뒤집힌다).
    assert.equal(pickRun([{ sid: 50, z: 4, y: 0 }, { sid: 51, z: 5, y: 1 }]).result, 50);
    // 더 깊은(작은 signed) 값은 앞선다.
    assert.equal(pickRun([{ sid: 50, z: -3 }, { sid: 51, z: -4 }]).result, 51);
    // 같은 깊이에서는 세로 좌표가 큰(부호까지 읽은) 후보가 앞선다(좌표 필드를 한 바이트 어긋나게 읽으면 부호를 잃어 뒤집힌다).
    assert.equal(pickRun([{ sid: 50, y: 1 }, { sid: 51, y: -1 }]).result, 50); assert.equal(pickRun([{ sid: 50, y: -1 }, { sid: 51, y: 1 }]).result, 51);
    assert.equal(pickRun([{ sid: 50, x: 1 }, { sid: 51, x: -1 }]).result, 50); assert.equal(pickRun([{ sid: 50, x: -1 }, { sid: 51, x: 1 }]).result, 51);
});
