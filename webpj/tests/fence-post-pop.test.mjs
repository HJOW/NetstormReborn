// 썬 바리케이드 후처리 raw-fence-post-pop.js(RawFencePostPop)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: fence-postpop-x86.tsv 330행(입력 6개: flags·테마·이웃 마스크·할당 성공 마스크·연결 수·변이 종류 → 부모 50과 이웃 51~54 슬롯 250바이트·호출 기록). 각 행은 독립 풀에서 실행한다.
//  - 계약: FencePostPop_RequiredBoundariesAndPartialWrites·AssetTailCurrent_Fence*(명시 사전 검사 비삽입, flags0 의 비조회, 이웃 변이의 물리 소유자 쓰기, 연결 반환 DWORD 전체 저장, 연결선 삭제 누락·마지막 예약 누락의 부분 쓰기).
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E60: 썬 바리케이드 후처리 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { RawFencePostPop } from '../www/core/raw-fence-post-pop.js';

// 원본 관측 파일이다. 없으면 해당 검사를 건너뛴다. 첫 행은 열 이름 행이다. (2026-11-08 WB04-E60)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'fence-postpop-x86.tsv'));

// 예외가 나는지 본다. (2026-11-08 WB04-E60)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E60)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 부모 50 과 네 이웃 51~54 의 전체 입력을 원문과 같은 패턴으로 준비한 장면이다. input 은 쉼표 목록 여섯 입력이다. (2026-11-08 WB04-E60)
function fenceScene(inputText) {
    const input = inputText.split(',').map((part) => Number(part) >>> 0);
    const pool = new SidPool(24000, { server: false });
    const scene = { pool, theme: input[1], neighbors: input[2], mask: input[3], count: input[4], mutation: input[5], allocations: 0, lines: 0, trace: [] };
    // 관찰 대상 54 까지 실제 할당한다.
    for (;;) {
        const sid = pool.allocate(2);
        if (sid === 54) break;
        if (!sid || sid > 54) throw new Error('펜스 검사 슬롯 확보 실패');
    }
    // 각각의 현재 좌표와 owner/padding 을 독립적으로 설정한다.
    for (let number = 50; number <= 54; number += 1) {
        const raw = pool.allocated(number);
        raw.fill(0xa5);
        put(raw, 0, 0x509470);
        raw[10] = 136;
        raw[11] = 0;
        raw[34] = 1;
        raw[40] = 0;
        put(raw, 14, floatToBits(number + 0.25));
        put(raw, 18, floatToBits(number + 0.5));
    }
    scene.event = (value) => scene.trace.push(value);
    // 명시 외부 효과의 반환과 변경 입력만 제공하며 부모의 예약/쓰기는 계산하지 않는다.
    scene.hooks = () => ({
        theme: (sid) => {
            assert.equal(sid, 50);
            scene.event('Theme,50');
            if (scene.mutation === 1) pool.allocated(sid)[34] = 7;
            return scene.theme;
        },
        neighbor: (sid, direction) => {
            assert.equal(sid, 50);
            scene.event(`Neighbor,50,${direction}`);
            if (scene.mutation === 2 && direction === 0) pool.allocated(sid)[34] = 6;
            return (scene.neighbors & (1 << (direction / 2))) !== 0 ? 51 + direction / 2 : 0;
        },
        reserveRegular: () => {
            scene.event('Reserve,40');
            const success = ((scene.mask >>> scene.allocations) & 1) !== 0;
            scene.allocations += 1;
            if (scene.mutation === 3 && scene.allocations === 1) put(pool.allocated(51), 14, 0x42f00000);
            return success;
        },
        addRegular: (sid, event, delay) => scene.event(`Regular,${event},${sid},${floatToBits(delay)}`),
        clearLine: (points) => {
            scene.event(`Clear${points.map((point) => `,${floatToBits(point)}`).join('')}`);
            scene.lines += 1;
            if (scene.mutation === 4 && scene.lines === 1) {
                put(pool.allocated(52), 18, 0x42fa0000);
                pool.allocated(50)[34] = 5;
            }
        },
        connections: (sid) => {
            assert.equal(sid, 50);
            scene.event('Connections,50');
            if (scene.mutation === 5) put(pool.allocated(sid), 46, 77);
            return scene.count;
        },
        damageable: (sid, flags) => {
            assert.equal(sid, 50);
            scene.event(`Base,${flags}`);
            if (scene.mutation === 6) put(pool.allocated(sid), 12, 0x7777, 2);
        }
    });
    // 부모와 외부 효과가 바꾼 다섯 슬롯을 함께 대조한다(보존 영역을 포함한 전체 바이트).
    scene.objects = () => [50, 51, 52, 53, 54].map((number) => hex(pool.slot(number))).join('');
    scene.traceText = () => scene.trace.join(';');
    return scene;
}

test('FencePostPop: 원본 10.78 330행의 전체 250바이트와 호출 기록이 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    assert.equal(rows.length, 331);
    assert.equal(rows[0][0], 'input');
    // 매 입력을 독립 풀에서 재생해 앞선 원문의 결과를 재사용하지 않는다.
    for (let index = 1; index < rows.length; index += 1) {
        const row = rows[index];
        assert.equal(row.length, 3);
        const input = row[0].split(',').map((part) => Number(part) >>> 0);
        assert.equal(input.length, 6);
        const scene = fenceScene(row[0]);
        new RawFencePostPop(scene.pool, scene.hooks()).postPop(50, input[0]);
        assert.equal(hex(scene.pool.slot(50)).slice(0, 8), '70945000');
        assert.equal(scene.objects(), row[1], `행 ${index}`);
        assert.equal(scene.traceText(), row[2]);
    }
});

test('FencePostPop: 선택하지 않은 접두에는 하위 조회가 필요 없고 실패 시 앞선 owner 쓰기만 남는다', () => {
    const scene = fenceScene('1,0,0,63,4,0');
    const noTheme = scene.hooks();
    noTheme.theme = undefined;
    const missingTheme = new RawFencePostPop(scene.pool, noTheme);
    missingTheme.postPop(50, 2);
    assert.equal(scene.traceText(), 'Base,2');
    assert.ok(throws(() => missingTheme.postPop(50, 1)));
    const noCount = scene.hooks();
    noCount.connections = undefined;
    const missingCount = new RawFencePostPop(scene.pool, noCount);
    assert.ok(throws(() => missingCount.postPop(50, 1)));
    assert.ok(get(scene.pool.slot(50), 42) === 1 && get(scene.pool.slot(50), 46) === 0xa5a5a5a5 && scene.allocations === 0);
    const noBase = scene.hooks();
    noBase.damageable = undefined;
    const missingBase = new RawFencePostPop(scene.pool, noBase);
    assert.ok(throws(() => missingBase.postPop(50, 1)));
    assert.ok(get(scene.pool.slot(50), 46) === 4 && scene.allocations === 1);
    put(scene.pool.allocated(50), 0, 0x509930);
    assert.ok(!missingBase.handles(50) && throws(() => missingBase.validate(50, 1)), '명시 사전 계약과 이미 선택된 원문 몸체의 실제 실행을 구별한다');
});

test('AssetTailCurrent: 썬 바리케이드는 명시 사전 검사를 실행하지 않고 flags0 은 가상 표/TYPE/장치를 요구하지 않는다', () => {
    const scene = fenceScene('1,0,0,0,4,0');
    const hooks = scene.hooks();
    hooks.validateBase = () => { throw new Error('명시 사전 장치'); };
    const body = new RawFencePostPop(scene.pool, hooks);
    assert.ok(throws(() => body.validate(50, 1)) && scene.trace.length === 0);
    body.postPop(50, 1);
    assert.ok(get(scene.pool.slot(50), 42) === 1 && get(scene.pool.slot(50), 46) === 4 && scene.traceText().endsWith('Base,1'));
    const none = fenceScene('0,0,0,0,0,0');
    const raw = none.pool.physical(50);
    put(raw, 0, 0);
    raw[10] = 255;
    raw[11] = 9;
    const only = new RawFencePostPop(none.pool, { damageable: (sid, flags) => { assert.ok(sid === 50 && flags === 0); none.event('D'); } });
    assert.ok(!only.handles(50) && throws(() => only.validate(50, 0)));
    only.postPop(50, 0);
    assert.ok(none.traceText() === 'D' && none.pool.slot(50)[11] === 9);
});

test('AssetTailCurrent: 이웃 조회가 슬롯을 바꿔도 현재 owner 를 DWORD 로 쓰고 연결 반환값 DWORD 전체를 현재 물리에 저장한다', () => {
    const scene = fenceScene('1,0,0,0,4,0');
    const hooks = scene.hooks();
    hooks.neighbor = (sid, direction) => {
        scene.event(`N${direction}`);
        if (direction === 6) {
            const raw = scene.pool.physical(sid);
            raw[11] = 9;
            raw[34] = 7;
            put(raw, 0, 0);
        }
        return 0;
    };
    new RawFencePostPop(scene.pool, hooks).postPop(50, 1);
    assert.ok(scene.traceText().startsWith('Theme,50;N0;N2;N4;N6;'));
    assert.ok(get(scene.pool.slot(50), 42) === 7 && get(scene.pool.slot(50), 46) === 4 && scene.pool.slot(50)[11] === 9);
    const connection = fenceScene('1,0,0,0,4,0');
    const connectionHooks = connection.hooks();
    connectionHooks.connections = (sid) => {
        connection.event('C');
        const raw = connection.pool.physical(sid);
        raw[11] = 9;
        raw[34] = 8;
        put(raw, 0, 0);
        return 0x89abcdef;
    };
    new RawFencePostPop(connection.pool, connectionHooks).postPop(50, 1);
    assert.ok(get(connection.pool.slot(50), 42) === 1 && get(connection.pool.slot(50), 46) === 0x89abcdef && connection.pool.slot(50)[11] === 9);
    assert.ok(connection.traceText().endsWith('C;Reserve,40;Base,1'));
});

test('AssetTailCurrent: 연결선 삭제 누락은 두 확보와 owner 쓰기를 보존하고 마지막 예약 누락은 연결 DWORD 저장 뒤 드러난다', () => {
    const scene = fenceScene('1,0,5,0,4,0');
    const noClear = scene.hooks();
    noClear.clearLine = undefined;
    assert.ok(throws(() => new RawFencePostPop(scene.pool, noClear).postPop(50, 1)));
    assert.ok(scene.allocations === 2 && get(scene.pool.slot(50), 42) === 1 && get(scene.pool.slot(50), 46) === 0xa5a5a5a5 && !scene.traceText().includes('Connections'));
    const final = fenceScene('1,0,0,0,4,0');
    const noReserve = final.hooks();
    noReserve.reserveRegular = undefined;
    assert.ok(throws(() => new RawFencePostPop(final.pool, noReserve).postPop(50, 1)));
    assert.ok(get(final.pool.slot(50), 42) === 1 && get(final.pool.slot(50), 46) === 4 && final.traceText().endsWith('Connections,50'));
    assert.ok(throws(() => new RawFencePostPop({ edition: 'cd1072' }, {})), 'CD 판본은 거부');
});
