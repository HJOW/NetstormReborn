// 원본 초기 시야 선택/좌표/억제 깊이·요청/변환 번호 1,182행과 ViewSpot 인자 34행을 독립 원본 관측에 대조한다. 원본게임을 실행하지 않는다. (2026-11-15 WB04-E464)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import { SidPool } from '../www/core/sid-pool.js';
import { ClientCameraGlobals } from '../www/core/client-camera-globals.js';
import { CameraHome, VIEW_SPOT_COMMAND, createCameraHomeState, parseViewSpot } from '../www/core/camera-home.js';

// 탭 구분 fixture 의 주석/머리글을 건너뛰고 열 목록으로 읽는다. 빈 열도 보존한다.
function rows(name, header = false) {
    const lines = fs.readFileSync(new URL(`../../cpppj/tests/fixtures/${name}`, import.meta.url), 'utf8').split(/\r?\n/).filter((line) => line && !line.startsWith('#'));
    return (header ? lines.slice(1) : lines).map((line) => line.split('\t'));
}
// 원본 실행기에 넣은 동일 TYPE 번호/genus/footX/footY 의 물리 입력이다([번호, flags2, footX, footY]).
const CAMERA_TYPES = [[70, 0x200, 3, 6], [71, 0x210000, 1, 1], [72, 0x200000, 3, 2], [73, 0x10000, 6, 4], [74, 0, 1, 1]];
// 슬롯의 DWORD 를 little endian 으로 쓴다.
const put = (slot, offset, value) => new DataView(slot.buffer, slot.byteOffset + offset, 4).setUint32(0, value >>> 0, true);
// 같은 SID 풀에서 SID 5..127 슬롯 123개를 확보한다(원본 입력은 5..127 번호만 쓰며 나머지는 실제 free 슬롯이다).
function makePool() {
    const pool = new SidPool(24000, { server: false }); const raw = [];
    for (let sid = 5; sid < 128; sid += 1) { const allocated = pool.allocate(2); assert.equal(allocated, sid); raw.push(pool.allocated(allocated)); }
    return { pool, raw };
}
// 원본 TYPE 표(96칸)이며 입력 다섯 타입만 genus/발자국을 가진다.
function makeTypes() { const types = Array.from({ length: 96 }, () => ({ flags2: 0, footX: 0, footY: 0 })); for (const [number, flags2, footX, footY] of CAMERA_TYPES) Object.assign(types[number], { flags2, footX, footY }); return types; }
// world float 를 실제 433aa0 으로 변환하고 commit 직전의 픽셀/현재 카운터만 관측기에 전달하는 독립 장치다(요청/변환 번호는 state 를 직접 증가시킨다).
function makePointDevice(state, observe) {
    const camera = new ClientCameraGlobals(); const requests = { get value() { return state.requests; }, set value(next) { state.requests = next >>> 0; } }; const revision = { get value() { return state.revision; }, set value(next) { state.revision = next >>> 0; } };
    return (x, y) => camera.centerImmediately(x, y, state.width, state.height, requests, revision, () => observe(camera.target().pixels[0], camera.target().pixels[1]));
}

test('CameraHome 원본 10.78 1,182행: 초기 시야 선택·좌표·억제 깊이·요청/변환 번호가 전부 일치한다', () => {
    const { pool, raw } = makePool(); const types = makeTypes(); let count = 0;
    for (const row of rows('camera-home-x86.tsv')) {
        if (row[0] !== 'originals') continue; assert.equal(row.length, 16, `행 ${count}`);
        // 앞선 행의 입력 바이트를 없애고 실제 free 상태를 만든다.
        for (const slot of raw) { slot.fill(0); slot[11] = 1; }
        // 실제 PE 에 공급한 번호/타입/상태/소유자/좌표 DWORD 를 같은 위치에 쓴다.
        if (row[2] !== '-') for (const encoded of row[2].split(';')) { const object = encoded.split(',').map(Number); const slot = raw[object[0] - 5]; slot[10] = object[1]; slot[11] = object[2]; slot[34] = object[3]; put(slot, 14, object[4]); put(slot, 18, object[5]); }
        const state = createCameraHomeState(); Object.assign(state, { fort: Number(row[3]) !== 0, battle: Number(row[4]) !== 0, challenge: Number(row[5]) !== 0, localPlayer: Number(row[6]), width: Number(row[7]), height: Number(row[8]), suppressionDepth: Number(row[9]), requests: Number(row[10]), revision: Number(row[11]), challengeType: 74, worldWidth: 4097, worldHeight: 2819 });
        const applied = []; const logic = new CameraHome(pool, types, state, makePointDevice(state, (x, y) => applied.push(`${x},${y},${state.suppressionDepth},${state.requests},${state.revision}`)));
        logic.reset(); assert.equal(applied.join('|') || '-', row[12], `행 ${count} ${row[2]}`); assert.deepEqual([state.suppressionDepth, state.requests, state.revision], [Number(row[13]), Number(row[14]), Number(row[15])], `행 ${count}`); count += 1;
    }
    assert.equal(count, 1182);
});

test('ViewSpot 원본 34행: 토큰 분리와 atol→float 결과가 입력 hex 와 일치한다', () => {
    let count = 0;
    // 입력은 탭/따옴표를 표 구분자와 분리한 16진수이며 '-' 는 빈 문자열이다.
    for (const row of rows('viewspot-x86.tsv')) {
        if (row[0] === 'input_hex') continue; const input = row[0] === '-' ? '' : Buffer.from(row[0], 'hex').toString('latin1'); const spot = parseViewSpot(input);
        const bits = (value) => { const view = new DataView(new ArrayBuffer(4)); view.setFloat32(0, value, true); return view.getUint32(0, true); };
        assert.deepEqual([bits(spot.x), bits(spot.y)], [parseInt(row[1], 16), parseInt(row[2], 16)], row[0]); count += 1;
    }
    assert.equal(count, 34); assert.equal(VIEW_SPOT_COMMAND, 0x020000d9);
});

test('ViewSpot 인자가 즉시 이동 부모의 목표/두 카운터/적용 호출까지 이어진다', () => {
    const camera = new ClientCameraGlobals(); const worldRedraw = { value: 0 }; const revision = { value: 0 }; let commits = 0; const spot = parseViewSpot('(139x88)');
    camera.centerImmediately(spot.x, spot.y, 1024, 768, worldRedraw, revision, () => { commits += 1; });
    assert.deepEqual([...camera.target().pixels], [1712, 584]); assert.deepEqual([worldRedraw.value, revision.value, commits], [1, 1, 1]);
});

test('CameraHome 화면 적용 예외도 억제 깊이를 복구하고 이후 요청/변환 번호는 이미 쓴 값으로 남는다', () => {
    const { pool } = makePool(); const types = Array.from({ length: 96 }, () => ({ flags2: 0, footX: 0, footY: 0 })); const state = createCameraHomeState(); state.suppressionDepth = 3;
    const logic = new CameraHome(pool, types, state, makePointDevice(state, () => { throw new Error('명시 장치 실패 입력'); }));
    assert.throws(() => logic.reset(), /명시 장치 실패 입력/); assert.deepEqual([state.suppressionDepth, state.requests, state.revision], [3, 1, 1]);
});

// 관측 fixture 가 닿지 않는 기본값/생성 검사/장치 경계/번호 순서/genus 비트를 독립 입력으로 고정한다(변형 확인에서 생존한 지점). (2026-11-15 WB04-E464)
// 슬롯 하나를 실제 free 가 아닌 객체로 만든다(TYPE, 상태, 소유자, 좌표 float).
function place(slot, { type, state = 0, owner = 1, x = 0, y = 0 }) {
    slot.fill(0); slot[10] = type; slot[11] = state; slot[34] = owner; const view = new DataView(slot.buffer, slot.byteOffset, slot.byteLength); view.setFloat32(14, x, true); view.setFloat32(18, y, true);
}
// 중심 호출만 기록하는 장치와 풀/타입을 가진 독립 장면이다.
function scene(state = createCameraHomeState(), hooks = {}) {
    const { pool, raw } = makePool(); const types = makeTypes(); const centers = []; const home = new CameraHome(pool, types, state, (x, y) => { centers.push([x, y]); }, hooks);
    for (const slot of raw) { slot.fill(0); slot[11] = 1; } return { pool, raw, types, centers, home, state };
}

test('CameraHome 기본 상태와 생성 검사: 원본 초기 모드/화면/월드 크기와 풀/타입/화면 소비자 범위', () => {
    assert.deepEqual(createCameraHomeState(), { localPlayer: 1, fort: false, battle: true, challenge: false, challengeType: 0, width: 640, height: 480, worldWidth: 4096, worldHeight: 2816, suppressionDepth: 0, requests: 0, revision: 0 });
    const { pool } = makePool(); const types = makeTypes(); const state = createCameraHomeState(); const center = () => {};
    assert.doesNotThrow(() => new CameraHome(pool, types, state, center)); assert.doesNotThrow(() => new CameraHome(pool, types, { ...state, width: 1, height: 1 }, center));
    assert.doesNotThrow(() => new CameraHome({ capacity: 65536 }, types, state, center)); // 웹 SidPool 은 65536 이상을 만들 수 없어 풀 크기 검사만 같은 모양의 객체로 확인한다. assert.doesNotThrow(() => new CameraHome(pool, types, state, center, { suppressionDepth: () => 0, setSuppressionDepth: () => {} }));
    for (const [label, build] of [['빈 타입', () => new CameraHome(pool, [], state, center)], ['타입 없음', () => new CameraHome(pool, undefined, state, center)], ['중심 장치 없음', () => new CameraHome(pool, types, state, null)],
        ['폭 0', () => new CameraHome(pool, types, { ...state, width: 0 }, center)], ['높이 0', () => new CameraHome(pool, types, { ...state, height: 0 }, center)], ['폭 음수', () => new CameraHome(pool, types, { ...state, width: -1 }, center)],
        ['풀 65537', () => new CameraHome({ capacity: 65537 }, types, state, center)], ['getter 만', () => new CameraHome(pool, types, state, center, { suppressionDepth: () => 0 })], ['setter 만', () => new CameraHome(pool, types, state, center, { setSuppressionDepth: () => {} })]]) assert.throws(build, TypeError, label);
});

test('CameraHome 번호 순서와 genus: 번호 2·3 도 후보이고 마지막 사제가 선택되며 사제/Carrier 판정은 정확한 비트만 쓴다', () => {
    const s = scene(); s.types[75] = { flags2: 0x200000, footX: 0, footY: 0 };
    // SID 2 와 3 의 물리 슬롯도 순회한다(원본 반복자는 2 부터 시작).
    place(s.pool.physical(2), { type: 72, x: 10, y: 20 }); s.home.reset(); assert.deepEqual(s.centers, [[7, 18]]); // 중심 이동은 발자국 전체(3,2)를 뺀다.
    s.centers.length = 0; place(s.pool.physical(3), { type: 72, x: 30, y: 40 }); s.home.reset(); assert.deepEqual(s.centers, [[27, 38]]);
    // 연속한 번호 중 마지막 후보가 고르고, 홀수 번호도 후보다.
    s.centers.length = 0; place(s.raw[0], { type: 72, x: 50, y: 60 }); place(s.raw[1], { type: 71, x: 70, y: 80 }); s.home.reset(); assert.deepEqual(s.centers, [[69, 79]]);
    s.centers.length = 0; place(s.raw[0], { type: 72, x: 50, y: 60 }); s.pool.physical(3).fill(0).fill(1, 11, 12); s.pool.physical(2).fill(0).fill(1, 11, 12); s.raw[1].fill(0).fill(1, 11, 12); s.home.reset(); assert.deepEqual(s.centers, [[47, 58]]);
    // flags2 의 낮은 비트만 가진 TYPE 은 사제도 Carrier 도 아니다.
    const low = scene(); low.types[76] = { flags2: 1, footX: 0, footY: 0 }; place(low.raw[0], { type: 76, x: 5, y: 5 }); low.home.reset(); assert.deepEqual(low.centers, [[128, 128]]);
    let previews = 0; const carrier = scene(createCameraHomeState(), { contentPreview: () => { previews += 1; } }); carrier.types[76] = { flags2: 1, footX: 0, footY: 0 }; place(carrier.raw[0], { type: 76 }); carrier.home.centerOn(5); assert.equal(previews, 0);
    place(carrier.raw[1], { type: 73 }); carrier.home.centerOn(6); assert.equal(previews, 1);
    // dead(2)/free(1) 후보는 다른 후보를 찾지 않고 자연 반환하며 둘 다 아니면 이동한다.
    carrier.centers.length = 0; place(carrier.raw[2], { type: 72, state: 2 }); carrier.home.centerOn(7); assert.deepEqual(carrier.centers, []); place(carrier.raw[2], { type: 72, state: 1 }); carrier.home.centerOn(7); assert.deepEqual(carrier.centers, []);
    place(carrier.raw[2], { type: 72, state: 4 }); carrier.home.centerOn(7); assert.equal(carrier.centers.length, 1); carrier.home.centerOn(0); assert.equal(carrier.centers.length, 1);
});

test('CameraHome TYPE 범위: 현재 TYPE 번호가 표 크기와 같으면 순회/이동/이동 뒤 재조회에서 모두 거부한다', () => {
    const state = createCameraHomeState(); const s = scene(state); place(s.raw[0], { type: 96 }); assert.throws(() => s.home.reset(), /순회의 TYPE 범위/); assert.deepEqual([state.suppressionDepth], [0]);
    place(s.raw[0], { type: 95, owner: 9 }); s.home.reset(); assert.deepEqual(s.centers, [[128, 128]]);
    assert.throws(() => { place(s.raw[0], { type: 96 }); s.home.centerOn(5); }, /현재 TYPE 누락/);
    // 중심 이동 효과가 슬롯의 TYPE 을 범위 밖으로 바꾸면 이동 뒤 재조회에서 거부한다.
    const flip = scene(); flip.home = new CameraHome(flip.pool, flip.types, flip.state, () => { flip.raw[0][10] = 96; }); place(flip.raw[0], { type: 72 }); assert.throws(() => flip.home.centerOn(5), /중심 이동 뒤 현재 TYPE/);
});

test('CameraHome 억제 깊이: 실제 소유자를 매번 읽고 쓰며 감김 직전은 거부하고 Carrier 표시는 깊이 0 에서만 미연결로 거부한다', () => {
    let depth = 5; const log = []; const hooks = { suppressionDepth: () => depth, setSuppressionDepth: (value) => { log.push(['set', value]); depth = value; } };
    const s = scene(createCameraHomeState(), hooks); s.home.reset(); assert.deepEqual([depth, s.state.suppressionDepth, log], [5, 5, [['set', 6], ['set', 5]]]);
    // 중심 이동 효과가 깊이를 바꿔도 마지막 복구는 그 현재 값에서 하나를 뺀다.
    const bump = scene(createCameraHomeState(), hooks); bump.home = new CameraHome(bump.pool, bump.types, bump.state, () => { depth = 9; }, hooks); depth = 5; log.length = 0; bump.home.reset(); assert.deepEqual([depth, log], [8, [['set', 6], ['set', 8]]]);
    depth = 0xffffffff; log.length = 0; assert.throws(() => s.home.reset(), RangeError); assert.deepEqual(log, []);
    // 장치가 없으면 state 의 독립 저장소를 쓴다. 감김 직전도 같다.
    const state = createCameraHomeState(); state.suppressionDepth = 0xffffffff; assert.throws(() => scene(state).home.reset(), RangeError); assert.equal(state.suppressionDepth, 0xffffffff);
    // Carrier 표시 장치가 없으면 깊이 0 에서만 미연결로 거부하고 깊이가 있으면(Reset 안) 건너뛴다.
    const plain = scene(); place(plain.raw[0], { type: 73 }); assert.throws(() => plain.home.centerOn(5), /Carrier의 실제 내용물 표시 미연결/); plain.state.suppressionDepth = 1; plain.home.centerOn(5);
    const inReset = scene(); place(inReset.raw[0], { type: 73, owner: 1 }); inReset.types[73] = { flags2: 0x230000, footX: 6, footY: 4 }; inReset.home.reset(); assert.deepEqual(inReset.centers, [[-6, -4]]);
});

test('ViewSpot 인자: 괄호가 없으면 첫 글자부터 읽고 숫자가 아닌 필드는 0 이다', () => {
    assert.deepEqual(parseViewSpot('139x88)'), { x: 139, y: 88 }); assert.deepEqual(parseViewSpot('(139x88)'), { x: 139, y: 88 }); assert.deepEqual(parseViewSpot('(ax88)'), { x: 0, y: 88 }); assert.deepEqual(parseViewSpot(''), { x: 0, y: 0 });
});
