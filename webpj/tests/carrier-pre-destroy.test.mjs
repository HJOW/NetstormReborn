// Carrier 삭제 준비·내용물 조회 raw-carrier-pre-destroy.js(RawCarrierPreDestroy)와 공용 커서 raw-contained-finder.js(RawContainedFinder·countContained)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: carrierpredestroy-x86.tsv 의 10.78(originals) 960행(직접 조회 Q 480·Carrier 전체 P 480). cpppj carrier_predestroy_patch_x86 과 같은 raw 배치로
//    FindContained 반환 또는 Damageable 대체 사건·전역 후처리·boss·다섯 슬롯 전체 바이트를 비교한다(Damageable 와 전역 후처리만 대체).
//  - 계약: carrier_predestroy_guards_and_priest_binding·CarrierPreDestroy_SharedBossRequeriesAfterDamageAndInContainedFinder 의 10.78 부분과 countContained 의 DWORD kind 비교(웹 보강).
//  - 범위 밖: originalCD·original1037 행은 웹 범위(10.78) 밖이다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E56: 내용물 검색기·Carrier 삭제 준비 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put } from './raw-support.mjs';
import { hex } from './player-scene-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { ContainedFinderState, RawContainedFinder, countContained } from '../www/core/raw-contained-finder.js';
import { RawCarrierPreDestroy, makeCarrierPreDestroyHooks } from '../www/core/raw-carrier-pre-destroy.js';

// 원본 관측 10.78 행이다. 파일이 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E56)
const FIXTURES = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures');
const carrierRows = readRows(path.join(FIXTURES, 'carrierpredestroy-x86.tsv'))?.filter((row) => row[0] === 'originals') ?? null;

// 관찰 부모(50)와 종속 네 슬롯(60~63), 풀 크기(cpppj kCapacity)이다. (2026-11-08 WB04-E56)
const SOURCE = 50;
const IDS = Object.freeze([50, 60, 61, 62, 63]);
const CAPACITY = 24000;

// 예외가 나는지 본다. (2026-11-08 WB04-E56)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// cpppj CarrierScene: client FIFO 로 5~63 을 확보하고 관찰 다섯 슬롯의 쓰기 보기를 한 번만 빌린다(이후 합성 free 비트가 생겨도 같은 보기를 쓴다). (2026-11-08 WB04-E56)
function carrierScene() {
    const pool = new SidPool(CAPACITY, { server: false });
    // 번호는 기존 client FIFO 할당으로 확보한다.
    for (let id = 5; id <= 63; id += 1) assert.equal(pool.allocate(2), id);
    const scene = { pool, state: new ContainedFinderState(), slots: IDS.map((sid) => pool.allocated(sid)), events: '', after: '-', validations: 0 };
    // 원본에서 대체한 두 하위 몸체만 같은 사건과 입력 변경으로 제공한다.
    scene.hooks = () => ({
        validateDamageable: () => { scene.validations += 1; },
        damageablePre: (sid, flags) => {
            scene.events = `D:${sid}:${flags >>> 0}:${scene.state.boss ? 1 : 0}`;
            if (scene.after !== '-') {
                const values = scene.after.split(',').map(Number);
                scene.state.boss = values[0] !== 0;
                put(scene.slots[0], 6, values[1], 2);
                put(scene.slots[4], 20, values[2], 2);
            }
        },
        dependentFound: () => { scene.events += ';G'; },
    });
    // 입력 칸을 raw/state 에 넣으며 기대 분기나 반환을 계산하지 않는다.
    scene.prepare = (row) => {
        assert.equal(row.length, 16);
        scene.events = '';
        scene.validations = 0;
        scene.after = row[9];
        scene.state.boss = Number(row[3]) !== 0;
        scene.state.containedType = Number(row[4]) >>> 0;
        // 지정하지 않은 바이트도 실행기와 같은 0xab 로 채운다.
        for (const raw of scene.slots) {
            raw.fill(0xab);
            raw[10] = 158;
            raw[11] = 0;
            put(raw, 4, 0);
        }
        scene.slots[0][11] = Number(row[2]);
        put(scene.slots[0], 6, Number(row[10]), 2);
        // 각 종속은 번호·타입·상태·next·kind·mask 순서다.
        for (const node of row[11].split(';')) {
            if (!node) continue;
            const values = node.split(',').map(Number);
            const raw = scene.slots[values[0] - 60 + 1];
            raw[10] = values[1];
            raw[11] = values[2];
            put(raw, 4, values[3], 2);
            put(raw, 18, values[4], 2);
            put(raw, 20, values[5], 2);
        }
    };
    // 현재 슬롯 전체를 실행기의 비교 순서로 직렬화한다.
    scene.slotText = () => IDS.map((sid) => hex(pool.slot(sid))).join('|');
    return scene;
}

test('CarrierPreDestroy: 원본 10.78 직접 조회·Carrier 전체 960행과 같다', { skip: !carrierRows && '원본 fixture 없음' }, () => {
    const scene = carrierScene();
    let count = 0;
    // 두 API 는 같은 raw 배치를 쓰지만 기대값은 각각 실제 PE 에서 얻었다.
    for (const row of carrierRows) {
        scene.prepare(row);
        const carrier = new RawCarrierPreDestroy(scene.pool, scene.state, scene.hooks());
        if (row[1] === 'Q') assert.equal(carrier.findContained(SOURCE, Number(row[5]), Number(row[6]), Number(row[7])), Number(row[12]), `Q 행 ${count}`);
        else carrier.preDestroy(SOURCE, Number(row[8]));
        assert.deepEqual([scene.events, scene.slotText(), scene.state.boss, scene.validations], [row[14], row[15], Number(row[13]) !== 0, row[1] === 'P' ? 1 : 0], `${row[1]} 행 ${count}`);
        count += 1;
    }
    assert.equal(count, 960);
});

test('CarrierPreDestroy: 하위 경계 누락·잘못된 자산·손상 체인·10.78 dead 진단은 명시 거부하고 사제 경계를 같은 풀에 연결한다', { skip: !carrierRows && '원본 fixture 없음' }, () => {
    const scene = carrierScene();
    scene.prepare(carrierRows[0]);
    const missing = scene.hooks();
    delete missing.dependentFound;
    assert.ok(throws(() => new RawCarrierPreDestroy(scene.pool, scene.state, missing)));
    const carrier = new RawCarrierPreDestroy(scene.pool, scene.state, scene.hooks());
    // 효과 전에 거부하고 사건·슬롯을 바꾸지 않는다.
    const reject = (sid) => {
        const before = scene.slotText();
        assert.ok(throws(() => carrier.preDestroy(sid, 0)), `거부 ${sid}`);
        assert.ok(scene.events === '' && scene.slotText() === before);
    };
    [0, 1, 24000, 65535].forEach(reject);
    scene.slots[0][11] = 1;
    reject(SOURCE);
    scene.slots[0][11] = 8;
    reject(SOURCE);
    scene.slots[0][11] = 2;
    scene.slots[0][10] = 6;
    reject(SOURCE);
    scene.slots[0][10] = 255;
    reject(SOURCE);
    scene.slots[0][10] = 158;
    const blockedHooks = scene.hooks();
    blockedHooks.validateDamageable = () => { throw new Error('하위 효과 미복원'); };
    const blocked = new RawCarrierPreDestroy(scene.pool, scene.state, blockedHooks);
    const before = scene.slotText();
    assert.ok(throws(() => blocked.preDestroy(SOURCE, 1)));
    assert.ok(scene.events === '' && scene.slotText() === before);
    // 원본 Begin 은 실제 0번 head 를 읽으며 SID 0 자체를 거부하지 않는다.
    assert.equal(carrier.findContained(0, 1, 0, 0), 0);
    put(scene.slots[0], 6, 60, 2);
    scene.slots[1][10] = 6;
    put(scene.slots[1], 20, 1, 2);
    put(scene.slots[1], 4, 60, 2);
    assert.equal(carrier.findContained(SOURCE, 1, 0, 0), 60);
    assert.ok(throws(() => carrier.findContained(SOURCE, 1, 0, 1)));
    put(scene.slots[1], 4, 65535, 2);
    assert.ok(throws(() => carrier.findContained(SOURCE, 1, 0, 1)));
    put(scene.slots[1], 4, 0, 2);
    scene.state.boss = false;
    scene.state.checkingDead = true;
    assert.ok(throws(() => carrier.findContained(SOURCE, 1, 0, 0)));
    scene.state.checkingDead = false;
    const other = new SidPool(CAPACITY, { server: false });
    assert.ok(throws(() => makeCarrierPreDestroyHooks(other, carrier, {})));
    let protection = 0;
    const bound = makeCarrierPreDestroyHooks(scene.pool, carrier, { findForcefield: () => { protection += 1; return 0; } });
    assert.ok(bound.findForcefield(SOURCE) === 0 && protection === 1);
    bound.validateCarrier(SOURCE, 3);
    bound.carrierPre(SOURCE, 3);
    assert.equal(scene.events, 'D:50:3:0');
});

test('CarrierPreDestroy: Damageable 효과 뒤 공유 boss 를 다시 읽고 내용물 검색의 dead 진단도 같은 소유자를 쓴다', () => {
    const scene = carrierScene();
    // 실제 할당된 부모와 종속의 입력 바이트만 준비한다.
    for (const raw of scene.slots) {
        raw.fill(0);
        raw[10] = 158;
    }
    put(scene.slots[0], 6, 60, 2);
    scene.slots[1][10] = 6;
    put(scene.slots[1], 20, 1, 2);
    const boss = { value: true };
    let nextBoss = false;
    scene.state.bindBoss(boss);
    scene.state.checkingDead = true;
    let damages = 0;
    let dependents = 0;
    const carrier = new RawCarrierPreDestroy(scene.pool, scene.state, {
        validateDamageable: () => { scene.validations += 1; },
        damageablePre: () => { damages += 1; boss.value = nextBoss; },
        dependentFound: () => { dependents += 1; },
    });
    carrier.preDestroy(SOURCE, 0);
    assert.deepEqual([damages, dependents, boss.value], [1, 0, false]);
    nextBoss = true;
    carrier.preDestroy(SOURCE, 0);
    assert.deepEqual([damages, dependents, boss.value], [2, 1, true]);
    scene.slots[0][11] |= 2;
    boss.value = false;
    assert.ok(throws(() => carrier.findContained(SOURCE, 1, 0, 0)));
    boss.value = true;
    assert.equal(carrier.findContained(SOURCE, 1, 0, 0), 60);
});

test('RawContainedFinder: dead 진단 장치는 복귀 뒤 head 를 읽고 countContained 는 kind 를 DWORD 그대로 비교한다(웹 보강)', () => {
    const scene = carrierScene();
    // 부모와 세 종속(두 개만 현재 contained 타입)을 준비한다.
    for (const raw of scene.slots) raw.fill(0);
    put(scene.slots[0], 6, 60, 2);
    [[1, 6, 61, 7, 3], [2, 9, 62, 7, 3], [3, 6, 0, 7, 2]].forEach(([at, type, next, kind, mask]) => {
        scene.slots[at][10] = type;
        put(scene.slots[at], 4, next, 2);
        put(scene.slots[at], 18, kind, 2);
        put(scene.slots[at], 20, mask, 2);
    });
    assert.equal(countContained(scene.pool, scene.state, SOURCE, 1, 7), 1);
    assert.equal(countContained(scene.pool, scene.state, SOURCE, 0, 0), 2);
    assert.equal(countContained(scene.pool, scene.state, SOURCE, 0, 7 + 65536), 0);
    // 권한 없는 dead 부모는 진단 장치를 부른 뒤(복귀) 현재 head 로 순회한다.
    scene.slots[0][11] = 2;
    scene.state.boss = false;
    scene.state.bindChecking(() => 1);
    const calls = [];
    scene.state.assertion = (expression, file, line) => { calls.push(line); put(scene.slots[0], 6, 62, 2); };
    const finder = new RawContainedFinder(scene.pool, scene.state);
    assert.equal(finder.begin(SOURCE), 62);
    assert.deepEqual(calls, [0x13c]);
    assert.equal(finder.begin(SOURCE, true), 62);
    assert.deepEqual(calls, [0x13c]);
    assert.ok(throws(() => finder.begin(CAPACITY)));
});
