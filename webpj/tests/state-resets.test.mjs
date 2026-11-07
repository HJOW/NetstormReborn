// State 정리의 소형 몸체 다섯 개를 원본 10.78 관측으로 대조한다(WB04-E11, cpppj ClockSyncResetTests·StateStorageCleanupTests·StateBridgeResetTests·RitualRefreshTests·BulwarkHitTests 이식).
//  - clock-sync-reset-x86.tsv 64행(96 BYTE 두 영역 전체와 인접 카운터 보존), state-zone-reset-x86.tsv 10행 + 길이 0..8 경계(정확히 6 BYTE),
//    state-bridge-reset-x86.tsv 64행(소유자 색 9칸·다리 붕괴 스캔의 212 BYTE 네 영역), ritual-refresh-x86.tsv 54행(두 현재 owner load·세 자식·각 중단 부분 효과),
//    bulwark-hit-x86.tsv 492행(비행 속성·공격자 0·생존 필터 부재·일반 피해 인수/반환 폭).
//  - 계약: 빌린 DWORD 만 쓰고 이웃은 보존, 짧은 물리는 prefix 를 남기고 거부, 선택된 장치 누락은 앞선 효과를 남김.
// cpppj 의 Client/World 통합 검사(실제 ClientStateStorageCleanup 연결)는 해당 단위를 옮길 때 대조한다. (2026-11-08 WB04-E11: State 정리 소형 몸체 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, bitsToFloat } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { resetClockSynchronization } from '../www/core/raw-clock-sync-reset.js';
import { resetStateZone } from '../www/core/raw-state-zone-reset.js';
import { resetStateOwnerColors, resetStateBridgeDecayScan } from '../www/core/raw-state-bridge-reset.js';
import { refreshLocalRitualData } from '../www/core/raw-ritual-refresh.js';
import { rawBulwarkHit } from '../www/core/raw-bulwark-hit.js';

// 원본 관측 행이다. 파일이 없으면 검사를 건너뛴다. (2026-11-08 WB04-E11)
const fixture = (name) => readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', name));
const syncRows = fixture('clock-sync-reset-x86.tsv');
const zoneRows = fixture('state-zone-reset-x86.tsv');
const bridgeRows = fixture('state-bridge-reset-x86.tsv');
const ritualRows = fixture('ritual-refresh-x86.tsv');
const bulwarkRows = fixture('bulwark-hit-x86.tsv');

// 소문자 hex 로 표시하거나 hex 를 바이트로 읽는다. (2026-11-08 WB04-E11)
const hex = (bytes) => Buffer.from(bytes).toString('hex');
const unhex = (text) => new Uint8Array(Buffer.from(text, 'hex'));

// 96 BYTE hex 를 24 DWORD 저장소로 읽고 되돌린다. (2026-11-08 WB04-E11)
function syncWords(text) {
    assert.equal(text.length, 192);
    const bytes = unhex(text);
    const words = new Uint32Array(24);
    for (let index = 0; index < 24; index += 1) words[index] = get(bytes, index * 4);
    return words;
}
function syncHex(words) {
    const bytes = new Uint8Array(96);
    for (let index = 0; index < 24; index += 1) put(bytes, index * 4, words[index]);
    return hex(bytes);
}

// 세 참조(connection·replyMode·gameTicks)를 서로 다른 저장소의 DWORD 에 묶는 대여 객체이다. (2026-11-08 WB04-E11)
function syncGlobals(sync, clock) {
    return {
        get connection() { return sync[5]; }, set connection(value) { sync[5] = value; },
        get replyMode() { return sync[8]; }, set replyMode(value) { sync[8] = value; },
        get gameTicks() { return clock[3]; }, set gameTicks(value) { clock[3] = value; },
    };
}

// 제품 전 고정한 두 정밀도·반복 전체 64관측과 같은 실제 저장소를 대조한다.
test('시간 동기화 초기화: 원본10.78 64행(96 BYTE 두 영역 전체)', { skip: !syncRows && '원본 fixture 없음' }, () => {
    let key = '';
    let sync = new Uint32Array(24);
    let clock = new Uint32Array(24);
    let rows = 0;
    let first = 0;
    let repeated = 0;
    for (const fields of syncRows) {
        assert.equal(fields.length, 13);
        const current = `${fields[0]}:${fields[1]}`;
        // 제어어·입력별 반복 두 번이 같은 저장소를 쓴다.
        if (current !== key) { key = current; sync = syncWords(fields[6]); clock = syncWords(fields[7]); assert.equal(fields[2], '0'); }
        if (fields[2] === '0') first += 1; else { assert.equal(fields[2], '1'); repeated += 1; }
        resetClockSynchronization(syncGlobals(sync, clock));
        assert.equal(syncHex(sync), fields[8], `행 ${rows} sync`);
        assert.equal(syncHex(clock), fields[9], `행 ${rows} clock`);
        rows += 1;
    }
    assert.deepEqual([rows, first, repeated], [64, 32, 32]);
});

// 실제 세 참조를 재사용하고 인접 카운터·별도 시간·기록을 보존하며 바인딩 자체는 쓰지 않는다.
test('시간 동기화 초기화: 빌린 DWORD 만 쓰고 이웃을 보존한다', () => {
    const words = new Uint32Array([11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22]);
    const before = words.slice();
    const globals = { get connection() { return words[1]; }, set connection(v) { words[1] = v; }, get replyMode() { return words[5]; }, set replyMode(v) { words[5] = v; }, get gameTicks() { return words[9]; }, set gameTicks(v) { words[9] = v; } };
    assert.deepEqual(words, before, '바인딩만으로는 쓰지 않는다');
    resetClockSynchronization(globals);
    for (let word = 0; word < words.length; word += 1) assert.equal(words[word], (word === 1 || word === 5 || word === 9) ? 0 : before[word]);
    // 같은 owner 의 현재 값이 바뀌어도 이전 참조를 재사용한다.
    words[1] = 0xffffffff; words[5] = 0x80000000; words[9] = 0x12345678; words[2] = 99;
    resetClockSynchronization(globals);
    assert.deepEqual([words[1], words[5], words[9], words[2], words[3], words[10]], [0, 0, 0, 99, before[3], before[10]]);
});

// 원문 9 BYTE 가 남기는 전체 24 BYTE 와 두 store 의 폭은 최초 PE 10관측에서만 읽는다.
test('State Zone 초기화: 원본10.78 10행과 정확한 6 BYTE', { skip: !zoneRows && '원본 fixture 없음' }, () => {
    let rows = 0;
    for (const row of zoneRows) {
        assert.equal(row.length, 11);
        const bytes = new Uint8Array(24);
        const seed = Number(row[0]);
        // CPU 실행 전과 같은 명시 입력이다.
        for (let byte = 0; byte < bytes.length; byte += 1) bytes[byte] = (seed + byte * 17) & 0xff;
        resetStateZone(bytes);
        assert.equal(row[6], 'normal');
        assert.equal(hex(bytes), row[7], `행 ${rows}`);
        assert.equal(row[9], '0:4:0;4:2:0');
        rows += 1;
    }
    assert.equal(rows, 10);
    // 선행 전체 폭 검사나 8 BYTE 0 쓰기가 첫 DWORD prefix/인접 WORD 를 잃는지 구별한다.
    for (let length = 0; length <= 8; length += 1) {
        const bytes = new Uint8Array(10).fill(0xa5);
        let rejected = false;
        try { resetStateZone(bytes.subarray(0, length)); } catch (error) { rejected = error instanceof RangeError; }
        assert.equal(rejected, length < 6, `길이 ${length}`);
        const changed = length < 4 ? 0 : length < 6 ? 4 : 6;
        for (let byte = 0; byte < bytes.length; byte += 1) assert.equal(bytes[byte], byte < changed ? 0 : 0xa5, `길이 ${length} 바이트 ${byte}`);
    }
});

// 두 실제 몸체의 전체 네 영역 212 BYTE 와 실제 store 의 주소/폭을 원문 기대에서 대조한다.
test('State 다리 초기화: 원본10.78 64행(색 9칸·붕괴 스캔 212 BYTE)', { skip: !bridgeRows && '원본 fixture 없음' }, () => {
    const view = (bytes) => new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
    let rows = 0;
    for (const fields of bridgeRows) {
        assert.equal(fields.length, 8);
        const before = unhex(fields[3]);
        const wanted = unhex(fields[4]);
        assert.deepEqual([before.length, wanted.length], [212, 212]);
        const actual = before.slice();
        const stores = fields[5].split(';');
        if (fields[0] === 'colors') {
            const colors = new Int32Array(9);
            // Native 의 실제 시작 색 표만 입력한다.
            for (let owner = 0; owner < 9; owner += 1) colors[owner] = get(before, 16 + owner * 4) | 0;
            resetStateOwnerColors(colors);
            assert.equal(stores.length, 9);
            for (let owner = 0; owner < 9; owner += 1) {
                put(actual, 16 + owner * 4, colors[owner] >>> 0);
                const store = stores[owner].split(':');
                assert.ok(store.length === 3 && Number(store[0]) === 0x531b08 + owner * 4 && store[1] === '4');
            }
        } else {
            assert.equal(fields[0], 'scan');
            const state = { first: get(before, 88) | 0, last: get(before, 92) | 0, cursor: get(before, 124) | 0, next: view(before).getFloat64(140, true) };
            resetStateBridgeDecayScan(state, bitsToFloat(get(before, 84)), view(before).getFloat64(188, true));
            put(actual, 88, state.first >>> 0); put(actual, 92, state.last >>> 0); put(actual, 124, state.cursor >>> 0);
            view(actual).setFloat64(140, state.next, true);
            const addresses = [0x52f96c, 0x52f970, 0x5453a8, 0x5453b8];
            assert.equal(stores.length, addresses.length);
            for (let index = 0; index < addresses.length; index += 1) {
                const store = stores[index].split(':');
                assert.ok(store.length === 3 && Number(store[0]) === addresses[index] && Number(store[1]) === (index === 3 ? 8 : 4));
            }
        }
        assert.equal(hex(actual), hex(wanted), `행 ${rows} ${fields[0]}`);
        rows += 1;
    }
    assert.equal(rows, 64);
});

// 색 표의 첫 9칸만 쓰고 host 범위 부족은 어떤 prefix 도 쓰기 전에 거부한다.
test('State 다리 초기화: 색 표 물리 경계와 열째 칸 보존', () => {
    const colors = new Int32Array(10).fill(-123);
    const before = colors.slice();
    assert.throws(() => resetStateOwnerColors(colors.subarray(0, 8)), RangeError);
    assert.deepEqual(colors, before);
    resetStateOwnerColors(colors);
    assert.deepEqual([colors[0], colors[8], colors[9]], [0, 8, -123]);
});

// 상위 희생을 실행하지 않고 로컬 완료 분기 54개 원문 관측의 순서/부분 효과를 비교한다.
test('의식 뒤 로컬 갱신: 원본10.78 54행(두 현재 owner·세 자식·부분 효과)', { skip: !ritualRows && '원본 fixture 없음' }, () => {
    class Stop extends Error {}
    let count = 0;
    for (const fields of ritualRows) {
        assert.equal(fields.length, 9);
        // 원문 자식의 독립 반환/현재 owner 변경만 공급한다.
        const probe = { first: Number(fields[0]), second: Number(fields[1]), stop: Number(fields[2]), events: 0, owner: 7, core: false, snapshot: false, written: null, trace: [] };
        const event = (value) => { probe.trace.push(value); probe.events += 1; if (probe.events === probe.stop) throw new Stop(); };
        const hooks = {
            core: () => { event('C'); probe.core = true; probe.owner = probe.first; },
            templateOwner: () => { const value = probe.owner; event(`O,${value}`); return value; },
            coreData: (selected) => { event(`D,${selected}`); probe.written = selected; probe.owner = probe.second; },
            snapshot: () => { event('S'); probe.snapshot = true; },
        };
        let completed = true;
        try { refreshLocalRitualData(hooks); } catch (error) { if (error instanceof Stop) completed = false; else throw error; }
        const label = `행 ${count} ${fields.join(',')}`;
        assert.equal(completed, fields[3] === '1', `${label} 완료`);
        assert.equal(probe.trace.join(';'), fields[4], `${label} 순서`);
        assert.equal(probe.owner, Number(fields[5]), `${label} owner`);
        assert.equal(probe.core, fields[6] === '1', `${label} Core`);
        assert.equal(probe.snapshot, fields[8] === '1', `${label} snapshot`);
        assert.equal(probe.written !== null, fields[7] !== '-', `${label} 기록 여부`);
        if (probe.written !== null) assert.equal(probe.written, Number(fields[7]), `${label} 기록 owner`);
        count += 1;
    }
    assert.equal(count, 54);
});

// 도달한 기록 경계 누락은 Core/첫 현재 읽기를 보존하고 snapshot 성공으로 바꾸지 않는다.
test('의식 뒤 로컬 갱신: 선택된 장치 누락은 앞선 효과를 남긴다', () => {
    const trace = [];
    let core = false;
    const hooks = { core: () => { trace.push('C'); core = true; }, templateOwner: () => { trace.push('O,8'); return 8; }, snapshot: () => trace.push('S') };
    assert.throws(() => refreshLocalRitualData(hooks), /선택 장치 미연결/);
    assert.deepEqual([core, trace.join(';')], [true, 'C;O,8']);
});

// 비행 속성/공격자 0/생존 필터 부재와 일반 피해의 인수·반환 폭을 확인한다.
test('불워크 피해 관문: 원본10.78 492행(효과·두 슬롯·전체 반환)', { skip: !bulwarkRows && '원본 fixture 없음' }, () => {
    let count = 0;
    for (const row of bulwarkRows) {
        assert.equal(row.length, 4);
        // 부호 비트도 DWORD 로 보존한다.
        const a = row[0].split(',').map((part) => Number(BigInt(part) & 0xffffffffn));
        assert.equal(a.length, 8);
        const pool = new SidPool(24000);
        const types = Array.from({ length: 188 }, () => ({ flags2: 0 }));
        const victim = 50;
        const source = 60;
        // free/dead 입력도 원문 그대로 설치한다(주변 바이트를 포함하는 원래 50 BYTE 입력).
        for (const sid of [victim, source]) {
            const raw = pool.physical(sid);
            raw.fill(0);
            put(raw, 0, sid === victim ? 0x50ea08 : 0x12200000);
            raw[10] = (sid === victim ? 79 : a[1]) & 0xff;
            raw[34] = sid === victim ? 2 : 1;
            if (sid === victim) put(raw, 26, 0x12345678);
            else { raw[11] = a[3] & 0xff; raw[40] = a[4] & 0xff; }
        }
        types[a[1]].flags2 = a[2];
        let effects = '';
        const result = rawBulwarkHit(pool, types, victim, a[5] | 0, a[6], a[0] & 0xffff, (target, amount, owner, attacker) => {
            effects = `H:${target}:${amount >>> 0}:${owner}:${attacker}`;
            put(pool.physical(target), 26, (0x12345678 - (amount >>> 0)) >>> 0);
            pool.physical(target)[34] = owner & 0xff;
            if (attacker) pool.physical(attacker)[10] = 70;
            return a[7];
        });
        const raws = hex(pool.slot(victim)) + hex(pool.slot(source));
        const label = `원문행 ${count}: ${row[0]}`;
        assert.equal(effects || '-', row[1], `${label} 효과`);
        assert.equal(raws, row[2], `${label} 슬롯`);
        assert.equal(result, Number(row[3]), `${label} 반환`);
        count += 1;
    }
    assert.equal(count, 492);
});

// 일반 피해 장치가 없으면 비행체 관문을 통과한 뒤에만 거부하고, 판본/TYPE 표 크기를 첫 효과 전에 거부한다.
test('불워크 피해 관문: 장치 누락·표 크기 계약', () => {
    const pool = new SidPool(24000);
    const types = Array.from({ length: 188 }, () => ({ flags2: 0 }));
    assert.throws(() => rawBulwarkHit(pool, types, 50, 1, 2, 0, null), /일반 피해 미연결/);
    assert.throws(() => rawBulwarkHit(pool, types.slice(0, 10), 50, 1, 2, 0, () => 0), TypeError);
    // 공격자 TYPE 이 비행체이면 일반 피해 장치 없이 0 을 돌려준다.
    pool.physical(60).fill(0);
    pool.physical(60)[10] = 5;
    types[5].flags2 = 0x100000;
    assert.equal(rawBulwarkHit(pool, types, 50, 1, 2, 60, null), 0);
});
