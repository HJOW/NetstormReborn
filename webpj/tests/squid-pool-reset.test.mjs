// 월드 Pool 정리 부모(ResetSquidWorldPool/FinishSquidPoolReset)를 원본 10.78 관측으로 대조한다(WB04-E37, cpppj SquidPoolResetTests 이식).
//  - squid-pool-reset-x86.tsv 12행 중 10.78(originals) 6행: 실제 부모·하위 Pool·삭제 기록 80DWORD·실패점 Y/X 의 전체 raw Pool Adler-32, 카운터, 목록 머리/꼬리, 삭제 기록.
//  - 계약: 하위 reset 은 기록을 보존하고 새 부모는 기록을 지운 뒤 재확보하며, 하위 실패 뒤에는 기록·실패점을 건드리지 않고, 기록 정리만 실행할 때는 다른 Pool 필드를 보존한다.
// 실제 Client 대여/외부 권한 거부(ResetClientSquidPool)와 Board/Dais 실패점 재조회는 해당 소비자 단위를 옮길 때 대조한다. (2026-11-08 WB04-E37: Pool 정리 부모 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { adler32, floatToBits, bitsToFloat, readRows } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { finishSquidPoolReset, resetSquidWorldPool } from '../www/core/raw-squid-pool-reset.js';

// 원본 관측 행이다. 파일이 없으면 검사를 건너뛴다. (2026-11-08 WB04-E37)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'squid-pool-reset-x86.tsv'));

// 호스트 패딩을 읽지 않고 실제 SID/type 두 DWORD 를 순서대로 직렬화한다(각 20쌍의 낮은 바이트부터). (2026-11-08 WB04-E37)
function records(pool, client) {
    const bytes = [];
    for (const record of pool.deletions(client)) {
        for (const value of [record.sid, record.type]) for (let byte = 0; byte < 4; byte += 1) bytes.push((value >>> (byte * 8)) & 0xff);
    }
    return Uint8Array.from(bytes);
}
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 원본과 같은 실제 할당·반납으로 양쪽 20개 삭제 기록을 채운다. (2026-11-08 WB04-E37)
function fillHistory(pool) {
    // client 와 other 의 실제 서로 다른 번호 영역을 사용한다.
    for (const flags of [2, 0]) {
        const allocated = [];
        for (let index = 0; index < 20; index += 1) {
            const sid = pool.allocate(flags);
            const raw = pool.allocated(sid);
            // 파생 생성자의 명시 입력이며 출력 계산이 아니다.
            for (let byte = 0; byte < raw.length; byte += 1) raw[byte] = (83 + index + byte * 37) & 0xff;
            raw[10] = 83 + index;
            raw[11] = 4;
            allocated.push(sid);
        }
        // 반납 순서로 실제 20쌍 이동과 FIFO 를 실행한다.
        for (const sid of allocated) pool.release(sid);
    }
}

// 실제 하위 reset 의 기존 주요 기록과 부모의 삭제 기록을 같은 열로 관측한다. (2026-11-08 WB04-E37)
function snapshot(pool) {
    const logs = Uint8Array.from([...records(pool, true), ...records(pool, false)]);
    return [0, pool.freeCount, pool.predictableCursor, pool.firstFree(true), pool.tail(true), pool.firstFree(false), pool.tail(false), adler32(pool.bytes()), adler32(logs)];
}

// 부모 12관측 중 10.78 행을 재생한다: 0/음수 인자와 두 x87 정밀도가 모두 같은 값을 기대한다.
test('SquidPoolReset: 원본10.78 부모 관측(Pool·기록 80DWORD·실패점 Y/X 전체)', { skip: !rows && '원본 fixture 없음' }, () => {
    let count = 0;
    for (const row of rows) {
        assert.equal(row.length, 21);
        // 원본 10.78 PE 행만 쓴다. CD 10.72(originalCD)는 범위 밖이다.
        if (row[0] !== 'originals') continue;
        const pool = new SidPool(32768, { server: true });
        fillHistory(pool);
        assert.equal(hex(records(pool, true)), row[3]);
        assert.equal(hex(records(pool, false)), row[4]);
        const invalid = { x: bitsToFloat(0x3f123456), y: bitsToFloat(0xc0123456) };
        const argument = Number(BigInt.asIntN(32, BigInt(row[1])));
        resetSquidWorldPool(pool, invalid, argument);
        const observed = snapshot(pool);
        // 전체 raw Pool·카운터·목록·삭제 기록은 Native 기대에서만 읽는다.
        observed.forEach((value, column) => assert.equal(value, Number(row[column + 5]), `행 ${count + 1} 열 ${column}`));
        assert.equal(hex(records(pool, true)), row[14]);
        assert.equal(hex(records(pool, false)), row[15]);
        assert.deepEqual([floatToBits(invalid.x), floatToBits(invalid.y)], [Number(row[16]), Number(row[17])]);
        assert.equal(row[18], 'wrapper,pool,history');
        count += 1;
    }
    assert.equal(count, 6);
});

// 기존 하위 reset 의 기록 보존과 새 부모의 기록 삭제·재확보를 구별한다.
test('SquidPoolReset: 하위 reset 은 기록을 보존하고 부모는 지운 뒤 재확보한다', () => {
    const pool = new SidPool(32768, { server: true });
    fillHistory(pool);
    const client = hex(records(pool, true));
    const other = hex(records(pool, false));
    pool.reset();
    assert.deepEqual([hex(records(pool, true)), hex(records(pool, false))], [client, other]);
    const invalid = { x: 11.25, y: -7.5 };
    resetSquidWorldPool(pool, invalid, 40000);
    assert.deepEqual([pool.capacity, pool.freeCount, pool.predictableCursor, invalid.x, invalid.y], [40000, 39995, 1, -10000, -10000]);
    assert.ok(pool.deletions(true).every((record) => record.sid === 0 && record.type === 0));
    assert.ok(pool.deletions(false).every((record) => record.sid === 0 && record.type === 0));
});

// 하위 실패 뒤 꼬리를 앞당기지 않고, 기록 정리만 실행할 때는 다른 Pool 필드를 보존한다.
test('SquidPoolReset: 하위 실패는 상태를 보존하고 기록 정리는 다른 필드를 보존한다', () => {
    const pool = new SidPool(32768, { server: true });
    fillHistory(pool);
    const invalid = { x: 11.25, y: -7.5 };
    const before = snapshot(pool);
    // 하한보다 작은 용량 요청은 하위 reset 이 거부한다.
    assert.throws(() => resetSquidWorldPool(pool, invalid, 5));
    assert.deepEqual(snapshot(pool), before);
    assert.deepEqual([invalid.x, invalid.y], [11.25, -7.5]);
    finishSquidPoolReset(pool, invalid);
    const after = snapshot(pool);
    // 기록 80DWORD 외의 현재 Pool 필드(앞 8열)는 그대로이고 실패점은 −10000 으로 쓴다.
    assert.deepEqual(after.slice(0, 8), before.slice(0, 8));
    assert.deepEqual([invalid.x, invalid.y], [-10000, -10000]);
});
