// 아이스 타워 raw-ice-tower.js(RawIceTower·postPopGrowingIce)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: ice-tower-x86.tsv 1426행 — 종류 P 성장형 최초 예약/R 성장 이벤트 처리/D 완성형 파괴/G 성장형 삭제, 입력(flags·extra·프레임 수·할당 성공·boss·프레임·TYPE·로딩·battle·표면·섬 존재·이벤트·변이 0~12)
//    → 외부 효과 순서(N 예약 40/R 이벤트 부착/P·D·B 공통 후처리/Q 표면 칸/G 일반 finder/J 받침 정리/C 생성/U 공간 해제/K 삭제/O 소유자/V 재배치/T 다리 통지/F 프레임 갱신), 부모(50)·표면(60)·생성물(61) 세 슬롯 전체 raw, 반환 float 비트.
//  - 계약: 성장형 예약의 판본/프레임 표 크기 거부, 누락 훅의 지연 거부.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E109: 아이스 타워 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, bitsToFloat, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { RawIceTower, postPopGrowingIce } from '../www/core/raw-ice-tower.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E109)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'ice-tower-x86.tsv'));
const hex = (bytes) => Buffer.from(bytes).toString('hex');
const bits = floatToBits;

// 예외가 나는지 본다. (2026-11-08 WB04-E109)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 동결 입력의 부호/float 비트를 동일 DWORD 로 읽는다. (2026-11-08 WB04-E109)
const numbers = (text) => text.split(',').map((value) => Number(BigInt.asUintN(32, BigInt(value))));

test('IceTower: 원본 10.78 1426입력의 예약/재생성/삭제/현재 프레임 주기 효과 순서·세 슬롯 전체 raw·반환 float 이 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    let count = 0;
    // 기존 fixture 를 수정하지 않고 새 관측을 읽는다.
    for (const row of rows) {
        assert.equal(row.length, 5);
        const a = numbers(row[1]);
        const kind = row[0];
        const pool = new SidPool(32768);
        const parent = 50;
        const surfaceSid = 60;
        const born = 61;
        const writable = (sid) => pool.physical(sid);
        // 경계가 공급한 현재 좌표만 바꾼다.
        const position = (x, y) => { put(writable(parent), 14, bits(x)); put(writable(parent), 18, bits(y)); };
        const mutation = a[a.length - 1];
        let flags = 0;
        let extra = 0;
        let total = 7;
        let loading = 0;
        let surface = 0;
        let found = 0;
        let allocation = 1;
        let type = 150;
        let frame = 4;
        let battle = true;
        let boss = true;
        const types = { ice: 147, growing: 150, noIsland: 157 };
        if (kind === 'P') { flags = a[0]; extra = a[1]; total = a[2]; allocation = a[3]; }
        else if (kind === 'R') { extra = a[2]; total = a[3]; boss = a[4] !== 0; frame = a[5]; }
        else if (kind === 'D') { flags = a[0]; extra = a[1]; type = a[2]; loading = a[3]; battle = a[4] !== 0; surface = a[5]; }
        else { flags = a[0]; extra = a[1]; found = a[2]; }
        // 세 슬롯의 원래 50바이트를 명시 설치한다.
        for (const sid of [parent, surfaceSid, born]) {
            const raw = writable(sid);
            raw.fill(0);
            put(raw, 0, 0x12200000 + (sid === born ? 0x100 : 0));
            raw[10] = sid === parent ? type : sid === surfaceSid ? surface : 0;
            raw[11] = sid === parent && (kind === 'D' || kind === 'G') ? 6 : sid === born ? 1 : 0;
            raw[34] = sid !== born ? 2 : 0;
            if (sid === parent) {
                put(raw, 12, 0xa55a, 2);
                put(raw, 14, bits(20.25));
                put(raw, 18, bits(21.5));
                raw[35] = 0x44;
                raw[40] = extra;
                put(raw, 36, frame);
            }
        }
        // 현재 TYPE 별 numFrames 입력(개수만 공급)이다.
        const frames = Array.from({ length: 188 }, () => ({ codes: [] }));
        for (const [typeNumber, length] of [[150, total], [151, 9]]) frames[typeNumber] = { codes: Array.from({ length }, (_, index) => ({ side: 0x41, variant: 0, flags: 0, number: index })) };
        const effects = [];
        const event = (name, values = []) => effects.push(values.reduce((text, value) => `${text}:${value >>> 0}`, name));
        const hooks = {
            reserveRegular: () => { event('N', [40]); if (mutation === 1) writable(parent)[10] = 151; return allocation !== 0; },
            regular: (sid, eventId, period) => { event('R', [eventId, sid, bits(period)]); if (mutation === 10) position(61, 62); },
            basePop: (sid, value) => { assert.equal(sid, parent); event('P', [value]); },
            damageablePost: (sid, value) => { assert.equal(sid, parent); event('D', [value]); },
            basePost: (sid, value) => { assert.equal(sid, parent); event('B', [value]); },
            grid: (x, y) => {
                event('Q', [bits(x), bits(y)]);
                if (mutation === 6) { battle = false; position(30.5, 31.25); writable(parent)[34] = 4; }
                if (mutation === 8) types.noIsland = 95;
                return surface ? surfaceSid : 0;
            },
            genusAt: (x, y, genus) => {
                event('G', [bits(x), bits(y), genus]);
                if (mutation === 11) { position(50, 51); writable(parent)[34] = 4; }
                return found ? surfaceSid : 0;
            },
            dropIsland: (x, y, value, owner) => event('J', [bits(x), bits(y), value, owner]),
            create: (requested, value) => {
                event('C', [requested, value]);
                const raw = writable(born);
                raw.fill(0);
                put(raw, 0, 0x12200100);
                raw[10] = mutation === 7 ? 82 : requested;
                raw[11] = 4;
                raw[35] = 0x99;
                put(raw, 12, 0x7788, 2);
                put(raw, 36, 4);
                if (mutation === 2) { writable(parent)[34] = 3; position(11.5, 12.25); put(writable(parent), 12, 0x3344, 2); writable(parent)[35] = 77; }
                if (mutation === 12) writable(parent)[40] = 32;
                return born;
            },
            unpop: (sid, value) => { event('U', [sid, value]); writable(sid)[11] |= 4; if (mutation === 5) battle = false; },
            destroy: (sid, value) => { event('K', [sid, value]); writable(sid)[11] |= 3; if (mutation === 4) position(90, 91); },
            owner: (sid, owner) => { event('O', [sid, owner]); writable(sid)[34] = owner; if (mutation === 3) { position(13.5, 14.25); writable(parent)[35] = 78; } },
            pop: (sid, x, y, value) => {
                event('V', [sid, bits(x), bits(y), value]);
                put(writable(sid), 14, bits(x));
                put(writable(sid), 18, bits(y));
                writable(sid)[11] &= ~4 & 0xff;
            },
            word: (sid, value) => {
                put(writable(sid), 12, value, 2);
                if (writable(sid)[10] === 82) { event('T', [sid]); writable(parent)[35] = 88; writable(parent)[34] = 4; position(19.5, 20.5); }
            },
            setFrame: (sid, value, mode) => {
                event('F', [value, mode]);
                put(writable(sid), 36, value >>> 0);
                if (mutation === 9) writable(sid)[10] = 151;
            }
        };
        const ice = new RawIceTower(pool, frames, types, () => battle, () => boss, () => loading, hooks);
        let result = 0;
        if (kind === 'P') ice.postPop(parent, flags);
        else if (kind === 'D') ice.postDestroy(parent, flags);
        else if (kind === 'G') ice.growingPostDestroy(parent, flags);
        else result = bits(ice.handle(parent, a[0], a[1], bitsToFloat(a[6])));
        const raws = [parent, surfaceSid, born].map((sid) => hex(pool.slot(sid))).join('');
        const label = `아이스 원문행 ${count}/${kind}: ${row[1]}`;
        assert.equal(effects.length === 0 ? '-' : effects.join(';'), row[2], label);
        assert.equal(raws, row[3], label);
        assert.equal(result, Number(row[4]), label);
        count += 1;
    }
    assert.equal(count, 1426);
});

test('IceTower: 성장형 예약의 판본/프레임 표 크기 거부와 누락 훅의 지연 거부가 cpppj 와 같다', () => {
    const pool = new SidPool(24000);
    const frames = Array.from({ length: 188 }, () => ({ codes: [{}, {}, {}, {}] }));
    pool.physical(50)[10] = 150;
    const calls = [];
    postPopGrowingIce(pool, frames, 50, 1, () => true, (sid, event, period) => calls.push(['R', event, period]), () => calls.push(['B']));
    assert.deepEqual(calls, [['R', 0x3a6, 10], ['B']]);
    calls.length = 0;
    postPopGrowingIce(pool, frames, 50, 0, null, null, () => calls.push(['B']));
    assert.deepEqual(calls, [['B']]);
    assert.ok(throws(() => postPopGrowingIce({ edition: 'cd1072' }, frames, 50, 0, null, null, () => {})));
    assert.ok(throws(() => postPopGrowingIce(pool, frames.slice(0, 171), 50, 0, null, null, () => {})));
    assert.ok(throws(() => postPopGrowingIce(pool, frames, 50, 1, null, null, () => {})));
    assert.ok(throws(() => postPopGrowingIce(pool, frames, 50, 0, null, null, null)));
    const ice = new RawIceTower(pool, frames, { ice: 147, growing: 150, noIsland: 157 }, () => true, () => true, () => 0, {});
    assert.equal(ice.handle(50, 1, 0, 3.5), 3.5);
    assert.ok(throws(() => ice.handle(50, 0x3a6, 0, 0)));
    assert.ok(throws(() => new RawIceTower({ edition: 'cd1072' }, frames, {}, () => true, () => true, () => 0, {})));
});
