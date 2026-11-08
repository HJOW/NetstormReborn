// 총 공격 방향 getter raw-gun-mode.js(RawGunMode)를 cpppj 소스(패치 004d4d00)의 순서 계약으로 대조한다.
//  - cpppj 에는 이 파일만의 독립 fixture 가 없고(캠페인 프레임·얼음/천둥 미사일 시험이 소비) 소스 주석의 계약만 둔다. 이 검사는 그 계약을 직접 입력으로 확인한다:
//    비astral 크로스보우는 현재 graph BYTE × 2, 나머지는 signed 프레임 문자 × 2 - 152 의 0..7 만(밖은 0), 현재 슬롯에서 매 조회, 표 밖 타입/프레임 거부.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E71: 총 공격 모드 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import { put } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { RawGunMode } from '../www/core/raw-gun-mode.js';

// 관찰 SID, 크로스보우(windArcher) 타입과 일반 총 타입 번호이다. (2026-11-08 WB04-E71)
const SOURCE = 5;
const WIND_ARCHER = 140;
const GUN = 120;

// 예외가 나는지 본다. (2026-11-08 WB04-E71)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 글자 코드(side) 목록으로 188칸 코드표를 만든다. (2026-11-08 WB04-E71)
function tables(sides) {
    const frames = Array.from({ length: 188 }, () => new FrameCodeTable([]));
    frames[GUN] = new FrameCodeTable(sides.map((side) => ({ side, variant: 0x50, flags: 0 })));
    frames[WIND_ARCHER] = frames[GUN];
    return frames;
}

test('GunMode: signed 프레임 문자 × 2 - 152 의 0..7 만 반환하고 밖은 0 이다', () => {
    const pool = new SidPool(32768);
    const sides = [0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0xcc, 0x41];
    const mode = new RawGunMode(pool, tables(sides), () => WIND_ARCHER);
    const raw = pool.physical(SOURCE);
    raw.fill(0);
    raw[10] = GUN;
    // K(-2), L(0), M(2), N(4), O(6), P(8 은 범위 밖), 음수 BYTE(0xcc), A(음수) 순서의 기대값이다.
    const expected = [0, 0, 2, 4, 6, 0, 0, 0];
    expected.forEach((value, frame) => {
        put(raw, 36, frame);
        assert.equal(mode.get(SOURCE), value, `프레임 ${frame}`);
    });
});

test('GunMode: 비astral 크로스보우는 현재 graph BYTE × 2 를 반환하고 astral 은 프레임 문자를 쓴다', () => {
    const pool = new SidPool(32768);
    let windArcher = WIND_ARCHER;
    const mode = new RawGunMode(pool, tables([0x4e]), () => windArcher);
    const raw = pool.physical(SOURCE);
    raw.fill(0);
    raw[10] = WIND_ARCHER;
    raw[30] = 3;
    assert.equal(mode.get(SOURCE), 6);
    raw[40] = 8;
    assert.equal(mode.get(SOURCE), 4);
    raw[40] = 0;
    raw[30] = 200;
    assert.equal(mode.get(SOURCE), 400);
    // 원본의 mutable 타입 DWORD 를 매 조회 읽는다.
    windArcher = 141;
    assert.equal(mode.get(SOURCE), 4);
});

test('GunMode: 표 밖 타입/프레임과 잘못된 연결은 거부한다', () => {
    const pool = new SidPool(32768);
    const mode = new RawGunMode(pool, tables([0x4e]), () => WIND_ARCHER);
    const raw = pool.physical(SOURCE);
    raw.fill(0);
    raw[10] = 69;
    assert.ok(throws(() => mode.get(SOURCE)));
    raw[10] = 200;
    assert.ok(throws(() => mode.get(SOURCE)));
    raw[10] = GUN;
    put(raw, 36, 1);
    assert.ok(throws(() => mode.get(SOURCE)));
    assert.ok(throws(() => new RawGunMode(pool, tables([]).slice(1), () => WIND_ARCHER)));
    assert.ok(throws(() => new RawGunMode(pool, tables([]), 140)));
    assert.equal(mode.pool, pool);
});
