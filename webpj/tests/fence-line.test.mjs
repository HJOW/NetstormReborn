// 썬 바리케이드 연결선 삭제 raw-fence-line.js(RawFenceLine)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: fence-line-x86.tsv(10.78 전용; 첫 행은 머리글) 99행 — 단정도 네 끝점 비트(`x,y,endX,endY`) → 원본 mode 0 이 finder 에 넘기는 정수 검색점 순서(100회 상한·끝점 한 번 포함). 두 x87 정밀도에서 같은 값이다.
//  - 계약: FenceLine_CurrentHashFrameAndRequiredDeletion(현재 해시/방향 코드가 맞는 mark 만 삭제하며 누락한 삭제 소비자는 선택 위치에서 거부).
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E74: 연결선 삭제 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, bitsToFloat, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { RawFenceLine } from '../www/core/raw-fence-line.js';

// 원본 관측 행이다(머리글 포함). 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E74)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'fence-line-x86.tsv'));

// 예외가 나는지 본다. (2026-11-08 WB04-E74)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

test('FenceLine: 원본 10.78 99개 입력의 보정 검색점·끝점 포함·100회 상한이 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    assert.ok(rows.length === 100 && rows[0][0] === 'input');
    // 각 독립 네 float 입력을 그대로 재생한다.
    for (let index = 1; index < rows.length; index += 1) {
        const row = rows[index];
        assert.equal(row.length, 2);
        const input = row[0].split(',');
        assert.equal(input.length, 4);
        const endpoints = input.map((bits) => bitsToFloat(Number(bits)));
        const text = RawFenceLine.searchPoints(endpoints).map((point) => `${point[0]},${point[1]}`).join(';');
        assert.equal(text, row[1], `입력 ${row[0]}`);
    }
});

test('FenceLine: 현재 해시/방향 코드가 맞는 mark 만 삭제하며 누락한 삭제 소비자는 선택 위치에서 거부한다', () => {
    const pool = new SidPool(24000, { server: false });
    const hash = new SquidHash();
    const types = Array.from({ length: 188 }, () => ({ footX: 0, footY: 0, flags2: 0 }));
    const frames = Array.from({ length: 188 }, () => new FrameCodeTable([]));
    const markType = 170;
    types[170].footX = types[170].footY = 1;
    types[90].footX = types[90].footY = 1;
    frames[170] = new FrameCodeTable([{ side: 0x4a, variant: 0, flags: 0 }, { side: 0x4b, variant: 0, flags: 0 }]);
    for (;;) {
        const sid = pool.allocate(2);
        if (sid === 53) break;
        if (!sid || sid > 53) throw new Error('펜스 선 검사 슬롯 확보 실패');
    }
    // 세 mark 와 타입이 다른 객체를 실제 해시에 등록한다.
    for (let number = 50; number <= 53; number += 1) {
        const raw = pool.allocated(number);
        raw.fill(0);
        raw[10] = number === 53 ? 90 : 170;
        put(raw, 14, floatToBits(49.0));
        put(raw, 18, floatToBits(40.0 + number - 50));
        put(raw, 36, number === 51 ? 1 : 0);
        const head = hash.bucket(1, 49, 40.0 + number - 50);
        put(raw, 4, head, 2);
        hash.setBucket(1, 49, 40.0 + number - 50, number);
    }
    const removed = [];
    const body = new RawFenceLine(pool, hash, types, frames, () => markType, (sid, flags) => {
        assert.equal(flags, 0x40);
        removed.push(sid);
        pool.allocated(sid)[40] |= 8;
    });
    body.clear([50, 40, 50, 48]);
    assert.ok(removed.length === 2 && removed[0] === 50 && removed[1] === 52);
    assert.ok(pool.slot(51)[40] === 0 && pool.slot(53)[40] === 0);
    pool.allocated(50)[40] = 0;
    const missing = new RawFenceLine(pool, hash, types, frames, () => markType, null);
    assert.ok(throws(() => missing.clear([50, 40, 50, 48])));
    assert.ok(throws(() => RawFenceLine.searchPoints([Infinity, 0, 0, 0])));
    assert.ok(throws(() => new RawFenceLine(pool, hash, types.slice(1), frames, () => markType, null)));
});
