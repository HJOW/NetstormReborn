// Player 간헐천 범위 raw-player-geyser-bounds.js(RawPlayerGeyserBounds)와 그 패턴 폭/높이 계산(territory.js 의 CanonDecoder 발자국 범위)을 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: player-geyser-bounds-x86.tsv 439행(Bounds·RegionRecord·Placement, 10.78 PE 전용 관측). cpppj PlayerGeyserBounds_IndependentPeFixture 와 같은 청크 지도·Player 9기록·
//    TYPE 발자국/프레임·현재 모드와 진단 효과(지역/모드/발자국/패턴 타입/record 변경)로 출력 16 BYTE·효과 뒤 현재 저장소 8칸·진단 순서를 비교한다.
//  - 계약: PlayerGeyserBounds_RejectsMissingAssertion, CanonConsumers_PlayerGeyserPatternDimensions(같은 Canon 진단 소유자의 금 간 프레임 진단 0x325).
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E57: Player 간헐천 범위 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { ChunkMap } from '../www/assets/territory.js';
import { PLAYER_GEYSER_BOUNDS_ASSERTION, RawPlayerGeyserBounds } from '../www/core/raw-player-geyser-bounds.js';

// 원본 관측 행(판본 열 없음)이다. 파일이 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E57)
const FIXTURES = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures');
const boundsRows = readRows(path.join(FIXTURES, 'player-geyser-bounds-x86.tsv'));

// 중복 제거한 원본 입력 수, 지도 크기 세 가지, 발자국 네 가지 입력이다(cpppj 와 같음). (2026-11-08 WB04-E57)
const CASES = 439;
const DIMENSIONS = Object.freeze([[16, 16], [3, 4], [1, 1]]);
const FEET = Object.freeze([[1, 1], [2, 3], [0, 1], [-1, 2]]);

// 예외가 나는지 본다. (2026-11-08 WB04-E57)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 기대 사각형을 계산하지 않고 같은 record 6 BYTE 만 만든다(cpppj BoundsRecord). (2026-11-08 WB04-E57)
function boundsRecord(row, alternate = false) {
    return Uint8Array.from([
        (((Number(row[2]) + (alternate ? 7 : 0)) & 63) | (Number(row[3]) << 6)) & 0xff, 0xa5,
        (Number(row[4]) ^ (alternate ? 0x55 : 0)) & 0xff, 17, 29, 41,
    ]);
}

// 실제 원문 검색을 검증할 지도만 입력하며 첫 일치는 모듈이 찾는다(cpppj BoundsMap). (2026-11-08 WB04-E57)
function boundsMap(chunks, row) {
    // 유효 지도 안 전체를 원본과 같은 기본 127/stale record 로 채운다.
    for (let x = 0; x < 16; x += 1) {
        // 유효 지도 안만 쓰면 가장자리 당김에 따른 별도 덮어쓰기도 없다.
        for (let y = 0; y < 16; y += 1) {
            if (x < chunks.xLength && y < chunks.yLength) {
                chunks.at(x, y).island = 127;
                chunks.at(x, y).record.set([31, 41, 59, 26, 53, 58]);
            }
        }
    }
    const x = Math.min(2, chunks.xLength - 1);
    const y = Math.min(3, chunks.yLength - 1);
    const layout = Number(row[11]);
    const place = (px, py, region, alternate = false) => {
        const entry = chunks.at(px, py);
        entry.island = region;
        entry.record.set(boundsRecord(row, alternate));
    };
    if (layout === 0) place(x, y, 7);
    else if (layout === 1) {
        place(Math.min(3, chunks.xLength - 1), 0, 7);
        place(0, Math.min(3, chunks.yLength - 1), 7, true);
    } else if (layout === 3) {
        place(x, y, 7);
        place(0, 0, 8, true);
    } else if (layout === 4) place(x, y, 255);
    else if (layout === 5) place(x, y, 128);
}

// 188칸 TYPE(발자국 0)과 빈 프레임 표를 만든다. (2026-11-08 WB04-E57)
const emptyTypes = () => Array.from({ length: 188 }, () => ({ footX: 0, footY: 0 }));
const emptyFrames = () => Array.from({ length: 188 }, () => []);

// 모든 방향 글자 A~P·변형 'P'·번호 0~10 의 프레임 코드(패턴 셀이 조회하는 번호를 모두 포함)이다. (2026-11-08 WB04-E57)
function allCodes(flags = 0) {
    const codes = [];
    // 방향마다 번호 0~10 을 넣는다.
    for (let side = 0x41; side <= 0x50; side += 1) for (let number = 0; number < 11; number += 1) codes.push({ side, variant: 0x50, number, flags });
    return codes;
}

test('PlayerGeyserBounds: 원본 439행의 실제 검색/양쪽 배치/Canon 과 전체 상태/진단 순서가 같다', { skip: !boundsRows && '원본 fixture 없음' }, () => {
    let count = 0;
    // 사례마다 같은 입력을 독립 저장소에 공급한다.
    for (const row of boundsRows) {
        assert.equal(row.length, 20);
        const pool = new SidPool(32768);
        const [width, height] = DIMENSIONS[Number(row[12])];
        const chunks = new ChunkMap(width, height);
        boundsMap(chunks, row);
        const types = emptyTypes();
        const frames = emptyFrames();
        const [footX, footY] = FEET[Number(row[13])];
        const codes = Number(row[14]) === 0 ? allCodes() : [];
        // 진단 효과가 고를 타입도 같은 원문 표에 존재한다.
        for (const type of [107, 108]) {
            types[type].footX = footX;
            types[type].footY = footY;
            frames[type] = codes;
        }
        const owner = Number(row[1]) | 0;
        const player = owner >= 0 && owner < 9 ? owner : 1;
        const players = new Uint8Array(9 * 0xb4);
        put(players, player * 0xb4 + 0x50, Number(row[6]), 1);
        put(players, player * 0xb4 + 0x78, (Number(row[5]) | 0) >>> 0);
        const directions = [0, 4, 0, 6, 2, 7, 3, 5, 1];
        // 현재 배치표 자체가 바뀐 명시 입력이며 ROM 기본 순서로 고정하지 않는다.
        if (Number(row[16])) for (let index = 0; index < directions.length; index += 1) directions[index] = (directions[index] + 2) & 7;
        const state = { battle: Number(row[7]) !== 0, mission: Number(row[8]) !== 0, metamap: Number(row[9]) !== 0, mapSize: Number(row[10]) & 0xff, puzzlePieceType: 107, playerDirections: directions };
        const mutation = Number(row[15]);
        const trace = [];
        let mutated = false;
        const body = new RawPlayerGeyserBounds(pool, chunks, types, frames, players, state, (kind) => {
            trace.push(kind);
            if (mutated || !mutation) return;
            mutated = true;
            if (mutation === 1) put(players, player * 0xb4 + 0x78, 8);
            else if (mutation === 2) { state.mission = false; state.mapSize = 3; }
            else if (mutation === 3) { state.mission = true; types[107].footX = 3; }
            else if (mutation === 4) { types[107].footX = 2; types[107].footY = 3; state.puzzlePieceType = 108; }
            else if (mutation === 5) chunks.at(Math.min(2, width - 1), Math.min(3, height - 1)).record.set(boundsRecord(row, true));
        });
        assert.ok(body.pool === pool && body.map === chunks);
        const output = new Uint8Array(16);
        // 부분 record 출력의 뒤 stale BYTE 도 원본과 같은 값으로 보존한다.
        for (let index = 0; index < 4; index += 1) put(output, index * 4, 0xdeadbeef);
        if (row[0] === 'B') {
            const area = body.bounds(owner >>> 0);
            [area.left, area.top, area.right, area.bottom].forEach((value, index) => put(output, index * 4, floatToBits(value)));
        } else if (row[0] === 'R') output.set(body.regionRecord(Number(row[5]) | 0), 0);
        else if (row[0] === 'P') {
            const area = body.placement(owner, boundsRecord(row));
            [area.left, area.top, area.right, area.bottom].forEach((value, index) => put(output, index * 4, value >>> 0));
        } else assert.fail(`Player 간헐천 범위 fixture 연산 오류 ${row[0]}`);
        const actual = [0, 4, 8, 12].map((offset) => get(output, offset)).join(',');
        const current = [state.battle ? 1 : 0, state.mission ? 1 : 0, state.metamap ? 1 : 0, state.mapSize, state.puzzlePieceType, types[107].footX >>> 0, types[107].footY >>> 0, get(players, player * 0xb4 + 0x78)].join(',');
        assert.deepEqual([actual, current, trace.length ? trace.join(',') : '-'], [row[17], row[18], row[19]], `행 ${count} ${row.slice(0, 17).join(' ')}`);
        count += 1;
    }
    assert.equal(count, CASES);
});

test('PlayerGeyserBounds: 필수 실제 진단이 없으면 손상 모드/프레임을 조용한 성공으로 바꾸지 않는다', () => {
    const state = { battle: true, mission: true, metamap: false, mapSize: 0, puzzlePieceType: 107, playerDirections: new Array(9).fill(0) };
    assert.ok(throws(() => new RawPlayerGeyserBounds(new SidPool(32768), new ChunkMap(), emptyTypes(), emptyFrames(), new Uint8Array(9 * 0xb4), state, null)));
    assert.ok(throws(() => new RawPlayerGeyserBounds(new SidPool(32768), new ChunkMap(), emptyTypes().slice(1), emptyFrames(), new Uint8Array(9 * 0xb4), state, () => {})));
});

test('CanonConsumers: Player 패턴 폭/높이 계산도 같은 현재 Canon 진단 소유자(금 간 프레임 0x325)를 소비한다', () => {
    const frames = emptyFrames();
    // 패턴 선택 순서를 유지하고 금 간 비트만 입력한다.
    frames[107] = allCodes(0x20);
    let checking = 1;
    let reported = 0;
    const diagnostics = {
        checking: () => checking,
        assertion: (expression, file, line) => {
            assert.equal(line, 0x325);
            reported += 1;
            throw new Error('Player 범위 Canon 진단 중단');
        },
    };
    const state = { battle: true, mission: true, metamap: false, mapSize: 0, puzzlePieceType: 107, playerDirections: new Array(9).fill(0), decoderDiagnostics: diagnostics };
    const body = new RawPlayerGeyserBounds(new SidPool(32768), new ChunkMap(), emptyTypes(), frames, new Uint8Array(9 * 0xb4), state, () => { throw new Error('다른 Player 범위 진단 미선택'); });
    const record = Uint8Array.from([3, 1, 0, 7, 0xaa, 0xbb]);
    assert.ok(throws(() => body.originPlacement(record)));
    assert.equal(reported, 1);
    // checking 이 꺼지면 같은 금 간 프레임으로도 진단 없이 원점 배치를 돌려준다.
    checking = 0;
    const placed = body.originPlacement(record);
    assert.deepEqual([placed.left, placed.top, reported], [6, 6, 1]);
    assert.equal(PLAYER_GEYSER_BOUNDS_ASSERTION.missingFrame, 4);
});
