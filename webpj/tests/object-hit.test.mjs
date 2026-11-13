// 실제 선택용200×200 원색 판정을 원본608행과 현재 raw/헤더 계약으로 대조한다. 합성TYPE80만 쓰며 공중3종을 생성하지 않는다. (2026-11-14 WB04-E408)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, adler32, floatToBits, get } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { RawObjectHit } from '../www/core/raw-object-hit.js';

// 독립 관측과 선택 대상의 합성SID/TYPE다.
const rows = readRows(path.join(REPO_ROOT, 'cpppj/tests/fixtures/object-hit-x86.tsv')); const SID = 50; const TYPE = 80;

// 같은 실제 풀에 원본 입력을 공급할 빈 타입/SHP 목록이다.
function scene() { const pool = new SidPool(24000, { server: true }); const raw = pool.physical(SID); raw.fill(0); raw[10] = TYPE; const types = Array.from({ length: 188 }, () => ({ flags1: 0, hotspotX: 0, hotspotY: 0, maxHitPoints: 0 })); const shapes = Array.from({ length: 188 }, () => ({ loaded: false, frameCount: 0, frames: [] })); return { pool, raw, types, shapes }; }

test('ObjectHit_ReplaysNative608PixelBranchesAndCallOrder: 원본608행의 반환·getter/plain/추가 순서·원색 버퍼Adler와 raw 불변이 같다', { skip: rows === null }, () => {
    assert.equal(rows.length, 608); const s = scene(); let accepted = 0;
    // 원본17개 입력만 공급하며 반환/사건/버퍼 기대값은 관측 열에서 읽는다.
    for (const row of rows) {
        assert.equal(row.length, 4); const v = row[0].split(',').map(Number); assert.equal(v.length, 17); s.raw.fill(0); s.raw[10] = TYPE; s.raw[40] = v[7] & 255; put(s.raw, 14, v[0]); put(s.raw, 18, v[1]); put(s.raw, 36, v[8]);
        Object.assign(s.types[TYPE], { flags1: v[6] >>> 0, hotspotX: 3, hotspotY: -2 }); Object.assign(s.shapes[TYPE], { loaded: true, frameCount: 4, frames: Array.from({ length: 4 }, () => ({ width: v[12] << 16 >> 16, height: v[13] << 16 >> 16, hotspotX: v[14] << 16 >> 16, hotspotY: v[15] << 16 >> 16 })) });
        let capture = new Uint8Array(40000).fill(173); const trace = []; let getter = 0; const before = s.raw.slice();
        // 원문 격리 장치와 같은 명시 픽셀만 공급하며 RLE 결과를 계산하지 않는다.
        const pixel = (target, x, y, color) => { if (x >= 0 && x < 200 && y >= 0 && y < 200) target[y * 200 + x] = color; };
        const hooks = { frame: (sid) => { assert.equal(sid, SID); const value = v[9 + Math.min(getter++, 2)]; trace.push(`frame:${value}`); return value; },
            plain: (type, frame, x, y, target) => { assert.equal(type, TYPE); assert.equal(target.length, 40000); trace.push(`plain:${frame},${x},${y}`); const colors = [255, 9, 0, 255]; pixel(target, x, y, colors[frame]); pixel(target, x + 1, y + 1, colors[frame]); capture = target.slice(); },
            additional: (sid, x, y, target) => { assert.equal(sid, SID); trace.push(`extra:${x},${y}`); if (v[16]) pixel(target, x + 2, y + 2, v[16] === 1 ? 7 : 255); capture = target.slice(); } };
        const result = new RawObjectHit(s.pool, s.types, s.shapes, hooks).hit(SID, v[4] | 0, v[5] | 0, [v[2] | 0, v[3] | 0]); assert.equal(result, Number(row[1]) !== 0, row[0]); assert.equal(trace.join(';'), row[2], row[0]); assert.equal(adler32(capture), Number(row[3]), row[0]); assert.deepEqual(s.raw, before, row[0]); if (result) accepted += 1;
    }
    assert.ok(accepted > 0 && accepted < rows.length);
});

test('ObjectHit_RequiresOnlyReachedDrawingAndRejectsInvalidHeader: opaque/음수점/200밖의 도달과 헤더/HP 진단을 구별한다', () => {
    const s = scene(); s.types[TYPE].flags1 = 0x800000; Object.assign(s.shapes[TYPE], { loaded: true, frameCount: 1, frames: [{ width: 20, height: 20, hotspotX: 0, hotspotY: 0 }] }); const hooks = { frame: () => 0 }; const body = new RawObjectHit(s.pool, s.types, s.shapes, hooks);
    assert.equal(body.hit(SID, 1, 1, [0, 0]), true); s.types[TYPE].flags1 = 0; assert.equal(body.hit(SID, -1, -1, [0, 0]), false); assert.throws(() => body.hit(SID, 200, 200, [0, 0]), /하위 미연결/); s.shapes[TYPE].loaded = false; assert.throws(() => body.hit(SID, -1, -1, [0, 0]), /frameCheck/); s.shapes[TYPE].loaded = true;
    // 원문의 두 힙 채움 HP 패턴은 음수점/opaque보다 먼저 거부한다.
    for (const value of [0xdddddddd, 0xcdcdcdcd]) { s.types[TYPE].maxHitPoints = value | 0; assert.throws(() => body.hit(SID, -1, -1, [0, 0]), /frameCheck/); }
    s.types[TYPE].maxHitPoints = 0; s.types[TYPE].flags1 = 0x840000; s.shapes[TYPE].frames.push({ width: 20, height: 20, hotspotX: 0, hotspotY: 0 }); hooks.frame = () => 1; assert.equal(body.hit(SID, 1, 1, [0, 0]), true); s.types[TYPE].flags1 = 0x800000; assert.throws(() => body.hit(SID, 1, 1, [0, 0]), /frameCheck/);
    assert.throws(() => new RawObjectHit({ edition: 'cd1072' }, s.types, s.shapes), /TYPE\/SHP/); assert.throws(() => new RawObjectHit(s.pool, s.types.slice(1), s.shapes), /TYPE\/SHP/); s.raw[10] = 255; assert.throws(() => body.hit(SID, 0, 0, [0, 0]), /현재TYPE/);
});

test('ObjectHit 현재값: 첫TYPE·선택 헤더/상대점은 저장하고 getter 뒤 살아 있는 raw·추가 분기를 읽는다', () => {
    const s = scene(); Object.assign(s.types[TYPE], { hotspotX: 3, hotspotY: -2 }); Object.assign(s.shapes[TYPE], { loaded: true, frameCount: 3, frames: Array.from({ length: 3 }, () => ({ width: 20, height: 20, hotspotX: 5, hotspotY: 6 })) }); const events = []; let calls = 0;
    const hooks = { frame: () => { const frame = calls++; events.push(`frame:${frame}`); if (frame === 0) { s.raw[10] = 81; s.raw[40] = 2; put(s.raw, 14, floatToBits(10)); put(s.raw, 18, floatToBits(20)); } return frame; },
        plain: (type, frame, x, y, pixels) => { assert.deepEqual([type, frame, x, y], [80, 2, 5, 6]); events.push('plain'); pixels[0] = 9; s.shapes[TYPE].frames[1].hotspotX = 999; s.types[TYPE].hotspotX = 1000; put(s.raw, 36, 3); },
        additional: (sid, x, y, pixels) => { assert.deepEqual([sid, x, y, pixels[0]], [SID, 5, 6, 9]); events.push('extra'); put(s.raw, 14, floatToBits(1000)); } };
    assert.equal(new RawObjectHit(s.pool, s.types, s.shapes, hooks).hit(SID, 152, 216, [0, 0]), true); assert.deepEqual(events, ['frame:0', 'frame:1', 'frame:2', 'plain', 'extra']); assert.equal(s.raw[10], 81); assert.equal(get(s.raw, 36), 3); assert.equal(get(s.raw, 14), floatToBits(1000));
});
