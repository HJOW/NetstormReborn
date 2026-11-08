// 사제 도착 raw-priest-arrival.js(RawPriestArrival)의 중립 회복/내용물 부착·삭제 순서와 Carrier 도착 위임을 cpppj 계약으로 검사한다.
//  - 이 몸체에는 독립 원본 관측 fixture 가 없다(cpppj 시험은 실제 경로 프로세스/Host 전체 통합이며 그 사슬은 경로 프로세스 단위에서 다룬다). 그래서 같은 풀의 하위 조회(현재 HP·최대 HP·지역·섬 Temple·발자국)와
//    큰 효과(내용물 생성·자기 삭제·귀환·Carrier 도착)를 기록 경계로 두고 원본 분기 순서와 값을 직접 검증한다: 저 HP 중립의 섬 복귀(내용물 생성 → 자기 삭제 → 1001), 절반 이상 중립 귀환(1001), 그 밖의 Carrier 위임(인자 전달).
//    거리 helper 는 축별 signed 거리의 큰 쪽과 1.0 의 포함 비교이며 발자국 안의 음수 거리도 그대로 유지한다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E162: 사제 도착 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import { put, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { RawPriestArrival, createPriestArrivalState } from '../www/core/raw-priest-arrival.js';

// 풀 크기, 사제/Temple 번호, 사제 타입, 섬 번호이다. (2026-11-09 WB04-E162)
const CAPACITY = 24000;
const PRIEST = 50;
const TEMPLE = 60;
const TYPE = 158;
const ISLAND = 3;

// 예외가 나는지 본다. (2026-11-09 WB04-E162)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 같은 풀의 하위 조회와 큰 효과 기록 경계를 가진 도착 시험 장면이다. (2026-11-09 WB04-E162)
function makeScene() {
    const pool = new SidPool(CAPACITY, { server: true });
    const scene = {
        pool, hp: 150, maxHp: 200, region: ISLAND, temple: TEMPLE, rect: { left: 10, top: 10, right: 12, bottom: 12 }, events: [],
        state: createPriestArrivalState()
    };
    const priestRaw = pool.physical(PRIEST);
    priestRaw.fill(0);
    priestRaw[10] = TYPE;
    priestRaw[34] = 0;
    put(priestRaw, 12, 0x8101, 2);
    put(priestRaw, 14, floatToBits(13));
    put(priestRaw, 18, floatToBits(11));
    const templeRaw = pool.physical(TEMPLE);
    templeRaw.fill(0);
    templeRaw[34] = 0;
    scene.priest = { pool, currentHitPoints: () => scene.hp };
    scene.hitPoints = { pool, maxHitPoints: () => scene.maxHp };
    scene.terrain = { pool, regionAt: (x, y) => { scene.events.push(`R:${x},${y}`); return scene.region; } };
    scene.islands = { pool, temple: (island) => { scene.events.push(`T:${island}`); return scene.temple; } };
    scene.footprint = { pool, footprint: (sid) => { assert.equal(sid, TEMPLE); return scene.rect; } };
    scene.create = { pool, create: (...args) => { scene.events.push(`C:${args.join(',')}`); } };
    scene.carrier = { pool, arrival: (sid, a, b) => { scene.events.push(`A:${sid},${a},${b}`); return 1000; } };
    scene.arrival = new RawPriestArrival(pool, scene.priest, scene.hitPoints, scene.terrain, scene.islands, scene.footprint, scene.create, scene.carrier, scene.state, {
        destroy: (sid, flags) => { scene.events.push(`D:${sid},${flags}`); },
        neutralReturn: (sid) => { scene.events.push(`N:${sid}`); }
    });
    return scene;
}

test('사제 도착: 저 HP 중립이 현재 섬 Temple 근처이면 내용물을 Temple 에 만들고 자기 삭제한 뒤 1001 을 돌려준다', () => {
    const scene = makeScene();
    scene.hp = 50;
    assert.equal(scene.arrival.arrival(PRIEST, 7, 8), 1001);
    assert.deepEqual(scene.events, ['R:13,11', `T:${ISLAND}`, `C:${TEMPLE},${TYPE},${0x8101},32,0,0`, `D:${PRIEST},0`]);
    // 최대 HP 미만이면 절반 미만이든 이상이든 먼저 섬 복귀를 본다.
    const second = makeScene();
    second.hp = 199;
    assert.equal(second.arrival.arrival(PRIEST, 0, 0), 1001);
    assert.ok(second.events.some((event) => event.startsWith('C:')) && second.events.at(-1) === `D:${PRIEST},0`);
});

test('사제 도착: 섬 복귀 조건이 맞지 않으면 절반 이상 중립 귀환 또는 Carrier 도착 위임으로 이어진다', () => {
    // Temple 이 없거나(지역 127) 멀면 섬 복귀를 하지 않고 HP 에 따라 귀환(절반 이상)이거나 Carrier 로 위임(절반 미만)한다.
    const none = makeScene();
    none.region = 127;
    none.hp = 150;
    assert.equal(none.arrival.arrival(PRIEST, 1, 2), 1001);
    assert.deepEqual(none.events, ['R:13,11', `N:${PRIEST}`]);
    const far = makeScene();
    far.rect = { left: 10, top: 10, right: 11, bottom: 11 };
    far.hp = 50;
    assert.equal(far.arrival.arrival(PRIEST, 1, 2), 1000);
    assert.deepEqual(far.events, ['R:13,11', `T:${ISLAND}`, `A:${PRIEST},1,2`]);
    // 최대 HP 와 같으면 섬 복귀 대신 귀환이다.
    const full = makeScene();
    full.hp = 200;
    assert.equal(full.arrival.arrival(PRIEST, 0, 0), 1001);
    assert.deepEqual(full.events, [`N:${PRIEST}`]);
    // Temple 의 소유자가 다르면 섬 복귀를 하지 않는다.
    const foreign = makeScene();
    foreign.pool.physical(TEMPLE)[34] = 2;
    foreign.hp = 50;
    assert.equal(foreign.arrival.arrival(PRIEST, 0, 0), 1000);
    assert.ok(!foreign.events.some((event) => event.startsWith('C:') || event.startsWith('D:')));
});

test('사제 도착: 권한·로딩·억제·소유자가 중립 처리를 막으면 항상 Carrier 도착으로 위임하고 인자를 그대로 전달한다', () => {
    const cases = [
        (scene) => { scene.state.boss = false; },
        (scene) => { scene.state.loading = true; },
        (scene) => { scene.state.suppressNeutralReturn = true; },
        (scene) => { scene.pool.physical(PRIEST)[34] = 1; }
    ];
    for (const change of cases) {
        const scene = makeScene();
        change(scene);
        scene.hp = 50;
        assert.equal(scene.arrival.arrival(PRIEST, 0x1234, 0x5678), 1000);
        assert.deepEqual(scene.events, [`A:${PRIEST},${0x1234},${0x5678}`]);
    }
    // 효과 뒤 현재 값을 다시 읽는다: 섬 복귀 판정 뒤 권한이 바뀌어도 이후 조건은 현재 상태를 쓴다.
    const scene = makeScene();
    scene.hp = 150;
    scene.region = 127;
    scene.terrain.regionAt = () => { scene.state.boss = false; return 127; };
    assert.equal(scene.arrival.arrival(PRIEST, 3, 4), 1000);
    assert.deepEqual(scene.events, [`A:${PRIEST},3,4`]);
});

test('사제 도착: 거리 helper 는 축별 signed 거리의 큰 쪽과 1.0 의 포함 비교이며 발자국 안의 음수 거리도 유지한다', () => {
    const near = (x, y) => {
        const scene = makeScene();
        scene.hp = 50;
        put(scene.pool.physical(PRIEST), 14, floatToBits(x));
        put(scene.pool.physical(PRIEST), 18, floatToBits(y));
        return scene.arrival.arrival(PRIEST, 0, 0) === 1001;
    };
    // 발자국 오른쪽에서 정확히 1.0 이면 가깝다(포함), 그보다 멀면 멀다.
    assert.ok(near(13, 11));
    assert.ok(!near(13.5, 11));
    // 발자국 안(음수 거리)과 아래쪽 경계도 가깝다. 두 축이 모두 멀면 큰 쪽이 기준이다.
    assert.ok(near(11, 11));
    assert.ok(near(9, 13));
    assert.ok(!near(9, 14));
    assert.ok(!near(14, 14));
});

test('사제 도착: 지역 조회·Temple SID·좌표 범위·필수 연결을 거부한다', () => {
    const scene = makeScene();
    // 지역 127(지면 없음)은 Temple 0 이다.
    scene.region = 127;
    assert.equal(scene.arrival.templeAt(13, 11), 0);
    // Temple DWORD 가 풀 용량 이상이면 거부한다.
    scene.region = ISLAND;
    scene.temple = CAPACITY;
    assert.ok(throws(() => scene.arrival.templeAt(13, 11)));
    // 비유한/범위 밖 좌표는 거부한다.
    assert.ok(throws(() => scene.arrival.templeAt(Number.NaN, 11)));
    assert.ok(throws(() => scene.arrival.templeAt(2147483648, 11)));
    // 다른 풀의 하위 모듈이나 누락된 가상 경계는 첫 효과 전에 거부한다.
    const other = new SidPool(CAPACITY, { server: true });
    const make = (override = {}) => new RawPriestArrival(scene.pool, override.priest ?? scene.priest, scene.hitPoints, scene.terrain, scene.islands, scene.footprint, scene.create, scene.carrier, scene.state,
        override.hooks ?? { destroy: () => {}, neutralReturn: () => {} });
    assert.ok(!throws(() => make()));
    assert.ok(throws(() => make({ priest: { ...scene.priest, pool: other } })));
    assert.ok(throws(() => make({ hooks: { destroy: () => {} } })));
    assert.ok(throws(() => make({ hooks: { neutralReturn: () => {} } })));
    assert.equal(scene.arrival.pool, scene.pool);
});
