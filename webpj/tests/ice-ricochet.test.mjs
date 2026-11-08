// 아이스 반사 raw-ice-ricochet.js(RawIceRicochet)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측 1: ice-ricochet-x86.tsv — 종류(0 반사점 / 1 후보 선택·후속 요청)·난수 시드·원점·현재 위치·사거리·현재 표적·속도·후보 노드·변이로 외부 효과 순서(B finder 시작/N 다음/J 후속 Missile 인자 12개),
//    반사점 8바이트, 128바이트 필드 이미지, 공유 난수 최종 상태가 같다.
//  - 원본 관측 2: ice-ricochet-scratch-x86.tsv 213행 — 변위 0/비영 변위·NaN·경계·후보·현재 필드와 호출 전 지역 점 8바이트(initialPointBytes) 입력으로 같은 출력과 입력 호출 횟수가 같다.
//  - 계약: 후속 Missile 수명이 없으면 거부, 지역 점 입력은 확률 관문과 변위 0 에서만 요구, 실제 해시 체인/HP(SquidReward)가 원문의 같은 후보로 같은 인자를 만든다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E124: 아이스 반사 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, bitsToFloat, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { GameRandom } from '../www/core/random.js';
import { ScrambledSpStore } from '../www/core/sp-store.js';
import { SquidReward, SquidRewardState } from '../www/core/squid-reward.js';
import { SquidPostPopState } from '../www/core/squid-postpop-state.js';
import { MissileFields } from '../www/core/raw-missile-fields.js';
import { RawIceRicochet, makeIceRicochetWorldHooks } from '../www/core/raw-ice-ricochet.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E124)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'ice-ricochet-x86.tsv')) ?? [];
const scratchRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'ice-ricochet-scratch-x86.tsv')) ?? [];
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 원문 도구 KEYS 와 같은 독립 입력 열 번호이다. (2026-11-08 WB04-E124)
const KIND = 0;
const SEED = 1;
const ORIGIN_X = 2;
const ORIGIN_Y = 3;
const X = 4;
const Y = 5;
const RANGE = 6;
const TARGET = 7;
const SPEED = 8;
const NODES = 9;
const MUTATION = 10;
const OUTPUT = 11;

// 예외가 나는지 본다. (2026-11-08 WB04-E124)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 같은 128바이트/표시 두 슬롯/일반 Finder 후보와 공유 난수를 가진 대조 장면이다. (2026-11-08 WB04-E124)
function makeScene() {
    const scene = {
        pool: new SidPool(24000, { server: false }),
        types: Array.from({ length: 188 }, () => ({ footX: 0, footY: 0, flags1: 0, flags2: 0, group: 0, hotspotX: 0, hotspotY: 0, maxHitPoints: 0 })),
        missileTypes: Array.from({ length: 188 }, () => ({ range: 0 })),
        random: new GameRandom(0),
        fields: new MissileFields(),
        events: [],
        candidates: [],
        index: 0,
        input: null,
        initialPointBytes: undefined
    };
    // 입력행마다 원문과 동일한 현재 객체/TYPE/난수를 설치한다.
    scene.prepare = (row) => {
        assert.equal(row.length, OUTPUT + 4);
        scene.input = row;
        scene.events = [];
        scene.candidates = [];
        scene.index = 0;
        scene.random.setState(Number(row[SEED]));
        const bytes = scene.fields.bytes;
        // 생성자가 쓰지 않은 padding 도 같은 비트로 대조한다.
        for (let at = 0; at < bytes.length; at += 1) bytes[at] = (at * 13 + 7) & 0xff;
        put(bytes, 4, 81);
        put(bytes, 8, 90);
        put(bytes, 24, Number(row[TARGET]));
        put(bytes, 48, Number(row[SPEED]));
        put(bytes, 56, Number(row[ORIGIN_X]));
        put(bytes, 60, Number(row[ORIGIN_Y]));
        scene.types.forEach((type, at) => { scene.types[at] = { footX: 0, footY: 0, flags1: 0, flags2: 0, group: 0, hotspotX: 0, hotspotY: 0, maxHitPoints: 0 }; });
        scene.missileTypes.forEach((type, at) => { scene.missileTypes[at] = { range: 0 }; });
        scene.missileTypes[130].range = bitsToFloat(Number(row[RANGE]));
        // 부모가 순회 중 바뀌는 입력도 처음 캡처한 90 슬롯과 구분한다.
        for (const sid of [90, 91]) {
            const raw = scene.pool.physical(sid);
            raw.fill(0);
            put(raw, 6, 81, 2);
            raw[10] = 130;
            put(raw, 14, Number(row[X]));
            put(raw, 18, Number(row[Y]));
        }
        if (row[NODES] === '-') return;
        // 후보별로 다른 TYPE 를 써 플래그가 서로 덮이지 않게 한다.
        for (const text of row[NODES].split(';')) {
            const node = text.split(',');
            assert.equal(node.length, 7);
            const type = 70 + scene.candidates.length;
            const sid = Number(node[0]);
            scene.candidates.push(sid);
            const raw = scene.pool.physical(sid);
            raw.fill(0);
            raw[10] = type;
            raw[11] = Number(node[6]);
            raw[40] = Number(node[3]);
            put(raw, 14, Number(node[4]));
            put(raw, 18, Number(node[5]));
            scene.types[type].flags1 = Number(node[1]);
            scene.types[type].flags2 = Number(node[2]);
        }
    };
    // 원문 경계의 동일 시점에 현재 필드를 바꾸고 다음 후보를 제공한다.
    scene.next = () => {
        scene.events.push('N');
        scene.index += 1;
        if (scene.index === 1) {
            const mutation = Number(scene.input[MUTATION]);
            if (mutation === 1) put(scene.fields.bytes, 24, 15001);
            if (mutation === 2) put(scene.fields.bytes, 48, floatToBits(37.25));
            if (mutation === 3) put(scene.fields.bytes, 8, 91);
            if (mutation === 4) {
                scene.pool.physical(90)[10] = 132;
                put(scene.pool.physical(90), 14, floatToBits(81.25));
                put(scene.pool.physical(90), 18, floatToBits(82.5));
            }
        }
        return scene.index < scene.candidates.length ? scene.candidates[scene.index] : 0;
    };
    // 원본이 넘긴 12개 DWORD 인자의 실제 비트를 순서대로 기록한다.
    scene.create = (request) => {
        const args = [request.firing, request.target, floatToBits(request.speed), request.displayType, request.effectType,
            floatToBits(request.targetPosition[0]), floatToBits(request.targetPosition[1]), floatToBits(request.targetCenter[0]), floatToBits(request.targetCenter[1]),
            request.ricochet, floatToBits(request.firingPosition[0]), floatToBits(request.firingPosition[1])];
        scene.events.push(`J:${args.join(',')}`);
    };
    // 실제 제품 point/generate 와 같은 원본 HP 몸체를 실행한다.
    scene.run = () => {
        const bookkeeping = new SquidPostPopState();
        const rewardState = new SquidRewardState();
        const store = new ScrambledSpStore(scene.random, () => 1);
        const reward = new SquidReward(scene.pool, scene.types, bookkeeping, rewardState, store);
        const hooks = {
            begin: (area, flags) => {
                assert.equal(flags, 0);
                scene.index = 0;
                scene.events.push(`B:${area.left},${area.top},${area.right},${area.bottom}:0:0`);
                return scene.candidates.length ? scene.candidates[0] : 0;
            },
            next: () => scene.next(),
            hasHitPoints: (sid) => reward.hasHitPoints(sid),
            createProjectile: (request) => scene.create(request),
            initialPointBytes: scene.initialPointBytes
        };
        const logic = new RawIceRicochet(scene.pool, scene.types, scene.missileTypes, scene.random, hooks);
        const point = [123.25, 124.5];
        if (Number(scene.input[KIND]) === 0) logic.point(scene.fields, point);
        else logic.generate(scene.fields);
        const pointBytes = new Uint8Array(8);
        put(pointBytes, 0, floatToBits(point[0]));
        put(pointBytes, 4, floatToBits(point[1]));
        return [scene.events.join(';'), hex(pointBytes), hex(scene.fields.bytes), String(scene.random.state)];
    };
    return scene;
}

test('IceRicochet: 원본 10.78 반사점·확률·후보 선택·현재 필드 재조회·공유 난수가 같다', { skip: rows.length === 0 && '원본 fixture 없음' }, () => {
    const scene = makeScene();
    let count = 0;
    // 점 계산·선택·재조회 입력을 모두 소비한다. 기대값은 제품 출력으로 만들지 않는다.
    rows.forEach((row, index) => {
        scene.prepare(row);
        const actual = scene.run();
        count += 1;
        assert.deepEqual(actual, row.slice(OUTPUT, OUTPUT + 4), `아이스 반사 행 ${index} 종류 ${row[KIND]} 변이 ${row[MUTATION]}`);
    });
    assert.ok(count > 0);
});

test('IceRicochet: 새 원문 213행의 변위 0/비영 변위·NaN/경계·후보/현재 필드와 지역 점 입력 횟수가 같다', { skip: scratchRows.length === 0 && '원본 fixture 없음' }, () => {
    assert.equal(scratchRows.length, 213);
    const scene = makeScene();
    // 원문 입력 11열과 독립 좌표 8바이트를 공급하고 최초 출력 4열 전체를 비교한다.
    scratchRows.forEach((row, index) => {
        assert.equal(row.length, 20);
        scene.prepare([...row.slice(0, 11), ...row.slice(13, 17)]);
        let fills = 0;
        scene.initialPointBytes = (bytes) => {
            assert.equal(bytes.length, 8);
            fills += 1;
            put(bytes, 0, Number(row[11]));
            put(bytes, 4, Number(row[12]));
        };
        const actual = scene.run();
        assert.deepEqual(actual, row.slice(13, 17), `반사 지역점 행 ${index}`);
        const equal = row[X] === row[ORIGIN_X] && row[Y] === row[ORIGIN_Y];
        assert.equal(fills, equal ? Number(row[18]) : 0);
        if (fills) assert.equal(row[19], '1');
    });
});

test('IceRicochet: 후속 Missile 수명 누락·지역 점 입력 누락·판본/표 크기를 거부한다', { skip: rows.length === 0 && '원본 fixture 없음' }, () => {
    const scene = makeScene();
    const row = [...rows[0]];
    row[KIND] = '1';
    row[SEED] = '20';
    row[RANGE] = '0';
    scene.prepare(row);
    // 새 탄의 수명이 없으면 빈 성공으로 처리하지 않는다(앞선 효과 없음).
    const logic = new RawIceRicochet(scene.pool, scene.types, scene.missileTypes, scene.random, { begin: () => 0, next: () => 0, hasHitPoints: () => false });
    assert.ok(throws(() => logic.generate(scene.fields)));
    assert.deepEqual(scene.events, []);
    // 변위 0 인데 지역 점 입력이 없으면 확률 관문 뒤에만 거부하고 앞선 난수/필드를 보존한다.
    const equalRow = [...rows[0]];
    equalRow[KIND] = '1';
    equalRow[SEED] = '20';
    equalRow[ORIGIN_X] = equalRow[X];
    equalRow[ORIGIN_Y] = equalRow[Y];
    scene.prepare(equalRow);
    assert.ok(throws(() => scene.run()));
    assert.deepEqual(scene.events, []);
    assert.equal(scene.random.state, 0x0014003f);
    equalRow[SEED] = '1';
    scene.prepare(equalRow);
    assert.ok(!throws(() => scene.run()));
    assert.deepEqual(scene.events, []);
    assert.equal(scene.random.state, 0x00010006);
    // 판본/표 크기.
    assert.ok(throws(() => new RawIceRicochet({ edition: 'cd1072' }, scene.types, scene.missileTypes, scene.random, {})));
    assert.ok(throws(() => new RawIceRicochet(scene.pool, scene.types.slice(0, 10), scene.missileTypes, scene.random, {})));
    assert.ok(throws(() => new RawIceRicochet(scene.pool, scene.types, scene.missileTypes.slice(0, 10), scene.random, {})));
});

test('IceRicochet: 실제 해시 체인과 HP(SquidReward)가 원문의 같은 후보로 같은 후속 인자를 만든다', { skip: rows.length === 0 && '원본 fixture 없음' }, () => {
    const found = rows.find((row) => row[KIND] === '1' && row[SEED] === '20' && row[RANGE] === '0' && row[TARGET] === '60' && row[MUTATION] === '0' && row[NODES] === '15000,16,0,0,1114636288,1112014848,0');
    assert.ok(found);
    const scene = makeScene();
    scene.prepare(found);
    const hash = new SquidHash();
    hash.setBucket(0, 60, 50, 15000);
    put(scene.pool.physical(15000), 4, 0, 2);
    const bookkeeping = new SquidPostPopState();
    const rewardState = new SquidRewardState();
    const store = new ScrambledSpStore(scene.random, () => 1);
    const reward = new SquidReward(scene.pool, scene.types, bookkeeping, rewardState, store);
    const hooks = makeIceRicochetWorldHooks(scene.pool, hash, scene.types, reward, { createProjectile: (request) => scene.create(request) });
    new RawIceRicochet(scene.pool, scene.types, scene.missileTypes, scene.random, hooks).generate(scene.fields);
    assert.equal(scene.events.length, 1);
    const expected = found[OUTPUT];
    assert.equal(scene.events[0], expected.slice(expected.lastIndexOf('J:')));
    assert.equal(hex(scene.fields.bytes), found[OUTPUT + 2]);
    assert.equal(scene.random.state, Number(found[OUTPUT + 3]));
    assert.equal(store.initialized, false);
    assert.ok(throws(() => makeIceRicochetWorldHooks(new SidPool(24000, { server: false }), hash, scene.types, reward, {})));
});
