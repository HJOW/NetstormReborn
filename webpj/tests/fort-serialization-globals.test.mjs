// Fort 저장 전역 묶음(cpppj `o/FortSerializationGlobals.h`)과 이를 빌리는 로더 상태 생성자(`RawFortObjectLoad.h` 의 두 번째/세 번째 대여 생성자)의 웹 대응
// `core/fort-serialization-globals.js` · `core/fort-object-load.js` 의 `createFortObjectLoadState(versionOwner, depthOwner)` · `createFortObjectLoadStateFromGlobals(globals)` 를 검사한다(4-B, WB04-E279).
//  - 묶음: 다섯 DWORD(판본·깊이·scope·소유자 저장 마스크 0x5d77cf00·전투 저장 마스크 0x40480002)의 초기값과 32비트 감김.
//  - 대여: 상태의 다섯 칸이 묶음의 같은 칸을 읽고 쓰며(양방향), 나머지 칸은 로컬이고, 만들 때 값을 되돌리거나 복제하지 않으며, 두 상태가 한 묶음을 공유한다.
//    깊이 상자만 빌리는 두 번째 생성자는 깊이만 공유하고 scope/두 마스크는 로컬이다.
//  - 로더 연결: 원본 10.78 fortobjectload-x86.tsv 609행을 묶음/깊이 상자를 빌린 상태로 다시 재생해 독립 상태와 같은 결과(효과·슬롯·난수·전역)를 내고 묶음의 칸이 같은 값을 갖는지,
//    소유자 저장 마스크를 바꾸면 독립 상태에서 바꾼 것과 같은 결과가 나오는지(로더가 빌린 마스크를 읽음) 확인한다.
// 실제 게임·브라우저 실행이 아니다. 기준선 뒤 cpppj 변경은 3.6절 ③(4-B)이며 이 단위에서 반영했다. (2026-11-11 WB04-E279)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { FortObjectLoad, createFortObjectLoadState, createFortObjectLoadStateFromGlobals } from '../www/core/fort-object-load.js';
import { FORT_BATTLE_SAVED_MASK_INITIAL, FORT_OWNER_SAVED_MASK_INITIAL, FortSerializationGlobals } from '../www/core/fort-serialization-globals.js';
import { F, harness, record } from './fort-load-scene.mjs';

// 원본 관측 파일과 패치 10.78 행(originals)이다. 파일이 없으면 해당 검사를 건너뛴다. (2026-11-11 WB04-E279)
const fixturePath = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'fortobjectload-x86.tsv');
const rows = fs.existsSync(fixturePath) ? fs.readFileSync(fixturePath, 'utf8').split(/\r?\n/)
    .filter((line) => line.startsWith('originals\t')).map((line) => line.split('\t')) : null;

// 묶음이 가진 다섯 칸 이름이다. (2026-11-11 WB04-E279)
const FIELDS = ['version', 'loadingDepth', 'scope', 'ownerSavedMask', 'battleSavedMask'];

// 예외가 나는지 본다. (2026-11-11 WB04-E279)
function throws(action, type) {
    try {
        action();
        return false;
    } catch (error) {
        return type ? error instanceof type : true;
    }
}

// fortobjectload 한 행을 실행하고 원본 관측과 같은 모양의 결과 열을 돌려준다. options 는 harness 의 상태 생성 옵션이다. (2026-11-11 WB04-E279)
function runRow(row, options, adjust = null) {
    const v = row[2].split(',').map(Number);
    const scene = harness(v, options);
    if (adjust) adjust(scene);
    const loader = new FortObjectLoad(scene);
    if (row[1] === 'O') loader.object(record(v), v[F.originX] | 0, v[F.originY] | 0, v[F.defaultOwner]);
    else {
        const fort = { territories: Array.from({ length: 20 }, () => ({ present: false, version: 0, chunks: [] })) };
        const section = () => ({ present: true, version: 2, chunks: Array.from({ length: 4 }, () => ({ objects: [{ ...record(v), position: 0x21 }] })) });
        fort.territories[0] = section();
        fort.territories[1] = section();
        fort.chaff = section();
        loader.load(fort, v[F.slot], 0, 20);
    }
    const state = [scene.state.version, scene.state.loadingDepth, scene.state.scope, Number(scene.state.fortDestroyBlocked)].join(',');
    return { columns: [scene.events.join(';') || '-', scene.raws(), String(scene.random.state), state], scene };
}

// 묶음의 초기값과 32비트 감김이다. (2026-11-11 WB04-E279)
test('FortSerializationGlobals: 다섯 DWORD 의 초기값과 32비트 감김', () => {
    const globals = new FortSerializationGlobals();
    assert.deepEqual(FIELDS.map((name) => globals[name]), [0, 0, 0, 0x5d77cf00, 0x40480002]);
    assert.equal(FORT_OWNER_SAVED_MASK_INITIAL, 0x5d77cf00);
    assert.equal(FORT_BATTLE_SAVED_MASK_INITIAL, 0x40480002);
    // 쓰기는 하위 32비트만 남기고 음수는 부호 없는 DWORD 가 된다.
    for (const name of FIELDS) {
        globals[name] = -1;
        assert.equal(globals[name], 0xffffffff, name);
        globals[name] = 0x1_0000_0005;
        assert.equal(globals[name], 5, name);
    }
    // 두 묶음은 서로 독립이다.
    const other = new FortSerializationGlobals();
    other.scope = 2;
    assert.equal(globals.scope, 5);
});

// 전체 대여: 다섯 칸이 묶음의 같은 칸이며 양방향이다. 나머지 칸은 로컬이고 만들 때 값을 되돌리지 않는다. (2026-11-11 WB04-E279)
test('createFortObjectLoadStateFromGlobals: 다섯 칸을 양방향으로 빌리고 나머지는 로컬이며 만들 때 초기화하지 않는다', () => {
    const globals = new FortSerializationGlobals();
    globals.version = 0x89abcdef;
    globals.loadingDepth = 7;
    globals.scope = 2;
    globals.ownerSavedMask = 0x1234;
    globals.battleSavedMask = 0x5678;
    const state = createFortObjectLoadStateFromGlobals(globals);
    // 만든 뒤에도 묶음 값은 그대로이고 상태는 현재 값을 읽는다(PE 초기값으로 덮지 않음).
    assert.deepEqual(FIELDS.map((name) => state[name]), [0x89abcdef, 7, 2, 0x1234, 0x5678]);
    assert.deepEqual(FIELDS.map((name) => globals[name]), [0x89abcdef, 7, 2, 0x1234, 0x5678]);
    // 상태에 쓰면 묶음이 바뀌고(32비트 감김), 묶음에 쓰면 상태가 읽는다.
    state.version = 2;
    state.loadingDepth = 0xffffffff + 4;
    state.scope = 1;
    state.ownerSavedMask = -2;
    state.battleSavedMask = 9;
    assert.deepEqual(FIELDS.map((name) => globals[name]), [2, 3, 1, 0xfffffffe, 9]);
    globals.version = 0;
    globals.loadingDepth = 11;
    globals.scope = 0;
    globals.ownerSavedMask = 4;
    globals.battleSavedMask = 8;
    assert.deepEqual(FIELDS.map((name) => state[name]), [0, 11, 0, 4, 8]);
    // 같은 묶음을 빌린 두 상태는 같은 값을 본다.
    const second = createFortObjectLoadStateFromGlobals(globals);
    second.scope = 2;
    assert.equal(state.scope, 2);
    // 빌리지 않는 칸은 로컬 값이며 두 상태가 서로 공유하지 않는다.
    assert.deepEqual([state.bridgeType, state.islandType, state.largeIslandType, state.noIslandType], [82, 94, 95, 157]);
    assert.deepEqual([state.ownerClearedMask, state.fortKeepMask], [0x10002000, 0x40480002]);
    state.fortKeepMask = 1;
    state.special = true;
    assert.equal(second.fortKeepMask, 0x40480002);
    assert.equal(second.special, false);
    assert.deepEqual([state.chunksX, state.chunksY], [16, 16]);
});

// 깊이 상자만 대여: 깊이는 상자와 공유하고 scope/두 저장 마스크는 로컬이며 묶음 전체 대여와 구별된다. (2026-11-11 WB04-E279)
test('createFortObjectLoadState: 깊이 상자 대여는 깊이만 공유하고 scope/저장 마스크는 로컬이다', () => {
    const version = { value: 2 };
    const depth = { value: 5 };
    const state = createFortObjectLoadState(version, depth);
    assert.equal(state.loadingDepth, 5);
    state.loadingDepth = 6;
    assert.equal(depth.value, 6);
    depth.value = 0xffffffff;
    assert.equal(state.loadingDepth, 0xffffffff);
    // 상자에 직접 쓴 음수도 읽을 때 32비트 DWORD 로 본다.
    depth.value = -1;
    assert.equal(state.loadingDepth, 0xffffffff);
    state.loadingDepth = depth.value + 2;
    assert.equal(depth.value, 1);
    // 깊이를 주지 않으면 이 상태의 로컬 값이고 판본 상자와 섞이지 않는다.
    const plain = createFortObjectLoadState(version);
    plain.loadingDepth = 9;
    assert.equal(depth.value, 1);
    assert.equal(version.value, 2);
    // 로컬 칸의 초기값과 쓰기는 다른 상태에 영향이 없다.
    assert.deepEqual([state.scope, state.ownerSavedMask, state.battleSavedMask], [0, 0x5d77cf00, 0x40480002]);
    state.scope = 2;
    state.ownerSavedMask = 0;
    state.battleSavedMask = 0;
    assert.deepEqual([plain.scope, plain.ownerSavedMask, plain.battleSavedMask], [0, 0x5d77cf00, 0x40480002]);
    // 판본은 상자를 빌린다.
    state.version = 7;
    assert.equal(version.value, 7);
});

// 생성 계약: 묶음 종류와 상자 모양이 맞지 않으면 거부한다. (2026-11-11 WB04-E279)
test('Fort 로더 상태 생성 계약: 묶음/상자 모양 거부', () => {
    assert.ok(throws(() => createFortObjectLoadStateFromGlobals(), TypeError));
    assert.ok(throws(() => createFortObjectLoadStateFromGlobals({ version: 0, loadingDepth: 0, scope: 0, ownerSavedMask: 0, battleSavedMask: 0 }), TypeError));
    assert.ok(throws(() => createFortObjectLoadState(), TypeError));
    assert.ok(throws(() => createFortObjectLoadState({ value: 0 }, {}), TypeError));
    assert.ok(throws(() => createFortObjectLoadState({ value: 0 }, 3), TypeError));
    // 판본 상자가 없거나 숫자가 아니어도 거부한다.
    assert.ok(throws(() => createFortObjectLoadState({}, { value: 0 }), TypeError));
});

// 원본 609행을 빌린 상태로 재생: 독립 상태와 같은 관측(효과·슬롯·난수·전역)이고 묶음/상자의 칸이 상태와 같은 값이다. (2026-11-11 WB04-E279)
test('Fort 객체 로더: 전체 대여/깊이 대여 상태로도 원본 10.78 609행이 같고 빌린 칸이 상태와 일치한다', { skip: rows ? false : 'fortobjectload-x86.tsv 없음' }, () => {
    assert.equal(rows.length, 609);
    // 각 관찰마다 새 묶음/상자로 실행한다.
    for (const row of rows) {
        const plain = runRow(row, {});
        assert.deepEqual(plain.columns, row.slice(4), `${row[1]} ${row[2]}`);
        const globals = new FortSerializationGlobals();
        const shared = runRow(row, { globals });
        assert.deepEqual(shared.columns, plain.columns, `전체 대여 ${row[1]} ${row[2]}`);
        const { state } = shared.scene;
        assert.deepEqual(FIELDS.map((name) => globals[name]), FIELDS.map((name) => state[name]), `묶음 칸 ${row[1]} ${row[2]}`);
        const depthOwner = { value: 0 };
        const borrowed = runRow(row, { depthOwner });
        assert.deepEqual(borrowed.columns, plain.columns, `깊이 대여 ${row[1]} ${row[2]}`);
        assert.equal(depthOwner.value, borrowed.scene.state.loadingDepth, `깊이 상자 ${row[1]} ${row[2]}`);
    }
});

// 소유자 저장 마스크를 바꿔도 묶음을 빌린 상태가 독립 상태와 같은 결과를 낸다(로더가 빌린 마스크를 읽음). 마스크는 객체 로더가 flags2 와 비교해 소유자를 쓰는 분기에 쓰인다. (2026-11-11 WB04-E279)
test('Fort 객체 로더: 빌린 소유자 저장 마스크를 로더가 읽는다(독립 상태에서 같은 값을 쓴 결과와 같다)', { skip: rows ? false : 'fortobjectload-x86.tsv 없음' }, () => {
    let differing = 0;
    // 마스크를 0 과 모든 비트로 바꿔 각 행을 독립 상태/묶음 상태에서 실행한다.
    for (const mask of [0, 0xffffffff]) {
        for (const row of rows) {
            const independent = runRow(row, {}, (scene) => { scene.state.ownerSavedMask = mask; });
            const globals = new FortSerializationGlobals();
            const borrowed = runRow(row, { globals }, (scene) => { globals.ownerSavedMask = mask; });
            assert.deepEqual(borrowed.columns, independent.columns, `마스크 ${mask.toString(16)} ${row[1]} ${row[2]}`);
            if (independent.columns.join('|') !== row.slice(4).join('|')) differing += 1;
        }
    }
    // 마스크 변경이 실제로 결과를 바꾸는 입력이 있어야 이 검사가 연결을 구별한다.
    assert.ok(differing > 0, '마스크를 바꿔도 결과가 변하는 입력이 없음');
});
