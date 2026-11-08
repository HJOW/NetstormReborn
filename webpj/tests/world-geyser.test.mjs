// 세계 간헐천 raw-world-geyser-bounds.js(청크 범위·간격·예산)와 raw-world-geyser.js(생성 부모)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: world-geyser-bounds-x86.tsv 820행(연산 W 전체/P 분할/B 예산 — 모드·크기 옵션·셀/격자·두 지역 청크 정수·변이 5종·Player 존재/숨김·battle → 16바이트 x87 사각형·공유 여백 DWORD·예산·청크/간격 호출 순서·모드·크기)
//    world-geyser-x86.tsv 354행(전투·Player 존재/숨김·간격/밀도/모드/장식 옵션·predictableGeysers·벽시각 비트·변이 10종·난수 소비 → 로그/로컬/장식/요청 순서와 전체 인자·옵션 87바이트·Player 1620바이트·장식 타입·demo).
//  - 계약: 미선택 하위는 요구하지 않고 선택된 하위 누락은 선행 효과를 남기며, 표 밖 옵션과 Player 표 크기를 거부한다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E107: 세계 간헐천 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, floatToBits } from './raw-support.mjs';
import { GameRandom } from '../www/core/random.js';
import { RawWorldGeyserBounds, worldGeyserBudget, worldGeyserSpacing } from '../www/core/raw-world-geyser-bounds.js';
import { RawWorldGeyser, timeSeed, worldGeyserDensity } from '../www/core/raw-world-geyser.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E107)
const fixtures = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures');
const boundsRows = readRows(path.join(fixtures, 'world-geyser-bounds-x86.tsv'));
const geyserRows = readRows(path.join(fixtures, 'world-geyser-x86.tsv'));

// 예외가 나는지 본다. (2026-11-08 WB04-E107)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

const hex = (bytes) => Buffer.from(bytes).toString('hex');
const bits = floatToBits;
const num = (text) => Number(BigInt.asUintN(32, BigInt(text)));

// 사각형을 원문 16바이트 little endian 문자열로 보존한다. (2026-11-08 WB04-E107)
function rectangleHex(rectangle) {
    const bytes = new Uint8Array(16);
    put(bytes, 0, bits(rectangle.left));
    put(bytes, 4, bits(rectangle.top));
    put(bytes, 8, bits(rectangle.right));
    put(bytes, 12, bits(rectangle.bottom));
    return hex(bytes);
}

test('WorldGeyserBounds: 원본 10.78 820입력의 전체/분할 사각형·공유 여백·예산·청크/간격 호출 순서·모드·크기가 같다', { skip: !boundsRows && '원본 fixture 없음' }, () => {
    let count = 0;
    // 사례마다 모드/옵션/Player/여백을 독립 초기화한다.
    for (const row of boundsRows) {
        assert.equal(row.length, 23);
        let mission = num(row[1]) !== 0;
        const battle = num(row[16]) !== 0;
        let size = num(row[2]) & 0xff;
        let margin = 0x12345678;
        const mapLimit = 256;
        let calls = 0;
        const alter = num(row[13]);
        const trace = [];
        const hooks = {
            // 큰 ChunkMap 의 명시 입력과 앞선 하위의 현재 모드/옵션 변경만 공급한다.
            chunkBounds: (region) => {
                trace.push(`C,${region}`);
                calls += 1;
                if (calls === 1) {
                    if (alter === 1 || alter === 5) mission = true;
                    else if (alter === 2) mission = false;
                    else if (alter === 3) size = 3;
                    else if (alter === 4) size = 0;
                } else if (calls === 2 && alter === 5) mission = false;
                const at = region === 0 ? 5 : 9;
                return { left: Number(row[at]) | 0, top: Number(row[at + 1]) | 0, right: Number(row[at + 2]) | 0, bottom: Number(row[at + 3]) | 0 };
            },
            // 원본 getter 본문과 같은 실제 표를 실행한다.
            spacing: () => { trace.push('S'); return worldGeyserSpacing(size); }
        };
        const state = { get missionPost() { return mission; }, get mapLimit() { return mapLimit; }, get spacingMargin() { return margin; }, set spacingMargin(value) { margin = value; } };
        const body = new RawWorldGeyserBounds(state, hooks);
        let rectangle = 'a5'.repeat(16);
        let budget = 0;
        if (row[0] === 'B') {
            const players = new Uint8Array(9 * 0xb4);
            const options = new Uint8Array(87);
            const present = num(row[14]);
            const hidden = num(row[15]);
            // owner 0/flags50 비영도 같은 원문 입력이며 예산 관문으로 추정하지 않는다.
            for (let owner = 0; owner < 9; owner += 1) {
                put(players, owner * 0xb4 + 0x2c, owner === 0 || (present & (1 << (owner - 1))) ? 0x80000000 : 0);
                players[owner * 0xb4 + 0x50] = 0xff;
                if (owner) options[62 + owner] = hidden & (1 << (owner - 1)) ? 0x80 : 0;
            }
            budget = worldGeyserBudget(battle, players, options);
        } else {
            rectangle = rectangleHex(row[0] === 'W' ? body.whole() : body.partition(Number(row[3]) | 0, Number(row[4]) | 0));
        }
        const actual = [rectangle, String(margin >>> 0), String(budget), trace.length === 0 ? '-' : trace.join(';'), mission ? '1' : '0', String(size)];
        // 실패 시 최초 입력 번호와 해당 출력만 보여 fixture 를 바꾸지 않는다.
        for (let field = 0; field < actual.length; field += 1) assert.equal(actual[field], row[17 + field], `사례 ${count} 필드 ${field}: ${row.slice(0, 17).join(',')}`);
        count += 1;
    }
    assert.equal(count, 820);
});

test('WorldGeyserBounds: 미선택 spacing 은 요구하지 않고 분기 도달 시 앞선 청크 조회를 보존하며 표 밖/크기 오류를 거부한다', () => {
    let mission = true;
    let margin = 17;
    const regions = [];
    const state = { get missionPost() { return mission; }, mapLimit: 256, get spacingMargin() { return margin; }, set spacingMargin(value) { margin = value; } };
    const body = new RawWorldGeyserBounds(state, { chunkBounds: (region) => { regions.push(region); return { left: 2, top: 3, right: 13, bottom: 12 }; } });
    body.whole();
    assert.deepEqual(regions, [0, 1]);
    assert.equal(margin, 17);
    regions.length = 0;
    body.partition(1, 0);
    assert.deepEqual(regions, [0, 0, 1]);
    mission = false;
    regions.length = 0;
    assert.ok(throws(() => body.whole()));
    assert.deepEqual(regions, [0]);
    assert.equal(margin, 17);
    assert.ok(throws(() => body.partition(0, 0)));
    assert.ok(throws(() => worldGeyserSpacing(4)));
    assert.ok(throws(() => worldGeyserBudget(false, new Uint8Array(1619), new Uint8Array(87))));
    assert.ok(throws(() => worldGeyserDensity(4)));
    assert.equal(worldGeyserBudget(false, new Uint8Array(1620), new Uint8Array(87)), 0);
});

test('WorldGeyser: 원본 10.78 354입력의 로그/로컬/장식/요청 순서와 전체 인자·옵션·Player·장식 타입·demo 가 같다', { skip: !geyserRows && '원본 fixture 없음' }, () => {
    let count = 0;
    // 각 원문 입력을 독립 Player/옵션/타입에 설치한다.
    for (const row of geyserRows) {
        assert.equal(row.length, 16);
        count += 1;
        let battle = num(row[0]) !== 0;
        const options = new Uint8Array(87);
        const players = new Uint8Array(1620);
        let types = [100, 184, 101, 185];
        options[6] = num(row[3]);
        options[17] = num(row[4]);
        options[18] = num(row[5]);
        options[9] = num(row[6]);
        // 원문 예산의 실제 입력 위치만 작성한다.
        for (let owner = 1; owner <= 8; owner += 1) {
            put(players, owner * 0xb4 + 0x2c, num(row[1]) & (1 << (owner - 1)) ? 1 : 0);
            options[62 + owner] = num(row[2]) & (1 << (owner - 1)) ? 128 : 0;
        }
        const alter = num(row[9]);
        const consume = num(row[10]);
        let timeCalls = 0;
        let decorCalls = 0;
        let popCalls = 0;
        let densityCalls = 0;
        let demoCalls = 0;
        let demo = 0;
        const trace = [];
        const view = new DataView(new ArrayBuffer(8));
        view.setBigUint64(0, BigInt(row[8]), true);
        const wall = view.getFloat64(0, true);
        const hooks = {
            log: (text) => { assert.equal(text, 'Make Local Geysers\n'); trace.push('Log'); },
            local: () => {
                trace.push('Local');
                if (alter === 1) { battle = true; options[9] = 1; }
                if (alter === 7) types = [101, 184, 100, 185];
            },
            configInt: (key, value) => {
                assert.equal(key, 'predictableGeysers');
                assert.equal(value, 1);
                trace.push('Config');
                if (alter === 8) { options[18] = 1; options[17] = 3; options[6] = 0; }
                return Number(row[7]) | 0;
            },
            wallSeconds: () => { trace.push('Time'); const value = wall + timeCalls * 0.125; timeCalls += 1; return value; },
            budget: () => { trace.push('Budget'); return worldGeyserBudget(battle, players, options); },
            density: () => {
                trace.push('Density');
                densityCalls += 1;
                if (alter === 10) options[17] = [1, 1, 2, 0, 3][Math.min(densityCalls - 1, 4)];
                return worldGeyserDensity(options[17]);
            },
            spacing: () => { trace.push('Spacing'); return worldGeyserSpacing(options[6]); },
            demo: () => {
                trace.push('Demo');
                demoCalls += 1;
                if (alter === 5) demo = demoCalls === 1 ? 1 : 0;
                else if (alter === 9) demo = 1;
                return demo !== 0 ? 1 : 0;
            },
            whole: () => { trace.push('Whole'); return { left: 16.25, top: 24.5, right: 240.75, bottom: 232.25 }; },
            partition: (cell, grid) => {
                trace.push(`Part:${cell}:${grid}`);
                const left = Math.fround(cell * 10.25);
                return { left, top: 24.5, right: Math.fround(left + 32.5), bottom: 72.25 };
            },
            decorate: (random, rectangle, region, quantity, type, force) => {
                trace.push(`Decor:${random.state}:${rectangleHex(rectangle)}:${region},${quantity},${type},${force}`);
                decorCalls += 1;
                if (alter === 2 && decorCalls === 1) options[9] = 0;
                if (alter === 6 && decorCalls === 1) types[3] = 100;
                if (consume) random.setState((random.state + 0x10203) >>> 0);
            },
            populate: (quantity, random, rectangle, flags) => {
                trace.push(`Pop:${quantity >>> 0}:${random.state}:${rectangleHex(rectangle)}:${flags.excludePlayers},${flags.allowLand},${flags.allowVoid}`);
                popCalls += 1;
                if (alter === 3 && popCalls === 1) { options[18] = 1; options[17] = 3; }
                if (alter === 4 && popCalls === 1) { put(players, 2 * 0xb4 + 0x2c, 0); options[63] = 1; }
                if (consume) random.setState((random.state + 0x10203) >>> 0);
            }
        };
        const state = { get battle() { return battle; }, options, get decorationTypes() { return types; } };
        new RawWorldGeyser('patch1078', state, hooks).generate();
        const actual = [trace.join(';'), hex(options), hex(players), types.join(','), String(demo)];
        // 최초 불일치의 사례/필드를 드러내며 fixture 를 수정하지 않는다.
        for (let field = 0; field < actual.length; field += 1) assert.equal(actual[field], row[11 + field], `세계 간헐천 부모 사례 ${count} 필드 ${field}`);
    }
    assert.equal(count, 354);
});

test('WorldGeyser: 미선택 시각/배치 하위를 요구하지 않으며 선택된 하위 누락은 선행 효과를 남기고 시각 시드 변환이 cpppj 와 같다', () => {
    let battle = false;
    const options = new Uint8Array(87);
    const types = [100, 184, 101, 185];
    let trace = '';
    const state = { get battle() { return battle; }, options, decorationTypes: types };
    const hooks = { log: () => { trace += 'L'; } };
    assert.ok(throws(() => new RawWorldGeyser('patch1078', state, hooks).generate()));
    assert.equal(trace, 'L');
    hooks.local = () => { trace += 'O'; };
    hooks.configInt = () => 1;
    hooks.budget = () => 0;
    hooks.density = () => 0;
    hooks.spacing = () => 3;
    trace = '';
    new RawWorldGeyser('patch1078', state, hooks).generate();
    assert.equal(trace, 'LO');
    battle = true;
    options[9] = 1;
    trace = '';
    assert.ok(throws(() => new RawWorldGeyser('patch1078', state, hooks).generate()));
    assert.equal(trace, 'LO');
    assert.ok(throws(() => worldGeyserDensity(4)));
    assert.ok(throws(() => new RawWorldGeyser('cd1072', state, hooks)));
    assert.ok(throws(() => new RawWorldGeyser('patch1078', { battle, options: new Uint8Array(10), decorationTypes: types }, hooks)));
    // 시각 시드: 0/비유한은 low 0, 1초는 value=1013 이므로 (1013>>4) + 1013*33 이다.
    assert.equal(timeSeed(0), 0);
    assert.equal(timeSeed(Number.NaN), 0);
    assert.equal(timeSeed(1), ((1013 >> 4) + 1013 * 33) >>> 0);
    assert.equal(timeSeed(-1), (((-1013) >> 4) + Math.imul(-1013, 33)) >>> 0);
    assert.ok(new GameRandom(timeSeed(10)).next(6) < 6);
    assert.equal(bits(0), 0);
});
