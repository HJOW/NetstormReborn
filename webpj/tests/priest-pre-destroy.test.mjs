// 사제 삭제 준비 raw-priest-pre-destroy.js(RawPriestPreDestroy·makePriestPreDestroyHooks)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: priestdestroy-x86.tsv 10.78(originals) 2184행(extra·상태·flags·목록 개수/항목·보호막 조회 결과·조회 중 변경 → 목록 압축 결과와 호출 기록·슬롯 50바이트). CD 10.72·10.37 행은 웹 범위(10.78) 밖이다.
//  - 계약: priest_predestroy_guards_and_dispatch(잘못된 raw/목록/미연결 하위는 목록 변경·외부 호출 전 거부, 추상 사제는 목록을 읽지 않음, 분배기는 사제 사건만 가로채고 나머지는 fallback).
//    실제 Damageable/Carrier/종속 form/Kernel 정리 통합(priest_predestroy_cleans_real_regen_form_kernel_and_common_bookkeeping)은 해당 부품의 통합 단위에서 대조한다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E63: 사제 삭제 준비 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SQUID_DESTROY_EFFECT } from '../www/core/squid-destroy.js';
import { PRIEST_TYPE, PRIEST_VTABLE } from '../www/core/raw-priest-owner.js';
import { PriestPostPopState } from '../www/core/raw-priest-post-pop.js';
import { RawPriestPreDestroy, makePriestPreDestroyHooks } from '../www/core/raw-priest-pre-destroy.js';

// 원본 관측 10.78 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E63)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'priestdestroy-x86.tsv'))?.filter((row) => row[0] === 'originals') ?? null;

// 관찰 SID, 판본당 관찰 행 수, 공통 가상 표이다. (2026-11-08 WB04-E63)
const SOURCE = 50;
const ROWS = 2184;
const COMMON_VTABLE = 0x11010000;

// 예외가 나는지 본다. (2026-11-08 WB04-E63)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E63)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 실제 client 할당으로 관찰 대상 번호를 확보한 장면이다. (2026-11-08 WB04-E63)
function destroyScene() {
    const pool = new SidPool(24000, { server: false });
    for (let id = 5; id <= SOURCE; id += 1) assert.equal(pool.allocate(2), id);
    const scene = { pool, state: new PriestPostPopState(), events: '', after: '-', found: 0, validations: 0 };
    // 현재 목록 개수/extra 는 외부 호출 시점마다 새로 읽는다.
    scene.record = (marker, sid, flags = '') => {
        scene.events += `${scene.events ? ';' : ''}${marker}:${sid}${flags}:${scene.state.priests.count}:${pool.slot(SOURCE)[40]}`;
    };
    // lookup/destroy/Carrier 몸체만 원본 실행기와 같은 명시적 대체로 공급한다.
    scene.hooks = () => ({
        findForcefield: (sid) => {
            scene.record('F', sid);
            if (scene.after !== '-') pool.allocated(sid)[40] = Number(scene.after);
            return scene.found;
        },
        destroyForcefield: (sid, flags) => scene.record('D', sid, `:${flags}`),
        validateCarrier: () => { scene.validations += 1; },
        carrierPre: (sid, flags) => scene.record('C', sid, `:${flags}`)
    });
    // 입력 칸만 raw/목록에 넣는다. 기대 압축·분기는 계산하지 않는다.
    scene.prepare = (row) => {
        assert.equal(row.length, 12);
        scene.events = '';
        scene.validations = 0;
        const raw = pool.allocated(SOURCE);
        raw.fill(0xab);
        put(raw, 0, PRIEST_VTABLE);
        raw[10] = PRIEST_TYPE;
        raw[11] = Number(row[2]);
        raw[40] = Number(row[1]);
        scene.state.priests.count = Number(row[4]);
        scene.state.priests.entries = row[5].split(',').map(Number);
        scene.found = Number(row[6]);
        scene.after = row[7];
    };
    scene.entries = () => scene.state.priests.entries.join(',');
    return scene;
}

test('PriestPreDestroy: 원본 10.78 2184행의 목록 압축·보호막 삭제 요청·Carrier 호출·슬롯 전체가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = destroyScene();
    let count = 0;
    // 각 행의 목록/가상 호출/raw 슬롯 전체를 비교한다.
    for (const row of rows) {
        scene.prepare(row);
        new RawPriestPreDestroy(scene.pool, scene.state, scene.hooks()).preDestroy(SOURCE, Number(row[3]));
        assert.equal(scene.state.priests.count, Number(row[8]), `행 ${count}`);
        assert.equal(scene.entries(), row[9], `행 ${count}`);
        assert.equal(scene.events, row[10], `행 ${count}`);
        assert.equal(hex(scene.pool.slot(SOURCE)), row[11], `행 ${count}`);
        assert.equal(scene.validations, 1);
        count += 1;
    }
    assert.equal(count, ROWS);
});

test('PriestPreDestroy: 잘못된 raw/목록/미연결 하위 경계는 목록 변경과 외부 호출 전에 거부하고 분배기는 사제 사건만 가로챈다', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = destroyScene();
    scene.prepare(rows[0]);
    assert.ok(throws(() => new RawPriestPreDestroy(scene.pool, scene.state, { ...scene.hooks(), carrierPre: undefined })));
    const priest = new RawPriestPreDestroy(scene.pool, scene.state, scene.hooks());
    // 거부 전에 변경된 슬롯/목록이 없어야 한다.
    const reject = (sid) => {
        const before = hex(scene.pool.slot(SOURCE));
        const entries = scene.entries();
        const count = scene.state.priests.count;
        assert.ok(throws(() => priest.preDestroy(sid, 0)));
        assert.ok(scene.events === '' && scene.state.priests.count === count && scene.entries() === entries && hex(scene.pool.slot(SOURCE)) === before);
    };
    for (const sid of [0, 51, 24000, 65535]) reject(sid);
    const raw = scene.pool.allocated(SOURCE);
    raw[11] = 1;
    reject(SOURCE);
    raw[11] = 8;
    reject(SOURCE);
    scene.pool.allocated(SOURCE)[11] = 0;
    put(scene.pool.allocated(SOURCE), 0, COMMON_VTABLE);
    reject(SOURCE);
    put(scene.pool.allocated(SOURCE), 0, PRIEST_VTABLE);
    scene.state.priests.count = 7;
    reject(SOURCE);
    scene.state.priests.count = 6;
    const blockedHooks = scene.hooks();
    blockedHooks.validateCarrier = () => { throw new Error('미복원 Carrier 거부'); };
    const entries = scene.entries();
    assert.ok(throws(() => new RawPriestPreDestroy(scene.pool, scene.state, blockedHooks).preDestroy(SOURCE, 0)));
    assert.ok(scene.events === '' && scene.state.priests.count === 6 && scene.entries() === entries);
    // 추상 사제는 목록을 읽지 않으므로 손상된 활성 개수도 이 접두에서 접근하지 않는다.
    scene.state.priests.count = 7;
    scene.pool.allocated(SOURCE)[40] = 9;
    priest.preDestroy(SOURCE, 123);
    assert.ok(scene.events === 'C:50:123:7:9' && scene.state.priests.count === 7 && scene.entries() === entries);
    let forwarded = 0;
    const fallback = { emit: () => { forwarded += 1; }, selected: () => 0 };
    assert.ok(throws(() => makePriestPreDestroyHooks(new SidPool(24000, { server: false }), priest, fallback)));
    const dispatch = makePriestPreDestroyHooks(scene.pool, priest, fallback);
    dispatch.emit({ effect: SQUID_DESTROY_EFFECT.postDestroy, sid: SOURCE, parent: 0, flags: 1 });
    assert.equal(forwarded, 1);
    dispatch.emit({ effect: SQUID_DESTROY_EFFECT.preDestroy, sid: 51, parent: 0, flags: 1 });
    assert.ok(forwarded === 2 && dispatch.selected() === 0);
    assert.ok(throws(() => makePriestPreDestroyHooks(scene.pool, priest, {})));
});
