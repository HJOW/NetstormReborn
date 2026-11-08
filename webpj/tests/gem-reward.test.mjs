// 오벨리스크 보상 raw-gem-reward.js(RawGemReward)와 Deck 추첨 raw-deck-draw.js(RawDeckDraw)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: gem-reward-x86.tsv 1250행(두 정밀도 열 동일) — 입력 14 DWORD(연산 0 선택/1 공개/2 직접 Draw·TYPE 변이·제외 비트·seed·벽시각 비트·부모 TYPE/상태/보상 TYPE·진단 변이·직접 Deck 모양·seed/열림·optional power)
//    → 결과(선택 TYPE/생성 SID/Draw TYPE 또는 범위 실패 `fault`), 외부 효과 순서(I 진단/W 벽시각/M 합계 불일치 로그/C 생성/P 배치/D·E 삭제), 게임 난수 상태, 직접 Deck 352바이트 hex, 부모와 생성 슬롯 raw, power, TYPE 개수.
//  - 계약: 선택하지 않은 경계는 요구하지 않고, 판본/TYPE 표 크기와 Deck 크기를 거부한다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E105: 오벨리스크 보상/Deck 추첨 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { GameRandom } from '../www/core/random.js';
import { SquidPostPopList } from '../www/core/squid-postpop-state.js';
import { PRODUCTION_DECK_BYTES, RawDeckDraw } from '../www/core/raw-deck-draw.js';
import { GemRewardState, RawGemReward } from '../www/core/raw-gem-reward.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E105)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'gem-reward-x86.tsv'));

// 예외가 나는지 본다. (2026-11-08 WB04-E105)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

const hex = (bytes) => Buffer.from(bytes).toString('hex');
const bits = (value) => floatToBits(value);

// 큰 장치·진단·생성 효과 입력만 공급하는 장면이다. (2026-11-08 WB04-E105)
function makeScene() {
    const scene = {
        pool: new SidPool(24000),
        types: Array.from({ length: 188 }, () => ({ flags2: 0, level: 0 })),
        random: new GameRandom(),
        excluded: new SquidPostPopList(),
        state: new GemRewardState(),
        deck: new Uint8Array(PRODUCTION_DECK_BYTES),
        currentDeck: null,
        row: [],
        trace: []
    };
    const raw = (sid) => scene.pool.physical(sid);
    const event = (name, values = []) => scene.trace.push(values.reduce((text, value) => `${text}:${value >>> 0}`, name));
    // 독립 원문과 동일한 초기 입력만 준비한다.
    scene.prepare = (input) => {
        const row = input;
        scene.row = row;
        assert.equal(row.length, 14);
        scene.trace = [];
        for (const type of scene.types) { type.flags2 = 0; type.level = 0; }
        scene.state.typeCount = row[1] === 5 ? 73 : 188;
        scene.state.buriedType = 112;
        scene.random.setState(row[3]);
        scene.excluded.entries = [];
        scene.excluded.count = 0;
        scene.deck.fill(0xa5);
        scene.currentDeck = scene.deck;
        // 현재 6타입의 수준과 genus 를 명시 입력으로 둔다.
        const levels = [0, 1, 2, 3, 98, 99];
        for (let index = 0; index < levels.length; index += 1) {
            scene.types[70 + index].flags2 = row[1] === 1 ? 0 : 0x100;
            scene.types[70 + index].level = levels[index];
            if (row[2] & (1 << index)) scene.excluded.entries.push(70 + index);
        }
        if (row[2] & 64) scene.excluded.entries.push(0);
        scene.excluded.count = scene.excluded.entries.length;
        if (row[1] === 2) scene.types[70].level = -1;
        if (row[1] === 3) { scene.types[0].flags2 = 0x100; scene.types[0].level = 3; }
        if (row[1] === 4) scene.types[70].flags2 = 0x2100;
        // 원문과 같은 두 전체 물리 슬롯을 입력한다.
        for (const sid of [50, 61]) {
            const slot = raw(sid);
            slot.fill(0xa5);
            put(slot, 0, 0x12200000);
            slot[10] = sid === 50 ? row[7] : 70;
            slot[11] = sid === 50 ? row[8] : 0;
            put(slot, 12, row[6], 2);
            put(slot, 14, bits(98));
            put(slot, 18, bits(122));
        }
        if (row[0] !== 2) return;
        let entries = [[70, 20, 1, 255, 254], [71, 30, 1, 255, 0], [74, 50, 1, 255, 255]];
        if (row[10] === 1) entries[0] = [70, 20, 0, 255, 0];
        if (row[10] === 2) entries[0] = [70, 20, 255, 255, 0];
        if (row[10] === 3) entries = [];
        if (row[10] === 4) entries = [[70, 0, 1, 255, 0], [71, 0, 1, 255, 0]];
        if (row[10] === 5) entries = [[70, 255, 127, 255, 255]];
        if (row[10] === 6) entries = [[70, 20, 128, 255, 0], [71, 30, 1, 255, 0]];
        let sum = 0;
        // 각 초기 항목의 5바이트만 쓴다.
        for (let index = 0; index < entries.length; index += 1) {
            scene.deck.set(entries[index], index * 5);
            sum += entries[index][1];
        }
        put(scene.deck, 0x14c, row[10] === 7 ? 0xffffffff : entries.length);
        put(scene.deck, 0x150, row[10] === 1 || row[10] === 2 ? 999 : sum);
        put(scene.deck, 0x154, 0xffffffff);
        put(scene.deck, 0x158, row[11]);
        put(scene.deck, 0x15c, row[12]);
    };
    // 선택한 진단이 돌려준 뒤 원본과 동일한 명시 변경만 수행한다.
    const assertion = (line) => {
        event('I', [line]);
        if (scene.row[9] === 1 && line === 0xf6) { scene.state.typeCount = 72; scene.types[71].level = 3; }
        if (scene.row[9] === 6 && line === 0x10a) { raw(50)[10] = 112; put(raw(50), 12, 70, 2); }
    };
    // 원문의 실제 시각/진단/로그·optional power 분류 경계이다.
    scene.drawHooks = () => ({
        wallSeconds: () => {
            event('W', [scene.row[4], scene.row[5]]);
            if (scene.row[9] === 3) scene.currentDeck[2] = 0;
            const view = new DataView(new ArrayBuffer(8));
            view.setUint32(0, scene.row[4], true);
            view.setUint32(4, scene.row[5], true);
            return view.getFloat64(0, true);
        },
        assertion,
        mismatch: (stored, sum) => {
            event('M', [stored, sum]);
            if (scene.row[9] === 2) { put(scene.currentDeck, 0x14c, 0); put(scene.currentDeck, 0x150, 999); }
        },
        isProduct: (type) => type >= 70 && type <= 73
    });
    // 실제 계산은 제품이 수행하고 큰 게임 호출의 원문 입력만 제공한다.
    scene.hooks = {
        fill: (deckRaw) => { deckRaw.fill(0xa5); scene.currentDeck = deckRaw; },
        deck: scene.drawHooks(),
        assertion,
        deckAssertion: () => { throw new Error('보상 원문에서 선택하지 않은 항목 진단'); },
        create: (type, flags) => {
            event('C', [type, flags]);
            raw(61)[10] = type;
            if (scene.row[9] === 4) { put(raw(50), 14, bits(101.5)); put(raw(50), 18, bits(-4.25)); }
            return 61;
        },
        pop: (sid, x, y, flags) => {
            event('P', [sid, bits(x), bits(y), flags]);
            if (scene.row[9] === 5) put(raw(50), 0, 0x12200100);
        },
        destroy: (sid, flags) => {
            event(get(raw(sid), 0) === 0x12200000 ? 'D' : 'E', [sid, flags]);
            raw(sid)[11] = 1;
        }
    };
    return scene;
}

test('GemReward: 원본 10.78 1250입력의 선택/공개/직접 Deck 추첨 반환·효과 순서·난수 상태·Deck·슬롯 raw·power·TYPE 개수가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = makeScene();
    let count = 0;
    let faults = 0;
    // 기대값은 fixture 에서만 읽으며 제품 분기와 독립이다.
    for (const fields of rows) {
        assert.equal(fields.length, 15);
        scene.prepare(fields[0].split(',').map((value) => Number(BigInt.asUintN(32, BigInt(value)))));
        const row = scene.row;
        const gem = new RawGemReward(scene.pool, scene.types, scene.random, scene.excluded, scene.state, scene.hooks);
        let result;
        const power = { value: 0x55555555 };
        try {
            if (row[0] === 0) result = String(gem.choose());
            else if (row[0] === 1) result = String(gem.reveal(50));
            else result = String(new RawDeckDraw(scene.deck, scene.random, scene.drawHooks()).draw(row[13] ? power : null));
        } catch (error) {
            assert.ok(error instanceof RangeError);
            assert.ok(row[0] === 1 && row[6] === 65535);
            result = 'fault';
            faults += 1;
        }
        const trace = scene.trace.length === 0 ? '-' : scene.trace.join(';');
        const raws = `${hex(scene.pool.slot(50))},${hex(scene.pool.slot(61))}`;
        const label = `행 ${count}: 입력 ${fields[0]}`;
        assert.equal(result, fields[1], label);
        assert.equal(trace, fields[2], label);
        assert.equal(String(scene.random.state), fields[3], label);
        assert.equal(raws, fields[5], label);
        assert.equal(String(power.value >>> 0), fields[6], label);
        assert.equal(String(scene.state.typeCount >>> 0), fields[7], label);
        if (row[0] === 2) assert.equal(hex(scene.deck), fields[4], label);
        // 두 정밀도 원문은 모든 관측 열에서 같아야 한다.
        for (let index = 1; index <= 7; index += 1) assert.equal(fields[index], fields[index + 7], label);
        count += 1;
    }
    assert.equal(count, 1250);
    assert.equal(faults, 32);
});

test('GemReward: 시각 seed 변환·열기/닫기 비트·추가 진단·판본/Deck 크기 계약이 cpppj 와 같다', () => {
    // 4bcab0: 곱 1013 의 하위 DWORD, 부호 확장 >>4, *33 감김, 비유한은 0 이다.
    assert.equal(RawDeckDraw.wallSeed(0), 0);
    assert.equal(RawDeckDraw.wallSeed(Number.NaN), 0);
    assert.equal(RawDeckDraw.wallSeed(Infinity), 0);
    assert.equal(RawDeckDraw.wallSeed(1e30), 0);
    assert.equal(RawDeckDraw.wallSeed(1), (1013 * 33 + (1013 >>> 4)) >>> 0);
    // 열기/닫기는 열림 bit0 만 바꾸고 seed 0 이면 게임 난수를 소비한다.
    const raw = new Uint8Array(PRODUCTION_DECK_BYTES).fill(0xa5);
    RawDeckDraw.initialize(raw);
    const random = new GameRandom(55);
    const draw = new RawDeckDraw(raw, random, {});
    put(raw, 0x15c, 0xf0);
    draw.open(0);
    assert.equal(get(raw, 0x15c), 0xf1);
    assert.notEqual(get(raw, 0x158), 0);
    draw.close();
    assert.equal(get(raw, 0x15c), 0xf0);
    // 추가: 중복 진단 뒤에도 계속 추가하고, 가득 차면 진단 뒤 거부하며, BYTE 에 안 맞으면 진단을 부른다.
    const kinds = [];
    const record = (kind) => kinds.push(kind);
    RawDeckDraw.initialize(raw);
    RawDeckDraw.append(raw, 5, 10, record);
    RawDeckDraw.append(raw, 5, 300, record);
    assert.deepEqual(kinds, [0, 3]);
    assert.equal(get(raw, 0x14c), 2);
    assert.equal(get(raw, 0x150), 310);
    put(raw, 0x14c, 66);
    assert.ok(throws(() => RawDeckDraw.append(raw, 9, 1, record)));
    assert.equal(kinds.at(-1), 1);
    assert.ok(throws(() => RawDeckDraw.append(raw, 9, 1, null)));
    assert.ok(throws(() => new RawDeckDraw(new Uint8Array(10), random, {})));
    // 판본/TYPE 표 크기와 하위 미연결은 거부한다.
    const pool = new SidPool(24000);
    const types = Array.from({ length: 188 }, () => ({ flags2: 0, level: 0 }));
    assert.ok(throws(() => new RawGemReward({ edition: 'cd1072' }, types, random, new SquidPostPopList(), new GemRewardState(), {})));
    assert.ok(throws(() => new RawGemReward(pool, types.slice(0, 171), random, new SquidPostPopList(), new GemRewardState(), {})));
    const gem = new RawGemReward(pool, types, random, new SquidPostPopList(), new GemRewardState(), {});
    assert.equal(gem.pool, pool);
    assert.ok(throws(() => gem.choose()));
});
