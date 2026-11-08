// 썬 디스크 발사기(SunArcher 71)의 sunDisc 83 Missile raw-missile.js(RawMissile 생성자·프레임)를 원본 10.78 관측과 대조한다.
//  - 원본 관측: archermissile-x86.tsv 의 `originals` 행 — 몸체(0 생성자 / 1 프레임)와 포구·hotspot·종점·시각·표적 상태·코너·사거리·소리 입력으로 외부 효과 순서(M 포구/H hotspot/D 표적 치수/R 사정선/A 자산/O 소유자/P·U 공간/E 비행 효과/S 소리/C 중심/T 표적 사각형/G 직접 피해/H 충돌 효과/Q 갱신/I 충돌 표시/X 삭제/B 부착),
//    128바이트 필드 이미지, 표시 자산 슬롯 50바이트가 같다. `originalCD` 행은 10.72(0x68 필드)라 사용하지 않는다.
//  - 계약은 이후 단위(크로스보우·썬 캐논·썬더·아이스)의 같은 모듈 시험과 함께 확인한다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E125: RawMissile 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, bitsToFloat, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { MissileFields } from '../www/core/raw-missile-fields.js';
import { MissileState, RawMissile } from '../www/core/raw-missile.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E125)
const allRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'archermissile-x86.tsv')) ?? [];
const rows = allRows.filter((row) => row[0] === 'originals');
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 숫자 입력·raw 필드·관찰 사건을 같은 풀에 준비하는 장면이다. (2026-11-08 WB04-E125)
function makeScene() {
    const scene = {
        pool: new SidPool(24000, { server: false }),
        types: Array.from({ length: 188 }, () => ({ footX: 0, footY: 0, flags1: 0, flags2: 0, group: 0, hotspotX: 0, hotspotY: 0, maxHitPoints: 0 })),
        missileTypes: Array.from({ length: 188 }, () => ({ range: 0, frameCount: 0, firingSound: 0, impactSound: 0, damagePerSecond: 0, specialFlyerDamage: false, flyerDamagePerSecond: 0 })),
        state: new MissileState(),
        fields: new MissileFields(),
        events: [],
        input: null
    };
    // 실제 입력 번호 5~100 의 슬롯을 유지한다.
    for (let id = 5; id <= 100; id += 1) assert.equal(scene.pool.allocate(2), id);
    scene.n = (index) => Number(scene.input[index]);
    scene.f = (index) => bitsToFloat(scene.n(index));
    // 고유 입력의 타입·자산·padding 을 준비한다.
    scene.prepare = (row) => {
        assert.equal(row.length, 20);
        scene.input = row;
        scene.events = [];
        scene.state = new MissileState({ now: scene.n(1) === 0 ? 10.25 : 10 + scene.n(10) * 0.5 });
        const bytes = scene.fields.bytes;
        // 생성자 padding 과 판본 밖의 메모리는 원본 실행기와 같은 패턴이다.
        for (let at = 0; at < bytes.length; at += 1) bytes[at] = (at * 13 + 7) & 0xff;
        // native 세 자산의 같은 raw 필드를 구성한다.
        for (const [sid, type] of [[50, 71], [60, 71], [90, 83]]) {
            const raw = scene.pool.physical(sid);
            raw.fill(0);
            put(raw, 0, 0x11319000);
            raw[10] = type;
            raw[11] = sid === 60 ? scene.n(11) : 0;
            raw[34] = 1;
            put(raw, 14, 0x42480000);
            put(raw, 18, 0x42480000);
        }
        scene.types[71].group = 0;
        scene.types[71].hotspotX = scene.n(6) | 0;
        scene.types[71].hotspotY = scene.n(7) | 0;
        scene.missileTypes[71].range = scene.f(14);
        scene.missileTypes[71].firingSound = scene.n(15);
        scene.missileTypes[83].frameCount = 1;
        scene.missileTypes[83].impactSound = scene.n(16);
        if (scene.n(1) === 0) return;
        put(bytes, 0, 0x50d630);
        put(bytes, 4, 81);
        put(bytes, 8, 90);
        put(bytes, 12, 0);
        put(bytes, 16, 0);
        put(bytes, 20, 50);
        put(bytes, 24, 60);
        put(bytes, 28, scene.n(13));
        put(bytes, 48, 0x420c0000);
        put(bytes, 52, 0);
        put(bytes, 56, scene.n(2));
        put(bytes, 60, scene.n(3));
        put(bytes, 64, scene.n(8));
        put(bytes, 68, scene.n(9));
        put(bytes, 72, scene.n(8));
        put(bytes, 76, scene.n(9));
        // 실제 ctor 가 아닌 독립 frame 입력의 box/time/이전 좌표를 쓴다.
        for (let offset = 88; offset < 104; offset += 4) put(bytes, offset, 0xc61c4000);
        const time = new DataView(new ArrayBuffer(16));
        time.setFloat64(0, 12.0, true);
        time.setFloat64(8, 10.0, true);
        put(bytes, 104, time.getUint32(0, true));
        put(bytes, 108, time.getUint32(4, true));
        put(bytes, 112, time.getUint32(8, true));
        put(bytes, 116, time.getUint32(12, true));
        put(bytes, 120, 0x42480000);
        put(bytes, 124, scene.n(12) ? floatToBits(89.0) : 0xc61c4000);
        put(bytes, 32, 0);
        put(bytes, 36, 0);
        put(bytes, 40, 0);
        put(bytes, 44, 0);
    };
    // 원본 외부 자료/자산/Pop/effect/삭제 경계를 같은 인자로 관찰한다.
    scene.hooks = () => {
        const bits = (value) => floatToBits(value);
        return {
            muzzle: () => { scene.events.push('M:50'); return [scene.f(2), scene.f(3)]; },
            hotspot: () => { scene.events.push('H:50'); return [scene.n(4) | 0, scene.n(5) | 0]; },
            targetDimensions: () => { scene.events.push('D:60'); return [2, 3]; },
            rayEnd: (from, to, firing) => {
                scene.events.push(`R:${bits(from[0])},${bits(from[1])},${bits(to[0])},${bits(to[1])},${firing}`);
                return [scene.f(8), scene.f(9)];
            },
            create: (type, flags) => { scene.events.push(`A:${type}:${flags}`); return 90; },
            owner: (sid, owner) => { scene.pool.allocated(sid)[34] = owner; scene.events.push(`O:90:${owner}`); },
            pop: (sid, x, y, flags) => {
                put(scene.pool.allocated(sid), 14, bits(x));
                put(scene.pool.allocated(sid), 18, bits(y));
                scene.events.push(`P:90:${bits(x)}:${bits(y)}:${flags}`);
            },
            unpop: (sid, flags) => scene.events.push(`U:90:${flags}`),
            flightEffect: (type, asset) => scene.events.push(`E:${type}:${asset}:0`),
            // 실제 발사와 충돌은 같은 cdecl 사운드 토큰 경계 인자를 사용한다.
            firingSound: (x, y, token) => scene.events.push(`S:${bits(x)}:${bits(y)}:${token}:0:0`),
            center: () => { scene.events.push('C:50'); return [50, 50]; },
            targetBounds: (sid, at) => { scene.events.push(`T:${bits(at[0])}:${bits(at[1])}`); return [49, 89, 51, 91]; },
            hit: (firing, target) => scene.events.push(`G:${firing}:${target}`),
            hitEffect: (x, y) => scene.events.push(`H:${bits(x)}:${bits(y)}`),
            refresh: () => scene.events.push('Q:50'),
            impact: (x, y, mode, payload) => scene.events.push(`I:${bits(x)}:${bits(y)}:${mode}:${bits(payload)}`),
            impactSound: (x, y, token) => scene.events.push(`S:${bits(x)}:${bits(y)}:${token}:0:0`),
            destroy: (sid, flags) => scene.events.push(`X:90:${flags}`)
        };
    };
    // 같은 실제 숫자 몸체를 실행하고 관찰 결과를 직렬화한다.
    scene.run = () => {
        const logic = new RawMissile(scene.pool, scene.types, scene.missileTypes, scene.state, scene.hooks());
        if (scene.n(1) === 0) {
            const request = {
                firing: 50,
                target: 60,
                speed: 35,
                displayType: 83,
                effectType: 0,
                targetPosition: [100, 90],
                targetCenter: scene.n(13) ? [100, 90] : [-10000, -10000],
                ricochet: 0,
                firingPosition: [0, 0]
            };
            const asset = logic.construct(scene.fields, request);
            scene.events.push('B:28:90:0:16');
            put(scene.fields.bytes, 4, 81);
            put(scene.fields.bytes, 8, asset);
        } else logic.frame(scene.fields);
        return [scene.events.join(';'), hex(scene.fields.bytes), hex(scene.pool.slot(90))];
    };
    return scene;
}

test('ArcherMissile: 원본 10.78 의 0x80 필드 생성자와 프레임 숫자·효과 순서·표시 슬롯이 같다', { skip: rows.length === 0 && '원본 fixture 없음' }, () => {
    const scene = makeScene();
    let count = 0;
    // 관찰 행마다 전체 raw 필드/자산/사건 순서를 비교한다.
    for (const row of rows) {
        scene.prepare(row);
        assert.deepEqual(scene.run(), row.slice(17), `Missile 행 ${count}: body ${row[1]} ${row.slice(2, 17).join(',')}`);
        count += 1;
    }
    assert.equal(count, allRows.length / 2);
});
