// 간헐천 수명 raw-geyser-lifetime.js(RawGeyserLifetime)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: geyser-lifetime-x86.tsv(10.78 전용; 두 x87 정밀도에서 일치) 2016행 — 입력 7열(모드 0 preDestroy / 1 postDestroy·extra·편집·기본 프레임·목록 패턴·변이·flags) → 호출 순서(F 기본 프레임 / U 표시 / I AI 제거 / P·T Damageable / D 섬 정리),
//    효과 뒤 `목록 개수,편집 모드`, 부모 슬롯 50 raw, 지면 목록 물리 꼬리 8칸 전체.
//  - 계약: GeyserLifetime_EditionAndSelectedBoundary(현재 가상 표만 선택하고 실제 도달한 효과의 누락과 CD 혼합을 거부).
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E75: 간헐천 수명 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidPostPopList } from '../www/core/squid-postpop-state.js';
import { RawGeyserLifetime } from '../www/core/raw-geyser-lifetime.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E75)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'geyser-lifetime-x86.tsv'));

// 독립 입력 수와 관찰 SID, 원래 목록 패턴 세 가지(비활성 물리 꼬리는 99)이다. (2026-11-08 WB04-E75)
const ROWS = 2016;
const PARENT = 50;
const PATTERNS = Object.freeze([[], [50, 77, 50, 0, 50, 88], [77, 88, 0, 99]]);

// 예외가 나는지 본다. (2026-11-08 WB04-E75)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E75)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

test('GeyserLifetime: 원본 10.78 2016입력의 현재 extra/편집 모드/중복 제거/효과 뒤 변경/직접 프레임 DWORD 와 목록 꼬리가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    const pool = new SidPool(24000);
    let count = 0;
    // 기존 목격 자료를 한 행씩 공급하며 예상 목록이나 분기를 계산하지 않는다.
    for (const row of rows) {
        assert.equal(row.length, 9);
        const c = row[0].split(',').map(Number);
        assert.equal(c.length, 7);
        const parent = pool.physical(PARENT);
        parent.fill(0xa5);
        put(parent, 0, 0x50da88);
        put(parent, 10, 122, 1);
        put(parent, 11, 2, 1);
        put(parent, 40, c[1], 1);
        put(parent, 34, 3, 1);
        put(parent, 36, 777);
        put(parent, 14, 0x41a20000);
        put(parent, 18, 0x41ac0000);
        const list = new SquidPostPopList();
        list.entries = new Array(8).fill(99);
        PATTERNS[c[4]].forEach((value, index) => { list.entries[index] = value; });
        list.count = PATTERNS[c[4]].length;
        let editor = c[2];
        const events = [];
        // 실제 발생 순서의 DWORD 인자를 같은 원문 문자열로 붙인다.
        const event = (kind, args) => events.push([kind, ...args].join(':'));
        const hooks = {
            defaultFrame: (sid) => { event('F', [sid]); if (c[5] === 1) parent[40] = 8; return c[3]; },
            update: (sid) => { event('U', [sid, get(parent, 36)]); if (c[5] === 2 || c[5] === 3) parent[40] = c[5] === 2 ? 8 : 0; },
            registerAi: (sid, state) => {
                event('I', [sid, state]);
                if (c[5] === 4) { list.entries[0] = 50; list.entries[1] = 77; list.entries[2] = 50; list.count = 3; parent[40] = 8; }
                if (c[5] === 5) { editor = 1; put(parent, 36, 12345); }
            },
            damageablePre: (sid, flags) => { event('P', [sid, flags]); },
            damageablePost: (sid, flags) => { event('T', [sid, flags]); },
            dropIsland: (x, y, flags, owner) => {
                event('D', [floatToBits(x), floatToBits(y), flags, owner]);
                if (c[5] === 6) { put(parent, 14, 0x3fc00000); put(parent, 18, 0x40200000); parent[34] = 9; parent[40] = 9; }
            }
        };
        const life = new RawGeyserLifetime(pool, list, () => editor, hooks);
        assert.ok(life.handles(PARENT) && life.pool === pool);
        if (c[0]) life.postDestroy(PARENT, c[6]);
        else life.preDestroy(PARENT, c[6]);
        // 미사용 물리 꼬리까지 8개를 모두 대조한다.
        assert.equal(events.join(';'), row[1], `행 ${count}: 입력 ${row[0]}`);
        assert.equal(`${list.count},${editor}`, row[2], `행 ${count}: 입력 ${row[0]}`);
        assert.equal(hex(parent), row[3], `행 ${count}: 입력 ${row[0]}`);
        assert.equal(list.entries.join(','), row[4], `행 ${count}: 입력 ${row[0]}`);
        assert.ok(row.slice(1, 5).every((value, index) => value === row[5 + index]));
        count += 1;
    }
    assert.equal(count, ROWS);
});

test('GeyserLifetime: 현재 가상 표만 선택하고 실제 도달한 효과의 누락과 CD 혼합을 거부한다', () => {
    const pool = new SidPool(24000);
    const list = new SquidPostPopList();
    const life = new RawGeyserLifetime(pool, list, () => 0, {});
    assert.ok(!life.handles(0));
    assert.ok(throws(() => life.preDestroy(PARENT, 0)));
    assert.ok(throws(() => new RawGeyserLifetime({ edition: 'cd1072' }, list, () => 0, {})));
    assert.ok(throws(() => new RawGeyserLifetime(pool, list, 0, {})));
});
