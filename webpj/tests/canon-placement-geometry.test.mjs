// 일반 배치 모양 기하 raw-canon-placement-geometry.js(RawCanonPlacementGeometry·canonCollisionArea)를 원본 10.78 실제 PE 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: canongeometry-x86.tsv 의 `originals` 3159행 — 타입 전역 9종(영역·다리·섬·받침·전투 넷·일반 사제)×인자×방향(홀수·signed −1 포함)×프레임 표 3종×전역 별칭×기본 프레임×발자국 변경(mutation)에서
//    미리보기 범위(bounds)와 모든 유효 칸의 finder 직전 계산(프레임·원래 좌표 비트·번호·방향 글자·절삭 좌표 비트·finder 정수 사각형 네 값)이 같고, 칸마다 준비/후처리가 한 번씩, 최종 판정이 한 번 불린다.
//    `originalCD`·`original1037` 행은 다른 판본이라 사용하지 않는다.
//  - 계약: 다중 칸의 준비/후처리/조기 거부 순서와 빈 모양의 최종 호출, 요청의 값 캡처, 실제 해시/finder 가 패턴 모양의 차례대로 후보를 찾고 거부 뒤 지형 판정을 생략, 누락 경계·표 밖 타입·음수 발자국·비유한 좌표 거부.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E146: 일반 배치 모양 기하 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, bitsToFloat, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { RawSquidFinder } from '../www/core/raw-squid-finder.js';
import { RawCanonPlacementGeometry, canonCollisionArea } from '../www/core/raw-canon-placement-geometry.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-09 WB04-E146)
const rows = (readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'canongeometry-x86.tsv')) ?? []).filter((row) => row[0] === 'originals');

// 실제 타입 전역의 패턴 번호와 마지막 비패턴 사제 번호, 판본 행 수, 풀 크기이다. (2026-11-09 WB04-E146)
const TYPES = [107, 82, 94, 157, 131, 129, 140, 142, 158];
const ROWS = 3159;
const CAPACITY = 24000;

// 예외가 나는지 본다. (2026-11-09 WB04-E146)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 기대 좌표를 계산하지 않고 기계어 입력의 코드 순서만 구성한다. profile 0 정상, 1 역순, 2 빈 표이다. (2026-11-09 WB04-E146)
function inputFrames(profile) {
    const codes = [];
    if (profile !== 2) {
        // 정상 방향 A..P 와 원본 셀의 번호 영역을 모두 공급한다(번호 10 도 포함).
        for (let side = 65; side <= 80; side += 1) {
            for (let number = 0; number <= 10; number += 1) codes.push({ side, variant: 80, number, flags: 0 });
        }
    }
    if (profile === 1) codes.reverse();
    if (profile !== 2) codes.push(codes[0]);
    return codes;
}

// 기본 지역 경계는 순회 검사에서만 명시적으로 허용한다. 실제 지형 정책을 대신하지 않는다. (2026-11-09 WB04-E146)
const allowRegions = () => ({ beginRegions: () => {}, endShape: () => true, finishRegions: () => true });

// 풀·타입·프레임·전역을 만든다. (2026-11-09 WB04-E146)
function makeWorld() {
    const pool = new SidPool(CAPACITY, { server: true });
    const types = Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, footX: 0, footY: 0 }));
    const frames = Array.from({ length: 188 }, () => ({ frames: [], defaultFrame: 0 }));
    const state = { patternTypes: TYPES.slice(0, 8), decoderDiagnostics: null };
    return { pool, types, frames, state, geometry: new RawCanonPlacementGeometry(pool, types, frames, state) };
}

test('일반 배치 모양: 원본 10.78 관측 3159행의 범위·모든 칸의 finder 직전 계산이 같다', { skip: rows.length === 0 }, () => {
    assert.equal(rows.length, ROWS);
    const { types, frames, state, geometry } = makeWorld();
    // 입력의 패턴 번호/별칭/현재 발자국과 프레임 메타를 독립적으로 구성한다.
    for (const row of rows) {
        assert.equal(row.length, 15);
        const type = TYPES[Number(row[1])];
        state.patternTypes = TYPES.slice(0, 8);
        if (row[10] === '1') state.patternTypes[0] = state.patternTypes[1];
        if (row[10] === '2') state.patternTypes[2] = state.patternTypes[4];
        frames[type].frames = inputFrames(Number(row[4]));
        frames[type].defaultFrame = Number(row[11]) | 0;
        const wx = Number(row[7]) | 0;
        const wy = Number(row[8]) | 0;
        types[type].footX = wx;
        types[type].footY = wy;
        const query = { type, argument: Number(row[2]) | 0, direction: Number(row[3]) | 0, x: bitsToFloat(Number(row[5])), y: bitsToFloat(Number(row[6])), explicitFrame: row[9] !== '0' };
        const where = row.slice(0, 13).join(' ');
        const bounds = geometry.bounds(query);
        assert.deepEqual([bounds.left, bounds.top, bounds.right, bounds.bottom], row[13].split(',').map(Number), where);
        const expected = row[14] === '-' ? [] : row[14].split(';');
        let ordinal = 0;
        let begun = 0;
        let ended = 0;
        let finished = 0;
        const hooks = allowRegions();
        hooks.beginRegions = () => { begun += 1; };
        hooks.endShape = () => { ended += 1; return true; };
        hooks.finishRegions = () => { finished += 1; return true; };
        const result = geometry.inspect(query, (cell) => {
            assert.ok(ordinal < expected.length, where);
            const fields = expected[ordinal].split(',').map(Number);
            assert.equal(fields.length, 11);
            assert.equal(cell.frame, fields[0], where);
            assert.equal(floatToBits(cell.x), fields[1], where);
            assert.equal(floatToBits(cell.y), fields[2], where);
            assert.equal(cell.label, fields[3], where);
            assert.equal(cell.side, fields[4], where);
            assert.equal(floatToBits(cell.snappedX), fields[5], where);
            assert.equal(floatToBits(cell.snappedY), fields[6], where);
            assert.deepEqual([cell.area.left, cell.area.top, cell.area.right, cell.area.bottom], fields.slice(7), where);
            ordinal += 1;
            // 다음 칸의 발자국 변경은 기대 사각형이 아닌 원본 분석 입력과 동일한 메타 변경이다.
            types[type].footX = wx + (row[12] !== '0' ? ordinal % 3 : 0);
            types[type].footY = wy + (row[12] !== '0' ? (ordinal + 1) % 3 : 0);
            return true;
        }, hooks);
        assert.equal(result, true, where);
        assert.equal(ordinal, expected.length, where);
        assert.equal(begun, 1, where);
        assert.equal(ended, expected.length, where);
        assert.equal(finished, 1, where);
    }
});

test('일반 배치 모양: 다중 칸의 준비/후처리/조기 거부 순서와 빈 모양의 최종 호출을 지킨다', () => {
    const { types, frames, geometry } = makeWorld();
    types[157].footX = types[157].footY = 1;
    frames[157].frames = inputFrames(0);
    let query = { type: 157, argument: 0, direction: 0, x: 20.5, y: 21.5, explicitFrame: false };
    const events = [];
    let scanned = 0;
    const hooks = allowRegions();
    // 요청은 값으로 캡처되어 훅이 호출자의 객체를 바꿔도 처음 좌표를 유지한다.
    hooks.beginRegions = (q) => { assert.equal(q.x, 20.5); events.push('begin'); query.x = 99; };
    hooks.beginShape = (q) => { assert.equal(q.x, 20.5); events.push('prepare'); };
    hooks.endShape = () => { events.push('end'); return true; };
    hooks.finishRegions = () => { events.push('finish'); return true; };
    assert.equal(geometry.inspect(query, () => { events.push('scan'); scanned += 1; return scanned !== 2; }, hooks), false);
    assert.deepEqual(events, ['begin', 'prepare', 'scan', 'end', 'prepare', 'scan']);
    events.length = 0;
    scanned = 0;
    query.x = 20.5;
    hooks.endShape = () => { events.push('end'); return false; };
    assert.equal(geometry.inspect(query, () => { scanned += 1; events.push('scan'); return true; }, hooks), false);
    assert.equal(scanned, 1);
    assert.equal(events.at(-1), 'end');
    // 기본 프레임이 -1 이면 유효 칸이 없어도 시작/최종 판정은 불린다. (2026-11-09 WB04-E146)
    query = { type: 70, argument: 70, direction: 0, x: 20.5, y: 21.5, explicitFrame: false };
    types[70].footX = types[70].footY = 1;
    frames[70].defaultFrame = -1;
    events.length = 0;
    assert.equal(geometry.inspect(query, () => { assert.fail('칸이 없다'); }, hooks), true);
    assert.deepEqual(events, ['begin', 'finish']);
    // 시작 뒤 기본 프레임을 바꿔도 디코더 생성 때의 첫 프레임 선택을 유지한다.
    frames[70].frames = inputFrames(0);
    frames[70].defaultFrame = 2;
    const mutating = allowRegions();
    mutating.beginRegions = () => { frames[70].defaultFrame = -1; };
    let seen = 0;
    assert.equal(geometry.inspect(query, (cell) => { assert.equal(cell.frame, 2); seen += 1; return true; }, mutating), true);
    assert.ok(seen > 0);
});

test('일반 배치 모양: 실제 해시/finder 가 패턴 모양의 차례대로 후보를 찾고 거부 뒤 지형 판정을 생략한다', () => {
    const { pool, types, frames, geometry } = makeWorld();
    const hash = new SquidHash();
    types[157].footX = types[157].footY = 1;
    types[83].footX = types[83].footY = 1;
    frames[157].frames = inputFrames(0);
    const blocker = pool.allocate();
    const raw = pool.allocated(blocker);
    raw[10] = 83;
    put(raw, 14, floatToBits(21));
    put(raw, 18, floatToBits(22));
    hash.setBucket(1, 21, 22, blocker);
    const finder = new RawSquidFinder(pool, hash, types);
    let scanned = 0;
    let ended = 0;
    let finished = 0;
    const hooks = allowRegions();
    hooks.endShape = () => { ended += 1; return true; };
    hooks.finishRegions = () => { finished += 1; return true; };
    assert.equal(geometry.inspect({ type: 157, argument: 0, direction: 0, x: 20.5, y: 21.5, explicitFrame: false }, (cell) => { scanned += 1; return finder.begin(cell.area) === 0; }, hooks), false);
    assert.ok(scanned > 0 && scanned <= 9);
    assert.equal(ended, scanned - 1);
    assert.equal(finished, 0);
    assert.equal(finder.state.current, blocker);
});

test('일반 배치 모양: 누락 경계·표 밖 타입·음수 발자국·비유한 좌표를 거부하고 방향 코드 없이도 모양을 계산한다', () => {
    const { pool, types, frames, state, geometry } = makeWorld();
    const hooks = allowRegions();
    const query = { type: 70, argument: 70, direction: 0, x: 20, y: 21, explicitFrame: false };
    assert.ok(throws(() => new RawCanonPlacementGeometry(pool, types, [], state)));
    assert.ok(throws(() => geometry.bounds({ type: 188, argument: 0, direction: 0, x: 0, y: 0 })));
    assert.ok(throws(() => geometry.inspect(query, null, hooks)));
    assert.ok(throws(() => geometry.inspect(query, () => true, {})));
    // 모양 계산에는 방향 코드가 필요 없다. 원본 기본 프레임의 개수 검사 생략을 보존한다.
    frames[70].defaultFrame = 4;
    types[70].footX = types[70].footY = 1;
    assert.equal(geometry.inspect(query, (cell) => { assert.equal(cell.frame, 4); assert.equal(cell.side, 0); return true; }, hooks), true);
    assert.ok(throws(() => canonCollisionArea(20, 21, -1, 1)));
    assert.ok(throws(() => canonCollisionArea(Infinity, 21, 1, 1)));
    assert.deepEqual(canonCollisionArea(20.5, 21.25, 3, 2), { left: 18, top: 20, right: 21, bottom: 22 });
});
