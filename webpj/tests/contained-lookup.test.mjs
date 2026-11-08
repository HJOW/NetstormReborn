// 종속 조회/삭제 raw-contained-lookup.js(RawContainedLookup)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: contained-x86.tsv 의 `originals` 805행 — 입력 6 값(서버 권한·체인 패턴 0~4·타입·순번·삭제 여부·삭제 콜백 변이 0~5) → 반환 SID, 가상 삭제 요청 순서(`D:SID:flags`), 부모와 일곱 종속 슬롯 전체 raw.
//    (`originalCD` 805행과 `original1037` 행은 다른 판본이라 사용하지 않는다.)
//  - 계약: 범위 밖/순환 링크와 삭제 뒤 손상된 next 는 무한 반복이나 배열 밖 접근 전에 거부하고, 삭제 경계 누락과 다른 월드의 건설 어댑터 연결은 효과 전에 거부한다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E108: 종속 조회/삭제 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { RawContainedLookup, makeContainedConstructionHooks } from '../www/core/raw-contained-lookup.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E108)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'contained-x86.tsv'));
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 예외가 나는지 본다. (2026-11-08 WB04-E108)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 부모 SID 와 입력 열 위치, 주변 표식에 쓰는 가상 표 값이다. (2026-11-08 WB04-E108)
const PARENT = 50;
const TABLE = 0x12010000;
const I = { Server: 0, Pattern: 1, Type: 2, Ordinal: 3, Remove: 4, Mutate: 5 };

// 실제 풀에 입력 체인과 가상 삭제의 응답만 공급하는 장면이다. (2026-11-08 WB04-E108)
function makeScene(server) {
    const scene = { pool: new SidPool(32768, { server }), children: [], input: [], events: [] };
    const layout = scene.pool.layout;
    scene.children = [1, 4, 5, 6, layout.serverFirst - 1, layout.serverFirst, layout.predictableFirst + 1];
    // 명시 삭제 경계에서 호출만 기록하고 입력에 지정된 raw 변화를 공급한다.
    const destroy = (sid, flags) => {
        scene.events.push(`D:${sid}:${flags}`);
        const raw = scene.pool.physical(sid);
        const mutate = scene.input[I.Mutate];
        if (mutate === 1) put(raw, 4, scene.children[3], 2);
        if (mutate === 2) put(raw, 4, 0, 2);
        if (mutate === 3) scene.pool.physical(scene.children[1])[11] = 2;
        if (mutate === 4) { const type = raw[10]; raw.fill(0); raw[10] = type; raw[11] = 3; }
        if (mutate === 5) put(scene.pool.physical(PARENT), 6, 0, 2);
    };
    scene.lookup = new RawContainedLookup(scene.pool, destroy);
    // 입력 표식과 상태/타입만 설정하며 결과를 재계산하지 않는다.
    scene.prepare = (values) => {
        scene.input = values;
        scene.events = [];
        assert.equal(scene.pool.isServer, values[I.Server] !== 0);
        // 부모와 모든 종속 슬롯의 주변 바이트를 실행기와 같은 값으로 채운다.
        for (let index = 0; index <= scene.children.length; index += 1) {
            const sid = index ? scene.children[index - 1] : PARENT;
            const raw = scene.pool.physical(sid);
            // 표식 바이트는 whole-slot 보존 대조에 사용한다.
            for (let at = 0; at < raw.length; at += 1) raw[at] = (at * 29 + index * 17 + 7) & 0xff;
            put(raw, 0, TABLE);
            put(raw, 4, 0, 2);
            put(raw, 6, index === 0 && values[I.Pattern] !== 4 ? scene.children[0] : 0, 2);
            raw[10] = index ? 47 : 100;
            raw[11] = index ? 8 : 0;
            if (!index) continue;
            put(raw, 4, index < scene.children.length ? scene.children[index] : 0, 2);
            // 세 패턴의 타입/상태는 PE 실행에 공급한 입력과 동일하다.
            const types1 = [46, 47, 47, 46, 47, 47, 47];
            const types3 = [0, 255, 46, 47, 47, 255, 0];
            const states2 = [2, 10, 8, 1, 4, 0, 8];
            const states3 = [0, 8, 2, 10, 1, 4, 0];
            if (values[I.Pattern] === 1) raw[10] = types1[index - 1];
            if (values[I.Pattern] === 2) raw[11] = states2[index - 1];
            if (values[I.Pattern] === 3) { raw[10] = types3[index - 1]; raw[11] = states3[index - 1]; }
        }
    };
    // 지정 raw 전체를 원본 실행기와 같은 순서로 합친다.
    scene.dump = () => [PARENT, ...scene.children].map((sid) => hex(scene.pool.slot(sid))).join(':');
    scene.eventText = () => (scene.events.length === 0 ? '-' : scene.events.join(';'));
    return scene;
}

test('ContainedLookup: 원본 10.78 805입력의 반환 SID·가상 삭제 요청 순서·부모와 일곱 종속 슬롯 전체 raw 가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    const client = makeScene(false);
    const server = makeScene(true);
    let count = 0;
    // 입력만 공급하고 원본이 저장한 기대 관찰을 읽는다.
    for (const row of rows) {
        if (row[0] !== 'originals') continue;
        assert.equal(row.length, 5);
        const values = row[1].split(',').map((value) => Number(BigInt.asUintN(32, BigInt(value))));
        const scene = values[I.Server] ? server : client;
        scene.prepare(values);
        const result = scene.lookup.find(PARENT, values[I.Type], values[I.Ordinal], values[I.Remove] !== 0);
        const label = `행 ${count}: ${row[1]}`;
        assert.equal(result, Number(row[2]), label);
        assert.equal(scene.eventText(), row[3], label);
        assert.equal(scene.dump(), row[4], label);
        count += 1;
    }
    assert.equal(count, 805);
});

test('ContainedLookup: 범위 밖/순환 링크와 삭제 뒤 손상된 next 는 거부하고 삭제 경계 누락과 다른 월드의 건설 어댑터 연결은 효과 전에 거부한다', () => {
    const pool = new SidPool(24000);
    let calls = 0;
    const parent = pool.physical(PARENT);
    const child = pool.physical(5);
    put(parent, 6, 5, 2);
    child[10] = 47;
    child[11] = 8;
    put(child, 4, 5, 2);
    const lookup = new RawContainedLookup(pool, () => { calls += 1; put(child, 4, 24000, 2); });
    assert.ok(throws(() => lookup.find(PARENT, 46)));
    assert.ok(throws(() => lookup.find(PARENT, 47, 0, true)));
    assert.equal(calls, 1);
    put(child, 4, 0, 2);
    assert.equal(lookup.find(PARENT, 46), 0);
    assert.ok(throws(() => lookup.find(24000, 47)));
    // 삭제 경계 누락과 다른 월드의 어댑터 연결은 효과를 실행하기 전에 거부한다.
    const other = new SidPool(24000);
    assert.ok(throws(() => new RawContainedLookup(pool, null)));
    const quiet = new RawContainedLookup(pool, () => {});
    assert.ok(throws(() => makeContainedConstructionHooks(other, quiet, {})));
    const hooks = makeContainedConstructionHooks(pool, quiet, { keep: 1 });
    assert.equal(hooks.keep, 1);
    assert.equal(typeof hooks.removeContained, 'function');
    assert.equal(quiet.pool, pool);
});
