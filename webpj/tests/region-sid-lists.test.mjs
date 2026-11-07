// 표면 소유자(RegionOwner)·SID 건전성 검사(isSidRobust/squidIsRobust)·정적 SID 목록 등록 표(RawStateSidLists)를 원본 10.78 관측으로 대조한다(WB04-E26, cpppj RegionOwnerTests·SidRobustParentTests·StateSidListsTests 이식).
//  - region-owner-x86.tsv: 두 PE(originals 816행·originalCD 816행) 중 10.78 816행(정수 칸·float snap·CRT 절삭·TYPE/상태/소유자 조합).
//  - sid-robust-parent-x86.tsv 1012행: 두 부모의 모든 실패 비트·번호 경계·선택 장치 호출 순서·진단 재진입·32비트 주소 감김.
//  - state-sid-lists-x86.tsv 66행: 등록/해제/개수 정리의 표·헤더 전체 출력과 개수 쓰기 순서. 정적 용량 여덟 개와 등록 순서 아홉 개, 복제 등록·쓰기 중단 prefix 계약.
// 실제 Client 정리(ResetClientStateWorldStorage)·표면 저장소·사제 접두·다중선택 소유 Client 필드 연결은 해당 단위를 옮길 때 대조한다. (2026-11-08 WB04-E26: 세 모듈 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { bitsToFloat, readRows } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { RegionOwner } from '../www/core/raw-region-owner.js';
import { isSidRobust, squidIsRobust } from '../www/core/raw-sid-robust.js';
import {
    RawStateSidLists, registerStateSidList, resetRegisteredStateSidLists, unregisterStateSidList,
} from '../www/core/raw-state-sid-lists.js';

// 원본 관측 행이다. 파일이 없으면 검사를 건너뛴다. (2026-11-08 WB04-E26)
const fixture = (name) => readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', name));
const regionRows = fixture('region-owner-x86.tsv');
const robustRows = fixture('sid-robust-parent-x86.tsv');
const listRows = fixture('state-sid-lists-x86.tsv');

// 원문 DWORD 를 읽고(감김) 부호 있는 값으로 되돌린다. 큰 십진 입력도 BigInt 로 정확히 감는다. (2026-11-08 WB04-E26)
const number = (text) => Number(BigInt(text) & 0xffffffffn);
const signed = (text) => Number(BigInt.asIntN(32, BigInt(text)));
// 도달한 하위 실패를 잡는다. (2026-11-08 WB04-E26)
const failed = (action) => { try { action(); } catch { return true; } return false; };

// 독립 PE 가 고른 결과만 비교하며 정답 칸/소유자를 테스트에서 다시 계산하지 않는다.
test('RegionOwner: 원본10.78 816행(정수 칸·float snap·TYPE/상태/소유자)', { skip: !regionRows && '원본 fixture 없음' }, () => {
    const pool = new SidPool(32768);
    const surface = new Uint16Array(65536);
    let count = 0;
    for (const row of regionRows) {
        // 원본 10.78 PE 행만 쓴다. CD 10.72 는 범위 밖이다.
        if (row[0] !== 'originals') continue;
        assert.equal(row.length, 10);
        const [, operation, x, y, sid, terrain, actual, owner, state, expected] = row;
        const raw = pool.physical(5);
        raw[10] = number(actual);
        raw[11] = number(state);
        raw[34] = number(owner);
        surface.fill(number(sid));
        const getter = new RegionOwner(pool, surface, () => number(terrain));
        const observed = operation === 'Integer' ? getter.atCell(signed(x), signed(y)) : getter.atPoint(bitsToFloat(number(x)), bitsToFloat(number(y)));
        count += 1;
        assert.equal(observed, number(expected), `행 ${count} ${operation} ${x} ${y}`);
    }
    assert.equal(count, 816);
});

// 모든 상태와 type/owner/표면 교체가 복사본 없이 다음 조회에 보인다.
test('RegionOwner: 현재 raw 입력과 경계', () => {
    const pool = new SidPool(32768);
    const surface = new Uint16Array(65536);
    let terrain = 97;
    const raw = pool.physical(5);
    raw[10] = 97; raw[11] = 7; raw[34] = 255; surface[0] = 5;
    const getter = new RegionOwner(pool, surface, () => terrain);
    assert.equal(getter.atCell(0, 0), 255);
    assert.equal(getter.pool, pool);
    raw[34] = 2;
    assert.equal(getter.atPoint(0, 0), 2);
    terrain = 98;
    assert.equal(getter.atCell(0, 0), 0);
    raw[10] = 98;
    assert.equal(getter.atCell(0, 0), 2);
    surface[0] = 0;
    assert.equal(getter.atCell(0, 0), 0);
    assert.deepEqual([getter.atCell(-1, 0), getter.atCell(256, 0), getter.atCell(0, 256)], [0, 0, 0]);
    // 원문 표면 WORD 지도는 한 변 256 이어야 한다.
    assert.throws(() => new RegionOwner(pool, surface.subarray(0, 65535), () => terrain), TypeError);
    // 풀 용량 이상의 표면 번호는 현재 슬롯을 읽기 전에 거부한다.
    surface[1] = 40000;
    assert.throws(() => getter.atCell(1, 0), RangeError);
    // 유한하지 않은 좌표는 0 칸으로 절삭된다(원문 _ftol 의 무효 결과).
    surface[0] = 5;
    raw[10] = 98;
    assert.equal(getter.atPoint(Number.NaN, Number.POSITIVE_INFINITY), 2);
});

// 입력 열은 장치로만 쓰고 마지막 열은 원문 기대값이다.
test('SidRobust: 원본10.78 1012행(두 부모·모든 실패 비트·번호 경계·장치 순서·진단 재진입)', { skip: !robustRows && '원본 fixture 없음' }, () => {
    // 최근 진단 seed 와 합성 Pool 토큰이다(원본 입력과 같음).
    const OLD_ROBUST = 0xaaaa5555;
    const OLD_TYPE = 0x55bbdd66;
    const POOL = 0x14010000;
    class AssertionFailure extends Error {}
    let count = 0;
    for (const row of robustRows) {
        assert.equal(row.length, 18);
        count += 1;
        const input = BigInt(row[1]);
        const live = signed(row[2]);
        let checking = number(row[3]);
        let base = number(row[4]);
        let capacity = number(row[5]);
        const table = number(row[6]);
        const flags = number(row[7]);
        const kind = number(row[8]);
        const mode = number(row[9]);
        const pointer = row[0] === 'object' ? Number(BigInt.asUintN(32, input)) : Number(BigInt.asUintN(32, BigInt(base) + BigInt.asUintN(32, input) * 50n));
        const raw = new Uint8Array(50);
        new DataView(raw.buffer).setUint32(0, table, true);
        raw[10] = kind;
        raw[11] = flags;
        const diagnostics = { lastRobust: OLD_ROBUST, type: OLD_TYPE };
        const events = [];
        const state = () => `${diagnostics.lastRobust}:${diagnostics.type}`;
        const hooks = {
            poolBase: () => { events.push(`P:${base}:${state()}`); return base; },
            capacity: () => { events.push(`C:${capacity}:${state()}`); return capacity; },
            checking: () => { events.push(`K:${checking}:${state()}`); return checking; },
            memory: (token, width) => {
                events.push(`M:${token}:${width}:${state()}`);
                if (token < pointer || token - pointer > raw.length || width > raw.length - (token - pointer)) throw new RangeError('SID Native 입력 물리');
                return raw.subarray(token - pointer, token - pointer + width);
            },
            assertion: (expression, line) => {
                assert.deepEqual([expression, line], ['resolve( sid ).isRobust()', 0x218]);
                events.push(`A:${line}:${state()}`);
                if (mode === 1) throw new AssertionFailure('진단 중단');
                // 진단 재진입이 바꾼 현재 전역을 복귀 뒤 되돌리지 않는다.
                if (mode === 2) { diagnostics.lastRobust = 0x11223344; diagnostics.type = 0x55667788; checking = 0; base = POOL + 50; capacity = 7; }
            },
        };
        let result = false;
        let outcome = 'ok';
        try {
            result = row[0] === 'object' ? squidIsRobust(pointer, live, diagnostics, hooks) : isSidRobust(Number(BigInt.asIntN(32, input)), live, diagnostics, hooks);
        } catch (error) {
            if (!(error instanceof AssertionFailure)) throw error;
            outcome = 'assertion';
        }
        const label = `행 ${count} ${row[0]} ${row[1]}`;
        assert.equal(outcome, row[10], `${label} 종료`);
        assert.equal(result, number(row[11]) !== 0, `${label} 결과`);
        assert.deepEqual([diagnostics.lastRobust, diagnostics.type], [number(row[12]), number(row[13])], `${label} 진단`);
        assert.deepEqual([checking, base, capacity], [number(row[14]), number(row[15]), number(row[16])], `${label} 최종 전역`);
        assert.equal(events.length === 0 ? '-' : events.join(';'), row[17], `${label} 장치 순서`);
    }
    assert.equal(count, 1012);
});

// 번호 범위 밖과 live 0/checking 0 은 pool 범위나 진단 초기화를 더하지 않는다.
test('SidRobust: 미선택 번호·live·checking 은 풀을 요구하지 않는다', () => {
    const diagnostics = { lastRobust: 0xaaaa5555, type: 0x55bbdd66 };
    assert.equal(isSidRobust(-1, 1, diagnostics, {}), false);
    assert.equal(isSidRobust(0, 0, diagnostics, {}), false);
    assert.equal(isSidRobust(65535, 1, diagnostics, {}), false);
    assert.equal(isSidRobust(65536, 0, diagnostics, {}), false);
    assert.equal(isSidRobust(1, 0, diagnostics, {}), true);
    assert.equal(isSidRobust(65534, 0, diagnostics, {}), true);
    assert.equal(isSidRobust(65534, 1, diagnostics, { checking: () => 0 }), true);
    assert.deepEqual([diagnostics.lastRobust, diagnostics.type], [0xaaaa5555, 0x55bbdd66]);
    // live 이고 checking 장치가 없으면 그 위치에서 거부한다.
    assert.throws(() => isSidRobust(5, 1, diagnostics, {}), /미연결/);
});

// 같은 숫자 SID 가 재사용되면 현재 슬롯을 읽으며 옛 포인터나 별도 세대 비트를 보관하지 않는다.
test('SidRobust: 현재 호스트 풀 물리와 세대 비트 없는 번호', () => {
    const pool = new SidPool(24000, { server: false });
    const BASE = 0x100000;
    let checking = 1;
    let assertions = 0;
    // 호스트 포인터 폭 모드에서 가상 토큰을 현재 풀 바이트로 되짚어 읽는다.
    const hooks = {
        addressMode: 'host',
        poolBase: () => BASE,
        capacity: () => pool.capacity,
        checking: () => checking,
        memory: (token, width) => {
            const bytes = pool.bytes();
            if (token < BASE || token - BASE > bytes.length || width > bytes.length - (token - BASE)) throw new RangeError('SID 검사 현재 Pool 물리');
            return bytes.subarray(token - BASE, token - BASE + width);
        },
        assertion: (expression, line) => { assert.deepEqual([expression, line], ['resolve( sid ).isRobust()', 0x218]); assertions += 1; },
    };
    const diagnostics = { lastRobust: 0, type: 0 };
    const sid = pool.allocate(2);
    let raw = pool.allocated(sid);
    new DataView(raw.buffer, raw.byteOffset, 4).setUint32(0, 0x12345678, true);
    raw[10] = 7;
    assert.equal(isSidRobust(sid, 1, diagnostics, hooks), true);
    assert.deepEqual([diagnostics.lastRobust, diagnostics.type, assertions], [0, 7, 0]);
    // free/dead 상세 실패라도 진단이 복귀하면 부모는 true 이다.
    raw[11] = 3;
    assert.equal(isSidRobust(sid, 1, diagnostics, hooks), true);
    assert.deepEqual([diagnostics.lastRobust, diagnostics.type, assertions], [12, 7, 1]);
    pool.reset(24500);
    assert.equal(isSidRobust(sid, 1, diagnostics, hooks), true);
    assert.deepEqual([diagnostics.lastRobust, diagnostics.type, assertions], [22, 0, 2]);
    const reused = pool.allocate(2);
    assert.equal(reused, sid);
    raw = pool.allocated(reused);
    new DataView(raw.buffer, raw.byteOffset, 4).setUint32(0, 0x12345678, true);
    raw[10] = 15;
    assert.equal(isSidRobust(sid, 1, diagnostics, hooks), true);
    assert.deepEqual([diagnostics.lastRobust, diagnostics.type, assertions], [0, 15, 2]);
    // checking 이 꺼지면 풀 상한보다 큰 번호도 상세 검사를 합성하지 않는다.
    checking = 0;
    assert.equal(isSidRobust(65534, 1, diagnostics, hooks), true);
    assert.deepEqual([diagnostics.lastRobust, diagnostics.type, assertions], [0, 15, 2]);
});

// 원문의 잘못된 물리 읽기를 정상값으로 대신하지 않고 선행 진단 쓰기만 보존한다.
test('SidRobust: TYPE 읽기가 논리 범위보다 앞서고 물리 부족은 거기서 멈춘다', () => {
    const OLD_TYPE = 0x55bbdd66;
    const diagnostics = { lastRobust: 0xaaaa5555, type: OLD_TYPE };
    const hooks = { memory: () => new Uint8Array(0) };
    assert.throws(() => squidIsRobust(1000, 1, diagnostics, hooks), RangeError);
    // poolBase/capacity 가 미연결이어도 먼저 부족한 물리에서 끝난다.
    assert.deepEqual([diagnostics.lastRobust, diagnostics.type], [0, OLD_TYPE]);
    hooks.memory = (token, width) => { assert.deepEqual([token, width], [1010, 1]); return Uint8Array.of(255); };
    hooks.poolBase = () => 1000;
    assert.equal(squidIsRobust(1000, 1, diagnostics, hooks), false);
    // 하한 실패는 capacity 조차 요구하지 않고 type 현재값은 이미 썼다.
    assert.deepEqual([diagnostics.lastRobust, diagnostics.type], [1, 255]);
});

// 표·헤더 전체 출력(canary 포함)과 reset 의 개수 쓰기 순서를 원문 관측에서 읽는다.
test('StateSidLists: 원본10.78 66행(등록·해제·개수 정리의 표/헤더/쓰기 순서)', { skip: !listRows && '원본 fixture 없음' }, () => {
    const REGISTRY = 0x592938;
    const HEADER = 0x12000100;
    const bytesOf = (hex) => Uint8Array.from(Buffer.from(hex, 'hex'));
    let rows = 0;
    let resetRows = 0;
    for (const row of listRows) {
        assert.equal(row.length, 10);
        const table = bytesOf(row[4]);
        const headers = bytesOf(row[5]);
        const tableView = new DataView(table.buffer);
        const headerView = new DataView(headers.buffer);
        // 실제 CPU 입력을 선택 토큰으로만 해석한다.
        const pointers = Array.from({ length: 20 }, (_, index) => tableView.getUint32(16 + index * 4, true));
        const stores = [];
        const argument = number(row[3]);
        if (row[0] === 'register') registerStateSidList(pointers, argument);
        else if (row[0] === 'unregister') unregisterStateSidList(pointers, argument);
        else {
            resetRows += 1;
            resetRegisteredStateSidLists(pointers, (header) => {
                const target = header + 12;
                stores.push(`${target}:4:0`);
                if (target >= REGISTRY && target < REGISTRY + 80) pointers[(target - REGISTRY) / 4] = 0;
                else {
                    assert.ok(target >= HEADER && target < HEADER + 640, `헤더 ${header}`);
                    headerView.setUint32(16 + target - HEADER, 0, true);
                }
            });
        }
        // 비활성 포인터와 표 이웃 32바이트도 전체 비교한다.
        pointers.forEach((value, index) => tableView.setUint32(16 + index * 4, value, true));
        const label = `행 ${rows + 1} ${row[0]} ${row[1]}`;
        assert.equal(Buffer.from(table).toString('hex'), row[6], `${label} 표`);
        assert.equal(Buffer.from(headers).toString('hex'), row[7], `${label} 헤더`);
        if (row[0] === 'reset') assert.equal(stores.length === 0 ? '-' : stores.join(';'), row[8], `${label} 쓰기 순서`);
        assert.ok(Number(row[9]) > 0);
        rows += 1;
    }
    assert.deepEqual([rows, resetRows], [66, 22]);
});

// 정적 용량 여덟 개와 등록 순서 아홉 개이다.
test('StateSidLists: 정확한 정적 용량 여덟 개와 실제 헤더 아홉 개', () => {
    const lists = new RawStateSidLists();
    const owners = [lists.factories, lists.dudes, lists.outposts, lists.providers, lists.geysers, lists.priests, lists.residences, lists.temples];
    const capacities = [500, 30, 100, 256, 100, 16, 50, 20];
    owners.forEach((owner, index) => assert.deepEqual([owner.entries.length, owner.count], [capacities[index], 0]));
    assert.deepEqual(lists.registeredHeaders.slice(0, 9), [0x55a4d0, 0x55a4ec, 0x55a708, 0x592f80, 0x5950d8, 0x5954c0, 0x59a9d4, 0x5caf28, 0x5e1898]);
    assert.ok(lists.registeredHeaders.slice(9).every((value) => value === 0));
});

// 선택된 쓰기 실패는 완료 prefix 를 보존하고 빈 표에는 효과 함수를 요구하지 않는다.
test('StateSidLists: 쓰기 중단 prefix·복제 등록·짧은 표 거부', () => {
    const headers = new Array(21).fill(0);
    headers[20] = 0x87654321;
    resetRegisteredStateSidLists(headers, undefined);
    assert.ok(failed(() => registerStateSidList(headers.slice(0, 19), 1)));
    registerStateSidList(headers, 7);
    registerStateSidList(headers, 7);
    registerStateSidList(headers, 0);
    assert.deepEqual([headers[0], headers[1], headers[2], headers[20]], [7, 7, 0, 0x87654321]);
    let count = 9;
    let calls = 0;
    assert.ok(failed(() => resetRegisteredStateSidLists(headers, (header) => { assert.equal(header, 7); calls += 1; if (calls === 2) throw new Error('명시 실제 store 중단'); count = 0; })));
    assert.deepEqual([calls, count, headers[0], headers[1], headers[20]], [2, 0, 7, 7, 0x87654321]);
    // 해제는 중복을 포함한 모든 일치 칸만 지운다.
    unregisterStateSidList(headers, 7);
    assert.deepEqual([headers[0], headers[1], headers[20]], [0, 0, 0x87654321]);
    assert.ok(failed(() => resetRegisteredStateSidLists(headers.slice(0, 19), undefined)));
    // 비영 헤더에 도달했는데 쓰기 장치가 없으면 거부한다.
    headers[3] = 0x55a4d0;
    assert.throws(() => resetRegisteredStateSidLists(headers, undefined), /미연결/);
});

// count 만 지우고 데이터/용량/등록표를 보존하며 다중선택은 빌린 Client 필드를 쓴다. 일반 fresh 는 같은 주소에서 비운다.
test('StateSidLists: 개수 정리·다중선택 대여·일반 fresh 정책', () => {
    const lists = new RawStateSidLists();
    const selection = { multipleCount: -7 };
    const owners = [lists.factories, lists.dudes, lists.outposts, lists.providers, lists.geysers, lists.priests, lists.residences, lists.temples];
    const pointers = owners.map((owner) => owner.entries);
    owners.forEach((owner, index) => { owner.entries.fill(0xa5000000 + index); owner.count = index === 0 ? 0xffffffff : index + 1; });
    const registry = [...lists.registeredHeaders];
    resetRegisteredStateSidLists(lists.registeredHeaders, (header) => lists.resetCount(header, selection));
    owners.forEach((owner, index) => {
        assert.equal(owner.count, 0);
        assert.equal(owner.entries, pointers[index]);
        assert.ok(owner.entries.every((value) => value === 0xa5000000 + index), '비활성 데이터 보존');
    });
    assert.deepEqual([lists.registeredHeaders, selection.multipleCount], [registry, 0]);
    assert.throws(() => lists.resetCount(0x12345678, selection), RangeError);
    // 일반 fresh 만 같은 배열 주소에서 내용과 개수를 비운다.
    owners.forEach((owner) => { owner.entries[owner.entries.length - 1] = 17; owner.count = 1; });
    lists.prepareGeneralWorld();
    owners.forEach((owner, index) => {
        assert.equal(owner.count, 0);
        assert.equal(owner.entries, pointers[index]);
        assert.ok(owner.entries.every((value) => value === 0));
    });
});
