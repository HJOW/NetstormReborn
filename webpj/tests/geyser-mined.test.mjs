// 간헐천 채집 통지 raw-geyser-mined.js(RawGeyserMined)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: geysermined-x86.tsv 의 `originals` 1156행 — 입력 7 값(수량 WORD·cost 비트·난수 seed·free·boss·표면 타입·소리 콜백 변이 0~12) → 게임 난수 상태, 효과 순서(S 소리[좌표 비트·번호·flags]/D 삭제/C 생성/P 배치/A assert),
//    간헐천 슬롯(6)과 생성물 슬롯(7) 50바이트(가상 표 DWORD 제외 전체). (`originalCD` 1156행은 10.72 라 사용하지 않는다.)
//  - 계약: 필수 효과/타입 표 누락·짧은 패킷·CD 판본을 raw 수량이나 난수를 바꾸기 전에 거부하고, 소리 뒤 현재 boss 로 고갈 삭제 여부를 정한다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E108: 간헐천 채집 통지 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, bitsToFloat, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { GameRandom } from '../www/core/random.js';
import { RawGeyserMined } from '../www/core/raw-geyser-mined.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E108)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'geysermined-x86.tsv'));
const hex = (bytes) => Buffer.from(bytes).toString('hex');
const bits = floatToBits;

// 예외가 나는지 본다. (2026-11-08 WB04-E108)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

const freshTypes = () => Array.from({ length: 188 }, () => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0 }));

test('GeyserMined: 원본 10.78 1156입력의 소리/수량/난수/고갈 삭제/emptyGeyser 생성의 효과 순서·난수 상태·간헐천과 생성물 슬롯이 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    let checked = 0;
    // 각 입력을 새 같은 풀/hash/난수 위에서 실행한다. 기대 수량이나 분기는 계산하지 않는다.
    for (const row of rows) {
        if (row[0] !== 'originals') continue;
        assert.equal(row.length, 6);
        const input = row[1].split(',').map((value) => Number(BigInt.asUintN(32, BigInt(value))));
        assert.equal(input.length, 7);
        const [quantity, cost, seed, free, boss, surface, mutation] = input;
        const pool = new SidPool(32768);
        const hash = new SquidHash();
        const types = freshTypes();
        // 원본 격리 입력 번호 5..8 을 실제 client free list 로 확보한다.
        for (let sid = 5; sid <= 8; sid += 1) assert.equal(pool.allocate(2), sid);
        pool.allocated(7).fill(0);
        const raw = pool.allocated(6);
        raw.fill(0);
        raw[10] = 122;
        raw[11] = free;
        put(raw, 12, quantity, 2);
        put(raw, 14, bits(40.75));
        put(raw, 18, bits(41.25));
        types[154].cost = bitsToFloat(cost);
        pool.allocated(8)[10] = surface;
        hash.setCell(0, 40, 41, surface ? 8 : 0);
        const random = new GameRandom(seed);
        const state = { boss: boss !== 0, checking: false, nuggetType: 154, emptyGeyserType: 84, isleType: 97, bridgeType: 82 };
        const events = [];
        const hooks = {
            sound: (x, y, name, flags) => {
                assert.ok(/^geyserMined\d\.wav$/.test(name) && flags === 0);
                events.push(`S:${bits(x)},${bits(y)},${name[11]},${flags}`);
                if (mutation === 1) { put(raw, 12, 400, 2); state.nuggetType = 155; types[155].cost = 199.5; state.boss = true; }
                else if (mutation === 2) { put(raw, 14, bits(42.25)); put(raw, 18, bits(43.75)); put(raw, 12, 200, 2); }
                else if (mutation >= 4 && mutation <= 9) {
                    const cellX = mutation === 4 ? -0.25 : mutation === 5 ? 256 : mutation === 6 ? bitsToFloat(0x7fc00000) : mutation === 7 ? 4294967296 : mutation === 8 ? -4294967296 : Infinity;
                    const cellY = mutation === 4 ? 255.75 : mutation <= 6 ? 41.25 : 0;
                    put(raw, 14, bits(cellX));
                    put(raw, 18, bits(cellY));
                    put(raw, 12, 200, 2);
                } else if (mutation >= 10) {
                    raw[10] = 82;
                    raw[40] = mutation >= 12 ? 8 : 0;
                    state.checking = mutation === 11 || mutation === 12;
                }
            },
            destroy: (sid, flags) => {
                assert.ok(sid === 6 && flags === 0x1000);
                events.push(`D:${flags}`);
                raw[11] |= 2;
                if (mutation === 3) {
                    const x = Math.trunc(bitsToFloat(get(raw, 14)));
                    const y = Math.trunc(bitsToFloat(get(raw, 18)));
                    hash.setCell(0, x, y, surface === 97 ? 0 : 8);
                    pool.allocated(8)[10] = 97;
                }
            },
            create: (type, flags) => {
                assert.ok(type === 84 && flags === 0);
                events.push(`C:${type},${flags}`);
                const born = pool.allocated(7);
                born.fill(0);
                born[10] = type;
                born[11] = 4;
                return 7;
            },
            pop: (sid, x, y, flags) => {
                assert.ok(sid === 7 && flags === 0);
                events.push(`P:${bits(x)},${bits(y)},${flags}`);
                const born = pool.allocated(sid);
                put(born, 14, bits(x));
                put(born, 18, bits(y));
                born[11] = 0;
            }
        };
        if (mutation >= 4 && mutation <= 9) {
            const x = mutation === 5 ? 40 : 0;
            const y = mutation === 4 ? 255 : mutation <= 6 ? 41 : 0;
            hash.setCell(0, x, y, surface ? 8 : 0);
        }
        const handler = new RawGeyserMined(pool, hash, types, random, state, hooks);
        const packet = Uint8Array.of(0xab, 0xcd, 0xef, 0xff, 6, 0);
        if (row[3].endsWith(';A')) {
            assert.ok(throws(() => handler.handlePacket(packet)));
            events.push('A');
        } else {
            handler.handlePacket(packet);
        }
        const label = `행 ${checked}: ${row[1]}`;
        assert.equal(String(random.state), row[2], label);
        assert.equal(events.length === 0 ? '-' : events.join(';'), row[3], label);
        // 원본에서 가상 표만 격리 주소로 주입했으므로 그것을 제외한 모든 raw byte 를 비교한다.
        assert.equal(hex(pool.slot(6)).slice(8), row[4].slice(8), label);
        assert.equal(hex(pool.slot(7)).slice(8), row[5].slice(8), label);
        checked += 1;
    }
    assert.equal(checked, 1156);
});

test('GeyserMined: 필수 효과/타입 표 누락·짧은 패킷·CD 판본은 거부하고 소리 뒤 현재 boss 로 고갈 삭제 여부를 정한다', () => {
    const pool = new SidPool(32768);
    const hash = new SquidHash();
    const types = freshTypes();
    types[122].footX = types[122].footY = types[84].footX = types[84].footY = 1;
    types[154].cost = 200;
    const full = { sound: () => {}, destroy: () => {}, create: () => 7, pop: () => {} };
    const state = { boss: false, checking: false, nuggetType: 154, emptyGeyserType: 84, isleType: 97, bridgeType: 82 };
    assert.ok(throws(() => new RawGeyserMined(pool, hash, types, new GameRandom(), state, {})));
    assert.ok(throws(() => new RawGeyserMined(pool, hash, types, new GameRandom(), state, { ...full, pop: null })));
    assert.ok(throws(() => new RawGeyserMined({ edition: 'cd1072' }, hash, types, new GameRandom(), state, full)));
    assert.ok(throws(() => new RawGeyserMined(pool, hash, types.slice(0, 171), new GameRandom(), state, full)));
    const handler = new RawGeyserMined(pool, hash, types, new GameRandom(), state, full);
    assert.equal(handler.pool, pool);
    assert.ok(throws(() => handler.handlePacket(new Uint8Array(0))));
    // 소리 뒤 현재 boss 를 다시 읽는다: 소리 콜백이 boss 를 켜면 같은 호출에서 고갈 삭제와 생성/배치가 일어난다.
    const geyser = pool.allocate(2);
    const raw = pool.allocated(geyser);
    raw[10] = 122;
    put(raw, 12, 200, 2);
    put(raw, 14, bits(40.75));
    put(raw, 18, bits(41.25));
    const effects = [];
    const live = new RawGeyserMined(pool, hash, types, new GameRandom(7), state, {
        sound: () => { effects.push('S'); state.boss = true; },
        destroy: (sid, flags) => { effects.push('D' + flags); },
        create: (type) => { effects.push('C' + type); return 9; },
        pop: (sid, x, y) => { effects.push('P' + sid + ':' + x + ',' + y); }
    });
    live.handle(geyser);
    assert.deepEqual(effects, ['S', 'D4096', 'C84', 'P9:40.75,41.25']);
    assert.equal(get(pool.slot(geyser), 12) & 0xffff, 0);
});
