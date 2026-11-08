// 너겟 보충 raw-nugget-replenish.js(RawNuggetReplenish)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: nugget-replenish-x86.tsv(10.78 전용; x87 정밀도 53/64 두 관측이 같은 열) 행 — 권한·단독·요새·편집·지역 확장 단위·record 위치/높낮이·발자국·프레임 유무·지도 배치·진단 효과 변이 → 호출 순서(A 진단 / P 배치 사각형 네 float 비트와 수량)와
//    효과 뒤 현재 저장소 8칸(권한·단독·요새·편집·확장 단위·조각 타입·타입 107 발자국 둘). 진입 표식(R/O/C)은 외부 효과 비교에서 제외한다.
//  - 계약: NuggetReplenish_CurrentModesAndRequiredEffects(모드 관문에서는 미선택 하위를 요구하지 않고 선택된 미연결 효과는 거부).
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E71: 너겟 보충 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { ChunkMap } from '../www/assets/territory.js';
import { PLAYER_GEYSER_BOUNDS_ASSERTION, RawPlayerGeyserBounds } from '../www/core/raw-player-geyser-bounds.js';
import { RawNuggetReplenish } from '../www/core/raw-nugget-replenish.js';

// 원본 관측 행이다(단일 판본 파일). 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E71)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'nugget-replenish-x86.tsv'));

// 예외가 나는지 본다. (2026-11-08 WB04-E71)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 진입 표식(R/O/C)과 빈 표식을 제외한 외부 효과 문자열이다. (2026-11-08 WB04-E71)
function effects(native) {
    const kept = native.split(';').filter((part) => part !== 'R' && part !== 'O' && part !== 'C' && part !== '-');
    return kept.length === 0 ? '-' : kept.join(';');
}

// 188칸 TYPE(발자국 0)과 빈 프레임 표를 만든다. (2026-11-08 WB04-E71)
const emptyTypes = () => Array.from({ length: 188 }, () => ({ footX: 0, footY: 0 }));
const emptyFrames = () => Array.from({ length: 188 }, () => []);

test('NuggetReplenish: 원본 10.78 행의 전체 패턴/회전·폭 감김과 현재 전역 재독이 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    const feet = [[1, 1], [2, 3], [0, 1], [-1, 2]];
    let count = 0;
    // 기대 사각형을 계산하지 않고 같은 입력 자료만 준비한다.
    for (const row of rows) {
        assert.equal(row.length, 5);
        const args = row[0].split(',').map(Number);
        assert.equal(args.length, 11);
        assert.ok(row[1] === row[3] && row[2] === row[4]);
        let boss = args[0] !== 0;
        let single = args[1] !== 0;
        let fort = args[2] !== 0;
        let editor = args[3];
        let span = args[4] | 0;
        let piece = 107;
        const pool = new SidPool(32768);
        const chunks = new ChunkMap(16, 16);
        const types = emptyTypes();
        const frames = emptyFrames();
        const players = new Uint8Array(9 * 0xb4);
        const [footX, footY] = feet[args[7]];
        const codes = [];
        if (!args[8]) {
            // 모든 방향/셀 번호를 명시 입력한다.
            for (let side = 0x41; side <= 0x50; side += 1) for (let number = 0; number < 11; number += 1) codes.push({ side, variant: 0x50, number, flags: 0 });
        }
        // 진단 뒤 선택할 타입도 같은 입력을 가진다.
        for (const type of [107, 108]) {
            types[type].footX = footX;
            types[type].footY = footY;
            frames[type] = codes;
        }
        // 원본 준비의 기본 지도 전체를 동일한 stale record 로 초기화한다.
        for (let x = 0; x < 16; x += 1) {
            for (let y = 0; y < 16; y += 1) {
                chunks.at(x, y).island = 127;
                chunks.at(x, y).record.set([31, 41, 59, 26, 53, 58]);
            }
        }
        if (args[9] !== 2) {
            chunks.at(2, 3).island = 0;
            chunks.at(2, 3).record.set([(args[5] | (args[6] << 6)) & 0xff, 0xa5, 0x32, 17, 29, 41]);
        }
        if (args[9] === 1) {
            chunks.at(3, 0).island = 0;
            chunks.at(3, 0).record.set([(((args[5] + 7) & 63) | (args[6] << 6)) & 0xff, 0xa5, 0xff, 17, 29, 41]);
        }
        const state = { battle: true, mission: true, metamap: false, mapSize: 0, get puzzlePieceType() { return piece; }, playerDirections: [0, 4, 0, 6, 2, 7, 3, 5, 1] };
        const events = [];
        let changed = false;
        const bounds = new RawPlayerGeyserBounds(pool, chunks, types, frames, players, state, (kind) => {
            assert.equal(kind, PLAYER_GEYSER_BOUNDS_ASSERTION.missingFrame);
            events.push('A:299');
            if (changed) return;
            changed = true;
            if (args[10] === 6) span = 0x8000000;
            else if (args[10] === 7) { types[107].footX = 2; types[107].footY = 3; piece = 108; }
        });
        const hooks = {
            assertion: (line) => {
                assert.equal(line, 0x246);
                events.push(`A:${line}`);
                if (!args[10]) return;
                boss = true;
                if (args[10] === 2) single = true;
                else if (args[10] === 3) fort = true;
                else if (args[10] === 4) editor = 1;
                else if (args[10] === 5) span = 0x7fffffff;
            },
            place: (area, quantity) => {
                events.push(`P:${[floatToBits(area.left), floatToBits(area.top), floatToBits(area.right), floatToBits(area.bottom), quantity].join(':')}`);
            }
        };
        const world = { get boss() { return boss; }, get single() { return single; }, get fort() { return fort; }, get editor() { return editor; }, get archipelagoSpan() { return span; } };
        new RawNuggetReplenish(bounds, world, hooks).run();
        const current = [boss ? 1 : 0, single ? 1 : 0, fort ? 1 : 0, editor >>> 0, span >>> 0, piece, types[107].footX >>> 0, types[107].footY >>> 0].join(',');
        assert.equal(events.length === 0 ? '-' : events.join(';'), effects(row[1]), `행 ${count}: 입력 ${row[0]} 원본 ${row[1]}`);
        assert.equal(current, row[2], `행 ${count}: 입력 ${row[0]}`);
        count += 1;
    }
    assert.ok(count > 350);
});

test('NuggetReplenish: 모드 관문에서는 미선택 하위를 요구하지 않고 선택된 미연결 효과는 거부한다', () => {
    const pool = new SidPool(32768);
    const chunks = new ChunkMap(16, 16);
    const types = emptyTypes();
    const frames = emptyFrames();
    const players = new Uint8Array(9 * 0xb4);
    let boss = true;
    let single = true;
    let fort = false;
    let editor = 0;
    const state = { battle: true, mission: true, metamap: false, mapSize: 0, puzzlePieceType: 107, playerDirections: new Array(9).fill(0) };
    const bounds = new RawPlayerGeyserBounds(pool, chunks, types, frames, players, state, () => { throw new Error('선택되지 않은 범위 조회'); });
    const world = { get boss() { return boss; }, get single() { return single; }, get fort() { return fort; }, get editor() { return editor; }, archipelagoSpan: 1 };
    const body = new RawNuggetReplenish(bounds, world, {});
    body.run();
    single = false;
    fort = true;
    body.run();
    fort = false;
    editor = 1;
    body.run();
    editor = 0;
    boss = false;
    assert.ok(throws(() => body.run()));
    // 선택된 Canon 또는 배치 경계의 누락은 성공으로 처리되지 않는다.
    boss = true;
    assert.ok(throws(() => body.run()));
});
