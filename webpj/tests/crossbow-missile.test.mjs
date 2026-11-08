// 크로스보우(TYPE 140)의 arrow 81 Missile raw-missile.js(RawMissile)를 원본 10.78 관측과 대조한다.
//  - 원본 관측: crossbow-missile-x86.tsv 703행 — 몸체(0 생성자 / 1 프레임 / 2 가상 포구)와 포구·SHP/타입 hotspot·종점·시각 비율·표적 상태·이전 좌표·코너·사거리·소리·방향 수/프레임 수·표적 추가 비트·통과 표시 입력으로
//    외부 효과 순서(H hotspot/D 표적 치수/F 방향 프레임/A 자산/O 소유자/P·U 공간/S 소리/C 중심/T 표적 사각형/G 직접 피해/H 충돌 효과/Q 갱신/I 충돌 표시/X 삭제/B 부착), 128바이트 필드 이미지, 세 슬롯 150바이트,
//    가상 포구 8바이트가 같다. 포구/hotspot/방향 수치는 이식된 RawProjectileGeometry 를 그대로 쓴다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E125: RawMissile 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, bitsToFloat, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { MissileFields } from '../www/core/raw-missile-fields.js';
import { MissileState, RawMissile } from '../www/core/raw-missile.js';
import { RawProjectileGeometry } from '../www/core/raw-projectile-geometry.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E125)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'crossbow-missile-x86.tsv')) ?? [];
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 원본 도구의 KEYS 순서와 일치하는 독립 입력 열 번호이다. (2026-11-08 WB04-E125)
const COLUMNS = ['Kind', 'X', 'Y', 'HotX', 'HotY', 'TypeHotX', 'TypeHotY', 'EndX', 'EndY', 'Fraction', 'TargetState', 'Previous', 'Corner', 'Range', 'FiringSound', 'DisplaySound', 'ImpactSound', 'Directions', 'Frames', 'Extra', 'Passed', 'Output'];
const C = Object.fromEntries(COLUMNS.map((name, index) => [name, index]));

// 원본의 세 자산과 같은 raw 저장소·타입·공간 경계를 구성한 장면이다. (2026-11-08 WB04-E125)
function makeScene() {
    const scene = {
        pool: new SidPool(24000, { server: false }),
        types: Array.from({ length: 188 }, () => ({ footX: 0, footY: 0, flags1: 0, flags2: 0, group: 0, hotspotX: 0, hotspotY: 0, maxHitPoints: 0 })),
        missileTypes: Array.from({ length: 188 }, () => ({ range: 0, frameCount: 0, firingSound: 0, impactSound: 0, damagePerSecond: 0, specialFlyerDamage: false, flyerDamagePerSecond: 0 })),
        geometryTypes: Array.from({ length: 188 }, () => ({ defaultFrame: 0, frameCount: 0, directions: 0, shotOffsets: [], heightExtension: 0, range: 0 })),
        state: new MissileState(),
        fields: new MissileFields(),
        events: [],
        input: null
    };
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
        // 원본의 source/target/display 세 슬롯을 동일하게 구성한다.
        for (const [sid, type] of [[50, 140], [60, 71], [90, 81]]) {
            const raw = scene.pool.physical(sid);
            raw.fill(0);
            put(raw, 0, 0x11319000);
            raw[10] = type;
            raw[11] = sid === 60 ? scene.n('TargetState') : 0;
            raw[34] = 1;
            raw[40] = sid === 60 ? scene.n('Extra') : 0;
            put(raw, 14, sid === 50 ? scene.n('X') : floatToBits(50.0));
            put(raw, 18, sid === 50 ? scene.n('Y') : floatToBits(50.0));
        }
        scene.types[140].group = 0;
        scene.types[140].hotspotX = Number(row[C.TypeHotX]) | 0;
        scene.types[140].hotspotY = Number(row[C.TypeHotY]) | 0;
        scene.missileTypes[140].range = scene.f('Range');
        scene.missileTypes[140].firingSound = scene.n('FiringSound');
        scene.missileTypes[81].firingSound = scene.n('DisplaySound');
        scene.missileTypes[81].impactSound = scene.n('ImpactSound');
        scene.missileTypes[81].frameCount = Number(row[C.Directions]) | 0;
        scene.missileTypes[136].impactSound = scene.n('ImpactSound');
        scene.geometryTypes[140].defaultFrame = 0;
        scene.geometryTypes[140].frameCount = 1;
        scene.geometryTypes[140].shotOffsets = [[Number(row[C.HotX]) | 0, Number(row[C.HotY]) | 0]];
        scene.geometryTypes[81].directions = Number(row[C.Directions]) | 0;
        scene.geometryTypes[81].frameCount = Number(row[C.Frames]) | 0;
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
        put(bytes, 48, floatToBits(35.0));
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
            targetDimensions: () => { scene.events.push('D:60'); return [2, 3]; },
            directionFrame: (from, to, type) => {
                scene.events.push(`F:${bits(from[0])}:${bits(from[1])}:${bits(to[0])}:${bits(to[1])}:${type}`);
                return geometry.directionFrame(from, to, type);
            },
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
            center: () => { scene.events.push('C:50'); return [50, 50]; },
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
        const geometry = new RawProjectileGeometry(scene.pool, scene.types, scene.geometryTypes);
        const logic = new RawMissile(scene.pool, scene.types, scene.missileTypes, scene.state, scene.hooks(geometry));
        let point = [0, 0];
        const kind = scene.n('Kind');
        if (kind === 0) {
            const targetPosition = [scene.f('EndX'), scene.f('EndY')];
            const request = {
                firing: 50,
                target: 60,
                speed: 35,
                displayType: 81,
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
            const muzzle = geometry.muzzle(50);
            point = [floatToBits(muzzle[0]), floatToBits(muzzle[1])];
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

// 한 원본 몸체 종류의 모든 고유 행을 기대값 변경 없이 대조한다. (2026-11-08 WB04-E125)
function replay(kind) {
    const scene = makeScene();
    let count = 0;
    // 입력마다 전체 raw/효과/포구 결과를 검사한다.
    for (const row of rows) {
        if (Number(row[C.Kind]) !== kind) continue;
        scene.prepare(row);
        assert.deepEqual(scene.run(), row.slice(C.Output), `CrossbowMissile kind ${kind} 행 ${count}`);
        count += 1;
    }
    return count;
}

test('CrossbowMissile: 현재 SHP/type hotspot 두 단계와 arrow 81 실제 방향/사운드 우선순위(생성자)가 같다', { skip: rows.length === 0 && '원본 fixture 없음' }, () => {
    assert.ok(replay(0) > 0);
});

test('CrossbowMissile: 독립 비행 보간·일반/코너 충돌·사거리 종료·효과음·삭제(프레임)가 같다', { skip: rows.length === 0 && '원본 fixture 없음' }, () => {
    assert.ok(replay(1) > 0);
});

test('CrossbowMissile: TYPE 140 의 실제 가상 포구가 원본 raw 좌표 두 DWORD 를 복사한다', { skip: rows.length === 0 && '원본 fixture 없음' }, () => {
    assert.ok(replay(2) > 0);
    assert.equal(rows.length, 703);
});
