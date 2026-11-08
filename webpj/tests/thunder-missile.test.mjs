// 썬더 캐논(TYPE 131, group 1)의 132 Missile raw-missile.js(RawMissile)를 원본 10.78 관측과 대조한다.
//  - 원본 관측 1: thunder-missile-x86.tsv — 몸체(0 생성자 / 1 프레임 / 2 가상 포구 / 3 사정선 끝)와 포구·SHP/타입 hotspot·종점·시각 비율·표적 상태·이전 좌표·코너·사거리·소리·방향 수/프레임 수·표적 추가 비트·통과 표시·발사자 현재 프레임/방향 문자로
//    외부 효과 순서, 128바이트 필드 이미지, 세 슬롯 150바이트, 포구/사정선 8바이트가 같다. 포구·hotspot·치수·방향 수치와 방향 getter 는 이식된 RawProjectileGeometry/RawGunMode 를 그대로 쓴다.
//  - 원본 관측 2: thunder-hit-event-x86.tsv 688행 — 썬더×스톤 타워 충돌의 명령 큐 알림(24건)과 음성·코너·free/dead·실패 타입·통과/충돌 억제·사거리 종료의 효과·전체 이미지, 큐 훅 누락 시 거부.
//  - 원본 관측 3: missile-floater-x86.tsv 481행 — 클라우드 플로터 회피(공유 게임 난수 한 번 소비, 회피 뒤 통과 표시)·일반 적중·코너·범위 종료·난수 미선택과 다섯 발사자의 전체 상태·난수 상태.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E126: 썬더 Missile 시험 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, bitsToFloat, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { GameRandom } from '../www/core/random.js';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { MissileFields } from '../www/core/raw-missile-fields.js';
import { MissileState, RawMissile } from '../www/core/raw-missile.js';
import { RawProjectileGeometry } from '../www/core/raw-projectile-geometry.js';
import { RawGunMode } from '../www/core/raw-gun-mode.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E126)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'thunder-missile-x86.tsv')) ?? [];
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 원본 도구의 KEYS 순서와 일치하는 독립 입력 열 번호이다. (2026-11-08 WB04-E126)
const COLUMNS = ['Kind', 'X', 'Y', 'HotX', 'HotY', 'TypeHotX', 'TypeHotY', 'EndX', 'EndY', 'Fraction', 'TargetState', 'Previous', 'Corner', 'Range', 'FiringSound', 'DisplaySound', 'ImpactSound', 'Directions', 'Frames', 'Extra', 'Passed', 'SourceFrame', 'Side', 'Output'];
const C = Object.fromEntries(COLUMNS.map((name, index) => [name, index]));

// 원본의 세 자산과 같은 raw 저장소·타입·공간 경계를 구성한 장면이다. (2026-11-08 WB04-E126)
function makeScene() {
    const scene = {
        pool: new SidPool(24000, { server: false }),
        types: Array.from({ length: 188 }, () => ({ footX: 0, footY: 0, flags1: 0, flags2: 0, group: 0, hotspotX: 0, hotspotY: 0, maxHitPoints: 0 })),
        missileTypes: Array.from({ length: 188 }, () => ({ range: 0, frameCount: 0, firingSound: 0, impactSound: 0, damagePerSecond: 0, specialFlyerDamage: false, flyerDamagePerSecond: 0 })),
        geometryTypes: Array.from({ length: 188 }, () => ({ defaultFrame: 0, frameCount: 0, directions: 0, shotOffsets: [], heightExtension: 0, range: 0 })),
        frameCodes: Array.from({ length: 188 }, () => new FrameCodeTable([])),
        windType: 140,
        state: new MissileState(),
        fields: new MissileFields(),
        events: [],
        input: null
    };
    scene.gunMode = new RawGunMode(scene.pool, scene.frameCodes, () => scene.windType);
    // 실제 입력 번호를 유지하도록 슬롯을 미리 확보한다.
    for (let id = 5; id <= 100; id += 1) assert.equal(scene.pool.allocate(2), id);
    scene.n = (column) => Number(scene.input[C[column]]);
    scene.f = (column) => bitsToFloat(scene.n(column));
    // native 실행기와 동일한 독립 이미지와 메타데이터를 구성한다.
    scene.prepare = (row) => {
        assert.equal(row.length, C.Output + 4);
        scene.input = row;
        scene.events = [];
        scene.state = new MissileState({ now: scene.n('Kind') === 0 ? 10.25 : 10 + scene.n('Fraction') * 0.5 });
        const bytes = scene.fields.bytes;
        // 원본 생성자가 쓰지 않는 바이트도 동일하게 비교한다.
        for (let at = 0; at < bytes.length; at += 1) bytes[at] = (at * 13 + 7) & 0xff;
        // 원본의 source/target/display 세 슬롯을 동일하게 구성한다. 현재 source 문자는 별도 프레임 입력이다.
        for (const [sid, type] of [[50, 131], [60, 71], [90, 132]]) {
            const raw = scene.pool.physical(sid);
            raw.fill(0);
            put(raw, 0, 0x11319000);
            raw[10] = type;
            raw[11] = sid === 60 ? scene.n('TargetState') : 0;
            raw[34] = 1;
            raw[40] = (sid === 50 || sid === 60) ? scene.n('Extra') : 0;
            put(raw, 14, sid === 50 ? scene.n('X') : floatToBits(50.0));
            put(raw, 18, sid === 50 ? scene.n('Y') : floatToBits(50.0));
        }
        scene.types[131].group = 1;
        scene.types[131].hotspotX = Number(row[C.TypeHotX]) | 0;
        scene.types[131].hotspotY = Number(row[C.TypeHotY]) | 0;
        scene.missileTypes[131].range = scene.f('Range');
        scene.missileTypes[131].firingSound = scene.n('FiringSound');
        scene.missileTypes[132].firingSound = scene.n('DisplaySound');
        scene.missileTypes[132].impactSound = scene.n('ImpactSound');
        scene.missileTypes[132].frameCount = Number(row[C.Directions]) | 0;
        scene.missileTypes[136].impactSound = scene.n('ImpactSound');
        scene.geometryTypes[131].defaultFrame = 0;
        scene.geometryTypes[131].frameCount = 36;
        scene.geometryTypes[131].shotOffsets = Array.from({ length: 36 }, () => [Number(row[C.HotX]) | 0, Number(row[C.HotY]) | 0]);
        scene.geometryTypes[132].directions = Number(row[C.Directions]) | 0;
        scene.geometryTypes[132].frameCount = Number(row[C.Frames]) | 0;
        put(scene.pool.physical(50), 36, scene.n('SourceFrame'));
        scene.types[131].footX = 0;
        scene.types[131].footY = 0;
        scene.types[71].footX = 2;
        scene.types[71].footY = 3;
        scene.geometryTypes[131].range = scene.f('Range');
        const codes = Array.from({ length: 36 }, () => ({ side: 0, variant: 0, flags: 0 }));
        codes[scene.n('SourceFrame')].side = scene.n('Side');
        scene.frameCodes[131] = new FrameCodeTable(codes);
        if (scene.n('Kind') !== 1) return;
        put(bytes, 0, 0x50d630);
        put(bytes, 4, 81);
        put(bytes, 8, 90);
        put(bytes, 12, 0);
        put(bytes, 16, scene.n('Passed'));
        put(bytes, 20, 50);
        put(bytes, 24, 60);
        put(bytes, 28, scene.n('Corner'));
        // 독립 frame 입력은 ctor 결과가 아닌 선언한 두 시각/좌표/상자다.
        for (let offset = 32; offset < 48; offset += 4) put(bytes, offset, 0);
        put(bytes, 48, floatToBits(20.0));
        put(bytes, 52, 0);
        put(bytes, 56, scene.n('X'));
        put(bytes, 60, scene.n('Y'));
        put(bytes, 64, scene.n('EndX'));
        put(bytes, 68, scene.n('EndY'));
        put(bytes, 72, scene.n('EndX'));
        put(bytes, 76, scene.n('EndY'));
        // 원본 무효 사각형과 이전 위치의 독립 입력이다.
        for (let offset = 88; offset < 104; offset += 4) put(bytes, offset, 0xc61c4000);
        const time = new DataView(new ArrayBuffer(16));
        time.setFloat64(0, 12.0, true);
        time.setFloat64(8, 10.0, true);
        put(bytes, 104, time.getUint32(0, true));
        put(bytes, 108, time.getUint32(4, true));
        put(bytes, 112, time.getUint32(8, true));
        put(bytes, 116, time.getUint32(12, true));
        put(bytes, 120, floatToBits(50.0));
        put(bytes, 124, scene.n('Previous') ? floatToBits(89.0) : 0xc61c4000);
    };
    // 원본 외부 효과만 관찰하며 포구·SHP·방향 수치는 이식된 제품 모듈을 사용한다.
    scene.hooks = (geometry) => {
        const bits = (value) => floatToBits(value);
        const sound = (x, y, token) => scene.events.push(`S:${bits(x)}:${bits(y)}:${token}:0:0`);
        return {
            muzzle: (sid) => geometry.muzzle(sid),
            hotspot: (sid) => { scene.events.push('H:50'); return geometry.hotspot(sid); },
            targetDimensions: (sid) => geometry.dimensions(sid),
            rayEnd: (from, to, sid) => geometry.cannonRayEnd(from, to, sid),
            directionFrame: (from, to, type) => geometry.directionFrame(from, to, type),
            create: (type, flags) => { scene.events.push(`A:${type}:${flags}`); return 90; },
            owner: (sid, owner) => { scene.pool.physical(sid)[34] = owner; scene.events.push(`O:90:${owner}`); },
            pop: (sid, x, y, flags) => {
                put(scene.pool.physical(sid), 14, bits(x));
                put(scene.pool.physical(sid), 18, bits(y));
                scene.events.push(`P:90:${bits(x)}:${bits(y)}:${flags}`);
            },
            unpop: (sid, flags) => scene.events.push(`U:90:${flags}`),
            firingSound: sound,
            impactSound: sound,
            center: (sid) => {
                const raw = scene.pool.slot(sid);
                return [bitsToFloat(get(raw, 14)), bitsToFloat(get(raw, 18))];
            },
            targetBounds: (sid, at) => { scene.events.push(`T:${bits(at[0])}:${bits(at[1])}`); return [49, 89, 51, 91]; },
            hit: (firing, target) => scene.events.push(`G:${firing}:${target}`),
            hitEffect: (x, y) => scene.events.push(`H:${bits(x)}:${bits(y)}`),
            refresh: () => scene.events.push('Q:50'),
            impact: (x, y, mode, payload) => scene.events.push(`I:${bits(x)}:${bits(y)}:${mode}:${bits(payload)}`),
            destroy: (sid, flags) => scene.events.push(`X:90:${flags}`)
        };
    };
    // 실제 제품의 포구/생성/비행을 실행하고 전체 관찰 이미지를 직렬화한다.
    scene.run = () => {
        const geometry = new RawProjectileGeometry(scene.pool, scene.types, scene.geometryTypes, 16.0, 11.0, scene.gunMode);
        const logic = new RawMissile(scene.pool, scene.types, scene.missileTypes, scene.state, scene.hooks(geometry));
        let point = [0, 0];
        const kind = scene.n('Kind');
        if (kind === 0) {
            const targetPosition = [scene.f('EndX'), scene.f('EndY')];
            const request = {
                firing: 50,
                target: 60,
                speed: 20,
                displayType: 132,
                effectType: 0,
                targetPosition,
                targetCenter: scene.n('Corner') ? targetPosition : [-10000, -10000],
                ricochet: 0,
                firingPosition: [-10000, -10000]
            };
            const asset = logic.construct(scene.fields, request);
            scene.events.push('B:28:90:0:16');
            put(scene.fields.bytes, 4, 81);
            put(scene.fields.bytes, 8, asset);
        } else if (kind === 1) logic.frame(scene.fields);
        else {
            const result = kind === 2 ? geometry.muzzle(50) : geometry.cannonRayEnd([scene.f('X'), scene.f('Y')], [scene.f('EndX'), scene.f('EndY')], 50);
            point = [floatToBits(result[0]), floatToBits(result[1])];
        }
        let slots = '';
        // 원본의 세 슬롯 전체 150바이트를 같은 순서로 비교한다.
        for (const sid of [50, 60, 90]) slots += hex(scene.pool.slot(sid));
        const output = new Uint8Array(8);
        put(output, 0, point[0]);
        put(output, 4, point[1]);
        return [scene.events.join(';'), hex(scene.fields.bytes), slots, hex(output)];
    };
    return scene;
}

// 한 원본 몸체 종류의 모든 고유 행을 기대값 변경 없이 대조한다. (2026-11-08 WB04-E126)
function replay(kind) {
    const scene = makeScene();
    let count = 0;
    // 입력마다 전체 raw/효과/포구 결과를 검사한다.
    for (const row of rows) {
        if (Number(row[C.Kind]) !== kind) continue;
        scene.prepare(row);
        assert.deepEqual(scene.run(), row.slice(C.Output), `ThunderMissile kind ${kind} 행 ${count}`);
        count += 1;
    }
    return count;
}

// 실제 원본 보고서의 종류별 고유 입력 수이다. (2026-11-08 WB04-E126)
const EXPECTED = [320, 454, 8, 336];

test('ThunderMissile: 현재 SHP/type hotspot·실제 종점과 132 방향/사운드 우선순위(생성자)가 같다', { skip: rows.length === 0 && '원본 fixture 없음' }, () => {
    assert.equal(replay(0), EXPECTED[0]);
});

test('ThunderMissile: 독립 비행 보간·일반/코너 충돌·사거리 종료·효과음·삭제(프레임)가 같다', { skip: rows.length === 0 && '원본 fixture 없음' }, () => {
    assert.equal(replay(1), EXPECTED[1]);
});

test('ThunderMissile: TYPE 131 의 실제 가상 포구가 원본 raw 좌표 두 DWORD 를 복사한다', { skip: rows.length === 0 && '원본 fixture 없음' }, () => {
    assert.equal(replay(2), EXPECTED[2]);
});

test('ThunderMissile: 모든 네 방향·signed 문자 fallback·불일치 사거리/클램프를 실제 종점과 비교한다', { skip: rows.length === 0 && '원본 fixture 없음' }, () => {
    assert.equal(replay(3), EXPECTED[3]);
});

// 썬더 충돌 인자와 효과/소리/명령/피해/삭제의 순서를 원본 688개 전체 이미지와 비교하는 장면 확장이다. (2026-11-08 WB04-E126)
const hitRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'thunder-hit-event-x86.tsv')) ?? [];
const floaterRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'missile-floater-x86.tsv')) ?? [];

// 새 세 입력 열(표적 타입·표시 타입·발사자 상태)을 적용하고 실제 frame 의 전체 필드/슬롯/외부 효과를 반환한다. (2026-11-08 WB04-E126)
function runHitEvent(scene, row, connectQueue = true) {
    assert.equal(row.length, C.Output + 7);
    // 기존 23개 입력 형식을 유지한다.
    scene.prepare([...row.slice(0, C.Output), ...row.slice(C.Output + 3)]);
    // 원본 명령은 SID 가 아니라 이 두 타입을 보낸다.
    const target = Number(row[C.Output]);
    const display = Number(row[C.Output + 1]);
    scene.pool.physical(60)[10] = target;
    scene.pool.physical(90)[10] = display;
    scene.pool.physical(50)[11] = Number(row[C.Output + 2]);
    scene.types[target].footX = 2;
    scene.types[target].footY = 3;
    scene.missileTypes[display].impactSound = scene.n('ImpactSound');
    const geometry = new RawProjectileGeometry(scene.pool, scene.types, scene.geometryTypes, 16.0, 11.0, scene.gunMode);
    // 숫자·충돌 몸체는 제품 구현이며 효과만 외부 관측 경계다.
    const hooks = scene.hooks(geometry);
    if (connectQueue) hooks.queueCommand = (command, first, second) => scene.events.push(`E:${command}:${first}:${second}`);
    new RawMissile(scene.pool, scene.types, scene.missileTypes, scene.state, hooks).frame(scene.fields);
    let slots = '';
    // 같은 세 raw 슬롯 전체를 직렬화해 부수 변경도 검사한다.
    for (const sid of [50, 60, 90]) slots += hex(scene.pool.slot(sid));
    return [scene.events.join(';'), hex(scene.fields.bytes), slots, '0000000000000000'];
}

test('ThunderMissile: 충돌 인자와 효과/소리/명령/피해/삭제의 순서가 원본 688개 전체 이미지와 같고 명령 큐 누락은 거부한다', { skip: hitRows.length === 0 && '원본 fixture 없음' }, () => {
    const scene = makeScene();
    let count = 0;
    let eventCount = 0;
    // 음성·코너·free/dead·실패 타입·통과/충돌 억제·사거리 종료도 모두 검사한다.
    for (const row of hitRows) {
        const actual = runHitEvent(scene, row);
        const expected = row.slice(C.Output + 3);
        assert.deepEqual(actual, expected, `ThunderHitEvent 행 ${count}`);
        count += 1;
        if (expected[0].includes('E:')) eventCount += 1;
    }
    assert.equal(count, 688);
    assert.equal(eventCount, 24);
    // 빈 성공으로 명령을 버리지 않는지 별도로 검사한다.
    const event = hitRows.find((row) => row[C.Output + 3].includes('E:'));
    assert.ok(event);
    assert.throws(() => runHitEvent(scene, event, false), Error);
});

// 기존 독립 frame 장면에 현재 플로터 전역/같은 게임 난수 입력만 더한 회귀 장면이다. (2026-11-08 WB04-E126)
const FLOATER_INPUT = C.Output + 6;
function makeFloaterScene() {
    const scene = makeScene();
    scene.random = new GameRandom(0);
    // 실제 투사체 frame 을 실행하며 같은 GameRandom 을 어댑터와 공유한다.
    scene.frameOnce = (connectRandom = true) => {
        const geometry = new RawProjectileGeometry(scene.pool, scene.types, scene.geometryTypes, 16.0, 11.0, scene.gunMode);
        const hooks = scene.hooks(geometry);
        // 반사 생성은 Native 와 같은 명시 경계다.
        hooks.ricochet = () => scene.events.push('R:90:60');
        if (connectRandom) {
            hooks.randomFloat = (low, high) => {
                scene.events.push(`N:${floatToBits(low)}:${floatToBits(high)}`);
                return low + (scene.random.next(65536) * (1.0 / 65536.0)) * Math.abs(high - low);
            };
        }
        new RawMissile(scene.pool, scene.types, scene.missileTypes, scene.state, hooks).frame(scene.fields);
    };
    // 기대값은 원문 행에서 읽고 준비 단계는 동일 입력만 설치한다.
    scene.runFloater = (row, connectRandom = true) => {
        if (row.length !== FLOATER_INPUT + 5) throw new Error('플로터 투사체 원문 열 수 오류');
        scene.prepare([...row.slice(0, C.Output), ...row.slice(FLOATER_INPUT, FLOATER_INPUT + 4)]);
        const source = Number(row[C.Output + 4]);
        const display = Number(row[C.Output + 5]);
        scene.types[source] = { ...scene.types[131], group: (source === 71 || source === 140) ? 0 : 1 };
        scene.missileTypes[source] = { ...scene.missileTypes[131] };
        scene.missileTypes[display] = { ...scene.missileTypes[132] };
        scene.pool.physical(50)[10] = source;
        scene.pool.physical(60)[10] = Number(row[C.Output]);
        scene.pool.physical(90)[10] = display;
        scene.state.floaterType = Number(row[C.Output + 1]);
        scene.state.avoidanceCutoff = Number(row[C.Output + 2]) | 0;
        scene.random.setState(Number(row[C.Output + 3]));
        scene.frameOnce(connectRandom);
        let slots = '';
        // 세 슬롯 전체를 원문과 대조한다.
        for (const sid of [50, 60, 90]) slots += hex(scene.pool.slot(sid));
        return [scene.events.join(';'), hex(scene.fields.bytes), slots, '0000000000000000', String(scene.random.state)];
    };
    return scene;
}

test('MissileFloater: 회피/일반 적중·코너·범위 종료·난수 미선택과 다섯 발사자의 전체 상태가 원문 481행과 같다', { skip: floaterRows.length === 0 && '원본 fixture 없음' }, () => {
    const scene = makeFloaterScene();
    let count = 0;
    // 같은 독립 입력만 설치하며 원문 기대 필드를 변경하지 않는다.
    for (const row of floaterRows) {
        assert.deepEqual(scene.runFloater(row), row.slice(FLOATER_INPUT), `플로터 투사체 사례 ${count + 1}`);
        count += 1;
    }
    assert.equal(count, 481);
});

test('MissileFloater: 회피 뒤 두 번째 frame 은 통과 표시로 난수를 생략하고 누락된 난수 경계는 선행 이동을 남긴다', { skip: floaterRows.length === 0 && '원본 fixture 없음' }, () => {
    const selected = floaterRows.find((row) => Number(row[C.Output]) === 146 && Number(row[C.Output + 1]) === 146 && row[C.Output + 2] === '800' && Number(row[C.Output + 3]) === 0xfed
        && Number(row[C.Output + 4]) === 131 && Number(row[C.Fraction]) === 4 && !Number(row[C.TargetState]) && !Number(row[C.Corner]) && !Number(row[C.Passed]));
    assert.ok(selected);
    const scene = makeFloaterScene();
    scene.runFloater(selected);
    assert.equal(get(scene.fields.bytes, 16), 1);
    const saved = scene.random.state;
    scene.state.now += 0.01;
    scene.frameOnce();
    assert.equal(scene.random.state, saved);
    assert.equal(scene.events.filter((event) => event === 'N:0:1148846080').length, 1);
    assert.throws(() => scene.runFloater(selected, false), Error);
    assert.equal(get(scene.fields.bytes, 16), 0);
    assert.equal(scene.random.state, 0xfed);
    assert.ok(scene.events.includes('U:90:0'));
});
