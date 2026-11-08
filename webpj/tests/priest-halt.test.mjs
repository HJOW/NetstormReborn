// 사제 Halt raw-priest-halt.js(RawPriestHalt·PriestHaltState)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: priesthalt-x86.tsv 10.78(originals) 432행(판본별 입력 JSON: 전투/권한/변경 단계/낙하/genus/지면/현재 HP/종류/이웃 타입/응답/좌표) → 반환 코드, 호출 순서(F/U/P/C/N), 슬롯 50 전체 raw.
//    기존 실제 Carrier 지면 조회·사제 HP·일반 finder 를 같은 풀에서 쓰고 공간 복귀·포획·너겟 전달만 명시 경계로 기록한다. CD 10.72·10.37 행은 웹 범위(10.78) 밖이다.
//  - 계약: 연결 오류 거부(다른 풀·표 크기·지도 크기·누락 훅).
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E66: 사제 Halt 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, floatToBits, bitsToFloat } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { GameRandom } from '../www/core/random.js';
import { ScrambledSpStore } from '../www/core/sp-store.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidPostPopState } from '../www/core/squid-postpop-state.js';
import { SquidReward, SquidRewardState } from '../www/core/squid-reward.js';
import { RawPriestState } from '../www/core/raw-priest-state.js';
import { RawCarrierCheck } from '../www/core/raw-carrier-check.js';
import { RawPriestHalt, PriestHaltState } from '../www/core/raw-priest-halt.js';

// 원본 관측 10.78 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E66)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'priesthalt-x86.tsv'))?.filter((row) => row[0] === 'originals') ?? null;

// 원본 실행기의 합성 가상 표(호스트 코드 포인터가 아님), 사제/이웃 SID, 판본당 독립 행 수이다. (2026-11-08 WB04-E66)
const VTABLE = 0x11018000;
const PRIEST = 50;
const NEIGHBOR = 60;
const ROWS = 432;

// 예외가 나는지 본다. (2026-11-08 WB04-E66)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E66)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 188칸 TYPE 입력이다. (2026-11-08 WB04-E66)
const haltTypes = () => Array.from({ length: 188 }, () => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0 }));

test('PriestHalt: 원본 10.78 432행의 반환 코드·호출 순서·슬롯 50 raw 가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    let checked = 0;
    // 각 원본 입력은 독립 상태와 실제 raw client 슬롯을 사용한다.
    for (const row of rows) {
        assert.equal(row.length, 5);
        const input = JSON.parse(row[1]);
        const pool = new SidPool(24000, { server: true });
        const hash = new SquidHash();
        const types = haltTypes();
        const spots = new Uint8Array(65536);
        const events = [];
        // 기록된 SID 50/60 까지 실제 client free list 에서 동일 번호를 확보한다.
        for (let sid = 5; sid <= 60; sid += 1) {
            assert.equal(pool.allocate(2), sid);
            pool.allocated(sid).fill(0);
        }
        // raw 필드/발자국/버킷만 준비하고 검색 및 Halt 분기 결과는 계산하지 않는다.
        const insert = (sid, type, xBits, yBits) => {
            const raw = pool.allocated(sid);
            put(raw, 0, VTABLE);
            raw[10] = type;
            put(raw, 14, xBits);
            put(raw, 18, yBits);
            types[type].footX = types[type].footY = 1;
            put(raw, 4, hash.bucket(1, bitsToFloat(xBits), bitsToFloat(yBits)), 2);
            hash.setBucket(1, bitsToFloat(xBits), bitsToFloat(yBits), sid);
        };
        insert(PRIEST, 158, input.x, input.y);
        if (input.neighbor) insert(NEIGHBOR, input.neighbor, floatToBits(20.2), floatToBits(21.3));
        put(pool.allocated(PRIEST), 26, input.hp);
        types[158].maxHitPoints = 200;
        types[158].flags2 = input.genus;
        const x = bitsToFloat(input.x);
        const y = bitsToFloat(input.y);
        spots[Math.trunc(y + Math.fround(0.9999)) * 256 + Math.trunc(x + Math.fround(0.9999))] = input.ground;
        const book = new SquidPostPopState();
        const reward = new SquidReward(pool, types, book, new SquidRewardState(), new ScrambledSpStore(new GameRandom(), () => 1));
        const priest = new RawPriestState(pool, reward, types, spots);
        const check = new RawCarrierCheck(pool, types, spots, (sid) => {
            assert.equal(sid, PRIEST);
            events.push(`F:50:${input.fall}`);
            return input.fall !== 0;
        });
        const state = new PriestHaltState();
        state.battle = input.battle !== 0;
        state.boss = input.boss !== 0;
        const hooks = {
            unpop: (sid, flags) => {
                assert.ok(sid === PRIEST && flags === 0);
                events.push('U:50:0');
                if (input.change) {
                    const raw = pool.allocated(sid);
                    put(raw, 14, floatToBits(30.75));
                    put(raw, 18, floatToBits(31.9));
                }
            },
            pop: (sid, px, py, flags) => {
                assert.ok(sid === PRIEST && flags === 0);
                events.push(`P:50:${floatToBits(px)}:${floatToBits(py)}:0`);
                const raw = pool.allocated(sid);
                put(raw, 14, floatToBits(px));
                put(raw, 18, floatToBits(py));
            },
            carrierPickup: (sid) => { assert.equal(sid, NEIGHBOR); events.push(`C:60:${input.reply}`); return input.reply; },
            nuggetTransfer: (sid) => { assert.equal(sid, NEIGHBOR); events.push(`N:60:${input.reply}`); return input.reply; }
        };
        const halt = new RawPriestHalt(pool, hash, types, spots, check, priest, reward, state, hooks);
        const result = input.kind === 'Halt' ? halt.halt(PRIEST) : halt.commonHalt(PRIEST);
        assert.equal(result, Number(row[2]), `행 ${checked}: ${row[1]}`);
        assert.equal(events.length === 0 ? '-' : events.join(';'), row[3], `행 ${checked}: ${row[1]}`);
        assert.equal(hex(pool.slot(PRIEST)), row[4], `행 ${checked}: ${row[1]}`);
        checked += 1;
    }
    assert.equal(checked, ROWS);
});

test('PriestHalt: 다른 풀·표/지도 크기·누락 훅은 첫 몸체 실행 전에 거부한다', () => {
    const pool = new SidPool(24000, { server: true });
    const other = new SidPool(24000, { server: true });
    const types = haltTypes();
    const spots = new Uint8Array(65536);
    const hash = new SquidHash();
    const reward = new SquidReward(pool, types, new SquidPostPopState(), new SquidRewardState(), new ScrambledSpStore(new GameRandom(), () => 1));
    const priest = new RawPriestState(pool, reward, types, spots);
    const check = new RawCarrierCheck(pool, types, spots, () => false);
    const hooks = { unpop: () => {}, pop: () => {}, carrierPickup: () => 0, nuggetTransfer: () => 0 };
    const state = new PriestHaltState();
    assert.doesNotThrow(() => new RawPriestHalt(pool, hash, types, spots, check, priest, reward, state, hooks));
    const otherReward = new SquidReward(other, types, new SquidPostPopState(), new SquidRewardState(), new ScrambledSpStore(new GameRandom(), () => 1));
    assert.ok(throws(() => new RawPriestHalt(pool, hash, types, spots, check, priest, otherReward, state, hooks)));
    assert.ok(throws(() => new RawPriestHalt(pool, hash, types.slice(1), spots, check, priest, reward, state, hooks)));
    assert.ok(throws(() => new RawPriestHalt(pool, hash, types, new Uint8Array(65535), check, priest, reward, state, hooks)));
    assert.ok(throws(() => new RawPriestHalt(pool, hash, types, spots, check, priest, reward, state, { ...hooks, nuggetTransfer: undefined })));
    assert.ok(throws(() => new RawPriestHalt(other, hash, types, spots, check, priest, reward, state, hooks)));
});
