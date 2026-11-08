// buried 후처리 raw-buried-post-pop.js(RawBuriedPostPop)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: buried-postpop-x86.tsv 580행(입력 7개: boss·로딩·flags·할당 성공 마스크·QA·선택 타입·변이 종류 → 슬롯 50바이트·boss·로딩·호출 기록). 각 행은 독립 풀에서 실행한다.
//  - 계약: BuriedPostPop_RequiredBoundariesAndPartialWrites·BuriedRuneCurrent_*(미선택 장치 비요구, 추첨 뒤 변경한 수명/TYPE/Bridge DWORD, 도달한 통지 누락의 부분 쓰기,
//    첫 확보 실패 뒤 두 번째 생성자, 명시 Validate 분리, flags0 의 물리 SID 읽기).
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E60: buried 후처리 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { RawBuriedPostPop } from '../www/core/raw-buried-post-pop.js';

// 원본 관측 파일이다. 없으면 해당 검사를 건너뛴다. 첫 행은 열 이름 행이다. (2026-11-08 WB04-E60)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'buried-postpop-x86.tsv'));

// 예외가 나는지 본다. (2026-11-08 WB04-E60)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E60)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 원본과 같은 입력만 소유하고 출력은 실제 제품 몸체가 기록하는 장면이다. input 은 쉼표 목록 일곱 입력이다. (2026-11-08 WB04-E60)
function buriedScene(inputText) {
    const input = inputText.split(',').map((part) => Number(part) >>> 0);
    const pool = new SidPool(24000, { server: false });
    const scene = { pool, boss: input[0] !== 0, loading: input[1], mask: input[3], chosen: input[5], mutation: input[6], allocations: 0, trace: [] };
    // 관찰 대상 50 까지 실제 슬롯을 확보한다.
    for (;;) {
        const sid = pool.allocate(2);
        if (sid === 50) break;
        if (!sid || sid > 50) throw new Error('buried 검사 SID 준비 오류');
    }
    const raw = pool.allocated(50);
    raw.fill(0xa5);
    put(raw, 0, 0x50a748);
    raw[10] = 112;
    raw[11] = 0;
    raw[34] = 1;
    raw[40] = 0;
    put(raw, 12, input[4], 2);
    scene.event = (value) => scene.trace.push(value);
    // 외부 반환/변경 입력만 공급하고 제품 분기나 최종 QA 를 대신 계산하지 않는다.
    scene.hooks = () => ({
        chooseType: (tier) => {
            scene.event(`Choose,${tier}`);
            if (scene.mutation === 3) { scene.boss = false; scene.loading = 1; }
            return scene.chosen;
        },
        reserveRegular: () => {
            scene.event('Reserve,40');
            const success = ((scene.mask >>> scene.allocations) & 1) !== 0;
            scene.allocations += 1;
            if (scene.mutation === 1 && scene.allocations === 1) put(pool.allocated(50), 12, 0xbeef, 2);
            return success;
        },
        addRegular: (sid, event, delay) => {
            scene.event(`Regular,${event},${sid},${floatToBits(delay)}`);
            if (scene.mutation === 2 && event === 0) scene.loading = 1;
        },
        base: (sid, flags) => {
            scene.event(`Base,${flags}`);
            if (scene.mutation === 4) put(pool.allocated(sid), 12, 0x7777, 2);
        }
    });
    scene.make = (hooks = scene.hooks(), bridge = () => 82) => new RawBuriedPostPop(pool, () => scene.boss, () => scene.loading, hooks, bridge);
    scene.traceText = () => scene.trace.join(';');
    return scene;
}

test('BuriedPostPop: 원본 10.78 580행의 슬롯·boss·로딩·호출 기록이 같다(실패한 첫 할당도 다음 할당을 막지 않는다)', { skip: !rows && '원본 fixture 없음' }, () => {
    assert.equal(rows.length, 581);
    assert.equal(rows[0][0], 'input');
    // 각 원본 행은 독립된 현재 전역/풀에서 실행한다.
    for (let index = 1; index < rows.length; index += 1) {
        const row = rows[index];
        assert.equal(row.length, 5);
        const input = row[0].split(',').map((part) => Number(part) >>> 0);
        assert.equal(input.length, 7);
        const scene = buriedScene(row[0]);
        scene.make().postPop(50, input[2]);
        assert.equal(hex(scene.pool.slot(50)), row[1], `행 ${index}`);
        assert.equal(scene.boss, Number(row[2]) !== 0);
        assert.equal(scene.loading, Number(row[3]));
        assert.equal(scene.traceText(), row[4]);
    }
});

test('BuriedPostPop: 미선택 보상 추첨은 불필요하며 선택된 예약 실패는 앞선 QA 쓰기만 보존한다', () => {
    const scene = buriedScene('1,1,1,3,150,187,0');
    const hooks = scene.hooks();
    hooks.chooseType = undefined;
    const loading = scene.make(hooks);
    loading.postPop(50, 1);
    assert.equal(get(scene.pool.slot(50), 12, 2), 150);
    assert.equal(scene.allocations, 2);
    scene.loading = 0;
    assert.ok(throws(() => loading.postPop(50, 1)));
    assert.equal(get(scene.pool.slot(50), 12, 2), 150);
    const noReserve = scene.hooks();
    noReserve.reserveRegular = undefined;
    const missing = scene.make(noReserve);
    assert.ok(throws(() => missing.postPop(50, 1)));
    assert.equal(get(scene.pool.slot(50), 12, 2), 187);
    put(scene.pool.allocated(50), 0, 0x501dd0);
    assert.ok(!missing.handles(50) && throws(() => missing.validate(50, 1)), '명시 사전 계약과 이미 선택된 가상 몸체의 실제 진입을 구별한다');
});

test('BuriedRuneCurrent: 최초 비트가 없으면 미선택 장치/사전 검사를 건너뛰고 사용하지 않는 QA 주소에 타입 범위를 강제하지 않는다', () => {
    // 각 최초 비트가 없는 독립 입력을 원문 getter→공통 호출로 전달한다.
    for (const flags of [0, 2, 4]) {
        const scene = buriedScene('1,0,0,0,65535,187,0');
        const hooks = scene.hooks();
        hooks.chooseType = undefined;
        hooks.reserveRegular = undefined;
        hooks.addRegular = undefined;
        hooks.validateBase = () => { throw new Error('명시 사전 검사'); };
        const raw = scene.pool.allocated(50);
        put(raw, 0, 0);
        raw[11] = 1;
        raw[40] = 8;
        scene.make(hooks).postPop(50, flags);
        assert.equal(scene.traceText(), `Base,${flags}`);
        assert.equal(get(scene.pool.slot(50), 12, 2), 65535);
    }
});

test('BuriedRuneCurrent: 추첨 뒤 변경한 수명/TYPE/Bridge DWORD 에서도 WORD 저장과 조건부 통지 순서를 유지한다', () => {
    const scene = buriedScene('1,0,1,0,150,187,0');
    let bridge = 82;
    const hooks = scene.hooks();
    const raw = scene.pool.allocated(50);
    hooks.chooseType = (tier) => {
        assert.equal(tier, 5);
        scene.event('Choose');
        raw[11] = 1;
        raw[40] = 8;
        raw[10] = 74;
        bridge = 74;
        scene.boss = false;
        scene.loading = 1;
        return 0x12345678;
    };
    hooks.notifySurface = (sid) => {
        assert.ok(sid === 50 && get(scene.pool.slot(sid), 12, 2) === 0x5678);
        scene.event('Notify');
        raw[10] = 255;
        put(raw, 0, 0);
    };
    scene.make(hooks, () => bridge).postPop(50, 1);
    assert.equal(scene.traceText(), 'Choose;Notify;Reserve,40;Reserve,40;Base,1');
    assert.ok(scene.allocations === 2 && raw[11] === 1 && raw[10] === 255 && raw[40] === 8);
});

test('BuriedRuneCurrent: 도달한 통지가 없으면 먼저 저장한 QA 만 남기고 두 확보에는 도달하지 않는다', () => {
    const scene = buriedScene('1,0,1,3,150,187,0');
    const body = scene.make(scene.hooks(), () => 112);
    assert.ok(throws(() => body.postPop(50, 1)));
    assert.equal(get(scene.pool.slot(50), 12, 2), 187);
    assert.equal(scene.traceText(), 'Choose,5');
    assert.equal(scene.allocations, 0);
});

test('BuriedRuneCurrent: 현재 Bridge DWORD 를 BYTE 로 줄여 잘못된 통지를 선택하지 않는다', () => {
    const scene = buriedScene('1,0,1,0,150,187,0');
    scene.make(scene.hooks(), () => 112 + 256).postPop(50, 1);
    assert.equal(scene.traceText(), 'Choose,5;Reserve,40;Reserve,40;Base,1');
    assert.equal(get(scene.pool.slot(50), 12, 2), 187);
});

test('BuriedRuneCurrent: 첫 확보 실패 뒤에도 두 번째 성공한 생성자만 요구하고, 성공한 확보 뒤 생성자 누락은 앞선 쓰기를 보존한다', () => {
    const scene = buriedScene('0,0,1,2,65535,187,0');
    const hooks = scene.hooks();
    hooks.chooseType = undefined;
    scene.make(hooks).postPop(50, 1);
    assert.equal(scene.allocations, 2);
    assert.equal(scene.traceText(), 'Reserve,40;Reserve,40;Regular,1,50,1008981770;Base,1');
    const missing = buriedScene('1,0,1,1,150,187,0');
    const missingHooks = missing.hooks();
    missingHooks.addRegular = undefined;
    assert.ok(throws(() => missing.make(missingHooks).postPop(50, 1)));
    assert.ok(missing.allocations === 1 && get(missing.pool.slot(50), 12, 2) === 187 && missing.traceText() === 'Choose,5;Reserve,40');
});

test('BuriedRuneCurrent: 명시 Validate 는 실제 본문과 분리되고 flags0 도 물리 SID 를 읽은 뒤에만 공통 부모로 간다', () => {
    const scene = buriedScene('0,0,0,0,150,187,0');
    const hooks = scene.hooks();
    hooks.validateBase = () => { throw new Error('명시 사전 검사'); };
    const body = scene.make(hooks);
    assert.ok(throws(() => body.validate(50, 0)) && scene.trace.length === 0);
    body.postPop(50, 0);
    assert.equal(scene.traceText(), 'Base,0');
    const physical = buriedScene('0,0,0,0,150,187,0');
    assert.ok(throws(() => physical.make().postPop(24000, 0)) && physical.trace.length === 0);
    assert.ok(throws(() => new RawBuriedPostPop(physical.pool, true, () => 0, {})), 'getter 가 아닌 전역 입력은 거부');
});
