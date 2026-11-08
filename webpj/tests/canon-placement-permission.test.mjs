// 일반 배치 권한 raw-canon-placement-permission.js(RawCanonPlacementPermission·지형 훅 어댑터)를 원본 10.78 실제 PE 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: canonpermission-x86.tsv 의 `originals` 5850행 — 후보 권한(C)과 방향 관계 helper(R)를 그래프 준비/편집기/방향 동맹/다른 소유자 허용 전역, 현재 소유자·요청자, 정방향/역방향 관계 표 값, Player 조회 응답,
//    후보 SID/타입/좌표(부호 있는 0·NaN 포함), 조회 중 raw·전역 변경 입력으로 실제 PE 에서 실행해 반환 DWORD·Player 조회 횟수와 인자 비트 네 개·최종 raw 슬롯 전체·편집기 값·관계 표 Adler-32 가 같다.
//    `originalCD`·`original1037` 행은 다른 판본이라 사용하지 않는다.
//  - 계약: 요청 DWORD 를 유지하며 helper 에는 부호 있는 BYTE 소유자를 전달하는 지형 훅 어댑터, 현재 raw 를 매번 다시 읽음, 필수 Player 경계·관계 표 범위·다른 풀 거부, 주변 순회 바깥 관계 검사 순서.
//  - 이월: C++ 의 실제 3×3 decoder/미리보기/finder/지형 통합 시험(`canon_permission_pattern_terrain_integration`)은 충돌/미리보기/배치 접두 모듈 이식 뒤 그 단위의 통합 시험에서 다룬다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E149: 일반 배치 권한 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, adler32 } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { RawCanonPlacementPermission, createCanonPlacementPermissionState, makeCanonPermissionTerrainHooks } from '../www/core/raw-canon-placement-permission.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-09 WB04-E149)
const allRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'canonpermission-x86.tsv')) ?? [];
const rows = allRows.filter((row) => row[0] === 'originals');

// 판본 행 수, 전체 행 수(세 PE), 풀 크기, 후보 타입 번호이다. (2026-11-09 WB04-E149)
const PATCH_ROWS = 5850;
const CD_ROWS = 5520;
const CAPACITY = 24000;
const OTHER = 83;

// 예외가 나는지 본다. (2026-11-09 WB04-E149)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 16진 문자열로 바꾼다. (2026-11-09 WB04-E149)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

test('일반 배치 권한: 원본 10.78 관측 5850행의 반환·Player 조회 인자·raw·관계 표가 같다', { skip: rows.length === 0 }, () => {
    assert.equal(allRows.length, PATCH_ROWS + 2 * CD_ROWS);
    assert.equal(rows.length, PATCH_ROWS);
    const pool = new SidPool(CAPACITY, { server: false });
    const state = createCanonPlacementPermissionState();
    const counts = { C: 0, R: 0, mutated: 0 };
    // 이전 입력의 전역·콜백 결과·raw 바이트가 다음 입력에 남지 않게 초기화한다.
    for (const row of rows) {
        assert.equal(row.length, 25);
        state.graphReady = Number(row[2]) >>> 0;
        state.editor = Number(row[3]) >>> 0;
        state.useAlliances = Number(row[4]) >>> 0;
        state.allowOtherOwners = Number(row[5]) >>> 0;
        state.alliances.fill(0);
        const current = Number(row[6]) >>> 0;
        const owner = Number(row[7]) >>> 0;
        const sid = Number(row[11]);
        const forward = ((current * 9) + owner) >>> 0;
        const reverse = ((owner * 9) + current) >>> 0;
        if (forward < state.alliances.length) state.alliances[forward] = Number(row[8]);
        if (reverse < state.alliances.length && reverse !== forward) state.alliances[reverse] = Number(row[9]);
        const raw = pool.physical(sid < 128 ? sid : 50);
        raw.fill(0xcd);
        raw[34] = current & 0xff;
        raw[10] = Number(row[12]) & 0xff;
        put(raw, 14, Number(row[13]));
        put(raw, 18, Number(row[14]));
        let calls = 0;
        let args = [0, 0, 0, 0];
        const permission = new RawCanonPlacementPermission(pool, state, (player, type, x, y, xBits, yBits) => {
            calls += 1;
            args = [player, type, xBits, yBits];
            // 부동소수 값 자체는 비트와 같은 수(NaN 은 NaN)여야 한다.
            assert.ok(Object.is(x, new DataView(new Uint32Array([xBits]).buffer).getFloat32(0, true)) && Object.is(y, new DataView(new Uint32Array([yBits]).buffer).getFloat32(0, true)));
            if (Number(row[15])) {
                raw[34] = 3;
                raw[10] = 84;
                put(raw, 14, 0x41f00000);
                put(raw, 18, 0x41f80000);
                state.editor ^= 1;
                counts.mutated += 1;
            }
            return Number(row[10]);
        });
        let result;
        if (row[1] === 'R') { result = permission.related(current, owner) ? 1 : 0; counts.R += 1; }
        else { result = permission.candidate(sid, owner); counts.C += 1; }
        const relationBytes = new Uint8Array(324);
        // 기계어 실행기의 리틀엔디언 관계 표 checksum 과 같은 바이트 순서로 직렬화한다.
        for (let index = 0; index < state.alliances.length; index += 1) put(relationBytes, 4 * index, state.alliances[index]);
        const where = row.slice(0, 22).join(' ');
        assert.equal(result, Number(row[16]), where);
        assert.equal(calls, Number(row[17]), where);
        assert.deepEqual(args, [18, 19, 20, 21].map((index) => Number(row[index])), where);
        assert.equal(hex(raw), row[22], where);
        assert.equal(state.editor >>> 0, Number(row[23]), where);
        assert.equal(adler32(relationBytes), Number(row[24]), where);
    }
    // 두 종류와 조회 중 변경 입력이 실제로 실행되었는지 확인한다.
    assert.ok(counts.C > 0 && counts.R > 0 && counts.mutated > 0, JSON.stringify(counts));
});

test('일반 배치 권한: 요청 DWORD 를 유지하고 지형 훅에는 부호 있는 BYTE 소유자를 전달하며 현재 raw 를 매번 다시 읽는다', () => {
    const pool = new SidPool(CAPACITY, { server: true });
    const sid = pool.allocate();
    const raw = pool.allocated(sid);
    raw[34] = 1;
    raw[10] = OTHER;
    put(raw, 14, 0x80000000);
    put(raw, 18, 0x7fc12345);
    const state = createCanonPlacementPermissionState();
    state.graphReady = 1;
    state.useAlliances = 1;
    state.alliances[8] = 1;
    let calls = 0;
    const permission = new RawCanonPlacementPermission(pool, state, (owner, type, x, y, xBits, yBits) => {
        calls += 1;
        // NaN 페이로드까지 같은 비트가 전달된다.
        assert.ok(owner === 0xffffffff && type === OTHER && xBits === 0x80000000 && yBits === 0x7fc12345 && Object.is(x, -0) && Number.isNaN(y));
        raw[34] = 2;
        state.useAlliances = 0;
        return 0x80000000;
    });
    const hooks = makeCanonPermissionTerrainHooks(pool, permission, {});
    const query = { type: 82, argument: 25, x: 20, y: 21, flags: 0, owner: 0xabff, mode: 7 };
    assert.equal(hooks.candidatePermission(query, sid), true);
    assert.ok(query.owner === 0xabff && calls === 1);
    // 두 번째는 현재 raw 소유자가 2 로 바뀌어 관계가 끊겨 Player 조회 없이 거부한다.
    assert.equal(hooks.candidatePermission(query, sid), false);
    assert.equal(calls, 1);
    state.graphReady = 0;
    assert.equal(permission.candidate(65535, 1), 0);
    state.graphReady = 1;
    assert.equal(permission.candidate(65535, 0), 1);
    assert.equal(calls, 1);
    // 기존 훅은 유지한다.
    const kept = makeCanonPermissionTerrainHooks(pool, permission, { shapePermission: 7 });
    assert.equal(kept.shapePermission, 7);
});

test('일반 배치 권한: 주변 순회의 바깥 관계 검사는 중립에도 적용되며 다른 소유자 허용은 마지막에 읽는다', () => {
    const pool = new SidPool(CAPACITY, { server: true });
    const sid = pool.allocate();
    const raw = pool.allocated(sid);
    raw[10] = OTHER;
    let calls = 0;
    const state = createCanonPlacementPermissionState();
    state.graphReady = 1;
    const permission = new RawCanonPlacementPermission(pool, state, () => { calls += 1; return 5; });
    // 중립(0) 후보도 바깥 관계가 없으면 거부된다.
    raw[34] = 0;
    assert.equal(permission.surrounding(sid, 1), 0);
    assert.equal(calls, 0);
    // 다른 소유자 허용이 켜지면 바깥 검사는 통과하고 후보 검사가 이어져 Player 조회가 불린다.
    state.allowOtherOwners = 1;
    assert.equal(permission.surrounding(sid, 1), 5);
    assert.equal(calls, 1);
    // 같은 소유자는 항상 관계가 있다.
    state.allowOtherOwners = 0;
    raw[34] = 1;
    assert.equal(permission.surrounding(sid, 1), 5);
    // 그래프 비활성이면 후보 검사가 0 을 돌려준다.
    state.graphReady = 0;
    assert.equal(permission.surrounding(sid, 1), 0);
});

test('일반 배치 권한: 필수 Player 경계·관계 표 범위·다른 풀을 거부한다', () => {
    const pool = new SidPool(CAPACITY, { server: true });
    const other = new SidPool(CAPACITY, { server: true });
    const state = createCanonPlacementPermissionState();
    assert.ok(throws(() => new RawCanonPlacementPermission(pool, state, null)));
    assert.ok(throws(() => new RawCanonPlacementPermission(pool, { ...state, alliances: new Uint32Array(3) }, () => 0)));
    const permission = new RawCanonPlacementPermission(pool, state, () => { assert.fail('조회하지 않는다'); });
    assert.ok(throws(() => makeCanonPermissionTerrainHooks(other, permission, {})));
    const sid = pool.allocate();
    const raw = pool.allocated(sid);
    raw[34] = 255;
    state.graphReady = state.useAlliances = state.allowOtherOwners = 1;
    assert.ok(throws(() => permission.candidate(sid, 128)));
    assert.ok(throws(() => permission.related(255, 128)));
    // 인덱스 산술은 DWORD 로 감기므로 1×9+0xffffffff 는 8 번 칸을 읽고, 감기지 않는 표 밖 번호는 진단한다.
    state.alliances[8] = 1;
    assert.equal(permission.alliance(1, 0xffffffff), true);
    assert.ok(throws(() => permission.alliance(1, 72)));
    state.editor = 1;
    assert.equal(permission.related(255, 128), true);
});
